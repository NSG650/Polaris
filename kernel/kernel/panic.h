#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>

void panic(char message[], char file[], char assert, uint32_t line);

#define PANIC(b)  (panic(b, __FILE__, 0, __LINE__));
#define ASSERT(b) ((b) ? (void)0 : panic(#b, __FILE__, 1, __LINE__));

#endif
