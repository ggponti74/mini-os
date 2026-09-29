// fs/vfs.h
#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define FS_FILE      0x01
#define FS_DIRECTORY 0x02

struct vfs_node;

typedef uint32_t (*read_type_t)(struct vfs_node*, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef uint32_t (*write_type_t)(struct vfs_node*, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef void (*open_type_t)(struct vfs_node*);
typedef void (*close_type_t)(struct vfs_node*);

typedef struct vfs_node {
    char name[128];
    uint32_t flags;       // FS_FILE or FS_DIRECTORY
    uint32_t length;      // Size in bytes
    
    // FS Driver Callbacks
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
} vfs_node_t;

// Top-level Kernel API
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif