#include <stddef.h>

#include "../../drivers/e1000.h"
#include "../../drivers/pci.h"
#include "../../string.h"
#include "network.h"

static net_device_t *default_net_dev;
static net_device_t e1000_device;
static uint8_t ipv4_address[4];
static uint8_t ipv4_subnet_mask[4];
static uint8_t ipv4_gateway[4];
static int ipv4_configured;

int dhcp_request_ipv4(net_device_t *dev, uint8_t address[4],
                      uint8_t subnet_mask[4], uint8_t gateway[4]);
int network_ping_ipv4(net_device_t *dev, const uint8_t address[4],
                     const uint8_t subnet_mask[4], const uint8_t gateway[4],
                     const uint8_t destination[4]);

void hal_network_init(void) {
  for (int bus = 0; bus < 256; bus++) {
    for (int slot = 0; slot < 32; slot++) {
      for (int func = 0; func < 8; func++) {
        uint16_t vendor = pci_get_vendor_id(bus, slot, func);
        if (vendor == 0xFFFF) {
          continue;
        }

        uint16_t device = pci_get_device_id(bus, slot, func);
        if (vendor != 0x8086 || device != 0x100E) {
          continue;
        }

        uint32_t command = pci_config_read(bus, slot, func, 0x04);
        command |= (1U << 1) | (1U << 2);
        pci_config_write(bus, slot, func, 0x04, command);

        uint32_t bar0 = pci_get_bar0(bus, slot, func);
        uint32_t mmio_base = bar0 & ~0xF;
        e1000_set_bar0(bar0);
        e1000_set_mmio_base(mmio_base);
        if (e1000_initialize(&e1000_device, mmio_base) == 0) {
          e1000_device.name[0] = 'e';
          e1000_device.name[1] = 'n';
          e1000_device.name[2] = '0';
          e1000_device.name[3] = '\0';
          hal_network_register_device(&e1000_device);
        }
        return;
      }
    }
  }
}

int hal_network_register_device(net_device_t *dev) {
  if (!dev) {
    return -1;
  }
  default_net_dev = dev;
  return 0;
}

int hal_network_send(const void *packet, size_t length) {
  if (!default_net_dev || !default_net_dev->send_packet) {
    return -1;
  }
  return default_net_dev->send_packet(default_net_dev, packet, length);
}

int hal_network_poll(void *packet, size_t max_length) {
  if (!default_net_dev || !default_net_dev->poll_packet) {
    return -1;
  }
  return default_net_dev->poll_packet(default_net_dev, packet, max_length);
}

int hal_network_configure_dhcp(void) {
  if (!default_net_dev) {
    return -1;
  }
  ipv4_configured = 0;
  memset(ipv4_address, 0, sizeof(ipv4_address));
  memset(ipv4_subnet_mask, 0, sizeof(ipv4_subnet_mask));
  memset(ipv4_gateway, 0, sizeof(ipv4_gateway));
  if (dhcp_request_ipv4(default_net_dev, ipv4_address, ipv4_subnet_mask,
                        ipv4_gateway) != 0) {
    return -2;
  }
  ipv4_configured = 1;
  return 0;
}

int hal_network_get_ipv4(uint8_t address[4]) {
  if (!address || !ipv4_configured) {
    return -1;
  }
  for (size_t i = 0; i < sizeof(ipv4_address); i++) {
    address[i] = ipv4_address[i];
  }
  return 0;
}

int hal_network_get_mac(uint8_t address[NET_MAC_LEN]) {
  if (!address || !default_net_dev) {
    return -1;
  }
  for (size_t i = 0; i < NET_MAC_LEN; i++) {
    address[i] = default_net_dev->mac_addr[i];
  }
  return 0;
}

int hal_network_ping_ipv4(const uint8_t destination[4]) {
  if (!destination || !default_net_dev || !ipv4_configured) {
    return -1;
  }
  return network_ping_ipv4(default_net_dev, ipv4_address, ipv4_subnet_mask,
                           ipv4_gateway, destination);
}

void hal_network_receive_callback(const void *packet, size_t length) {
  (void)packet;
  (void)length;
}
