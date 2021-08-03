#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <stdint.h>

typedef char symbol[];

void *memccpy(void *restrict dest, const void *restrict src, int c, size_t n);
void *memmove(void *dest, const void *src, size_t nbytes);
void *mempcpy(void *dest, const void *src, size_t nbytes);
void *memcpy(void *dest, const void *src, size_t nbytes);
int memcmp(const void *s1, const void *s2, size_t n);
void *memset(void *dest, uint8_t val, size_t len);
void *memchr(const void *buf, int c, size_t n);
void *memrchr(const void *m, int c, size_t n);

#endif
