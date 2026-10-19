#include <stddef.h>
#include <stdint.h>

#include "../../rtc.h"
#include "../../string.h"
#include "network.h"

#define ETHERNET_HEADER_SIZE 14
#define IPV4_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8
#define DHCP_FIXED_SIZE 240
#define DHCP_XID 0x4D4F5344
#define DHCP_POLL_LIMIT 100000000
#define DHCP_TIMEOUT_SECONDS 3

enum {
  DHCP_DISCOVER = 1,
  DHCP_OFFER = 2,
  DHCP_REQUEST = 3,
  DHCP_ACK = 5
};

static uint16_t read_be16(const uint8_t *data) {
  return (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static uint32_t read_be32(const uint8_t *data) {
  return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
         ((uint32_t)data[2] << 8) | data[3];
}

static int address_is_zero(const uint8_t address[4]) {
  return address[0] == 0 && address[1] == 0 && address[2] == 0 &&
         address[3] == 0;
}

static int addresses_equal(const uint8_t left[4], const uint8_t right[4]) {
  return left[0] == right[0] && left[1] == right[1] &&
         left[2] == right[2] && left[3] == right[3];
}

static void write_be16(uint8_t *data, uint16_t value) {
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)value;
}

static void write_be32(uint8_t *data, uint32_t value) {
  data[0] = (uint8_t)(value >> 24);
  data[1] = (uint8_t)(value >> 16);
  data[2] = (uint8_t)(value >> 8);
  data[3] = (uint8_t)value;
}

static uint16_t ipv4_checksum(const uint8_t *header, size_t length) {
  uint32_t sum = 0;
  for (size_t i = 0; i < length; i += 2) {
    sum += ((uint16_t)header[i] << 8) | header[i + 1];
  }
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }
  return (uint16_t)~sum;
}

static size_t append_option(uint8_t *options, size_t offset, uint8_t code,
                            const uint8_t *value, uint8_t length) {
  options[offset++] = code;
  options[offset++] = length;
  for (uint8_t i = 0; i < length; i++) {
    options[offset++] = value[i];
  }
  return offset;
}

static int send_dhcp_message(net_device_t *dev, uint8_t message_type,
                             const uint8_t *requested_ip,
                             const uint8_t *server_id) {
  uint8_t packet[NET_MAX_PACKET_SIZE];
  memset(packet, 0, sizeof(packet));

  for (size_t i = 0; i < 6; i++) {
    packet[i] = 0xFF;
    packet[6 + i] = dev->mac_addr[i];
  }
  write_be16(packet + 12, 0x0800);

  uint8_t *ip = packet + ETHERNET_HEADER_SIZE;
  uint8_t *udp = ip + IPV4_HEADER_SIZE;
  uint8_t *bootp = udp + UDP_HEADER_SIZE;
  uint8_t *options = bootp + DHCP_FIXED_SIZE;

  bootp[0] = 1;
  bootp[1] = 1;
  bootp[2] = 6;
  write_be32(bootp + 4, DHCP_XID);
  write_be16(bootp + 10, 0x8000);
  for (size_t i = 0; i < 6; i++) {
    bootp[28 + i] = dev->mac_addr[i];
  }
  write_be32(bootp + 236, 0x63825363);

  size_t options_length = 0;
  options_length = append_option(options, options_length, 53, &message_type, 1);
  if (message_type == DHCP_DISCOVER) {
    const uint8_t parameter_request_list[] = {1, 3, 6, 51};
    options_length = append_option(options, options_length, 55,
                                   parameter_request_list,
                                   sizeof(parameter_request_list));
  } else {
    options_length = append_option(options, options_length, 50, requested_ip, 4);
    options_length = append_option(options, options_length, 54, server_id, 4);
    const uint8_t parameter_request_list[] = {1, 3, 6, 51};
    options_length = append_option(options, options_length, 55,
                                   parameter_request_list,
                                   sizeof(parameter_request_list));
  }
  options[options_length++] = 255;

  size_t dhcp_length = DHCP_FIXED_SIZE + options_length;
  size_t udp_length = UDP_HEADER_SIZE + dhcp_length;
  size_t ip_length = IPV4_HEADER_SIZE + udp_length;
  size_t packet_length = ETHERNET_HEADER_SIZE + ip_length;

  ip[0] = 0x45;
  write_be16(ip + 2, (uint16_t)ip_length);
  write_be16(ip + 4, 1);
  ip[8] = 64;
  ip[9] = 17;
  ip[12] = 0;
  ip[13] = 0;
  ip[14] = 0;
  ip[15] = 0;
  ip[16] = 255;
  ip[17] = 255;
  ip[18] = 255;
  ip[19] = 255;
  write_be16(ip + 10, ipv4_checksum(ip, IPV4_HEADER_SIZE));

  write_be16(udp, 68);
  write_be16(udp + 2, 67);
  write_be16(udp + 4, (uint16_t)udp_length);
  write_be16(udp + 6, 0);

  return hal_network_send(packet, packet_length);
}

