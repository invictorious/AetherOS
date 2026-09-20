[BITS 64]

global switch_stack

; void switch_stack(uint64_t new_rsp) __attribute__((noreturn))
; RDI = nuevo RSP (apunta a r15 guardado)
switch_stack:
    mov rsp, rdi

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
