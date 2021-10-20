/*
 * Copyright 2021 Misha
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

#include "../acpi/acpi.h"
#include "../cpu/apic.h"
#include "../cpu/cpu.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../dev/initramfs.h"
#include "../fs/devtmpfs.h"
#include "../fs/tmpfs.h"
#include "../fs/vfs.h"
#include "../klibc/printf.h"
#include "../klibc/resource.h"
#include "../mm/pmm.h"
#include "../mm/vmm.h"
#include "../sched/process.h"
#include "../sched/scheduler.h"
#include "../serial/serial.h"
#include "../sys/gdt.h"
#include "../video/video.h"
#include "panic.h"
#include <liballoc.h>
#include <stdint.h>
#include <stivale2.h>

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
	.flags = (1 << 1) | (1 << 2),
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

void a(void) {
	printf("a running on CPU%d\n", this_cpu->cpu_number);
	process_exit();
}

void kernel_main(struct stivale2_struct *stivale2_struct) {
	printf("Hello World!\n");
	printf("A (4 bytes): %p\n", kmalloc(4));
	void *ptr = kmalloc(8);
	printf("B (8 bytes): %p\n", ptr);
	kfree(ptr);
	printf("Freed B\n");
	void *ptr2 = kmalloc(16);
	printf("C (16 bytes): %p\n", ptr2);
	void *ptr3 = kmalloc(32);
	printf("D (32 bytes): %p\n", ptr3);
	printf("C (16 bytes to 32 bytes realloc): %p\n", krealloc(ptr2, 32));
	printf("D (32 bytes after C realloc): %p\n", ptr3);
	printf("E (4 int calloc): %p\n", kcalloc(4, sizeof(int)));
	vfs_install_fs(&tmpfs);
	vfs_install_fs(&devtmpfs);
	vfs_mount("tmpfs", "/", "tmpfs");
	vfs_mkdir(NULL, "/dev", 0755, true);
	vfs_mount("devtmpfs", "/dev", "devtmpfs");
	struct stivale2_struct_tag_modules *modules_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MODULES_ID);
	initramfs_init(modules_tag);
	struct resource *res = vfs_open("/root/initramfs.txt", O_RDONLY, 0644);
	struct stat *st = kmalloc(sizeof(struct stat));
	vfs_stat("/root/initramfs.txt", st);
	char *buf = kmalloc(st->st_size);
	res->read(res, buf, 0, st->st_size);
	printf("Reading /root/initramfs.txt: %s\n", buf);
	process_create("a", (uintptr_t)a, 0, HIGH);
	for (;;)
		asm("hlt");
}

void _start(struct stivale2_struct *stivale2_struct) {
	gdt_init();
	struct stivale2_struct_tag_framebuffer *fb_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);
	video_init(fb_tag);
	struct stivale2_struct_tag_memmap *memmap_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_MEMMAP_ID);
	pmm_init((void *)memmap_tag->memmap, memmap_tag->entries);
	struct stivale2_struct_tag_pmrs *pmrs_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_PMRS_ID);
	vmm_init((void *)memmap_tag->memmap, memmap_tag->entries,
			 (void *)pmrs_tag->pmrs, pmrs_tag->entries);
	serial_install();
	printf("Kernel build: %s\n", KVERSION);
	isr_install();
	asm volatile("sti");
	wsmp_cpu_init();
	struct stivale2_struct_tag_rsdp *rsdp_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_RSDP_ID);
	acpi_init((void *)rsdp_tag->rsdp);
	pic_init();
	apic_init();
	struct stivale2_struct_tag_smp *smp_tag =
		stivale2_get_tag(stivale2_struct, STIVALE2_STRUCT_TAG_SMP_ID);
	smp_init(smp_tag);
	while (return_installed_cpus() != smp_tag->cpu_count)
		;
	process_init((uintptr_t)kernel_main, (uint64_t)stivale2_struct);
	sched_init();
	for (;;)
		asm("hlt");
}