static int parse_dhcp_reply(const uint8_t *packet, size_t packet_length,
                            const net_device_t *dev, uint8_t expected_type,
                            uint8_t address[4], uint8_t server_id[4],
                            uint8_t subnet_mask[4], uint8_t gateway[4]) {
  const size_t minimum_length =
      ETHERNET_HEADER_SIZE + IPV4_HEADER_SIZE + UDP_HEADER_SIZE + DHCP_FIXED_SIZE;
  if (packet_length < minimum_length ||
      read_be16(packet + 12) != 0x0800) {
    return 0;
  }

  const uint8_t *ip = packet + ETHERNET_HEADER_SIZE;
  size_t ip_header_length = (size_t)(ip[0] & 0x0F) * 4;
  uint16_t ip_length = read_be16(ip + 2);
  if ((ip[0] >> 4) != 4 || ip_header_length < IPV4_HEADER_SIZE ||
      ip_length < ip_header_length + UDP_HEADER_SIZE + DHCP_FIXED_SIZE ||
      ip_length > packet_length - ETHERNET_HEADER_SIZE ||
      ip[9] != 17 || (read_be16(ip + 6) & 0x3FFF) != 0) {
    return 0;
  }

  const uint8_t *udp = ip + ip_header_length;
  uint16_t udp_length = read_be16(udp + 4);
  if (read_be16(udp) != 67 || read_be16(udp + 2) != 68 ||
      udp_length < UDP_HEADER_SIZE + DHCP_FIXED_SIZE ||
      udp_length > ip_length - ip_header_length) {
    return 0;
  }

  const uint8_t *bootp = udp + UDP_HEADER_SIZE;
  size_t bootp_length = udp_length - UDP_HEADER_SIZE;
  if (bootp[0] != 2 || bootp[1] != 1 || bootp[2] != 6 ||
      read_be32(bootp + 4) != DHCP_XID ||
      read_be32(bootp + 236) != 0x63825363) {
    return 0;
  }
  for (size_t i = 0; i < 6; i++) {
    if (bootp[28 + i] != dev->mac_addr[i]) {
      return 0;
    }
  }

  uint8_t message_type = 0;
  uint8_t found_server_id[4] = {ip[12], ip[13], ip[14], ip[15]};
  uint8_t found_subnet_mask[4] = {0};
  uint8_t found_gateway[4] = {0};
  size_t offset = DHCP_FIXED_SIZE;
  while (offset < bootp_length) {
    uint8_t code = bootp[offset++];
    if (code == 255) {
      break;
    }
    if (code == 0) {
      continue;
    }
    if (offset >= bootp_length) {
      return 0;
    }
    uint8_t option_length = bootp[offset++];
    if (option_length > bootp_length - offset) {
      return 0;
    }
    if (code == 53 && option_length == 1) {
      message_type = bootp[offset];
    } else if (code == 54 && option_length == 4) {
      for (size_t i = 0; i < 4; i++) {
        found_server_id[i] = bootp[offset + i];
      }
    } else if (code == 1 && option_length == 4) {
      for (size_t i = 0; i < 4; i++) {
        found_subnet_mask[i] = bootp[offset + i];
      }
    } else if (code == 3 && option_length >= 4) {
      for (size_t i = 0; i < 4; i++) {
        found_gateway[i] = bootp[offset + i];
      }
    }
    offset += option_length;
  }
  if (message_type != expected_type) {
    return 0;
  }

  for (size_t i = 0; i < 4; i++) {
    address[i] = bootp[16 + i];
    server_id[i] = found_server_id[i];
    subnet_mask[i] = found_subnet_mask[i];
    gateway[i] = found_gateway[i];
  }
  return !address_is_zero(address) && !address_is_zero(server_id);
}

