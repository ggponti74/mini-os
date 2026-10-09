#include <stddef.h>

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
