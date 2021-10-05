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

#include "alloc.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "lock.h"
#include "math.h"

struct alloc_metadata {
	size_t pages;
	size_t size;
};

void *alloc(size_t size) {
	size_t page_count = DIV_ROUNDUP(size, PAGE_SIZE);

	void *ptr = (char *)pmm_allocz(page_count + 1);

	if (!ptr)
		return NULL;

	ptr += MEM_PHYS_OFFSET;

	struct alloc_metadata *metadata = ptr;
	ptr += PAGE_SIZE;

	metadata->pages = page_count;
	metadata->size = size;

	return ptr;
}

void free(void *ptr) {
	struct alloc_metadata *metadata = ptr - PAGE_SIZE;

	pmm_free((void *)metadata - MEM_PHYS_OFFSET, metadata->pages + 1);
}

void *liballoc_alloc(size_t size) {
	return pmm_allocz(size) + MEM_PHYS_OFFSET;
}

int liballoc_free(void *ptr, size_t size) {
	pmm_free(ptr - MEM_PHYS_OFFSET, size);
	return 0;
}

lock_t alloc_lock;

int liballoc_lock() {
	LOCK(alloc_lock);
	asm volatile("cli");
	return 0;
}

int liballoc_unlock() {
	UNLOCK(alloc_lock);
	asm volatile("sti");
	return 0;
}
