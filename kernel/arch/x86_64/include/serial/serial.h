#ifndef SERIAL_H
#define SERIAL_H

#define COM1 0x3F8

void serial_init(void);
void serial_putchar(char c);
void serial_puts(char *string);

#endif
