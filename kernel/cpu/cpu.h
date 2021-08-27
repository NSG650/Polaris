#ifndef CPU_H
#define CPU_H

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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>

extern uint64_t cpu_tsc_frequency;
extern size_t cpu_fpu_storage_size;

extern void (*cpu_fpu_save)(void *);
extern void (*cpu_fpu_restore)(void *);

void smp_init(struct stivale2_struct_tag_smp *smp_tag);
void cpu_init(void);

#define write_cr(reg, val) \
	asm volatile("mov cr" reg ", %0" ::"r"(val) : "memory");

#define read_cr(reg)                                         \
	({                                                       \
		size_t cr;                                           \
		asm volatile("mov %0, cr" reg : "=r"(cr)::"memory"); \
		cr;                                                  \
	})

#define CPUID_INVARIANT_TSC (1 << 8)
#define CPUID_TSC_DEADLINE	(1 << 24)
#define CPUID_SMEP			(1 << 7)
#define CPUID_SMAP			(1 << 20)
#define CPUID_UMIP			(1 << 2)

#endif
