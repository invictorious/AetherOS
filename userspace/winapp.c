__declspec(dllimport) void  ExitProcess(unsigned int code);
__declspec(dllimport) void *GetStdHandle(unsigned long h);
__declspec(dllimport) int   WriteFile(void *h, const void *buf,
                                       unsigned long n, unsigned long *w,
                                       void *ov);
__declspec(dllimport) void *CreateWindowExA(unsigned long ex_style,
                                             const char *class_name,
                                             const char *window_name,
                                             unsigned long style,
                                             int x, int y, int w, int h,
                                             void *parent, void *menu,
                                             void *inst, void *param);
__declspec(dllimport) int   ShowWindow(void *hwnd, int cmd);
__declspec(dllimport) int   SetWindowTextA(void *hwnd, const char *text);
__declspec(dllimport) int   GetMessageA(void *msg, void *hwnd,
                                          unsigned min, unsigned max);
__declspec(dllimport) int   TextOutA(void *hdc, int x, int y,
                                       const char *s, int len);
__declspec(dllimport) int   UpdateWindow(void *hwnd);

#define STD_OUTPUT_HANDLE ((unsigned long)-11)
#define SW_SHOW 5

static int slen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void println(void *h, const char *s) {
    unsigned long w;
    WriteFile(h, s, slen(s), &w, 0);
}

void entry(void) {
    void *h = GetStdHandle(STD_OUTPUT_HANDLE);
    println(h, "Creando ventanas con contenido...\r\n");

    void *w1 = CreateWindowExA(0, "Clase", "Titulo", 0,
                                100, 100, 420, 280, 0, 0, 0, 0);
    void *w2 = CreateWindowExA(0, "Clase", "Titulo", 0,
                                480, 220, 420, 280, 0, 0, 0, 0);

    if (w1) {
        SetWindowTextA(w1, "Aplicacion 1 - Creada por .exe");
        ShowWindow(w1, SW_SHOW);
    }
    if (w2) {
        SetWindowTextA(w2, "Aplicacion 2 - Otra ventana");
        ShowWindow(w2, SW_SHOW);
    }

    /* Dibujar los marcos de todas las ventanas de una vez */
    UpdateWindow(0);

    /* Ahora dibujar el contenido encima */
    if (w1) {
        TextOutA(w1, 20, 20, "Hola desde la ventana 1!", 25);
        TextOutA(w1, 20, 40, "Esto lo dibuja un .exe", 22);
        TextOutA(w1, 20, 60, "de Windows (PE32+).", 19);
        TextOutA(w1, 20, 100, "No es ELF. Es PE.", 17);
        TextOutA(w1, 20, 120, "El kernel lo cargo y", 20);
        TextOutA(w1, 20, 140, "el .exe dibuja aqui.", 20);
    }
    if (w2) {
        TextOutA(w2, 20, 20, "Aplicacion 2", 12);
        TextOutA(w2, 20, 40, "Distinta ventana.", 17);
        TextOutA(w2, 20, 60, "Creada por el mismo .exe", 24);
        TextOutA(w2, 20, 100, "El gestor de ventanas", 21);
        TextOutA(w2, 20, 120, "soporta varias a la vez.", 24);
    }

    unsigned char msg[48];
    int count = 0;
    while (count < 10) {
        if (!GetMessageA(msg, 0, 0, 0)) break;
        unsigned int key = *(unsigned int*)(msg + 16);
        println(h, "tecla recibida!\r\n");
        count++;
        if (key == 27) break;
    }

    println(h, "Saliendo.\r\n");
    ExitProcess(0);
}
