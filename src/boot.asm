[BITS 16]
[ORG 0x7C00]

KERNEL_OFFSET equ 0x1000

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, msg_loading
    call imprimir

    mov bx, KERNEL_OFFSET
    mov ah, 0x02
    mov al, 64
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    call query_e820

    jmp KERNEL_OFFSET

disk_error:
    mov si, msg_error
    call imprimir
    cli
    hlt

query_e820:
    xor ax, ax
    mov es, ax
    mov dword [0x8000], 0
    mov di, 0x8004
    xor ebx, ebx
.loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    int 0x15
    jc .done
    cmp eax, 0x534D4150
    jne .done
    test ecx, ecx
    jz .done
    inc dword [0x8000]
    add di, 24
    test ebx, ebx
    jnz .loop
.done:
    ret

imprimir:
    mov ah, 0x0E
.siguiente:
    lodsb
    cmp al, 0
    je .fin
    int 0x10
    jmp .siguiente
.fin:
    ret

boot_drive  db 0
msg_loading db 'AetherOS: cargando kernel...', 13, 10, 0
msg_error   db 'Error al leer el disco!', 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
