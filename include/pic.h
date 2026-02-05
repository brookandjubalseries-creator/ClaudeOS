/**
 * ClaudeOS PIC Driver - pic.h
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: 8259 PIC initialization and control
 */

#ifndef _CLAUDEOS_PIC_H
#define _CLAUDEOS_PIC_H

#include "types.h"

/* PIC I/O ports */
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/* PIC commands */
#define PIC_EOI         0x20    /* End of interrupt */

/* ICW1 (Initialization Command Word 1) */
#define ICW1_ICW4       0x01    /* ICW4 needed */
#define ICW1_SINGLE     0x02    /* Single mode (vs cascade) */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 */
#define ICW1_LEVEL      0x08    /* Level triggered mode */
#define ICW1_INIT       0x10    /* Initialization */

/* ICW4 */
#define ICW4_8086       0x01    /* 8086/88 mode */
#define ICW4_AUTO       0x02    /* Auto EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM       0x10    /* Special fully nested */

/* Initialize and remap the PIC */
void pic_init(void);

/* Send EOI to PIC */
void pic_send_eoi(uint8_t irq);

/* Enable/disable specific IRQ */
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

#endif /* _CLAUDEOS_PIC_H */
