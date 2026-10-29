// src/fs/initrd.c
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../display.h"
#include "initrd.h"
#include "vfs.h"

#define MAX_NODES 32
#define MAX_FILE_SIZE 8192

typedef struct {
  vfs_node_t node;
  uint8_t data[MAX_FILE_SIZE];
} initrd_file_t;

static initrd_file_t file_pool[MAX_NODES];
// Keep the regular file pool compact while allowing one file to exceed 8 KiB.
static uint8_t extended_file_data[INITRD_MAX_FILE_SIZE];
static int extended_file_in_use;
static uint32_t node_count = 0;
static vfs_node_t initrd_root;

extern void itoa(uint32_t val, char *buf);

static uint32_t initrd_node_index(const vfs_node_t *node) {
  for (uint32_t i = 0; i < node_count; i++) {
    if (&file_pool[i].node == node) {
      return i;
    }
  }
  return node_count;
}

static void initrd_remove_node_at(uint32_t index) {
  if (file_pool[index].node.device_data == extended_file_data) {
    extended_file_in_use = 0;
  }
  for (uint32_t i = index; i + 1 < node_count; i++) {
    void *source_data = file_pool[i + 1].node.device_data;
    file_pool[i] = file_pool[i + 1];
    if ((file_pool[i].node.flags & FS_FILE) &&
        source_data == file_pool[i + 1].data) {
      file_pool[i].node.device_data = file_pool[i].data;
    }
  }
  node_count--;
  file_pool[node_count].node.name[0] = '\0';
  file_pool[node_count].node.length = 0;
  file_pool[node_count].node.flags = 0;
  file_pool[node_count].node.device_data = NULL;
}

