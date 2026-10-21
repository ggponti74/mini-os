#include <stddef.h>
#include <stdint.h>

#include "../display.h"
#include "../fs/initrd.h"
#include "../fs/vfs.h"
#include "../keyboard.h"
#include "../shell.h"
#include "syscall.h"

#define SYSCALL_FD_COUNT 8
#define SYSCALL_PATH_LENGTH 128

typedef struct {
  vfs_node_t *node;
  uint32_t offset;
  uint8_t used;
  uint8_t readable;
  uint8_t writable;
} syscall_fd_t;

static syscall_fd_t descriptors[SYSCALL_FD_COUNT];

static int open_file(const char *path, uint32_t flags) {
  if (!path || *path == '\0') {
    return -1;
  }

  uint32_t length = 0;
  while (path[length] != '\0' && length < SYSCALL_PATH_LENGTH) {
    length++;
  }
  if (length == SYSCALL_PATH_LENGTH) {
    return -1;
  }

  char resolved_path[SYSCALL_PATH_LENGTH];
  if (shell_resolve_path(path, resolved_path, sizeof(resolved_path)) != 0) {
    return -1;
  }
  vfs_node_t *node = vfs_lookup(fs_root, resolved_path);
  if (!node && (flags & OPEN_CREAT)) {
    node = vfs_create(fs_root, resolved_path, FS_FILE);
  }
  if (!node || (node->flags & FS_DIRECTORY)) {
    return -1;
  }

  for (int fd = 3; fd < SYSCALL_FD_COUNT; fd++) {
    if (!descriptors[fd].used) {
      descriptors[fd].node = node;
      descriptors[fd].offset = 0;
      descriptors[fd].used = 1;
      descriptors[fd].readable = (flags & 3) != OPEN_WRONLY;
      descriptors[fd].writable =
          (flags & 3) == OPEN_WRONLY || (flags & 3) == OPEN_RDWR;
      return fd;
    }
  }
  return -1;
}

int32_t syscall_dispatch(uint32_t number, uint32_t arg1, uint32_t arg2,
                         uint32_t arg3) {
  if (number == SYS_EXIT) {
    return (int32_t)arg1;
  }
  if (number == SYS_OPEN) {
    return open_file((const char *)arg1, arg2);
  }
  if (number == SYS_READ) {
    int fd = (int)arg1;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used ||
        !descriptors[fd].readable || !arg2) {
      return -1;
    }
    uint32_t bytes = vfs_read(descriptors[fd].node, descriptors[fd].offset,
                              arg3, (uint8_t *)arg2);
    descriptors[fd].offset += bytes;
    return (int32_t)bytes;
  }
  if (number == SYS_WRITE) {
    int fd = (int)arg1;
    const uint8_t *buffer = (const uint8_t *)arg2;
    if (!buffer) {
      return -1;
    }
    if (fd == 1 || fd == 2) {
      for (uint32_t i = 0; i < arg3; i++) {
        kputchar_color((char)buffer[i], COLOR_DEFAULT);
      }
      return (int32_t)arg3;
    }
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used ||
        !descriptors[fd].writable) {
      return -1;
    }
    uint32_t bytes = vfs_write(descriptors[fd].node, descriptors[fd].offset,
                               arg3, buffer);
    descriptors[fd].offset += bytes;
    return (int32_t)bytes;
  }
  if (number == SYS_CLOSE) {
    int fd = (int)arg1;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used) {
      return -1;
    }
    descriptors[fd].used = 0;
    descriptors[fd].node = NULL;
    descriptors[fd].offset = 0;
    return 0;
  }
  if (number == SYS_GETKEY) {
    int key;
    while ((key = keyboard_read_key()) < 0) {
      __asm__ volatile("pause");
    }
    return key;
  }
  if (number == SYS_CLEAR) {
    clear_screen();
    return 0;
  }
  if (number == SYS_CURSOR) {
    display_set_console_cursor((uint8_t)arg1, (uint8_t)arg2);
    return 0;
  }
  if (number == SYS_FTRUNCATE) {
    int fd = (int)arg1;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used ||
        !descriptors[fd].writable) {
      return -1;
    }
    descriptors[fd].node->length = 0;
    descriptors[fd].offset = 0;
    return 0;
  }

  return -1;
}
