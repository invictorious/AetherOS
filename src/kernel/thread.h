#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

#define THREAD_STATE_READY   0
#define THREAD_STATE_RUNNING 1
#define THREAD_STATE_BLOCKED 2
#define THREAD_STATE_DEAD    3

#define THREAD_STACK_SIZE (16 * 1024)

typedef void (*thread_fn_t)(void);

typedef struct thread {
    uint64_t       rsp;
    uint64_t       stack_base;
    uint32_t       id;
    uint32_t       state;
    uint8_t        priority;
    thread_fn_t    entry;
    struct thread *next;
} thread_t;

thread_t *thread_create(thread_fn_t fn, uint8_t priority, const char *name);
thread_t *thread_current(void);
void      thread_set_current(thread_t *t);
void      thread_exit(void);
void      thread_idle(void);

#endif
