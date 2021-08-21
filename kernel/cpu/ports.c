#include "ports.h"

uint8_t port_byte_in(uint16_t port) {
	uint8_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

void port_byte_out(uint16_t port, uint8_t data) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(data) : "memory");
}

uint16_t port_word_in(uint16_t port) {
	uint16_t ret;
	asm volatile("in %0, %1\n\t" : "=a"(ret) : "d"(port) : "memory");
	return ret;
}

void port_word_out(uint16_t port, uint16_t data) {
	asm volatile("out %0, %1\n\t" : : "d"(port), "a"(data) : "memory");
}
