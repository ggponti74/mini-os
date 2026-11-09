#include <stddef.h>
#include <stdint.h>

#include "display.h"
#include "errno.h"
#include "fs/vfs.h"
#include "memory.h"
#include "memory_layout.h"
#include "sys/stat.h"
#include "sys/syscall.h"

#define CHECK(condition)                                                       \
  do {                                                                         \
    if (!(condition)) {                                                        \
      failures++;                                                              \
    }                                                                          \
  } while (0)

_Static_assert(SYS_APP_SBRK == 26 && SYS_APP_SBRK != SYS_SBRK,
               "application heap syscall number");

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
static vfs_node_t directory_nodes[8];
static uint32_t directory_count;
static int created_exists;
static uint8_t created_data[256];
static int keyboard_keys[8];
static uint32_t keyboard_key_count;
static uint32_t keyboard_key_index;
static uintptr_t heap_break = 0x1000;
static uint64_t test_timer_ticks;
static uint32_t test_rtc_epoch = 1791500000u;
static int test_rtc_error;

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
  file_node.mode = 0600;
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
  CHECK(syscall_dispatch(SYS_STAT, (uint32_t)"file",
                         (uint32_t)&status, 0) == 0);
  CHECK(S_ISREG(status.st_mode) && status.st_ino == 42 &&
        status.st_size == 3);
  CHECK(syscall_dispatch(SYS_STAT, (uint32_t)"dir",
                         (uint32_t)&status, 0) == 0);
  CHECK(S_ISDIR(status.st_mode) && status.st_ino == 7);
  CHECK(syscall_dispatch(SYS_STAT, (uint32_t)"missing",
                         (uint32_t)&status, 0) == -ENOENT);
  CHECK(syscall_dispatch(SYS_STAT, (uint32_t)"dir", 0, 0) == -EFAULT);
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
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_SET) == 0);
  int duplicate_fd = syscall_dispatch(SYS_DUP, fd, 0, 0);
  CHECK(duplicate_fd >= 3 && duplicate_fd != fd);
  CHECK(syscall_dispatch(SYS_READ, fd, (uint32_t)buffer, 1) == 1);
  CHECK(buffer[0] == 'a');
  CHECK(syscall_dispatch(SYS_READ, duplicate_fd, (uint32_t)buffer, 1) == 1);
  CHECK(buffer[0] == 'b');
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_CUR) == 2);
  CHECK(syscall_dispatch(SYS_CLOSE, duplicate_fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_SET) == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == -EBADF);
  CHECK(syscall_dispatch(SYS_LSEEK, 1, 0, SEEK_SET) == -ESPIPE);
}

