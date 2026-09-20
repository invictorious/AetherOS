#include "syscall.h"
#include <stddef.h>

/* Usar la misma funcion de escritura que el kernel,
 * para que compartan el mismo cursor */
extern void putchar(char c);

void syscall_init(void) {
    /* Nada por ahora */
}

void sys_write(const char *s) {
    while (*s) putchar(*s++);
}

void sys_exit(int code) {
    (void)code;
    sys_write("\n[USER] Proceso termino.\n");
    for (;;) __asm__ volatile ("hlt");
}

uint64_t syscall_dispatch(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3) {
    (void)a2; (void)a3;
    switch (nr) {
        case SYS_WRITE:
            sys_write((const char*)a1);
            return 0;
        case SYS_EXIT:
            sys_exit((int)a1);
            return 0;
        case SYS_GETPID:
            return 1;
        default:
            sys_write("[SYSCALL] numero desconocido\n");
            return (uint64_t)-1;
    }
}
