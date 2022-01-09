/*
 * Copyright 2021 Sebastian
 * Copyright 2021 NSG650
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

#include "scheduler.h"
#include "../cpu/apic.h"
#include "../cpu/cpu.h"
#include "../kernel/panic.h"
#include "../klibc/lock.h"
#include "../klibc/printf.h"

lock_t sched_lock;

extern void context_switch(struct cpu_context **old, struct cpu_context *new);

void sched_init(void) {
	while (1) {
		asm volatile("sti");
		LOCK(sched_lock);
		// Primitive priority system
		struct process *toproc = kmalloc(sizeof(struct process));
		struct thread *topthrd = kmalloc(sizeof(struct thread));
		for (int i = 0; i < ptable.length; i++) {
			struct process *proc = ptable.data[i];
			if (proc->state != READY)
				continue;
			for (int j = 0; j < proc->ttable.length; j++) {
				if (proc->ttable.data[j]->state_t != READY)
					continue;
				if (proc->priority >= toproc->priority) {
					topthrd = proc->ttable.data[j];
					toproc = proc;
				}
			}
		}

		size_t next_sched_tick = timer_tick + toproc->timeslice;
		while (timer_tick < next_sched_tick && toproc->state == READY) {
			this_cpu->cpu_state->running_proc = toproc;
			this_cpu->cpu_state->running_thrd = topthrd;
			toproc->state = RUNNING;
			topthrd->state_t = RUNNING;
			if (toproc->process_pagemap == NULL)
				PANIC("running process does not have process pagemap");
			context_switch(&this_cpu->cpu_state->scheduler, topthrd->context);

			this_cpu->cpu_state->running_proc = NULL;
			this_cpu->cpu_state->running_thrd = NULL;
		}
		// Free memory used for top-most process
		kfree(toproc);
		kfree(topthrd);
		UNLOCK(sched_lock);
	}
}

inline struct process *running_proc(void) {
	asm volatile("cli");
	struct process *proc = this_cpu->cpu_state->running_proc;
	asm volatile("sti");
	return proc;
}

inline struct thread *running_thrd(void) {
	asm volatile("cli");
	struct thread *thrd = this_cpu->cpu_state->running_thrd;
	asm volatile("sti");
	return thrd;
}

void yield_to_scheduler(void) {
	asm volatile("cli");
	struct thread *thrd = running_thrd();
	if (thrd->state_t != RUNNING) {
		context_switch(&thrd->context, this_cpu->cpu_state->scheduler);
	}
}
