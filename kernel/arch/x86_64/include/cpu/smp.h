#ifndef SMP_H
#define SMP_H

#include <cpu/msr.h>
#include <limine.h>
#include <stddef.h>
#include <stdint.h>

static inline void set_kernel_gs(uint64_t address) {
	wrmsr(0xc0000101, address);
}

static inline void set_user_gs(uint64_t address) {
	wrmsr(0xc0000102, address);
}

static inline uint64_t read_kernel_gs(void) {
	return rdmsr(0xc0000101);
}

static inline uint64_t read_user_gs(void) {
	return rdmsr(0xc0000102);
}

void smp_init(struct limine_smp_response *smp_info);

#endif
