#ifndef MEM_H
#define MEM_H

/*
 * Copyright 2021 - 2023 NSG650
 * Copyright 2021 - 2023 Neptune
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define memzero(a, b) memset(a, 0, b)

void *memcpy(void *d, const void *s, size_t n);
void *memset32(void *d, uint32_t c, size_t n);
void *memset(void *d, int c, size_t n);
int memcmp(const void *l, const void *r, size_t n);
void *memmove(void *d, const void *s, size_t n);
char *strncpy(char *restrict d, const char *restrict s, size_t n);
char *strcpy(char *restrict d, const char *restrict s);
int strcmp(const char *l, const char *r);
int strncmp(const char *_l, const char *_r, size_t n);
char *strcat(char *restrict dest, const char *restrict src);
size_t strlen(const char *s);
char *ltoa(int64_t value, char *str, int base);
char *ultoa(uint64_t value, char *str, int base);
int64_t atol(const char *string);
char *strtok_r(char *restrict s, const char *restrict sep, char **restrict p);
size_t strspn(const char *s, const char *c);
char *strpbrk(const char *s, const char *b);
void strrev(char *str);
size_t strsplit(const char *txt, char delim, char ***tokens);
char *strdup(const char *s);

#endif
