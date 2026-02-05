/**
 * ClaudeOS Kernel Memory Allocator - kmalloc.h
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: Simple bump allocator for kernel heap
 */

#ifndef _CLAUDEOS_KMALLOC_H
#define _CLAUDEOS_KMALLOC_H

#include "types.h"

/* Heap configuration */
#define HEAP_START  0x200000    /* 2MB mark - after kernel */
#define HEAP_SIZE   0x400000    /* 4MB heap */
#define HEAP_END    (HEAP_START + HEAP_SIZE)

/* Initialize the kernel heap */
void kmalloc_init(void);

/* Allocate memory */
void* kmalloc(size_t size);

/* Allocate aligned memory */
void* kmalloc_aligned(size_t size, size_t alignment);

/* Free memory (no-op for bump allocator) */
void kfree(void* ptr);

/* Get heap usage statistics */
size_t kmalloc_used(void);
size_t kmalloc_free(void);

#endif /* _CLAUDEOS_KMALLOC_H */
