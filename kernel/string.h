#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

void memset(void *, int, uint32_t);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
size_t strlen(const char *) ;
    
#endif // STRING_H