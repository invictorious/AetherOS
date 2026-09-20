#include "window.h"
#include "../drivers/framebuffer.h"

/* ------------------------------------------------------------
 * Estado global del gestor de ventanas
 * ------------------------------------------------------------ */
static window_t windows[WM_MAX_WINDOWS];
static int      num_windows = 0;
static int      next_id = 1;
static int      focused_id = -1;

/* Orden de apilado: [0] = mas atras, [n-1] = mas al frente */
static int      z_order[WM_MAX_WINDOWS];

int wm_init(void) {
    for (int i = 0; i < WM_MAX_WINDOWS; i++) {
        windows[i].used = 0;
        z_order[i] = -1;
    }
    num_windows = 0;
    next_id = 1;
    focused_id = -1;
    return 0;
}

int wm_focused_id(void) { return focused_id; }

window_t *wm_get(int id) {
    for (int i = 0; i < WM_MAX_WINDOWS; i++) {
        if (windows[i].used && windows[i].id == id) return &windows[i];
    }
    return 0;
}

/* Inserta en la parte delantera del z_order */
static void z_to_front(int id) {
    int pos = -1;
    for (int i = 0; i < num_windows; i++) {
        if (z_order[i] == id) { pos = i; break; }
    }
    if (pos < 0) return;
    /* Mover todo lo posterior un puesto hacia atras */
    for (int i = pos; i < num_windows - 1; i++) {
        z_order[i] = z_order[i + 1];
    }
    z_order[num_windows - 1] = id;
}

int wm_create(int x, int y, int w, int h, const char *title) {
    if (num_windows >= WM_MAX_WINDOWS) return -1;

    /* Sanity checks para valores absurdos */
    if (x < 0 || x > 2000) x = 50;
    if (y < 0 || y > 2000) y = 50;
    if (w < 50 || w > 1500) w = 400;
    if (h < 50 || h > 1000) h = 300;
    if (!title) title = "Ventana";

    /* Buscar slot libre */
    int slot = -1;
    for (int i = 0; i < WM_MAX_WINDOWS; i++) {
        if (!windows[i].used) { slot = i; break; }
    }
    if (slot < 0) return -1;

    window_t *win = &windows[slot];
    win->used = 1;
    win->id = next_id++;
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->visible = 1;
    win->content_bg = WM_COLOR_CONTENT;

    /* Copiar titulo (max 63) */
    int i = 0;
    while (title[i] && i < 63) { win->title[i] = title[i]; i++; }
    win->title[i] = 0;

    z_order[num_windows] = win->id;
    num_windows++;
    focused_id = win->id;
    return win->id;
}

void wm_destroy(int id) {
    for (int i = 0; i < WM_MAX_WINDOWS; i++) {
        if (windows[i].used && windows[i].id == id) {
            windows[i].used = 0;
            /* Quitar de z_order */
            for (int j = 0; j < num_windows; j++) {
                if (z_order[j] == id) {
                    for (int k = j; k < num_windows - 1; k++) {
                        z_order[k] = z_order[k + 1];
                    }
                    z_order[num_windows - 1] = -1;
                    num_windows--;
                    break;
                }
            }
            if (focused_id == id) {
                focused_id = (num_windows > 0) ? z_order[num_windows - 1] : -1;
            }
            return;
        }
    }
}

void wm_focus(int id) {
    if (focused_id == id) return;
    if (!wm_get(id)) return;
    z_to_front(id);
    focused_id = id;
}

/* ------------------------------------------------------------
 * Dibujo
 * ------------------------------------------------------------ */
static void draw_border(window_t *w, uint32_t color) {
    /* Marco exterior de 2px */
    fb_fill_rect(w->x, w->y, w->w, WM_BORDER, color);
    fb_fill_rect(w->x, w->y + w->h - WM_BORDER, w->w, WM_BORDER, color);
    fb_fill_rect(w->x, w->y, WM_BORDER, w->h, color);
    fb_fill_rect(w->x + w->w - WM_BORDER, w->y, WM_BORDER, w->h, color);
}

