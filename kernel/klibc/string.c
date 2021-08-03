#include "string.h"
#include "ctype.h"
#include "mem.h"
#include "../klibc/printf.h"

#define BITOP(a,b,op) \
 ((a)[(size_t)(b) / (8*sizeof *(a))] op (size_t)1 << ((size_t)(b) % (8*sizeof *(a))))

/**
 * K&R implementation
 */
char *int_to_ascii(int n, char str[]) {
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do {
        str[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) str[i++] = '-';
    str[i] = '\0';

    reverse(str);

    return (str);
}

char *long_to_ascii(unsigned long long n, char str[]) {
    int i, sign;
    if ((sign = n) < 0) n = -n;
    i = 0;
    do {
        str[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) str[i++] = '-';
    str[i] = '\0';

    reverse(str);

    return (str);
}


char *hex_to_ascii(int n, char str[]) {
    append(str, '0');
    append(str, 'x');
    char zeros = 0;

    int i;
    for (i = 28; i >= 0; i -= 4) {
        int32_t tmp;
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && zeros == 0) continue;
        zeros = 1;
        if (tmp >= 0xA) append(str, tmp - 0xA + 'a');
        else append(str, tmp + '0');
    }

    return (str);
}

char *hex_to_ascii_upper(int n, char str[]) {
    append(str, '0');
    append(str, 'x');
    char zeros = 0;

    int32_t tmp;
    int i;
    for (i = 28; i >= 0; i -= 4) {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && zeros == 0) continue;
        zeros = 1;
        if (tmp >= 0xA) append(str, tmp - 0xA + 'A');
        else append(str, tmp + '0');
    }

    return (str);
}

char *octal_to_ascii(int n) {
    static char representation[] = "01234567";
    static char buffer[50];
    char *str;

    str = &buffer[49];
    *str = '\0';

    do {
        *--str = representation[n % 8];
        n /= 8;
    } while (n != 0);

    return (str);
}

char *itoa( int n, char *str1, unsigned int base ) {
	int i = 0;
	unsigned char negative = 0;
	if (n == 0) {
		str1[ i++ ] = '0';
		str1[ i ] = 0;

		return str1;
	}

	if (base == 10 && n < 0) {
		negative = 1;
		n = -n;
	}

	while (n != 0) {
		int r = n % base;
		str1[i++] = ((char)r > 9) ? ((char)r - 10) + 'a' : (char)r + '0';
		n /= base;
	}

	if (negative)
		str1[i++] = '-';

	str1[i] = 0;

	return strrev( str1 );
}

int startswith(char *str, char *accept) {
    size_t s = strlen(accept);

    for (size_t i = 0; i < s; ++i) {
        if (*str != *accept) {
            return 0;
        }
        str++;
        accept++;
    }

    return 1;
}

char* strrev(char s[]) {
    return reverse(s);
}

char* reverse(char s[]) {
    int i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        int c;
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }

    return s;
}

void swap(int a, int b) {
    int t = a;
    a = b;
    b = t;
}

void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

void backspace(char s[]) {
    int len = strlen(s);
    s[len-1] = '\0';
}

/* K&R
 * Returns <0 if s1<s2, 0 if s1==s2, >0 if s1>s2 */
int strcmp(const char s1[], const char s2[]) {
    int i;

    for (i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0') return 0;
    }

    return s1[i] - s2[i];
}

int strncmp(const char *s1, const char *s2, size_t n) {
    int count = 0;
	while (count < n)
	{
		if (s1[count] == s2[count])
		{
			if (s1[count] == '\0')
			{
                //Found null termination
                return 0;
            }
			else
            {
                count++;
            }
		}
		else
			return s1[count] - s2[count];
	}

    return 0;
}

size_t strlen(const char *s) {
    int i = 0;

    while (s[i] != '\0') {
        ++i;
    }

    return i;
}

size_t strnlen(const char *s, int32_t n) {
    const char *p = memchr(s, 0, n);
    return p ? p - s : n;
}

