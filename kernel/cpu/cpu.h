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

#include "../sched/scheduler.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stivale2.h>

struct cpu_local {
	uint64_t cpu_number;
	uint32_t lapic_id;
	uint64_t tsc_frequency;
	size_t fpu_storage_size;
	void (*fpu_save)(void *);
	void (*fpu_restore)(void *);
	struct cpu_state *cpu_state;
};

extern struct cpu_local *cpu_locals;

#define this_cpu                                \
	({                                          \
		uint64_t cpu_number;                    \
		asm volatile("mov %0, qword ptr gs:[0]" \
					 : "=r"(cpu_number)         \
					 :                          \
					 : "memory");               \
		&cpu_locals[cpu_number];                \
	})

uint64_t return_bsp_lapic(void);
uint64_t return_installed_cpus(void);
void smp_init(struct stivale2_struct_tag_smp *smp_tag);
void wsmp_cpu_init(void);

#define write_cr(reg, val) \
	asm volatile("mov cr" reg ", %0" ::"r"(val) : "memory");

#define read_cr(reg)                                         \
	({                                                       \
		size_t cr;                                           \
		asm volatile("mov %0, cr" reg : "=r"(cr)::"memory"); \
		cr;                                                  \
	})

#define CPUID_INVARIANT_TSC (1 << 8)
#define CPUID_TSC_DEADLINE (1 << 24)
#define CPUID_SMEP (1 << 7)
#define CPUID_SMAP (1 << 20)
#define CPUID_UMIP (1 << 2)

#endif
