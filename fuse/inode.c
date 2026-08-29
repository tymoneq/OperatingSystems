#include <sys/stat.h>
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

static struct dir_entry* compare_names(const char* buffor,
                                       struct dir_entry* current_dir) {
  struct dir_entry* current_file = current_dir;

  while (current_file != NULL) {
    if (strcmp(buffor, current_file->name) == 0) {
      return current_file;
    }
    current_file = current_file->next;
  }
  return NULL;
}

static struct dir_entry* find_file(const char* path,
                                   struct dir_entry*** out_target_list) {
  struct dir_entry** current_list_head = &root_dentries;
  out_target_list = &current_list_head;
  const char* current_letter = path;
  size_t len = 0;
  char buffer[MAX_FILE_NAME];

  if (path[0] == '/')
    current_letter++;

  while (*current_letter != '\0') {
    if (*current_letter == '/') {
      buffer[len] = '\0';
      len = 0;

      struct dir_entry* dir = compare_names(buffer, *current_list_head);
      if (dir == NULL) {
        return NULL;
      }
      //  changing current dir
      unsigned int ino = dir->ino;

      if (inode_table[ino].mode & S_IFDIR) {
        current_list_head = &inode_table[ino].dir_entry;
      }
    } else {
      buffer[len] = *current_letter;
      len++;
      if (len >= MAX_FILE_NAME)
        return NULL;
    }
    current_letter++;
  }
  if (len != 0) {
    buffer[len] = '\0';
  }
  *out_target_list = current_list_head;

  return compare_names(buffer, *current_list_head);
}

static void get_file_name_from_path(const char* path, char* buffer) {
  const char* current_letter = path;
  size_t len = 0;
  buffer[0] = '\0';

  if (path[0] == '/')
    current_letter++;

  while (*current_letter != '\0') {
    if (*current_letter == '/') {
      buffer[len] = '\0';
      len = 0;

    } else {
      buffer[len] = *current_letter;
      len++;
      if (len >= MAX_FILE_NAME) {
        buffer[MAX_FILE_NAME - 1] = '\0';
        return;
      }
    }
    current_letter++;
  }
  if (len != 0) {
    buffer[len] = '\0';
  }

  buffer[MAX_FILE_NAME - 1] = '\0';
}

static int inode_create(const char* path,
                        mode_t mode,
                        struct fuse_file_info* fi) {
  (void)fi;

  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);

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

  get_file_name_from_path(path, new_file->name);

  inode_table[new_file->ino].ino = new_file->ino;
  inode_table[new_file->ino].mode = mode;
  inode_table[new_file->ino].size = 0;
  inode_table[new_file->ino].link_count = 1;
  clock_gettime(CLOCK_REALTIME, &inode_table[new_file->ino].a_time);
  clock_gettime(CLOCK_REALTIME, &inode_table[new_file->ino].m_time);
  inode_table[new_file->ino].file_data = NULL;
  inode_table[new_file->ino].dir_entry = NULL;

  new_file->next = *parent_dir;
  *parent_dir = new_file;

  return 0;
}

static int inode_utimens(const char* path,
                         const struct timespec tv[2],
                         struct fuse_file_info* fi) {
  (void)fi;

  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);
  // Make sure the file or root directory actually exists first
  if (strcmp(path, "/") != 0 && file != NULL) {
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

  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);
  if (file == NULL)
    return -ENOENT;

  if (file != NULL) {
    stbuf->st_nlink = 1;
    stbuf->st_mode = inode_table[file->ino].mode;
    stbuf->st_size = inode_table[file->ino].size;
    return 0;
  } else {
    return -ENOENT;
  }
}

// add oder dir reading
static int inode_readdir(const char* path,
                         void* buf,
                         fuse_fill_dir_t filler,
                         off_t offset,
                         struct fuse_file_info* fi,
                         enum fuse_readdir_flags flags) {
  (void)offset;
  (void)fi;
  (void)flags;

  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);

  if (strcmp(path, "/") != 0) {
    if (!file || !(inode_table[file->ino].mode & S_IFDIR))
      return -ENONET;
  }

  filler(buf, ".", NULL, 0, FUSE_FILL_DIR_DEFAULTS);
  filler(buf, "..", NULL, 0, FUSE_FILL_DIR_DEFAULTS);

  while (parent_dir != NULL) {
    filler(buf, (*parent_dir)->name, NULL, 0, FUSE_FILL_DIR_DEFAULTS);

    *parent_dir = (*parent_dir)->next;
  }
  return 0;
}
static int inode_mkdir(const char* path, mode_t mode) {
  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);

  if (file != NULL)
    return -EEXIST;

  if (next_inode_id >= MAX_NUMBER_OF_INODES)
    return -ENOSPC;

  struct dir_entry* new_dir =
      (struct dir_entry*)malloc(sizeof(struct dir_entry));

  if (new_dir == NULL)
    return -ENOMEM;

  get_file_name_from_path(path, new_dir->name);

  new_dir->ino = next_inode_id;
  next_inode_id += 1;

  inode_table[new_dir->ino].ino = new_dir->ino;
  inode_table[new_dir->ino].mode = mode | S_IFDIR;
  inode_table[new_dir->ino].size = 0;
  inode_table[new_dir->ino].link_count = 1;
  clock_gettime(CLOCK_REALTIME, &inode_table[new_dir->ino].a_time);
  clock_gettime(CLOCK_REALTIME, &inode_table[new_dir->ino].m_time);
  inode_table[new_dir->ino].file_data = NULL;
  inode_table[new_dir->ino].dir_entry = NULL;

  new_dir->next = *parent_dir;
  *parent_dir = new_dir;
  return 0;
}
static void inode_destroy(void* private_data) {
  (void)private_data;

  struct dir_entry* current_file = root_dentries;
  struct dir_entry* next_file = NULL;

  while (current_file != NULL) {
    next_file = current_file->next;

    if (inode_table[current_file->ino].file_data != NULL) {
      free(inode_table[current_file->ino].file_data);
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
    .mkdir = inode_mkdir,
};

int main(int argc, char* argv[]) {
  struct dir_entry* new_dentry = malloc(sizeof(struct dir_entry));
  if (new_dentry == NULL)
    return -ENOMEM;
  return fuse_main(argc, argv, &ram_oper, NULL);
}
