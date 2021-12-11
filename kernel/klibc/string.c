/*
 * Copyright 2021 Misha
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

#include "string.h"
#include "mem.h"
#include <limits.h>

#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x)-ONES) & ~(x)&HIGHS)

// Mostly grabbed from Musl
int strcmp(const char *l, const char *r) {
	for (; *l == *r && *l; l++, r++)
		;
	return *(uint8_t *)l - *(uint8_t *)r;
}

int strncmp(const char *_l, const char *_r, size_t n) {
	const uint8_t *l = (void *)_l, *r = (void *)_r;
	if (!n--)
		return 0;
	for (; *l && *r && n && *l == *r; l++, r++, n--)
		;
	return *l - *r;
}

size_t strlen(const char *s) {
	const char *a = s;
#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	const word *w;
	for (; (uintptr_t)s % sizeof(size_t); s++)
		if (!*s)
			return s - a;
	for (w = (const void *)s; !HASZERO(*w); w++)
		;
	s = (const void *)w;
#endif
	for (; *s; s++)
		;
	return s - a;
}

char *strcpy(char *restrict d, const char *restrict s) {
#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	word *wd;
	const word *ws;
	if ((uintptr_t)s % sizeof(size_t) == (uintptr_t)d % sizeof(size_t)) {
		for (; (uintptr_t)s % sizeof(size_t); s++, d++)
			if (!(*d = *s))
				return d;
		wd = (void *)d;
		ws = (const void *)s;
		for (; !HASZERO(*ws); *wd++ = *ws++)
			;
		d = (void *)wd;
		s = (const void *)ws;
	}
#endif
	for (; (*d = *s); s++, d++)
		;

	return d;
}

char *strncpy(char *restrict d, const char *restrict s, size_t n) {
#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	word *wd;
	const word *ws;
	if (((uintptr_t)s & (sizeof(size_t) - 1)) ==
		((uintptr_t)d & (sizeof(size_t) - 1))) {
		for (; ((uintptr_t)s & (sizeof(size_t) - 1)) && n && (*d = *s);
			 n--, s++, d++)
			;
		if (!n || !*s)
			goto tail;
		wd = (void *)d;
		ws = (const void *)s;
		for (; n >= sizeof(size_t) && !HASZERO(*ws);
			 n -= sizeof(size_t), ws++, wd++)
			*wd = *ws;
		d = (void *)wd;
		s = (const void *)ws;
	}
#endif
	for (; n && (*d = *s); n--, s++, d++)
		;
tail:
	memset(d, 0, n);
	return d;
}

char *strchrnul(const char *s, int c) {
	c = (uint8_t)c;
	if (!c)
		return (char *)s + strlen(s);

#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	const word *w;
	for (; (uintptr_t)s % sizeof(size_t); s++)
		if (!*s || *(uint8_t *)s == c)
			return (char *)s;
	size_t k = ONES * c;
	for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w ^ k); w++)
		;
	s = (void *)w;
#endif
	for (; *s && *(uint8_t *)s != c; s++)
		;
	return (char *)s;
}

char *strchr(const char *s, int c) {
	char *r = strchrnul(s, c);
	return *(uint8_t *)r == (uint8_t)c ? r : 0;
}
