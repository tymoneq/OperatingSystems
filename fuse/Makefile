
main: hello_fuse.c
	gcc -Wall -g -fsanitize=address,leak hello_fuse.c `pkg-config fuse3 --cflags --libs` -o main

unmount:
	fusermount3 -u /tmp/myfs