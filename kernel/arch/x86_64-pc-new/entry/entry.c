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
#include <cpu/smp.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <fw/acpi.h>
#include <klibc/kargs.h>
#include <limine.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <serial/serial.h>
#include <sys/apic.h>
#include <sys/gdt.h>
#include <sys/idt.h>
#include <sys/isr.h>

extern bool print_now;

volatile struct limine_stack_size_request stack_size_request = {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.stack_size = 32768,
};

static volatile struct limine_kernel_file_request limine_kernel_file_request = {
	.id = LIMINE_KERNEL_FILE_REQUEST, .revision = 0};

static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

volatile struct limine_hhdm_request hhdm_request = {.id = LIMINE_HHDM_REQUEST,
													.revision = 0};

static volatile struct limine_kernel_address_request kernel_address_request = {
	.id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0};

static volatile struct limine_5_level_paging_request five_level_paging_request =
	{.id = LIMINE_5_LEVEL_PAGING_REQUEST, .revision = 0};

static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

static volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST, .revision = 0};

static volatile struct limine_smp_request smp_request = {
	.id = LIMINE_SMP_REQUEST, .revision = 0, .response = NULL, .flags = 1};

void arch_entry(void) {
	cli();

	struct limine_memmap_entry **memmap = memmap_request.response->entries;
	size_t memmap_entries = memmap_request.response->entry_count;
	pmm_init(memmap, memmap_entries);
	slab_init();

	struct limine_framebuffer *framebuffer =
		framebuffer_request.response->framebuffers[0];
	struct framebuffer fb = {0};
	fb.address = (uint32_t *)framebuffer->address;
	fb.pitch = framebuffer->pitch;
	fb.bpp = framebuffer->bpp;
	fb.width = framebuffer->width;
	fb.height = framebuffer->height;
	fb.tex_color = 0x00eee8d5;
	fb.tex_x = 0;
	fb.tex_y = 0;
	fb.bg_color = 0x00124560;
	framebuffer_init(&fb);
	print_now = true;
	serial_init();

	struct limine_file *kernel_file =
		limine_kernel_file_request.response->kernel_file;

	kargs_init(kernel_file->cmdline);
	uint16_t kernel_args_num = kernel_arguments.kernel_args;
	uint32_t cpu_count = kernel_arguments.cpu_count;

	if ((kernel_args_num & KERNEL_ARGS_KPRINTF_LOGS)) {
		put_to_fb = true;
	}

	kprintf("Hello x86_64 yet again!\n");
#ifdef GIT_VERSION
	kprintf("Version: %s\n", GIT_VERSION);
#endif
	kprintf("Got kernel cmdline as \"%s\"\n", kernel_file->cmdline);

	if ((kernel_args_num & KERNEL_ARGS_CPU_COUNT_GIVEN))
		kprintf("cpu count is %u\n", cpu_count);

	gdt_init();
	isr_install();

	acpi_init(rsdp_request.response->address);
	apic_init();

	smp_init(smp_request.response);
	kprintf("Neptune we need the VMM man\n");
	for (;;) {
		cli();
		halt();
	}
}
