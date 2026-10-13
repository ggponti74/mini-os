// src/exec.c
#include "display.h"
#include "fs/initrd.h"
#include "fs/vfs.h"
#include <stddef.h>
#include <stdint.h>

// External references to your PMM
extern uintptr_t pmm_alloc_frame(void);
extern void pmm_free_frame(uintptr_t frame_addr);

int exec_flat_binary(const char *path) {

  // 1. Look up the binary file in the VFS / RAMDisk pool
  vfs_node_t *file = initrd_find_file(path);
  if (!file) {
    return -1; // File not found -> shell will print "Unknown command or file"
  }

  // Safety check: Ensure file has actual content
  if (file->length == 0) {
    return -1;
  }

  // 2. Calculate how many 4 KB pages are needed
  size_t pages_needed = (file->length + 4095) / 4096;

  // 3. Allocate physical frames sequentially
  uintptr_t first_frame = 0;
  for (size_t i = 0; i < pages_needed; i++) {
    uintptr_t frame = pmm_alloc_frame();
    if (frame == 0) {
      return -2; // Out of memory
    }
    if (i == 0) {
      first_frame = frame;
    }
  }

  // 4. Copy raw payload bytes from the RAMDisk node
  uint8_t *dest_ptr = (uint8_t *)first_frame;
  vfs_read(file, 0, file->length, dest_ptr);

  // 5. Cast and execute
  void (*entry)(void) = (void (*)(void))first_frame;
  entry();

  return 0;
}
