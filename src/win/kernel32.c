#include "win_abi.h"
#include "../kernel/console.h"
#include "../mm/heap.h"

extern void *win_lookup_export(const char *dll, const char *func);
extern uint64_t sys_read(void);
extern int64_t sys_read_fd(int fd, void *buf, uint64_t n);
extern void sys_exit(int code);
extern uint32_t process_current_pid(void);

/* ------------------------------------------------------------
 * kernel32
 * ------------------------------------------------------------ */
static uint64_t win_GetStdHandle(uint64_t n) {
    return n & 0xFFFFFFFF;
}

static uint64_t win_WriteFile(uint64_t handle, uint64_t buf,
                              uint64_t n, uint64_t written_ptr) {
    (void)handle;
    const char *s = (const char*)buf;
    for (uint64_t i = 0; i < n; i++) console_putchar(s[i]);
    if (written_ptr) *(uint32_t*)written_ptr = (uint32_t)n;
    return 1;
}

static uint64_t win_ReadFile(uint64_t handle, uint64_t buf,
                             uint64_t n, uint64_t read_ptr) {
    /* Si el handle es un FD de archivo (>= 3 del kernel), leer del archivo.
     * Si es handle estandar (STD_INPUT = -10 -> 0xFFFFFFF6, etc.), leer del teclado. */
    uint32_t h32 = (uint32_t)handle;

    /* Handles estandar de Windows: -10, -11, -12 (como DWORD) */
    if (h32 == 0xFFFFFFF5u || h32 == 0xFFFFFFF4u || h32 == 0xFFFFFFF6u) {
        /* STD_INPUT/OUTPUT/ERROR - solo INPUT tiene sentido leer */
        if (n == 0) return 0;
        ((char*)buf)[0] = (char)sys_read();
        if (read_ptr) *(uint32_t*)read_ptr = 1;
        return 1;
    }

    /* Si no, asumimos FD de archivo del kernel */
    int64_t got = sys_read_fd((int)handle, (void*)buf, n);
    if (got < 0) {
        if (read_ptr) *(uint32_t*)read_ptr = 0;
        return 0;
    }
    if (read_ptr) *(uint32_t*)read_ptr = (uint32_t)got;
    /* Windows devuelve TRUE incluso si got==0 (EOF); el .exe
     * normalmente mira *read_ptr para saber si ha terminado */
    return 1;
}

static uint64_t win_ExitProcess(uint64_t code) {
    sys_exit((int)code);
    return 0;
}

static uint64_t win_Sleep(uint64_t ms) {
    (void)ms;
    __asm__ volatile ("sti; hlt; cli");
    return 0;
}

static uint64_t win_GetLastError(void)  { return 0; }
static uint64_t win_SetLastError(uint64_t e) { (void)e; return 0; }

static uint64_t win_GetTickCount(void) {
    extern volatile uint64_t ticks;
    return ticks * 10;   /* 100 Hz -> ms */
}

static uint64_t win_VirtualAlloc(uint64_t addr, uint64_t size,
                                  uint64_t type, uint64_t prot) {
    (void)addr; (void)type; (void)prot;
    return (uint64_t)kmalloc(size);
}

static uint64_t win_VirtualFree(uint64_t addr, uint64_t size, uint64_t type) {
    (void)size; (void)type;
    if (addr) kfree((void*)addr);
    return 1;
}

static uint64_t win_GetProcessHeap(void) { return 0x10000; }

static uint64_t win_CloseHandle(uint64_t h) { (void)h; return 1; }

static uint64_t win_GetCurrentProcessId(void) {
    return process_current_pid();
}

static uint64_t win_GetCommandLineA(void) {
    static char cmdline[] = "program.exe";
    return (uint64_t)cmdline;
}

/* ------------------------------------------------------------
 * msvcrt (simples)
 * ------------------------------------------------------------ */
static uint64_t win_strlen(uint64_t s) {
    const char *p = (const char*)s;
    uint64_t n = 0;
    while (p[n]) n++;
    return n;
}

static uint64_t win_strcmp(uint64_t a, uint64_t b) {
    const unsigned char *p = (const unsigned char*)a;
    const unsigned char *q = (const unsigned char*)b;
    while (*p && *p == *q) { p++; q++; }
    return (int)*p - (int)*q;
}

static uint64_t win_strcpy(uint64_t dst, uint64_t src) {
    char *d = (char*)dst;
    const char *s = (const char*)src;
    while ((*d++ = *s++));
    return dst;
}

