#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

/* Info guardada por el bootloader en 0x5400 */
typedef struct __attribute__((packed)) {
    uint32_t magic;      /* 'VBE ' si OK */
    uint32_t addr;       /* framebuffer lineal */
    uint32_t width;
    uint32_t height;
    uint32_t pitch;      /* bytes por linea */
    uint8_t  bpp;
} fb_info_t;

#define FB_INFO ((volatile fb_info_t*)0x5400)
#define FONT_ADDR ((const uint8_t*)0x6000)

int      fb_init(void);
uint32_t fb_width(void);
uint32_t fb_height(void);
void     fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void     fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void     fb_draw_char(uint32_t px, uint32_t py, char c, uint32_t fg, uint32_t bg);
void     fb_clear(uint32_t color);

/* Colores RGB */
#define RGB(r,g,b) (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define COLOR_BLACK   RGB(0,0,0)
#define COLOR_WHITE   RGB(255,255,255)
#define COLOR_RED     RGB(255,0,0)
#define COLOR_GREEN   RGB(0,255,0)
#define COLOR_BLUE    RGB(0,0,255)
#define COLOR_CYAN    RGB(0,255,255)
#define COLOR_YELLOW  RGB(255,255,0)
#define COLOR_MAGENTA RGB(255,0,255)

#endif
