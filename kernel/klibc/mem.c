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

#include "mem.h"
#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
	uint8_t *d = dest;
	const uint8_t *s = src;

#ifdef __GNUC__
	typedef uint32_t __attribute__((__may_alias__)) u32;
	uint32_t w, x;

	for (; (uintptr_t)s % 4 && n; n--)
		*d++ = *s++;

	if ((uintptr_t)d % 4 == 0) {
		for (; n >= 16; s += 16, d += 16, n -= 16) {
			*(u32 *)(d + 0) = *(u32 *)(s + 0);
			*(u32 *)(d + 4) = *(u32 *)(s + 4);
			*(u32 *)(d + 8) = *(u32 *)(s + 8);
			*(u32 *)(d + 12) = *(u32 *)(s + 12);
		}
		if (n & 8) {
			*(u32 *)(d + 0) = *(u32 *)(s + 0);
			*(u32 *)(d + 4) = *(u32 *)(s + 4);
			d += 8;
			s += 8;
		}
		if (n & 4) {
			*(u32 *)(d + 0) = *(u32 *)(s + 0);
			d += 4;
			s += 4;
		}
		if (n & 2) {
			*d++ = *s++;
			*d++ = *s++;
		}
		if (n & 1) {
			*d = *s;
		}
		return dest;
	}

	if (n >= 32)
		switch ((uintptr_t)d % 4) {
			case 1:
				w = *(u32 *)s;
				*d++ = *s++;
				*d++ = *s++;
				*d++ = *s++;
				n -= 3;
				for (; n >= 17; s += 16, d += 16, n -= 16) {
					x = *(u32 *)(s + 1);
					*(u32 *)(d + 0) = (w >> 24) | (x << 8);
					w = *(u32 *)(s + 5);
					*(u32 *)(d + 4) = (x >> 24) | (w << 8);
					x = *(u32 *)(s + 9);
					*(u32 *)(d + 8) = (w >> 24) | (x << 8);
					w = *(u32 *)(s + 13);
					*(u32 *)(d + 12) = (x >> 24) | (w << 8);
				}
				break;
			case 2:
				w = *(u32 *)s;
				*d++ = *s++;
				*d++ = *s++;
				n -= 2;
				for (; n >= 18; s += 16, d += 16, n -= 16) {
					x = *(u32 *)(s + 2);
					*(u32 *)(d + 0) = (w >> 16) | (x << 16);
					w = *(u32 *)(s + 6);
					*(u32 *)(d + 4) = (x >> 16) | (w << 16);
					x = *(u32 *)(s + 10);
					*(u32 *)(d + 8) = (w >> 16) | (x << 16);
					w = *(u32 *)(s + 14);
					*(u32 *)(d + 12) = (x >> 16) | (w << 16);
				}
				break;
			case 3:
				w = *(u32 *)s;
				*d++ = *s++;
				n -= 1;
				for (; n >= 19; s += 16, d += 16, n -= 16) {
					x = *(u32 *)(s + 3);
					*(u32 *)(d + 0) = (w >> 8) | (x << 24);
					w = *(u32 *)(s + 7);
					*(u32 *)(d + 4) = (x >> 8) | (w << 24);
					x = *(u32 *)(s + 11);
					*(u32 *)(d + 8) = (w >> 8) | (x << 24);
					w = *(u32 *)(s + 15);
					*(u32 *)(d + 12) = (x >> 8) | (w << 24);
				}
				break;
		}
	if (n & 16) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}
	if (n & 8) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}
	if (n & 4) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
	}
	if (n & 2) {
		*d++ = *s++;
		*d++ = *s++;
	}
	if (n & 1) {
		*d = *s;
	}
	return dest;
#endif

	for (; n; n--)
		*d++ = *s++;
	return dest;
}

