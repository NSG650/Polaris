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
#include <cpu/smp.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <fw/acpi.h>
#include <fw/madt.h>
#include <klibc/mem.h>
#include <klibc/time.h>
#include <limine.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/vmm.h>
#include <sched/sched.h>
#include <sched/syscall.h>
#include <serial/serial.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/apic.h>
#include <sys/gdt.h>
#include <sys/halt.h>
#include <sys/hpet.h>
#include <sys/isr.h>
#include <sys/timer.h>

void breakpoint_handler(registers_t *reg);

static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

static volatile struct limine_stack_size_request stack_size_request = {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.response = NULL,
	.stack_size = STACK_SIZE};

static volatile struct limine_smp_request smp_request = {
	.id = LIMINE_SMP_REQUEST, .revision = 0, .response = NULL, .flags = 1};

static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

static volatile struct limine_module_request module_request = {
	.id = LIMINE_MODULE_REQUEST, .revision = 0};

static volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST, .revision = 0};

void arch_entry(void) {
	struct limine_memmap_entry **memmap = memmap_request.response->entries;
	size_t memmap_entries = memmap_request.response->entry_count;
	pmm_init(memmap, memmap_entries);
	slab_init();
	vmm_init(memmap, memmap_entries);
	struct limine_framebuffer *framebuffer =
		framebuffer_request.response->framebuffers[0];
	struct framebuffer fb;
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
	kprintf("Hello x86_64!\n");
#ifdef GIT_VERSION
	kprintf("Version: %s\n", GIT_VERSION);
#endif
	cli();
	isr_register_handler(0xff, halt_current_cpu);
	isr_register_handler(48, resched);
	isr_register_handler(0xe, vmm_page_fault_handler);
	isr_register_handler(0x3, breakpoint_handler);
	acpi_init(rsdp_request.response->address);
	pic_init();
	smp_init(smp_request.response);
	time_init();
	ioapic_redirect_irq(0, 48);
	syscall_install_handler();
	if (module_request.response->module_count < 1)
		panic("No init found\n");
	size_t module_info[2] = {0};
	struct limine_file *module = module_request.response->modules[0];
	module_info[0] = (size_t)module->address;
	module_info[1] = (size_t)module->size;
	sched_init((uint64_t)module_info);
	timer_sched_oneshot(32, 20000);
	sti();
	for (;;) {
		halt();
	}
}
