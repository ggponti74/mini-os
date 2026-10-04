#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define FS_FILE      0x01
#define FS_DIRECTORY 0x02

struct vfs_node;

// Function pointer signatures for generic node operations
typedef uint32_t (*vfs_read_t)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef uint32_t (*vfs_write_t)(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
typedef void     (*vfs_open_t)(struct vfs_node *node);
typedef void     (*vfs_close_t)(struct vfs_node *node);

typedef struct vfs_node {
    char name[128];
    uint32_t flags;       // FS_FILE or FS_DIRECTORY
    uint32_t length;      // Size of the file in bytes
    uint32_t inode;       // Optional unique identifier/index
    
    // File operations
    vfs_read_t  read;
    vfs_write_t write;
    vfs_open_t  open;
    vfs_close_t close;
    
    // Pointer reserved for driver-specific private data (e.g., memory offset pointer)
    void *device_data; 
} vfs_node_t;

// Standard VFS Kernel API
uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer);
vfs_node_t *vfs_lookup(vfs_node_t *root, const char *path);

#endif