void *memset(void *dest, int c, size_t n) {
	uint8_t *s = dest;
	size_t k;

	if (!n)
		return dest;
	s[0] = c;
	s[n - 1] = c;
	if (n <= 2)
		return dest;
	s[1] = c;
	s[2] = c;
	s[n - 2] = c;
	s[n - 3] = c;
	if (n <= 6)
		return dest;
	s[3] = c;
	s[n - 4] = c;
	if (n <= 8)
		return dest;

	k = -(uintptr_t)s & 3;
	s += k;
	n -= k;
	n &= -4;

#ifdef __GNUC__
	typedef uint32_t __attribute__((__may_alias__)) u32;
	typedef uint64_t __attribute__((__may_alias__)) u64;

	u32 c32 = ((u32)-1) / 255 * (uint8_t)c;

	*(u32 *)(s + 0) = c32;
	*(u32 *)(s + n - 4) = c32;
	if (n <= 8)
		return dest;
	*(u32 *)(s + 4) = c32;
	*(u32 *)(s + 8) = c32;
	*(u32 *)(s + n - 12) = c32;
	*(u32 *)(s + n - 8) = c32;
	if (n <= 24)
		return dest;
	*(u32 *)(s + 12) = c32;
	*(u32 *)(s + 16) = c32;
	*(u32 *)(s + 20) = c32;
	*(u32 *)(s + 24) = c32;
	*(u32 *)(s + n - 28) = c32;
	*(u32 *)(s + n - 24) = c32;
	*(u32 *)(s + n - 20) = c32;
	*(u32 *)(s + n - 16) = c32;

	k = 24 + ((uintptr_t)s & 4);
	s += k;
	n -= k;

	u64 c64 = c32 | ((u64)c32 << 32);
	for (; n >= 32; n -= 32, s += 32) {
		*(u64 *)(s + 0) = c64;
		*(u64 *)(s + 8) = c64;
		*(u64 *)(s + 16) = c64;
		*(u64 *)(s + 24) = c64;
	}
#else
	for (; n; n--, s++)
		*s = c;
#endif

	return dest;
}

#ifdef __GNUC__
typedef __attribute__((__may_alias__)) size_t WT;
#define WS (sizeof(WT))
#endif

void *memmove(void *dest, const void *src, size_t n) {
	char *d = dest;
	const char *s = src;

	if (d == s)
		return d;
	if ((uintptr_t)s - (uintptr_t)d - n <= -2 * n)
		return memcpy(d, s, n);

	if (d < s) {
#ifdef __GNUC__
		if ((uintptr_t)s % WS == (uintptr_t)d % WS) {
			while ((uintptr_t)d % WS) {
				if (!n--)
					return dest;
				*d++ = *s++;
			}
			for (; n >= WS; n -= WS, d += WS, s += WS)
				*(WT *)d = *(WT *)s;
		}
#endif
		for (; n; n--)
			*d++ = *s++;
	} else {
#ifdef __GNUC__
		if ((uintptr_t)s % WS == (uintptr_t)d % WS) {
			while ((uintptr_t)(d + n) % WS) {
				if (!n--)
					return dest;
				d[n] = s[n];
			}
			while (n >= WS)
				n -= WS, *(WT *)(d + n) = *(WT *)(s + n);
		}
#endif
		while (n)
			n--, d[n] = s[n];
	}

	return dest;
}

void *memchr(const void *src, int c, size_t n) {
	const uint8_t *s = src;
	c = (uint8_t)c;
#ifdef __GNUC__
	for (; ((uintptr_t)s & (sizeof(size_t) - 1)) && n && *s != c; s++, n--)
		;
	if (n && *s != c) {
		typedef size_t __attribute__((__may_alias__)) word;
		const word *w;
		size_t k = ONES * c;
		for (w = (const void *)s; n >= (sizeof(size_t)) && !HASZERO(*w ^ k);
			 w++, n -= (sizeof(size_t)))
			;
		s = (const void *)w;
	}
#endif
	for (; n && *s != c; s++, n--)
		;
	return n ? (void *)s : 0;
}

int memcmp(const void *vl, const void *vr, size_t n) {
	const uint8_t *l = vl, *r = vr;
	for (; n && *l == *r; n--, l++, r++)
		;
	return n ? *l - *r : 0;
}
