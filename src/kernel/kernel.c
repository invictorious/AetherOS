#include <stdint.h>
#include <stddef.h>
#include "../arch/x86_64/idt.h"
#include "../arch/x86_64/gdt.h"
#include "../drivers/pic.h"
#include "../drivers/pit.h"
#include "../drivers/keyboard.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "thread.h"
#include "scheduler.h"
#include "syscall.h"
#include "console.h"

volatile uint64_t ticks = 0;

static void timer_handler(registers_t *regs) {
    ticks++;
    pic_send_eoi(0);
    scheduler_tick(regs);
}

static void keyboard_irq_handler(registers_t *regs) {
    (void)regs;
    extern void keyboard_handler(void);
    keyboard_handler();
    pic_send_eoi(1);
}

/* Pila del kernel para syscalls desde ring 3 */
static uint8_t kernel_stack[16384] __attribute__((aligned(16)));
static uint8_t user_stack[8192]   __attribute__((aligned(16)));

extern void enter_usermode(uint64_t entry, uint64_t user_stack) __attribute__((noreturn));
extern void user_program(void);

void kernel_main(void) {
    __asm__ volatile ("cli");
    console_init();
    console_clear();

    console_printf("================================\n");
    console_printf("  AetherOS v0.0.9\n");
    console_printf("  Consola + teclado con shift\n");
    console_printf("================================\n\n");

    gdt_init();
    tss_set_kernel_stack((uint64_t)(kernel_stack + sizeof(kernel_stack)));
    console_printf("[OK] GDT + TSS\n");

    idt_init();
    pic_remap(0x20, 0x28);
    register_interrupt_handler(32, timer_handler);
    register_interrupt_handler(33, keyboard_irq_handler);
    pit_init(100);
    keyboard_init();
    pic_clear_mask(0);
    pic_clear_mask(1);
    console_printf("[OK] IDT + PIC + PIT + teclado\n");

    pmm_init();
    heap_init();
    console_printf("[OK] PMM (%lu MB) + Heap (%lu MB)\n",
                   pmm_total_memory() / (1024*1024),
                   (unsigned long)(16));

    syscall_init();
    console_printf("[OK] Syscalls\n");

    console_printf("\nEntrando a ring 3...\n");

    uint64_t user_stack_top = (uint64_t)(user_stack + sizeof(user_stack));
    user_stack_top &= ~0xFULL;
    enter_usermode((uint64_t)user_program, user_stack_top);

    for (;;) __asm__ volatile ("hlt");
}
