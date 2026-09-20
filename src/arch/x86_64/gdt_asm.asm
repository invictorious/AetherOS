[BITS 64]

global gdt_flush
global tss_flush

; void gdt_flush(uint64_t gdtp_addr)
; RDI = dirección de struct {u16 limit; u64 base;}
gdt_flush:
    lgdt [rdi]
    ; Recargar CS con far return
    push qword 0x08
    lea rax, [rel .reload]
    push rax
    retfq
.reload:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

; void tss_flush(uint16_t tss_selector)
; DI = selector del TSS
tss_flush:
    mov ax, di
    ltr ax
    ret
