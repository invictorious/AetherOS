/* .exe de prueba que usa msvcrt basico (sin printf) */

__declspec(dllimport) void *GetStdHandle(unsigned long h);
__declspec(dllimport) int   WriteFile(void *h, const void *buf,
                                       unsigned long n, unsigned long *written,
                                       void *ov);
__declspec(dllimport) void *malloc(unsigned long size);
__declspec(dllimport) void  free(void *p);
__declspec(dllimport) unsigned long strlen(const char *s);
__declspec(dllimport) void *memcpy(void *d, const void *s, unsigned long n);
__declspec(dllimport) void  ExitProcess(unsigned int code);

#define STD_OUTPUT_HANDLE ((unsigned long)-11)

static void print(void *h, const char *s) {
    unsigned long w;
    WriteFile(h, s, strlen(s), &w, 0);
}

void entry(void) {
    void *h = GetStdHandle(STD_OUTPUT_HANDLE);

    print(h, "\r\n================================\r\n");
    print(h, "  HELLO3 - usa msvcrt!\r\n");
    print(h, "================================\r\n\r\n");

    /* Test malloc */
    char *buf = (char*)malloc(64);
    if (!buf) {
        print(h, "malloc fallo\r\n");
        ExitProcess(1);
    }

    print(h, "malloc OK. Llenando con memcpy...\r\n");
    memcpy(buf, "AetherOS rocks!", 15);
    buf[15] = 0;

    print(h, "buf = ");
    print(h, buf);
    print(h, "\r\n");

    print(h, "strlen(buf) = ");
    unsigned long len = strlen(buf);
    /* Convertir a string simple */
    char num[8]; int i = 0;
    if (len == 0) num[i++] = '0';
    while (len > 0) { num[i++] = '0' + (len % 10); len /= 10; }
    num[i] = 0;
    /* Invertir */
    for (int a = 0, b = i - 1; a < b; a++, b--) {
        char t = num[a]; num[a] = num[b]; num[b] = t;
    }
    print(h, num);
    print(h, "\r\n\r\n");

    print(h, "Liberando memoria...\r\n");
    free(buf);

    print(h, "Adios desde el .exe!\r\n");
    ExitProcess(0);
}
