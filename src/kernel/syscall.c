#include "syscall.h"
#include "console.h"
#include "../drivers/keyboard.h"

void syscall_init(void) { }

void sys_write(const char *s) { console_write(s); }

void sys_exit(int code) {
    (void)code;
    console_write("\n[USER] Proceso termino.\n");
    for (;;) __asm__ volatile ("hlt");
}

uint64_t sys_read(void) {
    if (!keyboard_has_key()) return 0;
    return (uint64_t)(uint8_t)keyboard_getchar();
}

uint64_t syscall_dispatch(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    switch (nr) {
        case SYS_WRITE:  sys_write((const char*)a1); return 0;
        case SYS_EXIT:   sys_exit((int)a1); return 0;
        case SYS_GETPID: return 1;
        case SYS_READ:   return sys_read();
        default:
            console_write("[SYSCALL] numero desconocido\n");
            return (uint64_t)-1;
    }
}
