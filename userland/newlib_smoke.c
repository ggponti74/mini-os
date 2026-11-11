#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <unistd.h>

static int test_sbrk(void) {
  char *initial_break = sbrk(0);
  if (initial_break == (void *)-1) {
    return 0;
  }

  if (sbrk(16) != initial_break || sbrk(0) != initial_break + 16 ||
      sbrk(-16) != initial_break + 16 || sbrk(0) != initial_break) {
    return 0;
  }

  errno = 0;
  if (sbrk((ptrdiff_t)0x7fffffff) != (void *)-1 || errno != ENOMEM) {
    return 0;
  }
  return sbrk(0) == initial_break;
}

_Static_assert(O_APPEND == 0x0008 && O_CREAT == 0x0200 &&
                   O_TRUNC == 0x0400 && O_EXCL == 0x0800 &&
                   O_DIRECTORY == 0x10000,
               "Newlib open flag ABI");
_Static_assert(ENOSYS == 88 && ENAMETOOLONG == 91 && EOVERFLOW == 139,
               "Newlib errno ABI");
_Static_assert(sizeof(struct stat) == 72 &&
                   offsetof(struct stat, st_size) == 16 &&
                   offsetof(struct stat, st_atim) == 20 &&
                   offsetof(struct stat, st_blksize) == 56 &&
                   offsetof(struct stat, st_blocks) == 60,
               "Newlib i686 stat ABI");

extern int _getpid(void);
extern uid_t _getuid(void);
extern uid_t _geteuid(void);
extern gid_t _getgid(void);
extern gid_t _getegid(void);
extern int _gethostname(char *name, size_t length);
extern int _truncate(const char *path, off_t length);
extern int _kill(int pid, int signal);
extern int _fork(void);
extern int _execve(const char *pathname, char *const argv[], char *const envp[]);
extern int _wait(int *status);
extern int _gettimeofday(struct timeval *time, void *timezone);
extern clock_t _times(struct tms *buffer);
extern int _link(const char *existing, const char *newpath);

