[BITS 64]

global enter_usermode

; void enter_usermode(uint64_t entry, uint64_t user_stack)
; RDI = entry point del programa de usuario
; RSI = puntero a la pila de usuario
enter_usermode:
    cli

    mov ax, 0x23          ; user data selector (4*8 | 3 = 0x23)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push qword 0x23       ; SS (user)
    push rsi              ; RSP
    push qword 0x202      ; RFLAGS (IF=1)
    push qword 0x1B       ; CS (user) = 3*8 | 3 = 0x1B
    push rdi              ; RIP = entry

    iretq
