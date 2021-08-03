#include "mem.h"

void *memcpy(void *dest, const void *src, size_t nbytes) {
	uint8_t *q = (uint8_t *)dest;
	uint8_t *p = (uint8_t *)src;
	uint8_t *end = p + nbytes;

	while (p != end) {
		*q++ = *p++;
	}

	return dest;
}

void *memccpy(void *restrict dest, const void *restrict src, int c, size_t n) {
	uint8_t *d = dest;
	const uint8_t *s = src;

	c = (uint8_t)c;
	for (; n && (*d = *s) != c; n--, s++, d++)
		;
	if (n) {
		return d + 1;
	}
	return 0;
}

void *mempcpy(void *dest, const void *src, size_t nbytes) {
	return (char *)memcpy(dest, src, nbytes) + nbytes;
}

void *memset(void *dest, uint8_t val, size_t len) {
	uint8_t *temp = (uint8_t *)dest;
	for (; len != 0; len--)
		*temp++ = val;
	return dest;
}

void *memmove(void *dest, const void *src, size_t nbytes) {
	uint8_t *p = (uint8_t *)src;
	uint8_t *q = (uint8_t *)dest;
	uint8_t *end = p + nbytes;

	if (q > p && q < end) {
		p = end;
		q += nbytes;

		while (p != src) {
			*--q = *--p;
		}
	} else {
		while (p != end) {
			*q++ = *p++;
		}
	}

	return dest;
}

void *memchr(const void *buf, int c, size_t n) {
	const uint8_t *p = (uint8_t *)buf;
	c = (uint8_t)c;

	for (; n && *p != c; p++, n--)
		;
	return n ? (void *)p : 0;
}

void *memrchr(const void *buf, int c, size_t n) {
	const uint8_t *s = buf;
	c = (uint8_t)c;
	while (n--) {
		if (s[n] == c) {
			return (void *)(s + n);
		}
	}

	return 0;
}

int memcmp(const void *s1, const void *s2, size_t n) {
	uint8_t *byte1 = (uint8_t *)s1;
	uint8_t *byte2 = (uint8_t *)s2;
	while ((*byte1 == *byte2) && (n > 0)) {
		++byte1;
		++byte2;
		--n;
	}

	if (n == 0) {
		return 0;
	}
	return *byte1 - *byte2;
}
