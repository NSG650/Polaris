#include <klibc/mem.h>
#include <limits.h>

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

// Mostly taken from musl for optimizations
#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x)-ONES) & ~(x)&HIGHS)

char *strcpy(char *restrict d, const char *restrict s) {
#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	word *wd;
	const word *ws;
	if ((uintptr_t)s % ALIGN == (uintptr_t)d % ALIGN) {
		for (; (uintptr_t)s % ALIGN; s++, d++)
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

int strcmp(const char *l, const char *r) {
	for (; *l == *r && *l; l++, r++)
		;
	return *(unsigned char *)l - *(unsigned char *)r;
}

size_t strlen(const char *s) {
	const char *a = s;
#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	const word *w;
	for (; (uintptr_t)s % ALIGN; s++)
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

char *strncpy(char *restrict d, const char *restrict s, size_t n) {
#undef ALIGN
#define ALIGN (sizeof(size_t) - 1)
#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	word *wd;
	const word *ws;
	if (((uintptr_t)s & ALIGN) == ((uintptr_t)d & ALIGN)) {
		for (; ((uintptr_t)s & ALIGN) && n && (*d = *s); n--, s++, d++)
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

char *strcat(char *dest, char *src) {
	char* ptr = dest + strlen(dest);
	while (*src != '\0') {
        *ptr++ = *src++;
    }
	*ptr = '\0';
	return dest;
}

char *ltoa(int64_t value, char *str, int base) {
	char *rc;
	char *ptr;
	char *low;
	// Check for supported base.
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	rc = ptr = str;
	// Set '-' for negative decimals.
	if (value < 0 && base == 10) {
		*ptr++ = '-';
	}
	// Remember where the numbers start.
	low = ptr;
	// The actual conversion.
	do {
		// Modulo is negative for negative value. This trick makes abs()
		// unnecessary.
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnop"
				 "qrstuvwxyz"[35 + value % base];
		value /= base;
	} while (value);
	// Terminating the string.
	*ptr-- = '\0';
	// Invert the numbers.
	while (low < ptr) {
		char tmp = *low;
		*low++ = *ptr;
		*ptr-- = tmp;
	}
	return rc;
}

char *ultoa(uint64_t value, char *str, int base) {
	char *rc;
	char *ptr;
	char *low;
	// Check for supported base.
	if (base < 2 || base > 36) {
		*str = '\0';
		return str;
	}
	rc = ptr = str;
	// Remember where the numbers start.
	low = ptr;
	// The actual conversion.
	do {
		*ptr++ = "0123456789abcdefghijklmnop"
				 "qrstuvwxyz"[value % base];
		value /= base;
	} while (value);
	// Terminating the string.
	*ptr-- = '\0';
	// Invert the numbers.
	while (low < ptr) {
		char tmp = *low;
		*low++ = *ptr;
		*ptr-- = tmp;
	}
	return rc;
}
