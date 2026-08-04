# Hack VM

A small educational project for learning how virtual memory works in Linux.

This repository explores the idea that a process does not directly access physical memory. Instead, the operating system gives each process its own virtual address space, and the kernel maps that space to real memory through page tables.

## What you will learn

- how a process sees its own memory layout
- the difference between virtual addresses and physical memory
- how the heap is managed at runtime
- how /proc entries can reveal memory mappings

## Project files

- simple.c: a tiny C program that allocates a string on the heap and keeps it alive so you can observe its memory behavior
- read_write_heap.go: a small Go program that reads and writes a process heap using /proc/<pid>/maps and /proc/<pid>/mem

## How it works

The Go tool inspects the target process memory mappings, finds the heap region, and then reads or overwrites bytes in that region. This is a hands-on way to understand how memory is mapped and how processes interact with their own address space.

## Quick start

1. Build the Go tool:
   ```bash
   go build -o read_write_heap read_write_heap.go
   ```
2. Run the sample program:
   ```bash
   gcc -o simple simple.c
   ./simple
   ```
3. In another terminal, run the memory tool with sudo:
   ```bash
   sudo ./read_write_heap <pid> "Holberton" "Hello"
   ```

> This project is meant for learning and experimentation. Use it only on processes you control.
