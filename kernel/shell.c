#include <stdint.h>

#include "display.h"
#include "drivers/e1000.h"
#include "exec.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include "keyboard.h"
#include "memory.h"
#include "rtc.h"
#include "shell.h"
#include "sound.h"
#include "string.h"

#define COLOR_DEFAULT 0x0F
#define BUILD_NUM 1

#define MAX_HISTORY 10

static char command_buffer[MAX_BUFFER_SIZE];

// Forward declarations
static void command_alias(const char *args);
static void command_beep(const char *args);
static void command_cat(const char *args);
static void command_clear(const char *args);
static void command_cp(const char *args);
static void command_date(const char *args);
static void command_echo(const char *args);
static void command_exec(const char *args);
static void command_help(const char *args);
static void command_hexdump(const char *args);
static void command_history(const char *args);
static void command_ls(const char *args);
static void command_mv(const char *args);
static void command_net(const char *args);
static void command_pause(const char *args);
static void command_restart(const char *args);
static void command_rm(const char *args);
static void command_run(const char *args);
static void command_screenshot(const char *args);
static void command_set(const char *args);
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

static const char *command_alias_get(const char *name);
static void command_alias_list(void);
static void command_alias_set(const char *, const char *);

static const char *command_set_get(const char *name);
static void command_set_list(void);
static void command_set_set(const char *, const char *);

struct shell_command {
  const char *name;
  const char *description;
  void (*function)(const char *args);
};

typedef struct set_node {
  char *name;
  char *value;
  struct set_node *next;
} set_node_t;

static const struct shell_command commands[] = {
    {"alias", "Define or display command aliases (Usage: alias name=command)",
     command_alias},
    {"beep", "Play a beep sound", command_beep},
    {"cat", "Display file contents (Usage: cat <file>)", command_cat},
    {"clear", "Clear the screen", command_clear},
    {"cp", "Copy a file", command_cp},
    {"date", "Show current date and timestamp", command_date},
    {"echo", "Print text to output (Usage: echo <text>)", command_echo},
    {"exec", "Execute commands from script (e.g. system.cfg)", command_exec},
    {"help", "Display this help message", command_help},
    {"hexdump", "Hex dump file contents (Usage: hexdump <file>)",
     command_hexdump},
    {"history", "Show command history", command_history},
    {"ls", "List files in current directory", command_ls},
    {"mv", "Move a file", command_mv},
    {"net", "Network utilities", command_net},
    {"pause", "Wait for a key to be pressed", command_pause},
    {"restart", "Restart the system", command_restart},
    {"rm", "Remove a file", command_rm},
    {"run", "Run a program", command_run},
    {"screenshot", "Take and restore a screenshot", command_screenshot},
    {"set", "Set enviroment variables", command_set},
    {"shutdown", "Shut down the system", command_shutdown},
    {"sound", "Toggle the sound on or off", command_sound},
    {"test", "Run diagnostic test", command_test},
    {"time", "Show current system time", command_time},
    {"timezone", "Set or display time zone (+/- hours)", command_timezone},
    {"touch", "Create an empty file (Usage: touch <file>)", command_touch},
    {"version", "Show operating system version", command_version},
};

static int buffer_index = 0;
static int cursor_index = 0;
static int history_count = 0;
static int history_index = -1;
static int insert_mode = 1;

static int tz_offset = 0; // Time zone offset in hours (-12 to +14)

static alias_node_t *alias_head = NULL;
static set_node_t *set_head = NULL;

screen_snapshot_t saved_screen;

