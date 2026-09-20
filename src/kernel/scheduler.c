#include "scheduler.h"
#include "console.h"

extern void switch_stack(uint64_t new_rsp) __attribute__((noreturn));
extern void tss_set_kernel_stack(uint64_t rsp0);

static process_t *current = NULL;
static volatile int in_critical = 0;

void scheduler_enter_critical(void) { in_critical = 1; }
void scheduler_exit_critical(void)  { in_critical = 0; }

void scheduler_init(void) { current = NULL; }

void scheduler_start(process_t *first) {
    current = first;
    current->state = PROC_RUNNING;
    tss_set_kernel_stack(current->kernel_stack_top);
    switch_stack(current->saved_rsp);
}

void scheduler_tick(registers_t *regs) {
    if (!current) return;
    if (in_critical) return;

    current->saved_rsp = (uint64_t)regs;

    /* Buscar siguiente proceso elegible:
     * acepta RUNNING y READY, ignora ZOMBIE y BLOCKED */
    process_t *next = current->next;
    while (next != current) {
        if (next->state == PROC_READY || next->state == PROC_RUNNING) break;
        next = next->next;
    }

    if (next == current) {
        if (current->state == PROC_ZOMBIE) {
            console_write("[sched] Sin procesos vivos\n");
            for (;;) __asm__ volatile ("hlt");
        }
        return;
    }

    if (current->state == PROC_RUNNING) current->state = PROC_READY;
    next->state = PROC_RUNNING;
    current = next;

    tss_set_kernel_stack(current->kernel_stack_top);
    switch_stack(current->saved_rsp);
}

process_t *scheduler_current(void) { return current; }
