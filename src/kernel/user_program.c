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

void user_program(void) {
    user_print("\n");
    user_print("================================\n");
    user_print("  AetherOS v0.1.0\n");
    user_print("  Shell con syscalls de archivo\n");
    user_print("================================\n\n");

    user_print("> cat README.TXT\n\n");
    cmd_cat("README.TXT");

    user_print("\n[USER] Ahora puedes escribir:\n");
    user_print("[USER] > ");

    for (;;) {
        uint64_t c = sys1(SYS_READ, 0);
        if (c != 0) {
            char b[2] = { (char)c, 0 };
            user_print(b);
            if (c == '\n') user_print("[USER] > ");
        }
        __asm__ volatile ("pause");
    }
}
