#ifndef VMM_H
#define VMM_H

/*
 * Copyright 2021 - 2023 NSG650
 * Copyright 2021 - 2023 Neptune
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

#include <limine.h>
#include <locks/spinlock.h>
#include <mm/mmap.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/isr.h>

extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_kernel_address_request kernel_address_request;

#define PAGE_SIZE ((size_t)4096)
#define MEM_PHYS_OFFSET (hhdm_request.response->offset)
#define KERNEL_BASE (kernel_address_request.response->virtual_base)
#define INVALID_PHYS ((uint64_t)0xffffffffffffffff)

#define PAGE_READ (0b1)
#define PAGE_WRITE (0b10)
#define PAGE_USER (0b100)
#define PAGE_NO_EXECUTE (1ull << 63ull)

struct pagemap {
	lock_t lock;
	uint64_t *top_level;
	vec_t(struct mmap_range_local *) mmap_ranges;
};

extern struct pagemap *kernel_pagemap;

enum page_size { Size4KiB, Size2MiB, Size1GiB };

void vmm_init(struct limine_memmap_entry **memmap, size_t memmap_entries);
void vmm_switch_pagemap(struct pagemap *pagemap);
struct pagemap *vmm_new_pagemap(void);
bool vmm_map_page(struct pagemap *pagemap, uint64_t virt_addr,
				  uint64_t phys_addr, uint64_t flags, enum page_size pg_size);
bool vmm_remap_page(struct pagemap *pagemap, uintptr_t virt, uint64_t flags,
					bool locked);
bool vmm_unmap_page(struct pagemap *pagemap, uintptr_t virt, bool locked);
uint64_t vmm_virt_to_phys(struct pagemap *pagemap, uint64_t virt_addr);
uint64_t vmm_virt_to_kernel(struct pagemap *pagemap, uint64_t virt_addr);
void vmm_page_fault_handler(registers_t *reg);
struct pagemap *vmm_fork_pagemap(struct pagemap *pagemap);
void vmm_destroy_pagemap(struct pagemap *pagemap);

#endif
