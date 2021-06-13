#include "string.h"
#include <limits.h>
#include "ctype.h"
#include "mem.h"

#define ALIGN      (sizeof(size_t))
#define ONES       ((size_t)-1 / UCHAR_MAX)
#define HIGHS      (ONES * (UCHAR_MAX / 2 + 1))
#define HASZERO(x) (((x)-ONES) & ~(x)&HIGHS)
#define BITOP(a, b, op)                                \
    ((a)[(size_t)(b) / (8 * sizeof *(a))] op(size_t) 1 \
     << ((size_t)(b) % (8 * sizeof *(a))))

char *int_to_ascii(int n, char str[]) {
    int i, sign;
    if ((sign = n) < 0) {
        n = -n;
    }

    i = 0;
    do {
        str[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) {
        str[i++] = '-';
    }

    str[i] = '\0';

    reverse(str);

    return (str);
}

char *hex_to_ascii(int n, char str[]) {
    char zeros = 0;

    int i;
    for (i = 28; i >= 0; i -= 4) {
        int32_t tmp;
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && zeros == 0) {
            continue;
        }
        zeros = 1;
        if (tmp >= 0xA) {
            append(str, tmp - 0xA + 'a');
        } else {
            append(str, tmp + '0');
        }
    }

    return (str);
}

char *hex_to_ascii_upper(int n, char str[]) {
    char zeros = 0;

    int32_t tmp;
    int i;
    for (i = 28; i >= 0; i -= 4) {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && zeros == 0) {
            continue;
        }
        zeros = 1;
        if (tmp >= 0xA) {
            append(str, tmp - 0xA + 'A');
        } else {
            append(str, tmp + '0');
        }
    }

    return (str);
}

char *alt_hex_to_ascii(int n, char str[]) {
    append(str, '0');
    append(str, 'x');
    char zeros = 0;

    int i;
    for (i = 28; i >= 0; i -= 4) {
        int32_t tmp;
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && zeros == 0) {
            continue;
        }
        zeros = 1;
        if (tmp >= 0xA) {
            append(str, tmp - 0xA + 'a');
        } else {
            append(str, tmp + '0');
        }
    }

    return (str);
}

