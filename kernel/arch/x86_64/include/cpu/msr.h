#ifndef MSR_H
#define MSR_H

#include <stdint.h>

uint64_t rdmsr(uint32_t msr);
void wrmsr(uint32_t msr, uint64_t value);

#endif
