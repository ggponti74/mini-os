#include "e1000.h"

uint32_t bar0;
uint32_t mmio_base;

void e1000_set_bar0(uint32_t value) { bar0 = value; }
uint32_t e1000_get_bar0() { return bar0; };

void e1000_set_mmio_base(uint32_t value) { mmio_base = value; }
uint32_t e1000_get_mmio_base() { return mmio_base; };

void e1000_check_hardware() {
  uint32_t ctrl = e1000_read_reg(mmio_base, 0x0000);
  uint32_t status = e1000_read_reg(mmio_base, 0x0008);

  print_hex32(ctrl);
  kputchar('\n');
  print_hex32(status);
  kputchar('\n');
}

static inline uint32_t e1000_read_reg(uint32_t mmio_base, uint16_t offset) {
  volatile uint32_t *reg = (volatile uint32_t *)(uintptr_t)(mmio_base + offset);
  return *reg;
}

static inline void e1000_write_reg(uint32_t mmio_base, uint16_t offset,
                                   uint32_t value) {
  volatile uint32_t *reg = (volatile uint32_t *)(uintptr_t)(mmio_base + offset);
  *reg = value;
}

void e1000_get_mac_address(uint32_t mmio_base, uint8_t *mac_out) {
    
    uint32_t ral = e1000_read_reg(mmio_base, 0x5400);
    uint32_t rah = e1000_read_reg(mmio_base, 0x5404);

    mac_out[0] = (uint8_t)(ral & 0xFF);
    mac_out[1] = (uint8_t)((ral >> 8) & 0xFF);
    mac_out[2] = (uint8_t)((ral >> 16) & 0xFF);
    mac_out[3] = (uint8_t)((ral >> 24) & 0xFF);
    mac_out[4] = (uint8_t)(rah & 0xFF);
    mac_out[5] = (uint8_t)((rah >> 8) & 0xFF);
}