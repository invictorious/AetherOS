#include "idt.h"
#include "../../drivers/pic.h"
#include "../../kernel/syscall.h"

#define IDT_ENTRIES 256

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;
static isr_t       handlers[IDT_ENTRIES] = {0};

extern void idt_load(uint64_t);
extern void isr_default(void);
extern void isr_divide_by_zero(void);
extern void isr_page_fault(void);
extern void isr_irq0(void);
extern void isr_irq1(void);
extern void isr_syscall(void);
extern void isr_win_syscall(void);

extern void console_printf(const char *fmt, ...);
extern void console_write(const char *);

void idt_set_gate(int n, uint64_t handler) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x08;
    idt[n].ist         = 0;
    idt[n].type_attr   = 0x8E;
    idt[n].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].zero        = 0;
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    handlers[n] = handler;
}

void idt_init(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++)
        idt_set_gate(i, (uint64_t)isr_default);

    idt_set_gate(0,  (uint64_t)isr_divide_by_zero);
    idt_set_gate(14, (uint64_t)isr_page_fault);
    idt_set_gate(32, (uint64_t)isr_irq0);
    idt_set_gate(33, (uint64_t)isr_irq1);
    idt_set_gate(0x80, (uint64_t)isr_syscall);
    idt_set_gate(0x81, (uint64_t)isr_win_syscall);
    idt[0x80].type_attr = 0xEE;
    idt[0x81].type_attr = 0xEE;

    idt_load((uint64_t)&idt_ptr);
}

void isr_handler(registers_t *regs) {
    uint64_t n = regs->int_no;

    if (n == 0x80) {
        uint64_t ret = syscall_dispatch(regs->rax, regs->rdi, regs->rsi, regs->rdx);
        regs->rax = ret;
        return;
    }
    if (n == 0x81) {
        extern uint64_t win_dispatch(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
        uint64_t ret = win_dispatch(regs->rax, regs->rcx, regs->rdx, regs->r8, regs->r9);
        regs->rax = ret;
        return;
    }

    if (n < IDT_ENTRIES && handlers[n]) {
        handlers[n](regs);
        return;
    }

    /* Fallback: imprimir en el cursor actual y halt */
    console_printf("\n\n[!!!] EXC %lu  RIP=0x%lx  CS=0x%lx  ERR=0x%lx\n",
                   n, regs->rip, regs->cs, regs->err_code);

    if (n == 14) {
        uint64_t cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        console_printf("[!!!] CR2=0x%lx\n", cr2);
    }

    console_write("[!!!] HALT\n");
    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}
