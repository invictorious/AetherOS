#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include "../arch/x86_64/idt.h"

void scheduler_init(void);
void scheduler_tick(registers_t *regs);
void scheduler_start(process_t *first);
process_t *scheduler_current(void);
void scheduler_enter_critical(void);
void scheduler_exit_critical(void);

#endif
