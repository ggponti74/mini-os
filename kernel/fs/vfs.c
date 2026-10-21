#include <stddef.h>
#include <stdint.h>
#if defined(__has_include) && __has_include("display.h")
#include "display.h"
#else
#include "../display.h"
#endif
#include "vfs.h"
#include "initrd.h"

vfs_node_t *fs_root = NULL;

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    display_set_fs_indicator(FS_IND_READ,FS_IND_IDLE);
    if (!node || !buffer || size == 0) {
        return 0;
    }
    if (node && node->read) {
        return node->read(node, offset, size, buffer);
    }
    return 0;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    display_set_fs_indicator(FS_IND_IDLE,FS_IND_WRITE);
    if (!node || !buffer || size == 0) {
        return 0;
    }
    if (node && node->write) {
        return node->write(node, offset, size, buffer);
    }
    return 0;
}

vfs_node_t *vfs_lookup(vfs_node_t *root, const char *path) {
    if (!path) {
        return NULL;
    }
    if (!root) {
        root = fs_root;
    }
    while (*path == '/') {
        path++;
    }
    if (*path == '.' && (path[1] == '\0' || (path[1] == '.' && path[2] == '\0'))) {
        return root;
    }
    if (*path == '\0') {
        return root;
    }
    return initrd_find_file(path);
}

vfs_node_t *vfs_create(vfs_node_t *parent, const char *name, uint32_t flags) {
    display_set_fs_indicator(FS_IND_WRITE, FS_IND_IDLE);
    if (!parent) {
        parent = fs_root;
    }
    while (name && *name == '/') {
        name++;
    }
    if (!parent || !name) {
        return NULL;
    }

    char component[128];
    while (*name) {
        size_t length = 0;
        while (name[length] != '\0' && name[length] != '/') {
            if (length >= sizeof(component) - 1) {
                return NULL;
            }
            component[length] = name[length];
            length++;
        }
        if (length == 0) {
            return NULL;
        }
        component[length] = '\0';
        name += length;

        if (*name == '/') {
            name++;
            if (*name == '\0' || *name == '/') {
                return NULL;
            }
            if (length == 1 && component[0] == '.') {
                continue;
            }
            if (length == 2 && component[0] == '.' && component[1] == '.') {
                return NULL;
            }
            char path[128];
            size_t parent_length = 0;
            while (parent->name[parent_length] != '\0') {
                if (parent_length >= sizeof(path) - 1) {
                    return NULL;
                }
                path[parent_length] = parent->name[parent_length];
                parent_length++;
            }
            if (parent_length > 0) {
                if (parent_length + 1 >= sizeof(path)) {
                    return NULL;
                }
                path[parent_length++] = '/';
            }
            if (parent_length + length >= sizeof(path)) {
                return NULL;
            }
            for (size_t i = 0; i < length; i++) {
                path[parent_length + i] = component[i];
            }
            path[parent_length + length] = '\0';
            parent = initrd_find_file(path);
            if (!parent || !(parent->flags & FS_DIRECTORY)) {
                return NULL;
            }
        } else {
            if ((length == 1 && component[0] == '.') ||
                (length == 2 && component[0] == '.' && component[1] == '.')) {
                return NULL;
            }
            if ((parent->flags & FS_DIRECTORY) && parent->create) {
                return parent->create(parent, component, flags);
            }
            return NULL;
        }
    }

    return NULL;
}