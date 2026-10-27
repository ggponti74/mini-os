// kernel/keyboard.h
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define CMD_BUFFER_SIZE 128

#define KEY_EVENT_UP       0x100
#define KEY_EVENT_DOWN     0x101
#define KEY_EVENT_LEFT     0x102
#define KEY_EVENT_RIGHT    0x103
#define KEY_EVENT_HOME     0x104
#define KEY_EVENT_END      0x105
#define KEY_EVENT_DELETE   0x106
#define KEY_EVENT_PAGE_UP  0x107
#define KEY_EVENT_PAGE_DOWN 0x108
#define KEY_EVENT_CTRL_S   0x110
#define KEY_EVENT_CTRL_Q   0x111
#define KEY_EVENT_CTRL_HOME 0x112
#define KEY_EVENT_CTRL_END  0x113

void keyboard_init(void);
void keyboard_handler(void);
int keyboard_read_key(void);
void keyboard_clear_queue(void);
void set_keyboard_pause_state(int);
int get_keyboard_pause_state(void);

#endif