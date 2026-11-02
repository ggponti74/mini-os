#ifndef POSIX_H
#define POSIX_H

#include "errno.h"

int *__posix_errno_location(void);
int open(const char *pathname, int flags, ...);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
int fstat(int fd, struct stat *status);
int stat(const char *path, struct stat *status);
int mkdir(const char *pathname, mode_t mode);
int rmdir(const char *pathname);
int unlink(const char *pathname);
int access(const char *pathname, int mode);
int rename(const char *oldpath, const char *newpath);
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int isatty(int fd);
off_t lseek(int fd, off_t offset, int whence);
int ftruncate(int fd, off_t length);
void _exit(int status);
pid_t getpid(void);
int kill(pid_t pid, int sig);
void *brk(void *addr);

#endif