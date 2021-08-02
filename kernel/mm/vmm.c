#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vmm.h"
#include "pmm.h"
#include "../klibc/lock.h"
#include "../klibc/alloc.h"
#include "../klibc/string.h"
#include "../serial/serial.h"
#include "../klibc/mem.h"

struct mmap_range {
    uintptr_t base;
    size_t    length;
    struct resource *res;
    off_t     offset;
    int       prot;
    int       flags;
};

static struct pagemap kernel_pagemap;

void vmm_init(struct stivale2_mmap_entry *memmap, size_t memmap_entries) {
    kernel_pagemap.top_level = pmm_allocz(1);
    for (uintptr_t p = 0; p < 4096UL * 1024 * 1024; p += PAGE_SIZE) {
        vmm_map_page(&kernel_pagemap, p, p, 0b11);
        vmm_map_page(&kernel_pagemap, p + MEM_PHYS_OFFSET, p, 0b11);
    }
    for (uintptr_t p = 0; p < 2048UL * 1024 * 1024; p += PAGE_SIZE) {
        vmm_map_page(&kernel_pagemap, KERNEL_BASE + p, p, 0b11);
    }
    for (size_t i = 0; i < memmap_entries; i++) {
        for (uintptr_t p = 0; p < memmap[i].length; p += PAGE_SIZE)
            vmm_map_page(&kernel_pagemap, MEM_PHYS_OFFSET + p, p, 0b11);
    }
    vmm_switch_pagemap(&kernel_pagemap);
}

void vmm_switch_pagemap(struct pagemap *pagemap) {
    asm volatile (
        "mov cr3, %0"
        :
        : "r" (pagemap->top_level)
        : "memory"
    );
}

static uintptr_t *get_next_level(uintptr_t *current_level, size_t entry) {
    uintptr_t ret;
    if ((current_level[entry] & 0x1)) {
        // Present flag set
        ret = current_level[entry] & ~((uintptr_t)0xfff);
    } else {
        // Allocate a table for the next level
        ret = (uintptr_t)pmm_allocz(1);
        // Present + writable + user (0b111)
        current_level[entry] = ret | 0b111;
    }

    return (uintptr_t *)ret;
}



bool vmm_map_page(struct pagemap *pagemap, uintptr_t virt_addr, uintptr_t phys_addr, uintptr_t flags) {
    uintptr_t pml4_entry = (virt_addr & ((uintptr_t)0x1ff << 39)) >> 39;
    uintptr_t pml3_entry = (virt_addr & ((uintptr_t)0x1ff << 30)) >> 30;
    uintptr_t pml2_entry = (virt_addr & ((uintptr_t)0x1ff << 21)) >> 21;
    uintptr_t pml1_entry = (virt_addr & ((uintptr_t)0x1ff << 12)) >> 12;

    uintptr_t *pml4, *pml3, *pml2, *pml1;

    pml4 = pagemap->top_level;
    pml3 = get_next_level(pml4, pml4_entry);
    if (pml3 == NULL)
        return false;
    pml2 = get_next_level(pml3, pml3_entry);
    if (pml2 == NULL)
        return false;
    pml1 = get_next_level(pml2, pml2_entry);
    if (pml1 == NULL)
        return false;

    pml1[pml1_entry] = phys_addr | flags;
    return true;
}
