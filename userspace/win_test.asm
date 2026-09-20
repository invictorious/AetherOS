[BITS 64]
global _start

; Syscall Windows-style:
;   RAX = numero
;   RCX = arg1
;   RDX = arg2
;   R8  = arg3
;   R9  = arg4
;   int 0x81

WIN_SYS_WRITEFILE     equ 0x1001
WIN_SYS_EXITPROCESS   equ 0x1003
WIN_SYS_GETSTDHANDLE  equ 0x1004

section .text
_start:
    ; 1. GetStdHandle(STD_OUTPUT_HANDLE)
    mov rax, WIN_SYS_GETSTDHANDLE
    mov rcx, 0xFFFFFFFFFFFFFFF5   ; -11 (STD_OUTPUT_HANDLE)
    int 0x81
    mov rbx, rax                  ; guardar handle

    ; 2. WriteFile(handle, msg, len, NULL)
    mov rax, WIN_SYS_WRITEFILE
    mov rcx, rbx
    lea rdx, [rel msg]
    mov r8, msg_len
    xor r9, r9
    int 0x81

    ; 3. ExitProcess(0)
    mov rax, WIN_SYS_EXITPROCESS
    xor rcx, rcx
    int 0x81

.hang:
    hlt
    jmp .hang

section .rodata
msg db 10, '================================', 10
    db '  Hola desde el ABI de Windows!', 10
    db '  (RCX, RDX, R8, R9 - no RDI)', 10
    db '================================', 10, 10
    db 'Esta llamada usa int 0x81, que es', 10
    db 'lo que usara el cargador PE cuando', 10
    db 'cargue un .exe real de verdad.', 10, 10, 0
msg_len equ $ - msg - 1
