#ifndef PMM_H
#define PMM_H

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

#include <asm/asm.h>
#include <limine.h>
#include <stdbool.h>
#include <stddef.h>

void *pmm_alloc(size_t pages);
void *pmm_allocz(size_t pages);
void pmm_free(void *addr, size_t pages);
void pmm_init(struct limine_memmap_entry **memmap, size_t memmap_entries);
void pmm_get_memory_info(uint64_t *info);

#endif
