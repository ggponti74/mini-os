#include <stddef.h>

// 64 KB static buffer reserved for the userland heap break
static char user_heap[64 * 1024]; 
static char *heap_ptr = user_heap;

void *_sbrk(int incr) {
    char *prev_heap_ptr = heap_ptr;

    // Check if the requested increment exceeds the available buffer
    if (heap_ptr + incr > user_heap + sizeof(user_heap)) {
        return (void *)-1; // Out of memory
    }

    heap_ptr += incr;
    return (void *)prev_heap_ptr;
}

void _exit(int status) {
    __asm__ volatile (
        "int $0x80"
        :
        : "a"(1), "b"(status)
    );
    while (1) { __asm__ volatile("hlt"); }
}

int _read(int fd, void *buf, size_t count) {
    int bytes_read;
    __asm__ volatile (
        "int $0x80"
        : "=a"(bytes_read)
        : "a"(3), "b"(fd), "c"(buf), "d"(count)
    );
    return bytes_read;
}

int _write(int fd, const void *buf, size_t count) {
    int bytes_written;
    __asm__ volatile (
        "int $0x80"
        : "=a"(bytes_written)
        : "a"(4), "b"(fd), "c"(buf), "d"(count)
    );
    return bytes_written;
}
