// kernel/shell.c
#include "shell.h"
#include "display.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include "rtc.h"
#include "sound.h"
#include "string.h"

#define COLOR_PROMPT 0x0B // Light Cyan
#define COLOR_WHITE  0x0F

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
  void (*function)(const char *args);
};

// Forward declarations
static void command_about(const char *args);
static void command_beep(const char *args);
static void command_cat(const char *args);
static void command_clear(const char *args);
static void command_date(const char *args);
static void command_help(const char *args);
static void command_history(const char *args);
static void command_ls(const char *args);
static void command_restart(const char *args);
static void command_shutdown(const char *args);
static void command_timezone(const char *args);
static void command_test(const char *args);
static void command_time(const char *args);

static const struct shell_command commands[] = {
    {"about",    "Show operating system info",                 command_about},
    {"beep",     "Play a beep sound",                         command_beep},
    {"cat",      "Display file contents (Usage: cat <file>)", command_cat},
    {"clear",    "Clear the screen",                          command_clear},
    {"date",     "Show current date and timestamp",           command_date},
    {"help",     "Display this help message",                 command_help},
    {"history",  "Show command history",                      command_history},
    {"ls",       "List files in current directory",           command_ls},
    {"restart",  "Restart the system",                        command_restart},
    {"shutdown", "Shut down the system",                      command_shutdown},
    {"test",     "Run diagnostic test",                       command_test},
    {"timezone", "Set or display time zone (+/- hours)",      command_timezone},
    {"time",     "Show current system time",                  command_time},
};

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
    kprint_color(" UTC\n", COLOR_DEFAULT);
  } else {
    kprint_color(" UTC", COLOR_DEFAULT);
    if (tz_offset > 0) {
      kputchar_color('+', COLOR_DEFAULT);
      print_uint((uint32_t)tz_offset);
    } else {
      kputchar_color('-', COLOR_DEFAULT);
      print_uint((uint32_t)(-tz_offset));
    }
    kprint_color("\n", COLOR_DEFAULT);
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

static void command_cat(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: cat <filename>\n", COLOR_DEFAULT);
    return;
  }
  initrd_cat_file(args);
}

static void command_ls(const char *args) {
  (void)args;
  initrd_list_files();
}

static void command_date(const char *args) {
  (void)args;
  rtc_time_t rtc;
  get_local_time(&rtc);

  kprint_color("Date:      ", COLOR_DEFAULT);
  print_uint(rtc.year);
  kputchar_color('-', COLOR_DEFAULT);
  print_dec2(rtc.month);
  kputchar_color('-', COLOR_DEFAULT);
  print_dec2(rtc.day);
  kprint_color(" ", COLOR_DEFAULT);

  kprint_color("Time:      ", COLOR_DEFAULT);
  print_dec2(rtc.hour);
  kputchar_color(':', COLOR_DEFAULT);
  print_dec2(rtc.minute);
  kputchar_color(':', COLOR_DEFAULT);
  print_dec2(rtc.second);
  print_tz_suffix();

  kprint_color("Timestamp: ", COLOR_DEFAULT);
  rtc_time_t utc;
  rtc_read_datetime(&utc);
  print_uint(rtc_to_epoch(&utc));
  kprint_color(" s (UNIX epoch)\n", COLOR_DEFAULT);
}

static void command_time(const char *args) {
  (void)args;
  rtc_time_t rtc;
  get_local_time(&rtc);

  kprint_color("Time: ", COLOR_DEFAULT);
  print_dec2(rtc.hour);
  kputchar_color(':', COLOR_DEFAULT);
  print_dec2(rtc.minute);
  kputchar_color(':', COLOR_DEFAULT);
  print_dec2(rtc.second);
  print_tz_suffix();
}

static void handle_set_timezone(const char *arg) {
  while (*arg == ' ') {
    arg++;
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
    kprint_color("Invalid format. Usage: timezone +/- hours (e.g., timezone +2)\n", COLOR_DEFAULT);
    return;
  }

  if (*arg < '0' || *arg > '9') {
    kprint_color("Expected hour digits. Usage: timezone +/- hours\n", COLOR_DEFAULT);
    return;
  }

  int hours = 0;
  while (*arg >= '0' && *arg <= '9') {
    hours = hours * 10 + (*arg - '0');
    arg++;
  }

  int offset = sign * hours;
  if (offset < -12 || offset > 14) {
    kprint_color("Invalid offset. Valid range is -12 to +14 hours.\n", COLOR_DEFAULT);
    return;
  }

  tz_offset = offset;
  kprint_color("Time zone updated to: ", COLOR_DEFAULT);
  print_tz_suffix();
  update_status_bar();
}

