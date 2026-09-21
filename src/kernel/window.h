#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>
#include "../drivers/framebuffer.h"

#define WM_MAX_WINDOWS  8
#define WM_TITLE_H      20
#define WM_BORDER       2
#define WM_TASKBAR_H    28

/* Colores */
#define WM_COLOR_TITLE_ACTIVE   RGB(0, 0, 170)     /* azul */
#define WM_COLOR_TITLE_INACTIVE RGB(85, 85, 85)    /* gris oscuro */
#define WM_COLOR_BORDER_ACTIVE  RGB(255, 255, 255)
#define WM_COLOR_BORDER_INACTIVE RGB(170, 170, 170)
#define WM_COLOR_CONTENT        RGB(200, 200, 200) /* gris claro */
#define WM_COLOR_TASKBAR_BG     RGB(60, 60, 60)
#define WM_COLOR_TASKBAR_BTN    RGB(0, 0, 120)
#define WM_COLOR_TASKBAR_TEXT   RGB(255, 255, 255)

typedef struct {
    int      used;
    int      id;
    int      x, y, w, h;
    char     title[64];
    uint32_t content_bg;
    int      visible;
    uint32_t *content_buf;      /* buffer del area cliente */
    int      buf_w, buf_h;
} window_t;

int       wm_init(void);
int       wm_create(int x, int y, int w, int h, const char *title);
void      wm_destroy(int id);
void      wm_focus(int id);
window_t *wm_get(int id);
int       wm_focused_id(void);
int       wm_has_windows(void);
void      wm_draw_all(void);
void      wm_cursor_draw(void);
void      wm_cursor_hide(void);
void      wm_handle_mouse(void);
int       wm_drag_id(void);

/* "Superficie" de dibujo sobre una ventana */
void wm_put_pixel(int id, int x, int y, uint32_t color);
void wm_fill(int id, int x, int y, int w, int h, uint32_t color);
void wm_draw_text(int id, int x, int y, const char *s, uint32_t fg);

#endif
