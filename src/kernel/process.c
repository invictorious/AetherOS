#include "process.h"
#include "console.h"
#include "scheduler.h"
#include "../mm/heap.h"

static uint32_t   next_pid = 1;
static process_t *head     = NULL;

/* process.c NO tiene su propio 'current': usa el del scheduler */
#define current scheduler_current()

void process_init(void) {
    next_pid = 1;
    head = NULL;
}

process_t *process_current(void) { return scheduler_current(); }
void       process_set_current(process_t *p) { (void)p; }
uint32_t   process_current_pid(void) {
    process_t *c = scheduler_current();
    return c ? c->pid : 0;
}

void process_add(process_t *p) {
    if (!p) return;
    if (!head) { head = p; p->next = p; }
    else {
        process_t *q = head;
        while (q->next != head) q = q->next;
        q->next = p;
        p->next = head;
    }
}

process_t *process_find(uint32_t pid) {
    if (!head) return NULL;
    process_t *p = head;
    do {
        if (p->pid == pid) return p;
        p = p->next;
    } while (p != head);
    return NULL;
}

process_t *process_create(uint64_t entry) {
    process_t *p = (process_t*)kmalloc(sizeof(process_t));
    if (!p) return NULL;

    p->pid       = next_pid++;
    p->ppid      = process_current_pid();
    p->state     = PROC_READY;
    p->entry     = entry;
    p->exit_code = 0;
    p->parent    = scheduler_current();
    p->next      = NULL;

    uint8_t *kstack = (uint8_t*)kmalloc(KSTACK_SIZE);
    if (!kstack) { kfree(p); return NULL; }
    p->kernel_stack_top = ((uint64_t)kstack + KSTACK_SIZE) & ~0xFULL;

    uint8_t *ustack = (uint8_t*)kmalloc(USTACK_SIZE);
    if (!ustack) { kfree(kstack); kfree(p); return NULL; }
    p->user_stack_top = ((uint64_t)ustack + USTACK_SIZE) & ~0xFULL;

    uint64_t *sp = (uint64_t*)p->kernel_stack_top;

    *(--sp) = 0x23;
    *(--sp) = p->user_stack_top;
    *(--sp) = 0x202;
    *(--sp) = 0x1B;
    *(--sp) = entry;

    *(--sp) = 0;
    *(--sp) = 0;
    for (int i = 0; i < 15; i++) *(--sp) = 0;

    p->saved_rsp = (uint64_t)sp;
    return p;
}

void process_exit(int code) {
    process_t *c = scheduler_current();
    if (!c) return;
    c->exit_code = code;
    c->state = PROC_ZOMBIE;
    /* Ceder CPU. El PIT disparara, el scheduler buscara otro proceso */
    for (;;) __asm__ volatile ("sti; hlt");
}

void process_wait(uint32_t pid) {
    process_t *child = process_find(pid);
    if (!child) return;
    /* Bucle simple: ceder el CPU hasta que el hijo sea ZOMBIE */
    while (child->state != PROC_ZOMBIE) {
        __asm__ volatile ("sti; hlt; cli");
    }
}
