/*
 * Copyright 2021, 2022 NSG650
 * Copyright 2021, 2022 Sebastian
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

bool spinlock_acquire(lock_t spin) {
	return !__atomic_test_and_set(&spin, __ATOMIC_ACQUIRE);
}

void spinlock_acquire_or_wait(lock_t spin) {
retry_lock:
	if (spinlock_acquire(spin)) {
		return;
	}

	// Do a rough wait until the lock is free for cache-locality
	for (;;) {
		if (__atomic_load_n(&spin, __ATOMIC_RELAXED) == 0) {
			goto retry_lock;
		}
		pause();
	}
}

void spinlock_drop(lock_t spin) {
	__atomic_clear(&spin, __ATOMIC_RELEASE);
}

// liballoc
lock_t lib_lock;

int liballoc_lock() {
	spinlock_acquire(lib_lock);
	cli();
	return 0;
}

int liballoc_unlock() {
	spinlock_drop(lib_lock);
	sti();
	return 0;
}
