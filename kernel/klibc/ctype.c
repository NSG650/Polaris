#include "ctype.h"
int isblank(int c) {
    return c == '\t' || c == ' ';
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int isupper(int c) {
    return c >= 'A' && c <= 'Z';
}

int isprint(int c) {
    return c > 0x1F && c != 0x7F;
}

int iscntrl(int c) {
    return c <= 0x1F || c == 0x7F;
}

int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

int isgraph(int c) {
    return isprint(c) && c != ' ';
}

int isxdigit(int c) {
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int isalpha(int c) {
    return islower(c) || isupper(c);
}

int isalnum(int c) {
    return isdigit(c) || isalpha(c);
}

int ispunct(int c) {
    return isgraph(c) && !isalnum(c);
}

int tolower(int c) {
    if (isupper(c)) {
        c = c + 'a' - 'A';
    }
    return c;
}

int toupper(int c) {
    if (islower(c)) {
        c = c - 'a' + 'A';
    }
    return c;
}

