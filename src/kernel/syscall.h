#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_WRITE  1
#define SYS_EXIT   2
#define SYS_GETPID 3

typedef struct {
    uint64_t nr;
    uint64_t arg1;
    uint64_t arg2;
    uint64_t arg3;
    uint64_t arg4;
    uint64_t arg5;
} syscall_args_t;

void     syscall_init(void);
uint64_t syscall_dispatch(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3);
void     sys_write(const char *s);
void     sys_exit(int code);

#endif
