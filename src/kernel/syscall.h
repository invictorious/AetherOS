#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_WRITE      1
#define SYS_EXIT       2
#define SYS_GETPID     3
#define SYS_READ       4
#define SYS_OPEN       5
#define SYS_READ_FD    6
#define SYS_CLOSE      7
#define SYS_EXEC       8

void     syscall_init(void);
uint64_t syscall_dispatch(uint64_t nr, uint64_t a1, uint64_t a2, uint64_t a3);
void     sys_write(const char *s);
void     sys_exit(int code);
uint64_t sys_read(void);
int64_t  sys_open(const char *name);
int64_t  sys_read_fd(int fd, void *buf, uint64_t n);
int64_t  sys_close(int fd);
void     sys_exec(const char *path) __attribute__((noreturn));

#endif
