#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <stddef.h>
#include <stdint.h>

#if !defined(__ssize_t_defined) && !defined(_SSIZE_T_DEFINED)
#define __ssize_t_defined
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#else
#define __ssize_t_defined
#define _SSIZE_T_DEFINED
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef long off_t;
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef int64_t time_t;
#endif

typedef int32_t pid_t;
typedef uint32_t mode_t;
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef int16_t dev_t;
typedef uint16_t ino_t;
typedef uint16_t nlink_t;
typedef long blksize_t;
typedef long blkcnt_t;

#endif
