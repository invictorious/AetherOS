[BITS 64]
global _start

section .text
_start:
    mov rax, 1              ; SYS_WRITE
    lea rdi, [rel msg1]
    xor rsi, rsi
    xor rdx, rdx
    int 0x80

    mov rax, 2              ; SYS_EXIT
    xor rdi, rdi
    int 0x80
.hang:
    hlt
    jmp .hang

section .rodata
msg1 db 10, '================================', 10
     db '  HOLA desde un binario', 10
     db '  cargado del disco!', 10
     db '================================', 10, 10
     db 'Este codigo NO esta en el kernel.', 10
     db 'Vive en /HOLA.BIN dentro del disco FAT32.', 10
     db 'El kernel lo ha cargado en memoria y ha', 10
     db 'saltado a el en ring 3.', 10, 10, 0
