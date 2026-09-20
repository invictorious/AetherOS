[BITS 16]
[extern kernel_main]
global _start

_start:
    mov ax, 0xB800
    mov es, ax
    mov byte [es:0x0000], '1'
    mov byte [es:0x0001], 0x0F

    cli
    a32 lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp dword 0x08:pm32

[BITS 32]
pm32:
    mov byte [0xB8002], '2'
    mov byte [0xB8003], 0x0F

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

    mov byte [0xB8004], '3'
    mov byte [0xB8005], 0x0F

    mov edi, 0x100000
    xor eax, eax
    mov ecx, 4096 * 3 / 4
    rep stosd

    mov edi, 0x100000
    ; PML4[0] -> PDPT  (Present + RW + User)
    mov dword [edi],          0x101007
    mov dword [edi + 4],      0
    ; PDPT[0] -> PD    (Present + RW + User)
    mov dword [edi + 0x1000], 0x102007
    mov dword [edi + 0x1004], 0

    ; PD: 16 paginas de 2 MB con Present+RW+User+PS
    mov edi, 0x102000
    mov eax, 0x00000087        ; P=1 RW=1 US=1 PS=1
    mov ecx, 16
.fill_pd:
    mov dword [edi], eax
    mov dword [edi + 4], 0
    add eax, 0x200000
    add edi, 8
    loop .fill_pd

    mov byte [0xB8006], '4'
    mov byte [0xB8007], 0x0F

    mov eax, 0x100000
    mov cr3, eax

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov byte [0xB8008], '5'
    mov byte [0xB8009], 0x0F

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    mov byte [0xB800A], '6'
    mov byte [0xB800B], 0x0F

    lgdt [gdt64_descriptor]

    mov byte [0xB800C], '7'
    mov byte [0xB800D], 0x0F

    jmp 0x08:long_mode

no_long:
    mov byte [0xB8000], 'X'
    mov byte [0xB8001], 0x0F
    hlt
    jmp $

[BITS 64]
long_mode:
    mov byte [abs 0xB800E], '8'
    mov byte [abs 0xB800F], 0x0F

    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rsp, 0x90000

    mov byte [abs 0xB8010], '9'
    mov byte [abs 0xB8011], 0x0F

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