static void command_timezone(const char *args) {
  if (args && *args != '\0') {
    handle_set_timezone(args);
  } else {
    kprint_color("Current time zone: ", COLOR_DEFAULT);
    print_tz_suffix();
    kprint_color("Usage: timezone +/- hours (e.g. timezone +2, timezone -5)\n", COLOR_DEFAULT);
  }
}

int shell_get_timezone(void) { return tz_offset; }

static void add_to_history(const char *cmd) {
  if (cmd[0] == '\0')
    return;

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
  if (history_count == 0)
    return;
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
  if (history_index == -1)
    return;

  if (history_index < history_count - 1) {
    history_index++;
    set_input_buffer(history[history_index % MAX_HISTORY]);
  } else {
    history_index = -1;
    clear_input_line();
  }
}

static void command_history(const char *args) {
  (void)args;
  int total = history_count < MAX_HISTORY ? history_count : MAX_HISTORY;
  if (total == 0) {
    kprint_color("No commands in history.\n", COLOR_DEFAULT);
    return;
  }
  int start = history_count - total;
  for (int i = 0; i < total; i++) {
    int idx = start + i;
    kprint_color("  ", COLOR_DEFAULT);
    print_uint((uint32_t)(idx + 1));
    kprint_color(": ", COLOR_DEFAULT);
    kprint_color(history[idx % MAX_HISTORY], COLOR_DEFAULT);
    kprint_color("\n", COLOR_DEFAULT);
  }
}

static void print_prompt(void) {
  kprint_color("mini-os> ", COLOR_PROMPT);
}

void process_command(const char *cmd) {
  if (cmd[0] == '\0') {
    return;
  }

  // Parse command name vs arguments
  char cmd_name[64];
  int i = 0;
  while (cmd[i] != '\0' && cmd[i] != ' ' && i < 63) {
    cmd_name[i] = cmd[i];
    i++;
  }
  cmd_name[i] = '\0';

  const char *args = cmd + i;
  while (*args == ' ') {
    args++;
  }

  for (unsigned int j = 0; j < sizeof(commands) / sizeof(commands[0]); j++) {
    if (strcmp(cmd_name, commands[j].name) == 0) {
      commands[j].function(args);
      return;
    }
  }

  kprint_color("Unknown command: ", COLOR_DEFAULT);
  kprint_color(cmd_name, COLOR_DEFAULT);
  kprint_color("\nType 'help' for available commands.\n", COLOR_DEFAULT);
}

static void command_beep(const char *args) {
  (void)args;
  beep(440, 50);
}

static void command_help(const char *args) {
  (void)args;
  kprint_color("Available commands:\n", COLOR_DEFAULT);

  for (unsigned int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    unsigned int name_length = 0;
    kprint_color("  ", COLOR_DEFAULT);
    kprint_color(commands[i].name, COLOR_DEFAULT);
    while (commands[i].name[name_length] != '\0')
      name_length++;
    while (name_length++ < 12)
      kprint_color(" ", COLOR_DEFAULT);
    kprint_color(" - ", COLOR_DEFAULT);
    kprint_color(commands[i].description, COLOR_DEFAULT);
    kprint_color("\n", COLOR_DEFAULT);
  }
}

static void command_clear(const char *args) {
  (void)args;
  clear_screen();
}

static void command_about(const char *args) {
  (void)args;
  kprint_color("mini-os v1.0 - A lightweight 32-bit x86 kernel built from scratch.\n", COLOR_DEFAULT);

}

static void command_test(const char *args) {
  (void)args;
  kprint_color("System diagnostic OK.\n", COLOR_DEFAULT);
}

static void command_shutdown(const char *args) {
  (void)args;
  __asm__ volatile("outw %0, %1"
                   :
                   : "a"((uint16_t)0x00), "Nd"((uint16_t)0x501));
}

static void command_restart(const char *args) {
  (void)args;
  __asm__ volatile("movb $0xFE, %%al\n\t"
                   "outb %%al, $0x64\n\t"
                   :
                   :
                   : "al");

  while (1) {
    __asm__ volatile("hlt");
  }
}

static void execute_command(void) {
  command_buffer[buffer_index] = '\0';

  kputchar_color('\n', COLOR_DEFAULT);
  if (buffer_index > 0) {
    add_to_history(command_buffer);
  }
  process_command(command_buffer);

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
      kputchar_color('\b', COLOR_DEFAULT);
    }
  } else if (c >= ' ' && c <= '~') {
    if (buffer_index < MAX_BUFFER_SIZE - 1) {
      command_buffer[buffer_index++] = c;
      kputchar_color(c, COLOR_DEFAULT);
    }
  }
}