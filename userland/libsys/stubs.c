// userland/libsys/stubs.c
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>

#define SYS_SBRK 7
#define SYS_LSEEK 13

static int syscall_errno;

int *__errno_location(void) {
    return &syscall_errno;
}

static int syscall3(int number, int arg1, int arg2, int arg3) {
    __asm__ volatile (
        "int $0x80"
        : "+a"(number)
        : "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return number;
}

static int syscall_result(int result) {
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return result;
}

// 1. Process Exit Stub
void _exit(int status) {
    syscall3(1, status, 0, 0);
    while (1) { __asm__ volatile("hlt"); }
}

int _open(const char *pathname, int flags, ...) {
    return syscall_result(syscall3(5, (int)pathname, flags, 0));
}

// 2. Read Stub (POSIX compliant signature)
int _read(int fd, void *buf, size_t count) {
    return syscall_result(syscall3(3, fd, (int)buf, (int)count));
}

// 3. Write Stub (POSIX compliant signature)
int _write(int fd, const void *buf, size_t count) {
    return syscall_result(syscall3(4, fd, (int)buf, (int)count));
}

int _close(int fd) {
    return syscall_result(syscall3(6, fd, 0, 0));
}

int _fstat(int fd, struct stat *status) {
    return syscall_result(syscall3(8, fd, (int)status, 0));
}

int _isatty(int fd) {
    struct stat status;
    if (_fstat(fd, &status) < 0) {
        return 0;
    }
    return S_ISCHR(status.st_mode);
}

long _lseek(int fd, long offset, int whence) {
    return syscall_result(syscall3(SYS_LSEEK, fd, (int)offset, whence));
}

void *_sbrk(ptrdiff_t incr) {
    int previous_break = syscall3(SYS_SBRK, (int)incr, 0, 0);
    if (previous_break < 0) {
        errno = -previous_break;
        return (void *)-1;
    }
    return (void *)previous_break;
}

int open(const char *pathname, int flags, ...) {
    return _open(pathname, flags);
}

int read(int fd, void *buf, size_t count) {
    return _read(fd, buf, count);
}

int write(int fd, const void *buf, size_t count) {
    return _write(fd, buf, count);
}

int close(int fd) {
    return _close(fd);
}

int fstat(int fd, struct stat *status) {
    return _fstat(fd, status);
}

int isatty(int fd) {
    return _isatty(fd);
}

long lseek(int fd, long offset, int whence) {
    return _lseek(fd, offset, whence);
}

void *sbrk(ptrdiff_t incr) {
    return _sbrk(incr);
}

int getpid(void) {
    return 1;
}

int kill(int pid, int signal) {
    (void)pid;
    (void)signal;
    errno = ENOSYS;
    return -1;
}