char *strcpy(char *restrict dest, const char *restrict src) {
    stpcpy(dest, src);
    return dest;
}

size_t strlcpy(char *d, const char *s, size_t n) {
    char *d0 = d;

    if (!n--) {
        goto finish;
    }

    for (; n && (*d = *s); n--, s++, d++);
    *d = 0;
finish:
    return d - d0 + strlen(s);
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *ptr = dest;

    while (*src != '\0' && n--) {
        *ptr++ = *src++;
    }

    *ptr = '\0';
    return dest;
}

char *stpcpy(char *restrict d, const char *restrict s) {
    for (; (*d = *s); s++, d++);

    return d;
}

char *stpncpy(char *restrict d, const char *restrict s, size_t n) {
    for (; n && (*d = *s); n--, s++, d++);
    memset(d, 0, n);
    return d;
}

char *strcat(char *restrict dest, const char *restrict src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

char *strncat(char *restrict dest, const char *restrict src, size_t n) {
    char *ptr = dest + strlen(dest);

    while (n && *src) {
        n--;
        *ptr++ = *src++;
    }

    *ptr++ = '\0';
    return dest;
}

char *strstr(const char *in, const char *str) {
    char c;
    uint32_t len;

    c = *str++;
    if (!c) {
        return (char *) in;
    }

    len = strlen(str);
    do {
        char sc;

        do {
            sc = *in++;
            if (!sc) {
                return (char *) 0;
            }
        } while (sc != c);
    } while (strncmp(in, str, len) != 0);

    return (char *) (in - 1);
}

char *strchrnul(const char *s, int c) {
    c = (uint8_t)c;
    if (!c) {
        return (char *)s + strlen(s);
    }

    for (; *s && *(uint8_t *)s != c; s++);
    return (char *)s;
}

char *strchr(const char *s, int c) {
    char *r = strchrnul(s, c);
    return *(uint8_t *)r == (uint8_t)c ? r : 0;
}

char *strrchr(const char *s, int c) {
    return memrchr(s, c, strlen(s) + 1);
}

size_t strspn(const char *s, const char *c) {
    const char *a = s;
    size_t byteset[32 / sizeof(size_t)] = { 0 };

    if (!c[0]) {
        return 0;
    }

    if (!c[1]) {
        for (; *s == *c; s++);
        return s - a;
    }

    for (; *c && BITOP(byteset, *(uint8_t *)c, |=); c++);
    for (; *s && BITOP(byteset, *(uint8_t *)s, &); s++);
    return s - a;
}

size_t strcspn(const char *s, const char *c) {
    const char *a = s;
    size_t byteset[32 / sizeof(size_t)];

    if (!c[0] || !c[1]) {
        return strchrnul(s, *c) - a;
    }

    memset(byteset, 0, sizeof byteset);
    for (; *c && BITOP(byteset, *(uint8_t *)c, |=); c++);
    for (; *s && !BITOP(byteset, *(uint8_t *)s, &); s++);
    return s - a;
}

char *strtok(char *restrict s, const char *restrict sep) {
    static char *p;
    if (!s && !(s = p)) {
        return NULL;
    }

    s += strspn(s, sep);
    if (!*s) {
        p = 0;
    }

    p = s + strcspn(s, sep);
    if (*p) {
        *p++ = 0;
    } else {
        p = 0;
    }

    return s;
}

char *strpbrk(const char *s, const char *b) {
    s += strcspn(s, b);
    return *s ? (char *)s : 0;
}

int strcoll(const char *l, const char *r) {
    return strcmp(l, r);
}

size_t strxfrm(char *restrict dest, const char *restrict src, size_t n) {
    size_t l = strlen(src);
    if (n > l) {
        strcpy(dest, src);
    }
    return l;
}

char *strtruncate(char *str, int n) {
    if (n <= 0) return str;
    int l = n;
    int len = strlen(str);
    if (n > len) l = len;
    str[len-l] = '\0';
    return str;
}


