#ifndef PMM_H
#define PMM_H

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
#include <limine.h>
#include <stdbool.h>
#include <stddef.h>

static inline bool bitmap_test(void *bitmap, size_t bit) {
	uint8_t *bitmap_u8 = bitmap;
	return bitmap_u8[bit / 8] & (1 << (bit % 8));
}

static inline void bitmap_set(void *bitmap, size_t bit) {
	uint8_t *bitmap_u8 = bitmap;
	bitmap_u8[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_reset(void *bitmap, size_t bit) {
	uint8_t *bitmap_u8 = bitmap;
	bitmap_u8[bit / 8] &= ~(1 << (bit % 8));
}

#define DIV_ROUNDUP(A, B)        \
	({                           \
		typeof(A) _a_ = A;       \
		typeof(B) _b_ = B;       \
		(_a_ + (_b_ - 1)) / _b_; \
	})

#define ALIGN_UP(A, B)                  \
	({                                  \
		typeof(A) _a__ = A;             \
		typeof(B) _b__ = B;             \
		DIV_ROUNDUP(_a__, _b__) * _b__; \
	})

#define ALIGN_DOWN(A, B)   \
	({                     \
		typeof(A) _a_ = A; \
		typeof(B) _b_ = B; \
		(_a_ / _b_) * _b_; \
	})

void *pmm_alloc(size_t pages);
void *pmm_allocz(size_t pages);
void pmm_free(void *addr, size_t pages);
void pmm_init(struct limine_memmap_entry **memmap, size_t memmap_entries);

#endif
