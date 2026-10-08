#include <stdint.h>

#include "display.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include "memory.h"
#include "rtc.h"
#include "shell.h"
#include "sound.h"
#include "string.h"

#define COLOR_PROMPT 0x0B // Light Cyan
#define COLOR_WHITE 0x0F
#define BUILD_NUM 1

#define MAX_HISTORY 10

static char command_buffer[MAX_BUFFER_SIZE];

// Forward declarations
static void command_beep(const char *args);
static void command_cat(const char *args);
static void command_clear(const char *args);
static void command_date(const char *args);
static void command_echo(const char *args);
static void command_exec(const char *args);
static void command_help(const char *args);
static void command_hexdump(const char *args);
static void command_history(const char *args);
static void command_ls(const char *args);
static void command_restart(const char *args);
static void command_shutdown(const char *args);
static void command_sound(const char *args);
static void command_test(const char *args);
static void command_time(const char *args);
static void command_timezone(const char *args);
static void command_touch(const char *args);
static void command_version(const char *args);

/* Helper forward declarations */
static void print_dec2(uint8_t val);
static void print_uint(uint32_t val);
static void print_tz_suffix(void);
static void get_local_time(rtc_time_t *local);
static void add_to_history(const char *cmd);
static char history[MAX_HISTORY][MAX_BUFFER_SIZE];

struct shell_command {
  const char *name;
  const char *description;
  void (*function)(const char *args);
};

static const struct shell_command commands[] = {
    {"beep", "Play a beep sound", command_beep},
    {"cat", "Display file contents (Usage: cat <file>)", command_cat},
    {"clear", "Clear the screen", command_clear},
    {"date", "Show current date and timestamp", command_date},
    {"hexdump", "Hex dump file contents (Usage: hexdump <file>)",
     command_hexdump},
    {"echo", "Print text to output (Usage: echo <text>)", command_echo},
    {"exec", "Execute commands from script (e.g. system.cfg)", command_exec},
    {"help", "Display this help message", command_help},
    {"history", "Show command history", command_history},
    {"ls", "List files in current directory", command_ls},
    {"restart", "Restart the system", command_restart},
    {"shutdown", "Shut down the system", command_shutdown},
    {"sound", "Toggle the sound on or off", command_sound},
    {"test", "Run diagnostic test", command_test},
    {"timezone", "Set or display time zone (+/- hours)", command_timezone},
    {"time", "Show current system time", command_time},
    {"touch", "Create an empty file (Usage: touch <file>)", command_touch},
    {"version", "Show operating system version", command_version},
};
static int buffer_index = 0;
static int history_count = 0;
static int history_index = -1;

static int tz_offset = 0; // Time zone offset in hours (-12 to +14)

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
  set_text_color(COLOR_PROMPT);
  kprint("mini-os> ");
  set_text_color(COLOR_DEFAULT);
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

static void execute_script(const char *filename, int silent) {
  vfs_node_t *file = vfs_lookup(fs_root, filename);

  if (!file) {
    if (!silent) {
      kprint_color("Script not found: ", COLOR_DEFAULT);
      kprint_color(filename, COLOR_DEFAULT);
      kputchar_color('\n', COLOR_DEFAULT);
    }
    return;
  }

  uint32_t size = file->length;
  if (size == 0) {
    if (!silent) {
      kprint_color("Script is empty.\n", COLOR_DEFAULT);
    }
    return;
  }

  if (silent) {
    display_set_silent(1);
  }

  char line[MAX_BUFFER_SIZE];
  uint32_t line_len = 0;
  uint8_t ch = 0;

  for (uint32_t i = 0; i < size; i++) {
    if (vfs_read(file, i, 1, &ch) == 0) {
      break;
    }
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n') {
      line[line_len] = '\0';
      if (line_len > 0) {
        process_command(line);
      }
      line_len = 0;
    } else {
      if (line_len < MAX_BUFFER_SIZE - 1) {
        line[line_len++] = (char)ch;
      }
    }
  }

  if (line_len > 0) {
    line[line_len] = '\0';
    process_command(line);
  }

  if (silent) {
    display_set_silent(0);
  }
}

static void command_exec(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: exec <filename>\n", COLOR_DEFAULT);
    return;
  }
  execute_script(args, 0);
}

