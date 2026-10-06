
#include <stdint.h>
#include <stddef.h>

#include "display.h"

typedef int int32_t;          // Standard 32-bit signed integer
typedef unsigned int uint32_t; // Standard 32-bit unsigned integer

// Minimal types for Newlib stubs
typedef int32_t off_t;
typedef int32_t dev_t;
typedef uint32_t mode_t;
typedef uint32_t ino_t;
typedef uint32_t nlink_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;

struct stat {
    dev_t     st_dev;
    ino_t     st_ino;
    mode_t    st_mode;
    nlink_t   st_nlink;
    uid_t     st_uid;
    gid_t     st_gid;
    off_t     st_size;
};

#ifndef S_IFCHR
#define S_IFCHR 0020000
#endif

// 64 KB static buffer reserved for userland heap (or point to kernel heap end)
static char user_heap[64 * 1024]; 
static char *heap_ptr = user_heap;

void *_sbrk(int incr) {
    char *prev_heap_ptr = heap_ptr;

    // Check if allocation exceeds available buffer
    if (heap_ptr + incr > user_heap + sizeof(user_heap)) {
        return (void *)-1; // Out of memory
    }

    heap_ptr += incr;
    return (void *)prev_heap_ptr;
}

int _write(int file, char *ptr, int len) {
if (file == 1 || file == 2) { // stdout or stderr
        for (int i = 0; i < len; i++) {
            // Replace with your kernel/HAL display output function
            kputchar_color(ptr[i], COLOR_DEFAULT); 
        }
        return len;
    }
    return -1;
}

int _read(int file, char *ptr, int len) {
    return 0;
}

int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    return 1;
}

void _exit(int status) {
    while(1);
}
