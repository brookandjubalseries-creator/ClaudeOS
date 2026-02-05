/**
 * ClaudeOS VGA Text Mode Driver - vga.h
 * Author: Worker1 (Kernel Claude)
 * Note: Basic VGA driver for kernel boot. Driver Claude can expand this.
 */

#ifndef _CLAUDEOS_VGA_H
#define _CLAUDEOS_VGA_H

#include "types.h"

/* VGA text mode dimensions */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

/* VGA colors */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color_t;

/* Initialize VGA text mode */
void vga_init(void);

/* Clear the screen */
void vga_clear(void);

/* Print a single character */
void vga_putchar(char c);

/* Print a null-terminated string */
void vga_puts(const char* str);

/* Set cursor position */
void vga_set_cursor(uint8_t x, uint8_t y);

/* Set text color */
void vga_set_color(vga_color_t fg, vga_color_t bg);

/* Scroll the screen up by one line */
void vga_scroll(void);

#endif /* _CLAUDEOS_VGA_H */
