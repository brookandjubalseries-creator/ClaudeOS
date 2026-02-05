/**
 * ClaudeOS Interrupt Descriptor Table - idt.h
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: IDT structures and interrupt handling
 */

#ifndef _CLAUDEOS_IDT_H
#define _CLAUDEOS_IDT_H

#include "types.h"

/* IDT entry (interrupt gate descriptor) - 8 bytes for 32-bit */
typedef struct {
    uint16_t base_low;      /* Lower 16 bits of handler address */
    uint16_t selector;      /* Kernel code segment selector */
    uint8_t  zero;          /* Always zero */
    uint8_t  flags;         /* Type and attributes */
    uint16_t base_high;     /* Upper 16 bits of handler address */
} __attribute__((packed)) idt_entry_t;

/* IDT pointer structure for LIDT instruction */
typedef struct {
    uint16_t limit;         /* Size of IDT - 1 */
    uint32_t base;          /* Base address of IDT */
} __attribute__((packed)) idt_ptr_t;

/* IDT flags */
#define IDT_FLAG_PRESENT    0x80    /* Entry is present */
#define IDT_FLAG_DPL0       0x00    /* Ring 0 (kernel) */
#define IDT_FLAG_DPL3       0x60    /* Ring 3 (user) */
#define IDT_FLAG_INT_GATE   0x0E    /* 32-bit interrupt gate */
#define IDT_FLAG_TRAP_GATE  0x0F    /* 32-bit trap gate */

/* Standard kernel interrupt gate */
#define IDT_KERNEL_INT  (IDT_FLAG_PRESENT | IDT_FLAG_DPL0 | IDT_FLAG_INT_GATE)

/* Number of IDT entries */
#define IDT_ENTRIES     256

/* CPU exception handlers (0-31) */
#define INT_DIVIDE_ERROR        0
#define INT_DEBUG               1
#define INT_NMI                 2
#define INT_BREAKPOINT          3
#define INT_OVERFLOW            4
#define INT_BOUND_EXCEEDED      5
#define INT_INVALID_OPCODE      6
#define INT_NO_COPROCESSOR      7
#define INT_DOUBLE_FAULT        8
#define INT_COPROCESSOR_SEG     9
#define INT_INVALID_TSS         10
#define INT_SEGMENT_NOT_PRESENT 11
#define INT_STACK_FAULT         12
#define INT_GENERAL_PROTECTION  13
#define INT_PAGE_FAULT          14
#define INT_RESERVED            15
#define INT_COPROCESSOR_ERROR   16

/* Hardware IRQs (remapped to 32-47) */
#define IRQ_BASE        32
#define IRQ0            (IRQ_BASE + 0)   /* Timer */
#define IRQ1            (IRQ_BASE + 1)   /* Keyboard */
#define IRQ2            (IRQ_BASE + 2)   /* Cascade */
#define IRQ3            (IRQ_BASE + 3)   /* COM2 */
#define IRQ4            (IRQ_BASE + 4)   /* COM1 */
#define IRQ5            (IRQ_BASE + 5)   /* LPT2 */
#define IRQ6            (IRQ_BASE + 6)   /* Floppy */
#define IRQ7            (IRQ_BASE + 7)   /* LPT1 */
#define IRQ8            (IRQ_BASE + 8)   /* RTC */
#define IRQ12           (IRQ_BASE + 12)  /* PS/2 Mouse */
#define IRQ14           (IRQ_BASE + 14)  /* Primary ATA */
#define IRQ15           (IRQ_BASE + 15)  /* Secondary ATA */

/* Initialize the IDT */
void idt_init(void);

/* Set an IDT entry */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);

/* Register an interrupt handler */
typedef void (*interrupt_handler_t)(void);
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler);

#endif /* _CLAUDEOS_IDT_H */
