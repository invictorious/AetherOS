#include "mouse.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

static void wait_write(void) {
    for (int i = 0; i < 100000; i++) if (!(inb(0x64) & 0x02)) return;
}
static void wait_read(void) {
    for (int i = 0; i < 100000; i++) if (inb(0x64) & 0x01) return;
}
static void mouse_write(uint8_t val) {
    wait_write(); outb(0x64, 0xD4);
    wait_write(); outb(0x60, val);
}
static uint8_t mouse_read(void) {
    wait_read();
    return inb(0x60);
}
/* Lee hasta que el buffer esté vacío */
static void mouse_flush(void) {
    for (int i = 0; i < 100; i++) {
        if (!(inb(0x64) & 0x01)) return;
        inb(0x60);
    }
}

/* Estado */
static uint8_t  packet[6];
static int      packet_idx = 0;
static int      mx = 512, my = 384;
static uint8_t  buttons = 0;
static volatile int changed = 0;
static int      absolute_mode = 0;

/* Contadores de debug */
volatile uint32_t mouse_irq_count    = 0;
volatile uint32_t mouse_byte_count   = 0;
volatile uint32_t mouse_packet_count = 0;

uint32_t mouse_get_irq_count(void)    { return mouse_irq_count; }
uint32_t mouse_get_byte_count(void)   { return mouse_byte_count; }
uint32_t mouse_get_packet_count(void) { return mouse_packet_count; }
int      mouse_get_mode(void) { return absolute_mode; }

/* Valores crudos para debug */
static int raw_x_last = 0, raw_y_last = 0;
int mouse_raw_x(void) { return raw_x_last; }
int mouse_raw_y(void) { return raw_y_last; }

int      mouse_x(void)       { return mx; }
int      mouse_y(void)       { return my; }
uint8_t  mouse_buttons(void) { return buttons; }
int      mouse_changed(void) { int c = changed; changed = 0; return c; }

void mouse_init(void) {
    /* 1. Habilitar puerto auxiliar (raton) */
    wait_write(); outb(0x64, 0xA8);

    /* 2. Leer config */
    wait_write(); outb(0x64, 0x20);
    wait_read();
    uint8_t status = inb(0x60);
    status |= 0x02;      /* IRQ12 ON */
    status &= ~0x20;     /* mouse clock ON */

    /* 3. Escribir config */
    wait_write(); outb(0x64, 0x60);
    wait_write(); outb(0x60, status);

    /* 4. Reset defaults */
    mouse_write(0xF6); mouse_read();

    /* NO activamos modo absoluto (produce desincronizacion).
     * Modo relativo es mas robusto. */
    absolute_mode = 0;

    /* 6. Enable data reporting */
    mouse_write(0xF4); mouse_read();

    /* 7. Limpiar cualquier byte residual */
    mouse_flush();

    packet_idx = 0;
    changed = 0;
}

/* Llamado desde IRQ12 */
void mouse_handler(void) {
    mouse_irq_count++;
    uint8_t data = inb(0x60);
    mouse_byte_count++;

    if (absolute_mode) {
        /* Paquetes de 6 bytes en modo absoluto:
         *   [0] flags
         *   [1] x_low
         *   [2] x_high
         *   [3] y_low
         *   [4] y_high
         *   [5] 0 (siempre)
         */
        if (packet_idx == 0 && !(data & 0x08)) return;
        packet[packet_idx++] = data;
        if (packet_idx < 6) return;
        packet_idx = 0;
        mouse_packet_count++;

        uint8_t flags = packet[0];
        int x_abs = packet[1] | (packet[2] << 8);
        int y_abs = packet[3] | (packet[4] << 8);
        raw_x_last = x_abs;
        raw_y_last = y_abs;

        /* Escalar de 0-32767 a tamaño de pantalla */
        extern uint32_t fb_width(void);
        extern uint32_t fb_height(void);
        uint32_t w = fb_width();
        uint32_t h = fb_height();
        if (w == 0) w = 1024;
        if (h == 0) h = 768;

        int new_mx = (int)((uint64_t)x_abs * w / 32767);
        int new_my = (int)((uint64_t)y_abs * h / 32767);

        if (new_mx != mx || new_my != my) changed = 1;
        mx = new_mx;
        my = new_my;

        uint8_t new_buttons = flags & 0x07;
        if (new_buttons != buttons) changed = 1;
        buttons = new_buttons;
    } else {
        /* Modo relativo (raton PS/2 estandar) */
        if (packet_idx == 0 && !(data & 0x08)) return;
        packet[packet_idx++] = data;
        if (packet_idx < 3) return;
        packet_idx = 0;
        mouse_packet_count++;

        uint8_t flags = packet[0];
        if (flags & 0xC0) return;   /* overflow */

        int dx = (int)(int8_t)packet[1];
        int dy = (int)(int8_t)packet[2];

        /* Acelerar x2 (el raton PS/2 es lento) */
        mx += dx * 2;
        my -= dy * 2;

        if (mx < 0) mx = 0;
        if (my < 0) my = 0;
        if (mx > 1023) mx = 1023;
        if (my > 767)  my = 767;

        uint8_t new_buttons = flags & 0x07;
        if (new_buttons != buttons) changed = 1;
        buttons = new_buttons;

        if (dx || dy) changed = 1;
    }
}
