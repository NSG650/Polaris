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

#include <cpu/cr.h>
#include <cpu_features.h>
#include <cpuid.h>
#include <debug/debug.h>
#include <klibc/misc.h>
#include <mm/mmap.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <sys/prcb.h>

struct pagemap *kernel_pagemap = NULL;

extern char text_start_addr[], text_end_addr[];
extern char rodata_start_addr[], rodata_end_addr[];
extern char data_start_addr[], data_end_addr[];

volatile struct limine_hhdm_request hhdm_request = {.id = LIMINE_HHDM_REQUEST,
													.revision = 0};

static volatile struct limine_kernel_address_request kernel_address_request = {
	.id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0};

static volatile struct limine_5_level_paging_request five_level_paging_request =
	{.id = LIMINE_5_LEVEL_PAGING_REQUEST, .revision = 0};

static uint64_t *get_next_level(uint64_t *top_level, size_t idx,
								bool allocate) {
	if (top_level[idx] & 1) {
		return (uint64_t *)((size_t)(top_level[idx] & ~((uint64_t)0xFFF)) +
							MEM_PHYS_OFFSET);
	}

	if (!allocate) {
		return NULL;
	}

	void *next_level = pmm_allocz(1);
	top_level[idx] = (uint64_t)next_level | 0b111;

	return next_level + MEM_PHYS_OFFSET;
}

void vmm_init(struct limine_memmap_entry **memmap, size_t memmap_entries) {
	kernel_pagemap = kmalloc(sizeof(struct pagemap));
	kernel_pagemap->lock = 0;
	kernel_pagemap->top_level = pmm_allocz(1) + MEM_PHYS_OFFSET;

	for (uint64_t p = 256; p < 512; p++)
		get_next_level(kernel_pagemap->top_level, p, true);

	for (uint64_t p = 0; p < 0x200000; p += PAGE_SIZE) {
		if (p != 0) {
			vmm_map_page(kernel_pagemap, p, p, 0b111, Size4KiB);
		}
		vmm_map_page(kernel_pagemap, p + MEM_PHYS_OFFSET, p, 0b111, Size4KiB);
	}

	for (uint64_t p = 0x200000; p < 0x40000000; p += PAGE_SIZE) {
		vmm_map_page(kernel_pagemap, p, p, 0b111, Size4KiB);
		vmm_map_page(kernel_pagemap, p + MEM_PHYS_OFFSET, p, 0b111, Size4KiB);
	}

	for (uint64_t p = 0x40000000; p < 4096UL * 1024 * 1024; p += PAGE_SIZE) {
		vmm_map_page(kernel_pagemap, p, p, 0b111, Size4KiB);
		vmm_map_page(kernel_pagemap, p + MEM_PHYS_OFFSET, p, 0b111, Size4KiB);
	}

	for (size_t i = 0; i < memmap_entries; i++) {
		uint64_t base = memmap[i]->base;
		uint64_t length = memmap[i]->length;
		uint64_t top = base + length;

		if (base < 0x100000000)
			base = 0x100000000;

		if (base >= top)
			continue;

		uint64_t aligned_base = ALIGN_DOWN(base, PAGE_SIZE);
		uint64_t aligned_top = ALIGN_UP(top, PAGE_SIZE);
		uint64_t aligned_length = aligned_top - aligned_base;

		for (uint64_t j = 0; j < aligned_length; j += PAGE_SIZE) {
			uint64_t page = aligned_base + j;
			vmm_map_page(kernel_pagemap, page, page, 0b111, Size4KiB);
			vmm_map_page(kernel_pagemap, page + MEM_PHYS_OFFSET, page, 0b111,
						 Size4KiB);
		}
	}

	uintptr_t text_start = ALIGN_DOWN((uintptr_t)text_start_addr, PAGE_SIZE),
			  rodata_start =
				  ALIGN_DOWN((uintptr_t)rodata_start_addr, PAGE_SIZE),
			  data_start = ALIGN_DOWN((uintptr_t)data_start_addr, PAGE_SIZE),
			  text_end = ALIGN_UP((uintptr_t)text_end_addr, PAGE_SIZE),
			  rodata_end = ALIGN_UP((uintptr_t)rodata_end_addr, PAGE_SIZE),
			  data_end = ALIGN_UP((uintptr_t)data_end_addr, PAGE_SIZE);

	uint64_t paddr = kernel_address_request.response->physical_base;
	uint64_t vaddr = kernel_address_request.response->virtual_base;

	for (uintptr_t text_addr = text_start; text_addr < text_end;
		 text_addr += PAGE_SIZE) {
		uintptr_t phys = text_addr - vaddr + paddr;
		vmm_map_page(kernel_pagemap, text_addr, phys, 1, Size4KiB);
	}

	for (uintptr_t rodata_addr = rodata_start; rodata_addr < rodata_end;
		 rodata_addr += PAGE_SIZE) {
		uintptr_t phys = rodata_addr - vaddr + paddr;
		vmm_map_page(kernel_pagemap, rodata_addr, phys, 1 | 1ull << 63ull,
					 Size4KiB);
	}

	for (uintptr_t data_addr = data_start; data_addr < data_end;
		 data_addr += PAGE_SIZE) {
		uintptr_t phys = data_addr - vaddr + paddr;
		vmm_map_page(kernel_pagemap, data_addr, phys, 0b11 | 1ull << 63ull,
					 Size4KiB);
	}
	// Switch to the new page map, dropping Limine's default one
	vmm_switch_pagemap(kernel_pagemap);
}