static void command_beep(const char *args) {
  (void)args;
  if (sound_is_enabled()) {
    beep(440, 50);
  }
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

// void command_hexdump(const char *args) {
//     if (!args || *args == '\0') {
//         kprint_color("Usage: hexdump <filename>\n", COLOR_DEFAULT);
//         return;
//     }

//     uint8_t *file_data;
//     uint32_t file_size;
//     if (initrd_get_file(args, &file_data, &file_size) != 0) {
//         kprint_color("File not found: ", COLOR_DEFAULT);
//         kprint_color(args, COLOR_DEFAULT);
//         kputchar_color('\n', COLOR_DEFAULT);
//         return;
//     }

//     for (uint32_t i = 0; i < file_size; i += 16) {
//         print_hex32(i); // Prints offset like 0x00000010
//         kprint_color(": ", COLOR_DEFAULT);

//         // Print Hex values
//         for (uint32_t j = 0; j < 16; j++) {
//             if (i + j < file_size) {
//                 print_hex8(file_data[i + j]);
//                 kputchar_color(' ', COLOR_DEFAULT);
//             } else {
//                 kprint_color("   ", COLOR_DEFAULT);
//             }
//         }

//         kprint_color(" | ", COLOR_DEFAULT);

//         // Print ASCII printable representation
//         for (uint32_t j = 0; j < 16 && (i + j) < file_size; j++) {
//             char c = file_data[i + j];
//             kputchar_color((c >= 32 && c <= 126) ? c : '.', COLOR_WHITE);
//         }
//         kputchar_color('\n', COLOR_DEFAULT);
//     }
// }

static void command_clear(const char *args) {
  (void)args;
  clear_screen();
}

void print_dec(uint32_t val) {
  if (val == 0) {
    kputchar_color('0', COLOR_DEFAULT);
    return;
  }
  char buf[10];
  int i = 0;
  while (val > 0) {
    buf[i++] = '0' + (val % 10);
    val /= 10;
  }
  while (i > 0) {
    kputchar_color(buf[--i], COLOR_DEFAULT);
  }
}

static void command_version(const char *args) {
  (void)args;

  kprint_color("mini-os version 1.0 (build ", COLOR_DEFAULT);
  char buf[12];
  (void)itoa(BUILD_NUM, buf); // Uses existing itoa in display.c
  kprint_color(buf, COLOR_DEFAULT);
  kprint_color(")\n", COLOR_DEFAULT);

#ifdef _NEWLIB_VERSION
  kprint_color("C Library : Newlib v", COLOR_DEFAULT);
  kprint_color(_NEWLIB_VERSION, COLOR_DEFAULT);
  kprint_color("\n", COLOR_DEFAULT);
#else
  kprint_color("Standalone C Library\n", COLOR_DEFAULT);
#endif

  kprint_color("Built with Google Gemini (yeah, but you still need to know "
               "what to build ;-))\n",
               COLOR_DEFAULT);
}

static void command_test(const char *args) {
  (void)args;
  kprint_color("Setting FS I/O indicators...\n", COLOR_DEFAULT);
  display_set_fs_indicator(FS_IND_READ, FS_IND_WRITE);

  kprint_color("Testing malloc()/free()...\n", COLOR_DEFAULT);
  char *buffer = (char *)kmalloc(128);
  if (buffer != NULL) {
    kfree(buffer);
  }

  kprint_color("Testing ANSI escape codes\n", COLOR_DEFAULT);

  kprint("\033[31mRed Text\033[0m ");
  kprint("\033[32mGreen Text\033[0m ");
  kprint("\033[33mYellow Text\033[0m ");
  kprint("\033[34mBlue Text\033[0m\n");
}

static void command_shutdown(const char *args) {
  (void)args;
  kprint_color("Shutting down...\n", COLOR_DEFAULT);

  // QEMU ISA debug exit device (port 0x501)
  __asm__ volatile("outw %0, %1"
                   :
                   : "a"((uint16_t)0x00), "Nd"((uint16_t)0x501));

  // Fallback ACPI shutdown for older QEMU / Bochs
  __asm__ volatile("outw %0, %1"
                   :
                   : "a"((uint16_t)0x2000), "Nd"((uint16_t)0xB004));

  while (1) {
    __asm__ volatile("hlt");
  }
}

static void command_restart(const char *args) {
  (void)args;
  kprint_color("Resetting CPU...\n", COLOR_DEFAULT);
  // Pulse line 2 on the PS/2 keyboard controller
  __asm__ volatile("outb %%al, $0x64" : : "a"((uint8_t)0xFE));

  // Triple fault fallback if controller reset fails
  __asm__ volatile("cli; hlt");
}

static void command_touch(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: touch <filename>\n", COLOR_DEFAULT);
    return;
  }
  // 1. Check if the file already exists
  vfs_node_t *file = vfs_lookup(fs_root, args);
  if (file) {
    // File exists: update timestamp / touch operation
    kprint_color("File already exists.\n", COLOR_DEFAULT);
    return;
  }

  // 2. Create a new empty file entry
  vfs_node_t *new_node = vfs_create(fs_root, args, FS_FILE);
  if (!new_node) {
    kprint_color("Failed to create file.\n", COLOR_DEFAULT);
    return;
  }

  kprint_color("File created successfully.\n", COLOR_DEFAULT);
}

