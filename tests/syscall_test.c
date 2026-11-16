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
  if ((strings_equal(old_name, "file") ||
       strings_equal(old_name, "dir/file")) &&
      !strings_equal(new_name, "file")) {
    file_node.name[0] = '\0';
    size_t i = 0;
    for (; new_name[i] != '\0' && i < sizeof(file_node.name) - 1; i++) {
      file_node.name[i] = new_name[i];
    }
    file_node.name[i] = '\0';
    return 0;
  }
  return -1;
}

int initrd_remove_file(const char *filename) {
  if (!filename) {
    return -1;
  }
  if (strings_equal(filename, "file") ||
      strings_equal(filename, "dir/renamed")) {
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

int vfs_readdir(vfs_node_t *directory, uint32_t index,
                vfs_dirent_t *entry) {
  if (!directory || !entry || !(directory->flags & FS_DIRECTORY)) return -1;
  if (directory != &directory_node) return 0;
  if (index != 0) return 0;
  entry->name[0] = 'f'; entry->name[1] = 'i'; entry->name[2] = 'l';
  entry->name[3] = 'e'; entry->name[4] = '\0';
  entry->inode = 42;
  entry->type = FS_FILE;
  return 1;
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
