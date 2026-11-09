#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/types.h>
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
    printf("access existence check failed: %d\\n", errno);
    fflush(stdout);
    return 25;
  }
  errno = 0;
  if (access("newlib-cwd-test", X_OK) == 0 || errno != EACCES) {
    printf("access execute-permission check failed: %d\\n", errno);
    fflush(stdout);
    return 26;
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
    printf("page-size query check failed: %d\\n", errno);
    fflush(stdout);
    return 49;
  }
  errno = 0;
  if (sysconf(-1) != -1 || errno != EINVAL) {
    printf("invalid sysconf query check failed: %d\\n", errno);
    fflush(stdout);
    return 50;
  }
#ifdef _SC_OPEN_MAX
  if (sysconf(_SC_OPEN_MAX) != 8 || getdtablesize() != 8) {
    printf("open-descriptor limit check failed: %d\\n", errno);
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
    printf("link check failed: %d\\n", errno);
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
    printf("monotonic clock/sleep check failed: %d\\n", errno);
    fflush(stdout);
    return 28;
  }
  struct timespec invalid_sleep = {0, 1000000000};
  errno = 0;
  if (nanosleep(&invalid_sleep, NULL) != -1 || errno != EINVAL) {
    printf("nanosleep validation check failed: %d\\n", errno);
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
    printf("path truncate check failed: %d\\n", errno);
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
    printf("path truncate extension check failed: %d\\n", errno);
    fflush(stdout);
    if (truncate_fd >= 0) close(truncate_fd);
    unlink("newlib-truncate.dat");
    return 46;
  }
  errno = 0;
  if (truncate("newlib-truncate.dat", -1) != -1 || errno != EINVAL) {
    printf("negative truncate length check failed: %d\\n", errno);
    fflush(stdout);
    return 47;
  }
  errno = 0;
  if (truncate("newlib-truncate-missing.dat", 1) != -1 || errno != ENOENT) {
    printf("missing truncate path check failed: %d\\n", errno);
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
  umask(previous_umask);

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
    printf("fchmod invalid descriptor check failed: %d\\n", errno);
    fflush(stdout);
    close(chmod_test_fd);
    unlink("newlib-fchmod.dat");
    return 66;
  }
  if (close(chmod_test_fd) != 0 || unlink("newlib-fchmod.dat") != 0) {
    printf("fchmod test cleanup failed: %d\\n", errno);
    fflush(stdout);
    return 67;
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
  if (fdatasync(readonly_fd) != -1 || errno != EBADF ||
      close(readonly_fd) != 0) {
    printf("read-only fdatasync check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 39;
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
  if (fcntl(fd, F_GETFD) != 0 ||
      fcntl(fd, F_SETFD, FD_CLOEXEC) != 0 ||
      fcntl(fd, F_GETFD) != FD_CLOEXEC) {
    printf("fcntl descriptor-flag check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 35;
  }
  int duplicate_fd = fcntl(fd, F_DUPFD, 5);
  if (duplicate_fd < 5 || fcntl(duplicate_fd, F_GETFD) != 0 ||
      close(duplicate_fd) != 0) {
    printf("fcntl minimum-duplicate check failed: %d\n", errno);
    fflush(stdout);
    close(fd);
    free(allocation);
    return 36;
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
  int truncate_ok = truncate_result == 0 && fstat(fd, &status) == 0 &&
                    status.st_size == 10;
  if (truncate_ok) {
    char zeroes[4] = {1, 1, 1, 1};
    truncate_ok = lseek(fd, 6, SEEK_SET) == 6 &&
                  read(fd, zeroes, sizeof(zeroes)) == (int)sizeof(zeroes) &&
                  zeroes[0] == 0 && zeroes[1] == 0 &&
                  zeroes[2] == 0 && zeroes[3] == 0;
  }
  int close_result = close(fd);
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
  printf("Newlib syscall smoke test passed\n");
  int flush_result = fflush(stdout);
  free(allocation);
  exit(flush_result == 0 ? 0 : 9);
}
