[BITS 64]

global switch_stack

; void switch_stack(uint64_t new_rsp) __attribute__((noreturn))
; RDI = nuevo RSP (apunta a r15 guardado)
switch_stack:
    mov rsp, rdi

    ; Establecer segmentos de usuario (DPL=3) antes de iretq.
    ; En 64-bit el base es 0 para todos, asi que cualquier valor
    ; de DPL correcto sirve tanto para kernel como para user.
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq
