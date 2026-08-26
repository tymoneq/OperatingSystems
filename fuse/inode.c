#define FUSE_USE_VERSION 31
// fuse must be first

#include "inode.h"
#include "fuse.h"
#include <stdlib.h>

int InodeTable(struct InodeTable* table) {
  table =
      (struct InodeTable*)malloc(MAX_NUMBER_OF_INODES * sizeof(struct inode));

  if (table == NULL)
    return -1;

  table->next_inode = 3;
  
  return 0;
}

static const struct fuse_operations ram_oper = {

};

int main(int argc, char* argv[]) {
  return fuse_main(argc, argv, &ram_oper, NULL);
}
