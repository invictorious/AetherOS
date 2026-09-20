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

#define VGA_BUFFER 0xB8000
#define VGA_COLOR  0x0F
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static size_t cursor = 0;
static volatile uint8_t *vga = (volatile uint8_t *)VGA_BUFFER;
volatile uint64_t ticks = 0;

void clear_screen(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i * 2]     = ' ';
        vga[i * 2 + 1] = VGA_COLOR;
    }
    cursor = 0;
}

void putchar(char c) {
    if (c == '\n') {
        cursor += VGA_WIDTH - (cursor % VGA_WIDTH);
    } else {
        vga[cursor * 2]     = (uint8_t)c;
        vga[cursor * 2 + 1] = VGA_COLOR;
        cursor++;
    }
}

void print(const char *str) { while (*str) putchar(*str++); }

void vga_put_at(int x, int y, char c, uint8_t color) {
    vga[(y * 80 + x) * 2]     = (uint8_t)c;
    vga[(y * 80 + x) * 2 + 1] = color;
}

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

/* Pila del kernel para cuando int 0x80 pase desde ring 3 */
static uint8_t kernel_stack[16384] __attribute__((aligned(16)));

extern void enter_usermode(uint64_t entry, uint64_t user_stack) __attribute__((noreturn));
extern void user_program(void);

/* Programa de usuario en ring 3 */
static uint8_t user_stack[8192] __attribute__((aligned(16)));

void kernel_main(void) {
    __asm__ volatile ("cli");     /* Deshabilitar IRQs hasta que todo este listo */

    clear_screen();

    print("================================\n");
    print("  AetherOS v0.0.8\n");
    print("  Syscalls + Ring 3\n");
    print("================================\n\n");

    /* 1. GDT con segmentos de usuario + TSS */
    gdt_init();
    print("[OK] GDT + TSS listos\n");

    /* Configurar la pila del kernel que usara el CPU al entrar desde ring 3 */
    tss_set_kernel_stack((uint64_t)(kernel_stack + sizeof(kernel_stack)));
    print("[OK] Pila del kernel para syscalls\n");

    /* 2. IDT */
    idt_init();
    pic_remap(0x20, 0x28);
    register_interrupt_handler(32, timer_handler);
    register_interrupt_handler(33, keyboard_irq_handler);
    pit_init(100);
    keyboard_init();
    pic_clear_mask(0);
    pic_clear_mask(1);
    print("[OK] IDT + PIC + PIT + teclado\n");

    /* 3. Memoria */
    pmm_init();
    heap_init();
    print("[OK] PMM + Heap\n");

    /* 4. Syscalls */
    syscall_init();
    print("[OK] Syscalls inicializadas\n");

    print("\nEntrando a ring 3...\n");

    /* 5. Saltar a ring 3 */
    uint64_t user_stack_top = (uint64_t)(user_stack + sizeof(user_stack));
    user_stack_top &= ~0xFULL;

    enter_usermode((uint64_t)user_program, user_stack_top);

    /* Nunca llegamos aqui */
    for (;;) __asm__ volatile ("hlt");
}
