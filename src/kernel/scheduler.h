#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "thread.h"
#include "../arch/x86_64/idt.h"

void      scheduler_init(void);
void      scheduler_add(thread_t *t);
void      scheduler_tick(registers_t *regs);
thread_t *scheduler_current(void);

#endif
