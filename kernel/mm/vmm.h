#ifndef VMM_H
#define VMM_H

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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>

#define PAGE_SIZE ((size_t)4096)
#define MEM_PHYS_OFFSET ((uintptr_t)0xFFFF800000000000)
#define KERNEL_BASE ((uintptr_t)0xFFFFFFFF80000000)

struct pagemap {
	uintptr_t *top_level;
};

void vmm_init(struct stivale2_mmap_entry *memmap, size_t memmap_entries,
			  struct stivale2_pmr *pmrs, size_t pmr_entries);
void vmm_switch_pagemap(struct pagemap *pagemap);
struct pagemap *vmm_new_pagemap(void);
bool vmm_map_page(struct pagemap *pagemap, uintptr_t virt_addr,
				  uintptr_t phys_addr, uintptr_t flags);

#endif
