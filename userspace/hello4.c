__declspec(dllimport) void *GetStdHandle(unsigned long h);
__declspec(dllimport) int   WriteFile(void *h, const void *buf,
                                       unsigned long n, unsigned long *written,
                                       void *ov);
__declspec(dllimport) void *CreateFileA(const char *name, unsigned long access,
                                         unsigned long share, void *sec,
                                         unsigned long disp, unsigned long flags,
                                         void *templ);
__declspec(dllimport) int   ReadFile(void *h, void *buf, unsigned long n,
                                      unsigned long *read, void *ov);
__declspec(dllimport) int   CloseHandle(void *h);
__declspec(dllimport) void  ExitProcess(unsigned int code);

#define STD_OUTPUT_HANDLE ((unsigned long)-11)

static void println(void *h, const char *s) {
    unsigned long w;
    unsigned long n = 0;
    while (s[n]) n++;
    WriteFile(h, s, n, &w, 0);
}

void entry(void) {
    void *h = GetStdHandle(STD_OUTPUT_HANDLE);

    println(h, "\r\n================================\r\n");
    println(h, "  HELLO4 - abre README.TXT\r\n");
    println(h, "================================\r\n\r\n");

    void *f = CreateFileA("README.TXT", 0x80000000, 0, 0, 3, 0, 0);
    if (f == (void*)-1) {
        println(h, "No se pudo abrir README.TXT\r\n");
        ExitProcess(1);
    }
    println(h, "README.TXT abierto!\r\n\r\n");

    char buf[256];
    unsigned long n;
    while (ReadFile(f, buf, 255, &n, 0) && n > 0) {
        buf[n] = 0;
        println(h, buf);
    }

    CloseHandle(f);
    println(h, "\r\nAdios!\r\n");
    ExitProcess(0);
}
