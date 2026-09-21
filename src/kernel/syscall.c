#include "syscall.h"
#include "console.h"
#include "../kernel/console.h"
#include "exec.h"
#include "console.h"
#include "../kernel/console.h"
#include "process.h"
#include "scheduler.h"
#include "../drivers/keyboard.h"
#include "../fs/fat32.h"

#define MAX_FDS 16
#define FD_BUF_SIZE 4096

typedef struct {
    int      used;
    uint32_t size;
    uint32_t pos;
    uint8_t  data[FD_BUF_SIZE];
} fd_t;

static fd_t fd_table[MAX_FDS];

void syscall_init(void) {
    for (int i = 0; i < MAX_FDS; i++) fd_table[i].used = 0;
}

void sys_write(const char *s) { console_write(s); }

void sys_exit(int code) {
    process_exit(code);
}

void sys_wait(uint32_t pid) { process_wait(pid); }

uint64_t sys_read(void) {
    extern int wm_terminal_focused(void);
    /* Solo leemos teclado si la ventana de terminal tiene el foco */
    if (!wm_terminal_focused()) {
        /* Descartar teclas acumuladas mientras no hay foco */
        while (keyboard_has_key()) (void)keyboard_getchar();
        return 0;
    }
    if (!keyboard_has_key()) return 0;
    return (uint64_t)(uint8_t)keyboard_getchar();
}

int64_t sys_open(const char *name) {
    int fd = -1;
    for (int i = 3; i < MAX_FDS; i++) {
        if (!fd_table[i].used) { fd = i; break; }
    }
    if (fd < 0) return -1;

    fat32_dir_entry_t entry;
    if (fat32_open(name, &entry) != 0) return -1;

    fd_table[fd].used = 1;
    fd_table[fd].pos  = 0;
    fd_table[fd].size = fat32_read_file(&entry, fd_table[fd].data, FD_BUF_SIZE);
    return fd;
}

int64_t sys_read_fd(int fd, void *buf, uint64_t n) {
    if (fd < 0 || fd >= MAX_FDS) return -1;
    if (!fd_table[fd].used) return -1;
    uint32_t remaining = fd_table[fd].size - fd_table[fd].pos;
    if (remaining == 0) return 0;
    if (n > remaining) n = remaining;
    uint8_t *dst = (uint8_t*)buf;
    for (uint64_t i = 0; i < n; i++) {
        dst[i] = fd_table[fd].data[fd_table[fd].pos++];
    }
    return (int64_t)n;
}

int64_t sys_close(int fd) {
    if (fd < 0 || fd >= MAX_FDS) return -1;
    if (!fd_table[fd].used) return -1;
    fd_table[fd].used = 0;
    return 0;
}

void sys_exec(const char *path) {
    exec_run(path);
}

/* Spawn: crea un proceso nuevo desde un ELF y lo mete en el scheduler */
int64_t sys_spawn(const char *path) {
    /* Sin preemption durante el spawn: el hijo no debe correr
     * hasta que volvamos al shell */
    __asm__ volatile ("cli");

    extern void scheduler_enter_critical(void);
    extern void scheduler_exit_critical(void);
    scheduler_enter_critical();

    uint64_t entry = exec_load_elf(path);
    if (entry == 0) {
        scheduler_exit_critical();
        __asm__ volatile ("sti");
        return -1;
    }

    process_t *p = process_create(entry);
    if (!p) {
        scheduler_exit_critical();
        __asm__ volatile ("sti");
        return -1;
    }

    process_add(p);

    scheduler_exit_critical();
    __asm__ volatile ("sti");
    return (int64_t)p->pid;
}

/* Estructura de info del sistema que devolvemos al usuario */
typedef struct {
    uint64_t total_mem;      /* bytes */
    uint64_t free_mem;
    uint64_t heap_used;
    uint64_t ticks;          /* 100 Hz */
    uint32_t num_procs;
    uint32_t pids[16];
    uint32_t states[16];
    char     names[16][24];
} sysinfo_t;

uint64_t sys_getinfo(sysinfo_t *out) {
    if (!out) return (uint64_t)-1;
    extern uint64_t pmm_total_memory(void);
    extern uint64_t pmm_free_memory(void);
    extern size_t heap_used(void);
    extern volatile uint64_t ticks;

    out->total_mem  = pmm_total_memory();
    out->free_mem   = pmm_free_memory();
    out->heap_used  = heap_used();
    out->ticks      = ticks;

    /* Recorrer procesos (variable estatica de process.c) */
    extern void process_dump_for_sysinfo(sysinfo_t *info);
    process_dump_for_sysinfo(out);
    return 0;
}

uint64_t syscall_dispatch(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3) {
    switch (nr) {
        case SYS_WRITE:   sys_write((const char*)a1); return 0;
        case SYS_EXIT:    sys_exit((int)a1); return 0;
        case SYS_GETPID:  return process_current_pid();
        case SYS_READ:    return sys_read();
        case SYS_OPEN:    return (uint64_t)sys_open((const char*)a1);
        case SYS_READ_FD: return (uint64_t)sys_read_fd((int)a1, (void*)a2, a3);
        case SYS_CLOSE:   return (uint64_t)sys_close((int)a1);
        case SYS_CLEAR:   console_clear(); return 0;
        case SYS_GETINFO: return sys_getinfo((sysinfo_t*)a1);
        case SYS_EXEC:    return (uint64_t)sys_spawn((const char*)a1);  /* alias a spawn */
        case SYS_SPAWN:   return (uint64_t)sys_spawn((const char*)a1);
        case SYS_WAIT:    sys_wait((uint32_t)a1); return 0;
        default:
            console_write("[SYSCALL] numero desconocido\n");
            return (uint64_t)-1;
    }
}
