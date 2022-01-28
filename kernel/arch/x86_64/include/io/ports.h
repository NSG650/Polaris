#ifndef PORTS_H
#define PORTS_H

#include <stddef.h>
#include <stdint.h>

void outportb(uint16_t port, uint8_t val);
uint8_t inportb(uint16_t port);

#endif