char *alt_hex_to_ascii_upper(int n, char str[]) {
    append(str, '0');
    append(str, 'x');
    char zeros = 0;

    int32_t tmp;
    int i;
    for (i = 28; i >= 0; i -= 4) {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && zeros == 0) {
            continue;
        }
        zeros = 1;
        if (tmp >= 0xA) {
            append(str, tmp - 0xA + 'A');
        } else {
            append(str, tmp + '0');
        }
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

char *reverse(char s[]) {
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
    s[len + 1] = '\0';
}

void backspace(char s[]) {
    int len = strlen(s);
    s[len - 1] = '\0';
}

int strcmp(const char s1[], const char s2[]) {
    int i;

    for (i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0')
            return 0;
    }

    return s1[i] - s2[i];
}

int strncmp(const char s1[], const char s2[], size_t n) {
    int i;

    for (i = 0; n && s1[i] == s2[i]; ++i, --n) {
        if (s1[i] == '\0') {
            return 0;
        }
    }

    return (s1[i] - s2[i]);
}

size_t strlen(const char *s) {
    const char *a = s;
#ifdef __GNUC__
    typedef size_t __attribute__((__may_alias__)) word;
    const word *w;
    for (; (uintptr_t)s % ALIGN; s++) {
        if (!*s) {
            return s - a;
        }
    }

    for (w = (const void *)s; !HASZERO(*w); w++)
        ;
    s = (const void *)w;
#endif
    for (; *s; s++)
        ;
    return s - a;
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

    for (; n && (*d = *s); n--, s++, d++)
        ;
    *d = 0;
finish:
    return d - d0 + strlen(s);
}

size_t strlcat(char *d, const char *s, size_t n) {
    size_t l = strnlen(d, n);
    if (l == n) {
        return l + strlen(s);
    }

    return l + strlcpy(d + l, s, n - l);
}

char *strncpy(char *restrict dest, const char *restrict src, size_t n) {
    stpncpy(dest, src, n);
    return dest;
}

char *stpcpy(char *restrict d, const char *restrict s) {
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

char *stpncpy(char *restrict d, const char *restrict s, size_t n) {
#ifdef __GNUC__
    typedef size_t __attribute__((__may_alias__)) word;
    word *wd;
    const word *ws;
    if (((uintptr_t)s & ALIGNM) == ((uintptr_t)d & ALIGNM)) {
        for (; ((uintptr_t)s & ALIGNM) && n && (*d = *s); n--, s++, d++)
            ;
        if (!n || !*s) {
            goto tail;
        }

        wd = (void *)d;
        ws = (const void *)s;
        for (; n >= sizeof(size_t) && !HASZERO(*ws);
             n -= sizeof(size_t), ws++, wd++) {
            *wd = *ws;
        }

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

static chat *twobyte_strstr(const uint8_t *h, cost uint8_t *n) {
    uint16_t nw = n[0] << 8 | n[1], hw = h[0] << 8 | h[1];
    for (h++; *h && hw != nw; hw = hw << 8 | *++h)
        ;
    return *h ? (char *)h - 1 : 0;
}

static char *threebyte_strstr(const uint8_t *h, const uint8_t *n) {
    uint32_t nw = (uint32_t)n[0] << 24 | n[1] << 16 | n[2] << 8;
    uint32_t hw = (uint32_t)h[0] << 24 | h[1] << 16 | h[2] << 8;
    for (h += 2; *h && hw != nw; hw = (hw | *++h) << 8)
        ;
    return *h ? (char *)h - 2 : 0;
}

static char *fourbyte_strstr(const uint8_t *h, const uint8_t *n) {
    uint32_t nw = (uint32_t)n[0] << 24 | n[1] << 16 | n[2] << 8 | n[3];
    uint32_t hw = (uint32_t)h[0] << 24 | h[1] << 16 | h[2] << 8 | h[3];
    for (h += 3; *h && hw != nw; hw = hw << 8 | *++h)
        ;
    return *h ? (char *)h - 3 : 0;
}

static char *twoway_strstr(const uint8_t *h, const uint8_t *n) {
    const uint8_t *z;
    size_t l, ip, jp, k, p, ms, p0, mem, mem0;
    size_t byteset[32 / sizeof(size_t)] = {0};
    size_t shift[256];

    // Computing length of needle and fill shift table
    for (l = 0; n[l] && h[l]; l++) {
        BITOP(byteset, n[l], |=), shift[n[l]] = l + 1;
    }

    if (n[l]) {
        return 0; // Hit the end of h
    }

    // Compute maximal suffix
    ip = -1;
    jp = 0;
    k = p = 1;
    while (jp + k < l) {
        if (n[ip + k] == n[jp + k]) {
            if (k == p) {
                jp += p;
                k = 1;
            } else
                k++;
        } else if (n[ip + k] > n[jp + k]) {
            jp += k;
            k = 1;
            p = jp - ip;
        } else {
            ip = jp++;
            k = p = 1;
        }
    }
    ms = ip;
    p0 = p;

    // And with the opposite comparison
    ip = -1;
    jp = 0;
    k = p = 1;
    while (jp + k < l) {
        if (n[ip + k] == n[jp + k]) {
            if (k == p) {
                jp += p;
                k = 1;
            } else
                k++;
        } else if (n[ip + k] < n[jp + k]) {
            jp += k;
            k = 1;
            p = jp - ip;
        } else {
            ip = jp++;
            k = p = 1;
        }
    }

    if (ip + 1 > ms + 1) {
        ms = ip;
    } else {
        p = p0;
    }

    // Periodic needle?
    if (memcmp(n, n + p, ms + 1)) {
        mem0 = 0;
        p = max(ms, l - ms - 1) + 1;
    } else {
        mem0 = l - p;
    }
    mem = 0;

    // Initialize incremental end-of-haystack pointer
    z = h;

    // Search loop
    for (;;) {
        // Update incremental end-of-haystack pointer
        if (z - h < (int32_t)l) {
            // Fast estimate for max(l, 63)
            size_t grow = l | 63;
            const uint8_t *z2 = memchr(z, 0, grow);
            if (z2) {
                z = z2;
                if (z - h < (int32_t)l)
                    return 0;
            } else {
                z += grow;
            }
        }

        // Check last byte first; advance by shift on mismatch
        if (BITOP(byteset, h[l - 1], &)) {
            k = l - shift[h[l - 1]];
            if (k) {
                if (k < mem)
                    k = mem;
                h += k;
                mem = 0;
                continue;
            }
        } else {
            h += l;
            mem = 0;
            continue;
        }

        // Compare right half
        for (k = max(ms + 1, mem); n[k] && n[k] == h[k]; k++)
            ;
        if (n[k]) {
            h += k - ms;
            mem = 0;
            continue;
        }

        // Compare left half
        for (k = ms + 1; k > mem && n[k - 1] == h[k - 1]; k--)
            ;
        if (k <= mem) {
            return (char *)h;
        }
        h += p;
        mem = mem0;
    }
}

char *strstr(const char *h, const char *n) {
    // Return immediately on empty needle
    if (!n[0]) {
        return (char *)h;
    }

    // Use faster algorithms for short needles
    h = strchr(h, *n);
    if (!h || !n[1]) {
        return (char *)h;
    }

    if (!h[1]) {
        return 0;
    }

    if (!n[2]) {
        return twobyte_strstr((void *)h, (void *)n);
    }

    if (!h[2]) {
        return 0;
    }

    if (!n[3]) {
        return threebyte_strstr((void *)h, (void *)n);
    }

    if (!h[3]) {
        return 0;
    }

    if (!n[4]) {
        return fourbyte_strstr((void *)h, (void *)n);
    }

    return twoway_strstr((void *)h, (void *)n);
}

char *strrstr(const char *h, const char *n) {
    char *r = NULL;

    if (!n[0]) {
        return (char *)h + strlen(h);
    }

    while (1) {
        char *p = strstr(h, n);
        if (!p) {
            return r;
        }
        r = p;
        h = p + 1;
    }
}

char *strchrnul(const char *s, int c) {
    c = (uint8_t)c;
    if (!c) {
        return (char *)s + strlen(s);
    }

#ifdef __GNUC__
    typedef size_t __attribute__((__may_alias__)) word;
    const word *w;
    for (; (uintptr_t)s % ALIGN; s++) {
        if (!*s || *(uint8_t *)s == c) {
            return (char *)s;
        }
    }
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

char *strrchr(const char *s, int c) {
    return memrchr(s, c, strlen(s) + 1);
}

size_t strspn(const char *s, const char *c) {
    const char *a = s;
    size_t byteset[32 / sizeof(size_t)] = {0};

    if (!c[0]) {
        return 0;
    }

    if (!c[1]) {
        for (; *s == *c; s++)
            ;
        return s - a;
    }

    for (; *c && BITOP(byteset, *(uint8_t *)c, |=); c++)
        ;
    for (; *s && BITOP(byteset, *(uint8_t *)s, &); s++)
        ;
    return s - a;
}

size_t strcspn(const char *s, const char *c) {
    const char *a = s;
    size_t byteset[32 / sizeof(size_t)];

    if (!c[0] || !c[1]) {
        return strchrnul(s, *c) - a;
    }

    memset(byteset, 0, sizeof byteset);
    for (; *c && BITOP(byteset, *(uint8_t *)c, |=); c++)
        ;
    for (; *s && !BITOP(byteset, *(uint8_t *)s, &); s++)
        ;
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

char *strtok_r(char *restrict s, const char *restrict sep, char **restrict p) {
    if (!s && !(s = *p)) {
        return NULL;
    }

    s += strspn(s, sep);
    if (!*s) {
        return *p = 0;
    }

    *p = s + strcspn(s, sep);
    if (**p) {
        *(*p)++ = 0;
    } else {
        *p = 0;
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

char *strsep(char **str, const char *sep) {
    char *s = *str, *end;

    if (!s) {
        return NULL;
    }

    end = s + strcspn(s, sep);
    if (*end) {
        *end++ = 0;
    } else {
        end = 0;
    }

    *str = end;
    return s;
}

int strverscmp(const char *l0, const char *r0) {
    const uint8_t *l = (const void *)l0;
    const uint8_t *r = (const void *)r0;
    size_t i, dp, j;
    int z = 1;

    for (dp = i = 0; l[i] == r[i]; i++) {
        int c = l[i];
        if (!c) {
            return 0;
        }

        if (!isdigit(c)) {
            dp = i + 1;
            z = 1;
        } else if (c != '0') {
            z = 0;
        }
    }

    if (l[dp] != '0' && r[dp] != '0') {
        for (j = i; isdigit(l[j]); j++) {
            if (!isdigit(r[j])) {
                return 1;
            }
        }

        if (isdigit(r[j])) {
            return -1;
        }
    } else if (z && dp < i && (isdigit(l[i]) || isdigit(r[i]))) {
        return (uint8_t)(l[i] - '0') - (uint8_t)(r[i] - '0');
    }

    return l[i] - r[i];
}

static int strncasecmp(const char *_l, const char *_r, size_t n) {
    const uint8_t *l = (void *)_l, *r = (void *)_r;
    if (!n--) {
        return 0;
    }
    for (; *l && *r && n && (*l == *r || tolower(*l) == tolower(*r));
         l++, r++, n--)
        ;
    return tolower(*l) - tolower(*r);
}

char *strcasestr(const char *h, const char *n) {
    size_t l = strlen(n);
    for (; *h; h++) {
        if (!strncasecmp(h, n, l)) {
            return (char *)h;
        }
    }

    return 0;
}

char *strtruncate(char *str, int n) {
    if (n <= 0) {
        return str;
    }

    int l = n;
    int len = strlen(str);
    if (n > len) {
        l = len;
    }

    str[len - l] = '\0';
    return str;
}
