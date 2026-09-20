#include "idt.h"
#include "pic.h"

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

#define VGA 0xB8000
static void vga_put(int x, int y, char c, uint8_t color) {
    volatile uint8_t *v = (volatile uint8_t *)VGA;
    v[(y * 80 + x) * 2]     = (uint8_t)c;
    v[(y * 80 + x) * 2 + 1] = color;
}

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

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint64_t)isr_default);
    }

    idt_set_gate(0,  (uint64_t)isr_divide_by_zero);
    idt_set_gate(14, (uint64_t)isr_page_fault);
    idt_set_gate(32, (uint64_t)isr_irq0);
    idt_set_gate(33, (uint64_t)isr_irq1);

    idt_load((uint64_t)&idt_ptr);
}

void isr_handler(registers_t *regs) {
    uint64_t n = regs->int_no;

    if (n < IDT_ENTRIES && handlers[n]) {
        handlers[n](regs);
        return;
    }

    const char *msg = "EXCEPCION";
    int x = 0;
    for (int i = 0; msg[i]; i++) vga_put(x++, 20, msg[i], 0x0C);
    vga_put(x++, 20, ':', 0x0C);
    vga_put(x++, 20, ' ', 0x0C);
    vga_put(x++, 20, '0' + (char)(n / 10), 0x0C);
    vga_put(x++, 20, '0' + (char)(n % 10), 0x0C);

    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}
