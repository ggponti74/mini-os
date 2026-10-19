#include "editor.h"

int editor_entry(int argc, char **argv) {
  editor_buffer_t buffer;

  (void)argc;
  (void)argv;
  editor_buffer_init(&buffer);
  return 0;
}
