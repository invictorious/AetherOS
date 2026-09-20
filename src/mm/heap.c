#include "heap.h"
#include <stdint.h>

/* Mapa de memoria:
 * 0x000000 - 0x200000 : kernel + page tables
 * 0x300000 - 0x400000 : buffer FAT32
 * 0x400000 - 0x800000 : usuario (ELF) + pila
 * 0x1000000 - 0x1F00000 : HEAP (15 MB)
 */
#define HEAP_START 0x1000000
#define HEAP_SIZE  0xF00000
#define ALIGN 16
#define ALIGN_UP(x) (((x) + (ALIGN - 1)) & ~(ALIGN - 1))

typedef struct block {
    size_t        size;
    int           free;
    struct block *next;
    struct block *prev;
} block_t;

#define HEADER_SIZE sizeof(block_t)

static block_t *head = NULL;
static size_t   used_bytes = 0;
static size_t   total_bytes = 0;

void heap_init(void) {
    head = (block_t*)HEAP_START;
    head->size = HEAP_SIZE - HEADER_SIZE;
    head->free = 1;
    head->next = NULL;
    head->prev = NULL;
    used_bytes = 0;
    total_bytes = HEAP_SIZE;
}

static void split(block_t *b, size_t size) {
    if (b->size < size + HEADER_SIZE + ALIGN) return;
    block_t *nb = (block_t*)((uint8_t*)b + HEADER_SIZE + size);
    nb->size = b->size - size - HEADER_SIZE;
    nb->free = 1;
    nb->next = b->next;
    nb->prev = b;
    if (b->next) b->next->prev = nb;
    b->next = nb;
    b->size = size;
}

static void coalesce(block_t *b) {
    if (b->next && b->next->free) {
        b->size += HEADER_SIZE + b->next->size;
        b->next = b->next->next;
        if (b->next) b->next->prev = b;
    }
    if (b->prev && b->prev->free) {
        b->prev->size += HEADER_SIZE + b->size;
        b->prev->next = b->next;
        if (b->next) b->next->prev = b->prev;
    }
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = ALIGN_UP(size);
    block_t *b = head;
    while (b) {
        if (b->free && b->size >= size) {
            split(b, size);
            b->free = 0;
            used_bytes += b->size + HEADER_SIZE;
            return (uint8_t*)b + HEADER_SIZE;
        }
        b = b->next;
    }
    return NULL;
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_t *b = (block_t*)((uint8_t*)ptr - HEADER_SIZE);
    b->free = 1;
    used_bytes -= b->size + HEADER_SIZE;
    coalesce(b);
}

size_t heap_total(void) { return total_bytes; }
size_t heap_used(void)  { return used_bytes; }
