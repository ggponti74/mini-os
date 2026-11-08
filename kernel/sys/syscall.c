#include <stddef.h>
#include <stdint.h>

#include "../display.h"
#include "../fs/initrd.h"
#include "../fs/vfs.h"
#include "../keyboard.h"
#include "../memory.h"
#include "../memory_layout.h"
#include "../shell.h"
#include "../errno.h"
#include "syscall.h"
#include "stat.h"

#define SYSCALL_FD_COUNT 8
#define SYSCALL_PATH_LENGTH 128
#define STDIN_LINE_CAPACITY 128

typedef struct {
  vfs_node_t *node;
  uint32_t offset;
  uint8_t readable;
  uint8_t writable;
  uint8_t append;
  uint8_t references;
} syscall_open_file_t;

typedef struct {
  syscall_open_file_t *file;
  uint8_t used;
} syscall_fd_t;

static syscall_fd_t descriptors[SYSCALL_FD_COUNT];
static syscall_open_file_t open_files[SYSCALL_FD_COUNT];
static uint8_t stdin_line[STDIN_LINE_CAPACITY];
static uint32_t stdin_line_length;
static uint32_t stdin_line_offset;
static uint8_t stdin_line_ready;
static uint32_t app_heap_base;
static uint32_t app_heap_break;
static uint32_t app_heap_limit;
static uint8_t app_heap_initialized;
static uint32_t process_umask = 0022;

void syscall_reset_app_heap(void) {
  app_heap_base = 0;
  app_heap_break = 0;
  app_heap_limit = 0;
  app_heap_initialized = 0;
}

static int32_t app_sbrk(int32_t increment, uint32_t base, uint32_t limit) {
  if (!app_heap_initialized) {
    if (base < USER_APP_LOAD_BASE || base > USER_APP_HEAP_LIMIT ||
        limit != USER_APP_HEAP_LIMIT) {
      return -EINVAL;
    }
    app_heap_base = base;
    app_heap_break = base;
    app_heap_limit = limit;
    app_heap_initialized = 1;
  } else if (base != app_heap_base || limit != app_heap_limit) {
    return -EINVAL;
  }

  uint32_t previous_break = app_heap_break;
  if (increment >= 0) {
    uint32_t amount = (uint32_t)increment;
    if (amount > app_heap_limit - app_heap_break) {
      return -ENOMEM;
    }
    app_heap_break += amount;
  } else {
    uint32_t amount = 0u - (uint32_t)increment;
    if (amount > app_heap_break - app_heap_base) {
      return -ENOMEM;
    }
    app_heap_break -= amount;
  }
  return (int32_t)previous_break;
}

