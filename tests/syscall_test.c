#include <stddef.h>
#include <stdint.h>

#include "display.h"
#include "errno.h"
#include "fs/vfs.h"
#include "memory.h"
#include "sys/stat.h"
#include "sys/syscall.h"

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      failures++;                                                              \
    }                                                                          \
  } while (0)

_Static_assert(sizeof(struct stat) == 72 &&
                   offsetof(struct stat, st_size) == 16 &&
                   offsetof(struct stat, st_atim) == 20 &&
                   offsetof(struct stat, st_blksize) == 56 &&
                   offsetof(struct stat, st_blocks) == 60,
               "Newlib i686 stat ABI");

static int failures;
static uint8_t file_data[256];
static vfs_node_t file_node;
static vfs_node_t directory_node;
static vfs_node_t created_node;
static int created_exists;
static uint8_t created_data[256];
static int keyboard_keys[8];
static uint32_t keyboard_key_count;
static uint32_t keyboard_key_index;
static uintptr_t heap_break = 0x1000;

vfs_node_t *fs_root;

static int strings_equal(const char *left, const char *right) {
  while (*left && *right && *left == *right) {
    left++;
    right++;
  }
  return *left == *right;
}

static void reset_file(void) {
  for (uint32_t i = 0; i < sizeof(file_data); i++) {
    file_data[i] = 0;
  }
  file_data[0] = 'a';
  file_data[1] = 'b';
  file_data[2] = 'c';
  file_node.flags = FS_FILE;
  file_node.length = 3;
  file_node.inode = 42;
  file_node.device_data = file_data;
}

static void test_open_read_seek_and_stat(void) {
  uint8_t buffer[8] = {0};
  struct stat status;
  CHECK(syscall_dispatch(SYS_FSTAT, 0, (uint32_t)&status, 0) == 0);
  CHECK(S_ISCHR(status.st_mode));
  CHECK(status.st_size == 0);
  CHECK(status.st_blksize == 512);
  CHECK(status.st_blocks == 0);
  CHECK(syscall_dispatch(SYS_FSTAT, 1, (uint32_t)&status, 0) == 0);
  CHECK(S_ISCHR(status.st_mode));
  int fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR, 0);

  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_READ, fd, (uint32_t)buffer, 2) == 2);
  CHECK(buffer[0] == 'a' && buffer[1] == 'b');
  CHECK(syscall_dispatch(SYS_LSEEK, fd, (uint32_t)-1, SEEK_CUR) == 1);
  CHECK(syscall_dispatch(SYS_READ, fd, (uint32_t)buffer, sizeof(buffer)) == 2);
  CHECK(buffer[0] == 'b' && buffer[1] == 'c');
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_END) == 3);
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, 99) == -EINVAL);
  CHECK(syscall_dispatch(SYS_FSTAT, fd, (uint32_t)&status, 0) == 0);
  CHECK(status.st_ino == 42);
  CHECK(status.st_size == 3);
  CHECK(status.st_blksize == 512);
  CHECK(status.st_blocks == 1);
  CHECK(S_ISREG(status.st_mode));
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == -EBADF);
  CHECK(syscall_dispatch(SYS_LSEEK, 1, 0, SEEK_SET) == -ESPIPE);
}

static void test_open_flags_and_errors(void) {
  int fd;

  reset_file();
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_WRONLY | O_TRUNC, 0);
  CHECK(fd == 3);
  CHECK(file_node.length == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  reset_file();
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_WRONLY | O_APPEND, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_SET) == 0);
  CHECK(syscall_dispatch(SYS_WRITE, fd, (uint32_t)"x", 1) == 1);
  CHECK(file_node.length == 4 && file_data[3] == 'x');
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"created",
                        O_CREAT | O_EXCL | O_RDWR, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"created",
                         O_CREAT | O_EXCL | O_RDWR, 0) == -EEXIST);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"missing", O_RDONLY, 0) ==
        -ENOENT);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR | 0x8000, 0) ==
        -EINVAL);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"dir", O_RDONLY, 0) == -EISDIR);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR | O_TRUNC, 0) ==
        3);
  CHECK(file_node.length == 0);
}

