// src/exec.c
#include "display.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include <stddef.h>
#include <stdint.h>

static uint8_t binary_image[8192];

int exec_flat_binary(const char *path, int argc, char **argv) {

  // 1. Look up the binary file in the VFS / RAMDisk pool
  vfs_node_t *file = initrd_find_file(path);

  if (!file) {
    return -1; // File not found -> shell will print "Unknown command or file"
  }

  // Safety check: Ensure file has actual content
  if (file->length == 0) {
    return -1;
  }

  if (file->length > sizeof(binary_image)) {
    return -3;
  }

  if (vfs_read(file, 0, file->length, binary_image) != file->length) {
    return -3;
  }

  typedef int (*binary_main_t)(int argc, char **argv);
  binary_main_t entry = (binary_main_t)binary_image;
  return entry(argc, argv);
}
