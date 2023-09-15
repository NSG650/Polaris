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
#include <locks/spinlock.h>

bool spinlock_acquire(lock_t *spin) {
	return __sync_bool_compare_and_swap(spin, 0, 1);
}

void spinlock_acquire_or_wait(lock_t *spin) {
	for (;;) {
		if (spinlock_acquire(spin))
			break;
		pause();
	}
}

void spinlock_drop(lock_t *spin) {
	__atomic_store_n(spin, 0, __ATOMIC_SEQ_CST);
}