static void test_open_flags_and_errors(void) {
  int fd;
  uint8_t read_probe = 0;

  reset_file();
  file_node.mode = 0400;
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_WRONLY, 0) ==
        -EACCES);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR, 0) == -EACCES);
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDONLY, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  reset_file();
  file_node.mode = 0200;
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDONLY, 0) ==
        -EACCES);
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_WRONLY, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

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
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_GETFL, 0) ==
        (O_WRONLY | O_APPEND));
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_SETFL,
                         O_RDWR | O_APPEND) == 0);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_GETFL, 0) ==
        (O_WRONLY | O_APPEND));
  CHECK(syscall_dispatch(SYS_READ, fd, (uint32_t)&read_probe, 1) == -EBADF);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_SETFL, O_RDONLY) == 0);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_GETFL, 0) == O_WRONLY);
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_SET) == 0);
  CHECK(syscall_dispatch(SYS_WRITE, fd, (uint32_t)"y", 1) == 1);
  CHECK(file_data[0] == 'y' && file_node.length == 4);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_SETFL,
                         O_RDWR | O_APPEND) == 0);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_GETFL, 0) ==
        (O_WRONLY | O_APPEND));
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_GETFD, 0) == 0);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_SETFD, FD_CLOEXEC) == 0);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_GETFD, 0) == FD_CLOEXEC);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_SETFD, 2) == -EINVAL);
  int minimum_dup = syscall_dispatch(SYS_FCNTL, fd, F_DUPFD, 5);
  CHECK(minimum_dup >= 5);
  CHECK(syscall_dispatch(SYS_FCNTL, minimum_dup, F_GETFD, 0) == 0);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_GETFD, 0) == FD_CLOEXEC);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_DUPFD, (uint32_t)-1) == -EINVAL);
  CHECK(syscall_dispatch(SYS_FCNTL, fd, F_DUPFD, 8) == -EMFILE);
  CHECK(syscall_dispatch(SYS_CLOSE, minimum_dup, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"created",
                        O_CREAT | O_EXCL | O_RDWR, 0640);
  CHECK(fd == 3);
  struct stat created_status;
  CHECK(syscall_dispatch(SYS_STAT, (uint32_t)"created",
                         (uint32_t)&created_status, 0) == 0);
  CHECK((created_status.st_mode & 0777) == 0640);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"created",
                         O_CREAT | O_EXCL | O_RDWR, 0) == -EEXIST);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"missing", O_RDONLY, 0) ==
        -ENOENT);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR | 0x8000, 0) ==
        -EINVAL);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_DIRECTORY, 0) ==
        -ENOTDIR);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"missing", O_DIRECTORY, 0) ==
        -ENOENT);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"missing",
                         O_DIRECTORY | O_CREAT | O_RDONLY, 0600) == -EINVAL);
  CHECK(vfs_lookup(fs_root, "missing") == NULL);
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"dir", O_DIRECTORY | O_RDONLY, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"dir", O_RDONLY, 0);
  CHECK(fd == 3);
  struct stat directory_status;
  CHECK(syscall_dispatch(SYS_FSTAT, fd, (uint32_t)&directory_status, 0) == 0);
  CHECK(S_ISDIR(directory_status.st_mode));
  CHECK(syscall_dispatch(SYS_READ, fd, (uint32_t)&read_probe, 1) == -EISDIR);
  CHECK(syscall_dispatch(SYS_WRITE, fd, (uint32_t)"x", 1) == -EBADF);
  CHECK(syscall_dispatch(SYS_FTRUNCATE, fd, 0, 0) == -EBADF);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_OPEN, (uint32_t)"dir", O_WRONLY, 0) ==
        -EISDIR);
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR | O_TRUNC, 0);
  CHECK(fd == 3);
  CHECK(file_node.length == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  reset_file();
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_FTRUNCATE, fd, (uint32_t)-1, 0) == -EINVAL);
  CHECK(syscall_dispatch(SYS_FTRUNCATE, fd, 257, 0) == -EFBIG);
  CHECK(syscall_dispatch(SYS_FTRUNCATE, fd, 5, 0) == 0);
  CHECK(file_node.length == 5);
  CHECK(syscall_dispatch(SYS_FTRUNCATE, fd, 3, 0) == 0);
  CHECK(file_node.length == 3);
  CHECK(syscall_dispatch(SYS_FTRUNCATE, fd, 0, 0) == 0);
  CHECK(file_node.length == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  CHECK(syscall_dispatch(SYS_MKDIR, (uint32_t)"newdir", 0750, 0) == 0);
  CHECK(vfs_lookup(fs_root, "newdir") != NULL);
  struct stat mkdir_status;
  CHECK(syscall_dispatch(SYS_STAT, (uint32_t)"newdir",
                         (uint32_t)&mkdir_status, 0) == 0);
  CHECK((mkdir_status.st_mode & 0777) == 0750);
  CHECK(syscall_dispatch(SYS_RMDIR, (uint32_t)"newdir", 0, 0) == 0);
  CHECK(vfs_lookup(fs_root, "newdir") == NULL);
  CHECK(syscall_dispatch(SYS_MKDIR, (uint32_t)"tmpdir", 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_UNLINK, (uint32_t)"tmpdir", 0, 0) == -EISDIR);
  CHECK(vfs_lookup(fs_root, "tmpdir") != NULL);
  CHECK(syscall_dispatch(SYS_RMDIR, (uint32_t)"tmpdir", 0, 0) == 0);

  CHECK(syscall_dispatch(SYS_ACCESS, (uint32_t)"file", F_OK, 0) == 0);
  CHECK(syscall_dispatch(SYS_ACCESS, (uint32_t)"file", R_OK | W_OK, 0) == 0);
  CHECK(syscall_dispatch(SYS_ACCESS, (uint32_t)"file", X_OK, 0) == -EACCES);
  CHECK(syscall_dispatch(SYS_ACCESS, (uint32_t)"file", 8, 0) == -EINVAL);
  CHECK(syscall_dispatch(SYS_ACCESS, (uint32_t)"missing", F_OK, 0) == -ENOENT);
  CHECK(syscall_dispatch(SYS_RENAME, (uint32_t)"file", (uint32_t)"renamed", 0) == 0);
  CHECK(vfs_lookup(fs_root, "renamed") != NULL);
  CHECK(vfs_lookup(fs_root, "file") == NULL);
  CHECK(syscall_dispatch(SYS_CHMOD, (uint32_t)"renamed", (uint32_t)0644, 0) == 0);
  struct stat chmod_status;
  CHECK(syscall_dispatch(SYS_STAT, (uint32_t)"renamed",
                         (uint32_t)&chmod_status, 0) == 0);
  CHECK((chmod_status.st_mode & 0777) == 0644);
  CHECK(syscall_dispatch(SYS_ACCESS, (uint32_t)"renamed", X_OK, 0) ==
        -EACCES);
  CHECK(syscall_dispatch(SYS_CHMOD, (uint32_t)"renamed", (uint32_t)0755, 0) ==
        0);
  CHECK(syscall_dispatch(SYS_ACCESS, (uint32_t)"renamed", X_OK, 0) == 0);
  int chmod_fd = syscall_dispatch(SYS_OPEN, (uint32_t)"renamed", O_RDONLY, 0);
  CHECK(chmod_fd >= 3);
  CHECK(syscall_dispatch(SYS_FCHMOD, chmod_fd, (uint32_t)0600, 0) == 0);
  CHECK(syscall_dispatch(SYS_FSTAT, chmod_fd, (uint32_t)&chmod_status, 0) ==
        0);
  CHECK((chmod_status.st_mode & 0777) == 0600);
  CHECK(syscall_dispatch(SYS_CLOSE, chmod_fd, 0, 0) == 0);
  int directory_fd = syscall_dispatch(SYS_OPEN, (uint32_t)"dir",
                                       O_RDONLY, 0);
  CHECK(directory_fd == 3);
  directory_node.mode = 0600;
  CHECK(syscall_dispatch(SYS_CHDIR, (uint32_t)"dir", 0, 0) == -EACCES);
  CHECK(syscall_dispatch(SYS_FCHDIR, directory_fd, 0, 0) == -EACCES);
  directory_node.mode = 0700;
  CHECK(syscall_dispatch(SYS_CHDIR, (uint32_t)"dir", 0, 0) == 0);
  char cwd[32] = {0};
  CHECK(syscall_dispatch(SYS_GETCWD, (uint32_t)cwd, sizeof(cwd), 0) == 0);
  CHECK(strings_equal(cwd, "/dir"));
  CHECK(syscall_dispatch(SYS_FCHDIR, directory_fd, 0, 0) == 0);
  char fchdir_cwd[32] = {0};
  CHECK(syscall_dispatch(SYS_GETCWD, (uint32_t)fchdir_cwd,
                         sizeof(fchdir_cwd), 0) == 0);
  CHECK(strings_equal(fchdir_cwd, "/dir"));
  CHECK(syscall_dispatch(SYS_FCHDIR, 0, 0, 0) == -EBADF);
  CHECK(syscall_dispatch(SYS_FCHDIR, 7, 0, 0) == -EBADF);
  int regular_fd = syscall_dispatch(SYS_OPEN, (uint32_t)"renamed",
                                    O_RDONLY, 0);
  CHECK(regular_fd >= 3);
  CHECK(syscall_dispatch(SYS_FCHDIR, regular_fd, 0, 0) == -ENOTDIR);
  CHECK(syscall_dispatch(SYS_CLOSE, regular_fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, directory_fd, 0, 0) == 0);
  char short_cwd[2] = {0};
  CHECK(syscall_dispatch(SYS_GETCWD, (uint32_t)short_cwd,
                         sizeof(short_cwd), 0) == -ERANGE);

  int original_fd = syscall_dispatch(SYS_OPEN, (uint32_t)"renamed", O_RDONLY, 0);
  CHECK(original_fd >= 3);
  int dup_fd = syscall_dispatch(SYS_DUP, original_fd, 0, 0);
  CHECK(dup_fd >= 3 && dup_fd != original_fd);
  CHECK(syscall_dispatch(SYS_FCNTL, dup_fd, F_GETFD, 0) == 0);
  CHECK(syscall_dispatch(SYS_FCNTL, original_fd, F_SETFD, FD_CLOEXEC) == 0);
  CHECK(syscall_dispatch(SYS_DUP2, original_fd, 4, 0) == 4);
  CHECK(syscall_dispatch(SYS_FCNTL, 4, F_GETFD, 0) == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, original_fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_DUP2, dup_fd, 3, 0) == 3);
  CHECK(syscall_dispatch(SYS_CLOSE, dup_fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, 3, 0, 0) == 0);
}

