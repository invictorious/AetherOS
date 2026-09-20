#include "gdt.h"
#include <stddef.h>

/* ------------------------------------------------------------
 * Descriptor de segmento (8 bytes)
 * ------------------------------------------------------------ */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

/* ------------------------------------------------------------
 * TSS en modo largo (104 bytes)
 * ------------------------------------------------------------ */
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

/* 7 entradas: null, KCS, KDS, UCS, UDS, TSS(2 slots) */
#define GDT_ENTRIES 7

static struct gdt_entry gdt[GDT_ENTRIES] __attribute__((aligned(16)));
static struct gdt_ptr   gdtp;
static struct tss_entry tss __attribute__((aligned(16)));

extern void gdt_flush(uint64_t gdtp_addr);
extern void tss_flush(uint16_t tss_selector);

/* Descriptor clasico de 8 bytes */
static void set_gate(int i, uint32_t base, uint32_t limit,
                     uint8_t access, uint8_t gran) {
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].base_high   = (base >> 24) & 0xFF;
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[i].access      = access;
}

/* TSS: descriptor de 16 bytes = 2 slots GDT */
static void write_tss(int i) {
    uint64_t base  = (uint64_t)&tss;
    uint32_t limit = sizeof(tss) - 1;

    uint8_t *p = (uint8_t*)&gdt[i];

    /* Bytes 0-1: limit[15:0] */
    p[0] = limit & 0xFF;
    p[1] = (limit >> 8) & 0xFF;
    /* Bytes 2-3: base[15:0] */
    p[2] = base & 0xFF;
    p[3] = (base >> 8) & 0xFF;
    /* Byte 4: base[23:16] */
    p[4] = (base >> 16) & 0xFF;
    /* Byte 5: access (0x89 = present, DPL=0, type=64-bit TSS available) */
    p[5] = 0x89;
    /* Byte 6: flags (G=0, D/B=0, L=0, AVL=0) + limit[19:16] */
    p[6] = (limit >> 16) & 0x0F;
    /* Byte 7: base[31:24] */
    p[7] = (base >> 24) & 0xFF;
    /* Bytes 8-11: base[63:32] */
    uint32_t base_hi = (uint32_t)(base >> 32);
    p[8]  = base_hi & 0xFF;
    p[9]  = (base_hi >> 8) & 0xFF;
    p[10] = (base_hi >> 16) & 0xFF;
    p[11] = (base_hi >> 24) & 0xFF;
    /* Bytes 12-15: reserved */
    p[12] = 0; p[13] = 0; p[14] = 0; p[15] = 0;

    /* Limpiar la estructura TSS */
    uint8_t *t = (uint8_t*)&tss;
    for (size_t k = 0; k < sizeof(tss); k++) t[k] = 0;
    tss.iomap_base = sizeof(tss);
}

void gdt_init(void) {
    gdtp.limit = sizeof(gdt) - 1;
    gdtp.base  = (uint64_t)&gdt;

    set_gate(0, 0, 0,       0x00, 0x00);   /* null          */
    set_gate(1, 0, 0xFFFFF, 0x9A, 0xA0);   /* kernel CS     */
    set_gate(2, 0, 0xFFFFF, 0x92, 0xA0);   /* kernel DS     */
    set_gate(3, 0, 0xFFFFF, 0xFA, 0xA0);   /* user CS DPL=3 */
    set_gate(4, 0, 0xFFFFF, 0xF2, 0xA0);   /* user DS DPL=3 */
    write_tss(5);

    gdt_flush((uint64_t)&gdtp);
    tss_flush(5 * 8);
}

void tss_set_kernel_stack(uint64_t rsp0) {
    tss.rsp0 = rsp0;
}

uint16_t gdt_user_code_selector(void) { return (3 * 8) | 3; }
uint16_t gdt_user_data_selector(void) { return (4 * 8) | 3; }
uint16_t gdt_tss_selector(void)       { return 5 * 8; }