// Internal helper: Compare string up to max length
static int string_equals(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// Helper to check if an entry is root/dot directory marker
static int is_dot_entry(const char *name) {
  if (!name)
    return 1;

  // Strip leading "./" if present
  if (name[0] == '.' && name[1] == '/') {
    name += 2;
  }

  return (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 ||
          strcmp(name, "") == 0);
}

// src/fs/initrd.c

// 1. Copy file contents from source to destination
int initrd_copy_file(const char *src_name, const char *dest_name) {
  vfs_node_t *src = initrd_find_file(src_name);
  if (!src || (src->flags & FS_DIRECTORY))
    return -1;

  vfs_node_t *dest = initrd_find_file(dest_name);
  if (dest && (dest->flags & FS_DIRECTORY))
    return -1;
  if (!dest) {
    dest = vfs_create(fs_root, dest_name, FS_FILE);
    if (!dest)
      return -2;
  }

  uint8_t buf[64];
  for (uint32_t i = 0; i < src->length; i += sizeof(buf)) {
    uint32_t chunk =
        (src->length - i < sizeof(buf)) ? (src->length - i) : sizeof(buf);
    vfs_read(src, i, chunk, buf);
    vfs_write(dest, i, chunk, buf);
  }
  return 0;
}

// Move / Rename a file
int initrd_move_file(const char *old_name, const char *new_name) {
  vfs_node_t *file = initrd_find_file(old_name);
  if (!file || (file->flags & FS_DIRECTORY) || !new_name || !*new_name)
    return -1;

  size_t destination_length = strlen(new_name);
  if (destination_length >= sizeof(file->name))
    return -1;
  char destination[sizeof(file->name)];
  for (size_t i = 0; i <= destination_length; i++) {
    destination[i] = new_name[i];
  }

  char *base_name = destination;
  char *parent_path = NULL;
  for (size_t i = 0; i < destination_length; i++) {
    if (destination[i] == '/') {
      base_name = destination + i + 1;
      parent_path = destination + i;
    }
  }
  if (*base_name == '\0' || strcmp(base_name, ".") == 0 ||
      strcmp(base_name, "..") == 0)
    return -1;
  if (parent_path) {
    *parent_path = '\0';
  }
  vfs_node_t *parent =
      parent_path ? vfs_lookup(fs_root, destination) : fs_root;
  if (!parent || !(parent->flags & FS_DIRECTORY) ||
      initrd_find_file(new_name))
    return -1;

  for (size_t i = 0; i <= destination_length; i++) {
    file->name[i] = new_name[i];
  }
  return 0;
}

// Remove / Delete a file from the pool
int initrd_remove_file(const char *filename) {
  if (!filename)
    return -1;

  // Skip any leading spaces passed from the shell dispatcher
  while (*filename == ' ')
    filename++;

  // Normalize target filename by stripping leading "./" if present
  const char *target = filename;
  if (target[0] == '.' && target[1] == '/')
    target += 2;
  while (*target == '/')
    target++;

  if (target[0] == '/') {
    target++;
  }

  vfs_node_t *node = initrd_find_file(target);
  if (node && (node->flags & FS_DIRECTORY)) {
    return -2;
  }
  if (!node) {
    return -1;
  }

  uint32_t index = initrd_node_index(node);
  if (index == node_count)
    return -1;
  initrd_remove_node_at(index);
  return 0;
}

int initrd_remove_directory(const char *dirname) {
  if (!dirname)
    return -1;

  while (*dirname == ' ')
    dirname++;
  while (*dirname == '/')
    dirname++;
  if (dirname[0] == '.' && dirname[1] == '/')
    dirname += 2;

  vfs_node_t *directory = initrd_find_file(dirname);
  if (!directory || !(directory->flags & FS_DIRECTORY))
    return -1;

  size_t length = strlen(directory->name);
  for (uint32_t i = 0; i < node_count; i++) {
    const char *name = file_pool[i].node.name;
    if (&file_pool[i].node != directory &&
        strncmp(name, directory->name, length) == 0 &&
        name[length] == '/') {
      return -2;
    }
  }

  uint32_t index = initrd_node_index(directory);
  if (index == node_count)
    return -1;
  initrd_remove_node_at(index);
  return 0;
}

int initrd_get_file(const char *filename, uint8_t **out_data,
                    uint32_t *out_size) {
  const char *ptr = initrd_start;

  while (ptr < initrd_end) {
    tar_header_t *header = (tar_header_t *)ptr;

    // End of TAR archive marker
    if (header->name[0] == '\0') {
      break;
    }

    uint32_t file_size = oct2bin(header->size, 11);

    // Strip leading "./" if present before matching
    const char *hn = header->name;
    if (hn[0] == '.' && hn[1] == '/') {
      hn += 2;
    }
    const char *fn = filename;
    if (fn[0] == '.' && fn[1] == '/') {
      fn += 2;
    }
    if (string_equals(hn, fn) == 0) {
      if (out_data) {
        *out_data =
            (uint8_t *)(ptr +
                        512); // File payload starts right after header block
      }
      if (out_size) {
        *out_size = file_size;
      }
      return 0; // Success
    }

    // Advance pointer past header (512 bytes) + file payload (rounded to
    // 512-byte block boundaries)
    ptr += 512 + ((file_size + 511) & ~511);
  }

  return -1; // File not found
}

uint32_t oct2bin(const char *str, int size) {
  uint32_t n = 0;
  for (int i = 0; i < size && str[i] >= '0' && str[i] <= '7'; i++) {
    n = (n << 3) + (str[i] - '0');
  }
  return n;
}

// 1. Find/Lookup a file by name inside our dynamic in-memory file pool
vfs_node_t *initrd_find_file(const char *filename) {
  if (!filename)
    return NULL;

  for (uint32_t i = 0; i < node_count; i++) {
    // Strip leading "./" if present
    const char *entry_name = file_pool[i].node.name;
    if (entry_name[0] == '.' && entry_name[1] == '/') {
      entry_name += 2;
    }

    const char *target = filename;
    if (target[0] == '.' && target[1] == '/') {
      target += 2;
    }

    int match = 1;
    int j = 0;
    while (target[j] != '\0' || entry_name[j] != '\0') {
      if (target[j] != entry_name[j]) {
        match = 0;
        break;
      }
      j++;
    }

    if (match) {
      return &file_pool[i].node;
    }
  }

  return NULL; // Not found
}

// 2. List all files currently residing in the RAMDisk pool (including newly
// touched files) kernel/fs/initrd.c

static const char *initrd_child_name(const char *name, const char *directory,
                                     size_t directory_length) {
  const char *child = name;
  if (directory_length > 0) {
    if (strncmp(name, directory, directory_length) != 0 ||
        name[directory_length] != '/') {
      return NULL;
    }
    child = name + directory_length + 1;
  }
  if (*child == '\0') {
    return NULL;
  }
  for (size_t i = 0; child[i] != '\0'; i++) {
    if (child[i] == '/') {
      return NULL;
    }
  }
  return child;
}

void initrd_list_files(const char *directory) {
    // Collect valid, non-dot node indices
    uint32_t valid_indices[64]; 
    uint32_t count = 0;
    uint32_t totalLength = 0;
    size_t directory_length = directory ? strlen(directory) : 0;
    
    for (uint32_t i = 0; i < node_count && count < 64; i++)
    {
      const char *name =
          initrd_child_name(file_pool[i].node.name, directory, directory_length);
      if (name && !is_dot_entry(name))
      {
        valid_indices[count++] = i;
      }
    }

    // Sort valid entry indices alphabetically by node name (Bubble Sort)
    for (uint32_t i = 0; i < count; i++) {
        for (uint32_t j = i + 1; j < count; j++) {
            const char *name_i = file_pool[valid_indices[i]].node.name;
            const char *name_j = file_pool[valid_indices[j]].node.name;
            name_i = initrd_child_name(name_i, directory, directory_length);
            name_j = initrd_child_name(name_j, directory, directory_length);

            // Strip leading "./" if present for comparison[cite: 4]
            if (name_i[0] == '.' && name_i[1] == '/') name_i += 2;
            if (name_j[0] == '.' && name_j[1] == '/') name_j += 2;

            if (strcmp(name_i, name_j) > 0) {
                // Swap indices[cite: 4]
                uint32_t temp = valid_indices[i];
                valid_indices[i] = valid_indices[j];
                valid_indices[j] = temp;
            }
        }
    }

    // Print sorted files[cite: 4]
    for (uint32_t i = 0; i < count; i++) {
        uint32_t idx = valid_indices[i];
        const char *display_name = initrd_child_name(
            file_pool[idx].node.name, directory, directory_length);
        
        if (display_name[0] == '.' && display_name[1] == '/') {
            display_name += 2;
        }

        // Print type ('f' or 'd')[cite: 4]
        char type_char = (file_pool[idx].node.flags & FS_DIRECTORY) ? 'd' : 'f';
        kputchar_color(type_char, COLOR_DEFAULT);
        kprint_color("   ", COLOR_DEFAULT);

        // Format and right-align size in a 6-character column[cite: 4]
        char size_buf[12];
        itoa(file_pool[idx].node.length, size_buf);

        totalLength += file_pool[idx].node.length;
       
        int len = 0;
        while (size_buf[len] != '\0') len++;
        
        int width = 6;
        for (int p = len; p < width; p++) {
            kputchar_color(' ', COLOR_DEFAULT);
        }
        
        kprint_color(size_buf, COLOR_DEFAULT);
        kprint_color("   ", COLOR_DEFAULT);

        // Print filename[cite: 4]
        kprint_color(display_name, COLOR_DEFAULT);
        kputchar_color('\n', COLOR_DEFAULT);
    }

    char size_buf[12];
    itoa(totalLength, size_buf);
    kprint_color("     ", COLOR_DEFAULT);
    kprint_color(size_buf, COLOR_DEFAULT);
    kprint_color("\n", COLOR_DEFAULT); 
}

// 3. Print contents of a file using vfs_read or direct pool access
void initrd_cat_file(const char *filename) {
  vfs_node_t *file = initrd_find_file(filename);

  if (!file) {
    kprint_color("cat: file not found: ", COLOR_DEFAULT);
    kprint_color(filename, COLOR_DEFAULT);
    kputchar_color('\n', COLOR_DEFAULT);
    return;
  }

  uint8_t buf;
  for (uint32_t i = 0; i < file->length; i++) {
    if (vfs_read(file, i, 1, &buf) > 0) {
      kputchar_color((char)buf, COLOR_DEFAULT);
    }
  }

  if (file->length > 0 &&
      ((uint8_t *)file->device_data)[file->length - 1] != '\n') {
    kputchar_color('\n', COLOR_DEFAULT);
  }
}

static uint32_t initrd_read_file(vfs_node_t *node, uint32_t offset,
                                 uint32_t size, uint8_t *buffer) {
  if (!node || !node->device_data || offset >= node->length)
    return 0;
  if (offset + size > node->length)
    size = node->length - offset;

  const uint8_t *src = (const uint8_t *)node->device_data + offset;
  for (uint32_t i = 0; i < size; i++)
    buffer[i] = src[i];
  return size;
}

static uint32_t initrd_write_file(vfs_node_t *node, uint32_t offset,
                                  uint32_t size, const uint8_t *buffer) {
  if (!node || !node->device_data || !buffer)
    return 0;
  uint32_t capacity = MAX_FILE_SIZE;
  if (node->device_data == extended_file_data) {
    capacity = INITRD_MAX_FILE_SIZE;
  } else if ((offset >= MAX_FILE_SIZE ||
              size > MAX_FILE_SIZE - offset) &&
             !extended_file_in_use) {
    uint8_t *old_data = (uint8_t *)node->device_data;
    for (uint32_t i = 0; i < node->length; i++) {
      extended_file_data[i] = old_data[i];
    }
    node->device_data = extended_file_data;
    extended_file_in_use = 1;
    capacity = INITRD_MAX_FILE_SIZE;
  }
  if (offset >= capacity)
    return 0;
  uint8_t *dest = (uint8_t *)node->device_data;

  if (size > capacity - offset)
    size = capacity - offset;
  for (uint32_t i = 0; i < size; i++)
    dest[offset + i] = buffer[i];

  if (offset + size > node->length)
    node->length = offset + size;
  return size;
}

static vfs_node_t *initrd_create_file(vfs_node_t *parent, const char *name,
                                      uint32_t flags) {
  if (!parent || !(parent->flags & FS_DIRECTORY) || !name ||
      (flags != FS_FILE && flags != FS_DIRECTORY) || node_count >= MAX_NODES)
    return NULL;

  for (size_t i = 0; name[i] != '\0'; i++) {
    if (name[i] == '/') {
      return NULL;
    }
  }
  if (name[0] == '\0' || strcmp(name, ".") == 0 ||
      strcmp(name, "..") == 0) {
    return NULL;
  }

  char full_name[sizeof(file_pool[0].node.name)];
  size_t parent_length = strlen(parent->name);
  size_t name_length = strlen(name);
  size_t separator_length = parent_length > 0 ? 1 : 0;
  if (parent_length + separator_length + name_length >= sizeof(full_name)) {
    return NULL;
  }

  size_t offset = 0;
  for (size_t i = 0; i < parent_length; i++) {
    full_name[offset++] = parent->name[i];
  }
  if (separator_length) {
    full_name[offset++] = '/';
  }
  for (size_t i = 0; i < name_length; i++) {
    full_name[offset++] = name[i];
  }
  full_name[offset] = '\0';
  if (initrd_find_file(full_name)) {
    return NULL;
  }

  initrd_file_t *item = &file_pool[node_count++];
  for (size_t i = 0; i <= offset; i++) {
    item->node.name[i] = full_name[i];
  }
  item->node.flags = flags;
  item->node.length = 0;
  item->node.inode = node_count;
  item->node.read = flags == FS_FILE ? initrd_read_file : NULL;
  item->node.write = flags == FS_FILE ? initrd_write_file : NULL;
  item->node.create = flags == FS_DIRECTORY ? initrd_create_file : NULL;
  item->node.device_data = flags == FS_FILE ? item->data : NULL;

  return &item->node;
}

vfs_node_t *initrd_init(void) {
  // 1. Setup Root Directory Node
  node_count = 0;
  extended_file_in_use = 0;
  initrd_root.name[0] = '\0';
  initrd_root.flags = FS_DIRECTORY;
  initrd_root.create = initrd_create_file;
  fs_root = &initrd_root;

  // 2. Parse TAR Archive from initrd_start to initrd_end
  const char *ptr = initrd_start;

  while (ptr < initrd_end) {
    tar_header_t *header = (tar_header_t *)ptr;
    if (header->name[0] == '\0')
      break;

    uint32_t file_size = oct2bin(header->size, 11);

    const char *entry_name = header->name;
    if (entry_name[0] == '.' && entry_name[1] == '/') {
      entry_name += 2;
    }
    size_t name_length = strlen(entry_name);
    if (name_length > 0 &&
        !(name_length == 1 && entry_name[0] == '.') &&
        !(name_length == 2 && entry_name[0] == '.' && entry_name[1] == '.')) {
      char path[128];
      if (name_length < sizeof(path)) {
        for (size_t i = 0; i <= name_length; i++) {
          path[i] = entry_name[i];
        }
        if (header->typeflag == '5') {
          while (name_length > 0 && path[name_length - 1] == '/') {
            path[--name_length] = '\0';
          }
          if (name_length > 0) {
            vfs_create(&initrd_root, path, FS_DIRECTORY);
          }
        } else {
          vfs_node_t *node = vfs_create(&initrd_root, path, FS_FILE);
          if (node) {
            const uint8_t *file_data = (const uint8_t *)(ptr + 512);
            initrd_write_file(node, 0, file_size, file_data);
          }
        }
      }
    }

    ptr += 512 + ((file_size + 511) & ~511);
  }

  return &initrd_root;
}