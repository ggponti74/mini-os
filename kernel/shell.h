// kernel/shell.h
#ifndef SHELL_H
#define SHELL_H

#include "string.h"

#define MAX_BUFFER_SIZE 128

void shell_init(void);
void shell_input_char(char c);
void shell_handle_key_up(void);
void shell_handle_key_down(void);
void process_command(const char *cmd);
int shell_get_timezone(void);

#endif