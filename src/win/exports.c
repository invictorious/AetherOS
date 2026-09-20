#include <stdint.h>

extern void stub_GetStdHandle(void);
extern void stub_WriteFile(void);
extern void stub_ReadFile(void);
extern void stub_ExitProcess(void);
extern void stub_Sleep(void);
extern void stub_GetLastError(void);
extern void stub_SetLastError(void);
extern void stub_GetTickCount(void);
extern void stub_VirtualAlloc(void);
extern void stub_VirtualFree(void);
extern void stub_GetProcessHeap(void);
extern void stub_CloseHandle(void);
extern void stub_GetCurrentProcessId(void);
extern void stub_GetCommandLineA(void);
extern void stub_CreateFileA(void);
extern void stub_WriteConsoleA(void);
extern void stub_ReadConsoleA(void);
extern void stub_SetConsoleTitleA(void);
extern void stub_GetModuleHandleA(void);
extern void stub_GetProcAddress(void);
extern void stub_LoadLibraryA(void);
extern void stub_FreeLibrary(void);
extern void stub_WriteConsoleW(void);

extern void stub_strlen(void);
extern void stub_strcmp(void);
extern void stub_strcpy(void);
extern void stub_memcpy(void);
extern void stub_memset(void);
extern void stub_puts(void);
extern void stub_malloc(void);
extern void stub_free(void);

typedef struct {
    const char *name;
    void       *addr;
} win_export_t;

static const win_export_t g_exports[] = {
    /* kernel32 */
    {"GetStdHandle",         (void*)stub_GetStdHandle},
    {"WriteFile",            (void*)stub_WriteFile},
    {"ReadFile",             (void*)stub_ReadFile},
    {"ExitProcess",          (void*)stub_ExitProcess},
    {"Sleep",                (void*)stub_Sleep},
    {"GetLastError",         (void*)stub_GetLastError},
    {"SetLastError",         (void*)stub_SetLastError},
    {"GetTickCount",         (void*)stub_GetTickCount},
    {"VirtualAlloc",         (void*)stub_VirtualAlloc},
    {"VirtualFree",          (void*)stub_VirtualFree},
    {"GetProcessHeap",       (void*)stub_GetProcessHeap},
    {"CloseHandle",          (void*)stub_CloseHandle},
    {"GetCurrentProcessId",  (void*)stub_GetCurrentProcessId},
    {"GetCommandLineA",      (void*)stub_GetCommandLineA},
    {"CreateFileA",          (void*)stub_CreateFileA},
    {"WriteConsoleA",        (void*)stub_WriteConsoleA},
    {"ReadConsoleA",         (void*)stub_ReadConsoleA},
    {"SetConsoleTitleA",     (void*)stub_SetConsoleTitleA},
    {"GetModuleHandleA",     (void*)stub_GetModuleHandleA},
    {"GetProcAddress",       (void*)stub_GetProcAddress},
    {"LoadLibraryA",         (void*)stub_LoadLibraryA},
    {"FreeLibrary",          (void*)stub_FreeLibrary},
    {"WriteConsoleW",        (void*)stub_WriteConsoleW},

    /* msvcrt */
    {"strlen",               (void*)stub_strlen},
    {"strcmp",               (void*)stub_strcmp},
    {"strcpy",               (void*)stub_strcpy},
    {"memcpy",               (void*)stub_memcpy},
    {"memset",               (void*)stub_memset},
    {"puts",                 (void*)stub_puts},
    {"malloc",               (void*)stub_malloc},
    {"free",                 (void*)stub_free},

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
