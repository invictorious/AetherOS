#include <stdint.h>
#include <stddef.h>
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "keyboard.h"
#include "pmm.h"
#include "heap.h"

#define VGA_BUFFER 0xB8000
#define VGA_COLOR  0x0F
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static size_t cursor = 0;
static volatile uint8_t *vga = (volatile uint8_t *)VGA_BUFFER;
static volatile uint64_t ticks = 0;

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
    } else if (c == '\b') {
        if (cursor > 0) {
            cursor--;
            vga[cursor * 2]     = ' ';
            vga[cursor * 2 + 1] = VGA_COLOR;
        }
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

static void timer_handler(registers_t *regs) { (void)regs; ticks++; pic_send_eoi(0); }
static void keyboard_irq_handler(registers_t *regs) { (void)regs; extern void keyboard_handler(void); keyboard_handler(); pic_send_eoi(1); }

void kernel_main(void) {
    clear_screen();

    print("================================\n");
    print("  AetherOS v0.0.6\n");
    print("  Memoria (PMM + Heap)\n");
    print("================================\n\n");

    idt_init();
    pic_remap(0x20, 0x28);
    register_interrupt_handler(32, timer_handler);
    register_interrupt_handler(33, keyboard_irq_handler);
    pit_init(100);
    keyboard_init();
    pic_clear_mask(0);
    pic_clear_mask(1);
    __asm__ volatile ("sti");
    print("[OK] Hardware listo\n");

    pmm_init();
    print("[OK] PMM listo\n");
    print("     RAM total: "); print_dec(pmm_total_memory() / 1024 / 1024); print(" MB\n");
    print("     RAM libre: "); print_dec(pmm_free_memory()  / 1024 / 1024); print(" MB\n");

    heap_init();
    print("[OK] Heap listo (16 MB en 0x400000)\n");

    print("\n[HEAP] Test:\n");
    char *s1 = (char*)kmalloc(64);
    char *s2 = (char*)kmalloc(64);
    char *s3 = (char*)kmalloc(256);

    if (!s1 || !s2 || !s3) { print("  ERROR: kmalloc devolvio NULL\n"); }
    else {
        for (int i = 0; i < 5; i++) s1[i] = 'A' + i; s1[5] = 0;
        for (int i = 0; i < 5; i++) s2[i] = 'a' + i; s2[5] = 0;
        for (int i = 0; i < 10; i++) s3[i] = '0' + i; s3[10] = 0;

        print("  s1 = "); print(s1); print("\n");
        print("  s2 = "); print(s2); print("\n");
        print("  s3 = "); print(s3); print("\n");
        print("  Heap usado: "); print_dec(heap_used()); print(" bytes\n");

        kfree(s1); kfree(s2); kfree(s3);
        print("  Tras kfree: "); print_dec(heap_used()); print(" bytes\n");
    }

    print("\nEscribe algo:\n> ");
    for (;;) { char c = keyboard_getchar(); putchar(c); }
}