void vmm_switch_pagemap(struct pagemap *pagemap) {
	asm volatile("mov cr3, %0"
				 :
				 : "r"((void *)pagemap->top_level - MEM_PHYS_OFFSET)
				 : "memory");
}

// Creates a new dynamically allocated page map
struct pagemap *vmm_new_pagemap(void) {
	struct pagemap *pagemap = kmalloc(sizeof(struct pagemap));
	pagemap->top_level = pmm_allocz(1) + MEM_PHYS_OFFSET;
	pagemap->lock = 0;
	for (size_t i = 256; i < 512; i++)
		pagemap->top_level[i] = kernel_pagemap->top_level[i];
	return pagemap;
}

bool vmm_map_page(struct pagemap *pagemap, uint64_t virt_addr,
				  uint64_t phys_addr, uint64_t flags, enum page_size pg_size) {
	spinlock_acquire_or_wait(pagemap->lock);

	// Calculate the indices in the various tables using the virtual address
	size_t pml5_entry = (virt_addr & ((uint64_t)0x1FF << 48)) >> 48;
	size_t pml4_entry = (virt_addr & ((uint64_t)0x1FF << 39)) >> 39;
	size_t pml3_entry = (virt_addr & ((uint64_t)0x1FF << 30)) >> 30;
	size_t pml2_entry = (virt_addr & ((uint64_t)0x1FF << 21)) >> 21;
	size_t pml1_entry = (virt_addr & ((uint64_t)0x1FF << 12)) >> 12;

	uint64_t *pml5, *pml4, *pml3, *pml2, *pml1;

	if (five_level_paging_request.response != NULL) {
		pml5 = pagemap->top_level;
		goto level5;
	} else {
		pml4 = pagemap->top_level;
		goto level4;
	}

level5:
	pml4 = get_next_level(pml5, pml5_entry, true);
	if (pml4 == NULL) {
		return false;
	}
level4:
	pml3 = get_next_level(pml4, pml4_entry, true);
	if (pml3 == NULL) {
		return false;
	}

	if (pg_size == Size1GiB) {
		uint32_t a = 0, b = 0, c = 0, d = 0;
		__get_cpuid(0x80000001, &a, &b, &c, &d);
		// Check for 1GiB pages support.
		if (d & CPUID_GBPAGE) {
			pml3[pml3_entry] = phys_addr | flags | (1 << 7);
			spinlock_drop(pagemap->lock);
			return true;
		} else {
			// If 1GiB pages are not supported then emulate it by splitting
			// them into 2MiB pages.
			for (uint64_t i = 0; i < 0x40000000; i += 0x200000) {
				vmm_map_page(pagemap, virt_addr + i, phys_addr + i, flags,
							 Size2MiB);
			}
		}
	}

	pml2 = get_next_level(pml3, pml3_entry, true);

	if (pml2 == NULL) {
		return false;
	}

	if (pg_size == Size2MiB) {
		pml2[pml2_entry] = phys_addr | flags | (1 << 7);
		spinlock_drop(pagemap->lock);
		return true;
	}

	pml1 = get_next_level(pml2, pml2_entry, true);

	if (pml1 == NULL) {
		return false;
	}

	pml1[pml1_entry] = phys_addr | flags;
	spinlock_drop(pagemap->lock);
	return true;
}

