#include "win_abi.h"
#include "../kernel/console.h"
#include "../drivers/keyboard.h"

/* Box-drawing CP437 */
#define BOX_TL  0xC9   /* ╔ */
#define BOX_TR  0xBB   /* ╗ */
#define BOX_BL  0xC8   /* ╚ */
#define BOX_BR  0xBC   /* ╝ */
#define BOX_H   0xCD   /* ═ */
#define BOX_V   0xBA   /* ║ */
#define BOX_LT  0xCC   /* ╠ */
#define BOX_RT  0xB9   /* ╣ */

/* Atributo de color para consola VGA:
 * nibble alto = fondo, nibble bajo = texto.
 * 0x1F = texto blanco brillante sobre fondo azul */
#define ATTR_TITLE   0x1F   /* blanco sobre azul (barra de titulo) */
#define ATTR_BOX     0x70   /* gris sobre negro (borde) */
#define ATTR_TEXT    0x0F   /* blanco sobre negro */
#define ATTR_BUTTON  0x70   /* gris sobre negro */

static void put_at(int x, int y, char c, uint8_t attr) {
    console_put_at(x, y, c, attr);
}

static void put_str_at(int x, int y, const char *s, uint8_t attr) {
    while (*s) {
        put_at(x++, y, *s, attr);
        s++;
    }
}

static void put_str_n_at(int x, int y, const char *s, int n, uint8_t attr) {
    for (int i = 0; i < n && s[i]; i++) {
        put_at(x + i, y, s[i], attr);
    }
}

/* ------------------------------------------------------------
 * Dibuja un MessageBox en modo texto.
 * Retorna el "ID" de la tecla pulsada (siempre IDOK en esta version)
 * ------------------------------------------------------------ */
static uint64_t draw_messagebox(const char *text, const char *caption) {
    int w = 60, h = 12;
    int x = (80 - w) / 2;
    int y = (25 - h) / 2;

    /* 1. Limpiar pantalla y rellenar fondo del cuadro */
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            put_at(x + i, y + j, ' ', ATTR_TEXT);
        }
    }

    /* 2. Bordes */
    put_at(x, y, BOX_TL, ATTR_BOX);
    put_at(x + w - 1, y, BOX_TR, ATTR_BOX);
    put_at(x, y + h - 1, BOX_BL, ATTR_BOX);
    put_at(x + w - 1, y + h - 1, BOX_BR, ATTR_BOX);
    for (int i = 1; i < w - 1; i++) {
        put_at(x + i, y, BOX_H, ATTR_BOX);
        put_at(x + i, y + h - 1, BOX_H, ATTR_BOX);
    }
    for (int j = 1; j < h - 1; j++) {
        put_at(x, y + j, BOX_V, ATTR_BOX);
        put_at(x + w - 1, y + j, BOX_V, ATTR_BOX);
    }

    /* 3. Barra de titulo (fila y+1) */
    for (int i = 1; i < w - 1; i++) {
        put_at(x + i, y + 1, ' ', ATTR_TITLE);
    }
    put_str_n_at(x + 2, y + 1, caption ? caption : "AetherOS", w - 12, ATTR_TITLE);
    /* Botones tipo Windows: [_][O][X] */
    put_str_at(x + w - 10, y + 1, " [_][O][X] ", ATTR_TITLE);

    /* 4. Separador bajo la barra */
    put_at(x, y + 2, BOX_LT, ATTR_BOX);
    put_at(x + w - 1, y + 2, BOX_RT, ATTR_BOX);
    for (int i = 1; i < w - 1; i++) put_at(x + i, y + 2, BOX_H, ATTR_BOX);

    /* 5. Texto del mensaje (con soporte para \n y \r\n) */
    int tx = x + 3;
    int ty = y + 4;
    const char *p = text;
    while (*p && ty < y + h - 3) {
        if (*p == '\r') { p++; continue; }
        if (*p == '\n') {
            tx = x + 3;
            ty++;
            p++;
            continue;
        }
        /* Cortar si llega al borde */
        if (tx >= x + w - 3) {
            tx = x + 3;
            ty++;
            continue;
        }
        put_at(tx++, ty, *p++, ATTR_TEXT);
    }

    /* 6. Boton OK */
    int bx = x + (w - 8) / 2;
    int by = y + h - 2;
    put_str_at(bx, by, "[  OK  ]", ATTR_BUTTON);

    /* 7. Esperar tecla - habilitar IRQs mientras esperamos */
    while (!keyboard_has_key()) {
        __asm__ volatile ("sti; hlt; cli");
    }
    char c = keyboard_getchar();
    (void)c;

    /* 8. Limpiar pantalla antes de volver */
    console_clear();

    return IDOK;
}

/* ------------------------------------------------------------
 * MessageBoxA(hwnd, text, caption, type)
 *   RCX = hwnd (ignorado)
 *   RDX = text (char*)
 *   R8  = caption (char*)
 *   R9  = type (ignorado)
 * ------------------------------------------------------------ */
static uint64_t win_MessageBoxA(uint64_t hwnd, uint64_t text_ptr,
                                 uint64_t caption_ptr, uint64_t type) {
    (void)hwnd; (void)type;
    return draw_messagebox((const char*)text_ptr, (const char*)caption_ptr);
}

/* MessageBoxW: version wide (UTF-16LE simplificado a ASCII) */
static uint64_t win_MessageBoxW(uint64_t hwnd, uint64_t text_ptr,
                                 uint64_t caption_ptr, uint64_t type) {
    (void)hwnd; (void)type;

    /* Convertir UTF-16 -> ASCII temporalmente */
    static char tbuf[512];
    static char cbuf[64];

    const uint16_t *src = (const uint16_t*)text_ptr;
    int i = 0;
    while (src[i] && i < 511) { tbuf[i] = (char)(src[i] & 0xFF); i++; }
    tbuf[i] = 0;

    const uint16_t *cpsrc = (const uint16_t*)caption_ptr;
    i = 0;
    while (cpsrc[i] && i < 63) { cbuf[i] = (char)(cpsrc[i] & 0xFF); i++; }
    cbuf[i] = 0;

    return draw_messagebox(tbuf, cbuf);
}

static uint64_t win_MessageBeep(uint64_t type) {
    (void)type;
    /* Sin altavoz por ahora */
    return 1;
}

/* ------------------------------------------------------------
 * Dispatcher de user32
 * ------------------------------------------------------------ */
uint64_t user32_dispatch(uint64_t nr, uint64_t a1, uint64_t a2,
                         uint64_t a3, uint64_t a4) {
    switch (nr) {
        case WIN_MESSAGEBOXA: return win_MessageBoxA(a1, a2, a3, a4);
        case WIN_MESSAGEBOXW: return win_MessageBoxW(a1, a2, a3, a4);
        case WIN_MESSAGEBEEP: return win_MessageBeep(a1);
        default:
            console_printf("[USER32] syscall 0x%x desconocida\n", (unsigned)nr);
            return 0;
    }
}
