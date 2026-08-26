#pragma once

#include <sys/types.h>
#include <time.h>

#define MAX_FILE_NAME 256
#define MAX_NUMBER_OF_INODES 256

struct inode {
  unsigned int ino;
  mode_t mode;
  size_t size;
  unsigned int link_count;
  struct timespec a_time; // last access time
  struct timespec m_time; // last modify time

  char *data;
};

struct dir_entry {
  char name[MAX_FILE_NAME];
  unsigned int ino;
  struct dir_entry *next;
};

struct InodeTable {
  char *inodeTable;
  unsigned int next_inode;
};

int InitTable(struct InodeTable *table);