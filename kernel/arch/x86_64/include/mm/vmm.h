#ifndef VMM_H
#define VMM_H

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

#include <reg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>

#define PAGE_SIZE ((size_t)4096)
#define MEM_PHYS_OFFSET ((uint64_t)0xFFFF800000000000)

struct pagemap {
	void *top_level;
};

extern struct pagemap *kernel_pagemap;

void vmm_init(struct stivale2_mmap_entry *memmap, size_t memmap_entries,
			  struct stivale2_pmr *pmrs, size_t pmr_entries,
			  uint64_t virtual_base_address, uint64_t physical_base_address);
void vmm_switch_pagemap(struct pagemap *pagemap);
struct pagemap *vmm_new_pagemap(void);
bool vmm_map_page(struct pagemap *pagemap, uint64_t virt_addr,
				  uint64_t phys_addr, uint64_t flags, bool hugepages,
				  bool gbpages);
void vmm_page_fault_handler(registers_t *reg);

#endif
