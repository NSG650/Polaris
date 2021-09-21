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

#include "scheduler.h"
#include "../cpu/apic.h"
#include "../klibc/lock.h"

lock_t sched_lock;

struct cpu_state *cpu_state = {NULL};

extern void context_switch(struct process_context **old,
						   struct process_context *new);

void sched_init(void) {
	process_init((uintptr_t)__builtin_return_address(0));
	while (1) {
		asm volatile("sti");
		LOCK(sched_lock);
		struct process *proc;
		// Primitive priority system
		struct process *toproc;
		toproc->priority = LOW;
		for (proc = ptable; proc < &ptable[MAX_PROCS]; ++proc) {
			if (proc->state != READY)
				continue;

			if (proc->priority >= toproc->priority) {
				toproc = proc;
			}
		}

		size_t next_sched_tick = timer_tick + toproc->timeslice;
		while (timer_tick < next_sched_tick && toproc->state == READY) {
			cpu_state->running_proc = toproc;
			toproc->state = RUNNING;

			context_switch(&cpu_state->scheduler, toproc->context);

			cpu_state->running_proc = NULL;
		}

		UNLOCK(sched_lock);
	}
}

inline struct process *running_proc(void) {
	asm volatile("cli");
	struct process *proc = cpu_state->running_proc;
	asm volatile("sti");

	return proc;
}

void yield_to_scheduler(void) {
	asm volatile("cli");
	struct process *proc = running_proc();
	if (proc->state != RUNNING) {
		context_switch(&proc->context, cpu_state->scheduler);
	}
}
