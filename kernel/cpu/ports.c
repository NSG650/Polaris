#include "ports.h"
uint8_t port_byte_in(unsigned short port) {
   uint8_t ret;
    asm volatile (
        "in %0, %1\n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

void port_byte_out(unsigned short port, unsigned char data) {
   asm volatile (
        "out %0, %1\n\t"
        :
        : "d"(port), "a"(data)
        : "memory"
    );
}

uint16_t port_word_in(unsigned short port) {
   uint16_t ret;
    asm volatile (
        "in %0, %1\n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

void port_word_out(unsigned short port, unsigned short data) {
   asm volatile (
        "out %0, %1\n\t"
        :
        : "d"(port), "a"(data)
        : "memory"
    );
}
