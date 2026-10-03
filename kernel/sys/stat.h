#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

// Minimum structural compliance parameters mandated by POSIX
struct stat {
    dev_t     st_dev;     // Device number identification
    ino_t     st_ino;     // File structural serial number
    mode_t    st_mode;    // Active file type and permission layout
    nlink_t   st_nlink;   // Total hard connections mapped
    uid_t     st_uid;     // User descriptor
    gid_t     st_gid;     // Group descriptor
    dev_t     st_rdev;    // Device structural ID (if explicitly classified)
    off_t     st_size;    // Volumetric size metrics computed in bytes
    time_t    st_atime;   // Last accessible time indicator
    time_t    st_mtime;   // System write/modification marker
    time_t    st_ctime;   // Structural attribute updating timestamp
};

// Encoding Layout Enforcements
#define S_IFMT   0xF000   // System masking format configuration
#define S_IFDIR  0x4000   // Directory designation
#define S_IFCHR  0x2000   // Character asset designator
#define S_IFBLK  0x6000   // Block asset designator
#define S_IFREG  0x8000   // Standard text/binary file

// Functional Helper Validations
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)

// Native Access Mask Parameters
#define S_IRWXU  00700    // User execution read, write permissions
#define S_IRUSR  00400    // User read access
#define S_IWUSR  00200    // User write access
#define S_IXUSR  00100    // User execution routing

#endif // _SYS_STAT_H
