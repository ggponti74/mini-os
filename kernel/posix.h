#ifndef POSIX_H
#define POSIX_H

#include "errno.h"

int *__posix_errno_location(void);
int open(const char *pathname, int flags, ...);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);
void _exit(int status);
pid_t getpid(void);
int kill(pid_t pid, int sig);
void *brk(void *addr);

#endif