bool vmm_unmap_page(struct pagemap *pagemap, uintptr_t virt) {
	spinlock_acquire_or_wait(pagemap->lock);

	size_t pml5_entry = (virt & ((uint64_t)0x1FF << 48)) >> 48;
	size_t pml4_entry = (virt & ((uint64_t)0x1FF << 39)) >> 39;
	size_t pml3_entry = (virt & ((uint64_t)0x1FF << 30)) >> 30;
	size_t pml2_entry = (virt & ((uint64_t)0x1FF << 21)) >> 21;
	size_t pml1_entry = (virt & ((uint64_t)0x1FF << 12)) >> 12;

	uint64_t *pml5, *pml4, *pml3, *pml2, *pml1;

	if (five_level_paging_request.response != NULL) {
		pml5 = pagemap->top_level;
		goto level5;
	} else {
		pml4 = pagemap->top_level;
		goto level4;
	}

level5:
	pml4 = get_next_level(pml5, pml5_entry, false);
	if (pml4 == NULL) {
		spinlock_drop(pagemap->lock);
		return false;
	}
level4:
	pml3 = get_next_level(pml4, pml4_entry, false);
	if (pml3 == NULL) {
		spinlock_drop(pagemap->lock);
		return false;
	}

	pml2 = get_next_level(pml3, pml3_entry, false);
	if (pml2 == NULL) {
		spinlock_drop(pagemap->lock);
		return false;
	}

	pml1 = get_next_level(pml2, pml2_entry, false);
	if (pml1 == NULL) {
		spinlock_drop(pagemap->lock);
		return false;
	}

	if ((pml1[pml1_entry] & 1) == 0) {
		spinlock_drop(pagemap->lock);
		return false;
	}

	pml1[pml1_entry] = 0;

	asm volatile("invlpg [%0]" : : "r"(virt) : "memory");

	spinlock_drop(pagemap->lock);
	return true;
}

uint64_t *vmm_virt_to_pte(struct pagemap *pagemap, uintptr_t virt_addr,
						  bool allocate) {
	size_t pml5_entry = (virt_addr & ((uint64_t)0x1FF << 48)) >> 48;
	size_t pml4_entry = (virt_addr & ((uint64_t)0x1FF << 39)) >> 39;
	size_t pml3_entry = (virt_addr & ((uint64_t)0x1FF << 30)) >> 30;
	size_t pml2_entry = (virt_addr & ((uint64_t)0x1FF << 21)) >> 21;
	size_t pml1_entry = (virt_addr & ((uint64_t)0x1FF << 12)) >> 12;

	uint64_t *pml5, *pml4, *pml3, *pml2, *pml1;

	if (five_level_paging_request.response != NULL) {
		pml5 = pagemap->top_level;
		goto level5;
	} else {
		pml4 = pagemap->top_level;
		goto level4;
	}

level5:
	pml4 = get_next_level(pml5, pml5_entry, allocate);
	if (pml4 == NULL) {
		return NULL;
	}
level4:
	pml3 = get_next_level(pml4, pml4_entry, allocate);
	if (pml3 == NULL) {
		return NULL;
	}
	pml2 = get_next_level(pml3, pml3_entry, allocate);
	if (pml2 == NULL) {
		return NULL;
	}
	pml1 = get_next_level(pml2, pml2_entry, allocate);
	if (pml1 == NULL) {
		return NULL;
	}

	return &pml1[pml1_entry];
}

uint64_t vmm_virt_to_phys(struct pagemap *pagemap, uint64_t virt_addr) {
	uint64_t *pte = vmm_virt_to_pte(pagemap, virt_addr, false);
	if (pte == NULL || (((*pte) & ~0xffffffffff000) & 1) == 0)
		return INVALID_PHYS;

	return ((*pte) & 0xffffffffff000);
}

uint64_t vmm_virt_to_kernel(struct pagemap *pagemap, uint64_t virt_addr) {
	uint64_t aligned_virtual_address = ALIGN_DOWN(virt_addr, PAGE_SIZE);
	uint64_t phys_addr = vmm_virt_to_phys(pagemap, virt_addr);
	return (phys_addr + MEM_PHYS_OFFSET + virt_addr - aligned_virtual_address);
}

void vmm_page_fault_handler(registers_t *reg) {
	if (mmap_handle_pf(reg)) {
		return;
	}
	uint64_t faulting_address = read_cr("2");
	bool present = reg->errorCode & 0x1;
	bool read_write = reg->errorCode & 0x2;
	bool user_supervisor = reg->errorCode & 0x4;
	bool reserved = reg->errorCode & 0x8;
	bool execute = reg->errorCode & 0x10;
	if (reg->cs & 0x3) {
		struct thread *thrd = prcb_return_current_cpu()->running_thread;
		kprintf("Killing user thread tid %d under process %s for\n", thrd->tid,
				thrd->mother_proc->name);
		kprintf("User thread crashed at address: 0x%p\n", reg->rip);
		backtrace((void *)reg->rbp);
		kprintf("Page fault at 0x%p present: %s, read/write: %s, "
				"user/supervisor: %s, reserved: %s, execute: %s\n",
				faulting_address, present ? "P" : "NP", read_write ? "R" : "RW",
				user_supervisor ? "U" : "S", reserved ? "R" : "NR",
				execute ? "X" : "NX");
		thread_kill(thrd, 1);
	}
	panic_((void *)reg->rip, (void *)reg->rbp,
		   "Page fault at 0x%p present: %s, read/write: %s, "
		   "user/supervisor: %s, reserved: %s, execute: %s\n",
		   faulting_address, present ? "P" : "NP", read_write ? "R" : "RW",
		   user_supervisor ? "U" : "S", reserved ? "R" : "NR",
		   execute ? "X" : "NX");
}

