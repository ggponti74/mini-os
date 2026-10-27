#include "editor.h"

#define SYS_SBRK 7

typedef struct {
  size_t size;
  size_t padding;
} allocation_header_t;

extern int editor_syscall(int number, int arg1, int arg2, int arg3);

static void *realloc(void *pointer, size_t size) {
  size_t old_size = 0;
  size_t old_allocation_size;
  size_t allocation_size;
  if (size > (size_t)-1 - 7) {
    return NULL;
  }
  allocation_size = (size + 7) & ~(size_t)7;
  if (pointer) {
    old_size = ((allocation_header_t *)pointer)[-1].size;
    if (size <= old_size) {
      ((allocation_header_t *)pointer)[-1].size = size;
      return pointer;
    }

    old_allocation_size = (old_size + 7) & ~(size_t)7;
    int current_break = editor_syscall(SYS_SBRK, 0, 0, 0);
    if (current_break >= 0 &&
        (unsigned int)current_break ==
            (unsigned int)((char *)pointer + old_allocation_size)) {
      if (editor_syscall(SYS_SBRK,
                         (int)(allocation_size - old_allocation_size), 0, 0) >=
          0) {
        ((allocation_header_t *)pointer)[-1].size = size;
        return pointer;
      }
    }
  }

  int allocation = editor_syscall(
      SYS_SBRK, (int)(sizeof(allocation_header_t) + allocation_size), 0, 0);
  if (allocation < 0) {
    return NULL;
  }

  allocation_header_t *header = (allocation_header_t *)allocation;
  char *replacement = (char *)(header + 1);
  header->size = size;
  size_t copy_size = old_size < size ? old_size : size;
  for (size_t i = 0; pointer && i < copy_size; i++) {
    replacement[i] = ((char *)pointer)[i];
  }
  return replacement;
}

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

  buffer->text = NULL;
  buffer->length = 0;
  buffer->capacity = 0;
  buffer->cursor = 0;
  buffer->preferred_column = 0;
  buffer->preferred_column_valid = 0;
}

int editor_buffer_load(editor_buffer_t *buffer, const char *text, size_t length) {
  if (!buffer || (!text && length > 0)) {
    return -1;
  }

  if (length + 1 > buffer->capacity) {
    size_t capacity = buffer->capacity ? buffer->capacity : 64;
    while (capacity < length + 1) {
      if (capacity > (size_t)-1 / 2) {
        capacity = length + 1;
        break;
      }
      capacity *= 2;
    }
    char *text_buffer = realloc(buffer->text, capacity);
    if (!text_buffer) {
      return -1;
    }
    buffer->text = text_buffer;
    buffer->capacity = capacity;
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
  if (!buffer) {
    return -1;
  }

  if (buffer->length + 2 > buffer->capacity) {
    size_t capacity = buffer->capacity ? buffer->capacity * 2 : 64;
    if (capacity < buffer->length + 2) {
      capacity = buffer->length + 2;
    }
    char *text_buffer = realloc(buffer->text, capacity);
    if (!text_buffer) {
      return -1;
    }
    if (buffer->capacity == 0) {
      text_buffer[0] = '\0';
    }
    buffer->text = text_buffer;
    buffer->capacity = capacity;
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

void editor_buffer_document_home(editor_buffer_t *buffer) {
  if (buffer) {
    buffer->cursor = 0;
    reset_preferred_column(buffer);
  }
}

void editor_buffer_document_end(editor_buffer_t *buffer) {
  if (buffer) {
    buffer->cursor = buffer->length;
    reset_preferred_column(buffer);
  }
}