static void execute_command(void) {
  command_buffer[buffer_index] = '\0';

  display_set_fs_indicator(FS_IND_IDLE, FS_IND_IDLE);
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
  execute_script("system.cfg", 1);
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

static void command_hexdump(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: hexdump <filename>\n", COLOR_WHITE);
    return;
  }

  vfs_node_t *file = vfs_lookup(fs_root, args);
  if (!file) {
    kprint_color("File not found: ", COLOR_DEFAULT);
    kprint_color(args, COLOR_WHITE);
    kputchar_color('\n', COLOR_DEFAULT);
    return;
  }

  uint8_t chunk[16];
  for (uint32_t i = 0; i < file->length; i += 16) {
    uint32_t to_read = (file->length - i >= 16) ? 16 : (file->length - i);
    uint32_t bytes_read = vfs_read(file, i, to_read, chunk);
    if (bytes_read == 0) {
      break;
    }
    print_hex32(i); // Prints offset like 0x00000010
    kprint_color(": ", COLOR_DEFAULT);

    // Print Hex values
    for (uint32_t j = 0; j < 16; j++) {
      if (j < bytes_read) {
        print_hex8(chunk[j]);
        kputchar_color(' ', COLOR_DEFAULT);
      } else {
        kprint_color("   ", COLOR_DEFAULT);
      }
    }

    kprint_color(" | ", COLOR_DEFAULT);

    // Print ASCII printable representation
    for (uint32_t j = 0; j < bytes_read; j++) {
      char c = (char)chunk[j];
      kputchar_color((c >= 32 && c <= 126) ? c : '.', COLOR_WHITE);
    }
    kputchar_color('\n', COLOR_DEFAULT);
  }
}

static void command_sound(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: sound on|off\n", COLOR_DEFAULT);
    return;
  }
  if (strcmp(args, "on") == 0) {
    sound_set_enabled(1);
    kprint_color("Sound enabled.\n", COLOR_DEFAULT);
  } else if (strcmp(args, "off") == 0) {
    sound_set_enabled(0);
    kprint_color("Sound disabled.\n", COLOR_DEFAULT);
  } else {
    kprint_color("Invalid argument. Usage: sound on|off\n", COLOR_DEFAULT);
  }
  update_status_bar();
}

static void command_cat(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: cat <filename>\n", COLOR_DEFAULT);
    return;
  }

  vfs_node_t *file = vfs_lookup(fs_root, args);
  if (!file) {
    kprint_color("cat: file not found: ", COLOR_DEFAULT);
    kprint_color(args, COLOR_DEFAULT);
    kputchar_color('\n', COLOR_DEFAULT);
    return;
  }
  if (file->flags & FS_DIRECTORY) {
    kprint_color("cat: ", COLOR_DEFAULT);
    kprint_color(args, COLOR_DEFAULT);
    kprint_color(": Is a directory\n", COLOR_DEFAULT);
    return;
  }

  char buf[64];
  uint32_t offset = 0;
  while (offset < file->length) {
    uint32_t chunk = (file->length - offset > sizeof(buf))
                         ? sizeof(buf)
                         : (file->length - offset);
    uint32_t bytes = vfs_read(file, offset, chunk, (uint8_t *)buf);
    if (bytes == 0)
      break;
    for (uint32_t i = 0; i < bytes; i++) {
      kputchar_color(buf[i], COLOR_DEFAULT);
    }
    offset += bytes;
  }
  const char *data = (const char *)file->device_data;
  uint32_t size = file->length;
  if (data && size > 0 && data[size - 1] != '\n') {
    kputchar_color('\n', COLOR_DEFAULT);
  }
}

static void command_echo(const char *args) {
  if (!args || *args == '\0') {
    kputchar_color('\n', COLOR_DEFAULT);
    return;
  }
  kprint_color(args, COLOR_DEFAULT);
  kputchar_color('\n', COLOR_DEFAULT);
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
    kprint_color(
        "Invalid format. Usage: timezone +/- hours (e.g., timezone +2)\n",
        COLOR_DEFAULT);
    return;
  }

  if (*arg < '0' || *arg > '9') {
    kprint_color("Expected hour digits. Usage: timezone +/- hours\n",
                 COLOR_DEFAULT);
    return;
  }

  int hours = 0;
  while (*arg >= '0' && *arg <= '9') {
    hours = hours * 10 + (*arg - '0');
    arg++;
  }

  int offset = sign * hours;
  if (offset < -12 || offset > 14) {
    kprint_color("Invalid offset. Valid range is -12 to +14 hours.\n",
                 COLOR_DEFAULT);
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
    kprint_color("Usage: timezone +/- hours (e.g. timezone +2, timezone -5)\n",
                 COLOR_DEFAULT);
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