#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stddef.h>
#include <stdint.h>

void keyboard_write_config(uint8_t value);
size_t keyboard_gets(char *string, size_t count, bool fb);
char keyboard_getchar(void);
void keyboard_init(void);

#endif
