#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include <stivale2.h>

void pmm_init(struct stivale2_mmap_entry *memap, size_t memap_entries);
void pmm_reclaim_memory(struct stivale2_mmap_entry *memmap, size_t memap_entries);
void *pmm_alloc(size_t count);
void *pmm_allocz(size_t count);
void pmm_free(void *ptr, size_t count);
uintptr_t return_highest_page(void);
#endif
