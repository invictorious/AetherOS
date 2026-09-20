#include "syscall.h"
#include "console.h"
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
    (void)code;
    console_write("\n[USER] Proceso termino.\n");
    for (;;) __asm__ volatile ("hlt");
}

uint64_t sys_read(void) {
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

uint64_t syscall_dispatch(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3) {
    switch (nr) {
        case SYS_WRITE:   sys_write((const char*)a1); return 0;
        case SYS_EXIT:    sys_exit((int)a1); return 0;
        case SYS_GETPID:  return 1;
        case SYS_READ:    return sys_read();
        case SYS_OPEN:    return (uint64_t)sys_open((const char*)a1);
        case SYS_READ_FD: return (uint64_t)sys_read_fd((int)a1, (void*)a2, a3);
        case SYS_CLOSE:   return (uint64_t)sys_close((int)a1);
        default:
            console_write("[SYSCALL] numero desconocido\n");
            return (uint64_t)-1;
    }
}
