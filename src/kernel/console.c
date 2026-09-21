#include "console.h"
#include "../drivers/framebuffer.h"
#include "../kernel/window.h"

#define CHAR_W 8
#define CHAR_H 16
#define COLS_MAX 200
#define ROWS_MAX 200

static const uint32_t palette[16] = {
    RGB(0,   0,   0), RGB(0,   0,   170), RGB(0,   170, 0), RGB(0,   170, 170),
    RGB(170, 0,   0), RGB(170, 0,   170), RGB(170, 85,  0), RGB(170, 170, 170),
    RGB(85,  85,  85), RGB(85,  85,  255), RGB(85,  255, 85), RGB(85,  255, 255),
    RGB(255, 85,  85), RGB(255, 85,  255), RGB(255, 255, 85), RGB(255, 255, 255),
};

static uint32_t fg_color(uint8_t a) { return palette[a & 0x0F]; }
static uint32_t bg_color(uint8_t a) { return palette[(a >> 4) & 0x0F]; }

static int      cols = 0;
static int      rows = 0;
static int      cx = 0;
static int      cy = 0;
static uint8_t  current_attr = 0x0F;

/* Silent mode (para cuando solo queremos ventanas) */
static int      silent_mode = 0;
/* Ventana destino: -1 = framebuffer directo (modo viejo) */
static int      console_win = -1;

void console_set_silent(int s) { silent_mode = s; }
void console_set_window(int id) {
    console_win = id;
    cx = 0; cy = 0;
    if (id >= 0) {
        window_t *w = wm_get(id);
        if (w) {
            cols = w->buf_w / CHAR_W;
            rows = w->buf_h / CHAR_H;
            if (cols < 1) cols = 1;
            if (rows < 1) rows = 1;
        }
    } else {
        /* Volver al modo framebuffer directo */
        cols = fb_width()  / CHAR_W;
        rows = fb_height() / CHAR_H;
    }
}

int console_init(void) {
    if (fb_init() != 0) return -1;
    cols = fb_width()  / CHAR_W;
    rows = fb_height() / CHAR_H;
    cx = 0; cy = 0;
    current_attr = 0x0F;
    console_win = -1;
    return 0;
}

void console_clear(void) {
    if (console_win >= 0) {
        /* Limpiar contenido de la ventana */
        window_t *w = wm_get(console_win);
        if (w) {
            wm_fill(console_win, 0, 0, w->buf_w, w->buf_h, w->content_bg);
        }
    } else {
        fb_clear(bg_color(current_attr));
    }
    cx = 0; cy = 0;
}

/* Dibuja un caracter en (col,row) usando el modo adecuado */
static void putc_at(int col, int row, char c, uint8_t attr) {
    if (console_win >= 0) {
        window_t *w = wm_get(console_win);
        uint32_t bg = w ? w->content_bg : RGB(200,200,200);
        /* 1. Rellenar fondo del caracter */
        wm_fill(console_win, col * CHAR_W, row * CHAR_H, CHAR_W, CHAR_H, bg);
        /* 2. Dibujar el caracter con color negro */
        if (c != ' ') {
            wm_draw_text(console_win, col * CHAR_W, row * CHAR_H, (char[]){c,0}, RGB(0,0,0));
        }
    } else {
        /* Modo viejo: escribir en framebuffer directo */
        const uint8_t *glyph = FONT_ADDR + (uint8_t)c * 16;
        int px = col * CHAR_W, py = row * CHAR_H;
        fb_fill_rect(px, py, CHAR_W, CHAR_H, bg_color(attr));
        for (int r = 0; r < 16; r++) {
            uint8_t bits = glyph[r];
            for (int k = 0; k < 8; k++) {
                if (bits & (0x80 >> k)) fb_put_pixel(px + k, py + r, fg_color(attr));
            }
        }
    }
}

