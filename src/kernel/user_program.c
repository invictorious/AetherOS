#include "syscall.h"

static inline uint64_t sys3(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3) {
    uint64_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)
        : "a"(nr), "D"(a1), "S"(a2), "d"(a3)
        : "memory"
    );
    return ret;
}

static inline uint64_t sys1(uint64_t nr, uint64_t a1) { return sys3(nr, a1, 0, 0); }

static void user_print(const char *s) { sys1(SYS_WRITE, (uint64_t)s); }
static void user_println(const char *s) { user_print(s); user_print("\n"); }

/* Buffer de linea */
static char line_buf[128];
static int  line_len = 0;

static int streq(const char *a, const char *b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return *a == *b;
}

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) { if (*s != *prefix) return 0; s++; prefix++; }
    return 1;
}

static void cmd_cat(const char *filename) {
    int64_t fd = (int64_t)sys1(SYS_OPEN, (uint64_t)filename);
    if (fd < 0) {
        user_print("[cat] no existe: ");
        user_println(filename);
        return;
    }
    static char buf[4096];
    int64_t n;
    while ((n = (int64_t)sys3(SYS_READ_FD, (uint64_t)fd, (uint64_t)buf, sizeof(buf) - 1)) > 0) {
        buf[n] = 0;
        user_print(buf);
    }
    user_println("");
    sys1(SYS_CLOSE, (uint64_t)fd);
}

static void ejecutar_linea(void) {
    if (line_len == 0) return;

    if (streq(line_buf, "help")) {
        user_println("");
        user_println("Comandos: cat <archivo>, exec <bin>, help, clear");
        user_println("");
        return;
    }
    if (streq(line_buf, "clear")) {
        user_print("\x1b[2J");
        return;
    }
    if (starts_with(line_buf, "cat ")) {
        cmd_cat(line_buf + 4);
        return;
    }
    if (starts_with(line_buf, "exec ")) {
        /* SYS_EXEC no retorna: si el binario funciona, no volvemos aqui */
        sys1(SYS_EXEC, (uint64_t)(line_buf + 5));
        user_println("[exec] el binario retorno");
        return;
    }

    user_println("[shell] comando desconocido");
}

void user_program(void) {
    user_print("\n");
    user_print("================================\n");
    user_print("  AetherOS v0.1.1\n");
    user_print("  Shell + cargador de binarios\n");
    user_print("================================\n\n");
    user_print("Escribe 'help' para ver comandos.\n\n");
    user_print("> ");

    for (;;) {
        uint64_t c = sys1(SYS_READ, 0);
        if (c == 0) { __asm__ volatile ("pause"); continue; }

        char ch = (char)c;

        if (ch == '\n') {
            user_print("\n");
            line_buf[line_len] = 0;
            ejecutar_linea();
            line_len = 0;
            user_print("> ");
        } else if (ch == '\b') {
            if (line_len > 0) {
                line_len--;
                user_print("\b \b");
            }
        } else if (ch == '\r') {
            /* ignorar CR */
        } else if (ch >= 32 && ch < 127) {
            if (line_len < (int)sizeof(line_buf) - 1) {
                line_buf[line_len++] = ch;
                char b[2] = { ch, 0 };
                user_print(b);
            }
        }
    }
}
