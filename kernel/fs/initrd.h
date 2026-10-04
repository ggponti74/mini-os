// include/fs/initrd.h
#ifndef INITRD_H
#define INITRD_H

#include "vfs.h"

// Standard POSIX TAR block header structure
typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];     // File size stored as octal ASCII string
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
} __attribute__((packed)) tar_header_t;

vfs_node_t *initrd_init(void);
void initrd_list_files(void) ;

#endif
