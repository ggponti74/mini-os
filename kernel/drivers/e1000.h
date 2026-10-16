#ifndef E1000_H
#define E1000_H

#include <stdint.h>

#include "../display.h"

static inline uint32_t e1000_read_reg(uint32_t, uint16_t);
static inline void e1000_write_reg(uint32_t, uint16_t,uint32_t);

void e1000_check_hardware(void);

void e1000_get_mac_address(uint32_t, uint8_t *);

void e1000_set_bar0(uint32_t);
uint32_t e1000_get_bar0(void);

void e1000_set_mmio_base(uint32_t);
uint32_t e1000_get_mmio_base(void);

#endif