void command_screenshot(const char *args) {
  (void)args;

  if (args && *args != '\0') {
    if (strcmp(args, "take") == 0) {
      display_save_screenshot(&saved_screen);
    } else {
      display_restore_screenshot(&saved_screen);
    }
  } else {
    kprint_color("Usage: screenshot take|restore\n", COLOR_DEFAULT);
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
  set_text_color(COLOR_PROMPT);
  kprint_color("mini-os> ", COLOR_PROMPT);
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

  // 1. Check built-in command table first
  for (unsigned int j = 0; j < sizeof(commands) / sizeof(commands[0]); j++) {
    if (strcmp(cmd_name, commands[j].name) == 0) {
      commands[j].function(args);
      return;
    }
  }

  // 2. Safe Fallback: Try looking for a flat binary on the RAMDisk via VFS
  // We wrap this with a check so it doesn't jump blindly into garbage memory
  if (exec_flat_binary(cmd_name) == 0) {
    return;
  }

  // 3. Graceful error message if neither built-in nor valid binary exists
  kprint_color("Unknown command or file: ", COLOR_DEFAULT);
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

static void command_set(const char *args) {

  char name[32];
  int i = 0;
  const char *value;

  if (!args || *args == '\0') {
    command_set_list();
    return;
  }

  while (*args == ' ')
    args++;
  if (*args == '\0') {
    command_set_list();
    return;
  }

  // Parse variable name up to '=' or space
  while (args[i] != '=' && args[i] != ' ' && args[i] != '\0' && i < 31) {
    name[i] = args[i];
    i++;
  }
  name[i] = '\0';

  while (args[i] == ' ' || args[i] == '=')
    i++;

  value = &args[i];
  if (name[0] != '\0' && value[0] != '\0') {
    command_set_set(name, value);
    kprint_color("Environment variable set.\n", COLOR_DEFAULT);
  } else {
    kprint_color("Usage: set <name>=<value>\n", COLOR_DEFAULT);
  }
}

static const char *command_set_get_by(const char *name) {
  set_node_t *curr = set_head;

  while (curr != NULL) {
    if (strcmp(curr->name, name) == 0) {
      return curr->value;
    }
    curr = curr->next;
  }

  return NULL;
}

static const char *command_set_get(const char *name) {
  set_node_t *curr = set_head;

  while (curr != NULL) {
    if (strcmp(curr->name, name) == 0) {
      return curr->value;
    }
    curr = curr->next;
  }

  return NULL;
}

static void command_set_list(void) {
  set_node_t *curr = set_head; // Declared at the top
  if (!curr) {
    kprint_color("No variables defined.\n", COLOR_DEFAULT);
    return;
  }
  while (curr != NULL) {
    kprint_color(curr->name, COLOR_DEFAULT);
    kprint_color("=", COLOR_DEFAULT);
    kprint_color(curr->value, COLOR_DEFAULT);
    kputchar_color('\n', COLOR_DEFAULT);
    curr = curr->next;
  }
}

static void command_set_set(const char *name, const char *value_str) {
  set_node_t *curr;
  set_node_t *new_node;

  if (!name || !value_str || name[0] == '\0') {
    kprint_color("Set: invalid name\n", COLOR_DEFAULT);
    return;
  }

  // Clean any trailing whitespace or carriage returns from value
  char clean_value[64];
  int v = 0;
  while (value_str[v] != '\0' && value_str[v] != '\r' && value_str[v] != '\n' &&
         v < 63) {
    clean_value[v] = value_str[v];
    v++;
  }
  // Strip trailing spaces from value
  while (v > 0 && clean_value[v - 1] == ' ') {
    v--;
  }
  clean_value[v] = '\0';

  if (clean_value[0] == '\0') {
    kprint_color("Set: invalid value\n", COLOR_DEFAULT);
    return;
  }

  // Check if alias already exists and update
  curr = set_head;
  while (curr != NULL) {
    if (strcmp(curr->name, name) == 0) {
      kfree(curr->value);
      curr->value = (char *)kmalloc(strlen(clean_value) + 1);
      if (!curr->value) {
        kprint_color("alias: memory allocation failed\n", COLOR_DEFAULT);
        return;
      }
      strcpy(curr->value, clean_value);
      return;
    }
    curr = curr->next;
  }

  // Allocate new node
  new_node = (set_node_t *)kmalloc(sizeof(set_node_t));
  if (!new_node) {
    return;
  }

  new_node->name = (char *)kmalloc(strlen(name) + 1);
  if (!new_node->name) {
    kprint_color("set: kmalloc failed for name\n", COLOR_DEFAULT);
    kfree(new_node);
    return;
  }
  strcpy(new_node->name, name);

  new_node->value = (char *)kmalloc(strlen(clean_value) + 1);
  if (!new_node->value) {
    kprint_color("set: kmalloc failed for value\n", COLOR_DEFAULT);
    kfree(new_node->name);
    kfree(new_node);
    return;
  }
  strcpy(new_node->value, clean_value);

  new_node->next = set_head;
  set_head = new_node;
}

static void command_alias(const char *args) {
  if (!args || *args == '\0') {
    command_alias_list();
    return;
  }

  // Skip leading spaces
  while (*args == ' ')
    args++;
  if (*args == '\0') {
    command_alias_list();
    return;
  }

  // Parse name=value format
  char name[32];
  int i = 0;
  while (args[i] != '=' && args[i] != ' ' && args[i] != '\0' && i < 31) {
    name[i] = args[i];
    i++;
  }
  name[i] = '\0';

  while (args[i] == ' ' || args[i] == '=')
    i++; // Skip spaces and '='

  const char *value = &args[i];
  if (name[0] != '\0' && value[0] != '\0') {
    command_alias_set(name, value);
  } else {
    kprint_color("Usage: alias <name>=<command>|-list", COLOR_DEFAULT);
  }
}

static const char *command_alias_get(const char *name) {
  alias_node_t *curr = alias_head;

  while (curr != NULL) {
    if (strcmp(curr->name, name) == 0) {
      return curr->value;
    }
    curr = curr->next;
  }

  return NULL;
}

static void command_alias_set(const char *name, const char *value_str) {
  alias_node_t *curr;
  alias_node_t *new_node;

  if (!name || !value_str || name[0] == '\0') {
    kprint_color("alias: invalid name\n", COLOR_DEFAULT);
    return;
  }

  // Clean any trailing whitespace or carriage returns from value
  char clean_value[64];
  int v = 0;
  while (value_str[v] != '\0' && value_str[v] != '\r' && value_str[v] != '\n' &&
         v < 63) {
    clean_value[v] = value_str[v];
    v++;
  }
  // Strip trailing spaces from value
  while (v > 0 && clean_value[v - 1] == ' ') {
    v--;
  }
  clean_value[v] = '\0';

  if (clean_value[0] == '\0') {
    kprint_color("alias: invalid value\n", COLOR_DEFAULT);
    return;
  }

  // Check if alias already exists and update
  curr = alias_head;
  while (curr != NULL) {
    if (strcmp(curr->name, name) == 0) {
      kfree(curr->value);
      curr->value = (char *)kmalloc(strlen(clean_value) + 1);
      if (!curr->value) {
        kprint_color("alias: memory allocation failed\n", COLOR_DEFAULT);
        return;
      }
      strcpy(curr->value, clean_value);
      kprint_color("Alias updated.\n", COLOR_DEFAULT);
      return;
    }
    curr = curr->next;
  }

  // Allocate new node
  new_node = (alias_node_t *)kmalloc(sizeof(alias_node_t));
  if (!new_node) {
    kprint_color("alias: kmalloc failed for node\n", COLOR_DEFAULT);
    return;
  }

  new_node->name = (char *)kmalloc(strlen(name) + 1);
  if (!new_node->name) {
    kprint_color("alias: kmalloc failed for name\n", COLOR_DEFAULT);
    kfree(new_node);
    return;
  }
  strcpy(new_node->name, name);

  new_node->value = (char *)kmalloc(strlen(clean_value) + 1);
  if (!new_node->value) {
    kprint_color("alias: kmalloc failed for value\n", COLOR_DEFAULT);
    kfree(new_node->name);
    kfree(new_node);
    return;
  }
  strcpy(new_node->value, clean_value);

  new_node->next = alias_head;
  alias_head = new_node;
  kprint_color("Alias set successfully.\n", COLOR_DEFAULT);
}

static void command_alias_list(void) {
  alias_node_t *curr = alias_head; // Declared at the top
  if (!curr) {
    kprint_color("No aliases defined.\n", COLOR_DEFAULT);
    return;
  }
  while (curr != NULL) {
    kprint_color(curr->name, COLOR_DEFAULT);
    kprint_color("=", COLOR_DEFAULT);
    kprint_color(curr->value, COLOR_DEFAULT);
    kputchar_color('\n', COLOR_DEFAULT);
    curr = curr->next;
  }
}

static void command_beep(const char *args) {
  (void)args;
  if (sound_is_enabled()) {
    beep(440, 50);
  }
}

static void command_help(const char *args) {
  // Check if a specific command argument was provided
  if (args && *args != '\0') {
    // Trim leading spaces
    while (*args == ' ') {
      args++;
    }

    // Search for the matching command in the table
    for (unsigned int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
      if (strcmp(args, commands[i].name) == 0) {
        kprint_color("Help for ", COLOR_DEFAULT);
        kprint_color(commands[i].name, COLOR_DEFAULT);
        kprint_color(":\n  ", COLOR_DEFAULT);

        unsigned int name_length = 0;
        kprint_color(commands[i].name, COLOR_DEFAULT);
        while (commands[i].name[name_length] != '\0') {
          name_length++;
        }
        while (name_length++ < 12) {
          kprint_color(" ", COLOR_DEFAULT);
        }

        kprint_color(" - ", COLOR_DEFAULT);
        kprint_color(commands[i].description, COLOR_DEFAULT);
        kprint_color("\n", COLOR_DEFAULT);
        return;
      }
    }

    // If command wasn't found
    kprint_color("Unknown help topic: ", COLOR_DEFAULT);
    kprint_color(args, COLOR_DEFAULT);
    kprint_color("\nType 'help' for a list of available commands.\n",
                 COLOR_DEFAULT);
    return;
  }

  // Default behavior: print the full list of commands
  kprint_color("Available commands:\n", COLOR_DEFAULT);

  for (unsigned int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    unsigned int name_length = 0;
    kprint_color("  ", COLOR_DEFAULT);
    kprint_color(commands[i].name, COLOR_DEFAULT);
    while (commands[i].name[name_length] != '\0') {
      name_length++;
    }
    while (name_length++ < 12) {
      kprint_color(" ", COLOR_DEFAULT);
    }
    kprint_color(" - ", COLOR_DEFAULT);
    kprint_color(commands[i].description, COLOR_DEFAULT);
    kprint_color("\n", COLOR_DEFAULT);
  }
}

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
  itoa(BUILD_NUM, buf); // Uses existing itoa in display.c
  kprint_color(buf, COLOR_DEFAULT);
  kprint_color(")\n", COLOR_DEFAULT);

#ifdef _NEWLIB_VERSION
  kprint_color("C Library : Newlib v", COLOR_DEFAULT);
  kprint_color(_NEWLIB_VERSION, COLOR_DEFAULT);
  kprint_color("\n", COLOR_DEFAULT);
#else
  kprint_color("Standalone C Library\n", COLOR_DEFAULT);
#endif

  kprint_color("Built with Google Gemini (yeah, you still need to know "
               "what to build ;-))\n",
               COLOR_DEFAULT);
}

static void command_test(const char *args) {
  (void)args;
  kprint_color("Setting FS I/O indicators...\n", COLOR_DEFAULT);
  display_set_fs_indicator(FS_IND_READ, FS_IND_WRITE);

  kprint_color("Testing malloc()/free())...\n", COLOR_DEFAULT);
  char *buffer = (char *)kmalloc(12);
  if (buffer != NULL) {
    kfree(buffer);
  }

  kprint_color("Testing ANSI escape sequences...\n", COLOR_DEFAULT);

  kprint_color("\033[31mRed Text\033[0m ", COLOR_DEFAULT);
  kprint_color("\033[32mGreen Text\033[0m ", COLOR_DEFAULT);
  kprint_color("\033[33mYellow Text\033[0m ", COLOR_DEFAULT);
  kprint_color("\033[34mBlue Text\033[0m\n", COLOR_DEFAULT);
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

static void command_mv(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: mv <old> <new>\n", COLOR_DEFAULT);
    return;
  }

  char old_name[64], new_name[64];
  int i = 0, j = 0;
  while (args[i] != ' ' && args[i] != '\0' && i < 63) {
    old_name[i] = args[i];
    i++;
  }
  old_name[i] = '\0';

  while (args[i] == ' ')
    i++;

  while (args[i] != '\0' && j < 63) {
    new_name[j++] = args[i++];
  }
  new_name[j] = '\0';

  if (old_name[0] == '\0' || new_name[0] == '\0') {
    kprint_color("Usage: mv <old> <new>\n", COLOR_DEFAULT);
    return;
  }

  if (initrd_move_file(old_name, new_name) != 0) {
    kprint_color("mv: file not found\n", COLOR_DEFAULT);
  } else {
    kprint_color("File moved/renamed successfully.\n", COLOR_DEFAULT);
  }
}

static void command_net(const char *args) {

  if (!args || *args == '\0') {
    kprint_color("Usage: net <command>\n", COLOR_DEFAULT);
    return;
  }
  if (strcmp(args, "mac") == 0) {
    uint8_t mac[6];
    e1000_get_mac_address(e1000_get_mmio_base(), mac);
    int i = 0;
    print_hex8(mac[i++]);
    kputchar(':');
    print_hex8(mac[i++]);
    kputchar(':');
    print_hex8(mac[i++]);
    kputchar(':');
    print_hex8(mac[i++]);
    kputchar(':');
    print_hex8(mac[i++]);
    kputchar(':');
    print_hex8(mac[i++]);
    kputchar('\n');
  } else {
    kprint_color("Invalid command.\n", COLOR_DEFAULT);
  }
}

static void command_pause(const char *args) {
  (void)args;

  kprint_color("Press any key to continue...", COLOR_DEFAULT);

  set_keyboard_pause_state(1);
}

static void command_rm(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: rm <filename>\n", COLOR_DEFAULT);
    return;
  }

  if (initrd_remove_file(args) != 0) {
    kprint_color("rm: file not found: ", COLOR_DEFAULT);
    kprint_color(args, COLOR_DEFAULT);
    kputchar_color('\n', COLOR_DEFAULT);
  } else {
    kprint_color("File removed.\n", COLOR_DEFAULT);
  }
}

