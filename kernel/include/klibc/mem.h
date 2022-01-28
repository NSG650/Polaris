#ifndef MEM_H
#define MEM_H

#include <stddef.h>

#define memcpy __builtin_memcpy
#define memcmp __builtin_memcmp
#define memset __builtin_memset
#define memzero(a, b) memset(a, 0, b)

void strcpy(char *dest, char *src);
size_t strlen(char *string);

#endif
