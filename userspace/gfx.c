/* .exe que dibuja formas en su ventana */

__declspec(dllimport) void  ExitProcess(unsigned int code);
__declspec(dllimport) void *GetStdHandle(unsigned long h);
__declspec(dllimport) int   WriteFile(void *h, const void *buf,
                                       unsigned long n, unsigned long *w,
                                       void *ov);
__declspec(dllimport) void *CreateWindowExA(unsigned long ex, const char *c,
                                             const char *w, unsigned long s,
                                             int x, int y, int cx, int cy,
                                             void *p, void *m, void *i, void *pa);
__declspec(dllimport) int   ShowWindow(void *hwnd, int cmd);
__declspec(dllimport) int   SetWindowTextA(void *hwnd, const char *text);
__declspec(dllimport) int   GetMessageA(void *msg, void *hwnd,
                                          unsigned min, unsigned max);
__declspec(dllimport) int   UpdateWindow(void *hwnd);
__declspec(dllimport) int   TextOutA(void *hdc, int x, int y, const char *s, int len);

/* gdi32 */
__declspec(dllimport) int   SetPixel(void *hdc, int x, int y, unsigned long color);
__declspec(dllimport) int   MoveToEx(void *hdc, int x, int y, void *old);
__declspec(dllimport) int   LineTo(void *hdc, int x, int y);

#define STD_OUTPUT_HANDLE ((unsigned long)-11)
#define SW_SHOW 5

/* Colores: formato Windows 0x00BBGGRR */
#define RED     0x000000FF
#define GREEN   0x0000FF00
#define BLUE    0x00FF0000
#define YELLOW  0x0000FFFF
#define WHITE   0x00FFFFFF
#define BLACK   0x00000000
#define CYAN    0x00FFFF00

static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static void println(void *h, const char *s) {
    unsigned long w; WriteFile(h, s, slen(s), &w, 0);
}

void entry(void) {
    void *h = GetStdHandle(STD_OUTPUT_HANDLE);
    println(h, "GDI demo...\r\n");

    void *w = CreateWindowExA(0, "Clase", "Titulo", 0,
                               100, 100, 600, 450, 0, 0, 0, 0);
    if (!w) { ExitProcess(1); }

    SetWindowTextA(w, "GDI Demo - formas geometricas");
    ShowWindow(w, SW_SHOW);
    UpdateWindow(0);

    /* 1. Triangulo rojo */
    MoveToEx(w, 50, 50, 0);
    LineTo(w, 150, 50);
    LineTo(w, 100, 150);
    LineTo(w, 50, 50);

    /* 2. Cuadrado verde */
    for (int i = 0; i <= 100; i++) {
        SetPixel(w, 200 + i, 50, GREEN);
        SetPixel(w, 200 + i, 150, GREEN);
        SetPixel(w, 200, 50 + i, GREEN);
        SetPixel(w, 300, 50 + i, GREEN);
    }

    /* 3. Lineas azules en abanico */
    for (int i = 0; i < 10; i++) {
        MoveToEx(w, 400, 100, 0);
        LineTo(w, 500, 50 + i * 10);
    }

    /* 4. Cuadricula amarilla */
    for (int i = 0; i <= 500; i += 20) {
        SetPixel(w, 50 + i, 250, YELLOW);
        SetPixel(w, 50 + i, 251, YELLOW);
    }
    for (int i = 0; i <= 50; i++) {
        SetPixel(w, 50, 250 + i, YELLOW);
        SetPixel(w, 550, 250 + i, YELLOW);
    }

    /* 5. Texto */
    TextOutA(w, 50, 320, "GDI funciona! SetPixel + MoveTo + LineTo", 40);

    println(h, "Dibujado completo. Pulsa Escape para salir.\r\n");

    unsigned char msg[48];
    while (1) {
        if (!GetMessageA(msg, 0, 0, 0)) break;
        unsigned int key = *(unsigned int*)(msg + 16);
        if (key == 27) break;
    }
    ExitProcess(0);
}
