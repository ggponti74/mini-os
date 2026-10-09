// kernel/display.c
#include "display.h"
#include "io.h"
#include "rtc.h"
#include "shell.h"
#include "sys/metrics.h"
#include <stdint.h>

#define MAIN_SCREEN_HEIGHT 24 // Reserve Row 24 for the Status Line

// Color attributes
#define COLOR_STATUSBAR 0x1F // Bright white text on blue background

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static int sound_enabled = 1;
static fs_indicator_t current_fs_indicator = FS_IND_IDLE;
static int display_silent = 0;
static uint8_t current_color = COLOR_DEFAULT; // Bright white on black;

static const char hex_chars[] = "0123456789ABCDEF";

static ansi_state_t ansi_state = ANSI_STATE_NORMAL;
static int ansi_param = 0;

void set_text_color(uint8_t color) { current_color = color; }

// Map ANSI SGR codes (30-37, 90-97) to VGA attribute bytes
static uint8_t ansi_to_vga_color(int code) {
  switch (code) {
  case 0:
    return COLOR_DEFAULT; // Reset to default
  case 30:
    return 0x00; // Black
  case 31:
    return 0x04; // Red
  case 32:
    return 0x02; // Green
  case 33:
    return 0x06; // Yellow / Brown
  case 34:
    return 0x01; // Blue
  case 35:
    return 0x05; // Magenta
  case 36:
    return 0x03; // Cyan
  case 37:
    return 0x07; // Light Grey
  case 90:
    return 0x08; // Dark Grey
  case 91:
    return 0x0C; // Bright Red
  case 92:
    return 0x0A; // Bright Green
  case 93:
    return 0x0E; // Bright Yellow
  case 94:
    return 0x09; // Bright Blue
  case 95:
    return 0x0D; // Bright Magenta
  case 96:
    return 0x0B; // Bright Cyan
  case 97:
    return 0x0F; // Bright White
  default:
    return current_color;
  }
}

void print_hex8(uint8_t value) {
  kputchar_color(hex_chars[(value >> 4) & 0x0F], COLOR_DEFAULT);
  kputchar_color(hex_chars[value & 0x0F], COLOR_DEFAULT);
}

void print_hex32(uint32_t value) {
  kprint_color("0x", COLOR_DEFAULT);
  for (int i = 3; i >= 0; i--) {
    print_hex8((uint8_t)((value >> (i * 8)) & 0xFF));
  }
}

void display_set_fs_indicator(fs_indicator_t read_state,
                              fs_indicator_t write_state) {
  // ALL declarations at the very top
  uint16_t attr_idle;
  uint16_t attr_read;
  uint16_t attr_write;
  char fs_read_icon;
  uint16_t read_attr;
  char fs_write_icon;
  uint16_t write_attr;
  uint16_t *status_row;

  // Initialization & logic afterwards
  attr_idle = (COLOR_STATUSBAR << 8);
  attr_read = (0x2F << 8);
  attr_write = (0x4F << 8);

  fs_read_icon = ' ';
  read_attr = attr_idle;

  if (read_state == FS_IND_READ) {
    fs_read_icon = 'R';
    read_attr = attr_read;
  } else if (read_state == FS_IND_WRITE) {
    fs_read_icon = 'W';
    read_attr = attr_write;
  }

  fs_write_icon = ' ';
  write_attr = attr_idle;

  if (write_state == FS_IND_WRITE) {
    fs_write_icon = 'W';
    write_attr = attr_write;
  } else if (write_state == FS_IND_READ) {
    fs_write_icon = 'R';
    write_attr = attr_read;
  }

  status_row = (uint16_t *)(0xB8000 + (24 * 80 * 2));
  status_row[0] = (uint16_t)fs_read_icon | read_attr;
  status_row[1] = (uint16_t)fs_write_icon | write_attr;
}

void display_set_silent(int silent) { display_silent = silent; }

int display_is_silent(void) { return display_silent; }

// Simple integer to ASCII string converter
void itoa(uint32_t val, char *buf) {
  char temp[12];
  int i = 0;
  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return;
  }
  while (val > 0) {
    temp[i++] = (val % 10) + '0';
    val /= 10;
  }
  int j = 0;
  while (i > 0) {
    buf[j++] = temp[--i];
  }
  buf[j] = '\0';
}

void clear_screen(void) {
  uint16_t blank = ' ' | (COLOR_DEFAULT << 8);
  // Clear rows 0 to 23
  for (int i = 0; i < VGA_WIDTH * MAIN_SCREEN_HEIGHT; i++) {
    VGA_MEMORY[i] = blank;
  }
  cursor_x = 0;
  cursor_y = 0;
  update_hardware_cursor(cursor_x, cursor_y);
}

