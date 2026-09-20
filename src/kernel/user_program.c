#include "syscall.h"

/* Helpers de syscall en línea */
static inline uint64_t do_syscall(uint64_t nr, uint64_t a1) {
    uint64_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(nr), "D"(a1)
        : "memory"
    );
    return ret;
}

static void user_print(const char *s) {
    do_syscall(SYS_WRITE, (uint64_t)s);
}

/* Programa de usuario: corre en ring 3 */
void user_program(void) {
    user_print("\n");
    user_print("================================\n");
    user_print("  Hola desde RING 3!\n");
    user_print("  Este texto lo imprime codigo\n");
    user_print("  de usuario, no el kernel.\n");
    user_print("================================\n\n");

    /* Probar el timer: los threads de kernel siguen corriendo */
    user_print("[USER] Esperando...\n");
    for (volatile int i = 0; i < 50000000; i++);

    user_print("[USER] Listo. Saliendo...\n");
    do_syscall(SYS_EXIT, 0);

    /* Nunca se llega */
    for (;;);
}
