#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

size_t strxfrm(char *restrict dest, const char *restrict src, size_t n);
char *strncat(char *restrict dest, const char *restrict src, size_t n);
char *stpncpy(char *restrict d, const char *restrict s, size_t n);
char *strcat(char *restrict dest, const char *restrict src);
char *strcpy(char *restrict dest, const char *restrict src);
char *strtok(char *restrict s, const char *restrict sep);
int strncmp(const char s1[], const char s2[], size_t n);
char *stpcpy(char *restrict d, const char *restrict s);
char *strncpy(char *dest, const char *src, size_t n);
size_t strlcpy(char *d, const char *s, size_t n);
char *strstr(const char *in, const char *str);
int strcmp(const char s1[], const char s2[]);
size_t strcspn(const char *s, const char *c);
size_t strspn(const char *s, const char *c);
char *strpbrk(const char *s, const char *b);
int strcoll(const char *l, const char *r);
size_t strnlen(const char *s, int32_t n);
char *strchrnul(const char *s, int c);
char *strtruncate(char *str, int n);
char *strrchr(const char *s, int c);
char *strchr(const char *s, int c);
size_t strlen(const char s[]);
char *strrev(char s[]);

char *hex_to_ascii_upper(int n, char str[]);
char *hex_to_ascii(int n, char str[]);
char *int_to_ascii(int n, char str[]);
char *long_to_ascii(unsigned long long n, char str[]);
char *octal_to_ascii(int n);
int atoi(const char *str);

int startswith(char *str, char *accept);
void append(char s[], char n);
void backspace(char s[]);
void swap(int a, int b);
char *reverse(char s[]);

#endif
