#ifndef PCI_H
#define PCI_H

#include <stdint.h>

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_get_vendor_id(uint8_t bus, uint8_t slot, uint8_t func);
uint16_t pci_get_device_id(uint8_t bus, uint8_t slot, uint8_t func);
uint32_t pci_get_bar0(uint8_t bus, uint8_t slot, uint8_t func);

void pci_config_write(uint8_t, uint8_t, uint8_t, uint8_t, uint32_t);

#endif