static void command_run(const char *args) {

  // Skip any leading spaces
  while (*args == ' ') {
    args++;
  }

  if (*args == '\0') {
    kprint_color("Usage: run <binary>\n", COLOR_DEFAULT);
    return;
  }

  int result = exec_flat_binary(args);
  if (result != 0) {
    kprint_color("Failed to execute binary.\n", COLOR_DEFAULT);
  }
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
}

static void execute_command(void) {
  command_buffer[buffer_index] = '\0';
  kputchar_color('\n', COLOR_DEFAULT);

  if (buffer_index == 0) {
    buffer_index = 0;
    history_index = -1;
    update_status_bar();
    print_prompt();
    return;
  }

  add_to_history(command_buffer);

  // 1. Parse command name vs arguments (input_args)
  char cmd_name[64];
  int i = 0;
  while (command_buffer[i] != '\0' && command_buffer[i] != ' ' && i < 63) {
    cmd_name[i] = command_buffer[i];
    i++;
  }
  cmd_name[i] = '\0';

  const char *input_args = command_buffer + i;
  while (*input_args == ' ') {
    input_args++;
  }

  // 2. Check for Alias Expansion
  const char *aliased = command_alias_get(cmd_name);
  if (aliased) {
    int j = 0;
    while (aliased[j] != '\0' && aliased[j] != ' ' && j < 63) {
      cmd_name[j] = aliased[j];
      j++;
    }
    cmd_name[j] = '\0';
  }

  // 3. Dispatch to command table
  int found = 0;
  for (unsigned int j = 0; j < sizeof(commands) / sizeof(commands[0]); j++) {
    if (strcmp(cmd_name, commands[j].name) == 0) {
      commands[j].function(input_args);
      found = 1;
      break;
    }
  }

  if (!found) {
    kprint_color("Unknown command: ", COLOR_DEFAULT);
    kprint_color(cmd_name, COLOR_DEFAULT);
    kprint_color("\nType 'help' for available commands.\n", COLOR_DEFAULT);
  }

  buffer_index = 0;
  history_index = -1;
  update_status_bar();
  if (!get_keyboard_pause_state()) {
    print_prompt();
  }
}

