#include <stddef.h>
#include "memory.h"

void *malloc(size_t size) {
    (void)size;
    return NULL;
}

void free(void *ptr) {
    (void)ptr;
}
