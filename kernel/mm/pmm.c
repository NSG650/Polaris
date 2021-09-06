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

#include "pmm.h"
#include "../klibc/bitman.h"
#include "../klibc/math.h"
#include "../klibc/mem.h"
#include "vmm.h"
#include <stivale2.h>

static void *bitmap;
static size_t last_used_index = 0;
static uintptr_t highest_page = 0;

void pmm_reclaim_memory(struct stivale2_mmap_entry *memmap,
						size_t memmap_entries) {
	for (size_t i = 0; i < memmap_entries; i++) {
		if (memmap[i].type != STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE)
			continue;
		pmm_free((void *)memmap[i].base, memmap[i].length / PAGE_SIZE);
	}
}

void pmm_init(struct stivale2_mmap_entry *memmap, size_t memmap_entries) {
	// First, calculate how big the bitmap needs to be.
	for (size_t i = 0; i < memmap_entries; i++) {
		if (memmap[i].type != STIVALE2_MMAP_USABLE &&
			memmap[i].type != STIVALE2_MMAP_BOOTLOADER_RECLAIMABLE &&
			memmap[i].type != STIVALE2_MMAP_KERNEL_AND_MODULES)
			continue;

		uintptr_t top = memmap[i].base + memmap[i].length;

		if (top > highest_page)
			highest_page = top;
	}

	size_t bitmap_size = DIV_ROUNDUP(highest_page, PAGE_SIZE) / 8;

	// Second, find a location with enough free pages to host the bitmap.
	for (size_t i = 0; i < memmap_entries; i++) {
		if (memmap[i].type != STIVALE2_MMAP_USABLE)
			continue;

		if (memmap[i].length >= bitmap_size) {
			bitmap = (void *)memmap[i].base + MEM_PHYS_OFFSET;

			// Initialise entire bitmap to 1 (non-free)
			memset(bitmap, 0xFF, bitmap_size);

			memmap[i].length -= bitmap_size;
			memmap[i].base += bitmap_size;

			break;
		}
	}

	// Third, populate free bitmap entries according to memory map.
	for (size_t i = 0; i < memmap_entries; i++) {
		if (memmap[i].type != STIVALE2_MMAP_USABLE)
			continue;

		for (uintptr_t j = 0; j < memmap[i].length; j += PAGE_SIZE)
			bitmap_unset(bitmap, (memmap[i].base + j) / PAGE_SIZE);
	}
}

static void *inner_alloc(size_t count, size_t limit) {
	size_t p = 0;

	while (last_used_index < limit) {
		if (!bitmap_test(bitmap, last_used_index++)) {
			if (++p == count) {
				size_t page = last_used_index - count;
				for (size_t i = page; i < last_used_index; i++) {
					bitmap_set(bitmap, i);
				}
				return (void *)(page * PAGE_SIZE);
			}
		} else {
			p = 0;
		}
	}

	return NULL;
}

void *pmm_alloc(size_t count) {
	size_t l = last_used_index;
	void *ret = inner_alloc(count, highest_page / PAGE_SIZE);
	if (ret == NULL) {
		last_used_index = 0;
		ret = inner_alloc(count, l);
	}
	return ret;
}

void *pmm_allocz(size_t count) {
	char *ret = (char *)pmm_alloc(count);

	if (ret == NULL)
		return NULL;

	uint64_t *ptr = (uint64_t *)(ret + MEM_PHYS_OFFSET);

	for (size_t i = 0; i < count * (PAGE_SIZE / sizeof(uint64_t)); i++)
		ptr[i] = 0;

	return ret;
}

void pmm_free(void *ptr, size_t count) {
	size_t page = (size_t)ptr / PAGE_SIZE;
	for (size_t i = page; i < page + count; i++)
		bitmap_unset(bitmap, i);
}
