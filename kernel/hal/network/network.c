#include <stddef.h>

#include "../../display.h"
#include "../../drivers/pci.h"
#include "../../drivers/e1000.h"
#include "network.h"

static net_device_t *default_net_dev = NULL;

// Forward declarations for PCI functions
uint16_t pci_get_vendor_id(uint8_t bus, uint8_t slot, uint8_t func);
uint16_t pci_get_device_id(uint8_t bus, uint8_t slot, uint8_t func);
uint8_t pci_get_class_id(uint8_t bus, uint8_t slot, uint8_t func);

void hal_network_init(void) {
    // Simple brute-force scan across buses, slots, and functions
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            for (int func = 0; func < 8; func++) {
                uint16_t vendor = pci_get_vendor_id(bus, slot, func);
                if (vendor == 0xFFFF)
                    continue; // No device present

                uint16_t device = pci_get_device_id(bus, slot, func);

                // Check for Intel e1000 (82540EM is 0x100E)
                if (vendor == 0x8086 && device == 0x100E) {
                    kprint("[NET] Found Intel e1000 NIC!\n");

                    // 1. Enable Memory Space (bit 1) and Bus Master (bit 2) in Command Register
                    uint32_t cmd = pci_config_read(bus, slot, func, 0x04);
                    cmd |= (1 << 1) | (1 << 2);
                    pci_config_write(bus, slot, func, 0x04, cmd);

                    // 2. Fetch the actual BAR0 physical address from offset 0x10
                    uint32_t bar0 = pci_get_bar0(bus, slot, func);
                    uint32_t mmio_base = bar0 & ~0xF; // Mask lower attribute bits

                    e1000_set_bar0(bar0);
                    e1000_set_mmio_base(mmio_base);

                    // Optional: print mmio_base here to verify it's a real memory address (e.g., 0xFEB80000)
                    
                    return;
                }
            }
        }
    }

    kprint("[NET] No supported network card found.\n");
}

int hal_network_register_device(net_device_t *dev) {
  if (!dev)
    return -1;
  default_net_dev = dev;
  return 0;
}

int hal_network_send(const void *packet, size_t length) {
  if (!default_net_dev || !default_net_dev->send_packet) {
    return -1; // No network hardware loaded
  }
  return default_net_dev->send_packet(default_net_dev, packet, length);
}

void hal_network_receive_callback(const void *packet, size_t length) {
  // Placeholder: Route incoming Ethernet frame up to ARP / IPv4 / UDP handlers
  (void)packet;
  (void)length;
}