void scroll_screen(uint8_t override_color) {
  // 1. Shift lines 1..(MAIN_SCREEN_HEIGHT-1) up
  for (int y = 0; y < MAIN_SCREEN_HEIGHT - 1; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
    }
  }

  // 2. Clear the last line (row MAIN_SCREEN_HEIGHT - 1)
  uint16_t blank = ((uint16_t)override_color << 8) | ' ';
  for (int x = 0; x < VGA_WIDTH; x++) {
    VGA_MEMORY[(MAIN_SCREEN_HEIGHT - 1) * VGA_WIDTH + x] = blank;
  }

  // 3. Keep cursor locked to the bottom text row
  cursor_y = MAIN_SCREEN_HEIGHT - 1;
}

void kputchar(char c) {
    kputchar_color(c, 0); // Delegate single-argument calls to the color renderer
}

void kputchar_color(char c, uint8_t color) {
    if (display_is_silent()) {
        return;
    }

    // 1. ANSI Escape Sequence Parser
    if (ansi_state == ANSI_STATE_NORMAL) {
        if (c == 27) { // 0x1B ESC
            ansi_state = ANSI_STATE_ESC;
            return;
        }
    } else if (ansi_state == ANSI_STATE_ESC) {
        if (c == '[') {
            ansi_state = ANSI_STATE_CSI;
            ansi_param = 0;
            return;
        } else {
            ansi_state = ANSI_STATE_NORMAL;
        }
    } else if (ansi_state == ANSI_STATE_CSI) {
        if (c >= '0' && c <= '9') {
            ansi_param = ansi_param * 10 + (c - '0');
            return;
        } else if (c == 'm') { // SGR end marker
            current_color = ansi_to_vga_color(ansi_param);
            ansi_state = ANSI_STATE_NORMAL;
            return;
        } else {
            ansi_state = ANSI_STATE_NORMAL;
            return;
        }
    }

    // 2. Color Resolution
    uint8_t active_color = (color != 0) ? color : current_color;

    // 3. Character Rendering & Control Codes
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3;
    } else if (c == '\b' || c == 0x7F) {
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
        }
        int index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_MEMORY[index] = ((uint16_t)active_color << 8) | ' ';
    } else {
        int index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_MEMORY[index] = ((uint16_t)active_color << 8) | (uint8_t)c;
        cursor_x++;

        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    // 4. Boundary checks & scrolling using MAIN_SCREEN_HEIGHT (24)
    while (cursor_y >= MAIN_SCREEN_HEIGHT) {
        scroll_screen(COLOR_DEFAULT);
    }

    update_hardware_cursor(cursor_x, cursor_y);
}

void kprint_color(const char *str, uint8_t color) {
    if (color != 0) {
        current_color = color; // Only set if a non-zero override was given
    }
    for (int i = 0; str[i] != '\0'; i++) {
        kputchar(str[i]); // Let kputchar handle characters and inline ANSI codes uninterrupted
    }
}

void sound_set_enabled(int state) { sound_enabled = state; }

int sound_is_enabled(void) { return sound_enabled; }

void update_status_bar(void) {
  rtc_time_t rtc;
  rtc_read_datetime(&rtc);

  int tz = shell_get_timezone();
  if (tz != 0) {
    uint32_t epoch = rtc_to_epoch(&rtc);
    int32_t local_epoch = (int32_t)epoch + (tz * 3600);
    if (local_epoch >= 0) {
      epoch_to_rtc((uint32_t)local_epoch, &rtc);
    }
  }

  char am_pm = 'A';
  if(rtc.hour>12)
  {
    rtc.hour -= 12;
    am_pm = 'P';
  }

  char dt[20];
  uint32_t year = rtc.year;
  dt[0] = '0' + (rtc.month / 10);
  dt[1] = '0' + (rtc.month % 10);
  dt[2] = '/';
  dt[3] = '0' + (rtc.day / 10);
  dt[4] = '0' + (rtc.day % 10);
  dt[5] = '/';
  dt[6] = '0' + ((year / 1000) % 10);
  dt[7] = '0' + ((year / 100) % 10);
  dt[8] = '0' + ((year / 10) % 10);
  dt[9] = '0' + (year % 10);
  dt[10] = ' ';
  dt[11] = '0' + (rtc.hour / 10);
  dt[12] = '0' + (rtc.hour % 10);
  dt[13] = ':';
  dt[14] = '0' + (rtc.minute / 10);
  dt[15] = '0' + (rtc.minute % 10);
  dt[16] = ' ';
  dt[17] = am_pm;
  dt[18] = 'M';
  dt[19] = '\0';

  draw_status_bar(dt, sound_is_enabled());
}

