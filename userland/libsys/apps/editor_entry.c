#include "editor.h"

#define SYS_OPEN 5
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_CLOSE 6
#define SYS_GETKEY 9
#define SYS_CLEAR 10
#define SYS_CURSOR 11
#define SYS_FTRUNCATE 12

#define OPEN_RDWR 2
#define OPEN_CREAT 0x100

#define KEY_UP 0x100
#define KEY_DOWN 0x101
#define KEY_LEFT 0x102
#define KEY_RIGHT 0x103
#define KEY_HOME 0x104
#define KEY_END 0x105
#define KEY_DELETE 0x106
#define KEY_CTRL_S 0x110
#define KEY_CTRL_Q 0x111

#define EDITOR_ROWS 22
#define SCREEN_COLUMNS 79

extern int editor_syscall(int number, int arg1, int arg2, int arg3);

static int console_write(const char *text, unsigned int length) {
  return editor_syscall(SYS_WRITE, 1, (int)text, (int)length);
}

static unsigned int string_length(const char *text) {
  unsigned int length = 0;
  while (text[length] != '\0') {
    length++;
  }
  return length;
}

static void write_string(const char *text) {
  console_write(text, string_length(text));
}

static void redraw(editor_buffer_t *buffer, const char *filename,
                   const char *status, int dirty) {
  unsigned int cursor_line = 0;
  unsigned int cursor_column = 0;
  unsigned int first_line;
  unsigned int line_number = 0;
  unsigned int position = 0;

  for (unsigned int i = 0; i < buffer->cursor; i++) {
    if (buffer->text[i] == '\n') {
      cursor_line++;
      cursor_column = 0;
    } else {
      cursor_column++;
    }
  }

  first_line = cursor_line >= EDITOR_ROWS ? cursor_line - EDITOR_ROWS + 1 : 0;
  editor_syscall(SYS_CLEAR, 0, 0, 0);
  write_string("editor: ");
  for (unsigned int i = 0; filename[i] != '\0' && i < 55; i++) {
    console_write(&filename[i], 1);
  }
  write_string(dirty ? " *\n" : "\n");
  if (status && status[0] != '\0') {
    write_string(status);
  } else {
    write_string("Ctrl+S save | Ctrl+Q quit | Arrows move | Backspace/Delete edit");
  }
  write_string("\n");

  while (position < buffer->length && line_number < first_line) {
    if (buffer->text[position++] == '\n') {
      line_number++;
    }
  }

  for (unsigned int row = 0; row < EDITOR_ROWS; row++) {
    unsigned int line_length = 0;
    while (position + line_length < buffer->length &&
           buffer->text[position + line_length] != '\n') {
      line_length++;
    }
    unsigned int visible_length =
        line_length < SCREEN_COLUMNS ? line_length : SCREEN_COLUMNS;
    for (unsigned int i = 0; i < visible_length; i++) {
      char value = buffer->text[position + i];
      if ((unsigned char)value < ' ' || (unsigned char)value > '~') {
        value = '?';
      }
      console_write(&value, 1);
    }
    if (row + 1 < EDITOR_ROWS) {
      write_string("\n");
    }
    position += line_length;
    if (position < buffer->length && buffer->text[position] == '\n') {
      position++;
    } else if (position >= buffer->length) {
      break;
    }
  }

  if (cursor_column >= SCREEN_COLUMNS) {
    cursor_column = SCREEN_COLUMNS - 1;
  }
  unsigned int cursor_row = 2 + cursor_line - first_line;
  if (cursor_row >= 2 + EDITOR_ROWS) {
    cursor_row = 1 + EDITOR_ROWS;
  }
  editor_syscall(SYS_CURSOR, (int)cursor_column, (int)cursor_row, 0);
}

static int save_file(int fd, const editor_buffer_t *buffer) {
  if (editor_syscall(SYS_FTRUNCATE, fd, 0, 0) < 0) {
    return -1;
  }
  unsigned int written = 0;
  while (written < buffer->length) {
    int result = editor_syscall(SYS_WRITE, fd,
                                (int)(buffer->text + written),
                                (int)(buffer->length - written));
    if (result <= 0) {
      return -1;
    }
    written += (unsigned int)result;
  }
  return 0;
}

int editor_entry(int argc, char **argv) {
  editor_buffer_t buffer;
  const char *filename;
  const char *status = "";
  unsigned int length = 0;
  int fd;
  int dirty = 0;

  if (argc < 2 || !argv[1] || argv[1][0] == '\0') {
    write_string("Usage: editor <file>\n");
    return 1;
  }
  filename = argv[1];
  fd = editor_syscall(SYS_OPEN, (int)filename, OPEN_RDWR | OPEN_CREAT, 0);
  if (fd < 0) {
    write_string("editor: could not open file\n");
    return 1;
  }

  editor_buffer_init(&buffer);
  while (length < EDITOR_BUFFER_CAPACITY - 1) {
    int count = editor_syscall(SYS_READ, fd,
                               (int)(buffer.text + length),
                               (int)(EDITOR_BUFFER_CAPACITY - 1 - length));
    if (count < 0) {
      editor_syscall(SYS_CLOSE, fd, 0, 0);
      write_string("editor: could not read file\n");
      return 1;
    }
    if (count == 0) {
      break;
    }
    length += (unsigned int)count;
  }
  if (length == EDITOR_BUFFER_CAPACITY - 1) {
    char extra;
    if (editor_syscall(SYS_READ, fd, (int)&extra, 1) > 0) {
      editor_syscall(SYS_CLOSE, fd, 0, 0);
      write_string("editor: file exceeds the 4095-byte buffer limit\n");
      return 1;
    }
  }
  if (editor_buffer_load(&buffer, buffer.text, length) != 0) {
    editor_syscall(SYS_CLOSE, fd, 0, 0);
    write_string("editor: could not load file\n");
    return 1;
  }

  redraw(&buffer, filename, status, dirty);
  for (;;) {
    int key = editor_syscall(SYS_GETKEY, 0, 0, 0);
    status = "";
    switch (key) {
    case KEY_LEFT:
      editor_buffer_left(&buffer);
      break;
    case KEY_RIGHT:
      editor_buffer_right(&buffer);
      break;
    case KEY_UP:
      editor_buffer_up(&buffer);
      break;
    case KEY_DOWN:
      editor_buffer_down(&buffer);
      break;
    case KEY_HOME:
      editor_buffer_home(&buffer);
      break;
    case KEY_END:
      editor_buffer_end(&buffer);
      break;
    case KEY_DELETE:
      dirty |= editor_buffer_delete(&buffer) == 0;
      break;
    case '\b':
      dirty |= editor_buffer_backspace(&buffer) == 0;
      break;
    case '\n':
      dirty |= editor_buffer_insert(&buffer, '\n') == 0;
      break;
    case KEY_CTRL_S:
      if (save_file(fd, &buffer) == 0) {
        dirty = 0;
        status = "Saved";
      } else {
        status = "Save failed";
      }
      break;
    case KEY_CTRL_Q:
      editor_syscall(SYS_CLOSE, fd, 0, 0);
      editor_syscall(SYS_CLEAR, 0, 0, 0);
      write_string("Editor closed.\n");
      return 0;
    default:
      if (key >= ' ' && key <= '~') {
        dirty |= editor_buffer_insert(&buffer, (char)key) == 0;
      } else {
        continue;
      }
      break;
    }
    redraw(&buffer, filename, status, dirty);
  }
}
