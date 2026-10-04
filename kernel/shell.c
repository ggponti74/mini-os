// kernel/shell.c
#include "shell.h"
#include "display.h"
#include "sound.h"
#include "string.h"
#include "rtc.h"
#include "fs/initrd.h"
#include "fs/vfs.h"

#define COLOR_PROMPT 0x0B // Light Cyan
#define COLOR_WHITE 0x0F

#define MAX_HISTORY 10

static char command_buffer[MAX_BUFFER_SIZE];
static int buffer_index = 0;
static char history[MAX_HISTORY][MAX_BUFFER_SIZE];
static int history_count = 0;
static int history_index = -1;
static int tz_offset = 0; // Time zone offset in hours (-12 to +14)

struct shell_command {
  const char *name;
  const char *description;
  void (*function)(void);
};

static void command_about(void);
static void command_beep(void);
static void command_clear(void);
static void command_date(void);
static void command_help(void);
static void command_history(void);
static void command_ls(void);
static void command_restart(void);
static void command_shutdown(void);
static void command_timezone(void);
static void command_test(void);
static void command_time(void);

static const struct shell_command commands[] = {
    {"about", "Show operating system info", command_about},
    {"beep", "Play a beep sound", command_beep},
    {"clear", "Clear the screen", command_clear},
    {"date", "Show current date and timestamp", command_date},
    {"help", "Display this help message", command_help},
    {"history", "Show command history", command_history},
    {"ls", "List files in current directory", command_ls},
    {"restart", "Restart the system", command_restart},
    {"shutdown", "Shut down the system", command_shutdown},
    {"test", "Run diagnostic test", command_test},
    {"timezone", "Set or display time zone (+/- hours)", command_timezone},
    {"time", "Show current system time", command_time},
};

static void command_ls(void)
{
  initrd_list_files();
}

static void print_dec2(uint8_t val) {
  kputchar_color('0' + (val / 10), COLOR_DEFAULT);
  kputchar_color('0' + (val % 10), COLOR_DEFAULT);
}

static void print_uint(uint32_t val) {
  char buf[12];
  int i = 0;
  if (val == 0) {
    kputchar_color('0', COLOR_DEFAULT);
    return;
  }
  while (val > 0) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  while (i > 0) {
    kputchar_color(buf[--i], COLOR_DEFAULT);
  }
}

static void print_tz_suffix(void) {
  if (tz_offset == 0) {
    kprint(" UTC\n");
  } else {
    kprint(" UTC");
    if (tz_offset > 0) {
      kputchar_color('+', COLOR_DEFAULT);
      print_uint((uint32_t)tz_offset);
    } else {
      kputchar_color('-', COLOR_DEFAULT);
      print_uint((uint32_t)(-tz_offset));
    }
    kprint("\n");
  }
}

static void get_local_time(rtc_time_t *local) {
  rtc_read_datetime(local);
  if (tz_offset != 0) {
    uint32_t epoch = rtc_to_epoch(local);
    int32_t local_epoch = (int32_t)epoch + (tz_offset * 3600);
    if (local_epoch >= 0) {
      epoch_to_rtc((uint32_t)local_epoch, local);
    }
  }
}

static void command_date(void) {
  rtc_time_t rtc;
  get_local_time(&rtc);

  kprint("Date:      ");
  print_uint(rtc.year);
  kputchar_color('-', COLOR_DEFAULT);
  print_dec2(rtc.month);
  kputchar_color('-', COLOR_DEFAULT);
  print_dec2(rtc.day);
  kprint(" ");
  print_dec2(rtc.hour);
  kputchar_color(':', COLOR_DEFAULT);
  print_dec2(rtc.minute);
  kputchar_color(':', COLOR_DEFAULT);
  print_dec2(rtc.second);
  print_tz_suffix();

  kprint("Timestamp: ");
  rtc_time_t utc;
  rtc_read_datetime(&utc);
  print_uint(rtc_to_epoch(&utc));
  kprint(" s (UNIX epoch)\n");
}

static void command_time(void) {
  rtc_time_t rtc;
  get_local_time(&rtc);

  kprint("Time: ");
  print_dec2(rtc.hour);
  kputchar_color(':', COLOR_DEFAULT);
  print_dec2(rtc.minute);
  kputchar_color(':', COLOR_DEFAULT);
  print_dec2(rtc.second);
  print_tz_suffix();
}

static void command_timezone(void) {
  kprint("Current time zone: ");
  print_tz_suffix();
  kprint("Usage: timezone +/- hours (e.g. time zone +2, time zone -5)\n");
}

static int starts_with(const char *str, const char *prefix) {
  while (*prefix) {
    if (*prefix++ != *str++) {
      return 0;
    }
  }
  return 1;
}

static void handle_set_timezone(const char *arg) {
  while (*arg == ' ') {
    arg++;
  }
  if (*arg == '\0') {
    command_timezone();
    return;
  }

  int sign = 1;
  if (*arg == '+') {
    sign = 1;
    arg++;
  } else if (*arg == '-') {
    sign = -1;
    arg++;
  } else if (*arg >= '0' && *arg <= '9') {
    sign = 1;
  } else {
    kprint("Invalid format. Usage: time zone +/- hours (e.g. time zone +2)\n");
    return;
  }

  if (*arg < '0' || *arg > '9') {
    kprint("Expected hour digits. Usage: time zone +/- hours\n");
    return;
  }

  int hours = 0;
  while (*arg >= '0' && *arg <= '9') {
    hours = hours * 10 + (*arg - '0');
    arg++;
  }

  int offset = sign * hours;
  if (offset < -12 || offset > 14) {
    kprint("Invalid offset. Valid range is -12 to +14 hours.\n");
    return;
  }

  tz_offset = offset;
  kprint("Time zone updated to: ");
  print_tz_suffix();
  update_status_bar();
}

