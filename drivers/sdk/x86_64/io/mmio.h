#ifndef MMIO_H
#define MMIO_H

#if !(defined(__x86_64__))
#error "THIS IS ONLY FOR x86_64"
#endif

#include <asm/asm.h>
#include <stdint.h>

static inline void mmoutb(void *addr, uint8_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(BYTE_PTR(addr))
				 : "r"(value)
				 : "memory");
}

static inline void mmoutw(void *addr, uint16_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(WORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

static inline void mmoutd(void *addr, uint32_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(DWORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

static inline void mmoutq(void *addr, uint64_t value) {
	asm volatile("mov %0, %1\n\t"
				 : "=m"(QWORD_PTR(addr))
				 : "r"(value)
				 : "memory");
}

static inline uint8_t mminb(void *addr) {
	uint8_t ret;
	asm volatile("mov %0, %1\n\t" : "=r"(ret) : "m"(BYTE_PTR(addr)) : "memory");
	return ret;
}

static inline uint16_t mminw(void *addr) {
	uint16_t ret;
	asm volatile("mov %0, %1\n\t" : "=r"(ret) : "m"(WORD_PTR(addr)) : "memory");
	return ret;
}

static inline uint32_t mmind(void *addr) {
	uint32_t ret;
	asm volatile("mov %0, %1\n\t"
				 : "=r"(ret)
				 : "m"(DWORD_PTR(addr))
				 : "memory");
	return ret;
}

static inline uint64_t mminq(void *addr) {
	uint64_t ret;
	asm volatile("mov %0, %1\n\t"
				 : "=r"(ret)
				 : "m"(QWORD_PTR(addr))
				 : "memory");
	return ret;
}

#endif
