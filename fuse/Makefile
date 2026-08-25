
main: main.c
	gcc -Wall -g -fsanitize=address main.c `pkg-config fuse3 --cflags --libs` -o main

unmount:
	fusermount3 -u /tmp/myfs