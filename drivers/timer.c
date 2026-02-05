/**
 * ClaudeOS Timer Driver (PIT) - timer.c
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: Programmable Interval Timer driver for IRQ0
 */

#include "types.h"
#include "timer.h"
#include "idt.h"
#include "vga.h"

/* I/O helpers */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Timer state */
static volatile uint64_t timer_ticks = 0;
static timer_callback_t timer_callback = NULL;

/**
 * Timer interrupt handler (IRQ0)
 * Called at TIMER_FREQ_HZ (100 Hz)
 */
static void timer_handler(void) {
    timer_ticks++;

    /* Call registered callback if any */
    if (timer_callback) {
        timer_callback(timer_ticks);
    }
}

/**
 * Initialize the PIT timer
 */
void timer_init(void) {
    /* Calculate the divisor for desired frequency */
    uint16_t divisor = PIT_DIVISOR;

    /* Send command byte: channel 0, low/high access, mode 3 (square wave), binary */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_ACCESS_LOHI | PIT_CMD_MODE3 | PIT_CMD_BINARY);

    /* Send divisor (low byte first, then high byte) */
    outb(PIT_CHANNEL0, divisor & 0xFF);         /* Low byte */
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);  /* High byte */

    /* Register IRQ0 handler */
    register_interrupt_handler(IRQ0, timer_handler);

    /* Clear tick counter */
    timer_ticks = 0;

    vga_puts("[KERNEL] PIT timer initialized at ");

    /* Print frequency (simple decimal output) */
    char freq_str[8];
    uint32_t freq = TIMER_FREQ_HZ;
    int i = 0;
    do {
        freq_str[i++] = '0' + (freq % 10);
        freq /= 10;
    } while (freq > 0);

    /* Reverse the string */
    for (int j = 0; j < i / 2; j++) {
        char tmp = freq_str[j];
        freq_str[j] = freq_str[i - 1 - j];
        freq_str[i - 1 - j] = tmp;
    }
    freq_str[i] = '\0';

    vga_puts(freq_str);
    vga_puts(" Hz (IRQ0)\n");
}

/**
 * Get current tick count
 */
uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

/**
 * Sleep for specified milliseconds
 * Note: This is a busy-wait. In a real scheduler, this would yield the CPU.
 */
void timer_sleep_ms(uint32_t ms) {
    /* Calculate target tick count */
    uint64_t target_ticks = timer_ticks + (ms / MS_PER_TICK);

    /* Handle case where ms is less than one tick */
    if (ms > 0 && ms < MS_PER_TICK) {
        target_ticks = timer_ticks + 1;
    }

    /* Busy-wait until target reached */
    while (timer_ticks < target_ticks) {
        /* Halt until next interrupt to save power */
        __asm__ volatile ("hlt");
    }
}

/**
 * Get uptime in seconds
 */
uint32_t timer_get_uptime_seconds(void) {
    return (uint32_t)(timer_ticks / TIMER_FREQ_HZ);
}

/**
 * Get uptime in milliseconds
 */
uint64_t timer_get_uptime_ms(void) {
    return timer_ticks * MS_PER_TICK;
}

/**
 * Set timer callback
 */
void timer_set_callback(timer_callback_t callback) {
    timer_callback = callback;
}
