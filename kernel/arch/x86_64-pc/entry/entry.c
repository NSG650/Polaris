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
#include <cpuid.h>
#include <debug/debug.h>
#include <fb/fb.h>
#include <fw/acpi.h>
#include <klibc/kargs.h>
#include <klibc/module.h>
#include <klibc/random.h>
#include <klibc/time.h>
#include <limine.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <sched/sched.h>
#include <serial/serial.h>
#include <sys/apic.h>
#include <sys/elf.h>
#include <sys/gdt.h>
#include <sys/halt.h>
#include <sys/hpet.h>
#include <sys/idt.h>
#include <sys/isr.h>
#include <sys/pit.h>
#include <sys/prcb.h>
#include <sys/timer.h>

extern bool print_now;

volatile struct limine_stack_size_request stack_size_request = {
	.id = LIMINE_STACK_SIZE_REQUEST,
	.revision = 0,
	.stack_size = CPU_STACK_SIZE,
};

static volatile struct limine_kernel_file_request limine_kernel_file_request = {
	.id = LIMINE_KERNEL_FILE_REQUEST, .revision = 0};

static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

static volatile struct limine_rsdp_request rsdp_request = {
	.id = LIMINE_RSDP_REQUEST, .revision = 0};

static volatile struct limine_smp_request smp_request = {
	.id = LIMINE_SMP_REQUEST, .revision = 0, .response = NULL, .flags = 1};

static volatile struct limine_module_request module_request = {
	.id = LIMINE_MODULE_REQUEST, .revision = 0};

extern bool is_halting;
extern uint8_t is_pausing;

void nmi_vector(registers_t *reg) {
	if (is_halting) {
		for (;;) {
			cli();
			halt();
		}
	} else if (is_pausing) {
		while (is_pausing & PAUSING) {
			pause();
		}
		apic_eoi();
	} else {
		panic_((void *)(reg->rip), (void *)(reg->rbp), "Unexpected NMI\n");
	}
}

void breakpoint_handler(registers_t *reg);

static uint64_t rdseed(void) {
	uint64_t r = 0;
	asm volatile("rdseed %0" : "=r"(r));
	return r;
}

static uint64_t rdrand(void) {
	uint64_t r = 0;
	asm volatile("rdrand %0" : "=r"(r));
	return r;
}

void random_setup_seed_source(void) {
	uint32_t a = 0, b = 0, c = 0, d = 0;
	__get_cpuid(7, &a, &b, &c, &d);
	if (b & bit_RDSEED) {
		kprintf("Using RDSEED as seed source\n");
		random_get_seed = rdseed;
	}
	if (c & bit_RDRND) {
		kprintf("Using RDRAND as seed source\n");
		random_get_seed = rdrand;
	}
}

void arch_entry(void) {
	cli();

	struct limine_memmap_entry **memmap = memmap_request.response->entries;
	size_t memmap_entries = memmap_request.response->entry_count;
	pmm_init(memmap, memmap_entries);
	slab_init();
	vmm_init(memmap, memmap_entries);

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
	fb.color_masks[0].offset = framebuffer->red_mask_shift;
	fb.color_masks[0].length = framebuffer->red_mask_size;
	fb.color_masks[1].offset = framebuffer->green_mask_shift;
	fb.color_masks[1].length = framebuffer->green_mask_size;
	fb.color_masks[2].offset = framebuffer->blue_mask_shift;
	fb.color_masks[2].length = framebuffer->blue_mask_size;
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

	kprintf("Hello x86_64!\n");
#ifdef GIT_VERSION
	kprintf("Version: %s\n", GIT_VERSION);
#endif
	kprintf("Got kernel cmdline as \"%s\"\n", kernel_file->cmdline);

	if ((kernel_args_num & KERNEL_ARGS_CPU_COUNT_GIVEN))
		kprintf("CPU count is %u\n", cpu_count);

	gdt_init();
	isr_install();

	isr_register_handler(2, nmi_vector);
	isr_register_handler(3, breakpoint_handler);
	isr_register_handler(48, resched);

	elf_init_function_table(kernel_file->address);

	acpi_init(rsdp_request.response->address);
	timer_init();
	apic_init();

	smp_init(smp_request.response);

	// The NSA has also forced hardware manufacturers to backdoor their 'Random
	// Number Generators' to allow them to break RSA encryption

	if (!(kernel_arguments.kernel_args &
		  KERNEL_ARGS_DONT_TRUST_CPU_RANDOM_SEED)) {
		random_setup_seed_source();
	}
	random_set_seed(random_get_seed());

	size_t module_info[2] = {0};
	struct limine_file *module = module_request.response->modules[0];
	module_info[0] = (size_t)module->address;
	module_info[1] = (size_t)module->size;

	time_init();
	sched_init((uint64_t)module_info);

	timer_sched_oneshot(48, 20000);
	sti();

	for (;;) {
		halt();
	}
}
