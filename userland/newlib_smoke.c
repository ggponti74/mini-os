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
                   O_TRUNC == 0x0400 && O_EXCL == 0x0800,
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

  errno = 123;
  if (errno != 123) {
    printf("errno storage check failed\n");
    fflush(stdout);
    return 18;
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

  if (mkdir("newlib-cwd-test", 0700) != 0 ||
      chdir("newlib-cwd-test") != 0) {
    printf("nested chdir check failed: %d\n", errno);
    fflush(stdout);
    return 21;
  }
  char nested_cwd[32];
  if (getcwd(nested_cwd, sizeof(nested_cwd)) != nested_cwd ||
      strcmp(nested_cwd, "/newlib-cwd-test") != 0) {
    printf("nested getcwd check failed: %s (%d)\n",
           nested_cwd, errno);
    fflush(stdout);
    return 22;
  }
  if (chdir("/") != 0 || rmdir("newlib-cwd-test") != 0) {
    printf("return-to-root check failed: %d\n", errno);
    fflush(stdout);
    return 23;
  }

  if (_getpid() != 1 || getpid() != 1) {
    printf("getpid check failed\n");
    fflush(stdout);
    return 10;
  }
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
  if (_gettimeofday(&current_time, NULL) != -1 || errno != ENOSYS) {
    printf("gettimeofday check failed: %d\n", errno);
    fflush(stdout);
    return 15;
  }
  struct tms process_times;
  errno = 0;
  if (_times(&process_times) != (clock_t)-1 || errno != ENOSYS) {
    printf("times check failed: %d\n", errno);
    fflush(stdout);
    return 16;
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
  if (fd < 0) {
    printf("open failed: %d\n", errno);
    fflush(stdout);
    free(allocation);
    return 6;
  }
  int file_ok = write(fd, payload, sizeof(payload)) == sizeof(payload) &&
                lseek(fd, 0, SEEK_SET) == 0 &&
                read(fd, readback, sizeof(readback)) == sizeof(readback) &&
                memcmp(payload, readback, sizeof(payload)) == 0 &&
                fstat(fd, &status) == 0 && S_ISREG(status.st_mode) &&
                status.st_size == sizeof(payload);
  int close_result = close(fd);
  if (!file_ok || close_result != 0) {
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
  return flush_result == 0 ? 0 : 9;
}
