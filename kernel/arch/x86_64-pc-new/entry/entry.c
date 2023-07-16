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
#include <debug/debug.h>
#include <limine.h>
#include <serial/serial.h>
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

void arch_entry(void) {
	serial_init();
	print_now = true;
	kprintf("Hello x86_64 yet again!\n");

	struct limine_file *kernel_file =
		limine_kernel_file_request.response->kernel_file;

	kprintf("Got command line arguments as %s\n", kernel_file->cmdline);

	gdt_init();
	isr_install();

	asm volatile("int 3\n");

	for (;;) {
		cli();
		halt();
	}
}
