#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern uint64_t cpu_tsc_frequency;
extern size_t cpu_fpu_storage_size;

extern void (*cpu_fpu_save)(void *);
extern void (*cpu_fpu_restore)(void *);

void cpu_init(void);

#define write_cr(reg, val) \
	__asm__ volatile("mov cr" reg ", %0" ::"r"(val) : "memory");

#define read_cr(reg)                                             \
	({                                                           \
		size_t cr;                                               \
		__asm__ volatile("mov %0, cr" reg : "=r"(cr)::"memory"); \
		cr;                                                      \
	})

#define CPUID_INVARIANT_TSC (1 << 8)
#define CPUID_TSC_DEADLINE	(1 << 24)

#endif
