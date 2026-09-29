// hal/net.h
#ifndef HAL_NETWORK_H
#define HAL_NETWORK_H

#include <stdint.h>
#include <stddef.h>

#define NET_MAC_LEN 6
#define NET_MAX_PACKET_SIZE 1518

typedef struct net_device {
    char name[16];
    uint8_t mac_addr[NET_MAC_LEN];
    
    // Low-level NIC Function Pointers
    int (*send_packet)(struct net_device *dev, const void *buf, size_t len);
    int (*poll_packet)(struct net_device *dev, void *buf, size_t max_len);
    
    void *priv_data; // Pointer to driver-specific data (e.g., e1000 registers)
} net_device_t;

// Network HAL Function Signatures (Placeholders)
void hal_NETWORK_init(void);
int hal_NETWORK_register_device(net_device_t *dev);
int hal_NETWORK_send(const void *packet, size_t length);
void hal_NETWORK_receive_callback(const void *packet, size_t length);

#endif