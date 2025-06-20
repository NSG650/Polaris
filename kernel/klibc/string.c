#include <klibc/mem.h>
#include <limits.h>
#include <mm/slab.h>

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

// Mostly taken from musl for optimizations
/*
 * Copyright © 2005-2020 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1 / UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x) - ONES) & ~(x) & HIGHS)

static char *__strchrnul(const char *s, int c) {
	c = (unsigned char)c;
	if (!c)
		return (char *)s + strlen(s);

#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	const word *w;
	for (; (uintptr_t)s % ALIGN; s++)
		if (!*s || *(unsigned char *)s == c)
			return (char *)s;
	size_t k = ONES * c;
	for (w = (void *)s; !HASZERO(*w) && !HASZERO(*w ^ k); w++)
		;
	s = (void *)w;
#endif
	for (; *s && *(unsigned char *)s != c; s++)
		;
	return (char *)s;
}

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

int strncmp(const char *_l, const char *_r, size_t n) {
	const uint8_t *l = (void *)_l, *r = (void *)_r;
	if (!n--)
		return 0;
	for (; *l && *r && n && *l == *r; l++, r++, n--)
		;
	return *l - *r;
}

char *strcat(char *restrict dest, const char *restrict src) {
	strcpy(dest + strlen(dest), src);
	return dest;
}

char *strdup(const char *s) {
	size_t l = strlen(s);
	char *d = kmalloc(l + 1);
	if (!d)
		return NULL;
	return memcpy(d, s, l + 1);
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

int64_t atol(const char *string) {
	long num = 0;
	int i = 0, sign = 1;

	// skip white space characters
	while (string[i] == ' ' || string[i] == '\n' || string[i] == '\t') {
		i++;
	}

	// note sign of the number
	if (string[i] == '+' || string[i] == '-') {
		if (string[i] == '-') {
			sign = -1;
		}
		i++;
	}

	// run till the end of the string is reached, or the
	// current character is non-numeric
	while (string[i] && (string[i] >= '0' && string[i] <= '9')) {
		num = num * 10 + (string[i] - '0');
		i++;
	}

	return sign * num;
}

size_t lfind(const char *str, const char accept) {
	size_t i = 0;
	while (str[i] != accept) {
		i++;
	}
	return (size_t)(str) + i;
}

#define BITOP(a, b, op)                                \
	((a)[(size_t)(b) / (8 * sizeof *(a))] op(size_t) 1 \
	 << ((size_t)(b) % (8 * sizeof *(a))))

size_t strspn(const char *s, const char *c) {
	const char *a = s;
	size_t byteset[32 / sizeof(size_t)] = {0};

	if (!c[0])
		return 0;
	if (!c[1]) {
		for (; *s == *c; s++)
			;
		return s - a;
	}

	for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
		;
	for (; *s && BITOP(byteset, *(unsigned char *)s, &); s++)
		;
	return s - a;
}

static size_t strcspn(const char *s, const char *c) {
	const char *a = s;
	size_t byteset[32 / sizeof(size_t)];

	if (!c[0] || !c[1])
		return __strchrnul(s, *c) - a;

	memset(byteset, 0, sizeof byteset);
	for (; *c && BITOP(byteset, *(unsigned char *)c, |=); c++)
		;
	for (; *s && !BITOP(byteset, *(unsigned char *)s, &); s++)
		;
	return s - a;
}

char *strtok_r(char *restrict s, const char *restrict sep, char **restrict p) {
	if (!s && !(s = *p))
		return NULL;
	s += strspn(s, sep);
	if (!*s)
		return *p = 0;
	*p = s + strcspn(s, sep);
	if (**p)
		*(*p)++ = 0;
	else
		*p = 0;
	return s;
}

char *strpbrk(const char *s, const char *b) {
	s += strcspn(s, b);
	return *s ? (char *)s : 0;
}

void strrev(char *str) {
	size_t i, j;
	char a;
	size_t len = strlen((const char *)str);
	for (i = 0, j = len - 1; i < j; i++, j--) {
		a = str[i];
		str[i] = str[j];
		str[j] = a;
	}
}

// Taken from https://stackoverflow.com/a/34957656

size_t strsplit(const char *txt, char delim, char ***tokens) {
	size_t *tklen, *t, count = 1;
	char **arr, *p = (char *)txt;

	while (*p != '\0')
		if (*p++ == delim)
			count += 1;
	t = tklen = kcalloc(count, sizeof(size_t));
	for (p = (char *)txt; *p != '\0'; p++)
		*p == delim ? *t++ : (*t)++;
	*tokens = arr = kmalloc(count * sizeof(char *));
	t = tklen;
	p = *arr++ = kcalloc(*(t++) + 1, sizeof(char *));
	while (*txt != '\0') {
		if (*txt == delim) {
			p = *arr++ = kcalloc(*(t++) + 1, sizeof(char *));
			txt++;
		} else
			*p++ = *txt++;
	}
	kfree(tklen);
	return count;
}
