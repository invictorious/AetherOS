[BITS 16]
[extern kernel_main]
global _start

_start:
    ; Real mode. CS=0x1000, IP=0
    cli
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Marca '1' visible en VGA texto
    mov ax, 0xB800
    mov es, ax
    mov byte [es:0x00], '1'
    mov byte [es:0x01], 0x0F

    ; Reset ES para BIOS
    xor ax, ax
    mov es, ax

    ; 1. Copiar fuente 8x16 de la BIOS a 0x6000
    call get_font

    ; Marca '2'
    mov ax, 0xB800
    mov es, ax
    mov byte [es:0x02], '2'
    mov byte [es:0x03], 0x0F
    xor ax, ax
    mov es, ax

    ; 2. Set VBE mode
    call set_vbe

    ; 3. Pasar a modo protegido
    a32 lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp dword 0x08:pm32

; ============================================================
; get_font: copia 8x16 font (4096 bytes) de BIOS a 0x6000
; ============================================================
get_font:
    mov ax, 0x1130
    mov bh, 0x06
    int 0x10
    ; ES:BP = font, CX = bytes per char
    push ds
    push es
    pop ds
    mov si, bp
    mov ax, 0
    mov es, ax
    mov di, 0x6000
    mov cx, 4096 / 4
    cld
    rep movsd
    pop ds
    ret

; ============================================================
; set_vbe: intenta varios modos hasta que uno funcione
; Guarda info en 0x5400
; ============================================================
set_vbe:
    ; Query controller info (necesario)
    mov ax, 0x4F00
    mov di, 0x5000
    int 0x10
    cmp ax, 0x004F
    jne .fail

    ; Intentar modos en orden
    mov cx, 0x0144        ; 1024x768x32
    call try_mode
    jnc .ok
    mov cx, 0x0143        ; 800x600x32
    call try_mode
    jnc .ok
    mov cx, 0x0118        ; 1024x768x24
    call try_mode
    jnc .ok
    mov cx, 0x0115        ; 800x600x24
    call try_mode
    jnc .ok

.fail:
    mov dword [0x5400], 0
    ret

.ok:
    mov dword [0x5400], 0x20454256   ; 'VBE '
    ret

try_mode:
    push cx
    mov ax, 0x4F01
    mov di, 0x5200
    int 0x10
    pop cx
    cmp ax, 0x004F
    jne .fail

    ; bpp debe ser 24 o 32
    mov al, [0x5200 + 0x19]
    cmp al, 24
    je .setmode
    cmp al, 32
    jne .fail

.setmode:
    mov bx, cx
    or bx, 0x4000         ; linear FB
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne .fail

    ; Guardar info
    mov eax, [0x5200 + 0x28]
    mov [0x5404], eax          ; FB addr
    mov ax, [0x5200 + 0x12]
    movzx eax, ax
    mov [0x5408], eax          ; width
    mov ax, [0x5200 + 0x14]
    movzx eax, ax
    mov [0x540C], eax          ; height
    mov ax, [0x5200 + 0x10]
    movzx eax, ax
    mov [0x5410], eax          ; pitch
    mov al, [0x5200 + 0x19]
    mov [0x5414], al           ; bpp
    clc
    ret

.fail:
    stc
    ret

; ============================================================
[BITS 32]
pm32:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x90000

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_long

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz no_long

    ; Limpiar tablas de paginacion 0x100000-0x104000
    mov edi, 0x100000
    xor eax, eax
    mov ecx, 4096 * 4 / 4
    rep stosd

    mov edi, 0x100000
    mov dword [edi],          0x101007
    mov dword [edi + 4],      0
    mov dword [edi + 0x1000], 0x102007
    mov dword [edi + 0x1004], 0

    ; PD1: 0-32 MB identity, user-accessible
    mov edi, 0x102000
    mov eax, 0x00000087
    mov ecx, 16
.fill_pd1:
    mov dword [edi], eax
    mov dword [edi + 4], 0
    add eax, 0x200000
    add edi, 8
    loop .fill_pd1

    ; PD2: 0xC0000000-0x100000000 (1 GB) para el framebuffer
    mov edi, 0x103000
    mov eax, 0xC0000083           ; kernel-only
    mov ecx, 512
.fill_pd2:
    mov dword [edi], eax
    mov dword [edi + 4], 0
    add eax, 0x200000
    add edi, 8
    loop .fill_pd2

    ; PDPT[3] -> PD2
    mov edi, 0x101000
    mov dword [edi + 3*8],     0x103003
    mov dword [edi + 3*8 + 4], 0

    mov eax, 0x100000
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt64_descriptor]
    jmp 0x08:long_mode

no_long:
    hlt
    jmp $

[BITS 64]
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov rsp, 0x90000

    call kernel_main
    cli
hang64:
    hlt
    jmp hang64

gdt_start:
    dq 0x0
gdt_code_32:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0
gdt_data_32:
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt64_start:
    dq 0x0
gdt64_code:
    dw 0x0
    dw 0x0
    db 0x0
    db 10011010b
    db 10100000b
    db 0x0
gdt64_data:
    dw 0x0
    dw 0x0
    db 0x0
    db 10010010b
    db 10100000b
    db 0x0
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start
