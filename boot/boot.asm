; ============================================================================
; ClaudeOS Bootloader - boot.asm
; Author: Worker1 (Kernel Claude)
; Description: Multiboot compliant bootloader for ClaudeOS
; ============================================================================

; Multiboot 1 magic numbers (compatible with QEMU's -kernel option)
MBOOT_MAGIC     equ 0x1BADB002
MBOOT_FLAGS     equ 0x00000003  ; Page-align and provide memory map
MBOOT_CHECKSUM  equ -(MBOOT_MAGIC + MBOOT_FLAGS)

; VGA text mode constants
VGA_BUFFER          equ 0xB8000
VGA_WIDTH           equ 80
VGA_HEIGHT          equ 25
WHITE_ON_BLACK      equ 0x0F

section .multiboot_header
align 4
multiboot_header:
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384          ; 16 KB stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Set up stack
    mov esp, stack_top

    ; Clear direction flag
    cld

    ; Push multiboot info pointer (ebx) and magic number (eax)
    push ebx            ; Multiboot info structure
    push eax            ; Magic number

    ; Call our C kernel main
    call kernel_main

    ; If kernel_main returns, halt the CPU
.hang:
    cli
    hlt
    jmp .hang

; ============================================================================
; print_string - Print null-terminated string to VGA buffer
; Input: ESI = pointer to string
; ============================================================================
global print_string
print_string:
    push eax
    push ebx
    push esi

    mov ebx, VGA_BUFFER

.loop:
    lodsb                   ; Load byte from ESI into AL
    test al, al             ; Check if null terminator
    jz .done

    mov ah, WHITE_ON_BLACK  ; Attribute byte
    mov [ebx], ax           ; Write char + attribute to VGA
    add ebx, 2              ; Move to next character cell
    jmp .loop

.done:
    pop esi
    pop ebx
    pop eax
    ret

; ============================================================================
; clear_screen - Clear VGA buffer
; ============================================================================
global clear_screen
clear_screen:
    push eax
    push ecx
    push edi

    mov edi, VGA_BUFFER
    mov ecx, VGA_WIDTH * VGA_HEIGHT
    mov ax, 0x0F20          ; Space character with white on black
    rep stosw

    pop edi
    pop ecx
    pop eax
    ret
