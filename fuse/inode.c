#define FUSE_USE_VERSION 31
// fuse must be first

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <bits/time.h>
#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include <stdlib.h>
#include "fuse.h"
#include "inode.h"

static struct inode inode_table[MAX_NUMBER_OF_INODES];
static unsigned int next_inode_id = 3;  // Inode 1 is often system, 2 is Root
static struct dir_entry* root_dentries = NULL;

static struct dir_entry* find_file(const char* path) {
  struct dir_entry* current_dir_entries = root_dentries;
  while (current_dir_entries != NULL) {
    if (strcmp(path, current_dir_entries->name) == 0) {
      return current_dir_entries;
    }
    current_dir_entries = current_dir_entries->next;
  }

  return NULL;
}

static int inode_create(const char* path,
                        mode_t mode,
                        struct fuse_file_info* fi) {
  (void)fi;

  struct dir_entry* file = find_file(path);
  if (file != NULL)
    return -EEXIST;

  if (next_inode_id >= MAX_NUMBER_OF_INODES)
    return -ENOSPC;

  struct dir_entry* new_file =
      (struct dir_entry*)malloc(sizeof(struct dir_entry));

  if (new_file == NULL)
    return -ENOMEM;

  new_file->ino = next_inode_id;
  next_inode_id += 1;
  strncpy(new_file->name, path, sizeof(new_file->name) - 1);

  inode_table[new_file->ino].ino = new_file->ino;
  inode_table[new_file->ino].mode = mode;
  inode_table[new_file->ino].size = 0;
  inode_table[new_file->ino].link_count = 1;
  clock_gettime(CLOCK_REALTIME, &inode_table[new_file->ino].a_time);
  clock_gettime(CLOCK_REALTIME, &inode_table[new_file->ino].m_time);
  inode_table[new_file->ino].data = NULL;

  new_file->next = root_dentries;
  root_dentries = new_file;

  return 0;
}

static int inode_utimens(const char* path,
                         const struct timespec tv[2],
                         struct fuse_file_info* fi) {
  (void)fi;

  struct dir_entry* file = find_file(path);
  // Make sure the file or root directory actually exists first
  if (strcmp(path, "/") != 0 && file == NULL) {
    return -ENOENT;
  }
  clock_gettime(CLOCK_REALTIME, &inode_table[file->ino].a_time);
  clock_gettime(CLOCK_REALTIME, &inode_table[file->ino].m_time);

  return 0;
}

static int inode_getattr(const char* path,
                         struct stat* stbuf,
                         struct fuse_file_info* fi) {
  (void)fi;

  memset(stbuf, 0, sizeof(struct stat));
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  struct dir_entry* file = find_file(path);

  if (file != NULL) {
    stbuf->st_mode = inode_table[file->ino].mode;
    stbuf->st_nlink = 1;
    stbuf->st_size = inode_table[file->ino].size;
    return 0;
  } else {
    return -ENOENT;
  }
}

static int inode_readdir(const char* path,
                         void* buf,
                         fuse_fill_dir_t filler,
                         off_t offset,
                         struct fuse_file_info* fi,
                         enum fuse_readdir_flags flags) {
  (void)offset;
  (void)fi;
  (void)flags;

  if (strcmp(path, "/") != 0) {
    struct dir_entry* dir = find_file(path);

    if (!dir)
      return -ENONET;
  }

  filler(buf, ".", NULL, 0, FUSE_FILL_DIR_DEFAULTS);
  filler(buf, "..", NULL, 0, FUSE_FILL_DIR_DEFAULTS);

  struct dir_entry* files = root_dentries;

  while (files != NULL) {
    filler(buf, files->name + 1, NULL, 0, FUSE_FILL_DIR_DEFAULTS);

    files = files->next;
  }
  return 0;
}

static void inode_destroy(void* private_data) {
  (void)private_data;

  struct dir_entry* current_file = root_dentries;
  struct dir_entry* next_file = NULL;

  while (current_file != NULL) {
    next_file = current_file->next;

    if (inode_table[current_file->ino].data != NULL) {
      free(inode_table[current_file->ino].data);
    }

    free(current_file);
    current_file = next_file;
  }
}

static const struct fuse_operations ram_oper = {
    .create = inode_create,
    .utimens = inode_utimens,
    .getattr = inode_getattr,
    .readdir = inode_readdir,
    .destroy = inode_destroy,
};

int main(int argc, char* argv[]) {
  struct dir_entry* new_dentry = malloc(sizeof(struct dir_entry));
  if (new_dentry == NULL)
    return -ENOMEM;
  return fuse_main(argc, argv, &ram_oper, NULL);
}
