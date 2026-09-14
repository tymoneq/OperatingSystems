# Operating Systems Projects

This repository contains small projects for exploring operating-system concepts through working programs. Each directory is an independent experiment and has its own source code, build commands, and README.

The projects currently cover:

- Process creation and command execution in a small Unix-like shell
- File systems, inodes, directories, and FUSE callbacks in user space
- Virtual memory, heap mappings, and process memory inspection through `/proc`

These projects are intended for learning and experimentation rather than production use. They are designed primarily for Linux and may require system packages or elevated permissions.

## Repository Layout

```text
.
├── fuse/       In-memory FUSE file system
├── hackVM/     Virtual-memory and process-heap experiments
└── shell/      Minimal Unix-style shell
```

## Projects

### `shell`: Minimal Unix Shell

The shell project implements a small command interpreter in C. It reads a command line, splits it into arguments, starts external programs with `fork()` and `execvp()`, and waits for child processes with `waitpid()`.

It currently includes:

- An interactive command loop
- External command execution
- A `cd` built-in command
- Simple pipelines using `|`

It deliberately does not provide advanced shell features such as job control, command history, input/output redirection, quoting rules, or environment expansion.

Build and run it from the project directory:

```sh
cd shell
make
./main
```

Example commands:

```sh
ls -la
pwd
cd /tmp
```

The compiler command in `shell/Makefile` enables debugging symbols and sanitizers for address, leak, and undefined-behavior checks.

### `fuse`: In-Memory FUSE File System

The FUSE project implements a user-space file system. The recommended implementation in `inode.c` models files and directories with an in-memory inode table and linked directory entries. The older `ram.c` implementation stores complete paths and is kept as a simpler comparison project.

The inode implementation demonstrates:

- Regular files and directories
- File creation, reading, writing, and removal
- Directory listing and path lookup
- File metadata and permissions
- Hard links and symbolic links
- Timestamp updates
- File-system statistics
- AddressSanitizer-assisted development

All data is volatile. Files disappear when the file-system process exits, and the implementation does not provide persistence, journaling, locking, crash recovery, or rename support.

Install the usual Debian or Ubuntu dependencies:

```sh
sudo apt install build-essential pkg-config libfuse3-dev fuse3
```

Build the inode implementation and mount it in a separate temporary directory:

```sh
cd fuse
make inode
mkdir -p /tmp/myfs
./inode -f /tmp/myfs
```

While the file system is running, use another terminal to try it:

```sh
printf 'hello from FUSE\n' > /tmp/myfs/hello.txt
cat /tmp/myfs/hello.txt
mkdir /tmp/myfs/data
ls -la /tmp/myfs
```

Unmount it from another terminal:

```sh
cd fuse
make unmount
```

The older path-based implementation can be built with `make main`; that target produces the executable `ram`. See [fuse/Readme.md](fuse/Readme.md) for implementation details and limitations.

### `hackVM`: Virtual Memory Experiments

This project explores the relationship between a process's virtual address space and the memory managed by the operating system.

It contains:

- `simple.c`, a small C process that allocates a string on the heap and keeps running while printing its address
- `read_write_heap.go`, a Go utility that reads `/proc/<pid>/maps` to locate a target process's heap and uses `/proc/<pid>/mem` to find and overwrite a string

Build the programs:

```sh
cd hackVM
gcc -o simple simple.c
go build -o read_write_heap read_write_heap.go
```

Run the sample process in one terminal and note its process ID:

```sh
./simple
```

In another terminal, replace the string in the running process:

```sh
sudo ./read_write_heap <pid> "Holberton" "Hello"
```

The target string and replacement should fit the allocated memory region. Access to `/proc/<pid>/mem` is restricted on many Linux systems, which is why `sudo` may be required. Use this tool only with processes you own or are explicitly authorized to inspect.

## Requirements

The exact requirements depend on the project:

- A Linux environment
- GCC and GNU Make for the C projects
- AddressSanitizer support in the compiler used by the Makefiles
- FUSE 3 development files and `fusermount3` for `fuse`
- Go for `hackVM`

There is no top-level `Makefile`; build each project from its own directory. Generated binaries are local build artifacts and are not shared between projects.

## Learning Path

A useful order for exploring the repository is:

1. Start with `shell` to see processes, system calls, and parent/child relationships.
2. Continue with `fuse` to see how file-system operations are represented as callbacks and data structures.
3. Finish with `hackVM` to inspect how a running process is mapped into virtual memory.

For project-specific design notes and limitations, read:

- [shell/README.md](shell/README.md)
- [fuse/Readme.md](fuse/Readme.md)
- [hackVM/README.md](hackVM/README.md)

## Status

This is an educational collection that will evolve as new operating-system concepts are implemented. The code favors visibility and experimentation over complete POSIX compatibility, portability, or production-grade error handling.
