# Simple FUSE RAM Filesystem

This project implements a small user-space filesystem with [FUSE 3](https://github.com/libfuse/libfuse). Files and directory metadata are kept in process memory, so the mounted filesystem behaves like a normal filesystem while the program is running, but it does not provide persistent storage.

The repository contains two implementations:

- `inode.c` / `inode.h`: the recommended implementation. It stores files in a fixed inode table and stores directory contents as linked lists of directory entries.
- `ram.c`: an earlier, simpler implementation that stores complete paths in a linked list. It is useful for comparison and experimentation, but it is less complete than the inode-based implementation.

## Features

The inode-based filesystem currently supports:

- Regular files and directories
- File creation and opening
- Reading and writing file contents
- Directory listing with `.` and `..`
- Directory creation and removal
- File removal
- File metadata queries such as `stat` and `ls -l`
- File permission changes with `chmod`
- Timestamp updates used by commands such as `touch`
- Hard links to regular files
- Symbolic links and symbolic-link reads
- Filesystem capacity statistics through `stat -f`
- AddressSanitizer-enabled development builds

## Requirements

The project is intended for Linux and requires:

- GCC or another C compiler compatible with the Makefile
- GNU Make
- FUSE 3 development files and utilities
- `pkg-config`
- `fusermount3`

On Debian or Ubuntu, the usual packages are:

```sh
sudo apt install build-essential pkg-config libfuse3-dev fuse3
```

Your user must also be allowed to use FUSE. On many distributions this means being a member of the `fuse` group, although the exact setup depends on the distribution.

## Building

Build the inode-based filesystem with:

```sh
make inode
```

This runs the equivalent of:

```sh
gcc -Wall -g -fsanitize=address inode.c $(pkg-config fuse3 --cflags --libs) -o inode
```

Build the path-based prototype with:

```sh
make main
```

The `main` target compiles `ram.c` and produces an executable named `ram`. The target name is retained for compatibility with the existing Makefile.

To remove the generated executables manually:

```sh
rm -f inode ram
```

## Running the Filesystem

Create a mount point, then start the inode implementation in the foreground:

```sh
mkdir -p /tmp/myfs
./inode -f /tmp/myfs
```

The terminal running `./inode` remains occupied while the filesystem is mounted. Open another terminal and use `/tmp/myfs` like an ordinary directory:

```sh
printf 'hello from FUSE\n' > /tmp/myfs/hello.txt
cat /tmp/myfs/hello.txt
ls -la /tmp/myfs
stat /tmp/myfs/hello.txt
mkdir /tmp/myfs/data
printf 'record\n' > /tmp/myfs/data/record.txt
find /tmp/myfs -maxdepth 2 -type f -print
```

The `-f` option keeps FUSE in the foreground, which is convenient during development and makes errors visible in the terminal. FUSE options can be added after the executable when needed, for example:

```sh
./inode -f -o default_permissions /tmp/myfs
```

## Unmounting

Unmount the filesystem from another terminal:

```sh
make unmount
```

The Makefile target expects the mount point `/tmp/myfs`. The equivalent direct command is:

```sh
fusermount3 -u /tmp/myfs
```

If the mount is busy, leave any shells whose current directory is inside `/tmp/myfs`, close programs using files there, and retry. A lazy unmount may be useful while experimenting:

```sh
fusermount3 -uz /tmp/myfs
```

When the filesystem process exits, `inode_destroy` frees the in-memory directory entries and file buffers.

## Implementation Overview

### Inode table

`inode.c` maintains a fixed array of `struct inode` values. Inode identifiers start at `3`; inode `2` represents the root directory, while identifiers `1` and `2` are reserved by convention in this project. Each inode records:

- Its numeric inode identifier
- File type and permission bits
- File size
- Link count
- Access and modification timestamps
- Either file data or a linked list of directory entries

The filesystem supports at most `MAX_NUMBER_OF_INODES` entries, currently `256`. Inode identifiers are allocated monotonically and are not reused after deletion during a single run.

### Directory entries and paths

Each directory owns a linked list of `struct dir_entry` values. A directory entry contains a name, an inode number, and a pointer to the next entry. Path lookup walks the path one component at a time, starting at the root directory.

Hard links create another directory entry pointing to the same inode and increment that inode's link count. The inode's data is released only after its final directory entry is removed.

### File data and capacity

Regular-file contents are allocated with `malloc`/`realloc`. The whole filesystem has a logical data limit of `50 MiB`, configured by `RAMFS_MAX_BYTES`. `stat -f` reports capacity in blocks of `4096` bytes, configured by `RAMFS_BLOCK_SIZE`.

The data limit applies to file content, not to the memory used by inode and directory-entry metadata. Sparse writes are supported: writing beyond the current end of a file grows the file and fills the gap with zero bytes.

### FUSE callbacks

The `ram_oper` table connects filesystem actions to FUSE callbacks, including `getattr`, `readdir`, `create`, `open`, `read`, `write`, `mkdir`, `rmdir`, `unlink`, `link`, `symlink`, `readlink`, `chmod`, `utimens`, and `statfs`.

## Important Limitations

This is an educational filesystem rather than a production filesystem:

- All data is volatile and is lost when the filesystem process exits or is unmounted.
- There is no persistence layer, journaling, crash recovery, locking, or concurrent-access protection.
- The inode table and filename length are fixed at compile time.
- Inodes are allocated monotonically, so deleting a file does not make its inode number available again during the same run.
- The implementation does not implement rename operations.
- The filesystem does not implement ownership, group IDs, extended attributes, or access-control lists.
- Directory link counts and some root-directory metadata are simplified.
- Permission enforcement is delegated to FUSE/kernel options and is not a complete filesystem permission model.
- Symbolic links are stored as strings in memory and are intentionally minimal.
- The `ram.c` prototype has a separate data model and should not be treated as behaviorally identical to `inode.c`.

## Project Files

| File | Purpose |
| --- | --- |
| `inode.c` | Main inode-based FUSE filesystem and program entry point |
| `inode.h` | Inode, directory-entry, and filesystem-limit definitions |
| `ram.c` | Earlier path-based FUSE implementation |
| `Makefile` | Build and unmount commands |
| `compile_flags.txt` | Compiler flags used by editor tooling |
| `main` | Existing ELF binary artifact; normally rebuild with `make main` |

## Development and Debugging

Both Makefile build targets use:

- `-Wall` for compiler warnings
- `-g` for debugger symbols
- `-fsanitize=address` for detecting memory errors

For a development run, build and mount the inode implementation as follows:

```sh
make inode
mkdir -p /tmp/myfs
./inode -f /tmp/myfs
```

Exercise the filesystem from a second terminal, then unmount it with `make unmount`. AddressSanitizer diagnostics are printed by the filesystem process, so keep its terminal visible while testing.

## License

No license file is currently included. Add an explicit license before redistributing the project.
