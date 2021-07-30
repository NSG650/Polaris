#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vmm.h"
#include "pmm.h"
#include "../kernel/die.h"
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

static struct pagemap *kernel_pagemap;

inline static uint64_t read_cr3() { uint64_t val; asm volatile ("mov %0, %%cr3" : "=r"(val)); return val; }

void vmm_init(struct stivale2_mmap_entry *memmap, size_t memmap_entries) {
    for (uintptr_t p = 0; p < 0x100000000; p += PAGE_SIZE) {
        vmm_map_page(kernel_pagemap, MEM_PHYS_OFFSET + p, p, PAGE_PRESENT);
        write_serial("First loop!: ");
        char y[10];
        hex_to_ascii(p, y);
        write_serial(y);
        write_serial("\n");
    }

    for (uintptr_t p = 0; p < 0x80000000; p += PAGE_SIZE) {
        vmm_map_page(kernel_pagemap, KERNEL_BASE + p, p, PAGE_PRESENT);
        write_serial("Second loop!: ");
        char y[10];
        hex_to_ascii(p, y);
        write_serial(y);
        write_serial("\n");
    }

    for (size_t i = 0; i < memmap_entries; i++) {
        for (uintptr_t p = 0; p < memmap[i].length; p += PAGE_SIZE) {
            vmm_map_page(kernel_pagemap, MEM_PHYS_OFFSET + p, p, PAGE_PRESENT);
            write_serial("Third loop!: ");
            char y[10];
            hex_to_ascii(p, y);
            write_serial(y);
            write_serial("\n");
        }
    }

    //vmm_switch_pagemap(kernel_pagemap);
    // This is here for testing the vmm_map_page
    /*static struct pagemap *cr3;
    cr3->top_level =  (void*)read_cr3();
    uintptr_t phys = pmm_allocz(1);
    vmm_map_page(cr3, 0x2000000000, phys, 0x03);
    *(uint64_t*)0x2000000000 = 69;
    char x[10];
    int_to_ascii(*(uint64_t*)phys, x);
    char y[10];
    int_to_ascii(phys, y);
    write_serial(x);
    write_serial("\n");
    write_serial(y);*/
}

void vmm_switch_pagemap(struct pagemap *pagemap) {
    asm volatile (
        "mov cr3, %0"
        :
        : "r" (pagemap->top_level)
        : "memory"
    );
}

struct pagemap *vmm_new_pagemap() {
    struct pagemap *pagemap = alloc(sizeof(struct pagemap));
    pagemap->top_level = pmm_allocz(1);
    return pagemap;
}

static uintptr_t *get_next_level(uintptr_t *current_level, size_t entry) {
    uintptr_t ret;
    char y[10];
    write_serial("get_next_level before the if: ");
    memset(y, 0, 10);
    hex_to_ascii(ret, y);
    write_serial(y);
    write_serial("\n");
    write_serial("We are going to crash");
    if ((current_level[entry]& (~0xFFF)) & 0x1) {
        // Present flag set
        ret = current_level[entry] & ~((uintptr_t)0xfff);
    } else {
        // Allocate a table for the next level
        ret = (uintptr_t)pmm_allocz(1);
        char y[10];
        write_serial("get_next_level: ");
        hex_to_ascii(ret, y);
        write_serial(y);
        write_serial("\n");
        if (ret == 0) {
            write_serial("We got ret as 0\n");
            for(;;)
                return NULL;
        }
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

    pml4 = pagemap->top_level + MEM_PHYS_OFFSET;;
    write_serial("get_next_level[level 3]\n");
    pml3 = get_next_level(pml4, pml4_entry);
    if (pml3 == NULL)
        return false;
    write_serial("get_next_level[level 2]\n");
    pml2 = get_next_level(pml3, pml3_entry);
    if (pml2 == NULL)
        return false;
    write_serial("get_next_level[level 1]\n");
    pml1 = get_next_level(pml2, pml2_entry);
    if (pml1 == NULL)
        return false;

    pml1[pml1_entry] = phys_addr | flags;
    return true;
}
