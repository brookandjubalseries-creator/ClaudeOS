; ============================================================================
; ClaudeOS Interrupt Service Routines - isr.asm
; Author: Worker1 (Kernel+Driver Claude)
; Description: ISR and IRQ stub handlers
; ============================================================================

section .text

; External C handlers
extern isr_handler
extern irq_handler

; ============================================================================
; IDT Load function
; ============================================================================
global idt_load
idt_load:
    mov eax, [esp + 4]      ; Get IDT pointer from stack
    lidt [eax]              ; Load IDT
    ret

; ============================================================================
; Macro for ISR without error code
; ============================================================================
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0            ; Dummy error code
    push dword %1           ; Interrupt number
    jmp isr_common
%endmacro

; ============================================================================
; Macro for ISR with error code (CPU pushes it)
; ============================================================================
%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push dword %1           ; Interrupt number (error code already on stack)
    jmp isr_common
%endmacro

; ============================================================================
; CPU Exception handlers (ISR 0-31)
; ============================================================================
ISR_NOERR 0     ; Division by zero
ISR_NOERR 1     ; Debug
ISR_NOERR 2     ; NMI
ISR_NOERR 3     ; Breakpoint
ISR_NOERR 4     ; Overflow
ISR_NOERR 5     ; Bound range exceeded
ISR_NOERR 6     ; Invalid opcode
ISR_NOERR 7     ; Device not available
ISR_ERR   8     ; Double fault (has error code)
ISR_NOERR 9     ; Coprocessor segment overrun
ISR_ERR   10    ; Invalid TSS (has error code)
ISR_ERR   11    ; Segment not present (has error code)
ISR_ERR   12    ; Stack-segment fault (has error code)
ISR_ERR   13    ; General protection fault (has error code)
ISR_ERR   14    ; Page fault (has error code)
ISR_NOERR 15    ; Reserved
ISR_NOERR 16    ; x87 FPU error
ISR_ERR   17    ; Alignment check (has error code)
ISR_NOERR 18    ; Machine check
ISR_NOERR 19    ; SIMD floating-point
ISR_NOERR 20    ; Virtualization
ISR_NOERR 21    ; Reserved
ISR_NOERR 22    ; Reserved
ISR_NOERR 23    ; Reserved
ISR_NOERR 24    ; Reserved
ISR_NOERR 25    ; Reserved
ISR_NOERR 26    ; Reserved
ISR_NOERR 27    ; Reserved
ISR_NOERR 28    ; Reserved
ISR_NOERR 29    ; Reserved
ISR_ERR   30    ; Security exception (has error code)
ISR_NOERR 31    ; Reserved

; ============================================================================
; Common ISR handler stub
; ============================================================================
isr_common:
    ; Save all registers
    pusha
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C handler: isr_handler(int_no, err_code)
    ; Stack: [gs][fs][es][ds][pusha][int_no][err_code]
    mov eax, [esp + 48]     ; Error code
    push eax
    mov eax, [esp + 52]     ; Interrupt number
    push eax
    call isr_handler
    add esp, 8

    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popa

    ; Clean up pushed error code and interrupt number
    add esp, 8
    iret

; ============================================================================
; IRQ handlers (remapped to INT 32-47)
; ============================================================================
%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0            ; Dummy error code
    push dword %2           ; IRQ number (not INT number)
    jmp irq_common
%endmacro

IRQ 0, 0
IRQ 1, 1
IRQ 2, 2
IRQ 3, 3
IRQ 4, 4
IRQ 5, 5
IRQ 6, 6
IRQ 7, 7
IRQ 8, 8
IRQ 9, 9
IRQ 10, 10
IRQ 11, 11
IRQ 12, 12
IRQ 13, 13
IRQ 14, 14
IRQ 15, 15

; ============================================================================
; Common IRQ handler stub
; ============================================================================
irq_common:
    ; Save all registers
    pusha
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C handler: irq_handler(irq_no)
    mov eax, [esp + 48]     ; IRQ number
    push eax
    call irq_handler
    add esp, 4

    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popa

    ; Clean up
    add esp, 8
    iret
