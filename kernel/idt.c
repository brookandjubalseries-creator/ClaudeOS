/**
 * ClaudeOS Interrupt Descriptor Table - idt.c
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: IDT setup and interrupt dispatching
 */

#include "types.h"
#include "idt.h"
#include "vga.h"

/* IDT and pointer */
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

/* Interrupt handler table */
static interrupt_handler_t interrupt_handlers[IDT_ENTRIES];

/* External ISR stubs from assembly */
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

/* IRQ stubs */
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

/* Load IDT - defined in assembly */
extern void idt_load(uint32_t);

/**
 * Set an IDT entry
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector  = selector;
    idt[num].zero      = 0;
    idt[num].flags     = flags;
}

/**
 * Register an interrupt handler
 */
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler) {
    interrupt_handlers[num] = handler;
}

/**
 * Exception names for debugging
 */
static const char* exception_names[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point",
    "Virtualization",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Reserved"
};

/**
 * Common interrupt handler - called from assembly stubs
 */
void isr_handler(uint32_t int_no, uint32_t err_code) {
    if (interrupt_handlers[int_no]) {
        interrupt_handlers[int_no]();
    } else if (int_no < 32) {
        /* Unhandled CPU exception */
        vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
        vga_puts("\n*** KERNEL PANIC ***\n");
        vga_puts("Unhandled exception: ");
        vga_puts(exception_names[int_no]);
        vga_puts("\nError code: ");

        /* Print error code as hex */
        char hex[9] = "00000000";
        for (int i = 7; i >= 0; i--) {
            int digit = err_code & 0xF;
            hex[i] = digit < 10 ? '0' + digit : 'A' + digit - 10;
            err_code >>= 4;
        }
        vga_puts("0x");
        vga_puts(hex);
        vga_puts("\n\nSystem halted.");

        /* Halt forever */
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    }
}

/**
 * Common IRQ handler - called from assembly stubs
 */
void irq_handler(uint32_t irq_no) {
    /* Send EOI to PIC(s) */
    if (irq_no >= 8) {
        /* Send to slave PIC */
        __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
    }
    /* Send to master PIC */
    __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));

    /* Call registered handler */
    uint8_t int_no = IRQ_BASE + irq_no;
    if (interrupt_handlers[int_no]) {
        interrupt_handlers[int_no]();
    }
}

/**
 * Initialize the Interrupt Descriptor Table
 */
void idt_init(void) {
    /* Set up IDT pointer */
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;

    /* Clear IDT and handlers */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
        interrupt_handlers[i] = 0;
    }

    /* Install CPU exception handlers (ISR 0-31) */
    /* Code segment selector is 0x08 (GDT entry 1) */
    idt_set_gate(0,  (uint32_t)isr0,  0x08, IDT_KERNEL_INT);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, IDT_KERNEL_INT);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, IDT_KERNEL_INT);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, IDT_KERNEL_INT);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, IDT_KERNEL_INT);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, IDT_KERNEL_INT);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, IDT_KERNEL_INT);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, IDT_KERNEL_INT);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, IDT_KERNEL_INT);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, IDT_KERNEL_INT);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_KERNEL_INT);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_KERNEL_INT);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_KERNEL_INT);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_KERNEL_INT);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_KERNEL_INT);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_KERNEL_INT);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_KERNEL_INT);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_KERNEL_INT);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_KERNEL_INT);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_KERNEL_INT);
    idt_set_gate(20, (uint32_t)isr20, 0x08, IDT_KERNEL_INT);
    idt_set_gate(21, (uint32_t)isr21, 0x08, IDT_KERNEL_INT);
    idt_set_gate(22, (uint32_t)isr22, 0x08, IDT_KERNEL_INT);
    idt_set_gate(23, (uint32_t)isr23, 0x08, IDT_KERNEL_INT);
    idt_set_gate(24, (uint32_t)isr24, 0x08, IDT_KERNEL_INT);
    idt_set_gate(25, (uint32_t)isr25, 0x08, IDT_KERNEL_INT);
    idt_set_gate(26, (uint32_t)isr26, 0x08, IDT_KERNEL_INT);
    idt_set_gate(27, (uint32_t)isr27, 0x08, IDT_KERNEL_INT);
    idt_set_gate(28, (uint32_t)isr28, 0x08, IDT_KERNEL_INT);
    idt_set_gate(29, (uint32_t)isr29, 0x08, IDT_KERNEL_INT);
    idt_set_gate(30, (uint32_t)isr30, 0x08, IDT_KERNEL_INT);
    idt_set_gate(31, (uint32_t)isr31, 0x08, IDT_KERNEL_INT);

    /* Install IRQ handlers (IRQ 0-15 -> INT 32-47) */
    idt_set_gate(32, (uint32_t)irq0,  0x08, IDT_KERNEL_INT);
    idt_set_gate(33, (uint32_t)irq1,  0x08, IDT_KERNEL_INT);
    idt_set_gate(34, (uint32_t)irq2,  0x08, IDT_KERNEL_INT);
    idt_set_gate(35, (uint32_t)irq3,  0x08, IDT_KERNEL_INT);
    idt_set_gate(36, (uint32_t)irq4,  0x08, IDT_KERNEL_INT);
    idt_set_gate(37, (uint32_t)irq5,  0x08, IDT_KERNEL_INT);
    idt_set_gate(38, (uint32_t)irq6,  0x08, IDT_KERNEL_INT);
    idt_set_gate(39, (uint32_t)irq7,  0x08, IDT_KERNEL_INT);
    idt_set_gate(40, (uint32_t)irq8,  0x08, IDT_KERNEL_INT);
    idt_set_gate(41, (uint32_t)irq9,  0x08, IDT_KERNEL_INT);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_KERNEL_INT);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_KERNEL_INT);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_KERNEL_INT);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_KERNEL_INT);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_KERNEL_INT);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_KERNEL_INT);

    /* Load IDT */
    idt_load((uint32_t)&idt_ptr);

    vga_puts("[KERNEL] IDT initialized (256 entries)\n");
}
