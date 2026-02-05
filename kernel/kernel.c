/**
 * ClaudeOS Kernel - kernel.c
 * Author: Worker1 (Kernel+Driver Claude)
 * Description: Main kernel entry point for ClaudeOS
 */

#include "types.h"
#include "vga.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "kmalloc.h"
#include "timer.h"
#include "process.h"
#include "syscall.h"

/* External functions from other components */
extern void vfs_init(void);      /* From /fs/ramfs.c */
extern void shell_main(void);    /* From /shell/shell.c */

/* Kernel version */
#define KERNEL_VERSION "0.2.0"

/**
 * Enable interrupts
 */
static inline void sti(void) {
    __asm__ volatile ("sti");
}

/**
 * Halt CPU
 */
static inline void hlt(void) {
    __asm__ volatile ("hlt");
}

/**
 * kernel_main - Entry point called from boot.asm
 *
 * This is the first C code executed after the bootloader
 * sets up the protected mode environment.
 */
void kernel_main(void) {
    /* Initialize VGA display first so we can see output */
    vga_init();
    vga_clear();

    /* Boot banner */
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("================================================================================\n");
    vga_puts("                           ClaudeOS v");
    vga_puts(KERNEL_VERSION);
    vga_puts(" booting...\n");
    vga_puts("                        Built by MultiClaude Team\n");
    vga_puts("================================================================================\n\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    /* Initialize IDT (Interrupt Descriptor Table) */
    idt_init();

    /* Initialize PIC (Programmable Interrupt Controller) */
    pic_init();

    /* Initialize PS/2 keyboard driver */
    keyboard_init();

    /* Initialize kernel heap */
    kmalloc_init();

    /* Initialize PIT timer (100 Hz) */
    timer_init();

    /* Initialize system call interface */
    syscall_init();

    /* Initialize virtual filesystem */
    vfs_init();

    /* Initialize process scheduler */
    process_init();

    /* All systems go! */
    vga_puts("\n");
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_puts("[KERNEL] All systems initialized successfully!\n");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_puts("[KERNEL] Starting shell...\n\n");

    /* Enable interrupts - CRITICAL! Keyboard won't work without this */
    sti();

    /* Hand off to shell - this is where the user interacts */
    shell_main();

    /* Shell should never return, but if it does, halt gracefully */
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_puts("\n[KERNEL] Shell exited. System halted.\n");
    vga_puts("[KERNEL] Press reset button to restart.\n");

    /* Halt forever */
    for (;;) {
        hlt();
    }
}

/**
 * Kernel panic - called on unrecoverable errors
 */
void kernel_panic(const char* message) {
    /* Disable interrupts */
    __asm__ volatile ("cli");

    /* Red screen of death */
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_clear();
    vga_puts("\n\n");
    vga_puts("  ================================================================================\n");
    vga_puts("                              KERNEL PANIC\n");
    vga_puts("  ================================================================================\n\n");
    vga_puts("  Error: ");
    vga_puts(message);
    vga_puts("\n\n");
    vga_puts("  The system has been halted to prevent damage.\n");
    vga_puts("  Please restart your computer.\n");
    vga_puts("\n");
    vga_puts("  ================================================================================\n");

    /* Halt forever */
    for (;;) {
        hlt();
    }
}

/**
 * Reboot the system - called by shell 'reboot' command
 */
void kernel_reboot(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_puts("\n[KERNEL] Rebooting system...\n");

    /* Method 1: Triple fault by loading invalid IDT */
    /* This causes a reset on most x86 systems */
    static const uint8_t null_idt[6] = {0};
    __asm__ volatile (
        "cli\n"
        "lidt %0\n"     /* Load null IDT */
        "int $3\n"      /* Trigger interrupt with no handler -> triple fault */
        : : "m"(null_idt)
    );

    /* Method 2: Keyboard controller reset (fallback) */
    uint8_t good = 0x02;
    while (good & 0x02) {
        __asm__ volatile ("inb $0x64, %0" : "=a"(good));
    }
    __asm__ volatile ("outb %0, $0x64" : : "a"((uint8_t)0xFE));

    /* If we're still here, just halt */
    for (;;) {
        hlt();
    }
}
