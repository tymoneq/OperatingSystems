
main: ram.c
	gcc -Wall -g -fsanitize=address ram.c `pkg-config fuse3 --cflags --libs` -o ram

inode: inode.c inode.h
	gcc -Wall -g -fsanitize=address inode.c `pkg-config fuse3 --cflags --libs` -o inode

unmount:
	fusermount3 -u /tmp/myfs