// userland/libsys/stubs.c
#include <stddef.h>

// 4. Heap Expansion Stub (_sbrk for malloc)
extern char __end[]; // Defined by linker script marking kernel/userland end
static char *heap_end = NULL;

// 1. Process Exit Stub
void _exit(int status) {
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(1), "b"(status)
    );
    while (1) { __asm__ volatile("hlt"); }
}

// 2. Read Stub (POSIX compliant signature)
int _read(int fd, void *buf, size_t count) {
    int bytes_read;
    __asm__ volatile (
        "int $0x80"
        : "=a"(bytes_read)
        : "a"(3), "b"(fd), "c"(buf), "d"(count)
    );
    return bytes_read;
}

// 3. Write Stub (POSIX compliant signature)
int _write(int fd, const void *buf, size_t count) {
    int bytes_written;
    __asm__ volatile (
        "int $0x80"
        : "=a"(bytes_written)
        : "a"(4), "b"(fd), "c"(buf), "d"(count)
    );
    return bytes_written;
}

void *_sbrk(ptrdiff_t incr) {
    if (heap_end == NULL) {
        heap_end = __end;
    }
    
    char *prev_heap_end = heap_end;
    // Simple bump allocation check against limits if necessary
    heap_end += incr;
    return (void *)prev_heap_end;
}
