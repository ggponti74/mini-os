// src/fs/initrd.c
#include "../display.h"
#include "initrd.h"
#include "vfs.h"

extern const char initrd_start[];
extern const char initrd_end[];

// Helper to convert octal ASCII string in tar header to integer
static uint32_t oct2bin(const char *str, int size) {
    uint32_t n = 0;
    for (int i = 0; i < size && str[i] >= '0' && str[i] <= '7'; i++) {
        n = (n << 3) + (str[i] - '0');
    }
    return n;
}

void initrd_list_files(void) {
    const char *ptr = initrd_start;

    while (ptr < initrd_end) {
        tar_header_t *header = (tar_header_t *)ptr;

        // Tar archive end marker or empty block
        if (header->name[0] == '\0') {
            break;
        }

        uint32_t file_size = oct2bin(header->size, 11);

        // Skip non-file directory entries like "." or "./"
        if (header->name[0] != '.' || header->name[1] != '\0') {
            kprint_color("  ", COLOR_DEFAULT);
            kprint_color(header->name, COLOR_DEFAULT); // Print filename in high-white
            kprint_color("  \n", COLOR_DEFAULT);
        }

        // Advance pointer: Header size (512 bytes) + File size rounded up to next 512-byte boundary
        ptr += 512 + ((file_size + 511) & ~511);
    }
}

// Low-level read implementation backed by memcpy
static uint32_t initrd_read_file(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (offset >= node->length) return 0;
    if (offset + size > node->length) {
        size = node->length - offset;
    }
    
    const uint8_t *src = (const uint8_t *)node->device_data + offset;
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = src[i];
    }
    return size;
}

const char *initrd_find_file(const char *filename, uint32_t *out_size) {
    if (out_size) {
        *out_size = 0;
    }
    if (!filename) {
        return NULL;
    }

    const char *ptr = initrd_start;

    while (ptr < initrd_end) {
        tar_header_t *header = (tar_header_t *)ptr;

        if (header->name[0] == '\0') {
            break;
        }

        uint32_t file_size = oct2bin(header->size, 11);
        const char *data = ptr + 512;

        // Strip leading "./" if present for comparison flexibility
        uint32_t name_offset = 0;
        if (header->name[0] == '.' && header->name[1] == '/') {
            name_offset = 2;
        }

        uint32_t name_length = sizeof(header->name) - name_offset;
        uint32_t i = 0;
        while (i < name_length && filename[i] != '\0' &&
               filename[i] == header->name[name_offset + i]) {
            i++;
        }

        if (filename[i] == '\0' &&
            (i == name_length || header->name[name_offset + i] == '\0')) {
            if (out_size) {
                *out_size = file_size;
            }
            return data;
        }

        ptr += 512 + ((file_size + 511) & ~511);
    }

    return NULL;
}

void initrd_cat_file(const char *filename) {
    uint32_t file_size = 0;
    const char *data = initrd_find_file(filename, &file_size);
    if (!data) {
    kprint_color("cat: file not found: ", COLOR_DEFAULT);
    kprint_color(filename, COLOR_DEFAULT);
    kprint_color("\n", COLOR_DEFAULT);
        return;
    }

    for (uint32_t i = 0; i < file_size; i++) {
        kputchar_color(data[i], COLOR_DEFAULT);
    }
    if (file_size > 0 && data[file_size - 1] != '\n') {
        kputchar_color('\n', COLOR_DEFAULT);
    }
}

vfs_node_t *initrd_init(void) {
    // Placeholder setup: Initialize root directory or traverse TAR headers
    // 1. Scan memory between _initrd_start and _initrd_end
    // 2. Map file targets to static vfs_node_t instances
    // 3. Assign node->read = initrd_read_file
    return NULL;
}
