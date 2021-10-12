/*
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

#include "cpu.h"
#include "../klibc/alloc.h"
#include "../klibc/asm.h"
#include "../klibc/lock.h"
#include "../klibc/mem.h"
#include "../klibc/printf.h"
#include "../sys/gdt.h"
#include "apic.h"
#include <cpuid.h>
#include <liballoc.h>

struct cpu_local *cpu_locals;
uint64_t cpu_count;

static void cpu_init(struct stivale2_smp_info *smp_info);

#define MAX_TSC_CALIBRATIONS 4

lock_t cpu_lock;
uint64_t bsp_lapic_id = 0;

uint64_t rdmsr(uint32_t msr) {
	uint32_t edx, eax;
	asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");
	return ((uint64_t)edx << 32) | eax;
}

void wrmsr(uint32_t msr, uint64_t value) {
	uint32_t edx = value >> 32;
	uint32_t eax = (uint32_t)value;
	asm volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(msr) : "memory");
}

static void wrxcr(uint32_t i, uint64_t value) {
	uint32_t edx = value >> 32;
	uint32_t eax = (uint32_t)value;
	asm volatile("xsetbv" : : "a"(eax), "d"(edx), "c"(i) : "memory");
}

static void xsave(void *region) {
	asm volatile("xsave %0"
				 : "+m"(FLAT_PTR(region))
				 : "a"(0xFFFFFFFF), "d"(0xFFFFFFFF)
				 : "memory");
}

static void xrstor(void *region) {
	asm volatile("xrstor %0"
				 :
				 : "m"(FLAT_PTR(region)), "a"(0xFFFFFFFF), "d"(0xFFFFFFFF)
				 : "memory");
}

static void fxsave(void *region) {
	asm volatile("fxsave %0" : "+m"(FLAT_PTR(region)) : : "memory");
}

static void fxrstor(void *region) {
	asm volatile("fxrstor %0" : : "m"(FLAT_PTR(region)) : "memory");
}

void smp_init(struct stivale2_struct_tag_smp *smp_tag) {
	printf("CPU: Total processor count: %d\n", smp_tag->cpu_count);
	bsp_lapic_id = smp_tag->bsp_lapic_id;
	cpu_locals = alloc(sizeof(struct cpu_local) * smp_tag->cpu_count);
	for (size_t i = 0; i < smp_tag->cpu_count; ++i) {
		smp_tag->smp_info[i].extra_argument = (uint64_t)&cpu_locals[i];
		if (smp_tag->smp_info[i].lapic_id == bsp_lapic_id) {
			cpu_init((void *)&smp_tag->smp_info[i]);
			continue;
		}
		cpu_locals[i].cpu_number = i;
		uint8_t *stack = kmalloc(KSTACK_SIZE);
		smp_tag->smp_info[i].target_stack = (uintptr_t)stack + KSTACK_SIZE;
		smp_tag->smp_info[i].goto_address = (uintptr_t)cpu_init;
	}
}

// Initial setup needed before SMP
void wsmp_cpu_init(void) {
	// Firstly enable SSE/SSE2 as it's the baseline for x86_64
	uint64_t cr0 = 0;
	cr0 = read_cr("0");
	cr0 &= ~(1 << 2);
	cr0 |= (1 << 1);
	write_cr("0", cr0);

	uint64_t cr4 = 0;
	cr4 = read_cr("4");
	cr4 |= (3 << 9);
	write_cr("4", cr4);

	// Assume TSC is supported, is way older than x86_64
	cr4 = read_cr("4");
	cr4 |= (1 << 2);
	write_cr("4", cr4);

	// Enable some modern minor x86_64 features, ported from Sigma OS
	uint32_t a = 0, b = 0, c = 0, d = 0;
	if (__get_cpuid(7, &a, &b, &c, &d)) {
		if ((b & CPUID_SMEP)) {
			cr4 = read_cr("4");
			cr4 |= (1 << 20); // Enable SMEP
			write_cr("4", cr4);
		}
	}

	if (__get_cpuid(7, &a, &b, &c, &d)) {
		if ((b & CPUID_SMAP)) {
			cr4 = read_cr("4");
			cr4 |= (1 << 21); // Enable SMAP
			write_cr("4", cr4);
			asm("clac");
		}
	}

	if (__get_cpuid(7, &a, &b, &c, &d)) {
		if ((c & CPUID_UMIP)) {
			cr4 = read_cr("4");
			cr4 |= (1 << 11); // Enable UMIP
			write_cr("4", cr4);
		}
	}

	// Initialize the PAT
	uint64_t pat_msr = rdmsr(0x277);
	pat_msr &= 0xFFFFFFFF;
	// write-protect / write-combining
	pat_msr |= (uint64_t)0x0105 << 32;
	wrmsr(0x277, pat_msr);
}

static void cpu_init(struct stivale2_smp_info *smp_info) {
	LOCK(cpu_lock);
	gdt_init();
	// Load CPU local address in gsbase
	wrmsr(0xC0000101, (uintptr_t)smp_info->extra_argument);
	printf("CPU: Processor %d online!\n", this_cpu->cpu_number);

	this_cpu->lapic_id = smp_info->lapic_id;

	wsmp_cpu_init();

	uint64_t cr4 = 0;
	uint32_t a = 0, b = 0, c = 0, d = 0;
	__get_cpuid(1, &a, &b, &c, &d);
	if ((c & bit_XSAVE)) {
		cr4 = read_cr("4");
		cr4 |= (1 << 18); // Enable XSAVE and x{get, set}bv
		write_cr("4", cr4);

		uint64_t xcr0 = 0;
		xcr0 |= (1 << 0); // Save x87 state with xsave
		xcr0 |= (1 << 1); // Save SSE state with xsave

		if ((c & bit_AVX))
			xcr0 |= (1 << 2); // Enable AVX and save AVX state with xsave

		if (__get_cpuid(7, &a, &b, &c, &d)) {
			if ((b & bit_AVX512F)) {
				xcr0 |= (1 << 5); // Enable AVX-512
				xcr0 |= (1 << 6); // Enable management of ZMM{0 -> 15}
				xcr0 |= (1 << 7); // Enable management of ZMM{16 -> 31}
			}
		}
		wrxcr(0, xcr0);

		this_cpu->fpu_storage_size = (size_t)c;

		this_cpu->fpu_save = xsave;
		this_cpu->fpu_restore = xrstor;
	} else {
		this_cpu->fpu_storage_size = 512; // Legacy size for fxsave
		this_cpu->fpu_save = fxsave;
		this_cpu->fpu_restore = fxrstor;
	}
	memset(this_cpu->cpu_state, 0, sizeof(struct cpu_state));
	UNLOCK(cpu_lock);
	cpu_count++;
	if (this_cpu->lapic_id != bsp_lapic_id) {
		lapic_init(this_cpu->lapic_id);
		sched_init();
	}
}

uint64_t return_installed_cpus(void) {
	return cpu_count;
}
