#ifndef _ERRNO_H
#define _ERRNO_H

#define EPERM 1
#define ENOENT 2
#define EIO 5
#define EBADF 9
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define EEXIST 17
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define EMFILE 24
#define ENOSPC 28
#define ESPIPE 29
#define ENOSYS 88
#define ENAMETOOLONG 91
#define EOVERFLOW 139

int *__errno_location(void);

#define errno (*__errno_location())

#endif
