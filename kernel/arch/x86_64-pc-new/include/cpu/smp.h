#ifndef SMP_H
#define SMP_H

#include <asm/asm.h>
#include <cpu/msr.h>
#include <limine.h>
#include <stddef.h>
#include <stdint.h>

static inline void set_kernel_gs(uint64_t address) {
	wrmsr(0xc0000102, address);
}

static inline uintptr_t read_kernel_gs(void) {
	return rdmsr(0xc0000102);
}

static inline void set_user_gs(uint64_t address) {
	wrmsr(0xc0000101, address);
}

static inline uintptr_t read_user_gs(void) {
	return rdmsr(0xc0000101);
}

static inline void set_fs_base(uint64_t address) {
	wrmsr(0xc0000100, address);
}

static inline uintptr_t read_fs_base(void) {
	return rdmsr(0xc0000100);
}

static inline void swapgs(void) {
	asm volatile("swapgs" ::: "memory");
}

extern size_t fpu_storage_size;
extern void (*fpu_save)(void *ctx);
extern void (*fpu_restore)(void *ctx);

static inline void wrxcr(uint32_t reg, uint64_t value) {
	uint32_t a = value;
	uint32_t d = value >> 32;
	asm volatile("xsetbv" ::"a"(a), "d"(d), "c"(reg) : "memory");
}

static inline void xsave(void *ctx) {
	asm volatile("xsave %0"
				 : "+m"(FLAT_PTR(ctx))
				 : "a"(0xffffffff), "d"(0xffffffff)
				 : "memory");
}

static inline void xrstor(void *ctx) {
	asm volatile("xrstor %0"
				 :
				 : "m"(FLAT_PTR(ctx)), "a"(0xffffffff), "d"(0xffffffff)
				 : "memory");
}

static inline void fxsave(void *ctx) {
	asm volatile("fxsave %0" : "+m"(FLAT_PTR(ctx)) : : "memory");
}

static inline void fxrstor(void *ctx) {
	asm volatile("fxrstor %0" : : "m"(FLAT_PTR(ctx)) : "memory");
}

void smp_init(struct limine_smp_response *smp_info);

#endif
