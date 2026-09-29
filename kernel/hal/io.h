// hal/io.h
#ifndef HAL_IO_H
#define HAL_IO_H

#include <stdint.h>

static inline void hal_outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t hal_inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void hal_io_wait(void) {
    // Small delay for old hardware/ports
    hal_outb(0x80, 0);
}

#endif