/**
 * ClaudeOS VGA Text Mode Driver - vga.c
 * Author: Worker1 (Kernel Claude)
 * Description: Basic VGA text mode driver for ClaudeOS
 */

#include "types.h"
#include "vga.h"

/* VGA memory-mapped I/O address */
#define VGA_BUFFER ((uint16_t*)0xB8000)

/* VGA I/O ports for cursor control */
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

/* Current state */
static uint8_t vga_row = 0;
static uint8_t vga_col = 0;
static uint8_t vga_color = 0x0F; /* White on black */

/* Helper: Create VGA entry (character + color attribute) */
static inline uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* Helper: Create color attribute */
static inline uint8_t vga_make_color(vga_color_t fg, vga_color_t bg) {
    return fg | (bg << 4);
}

/* Helper: Write to I/O port */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Initialize VGA text mode */
void vga_init(void) {
    vga_row = 0;
    vga_col = 0;
    vga_color = vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/* Clear the screen */
void vga_clear(void) {
    uint16_t blank = vga_entry(' ', vga_color);

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[y * VGA_WIDTH + x] = blank;
        }
    }

    vga_row = 0;
    vga_col = 0;
    vga_set_cursor(0, 0);
}

/* Scroll the screen up by one line */
void vga_scroll(void) {
    uint16_t blank = vga_entry(' ', vga_color);

    /* Move all lines up by one */
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            VGA_BUFFER[(y - 1) * VGA_WIDTH + x] = VGA_BUFFER[y * VGA_WIDTH + x];
        }
    }

    /* Clear the last line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        VGA_BUFFER[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }
}

/* Print a single character */
void vga_putchar(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 8) & ~7;
    } else if (c == '\b') {
        if (vga_col > 0) {
            vga_col--;
            VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', vga_color);
        }
    } else {
        VGA_BUFFER[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_color);
        vga_col++;
    }

    /* Handle line wrap */
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }

    /* Handle scroll */
    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
        vga_row = VGA_HEIGHT - 1;
    }

    vga_set_cursor(vga_col, vga_row);
}

/* Print a null-terminated string */
void vga_puts(const char* str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

/* Set cursor position */
void vga_set_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * VGA_WIDTH + x;

    outb(VGA_CTRL_PORT, 0x0F);
    outb(VGA_DATA_PORT, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_PORT, 0x0E);
    outb(VGA_DATA_PORT, (uint8_t)((pos >> 8) & 0xFF));
}

/* Set text color */
void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_color = vga_make_color(fg, bg);
}
