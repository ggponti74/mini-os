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
#include <sys/utsname.h>
#include <unistd.h>
#include "memory.h"
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
#define SYS_OPENAT 36
#define SYS_FSTATAT 37
#define SYS_UNLINKAT 38
#define SYS_RENAMEAT 39
#define SYS_FACCESSAT 40
#define SYS_MKDIRAT 41
#define SYS_FCHMODAT 42
#define SYS_READDIR 43
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
#define SYS_FSYNC 34
#define SYS_FCHDIR 35

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

#ifndef AT_FDCWD
#define AT_FDCWD (-100)
#endif

#ifndef AT_REMOVEDIR
#define AT_REMOVEDIR 0x200
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

static int syscall_result(int result) {
    if (result < 0) {
        errno = -result;
        return -1;
    }
    return result;
}

typedef struct {
    int offset;
    unsigned int count;
} positional_io_request_t;

typedef struct {
    char name[128];
    unsigned int inode;
    unsigned int type;
} directory_entry_request_t;

typedef struct dirent {
    unsigned long d_ino;
    char d_name[256];
} mini_dirent_t;

typedef struct __dirstream {
    int fd;
    unsigned int index;
    mini_dirent_t entry;
    int used;
} DIR;

static DIR directory_streams[8];

static DIR *allocate_directory_stream(int fd) {
    for (int i = 0; i < 8; i++) {
        if (!directory_streams[i].used) {
            directory_streams[i].fd = fd;
            directory_streams[i].index = 0;
            directory_streams[i].used = 1;
            return &directory_streams[i];
        }
    }
    errno = EMFILE;
    return NULL;
}

