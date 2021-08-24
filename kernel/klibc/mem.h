#ifndef MEM_H
#define MEM_H

/*
 * Copyright 2021 NSG650
 * Copyright 2021 Sebastian
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

void *memccpy(void *restrict dest, const void *restrict src, int c, size_t n);
void *memmove(void *dest, const void *src, size_t nbytes);
void *mempcpy(void *dest, const void *src, size_t nbytes);
void *memcpy(void *dest, const void *src, size_t nbytes);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *buf, int c, size_t n);
void *memset(void *dest, int val, size_t len);
void *memrchr(const void *m, int c, size_t n);

#endif
