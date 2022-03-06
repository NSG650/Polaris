#include <klibc/mem.h>

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

void strcpy(char *dest, char *src) {
	for (; (*dest = *src); src++, dest++)
		;
}

int strcmp(const char s1[], const char s2[]) {
	int i;

	for (i = 0; s1[i] == s2[i]; i++) {
		if (s1[i] == '\0')
			return 0;
	}

	return s1[i] - s2[i];
}

size_t strlen(char *string) {
	size_t len = 0;
	while (string[len] != '\0')
		++len;
	return len;
}

char *strncpy(char *dest, const char *src, size_t n) {
	char *ptr = dest;

	while (*src != '\0' && n--) {
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

char *itoa(int value, char *str, int base) {
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

char *uitoa(uint32_t value, char *str, int base) {
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
