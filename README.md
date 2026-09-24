# Project - Personal Memory Allocation
This project creates a way for programs to allocate memory on demand, similar to the `malloc` function in C language.

## Objectives
- Keep a list of memory allocations inside the heap where the memory is free.
- Keep track off all allocations made.
- On demand, it can return a new memory area of a given size. If the size is bigger than all existing free blocks, request the OS to grow the heap.
- On demand, it can free a chosen block. Freeing blocks can result in a request to the OS to reduce the size of the heap.
- Thread-safe operation. It can be used by multiple threads at the same time.
