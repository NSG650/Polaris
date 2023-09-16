#include <klibc/misc.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/vmm.h>

struct pagemap *kernel_pagemap = NULL;

extern char text_start_addr[], text_end_addr[];
extern char rodata_start_addr[], rodata_end_addr[];
extern char data_start_addr[], data_end_addr[];

volatile struct limine_hhdm_request hhdm_request = {.id = LIMINE_HHDM_REQUEST,
													.revision = 0};

static volatile struct limine_kernel_address_request kernel_address_request = {
	.id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0};

static volatile struct limine_paging_mode_request paging_mode_request = {
	.id = LIMINE_PAGING_MODE_REQUEST,
	.revision = 0,
	.response = NULL,
	.mode = LIMINE_PAGING_MODE_X86_64_5LVL,
	.flags = 0};

static uint64_t *get_next_level(uint64_t *top_level, size_t idx,
								bool allocate) {
	if (top_level[idx] & 1) {
		return (uint64_t *)((size_t)(top_level[idx] & ~((uint64_t)0xFFF)) +
							MEM_PHYS_OFFSET);
	}

	if (!allocate) {
		return NULL;
	}

	void *next_level = pmm_allocz(1);
	top_level[idx] = (uint64_t)next_level | 0b111;

	return next_level + MEM_PHYS_OFFSET;
}

void vmm_init(struct limine_memmap_entry **memmap, size_t memmap_entries) {
	kernel_pagemap = kmalloc(sizeof(struct pagemap));
	kernel_pagemap->lock = 0;
	kernel_pagemap->top_level = pmm_allocz(1) + MEM_PHYS_OFFSET;

	for (uint64_t p = 256; p < 512; p++)
		get_next_level(kernel_pagemap->top_level, p, true);

	for (uint64_t p = 0; p < 0x200000; p += PAGE_SIZE) {
		vmm_map_page(kernel_pagemap, p + MEM_PHYS_OFFSET, p, 0b111);
	}

	for (uint64_t p = 0x200000; p < 0x40000000; p += PAGE_SIZE) {
		vmm_map_page(kernel_pagemap, p + MEM_PHYS_OFFSET, p, 0b111);
	}

	for (uint64_t p = 0x40000000; p < 4096UL * 1024 * 1024; p += PAGE_SIZE) {
		vmm_map_page(kernel_pagemap, p + MEM_PHYS_OFFSET, p, 0b111);
	}

	for (size_t i = 0; i < memmap_entries; i++) {
		uint64_t base = memmap[i]->base;
		uint64_t length = memmap[i]->length;
		uint64_t top = base + length;

		if (base < 0x100000000)
			base = 0x100000000;

		if (base >= top)
			continue;

		uint64_t aligned_base = ALIGN_DOWN(base, PAGE_SIZE);
		uint64_t aligned_top = ALIGN_UP(top, PAGE_SIZE);
		uint64_t aligned_length = aligned_top - aligned_base;

		for (uint64_t j = 0; j < aligned_length; j += PAGE_SIZE) {
			uint64_t page = aligned_base + j;
			vmm_map_page(kernel_pagemap, page + MEM_PHYS_OFFSET, page, 0b111);
		}
	}

	uintptr_t text_start = ALIGN_DOWN((uintptr_t)text_start_addr, PAGE_SIZE),
			  rodata_start =
				  ALIGN_DOWN((uintptr_t)rodata_start_addr, PAGE_SIZE),
			  data_start = ALIGN_DOWN((uintptr_t)data_start_addr, PAGE_SIZE),
			  text_end = ALIGN_UP((uintptr_t)text_end_addr, PAGE_SIZE),
			  rodata_end = ALIGN_UP((uintptr_t)rodata_end_addr, PAGE_SIZE),
			  data_end = ALIGN_UP((uintptr_t)data_end_addr, PAGE_SIZE);

	uint64_t paddr = kernel_address_request.response->physical_base;
	uint64_t vaddr = kernel_address_request.response->virtual_base;

	for (uintptr_t text_addr = text_start; text_addr < text_end;
		 text_addr += PAGE_SIZE) {
		uintptr_t phys = text_addr - vaddr + paddr;
		vmm_map_page(kernel_pagemap, text_addr, phys, 1);
	}

	for (uintptr_t rodata_addr = rodata_start; rodata_addr < rodata_end;
		 rodata_addr += PAGE_SIZE) {
		uintptr_t phys = rodata_addr - vaddr + paddr;
		vmm_map_page(kernel_pagemap, rodata_addr, phys, 1 | 1ull << 63ull);
	}

	for (uintptr_t data_addr = data_start; data_addr < data_end;
		 data_addr += PAGE_SIZE) {
		uintptr_t phys = data_addr - vaddr + paddr;
		vmm_map_page(kernel_pagemap, data_addr, phys, 0b11 | 1ull << 63ull);
	}
	// Switch to the new page map, dropping Limine's default one
	vmm_switch_pagemap(kernel_pagemap);
}

