#ifndef VMM_H
#define VMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../klibc/resource.h"
#include "../klibc/types.h"
#include <stivale2.h>

#define PAGE_SIZE		((size_t)4096)
#define MEM_PHYS_OFFSET ((uintptr_t)0xFFFF800000000000)
#define KERNEL_BASE		((uintptr_t)0xFFFFFFFF80000000)

#define PROT_NONE  0x00
#define PROT_READ  0x01
#define PROT_WRITE 0x02
#define PROT_EXEC  0x04

#define MAP_PRIVATE	  0x01
#define MAP_SHARED	  0x02
#define MAP_FIXED	  0x04
#define MAP_ANON	  0x08
#define MAP_ANONYMOUS 0x08

#define MAP_FAILED ((void *)-1)

#define PAGE_PRESENT (1ULL)
#define PAGE_RDWR	 (1ULL << 1)
#define PAGE_USER	 (1ULL << 2)
#define PAGE_WRITET	 (1ULL << 3)
#define PAGE_NCACHE	 (1ULL << 4)
#define PAGE_PCD	 (1ULL << 5)
#define PAGE_GLOBAL	 (1ULL << 8)
#define PAGE_HUGE	 (1ULL << 7)

#define PAGE_COW (1ULL)

struct pagemap {
	lock_t lock;
	uintptr_t *top_level;
};

void vmm_init(struct stivale2_mmap_entry *memmap, size_t memmap_entries);
void vmm_switch_pagemap(struct pagemap *pagemap);
struct pagemap *vmm_new_pagemap(void);
bool vmm_map_page(struct pagemap *pagemap, uintptr_t virt_addr,
				  uintptr_t phys_addr, uintptr_t flags);
// TODO: implmenet mmap
// void *mmap(struct pagemap *pm, void *addr, size_t length, int prot, int
// flags,
//           struct resource *res, off_t offset);

#endif
