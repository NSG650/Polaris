#ifndef BITMAN_H
#define BITMAN_H

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

#include "asm.h"
#include <stdbool.h>
#include <stddef.h>

static inline bool bitmap_test(void *bitmap, size_t bit) {
	bool ret;
	asm volatile("bt %1, %2"
				 : "=@ccc"(ret)
				 : "m"(FLAT_PTR(bitmap)), "r"(bit)
				 : "memory");
	return ret;
}

static inline bool bitmap_set(void *bitmap, size_t bit) {
	bool ret;
	asm volatile("bts %1, %2"
				 : "=@ccc"(ret), "+m"(FLAT_PTR(bitmap))
				 : "r"(bit)
				 : "memory");
	return ret;
}

static inline bool bitmap_unset(void *bitmap, size_t bit) {
	bool ret;
	asm volatile("btr %1, %2"
				 : "=@ccc"(ret), "+m"(FLAT_PTR(bitmap))
				 : "r"(bit)
				 : "memory");
	return ret;
}

#endif