static int wait_for_dhcp_reply(const net_device_t *dev, uint8_t expected_type,
                               const uint8_t expected_server_id[4],
                               uint8_t address[4], uint8_t server_id[4],
                               uint8_t subnet_mask[4], uint8_t gateway[4]) {
  uint8_t packet[NET_MAX_PACKET_SIZE];
  rtc_time_t start_time;
  rtc_read_datetime(&start_time);
  uint32_t start_epoch = rtc_to_epoch(&start_time);
  uint32_t polls_since_clock_read = 0;

  for (uint32_t polls = 0; polls < DHCP_POLL_LIMIT; polls++) {
    int length = hal_network_poll(packet, sizeof(packet));
    if (length > 0) {
      uint8_t received_address[4];
      uint8_t received_server_id[4];
      uint8_t received_subnet_mask[4];
      uint8_t received_gateway[4];
      if (parse_dhcp_reply(packet, (size_t)length, dev, expected_type,
                           received_address, received_server_id,
                           received_subnet_mask, received_gateway) &&
          (!expected_server_id ||
           addresses_equal(received_server_id, expected_server_id))) {
        for (size_t i = 0; i < 4; i++) {
          address[i] = received_address[i];
          server_id[i] = received_server_id[i];
          subnet_mask[i] = received_subnet_mask[i];
          gateway[i] = received_gateway[i];
        }
        return 0;
      }
    }

    if (++polls_since_clock_read == 8192) {
      rtc_time_t current_time;
      rtc_read_datetime(&current_time);
      if (rtc_to_epoch(&current_time) - start_epoch >=
          DHCP_TIMEOUT_SECONDS) {
        break;
      }
      polls_since_clock_read = 0;
    }
    __asm__ volatile("pause");
  }
  return -1;
}

int dhcp_request_ipv4(net_device_t *dev, uint8_t address[4],
                      uint8_t subnet_mask[4], uint8_t gateway[4]) {
  if (!dev || !address || !subnet_mask || !gateway) {
    return -1;
  }

  uint8_t offered_address[4] = {0};
  uint8_t server_id[4] = {0};
  uint8_t ignored_server_id[4] = {0};
  uint8_t ignored_subnet_mask[4] = {0};
  uint8_t ignored_gateway[4] = {0};
  int got_offer = 0;
  for (uint32_t attempt = 0; attempt < 2 && !got_offer; attempt++) {
    if (send_dhcp_message(dev, DHCP_DISCOVER, NULL, NULL) != 0) {
      return -1;
    }
    got_offer = wait_for_dhcp_reply(
                    dev, DHCP_OFFER, NULL, offered_address, server_id,
                    ignored_subnet_mask, ignored_gateway) == 0;
  }
  if (!got_offer) {
    return -1;
  }

  for (uint32_t attempt = 0; attempt < 2; attempt++) {
    if (send_dhcp_message(dev, DHCP_REQUEST, offered_address, server_id) != 0) {
      return -1;
    }
    if (wait_for_dhcp_reply(dev, DHCP_ACK, server_id, address,
                            ignored_server_id, subnet_mask, gateway) == 0) {
      return 0;
    }
  }
  return -1;
}