static uint64_t win_memcpy(uint64_t dst, uint64_t src, uint64_t n) {
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    for (uint64_t i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

static uint64_t win_memset(uint64_t dst, uint64_t val, uint64_t n) {
    uint8_t *d = (uint8_t*)dst;
    for (uint64_t i = 0; i < n; i++) d[i] = (uint8_t)val;
    return dst;
}

static uint64_t win_puts(uint64_t s) {
    const char *p = (const char*)s;
    while (*p) console_putchar(*p++);
    console_putchar('\n');
    return 0;
}

static uint64_t win_malloc(uint64_t size) { return (uint64_t)kmalloc(size); }
static uint64_t win_free(uint64_t p) { if (p) kfree((void*)p); return 0; }

/* ------------------------------------------------------------
 * Dispatcher
 * ------------------------------------------------------------ */

/* ------------------------------------------------------------
 * CreateFileA - abrimos archivos del disco FAT32
 * Simplificacion: solo lectura, devolvemos un indice de FD
 * ------------------------------------------------------------ */
#define MAX_WIN_FD 8

typedef struct {
    int      used;
    uint32_t size;
    uint32_t pos;
    char     name[64];
} win_fd_t;

static win_fd_t g_fds[MAX_WIN_FD];

extern int64_t sys_open(const char *name);
extern int64_t sys_close(int fd);

static uint64_t win_CreateFileA(uint64_t name_ptr, uint64_t access,
                                 uint64_t share, uint64_t sec,
                                 uint64_t disp, uint64_t flags, uint64_t templ) {
    (void)access; (void)share; (void)sec; (void)disp; (void)flags; (void)templ;
    const char *name = (const char*)name_ptr;

    /* Copiar nombre al buffer, convirtiendo '\\' en '/' (por si acaso) */
    char clean[64];
    int i = 0;
    while (name[i] && i < 63) { clean[i] = name[i]; i++; }
    clean[i] = 0;

    int64_t kfd = sys_open(clean);
    if (kfd < 0) return (uint64_t)-1;   /* INVALID_HANDLE_VALUE */

    /* Buscar slot en la tabla */
    int slot = -1;
    for (int k = 0; k < MAX_WIN_FD; k++) {
        if (!g_fds[k].used) { slot = k; break; }
    }
    if (slot < 0) {
        sys_close((int)kfd);
        return (uint64_t)-1;
    }

    /* Leer todo el archivo en memoria del FD kfd al slot */
    g_fds[slot].used = 1;
    g_fds[slot].pos  = 0;
    /* Necesitamos conocer el tamano: lo leemos completo por chunks */
    /* Simplificacion: usar sys_read_fd hasta agotar */
    /* Pero sys_read_fd solo lee un buffer; necesitamos leer en el sys_open */
    /* Como nuestro sys_open ya tiene todo el archivo, simplemente lo tratamos como FD kfd */

    g_fds[slot].size = 0;   /* lo rellenaremos cuando leamos */
    g_fds[slot].used = 0;   /* mejor: no usar la tabla win_fd, mapear directamente */

    /* Mapeo directo: el FD de kernel32 = kfd del kernel */
    return (uint64_t)kfd;
}

/* ------------------------------------------------------------
 * WriteConsoleA - igual que WriteFile, pero con handle de consola
 * ------------------------------------------------------------ */
static uint64_t win_WriteConsoleA(uint64_t h, uint64_t buf,
                                   uint64_t n, uint64_t written_ptr,
                                   uint64_t reserved) {
    (void)h; (void)reserved;
    return win_WriteFile(0, buf, n, written_ptr);
}

static uint64_t win_WriteConsoleW(uint64_t h, uint64_t buf,
                                   uint64_t n, uint64_t written_ptr,
                                   uint64_t reserved) {
    (void)h; (void)n; (void)reserved;
    /* Convertir UTF-16 -> ASCII simple */
    const uint16_t *src = (const uint16_t*)buf;
    uint32_t count = 0;
    while (src[count]) {
        char c = (char)(src[count] & 0xFF);
        console_putchar(c);
        count++;
    }
    if (written_ptr) *(uint32_t*)written_ptr = count;
    return 1;
}

/* ------------------------------------------------------------
 * ReadConsoleA - leer una linea de la consola
 * ------------------------------------------------------------ */
static uint64_t win_ReadConsoleA(uint64_t h, uint64_t buf, uint64_t n,
                                  uint64_t read_ptr, uint64_t reserved) {
    (void)h; (void)reserved;
    char *dst = (char*)buf;
    uint64_t i = 0;
    while (i < n - 1) {
        uint64_t c = sys_read();
        if (c == 0) { __asm__ volatile ("pause"); continue; }
        dst[i++] = (char)c;
        console_putchar((char)c);
        if (c == '\n') break;
    }
    dst[i] = 0;
    if (read_ptr) *(uint32_t*)read_ptr = (uint32_t)i;
    return 1;
}

static uint64_t win_SetConsoleTitleA(uint64_t title) {
    (void)title;
    return 1;
}

static uint64_t win_GetModuleHandleA(uint64_t name) {
    (void)name;
    return 0x400000;   /* Direccion base del .exe */
}

static uint64_t win_GetProcAddress(uint64_t mod, uint64_t name) {
    (void)mod;
    return (uint64_t)win_lookup_export("", (const char*)name);
}

static uint64_t win_LoadLibraryA(uint64_t name) {
    (void)name;
    return 0x400000;   /* fake */
}

static uint64_t win_FreeLibrary(uint64_t h) { (void)h; return 1; }


void win_init(void) { }

uint64_t win_dispatch(uint64_t nr, uint64_t a1, uint64_t a2,
                      uint64_t a3, uint64_t a4) {
    switch (nr) {
        case WIN_WRITEFILE:      return win_WriteFile(a1, a2, a3, a4);
        case WIN_READFILE:       return win_ReadFile(a1, a2, a3, a4);
        case WIN_EXITPROCESS:    return win_ExitProcess(a1);
        case WIN_GETSTDHANDLE:   return win_GetStdHandle(a1);
        case WIN_GETCMDLINEA:    return win_GetCommandLineA();
        case WIN_SLEEP:          return win_Sleep(a1);
        case WIN_GETLASTERROR:   return win_GetLastError();
        case WIN_SETLASTERROR:   return win_SetLastError(a1);
        case WIN_GETTICKCOUNT:   return win_GetTickCount();
        case WIN_VIRTUALALLOC:   return win_VirtualAlloc(a1, a2, a3, a4);
        case WIN_VIRTUALFREE:    return win_VirtualFree(a1, a2, a3);
        case WIN_GETPROCESSHEAP: return win_GetProcessHeap();
        case WIN_CLOSEHANDLE:    return win_CloseHandle(a1);
        case WIN_GETPID:         return win_GetCurrentProcessId();

        case WIN_CREATEFILEA:      return win_CreateFileA(a1, a2, a3, a4, 0, 0, 0);
        case WIN_WRITECONSOLEA:    return win_WriteConsoleA(a1, a2, a3, a4, 0);
        case WIN_WRITECONSOLEW:    return win_WriteConsoleW(a1, a2, a3, a4, 0);
        case WIN_READCONSOLEA:     return win_ReadConsoleA(a1, a2, a3, a4, 0);
        case WIN_SETCONSOLETITLEA: return win_SetConsoleTitleA(a1);
        case WIN_GETMODULEHANDLEA: return win_GetModuleHandleA(a1);
        case WIN_GETPROCADDRESS:   return win_GetProcAddress(a1, a2);
        case WIN_LOADLIBRARYA:     return win_LoadLibraryA(a1);
        case WIN_FREELIBRARY:      return win_FreeLibrary(a1);

        case WIN_STRLEN:  return win_strlen(a1);
        case WIN_STRCMP:  return win_strcmp(a1, a2);
        case WIN_STRCPY:  return win_strcpy(a1, a2);
        case WIN_MEMCPY:  return win_memcpy(a1, a2, a3);
        case WIN_MEMSET:  return win_memset(a1, a2, a3);
        case WIN_PUTS:    return win_puts(a1);
        case WIN_MALLOC:  return win_malloc(a1);
        case WIN_FREE:    return win_free(a1);

        default:
            /* Si es un syscall de user32 (0x3000+), delegar */
            if (nr >= 0x3000 && nr < 0x4000) {
                extern uint64_t user32_dispatch(uint64_t, uint64_t,
                                                 uint64_t, uint64_t, uint64_t);
                return user32_dispatch(nr, a1, a2, a3, a4);
            }
            console_printf("[WIN] syscall 0x%x desconocida\n", (unsigned)nr);
            return 0;
    }
}
