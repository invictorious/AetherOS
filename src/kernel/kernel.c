#include <stdint.h>
#include <stddef.h>
#include "../arch/x86_64/idt.h"
#include "../arch/x86_64/gdt.h"
#include "../drivers/pic.h"
#include "../drivers/pit.h"
#include "../drivers/keyboard.h"
#include "../drivers/ata.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../fs/fat32.h"
#include "thread.h"
#include "scheduler.h"
#include "process.h"
#include "syscall.h"
#include "console.h"
#include "exec.h"

volatile uint64_t ticks = 0;

extern void user_program(void);
extern void enter_usermode(uint64_t entry, uint64_t user_stack) __attribute__((noreturn));

static uint8_t kernel_stack[16384] __attribute__((aligned(16)));
static uint8_t user_stack[8192]   __attribute__((aligned(16)));

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

void kernel_main(void) {
    __asm__ volatile ("cli");
    console_init();
    console_clear();

    console_printf("================================\n");
    console_printf("  AetherOS v0.3.0\n");
    console_printf("  Procesos en ring 3\n");
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
    for (int i = 0; i < 16; i++) pic_set_mask(i);
    pic_clear_mask(0);
    pic_clear_mask(1);
    console_printf("[OK] IDT + PIC + PIT + teclado\n");

    pmm_init();
    heap_init();
    console_printf("[OK] PMM + Heap\n");

    ata_init();
    if (fat32_init() == 0) console_printf("[OK] FAT32 montado\n");
    else                    console_printf("[!] FAT32 fallo\n");

    syscall_init();
    process_init();
    scheduler_init();
    console_printf("[OK] Syscalls + procesos\n");

    /* Crear el proceso del shell: punto de entrada = user_program */
    process_t *shell = process_create((uint64_t)user_program);
    if (!shell) {
        console_printf("[!] No se pudo crear el shell\n");
        for (;;) __asm__ volatile ("hlt");
    }
    process_add(shell);
    console_printf("[OK] Shell creado (PID %u)\n\n", shell->pid);

    console_printf("Arrancando scheduler...\n\n");

    __asm__ volatile ("sti");
    scheduler_start(shell);

    for (;;) __asm__ volatile ("hlt");
}
