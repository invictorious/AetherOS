#include "fat32.h"
#include "../drivers/ata.h"
#include "../kernel/console.h"

/* Estado interno */
static fat32_bpb_t bpb;
static uint32_t    fat_start_lba  = 0;
static uint32_t    data_start_lba = 0;
static int         mounted        = 0;

/* Buffer de un sector */
static uint8_t sector_buf[512] __attribute__((aligned(16)));

/* ------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------ */
uint32_t fat32_root_cluster(void) { return bpb.root_cluster; }

uint32_t fat32_bytes_per_cluster(void) {
    return (uint32_t)bpb.bytes_per_sector * bpb.sectors_per_cluster;
}

/* Leer la entrada N de la FAT (indice = numero de cluster) */
static uint32_t fat_next_cluster(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_start_lba + (fat_offset / bpb.bytes_per_sector);
    uint32_t entry_off  = fat_offset % bpb.bytes_per_sector;

    if (ata_read_sectors(fat_sector, 1, sector_buf) != 0) return 0x0FFFFFFF;
    uint32_t val = *(uint32_t*)(sector_buf + entry_off);
    return val & 0x0FFFFFFF;
}

/* LBA del primer sector de un cluster */
static uint32_t cluster_to_lba(uint32_t cluster) {
    return data_start_lba + (cluster - 2) * bpb.sectors_per_cluster;
}

/* Leer un cluster completo en un buffer */
static int read_cluster(uint32_t cluster, void *buf) {
    return ata_read_sectors(cluster_to_lba(cluster),
                            bpb.sectors_per_cluster, buf);
}

/* Comparar nombre "8.3" con entrada (nombre ya en mayusculas) */
static int name_matches(const fat32_dir_entry_t *e, const char *name) {
    /* Formato "HOLA.TXT" -> name="HOLA    ", ext="TXT" */
    char base[9] = {0};
    char ext[4]  = {0};
    int bi = 0, ei = 0;

    for (int i = 0; name[i] && name[i] != '.'; i++) {
        if (bi < 8) base[bi++] = name[i];
    }
    const char *dot = name;
    while (*dot && *dot != '.') dot++;
    if (*dot == '.') dot++;
    for (int i = 0; dot[i]; i++) {
        if (ei < 3) ext[ei++] = dot[i];
    }

    for (int i = 0; i < 8; i++) {
        char c = (i < bi) ? base[i] : ' ';
        if (c >= 'a' && c <= 'z') c -= 32;
        if ((char)e->name[i] != c) return 0;
    }
    for (int i = 0; i < 3; i++) {
        char c = (i < ei) ? ext[i] : ' ';
        if (c >= 'a' && c <= 'z') c -= 32;
        if ((char)e->ext[i] != c) return 0;
    }
    return 1;
}

/* Imprimir nombre "8.3" de una entrada */
static void print_entry_name(const fat32_dir_entry_t *e) {
    char buf[13];
    int k = 0;
    for (int i = 0; i < 8; i++) {
        if (e->name[i] == ' ') break;
        buf[k++] = e->name[i];
    }
    buf[k++] = '.';
    for (int i = 0; i < 3; i++) {
        if (e->ext[i] == ' ') break;
        buf[k++] = e->ext[i];
    }
    buf[k] = 0;
    console_write(buf);
}

/* ------------------------------------------------------------
 * Montar
 * ------------------------------------------------------------ */
int fat32_init(void) {
    mounted = 0;

    /* Leer boot sector */
    if (ata_read_sectors(0, 1, &bpb) != 0) return -1;

    /* Validar firma 0xAA55 */
    uint8_t *raw = (uint8_t*)&bpb;
    if (raw[510] != 0x55 || raw[511] != 0xAA) return -1;

    /* Validar tipo FAT32 */
    if (bpb.bytes_per_sector != 512) return -1;
    if (bpb.sectors_per_cluster == 0) return -1;
    if (bpb.fat_size_32 == 0) return -1;

    fat_start_lba  = bpb.reserved_sectors;
    data_start_lba = bpb.reserved_sectors + (bpb.num_fats * bpb.fat_size_32);

    mounted = 1;
    return 0;
}

