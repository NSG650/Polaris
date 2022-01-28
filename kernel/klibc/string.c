#include <klibc/mem.h>

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