int shell_get_timezone(void) {
  return tz_offset;
}

static void add_to_history(const char *cmd) {
  if (cmd[0] == '\0') return;

  if (history_count > 0) {
    int last = (history_count - 1) % MAX_HISTORY;
    if (strcmp(history[last], cmd) == 0) {
      return;
    }
  }

  int slot = history_count % MAX_HISTORY;
  int i = 0;
  while (cmd[i] != '\0' && i < MAX_BUFFER_SIZE - 1) {
    history[slot][i] = cmd[i];
    i++;
  }
  history[slot][i] = '\0';
  history_count++;
}

static void clear_input_line(void) {
  while (buffer_index > 0) {
    buffer_index--;
    kputchar_color('\b', COLOR_DEFAULT);
  }
}

static void set_input_buffer(const char *str) {
  clear_input_line();
  while (*str && buffer_index < MAX_BUFFER_SIZE - 1) {
    command_buffer[buffer_index++] = *str;
    kputchar_color(*str, COLOR_DEFAULT);
    str++;
  }
  command_buffer[buffer_index] = '\0';
}

void shell_handle_key_up(void) {
  if (history_count == 0) return;
  int total = history_count < MAX_HISTORY ? history_count : MAX_HISTORY;
  int oldest = history_count - total;

  if (history_index == -1) {
    history_index = history_count - 1;
  } else if (history_index > oldest) {
    history_index--;
  }

  set_input_buffer(history[history_index % MAX_HISTORY]);
}

void shell_handle_key_down(void) {
  if (history_index == -1) return;

  if (history_index < history_count - 1) {
    history_index++;
    set_input_buffer(history[history_index % MAX_HISTORY]);
  } else {
    history_index = -1;
    clear_input_line();
  }
}

static void command_history(void) {
  int total = history_count < MAX_HISTORY ? history_count : MAX_HISTORY;
  if (total == 0) {
    kprint("No commands in history.\n");
    return;
  }
  int start = history_count - total;
  for (int i = 0; i < total; i++) {
    int idx = start + i;
    kprint("  ");
    print_uint((uint32_t)(idx + 1));
    kprint(": ");
    kprint(history[idx % MAX_HISTORY]);
    kprint("\n");
  }
}

static void print_prompt(void) {
  // CORRECT:
  kprint("mini-os> "); // Uses default COLOR_DEFAULT
}

void process_command(const char *cmd) {
  if (cmd[0] == '\0') {
    return; // Empty command
  }

  // Check for "time zone" prefix with arguments
  if (starts_with(cmd, "time zone")) {
    const char *arg = cmd + 9;
    if (*arg == ' ' || *arg == '\0') {
      handle_set_timezone(arg);
      return;
    }
  } else if (starts_with(cmd, "timezone")) {
    // Allow "timezone" as an alias
    const char *arg = cmd + 8;
    if (*arg == ' ' || *arg == '\0') {
      handle_set_timezone(arg);
      return;
    }
  }

  for (unsigned int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (strcmp(cmd, commands[i].name) == 0) {
      commands[i].function();
      return;
    }
  }

  kprint("Unknown command: ");
  kprint(cmd);
  kprint("\nType 'help' for available commands.\n");
}

static void command_beep(void) {
  beep(440, 50);
  kprint("Beeped!\n");
}

static void command_help(void) {
  kprint("Available commands:\n");

  for (unsigned int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    unsigned int name_length = 0;
    kprint("  ");
    kprint(commands[i].name);
    while (commands[i].name[name_length] != '\0')
      name_length++;
    while (name_length++ < 12)
      kprint(" ");
    kprint(" - ");
    kprint(commands[i].description);
    kprint("\n");
  }
}

static void command_clear(void) { clear_screen(); }

static void command_about(void) {
  kprint(
      "mini-os v1.0 - A lightweight 32-bit x86 kernel built from scratch.\n");
}

static void command_test(void) { kprint("System diagnostic OK.\n"); }

static void command_shutdown(void) {
    // ISA debug-exit device
    // Writing 0x00 exits QEMU with exit code (0x00 << 1) | 1 = 1
    __asm__ volatile("outw %0, %1" : : "a"((uint16_t)0x00), "Nd"((uint16_t)0x501));
}

static void command_restart(void) {
  __asm__ volatile("movb $0xFE, %%al\n\t"
                   "outb %%al, $0x64\n\t" // Write 0xFE to the command port 0x64
                   :
                   :
                   : "al");

  // If that fails, halt the CPU
  while (1) {
    __asm__ volatile("hlt");
  }
}

// Simple command processor
static void execute_command(void) {
  command_buffer[buffer_index] = '\0'; // Null-terminate string

  kputchar_color('\n', COLOR_DEFAULT);
  if (buffer_index > 0) {
    add_to_history(command_buffer);
  }
  process_command(command_buffer);

  // Reset buffer for next command
  buffer_index = 0;
  history_index = -1;
  update_status_bar();
  print_prompt();
}

void shell_init(void) {
  buffer_index = 0;
  history_index = -1;
  update_status_bar();
  print_prompt();
}

void shell_input_char(char c) {
  if (c == '\n') {
    execute_command();
  } else if (c == '\b') {
    if (buffer_index > 0) {
      buffer_index--;
      kputchar_color('\b', COLOR_DEFAULT); // Erase character on screen
    }
  } else if (c >= ' ' && c <= '~') { // Printable ASCII characters
    if (buffer_index < MAX_BUFFER_SIZE - 1) {
      command_buffer[buffer_index++] = c;
      kputchar_color(c, COLOR_DEFAULT);
    }
  }
}