/* ------------------------------------------------------------
 * Listar raiz
 * ------------------------------------------------------------ */
void fat32_list_root(void) {
    if (!mounted) { console_write("[FAT32] No montado\n"); return; }

    uint32_t cluster = bpb.root_cluster;
    uint32_t bpc     = fat32_bytes_per_cluster();

    console_printf("[FAT32] Directorio raiz (cluster %u):\n", cluster);

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint8_t *cluster_buf = (uint8_t*)0x300000;   /* 3 MB temporal */
        if (read_cluster(cluster, cluster_buf) != 0) return;

        for (uint32_t off = 0; off < bpc; off += 32) {
            fat32_dir_entry_t *e = (fat32_dir_entry_t*)(cluster_buf + off);
            if (e->name[0] == 0) return;                 /* fin */
            if (e->name[0] == 0xE5) continue;            /* borrado */
            if (e->attr == FAT32_ATTR_LFN) continue;     /* long name */

            console_write("  ");
            print_entry_name(e);
            if (e->attr & FAT32_ATTR_DIRECTORY) {
                console_write("  <DIR>\n");
            } else {
                console_printf("  %u bytes\n", e->size);
            }
        }
        cluster = fat_next_cluster(cluster);
    }
}

/* ------------------------------------------------------------
 * Buscar archivo
 * ------------------------------------------------------------ */
int fat32_open(const char *name, fat32_dir_entry_t *out) {
    if (!mounted) return -1;

    uint32_t cluster = bpb.root_cluster;
    uint32_t bpc     = fat32_bytes_per_cluster();

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint8_t *cluster_buf = (uint8_t*)0x300000;
        if (read_cluster(cluster, cluster_buf) != 0) return -1;

        for (uint32_t off = 0; off < bpc; off += 32) {
            fat32_dir_entry_t *e = (fat32_dir_entry_t*)(cluster_buf + off);
            if (e->name[0] == 0) return -1;
            if (e->name[0] == 0xE5) continue;
            if (e->attr == FAT32_ATTR_LFN) continue;
            if (e->attr & FAT32_ATTR_DIRECTORY) continue;

            if (name_matches(e, name)) {
                *out = *e;
                return 0;
            }
            /* DEBUG: mostrar entradas vistas */
            console_printf("  [dbg] entrada: '");
            for (int i = 0; i < 8; i++) console_putchar(e->name[i]);
            console_putchar('.');
            for (int i = 0; i < 3; i++) console_putchar(e->ext[i]);
            console_printf("' attr=0x%x\n", e->attr);
        }
        cluster = fat_next_cluster(cluster);
    }
    return -1;
}

/* ------------------------------------------------------------
 * Leer archivo completo
 * ------------------------------------------------------------ */
uint32_t fat32_read_file(const fat32_dir_entry_t *entry, void *buf, uint32_t max_size) {
    if (!mounted) return 0;

    uint32_t cluster = ((uint32_t)entry->first_cluster_hi << 16) | entry->first_cluster_lo;
    uint32_t bpc     = fat32_bytes_per_cluster();
    uint32_t total   = 0;
    uint8_t *dst     = (uint8_t*)buf;
    uint8_t *cluster_buf = (uint8_t*)0x300000;

    while (cluster >= 2 && cluster < 0x0FFFFFF8 && total < max_size) {
        if (read_cluster(cluster, cluster_buf) != 0) return total;

        uint32_t to_copy = bpc;
        if (to_copy > max_size - total) to_copy = max_size - total;

        for (uint32_t i = 0; i < to_copy; i++) dst[total + i] = cluster_buf[i];
        total += to_copy;

        if (total >= entry->size) break;
        cluster = fat_next_cluster(cluster);
    }
    return total;
}
