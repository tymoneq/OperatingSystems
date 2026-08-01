# Simple Shell

A minimal Unix-style shell implemented in C.

## Features

- Reads and executes commands in a loop
- Supports single commands and pipelines using `|`
- Handles `cd` as a built-in command
- Uses `execvp()` to launch external programs

## Build

From the repository root:

```sh
make shell
```

This compiles `main.c` and `utils.c` into the executable `main`.

## Run

```sh
./main
```

Type commands at the prompt, for example:

```sh
ls -la
cd /tmp
```

## Notes

- Input is read with `getline()` and split by whitespace
- No advanced job control, input/output redirection, or command history is implemented
- The shell uses `fork()` and `waitpid()` to manage subprocesses
