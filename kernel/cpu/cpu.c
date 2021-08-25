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
#include "../kernel/panic.h"
#include "../klibc/asm.h"
#include "../klibc/lock.h"
#include "../klibc/printf.h"
#include "../sys/hpet.h"
#include <cpuid.h>

#define MAX_TSC_CALIBRATIONS 4

uint64_t cpu_tsc_frequency;

size_t cpu_fpu_storage_size;

void (*cpu_fpu_save)(void *);
void (*cpu_fpu_restore)(void *);

DECLARE_LOCK(cpu_lock);

uint64_t rdtsc() {
	uint32_t edx, eax;
	asm volatile("rdtsc" : "=a"(eax), "=d"(edx));
	return ((uint64_t)edx << 32) | eax;
}

uint64_t rdmsr(uint32_t msr) {
	uint32_t edx, eax;
	asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");
	return ((uint64_t)edx << 32) | eax;
}

static void wrmsr(uint32_t msr, uint64_t value) {
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

static void cpu_start(void) {
	LOCK(cpu_lock);
	uint64_t rdi = 0;
	asm("nop" : "=D"(rdi));
	struct stivale2_smp_info *cpu_info = (void *)rdi;
	printf("CPU: Processor %d online!\n", cpu_info->lapic_id);
	UNLOCK(cpu_lock);
	for (;;)
		asm("hlt");
}

void cpu_init(struct stivale2_struct_tag_smp *smp_tag) {
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

	// Initialise the PAT
	uint64_t pat_msr = rdmsr(0x277);
	pat_msr &= 0xFFFFFFFF;
	// write-protect / write-combining
	pat_msr |= (uint64_t)0x0105 << 32;
	wrmsr(0x277, pat_msr);

	uint32_t a, b, c, d;
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

		cpu_fpu_storage_size = (size_t)c;

		cpu_fpu_save = xsave;
		cpu_fpu_restore = xrstor;
	} else {
		cpu_fpu_storage_size = 512; // Legacy size for fxsave
		cpu_fpu_save = fxsave;
		cpu_fpu_restore = fxrstor;
	}
	__get_cpuid(1, &a, &b, &c, &d);
	if ((c & CPUID_TSC_DEADLINE)) {
		// Check for invariant TSC
		__get_cpuid(0x80000007, &a, &b, &c, &d);
		if ((d & CPUID_INVARIANT_TSC)) {
			// Calibrate the TSC
			for (int i = 0; i < MAX_TSC_CALIBRATIONS; i++) {
				uint64_t initial_tsc_reading = rdtsc();

				// Wait 1 millisecond
				hpet_usleep(1000);

				uint64_t final_tsc_reading = rdtsc();

				uint64_t freq =
				  (final_tsc_reading - initial_tsc_reading) * 1000;
				printf("CPU: TSC reading #%d yielded a frequency of %u Hz.\n",
					   i, freq);

				cpu_tsc_frequency += freq;
			}

			// Average out all readings
			cpu_tsc_frequency /= MAX_TSC_CALIBRATIONS;
			printf("CPU: TSC frequency fixed at %u Hz.\n", cpu_tsc_frequency);
		}
	}

	printf("CPU: Total processor count: %d\n", smp_tag->cpu_count);
	printf("CPU: Processor %d online!\n", smp_tag->smp_info[0].lapic_id);
	for (size_t i = 0; i < smp_tag->cpu_count; i++) {
		static uint8_t stack[32768];
		smp_tag->smp_info[i].target_stack = (uintptr_t)stack + sizeof(stack);
		smp_tag->smp_info[i].goto_address = (uintptr_t)cpu_start;
	}
	// Wait 50 milliseconds
	hpet_usleep(50000);
}
