#include "editor.h"

static size_t line_start(const editor_buffer_t *buffer, size_t position) {
  while (position > 0 && buffer->text[position - 1] != '\n') {
    position--;
  }
  return position;
}

static size_t line_end(const editor_buffer_t *buffer, size_t position) {
  while (position < buffer->length && buffer->text[position] != '\n') {
    position++;
  }
  return position;
}

static void reset_preferred_column(editor_buffer_t *buffer) {
  buffer->preferred_column_valid = 0;
}

void editor_buffer_init(editor_buffer_t *buffer) {
  if (!buffer) {
    return;
  }

  buffer->text[0] = '\0';
  buffer->length = 0;
  buffer->cursor = 0;
  buffer->preferred_column = 0;
  buffer->preferred_column_valid = 0;
}

int editor_buffer_load(editor_buffer_t *buffer, const char *text, size_t length) {
  if (!buffer || (!text && length > 0) ||
      length >= EDITOR_BUFFER_CAPACITY) {
    return -1;
  }

  for (size_t i = 0; i < length; i++) {
    buffer->text[i] = text[i];
  }
  buffer->text[length] = '\0';
  buffer->length = length;
  buffer->cursor = 0;
  reset_preferred_column(buffer);
  return 0;
}

int editor_buffer_insert(editor_buffer_t *buffer, char value) {
  if (!buffer || value == '\0' ||
      buffer->length >= EDITOR_BUFFER_CAPACITY - 1) {
    return -1;
  }

  for (size_t i = buffer->length + 1; i > buffer->cursor; i--) {
    buffer->text[i] = buffer->text[i - 1];
  }
  buffer->text[buffer->cursor] = value;
  buffer->length++;
  buffer->cursor++;
  reset_preferred_column(buffer);
  return 0;
}

int editor_buffer_backspace(editor_buffer_t *buffer) {
  if (!buffer || buffer->cursor == 0) {
    return -1;
  }

  for (size_t i = buffer->cursor; i <= buffer->length; i++) {
    buffer->text[i - 1] = buffer->text[i];
  }
  buffer->length--;
  buffer->cursor--;
  reset_preferred_column(buffer);
  return 0;
}

int editor_buffer_delete(editor_buffer_t *buffer) {
  if (!buffer || buffer->cursor >= buffer->length) {
    return -1;
  }

  for (size_t i = buffer->cursor; i < buffer->length; i++) {
    buffer->text[i] = buffer->text[i + 1];
  }
  buffer->length--;
  reset_preferred_column(buffer);
  return 0;
}

void editor_buffer_left(editor_buffer_t *buffer) {
  if (buffer && buffer->cursor > 0) {
    buffer->cursor--;
    reset_preferred_column(buffer);
  }
}

void editor_buffer_right(editor_buffer_t *buffer) {
  if (buffer && buffer->cursor < buffer->length) {
    buffer->cursor++;
    reset_preferred_column(buffer);
  }
}

void editor_buffer_up(editor_buffer_t *buffer) {
  if (!buffer) {
    return;
  }

  size_t current_start = line_start(buffer, buffer->cursor);
  if (current_start == 0) {
    return;
  }

  if (!buffer->preferred_column_valid) {
    buffer->preferred_column = buffer->cursor - current_start;
    buffer->preferred_column_valid = 1;
  }

  size_t previous_end = current_start - 1;
  size_t previous_start = line_start(buffer, previous_end);
  size_t previous_length = previous_end - previous_start;
  size_t column = buffer->preferred_column;
  if (column > previous_length) {
    column = previous_length;
  }
  buffer->cursor = previous_start + column;
}

void editor_buffer_down(editor_buffer_t *buffer) {
  if (!buffer) {
    return;
  }

  size_t current_end = line_end(buffer, buffer->cursor);
  if (current_end == buffer->length) {
    return;
  }

  if (!buffer->preferred_column_valid) {
    size_t current_start = line_start(buffer, buffer->cursor);
    buffer->preferred_column = buffer->cursor - current_start;
    buffer->preferred_column_valid = 1;
  }

  size_t next_start = current_end + 1;
  size_t next_end = line_end(buffer, next_start);
  size_t next_length = next_end - next_start;
  size_t column = buffer->preferred_column;
  if (column > next_length) {
    column = next_length;
  }
  buffer->cursor = next_start + column;
}

void editor_buffer_home(editor_buffer_t *buffer) {
  if (buffer) {
    buffer->cursor = line_start(buffer, buffer->cursor);
    reset_preferred_column(buffer);
  }
}

void editor_buffer_end(editor_buffer_t *buffer) {
  if (buffer) {
    buffer->cursor = line_end(buffer, buffer->cursor);
    reset_preferred_column(buffer);
  }
}
