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


#include <asm/asm.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <serial/serial.h>
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>
#include <klibc/mem.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mem/liballoc.h>
#include <klibc/dynarray.h>

static uint8_t stack[32768];
static struct stivale2_header_tag_smp smp_hdr_tag = {
	.tag = {.identifier = STIVALE2_HEADER_TAG_SMP_ID, .next = 0}, .flags = 1};

static struct stivale2_header_tag_framebuffer framebuffer_hdr_tag = {
	// All tags need to begin with an identifier and a pointer to the next tag
	.tag = {.identifier = STIVALE2_HEADER_TAG_FRAMEBUFFER_ID,
			.next = (uintptr_t)&smp_hdr_tag},
	.framebuffer_width = 0,
	.framebuffer_height = 0,
	.framebuffer_bpp = 0};

__attribute__((section(".stivale2hdr"),
			   used)) static struct stivale2_header stivale_hdr = {
	.entry_point = 0,
	.stack = (uintptr_t)stack + sizeof(stack),
	.flags = (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
	.tags = (uintptr_t)&framebuffer_hdr_tag};

void *stivale2_get_tag(struct stivale2_struct *stivale2_struct, uint64_t id) {
	struct stivale2_tag *current_tag = (void *)stivale2_struct->tags;
	for (;;) {
		if (current_tag == NULL) {
			return NULL;
		}

		if (current_tag->identifier == id) {
			return current_tag;
		}

		current_tag = (void *)current_tag->next;
	}
}

void arch_entry(struct stivale2_struct *stivale2_struct) {
	struct stivale2_struct_tag_memmap *memmap_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
	pmm_init((void *)memmap_tag->memmap, memmap_tag->entries);
	struct stivale2_struct_tag_pmrs *pmrs_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_PMRS_ID);
	struct stivale2_struct_tag_kernel_base_address *kernel_base_tag =
		stivale2_get_tag(stivale2_struct,
						 STIVALE2_STRUCT_TAG_KERNEL_BASE_ADDRESS_ID);
	vmm_init((void *)memmap_tag->memmap, memmap_tag->entries,
			 (void *)pmrs_tag->pmrs, pmrs_tag->entries,
			 kernel_base_tag->virtual_base_address,
			 kernel_base_tag->physical_base_address);
	struct framebuffer fb;
	struct stivale2_struct_tag_framebuffer *fb_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
	fb.address = (uint8_t *)fb_tag->framebuffer_addr;
	fb.pitch = fb_tag->framebuffer_pitch;
	fb.bpp = fb_tag->framebuffer_bpp;
	fb.width = fb_tag->framebuffer_width;
	fb.height = fb_tag->framebuffer_height;
	fb.tex_color = 0xffffff;
	fb.tex_x = 0;
	fb.tex_y = 0;
	framebuffer_init(&fb);
	char *x = kmalloc(10);
	kprintf("memory located for x at %p\n", x);
	strcpy(x, "ABCDEFGHI");
	kprintf("x: %s\n", x);
	kprintf("freeing x\n", x);
	kfree(x);
	kprintf("Dynamic array test\n");
	dynarray_struct(int) funny = {0};
	dynarray_init(funny, 10);
	kprintf("Adding data to dynarray\n");
	for(int i = 0; i < 11; i++)
		dynarray_push(funny, i);
	for(int i = 0; i < funny.length; i++)
		kprintf("funny.storage[%d]: %d\n", i, funny.storage[i]);
	kprintf("Adding more data\n");
	dynarray_push(funny, 650);
	for(int i = 0; i < funny.length; i++)
		kprintf("funny.storage[%d]: %d\n", i, funny.storage[i]);
	dynarray_remove_by_value(funny, 5);
	kprintf("Deleting the number 5 value\n");
	for(int i = 0; i < funny.length; i++)
		kprintf("funny.storage[%d]: %d\n", i, funny.storage[i]);
	kprintf("Deleting dynarray\n");
	dynarray_delete(funny);
	for (;;) {
		cli();
		halt();
	}
}