static void test_standard_input_and_sbrk(void) {
  uint8_t buffer[8] = {0};

  keyboard_keys[0] = 'h';
  keyboard_keys[1] = 'i';
  keyboard_keys[2] = '\b';
  keyboard_keys[3] = 'e';
  keyboard_keys[4] = '\n';
  keyboard_key_count = 5;
  keyboard_key_index = 0;
  CHECK(syscall_dispatch(SYS_READ, 0, (uint32_t)buffer, 2) == 2);
  CHECK(buffer[0] == 'h' && buffer[1] == 'e');
  CHECK(syscall_dispatch(SYS_READ, 0, (uint32_t)buffer, sizeof(buffer)) == 1);
  CHECK(buffer[0] == '\n');
  CHECK(syscall_dispatch(SYS_READ, 0, (uint32_t)buffer, 0) == 0);
  CHECK(syscall_dispatch(SYS_READ, 0, 0, 1) == -EFAULT);
  CHECK(syscall_dispatch(SYS_READ, 7, (uint32_t)buffer, 1) == -EBADF);
  CHECK(syscall_dispatch(SYS_WRITE, 7, (uint32_t)buffer, 1) == -EBADF);

  CHECK(syscall_dispatch(SYS_SBRK, 16, 0, 0) == 0x1000);
  CHECK(syscall_dispatch(SYS_SBRK, 0, 0, 0) == 0x1010);
  CHECK(syscall_dispatch(SYS_SBRK, 1, 0, 0) == -ENOMEM);
  CHECK(syscall_dispatch(0x7fffffff, 0, 0, 0) == -ENOSYS);
}

static int32_t test_main(void) {
  reset_file();
  directory_node.flags = FS_DIRECTORY;
  directory_node.inode = 7;
  created_exists = 0;
  keyboard_key_count = 0;

  test_open_read_seek_and_stat();
  test_open_flags_and_errors();
  test_standard_input_and_sbrk();

  return failures;
}

void _start(void) {
  int result = test_main();
  __asm__ volatile("int $0x80" : : "a"(1), "b"(result) : "memory");
  __builtin_unreachable();
}

void kputchar_color(char character, uint8_t color) {
  (void)character;
  (void)color;
}

void clear_screen(void) {}

void display_set_console_cursor(uint8_t x, uint8_t y) {
  (void)x;
  (void)y;
}

int keyboard_read_key(void) {
  if (keyboard_key_index >= keyboard_key_count) {
    return -1;
  }
  return keyboard_keys[keyboard_key_index++];
}

void *ksbrk(ptrdiff_t increment) {
  if (increment == 16) {
    uintptr_t previous = heap_break;
    heap_break += (uintptr_t)increment;
    return (void *)previous;
  }
  if (increment == 0) {
    return (void *)heap_break;
  }
  return (void *)-1;
}

int shell_resolve_path(const char *path, char *resolved, uint32_t size) {
  uint32_t i = 0;
  while (path[i]) {
    if (i + 1 >= size) {
      return -1;
    }
    resolved[i] = path[i];
    i++;
  }
  resolved[i] = '\0';
  return 0;
}

vfs_node_t *vfs_lookup(vfs_node_t *root, const char *path) {
  (void)root;
  if (strings_equal(path, "file")) {
    return &file_node;
  }
  if (strings_equal(path, "dir")) {
    return &directory_node;
  }
  if (strings_equal(path, "created") && created_exists) {
    return &created_node;
  }
  return NULL;
}

vfs_node_t *vfs_create(vfs_node_t *parent, const char *name, uint32_t flags) {
  (void)parent;
  if (!strings_equal(name, "created") || flags != FS_FILE || created_exists) {
    return NULL;
  }
  created_exists = 1;
  created_node.flags = FS_FILE;
  created_node.length = 0;
  created_node.inode = 99;
  created_node.device_data = created_data;
  return &created_node;
}

uint32_t vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size,
                  uint8_t *buffer) {
  if (offset >= node->length) {
    return 0;
  }
  if (size > node->length - offset) {
    size = node->length - offset;
  }
  uint8_t *data = node->device_data;
  for (uint32_t i = 0; i < size; i++) {
    buffer[i] = data[offset + i];
  }
  return size;
}

uint32_t vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size,
                   const uint8_t *buffer) {
  if (offset >= sizeof(file_data)) {
    return 0;
  }
  if (size > sizeof(file_data) - offset) {
    size = sizeof(file_data) - offset;
  }
  uint8_t *data = node->device_data;
  for (uint32_t i = 0; i < size; i++) {
    data[offset + i] = buffer[i];
  }
  if (offset + size > node->length) {
    node->length = offset + size;
  }
  return size;
}
