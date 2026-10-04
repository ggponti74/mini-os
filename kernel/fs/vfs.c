#include "vfs.h"

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