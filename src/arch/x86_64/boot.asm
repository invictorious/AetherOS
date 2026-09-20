[BITS 16]
[ORG 0x7C00]

KERNEL_SEG     equ 0x1000       ; 0x1000:0x0000 = fisica 0x10000
KERNEL_SECTORS equ 96

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

    ; Destino: ES:BX = 0x1000:0x0000 (fisica 0x10000)
    mov ax, KERNEL_SEG
    mov es, ax
    xor bx, bx

    mov si, KERNEL_SECTORS
    mov ch, 0
    mov dh, 0
    mov cl, 2

.read_one:
    test si, si
    jz .done

    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    add bx, 512
    jnc .no_wrap
    mov ax, es
    add ax, 0x1000
    mov es, ax
.no_wrap:

    dec si

    inc cl
    cmp cl, 19
    jb .read_one
    mov cl, 1
    inc dh
    cmp dh, 2
    jb .read_one
    mov dh, 0
    inc ch
    jmp .read_one

.done:
    ; Marcador 'Q' (fila 2, col 0)
    mov ax, 0xB800
    mov es, ax
    mov byte [es:0x0140], 'Q'
    mov byte [es:0x0141], 0x0F

    call query_e820

    ; Saltar al kernel: 0x1000:0x0000 = fisica 0x10000
    jmp KERNEL_SEG:0x0000

disk_error:
    mov si, msg_error
    call imprimir
    cli
    hlt

query_e820:
    xor ax, ax
    mov es, ax
    mov dword [0x500], 0
    mov di, 0x504
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
    inc dword [0x500]
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
