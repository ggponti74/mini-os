// src/fs/initrd.c
#include <stddef.h>
#include <stdint.h>
#include "initrd.h"
#include "vfs.h"
#include "../display.h"

#define MAX_NODES 32
#define MAX_FILE_SIZE 1024

typedef struct {
    vfs_node_t node;
    uint8_t data[MAX_FILE_SIZE];
} initrd_file_t;

static initrd_file_t file_pool[MAX_NODES];
static uint32_t node_count = 0;
static vfs_node_t initrd_root;

// Internal helper: Compare string up to max length
static int string_equals(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int initrd_get_file(const char *filename, uint8_t **out_data, uint32_t *out_size) {
    const char *ptr = initrd_start;

    while (ptr < initrd_end) {
        tar_header_t *header = (tar_header_t *)ptr;

        // End of TAR archive marker
        if (header->name[0] == '\0') {
            break;
        }

        uint32_t file_size = oct2bin(header->size, 11);

        // Strip leading "./" if present before matching
        const char *hn = header->name;
        if (hn[0] == '.' && hn[1] == '/') {
            hn += 2;
        }
        const char *fn = filename;
        if (fn[0] == '.' && fn[1] == '/') {
            fn += 2;
        }
        if (string_equals(hn, fn) == 0) {
            if (out_data) {
                *out_data = (uint8_t *)(ptr + 512); // File payload starts right after header block
            }
            if (out_size) {
                *out_size = file_size;
            }
            return 0; // Success
        }

        // Advance pointer past header (512 bytes) + file payload (rounded to 512-byte block boundaries)
        ptr += 512 + ((file_size + 511) & ~511);
    }

    return -1; // File not found
}

uint32_t oct2bin(const char *str, int size) {
    uint32_t n = 0;
    for (int i = 0; i < size && str[i] >= '0' && str[i] <= '7'; i++) {
        n = (n << 3) + (str[i] - '0');
    }
    return n;
}

// 1. Find/Lookup a file by name inside our dynamic in-memory file pool
vfs_node_t *initrd_find_file(const char *filename) {
    if (!filename) return NULL;

    for (uint32_t i = 0; i < node_count; i++) {
        // Strip leading "./" if present
        const char *entry_name = file_pool[i].node.name;
        if (entry_name[0] == '.' && entry_name[1] == '/') {
            entry_name += 2;
        }

        const char *target = filename;
        if (target[0] == '.' && target[1] == '/') {
            target += 2;
        }

        int match = 1;
        int j = 0;
        while (target[j] != '\0' || entry_name[j] != '\0') {
            if (target[j] != entry_name[j]) {
                match = 0;
                break;
            }
            j++;
        }

        if (match) {
            return &file_pool[i].node;
        }
    }

    return NULL; // Not found
}

// 2. List all files currently residing in the RAMDisk pool (including newly touched files)
void initrd_list_files(void) {
    kprint_color("  .\n", COLOR_DEFAULT);
    kprint_color("  ..\n", COLOR_DEFAULT);
    for (uint32_t i = 0; i < node_count; i++) {
        kprint_color("  ", COLOR_DEFAULT);
        kprint_color(file_pool[i].node.name, COLOR_DEFAULT);
        kputchar_color('\n', COLOR_DEFAULT);
    }
}

// 3. Print contents of a file using vfs_read or direct pool access
void initrd_cat_file(const char *filename) {
    vfs_node_t *file = initrd_find_file(filename);

    if (!file) {
        kprint_color("cat: file not found: ", COLOR_DEFAULT);
        kprint_color(filename, COLOR_DEFAULT);
        kputchar_color('\n', COLOR_DEFAULT);
        return;
    }

    uint8_t buf;
    for (uint32_t i = 0; i < file->length; i++) {
        if (vfs_read(file, i, 1, &buf) > 0) {
            kputchar_color((char)buf, COLOR_DEFAULT);
        }
    }

    if (file->length > 0 && ((uint8_t *)file->device_data)[file->length - 1] != '\n') {
        kputchar_color('\n', COLOR_DEFAULT);
    }
}

static uint32_t initrd_read_file(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (!node || !node->device_data || offset >= node->length) return 0;
    if (offset + size > node->length) size = node->length - offset;

    const uint8_t *src = (const uint8_t *)node->device_data + offset;
    for (uint32_t i = 0; i < size; i++) buffer[i] = src[i];
    return size;
}

static uint32_t initrd_write_file(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    if (!node || !node->device_data || !buffer) return 0;
    if (offset >= MAX_FILE_SIZE) return 0;
    uint8_t *dest = (uint8_t *)node->device_data;

    if (offset + size > MAX_FILE_SIZE) size = MAX_FILE_SIZE - offset;
    for (uint32_t i = 0; i < size; i++) dest[offset + i] = buffer[i];

    if (offset + size > node->length) node->length = offset + size;
    return size;
}

static vfs_node_t *initrd_create_file(vfs_node_t *parent, const char *name, uint32_t flags) {
    (void)parent;
    if (node_count >= MAX_NODES) return NULL;

    initrd_file_t *item = &file_pool[node_count++];

    // Strip leading "./" if present
    if (name[0] == '.' && name[1] == '/') {
        name += 2;
    }
    
    // Copy filename
    int i = 0;
    while (name[i] != '\0' && i < 127 && name[i] != '/') {
        item->node.name[i] = name[i];
        i++;
    }
    item->node.name[i] = '\0';

    item->node.flags = flags;
    item->node.length = 0;
    item->node.read = initrd_read_file;
    item->node.write = initrd_write_file;
    item->node.device_data = item->data;

    return &item->node;
}

vfs_node_t *initrd_init(void) {
    // 1. Setup Root Directory Node
    initrd_root.flags = FS_DIRECTORY;
    initrd_root.create = initrd_create_file;

    // 2. Parse TAR Archive from initrd_start to initrd_end
    const char *ptr = initrd_start;

    while (ptr < initrd_end) {
        tar_header_t *header = (tar_header_t *)ptr;
        if (header->name[0] == '\0') break;

        uint32_t file_size = oct2bin(header->size, 11);

        // Skip directory headers and '.' / './' archive entries
        if (header->typeflag != '5' &&
            !(header->name[0] == '.' && (header->name[1] == '\0' || (header->name[1] == '/' && header->name[2] == '\0')))) {
            vfs_node_t *node = initrd_create_file(&initrd_root, header->name, FS_FILE);
            if (node) {
                const uint8_t *file_data = (const uint8_t *)(ptr + 512);
                initrd_write_file(node, 0, file_size, file_data);
            }
        }

        ptr += 512 + ((file_size + 511) & ~511);
    }

    return &initrd_root;
}