#include "scheduler.h"
#include "../mm/heap.h"

extern void switch_stack(uint64_t new_rsp) __attribute__((noreturn));

static thread_t *head    = NULL;
static thread_t *current = NULL;

void scheduler_init(void) {
    head = NULL;
    current = NULL;
}

void scheduler_add(thread_t *t) {
    if (!t) return;
    t->next = NULL;
    if (!head) {
        head = t;
        t->next = t;
    } else {
        thread_t *p = head;
        while (p->next != head) p = p->next;
        p->next = t;
        t->next = head;
    }
}

thread_t *scheduler_current(void) {
    return current;
}

void scheduler_tick(registers_t *regs) {
    if (!current) {
        current = head;
        current->state = THREAD_STATE_RUNNING;
        switch_stack(current->rsp);
    }

    current->rsp = (uint64_t)regs;

    thread_t *next = current->next;
    while (next != current) {
        if (next->state == THREAD_STATE_READY) break;
        next = next->next;
    }

    if (next == current) return;

    current->state = THREAD_STATE_READY;
    next->state    = THREAD_STATE_RUNNING;
    current        = next;

    switch_stack(current->rsp);
}