void vmm_switch_pagemap(struct pagemap *pagemap) {
	asm volatile("mov cr3, %0"
				 :
				 : "r"((void *)((uint64_t)pagemap->top_level - MEM_PHYS_OFFSET))
				 : "memory");
}

// Creates a new dynamically allocated page map
struct pagemap *vmm_new_pagemap(void) {
	struct pagemap *pagemap = kmalloc(sizeof(struct pagemap));
	pagemap->lock = 0;
	pagemap->top_level = pmm_allocz(1) + MEM_PHYS_OFFSET;
	for (size_t i = 256; i < 512; i++)
		pagemap->top_level[i] = kernel_pagemap->top_level[i];
	return pagemap;
}

bool vmm_map_page(struct pagemap *pagemap, uint64_t virt_addr,
				  uint64_t phys_addr, uint64_t flags) {
	spinlock_acquire_or_wait(&pagemap->lock);

	// Calculate the indices in the various tables using the virtual address
	size_t pml5_entry = (virt_addr & ((uint64_t)0x1FF << 48)) >> 48;
	size_t pml4_entry = (virt_addr & ((uint64_t)0x1FF << 39)) >> 39;
	size_t pml3_entry = (virt_addr & ((uint64_t)0x1FF << 30)) >> 30;
	size_t pml2_entry = (virt_addr & ((uint64_t)0x1FF << 21)) >> 21;
	size_t pml1_entry = (virt_addr & ((uint64_t)0x1FF << 12)) >> 12;

	uint64_t *pml5, *pml4, *pml3, *pml2, *pml1;

	if (paging_mode_request.response->mode == LIMINE_PAGING_MODE_X86_64_5LVL) {
		pml5 = pagemap->top_level;
		goto level5;
	} else {
		pml4 = pagemap->top_level;
		goto level4;
	}

level5:
	pml4 = get_next_level(pml5, pml5_entry, true);
	if (pml4 == NULL) {
		goto die;
	}
level4:
	pml3 = get_next_level(pml4, pml4_entry, true);
	if (pml3 == NULL) {
		goto die;
	}

	pml2 = get_next_level(pml3, pml3_entry, true);
	if (pml2 == NULL) {
		goto die;
	}

	pml1 = get_next_level(pml2, pml2_entry, true);
	if (pml1 == NULL) {
	die:
		spinlock_drop(&pagemap->lock);
		return false;
	}

	pml1[pml1_entry] = phys_addr | flags;
	spinlock_drop(&pagemap->lock);
	return true;
}

bool vmm_unmap_page(struct pagemap *pagemap, uintptr_t virt) {
	spinlock_acquire_or_wait(&pagemap->lock);

	size_t pml5_entry = (virt & ((uint64_t)0x1FF << 48)) >> 48;
	size_t pml4_entry = (virt & ((uint64_t)0x1FF << 39)) >> 39;
	size_t pml3_entry = (virt & ((uint64_t)0x1FF << 30)) >> 30;
	size_t pml2_entry = (virt & ((uint64_t)0x1FF << 21)) >> 21;
	size_t pml1_entry = (virt & ((uint64_t)0x1FF << 12)) >> 12;

	uint64_t *pml5, *pml4, *pml3, *pml2, *pml1;

	if (paging_mode_request.response->mode == LIMINE_PAGING_MODE_X86_64_5LVL) {
		pml5 = pagemap->top_level;
		goto level5;
	} else {
		pml4 = pagemap->top_level;
		goto level4;
	}

level5:
	pml4 = get_next_level(pml5, pml5_entry, false);
	if (pml4 == NULL) {
		goto die;
	}
level4:
	pml3 = get_next_level(pml4, pml4_entry, false);
	if (pml3 == NULL) {
		goto die;
	}

	pml2 = get_next_level(pml3, pml3_entry, false);
	if (pml2 == NULL) {
		goto die;
	}

	pml1 = get_next_level(pml2, pml2_entry, false);
	if (pml1 == NULL) {
		goto die;
	}

	if ((pml1[pml1_entry] & 1) == 0) {
	die:
		spinlock_drop(&pagemap->lock);
		return false;
	}

	pml1[pml1_entry] = 0;

	asm volatile("invlpg [%0]" : : "r"(virt) : "memory");

	spinlock_drop(&pagemap->lock);
	return true;
}
