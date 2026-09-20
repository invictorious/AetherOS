#include "ata.h"
#include "../kernel/console.h"

/* Puertos del canal primario (base 0x1F0) */
#define REG_DATA       0x00
#define REG_ERROR      0x01
#define REG_FEATURES   0x01
#define REG_SECCOUNT   0x02
#define REG_LBA_LOW    0x03
#define REG_LBA_MID    0x04
#define REG_LBA_HIGH   0x05
#define REG_DRIVE      0x06
#define REG_STATUS     0x07
#define REG_COMMAND    0x07

#define CMD_IDENTIFY   0xEC
#define CMD_READ_PIO   0x20
#define CMD_WRITE_PIO  0x30
#define CMD_FLUSH      0xE7

#define STATUS_BSY     0x80
#define STATUS_DRDY    0x40
#define STATUS_DRQ     0x08
#define STATUS_ERR     0x01

static uint16_t io_base = ATA_PRIMARY_IO;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t r;
    __asm__ volatile ("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t r;
    __asm__ volatile ("inw %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

/* Pequeño delay leyendo 4 veces el status */
static void io_wait(void) {
    for (int i = 0; i < 4; i++) inb(io_base + REG_STATUS);
}

/* Esperar a que el disco no esté ocupado */
static int wait_bsy(void) {
    for (int i = 0; i < 1000000; i++) {
        if (!(inb(io_base + REG_STATUS) & STATUS_BSY)) return 0;
    }
    return -1;
}

/* Esperar a que haya datos listos */
static int wait_drq(void) {
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb(io_base + REG_STATUS);
        if (s & STATUS_ERR) return -1;
        if (s & STATUS_DRQ) return 0;
    }
    return -1;
}

static uint32_t total_sectors = 0;

void ata_init(void) {
    total_sectors = 0;

    /* Desactivar IRQs del controlador ATA (nIEN=1) */
    outb(0x3F6, 0x02);

    /* Seleccionar master del primario */
    outb(io_base + REG_DRIVE, 0xA0);
    io_wait();

    /* Enviar IDENTIFY */
    outb(io_base + REG_SECCOUNT, 0);
    outb(io_base + REG_LBA_LOW,  0);
    outb(io_base + REG_LBA_MID,  0);
    outb(io_base + REG_LBA_HIGH, 0);
    outb(io_base + REG_COMMAND,  CMD_IDENTIFY);

    /* Si el status es 0, no hay disco */
    uint8_t status = inb(io_base + REG_STATUS);
    if (status == 0) return;

    if (wait_bsy() < 0) return;

    /* Si LBA_MID o LBA_HIGH != 0, no es ATA */
    if (inb(io_base + REG_LBA_MID) || inb(io_base + REG_LBA_HIGH)) return;

    if (wait_drq() < 0) return;

    /* Leer 256 words (512 bytes) del IDENTIFY */
    uint16_t identify[256];
    for (int i = 0; i < 256; i++) identify[i] = inw(io_base + REG_DATA);

    /* LBA total = words 60-61 (little endian doble word) */
    total_sectors = ((uint32_t)identify[61] << 16) | identify[60];
}

uint32_t ata_disk_size_sectors(void) { return total_sectors; }

void ata_print_info(void) {
    if (total_sectors == 0) {
        console_printf("[ATA] No se detecto disco\n");
        return;
    }
    console_printf("[ATA] Disco: %u sectores (%u MB)\n",
                   total_sectors,
                   total_sectors / 2048);
}

int ata_read_sectors(uint32_t lba, uint8_t count, void *buf) {
    if (total_sectors == 0) return -1;
    if (lba + count > total_sectors) return -1;

    if (wait_bsy() < 0) return -1;

    /* Modo LBA28: drive 0xE0 | bits 24-27 del LBA */
    outb(io_base + REG_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));
    outb(io_base + REG_SECCOUNT, count);
    outb(io_base + REG_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(io_base + REG_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(io_base + REG_COMMAND,  CMD_READ_PIO);

    uint16_t *ptr = (uint16_t*)buf;
    for (int s = 0; s < count; s++) {
        if (wait_bsy() < 0) return -1;
        if (wait_drq() < 0) return -1;
        for (int i = 0; i < 256; i++) {
            ptr[i] = inw(io_base + REG_DATA);
        }
        ptr += 256;
    }
    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t count, const void *buf) {
    if (total_sectors == 0) return -1;
    if (lba + count > total_sectors) return -1;

    if (wait_bsy() < 0) return -1;

    outb(io_base + REG_DRIVE,    0xE0 | ((lba >> 24) & 0x0F));
    outb(io_base + REG_SECCOUNT, count);
    outb(io_base + REG_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(io_base + REG_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(io_base + REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(io_base + REG_COMMAND,  CMD_WRITE_PIO);

    const uint16_t *ptr = (const uint16_t*)buf;
    for (int s = 0; s < count; s++) {
        if (wait_bsy() < 0) return -1;
        if (wait_drq() < 0) return -1;
        for (int i = 0; i < 256; i++) {
            outw(io_base + REG_DATA, ptr[i]);
        }
        ptr += 256;
    }

    outb(io_base + REG_COMMAND, CMD_FLUSH);
    if (wait_bsy() < 0) return -1;
    return 0;
}
