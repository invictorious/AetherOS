[BITS 64]
[extern isr_handler]

global idt_load
global isr_default
global isr_divide_by_zero
global isr_page_fault
global isr_irq0
global isr_irq1
global isr_syscall

idt_load:
    lidt [rdi]
    ret

isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    mov rbp, rsp
    and rsp, 0xFFFFFFFFFFFFFFF0
    call isr_handler
    mov rsp, rbp

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

%macro ISR_NOERR 2
global isr_%1
isr_%1:
    push qword 0
    push qword %2
    jmp isr_common
%endmacro

%macro ISR_ERR 2
global isr_%1
isr_%1:
    push qword %2
    jmp isr_common
%endmacro

ISR_NOERR divide_by_zero, 0
ISR_ERR   page_fault,     14
ISR_NOERR irq0,           32
ISR_NOERR irq1,           33
ISR_NOERR irq12,          44

; int 0x80 — desde ring 3 (DPL=3)
ISR_NOERR syscall,        0x80

isr_default:
    push qword 0
    push qword -1
    jmp isr_common
