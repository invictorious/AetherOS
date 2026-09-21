#include <stdint.h>
#include <stddef.h>
#include "../arch/x86_64/idt.h"
#include "../arch/x86_64/gdt.h"
#include "../drivers/pic.h"
#include "../drivers/pit.h"
#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"
#include "../drivers/ata.h"
#include "../mm/pmm.h"
#include "../mm/heap.h"
#include "../fs/fat32.h"
#include "thread.h"
#include "scheduler.h"
#include "process.h"
#include "syscall.h"
#include "console.h"
#include "version.h"
#include "../drivers/framebuffer.h"
#include "window.h"
extern void console_set_silent(int s);
#include "debug_overlay.h"

volatile uint64_t ticks = 0;
static int g_terminal_win = -1;
int wm_get_terminal(void) { return g_terminal_win; }

int wm_terminal_focused(void) {
    if (g_terminal_win < 0) return 0;
    if (!wm_get(g_terminal_win)) return 0;
    return wm_focused_id() == g_terminal_win;
}

int kernel_open_terminal(void) {
    /* Si ya existe, solo enfocar */
    if (g_terminal_win >= 0 && wm_get(g_terminal_win)) {
        console_set_silent(0);
        wm_focus(g_terminal_win);
        console_set_window(g_terminal_win);
        wm_draw_all();
        return g_terminal_win;
    }

    /* Crear nueva terminal: reactivar consola primero */
    console_set_silent(0);
    int id = wm_create(40, 40, 740, 560, "Terminal - AetherOS");
    if (id < 0) return -1;
    g_terminal_win = id;
    wm_focus(id);
    console_set_window(id);
    console_clear();
    console_printf("[AetherOS Terminal]\n");
    console_printf("> ");
    wm_draw_all();
    return id;
}

extern void user_program(void);

static void timer_handler(registers_t *regs) {
    ticks++;
    pic_send_eoi(0);

    /* Raton: gestion en cada tick, SIEMPRE (incluso sin ventanas) */
    {
        extern void wm_cursor_hide(void);
        extern void wm_cursor_draw(void);
        extern void wm_handle_mouse(void);

        wm_cursor_hide();
        if (mouse_changed()) {
            wm_handle_mouse();
        }
        wm_cursor_draw();
    }





    scheduler_tick(regs);
}

static void keyboard_irq_handler(registers_t *regs) {
    (void)regs;
    extern void keyboard_handler(void);
    keyboard_handler();
    pic_send_eoi(1);
}

static void mouse_irq_handler(registers_t *regs) {
    (void)regs;
    mouse_handler();
    pic_send_eoi(12);
}

void kernel_main(void) {
    __asm__ volatile ("cli");
    console_init();
    console_clear();

    console_printf(AETHEROS_BANNER_KERNEL);
    console_printf("\n");

    gdt_init();
    tss_set_kernel_stack(0x90000);
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
    pic_clear_mask(2);    /* cascade (necesario para IRQ8-15) */
    pic_clear_mask(12);   /* raton PS/2 */
    register_interrupt_handler(44, mouse_irq_handler);   /* IRQ12 -> vector 44 */
    mouse_init();
    console_printf("[OK] IDT + PIC + PIT + teclado + raton\n");

    pmm_init();
    heap_init();
    console_printf("[OK] PMM + Heap\n");

    ata_init();
    if (fat32_init() == 0) console_printf("[OK] FAT32 montado\n");
    else                    console_printf("[!] FAT32 fallo\n");

    /* Ahora si: crear la ventana TERMINAL (heap ya esta listo) */
    wm_init();
    int term_win = wm_create(40, 40, 740, 560, "Terminal - AetherOS");
    g_terminal_win = term_win;
    wm_focus(term_win);
    wm_draw_all();
    console_set_window(term_win);

    console_printf("[OK] Ventana terminal creada (id %d)\n", term_win);

    syscall_init();
    process_init();
    scheduler_init();
    console_printf("[OK] Syscalls + procesos\n");

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