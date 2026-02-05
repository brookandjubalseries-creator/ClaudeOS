/**
 * ClaudeOS PIC Driver - pic.c
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: 8259 PIC initialization and control
 */

#include "types.h"
#include "pic.h"
#include "vga.h"

/* I/O port helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    /* Port 0x80 is used for POST codes, safe to write garbage */
    outb(0x80, 0);
}

/**
 * Initialize and remap the PICs
 *
 * By default, IRQ 0-7 are mapped to INT 8-15, which conflicts with
 * CPU exceptions. We remap them to INT 32-47.
 *
 * Master PIC: IRQ 0-7  -> INT 32-39
 * Slave PIC:  IRQ 8-15 -> INT 40-47
 */
void pic_init(void) {
    uint8_t mask1, mask2;

    /* Save current masks */
    mask1 = inb(PIC1_DATA);
    mask2 = inb(PIC2_DATA);

    /* Start initialization sequence (cascade mode) */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: Set vector offset */
    outb(PIC1_DATA, 0x20);      /* Master: IRQ 0-7 -> INT 32-39 */
    io_wait();
    outb(PIC2_DATA, 0x28);      /* Slave: IRQ 8-15 -> INT 40-47 */
    io_wait();

    /* ICW3: Tell PICs about each other */
    outb(PIC1_DATA, 0x04);      /* Master: slave on IRQ2 (bit 2) */
    io_wait();
    outb(PIC2_DATA, 0x02);      /* Slave: cascade identity */
    io_wait();

    /* ICW4: Set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Restore masks (or set to allow all initially) */
    outb(PIC1_DATA, 0x00);      /* Enable all IRQs on master */
    outb(PIC2_DATA, 0x00);      /* Enable all IRQs on slave */

    vga_puts("[KERNEL] PIC remapped (IRQ 0-15 -> INT 32-47)\n");
}

/**
 * Send End-of-Interrupt signal
 */
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

/**
 * Disable specific IRQ (set mask bit)
 */
void pic_set_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

/**
 * Enable specific IRQ (clear mask bit)
 */
void pic_clear_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}
