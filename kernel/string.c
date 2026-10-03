#include "string.h"

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void memset(void *dest, int val, uint32_t count) {
    char *temp = (char *)dest;
    for (; count != 0; count--) {
        *temp++ = val;
    }
}