static int open_file(const char *path, uint32_t flags, uint32_t mode) {
  uint32_t access_mode = flags & O_ACCMODE;
  uint32_t supported_flags = O_ACCMODE | O_CREAT | O_TRUNC | O_APPEND | O_EXCL;
  if ((flags & ~supported_flags) != 0 || access_mode == 3 ||
      ((flags & O_TRUNC) && access_mode == O_RDONLY)) {
    return -EINVAL;
  }
  if (!path) {
    return -EFAULT;
  }
  if (*path == '\0') {
    return -ENOENT;
  }

  uint32_t length = 0;
  while (path[length] != '\0' && length < SYSCALL_PATH_LENGTH) {
    length++;
  }
  if (length == SYSCALL_PATH_LENGTH) {
    return -ENAMETOOLONG;
  }

  char resolved_path[SYSCALL_PATH_LENGTH];
  if (shell_resolve_path(path, resolved_path, sizeof(resolved_path)) != 0) {
    return -ENOENT;
  }
  vfs_node_t *node = vfs_lookup(fs_root, resolved_path);
  int created = 0;
  if (node && (flags & O_CREAT) && (flags & O_EXCL)) {
    return -EEXIST;
  }
  if (!node && (flags & OPEN_CREAT)) {
    node = vfs_create(fs_root, resolved_path, FS_FILE);
    created = node != NULL;
  }
  if (!node) {
    return -ENOENT;
  }
  if (node->flags & FS_DIRECTORY) {
    return -EISDIR;
  }
  if (created) {
    node->mode = mode & ~process_umask & 0777;
  }

  uint32_t permissions = node->mode & 0777;
  if (!created &&
      ((access_mode != O_WRONLY && !(permissions & S_IRUSR)) ||
       ((access_mode == O_WRONLY || access_mode == O_RDWR) &&
        !(permissions & S_IWUSR)))) {
    return -EACCES;
  }

  for (int fd = 3; fd < SYSCALL_FD_COUNT; fd++) {
    if (!descriptors[fd].used) {
      syscall_open_file_t *file = NULL;
      for (int i = 0; i < SYSCALL_FD_COUNT; i++) {
        if (open_files[i].references == 0) {
          file = &open_files[i];
          break;
        }
      }
      if (!file) {
        return -EMFILE;
      }
      file->node = node;
      file->offset = (flags & O_APPEND) ? node->length : 0;
      file->readable = access_mode != O_WRONLY;
      file->writable = access_mode == O_WRONLY || access_mode == O_RDWR;
      file->append = (flags & O_APPEND) != 0;
      file->references = 1;
      descriptors[fd].file = file;
      descriptors[fd].used = 1;
      if (flags & O_TRUNC) {
        node->length = 0;
        file->offset = 0;
      }
      return fd;
    }
  }
  return -EMFILE;
}