void shell_init(void) {
  buffer_index = 0;
  history_index = -1;
  execute_script("system.cfg", 1);
  update_status_bar();
  if (!get_keyboard_pause_state()) {
    print_prompt();
  }
}

void shell_pause_complete(void) {
  kputchar('\n');
  update_status_bar();
  print_prompt();
}

void shell_input_char(char c) {
  if (c == '\n') {
    execute_command();
  } else if (c == '\b') {
    if (buffer_index > 0 && cursor_index > 0) {
      // delete at cursor position, not just at end
      // for now, keep simple: delete last char
      buffer_index--;
      cursor_index = buffer_index;
      kputchar_color('\b', COLOR_DEFAULT);
    }
  } else if (c >= ' ' && c <= '~') {
    if (buffer_index < MAX_BUFFER_SIZE - 1) {
      command_buffer[buffer_index++] = c;
      cursor_index = buffer_index;
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
    kprint_color("Usage: hexdump <filename>\n", COLOR_DEFAULT);
    return;
  }

  vfs_node_t *file = vfs_lookup(fs_root, args);
  if (!file) {
    kprint_color("File not found: ", COLOR_DEFAULT);
    kprint_color(args, COLOR_DEFAULT);
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
      kputchar_color((c >= 32 && c <= 126) ? c : '.', COLOR_DEFAULT);
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

static void command_cp(const char *args) {
  if (!args || *args == '\0') {
    kprint_color("Usage: cp <source> <dest>\n", COLOR_DEFAULT);
    return;
  }

  // Split args into source and destination
  char src[64], dest[64];
  int i = 0, j = 0;
  while (args[i] != ' ' && args[i] != '\0' && i < 63) {
    src[i] = args[i];
    i++;
  }
  src[i] = '\0';

  while (args[i] == ' ')
    i++; // Skip whitespace

  while (args[i] != '\0' && j < 63) {
    dest[j++] = args[i++];
  }
  dest[j] = '\0';

  if (src[0] == '\0' || dest[0] == '\0') {
    kprint_color("Usage: cp <source> <dest>\n", COLOR_DEFAULT);
    return;
  }

  if (initrd_copy_file(src, dest) != 0) {
    kprint_color("cp: failed to copy file\n", COLOR_DEFAULT);
  }
}

static void command_echo(const char *args) {
  if (!args || *args == '\0') {
    kputchar_color('\n', COLOR_DEFAULT);
    return;
  }
  if (args[0] == '$') {
    const char *value = command_set_get(++args);
    if (value) {
      kprint_color(value, COLOR_DEFAULT);
    }

  } else {
    kprint_color(args, COLOR_DEFAULT);
  }
  kputchar_color('\n', COLOR_DEFAULT);
}

static void command_ls(const char *args) {
  (void)args;
  initrd_list_files();
}

// static void command_kilo(const char *args) {
//   (void)args;
//   char *argv[2];
//   kprint_color(args, COLOR_DEFAULT);
//   //kilo_main(2, argv);
// }

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

void shell_handle_key_left(void) {
  if (cursor_index > 0) {
    cursor_index--;
    shell_refresh_line();
  }
}

void shell_handle_key_right(void) {
  if (cursor_index < buffer_index) {
    cursor_index++;
    shell_refresh_line();
  }
}

void shell_handle_key_insert(void) {
  insert_mode = !insert_mode;
  set_hardware_cursor_shape(insert_mode);
}

void shell_refresh_line(void) {
  kputchar_color('\r', COLOR_DEFAULT);
  kprint_color("mini-os> ", COLOR_PROMPT);

  for (int i = 0; i < buffer_index; i++) {
    kputchar_color(command_buffer[i], COLOR_DEFAULT);
  }

  // Clear trailing char
  kputchar_color(' ', COLOR_DEFAULT);
  kputchar_color('\b', COLOR_DEFAULT);

  // Move cursor back from end to cursor_index
  int steps_back = buffer_index - cursor_index;
  for (int i = 0; i < steps_back; i++) {
    kputchar_color('\b', COLOR_DEFAULT);
  }
}
