#ifndef E1000_H
#define E1000_H

#include <stdint.h>

#include "../hal/network/network.h"

void e1000_check_hardware(void);

void e1000_get_mac_address(uint32_t, uint8_t *);
int e1000_initialize(net_device_t *dev, uint32_t mmio_base);

void e1000_set_bar0(uint32_t);
uint32_t e1000_get_bar0(void);

void e1000_set_mmio_base(uint32_t);
uint32_t e1000_get_mmio_base(void);

#endif