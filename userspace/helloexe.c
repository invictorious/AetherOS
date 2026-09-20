/* Programa .exe de Windows.
 * Compilar con: x86_64-w64-mingw32-gcc
 */

__declspec(dllimport) void *GetStdHandle(unsigned long nStdHandle);
__declspec(dllimport) int   WriteFile(void *h, const void *buf,
                                       unsigned long n, unsigned long *written,
                                       void *overlap);
__declspec(dllimport) void  ExitProcess(unsigned int code);

#define STD_OUTPUT_HANDLE ((unsigned long)-11)

void entry(void) {
    void *h = GetStdHandle(STD_OUTPUT_HANDLE);
    unsigned long written;
    const char msg[] =
        "\r\n"
        "================================\r\n"
        "  HOLA DESDE UN .EXE REAL!\r\n"
        "================================\r\n\r\n"
        "Este es un ejecutable PE64 compilado\r\n"
        "con MinGW-w64. No es ELF. Es .exe.\r\n"
        "El kernel ha leido sus headers PE,\r\n"
        "ha cargado sus secciones, ha resuelto\r\n"
        "sus imports desde nuestra kernel32, y\r\n"
        "lo ha ejecutado en ring 3.\r\n\r\n";
    WriteFile(h, msg, sizeof(msg) - 1, &written, 0);
    ExitProcess(0);
}