static int positional_io(int number, int fd, void *buffer,
                         size_t count, off_t offset) {
    positional_io_request_t request;
    request.offset = (int)offset;
    request.count = (unsigned int)count;
    return syscall_result(syscall3(number, fd, (int)buffer, (int)&request));
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

int _fstat(int fd, struct stat *status);
int _close(int fd);

DIR *fdopendir(int fd) {
    struct stat status;
    if (_fstat(fd, &status) < 0) return NULL;
    if (!S_ISDIR(status.st_mode)) {
        errno = ENOTDIR;
        return NULL;
    }
    return allocate_directory_stream(fd);
}

DIR *opendir(const char *path) {
    int fd = _open(path, O_RDONLY | O_DIRECTORY);
    if (fd < 0) return NULL;

    DIR *directory = fdopendir(fd);
    if (!directory) {
        int saved_errno = errno;
        _close(fd);
        errno = saved_errno;
    }
    return directory;
}

int dirfd(DIR *directory) {
    if (!directory || !directory->used) {
        errno = EBADF;
        return -1;
    }
    return directory->fd;
}

struct dirent *readdir(DIR *directory) {
    if (!directory || !directory->used) { errno = EBADF; return NULL; }
    directory_entry_request_t entry;
    int result = syscall3(SYS_READDIR, directory->fd, (int)directory->index,
                          (int)&entry);
    if (result < 0) { errno = -result; return NULL; }
    if (result == 0) return NULL;
    directory->index++;
    directory->entry.d_ino = entry.inode;
    unsigned int i = 0;
    while (i < sizeof(directory->entry.d_name) - 1 && entry.name[i]) {
        directory->entry.d_name[i] = entry.name[i];
        i++;
    }
    directory->entry.d_name[i] = '\0';
    return &directory->entry;
}

long telldir(DIR *directory) {
    if (!directory || !directory->used) { errno = EBADF; return -1; }
    return (long)directory->index;
}

void seekdir(DIR *directory, long location) {
    if (!directory || !directory->used) { errno = EBADF; return; }
    if (location < 0) { errno = EINVAL; return; }
    directory->index = (unsigned int)location;
}

void rewinddir(DIR *directory) {
    if (!directory || !directory->used) { errno = EBADF; return; }
    directory->index = 0;
}

int closedir(DIR *directory) {
    if (!directory || !directory->used) { errno = EBADF; return -1; }
    int result = _close(directory->fd);
    directory->used = 0;
    directory->fd = -1;
    directory->index = 0;
    return result;
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
    if (S_ISCHR(status.st_mode)) {
        return 1;
    }
    errno = ENOTTY;
    return 0;
}

long _lseek(int fd, long offset, int whence) {
    return syscall_result(syscall3(SYS_LSEEK, fd, (int)offset, whence));
}

int _ftruncate(int fd, off_t length) {
    return syscall_result(syscall3(SYS_FTRUNCATE, fd, (int)length, 0));
}

/* Implement path-based truncation using the existing descriptor operation. */
int _truncate(const char *path, off_t length) {
    if (length < 0) {
        errno = EINVAL;
        return -1;
    }

    int fd = _open(path, O_WRONLY);
    if (fd < 0) {
        return -1;
    }

    int result = _ftruncate(fd, length);
    int saved_errno = errno;
    int close_result = _close(fd);
    if (result < 0) {
        errno = saved_errno;
        return -1;
    }
    return close_result;
}

int _fsync(int fd) {
    return syscall_result(syscall3(SYS_FSYNC, fd, 0, 0));
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

int openat(int dirfd, const char *pathname, int flags, ...) {
    struct openat_request { int flags; int mode; } request;
    request.flags = flags;
    request.mode = 0;
    if (flags & O_CREAT) {
        va_list ap;
        va_start(ap, flags);
        request.mode = va_arg(ap, int);
        va_end(ap);
    }
    return syscall_result(syscall3(SYS_OPENAT, dirfd, (int)pathname,
                                   (int)&request));
}

int faccessat(int dirfd, const char *pathname, int mode, int flags) {
    struct at_mode_flags_request { unsigned int mode; unsigned int flags; } request;
    request.mode = (unsigned int)mode;
    request.flags = (unsigned int)flags;
    return syscall_result(syscall3(SYS_FACCESSAT, dirfd, (int)pathname,
                                   (int)&request));
}

int mkdirat(int dirfd, const char *pathname, mode_t mode) {
    return syscall_result(syscall3(SYS_MKDIRAT, dirfd, (int)pathname,
                                   (int)mode));
}

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
    struct at_mode_flags_request { unsigned int mode; unsigned int flags; } request;
    request.mode = (unsigned int)mode;
    request.flags = (unsigned int)flags;
    return syscall_result(syscall3(SYS_FCHMODAT, dirfd, (int)pathname,
                                   (int)&request));
}

int unlinkat(int dirfd, const char *pathname, int flags) {
    return syscall_result(syscall3(SYS_UNLINKAT, dirfd, (int)pathname,
                                   flags));
}

int _rename(const char *oldpath, const char *newpath);

int renameat(int olddirfd, const char *oldpath,
             int newdirfd, const char *newpath) {
    struct renameat_request { int newdirfd; const char *newpath; } request;
    request.newdirfd = newdirfd;
    request.newpath = newpath;
    return syscall_result(syscall3(SYS_RENAMEAT, olddirfd, (int)oldpath,
                                   (int)&request));
}

int fstatat(int dirfd, const char *pathname, struct stat *status, int flags) {
    struct fstatat_request { struct stat *status; int flags; } request;
    request.status = status;
    request.flags = flags;
    return syscall_result(syscall3(SYS_FSTATAT, dirfd, (int)pathname,
                                   (int)&request));
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

int _fchdir(int fd) {
    return syscall_result(syscall3(SYS_FCHDIR, fd, 0, 0));
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
    if (command == F_SETFL || command == F_SETFD || command == F_DUPFD) {
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

int truncate(const char *path, off_t length) {
    return _truncate(path, length);
}

int fsync(int fd) {
    return _fsync(fd);
}

int fdatasync(int fd) {
    return _fsync(fd);
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

int fchdir(int fd) {
    return _fchdir(fd);
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
    if (command == F_SETFL || command == F_SETFD || command == F_DUPFD) {
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

/* mini-os currently runs applications with a single root identity. */
uid_t _getuid(void) {
    return 0;
}

uid_t _geteuid(void) {
    return 0;
}

gid_t _getgid(void) {
    return 0;
}

gid_t _getegid(void) {
    return 0;
}

/* The current system has one fixed host identity and no hostname service. */
#define MINI_OS_HOSTNAME "mini-os"

int _gethostname(char *name, size_t length) {
    static const char hostname[] = MINI_OS_HOSTNAME;
    if (!name) {
        errno = EFAULT;
        return -1;
    }
    if (length == 0) {
        errno = ENAMETOOLONG;
        return -1;
    }

    size_t index = 0;
    while (index + 1 < length && hostname[index] != '\0') {
        name[index] = hostname[index];
        index++;
    }
    name[index] = '\0';
    if (hostname[index] != '\0') {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

static void copy_uts_field(char *destination, size_t capacity,
                           const char *source) {
    if (capacity == 0) {
        return;
    }
    size_t index = 0;
    while (index + 1 < capacity && source[index] != '\0') {
        destination[index] = source[index];
        index++;
    }
    destination[index] = '\0';
}

/* Report the page size used by the physical memory manager. */
long sysconf(int name) {
#ifdef _SC_PAGESIZE
    if (name == _SC_PAGESIZE) {
        return PAGE_SIZE;
    }
#endif
#if defined(_SC_PAGE_SIZE) && (!defined(_SC_PAGESIZE) || _SC_PAGE_SIZE != _SC_PAGESIZE)
    if (name == _SC_PAGE_SIZE) {
        return PAGE_SIZE;
    }
#endif
#ifdef _SC_OPEN_MAX
    if (name == _SC_OPEN_MAX) {
        return 8; /* SYSCALL_FD_COUNT, including the three standard descriptors. */
    }
#endif
    errno = EINVAL;
    return -1;
}

int getpagesize(void) {
    return PAGE_SIZE;
}

int getdtablesize(void) {
    return 8; /* SYSCALL_FD_COUNT, including the three standard descriptors. */
}

int uname(struct utsname *name) {
    if (!name) {
        errno = EFAULT;
        return -1;
    }

    copy_uts_field(name->sysname, sizeof(name->sysname), MINI_OS_HOSTNAME);
    copy_uts_field(name->nodename, sizeof(name->nodename), MINI_OS_HOSTNAME);
    copy_uts_field(name->release, sizeof(name->release), "0");
    copy_uts_field(name->version, sizeof(name->version), "development");
    copy_uts_field(name->machine, sizeof(name->machine), "i386");
    return 0;
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

uid_t getuid(void) {
    return _getuid();
}

uid_t geteuid(void) {
    return _geteuid();
}

gid_t getgid(void) {
    return _getgid();
}

gid_t getegid(void) {
    return _getegid();
}

int gethostname(char *name, size_t length) {
    return _gethostname(name, length);
}

int kill(int pid, int signal) {
    return _kill(pid, signal);
}
