[BITS 64]
[extern win_dispatch]

global isr_win_syscall

; int 0x81 - syscall con convencion Windows x64
; El usuario pone:
;   RAX = numero de syscall
;   RCX = arg1
;   RDX = arg2
;   R8  = arg3
;   R9  = arg4
;
; Devolvemos en RAX
isr_win_syscall:
    ; Guardar registros que vamos a usar
    push rbx
    push rbp
    push rsi
    push rdi

    ; Mover argumentos a la convencion System V (para C):
    ; C recibe: RDI=nr, RSI=a1, RDX=a2, RCX=a3, R8=a4
    mov rdi, rax        ; nr
    mov rsi, rcx        ; a1
    mov rdx, rdx        ; a2 (ya esta)
    mov rcx, r8         ; a3
    mov r8,  r9         ; a4

    ; Alinear pila
    mov rbp, rsp
    and rsp, 0xFFFFFFFFFFFFFFF0

    call win_dispatch

    mov rsp, rbp

    pop rdi
    pop rsi
    pop rbp
    pop rbx

    iretq
