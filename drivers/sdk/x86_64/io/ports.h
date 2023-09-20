#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

#if !(defined(__x86_64__))
#error "THIS IS ONLY FOR x86_64"
#endif

static inline void outb(uint16_t port, uint8_t val) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(val) : "memory");
}

static inline void outw(uint16_t port, uint16_t val) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(val) : "memory");
}

static inline void outd(uint16_t port, uint32_t val) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(val) : "memory");
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

static inline uint32_t ind(uint16_t port) {
	uint32_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

#endif
