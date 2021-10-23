/*
 * Copyright 2021 NSG650
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

#include "syscall.h"
#include "../klibc/printf.h"
#include "cpu.h"
#include "reg.h"
#include <liballoc.h>
#include <stdint.h>

extern void syscall_handle(void);

void syscall_handler(registers_t *reg) {
	printf("syscall invoked!\nRAX: %llx\nRDI: %llx\nRSI: %llx\nRDX: %llx\n",
		   reg->rax, reg->rdi, reg->rsi, reg->rdx);
}

void syscall_init(void) {
	/*
	 *	EFER - 0xC0000080
	 *	STAR - 0xC0000081
	 *	LSTAR - 0xC0000082
	 *	SYSCALL Extension - 1
	 *	SYSCALL Flag Mask - 0xC0000084
	 *	SYSCALL Flag turn off interrupt - 1 << 9
	 */
	wrmsr(0xC0000080, rdmsr(0xC0000080) | 1);
	/*
	 * CS = kernel code
	 * SS = kernel data
	 * when returning:
	 * CS = user code
	 * SS = user data
	 *
	 * GDT Ring 3 - 3
	 * GDT Kernel Code - 1
	 * GDT User Data - 3
	 * STAR Kernel code offset - 32
	 * STAR User code offset - 48
	 */
	wrmsr(0xC0000081,
		  ((uint64_t)(1 * 8) << 32) | ((uint64_t)(((3 - 1) * 8) | 3) << 48));
	wrmsr(0xC0000082, (uint64_t)syscall_handle);
	wrmsr(0xC0000084, (1 << 9));

	uint8_t *cpu_stack = kmalloc(KSTACK_SIZE);
	this_cpu->kernel_stack = (uintptr_t)cpu_stack + KSTACK_SIZE;
}
