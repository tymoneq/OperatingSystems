
main: main.c
	gcc -Wall -g -fsanitize=address main.c `pkg-config fuse3 --cflags --libs` -o main

inode: inode.c inode.h
	gcc -Wall -g -fsanitize=address inode.c `pkg-config fuse3 --cflags --libs` -o inode

unmount:
	fusermount3 -u /tmp/myfs