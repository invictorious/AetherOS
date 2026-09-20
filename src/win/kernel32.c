#include "win_abi.h"
#include "../kernel/console.h"

/* ------------------------------------------------------------
 * GetStdHandle(DWORD nStdHandle) -> HANDLE
 *   arg1 (RCX) = nStdHandle
 * ------------------------------------------------------------ */
static uint64_t win_GetStdHandle(uint64_t n) {
    /* En Windows moderno, los handles estandar son valores bajos.
     * Aqui hacemos un fake: devolvemos el mismo valor que nos piden. */
    return n & 0xFFFFFFFF;
}

/* ------------------------------------------------------------
 * WriteFile(HANDLE, LPCVOID buf, DWORD n, LPDWORD written, LPOVERLAPPED)
 *   arg1 (RCX) = handle
 *   arg2 (RDX) = buffer
 *   arg3 (R8)  = bytes a escribir
 *   arg4 (R9)  = puntero a "bytes escritos" (o NULL)
 * Devuelve TRUE (1) si OK, FALSE (0) si error
 * ------------------------------------------------------------ */
static uint64_t win_WriteFile(uint64_t handle, uint64_t buf,
                              uint64_t n, uint64_t written_ptr) {
    (void)handle;
    const char *s = (const char*)buf;
    for (uint64_t i = 0; i < n; i++) console_putchar(s[i]);
    if (written_ptr) *(uint32_t*)written_ptr = (uint32_t)n;
    return 1;   /* TRUE */
}

/* ------------------------------------------------------------
 * ReadFile(HANDLE, LPVOID buf, DWORD n, LPDWORD read, LPOVERLAPPED)
 * ------------------------------------------------------------ */
extern uint64_t sys_read(void);
static uint64_t win_ReadFile(uint64_t handle, uint64_t buf,
                             uint64_t n, uint64_t read_ptr) {
    (void)handle;
    if (n == 0) return 0;
    char *dst = (char*)buf;
    dst[0] = (char)sys_read();
    if (read_ptr) *(uint32_t*)read_ptr = 1;
    return 1;
}

/* ------------------------------------------------------------
 * ExitProcess(UINT code)
 *   arg1 (RCX) = codigo de salida
 * ------------------------------------------------------------ */
extern void sys_exit(int code);
static uint64_t win_ExitProcess(uint64_t code) {
    sys_exit((int)code);
    return 0;   /* no retorna */
}

/* ------------------------------------------------------------
 * Sleep(DWORD ms) - cede el CPU
 * ------------------------------------------------------------ */
static uint64_t win_Sleep(uint64_t ms) {
    (void)ms;
    /* Cede el CPU una vez */
    __asm__ volatile ("sti; hlt; cli");
    return 0;
}

/* ------------------------------------------------------------
 * Dispatcher
 * ------------------------------------------------------------ */
void win_init(void) { }

uint64_t win_dispatch(uint64_t nr, uint64_t a1, uint64_t a2,
                      uint64_t a3, uint64_t a4) {
    switch (nr) {
        case WIN_SYS_GETSTDHANDLE: return win_GetStdHandle(a1);
        case WIN_SYS_WRITEFILE:    return win_WriteFile(a1, a2, a3, a4);
        case WIN_SYS_READFILE:     return win_ReadFile(a1, a2, a3, a4);
        case WIN_SYS_EXITPROCESS:  return win_ExitProcess(a1);
        case WIN_SYS_SLEEP:        return win_Sleep(a1);
        default:
            console_write("[WIN] syscall desconocida: ");
            console_putchar('0' + (nr & 0xF));
            console_write("\n");
            return 0;
    }
}
