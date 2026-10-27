#ifndef USERLAND_EDITOR_H
#define USERLAND_EDITOR_H

#include <stddef.h>

typedef struct {
  char *text;
  size_t length;
  size_t capacity;
  size_t cursor;
  size_t preferred_column;
  int preferred_column_valid;
} editor_buffer_t;

void editor_buffer_init(editor_buffer_t *buffer);
int editor_buffer_load(editor_buffer_t *buffer, const char *text, size_t length);
int editor_buffer_insert(editor_buffer_t *buffer, char value);
int editor_buffer_backspace(editor_buffer_t *buffer);
int editor_buffer_delete(editor_buffer_t *buffer);
void editor_buffer_left(editor_buffer_t *buffer);
void editor_buffer_right(editor_buffer_t *buffer);
void editor_buffer_up(editor_buffer_t *buffer);
void editor_buffer_down(editor_buffer_t *buffer);
void editor_buffer_home(editor_buffer_t *buffer);
void editor_buffer_end(editor_buffer_t *buffer);
void editor_buffer_document_home(editor_buffer_t *buffer);
void editor_buffer_document_end(editor_buffer_t *buffer);

#endif
