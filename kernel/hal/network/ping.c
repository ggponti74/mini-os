#include <stddef.h>
#include <stdint.h>

#include "../../rtc.h"
#include "../../string.h"
#include "network.h"

#define ETHERNET_HEADER_SIZE 14
#define IPV4_HEADER_SIZE 20
#define ARP_PACKET_SIZE 28
#define ICMP_HEADER_SIZE 8
#define ICMP_PAYLOAD_SIZE 32
#define ARP_TIMEOUT_SECONDS 3
#define ICMP_TIMEOUT_SECONDS 3
#define NETWORK_POLL_LIMIT 100000000

static uint16_t read_be16(const uint8_t *data) {
  return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static void write_be16(uint8_t *data, uint16_t value) {
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)value;
}

static uint16_t checksum(const uint8_t *data, size_t length) {
  uint32_t sum = 0;
  while (length >= 2) {
    sum += read_be16(data);
    data += 2;
    length -= 2;
  }
  if (length != 0) {
    sum += (uint16_t)data[0] << 8;
  }
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }
  return (uint16_t)~sum;
}

static int ipv4_equal(const uint8_t left[4], const uint8_t right[4]) {
  return left[0] == right[0] && left[1] == right[1] &&
         left[2] == right[2] && left[3] == right[3];
}

static int ipv4_is_zero(const uint8_t address[4]) {
  return address[0] == 0 && address[1] == 0 && address[2] == 0 &&
         address[3] == 0;
}

static int ipv4_is_same_subnet(const uint8_t left[4], const uint8_t right[4],
                               const uint8_t mask[4]) {
  for (size_t i = 0; i < 4; i++) {
    if ((left[i] & mask[i]) != (right[i] & mask[i])) {
      return 0;
    }
  }
  return 1;
}

static int wait_for_packet(const uint8_t *local_address,
                           const uint8_t *peer_address,
                           uint8_t *peer_mac) {
  uint8_t packet[NET_MAX_PACKET_SIZE];
  rtc_time_t start_time;
  rtc_read_datetime(&start_time);
  uint32_t start_epoch = rtc_to_epoch(&start_time);
  uint32_t polls_since_clock_read = 0;

  for (uint32_t polls = 0; polls < NETWORK_POLL_LIMIT; polls++) {
    int length = hal_network_poll(packet, sizeof(packet));
    if (length < 0) {
      return -1;
    }
    if (length >= ETHERNET_HEADER_SIZE + ARP_PACKET_SIZE &&
        read_be16(packet + 12) == 0x0806) {
      const uint8_t *arp = packet + ETHERNET_HEADER_SIZE;
      if (read_be16(arp) == 1 && read_be16(arp + 2) == 0x0800 &&
          arp[4] == 6 && arp[5] == 4 && read_be16(arp + 6) == 2 &&
          ipv4_equal(arp + 14, local_address) &&
          ipv4_equal(arp + 24, peer_address)) {
        for (size_t i = 0; i < NET_MAC_LEN; i++) {
          peer_mac[i] = arp[8 + i];
        }
        return 0;
      }
    }

    if (++polls_since_clock_read == 8192) {
      rtc_time_t current_time;
      rtc_read_datetime(&current_time);
      if (rtc_to_epoch(&current_time) - start_epoch >=
          ARP_TIMEOUT_SECONDS) {
        break;
      }
      polls_since_clock_read = 0;
    }
    __asm__ volatile("pause");
  }
  return 1;
}

static int resolve_arp(net_device_t *dev, const uint8_t local_address[4],
                       const uint8_t target_address[4], uint8_t target_mac[6]) {
  uint8_t packet[ETHERNET_HEADER_SIZE + ARP_PACKET_SIZE];
  memset(packet, 0, sizeof(packet));
  for (size_t i = 0; i < NET_MAC_LEN; i++) {
    packet[i] = 0xFF;
    packet[6 + i] = dev->mac_addr[i];
  }
  write_be16(packet + 12, 0x0806);

  uint8_t *arp = packet + ETHERNET_HEADER_SIZE;
  write_be16(arp, 1);
  write_be16(arp + 2, 0x0800);
  arp[4] = 6;
  arp[5] = 4;
  write_be16(arp + 6, 1);
  for (size_t i = 0; i < NET_MAC_LEN; i++) {
    arp[8 + i] = dev->mac_addr[i];
  }
  for (size_t i = 0; i < 4; i++) {
    arp[14 + i] = local_address[i];
    arp[24 + i] = target_address[i];
  }

  if (hal_network_send(packet, sizeof(packet)) != 0) {
    return -1;
  }
  return wait_for_packet(local_address, target_address, target_mac);
}

