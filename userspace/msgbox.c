/* .exe que muestra un MessageBox de Windows */

__declspec(dllimport) void  ExitProcess(unsigned int code);
__declspec(dllimport) int   MessageBoxA(void *hwnd, const char *text,
                                         const char *caption, unsigned int type);
__declspec(dllimport) void *GetStdHandle(unsigned long h);
__declspec(dllimport) int   WriteFile(void *h, const void *buf,
                                       unsigned long n, unsigned long *w,
                                       void *ov);

#define STD_OUTPUT_HANDLE ((unsigned long)-11)

static void println(void *h, const char *s) {
    unsigned long w;
    unsigned long n = 0;
    while (s[n]) n++;
    WriteFile(h, s, n, &w, 0);
}

void entry(void) {
    void *h = GetStdHandle(STD_OUTPUT_HANDLE);

    println(h, "\r\nAntes del MessageBox...\r\n");

    int r = MessageBoxA(0,
        "Hola desde AetherOS!\n"
        "Este es un MessageBox real,\n"
        "dibujado por el kernel.\n"
        "\n"
        "Pulsa cualquier tecla para cerrar.",
        "AetherOS MessageBox",
        0);

    println(h, "MessageBox cerrado (");
    char buf[8]; int i = 0;
    if (r == 0) buf[i++] = '0';
    else {
        char tmp[8]; int j = 0;
        while (r > 0) { tmp[j++] = '0' + (r % 10); r /= 10; }
        while (j--) buf[i++] = tmp[j];
    }
    buf[i] = 0;
    println(h, buf);
    println(h, ")\r\n");
    println(h, "Despues del MessageBox.\r\n");

    ExitProcess(0);
}
