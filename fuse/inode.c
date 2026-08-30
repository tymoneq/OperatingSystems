#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
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
static size_t total_used_bytes = 0;

static void insert_file_metadata(unsigned int ino, mode_t mode) {
  inode_table[ino].ino = ino;
  inode_table[ino].mode = mode;
  inode_table[ino].size = 0;
  inode_table[ino].link_count = 1;
  clock_gettime(CLOCK_REALTIME, &inode_table[ino].a_time);
  clock_gettime(CLOCK_REALTIME, &inode_table[ino].m_time);
  inode_table[ino].file_data = NULL;
  inode_table[ino].dir_entry = NULL;
}

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
  *out_target_list = current_list_head;
  const char* current_letter = path;
  size_t len = 0;
  char buffer[MAX_FILE_NAME];

  buffer[0] = '\0';

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
        *out_target_list = current_list_head;
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

  insert_file_metadata(new_file->ino, mode | S_IFREG);

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
    stbuf->st_ino = 2;
    return 0;
  }

  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);

  if (file != NULL) {
    stbuf->st_ino = inode_table[file->ino].ino;
    stbuf->st_mode = inode_table[file->ino].mode;
    stbuf->st_size = inode_table[file->ino].size;
    stbuf->st_nlink = inode_table[file->ino].link_count;

    // 4. Map the timestamps so 'touch' and 'ls' work properly
    stbuf->st_atim = inode_table[file->ino].a_time;
    stbuf->st_mtim = inode_table[file->ino].m_time;
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

  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);

  struct dir_entry* current = NULL;

  if (strcmp(path, "/") == 0) {
    current = root_dentries;
  } else {
    if (!file || !(inode_table[file->ino].mode & S_IFDIR)) {
      return -ENOENT;
    }

    current = inode_table[file->ino].dir_entry;
  }
  filler(buf, ".", NULL, 0, FUSE_FILL_DIR_DEFAULTS);
  filler(buf, "..", NULL, 0, FUSE_FILL_DIR_DEFAULTS);

  while (current != NULL) {
    filler(buf, current->name, NULL, 0, FUSE_FILL_DIR_DEFAULTS);

    current = current->next;
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
  insert_file_metadata(new_dir->ino, mode | S_IFDIR);

  new_dir->next = *parent_dir;
  *parent_dir = new_dir;
  return 0;
}
static void inode_destroy(void* private_data) {
  (void)private_data;
  struct dir_entry* current_file = NULL;
  struct dir_entry* next_file = NULL;

  for (size_t i = 0; i < next_inode_id; i++) {
    if (inode_table[i].mode & S_IFDIR) {
      current_file = inode_table[i].dir_entry;
      while (current_file != NULL) {
        next_file = current_file->next;
        free(current_file);
        current_file = next_file;
      }

    } else if (inode_table[i].mode & S_IFREG &&
               inode_table[i].file_data != NULL) {
      free(inode_table[i].file_data);
    }
  }
}

static int inode_write(const char* path,
                       const char* buf,
                       size_t size,
                       off_t offset,
                       struct fuse_file_info* fi) {
  (void)fi;

  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);

  if (file == NULL) {
    return -ENOENT;
  }

  if (inode_table[file->ino].mode & S_IFDIR)
    return -ENOENT;

  size_t new_size = offset + size;

  if (new_size + total_used_bytes > RAMFS_MAX_BYTES)
    return -ENOMEM;

  if (new_size > inode_table[file->ino].size) {
    char* new_content = realloc(inode_table[file->ino].file_data, new_size);
    if (new_content == NULL)
      return -ENOMEM;

    if ((size_t)offset > inode_table[file->ino].size)
      memset(new_content + inode_table[file->ino].size, 0, offset);

    inode_table[file->ino].size = new_size;
    inode_table[file->ino].file_data = new_content;
    total_used_bytes += new_size - inode_table[file->ino].size;
  }

  memcpy(inode_table[file->ino].file_data + offset, buf, size);

  return size;
}

static int inode_read(const char* path,
                      char* buf,
                      size_t size,
                      off_t offset,
                      struct fuse_file_info* fs) {
  (void)fs;
  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);

  if (file == NULL)
    return -ENOENT;

  if (offset >= inode_table[file->ino].size)
    return 0;

  size_t read_size = offset + size;

  if (read_size > inode_table[file->ino].size)
    size = inode_table[file->ino].size - offset;

  memcpy(buf, inode_table[file->ino].file_data + offset, size);
  return size;
}

