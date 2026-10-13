#ifndef MINI_KILO_H
#define MINI_KILO_H

#include <stdint.h>
#include <stddef.h>

void editor_open(const char *filename);
void editor_process_key(int key);
void editor_refresh_screen(void);

#endif
