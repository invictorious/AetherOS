#include <stdint.h>
#include <stddef.h>
#include "../arch/x86_64/idt.h"
#include "../drivers/pic.h"
#include "../drivers/pit.h"
#include "../drivers/keyboard.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "thread.h"
#include "scheduler.h"

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

void print_dec(uint64_t v) {
    if (v == 0) { putchar('0'); return; }
    char buf[24];
    int i = 0;
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) putchar(buf[i]);
}

void vga_put_at(int x, int y, char c, uint8_t color) {
    vga[(y * 80 + x) * 2]     = (uint8_t)c;
    vga[(y * 80 + x) * 2 + 1] = color;
}

/* --- Handlers --- */
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

/* --- Threads de prueba --- */
static void thread_a(void) {
    uint64_t n = 0;
    for (;;) {
        vga_put_at(5, 20, 'A', 0x0A);
        vga_put_at(6, 20, ':', 0x0A);
        vga_put_at(7, 20, (char)('0' + (n % 10)), 0x0A);
        n++;
        for (volatile int i = 0; i < 10000000; i++);
    }
}

static void thread_b(void) {
    uint64_t n = 0;
    for (;;) {
        vga_put_at(20, 20, 'B', 0x0B);
        vga_put_at(21, 20, ':', 0x0B);
        vga_put_at(22, 20, (char)('0' + (n % 10)), 0x0B);
        n++;
        for (volatile int i = 0; i < 10000000; i++);
    }
}

static void thread_c(void) {
    uint64_t n = 0;
    for (;;) {
        vga_put_at(35, 20, 'C', 0x0E);
        vga_put_at(36, 20, ':', 0x0E);
        vga_put_at(37, 20, (char)('0' + (n % 10)), 0x0E);
        n++;
        for (volatile int i = 0; i < 10000000; i++);
    }
}

void kernel_main(void) {
    clear_screen();

    print("================================\n");
    print("  AetherOS v0.0.7\n");
    print("  Multitarea preemptiva\n");
    print("================================\n\n");

    idt_init();
    pic_remap(0x20, 0x28);
    register_interrupt_handler(32, timer_handler);
    register_interrupt_handler(33, keyboard_irq_handler);
    pit_init(100);
    keyboard_init();
    pic_clear_mask(0);
    pic_clear_mask(1);

    pmm_init();
    heap_init();
    print("[OK] Hardware + memoria listos\n");

    scheduler_init();
    print("[OK] Scheduler inicializado\n");

    thread_t *a = thread_create(thread_a, 0, "A");
    thread_t *b = thread_create(thread_b, 0, "B");
    thread_t *c = thread_create(thread_c, 0, "C");

    if (!a || !b || !c) {
        print("[ERR] No se pudieron crear los threads\n");
        for(;;) __asm__ volatile ("hlt");
    }

    scheduler_add(a);
    scheduler_add(b);
    scheduler_add(c);

    print("[OK] 3 threads creados\n\n");
    print("Fila 20: A=verde  B=cyan  C=amarillo\n");
    print("Los contadores avanzan a la vez.\n\n");

    __asm__ volatile ("sti");

    for (;;) __asm__ volatile ("hlt");
}
