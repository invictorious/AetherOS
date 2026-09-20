#include "console.h"
#include "../drivers/framebuffer.h"

#define CHAR_W 8
#define CHAR_H 16

/* Paleta VGA estandar -> RGB */
static const uint32_t palette[16] = {
    RGB(0,   0,   0),    /* 0 negro */
    RGB(0,   0,   170),  /* 1 azul */
    RGB(0,   170, 0),    /* 2 verde */
    RGB(0,   170, 170),  /* 3 cyan */
    RGB(170, 0,   0),    /* 4 rojo */
    RGB(170, 0,   170),  /* 5 magenta */
    RGB(170, 85,  0),    /* 6 marron */
    RGB(170, 170, 170),  /* 7 gris claro */
    RGB(85,  85,  85),   /* 8 gris oscuro */
    RGB(85,  85,  255),  /* 9 azul claro */
    RGB(85,  255, 85),   /* A verde claro */
    RGB(85,  255, 255),  /* B cyan claro */
    RGB(255, 85,  85),   /* C rojo claro */
    RGB(255, 85,  255),  /* D magenta claro */
    RGB(255, 255, 85),   /* E amarillo */
    RGB(255, 255, 255),  /* F blanco */
};

static int      silent_mode = 0;

void console_set_silent(int s) { silent_mode = s; }

static int      cols = 0;
static int      rows = 0;
static int      cx = 0;   /* columna actual */
static int      cy = 0;   /* fila actual */
static uint8_t  current_attr = 0x0F;

static uint32_t fg_color(uint8_t attr) { return palette[attr & 0x0F]; }
static uint32_t bg_color(uint8_t attr) { return palette[(attr >> 4) & 0x0F]; }

int console_init(void) {
    if (fb_init() != 0) return -1;
    cols = fb_width()  / CHAR_W;
    rows = fb_height() / CHAR_H;
    cx = 0;
    cy = 0;
    current_attr = 0x0F;
    return 0;
}

void console_clear(void) {
    fb_clear(bg_color(current_attr));
    cx = 0;
    cy = 0;
}

/* Scroll: mover todo hacia arriba CHAR_H pixeles */
static void console_scroll(void) {
    uint32_t fbsize = fb_height() * 0; /* dummy para no romper compilacion */
    (void)fbsize;
    uint32_t pitch = *(uint32_t*)0x5410;
    uint32_t height = *(uint32_t*)0x540C;
    volatile uint8_t *fb = (volatile uint8_t*)(uint64_t)*(uint32_t*)0x5404;
    uint32_t move_bytes = (height - CHAR_H) * pitch;
    for (uint32_t i = 0; i < move_bytes; i++) {
        fb[i] = fb[i + CHAR_H * pitch];
    }
    /* Limpiar la ultima fila */
    fb_fill_rect(0, (rows - 1) * CHAR_H,
                 cols * CHAR_W, CHAR_H, bg_color(current_attr));
}

void console_put_at(int x, int y, char c, uint8_t color) {
    if (x < 0 || y < 0 || x >= cols || y >= rows) return;
    fb_draw_char(x * CHAR_W, y * CHAR_H, c, fg_color(color), bg_color(color));
}

static int utf8_pending = 0;

static char utf8_to_cp437(uint8_t b2) {
    switch (b2) {
        case 0xA0: return (char)0xA0;  /* á */
        case 0xA9: return (char)0x82;  /* é */
        case 0xAD: return (char)0xA1;  /* í */
        case 0xB3: return (char)0xA2;  /* ó */
        case 0xBA: return (char)0xA3;  /* ú */
        case 0xB1: return (char)0xA4;  /* ñ */
        case 0x91: return (char)0xA5;  /* Ñ */
        case 0xBC: return (char)0x81;  /* ü */
    }
    return '?';
}

void console_putchar(char c) {
    if (silent_mode) return;
    uint8_t b = (uint8_t)c;
    if (utf8_pending) {
        utf8_pending = 0;
        c = utf8_to_cp437(b);
    } else if (b == 0xC3) {
        utf8_pending = 1;
        return;
    }
    if (c == '\n') {
        cx = 0;
        cy++;
    } else if (c == '\r') {
        cx = 0;
    } else if (c == '\b') {
        if (cx > 0) {
            cx--;
            fb_draw_char(cx * CHAR_W, cy * CHAR_H, ' ',
                          fg_color(current_attr), bg_color(current_attr));
        }
    } else if (c == '\t') {
        int next = (cx + 8) & ~7;
        while (cx < next && cx < cols) console_putchar(' ');
    } else {
        fb_draw_char(cx * CHAR_W, cy * CHAR_H, c,
                      fg_color(current_attr), bg_color(current_attr));
        cx++;
        if (cx >= cols) { cx = 0; cy++; }
    }

    if (cy >= rows) {
        console_scroll();
        cy = rows - 1;
    }
}

void console_write(const char *s) {
    while (*s) console_putchar(*s++);
}

/* ------- printf minimo (igual que antes) ------- */
static void print_uint(uint64_t v, int base, int width, char pad) {
    char buf[32];
    int i = 0;
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
            p++;
            precision = 0;
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
                int i = 0;
                if (precision >= 0) {
                    while (s[i] && i < precision) console_putchar(s[i++]);
                } else {
                    while (*s) console_putchar(*s++);
                }
                break;
            }
            case 'c': console_putchar((char)__builtin_va_arg(args, int)); break;
            case '%': console_putchar('%'); break;
            default: console_putchar('%'); console_putchar(*p); break;
        }
    }
    __builtin_va_end(args);
}
