// fs/blkdev.h
#ifndef BLKDEV_H
#define BLKDEV_H

#include <stdint.h>

#define BLOCK_SIZE 512

typedef struct block_device {
    const char *name;
    uint32_t total_blocks;
    
    // Low-level function pointers implemented by disk drivers
    int (*read_block)(struct block_device *dev, uint32_t block_num, void *buffer);
    int (*write_block)(struct block_device *dev, uint32_t block_num, const void *buffer);
} block_device_t;

// Standard driver registration
int blkdev_register(block_device_t *dev);
block_device_t* blkdev_get(const char *name);

#endif