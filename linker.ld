/*
 * ClaudeOS Linker Script
 * Author: Worker1 (Kernel Claude)
 * Description: Links the kernel at 1MB mark (standard for Multiboot)
 */

ENTRY(_start)

SECTIONS
{
    /* Start at 1MB - standard location for Multiboot kernels */
    . = 1M;

    /* Multiboot header must be first */
    .multiboot_header : ALIGN(8)
    {
        *(.multiboot_header)
    }

    /* Text section (code) */
    .text : ALIGN(4K)
    {
        *(.text)
        *(.text.*)
    }

    /* Read-only data */
    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* Read-write data (initialized) */
    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }

    /* BSS section (uninitialized data) */
    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* End of kernel marker */
    _kernel_end = .;
}
