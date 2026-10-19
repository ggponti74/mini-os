#include "e1000.h"

#include "../display.h"
#include "../string.h"

#define E1000_REG_CTRL 0x0000
#define E1000_REG_STATUS 0x0008
#define E1000_REG_TCTL 0x0400
#define E1000_REG_TIPG 0x0410
#define E1000_REG_RCTL 0x0100
#define E1000_REG_RDBAL 0x2800
#define E1000_REG_RDBAH 0x2804
#define E1000_REG_RDLEN 0x2808
#define E1000_REG_RDH 0x2810
#define E1000_REG_RDT 0x2818
#define E1000_REG_TDBAL 0x3800
#define E1000_REG_TDBAH 0x3804
#define E1000_REG_TDLEN 0x3808
#define E1000_REG_TDH 0x3810
#define E1000_REG_TDT 0x3818

#define E1000_RX_DESC_COUNT 16
#define E1000_TX_DESC_COUNT 8
#define E1000_RX_BUFFER_SIZE 2048
#define E1000_TX_BUFFER_SIZE 1518
#define E1000_DESC_DONE 0x01

typedef struct {
  uint32_t address_low;
  uint32_t address_high;
  uint16_t length;
  uint8_t checksum;
  uint8_t command;
  volatile uint8_t status;
  uint8_t checksum_start;
  uint16_t special;
} e1000_tx_desc_t;

typedef struct {
  uint32_t address_low;
  uint32_t address_high;
  uint16_t length;
  uint16_t checksum;
  volatile uint8_t status;
  uint8_t errors;
  uint16_t special;
} e1000_rx_desc_t;

static e1000_tx_desc_t tx_descriptors[E1000_TX_DESC_COUNT]
    __attribute__((aligned(16)));
static e1000_rx_desc_t rx_descriptors[E1000_RX_DESC_COUNT]
    __attribute__((aligned(16)));
static uint8_t tx_buffers[E1000_TX_DESC_COUNT][E1000_TX_BUFFER_SIZE]
    __attribute__((aligned(16)));
static uint8_t rx_buffers[E1000_RX_DESC_COUNT][E1000_RX_BUFFER_SIZE]
    __attribute__((aligned(16)));
static uint32_t tx_index;
static uint32_t rx_index;

uint32_t bar0;
uint32_t mmio_base;

static void copy_bytes(void *destination, const void *source, size_t length) {
  uint8_t *dst = destination;
  const uint8_t *src = source;
  for (size_t i = 0; i < length; i++) {
    dst[i] = src[i];
  }
}

static inline uint32_t e1000_read_reg(uint32_t base, uint16_t offset) {
  volatile uint32_t *reg = (volatile uint32_t *)(uintptr_t)(base + offset);
  return *reg;
}

static inline void e1000_write_reg(uint32_t base, uint16_t offset,
                                   uint32_t value) {
  volatile uint32_t *reg = (volatile uint32_t *)(uintptr_t)(base + offset);
  *reg = value;
}

void e1000_set_bar0(uint32_t value) { bar0 = value; }
uint32_t e1000_get_bar0() { return bar0; };

void e1000_set_mmio_base(uint32_t value) { mmio_base = value; }
uint32_t e1000_get_mmio_base() { return mmio_base; };

