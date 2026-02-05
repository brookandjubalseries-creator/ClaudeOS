/**
 * ClaudeOS PS/2 Keyboard Driver - keyboard.c
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: PS/2 keyboard interrupt handler and scancode translation
 */

#include "types.h"
#include "keyboard.h"
#include "idt.h"
#include "vga.h"

/* I/O helpers */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Keyboard state */
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool capslock_on = false;

/* Keyboard buffer */
#define KB_BUFFER_SIZE 256
static char kb_buffer[KB_BUFFER_SIZE];
static volatile uint32_t kb_buffer_head = 0;
static volatile uint32_t kb_buffer_tail = 0;

/* External function from Shell's io_stubs.c */
extern void keyboard_irq_handler(char c);

/**
 * US keyboard scancode to ASCII lookup table (lowercase)
 */
static const char scancode_to_ascii[128] = {
    0,    0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*',  0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,
    /* Remaining entries are 0 */
};

/**
 * US keyboard scancode to ASCII lookup table (uppercase/shifted)
 */
static const char scancode_to_ascii_shift[128] = {
    0,    0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*',  0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,    0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,   0,
    0,    0,   0,   0,   0,   0,   0,   0,   0,
    /* Remaining entries are 0 */
};

/**
 * Add character to keyboard buffer
 */
static void kb_buffer_put(char c) {
    uint32_t next = (kb_buffer_head + 1) % KB_BUFFER_SIZE;
    if (next != kb_buffer_tail) {
        kb_buffer[kb_buffer_head] = c;
        kb_buffer_head = next;
    }
}

/**
 * Keyboard interrupt handler (IRQ1)
 */
static void keyboard_handler(void) {
    uint8_t scancode = inb(KB_DATA_PORT);

    /* Check for key release (bit 7 set) */
    if (scancode & 0x80) {
        /* Key release */
        uint8_t key = scancode & 0x7F;

        if (key == KEY_LSHIFT || key == KEY_RSHIFT) {
            shift_pressed = false;
        } else if (key == KEY_LCTRL) {
            ctrl_pressed = false;
        } else if (key == KEY_LALT) {
            alt_pressed = false;
        }
    } else {
        /* Key press */
        if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
            shift_pressed = true;
        } else if (scancode == KEY_LCTRL) {
            ctrl_pressed = true;
        } else if (scancode == KEY_LALT) {
            alt_pressed = true;
        } else if (scancode == KEY_CAPSLOCK) {
            capslock_on = !capslock_on;
        } else if (scancode < 128) {
            /* Translate scancode to ASCII */
            char c;
            bool use_shift = shift_pressed;

            /* Caps lock affects letters */
            if (capslock_on) {
                char lower = scancode_to_ascii[scancode];
                if (lower >= 'a' && lower <= 'z') {
                    use_shift = !use_shift;
                }
            }

            if (use_shift) {
                c = scancode_to_ascii_shift[scancode];
            } else {
                c = scancode_to_ascii[scancode];
            }

            if (c != 0) {
                /* Ctrl+C handling */
                if (ctrl_pressed && (c == 'c' || c == 'C')) {
                    c = 3;  /* ASCII ETX (Ctrl+C) */
                }

                /* Add to buffer and notify shell */
                kb_buffer_put(c);
                keyboard_irq_handler(c);
            }
        }
    }
}

/**
 * Initialize keyboard driver
 */
void keyboard_init(void) {
    /* Register IRQ1 handler */
    register_interrupt_handler(IRQ1, keyboard_handler);

    /* Flush keyboard buffer */
    while (inb(KB_STATUS_PORT) & KB_STATUS_OUTPUT) {
        inb(KB_DATA_PORT);
    }

    vga_puts("[KERNEL] PS/2 keyboard initialized (IRQ1)\n");
}

/**
 * Check if a character is available
 */
bool keyboard_haschar(void) {
    return kb_buffer_head != kb_buffer_tail;
}

/**
 * Get next character from buffer (blocking)
 */
char keyboard_getchar(void) {
    /* Wait for character */
    while (kb_buffer_head == kb_buffer_tail) {
        __asm__ volatile ("hlt");
    }

    char c = kb_buffer[kb_buffer_tail];
    kb_buffer_tail = (kb_buffer_tail + 1) % KB_BUFFER_SIZE;
    return c;
}
