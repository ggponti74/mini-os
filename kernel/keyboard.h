// kernel/keyboard.h
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define CMD_BUFFER_SIZE 128

void keyboard_init(void);
void keyboard_handler(void);

#endif