#ifndef MSR_H
#define MSR_H

#include <stdint.h>

static inline uint64_t rdmsr(uint32_t msr) {
	uint32_t edx, eax;
	asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr) : "memory");
	return ((uint64_t)edx << 32) | eax;
}

static inline void wrmsr(uint32_t msr, uint64_t value) {
	uint32_t edx = value >> 32;
	uint32_t eax = (uint32_t)value;
	asm volatile("wrmsr" ::"a"(eax), "c"(msr), "d"(edx) : "memory");
}

#endif