static void test_sync_calls(void) {
  int fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_FSYNC, fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_FSYNC, fd, 1, 0) == 0);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDONLY, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_FSYNC, fd, 0, 0) == -EBADF);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);
  CHECK(syscall_dispatch(SYS_FSYNC, 3, 0, 0) == -EBADF);
  CHECK(syscall_dispatch(SYS_FSYNC, 0, 0, 0) == -EBADF);
}

static void test_positional_io(void) {
  struct test_positional_io_request {
    int32_t offset;
    uint32_t count;
  } request;
  uint8_t buffer[4] = {0};
  int fd;

  reset_file();
  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDWR, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 2, SEEK_SET) == 2);
  request.offset = 0;
  request.count = 2;
  CHECK(syscall_dispatch(SYS_PREAD, fd, (uint32_t)buffer,
                         (uint32_t)&request) == 2);
  CHECK(buffer[0] == 'a' && buffer[1] == 'b');
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_CUR) == 2);

  request.offset = 1;
  request.count = 2;
  CHECK(syscall_dispatch(SYS_PWRITE, fd, (uint32_t)"XY",
                         (uint32_t)&request) == 2);
  CHECK(file_data[0] == 'a' && file_data[1] == 'X' &&
        file_data[2] == 'Y');
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_CUR) == 2);
  CHECK(syscall_dispatch(SYS_PREAD, fd, (uint32_t)buffer,
                         (uint32_t)&request) == 2);
  CHECK(buffer[0] == 'X' && buffer[1] == 'Y');

  request.offset = -1;
  CHECK(syscall_dispatch(SYS_PREAD, fd, (uint32_t)buffer,
                         (uint32_t)&request) == -EINVAL);
  request.offset = 0;
  request.count = 1;
  CHECK(syscall_dispatch(SYS_PREAD, 7, (uint32_t)buffer,
                         (uint32_t)&request) == -EBADF);
  CHECK(syscall_dispatch(SYS_PREAD, fd, (uint32_t)buffer, 0) == -EFAULT);
  CHECK(syscall_dispatch(SYS_PREAD, fd, 0,
                         (uint32_t)&request) == -EFAULT);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file", O_RDONLY, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_PWRITE, fd, (uint32_t)"Z",
                         (uint32_t)&request) == -EBADF);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);

  fd = syscall_dispatch(SYS_OPEN, (uint32_t)"file",
                        O_RDWR | O_APPEND, 0);
  CHECK(fd == 3);
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 2, SEEK_SET) == 2);
  request.offset = 0;
  request.count = 1;
  CHECK(syscall_dispatch(SYS_PWRITE, fd, (uint32_t)"Z",
                         (uint32_t)&request) == 1);
  CHECK(file_data[0] == 'Z');
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_CUR) == 2);
  request.offset = 5;
  request.count = 1;
  CHECK(syscall_dispatch(SYS_PWRITE, fd, (uint32_t)"Q",
                         (uint32_t)&request) == 1);
  CHECK(file_node.length == 6 && file_data[3] == 0 &&
        file_data[4] == 0 && file_data[5] == 'Q');
  CHECK(syscall_dispatch(SYS_LSEEK, fd, 0, SEEK_CUR) == 2);
  CHECK(syscall_dispatch(SYS_CLOSE, fd, 0, 0) == 0);
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
  CHECK(heap_break == 0x1010);
  CHECK(syscall_dispatch(0x7fffffff, 0, 0, 0) == -ENOSYS);
}

