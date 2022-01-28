#include <io/ports.h>

void outportb(uint16_t port, uint8_t val) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(val) : "memory");
}

uint8_t inportb(uint16_t port) {
	uint8_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}
