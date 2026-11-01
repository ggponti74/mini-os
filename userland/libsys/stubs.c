// userland/libsys/stubs.c
#include <stddef.h>
#include <sys/stat.h>

#define SYS_SBRK 7
#define SYS_LSEEK 13

static int syscall3(int number, int arg1, int arg2, int arg3) {
    __asm__ volatile (
        "int $0x80"
        : "+a"(number)
        : "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return number;
}

// 1. Process Exit Stub
void _exit(int status) {
    syscall3(1, status, 0, 0);
    while (1) { __asm__ volatile("hlt"); }
}

int _open(const char *pathname, int flags, ...) {
    return syscall3(5, (int)pathname, flags, 0);
}

// 2. Read Stub (POSIX compliant signature)
int _read(int fd, void *buf, size_t count) {
    return syscall3(3, fd, (int)buf, (int)count);
}

// 3. Write Stub (POSIX compliant signature)
int _write(int fd, const void *buf, size_t count) {
    return syscall3(4, fd, (int)buf, (int)count);
}

int _close(int fd) {
    return syscall3(6, fd, 0, 0);
}

int _fstat(int fd, struct stat *status) {
    return syscall3(8, fd, (int)status, 0);
}

long _lseek(int fd, long offset, int whence) {
    return syscall3(SYS_LSEEK, fd, (int)offset, whence);
}

void *_sbrk(ptrdiff_t incr) {
    int previous_break = syscall3(SYS_SBRK, (int)incr, 0, 0);
    if (previous_break == -1) {
        return (void *)-1;
    }
    return (void *)previous_break;
}
