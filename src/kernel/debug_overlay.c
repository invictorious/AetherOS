#include "debug_overlay.h"
#include "../drivers/framebuffer.h"
#include "../drivers/mouse.h"

extern volatile uint64_t ticks;

/* Dibuja un texto pequeno en la esquina inferior derecha */
static void draw_text_small(int x, int y, const char *s, uint32_t fg) {
    while (*s) {
        const uint8_t *glyph = FONT_ADDR + (uint8_t)(*s) * 16;
        for (int row = 0; row < 16; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (0x80 >> col)) fb_put_pixel(x + col, y + row, fg);
            }
        }
        x += 8;
        s++;
    }
}

static int itoa_dec(uint32_t v, char *buf) {
    if (v == 0) { buf[0] = '0'; buf[1] = 0; return 1; }
    char tmp[12]; int n = 0;
    while (v) { tmp[n++] = '0' + (v % 10); v /= 10; }
    for (int i = 0; i < n; i++) buf[i] = tmp[n - 1 - i];
    buf[n] = 0;
    return n;
}

void debug_overlay_draw(void) {
    int w = fb_width();
    int h = fb_height();
    int x = w - 220;
    int y = h - 90;

    fb_fill_rect(x - 4, y - 4, 210, 80, RGB(0, 0, 0));

    char buf[64];
    extern int mouse_x(void);
    extern int mouse_y(void);
    extern uint32_t mouse_get_irq_count(void);
    extern uint32_t mouse_get_packet_count(void);

    draw_text_small(x, y, "Pkt:", RGB(255, 255, 0));
    itoa_dec(mouse_get_packet_count(), buf);
    draw_text_small(x + 40, y, buf, RGB(255, 255, 0));

    draw_text_small(x, y + 20, "X:", RGB(0, 255, 255));
    itoa_dec((uint32_t)mouse_x(), buf);
    draw_text_small(x + 24, y + 20, buf, RGB(0, 255, 255));

    draw_text_small(x + 90, y + 20, "Y:", RGB(0, 255, 255));
    itoa_dec((uint32_t)mouse_y(), buf);
    draw_text_small(x + 114, y + 20, buf, RGB(0, 255, 255));

    draw_text_small(x, y + 40, "IRQ:", RGB(0, 255, 0));
    itoa_dec(mouse_get_irq_count(), buf);
    draw_text_small(x + 40, y + 40, buf, RGB(0, 255, 0));
}
