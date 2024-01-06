/*
 * Copyright 2021 - 2023 NSG650
 * Copyright 2021 - 2023 Neptune
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

#include <asm/asm.h>
#include <debug/debug.h>
#include <klibc/mem.h>
#include <locks/spinlock.h>
#include <stddef.h>
#include <sys/prcb.h>

static void *last_addr = NULL;

extern bool sched_runit;
extern bool is_smp;

static void spinlock_spinning_for_too_long(lock_t *spin) {
	kputs_("\n\nPossible deadlock? Last owner: 0x");
	char string[20] = {0};
	ultoa((uintptr_t)spin->last_owner, string, 16);
	kputs_(string);
	kputs_(" Last CPU: ");
	memzero(string, 20);
	ultoa((uintptr_t)spin->last_cpu, string, 10);
	kputs_(string);
	kputs_(" deadlocked at: 0x");
	memzero(string, 20);
	ultoa((uintptr_t)last_addr, string, 16);
	kputs_(string);
	kputs_("\n");
	//	kprintf("Possible deadlock? Last owner: 0x%p Last CPU: %u deadlocked at:
	// 0x%p\n", spin->last_owner, spin->last_cpu, last_addr);

	if (sched_runit) {
		sched_resched_now();
	}
}

bool spinlock_acquire(lock_t *spin) {
    if (!spin) return false;
	bool ret = __sync_bool_compare_and_swap(&spin->lock, 0, 1);
	if (ret)
		spin->last_owner = __builtin_return_address(0);
	return ret;
}

void spinlock_acquire_or_wait(lock_t *spin) {
    if (!spin) return;
	volatile size_t deadlock_counter = 0;
	last_addr = __builtin_return_address(0);
	for (;;) {
		if (spinlock_acquire(spin))
			break;
		if (++deadlock_counter >= 100000000)
			spinlock_spinning_for_too_long(spin);
		pause();
	}
	spin->last_owner = __builtin_return_address(0);
	if (is_smp)
		spin->last_cpu = prcb_return_current_cpu()->cpu_number;
}

void spinlock_drop(lock_t *spin) {
    if (!spin) return;
	__atomic_store_n(&spin->lock, 0, __ATOMIC_SEQ_CST);
}
