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

#include <klibc/mem.h>
#include <klibc/misc.h>
#include <locks/spinlock.h>
#include <mm/pmm.h>
#include <mm/vmm.h>

static lock_t memory_lock = 0;
static uint8_t *bitmap = NULL;
static uint64_t total_page_count = 0;
static uint64_t last_used_index = 0;
static uint64_t free_pages = 0;

void pmm_init(struct limine_memmap_entry **memmap, size_t memmap_entries) {
	uint64_t highest_addr = 0;

	// First, calculate how big the bitmap needs to be
	for (size_t i = 0; i < memmap_entries; i++) {
		if (memmap[i]->type != LIMINE_MEMMAP_USABLE &&
			memmap[i]->type != LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
			continue;

		uint64_t top = memmap[i]->base + memmap[i]->length;
		if (top > highest_addr)
			highest_addr = top;
	}

	total_page_count = highest_addr / PAGE_SIZE;
	size_t bitmap_size = ALIGN_UP(total_page_count / 8, PAGE_SIZE);

	// Second, find a location with enough free pages to host the bitmap
	for (size_t i = 0; i < memmap_entries; i++) {
		if (memmap[i]->type != LIMINE_MEMMAP_USABLE)
			continue;

		if (memmap[i]->length >= bitmap_size) {
			bitmap = (void *)(memmap[i]->base + MEM_PHYS_OFFSET);

			// Initialise entire bitmap to 1 (non-free)
			memset(bitmap, 0xFF, bitmap_size);

			memmap[i]->length -= bitmap_size;
			memmap[i]->base += bitmap_size;

			break;
		}
	}

	// Third, populate free bitmap entries according to memory map
	for (size_t i = 0; i < memmap_entries; i++) {
		if (memmap[i]->type != LIMINE_MEMMAP_USABLE)
			continue;

		for (uint64_t j = 0; j < memmap[i]->length; j += PAGE_SIZE) {
			bitmap_reset(bitmap, (memmap[i]->base + j) / PAGE_SIZE);
			free_pages++;
		}
	}
}

static void *inner_alloc(size_t pages, size_t limit) {
	size_t p = 0;

	while (last_used_index < limit) {
		if (!bitmap_test(bitmap, last_used_index++)) {
			if (++p == pages) {
				size_t page = last_used_index - pages;
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

void *pmm_alloc(size_t pages) {
	spinlock_acquire_or_wait(memory_lock);
	size_t last = last_used_index;
	void *ret = inner_alloc(pages, total_page_count);
	if (ret == NULL) {
		last_used_index = 0;
		ret = inner_alloc(pages, last);
	}
	free_pages -= pages;
	spinlock_drop(memory_lock);
	return ret;
}

void *pmm_allocz(size_t pages) {
	void *ret = pmm_alloc(pages);

	if (ret != NULL)
		memset(ret + MEM_PHYS_OFFSET, 0, pages * PAGE_SIZE);

	return ret;
}

void pmm_free(void *addr, size_t pages) {
	spinlock_acquire_or_wait(memory_lock);
	size_t page = (size_t)addr / PAGE_SIZE;
	for (size_t i = page; i < page + pages; i++)
		bitmap_reset(bitmap, i);
	spinlock_drop(memory_lock);
}
