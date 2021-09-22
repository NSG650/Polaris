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

#include "vmm.h"
#include "../klibc/alloc.h"
#include "../klibc/math.h"
#include "pmm.h"

struct pagemap *kernel_pagemap = NULL;

void vmm_init(struct stivale2_mmap_entry *memmap, size_t memmap_entries,
			  struct stivale2_pmr *pmrs, size_t pmr_entries) {
	kernel_pagemap = vmm_new_pagemap();
	for (uint64_t p = 0; p < 4096UL * 1024 * 1024; p += 0x200000) {
		vmm_map_page(kernel_pagemap, p, p, 0b11 | 1UL << 63, true);
		vmm_map_page(kernel_pagemap, p + MEM_PHYS_OFFSET, p, 0b11 | 1UL << 63,
					 true);
	}
	for (size_t i = 0; i < pmr_entries; i++) {
		uint64_t virt = pmrs[i].base;
		uint64_t phys = virt - KERNEL_BASE;
		uint64_t pf =
			(pmrs[i].permissions & STIVALE2_PMR_EXECUTABLE ? 0 : 1UL << 63) |
			(pmrs[i].permissions & STIVALE2_PMR_WRITABLE ? 1 << 1 : 0) | 1;
		for (uint64_t p = 0; p < pmrs[i].length; p += PAGE_SIZE) {
			vmm_map_page(kernel_pagemap, virt + p, phys + p, pf, false);
		}
	}
	for (size_t i = 0; i < memmap_entries; i++) {
		uint64_t aligned_base = ALIGN_DOWN(memmap[i].base, 0x200000);
		uint64_t aligned_top =
			ALIGN_UP(memmap[i].base + memmap[i].length, 0x200000);
		uint64_t aligned_length = aligned_top - aligned_base;

		for (uint64_t p = 0; p < aligned_length; p += 0x200000) {
			uint64_t page = aligned_base + p;
			vmm_map_page(kernel_pagemap, page, page, 0b11 | 1UL << 63, true);
			vmm_map_page(kernel_pagemap, MEM_PHYS_OFFSET + page, page,
						 0b11 | 1UL << 63, true);
		}
	}
	vmm_switch_pagemap(kernel_pagemap);
}

void vmm_switch_pagemap(struct pagemap *pagemap) {
	asm volatile("mov cr3, %0" : : "r"(pagemap->top_level) : "memory");
}

struct pagemap *vmm_new_pagemap(void) {
	struct pagemap *pagemap = alloc(sizeof(struct pagemap));
	pagemap->top_level = pmm_allocz(1);
	return pagemap;
}

static uint64_t *get_next_level(uint64_t *current_level, size_t entry) {
	uint64_t *ret;
	if (current_level[entry] & 1) {
		// Present flag set
		ret = (uint64_t *)(size_t)(current_level[entry] & ~((uint64_t)0xFFF));
	} else {
		// Allocate a table for the next level
		ret = pmm_allocz(1);
		// Present + writable + user (0b111)
		current_level[entry] = (size_t)ret | 0b111;
	}

	return ret;
}

bool vmm_map_page(struct pagemap *pagemap, uint64_t virt_addr,
				  uint64_t phys_addr, uint64_t flags, bool hugepages) {
	size_t pml4_entry = (virt_addr & ((uint64_t)0x1FF << 39)) >> 39;
	size_t pml3_entry = (virt_addr & ((uint64_t)0x1FF << 30)) >> 30;
	size_t pml2_entry = (virt_addr & ((uint64_t)0x1FF << 21)) >> 21;
	size_t pml1_entry = (virt_addr & ((uint64_t)0x1FF << 12)) >> 12;

	uint64_t *pml4, *pml3, *pml2, *pml1;

	pml4 = pagemap->top_level;
	pml3 = get_next_level(pml4, pml4_entry);
	if (pml3 == NULL)
		return false;
	pml2 = get_next_level(pml3, pml3_entry);
	if (pml2 == NULL)
		return false;

	if (hugepages) {
		pml2[pml2_entry] = phys_addr | flags | (1 << 7);
		return true;
	}

	pml1 = get_next_level(pml2, pml2_entry);
	if (pml1 == NULL)
		return false;

	pml1[pml1_entry] = phys_addr | flags;
	return true;
}
