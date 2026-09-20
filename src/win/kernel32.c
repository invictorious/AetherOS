#include "win_abi.h"
#include "../kernel/console.h"

extern uint64_t sys_read(void);
extern void sys_exit(int code);

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
    (void)handle;
    if (n == 0) return 0;
    ((char*)buf)[0] = (char)sys_read();
    if (read_ptr) *(uint32_t*)read_ptr = 1;
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
            console_write("[WIN] syscall desconocida\n");
            return 0;
    }
}
