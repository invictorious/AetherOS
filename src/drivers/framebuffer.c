#include "framebuffer.h"

#define CHAR_W 8
#define CHAR_H 16

static volatile uint8_t *fb = 0;
static uint32_t fb_w = 0;
static uint32_t fb_h = 0;
static uint32_t fb_pitch = 0;
static uint8_t  fb_bpp = 0;

int fb_init(void) {
    if (FB_INFO->magic != 0x20454256) return -1;   /* 'VBE ' */
    fb       = (volatile uint8_t*)(uint64_t)FB_INFO->addr;
    fb_w     = FB_INFO->width;
    fb_h     = FB_INFO->height;
    fb_pitch = FB_INFO->pitch;
    fb_bpp   = FB_INFO->bpp;
    if (!fb || fb_w == 0 || fb_h == 0) return -1;
    return 0;
}

uint32_t fb_width(void)  { return fb_w; }
uint32_t fb_height(void) { return fb_h; }

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_w || y >= fb_h) return;
    if (fb_bpp == 32) {
        volatile uint32_t *p = (volatile uint32_t*)(fb + y * fb_pitch + x * 4);
        *p = color;
    } else if (fb_bpp == 24) {
        volatile uint8_t *p = fb + y * fb_pitch + x * 3;
        p[0] = color & 0xFF;
        p[1] = (color >> 8) & 0xFF;
        p[2] = (color >> 16) & 0xFF;
    }
}

uint32_t fb_get_pixel(uint32_t x, uint32_t y) {
    if (x >= fb_w || y >= fb_h) return 0;
    if (fb_bpp == 32) {
        volatile uint32_t *p = (volatile uint32_t*)(fb + y * fb_pitch + x * 4);
        return *p;
    } else if (fb_bpp == 24) {
        volatile uint8_t *p = fb + y * fb_pitch + x * 3;
        return p[0] | (p[1] << 8) | (p[2] << 16);
    }
    return 0;
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (x + w > fb_w) w = fb_w - x;
    if (y + h > fb_h) h = fb_h - y;

    if (fb_bpp == 32) {
        /* Escribir por pares de 32 bits (mas rapido, menos visible el flash) */
        uint64_t color64 = ((uint64_t)color << 32) | color;
        for (uint32_t j = 0; j < h; j++) {
            volatile uint64_t *row = (volatile uint64_t*)(fb + (y + j) * fb_pitch + x * 4);
            uint32_t i = 0;
            /* Escribir de 2 en 2 */
            for (; i + 1 < w; i += 2) {
                row[i/2] = color64;
            }
            /* Si queda 1 pixel suelto */
            if (i < w) {
                volatile uint32_t *rp = (volatile uint32_t*)(fb + (y + j) * fb_pitch + x * 4);
                rp[i] = color;
            }
        }
    } else if (fb_bpp == 24) {
        for (uint32_t j = 0; j < h; j++) {
            volatile uint8_t *row = fb + (y + j) * fb_pitch + x * 3;
            for (uint32_t i = 0; i < w; i++) {
                row[i*3 + 0] = color & 0xFF;
                row[i*3 + 1] = (color >> 8) & 0xFF;
                row[i*3 + 2] = (color >> 16) & 0xFF;
            }
        }
    }
}

void fb_draw_char(uint32_t px, uint32_t py, char c, uint32_t fg, uint32_t bg) {
    const uint8_t *glyph = FONT_ADDR + (uint8_t)c * 16;

    /* Primero pintamos el fondo */
    fb_fill_rect(px, py, CHAR_W, CHAR_H, bg);

    /* Luego los pixeles del caracter */
    for (int row = 0; row < CHAR_H; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < CHAR_W; col++) {
            if (bits & (0x80 >> col)) {
                fb_put_pixel(px + col, py + row, fg);
            }
        }
    }
}

void fb_clear(uint32_t color) {
    fb_fill_rect(0, 0, fb_w, fb_h, color);
}
