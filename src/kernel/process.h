#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define PROC_READY   0
#define PROC_RUNNING 1
#define PROC_BLOCKED 2
#define PROC_ZOMBIE  3

#define KSTACK_SIZE  16384
#define USTACK_SIZE  16384

typedef struct process {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    uint64_t entry;
    uint64_t kernel_stack_top;
    uint64_t user_stack_top;
    uint64_t saved_rsp;
    int      exit_code;
    struct process *parent;
    struct process *next;
} process_t;

void       process_init(void);
process_t *process_create(uint64_t entry);
process_t *process_current(void);
void       process_set_current(process_t *p);
void       process_add(process_t *p);
void       process_exit(int code);
process_t *process_find(uint32_t pid);
void       process_wait(uint32_t pid);
uint32_t   process_current_pid(void);

#endif
