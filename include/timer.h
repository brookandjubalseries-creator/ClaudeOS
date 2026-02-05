/**
 * ClaudeOS Timer Driver (PIT) - timer.h
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: Programmable Interval Timer interface
 */

#ifndef _CLAUDEOS_TIMER_H
#define _CLAUDEOS_TIMER_H

#include "types.h"

/* PIT I/O ports */
#define PIT_CHANNEL0    0x40    /* Channel 0 data port (IRQ0) */
#define PIT_CHANNEL1    0x41    /* Channel 1 data port (not used) */
#define PIT_CHANNEL2    0x42    /* Channel 2 data port (PC speaker) */
#define PIT_COMMAND     0x43    /* Mode/Command register */

/* PIT Command byte bits */
#define PIT_CMD_CHANNEL0    0x00    /* Select channel 0 */
#define PIT_CMD_CHANNEL1    0x40    /* Select channel 1 */
#define PIT_CMD_CHANNEL2    0x80    /* Select channel 2 */
#define PIT_CMD_LATCH       0x00    /* Latch count value */
#define PIT_CMD_ACCESS_LO   0x10    /* Access low byte only */
#define PIT_CMD_ACCESS_HI   0x20    /* Access high byte only */
#define PIT_CMD_ACCESS_LOHI 0x30    /* Access low then high byte */
#define PIT_CMD_MODE0       0x00    /* Interrupt on terminal count */
#define PIT_CMD_MODE1       0x02    /* Hardware retriggerable one-shot */
#define PIT_CMD_MODE2       0x04    /* Rate generator */
#define PIT_CMD_MODE3       0x06    /* Square wave generator */
#define PIT_CMD_MODE4       0x08    /* Software triggered strobe */
#define PIT_CMD_MODE5       0x0A    /* Hardware triggered strobe */
#define PIT_CMD_BINARY      0x00    /* Binary counter (16-bit) */
#define PIT_CMD_BCD         0x01    /* BCD counter (4 decades) */

/* PIT base frequency (1.193182 MHz) */
#define PIT_BASE_FREQ   1193182

/* Target frequency in Hz */
#define TIMER_FREQ_HZ   100     /* 100 Hz = 10ms per tick */

/* Divisor for target frequency */
#define PIT_DIVISOR     (PIT_BASE_FREQ / TIMER_FREQ_HZ)

/* Milliseconds per tick */
#define MS_PER_TICK     (1000 / TIMER_FREQ_HZ)

/**
 * Initialize the PIT timer
 * Sets channel 0 to generate IRQ0 at TIMER_FREQ_HZ
 */
void timer_init(void);

/**
 * Get current tick count since boot
 * @return Number of timer ticks since initialization
 */
uint64_t timer_get_ticks(void);

/**
 * Sleep for a specified number of milliseconds
 * Uses busy-wait based on tick count
 * @param ms Number of milliseconds to sleep
 */
void timer_sleep_ms(uint32_t ms);

/**
 * Get uptime in seconds since boot
 * @return Seconds since timer_init()
 */
uint32_t timer_get_uptime_seconds(void);

/**
 * Get uptime in milliseconds since boot
 * @return Milliseconds since timer_init()
 */
uint64_t timer_get_uptime_ms(void);

/**
 * Timer tick callback type
 * Can be registered to receive notifications on each tick
 */
typedef void (*timer_callback_t)(uint64_t ticks);

/**
 * Register a callback to be called on each timer tick
 * @param callback Function to call (NULL to disable)
 */
void timer_set_callback(timer_callback_t callback);

#endif /* _CLAUDEOS_TIMER_H */
