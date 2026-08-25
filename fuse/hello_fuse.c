#include <asm-generic/errno-base.h>
#include <stdlib.h>
#define FUSE_USE_VERSION 31

#include <asm-generic/errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

struct ram_file {
  char path[256];
  char* content;
  size_t size;
  mode_t mode;
  struct ram_file* next;
};

static struct ram_file* File_list = NULL;

static struct ram_file* find_file(const char* path) {
  struct ram_file* current_node = File_list;
  while (current_node != NULL) {
    if (strcmp(current_node->path, path) == 0) {
      return current_node;
    }
    current_node = current_node->next;
  }
  return NULL;
}

static int ram_create(const char* path,
                      mode_t mode,
                      struct fuse_file_info* fs) {
  if (find_file(path) != NULL)
    return -EEXIST;

  struct ram_file* new_file = (struct ram_file*)malloc(sizeof(struct ram_file));
  if (new_file == NULL)
    return -ENOMEM;

  strncpy(new_file->path, path, sizeof(new_file->path) - 1);
  new_file->content = NULL;
  new_file->mode = mode;
  new_file->size = 0;

  new_file->next = File_list;
  File_list = new_file;
  return 0;
}

// Fix for 'touch': Pretend we updated the file's timestamp
static int ram_utimens(const char* path,
                       const struct timespec tv[2],
                       struct fuse_file_info* fi) {
  (void)fi;
  (void)tv;

  // Make sure the file or root directory actually exists first
  if (strcmp(path, "/") != 0 && find_file(path) == NULL) {
    return -ENOENT;
  }

  // Return 0 to tell the OS "Yes, I successfully updated the time!"
  return 0;
}

static int ram_getattr(const char* path,
                       struct stat* stbuf,
                       struct fuse_file_info* fi) {
  (void)fi;

  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  struct ram_file* file = find_file(path);

  if (file != NULL) {
    stbuf->st_mode = file->mode;
    stbuf->st_nlink = 1;
    stbuf->st_size = file->size;
    return 0;
  } else {
    return -ENOENT;
  }
}

static int ram_readdir(const char* path,
                       void* buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info* fi,
                       enum fuse_readdir_flags flags) {
  (void)offset;
  (void)fi;
  (void)flags;

  if (strcmp(path, "/") != 0)
    return -ENONET;

  filler(buf, ".", NULL, 0, FUSE_FILL_DIR_DEFAULTS);
  filler(buf, "..", NULL, 0, FUSE_FILL_DIR_DEFAULTS);

  struct ram_file* file = File_list;
  while (file != NULL) {
    filler(buf, file->path + 1, NULL, 0, FUSE_FILL_DIR_DEFAULTS);
    file = file->next;
  }

  return 0;
}

static void ram_destroy(void* private_data) {
  (void)private_data;

  struct ram_file* current_file = File_list;
  struct ram_file* next_file = NULL;

  while (current_file != NULL) {
    next_file = current_file->next;
    if (current_file->content != NULL)
      free(current_file->content);
    free(current_file);
    current_file = next_file;
  }

  File_list = NULL;
}

static const struct fuse_operations ram_oper = {
    .getattr = ram_getattr,
    .create = ram_create,
    .utimens = ram_utimens,
    .readdir = ram_readdir,
    .destroy = ram_destroy,
};

int main(int argc, char* argv[]) {
  return fuse_main(argc, argv, &ram_oper, NULL);
}
