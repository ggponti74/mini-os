#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define FS_FILE      0x01
#define FS_DIRECTORY 0x02

// include/fs/vfs.h

struct vfs_node;

// Function pointer signatures
typedef uint32_t (*vfs_read_t)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef uint32_t (*vfs_write_t)(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
typedef void     (*vfs_open_t)(struct vfs_node *node);
typedef void     (*vfs_close_t)(struct vfs_node *node);

// Callback to create a child entry inside a directory node
typedef struct vfs_node *(*vfs_create_t)(struct vfs_node *parent, const char *name, uint32_t flags);

typedef struct vfs_node {
    char name[128];
    uint32_t flags;       // FS_FILE or FS_DIRECTORY
    uint32_t length;      // Size of the file in bytes
    uint32_t inode;       
    
    // File operations
    vfs_read_t   read;
    vfs_write_t  write;
    vfs_open_t   open;
    vfs_close_t  close;
    vfs_create_t create; // Directory callback for creating files/dirs
    
    void *device_data; 
} vfs_node_t;

extern vfs_node_t *fs_root;

// Standard VFS Kernel API
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
vfs_node_t *vfs_lookup(vfs_node_t *root, const char *path);

// Top-level creation wrapper
vfs_node_t *vfs_create(vfs_node_t *parent, const char *name, uint32_t flags);

#endif
