#include "pe.h"
#include "../kernel/console.h"

extern void *win_lookup_export(const char *dll, const char *func);

/* ------------------------------------------------------------
 * Traduce un RVA a puntero dentro del archivo
 * ------------------------------------------------------------ */
static const uint8_t *rva_to_file(const uint8_t *base, uint64_t size,
                                   const pe_section_t *sec, uint16_t num_sec,
                                   uint32_t rva) {
    for (uint16_t i = 0; i < num_sec; i++) {
        uint32_t va = sec[i].vaddr;
        uint32_t vs = sec[i].virtual_size ? sec[i].virtual_size : sec[i].size_raw;
        if (rva >= va && rva < va + vs) {
            uint64_t off = (uint64_t)(rva - va) + sec[i].ptr_raw;
            if (off >= size) return 0;
            return base + off;
        }
    }
    return 0;
}

/* ------------------------------------------------------------
 * Aplica las relocations de la seccion .reloc
 * ------------------------------------------------------------ */
static int apply_relocations(const uint8_t *base, uint64_t size,
                              const pe_section_t *sec, uint16_t num_sec,
                              uint64_t load_addr, uint64_t orig_base) {
    /* 1. Localizar la seccion .reloc */
    const pe_section_t *reloc_sec = 0;
    for (uint16_t i = 0; i < num_sec; i++) {
        const char *n = (const char*)sec[i].name;
        if (n[0] == '.' && n[1] == 'r' && n[2] == 'e' && n[3] == 'l' &&
            n[4] == 'o' && n[5] == 'c') {
            reloc_sec = &sec[i];
            break;
        }
    }

    if (!reloc_sec) {
        console_write("[PE] Sin seccion .reloc (no hace falta)\n");
        return 0;
    }

    if (reloc_sec->ptr_raw + reloc_sec->size_raw > size) {
        console_write("[PE] .reloc fuera de archivo\n");
        return -1;
    }

    int64_t delta = (int64_t)load_addr - (int64_t)orig_base;
    if (delta == 0) {
        console_write("[PE] delta = 0, nada que parchear\n");
        return 0;
    }

    console_printf("[PE] Relocations: load=0x%lx orig=0x%lx delta=%ld\n",
                   (unsigned long)load_addr,
                   (unsigned long)orig_base,
                   (long)delta);

    /* 2. Iterar bloques */
    const uint8_t *reloc_start = base + reloc_sec->ptr_raw;
    const uint8_t *reloc_end   = reloc_start + reloc_sec->size_raw;
    const uint8_t *p = reloc_start;
    uint32_t total_fixed = 0;

    while (p + sizeof(pe_reloc_block_t) <= reloc_end) {
        const pe_reloc_block_t *blk = (const pe_reloc_block_t*)p;
        if (blk->block_size < sizeof(pe_reloc_block_t)) break;

        uint32_t num_entries = (blk->block_size - sizeof(pe_reloc_block_t)) / 2;
        const uint16_t *entries = (const uint16_t*)(p + sizeof(pe_reloc_block_t));

        for (uint32_t k = 0; k < num_entries; k++) {
            uint16_t e = entries[k];
            uint32_t type   = (e >> 12) & 0xF;
            uint32_t offset = e & 0xFFF;

            uint32_t rva = blk->page_rva + offset;

            if (type == IMAGE_REL_BASED_DIR64) {
                uint64_t *target = (uint64_t*)(load_addr + rva);
                *target += delta;
                total_fixed++;
            } else if (type == IMAGE_REL_BASED_HIGHLOW) {
                uint32_t *target = (uint32_t*)(load_addr + rva);
                *target += (uint32_t)delta;
                total_fixed++;
            }
            /* type 0 = ABSOLUTE, se ignora */
        }

        p += blk->block_size;
    }

    console_printf("[PE] %u relocations aplicadas\n", total_fixed);
    return 0;
}

/* ------------------------------------------------------------
 * Cargador PE principal
 * ------------------------------------------------------------ */
