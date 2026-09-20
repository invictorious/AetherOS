#include "pe.h"
#include "../kernel/console.h"

extern void *win_lookup_export(const char *dll, const char *func);

/* ------------------------------------------------------------
 * Traduce un RVA (Relative Virtual Address) a un puntero dentro
 * del archivo usando la tabla de secciones.
 * Devuelve NULL si no encuentra.
 * ------------------------------------------------------------ */
static const uint8_t *rva_to_file(const uint8_t *base, uint64_t size,
                                   const pe_section_t *sec, uint16_t num_sec,
                                   uint32_t rva) {
    for (uint16_t i = 0; i < num_sec; i++) {
        uint32_t va = sec[i].vaddr;
        uint32_t vs = sec[i].virtual_size;
        if (rva >= va && rva < va + vs) {
            uint64_t off = (uint64_t)(rva - va) + sec[i].ptr_raw;
            if (off >= size) return 0;
            return base + off;
        }
    }
    return 0;
}

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
        if (s->ptr_raw + s->size_raw > size) {
            console_printf("[PE] sec[%u] fuera de archivo\n", i);
            continue;
        }
        uint64_t vaddr = image_base + s->vaddr;
        if (vaddr + s->size_raw > 0x2000000) {
            console_printf("[PE] sec[%u] fuera de rango\n", i);
            continue;
        }
        /* Copiar byte a byte (mas seguro que por 8) */
        for (uint32_t k = 0; k < s->size_raw; k++) {
            ((uint8_t*)vaddr)[k] = base[s->ptr_raw + k];
        }
    }
    console_write("[PE] Secciones copiadas\n");

    /* Resolver imports: TODO son RVAs, traducir a offset de archivo */
    if (opt->num_rva < 2 || opt->data_dir[1].rva == 0) {
        console_write("[PE] Sin imports\n");
        return image_base + entry_rva;
    }

    uint32_t imp_rva = opt->data_dir[1].rva;
    const uint8_t *imp_ptr = rva_to_file(base, size, sec, coff->num_sections, imp_rva);
    if (!imp_ptr) {
        console_printf("[PE] Import dir RVA 0x%x no traducible\n", imp_rva);
        return image_base + entry_rva;
    }

    console_printf("[PE] Import dir @ RVA 0x%x\n", imp_rva);

    const pe_import_t *imp = (const pe_import_t*)imp_ptr;
    int count = 0;
    while (imp->name != 0 && count < 32) {
        count++;
        const char *dll = (const char*)rva_to_file(base, size, sec,
                                                   coff->num_sections, imp->name);
        if (!dll) break;

        console_printf("[PE] Import: %s\n", dll);

        uint32_t ilt_rva = imp->orig_first_thunk ? imp->orig_first_thunk : imp->first_thunk;
        uint32_t iat_rva = imp->first_thunk;

        const uint64_t *ilt = (const uint64_t*)rva_to_file(base, size, sec,
                                                            coff->num_sections, ilt_rva);
        uint64_t *iat = (uint64_t*)(image_base + iat_rva);

        if (!ilt) break;

        while (*ilt != 0) {
            uint64_t val = *ilt;
            if (val & 0x8000000000000000ULL) {
                console_write("[PE]   ordinal (skip)\n");
                *iat = 0;
            } else {
                uint32_t name_rva = (uint32_t)val;
                /* El IMAGE_IMPORT_BY_NAME empieza con un WORD hint */
                const uint8_t *hint_ptr = rva_to_file(base, size, sec,
                                                      coff->num_sections, name_rva);
                if (!hint_ptr) break;
                const char *fname = (const char*)(hint_ptr + 2);   /* skip hint */
                void *fn = win_lookup_export(dll, fname);
                console_printf("[PE]   %s -> 0x%lx\n", fname, (unsigned long)fn);
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
