#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include <stddef.h>

#define ATA_SECTOR_SIZE 512

/* Registros del canal primario */
#define ATA_PRIMARY_IO   0x1F0
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_IO 0x170
#define ATA_SECONDARY_CTRL 0x376

void     ata_init(void);
int      ata_read_sectors(uint32_t lba, uint8_t count, void *buf);
int      ata_write_sectors(uint32_t lba, uint8_t count, const void *buf);
uint32_t ata_disk_size_sectors(void);
void     ata_print_info(void);

#endif
