#include <sys/types.h>
#include <sys/stat.h>

#include "posix.h"

// Custom minimal errno declaration since we aren't using Newlib headers
#define ENOSYS 38  // Function not implemented
#define EINVAL 22  // Invalid argument
static int mock_errno;

int* __posix_errno_location(void) {
    return &mock_errno;
}
#define errno (*__posix_errno_location())

/* ========================================================================
   FILE OPERATIONS
   ======================================================================== */

int open(const char *pathname, int flags, ...) {
    // ASM: Prepare registers with pathname, flags. Trigger syscall.
    errno = ENOSYS;
    return -1;
}

ssize_t read(int fd, void *buf, size_t count) {
    // ASM: Pass fd, buf pointer, and count to your asm reader.
    errno = ENOSYS;
    return -1;
}

ssize_t write(int fd, const void *buf, size_t count) {
    // Immediate Debug Hook: You can drop an inline assembly block here
    // to pipe string pointers straight to an early serial/UART out.
    errno = ENOSYS;
    return -1;
}

int close(int fd) {
    errno = ENOSYS;
    return -1;
}

off_t lseek(int fd, off_t offset, int whence) {
    errno = ENOSYS;
    return (off_t)-1;
}

/* ========================================================================
   PROCESS & MEMORY LIFECYCLE
   ======================================================================== */

void _exit(int status) {
    // ASM: CLI/HLT loop or system reset vector jump.
    while(1) {
        #if defined(__x86_64__) || defined(__i386__)
        __asm__ __volatile__("cli; hlt");
        #define HAS_HALT
        #endif
        #if defined(__arm__) || defined(__aarch64__)
        __asm__ __volatile__("wfi");
        #define HAS_HALT
        #endif
        #if !defined(HAS_HALT)
        // Fallback catch-all loop
        #endif
    }
}

pid_t getpid(void) {
    return 1; 
}

int kill(pid_t pid, int sig) {
    errno = EINVAL;
    return -1;
}

/* ========================================================================
   MEMORY ALLOCATION LAYER
   ======================================================================== */

void *brk(void *addr) {
    // Basic POSIX memory tracking stub
    static void *current_brk = 0;
    if (addr == 0) return current_brk;
    
    // ASM: Update kernel page allocation tables via assembly interrupt
    current_brk = addr;
    return current_brk;
}
