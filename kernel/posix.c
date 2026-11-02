#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "errno.h"
#include "fcntl.h"
#include "posix.h"
#include "sys/syscall.h"

static int mock_errno;

int *__errno_location(void) {
    return &mock_errno;
}

int *__posix_errno_location(void) {
    return __errno_location();
}

static int posix_syscall3(uint32_t number, uint32_t arg1, uint32_t arg2,
                         uint32_t arg3) {
    int result;
    __asm__ __volatile__("int $0x80"
                         : "=a"(result)
                         : "a"(number), "b"(arg1), "c"(arg2), "d"(arg3)
                         : "memory");
    return result;
}

/* ========================================================================
   FILE OPERATIONS
   ======================================================================== */

int open(const char *pathname, int flags, ...) {
    va_list args;
    mode_t mode = 0;

    if (flags & O_CREAT) {
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    int fd = posix_syscall3(SYS_OPEN, (uint32_t)(uintptr_t)pathname,
                            (uint32_t)flags, (uint32_t)mode);
    if (fd < 0) {
        errno = -fd;
        return -1;
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    int result = posix_syscall3(SYS_READ, (uint32_t)fd,
                                (uint32_t)(uintptr_t)buf, (uint32_t)count);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return (ssize_t)result;
}

ssize_t write(int fd, const void *buf, size_t count) {
    int result = posix_syscall3(SYS_WRITE, (uint32_t)fd,
                                (uint32_t)(uintptr_t)buf, (uint32_t)count);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return (ssize_t)result;
}

int close(int fd) {
    int result = posix_syscall3(SYS_CLOSE, (uint32_t)fd, 0, 0);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

int fstat(int fd, struct stat *status) {
    int result = posix_syscall3(SYS_FSTAT, (uint32_t)fd,
                                (uint32_t)(uintptr_t)status, 0);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

int stat(const char *path, struct stat *status) {
    if (!path || !status) {
        errno = EFAULT;
        return -1;
    }

    int fd = open(path, O_RDONLY, 0);
    if (fd < 0) {
        return -1;
    }

    int result = fstat(fd, status);
    int close_result = close(fd);
    if (result < 0) {
        return -1;
    }
    if (close_result < 0) {
        errno = EBADF;
        return -1;
    }
    return 0;
}

int mkdir(const char *pathname, mode_t mode) {
    (void)mode;
    int result = posix_syscall3(SYS_MKDIR, (uint32_t)(uintptr_t)pathname, 0, 0);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

int rmdir(const char *pathname) {
    int result = posix_syscall3(SYS_RMDIR, (uint32_t)(uintptr_t)pathname, 0, 0);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

int unlink(const char *pathname) {
    int result = posix_syscall3(SYS_UNLINK, (uint32_t)(uintptr_t)pathname, 0, 0);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

int access(const char *pathname, int mode) {
    if (!pathname) {
        errno = EFAULT;
        return -1;
    }
    int result = posix_syscall3(SYS_ACCESS, (uint32_t)(uintptr_t)pathname,
                                (uint32_t)mode, 0);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

int rename(const char *oldpath, const char *newpath) {
    if (!oldpath || !newpath) {
        errno = EFAULT;
        return -1;
    }
    int result = posix_syscall3(SYS_RENAME,
                                (uint32_t)(uintptr_t)oldpath,
                                (uint32_t)(uintptr_t)newpath, 0);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

int isatty(int fd) {
    struct stat status;
    if (fstat(fd, &status) < 0) {
        return 0;
    }
    return S_ISCHR(status.st_mode);
}

off_t lseek(int fd, off_t offset, int whence) {
    int result = posix_syscall3(SYS_LSEEK, (uint32_t)fd, (uint32_t)offset,
                                (uint32_t)whence);
    if (result < 0) {
        errno = -result;
        return (off_t)-1;
    }
    return (off_t)result;
}

int ftruncate(int fd, off_t length) {
    int result = posix_syscall3(SYS_FTRUNCATE, (uint32_t)fd,
                                (uint32_t)length, 0);
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return 0;
}

/* ========================================================================
   PROCESS & MEMORY LIFECYCLE
   ======================================================================== */

void _exit(int status) {
    posix_syscall3(SYS_EXIT, (uint32_t)status, 0, 0);
    while (1) {
#if defined(__x86_64__) || defined(__i386__)
        __asm__ __volatile__("cli; hlt");
#define HAS_HALT
#endif
#if defined(__arm__) || defined(__aarch64__)
        __asm__ __volatile__("wfi");
#define HAS_HALT
#endif
#if !defined(HAS_HALT)
        __asm__ __volatile__("nop");
#endif
    }
}

pid_t getpid(void) {
    return 1;
}

int kill(pid_t pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

/* ========================================================================
   MEMORY ALLOCATION LAYER
   ======================================================================== */

void *brk(void *addr) {
    static void *current_brk = NULL;
    if (current_brk == NULL) {
        current_brk = (void *)0x100000;
    }

    if (addr == NULL) {
        return current_brk;
    }

    if ((uintptr_t)addr < (uintptr_t)current_brk) {
        errno = EINVAL;
        return (void *)-1;
    }

    ptrdiff_t delta = (char *)addr - (char *)current_brk;
    if (posix_syscall3(SYS_SBRK, (uint32_t)delta, 0, 0) == -ENOMEM) {
        errno = ENOMEM;
        return (void *)-1;
    }

    current_brk = addr;
    return current_brk;
}