static void test_app_sbrk(void) {
  const uint32_t base = USER_APP_LOAD_BASE + 0x1000;
  const uint32_t next_base = USER_APP_LOAD_BASE + 0x2000;

  syscall_reset_app_heap();
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, USER_APP_LOAD_BASE - 1, USER_APP_HEAP_LIMIT) ==
        -EINVAL);
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, base, USER_APP_HEAP_LIMIT - 1) ==
        -EINVAL);
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, base, USER_APP_HEAP_LIMIT) ==
        (int32_t)base);
  CHECK(syscall_dispatch(SYS_APP_SBRK, 16, base, USER_APP_HEAP_LIMIT) ==
        (int32_t)base);
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, base, USER_APP_HEAP_LIMIT) ==
        (int32_t)(base + 16));
  CHECK(syscall_dispatch(SYS_APP_SBRK, -8, base, USER_APP_HEAP_LIMIT) ==
        (int32_t)(base + 16));
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, base, USER_APP_HEAP_LIMIT) ==
        (int32_t)(base + 8));
  CHECK(syscall_dispatch(SYS_APP_SBRK, -9, base, USER_APP_HEAP_LIMIT) ==
        -ENOMEM);
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0x7fffffff, base,
                         USER_APP_HEAP_LIMIT) == -ENOMEM);
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, base, USER_APP_HEAP_LIMIT) ==
        (int32_t)(base + 8));
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, base + 1, USER_APP_HEAP_LIMIT) ==
        -EINVAL);
  CHECK(heap_break == 0x1010);

  syscall_reset_app_heap();
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, USER_APP_HEAP_LIMIT,
                         USER_APP_HEAP_LIMIT) ==
        (int32_t)USER_APP_HEAP_LIMIT);
  CHECK(syscall_dispatch(SYS_APP_SBRK, 1, USER_APP_HEAP_LIMIT,
                         USER_APP_HEAP_LIMIT) == -ENOMEM);
  syscall_reset_app_heap();
  CHECK(syscall_dispatch(SYS_APP_SBRK, 0, next_base,
                         USER_APP_HEAP_LIMIT) == (int32_t)next_base);
  syscall_reset_app_heap();
}

