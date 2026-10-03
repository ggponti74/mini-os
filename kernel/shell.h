// kernel/shell.h
#ifndef SHELL_H
#define SHELL_H

#define MAX_BUFFER_SIZE 128

void shell_init(void);
void shell_input_char(char c);
void process_command(const char *cmd);

#endif