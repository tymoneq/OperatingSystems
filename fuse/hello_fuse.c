#define FUSE_USE_VERSION 31

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <asm-generic/errno.h>
#include <fuse.h>
#include <sys/stat.h>
#include <sys/types.h>

static const char* hello_str = "Hello World!\n";
static const char* hello_path = "/hello";

static int hello_getattr(const char* path,
                         struct stat* stbuf,
                         struct fuse_file_info* fi) {
  (void)fi;
  int res = 0;

  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    // it's the root
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else if (strcmp(path, hello_path) == 0) {
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(hello_str);
  } else {
    res = -ENONET;
  }

  return res;
}

static int hello_readdir(const char* path,
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

  filler(buf, ".", NULL, 0, 0);
  filler(buf, "..", NULL, 0, 0);

  filler(buf, hello_path + 1, NULL, 0, 0);

  return 0;
}

static int hello_read(const char* path,
                      char* buf,
                      size_t size,
                      off_t offset,
                      struct fuse_file_info* fi) {
  (void)fi;
  size_t len;

  if (strcmp(path, hello_path) != 0)
    return -ENONET;

  len = strlen(hello_str);

  if (offset >= len)
    return 0;

  if (offset + size > len)
    size = len - offset;

  memcpy(buf, hello_str + offset, size);

  return size;
}

static const struct fuse_operations hello_oper = {
    .getattr = hello_getattr,
    .readdir = hello_readdir,
    .read = hello_read,
};

int main(int argc, char* argv[]) {
  return fuse_main(argc, argv, &hello_oper, NULL);
}
