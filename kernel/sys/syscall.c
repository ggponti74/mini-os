#include <stddef.h>
#include <stdint.h>

#include "../display.h"
#include "../fs/initrd.h"
#include "../fs/vfs.h"
#include "../keyboard.h"
#include "../memory.h"
#include "../shell.h"
#include "syscall.h"
#include "stat.h"

#define SYSCALL_FD_COUNT 8
#define SYSCALL_PATH_LENGTH 128
#define STDIN_LINE_CAPACITY 128

typedef struct {
  vfs_node_t *node;
  uint32_t offset;
  uint8_t used;
  uint8_t readable;
  uint8_t writable;
  uint8_t append;
} syscall_fd_t;

static syscall_fd_t descriptors[SYSCALL_FD_COUNT];
static uint8_t stdin_line[STDIN_LINE_CAPACITY];
static uint32_t stdin_line_length;
static uint32_t stdin_line_offset;
static uint8_t stdin_line_ready;

static int open_file(const char *path, uint32_t flags) {
  uint32_t access_mode = flags & O_ACCMODE;
  uint32_t supported_flags = O_ACCMODE | O_CREAT | O_TRUNC | O_APPEND | O_EXCL;
  if ((flags & ~supported_flags) != 0 || access_mode == 3 ||
      ((flags & O_TRUNC) && access_mode == O_RDONLY)) {
    return -1;
  }
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
  if (node && (flags & O_CREAT) && (flags & O_EXCL)) {
    return -1;
  }
  if (!node && (flags & OPEN_CREAT)) {
    node = vfs_create(fs_root, resolved_path, FS_FILE);
  }
  if (!node || (node->flags & FS_DIRECTORY)) {
    return -1;
  }

  for (int fd = 3; fd < SYSCALL_FD_COUNT; fd++) {
    if (!descriptors[fd].used) {
      descriptors[fd].node = node;
      descriptors[fd].offset = (flags & O_APPEND) ? node->length : 0;
      descriptors[fd].used = 1;
      descriptors[fd].readable = access_mode != O_WRONLY;
      descriptors[fd].writable =
          access_mode == O_WRONLY || access_mode == O_RDWR;
      descriptors[fd].append = (flags & O_APPEND) != 0;
      if (flags & O_TRUNC) {
        node->length = 0;
        descriptors[fd].offset = 0;
      }
      return fd;
    }
  }
  return -1;
}

static int32_t read_stdin(uint8_t *buffer, uint32_t count) {
  while (!stdin_line_ready) {
    int key;
    while ((key = keyboard_read_key()) < 0) {
      __asm__ volatile("pause");
    }

    if (key == '\b' || key == 0x7F) {
      if (stdin_line_length > 0) {
        stdin_line_length--;
        kputchar_color('\b', COLOR_DEFAULT);
      }
    } else if (key == '\n' || key == '\r') {
      if (stdin_line_length < STDIN_LINE_CAPACITY) {
        stdin_line[stdin_line_length++] = '\n';
      }
      kputchar_color('\n', COLOR_DEFAULT);
      stdin_line_ready = 1;
    } else if ((key == '\t' || (key >= ' ' && key <= '~')) &&
               stdin_line_length < STDIN_LINE_CAPACITY - 1) {
      stdin_line[stdin_line_length++] = (uint8_t)key;
      kputchar_color((char)key, COLOR_DEFAULT);
    }
  }

  uint32_t available = stdin_line_length - stdin_line_offset;
  uint32_t bytes = count < available ? count : available;
  for (uint32_t i = 0; i < bytes; i++) {
    buffer[i] = stdin_line[stdin_line_offset + i];
  }
  stdin_line_offset += bytes;
  if (stdin_line_offset == stdin_line_length) {
    stdin_line_length = 0;
    stdin_line_offset = 0;
    stdin_line_ready = 0;
  }
  return (int32_t)bytes;
}

int32_t syscall_dispatch(uint32_t number, uint32_t arg1, uint32_t arg2,
                         uint32_t arg3) {
  if (number == SYS_SBRK) {
    return (int32_t)ksbrk((int32_t)arg1);
  }
  if (number == SYS_EXIT) {
    return (int32_t)arg1;
  }
  if (number == SYS_OPEN) {
    return open_file((const char *)arg1, arg2);
  }
  if (number == SYS_READ) {
    int fd = (int)arg1;
    if (arg3 == 0) {
      return 0;
    }
    if (!arg2) {
      return -1;
    }
    if (fd == 0) {
      return read_stdin((uint8_t *)arg2, arg3);
    }
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used ||
        !descriptors[fd].readable) {
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
    if (descriptors[fd].append) {
      descriptors[fd].offset = descriptors[fd].node->length;
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
    descriptors[fd].append = 0;
    return 0;
  }
  if (number == SYS_FSTAT) {
    int fd = (int)arg1;
    struct stat *status = (struct stat *)arg2;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used ||
        !status) {
      return -1;
    }

    vfs_node_t *node = descriptors[fd].node;
    status->st_dev = 0;
    status->st_ino = node->inode;
    status->st_mode = (node->flags & FS_DIRECTORY ? S_IFDIR : S_IFREG) |
                      S_IRUSR | S_IWUSR;
    status->st_nlink = 1;
    status->st_uid = 0;
    status->st_gid = 0;
    status->st_rdev = 0;
    status->st_size = node->length;
    status->st_atime = 0;
    status->st_mtime = 0;
    status->st_ctime = 0;
    return 0;
  }
  if (number == SYS_LSEEK) {
    int fd = (int)arg1;
    int32_t offset = (int32_t)arg2;
    int64_t base;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used) {
      return -1;
    }

    switch (arg3) {
    case SEEK_SET:
      base = 0;
      break;
    case SEEK_CUR:
      base = descriptors[fd].offset;
      break;
    case SEEK_END:
      base = descriptors[fd].node->length;
      break;
    default:
      return -1;
    }

    int64_t result = base + offset;
    if (result < 0 || result > INT32_MAX) {
      return -1;
    }
    descriptors[fd].offset = (uint32_t)result;
    return (int32_t)result;
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
