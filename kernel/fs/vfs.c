#include "vfs.h"

vfs_node_t *fs_root = NULL;

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node && node->read) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    if (node && node->write) {
        return node->write(node, offset, size, buffer);
    }
    return 0;
}

vfs_node_t *vfs_lookup(vfs_node_t *root, const char *path) {
    // Placeholder: Will search sub-nodes or tar tables matching path string
    (void)root;
    (void)path;
    return NULL;
}

vfs_node_t *vfs_create(vfs_node_t *parent, const char *name, uint32_t flags) {
    if (!parent || !name) {
        return NULL;
    }

    // Ensure parent node is a directory and implements the create function
    if ((parent->flags & FS_DIRECTORY) && parent->create) {
        return parent->create(parent, name, flags);
    }

    return NULL;
}