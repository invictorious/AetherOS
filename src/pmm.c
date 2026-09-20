#include "pmm.h"

typedef struct __attribute__((packed)) {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attr;
} e820_entry_t;

#define E820_COUNT   (*(volatile uint32_t*)0x8000)
#define E820_ENTRIES ((e820_entry_t*)0x8004)
#define E820_TYPE_FREE 1

#define BITMAP_ADDR 0x200000
#define BITMAP_SIZE (128 * 1024)

#define HEAP_START  0x400000
#define HEAP_SIZE   0x1000000

static uint8_t  *bitmap = (uint8_t*)BITMAP_ADDR;
static uint64_t total_pages = 0;
static uint64_t used_pages  = 0;

static inline void bm_set(uint64_t p)   { bitmap[p/8] |=  (1 << (p%8)); }
static inline void bm_clear(uint64_t p) { bitmap[p/8] &= ~(1 << (p%8)); }
static inline int  bm_test(uint64_t p)  { return bitmap[p/8] & (1 << (p%8)); }

static void reserve_range(uint64_t start, uint64_t end) {
    uint64_t p0 = (start + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t p1 = end / PAGE_SIZE;
    for (uint64_t p = p0; p < p1; p++) bm_set(p);
}

void pmm_init(void) {
    for (uint64_t i = 0; i < BITMAP_SIZE; i++) bitmap[i] = 0xFF;

    uint64_t highest = 0;
    for (uint32_t i = 0; i < E820_COUNT; i++) {
        e820_entry_t *e = &E820_ENTRIES[i];
        if (e->type == E820_TYPE_FREE) {
            uint64_t end = e->base + e->length;
            if (end > highest) highest = end;
        }
    }
    total_pages = highest / PAGE_SIZE;
    if (total_pages > BITMAP_SIZE * 8) total_pages = BITMAP_SIZE * 8;

    for (uint32_t i = 0; i < E820_COUNT; i++) {
        e820_entry_t *e = &E820_ENTRIES[i];
        if (e->type != E820_TYPE_FREE) continue;
        uint64_t p0 = (e->base + PAGE_SIZE - 1) / PAGE_SIZE;
        uint64_t p1 = (e->base + e->length) / PAGE_SIZE;
        for (uint64_t p = p0; p < p1; p++) bm_clear(p);
    }

    reserve_range(0x000000, 0x100000);
    reserve_range(0x100000, 0x103000);
    reserve_range(BITMAP_ADDR, BITMAP_ADDR + BITMAP_SIZE);
    reserve_range(HEAP_START, HEAP_START + HEAP_SIZE);

    used_pages = 0;
    for (uint64_t i = 0; i < total_pages; i++)
        if (bm_test(i)) used_pages++;
}

uint64_t pmm_alloc_page(void) {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bm_test(i)) { bm_set(i); used_pages++; return i * PAGE_SIZE; }
    }
    return 0;
}

void pmm_free_page(uint64_t addr) {
    uint64_t p = addr / PAGE_SIZE;
    if (p < total_pages && bm_test(p)) { bm_clear(p); used_pages--; }
}

uint64_t pmm_total_memory(void) { return total_pages * PAGE_SIZE; }
uint64_t pmm_free_memory(void)  { return (total_pages - used_pages) * PAGE_SIZE; }
