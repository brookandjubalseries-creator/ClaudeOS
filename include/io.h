/*
 * ClaudeOS I/O Interface
 * Defined by Shell Claude for Driver Claude to implement
 */

#ifndef CLAUDEOS_IO_H
#define CLAUDEOS_IO_H

#include "types.h"

/*
 * Keyboard Interface
 * Driver Claude: Please implement these in /drivers/keyboard.c
 */

/* Read a single character from keyboard (blocking) */
char keyboard_read_char(void);

/* Check if a key is available (non-blocking) */
int keyboard_has_char(void);

/* Read a line with echo and basic line editing (backspace) */
int keyboard_read_line(char *buffer, size_t max_len);

/* Special key codes */
#define KEY_ENTER       '\n'
#define KEY_BACKSPACE   '\b'
#define KEY_TAB         '\t'
#define KEY_ESCAPE      0x1B
#define KEY_UP          0x80
#define KEY_DOWN        0x81
#define KEY_LEFT        0x82
#define KEY_RIGHT       0x83

/*
 * Display Interface
 * Driver Claude: Please implement these in /drivers/display.c
 */

/* Print a null-terminated string */
void display_print(const char *str);

/* Print a single character */
void display_putchar(char c);

/* Clear the screen */
void display_clear(void);

/* Set cursor position (0-indexed) */
void display_set_cursor(int x, int y);

/* Get screen dimensions */
int display_get_width(void);
int display_get_height(void);

/* Color codes for VGA text mode */
typedef enum {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GRAY = 7,
    COLOR_DARK_GRAY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15
} display_color_t;

/* Set text color (foreground and background) */
void display_set_color(display_color_t fg, display_color_t bg);

#endif /* CLAUDEOS_IO_H */
