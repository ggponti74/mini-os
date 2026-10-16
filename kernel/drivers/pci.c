#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) |
                                  (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_get_vendor_id(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t reg = pci_config_read(bus, slot, func, 0x00);
    return (uint16_t)(reg & 0xFFFF);
}

uint16_t pci_get_device_id(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t reg = pci_config_read(bus, slot, func, 0x00);
    return (uint16_t)((reg >> 16) & 0xFFFF);
}

uint8_t pci_get_class_id(uint8_t bus, uint8_t slot, uint8_t func) {
    uint32_t reg = pci_config_read(bus, slot, func, 0x08);
    return (uint8_t)((reg >> 24) & 0xFF);
}

uint32_t pci_get_bar0(uint8_t bus, uint8_t slot, uint8_t func) {
    return pci_config_read(bus, slot, func, 0x10);
}

void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) |
                                  (func << 8) | (offset & 0xFC) | 0x80000000);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, val);
}