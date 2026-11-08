// userland/libsys/stubs.c
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "memory_layout.h"

extern char _end[];

/* The kernel errno header maps errno to this per-process storage. */
static int libsys_errno;

int *__errno_location(void) {
    return &libsys_errno;
}

#define SYS_SBRK 7
#define SYS_APP_SBRK 26
#define SYS_FSTAT 8
#define SYS_LSEEK 13
#define SYS_FTRUNCATE 12
#define SYS_MKDIR 14
#define SYS_RMDIR 15
#define SYS_UNLINK 16
#define SYS_ACCESS 17
#define SYS_RENAME 18
#define SYS_CHMOD 19
#define SYS_FCHMOD 20
#define SYS_CHDIR 21
#define SYS_GETCWD 22
#define SYS_DUP 23
#define SYS_DUP2 24
#define SYS_FCNTL 25
#define SYS_STAT 27
#define SYS_UMASK 28
#define SYS_CLOCK_GETTIME 29
#define SYS_NANOSLEEP 30
#define SYS_GETTIMEOFDAY 31
#define SYS_PREAD 32
#define SYS_PWRITE 33

#ifndef F_GETFL
#define F_GETFL 3
#define F_SETFL 4
#endif

#ifndef F_OK
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1
#endif

static int syscall3(int number, int arg1, int arg2, int arg3) {
    __asm__ volatile (
        "int $0x80"
        : "+a"(number)
        : "b"(arg1), "c"(arg2), "d"(arg3)
        : "memory"
    );
    return number;
}

typedef struct {
    int offset;
    unsigned int count;
} positional_io_request_t;

static int positional_io(int number, int fd, void *buffer,
                         size_t count, off_t offset) {
    positional_io_request_t request;
    request.offset = (int)offset;
    request.count = (unsigned int)count;
    return syscall_result(syscall3(number, fd, (int)buffer, (int)&request));
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
    /* Reached only if the kernel has no active application to exit. */
    while (1) { __asm__ volatile("hlt"); }
}

