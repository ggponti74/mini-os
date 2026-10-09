#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#ifndef _KERNEL_SYS_TYPES_H
#define _KERNEL_SYS_TYPES_H

#ifndef __ssize_t_defined
#define __ssize_t_defined
typedef long ssize_t;   // Use signed long instead of unsigned __SIZE_TYPE__
#endif

#ifndef __off_t_defined
#define __off_t_defined
typedef long off_t;
#endif

#ifndef __time_t_defined
#define __time_t_defined
typedef long time_t;
#endif

#endif

// Map seamlessly to compiler internal definitions for cross-platform safety
typedef __SIZE_TYPE__       size_t;
//typedef __SIZE_TYPE__       ssize_t;

typedef __INT32_TYPE__      pid_t;
typedef __INT32_TYPE__      int32_t;
typedef __UINT32_TYPE__     uint32_t;
typedef __UINT16_TYPE__     uint16_t;

typedef uint32_t            mode_t;   // Permissions profile
typedef uint32_t            uid_t;    // User identifier
typedef uint32_t            gid_t;    // Group identifier
typedef uint32_t            dev_t;    // Device ID designation
typedef uint32_t            ino_t;    // Inode serial identification
typedef uint16_t            nlink_t;  // Hard link counting tally
// typedef __INT64_TYPE__      off_t;    // Core file offsets or sizing metrics
// typedef __INT64_TYPE__      time_t;   // Timestamp tracking engine
typedef long off_t;   // Match host header long int definition
typedef long time_t;  // Match host header long int definition

typedef __SIZE_TYPE__        size_t;
typedef __INT32_TYPE__      pid_t;

#endif // _SYS_TYPES_H
