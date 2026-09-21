#include "win_abi.h"
#include "../kernel/console.h"
#include "../kernel/window.h"

/* Estado del contexto GDI: color actual y posicion actual */
static uint32_t current_color = 0;      /* RGB Windows (0x00BBGGRR) */
static uint32_t current_bkcolor = 0;
static int      current_x = 0, current_y = 0;

/* Convierte color Windows (0x00BBGGRR) a color interno (0x00RRGGBB) */
static uint32_t win_to_internal(uint32_t wincolor) {
    uint32_t r = wincolor & 0xFF;
    uint32_t g = (wincolor >> 8) & 0xFF;
    uint32_t b = (wincolor >> 16) & 0xFF;
    return (r << 16) | (g << 8) | b;
}

/* ------------------------------------------------------------
 * SetPixel(hdc, x, y, color)
 * ------------------------------------------------------------ */
static uint64_t win_SetPixel(uint64_t hdc, int x, int y, uint32_t color) {
    wm_put_pixel((int)hdc, x, y, win_to_internal(color));
    return 1;
}

/* ------------------------------------------------------------
 * MoveToEx(hdc, x, y, old_point)
 * ------------------------------------------------------------ */
static uint64_t win_MoveToEx(uint64_t hdc, int x, int y) {
    (void)hdc;
    current_x = x;
    current_y = y;
    return 1;
}

/* ------------------------------------------------------------
 * LineTo(hdc, x, y)
 * ------------------------------------------------------------ */
static uint64_t win_LineTo(uint64_t hdc, int x, int y) {
    int x0 = current_x, y0 = current_y;
    int x1 = x, y1 = y;

    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    uint32_t c = current_color;

    for (;;) {
        wm_put_pixel((int)hdc, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
    current_x = x;
    current_y = y;
    return 1;
}

/* ------------------------------------------------------------
 * Rectangle(hdc, left, top, right, bottom)
 * ------------------------------------------------------------ */
static uint64_t win_Rectangle(uint64_t hdc, int l, int t, int r, int b) {
    uint32_t c = current_color;

    /* Lados horizontales */
    for (int x = l; x <= r; x++) {
        wm_put_pixel((int)hdc, x, t, c);
        wm_put_pixel((int)hdc, x, b, c);
    }
    /* Lados verticales */
    for (int y = t; y <= b; y++) {
        wm_put_pixel((int)hdc, l, y, c);
        wm_put_pixel((int)hdc, r, y, c);
    }
    return 1;
}

/* ------------------------------------------------------------
 * FillSolidRect(hdc, l, t, r, b, color)
 * ------------------------------------------------------------ */
static uint64_t win_FillSolidRect(uint64_t hdc, int l, int t, int r, int b, uint32_t color) {
    uint32_t c = win_to_internal(color);
    for (int y = t; y <= b; y++)
        for (int x = l; x <= r; x++)
            wm_put_pixel((int)hdc, x, y, c);
    return 1;
}

/* ------------------------------------------------------------
 * Ellipse(hdc, left, top, right, bottom) - Bresenham para elipse
 * ------------------------------------------------------------ */
static uint64_t win_Ellipse(uint64_t hdc, int l, int t, int r, int b) {
    uint32_t c = current_color;

    int a = (r - l) / 2;
    int bb = (b - t) / 2;
    int cx = (l + r) / 2;
    int cy = (t + b) / 2;

    if (a <= 0 || bb <= 0) return 0;

    /* Version simple: recorrer todos los pixeles del bounding box
     * y comprobar si estan cerca del borde de la elipse */
    for (int y = -bb; y <= bb; y++) {
        for (int x = -a; x <= a; x++) {
            /* Distancia normalizada */
            int v = (x * x * bb * bb) + (y * y * a * a);
            int outer = a * a * bb * bb;
            /* Margen de 1 pixel: aceptar si esta en el borde */
            int inner_a = a - 1, inner_b = bb - 1;
            int inner = inner_a * inner_a * inner_b * inner_b;
            if (v <= outer && v >= inner) {
                wm_put_pixel((int)hdc, cx + x, cy + y, c);
            }
        }
    }
    return 1;
}

/* ------------------------------------------------------------
 * FillRect(hdc, RECT*, brush) - rellenar rectangulo
 * Simplificacion: brush = color directamente
 * ------------------------------------------------------------ */
static uint64_t win_FillRect(uint64_t hdc, uint64_t rect_ptr, uint64_t brush) {
    /* RECT = { long left, top, right, bottom; } */
    int32_t *rect = (int32_t*)rect_ptr;
    int l = rect[0], t = rect[1], r = rect[2], b = rect[3];
    uint32_t c = (uint32_t)brush;   /* el brush ya es un color */
    return win_FillSolidRect(hdc, l, t, r, b, c);
}

/* ------------------------------------------------------------
 * SetTextColor / SetBkColor
 * ------------------------------------------------------------ */
static uint64_t win_SetTextColor(uint64_t hdc, uint32_t color) {
    (void)hdc;
    uint32_t old = current_color;
    current_color = win_to_internal(color);
    return old;
}

static uint64_t win_SetBkColor(uint64_t hdc, uint32_t color) {
    (void)hdc;
    uint32_t old = current_bkcolor;
    current_bkcolor = win_to_internal(color);
    return old;
}

/* ------------------------------------------------------------
 * CreateSolidBrush(color) -> handle ficticio = color
 * DeleteObject(handle) -> no-op
 * ------------------------------------------------------------ */
static uint64_t win_CreateSolidBrush(uint32_t color) {
    return (uint64_t)color;
}

static uint64_t win_DeleteObject(uint64_t h) {
    (void)h;
    return 1;
}

/* ------------------------------------------------------------
 * Dispatcher
 * ------------------------------------------------------------ */
uint64_t gdi32_dispatch(uint64_t nr, uint64_t a1, uint64_t a2,
                        uint64_t a3, uint64_t a4) {
    switch (nr) {
        case WIN_SETPIXEL:         return win_SetPixel(a1, (int)a2, (int)a3, (uint32_t)a4);
        case WIN_MOVETOEX:         return win_MoveToEx(a1, (int)a2, (int)a3);
        case WIN_LINETO:           return win_LineTo(a1, (int)a2, (int)a3);
        case WIN_RECTANGLE:        return win_Rectangle(a1, (int)a2, (int)a3, (int)a4, (int)0);
        case WIN_ELLIPSE:          return win_Ellipse(a1, (int)a2, (int)a3, (int)a4, (int)0);
        case WIN_FILLRECT:         return win_FillRect(a1, a2, a3);
        case WIN_SETTEXTCOLOR:     return win_SetTextColor(a1, (uint32_t)a2);
        case WIN_SETBKCOLOR:       return win_SetBkColor(a1, (uint32_t)a2);
        case WIN_CREATESOLIDBRUSH: return win_CreateSolidBrush((uint32_t)a1);
        case WIN_DELETEOBJECT:     return win_DeleteObject(a1);
        case WIN_FILLSOLIDRECT:    return win_FillSolidRect(a1, (int)a2, (int)a3, (int)a4, 0, 0);
        default:
            console_printf("[GDI32] syscall 0x%x desconocida\n", (unsigned)nr);
            return 0;
    }
}