static int32_t test_main(void) {
  reset_file();
  directory_node.flags = FS_DIRECTORY;
  directory_node.mode = 0700;
  directory_node.inode = 7;
  created_exists = 0;
  keyboard_key_count = 0;
  syscall_dispatch(SYS_UMASK, 0022, 0, 0);
  test_timer_ticks = 1234;

  test_open_read_seek_and_stat();
  test_sync_calls();
  test_positional_io();
  test_open_flags_and_errors();
  CHECK(syscall_dispatch(SYS_UMASK, 0077, 0, 0) == 0022);
  CHECK(syscall_dispatch(SYS_MKDIR, (uint32_t)"masked-dir", 0777, 0) == 0);
  struct stat masked_status;
  CHECK(syscall_dispatch(SYS_STAT, (uint32_t)"masked-dir",
                         (uint32_t)&masked_status, 0) == 0);
  CHECK((masked_status.st_mode & 0777) == 0700);
  CHECK(syscall_dispatch(SYS_UMASK, 0022, 0, 0) == 0077);
  CHECK(syscall_dispatch(SYS_RMDIR, (uint32_t)"masked-dir", 0, 0) == 0);
  struct test_timespec { int32_t tv_sec; int32_t tv_nsec; }
      monotonic_time, request = {0, 1500001}, remaining = {-1, -1};
  CHECK(syscall_dispatch(SYS_CLOCK_GETTIME, CLOCK_MONOTONIC,
                         (uint32_t)&monotonic_time, 0) == 0);
  CHECK(monotonic_time.tv_sec == 1 && monotonic_time.tv_nsec == 234000000);
  CHECK(syscall_dispatch(SYS_CLOCK_GETTIME, CLOCK_REALTIME,
                         (uint32_t)&monotonic_time, 0) == 0);
  CHECK(monotonic_time.tv_sec == 1791500000 &&
        monotonic_time.tv_nsec == 0);
  struct test_timeval { int32_t tv_sec; int32_t tv_usec; } wall_time;
  CHECK(syscall_dispatch(SYS_GETTIMEOFDAY, (uint32_t)&wall_time, 0, 0) == 0);
  CHECK(wall_time.tv_sec == 1791500000 && wall_time.tv_usec == 0);
  CHECK(syscall_dispatch(SYS_GETTIMEOFDAY, 0, 0, 0) == -EFAULT);
  test_rtc_error = 1;
  CHECK(syscall_dispatch(SYS_CLOCK_GETTIME, CLOCK_REALTIME,
                         (uint32_t)&monotonic_time, 0) == -EIO);
  CHECK(syscall_dispatch(SYS_GETTIMEOFDAY, (uint32_t)&wall_time, 0, 0) == -EIO);
  test_rtc_error = 0;
  test_rtc_epoch = 0x80000000u;
  CHECK(syscall_dispatch(SYS_CLOCK_GETTIME, CLOCK_REALTIME,
                         (uint32_t)&monotonic_time, 0) == -EOVERFLOW);
  CHECK(syscall_dispatch(SYS_GETTIMEOFDAY, (uint32_t)&wall_time, 0, 0) == -EOVERFLOW);
  test_rtc_epoch = 1791500000u;
  CHECK(syscall_dispatch(SYS_CLOCK_GETTIME, 99,
                         (uint32_t)&monotonic_time, 0) == -EINVAL);
  CHECK(syscall_dispatch(SYS_CLOCK_GETTIME, CLOCK_MONOTONIC, 0, 0) == -EFAULT);
  CHECK(syscall_dispatch(SYS_NANOSLEEP, (uint32_t)&request,
                         (uint32_t)&remaining, 0) == 0);
  CHECK(test_timer_ticks == 1236);
  CHECK(remaining.tv_sec == 0 && remaining.tv_nsec == 0);
  request.tv_nsec = 1000000000;
  CHECK(syscall_dispatch(SYS_NANOSLEEP, (uint32_t)&request, 0, 0) == -EINVAL);
  request.tv_sec = -1; request.tv_nsec = 0;
  CHECK(syscall_dispatch(SYS_NANOSLEEP, (uint32_t)&request, 0, 0) == -EINVAL);
  CHECK(syscall_dispatch(SYS_NANOSLEEP, 0, 0, 0) == -EFAULT);
  test_standard_input_and_sbrk();
  test_app_sbrk();

  return failures;
}

