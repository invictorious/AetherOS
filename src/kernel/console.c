#include "console.h"

#define VGA_BUFFER 0xB8000
#define VGA_COLOR  0x0F
#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static size_t cursor = 0;
static volatile uint8_t *vga = (volatile uint8_t *)VGA_BUFFER;

static int utf8_pending = 0;

static char utf8_to_cp437(uint8_t b1, uint8_t b2) {
    (void)b1;
    switch (b2) {
        case 0xA0: return (char)0xA0;
        case 0xA9: return (char)0x82;
        case 0xAD: return (char)0xA1;
        case 0xB3: return (char)0xA2;
        case 0xBA: return (char)0xA3;
        case 0xB1: return (char)0xA4;
        case 0x91: return (char)0xA5;
        case 0xBC: return (char)0x81;
        case 0x9C: return (char)0x9A;
        case 0xA1: return (char)0xA8;
        case 0xB0: return (char)0xF8;
    }
    return '?';
}

/* Scroll: mueve todo hacia arriba y limpia la ultima fila */
static void console_scroll(void) {
    for (size_t i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga[i * 2]     = vga[(i + VGA_WIDTH) * 2];
        vga[i * 2 + 1] = vga[(i + VGA_WIDTH) * 2 + 1];
    }
    for (size_t i = 0; i < VGA_WIDTH; i++) {
        size_t p = (VGA_HEIGHT - 1) * VGA_WIDTH + i;
        vga[p * 2]     = ' ';
        vga[p * 2 + 1] = VGA_COLOR;
    }
    cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;
}

void console_init(void) { cursor = 0; utf8_pending = 0; }

void console_clear(void) {
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i * 2]     = ' ';
        vga[i * 2 + 1] = VGA_COLOR;
    }
    cursor = 0;
}

void console_putchar(char c) {
    uint8_t b = (uint8_t)c;

    if (utf8_pending) {
        utf8_pending = 0;
        c = utf8_to_cp437(0xC3, b);
    } else if (b == 0xC3) {
        utf8_pending = 1;
        return;
    }

    if (c == '\n') {
        cursor += VGA_WIDTH - (cursor % VGA_WIDTH);
        if (cursor >= VGA_WIDTH * VGA_HEIGHT) console_scroll();
    } else if (c == '\b') {
        if (cursor > 0) {
            cursor--;
            vga[cursor * 2]     = ' ';
            vga[cursor * 2 + 1] = VGA_COLOR;
        }
    } else if (c == '\t') {
        size_t next = (cursor + 8) & ~7ULL;
        while (cursor < next) console_putchar(' ');
    } else if (c == '\r') {
        /* ignorar */
    } else {
        vga[cursor * 2]     = (uint8_t)c;
        vga[cursor * 2 + 1] = VGA_COLOR;
        cursor++;
        if (cursor >= VGA_WIDTH * VGA_HEIGHT) console_scroll();
    }
}

void console_write(const char *s) { while (*s) console_putchar(*s++); }

void console_put_at(int x, int y, char c, uint8_t color) {
    vga[(y * VGA_WIDTH + x) * 2]     = (uint8_t)c;
    vga[(y * VGA_WIDTH + x) * 2 + 1] = color;
}

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
        int width = 0;
        char pad = ' ';
        if (*p == '0') { pad = '0'; p++; }
        while (*p >= '0' && *p <= '9') { width = width * 10 + (*p - '0'); p++; }
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
            case 's': console_write(__builtin_va_arg(args, const char*)); break;
            case 'c': console_putchar((char)__builtin_va_arg(args, int)); break;
            case '%': console_putchar('%'); break;
            default: console_putchar('%'); console_putchar(*p); break;
        }
    }
    __builtin_va_end(args);
}
