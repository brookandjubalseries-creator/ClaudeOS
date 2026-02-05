[BITS 16]
[ORG 0x7C00]

start:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    mov ax, 0x0003
    int 0x10
    mov si, banner
    call prt
    mov si, m1
    call prt
shell:
    mov si, ps
    call prt
    mov di, bf
    call gln
    mov si, bf
    cmp byte [si], 0
    je shell
    cmp word [si], 'he'
    je .hlp
    cmp word [si], 'ls'
    je .ls
    cmp word [si], 'cl'
    je .ai
    cmp word [si], 'un'
    je .un
    cmp word [si], 'wh'
    je .wh
    mov si, uk
    call prt
    jmp shell
.hlp:
    mov si, hp
    jmp .p
.ls:
    mov si, lm
    jmp .p
.ai:
    mov si, ai
    jmp .p
.un:
    mov si, un
    jmp .p
.wh:
    mov si, wm
.p:
    call prt
    jmp shell

prt:
    mov ah, 0x0E
.l: lodsb
    test al, al
    jz .d
    int 0x10
    jmp .l
.d: ret

gln:
    xor cx, cx
.l: xor ax, ax
    int 0x16
    cmp al, 13
    je .d
    cmp al, 8
    je .b
    cmp cx, 12
    jge .l
    stosb
    inc cx
    mov ah, 0x0E
    int 0x10
    jmp .l
.b: test cx, cx
    jz .l
    dec di
    dec cx
    mov ax, 0x0E08
    int 0x10
    mov al, ' '
    int 0x10
    mov al, 8
    int 0x10
    jmp .l
.d: mov byte [di], 0
    mov ax, 0x0E0D
    int 0x10
    mov al, 10
    int 0x10
    ret

banner: db 13,10," === CLAUDEOS v0.2.0 ===",13,10
        db " MultiClaude AI Team",13,10,13,10,0
m1: db "Type 'help' for cmds",13,10,13,10,0
ps: db "claude@os$ ",0
hp: db 13,10,"help ls uname whoami claude",13,10,0
lm: db "bin/ etc/ home/ tmp/",13,10,0
ai: db "[AI] Hi! Full ver has 22 cmds",13,10,0
un: db "ClaudeOS 0.2.0 i386",13,10,0
wm: db "claude",13,10,0
uk: db "? Try: help",13,10,0
bf: times 13 db 0

times 510-($-$$) db 0
dw 0xAA55
