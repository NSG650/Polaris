#ifndef VMM_H
#define VMM_H

#if !(defined(__x86_64__))
#error "THIS IS ONLY FOR x86_64"
#endif

#include <locks/spinlock.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE ((size_t)4096)

#define PAGE_READ (0b1)
#define PAGE_WRITE (0b10)
#define PAGE_EXECUTE (1ull << 63ull)

struct pagemap {
	lock_t lock;
	uint64_t *top_level;
};

extern struct pagemap *kernel_pagemap;

enum page_size { Size4KiB, Size2MiB, Size1GiB };

void vmm_switch_pagemap(struct pagemap *pagemap);
struct pagemap *vmm_new_pagemap(void);
bool vmm_map_page(struct pagemap *pagemap, uint64_t virt_addr,
				  uint64_t phys_addr, uint64_t flags, enum page_size pg_size);
bool vmm_unmap_page(struct pagemap *pagemap, uintptr_t virt);
uint64_t vmm_virt_to_phys(struct pagemap *pagemap, uint64_t virt_addr);
uint64_t vmm_virt_to_kernel(struct pagemap *pagemap, uint64_t virt_addr);

#endif
