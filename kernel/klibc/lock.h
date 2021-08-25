#ifndef LOCK_H
#define LOCK_H

/*
 * Copyright 2021 NSG650
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

#define DECLARE_LOCK(name) volatile int name##Locked

#define LOCK(name)                                             \
	while (!__sync_bool_compare_and_swap(&name##Locked, 0, 1)) \
		;

#define UNLOCK(name) __sync_lock_release(&name##Locked);

#endif