// Helper to write formatted text to a specific VGA line
void draw_status_bar(const char *datetime_str, int sound_enabled) {
  uint16_t *status_row = (uint16_t *)(0xB8000 + (24 * 80 * 2));
  uint16_t bg_attr = (COLOR_STATUSBAR << 8);

  // 1. Fill row 24 with background color, SKIPPING columns 0 and 1
  uint16_t blank = ' ' | bg_attr;
  for (int col = 2; col < VGA_WIDTH;
       col++) { // Clears background from col 2 to 79
    status_row[col] = blank;
  }

  // 2. Query system metrics
  mem_stats_t mem = sys_get_mem_stats();
  uint32_t cpu_pct = sys_get_cpu_usage();

  // 3. Format Memory String (KB or MB)
  char mem_buf[16];
  if (mem.free_kb >= 1024) {
    itoa(mem.free_kb / 1024, mem_buf);
  } else {
    itoa(mem.free_kb, mem_buf);
  }

  // Format CPU String
  char cpu_buf[8];
  itoa(cpu_pct, cpu_buf);

  // 4. Render Left Section (Start at col 4 to leave col 2 and 3 as margin)
  int idx = 4;

  const char *title = "CPU: ";
  while (*title) {
    status_row[idx++] = (uint16_t)*title++ | bg_attr;
  }

  char *c_ptr = cpu_buf;
  while (*c_ptr) {
    status_row[idx++] = (uint16_t)*c_ptr++ | bg_attr;
  }
  status_row[idx++] = '%' | bg_attr;

  const char *ram_label = " RAM: ";
  while (*ram_label) {
    status_row[idx++] = (uint16_t)*ram_label++ | bg_attr;
  }

  const char *unit = " KB";

  if (mem.free_kb >= 1048576) { // >= 1 GB (1024 * 1024 KB)
    itoa(mem.free_kb / 1048576, mem_buf);
    unit = " GB";
  } else if (mem.free_kb >= 1024) { // >= 1 MB (1024 KB)
    itoa(mem.free_kb / 1024, mem_buf);
    unit = " MB";
  } else {
    itoa(mem.free_kb, mem_buf);
    unit = " KB";
  }

  char *m_ptr = mem_buf;
  while (*m_ptr) {
    status_row[idx++] = (uint16_t)*m_ptr++ | bg_attr;
  }

  // Unit string (GB / MB / KB)
  while (*unit) {
    status_row[idx++] = (uint16_t)*unit++ | bg_attr;
  }

  // 5. Render Right-Hand Side Status: Date, Time, Sound Indicator
  int pos = 58;
  char sound_icon = sound_enabled ? 14 : 'x';

  for (int i = 0; datetime_str[i] != '\0' && pos < 80; i++, pos++) {
    status_row[pos] = (uint16_t)datetime_str[i] | bg_attr;
  }

  if (pos < 80) {
    status_row[pos++] = ' ' | bg_attr;
  }

  if (pos < 80) {
    status_row[pos++] = (uint16_t)sound_icon | bg_attr;
  }
}

// 1. Set cursor scanline height (Block vs Underscore)
void set_hardware_cursor_shape(int is_block) {
  outb(0x3D4, 0x0A); // Cursor Start Register
  if (is_block) {
    outb(0x3D5, 0x00); // Scanline 0 (Top of cell -> Solid Block)
  } else {
    outb(0x3D5, 0x0E); // Scanline 14 (Bottom -> Underscore)
  }

  outb(0x3D4, 0x0B); // Cursor End Register
  outb(0x3D5, 0x0F); // Scanline 15 (Bottom edge)
}

// Disable hardware cursor
void disable_hardware_cursor(void) {
  outb(0x3D4, 0x0A);
  outb(0x3D5, 0x20);
}

// 2. Send current cursor position to VGA CRT controller
void update_hardware_cursor(int x, int y) {
  uint16_t pos = y * VGA_WIDTH + x;

  // Send low byte (Register 0x0F)
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));

  // Send high byte (Register 0x0E)
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// void draw_cursor(void) {
//   int index = cursor_y * VGA_WIDTH + cursor_x;
//   uint16_t current_cell = VGA_MEMORY[index];
//   char c = (char)(current_cell & 0xFF);
//   if (c == ' ' || c == 0)
//     c = ' '; // Standard blank cell

//   // Invert attribute (0x70 = Black text on White background)
//   uint8_t cursor_color = 0x70;
//   VGA_MEMORY[index] = ((uint16_t)cursor_color << 8) | (uint8_t)c;
// }

void display_init(void) {
  // Choose 1 for Block or 0 for Underscore
  set_hardware_cursor_shape(1);
  clear_screen();
}