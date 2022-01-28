#ifndef DEBUG_H
#define DEBUG_H

#if defined(__x86_64__)
#include "../../arch/x86_64/include/serial/serial.h"
#define kputs_ serial_puts
#define kputchar_ serial_putchar
#endif

void kputchar(char c);
void kputs(char *string);
void kprintf(char *fmt, ...);

#endif
