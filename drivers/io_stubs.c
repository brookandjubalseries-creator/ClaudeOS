/*
 * ClaudeOS I/O Stub Implementations
 * Created by Shell Claude for Driver Claude to expand
 *
 * These are minimal implementations that wrap the kernel's VGA functions.
 * Driver Claude should replace these with proper driver implementations
 * that handle hardware directly.
 */

#include "../include/io.h"
#include "../include/vga.h"

/*
 * ===========================================================================
 * DISPLAY FUNCTIONS - Currently wrap VGA driver
 * ===========================================================================
 */

void display_print(const char *str) {
    vga_puts(str);
}

void display_putchar(char c) {
    vga_putchar(c);
}

void display_clear(void) {
    vga_clear();
}

void display_set_cursor(int x, int y) {
    vga_set_cursor((uint8_t)x, (uint8_t)y);
}

int display_get_width(void) {
    return VGA_WIDTH;
}

int display_get_height(void) {
    return VGA_HEIGHT;
}

void display_set_color(display_color_t fg, display_color_t bg) {
    /* Map our color enum to VGA colors (they match 1:1) */
    vga_set_color((vga_color_t)fg, (vga_color_t)bg);
}

/*
 * ===========================================================================
 * KEYBOARD FUNCTIONS - STUB IMPLEMENTATIONS
 * Driver Claude: Replace these with actual PS/2 or USB keyboard driver
 * ===========================================================================
 */

/* Simple keyboard buffer for stub implementation */
static char kbd_buffer[256];
static int kbd_head = 0;
static int kbd_tail = 0;

/* Check if keyboard has a character available */
int keyboard_has_char(void) {
    /* TODO: Driver Claude - check PS/2 keyboard status port */
    return kbd_head != kbd_tail;
}

/* Read a single character (blocking) */
char keyboard_read_char(void) {
    /* TODO: Driver Claude - implement proper keyboard IRQ handler */
    /* For now, busy-wait on keyboard port */
    while (!keyboard_has_char()) {
        /* In real implementation: hlt until IRQ */
        __asm__ volatile("hlt");
    }

    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % 256;
    return c;
}

/* Read a line with basic editing (blocking) */
int keyboard_read_line(char *buffer, size_t max_len) {
    if (!buffer || max_len == 0) return -1;

    size_t pos = 0;

    while (pos < max_len - 1) {
        char c = keyboard_read_char();

        if (c == KEY_ENTER || c == '\n') {
            display_putchar('\n');
            break;
        }
        else if (c == KEY_BACKSPACE || c == '\b') {
            if (pos > 0) {
                pos--;
                /* Erase character on screen */
                display_print("\b \b");
            }
        }
        else if (c >= 32 && c < 127) {
            /* Printable character */
            buffer[pos++] = c;
            display_putchar(c);
        }
        /* TODO: Handle arrow keys for history (KEY_UP, KEY_DOWN) */
    }

    buffer[pos] = '\0';
    return (int)pos;
}

/*
 * ===========================================================================
 * KEYBOARD IRQ HANDLER - STUB
 * Driver Claude: This should be called from your keyboard IRQ handler
 * ===========================================================================
 */
void keyboard_irq_handler(char c) {
    int next = (kbd_head + 1) % 256;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}