static void draw_window(window_t *w) {
    int active = (w->id == focused_id);

    /* Fondo */
    fb_fill_rect(w->x, w->y, w->w, w->h, w->content_bg);

    /* Barra de titulo */
    uint32_t bg = active ? WM_COLOR_TITLE_ACTIVE : WM_COLOR_TITLE_INACTIVE;
    fb_fill_rect(w->x + WM_BORDER, w->y + WM_BORDER,
                 w->w - 2 * WM_BORDER, WM_TITLE_H, bg);

    /* Botones */
    int bx = w->x + w->w - 60;
    int by = w->y + WM_BORDER + 3;
    uint32_t btn = RGB(220, 220, 220);
    fb_fill_rect(bx, by, 16, 14, btn);
    fb_fill_rect(bx + 20, by, 16, 14, btn);
    fb_fill_rect(bx + 40, by, 16, 14, btn);

    /* Texto del titulo (recorriendo el array sin modificar) */
    int tx = w->x + 8;
    int ty = w->y + WM_BORDER + 2;
    for (int i = 0; w->title[i] && tx < bx - 8; i++) {
        const uint8_t *glyph = FONT_ADDR + (uint8_t)(w->title[i]) * 16;
        for (int row = 0; row < 16; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (0x80 >> col))
                    fb_put_pixel(tx + col, ty + row, RGB(255, 255, 255));
            }
        }
        tx += 8;
    }

    /* Borde */
    draw_border(w, active ? WM_COLOR_BORDER_ACTIVE : WM_COLOR_BORDER_INACTIVE);
}

/* ------------------------------------------------------------
 * Barra de tareas
 * ------------------------------------------------------------ */
static void draw_taskbar(void) {
    int sw = fb_width();
    int sh = fb_height();
    int y = sh - WM_TASKBAR_H;

    /* Fondo */
    fb_fill_rect(0, y, sw, WM_TASKBAR_H, WM_COLOR_TASKBAR_BG);

    /* Separador superior */
    fb_fill_rect(0, y, sw, 2, RGB(0, 0, 170));

    /* Boton por cada ventana */
    int bx = 6;
    for (int i = 0; i < num_windows; i++) {
        window_t *w = wm_get(z_order[i]);
        if (!w) continue;

        int bw = 130;
        uint32_t color = (w->id == focused_id)
            ? RGB(0, 0, 200) : WM_COLOR_TASKBAR_BTN;
        fb_fill_rect(bx, y + 4, bw, WM_TASKBAR_H - 8, color);

        /* Texto (recortado a 15 chars) */
        int tx = bx + 6;
        int ty = y + 6;
        for (int k = 0; w->title[k] && k < 15; k++) {
            const uint8_t *glyph = FONT_ADDR + (uint8_t)(w->title[k]) * 16;
            for (int row = 0; row < 16; row++) {
                uint8_t bits = glyph[row];
                for (int col = 0; col < 8; col++) {
                    if (bits & (0x80 >> col))
                        fb_put_pixel(tx + col, ty + row, WM_COLOR_TASKBAR_TEXT);
                }
            }
            tx += 8;
        }
        bx += bw + 4;
        if (bx + bw > sw - 6) break;
    }
}

void wm_draw_all(void) {
    /* Limpiar pantalla con un "escritorio" (fondo gris oscuro) */
    fb_clear(RGB(30, 30, 40));

    /* Dibujar ventanas en orden z (de atras hacia delante) */
    for (int i = 0; i < num_windows; i++) {
        window_t *w = wm_get(z_order[i]);
        if (w && w->visible) draw_window(w);
    }

    /* Barra de tareas encima */
    draw_taskbar();
}

/* ------------------------------------------------------------
 * Dibujo sobre una ventana
 * ------------------------------------------------------------ */
void wm_put_pixel(int id, int x, int y, uint32_t color) {
    window_t *w = wm_get(id);
    if (!w) return;
    /* Offset por borde + barra de titulo */
    int px = w->x + WM_BORDER + x;
    int py = w->y + WM_BORDER + WM_TITLE_H + y;
    if (x < 0 || y < 0 || x >= w->w - 2 * WM_BORDER ||
        y >= w->h - 2 * WM_BORDER - WM_TITLE_H) return;
    fb_put_pixel(px, py, color);
}

void wm_fill(int id, int x, int y, int ww, int hh, uint32_t color) {
    for (int j = 0; j < hh; j++)
        for (int i = 0; i < ww; i++)
            wm_put_pixel(id, x + i, y + j, color);
}

void wm_draw_text(int id, int x, int y, const char *s, uint32_t fg) {
    while (*s) {
        const uint8_t *glyph = FONT_ADDR + (uint8_t)(*s) * 16;
        for (int row = 0; row < 16; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (0x80 >> col))
                    wm_put_pixel(id, x + col, y + row, fg);
            }
        }
        x += 8;
        s++;
    }
}