int main(int argc, char **argv) {
  const char payload[] = "newlib";
  char readback[sizeof(payload)] = {0};
  struct stat status;
  struct timespec monotonic_before, monotonic_after, sleep_request = {0, 2000000};

  errno = 123;
  if (errno != 123) {
    printf("errno storage check failed\n");
    fflush(stdout);
    return 18;
  }
  if (!isatty(0) || !isatty(1) || !isatty(2)) {
    printf("standard stream isatty check failed: %d\n", errno);
    fflush(stdout);
    return 32;
  }
  if (fcntl(0, F_GETFD) != 0 || fcntl(1, F_GETFD) != 0 ||
      fcntl(2, F_GETFD) != 0 ||
      fcntl(0, F_GETFL) != O_RDONLY ||
      fcntl(1, F_GETFL) != O_WRONLY ||
      fcntl(2, F_GETFL) != O_WRONLY ||
      fcntl(1, F_SETFD, FD_CLOEXEC) != 0 ||
      fcntl(1, F_GETFD) != FD_CLOEXEC ||
      fcntl(1, F_SETFD, 0) != 0 ||
      fcntl(1, F_GETFD) != 0) {
    printf("standard descriptor fcntl flag/status check failed: %d\\n", errno);
    fflush(stdout);
    return 125;
  }

  errno = 0;
  if (fcntl(0, F_SETFD, FD_CLOEXEC | 2) != -1 || errno != EINVAL) {
    printf("stdin invalid descriptor-flag check failed: %d\\n", errno);
    fflush(stdout);
    return 126;
  }
  errno = 0;
  if (fcntl(1, F_SETFD, FD_CLOEXEC | 2) != -1 || errno != EINVAL) {
    printf("stdout invalid descriptor-flag check failed: %d\\n", errno);
    fflush(stdout);
    return 127;
  }
  errno = 0;
  if (fcntl(2, F_SETFD, FD_CLOEXEC | 2) != -1 || errno != EINVAL) {
    printf("stderr invalid descriptor-flag check failed: %d\\n", errno);
    fflush(stdout);
    return 128;
  }
  errno = 0;
  if (fcntl(1, F_SETFL, O_APPEND) != -1 || errno != EINVAL) {
    printf("standard stream F_SETFL rejection check failed: %d\\n", errno);
    fflush(stdout);
    return 129;
  }
  errno = 0;
  if (fcntl(0, F_SETFL, O_APPEND) != -1 || errno != EINVAL) {
    printf("stdin F_SETFL rejection check failed: %d\\n", errno);
    fflush(stdout);
    return 132;
  }
  errno = 0;
  if (fcntl(2, F_SETFL, O_APPEND) != -1 || errno != EINVAL) {
    printf("stderr F_SETFL rejection check failed: %d\\n", errno);
    fflush(stdout);
    return 133;
  }
  errno = 0;
  if (fcntl(1, F_DUPFD, 3) != -1 || errno != EINVAL) {
    printf("standard stream F_DUPFD rejection check failed: %d\\n", errno);
    fflush(stdout);
    return 130;
  }
  errno = 0;
  if (fcntl(0, F_DUPFD, 3) != -1 || errno != EINVAL) {
    printf("stdin F_DUPFD rejection check failed: %d\\n", errno);
    fflush(stdout);
    return 134;
  }
  errno = 0;
  if (fcntl(2, F_DUPFD, 3) != -1 || errno != EINVAL) {
    printf("stderr F_DUPFD rejection check failed: %d\\n", errno);
    fflush(stdout);
    return 135;
  }
  errno = 0;
  if (fcntl(1, F_DUPFD_CLOEXEC, 3) != -1 || errno != EINVAL) {
    printf("standard stream F_DUPFD_CLOEXEC rejection check failed: %d\\n",
           errno);
    fflush(stdout);
    return 131;
  }
  errno = 0;
  if (fcntl(0, F_DUPFD_CLOEXEC, 3) != -1 || errno != EINVAL) {
    printf("stdin F_DUPFD_CLOEXEC rejection check failed: %d\\n", errno);
    fflush(stdout);
    return 136;
  }
  errno = 0;
  if (fcntl(2, F_DUPFD_CLOEXEC, 3) != -1 || errno != EINVAL) {
    printf("stderr F_DUPFD_CLOEXEC rejection check failed: %d\\n", errno);
    fflush(stdout);
    return 137;
  }

  errno = 0;
  if (!test_sbrk()) {
    printf("sbrk check failed: %d\n", errno);
    fflush(stdout);
    return 1;
  }

  char cwd[2];
  if (chdir("/") != 0 || getcwd(cwd, sizeof(cwd)) != cwd ||
      strcmp(cwd, "/") != 0) {
    printf("root working directory check failed: %d\n", errno);
    fflush(stdout);
    return 19;
  }
  errno = 0;
  if (getcwd(cwd, 1) != NULL || errno != ERANGE) {
    printf("short getcwd buffer check failed: %d\n", errno);
    fflush(stdout);
    return 20;
  }

  struct stat directory_status;
  if (mkdir("newlib-cwd-test", 0700) != 0 ||
      stat("newlib-cwd-test", &directory_status) != 0 ||
      !S_ISDIR(directory_status.st_mode)) {
    printf("stat directory check failed: %d\n", errno);
    fflush(stdout);
    return 24;
  }
  if (access("newlib-cwd-test", F_OK) != 0) {
    printf("access existence check failed: %d\n", errno);
    fflush(stdout);
    return 25;
  }
  errno = 0;
  if (access("newlib-cwd-test", X_OK) == 0 || errno != EACCES) {
    printf("access execute-permission check failed: %d\n", errno);
    fflush(stdout);
    return 26;
  }

  struct stat at_status;
  if (fstatat(AT_FDCWD, "newlib-cwd-test", &at_status, 0) != 0 ||
      !S_ISDIR(at_status.st_mode) ||
      faccessat(AT_FDCWD, "newlib-cwd-test", F_OK, 0) != 0) {
    printf("AT_FDCWD path-operation check failed: %d\n", errno);
    fflush(stdout);
    return 71;
  }
  errno = 0;
  if (faccessat(AT_FDCWD, "newlib-cwd-test", X_OK, 0) != -1 ||
      errno != EACCES) {
    printf("faccessat permission check failed: %d\n", errno);
    fflush(stdout);
    return 72;
  }
  errno = 0;
  if (fstatat(3, "newlib-cwd-test", &at_status, 0) != -1 ||
      errno != EBADF) {
    printf("fstatat directory-descriptor check failed: %d\n", errno);
    fflush(stdout);
    return 73;
  }
  errno = 0;
  if (fstatat(AT_FDCWD, "newlib-cwd-test", &at_status, 1) != -1 ||
      errno != EINVAL) {
    printf("fstatat flags check failed: %d\n", errno);
    fflush(stdout);
    return 176;
  }
  errno = 0;
  if (fstatat(AT_FDCWD, "newlib-cwd-test", NULL, 0) != -1 ||
      errno != EFAULT) {
    printf("fstatat null status check failed: %d\\n", errno);
    fflush(stdout);
    return 177;
  }
  errno = 0;
  if (fstatat(AT_FDCWD, NULL, &at_status, 0) != -1 || errno != EFAULT) {
    printf("fstatat null path check failed: %d\\n", errno);
    fflush(stdout);
    return 178;
  }
  errno = 0;
  if (fstatat(AT_FDCWD, "", &at_status, 0) != -1 || errno != ENOENT) {
    printf("fstatat empty path check failed: %d\\n", errno);
    fflush(stdout);
    return 179;
  }
  errno = 0;
  if (faccessat(AT_FDCWD, "", F_OK, 0) != -1 || errno != ENOENT) {
    printf("faccessat empty path check failed: %d\\n", errno);
    fflush(stdout);
    return 180;
  }
  errno = 0;
  if (faccessat(AT_FDCWD, "newlib-cwd-test", F_OK, 1) != -1 ||
      errno != EINVAL) {
    printf("faccessat flags check failed: %d\n", errno);
    fflush(stdout);
    return 74;
  }
  if (mkdirat(AT_FDCWD, "newlib-at-create", 0700) != 0 ||
      fchmodat(AT_FDCWD, "newlib-at-create", 0500, 0) != 0 ||
      stat("newlib-at-create", &at_status) != 0 ||
      (at_status.st_mode & 0777) != 0500) {
    printf("mkdirat/fchmodat mode check failed: %d\n", errno);
    fflush(stdout);
    return 75;
  }
  errno = 0;
  if (mkdirat(3, "newlib-at-invalid", 0700) != -1 || errno != EBADF) {
    printf("mkdirat directory-descriptor check failed: %d\n", errno);
    fflush(stdout);
    return 76;
  }
  errno = 0;
  if (fchmodat(3, "newlib-at-create", 0700, 0) != -1 || errno != EBADF) {
    printf("fchmodat directory-descriptor check failed: %d\n", errno);
    fflush(stdout);
    return 77;
  }
  errno = 0;
  if (fchmodat(AT_FDCWD, "newlib-at-create", 0700, 1) != -1 ||
      errno != EINVAL) {
    printf("fchmodat flags check failed: %d\n", errno);
    fflush(stdout);
    return 78;
  }
  if (fchmodat(AT_FDCWD, "newlib-at-create", 0700, 0) != 0) {
    printf("newlib at-file cleanup failed: %d\n", errno);
    fflush(stdout);
    return 79;
  }

  int at_rename_fd = open("newlib-at-rename-old",
                          O_CREAT | O_EXCL | O_WRONLY, 0600);
  if (at_rename_fd < 0 || close(at_rename_fd) != 0 ||
      renameat(AT_FDCWD, "newlib-at-rename-old",
               AT_FDCWD, "newlib-at-rename-new") != 0 ||
      access("newlib-at-rename-new", F_OK) != 0) {
    printf("renameat path-operation check failed: %d\n", errno);
    fflush(stdout);
    if (at_rename_fd >= 0) close(at_rename_fd);
    unlink("newlib-at-rename-old");
    unlink("newlib-at-rename-new");
    return 80;
  }
  errno = 0;
  if (renameat(3, "newlib-at-rename-new", AT_FDCWD,
               "newlib-at-rename-invalid") != -1 || errno != EBADF) {
    printf("renameat directory-descriptor check failed: %d\n", errno);
    fflush(stdout);
    unlink("newlib-at-rename-new");
    return 81;
  }
  if (unlinkat(AT_FDCWD, "newlib-at-rename-new", 0) != 0) {
    printf("unlinkat file removal check failed: %d\n", errno);
    fflush(stdout);
    return 82;
  }
  errno = 0;
  if (unlinkat(AT_FDCWD, "newlib-at-rename-new", 1) != -1 ||
      errno != EINVAL) {
    printf("unlinkat flags check failed: %d\n", errno);
    fflush(stdout);
    return 83;
  }
  if (unlinkat(AT_FDCWD, "newlib-at-create", AT_REMOVEDIR) != 0) {
    printf("unlinkat directory removal check failed: %d\n", errno);
    fflush(stdout);
    return 84;
  }
  errno = 0;
  if (unlinkat(3, "newlib-at-create", AT_REMOVEDIR) != -1 ||
      errno != EBADF) {
    printf("unlinkat directory-descriptor check failed: %d\n", errno);
    fflush(stdout);
    return 85;
  }

  const char openat_payload[] = "openat";
  char openat_readback[sizeof(openat_payload)] = {0};
  int openat_fd = openat(AT_FDCWD, "newlib-openat.dat",
                         O_CREAT | O_EXCL | O_RDWR, 0640);
  int openat_ok =
      openat_fd >= 0 &&
      write(openat_fd, openat_payload, sizeof(openat_payload)) ==
          (int)sizeof(openat_payload) &&
      lseek(openat_fd, 0, SEEK_SET) == 0 &&
      read(openat_fd, openat_readback, sizeof(openat_readback)) ==
          (int)sizeof(openat_readback) &&
      memcmp(openat_payload, openat_readback, sizeof(openat_payload)) == 0 &&
      fstat(openat_fd, &at_status) == 0 &&
      (at_status.st_mode & 0777) == 0640;
  if (openat_fd >= 0 && close(openat_fd) != 0) {
    openat_ok = 0;
  }
  if (!openat_ok || unlinkat(AT_FDCWD, "newlib-openat.dat", 0) != 0) {
    printf("openat create/read/mode check failed: %d\n", errno);
    fflush(stdout);
    unlinkat(AT_FDCWD, "newlib-openat.dat", 0);
    return 86;
  }
  errno = 0;
  if (openat(3, "newlib-openat-invalid", O_RDONLY) != -1 ||
      errno != EBADF) {
    printf("openat directory-descriptor check failed: %d\n", errno);
    fflush(stdout);
    return 87;
  }

  if (chdir("newlib-cwd-test") != 0) {
    printf("nested chdir check failed: %d\n", errno);
    fflush(stdout);
    return 21;
  }
  char nested_cwd[32] = {0};
  if (getcwd(nested_cwd, sizeof(nested_cwd)) != nested_cwd ||
      strcmp(nested_cwd, "/newlib-cwd-test") != 0) {
    printf("nested getcwd check failed: %s (%d)\n",
           nested_cwd, errno);
    fflush(stdout);
    return 22;
  }
  errno = 0;
  if (open("/newlib-cwd-test", O_DIRECTORY | O_CREAT | O_RDONLY, 0700) != -1 ||
      errno != EINVAL) {
    printf("O_DIRECTORY/O_CREAT validation check failed: %d\n", errno);
    fflush(stdout);
    return 57;
  }
  errno = 0;
  if (open("/newlib-cwd-test", O_DIRECTORY | O_WRONLY) != -1 ||
      errno != EISDIR) {
    printf("writable O_DIRECTORY open check failed: %d\n", errno);
    fflush(stdout);
    return 58;
  }
  errno = 0;
  if (open("/newlib-cwd-test/does-not-exist", O_DIRECTORY | O_RDONLY) != -1 ||
      errno != ENOENT) {
    printf("O_DIRECTORY missing-path check failed: %d\n", errno);
    fflush(stdout);
    return 61;
  }
  int directory_fd = open("/newlib-cwd-test", O_DIRECTORY | O_RDONLY);
  if (directory_fd < 0 || chdir("/") != 0 ||
      chmod("/newlib-cwd-test", 0600) != 0) {
    printf("directory search-permission setup failed: %d\n", errno);
    fflush(stdout);
    if (directory_fd >= 0) close(directory_fd);
    return 62;
  }
  errno = 0;
  if (chdir("/newlib-cwd-test") != -1 || errno != EACCES) {
    printf("chdir search-permission check failed: %d\n", errno);
    fflush(stdout);
    chmod("/newlib-cwd-test", 0700);
    close(directory_fd);
    return 63;
  }
  errno = 0;
  if (fchdir(directory_fd) != -1 || errno != EACCES) {
    printf("fchdir search-permission check failed: %d\n", errno);
    fflush(stdout);
    chmod("/newlib-cwd-test", 0700);
    close(directory_fd);
    return 64;
  }
  if (chmod("/newlib-cwd-test", 0700) != 0 ||
      fchdir(directory_fd) != 0 ||
      getcwd(nested_cwd, sizeof(nested_cwd)) != nested_cwd ||
      strcmp(nested_cwd, "/newlib-cwd-test") != 0) {
    printf("directory descriptor fchdir check failed: %d\n", errno);
    fflush(stdout);
    if (directory_fd >= 0) close(directory_fd);
    return 52;
  }
  char directory_byte;
  errno = 0;
  if (read(directory_fd, &directory_byte, 1) != -1 || errno != EISDIR) {
    printf("directory read rejection check failed: %d\n", errno);
    fflush(stdout);
    close(directory_fd);
    return 54;
  }
  errno = 0;
  if (open("/newlib-cwd-test", O_WRONLY) != -1 || errno != EISDIR) {
    printf("writable directory open rejection check failed: %d\n", errno);
    fflush(stdout);
    close(directory_fd);
    return 55;
  }
  errno = 0;
  if (fchdir(0) != -1 || errno != EBADF) {
    printf("invalid fchdir descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(directory_fd);
    return 53;
  }
  if (close(directory_fd) != 0 || chdir("/") != 0 ||
      rmdir("newlib-cwd-test") != 0) {
    printf("return-to-root check failed: %d\n", errno);
    fflush(stdout);
    return 23;
  }

  if (_getpid() != 1 || getpid() != 1) {
    printf("getpid check failed\n");
    fflush(stdout);
    return 10;
  }
  if (_getuid() != 0 || getuid() != 0 ||
      _geteuid() != 0 || geteuid() != 0 ||
      _getgid() != 0 || getgid() != 0 ||
      _getegid() != 0 || getegid() != 0) {
    printf("user/group identity check failed\n");
    fflush(stdout);
    return 40;
  }
  char hostname[16];
  char short_hostname[4];
  errno = 0;
  if (_gethostname(hostname, sizeof(hostname)) != 0 ||
      strcmp(hostname, "mini-os") != 0 ||
      gethostname(hostname, sizeof(hostname)) != 0 ||
      strcmp(hostname, "mini-os") != 0) {
    printf("hostname check failed: %d\n", errno);
    fflush(stdout);
    return 41;
  }
  errno = 0;
  if (gethostname(short_hostname, sizeof(short_hostname)) != -1 ||
      errno != ENAMETOOLONG || strcmp(short_hostname, "min") != 0) {
    printf("short hostname check failed: %d\n", errno);
    fflush(stdout);
    return 42;
  }
  struct utsname system_identity;
  if (uname(&system_identity) != 0 ||
      strcmp(system_identity.sysname, "mini-os") != 0 ||
      strcmp(system_identity.nodename, "mini-os") != 0 ||
      strcmp(system_identity.release, "0") != 0 ||
      strcmp(system_identity.version, "development") != 0 ||
      strcmp(system_identity.machine, "i386") != 0) {
    printf("uname check failed: %d\n", errno);
    fflush(stdout);
    return 43;
  }
  errno = 0;
  if (uname(NULL) != -1 || errno != EFAULT) {
    printf("uname null-argument check failed: %d\n", errno);
    fflush(stdout);
    return 44;
  }
  if (sysconf(_SC_PAGESIZE) != 4096 ||
      getpagesize() != 4096) {
    printf("page-size query check failed: %d\n", errno);
    fflush(stdout);
    return 49;
  }
  errno = 0;
  if (sysconf(-1) != -1 || errno != EINVAL) {
    printf("invalid sysconf query check failed: %d\n", errno);
    fflush(stdout);
    return 50;
  }
#ifdef _SC_OPEN_MAX
  if (sysconf(_SC_OPEN_MAX) != 8 || getdtablesize() != 8) {
    printf("open-descriptor limit check failed: %d\n", errno);
    fflush(stdout);
    return 51;
  }
#endif

  errno = 0;
  if (_kill(1, 0) != -1 || errno != ENOSYS) {
    printf("kill check failed: %d\n", errno);
    fflush(stdout);
    return 11;
  }

  errno = 0;
  if (_fork() != -1 || errno != ENOSYS) {
    printf("fork check failed: %d\n", errno);
    fflush(stdout);
    return 12;
  }
  errno = 0;
  if (_execve("program", NULL, NULL) != -1 || errno != ENOSYS) {
    printf("execve check failed: %d\n", errno);
    fflush(stdout);
    return 13;
  }
  errno = 0;
  if (_wait(NULL) != -1 || errno != ENOSYS) {
    printf("wait check failed: %d\n", errno);
    fflush(stdout);
    return 14;
  }

  errno = 0;
  if (_link("newlib-smoke.dat", "newlib-smoke.link") != -1 ||
      errno != ENOSYS) {
    printf("link check failed: %d\n", errno);
    fflush(stdout);
    return 17;
  }

  struct timeval current_time;
  errno = 0;
  if (_gettimeofday(&current_time, NULL) != 0 ||
      current_time.tv_sec <= 0 || current_time.tv_usec < 0 ||
      current_time.tv_usec >= 1000000) {
    printf("gettimeofday check failed: %d\n", errno);
    fflush(stdout);
    return 15;
  }
  struct timespec realtime_time;
  if (clock_gettime(CLOCK_REALTIME, &realtime_time) != 0 ||
      realtime_time.tv_sec < current_time.tv_sec ||
      realtime_time.tv_sec > current_time.tv_sec + 1 ||
      realtime_time.tv_nsec != 0) {
    printf("realtime clock check failed: %d\n", errno);
    fflush(stdout);
    return 30;
  }
  if (gettimeofday(&current_time, NULL) != 0 ||
      current_time.tv_sec <= 0 || current_time.tv_usec < 0 ||
      current_time.tv_usec >= 1000000) {
    printf("gettimeofday wrapper check failed: %d\n", errno);
    fflush(stdout);
    return 31;
  }
  struct tms process_times;
  errno = 0;
  if (_times(&process_times) != (clock_t)-1 || errno != ENOSYS) {
    printf("times check failed: %d\n", errno);
    fflush(stdout);
    return 16;
  }

  if (clock_gettime(CLOCK_MONOTONIC, &monotonic_before) != 0 ||
      monotonic_before.tv_sec < 0 || monotonic_before.tv_nsec < 0 ||
      monotonic_before.tv_nsec >= 1000000000 ||
      nanosleep(&sleep_request, NULL) != 0 ||
      clock_gettime(CLOCK_MONOTONIC, &monotonic_after) != 0 ||
      (monotonic_after.tv_sec * 1000000000LL + monotonic_after.tv_nsec) <
          (monotonic_before.tv_sec * 1000000000LL + monotonic_before.tv_nsec + 2000000LL)) {
    printf("monotonic clock/sleep check failed: %d\n", errno);
    fflush(stdout);
    return 28;
  }
  struct timespec invalid_sleep = {0, 1000000000};
  errno = 0;
  if (nanosleep(&invalid_sleep, NULL) != -1 || errno != EINVAL) {
    printf("nanosleep validation check failed: %d\n", errno);
    fflush(stdout);
    return 29;
  }

  const char truncate_payload[] = "truncate";
  int truncate_fd = open("newlib-truncate.dat", O_CREAT | O_TRUNC | O_RDWR, 0600);
  if (truncate_fd < 0 ||
      write(truncate_fd, truncate_payload, sizeof(truncate_payload)) !=
          (int)sizeof(truncate_payload) ||
      close(truncate_fd) != 0 ||
      _truncate("newlib-truncate.dat", 4) != 0 ||
      stat("newlib-truncate.dat", &status) != 0 || status.st_size != 4 ||
      truncate("newlib-truncate.dat", 8) != 0) {
    printf("path truncate check failed: %d\n", errno);
    fflush(stdout);
    if (truncate_fd >= 0) close(truncate_fd);
    unlink("newlib-truncate.dat");
    return 45;
  }
  truncate_fd = open("newlib-truncate.dat", O_RDONLY);
  char truncate_readback[8] = {1, 1, 1, 1, 1, 1, 1, 1};
  if (truncate_fd < 0 ||
      read(truncate_fd, truncate_readback, sizeof(truncate_readback)) !=
          (int)sizeof(truncate_readback) ||
      memcmp(truncate_readback, "trun", 4) != 0 ||
      truncate_readback[4] != 0 || truncate_readback[5] != 0 ||
      truncate_readback[6] != 0 || truncate_readback[7] != 0 ||
      close(truncate_fd) != 0 || unlink("newlib-truncate.dat") != 0) {
    printf("path truncate extension check failed: %d\n", errno);
    fflush(stdout);
    if (truncate_fd >= 0) close(truncate_fd);
    unlink("newlib-truncate.dat");
    return 46;
  }
  errno = 0;
  if (truncate("newlib-truncate.dat", -1) != -1 || errno != EINVAL) {
    printf("negative truncate length check failed: %d\n", errno);
    fflush(stdout);
    return 47;
  }
  errno = 0;
  if (truncate("newlib-truncate-missing.dat", 1) != -1 || errno != ENOENT) {
    printf("missing truncate path check failed: %d\n", errno);
    fflush(stdout);
    return 48;
  }

  mode_t previous_umask = umask(0077);
  int masked_fd = open("newlib-umask.dat", O_CREAT | O_EXCL | O_RDWR, 0666);
  if (masked_fd < 0 || fstat(masked_fd, &status) != 0 ||
      (status.st_mode & 0777) != 0600 || close(masked_fd) != 0 ||
      unlink("newlib-umask.dat") != 0) {
    printf("umask check failed: %d\n", errno);
    fflush(stdout);
    umask(previous_umask);
    return 27;
  }
  if (umask(previous_umask) != 0077) {
    printf("umask previous-mask return check failed: %d\\n", errno);
    fflush(stdout);
    return 176;
  }

  int chmod_test_fd = open("newlib-fchmod.dat", O_CREAT | O_EXCL | O_RDWR, 0644);
  if (chmod_test_fd < 0 || fchmod(chmod_test_fd, 0600) != 0 ||
      fstat(chmod_test_fd, &status) != 0 ||
      (status.st_mode & 0777) != 0600) {
    printf("fchmod permission update check failed: %d\n", errno);
    fflush(stdout);
    if (chmod_test_fd >= 0) close(chmod_test_fd);
    unlink("newlib-fchmod.dat");
    return 65;
  }
  errno = 0;
  if (fchmod(-1, 0644) != -1 || errno != EBADF) {
    printf("fchmod invalid descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(chmod_test_fd);
    unlink("newlib-fchmod.dat");
    return 66;
  }
  if (close(chmod_test_fd) != 0 || unlink("newlib-fchmod.dat") != 0) {
    printf("fchmod test cleanup failed: %d\n", errno);
    fflush(stdout);
    return 67;
  }

  int vector_fd = open("newlib-vector.dat", O_CREAT | O_TRUNC | O_RDWR, 0600);
  struct iovec write_vectors[2] = {
      {(void *)"New", 3}, {(void *)"lib", 3}};
  char vector_first[4] = {0};
  char vector_second[4] = {0};
  struct iovec read_vectors[2] = {
      {vector_first, 3}, {vector_second, 3}};
  if (vector_fd < 0 || writev(vector_fd, write_vectors, 2) != 6 ||
      lseek(vector_fd, 0, SEEK_SET) != 0 ||
      readv(vector_fd, read_vectors, 2) != 6 ||
      memcmp(vector_first, "New", 3) != 0 ||
      memcmp(vector_second, "lib", 3) != 0) {
    printf("Newlib vector I/O check failed: %d\n", errno);
    fflush(stdout);
    if (vector_fd >= 0) close(vector_fd);
    unlink("newlib-vector.dat");
    return 71;
  }
#ifdef _SC_IOV_MAX
  if (sysconf(_SC_IOV_MAX) != 16) {
    printf("Newlib vector I/O limit check failed: %d\n", errno);
    fflush(stdout);
    close(vector_fd);
    unlink("newlib-vector.dat");
    return 75;
  }
#endif
  errno = 0;
  if (readv(-1, read_vectors, 2) != -1 || errno != EBADF) {
    printf("Newlib readv invalid descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(vector_fd);
    unlink("newlib-vector.dat");
    return 72;
  }
  errno = 0;
  if (writev(vector_fd, NULL, 1) != -1 || errno != EFAULT) {
    printf("Newlib writev null vector check failed: %d\n", errno);
    fflush(stdout);
    close(vector_fd);
    unlink("newlib-vector.dat");
    return 73;
  }
  if (close(vector_fd) != 0 || unlink("newlib-vector.dat") != 0) {
    printf("Newlib vector I/O cleanup failed: %d\n", errno);
    fflush(stdout);
    return 74;
  }

  vector_fd = open("newlib-positional-vector.dat",
                    O_CREAT | O_TRUNC | O_RDWR, 0600);
  char positional_first[4] = {0};
  char positional_second[4] = {0};
  struct iovec positional_read_vectors[2] = {
      {positional_first, 3}, {positional_second, 3}};
  struct iovec positional_write_vectors[2] = {
      {(void *)"Pre", 3}, {(void *)"adv", 3}};
  if (vector_fd < 0 || writev(vector_fd, positional_write_vectors, 2) != 6 ||
      lseek(vector_fd, 4, SEEK_SET) != 4 ||
      preadv(vector_fd, positional_read_vectors, 2, 0) != 6 ||
      memcmp(positional_first, "Pre", 3) != 0 ||
      memcmp(positional_second, "adv", 3) != 0 ||
      lseek(vector_fd, 0, SEEK_CUR) != 4) {
    printf("Newlib preadv check failed: %d\n", errno);
    fflush(stdout);
    if (vector_fd >= 0) close(vector_fd);
    unlink("newlib-positional-vector.dat");
    return 76;
  }
  struct iovec positional_patch_vectors[2] = {
      {(void *)"P", 1}, {(void *)"W", 1}};
  if (pwritev(vector_fd, positional_patch_vectors, 2, 0) != 2 ||
      lseek(vector_fd, 0, SEEK_CUR) != 4) {
    printf("Newlib pwritev check failed: %d\n", errno);
    fflush(stdout);
    close(vector_fd);
    unlink("newlib-positional-vector.dat");
    return 77;
  }
  errno = 0;
  if (preadv(-1, positional_read_vectors, 2, 0) != -1 ||
      errno != EBADF || close(vector_fd) != 0 ||
      unlink("newlib-positional-vector.dat") != 0) {
    printf("Newlib positional vector cleanup/error check failed: %d\n",
           errno);
    fflush(stdout);
    if (vector_fd >= 0) close(vector_fd);
    unlink("newlib-positional-vector.dat");
    return 78;
  }

  char *allocation = malloc(128);
  if (!allocation) {
    printf("malloc failed: %d\n", errno);
    fflush(stdout);
    return 2;
  }
  strcpy(allocation, "Newlib malloc and stdio are available");
  printf("%s (%d arguments)\n", allocation, argc);

  char *expanded = realloc(allocation, 8192);
  if (!expanded || strcmp(expanded, "Newlib malloc and stdio are available") !=
                      0) {
    printf("realloc check failed: %d\n", errno);
    fflush(stdout);
    free(expanded ? expanded : allocation);
    return 3;
  }
  allocation = expanded;

  unsigned int *zeroed = calloc(64, sizeof(*zeroed));
  if (!zeroed) {
    printf("calloc failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 4;
  }
  for (unsigned int i = 0; i < 64; i++) {
    if (zeroed[i] != 0) {
      printf("calloc did not clear memory\n");
      fflush(stdout);
      free(zeroed);
      free(allocation);
      return 5;
    }
  }
  free(zeroed);

  int fd = open("newlib-smoke.dat", O_CREAT | O_TRUNC | O_RDWR, 0666);
  if (fd >= 0) {
    errno = 0;
    if (fchdir(fd) != -1 || errno != ENOTDIR) {
      printf("regular-file fchdir rejection check failed: %d\n", errno);
      fflush(stdout);
      close(fd);
      free(allocation);
      return 56;
    }
  }
  if (fd < 0) {
    printf("open failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 6;
  }
  errno = 0;
  if (isatty(fd) != 0 || errno != ENOTTY) {
    printf("regular-file isatty check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 33;
  }
  if (fsync(fd) != 0 || fdatasync(fd) != 0) {
    printf("file sync check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 37;
  }
  int readonly_fd = open("newlib-smoke.dat", O_RDONLY);
  if (readonly_fd < 0 || fcntl(readonly_fd, F_GETFL) != O_RDONLY) {
    printf("fcntl read-only access-mode check failed: %d\n", errno);
    fflush(stdout);
    if (readonly_fd >= 0) close(readonly_fd);
    close(fd);
    free(allocation);
    return 115;
  }
  int writeonly_fd = open("newlib-smoke.dat", O_WRONLY);
  if (writeonly_fd < 0 || fcntl(writeonly_fd, F_GETFL) != O_WRONLY ||
      close(writeonly_fd) != 0) {
    printf("fcntl write-only access-mode check failed: %d\n", errno);
    fflush(stdout);
    if (writeonly_fd >= 0) close(writeonly_fd);
    close(readonly_fd);
    close(fd);
    free(allocation);
    return 116;
  }
  int writeonly_append_fd = open("newlib-smoke.dat",
                                  O_WRONLY | O_APPEND);
  if (writeonly_append_fd < 0 ||
      fcntl(writeonly_append_fd, F_GETFL) != (O_WRONLY | O_APPEND) ||
      close(writeonly_append_fd) != 0) {
    printf("fcntl write-only append status check failed: %d\n", errno);
    fflush(stdout);
    if (writeonly_append_fd >= 0) close(writeonly_append_fd);
    close(readonly_fd);
    close(fd);
    free(allocation);
    return 161;
  }
  int writeonly_setfl_fd = open("newlib-smoke.dat", O_WRONLY);
  int writeonly_setfl_ok =
      writeonly_setfl_fd >= 0 &&
      fcntl(writeonly_setfl_fd, F_SETFL, O_WRONLY | O_APPEND) == 0 &&
      fcntl(writeonly_setfl_fd, F_GETFL) == (O_WRONLY | O_APPEND) &&
      fcntl(writeonly_setfl_fd, F_SETFL, O_WRONLY) == 0 &&
      fcntl(writeonly_setfl_fd, F_GETFL) == O_WRONLY;
  if (writeonly_setfl_fd >= 0 && close(writeonly_setfl_fd) != 0) {
    writeonly_setfl_ok = 0;
  }
  if (!writeonly_setfl_ok) {
    printf("fcntl write-only F_SETFL append toggle check failed: %d\n", errno);
    fflush(stdout);
    if (writeonly_setfl_fd >= 0) close(writeonly_setfl_fd);
    close(readonly_fd);
    close(fd);
    free(allocation);
    return 162;
  }
  int writeonly_access_fd = open("newlib-smoke.dat", O_WRONLY);
  int writeonly_access_ok =
      writeonly_access_fd >= 0 &&
      fcntl(writeonly_access_fd, F_SETFL, O_RDWR) == 0 &&
      fcntl(writeonly_access_fd, F_GETFL) == O_WRONLY &&
      fcntl(writeonly_access_fd, F_SETFL, O_RDONLY | O_APPEND) == 0 &&
      fcntl(writeonly_access_fd, F_GETFL) == (O_WRONLY | O_APPEND) &&
      fcntl(writeonly_access_fd, F_SETFL, O_WRONLY) == 0 &&
      fcntl(writeonly_access_fd, F_GETFL) == O_WRONLY;
  if (writeonly_access_fd >= 0 && close(writeonly_access_fd) != 0) {
    writeonly_access_ok = 0;
  }
  if (!writeonly_access_ok) {
    printf("fcntl write-only access-mode preservation check failed: %d\n",
           errno);
    fflush(stdout);
    if (writeonly_access_fd >= 0) close(writeonly_access_fd);
    close(readonly_fd);
    close(fd);
    free(allocation);
    return 163;
  }
  int readonly_access_fd = open("newlib-smoke.dat", O_RDONLY);
  int readonly_access_ok =
      readonly_access_fd >= 0 &&
      fcntl(readonly_access_fd, F_SETFL, O_WRONLY) == 0 &&
      fcntl(readonly_access_fd, F_GETFL) == O_RDONLY &&
      fcntl(readonly_access_fd, F_SETFL, O_RDWR | O_APPEND) == 0 &&
      fcntl(readonly_access_fd, F_GETFL) == (O_RDONLY | O_APPEND) &&
      fcntl(readonly_access_fd, F_SETFL, O_RDONLY) == 0 &&
      fcntl(readonly_access_fd, F_GETFL) == O_RDONLY;
  if (readonly_access_fd >= 0 && close(readonly_access_fd) != 0) {
    readonly_access_ok = 0;
  }
  if (!readonly_access_ok) {
    printf("fcntl read-only access-mode preservation check failed: %d\n",
           errno);
    fflush(stdout);
    if (readonly_access_fd >= 0) close(readonly_access_fd);
    close(readonly_fd);
    close(fd);
    free(allocation);
    return 164;
  }
  int setfl_descriptor_flags_fd = open("newlib-smoke.dat", O_RDWR);
  int setfl_descriptor_flags_ok =
      setfl_descriptor_flags_fd >= 0 &&
      fcntl(setfl_descriptor_flags_fd, F_SETFD, FD_CLOEXEC) == 0 &&
      fcntl(setfl_descriptor_flags_fd, F_GETFD) == FD_CLOEXEC &&
      fcntl(setfl_descriptor_flags_fd, F_SETFL, O_RDWR | O_APPEND) == 0 &&
      fcntl(setfl_descriptor_flags_fd, F_GETFL) == (O_RDWR | O_APPEND) &&
      fcntl(setfl_descriptor_flags_fd, F_GETFD) == FD_CLOEXEC &&
      fcntl(setfl_descriptor_flags_fd, F_SETFL, O_RDWR) == 0 &&
      fcntl(setfl_descriptor_flags_fd, F_GETFD) == FD_CLOEXEC;
  if (setfl_descriptor_flags_fd >= 0 &&
      close(setfl_descriptor_flags_fd) != 0) {
    setfl_descriptor_flags_ok = 0;
  }
  if (!setfl_descriptor_flags_ok) {
    printf("fcntl F_SETFL descriptor-flag preservation check failed: %d\n",
           errno);
    fflush(stdout);
    if (setfl_descriptor_flags_fd >= 0) close(setfl_descriptor_flags_fd);
    close(readonly_fd);
    close(fd);
    free(allocation);
    return 165;
  }
  errno = 0;
  if (readonly_fd < 0 || fsync(readonly_fd) != -1 || errno != EBADF) {
    printf("read-only fsync check failed: %d\n", errno);
    fflush(stdout);
    if (readonly_fd >= 0) close(readonly_fd);
    close(fd);
    free(allocation);
    return 38;
  }
  errno = 0;
  if (ftruncate(readonly_fd, 0) != -1 || errno != EBADF) {
    printf("read-only ftruncate check failed: %d\n", errno);
    fflush(stdout);
    close(readonly_fd);
    close(fd);
    free(allocation);
    return 68;
  }
  errno = 0;
  if (fdatasync(readonly_fd) != -1 || errno != EBADF ||
      close(readonly_fd) != 0) {
    printf("read-only fdatasync check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 39;
  }
  int cloexec_fd = open("newlib-cloexec.dat", O_CREAT | O_EXCL |
                         O_RDWR | O_CLOEXEC, 0600);
  if (cloexec_fd < 0 || fcntl(cloexec_fd, F_GETFD) != FD_CLOEXEC ||
      (fcntl(cloexec_fd, F_GETFL) & O_CLOEXEC) != 0 ||
      close(cloexec_fd) != 0 || unlink("newlib-cloexec.dat") != 0) {
    printf("open O_CLOEXEC check failed: %d\n", errno);
    fflush(stdout);
    if (cloexec_fd >= 0) close(cloexec_fd);
    unlink("newlib-cloexec.dat");
    close(fd);
    free(allocation);
    return 95;
  }
  int openat_cloexec_fd = openat(
      AT_FDCWD, "newlib-openat-cloexec.dat",
      O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);
  if (openat_cloexec_fd < 0 ||
      fcntl(openat_cloexec_fd, F_GETFD) != FD_CLOEXEC ||
      (fcntl(openat_cloexec_fd, F_GETFL) & O_CLOEXEC) != 0 ||
      close(openat_cloexec_fd) != 0 ||
      unlink("newlib-openat-cloexec.dat") != 0) {
    printf("openat O_CLOEXEC check failed: %d\n", errno);
    fflush(stdout);
    if (openat_cloexec_fd >= 0) close(openat_cloexec_fd);
    unlink("newlib-openat-cloexec.dat");
    close(fd);
    free(allocation);
    return 96;
  }
  int file_flags = fcntl(fd, F_GETFL);
  if (file_flags != O_RDWR ||
      fcntl(fd, F_SETFL, file_flags | O_APPEND) != 0 ||
      (fcntl(fd, F_GETFL) & O_APPEND) == 0 ||
      fcntl(fd, F_SETFL, file_flags & ~O_APPEND) != 0 ||
      fcntl(fd, F_GETFL) != file_flags) {
    printf("fcntl status-flag check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 34;
  }

  if (fcntl(fd, F_SETFL, O_WRONLY | O_APPEND) != 0 ||
      fcntl(fd, F_GETFL) != (O_RDWR | O_APPEND) ||
      fcntl(fd, F_SETFL, O_RDONLY) != 0 ||
      fcntl(fd, F_GETFL) != O_RDWR) {
    printf("fcntl access-mode preservation check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 112;
  }
  int append_fd = open("newlib-fcntl-append.dat",
                       O_CREAT | O_EXCL | O_RDWR | O_APPEND, 0600);
  char append_readback[2] = {0};
  int append_ok =
      append_fd >= 0 &&
      fcntl(append_fd, F_GETFL) == (O_RDWR | O_APPEND) &&
      fcntl(append_fd, F_SETFL, O_RDWR) == 0 &&
      write(append_fd, "A", 1) == 1 &&
      lseek(append_fd, 0, SEEK_SET) == 0 &&
      fcntl(append_fd, F_SETFL, O_RDWR | O_APPEND) == 0 &&
      write(append_fd, "B", 1) == 1 &&
      lseek(append_fd, 0, SEEK_SET) == 0 &&
      read(append_fd, append_readback, sizeof(append_readback)) ==
          (int)sizeof(append_readback) &&
      append_readback[0] == 'A' && append_readback[1] == 'B';
  if (append_fd >= 0 && close(append_fd) != 0) {
    append_ok = 0;
  }
  if (!append_ok || unlink("newlib-fcntl-append.dat") != 0) {
    printf("fcntl append-write behavior check failed: %d\n", errno);
    fflush(stdout);
    unlink("newlib-fcntl-append.dat");
    close(fd);
    free(allocation);
    return 98;
  }

  if (fcntl(fd, F_GETFD) != 0 ||
      fcntl(fd, F_SETFD, FD_CLOEXEC) != 0 ||
      fcntl(fd, F_GETFD) != FD_CLOEXEC ||
      fcntl(fd, F_SETFD, 0) != 0 ||
      fcntl(fd, F_GETFD) != 0 ||
      fcntl(fd, F_SETFD, FD_CLOEXEC) != 0 ||
      fcntl(fd, F_GETFD) != FD_CLOEXEC) {
    printf("fcntl descriptor-flag set/clear check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 35;
  }
  int duplicate_fd = fcntl(fd, F_DUPFD, 5);
  if (duplicate_fd < 5 || fcntl(duplicate_fd, F_GETFD) != 0) {
    printf("fcntl minimum-duplicate check failed: %d\n", errno);
    fflush(stdout);
    if (duplicate_fd >= 0) close(duplicate_fd);
    close(fd);
    free(allocation);
    return 36;
  }
  int next_duplicate_fd = fcntl(fd, F_DUPFD, 5);
  if (next_duplicate_fd != 6 || fcntl(next_duplicate_fd, F_GETFD) != 0 ||
      close(next_duplicate_fd) != 0) {
    printf("fcntl lowest-free duplicate check failed: %d\n", errno);
    fflush(stdout);
    if (next_duplicate_fd >= 0) close(next_duplicate_fd);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 109;
  }
  int low_minimum_duplicate_fd = fcntl(fd, F_DUPFD, 0);
  if (low_minimum_duplicate_fd != 4 ||
      fcntl(low_minimum_duplicate_fd, F_GETFD) != 0 ||
      close(low_minimum_duplicate_fd) != 0) {
    printf("fcntl below-standard duplicate minimum check failed: %d\\n", errno);
    fflush(stdout);
    if (low_minimum_duplicate_fd >= 0) close(low_minimum_duplicate_fd);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 110;
  }
  if (fcntl(duplicate_fd, F_SETFL, O_RDWR | O_APPEND) != 0 ||
      (fcntl(fd, F_GETFL) & O_APPEND) == 0 ||
      fcntl(fd, F_SETFL, O_RDWR) != 0 ||
      (fcntl(duplicate_fd, F_GETFL) & O_APPEND) != 0) {
    printf("fcntl shared status-flag check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 99;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD, -1) != -1 || errno != EINVAL) {
    printf("fcntl F_DUPFD negative minimum check failed: %d\\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 123;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD_CLOEXEC, -1) != -1 || errno != EINVAL) {
    printf("fcntl F_DUPFD_CLOEXEC negative minimum check failed: %d\\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 124;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD, 8) != -1 || errno != EMFILE) {
    printf("fcntl F_DUPFD descriptor-table boundary check failed: %d\\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 138;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD_CLOEXEC, 8) != -1 || errno != EMFILE) {
    printf("fcntl F_DUPFD_CLOEXEC descriptor-table boundary check failed: %d\\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 139;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD, 0x7fffffff) != -1 || errno != EMFILE) {
    printf("fcntl F_DUPFD large minimum check failed: %d\\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 140;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD_CLOEXEC, 0x7fffffff) != -1 || errno != EMFILE) {
    printf("fcntl F_DUPFD_CLOEXEC large minimum check failed: %d\\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 141;
  }
  errno = 0;
  if (fcntl(99, F_GETFD) != -1 || errno != EBADF) {
    printf("fcntl invalid-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 100;
  }
  if (fcntl(99, F_GETFL) != -1 || errno != EBADF) {
    printf("fcntl F_GETFL invalid-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 142;
  }
  errno = 0;
  if (fcntl(99, F_SETFD, FD_CLOEXEC) != -1 || errno != EBADF) {
    printf("fcntl F_SETFD invalid-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 117;
  }
  errno = 0;
  if (fcntl(99, F_SETFL, O_APPEND) != -1 || errno != EBADF) {
    printf("fcntl F_SETFL invalid-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 118;
  }
  errno = 0;
  if (fcntl(99, F_DUPFD, 3) != -1 || errno != EBADF) {
    printf("fcntl F_DUPFD invalid-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 119;
  }
  errno = 0;
  if (fcntl(99, F_DUPFD_CLOEXEC, 3) != -1 || errno != EBADF) {
    printf("fcntl F_DUPFD_CLOEXEC invalid-descriptor check failed: %d\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 120;
  }
  errno = 0;
  if (fcntl(-1, F_GETFD) != -1 || errno != EBADF) {
    printf("fcntl F_GETFD negative-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 143;
  }
  errno = 0;
  if (fcntl(-1, F_SETFD, FD_CLOEXEC) != -1 || errno != EBADF) {
    printf("fcntl F_SETFD negative-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 144;
  }
  errno = 0;
  if (fcntl(-1, F_GETFL) != -1 || errno != EBADF) {
    printf("fcntl F_GETFL negative-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 145;
  }
  errno = 0;
  if (fcntl(-1, F_SETFL, O_APPEND) != -1 || errno != EBADF) {
    printf("fcntl F_SETFL negative-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 146;
  }
  errno = 0;
  if (fcntl(-1, F_DUPFD, 3) != -1 || errno != EBADF) {
    printf("fcntl F_DUPFD negative-descriptor check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 147;
  }
  errno = 0;
  if (fcntl(-1, F_DUPFD_CLOEXEC, 3) != -1 || errno != EBADF) {
    printf("fcntl F_DUPFD_CLOEXEC negative-descriptor check failed: %d\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 148;
  }
  int exhaustion_fds[3] = {-1, -1, -1};
  for (int i = 0; i < 3; i++) {
    exhaustion_fds[i] = fcntl(fd, F_DUPFD, 3);
    if (exhaustion_fds[i] < 0) break;
  }
  errno = 0;
  int duplicate_exhaustion_result = fcntl(fd, F_DUPFD, 3);
  int duplicate_exhaustion_errno = errno;
  errno = 0;
  int cloexec_exhaustion_result = fcntl(fd, F_DUPFD_CLOEXEC, 3);
  int cloexec_exhaustion_errno = errno;
  if (exhaustion_fds[0] < 0 || exhaustion_fds[1] < 0 ||
      exhaustion_fds[2] < 0 || duplicate_exhaustion_result != -1 ||
      duplicate_exhaustion_errno != EMFILE ||
      cloexec_exhaustion_result != -1 ||
      cloexec_exhaustion_errno != EMFILE) {
    printf("fcntl duplicate descriptor exhaustion check failed: %d/%d\\n",
           duplicate_exhaustion_errno, cloexec_exhaustion_errno);
    fflush(stdout);
    for (int i = 0; i < 3; i++) {
      if (exhaustion_fds[i] >= 0) close(exhaustion_fds[i]);
    }
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 121;
  }
  for (int i = 0; i < 3; i++) {
    if (close(exhaustion_fds[i]) != 0) {
      printf("fcntl duplicate exhaustion cleanup failed: %d\\n", errno);
      fflush(stdout);
      close(duplicate_fd);
      close(fd);
      free(allocation);
      return 122;
    }
  }
  errno = 0;
  if (fcntl(fd, 999) != -1 || errno != EINVAL) {
    printf("fcntl invalid-command check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 101;
  }
  if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0 ||
      fcntl(fd, F_GETFD) != FD_CLOEXEC) {
    printf("fcntl invalid-descriptor-flags setup failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 172;
  }
  errno = 0;
  if (fcntl(fd, F_SETFD, FD_CLOEXEC | 2) != -1 || errno != EINVAL ||
      fcntl(fd, F_GETFD) != FD_CLOEXEC) {
    printf("fcntl invalid-descriptor-flags state preservation check failed: %d\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 173;
  }
  if (fcntl(fd, F_SETFD, FD_CLOEXEC) != 0 ||
      fcntl(duplicate_fd, F_SETFD, 0) != 0 ||
      fcntl(fd, F_GETFD) != FD_CLOEXEC ||
      fcntl(duplicate_fd, F_GETFD) != 0) {
    printf("fcntl duplicate descriptor-flag setup failed: %d\\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 174;
  }
  errno = 0;
  if (fcntl(duplicate_fd, F_SETFD, FD_CLOEXEC | 2) != -1 ||
      errno != EINVAL || fcntl(fd, F_GETFD) != FD_CLOEXEC ||
      fcntl(duplicate_fd, F_GETFD) != 0) {
    printf("fcntl invalid duplicate descriptor-flags state check failed: %d\\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 175;
  }
  errno = 0;
  if (fcntl(fd, F_SETFL, O_RDWR | 0x40000000) != -1 || errno != EINVAL) {
    printf("fcntl unsupported-status-flags check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 103;
  }
  if (fcntl(fd, F_SETFL, O_RDWR | O_APPEND) != 0) {
    printf("fcntl invalid-status-flags setup failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 166;
  }
  errno = 0;
  if (fcntl(fd, F_SETFL, O_RDWR | O_APPEND | 0x40000000) != -1 ||
      errno != EINVAL || fcntl(fd, F_GETFL) != (O_RDWR | O_APPEND)) {
    printf("fcntl invalid-status-flags state preservation check failed: %d\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 167;
  }
  if (fcntl(fd, F_SETFL, O_RDWR) != 0) {
    printf("fcntl invalid-status-flags cleanup failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 168;
  }
  if (fcntl(fd, F_SETFL, O_RDWR) != 0 ||
      fcntl(duplicate_fd, F_SETFL, O_RDWR | O_APPEND) != 0 ||
      fcntl(fd, F_GETFL) != (O_RDWR | O_APPEND)) {
    printf("fcntl duplicate invalid-status-flags setup failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 169;
  }
  errno = 0;
  if (fcntl(duplicate_fd, F_SETFL, O_RDWR | O_APPEND | 0x40000000) != -1 ||
      errno != EINVAL ||
      fcntl(fd, F_GETFL) != (O_RDWR | O_APPEND) ||
      fcntl(duplicate_fd, F_GETFL) != (O_RDWR | O_APPEND)) {
    printf("fcntl invalid-status-flags duplicate state check failed: %d\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 170;
  }
  if (fcntl(fd, F_SETFL, O_RDWR) != 0) {
    printf("fcntl duplicate invalid-status-flags cleanup failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 171;
  }
  int low_minimum_cloexec_fd = fcntl(fd, F_DUPFD_CLOEXEC, 1);
  if (low_minimum_cloexec_fd != 4 ||
      fcntl(low_minimum_cloexec_fd, F_GETFD) != FD_CLOEXEC ||
      close(low_minimum_cloexec_fd) != 0) {
    printf("fcntl below-standard close-on-exec duplicate minimum check failed: %d\\n",
           errno);
    fflush(stdout);
    if (low_minimum_cloexec_fd >= 0) close(low_minimum_cloexec_fd);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 111;
  }
  int cloexec_duplicate_fd = fcntl(fd, F_DUPFD_CLOEXEC, 5);
  if (cloexec_duplicate_fd != 6 ||
      fcntl(cloexec_duplicate_fd, F_GETFD) != FD_CLOEXEC ||
      fcntl(cloexec_duplicate_fd, F_GETFL) != fcntl(fd, F_GETFL)) {
    printf("fcntl close-on-exec duplicate check failed: %d\\n", errno);
    fflush(stdout);
    if (cloexec_duplicate_fd >= 0) close(cloexec_duplicate_fd);
    close(fd);
    free(allocation);
    return 97;
  }
  if (lseek(fd, 7, SEEK_SET) != 7 ||
      lseek(cloexec_duplicate_fd, 0, SEEK_CUR) != 7 ||
      lseek(cloexec_duplicate_fd, 0, SEEK_SET) != 0 ||
      lseek(fd, 0, SEEK_CUR) != 0 ||
      fcntl(cloexec_duplicate_fd, F_SETFD, 0) != 0 ||
      fcntl(cloexec_duplicate_fd, F_GETFD) != 0 ||
      fcntl(fd, F_GETFD) != FD_CLOEXEC ||
      fcntl(cloexec_duplicate_fd, F_SETFD, FD_CLOEXEC) != 0 ||
      fcntl(cloexec_duplicate_fd, F_GETFD) != FD_CLOEXEC) {
    printf("fcntl close-on-exec duplicate state check failed: %d\\n", errno);
    fflush(stdout);
    close(cloexec_duplicate_fd);
    close(fd);
    free(allocation);
    return 113;
  }
  if (close(cloexec_duplicate_fd) != 0) {
    printf("fcntl close-on-exec duplicate cleanup failed: %d\\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 114;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD, -1) != -1 || errno != EINVAL) {
    printf("fcntl negative duplicate minimum check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 104;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD, 8) != -1 || errno != EMFILE) {
    printf("fcntl exhausted duplicate minimum check failed: %d\n", errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 105;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD_CLOEXEC, -1) != -1 || errno != EINVAL) {
    printf("fcntl negative close-on-exec duplicate minimum check failed: %d\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 106;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD_CLOEXEC, 8) != -1 || errno != EMFILE) {
    printf("fcntl exhausted close-on-exec duplicate minimum check failed: %d\n",
           errno);
    fflush(stdout);
    close(duplicate_fd);
    close(fd);
    free(allocation);
    return 107;
  }
  if (close(duplicate_fd) != 0) {
    printf("fcntl duplicate cleanup failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 108;
  }
  int file_ok = write(fd, payload, sizeof(payload)) == sizeof(payload) &&
                lseek(fd, 0, SEEK_SET) == 0 &&
                read(fd, readback, sizeof(readback)) == sizeof(readback) &&
                memcmp(payload, readback, sizeof(payload)) == 0 &&
                fstat(fd, &status) == 0 && S_ISREG(status.st_mode) &&
                status.st_size == sizeof(payload);
  char positional_byte = 'N';
  char positional_readback[sizeof(payload)] = {0};
  int positional_ok =
      pread(fd, positional_readback, sizeof(payload), 0) == sizeof(payload) &&
      memcmp(payload, positional_readback, sizeof(payload)) == 0 &&
      lseek(fd, 0, SEEK_CUR) == (off_t)sizeof(payload) &&
      pwrite(fd, &positional_byte, 1, 0) == 1 &&
      lseek(fd, 0, SEEK_CUR) == (off_t)sizeof(payload) &&
      pread(fd, positional_readback, 1, 0) == 1 &&
      positional_readback[0] == 'N';
  int truncate_result = ftruncate(fd, 10);
  int truncate_ok = truncate_result == 0 &&
                    lseek(fd, 0, SEEK_CUR) == (off_t)sizeof(payload) &&
                    fstat(fd, &status) == 0 && status.st_size == 10;
  if (truncate_ok) {
    char zeroes[4] = {1, 1, 1, 1};
    truncate_ok = lseek(fd, 6, SEEK_SET) == 6 &&
                  read(fd, zeroes, sizeof(zeroes)) == (int)sizeof(zeroes) &&
                  zeroes[0] == 0 && zeroes[1] == 0 &&
                  zeroes[2] == 0 && zeroes[3] == 0;
  }
  int close_result = close(fd);
  errno = 0;
  if (fcntl(fd, F_GETFD) != -1 || errno != EBADF) {
    printf("closed-descriptor fcntl F_GETFD check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 149;
  }
  errno = 0;
  if (fcntl(fd, F_SETFD, FD_CLOEXEC) != -1 || errno != EBADF) {
    printf("closed-descriptor fcntl F_SETFD check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 150;
  }
  errno = 0;
  if (fcntl(fd, F_GETFL) != -1 || errno != EBADF) {
    printf("closed-descriptor fcntl F_GETFL check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 151;
  }
  errno = 0;
  if (fcntl(fd, F_SETFL, O_APPEND) != -1 || errno != EBADF) {
    printf("closed-descriptor fcntl F_SETFL check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 152;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD, 3) != -1 || errno != EBADF) {
    printf("closed-descriptor fcntl F_DUPFD check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 153;
  }
  errno = 0;
  if (fcntl(fd, F_DUPFD_CLOEXEC, 3) != -1 || errno != EBADF) {
    printf("closed-descriptor fcntl F_DUPFD_CLOEXEC check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 154;
  }
  errno = 0;
  if (fcntl(8, F_GETFD) != -1 || errno != EBADF) {
    printf("out-of-range fcntl F_GETFD check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 155;
  }
  errno = 0;
  if (fcntl(8, F_SETFD, FD_CLOEXEC) != -1 || errno != EBADF) {
    printf("out-of-range fcntl F_SETFD check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 156;
  }
  errno = 0;
  if (fcntl(8, F_GETFL) != -1 || errno != EBADF) {
    printf("out-of-range fcntl F_GETFL check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 157;
  }
  errno = 0;
  if (fcntl(8, F_SETFL, O_APPEND) != -1 || errno != EBADF) {
    printf("out-of-range fcntl F_SETFL check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 158;
  }
  errno = 0;
  if (fcntl(8, F_DUPFD, 3) != -1 || errno != EBADF) {
    printf("out-of-range fcntl F_DUPFD check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 159;
  }
  errno = 0;
  if (fcntl(8, F_DUPFD_CLOEXEC, 3) != -1 || errno != EBADF) {
    printf("out-of-range fcntl F_DUPFD_CLOEXEC check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 160;
  }
  errno = 0;
  if (ftruncate(fd, 0) != -1 || errno != EBADF) {
    printf("closed-descriptor ftruncate check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 69;
  }
  errno = 0;
  if (ftruncate(-1, 0) != -1 || errno != EBADF) {
    printf("invalid-descriptor ftruncate check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 70;
  }
  if (!file_ok || !positional_ok || !truncate_ok || close_result != 0) {
    printf("file syscall check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 7;
  }

  int missing_fd = open("newlib-smoke-missing", O_RDONLY);
  if (missing_fd != -1 || errno != ENOENT) {
    int error = errno;
    if (missing_fd >= 0) {
      close(missing_fd);
    }
    printf("errno translation failed: expected %d, got %d\n", ENOENT, error);
    fflush(stdout);
    free(allocation);
    return 8;
  }
  DIR *root_directory = opendir("/");
  if (!root_directory) {
    printf("opendir root failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 88;
  }
  struct dirent *directory_entry = readdir(root_directory);
  long directory_position = telldir(root_directory);
  if (!directory_entry || !directory_entry->d_name[0] ||
      directory_position != 1) {
    printf("readdir/telldir check failed: %d\n", errno);
    fflush(stdout);
    closedir(root_directory);
    free(allocation);
    return 89;
  }
  DIR *second_directory = opendir("/");
  if (!second_directory || telldir(second_directory) != 0) {
    printf("independent directory position check failed: %d\n", errno);
    fflush(stdout);
    if (second_directory) closedir(second_directory);
    closedir(root_directory);
    free(allocation);
    return 90;
  }
  rewinddir(root_directory);
  directory_entry = readdir(root_directory);
  if (!directory_entry || telldir(root_directory) != directory_position) {
    printf("rewinddir check failed: %d\n", errno);
    fflush(stdout);
    closedir(second_directory);
    closedir(root_directory);
    free(allocation);
    return 91;
  }
  seekdir(root_directory, directory_position);
  if (telldir(root_directory) != directory_position ||
      closedir(second_directory) != 0 ||
      closedir(root_directory) != 0) {
    printf("seekdir/closedir check failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 92;
  }

  int directory_fd_for_stream = open("/", O_RDONLY | O_DIRECTORY);
  DIR *fd_stream = directory_fd_for_stream >= 0
                       ? fdopendir(directory_fd_for_stream) : NULL;
  if (!fd_stream || dirfd(fd_stream) != directory_fd_for_stream ||
      closedir(fd_stream) != 0) {
    printf("fdopendir/dirfd ownership check failed: %d\n", errno);
    fflush(stdout);
    if (directory_fd_for_stream >= 0 && !fd_stream)
      close(directory_fd_for_stream);
    free(allocation);
    return 93;
  }

  int regular_fd_for_stream =
      open("newlib-fdopendir-file", O_CREAT | O_EXCL | O_RDWR, 0600);
  errno = 0;
  if (regular_fd_for_stream < 0 ||
      fdopendir(regular_fd_for_stream) != NULL || errno != ENOTDIR ||
      fstat(regular_fd_for_stream, &status) != 0 ||
      close(regular_fd_for_stream) != 0 ||
      unlink("newlib-fdopendir-file") != 0) {
    printf("fdopendir non-directory rejection check failed: %d\n", errno);
    fflush(stdout);
    if (regular_fd_for_stream >= 0) close(regular_fd_for_stream);
    unlink("newlib-fdopendir-file");
    free(allocation);
    return 94;
  }

  printf("Newlib syscall smoke test passed\n");
  int flush_result = fflush(stdout);
  free(allocation);
  exit(flush_result == 0 ? 0 : 9);
}
