// kernel/shell.h
#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

#define MAX_BUFFER_SIZE 128

// Initialize and setup shell state/prompt
void shell_init(void);

// Pass incoming keyboard characters to the shell buffer
void shell_input_char(char c);

// Parse and execute a command string directly
void process_command(const char *cmd);

// Arrow key navigation handlers for command history
void shell_handle_key_up(void);
void shell_handle_key_down(void);

// Getter for system time zone offset
int shell_get_timezone(void);

// External helper to redraw VGA status bar (implemented in kernel/display.c)
extern void update_status_bar(void);

#endif // SHELL_H