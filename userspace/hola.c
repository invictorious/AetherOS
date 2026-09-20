/* Programa de usuario que se ejecuta en ring 3.
 * No usa libc, no tiene main() estándar, empieza en _start.
 */

static inline long syscall3(long nr, long a, long b, long c) {
    long ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(nr), "D"(a), "S"(b), "d"(c)
        : "memory"
    );
    return ret;
}

static void print(const char *s) {
    syscall3(1, (long)s, 0, 0);   /* SYS_WRITE */
}

void _start(void) {
    print("\n");
    print("================================\n");
    print("  HOLA desde un binario\n");
    print("  cargado del disco!\n");
    print("================================\n\n");
    print("Este codigo NO esta en el kernel.\n");
    print("Vive en /HOLA.BIN dentro del disco FAT32.\n");
    print("El kernel lo ha cargado en memoria y ha\n");
    print("saltado a el en ring 3.\n\n");

    syscall3(2, 0, 0, 0);   /* SYS_EXIT */

    for (;;) __asm__ volatile ("hlt");
}
