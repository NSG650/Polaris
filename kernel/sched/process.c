/*
 * Copyright 2021 Sebastian
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "process.h"
#include "../cpu/apic.h"
#include "../kernel/panic.h"
#include "../klibc/lock.h"
#include "../klibc/printf.h"
#include "../klibc/string.h"
#include "scheduler.h"

process_vec_t ptable;
struct process *initproc;
static uint32_t next_pid = 1;
static bool is_init = true;
lock_t process_lock;

static struct process *alloc_new_process(void) {
	struct process *proc = kmalloc(sizeof(struct process));

	LOCK(process_lock);

	proc->kstack = kmalloc(KSTACK_SIZE);
	if (!proc->kstack) {
		PANIC("Failed to allocate kernel stack page");
		__builtin_unreachable();
	}

	proc->state = INITIAL;
	proc->block_on = NOTHING;
	proc->priority = NORMAL;
	proc->pid = next_pid++;
	proc->target_tick = 0;

	UNLOCK(process_lock);

	uint64_t sp = (uintptr_t)proc->kstack + KSTACK_SIZE;

	sp -= sizeof(struct process_context);
	proc->context = (struct process_context *)sp;
	memset(proc->context, 0, sizeof(struct process_context));

	vec_push(&ptable, proc);

	return proc;
}

void process_create(char *name, uintptr_t addr, uint64_t args,
					enum process_priority priority) {
	struct process *proc = alloc_new_process();
	strncpy(proc->name, name, 128);
	proc->parent = NULL;
	proc->context->rip = addr;
	proc->context->rdi = args;
	proc->timeslice = 1;
	proc->killed = false;
	proc->priority = priority;
	LOCK(process_lock);
	proc->state = READY;
	UNLOCK(process_lock);
}

void process_init(uintptr_t addr, uint64_t args) {
	if (!is_init)
		return;
	struct process *proc = alloc_new_process();
	strcpy(proc->name, "init");
	proc->parent = NULL;
	proc->context->rip = addr;
	proc->context->rdi = args;
	proc->timeslice = 1;
	proc->killed = false;
	proc->priority = HIGH;
	LOCK(process_lock);
	proc->state = READY;
	UNLOCK(process_lock);
	initproc = proc;
	is_init = false;
}

uint32_t process_fork(uint8_t timeslice) {
	if (timeslice < 1 || timeslice > 16)
		return -1;

	struct process *parent = running_proc();

	struct process *child = alloc_new_process();
	if (!child) {
		PANIC("Failed to allocate new child process");
		__builtin_unreachable();
	}

	child->timeslice = timeslice;

	child->context->rip = (uintptr_t)__builtin_return_address(0);
	child->context->rax = 0;

	child->parent = parent;
	strcpy(child->name, parent->name);

	child->killed = false;
	child->priority = parent->priority;

	LOCK(process_lock);
	child->state = READY;
	UNLOCK(process_lock);

	return child->pid;
}

inline void process_block(enum process_block_on reason) {
	asm volatile("cli");
	struct process *proc = running_proc();

	proc->block_on = reason;
	proc->state = BLOCKED;

	yield_to_scheduler();
}

inline void process_unblock(struct process *proc) {
	asm volatile("cli");
	proc->block_on = NOTHING;
	proc->state = READY;
}

void process_exit(void) {
	struct process *proc = running_proc();
	if (proc == initproc) {
		PANIC("ATTEMPTED TO KILL INIT!");
	}

	if (proc->parent->state == BLOCKED && proc->parent->block_on == ON_WAIT)
		process_unblock(proc->parent);

	for (int i = 0; i < ptable.length; i++) {
		struct process *child = ptable.data[i];
		if (child->parent == proc) {
			child->parent = initproc;
			if (child->state == TERMINATED)
				process_unblock(initproc);
		}
	}

	proc->state = TERMINATED;
	yield_to_scheduler();
}

void process_sleep(size_t sleep_ticks) {
	struct process *proc = running_proc();

	size_t curr_tick = timer_tick;
	size_t target_tick = curr_tick + sleep_ticks;
	proc->target_tick = target_tick;

	LOCK(process_lock);
	process_block(ON_SLEEP);
	UNLOCK(process_lock);
}

uint32_t process_wait(void) {
	struct process *proc = running_proc();

	LOCK(process_lock);

	while (1) {
		bool have_kids = false;

		for (int i = 0; i < ptable.length; i++) {
			struct process *child = ptable.data[i];
			if (child->parent != proc)
				continue;

			have_kids = true;

			if (child->state == TERMINATED) {
				kfree(child->kstack);

				uint32_t child_pid = child->pid;
				child->pid = 0;
				child->parent = NULL;
				child->name[0] = '\0';
				child->state = UNUSED;

				UNLOCK(process_lock);
				return child_pid;
			}
		}

		if (!have_kids || proc->killed) {
			UNLOCK(process_lock);
			return -1;
		}

		process_block(ON_WAIT);
	}
}

int32_t process_kill(uint32_t pid) {
	LOCK(process_lock);

	for (int i = 0; i < ptable.length; i++) {
		struct process *proc = ptable.data[i];
		if (proc->pid == pid) {
			proc->killed = true;

			if (proc->state == BLOCKED)
				process_unblock(proc);

			UNLOCK(process_lock);
			return 0;
		}
	}

	UNLOCK(process_lock);
	return -1;
}
