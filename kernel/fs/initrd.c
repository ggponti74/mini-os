// fs/initrd.c
#include <stdint.h>
#include <stddef.h>

extern const char _initrd_start[];
extern const char _initrd_end[];

// Standard POSIX Tar Header structure
typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];     // File size in Octal ASCII!
    char mtime[12];
    char chksum[8];
    char typeflag;
} __attribute__((packed)) tar_header_t;

// Helper to convert Octal ASCII string from tar header to uint32_t
static uint32_t oct2bin(const char *str, int size) {
    uint32_t n = 0;
    for (int i = 0; i < size && str[i] >= '0' && str[i] <= '7'; i++) {
        n = (n << 3) + (str[i] - '0');
    }
    return n;
}

void initrd_list_files(void) {
    const char *ptr = _initrd_start;

    while (ptr < _initrd_end) {
        tar_header_t *header = (tar_header_t *)ptr;

        // Tar archive end marker or empty block
        if (header->name[0] == '\0') break;

        uint32_t file_size = oct2bin(header->size, 11);
        
        // Print file name and size using your VGA driver
        // kprintf("File: %s, Size: %d bytes\n", header->name, file_size);

        // Advance pointer: Header size (512 bytes) + File size rounded up to next 512-byte boundary
        ptr += 512 + ((file_size + 511) & ~511);
    }
}