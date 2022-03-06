#ifndef MEM_H
#define MEM_H

/*
 * Copyright 2021, 2022 NSG650
 * Copyright 2021, 2022 Sebastian
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

#include <stddef.h>
#include <stdint.h>

#define memcpy __builtin_memcpy
#define memcmp __builtin_memcmp
#define memset __builtin_memset
#define memzero(a, b) memset(a, 0, b)

void *memmove(void *dest, const void *src, size_t n);
void strcpy(char *dest, char *src);
size_t strlen(char *string);
char *strncpy(char *dest, const char *src, size_t n);
char *ltoa(int64_t value, char *str, int base);
char *itoa(int value, char *str, int base);
char *ultoa(uint64_t value, char *str, int base);
char *uitoa(uint32_t value, char *str, int base);

#endif
