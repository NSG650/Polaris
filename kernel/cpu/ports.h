#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

uint8_t port_byte_in(unsigned short port);
void port_byte_out(unsigned short port, unsigned char data);
uint16_t port_word_in(unsigned short port);
void port_word_out(unsigned short port, unsigned short data);

#endif