static void fill_node_stat(struct stat *status, const vfs_node_t *node) {
  status->st_dev = 0;
  status->st_ino = node->inode;
  status->st_mode = (node->flags & FS_DIRECTORY ? S_IFDIR : S_IFREG) |
                    (node->mode & 0777);
  status->st_nlink = 1;
  status->st_uid = 0;
  status->st_gid = 0;
  status->st_rdev = 0;
  status->st_size = node->length;
  status->st_atim.tv_sec = 0;
  status->st_atim.tv_nsec = 0;
  status->st_mtim.tv_sec = 0;
  status->st_mtim.tv_nsec = 0;
  status->st_ctim.tv_sec = 0;
  status->st_ctim.tv_nsec = 0;
  status->st_blksize = 512;
  status->st_blocks = (node->length + 511) / 512;
  status->st_spare4[0] = 0;
  status->st_spare4[1] = 0;
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
    void *result = ksbrk((int32_t)arg1);
    return result == (void *)-1 ? -ENOMEM : (int32_t)result;
  }
  if (number == SYS_APP_SBRK) {
    return app_sbrk((int32_t)arg1, arg2, arg3);
  }
  if (number == SYS_EXIT) {
    return (int32_t)arg1;
  }
  if (number == SYS_OPEN) {
    return open_file((const char *)arg1, arg2, arg3);
  }
  if (number == SYS_READ) {
    int fd = (int)arg1;
    if (arg3 == 0) {
      return 0;
    }
    if (!arg2) {
      return -EFAULT;
    }
    if (fd == 0) {
      return read_stdin((uint8_t *)arg2, arg3);
    }
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used ||
        !descriptors[fd].file->readable) {
      return -EBADF;
    }
    uint32_t bytes = vfs_read(descriptors[fd].file->node,
                              descriptors[fd].file->offset,
                              arg3, (uint8_t *)arg2);
    descriptors[fd].file->offset += bytes;
    return (int32_t)bytes;
  }
  if (number == SYS_WRITE) {
    int fd = (int)arg1;
    const uint8_t *buffer = (const uint8_t *)arg2;
    if (arg3 == 0) {
      return 0;
    }
    if (!buffer) {
      return -EFAULT;
    }
    if (fd == 1 || fd == 2) {
      for (uint32_t i = 0; i < arg3; i++) {
        kputchar_color((char)buffer[i], COLOR_DEFAULT);
      }
      return (int32_t)arg3;
    }
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used ||
        !descriptors[fd].file->writable) {
      return -EBADF;
    }
    if (descriptors[fd].file->append) {
      descriptors[fd].file->offset = descriptors[fd].file->node->length;
    }
    uint32_t bytes = vfs_write(descriptors[fd].file->node,
                               descriptors[fd].file->offset, arg3, buffer);
    descriptors[fd].file->offset += bytes;
    return (int32_t)bytes;
  }
  if (number == SYS_CLOSE) {
    int fd = (int)arg1;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used) {
      return -EBADF;
    }
    descriptors[fd].file->references--;
    descriptors[fd].file = NULL;
    descriptors[fd].used = 0;
    return 0;
  }
  if (number == SYS_FSTAT) {
    int fd = (int)arg1;
    struct stat *status = (struct stat *)arg2;
    if (!status) {
      return -EFAULT;
    }

    if (fd >= 0 && fd < 3) {
      status->st_dev = 0;
      status->st_ino = 0;
      status->st_mode = S_IFCHR | S_IRUSR | S_IWUSR;
      status->st_nlink = 1;
      status->st_uid = 0;
      status->st_gid = 0;
      status->st_rdev = 0;
      status->st_size = 0;
      status->st_atim.tv_sec = status->st_atim.tv_nsec = 0;
      status->st_mtim.tv_sec = status->st_mtim.tv_nsec = 0;
      status->st_ctim.tv_sec = status->st_ctim.tv_nsec = 0;
      status->st_blksize = 512;
      status->st_blocks = 0;
      status->st_spare4[0] = status->st_spare4[1] = 0;
      return 0;
    }
    if (fd >= SYSCALL_FD_COUNT || fd < 0 || !descriptors[fd].used) {
      return -EBADF;
    }
    fill_node_stat(status, descriptors[fd].file->node);
    return 0;
  }
  if (number == SYS_STAT) {
    const char *path = (const char *)arg1;
    struct stat *status = (struct stat *)arg2;
    char resolved_path[SYSCALL_PATH_LENGTH];
    if (!path || !status) {
      return -EFAULT;
    }
    if (!path[0]) {
      return -ENOENT;
    }
    if (shell_resolve_path(path, resolved_path, sizeof(resolved_path)) != 0) {
      return -ENAMETOOLONG;
    }
    vfs_node_t *node = vfs_lookup(fs_root, resolved_path);
    if (!node) {
      return -ENOENT;
    }
    fill_node_stat(status, node);
    return 0;
  }
  if (number == SYS_LSEEK) {
    int fd = (int)arg1;
    int32_t offset = (int32_t)arg2;
    int64_t base;
    if (fd >= 0 && fd < 3) {
      return -ESPIPE;
    }
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used) {
      return -EBADF;
    }

    switch (arg3) {
    case SEEK_SET:
      base = 0;
      break;
    case SEEK_CUR:
      base = descriptors[fd].file->offset;
      break;
    case SEEK_END:
      base = descriptors[fd].file->node->length;
      break;
    default:
      return -EINVAL;
    }

    int64_t result = base + offset;
    if (result < 0 || result > INT32_MAX) {
      return result < 0 ? -EINVAL : -EOVERFLOW;
    }
    descriptors[fd].file->offset = (uint32_t)result;
    return (int32_t)result;
  }
  if (number == SYS_MKDIR) {
    char resolved_path[SYSCALL_PATH_LENGTH];
    if (!arg1 || !((const char *)arg1)[0]) {
      return -ENOENT;
    }
    if (shell_resolve_path((const char *)arg1, resolved_path,
                          sizeof(resolved_path)) != 0) {
      return -ENAMETOOLONG;
    }
    if (vfs_lookup(fs_root, resolved_path)) {
      return -EEXIST;
    }
    vfs_node_t *directory = vfs_create(fs_root, resolved_path, FS_DIRECTORY);
    if (!directory) {
      return -EACCES;
    }
    directory->mode = arg2 & ~process_umask & 0777;
    return 0;
  }
  if (number == SYS_RMDIR) {
    char resolved_path[SYSCALL_PATH_LENGTH];
    if (!arg1 || !((const char *)arg1)[0]) {
      return -ENOENT;
    }
    if (shell_resolve_path((const char *)arg1, resolved_path,
                          sizeof(resolved_path)) != 0) {
      return -ENAMETOOLONG;
    }
    vfs_node_t *directory = vfs_lookup(fs_root, resolved_path);
    if (!directory || !(directory->flags & FS_DIRECTORY)) {
      return -ENOENT;
    }
    int result = initrd_remove_directory(resolved_path);
    if (result == 0) {
      return 0;
    }
    if (result == -2) {
      return -ENOTEMPTY;
    }
    return -ENOENT;
  }
  if (number == SYS_UNLINK) {
    char resolved_path[SYSCALL_PATH_LENGTH];
    if (!arg1 || !((const char *)arg1)[0]) {
      return -ENOENT;
    }
    if (shell_resolve_path((const char *)arg1, resolved_path,
                          sizeof(resolved_path)) != 0) {
      return -ENAMETOOLONG;
    }
    vfs_node_t *node = vfs_lookup(fs_root, resolved_path);
    if (!node) {
      return -ENOENT;
    }
    if (node->flags & FS_DIRECTORY) {
      return -EISDIR;
    }
    int result = initrd_remove_file(resolved_path);
    if (result == 0) {
      return 0;
    }
    return -ENOENT;
  }
  if (number == SYS_ACCESS) {
    char resolved_path[SYSCALL_PATH_LENGTH];
    if (arg2 & ~(R_OK | W_OK | X_OK)) {
      return -EINVAL;
    }
    if (!arg1 || !((const char *)arg1)[0]) {
      return -ENOENT;
    }
    if (shell_resolve_path((const char *)arg1, resolved_path,
                          sizeof(resolved_path)) != 0) {
      return -ENAMETOOLONG;
    }
    vfs_node_t *node = vfs_lookup(fs_root, resolved_path);
    if (!node) {
      return -ENOENT;
    }
    if (arg2 == F_OK) {
      return 0;
    }
    uint32_t permissions = node->mode & 0777;
    if ((arg2 & R_OK) && !(permissions & S_IRUSR)) {
      return -EACCES;
    }
    if ((arg2 & W_OK) && !(permissions & S_IWUSR)) {
      return -EACCES;
    }
    if ((arg2 & X_OK) && !(permissions & S_IXUSR)) {
      return -EACCES;
    }
    return 0;
  }
  if (number == SYS_RENAME) {
    char old_path[SYSCALL_PATH_LENGTH];
    char new_path[SYSCALL_PATH_LENGTH];
    if (!arg1 || !((const char *)arg1)[0] || !arg2 || !((const char *)arg2)[0]) {
      return -ENOENT;
    }
    if (shell_resolve_path((const char *)arg1, old_path, sizeof(old_path)) != 0 ||
        shell_resolve_path((const char *)arg2, new_path, sizeof(new_path)) != 0) {
      return -ENAMETOOLONG;
    }
    if (!vfs_lookup(fs_root, old_path)) {
      return -ENOENT;
    }
    int result = initrd_move_file(old_path, new_path);
    if (result == 0) {
      return 0;
    }
    return -EACCES;
  }
  if (number == SYS_CHMOD) {
    char resolved_path[SYSCALL_PATH_LENGTH];
    if (!arg1 || !((const char *)arg1)[0]) {
      return -ENOENT;
    }
    if (shell_resolve_path((const char *)arg1, resolved_path,
                          sizeof(resolved_path)) != 0) {
      return -ENAMETOOLONG;
    }
    vfs_node_t *node = vfs_lookup(fs_root, resolved_path);
    if (!node) {
      return -ENOENT;
    }
    node->mode = arg2 & 0777;
    return 0;
  }
  if (number == SYS_FCHMOD) {
    int fd = (int)arg1;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used) {
      return -EBADF;
    }
    descriptors[fd].file->node->mode = arg2 & 0777;
    return 0;
  }
  if (number == SYS_CHDIR) {
    char resolved_path[SYSCALL_PATH_LENGTH];
    if (!arg1 || !((const char *)arg1)[0]) {
      return -ENOENT;
    }
    if (shell_resolve_path((const char *)arg1, resolved_path,
                          sizeof(resolved_path)) != 0) {
      return -ENAMETOOLONG;
    }
    vfs_node_t *directory = vfs_lookup(fs_root, resolved_path);
    if (!directory || !(directory->flags & FS_DIRECTORY)) {
      return -ENOTDIR;
    }
    if (shell_set_current_directory(resolved_path) != 0) {
      return -ENAMETOOLONG;
    }
    return 0;
  }
  if (number == SYS_GETCWD) {
    char *buffer = (char *)arg1;
    uint32_t size = arg2;
    if (!buffer || size == 0) {
      return -EINVAL;
    }
    if (shell_get_current_directory(buffer, size) != 0) {
      return -ERANGE;
    }
    return 0;
  }
  if (number == SYS_DUP) {
    int oldfd = (int)arg1;
    if (oldfd < 3 || oldfd >= SYSCALL_FD_COUNT || !descriptors[oldfd].used) {
      return -EBADF;
    }
    for (int fd = 3; fd < SYSCALL_FD_COUNT; fd++) {
      if (!descriptors[fd].used) {
        descriptors[fd] = descriptors[oldfd];
        descriptors[fd].file->references++;
        return fd;
      }
    }
    return -EMFILE;
  }
  if (number == SYS_DUP2) {
    int oldfd = (int)arg1;
    int newfd = (int)arg2;
    if (oldfd < 3 || oldfd >= SYSCALL_FD_COUNT || !descriptors[oldfd].used) {
      return -EBADF;
    }
    if (newfd < 3 || newfd >= SYSCALL_FD_COUNT) {
      return -EBADF;
    }
    if (oldfd == newfd) {
      return newfd;
    }
    if (descriptors[newfd].used) {
      descriptors[newfd].file->references--;
      descriptors[newfd].used = 0;
      descriptors[newfd].file = NULL;
    }
    descriptors[newfd] = descriptors[oldfd];
    descriptors[newfd].file->references++;
    return newfd;
  }
  if (number == SYS_FCNTL) {
    int fd = (int)arg1;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used) {
      return -EBADF;
    }
    syscall_open_file_t *file = descriptors[fd].file;
    if (arg2 == F_GETFL) {
      uint32_t access_mode = file->readable
                                 ? (file->writable ? O_RDWR : O_RDONLY)
                                 : O_WRONLY;
      return (int32_t)(access_mode | (file->append ? O_APPEND : 0));
    }
    if (arg2 == F_SETFL) {
      if (arg3 & ~(O_ACCMODE | O_APPEND)) {
        return -EINVAL;
      }
      file->append = (arg3 & O_APPEND) != 0;
      return 0;
    }
    return -EINVAL;
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
  if (number == SYS_UMASK) {
    uint32_t previous = process_umask;
    process_umask = arg1 & 0777;
    return (int32_t)previous;
  }
  if (number == SYS_FTRUNCATE) {
    int fd = (int)arg1;
    if (fd < 3 || fd >= SYSCALL_FD_COUNT || !descriptors[fd].used ||
        !descriptors[fd].file->writable) {
      return -EBADF;
    }
    if ((int32_t)arg2 < 0) {
      return -EINVAL;
    }
    if (vfs_truncate(descriptors[fd].file->node, arg2) != 0) {
      return -EFBIG;
    }
    if (descriptors[fd].file->offset > arg2) {
      descriptors[fd].file->offset = arg2;
    }
    return 0;
  }

  return -ENOSYS;
}