uint64_t pe_load(const void *data, uint64_t size) {
    const uint8_t *base = (const uint8_t*)data;

    if (size < 0x40) { console_write("[PE] Archivo muy pequeno\n"); return 0; }
    if (*(const uint16_t*)base != PE_DOS_MAGIC) {
        console_write("[PE] No es MZ\n"); return 0;
    }
    uint32_t e_lfanew = *(const uint32_t*)(base + 0x3C);
    if (e_lfanew + 24 > size) { console_write("[PE] e_lfanew invalido\n"); return 0; }
    if (*(const uint32_t*)(base + e_lfanew) != PE_SIG) {
        console_write("[PE] Sin firma PE\n"); return 0;
    }

    const pe_coff_t *coff = (const pe_coff_t*)(base + e_lfanew + 4);
    if (coff->machine != PE_MACHINE_X64) { console_write("[PE] No x86_64\n"); return 0; }

    const pe_opt64_t *opt = (const pe_opt64_t*)((const uint8_t*)coff + sizeof(pe_coff_t));
    if (opt->magic != PE_MAGIC_32PLUS) { console_write("[PE] No PE32+\n"); return 0; }

    uint64_t image_base = opt->image_base;
    uint32_t entry_rva  = opt->entry_point;
    uint32_t size_image = opt->size_image;

    console_printf("[PE] ImageBase 0x%lx Entry 0x%x Size 0x%x\n",
                   (unsigned long)image_base, entry_rva, size_image);

    if (image_base + size_image > 0x2000000) {
        console_write("[PE] Fuera de rango\n"); return 0;
    }

    const pe_section_t *sec = (const pe_section_t*)
        ((const uint8_t*)opt + coff->size_opt_header);

    /* Copiar secciones */
    for (uint16_t i = 0; i < coff->num_sections; i++) {
        const pe_section_t *s = &sec[i];
        if (s->size_raw == 0) continue;
        if (s->ptr_raw + s->size_raw > size) continue;
        uint64_t vaddr = image_base + s->vaddr;
        if (vaddr + s->size_raw > 0x2000000) continue;
        for (uint32_t k = 0; k < s->size_raw; k++) {
            ((uint8_t*)vaddr)[k] = base[s->ptr_raw + k];
        }
    }
    console_write("[PE] Secciones copiadas\n");

    /* Relocations (delta = 0 normalmente, pero prueba) */
    if (apply_relocations(base, size, sec, coff->num_sections,
                          image_base, image_base) != 0) {
        return 0;
    }

    /* Resolver imports */
    if (opt->num_rva < 2 || opt->data_dir[1].rva == 0) {
        console_write("[PE] Sin imports\n");
        return image_base + entry_rva;
    }

    uint32_t imp_rva = opt->data_dir[1].rva;
    const uint8_t *imp_ptr = rva_to_file(base, size, sec, coff->num_sections, imp_rva);
    if (!imp_ptr) { console_write("[PE] Import dir no traducible\n"); return image_base + entry_rva; }

    const pe_import_t *imp = (const pe_import_t*)imp_ptr;
    int count = 0;
    while (imp->name != 0 && count < 32) {
        count++;
        const char *dll = (const char*)rva_to_file(base, size, sec, coff->num_sections, imp->name);
        if (!dll) break;

        console_printf("[PE] Import: %s\n", dll);

        uint32_t ilt_rva = imp->orig_first_thunk ? imp->orig_first_thunk : imp->first_thunk;
        uint32_t iat_rva = imp->first_thunk;

        const uint64_t *ilt = (const uint64_t*)rva_to_file(base, size, sec, coff->num_sections, ilt_rva);
        uint64_t *iat = (uint64_t*)(image_base + iat_rva);

        if (!ilt) break;

        while (*ilt != 0) {
            uint64_t val = *ilt;
            if (val & 0x8000000000000000ULL) {
                *iat = 0;
            } else {
                const uint8_t *hint = rva_to_file(base, size, sec, coff->num_sections, (uint32_t)val);
                if (!hint) break;
                const char *fname = (const char*)(hint + 2);
                void *fn = win_lookup_export(dll, fname);
                *iat = (uint64_t)fn;
            }
            ilt++;
            iat++;
        }
        imp++;
    }

    console_write("[PE] Imports resueltos\n");
    return image_base + entry_rva;
}

/* Version futura: cargar en direccion arbitraria */
uint64_t pe_load_at(const void *data, uint64_t size, uint64_t load_addr) {
    /* Por ahora delega al load normal (que asume ImageBase) */
    (void)load_addr;
    return pe_load(data, size);
}
