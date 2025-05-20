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
#include <klibc/kargs.h>
#include <klibc/mem.h>
#include <locks/spinlock.h>
#include <stddef.h>
#include <sys/prcb.h>

static void *last_addr = NULL;

extern bool sched_runit;
extern bool is_smp;

static void spinlock_spinning_for_too_long(lock_t *spin) {
	panic("Deadlocked at %p. Last owner %p\n", last_addr, spin->last_owner);
}

bool spinlock_acquire(lock_t *spin) {
	if (!spin)
		return false;
	bool ret = __sync_bool_compare_and_swap(&spin->lock, 0, 1);
	if (ret)
		spin->last_owner = __builtin_return_address(0);
	return ret;
}

void spinlock_acquire_or_wait(lock_t *spin) {
	if (!spin)
		return;
	volatile size_t deadlock_counter = 0;
	last_addr = __builtin_return_address(0);
	for (;;) {
		if (spinlock_acquire(spin))
			break;
		if ((kernel_arguments.kernel_args & KERNEL_ARGS_PANIC_ON_DEADLOCK)) {
			if (++deadlock_counter >= 100000000)
				spinlock_spinning_for_too_long(spin);
		}
		pause();
	}
	spin->last_owner = __builtin_return_address(0);
}

void spinlock_drop(lock_t *spin) {
	if (!spin)
		return;
	__atomic_store_n(&spin->lock, 0, __ATOMIC_SEQ_CST);
}