static void console_scroll(void) {
    if (console_win >= 0) {
        /* Scroll dentro de la ventana: mover todo 1 linea arriba */
        window_t *w = wm_get(console_win);
        if (!w || !w->content_buf) return;
        int W = w->buf_w;
        int H = w->buf_h;
        int line_px = CHAR_H;
        /* Mover hacia arriba por bytes */
        for (int y = 0; y < H - line_px; y++) {
            for (int x = 0; x < W; x++) {
                w->content_buf[y * W + x] = w->content_buf[(y + line_px) * W + x];
            }
        }
        /* Limpiar la ultima linea */
        for (int y = H - line_px; y < H; y++)
            for (int x = 0; x < W; x++)
                w->content_buf[y * W + x] = bg_color(current_attr);
        /* Redibujar la ventana */
        wm_draw_all();
    } else {
        /* Modo viejo: scroll framebuffer */
        uint32_t pitch = *(uint32_t*)0x5410;
        uint32_t height = *(uint32_t*)0x540C;
        volatile uint8_t *fb = (volatile uint8_t*)(uint64_t)*(uint32_t*)0x5404;
        uint32_t move = (height - CHAR_H) * pitch;
        for (uint32_t i = 0; i < move; i++) fb[i] = fb[i + CHAR_H * pitch];
        fb_fill_rect(0, (rows-1) * CHAR_H, cols * CHAR_W, CHAR_H, bg_color(current_attr));
    }
}

void console_put_at(int x, int y, char c, uint8_t color) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) return;
    putc_at(x, y, c, color);
}

static int utf8_pending = 0;
static char utf8_to_cp437(uint8_t b2) {
    switch (b2) {
        case 0xA0: return (char)0xA0;
        case 0xA9: return (char)0x82;
        case 0xAD: return (char)0xA1;
        case 0xB3: return (char)0xA2;
        case 0xBA: return (char)0xA3;
        case 0xB1: return (char)0xA4;
        case 0x91: return (char)0xA5;
        case 0xBC: return (char)0x81;
    }
    return '?';
}

void console_putchar(char c) {
    if (silent_mode) return;
    uint8_t b = (uint8_t)c;
    if (utf8_pending) { utf8_pending = 0; c = utf8_to_cp437(b); }
    else if (b == 0xC3) { utf8_pending = 1; return; }

    if (c == '\n') {
        cx = 0; cy++;
    } else if (c == '\r') {
        cx = 0;
    } else if (c == '\b') {
        if (cx > 0) { cx--; putc_at(cx, cy, ' ', current_attr); }
    } else if (c == '\t') {
        int next = (cx + 8) & ~7;
        while (cx < next && cx < cols) console_putchar(' ');
    } else {
        putc_at(cx, cy, c, current_attr);
        cx++;
        if (cx >= cols) { cx = 0; cy++; }
    }

    /* Limite dentro de la ventana */
    int max_rows = rows;
    if (console_win >= 0) {
        window_t *w = wm_get(console_win);
        if (w) max_rows = w->buf_h / CHAR_H;
    }
    if (cy >= max_rows) {
        console_scroll();
        cy = max_rows - 1;
    }
}

void console_write(const char *s) { while (*s) console_putchar(*s++); }

/* --- printf --- */
static void print_uint(uint64_t v, int base, int width, char pad) {
    char buf[32]; int i = 0;
    const char *hex = "0123456789abcdef";
    if (v == 0) buf[i++] = '0';
    while (v) { buf[i++] = hex[v % base]; v /= base; }
    while (i < width) buf[i++] = pad;
    while (i--) console_putchar(buf[i]);
}
static void print_int(int64_t v) {
    if (v < 0) { console_putchar('-'); v = -v; }
    print_uint((uint64_t)v, 10, 0, ' ');
}
void console_printf(const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') { console_putchar(*p); continue; }
        p++;
        int width = 0, precision = -1;
        char pad = ' ';
        if (*p == '0') { pad = '0'; p++; }
        while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }
        if (*p == '.') {
            p++; precision = 0;
            while (*p >= '0' && *p <= '9') { precision = precision * 10 + (*p - '0'); p++; }
        }
        switch (*p) {
            case 'd': print_int(__builtin_va_arg(args, int)); break;
            case 'u': print_uint(__builtin_va_arg(args, unsigned int), 10, width, pad); break;
            case 'x': print_uint(__builtin_va_arg(args, unsigned int), 16, width, pad); break;
            case 'l': {
                p++;
                if (*p == 'd') print_int(__builtin_va_arg(args, long));
                else if (*p == 'u') print_uint(__builtin_va_arg(args, unsigned long), 10, width, pad);
                else if (*p == 'x') print_uint(__builtin_va_arg(args, unsigned long), 16, width, pad);
                break;
            }
            case 's': {
                const char *s = __builtin_va_arg(args, const char*);
                if (precision >= 0) { int i = 0; while (s[i] && i < precision) console_putchar(s[i++]); }
                else while (*s) console_putchar(*s++);
                break;
            }
            case 'c': console_putchar((char)__builtin_va_arg(args, int)); break;
            case '%': console_putchar('%'); break;
            default: console_putchar('%'); console_putchar(*p); break;
        }
    }
    __builtin_va_end(args);
}
