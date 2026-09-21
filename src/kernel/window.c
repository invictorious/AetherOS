#include "window.h"
#include "../drivers/framebuffer.h"
#include "../drivers/mouse.h"
#include "../mm/heap.h"

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
int wm_has_windows(void) { return num_windows > 0; }

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

    /* Buffer del area cliente */
    win->buf_w = w - 2 * WM_BORDER;
    win->buf_h = h - 2 * WM_BORDER - WM_TITLE_H;
    if (win->buf_w < 0) win->buf_w = 0;
    if (win->buf_h < 0) win->buf_h = 0;
    win->content_buf = (uint32_t*)kmalloc(win->buf_w * win->buf_h * 4);
    if (win->content_buf) {
        for (int k = 0; k < win->buf_w * win->buf_h; k++)
            win->content_buf[k] = win->content_bg;
    }

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
            if (windows[i].content_buf) kfree(windows[i].content_buf);
            windows[i].content_buf = 0;
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
            /* Si era la terminal, avisar al kernel */
            extern int wm_get_terminal(void);
            if (id == wm_get_terminal()) {
                /* La terminal fue cerrada; la consola queda huerfana */
                extern void console_set_silent(int s);
                console_set_silent(1);
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

    /* Contenido (restaurar desde buffer) */
    if (w->content_buf) {
        int ox = w->x + WM_BORDER;
        int oy = w->y + WM_BORDER + WM_TITLE_H;
        for (int j = 0; j < w->buf_h; j++) {
            for (int i = 0; i < w->buf_w; i++) {
                fb_put_pixel(ox + i, oy + j, w->content_buf[j * w->buf_w + i]);
            }
        }
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
    fb_fill_rect(0, y, sw, 2, RGB(0, 0, 170));

    /* Boton INICIO a la izquierda */
    int bx = 6;
    fb_fill_rect(bx, y + 4, 100, WM_TASKBAR_H - 8, RGB(0, 100, 0));

    /* Texto "Inicio" dibujado a mano (fuente FONT_ADDR) */
    {
        const char *txt = "Inicio";
        int tx = bx + 8;
        int ty = y + 6;
        for (int k = 0; txt[k]; k++) {
            const uint8_t *g = FONT_ADDR + (uint8_t)txt[k] * 16;
            for (int r = 0; r < 16; r++) {
                uint8_t bits = g[r];
                for (int col = 0; col < 8; col++)
                    if (bits & (0x80 >> col))
                        fb_put_pixel(tx + col, ty + r, RGB(255, 255, 255));
            }
            tx += 8;
        }
    }
    bx += 106;
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

void wm_redraw_window(int id) {
    window_t *w = wm_get(id);
    if (!w || !w->visible) return;
    draw_window(w);
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
    if (x < 0 || y < 0 || x >= w->buf_w || y >= w->buf_h) return;

    /* Guardar en buffer */
    if (w->content_buf) {
        w->content_buf[y * w->buf_w + x] = color;
    }

    /* Y pintar en pantalla */
    int px = w->x + WM_BORDER + x;
    int py = w->y + WM_BORDER + WM_TITLE_H + y;
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


/* ============================================================
 * CURSOR DEL RATON
 * ============================================================ */

#define CUR_W 12
#define CUR_H 19
#define CUR_MASK_SIZE (CUR_W * CUR_H)

/* Forma del cursor (1 = pintar). Flecha clasica. */
static const uint16_t cursor_shape[CUR_H] = {
    0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00,
    0xFE00, 0xFF00, 0xFF80, 0xFFC0, 0xFFE0, 0xF800,
    0xD800, 0x8C00, 0x0C00, 0x0600, 0x0600, 0x0300, 0x0300,
};

/* Guardamos el fondo que habia debajo del cursor para restaurarlo */
static uint32_t saved_pixels[CUR_MASK_SIZE];
static int      saved_x = -1, saved_y = -1;
static int      cursor_visible = 0;

void wm_cursor_hide(void) {
    if (!cursor_visible) return;
    if (saved_x < 0) return;
    for (int j = 0; j < CUR_H; j++) {
        for (int i = 0; i < CUR_W; i++) {
            fb_put_pixel(saved_x + i, saved_y + j, saved_pixels[j * CUR_W + i]);
        }
    }
    cursor_visible = 0;
}

void wm_cursor_draw(void) {
    int cx = mouse_x();
    int cy = mouse_y();

    /* 1. Guardar fondo real de 12x19 */
    for (int j = 0; j < CUR_H; j++) {
        for (int i = 0; i < CUR_W; i++) {
            saved_pixels[j * CUR_W + i] = fb_get_pixel(cx + i, cy + j);
        }
    }
    saved_x = cx;
    saved_y = cy;

    /* 2. Dibujar la flecha blanca con contorno negro para visibilidad */
    for (int j = 0; j < CUR_H; j++) {
        uint16_t row = cursor_shape[j];
        for (int i = 0; i < CUR_W; i++) {
            if (row & (0x8000 >> i)) {
                fb_put_pixel(cx + i, cy + j, RGB(255, 255, 255));
            }
        }
    }
    cursor_visible = 1;
}

/* ============================================================
 * HIT TESTING + INTERACCION
 * ============================================================ */

/* Devuelve el id de la ventana bajo (px,py) teniendo en cuenta z-order.
 * 0 si no hay ninguna. */
static int hit_test_window(int px, int py) {
    for (int i = num_windows - 1; i >= 0; i--) {
        window_t *w = wm_get(z_order[i]);
        if (!w || !w->visible) continue;
        if (px >= w->x && px < w->x + w->w &&
            py >= w->y && py < w->y + w->h) {
            return w->id;
        }
    }
    return 0;
}

/* Comprueba si (px,py) esta sobre el boton [X] de la ventana w */
static int hit_close_button(window_t *w, int px, int py) {
    if (!w) return 0;
    int bx = w->x + w->w - 60;
    int by = w->y + WM_BORDER + 3;
    /* Boton X = el tercero: bx+40, by, 16x14 */
    int xx = bx + 40, xy = by;
    if (px >= xx && px < xx + 16 && py >= xy && py < xy + 14) return 1;
    return 0;
}

/* Comprueba si (px,py) esta sobre la barra de titulo de la ventana */
static int hit_title_bar(window_t *w, int px, int py) {
    if (!w) return 0;
    int bx = w->x + w->w - 60;
    if (px >= w->x + WM_BORDER && px < bx &&
        py >= w->y + WM_BORDER && py < w->y + WM_BORDER + WM_TITLE_H) {
        return 1;
    }
    return 0;
}

/* Comprueba si (px,py) esta sobre algun boton de la barra de tareas */
static int hit_taskbar_button(int px, int py) {
    int sh = fb_height();
    int taskbar_y = sh - WM_TASKBAR_H;
    if (py < taskbar_y) return 0;

    int bx = 6;
    for (int i = 0; i < num_windows; i++) {
        window_t *w = wm_get(z_order[i]);
        if (!w) continue;
        int bw = 130;
        if (px >= bx && px < bx + bw &&
            py >= taskbar_y + 4 && py < taskbar_y + WM_TASKBAR_H - 4) {
            return w->id;
        }
        bx += bw + 4;
    }
    return 0;
}

/* Estado de arrastre */
static int dragging_id = -1;
static int drag_offset_x = 0;
static int drag_offset_y = 0;
static uint8_t prev_buttons = 0;

int wm_drag_id(void) { return dragging_id; }

/* Hit test del boton INICIO en la barra de tareas */
int wm_start_button_hit(int px, int py) {
    int sh = fb_height();
    int taskbar_y = sh - WM_TASKBAR_H;
    if (py < taskbar_y + 4) return 0;
    if (py >= taskbar_y + WM_TASKBAR_H - 4) return 0;
    if (px < 6) return 0;
    if (px >= 6 + 100) return 0;
    return 1;
}

/* Punto de entrada: se llama desde el kernel cuando mouse_changed() es 1 */
void wm_handle_mouse(void) {
    int mx = mouse_x();
    int my = mouse_y();
    uint8_t btn = mouse_buttons();
    uint8_t pressed = btn & ~prev_buttons;   /* bits que se acaban de pulsar */
    uint8_t released = prev_buttons & ~btn;  /* bits que se acaban de soltar */

    /* --- Click izquierdo pulsado --- */
    if (pressed & 0x01) {
        /* 0. Boton INICIO: abre/refoca la terminal */
        if (wm_start_button_hit(mx, my)) {
            extern int kernel_open_terminal(void);
            kernel_open_terminal();
            prev_buttons = btn;
            return;
        }

        /* 1. Boton de la barra de tareas */
        int id = hit_taskbar_button(mx, my);
        if (id) {
            wm_focus(id);
            wm_draw_all();
        } else {
            /* 2. Ventana bajo el cursor */
            id = hit_test_window(mx, my);
            if (id) {
                window_t *w = wm_get(id);

                /* 2a. Boton [X] */
                if (hit_close_button(w, mx, my)) {
                    wm_destroy(id);
                    wm_draw_all();
                }
                /* 2b. Barra de titulo -> empezar arrastre */
                else if (hit_title_bar(w, mx, my)) {
                    wm_focus(id);
                    dragging_id = id;
                    drag_offset_x = mx - w->x;
                    drag_offset_y = my - w->y;
                }
                /* 2c. Cuerpo de la ventana -> solo foco */
                else {
                    wm_focus(id);
                    wm_draw_all();
                }
            } else {
                /* 3. Click en el ESCRITORIO -> desenfocar todo */
                if (focused_id != -1) {
                    focused_id = -1;
                    wm_draw_all();
                }
            }
        }
    }

    /* --- Mover mientras se arrastra --- */
    if (dragging_id >= 0 && (btn & 0x01)) {
        window_t *w = wm_get(dragging_id);
        if (w) {
            w->x = mx - drag_offset_x;
            w->y = my - drag_offset_y;
            /* Limites */
            if (w->x < 0) w->x = 0;
            if (w->y < 0) w->y = 0;
            if (w->x + w->w > (int)fb_width())  w->x = fb_width()  - w->w;
            if (w->y + w->h > (int)fb_height() - WM_TASKBAR_H)
                w->y = fb_height() - WM_TASKBAR_H - w->h;

            wm_draw_all();
        }
    }

    /* --- Click soltado: parar arrastre --- */
    if (released & 0x01) {
        dragging_id = -1;
    }

    prev_buttons = btn;
}