static int send_echo_request(net_device_t *dev, const uint8_t local_address[4],
                             const uint8_t destination[4],
                             const uint8_t destination_mac[6]) {
  uint8_t packet[ETHERNET_HEADER_SIZE + IPV4_HEADER_SIZE + ICMP_HEADER_SIZE +
                 ICMP_PAYLOAD_SIZE];
  memset(packet, 0, sizeof(packet));
  for (size_t i = 0; i < NET_MAC_LEN; i++) {
    packet[i] = destination_mac[i];
    packet[6 + i] = dev->mac_addr[i];
  }
  write_be16(packet + 12, 0x0800);

  uint8_t *ip = packet + ETHERNET_HEADER_SIZE;
  size_t icmp_length = ICMP_HEADER_SIZE + ICMP_PAYLOAD_SIZE;
  size_t ip_length = IPV4_HEADER_SIZE + icmp_length;
  ip[0] = 0x45;
  write_be16(ip + 2, (uint16_t)ip_length);
  write_be16(ip + 4, 0x4D4F);
  write_be16(ip + 6, 0x4000);
  ip[8] = 64;
  ip[9] = 1;
  for (size_t i = 0; i < 4; i++) {
    ip[12 + i] = local_address[i];
    ip[16 + i] = destination[i];
  }
  write_be16(ip + 10, checksum(ip, IPV4_HEADER_SIZE));

  uint8_t *icmp = ip + IPV4_HEADER_SIZE;
  icmp[0] = 8;
  icmp[1] = 0;
  write_be16(icmp + 4, 0x4D4F);
  write_be16(icmp + 6, 1);
  for (size_t i = 0; i < ICMP_PAYLOAD_SIZE; i++) {
    icmp[ICMP_HEADER_SIZE + i] = (uint8_t)i;
  }
  write_be16(icmp + 2, checksum(icmp, icmp_length));
  return hal_network_send(packet, sizeof(packet));
}

static int wait_for_echo_reply(const uint8_t local_address[4],
                               const uint8_t destination[4]) {
  uint8_t packet[NET_MAX_PACKET_SIZE];
  rtc_time_t start_time;
  rtc_read_datetime(&start_time);
  uint32_t start_epoch = rtc_to_epoch(&start_time);
  uint32_t polls_since_clock_read = 0;

  for (uint32_t polls = 0; polls < NETWORK_POLL_LIMIT; polls++) {
    int length = hal_network_poll(packet, sizeof(packet));
    if (length < 0) {
      return -1;
    }
    if (length >= ETHERNET_HEADER_SIZE + IPV4_HEADER_SIZE +
                      ICMP_HEADER_SIZE &&
        read_be16(packet + 12) == 0x0800) {
      const uint8_t *ip = packet + ETHERNET_HEADER_SIZE;
      size_t ip_header_length = (size_t)(ip[0] & 0x0F) * 4;
      uint16_t ip_length = read_be16(ip + 2);
      if ((ip[0] >> 4) == 4 && ip_header_length >= IPV4_HEADER_SIZE &&
          ip_length >= ip_header_length + ICMP_HEADER_SIZE &&
          ip_length <= (size_t)length - ETHERNET_HEADER_SIZE &&
          ip[9] == 1 && (read_be16(ip + 6) & 0x3FFF) == 0 &&
          ipv4_equal(ip + 12, destination) &&
          ipv4_equal(ip + 16, local_address)) {
        const uint8_t *icmp = ip + ip_header_length;
        size_t icmp_length = ip_length - ip_header_length;
        if (icmp[0] == 0 && icmp[1] == 0 &&
            read_be16(icmp + 4) == 0x4D4F &&
            read_be16(icmp + 6) == 1 && checksum(icmp, icmp_length) == 0) {
          return 0;
        }
      }
    }

    if (++polls_since_clock_read == 8192) {
      rtc_time_t current_time;
      rtc_read_datetime(&current_time);
      if (rtc_to_epoch(&current_time) - start_epoch >=
          ICMP_TIMEOUT_SECONDS) {
        break;
      }
      polls_since_clock_read = 0;
    }
    __asm__ volatile("pause");
  }
  return 1;
}

int network_ping_ipv4(net_device_t *dev, const uint8_t local_address[4],
                      const uint8_t subnet_mask[4], const uint8_t gateway[4],
                      const uint8_t destination[4]) {
  if (!dev || !local_address || !subnet_mask || !gateway || !destination ||
      ipv4_is_zero(local_address) || ipv4_is_zero(destination) ||
      ipv4_is_zero(subnet_mask)) {
    return -1;
  }

  const uint8_t *next_hop = destination;
  if (!ipv4_is_same_subnet(local_address, destination, subnet_mask)) {
    if (ipv4_is_zero(gateway)) {
      return -2;
    }
    next_hop = gateway;
  }

  uint8_t destination_mac[NET_MAC_LEN];
  int arp_status = resolve_arp(dev, local_address, next_hop, destination_mac);
  if (arp_status == 1) {
    return -3;
  }
  if (arp_status != 0) {
    return -4;
  }
  if (send_echo_request(dev, local_address, destination, destination_mac) != 0) {
    return -4;
  }

  int reply_status = wait_for_echo_reply(local_address, destination);
  if (reply_status == 1) {
    return -5;
  }
  return reply_status == 0 ? 0 : -4;
}
