/**
 * ClaudeOS Kernel Memory Allocator - kmalloc.c
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: Simple bump allocator for kernel heap
 *
 * This is a simple bump/watermark allocator. It's fast and simple,
 * but kfree() is a no-op (memory is never reclaimed until reboot).
 * Good enough for a basic OS, can upgrade to free-list later.
 */

#include "types.h"
#include "kmalloc.h"
#include "vga.h"

/* Current allocation pointer */
static uint32_t heap_current = HEAP_START;
static bool heap_initialized = false;

/**
 * Initialize the kernel heap
 */
void kmalloc_init(void) {
    heap_current = HEAP_START;
    heap_initialized = true;
    vga_puts("[KERNEL] Heap initialized (4MB at 0x200000)\n");
}

/**
 * Allocate memory from kernel heap
 */
void* kmalloc(size_t size) {
    if (!heap_initialized || size == 0) {
        return NULL;
    }

    /* Align to 4 bytes for efficiency */
    size = (size + 3) & ~3;

    /* Check if we have enough space */
    if (heap_current + size > HEAP_END) {
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_puts("\n*** KERNEL: OUT OF MEMORY ***\n");
        return NULL;
    }

    /* Bump allocate */
    void* ptr = (void*)heap_current;
    heap_current += size;

    return ptr;
}

/**
 * Allocate aligned memory from kernel heap
 */
void* kmalloc_aligned(size_t size, size_t alignment) {
    if (!heap_initialized || size == 0 || alignment == 0) {
        return NULL;
    }

    /* Align current pointer */
    uint32_t aligned = (heap_current + alignment - 1) & ~(alignment - 1);

    /* Align size too */
    size = (size + 3) & ~3;

    /* Check if we have enough space */
    if (aligned + size > HEAP_END) {
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_puts("\n*** KERNEL: OUT OF MEMORY ***\n");
        return NULL;
    }

    /* Bump allocate from aligned position */
    heap_current = aligned + size;

    return (void*)aligned;
}

/**
 * Free memory - no-op for bump allocator
 * In a real OS, this would return memory to a free list
 */
void kfree(void* ptr) {
    /* No-op for bump allocator */
    (void)ptr;
}

/**
 * Get bytes used in heap
 */
size_t kmalloc_used(void) {
    return heap_current - HEAP_START;
}

/**
 * Get bytes free in heap
 */
size_t kmalloc_free(void) {
    return HEAP_END - heap_current;
}
