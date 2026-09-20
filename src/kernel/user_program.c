#include "syscall.h"
#include "version.h"

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

static void user_print_dec(uint64_t v) {
    if (v == 0) { user_print("0"); return; }
    char b[24]; int i = 0;
    while (v) { b[i++] = '0' + (v % 10); v /= 10; }
    while (i--) { char c[2] = { b[i], 0 }; user_print(c); }
}

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
    if (fd < 0) { user_print("[cat] no existe\n"); return; }
    static char buf[4096];
    int64_t n;
    while ((n = (int64_t)sys3(SYS_READ_FD, (uint64_t)fd, (uint64_t)buf, sizeof(buf) - 1)) > 0) {
        buf[n] = 0;
        user_print(buf);
    }
    user_println("");
    sys1(SYS_CLOSE, (uint64_t)fd);
}

static void cmd_run(const char *path) {
    user_print("[shell] creando proceso: ");
    user_println(path);

    int64_t pid = (int64_t)sys1(SYS_SPAWN, (uint64_t)path);
    if (pid < 0) {
        user_println("[shell] fallo al crear proceso");
        return;
    }
    user_print("[shell] PID = ");
    user_print_dec((uint64_t)pid);
    user_println("");
    user_println("[shell] esperando a que termine...");
    sys1(SYS_WAIT, (uint64_t)pid);
    user_println("[shell] proceso terminado");
}

static void ejecutar_linea(void) {
    if (line_len == 0) return;

    if (streq(line_buf, "help")) {
        user_println("");
        user_println("Comandos:");
        user_println("  cat <archivo>    - muestra archivo");
        user_println("  run <archivo>    - ejecuta ELF o PE");
        user_println("  pid              - muestra mi PID");
        user_println("  help             - esta ayuda");
        user_println("  clear            - limpia pantalla");
        user_println("");
        user_println("Atajos:");
        user_println("  exe              - run HELLO.EXE");
        user_println("  exe3             - run HELLO3.EXE");
        user_println("  exe4             - run HELLO4.EXE");
        user_println("  msgbox           - run MSGBOX.EXE");
        user_println("  win              - run WIN.BIN");
        user_println("");
        return;
    }
    if (streq(line_buf, "pid")) {
        user_print("PID actual: ");
        user_print_dec(sys1(SYS_GETPID, 0));
        user_println("");
        return;
    }
    if (streq(line_buf, "clear")) {
        sys1(SYS_CLEAR, 0);
        return;
    }
    if (streq(line_buf, "exe"))  { cmd_run("HELLO.EXE");  return; }
    if (streq(line_buf, "exe3")) { cmd_run("HELLO3.EXE"); return; }
    if (streq(line_buf, "exe4")) { cmd_run("HELLO4.EXE"); return; }
    if (streq(line_buf, "msgbox")) { cmd_run("MSGBOX.EXE"); return; }
    if (streq(line_buf, "win"))    { cmd_run("WIN.BIN");    return; }
    if (starts_with(line_buf, "cat "))  { cmd_cat(line_buf + 4); return; }
    if (starts_with(line_buf, "run "))  { cmd_run(line_buf + 4); return; }
    user_println("[shell] comando desconocido");
}

void user_program(void) {
    user_print("\n");
    user_print(AETHEROS_BANNER_SHELL);
    user_print("\nEscribe 'help' para ver comandos.\n\n");
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
            if (line_len > 0) { line_len--; user_print("\b \b"); }
        } else if (ch == '\r') {
            /* ignorar */
        } else if (ch >= 32 && ch < 127) {
            if (line_len < (int)sizeof(line_buf) - 1) {
                line_buf[line_len++] = ch;
                char b[2] = { ch, 0 };
                user_print(b);
            }
        }
    }
}
