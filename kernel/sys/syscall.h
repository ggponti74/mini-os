// include/sys/syscall.h
#ifndef MINI_OS_SYSCALL_H
#define MINI_OS_SYSCALL_H

#include <stdint.h>
#include "../fcntl.h"

#define SYS_EXIT   1
#define SYS_FORK   2
#define SYS_READ   3
#define SYS_WRITE  4
#define SYS_OPEN   5
#define SYS_CLOSE  6
#define SYS_SBRK   7
#define SYS_FSTAT  8
#define SYS_GETKEY 9
#define SYS_CLEAR  10
#define SYS_CURSOR 11
#define SYS_FTRUNCATE 12
#define SYS_LSEEK 13
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

#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define OPEN_WRONLY O_WRONLY
#define OPEN_RDWR   O_RDWR
#define OPEN_CREAT  O_CREAT

int32_t syscall_dispatch(uint32_t number, uint32_t arg1, uint32_t arg2,
                         uint32_t arg3);

#endif