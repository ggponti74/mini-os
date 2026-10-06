// include/fs/initrd.h
#ifndef INITRD_H
#define INITRD_H

#include "vfs.h"

// include/fs/initrd.h
#if defined(__WIN32__) || defined(_WIN32)
    extern const char _initrd_start[];
    extern const char _initrd_end[];
    #define initrd_start _initrd_start
    #define initrd_end   _initrd_end
#else
    extern const char initrd_start[];
    extern const char initrd_end[];
#endif

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

vfs_node_t *initrd_find_file(const char *filename);
vfs_node_t *initrd_init(void);
void initrd_cat_file(const char *filename);
void initrd_list_files(void);
int initrd_get_file(const char *filename, uint8_t **out_data, uint32_t *out_size);

uint32_t oct2bin(const char *str, int size) ;

#endif
