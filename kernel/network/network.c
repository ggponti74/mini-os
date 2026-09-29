// hal/net.c
#include "hal/net.h"
#include <stddef.h>

static net_device_t *default_net_dev = NULL;

void hal_net_init(void) {
    // Placeholder: Search for PCI network devices (e.g., Intel e1000 or RTL8139)
    default_net_dev = NULL;
}

int hal_net_register_device(net_device_t *dev) {
    if (!dev) return -1;
    default_net_dev = dev;
    return 0;
}

int hal_net_send(const void *packet, size_t length) {
    if (!default_net_dev || !default_net_dev->send_packet) {
        return -1; // No network hardware loaded
    }
    return default_net_dev->send_packet(default_net_dev, packet, length);
}

void hal_net_receive_callback(const void *packet, size_t length) {
    // Placeholder: Route incoming Ethernet frame up to ARP / IPv4 / UDP handlers
    (void)packet;
    (void)length;
}