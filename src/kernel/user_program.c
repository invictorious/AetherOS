#include "syscall.h"

static inline uint64_t sys(uint64_t nr, uint64_t a1) {
    uint64_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(nr), "D"(a1)
        : "memory"
    );
    return ret;
}

static inline uint64_t sys_read_key(void) {
    return sys(SYS_READ, 0);
}

static void user_print(const char *s) {
    sys(SYS_WRITE, (uint64_t)s);
}

void user_program(void) {
    user_print("\n");
    user_print("================================\n");
    user_print("  Hola desde RING 3!\n");
    user_print("  Prueba a escribir teclas.\n");
    user_print("================================\n\n");
    user_print("[USER] > ");

    for (;;) {
        uint64_t c = sys_read_key();
        if (c != 0) {
            char buf[2];
            buf[0] = (char)c;
            buf[1] = 0;
            user_print(buf);
            if (c == '\n') user_print("[USER] > ");
        }
        /* No busy-wait agresivo: pausa con pause */
        __asm__ volatile ("pause");
    }
}
