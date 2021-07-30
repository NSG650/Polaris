#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define COM1 0x3F8

void write_serial_port_char(uint16_t PORT, char word);
void write_serial_port(uint16_t PORT, char *word);
int serial_install_port(uint16_t PORT);
void write_serial_char(char word);
char read_serial(uint16_t PORT);
void write_serial(char *word);
void serial_install();

#endif