void e1000_check_hardware(void) {
  uint32_t ctrl = e1000_read_reg(mmio_base, 0x0000);
  uint32_t status = e1000_read_reg(mmio_base, 0x0008);

  print_hex32(ctrl);
  kputchar('\n');
  print_hex32(status);
  kputchar('\n');
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

static int e1000_send_packet(net_device_t *dev, const void *buf, size_t len) {
  (void)dev;
  if (!buf || len == 0 || len > E1000_TX_BUFFER_SIZE) {
    return -1;
  }

  e1000_tx_desc_t *desc = &tx_descriptors[tx_index];
  for (uint32_t timeout = 0; !(desc->status & E1000_DESC_DONE); timeout++) {
    if (timeout == 1000000) {
      return -1;
    }
    __asm__ volatile("pause");
  }

  copy_bytes(tx_buffers[tx_index], buf, len);
  desc->length = (uint16_t)len;
  desc->checksum = 0;
  desc->command = 0x0B;
  desc->status = 0;

  uint32_t next = (tx_index + 1) % E1000_TX_DESC_COUNT;
  __asm__ volatile("" : : : "memory");
  e1000_write_reg(mmio_base, E1000_REG_TDT, next);
  tx_index = next;
  return 0;
}

static int e1000_poll_packet(net_device_t *dev, void *buf, size_t max_len) {
  (void)dev;
  if (!buf || max_len == 0) {
    return -1;
  }

  e1000_rx_desc_t *desc = &rx_descriptors[rx_index];
  if (!(desc->status & E1000_DESC_DONE)) {
    return 0;
  }
  __asm__ volatile("" : : : "memory");

  size_t packet_len = desc->length;
  if (packet_len > max_len) {
    desc->status = 0;
    e1000_write_reg(mmio_base, E1000_REG_RDT, rx_index);
    rx_index = (rx_index + 1) % E1000_RX_DESC_COUNT;
    return -1;
  }
  copy_bytes(buf, rx_buffers[rx_index], packet_len);

  desc->status = 0;
  __asm__ volatile("" : : : "memory");
  e1000_write_reg(mmio_base, E1000_REG_RDT, rx_index);
  rx_index = (rx_index + 1) % E1000_RX_DESC_COUNT;
  return (int)packet_len;
}

int e1000_initialize(net_device_t *dev, uint32_t base) {
  if (!dev || base == 0) {
    return -1;
  }

  mmio_base = base;
  e1000_write_reg(base, E1000_REG_CTRL, 1U << 26);
  for (uint32_t timeout = 0;
       (e1000_read_reg(base, E1000_REG_CTRL) & (1U << 26)) != 0;
       timeout++) {
    if (timeout == 1000000) {
      return -1;
    }
    __asm__ volatile("pause");
  }

  memset(tx_descriptors, 0, sizeof(tx_descriptors));
  memset(rx_descriptors, 0, sizeof(rx_descriptors));
  memset(tx_buffers, 0, sizeof(tx_buffers));
  memset(rx_buffers, 0, sizeof(rx_buffers));

  for (uint32_t i = 0; i < E1000_TX_DESC_COUNT; i++) {
    tx_descriptors[i].address_low =
        (uint32_t)(uintptr_t)&tx_buffers[i][0];
    tx_descriptors[i].status = E1000_DESC_DONE;
  }
  for (uint32_t i = 0; i < E1000_RX_DESC_COUNT; i++) {
    rx_descriptors[i].address_low =
        (uint32_t)(uintptr_t)&rx_buffers[i][0];
  }

  __asm__ volatile("" : : : "memory");
  e1000_write_reg(base, E1000_REG_TDBAL,
                  (uint32_t)(uintptr_t)tx_descriptors);
  e1000_write_reg(base, E1000_REG_TDBAH, 0);
  e1000_write_reg(base, E1000_REG_TDLEN,
                  sizeof(tx_descriptors));
  e1000_write_reg(base, E1000_REG_TDH, 0);
  e1000_write_reg(base, E1000_REG_TDT, 0);
  e1000_write_reg(base, E1000_REG_TIPG, 0x0060200A);
  e1000_write_reg(base, E1000_REG_TCTL,
                  (1U << 1) | (1U << 3) | (0x10U << 4) | (0x40U << 12));

  e1000_write_reg(base, E1000_REG_RDBAL,
                  (uint32_t)(uintptr_t)rx_descriptors);
  e1000_write_reg(base, E1000_REG_RDBAH, 0);
  e1000_write_reg(base, E1000_REG_RDLEN,
                  sizeof(rx_descriptors));
  e1000_write_reg(base, E1000_REG_RDH, 0);
  e1000_write_reg(base, E1000_REG_RDT, E1000_RX_DESC_COUNT - 1);
  e1000_write_reg(base, E1000_REG_RCTL,
                  (1U << 1) | (1U << 15) | (1U << 26));

  tx_index = 0;
  rx_index = 0;
  e1000_get_mac_address(base, dev->mac_addr);
  dev->send_packet = e1000_send_packet;
  dev->poll_packet = e1000_poll_packet;
  return 0;
}