int _open(const char *pathname, int flags, ...) {
    va_list ap;
    int mode = 0;
    if (flags & O_CREAT) {
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    return syscall_result(syscall3(5, (int)pathname, flags, mode));
}

// 2. Read Stub (POSIX compliant signature)
int _pread(int fd, void *buf, size_t count, off_t offset) {
    return positional_io(SYS_PREAD, fd, buf, count, offset);
}

int _pwrite(int fd, const void *buf, size_t count, off_t offset) {
    return positional_io(SYS_PWRITE, fd, (void *)buf, count, offset);
}

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

int _stat(const char *path, struct stat *status) {
    return syscall_result(syscall3(SYS_STAT, (int)path, (int)status, 0));
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

int _ftruncate(int fd, off_t length) {
    return syscall_result(syscall3(SYS_FTRUNCATE, fd, (int)length, 0));
}

int _clock_gettime(clockid_t clock_id, struct timespec *time) {
    return syscall_result(syscall3(SYS_CLOCK_GETTIME, (int)clock_id, (int)time, 0));
}
int _nanosleep(const struct timespec *request, struct timespec *remaining) {
    return syscall_result(syscall3(SYS_NANOSLEEP, (int)request, (int)remaining, 0));
}

mode_t _umask(mode_t mask) {
    return (mode_t)syscall3(SYS_UMASK, (int)mask, 0, 0);
}

int _mkdir(const char *pathname, mode_t mode) {
    return syscall_result(syscall3(SYS_MKDIR, (int)pathname, (int)mode, 0));
}

int _rmdir(const char *pathname) {
    return syscall_result(syscall3(SYS_RMDIR, (int)pathname, 0, 0));
}

int _unlink(const char *pathname) {
    return syscall_result(syscall3(SYS_UNLINK, (int)pathname, 0, 0));
}

int _link(const char *existing, const char *newpath) {
    (void)existing;
    (void)newpath;
    errno = ENOSYS;
    return -1;
}

int _access(const char *pathname, int mode) {
    return syscall_result(syscall3(SYS_ACCESS, (int)pathname, mode, 0));
}

int _rename(const char *oldpath, const char *newpath) {
    return syscall_result(syscall3(SYS_RENAME, (int)oldpath, (int)newpath, 0));
}

int _chmod(const char *pathname, mode_t mode) {
    return syscall_result(syscall3(SYS_CHMOD, (int)pathname, (int)mode, 0));
}

int _fchmod(int fd, mode_t mode) {
    return syscall_result(syscall3(SYS_FCHMOD, fd, (int)mode, 0));
}

int _chdir(const char *path) {
    return syscall_result(syscall3(SYS_CHDIR, (int)path, 0, 0));
}

char *_getcwd(char *buf, size_t size) {
    int result = syscall3(SYS_GETCWD, (int)buf, (int)size, 0);
    if (result < 0) {
        errno = -result;
        return NULL;
    }
    return buf;
}

int _dup(int oldfd) {
    return syscall_result(syscall3(SYS_DUP, oldfd, 0, 0));
}

int _dup2(int oldfd, int newfd) {
    return syscall_result(syscall3(SYS_DUP2, oldfd, newfd, 0));
}

int _fcntl(int fd, int command, ...) {
    int argument = 0;
    if (command == F_SETFL) {
        va_list ap;
        va_start(ap, command);
        argument = va_arg(ap, int);
        va_end(ap);
    }
    return syscall_result(syscall3(SYS_FCNTL, fd, command, argument));
}

void *_sbrk(ptrdiff_t incr) {
    int previous_break = syscall3(SYS_APP_SBRK, (int)incr, (int)_end,
                                  USER_APP_HEAP_LIMIT);
    if (previous_break < 0) {
        errno = -previous_break;
        return (void *)-1;
    }
    return (void *)previous_break;
}

int open(const char *pathname, int flags, ...) {
    va_list ap;
    int mode = 0;
    if (flags & O_CREAT) {
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    return syscall_result(syscall3(5, (int)pathname, flags, mode));
}

int pread(int fd, void *buf, size_t count, off_t offset) {
    return _pread(fd, buf, count, offset);
}

int pwrite(int fd, const void *buf, size_t count, off_t offset) {
    return _pwrite(fd, buf, count, offset);
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

int stat(const char *path, struct stat *status) {
    return _stat(path, status);
}

int isatty(int fd) {
    return _isatty(fd);
}

long lseek(int fd, long offset, int whence) {
    return _lseek(fd, offset, whence);
}

int ftruncate(int fd, off_t length) {
    return _ftruncate(fd, length);
}

int clock_gettime(clockid_t clock_id, struct timespec *time) {
    return _clock_gettime(clock_id, time);
}
int nanosleep(const struct timespec *request, struct timespec *remaining) {
    return _nanosleep(request, remaining);
}
int gettimeofday(struct timeval *time, void *timezone) {
    return _gettimeofday(time, timezone);
}

mode_t umask(mode_t mask) {
    return _umask(mask);
}

int mkdir(const char *pathname, mode_t mode) {
    return _mkdir(pathname, mode);
}

int rmdir(const char *pathname) {
    return _rmdir(pathname);
}

int unlink(const char *pathname) {
    return _unlink(pathname);
}

int access(const char *pathname, int mode) {
    return _access(pathname, mode);
}

int rename(const char *oldpath, const char *newpath) {
    return _rename(oldpath, newpath);
}

int chmod(const char *pathname, mode_t mode) {
    return _chmod(pathname, mode);
}

int fchmod(int fd, mode_t mode) {
    return _fchmod(fd, mode);
}

int chdir(const char *path) {
    return _chdir(path);
}

char *getcwd(char *buf, size_t size) {
    return _getcwd(buf, size);
}

int dup(int oldfd) {
    return _dup(oldfd);
}

int dup2(int oldfd, int newfd) {
    return _dup2(oldfd, newfd);
}

int fcntl(int fd, int command, ...) {
    int argument = 0;
    if (command == F_SETFL) {
        va_list ap;
        va_start(ap, command);
        argument = va_arg(ap, int);
        va_end(ap);
    }
    return syscall_result(syscall3(SYS_FCNTL, fd, command, argument));
}

void *sbrk(ptrdiff_t incr) {
    return _sbrk(incr);
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int signal) {
    (void)pid;
    (void)signal;
    errno = ENOSYS;
    return -1;
}

int _fork(void) {
    errno = ENOSYS;
    return -1;
}

int _execve(const char *pathname, char *const argv[], char *const envp[]) {
    (void)pathname;
    (void)argv;
    (void)envp;
    errno = ENOSYS;
    return -1;
}

int _wait(int *status) {
    (void)status;
    errno = ENOSYS;
    return -1;
}

int _gettimeofday(struct timeval *time, void *timezone) {
    (void)timezone;
    return syscall_result(syscall3(SYS_GETTIMEOFDAY, (int)time, 0, 0));
}

clock_t _times(struct tms *buffer) {
    (void)buffer;
    errno = ENOSYS;
    return (clock_t)-1;
}

int getpid(void) {
    return _getpid();
}

int kill(int pid, int signal) {
    return _kill(pid, signal);
}
