// kernel/keyboard.h
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define CMD_BUFFER_SIZE 128

void keyboard_init(void);
void keyboard_handler(void);
void set_keyboard_pause_state(int);
int get_keyboard_pause_state(void);

#endif