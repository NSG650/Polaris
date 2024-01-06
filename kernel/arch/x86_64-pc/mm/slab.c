#include <klibc/mem.h>
#include <klibc/misc.h>
#include <locks/spinlock.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <stdbool.h>
#include <stdint.h>

// Mostly taken from Lyre, big thanks to them
/* Copyright 2022 mintsuki and contributors.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

struct slab {
	lock_t lock;
	void **first_free;
	size_t ent_size;
};

struct slab_header {
	struct slab *slab;
};

struct alloc_metadata {
	size_t pages;
	size_t size;
};

static struct slab slabs[10] = {0};

static inline struct slab *slab_for(size_t size) {
	for (size_t i = 0; i < SIZEOF_ARRAY(slabs); i++) {
		struct slab *slab = &slabs[i];
		if (slab->ent_size >= size) {
			return slab;
		}
	}
	return NULL;
}

static void create_slab(struct slab *slab, size_t ent_size) {
	spinlock_init(slab->lock);
	slab->first_free = (void **)((uint64_t)pmm_alloc(1) + MEM_PHYS_OFFSET);
	slab->ent_size = ent_size;

	size_t header_offset = ALIGN_UP(sizeof(struct slab_header), ent_size);
	size_t available_size = PAGE_SIZE - header_offset;

	struct slab_header *slab_ptr = (struct slab_header *)slab->first_free;
	slab_ptr->slab = slab;
	slab->first_free = (void **)((void *)slab->first_free + header_offset);

	void **arr = (void **)slab->first_free;
	size_t max = available_size / ent_size - 1;
	size_t fact = ent_size / sizeof(void *);

	for (size_t i = 0; i < max; i++) {
		arr[i * fact] = &arr[(i + 1) * fact];
	}
	arr[max * fact] = NULL;
}

static void *alloc_from_slab(struct slab *slab) {
	spinlock_acquire_or_wait(&slab->lock);

	if (slab->first_free == NULL) {
		create_slab(slab, slab->ent_size);
	}

	void **old_free = slab->first_free;
	slab->first_free = *old_free;
	memset(old_free, 0, slab->ent_size);

	spinlock_drop(&slab->lock);
	return old_free;
}

static void free_in_slab(struct slab *slab, void *addr) {
	spinlock_acquire_or_wait(&slab->lock);

	if (addr == NULL) {
		goto cleanup;
	}

	void **new_head = addr;
	*new_head = slab->first_free;
	slab->first_free = new_head;

cleanup:
	spinlock_drop(&slab->lock);
}

void slab_init(void) {
	memzero(slabs, sizeof(struct slab) * 10);
	create_slab(&slabs[0], 8);
	create_slab(&slabs[1], 16);
	create_slab(&slabs[2], 24);
	create_slab(&slabs[3], 32);
	create_slab(&slabs[4], 48);
	create_slab(&slabs[5], 64);
	create_slab(&slabs[6], 128);
	create_slab(&slabs[7], 256);
	create_slab(&slabs[8], 512);
	create_slab(&slabs[9], 1024);
}

void *slab_alloc(size_t size) {
	struct slab *slab = slab_for(size);
	if (slab != NULL) {
		return alloc_from_slab(slab);
	}

	size_t page_count = DIV_ROUNDUP(size, PAGE_SIZE);
	uint64_t ret = (uint64_t)pmm_allocz(page_count + 1);
	if ((void *)ret == NULL) {
		return NULL;
	}

	ret += MEM_PHYS_OFFSET;
	struct alloc_metadata *metadata = (struct alloc_metadata *)ret;

	metadata->pages = page_count;
	metadata->size = size;

	return (void *)(ret + PAGE_SIZE);
}

void *slab_realloc(void *addr, size_t new_size) {
	if (addr == NULL) {
		return slab_alloc(new_size);
	}

	if (((uintptr_t)addr & 0xfff) == 0) {
		struct alloc_metadata *metadata =
			(struct alloc_metadata *)(addr - PAGE_SIZE);
		if (DIV_ROUNDUP(metadata->size, PAGE_SIZE) ==
			DIV_ROUNDUP(new_size, PAGE_SIZE)) {
			metadata->size = new_size;
			return addr;
		}

		void *new_addr = slab_alloc(new_size);
		if (new_addr == NULL) {
			return NULL;
		}

		if (metadata->size > new_size) {
			memcpy(new_addr, addr, new_size);
		} else {
			memcpy(new_addr, addr, metadata->size);
		}

		slab_free(addr);
		return new_addr;
	}

	struct slab_header *slab_header =
		(struct slab_header *)((uintptr_t)addr & ~0xfff);
	struct slab *slab = slab_header->slab;

	if (new_size > slab->ent_size) {
		void *new_addr = slab_alloc(new_size);
		if (new_addr == NULL) {
			return NULL;
		}

		memcpy(new_addr, addr, slab->ent_size);
		free_in_slab(slab, addr);
		return new_addr;
	}

	return addr;
}

void slab_free(void *addr) {
	if (addr == NULL) {
		return;
	}

	if (((uintptr_t)addr & 0xfff) == 0) {
		struct alloc_metadata *metadata =
			(struct alloc_metadata *)(addr - PAGE_SIZE);
		pmm_free((void *)((uintptr_t)metadata - MEM_PHYS_OFFSET),
				 metadata->pages + 1);
		return;
	}

	struct slab_header *slab_header =
		(struct slab_header *)((uintptr_t)addr & ~0xfff);
	free_in_slab(slab_header->slab, addr);
}