static int inode_statfs(const char* path, struct statvfs* stbuf) {
  (void)path;

  memset(stbuf, 0, sizeof(struct statvfs));

  stbuf->f_bsize = RAMFS_BLOCK_SIZE;
  stbuf->f_frsize = RAMFS_BLOCK_SIZE;

  size_t total_blocks = RAMFS_MAX_BYTES / RAMFS_BLOCK_SIZE;
  size_t used_blocks =
      (total_used_bytes + RAMFS_BLOCK_SIZE - 1) / RAMFS_BLOCK_SIZE;
  size_t free_blocks =
      (total_blocks > used_blocks) ? total_blocks - used_blocks : 0;

  stbuf->f_blocks = total_blocks;
  stbuf->f_bfree = free_blocks;
  stbuf->f_bavail = free_blocks;

  stbuf->f_files = MAX_NUMBER_OF_INODES;
  stbuf->f_ffree = MAX_NUMBER_OF_INODES - next_inode_id;

  stbuf->f_namemax = MAX_FILE_NAME - 1;

  return 0;
}
static int inode_chmod(const char* path,
                       mode_t mode,
                       struct fuse_file_info* fi) {
  (void)fi;

  struct dir_entry** parent = NULL;
  struct dir_entry* file = find_file(path, &parent);

  if (file == NULL)
    return -ENOENT;

  inode_table[file->ino].mode = mode;
  return 0;
}

static int inode_unlink(const char* path) {}

static int inode_rmdir(const char* path) {
  struct dir_entry** parent = NULL;
  struct dir_entry* file = find_file(path, &parent);

  if (file == NULL)
    return -ENOENT;

  if (!(inode_table[file->ino].mode & S_IFDIR))
    return -ENOTDIR;

  return inode_unlink(path);
}

static int inode_link(const char* from, const char* to) {
  struct dir_entry** parent_dir = NULL;

  struct dir_entry* file = find_file(from, &parent_dir);
  struct dir_entry* hardlink = find_file(to, &parent_dir);

  if (file == NULL)
    return -ENOENT;

  if (inode_table[file->ino].mode & S_IFDIR)
    return -EPERM;

  if (hardlink != NULL)
    return -EEXIST;

  if (parent_dir == NULL)
    return -ENOENT;

  hardlink = (struct dir_entry*)malloc(sizeof(struct dir_entry));
  if (hardlink == NULL) {
    return -ENOMEM;
  }

  hardlink->ino = file->ino;
  get_file_name_from_path(to, hardlink->name);
  inode_table[hardlink->ino].link_count += 1;

  hardlink->next = *parent_dir;
  *parent_dir = hardlink;

  return 0;
}

// Note: 'target' is the path string being saved. 'linkpath' is where the new
// file goes.
static int inode_symlink(const char* target, const char* linkpath) {
  struct dir_entry** parent_dir = NULL;
  struct dir_entry* symlink = find_file(linkpath, &parent_dir);

  if (symlink != NULL)
    return -EEXIST;

  if (parent_dir == NULL)
    return -ENOENT;

  if (next_inode_id >= MAX_NUMBER_OF_INODES)
    return -ENOSPC;

  symlink = (struct dir_entry*)malloc(sizeof(struct dir_entry));
  if (symlink == NULL)
    return -ENOMEM;

  symlink->ino = next_inode_id;
  next_inode_id++;
  get_file_name_from_path(linkpath, symlink->name);

  insert_file_metadata(symlink->ino, S_IFLNK | 0777);

  inode_table[symlink->ino].size = strlen(target);
  inode_table[symlink->ino].file_data = strdup(target);

  symlink->next = *parent_dir;
  *parent_dir = symlink;
  return 0;
}

static int inode_readlink(const char* path, char* buf, size_t size) {
  struct dir_entry** parent_dir = NULL;
  struct dir_entry* file = find_file(path, &parent_dir);

  if (file == NULL)
    return -ENOENT;

  if (!(inode_table[file->ino].mode & S_IFLNK))
    return -EINVAL;

  if (inode_table[file->ino].file_data == NULL)
    return -ENOENT;

  strncpy(buf, (char*)inode_table[file->ino].file_data, size - 1);
  buf[size - 1] = '\0';
  return 0;
}
static const struct fuse_operations ram_oper = {
    .create = inode_create,
    .utimens = inode_utimens,
    .getattr = inode_getattr,
    .readdir = inode_readdir,
    .destroy = inode_destroy,
    .mkdir = inode_mkdir,
    .statfs = inode_statfs,
    .write = inode_write,
    .read = inode_read,
    .chmod = inode_chmod,
    .rmdir = inode_rmdir,
    .unlink = inode_unlink,
    .link = inode_link,
    .symlink = inode_symlink,
    .readlink = inode_readlink,
};

int main(int argc, char* argv[]) {
  inode_table[2].mode = S_IFDIR | 0755;
  inode_table[2].dir_entry = root_dentries;

  return fuse_main(argc, argv, &ram_oper, NULL);
}