void _start(void) {
  int result = test_main();
  __asm__ volatile("int $0x80" : : "a"(1), "b"(result) : "memory");
  __builtin_unreachable();
}

uint64_t hal_timer_get_ticks(void) { return test_timer_ticks; }
void hal_sleep(uint32_t milliseconds) { test_timer_ticks += milliseconds; }
int rtc_get_epoch(uint32_t *epoch) {
  if (!epoch || test_rtc_error) return -1;
  *epoch = test_rtc_epoch;
  return 0;
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

static char test_current_directory[128];

int shell_set_current_directory(const char *path) {
  uint32_t i = 0;
  if (!path) {
    return -1;
  }
  while (path[i]) {
    if (i + 1 >= sizeof(test_current_directory)) {
      return -1;
    }
    test_current_directory[i] = path[i];
    i++;
  }
  test_current_directory[i] = '\0';
  return 0;
}

int shell_get_current_directory(char *buffer, uint32_t size) {
  uint32_t length = 0;
  if (!buffer || size < 2) {
    return -1;
  }
  buffer[length++] = '/';
  for (uint32_t i = 0; test_current_directory[i]; i++) {
    if (length + 1 >= size) {
      return -1;
    }
    buffer[length++] = test_current_directory[i];
  }
  buffer[length] = '\0';
  return 0;
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
  if (strings_equal(path, file_node.name) && file_node.flags != 0) {
    return &file_node;
  }
  if (strings_equal(path, "file") && file_node.flags != 0 && file_node.name[0] == '\0') {
    return &file_node;
  }
  if (strings_equal(path, "dir") && directory_node.flags != 0) {
    return &directory_node;
  }
  if (strings_equal(path, "created") && created_exists) {
    return &created_node;
  }
  for (uint32_t i = 0; i < directory_count; i++) {
    if (directory_nodes[i].flags != 0 && strings_equal(path, directory_nodes[i].name)) {
      return &directory_nodes[i];
    }
  }
  return NULL;
}

vfs_node_t *vfs_create(vfs_node_t *parent, const char *name, uint32_t flags) {
  (void)parent;
  if (flags == FS_FILE) {
    if (!strings_equal(name, "created") || created_exists) {
      return NULL;
    }
    created_exists = 1;
    created_node.flags = FS_FILE;
    created_node.mode = 0600;
    created_node.length = 0;
    created_node.inode = 99;
    created_node.device_data = created_data;
    return &created_node;
  }
  if (flags == FS_DIRECTORY) {
    for (uint32_t i = 0; i < directory_count; i++) {
      if (directory_nodes[i].flags != 0 &&
          strings_equal(name, directory_nodes[i].name)) {
        return NULL;
      }
    }
    if (directory_count >= sizeof(directory_nodes) / sizeof(directory_nodes[0])) {
      return NULL;
    }
    vfs_node_t *node = &directory_nodes[directory_count++];
    for (size_t i = 0; i < sizeof(node->name); i++) {
      node->name[i] = 0;
    }
    for (size_t i = 0; name[i] != '\0' && i < sizeof(node->name) - 1; i++) {
      node->name[i] = name[i];
    }
    node->flags = FS_DIRECTORY;
    node->length = 0;
    node->inode = 200 + directory_count;
    node->device_data = NULL;
    return node;
  }
  return NULL;
}

int initrd_move_file(const char *old_name, const char *new_name) {
  if (!old_name || !new_name || !*new_name) {
    return -1;
  }
  if (strings_equal(old_name, "file") && !strings_equal(new_name, "file")) {
    file_node.name[0] = '\0';
    for (size_t i = 0; new_name[i] != '\0' && i < sizeof(file_node.name) - 1; i++) {
      file_node.name[i] = new_name[i];
    }
    return 0;
  }
  return -1;
}

int initrd_remove_file(const char *filename) {
  if (!filename) {
    return -1;
  }
  if (strings_equal(filename, "file")) {
    file_node.flags = 0;
    file_node.length = 0;
    file_node.inode = 0;
    return 0;
  }
  if (strings_equal(filename, "created") && created_exists) {
    created_node.flags = 0;
    created_node.length = 0;
    created_node.inode = 0;
    created_exists = 0;
    return 0;
  }
  return -1;
}

int initrd_remove_directory(const char *dirname) {
  if (!dirname) {
    return -1;
  }
  for (uint32_t i = 0; i < directory_count; i++) {
    if (directory_nodes[i].flags != 0 && strings_equal(dirname, directory_nodes[i].name)) {
      directory_nodes[i].flags = 0;
      directory_nodes[i].name[0] = '\0';
      directory_nodes[i].length = 0;
      directory_nodes[i].inode = 0;
      return 0;
    }
  }
  return -1;
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

int vfs_truncate(vfs_node_t *node, uint32_t length) {
  if (!node || length > sizeof(file_data)) {
    return -1;
  }
  uint8_t *data = node->device_data;
  for (uint32_t i = node->length; i < length; i++) {
    data[i] = 0;
  }
  node->length = length;
  return 0;
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
  for (uint32_t i = node->length; i < offset; i++) {
    data[i] = 0;
  }
  for (uint32_t i = 0; i < size; i++) {
    data[offset + i] = buffer[i];
  }
  if (offset + size > node->length) {
    node->length = offset + size;
  }
  return size;
}
