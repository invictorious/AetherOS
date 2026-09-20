#include <stdint.h>

/* Stubs en ASM que hacen int 0x81 en user space */
extern void stub_GetStdHandle(void);
extern void stub_WriteFile(void);
extern void stub_ReadFile(void);
extern void stub_ExitProcess(void);
extern void stub_Sleep(void);

typedef struct {
    const char *name;
    void       *addr;
} win_export_t;

static const win_export_t g_exports[] = {
    {"GetStdHandle", (void*)stub_GetStdHandle},
    {"WriteFile",    (void*)stub_WriteFile},
    {"ReadFile",     (void*)stub_ReadFile},
    {"ExitProcess",  (void*)stub_ExitProcess},
    {"Sleep",        (void*)stub_Sleep},
    {0, 0}
};

void *win_lookup_export(const char *dll, const char *func) {
    (void)dll;
    for (int i = 0; g_exports[i].name; i++) {
        const char *a = g_exports[i].name;
        const char *b = func;
        while (*a && *b && *a == *b) { a++; b++; }
        if (*a == 0 && *b == 0) return g_exports[i].addr;
    }
    return 0;
}
