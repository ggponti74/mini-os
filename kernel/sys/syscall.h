// include/sys/syscall.h
#ifndef MINI_OS_SYSCALL_H
#define MINI_OS_SYSCALL_H

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

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define OPEN_WRONLY O_WRONLY
#define OPEN_RDWR   O_RDWR
#define OPEN_CREAT  O_CREAT

#endif