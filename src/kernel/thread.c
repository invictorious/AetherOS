#include "thread.h"
#include "../mm/heap.h"
#include <stddef.h>

static uint32_t next_id = 1;

thread_t *thread_create(thread_fn_t fn, uint8_t priority, const char *name) {
    (void)name;

    thread_t *t = (thread_t*)kmalloc(sizeof(thread_t));
    if (!t) return NULL;

    uint8_t *stack = (uint8_t*)kmalloc(THREAD_STACK_SIZE);
    if (!stack) { kfree(t); return NULL; }

    t->stack_base = (uint64_t)stack;
    t->id         = next_id++;
    t->state      = THREAD_STATE_READY;
    t->priority   = priority;
    t->entry      = fn;
    t->next       = NULL;

    uint64_t stack_top = (uint64_t)stack + THREAD_STACK_SIZE;
    stack_top &= ~0xFULL;               /* 16-byte aligned */

    uint64_t *sp = (uint64_t*)stack_top;

    /* iretq consume: RIP, CS, RFLAGS, RSP, SS (en 64-bit SIEMPRE los 5) */
    *(--sp) = 0x10;                     /* SS   */
    *(--sp) = stack_top - 0x200;        /* RSP  <-- VALIDO (no 0!) */
    *(--sp) = 0x202;                    /* RFLAGS (IF=1) */
    *(--sp) = 0x08;                     /* CS   */
    *(--sp) = (uint64_t)fn;             /* RIP  */

    /* dummies (los consume 'add rsp, 16' en switch_stack) */
    *(--sp) = 0;                        /* err_code */
    *(--sp) = 0;                        /* int_no   */

    /* registros generales (pop en switch.asm) */
    *(--sp) = 0;  /* rax */
    *(--sp) = 0;  /* rbx */
    *(--sp) = 0;  /* rcx */
    *(--sp) = 0;  /* rdx */
    *(--sp) = 0;  /* rsi */
    *(--sp) = 0;  /* rdi */
    *(--sp) = 0;  /* rbp */
    *(--sp) = 0;  /* r8  */
    *(--sp) = 0;  /* r9  */
    *(--sp) = 0;  /* r10 */
    *(--sp) = 0;  /* r11 */
    *(--sp) = 0;  /* r12 */
    *(--sp) = 0;  /* r13 */
    *(--sp) = 0;  /* r14 */
    *(--sp) = 0;  /* r15 <- RSP apunta aqui */

    t->rsp = (uint64_t)sp;
    return t;
}