struct pagemap *vmm_fork_pagemap(struct pagemap *pagemap) {
	spinlock_acquire_or_wait(pagemap->lock);

	struct pagemap *new_pagemap = vmm_new_pagemap();
	if (new_pagemap == NULL) {
		goto cleanup;
	}

	struct mmap_range_local *local_range = NULL;
	int idxn = 0;
	vec_foreach(&pagemap->mmap_ranges, local_range, idxn) {
		struct mmap_range_global *global_range = local_range->global;

		struct mmap_range_local *new_local_range =
			kmalloc(sizeof(struct mmap_range_local));
		if (new_local_range == NULL) {
			goto cleanup;
		}

		*new_local_range = *local_range;
		new_local_range->pagemap = new_pagemap;

		if (global_range->res != NULL) {
			global_range->res->refcount++;
		}

		if ((local_range->flags & MAP_SHARED) != 0) {
			vec_push(&global_range->locals, new_local_range);
			for (uintptr_t i = local_range->base;
				 i < local_range->base + local_range->length; i += PAGE_SIZE) {
				uint64_t *old_pte = vmm_virt_to_pte(pagemap, i, false);
				if (old_pte == NULL) {
					continue;
				}

				uint64_t *new_pte = vmm_virt_to_pte(new_pagemap, i, true);
				if (new_pte == NULL) {
					goto cleanup;
				}
				*new_pte = *old_pte;
			}
		} else {
			struct mmap_range_global *new_global_range =
				kmalloc(sizeof(struct mmap_range_global));
			if (new_global_range == NULL) {
				goto cleanup;
			}

			new_global_range->shadow_pagemap = vmm_new_pagemap();
			if (new_global_range->shadow_pagemap == NULL) {
				goto cleanup;
			}

			new_global_range->base = global_range->base;
			new_global_range->length = global_range->length;
			new_global_range->res = global_range->res;
			new_global_range->offset = global_range->offset;

			vec_push(&new_global_range->locals, new_local_range);

			if ((local_range->flags & MAP_ANONYMOUS) != 0) {
				for (uintptr_t i = local_range->base;
					 i < local_range->base + local_range->length;
					 i += PAGE_SIZE) {
					uint64_t *old_pte = vmm_virt_to_pte(pagemap, i, false);
					if (old_pte == NULL || (((*old_pte) & 0xfff) & 1) == 0) {
						continue;
					}

					uint64_t *new_pte = vmm_virt_to_pte(new_pagemap, i, true);
					if (new_pte == NULL) {
						goto cleanup;
					}

					uint64_t *new_spte = vmm_virt_to_pte(
						new_global_range->shadow_pagemap, i, true);
					if (new_spte == NULL) {
						goto cleanup;
					}

					void *old_page = (void *)((*old_pte) & 0xffffffffff000);
					void *page = pmm_alloc(1);

					if (page == NULL) {
						goto cleanup;
					}

					memcpy(page + MEM_PHYS_OFFSET, old_page + MEM_PHYS_OFFSET,
						   PAGE_SIZE);
					*new_pte = ((*old_pte) & 0xfff) | (uint64_t)page;
					*new_spte = *new_pte;
				}
			} else {
				panic("Non anon fork\n");
			}
		}

		vec_push(&new_pagemap->mmap_ranges, new_local_range);
	}

	spinlock_drop(pagemap->lock);
	return new_pagemap;

cleanup:
	spinlock_drop(pagemap->lock);
	if (new_pagemap != NULL) {
		vmm_destroy_pagemap(new_pagemap);
	}
	return NULL;
}

static void destroy_level(uint64_t *pml, size_t start, size_t end, int level) {
	if (level == 0) {
		return;
	}

	for (size_t i = start; i < end; i++) {
		uint64_t *next_level = get_next_level(pml, i, false);
		if (next_level == NULL) {
			continue;
		}

		destroy_level(next_level, 0, 512, level - 1);
	}

	pmm_free((void *)pml, 1);
}

void vmm_destroy_pagemap(struct pagemap *pagemap) {
	spinlock_acquire_or_wait(pagemap->lock);

	while (pagemap->mmap_ranges.length > 0) {
		struct mmap_range_local *local_range = pagemap->mmap_ranges.data[0];

		munmap(pagemap, local_range->base, local_range->length);
	}

	destroy_level(pagemap->top_level, 0, 256, 4);
	kfree(pagemap);
}
