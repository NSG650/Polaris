#include "ports.h"
uint8_t port_byte_in(unsigned short port) {
  uint8_t res;
  __asm__("in %%dx, %%al" : "=a"(res) : "d"(port));
  return res;
}

void port_byte_out(unsigned short port, unsigned char data) {
  __asm__("out %%al, %%dx" ::"a"(data), "d"(port));
}

uint16_t port_word_in(unsigned short port) {
  uint16_t res;
  __asm__("in %%dx, %%ax" : "=a"(res) : "d"(port));
  return res;
}

void port_word_out(unsigned short port, unsigned short data) {
  __asm__("out %%ax, %%dx" : : "a"(data), "d"(port));
}