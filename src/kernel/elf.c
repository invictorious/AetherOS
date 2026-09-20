#include "elf.h"
#include "console.h"

static void mem_copy(void *dst, const void *src, uint64_t n) {
    uint8_t *d = (uint8_t*)dst;
    const uint8_t *s = (const uint8_t*)src;
    for (uint64_t i = 0; i < n; i++) d[i] = s[i];
}

static void mem_set(void *dst, uint8_t v, uint64_t n) {
    uint8_t *d = (uint8_t*)dst;
    for (uint64_t i = 0; i < n; i++) d[i] = v;
}

uint64_t elf_load(const void *data, uint64_t size) {
    const uint8_t *base = (const uint8_t*)data;

    if (size < sizeof(elf64_header_t)) {
        console_write("[ELF] Archivo demasiado pequeno\n");
        return 0;
    }

    const elf64_header_t *hdr = (const elf64_header_t*)base;

    /* 1. Validar magic: 0x7F 'E' 'L' 'F' */
    if (*(const uint32_t*)hdr->e_ident != ELF_MAGIC) {
        console_write("[ELF] Magic incorrecto\n");
        return 0;
    }

    /* 2. Validar clase, endianness, tipo, maquina */
    if (hdr->e_ident[4] != ELF_CLASS_64) {
        console_write("[ELF] No es ELF64\n");
        return 0;
    }
    if (hdr->e_ident[5] != ELF_DATA_LE) {
        console_write("[ELF] No es little-endian\n");
        return 0;
    }
    if (hdr->e_type != ELF_TYPE_EXEC) {
        console_write("[ELF] No es ejecutable (ET_EXEC)\n");
        return 0;
    }
    if (hdr->e_machine != ELF_MACHINE_X86_64) {
        console_write("[ELF] No es x86_64\n");
        return 0;
    }

    console_printf("[ELF] Entry point: 0x%lx\n", (unsigned long)hdr->e_entry);
    console_printf("[ELF] Program headers: %u x %u bytes\n",
                   hdr->e_phnum, hdr->e_phentsize);

    /* 3. Iterar sobre los program headers */
    uint32_t loaded = 0;
    for (uint16_t i = 0; i < hdr->e_phnum; i++) {
        const elf64_phdr_t *ph = (const elf64_phdr_t*)
            (base + hdr->e_phoff + i * hdr->e_phentsize);

        if (ph->p_type != PT_LOAD) continue;

        /* Comprobar que el segmento cabe en el archivo */
        if (ph->p_offset + ph->p_filesz > size) {
            console_write("[ELF] Segmento fuera del archivo\n");
            return 0;
        }

        /* Comprobar que cabe en los primeros 32 MB mapeados */
        if (ph->p_vaddr + ph->p_memsz >= 0x2000000) {
            console_write("[ELF] Segmento fuera de rango mapeado\n");
            return 0;
        }

        console_printf("[ELF] PT_LOAD vaddr=0x%lx filesz=%lu memsz=%lu flags=%u\n",
                       (unsigned long)ph->p_vaddr,
                       (unsigned long)ph->p_filesz,
                       (unsigned long)ph->p_memsz,
                       ph->p_flags);

        /* 4. Copiar datos */
        if (ph->p_filesz > 0) {
            mem_copy((void*)ph->p_vaddr, base + ph->p_offset, ph->p_filesz);
        }

        /* 5. Rellenar .bss (la parte que ocupa memoria pero no archivo) */
        if (ph->p_memsz > ph->p_filesz) {
            mem_set((void*)(ph->p_vaddr + ph->p_filesz), 0,
                    ph->p_memsz - ph->p_filesz);
        }

        loaded++;
    }

    if (loaded == 0) {
        console_write("[ELF] No se cargo ningun segmento\n");
        return 0;
    }

    console_printf("[ELF] %u segmentos cargados\n", loaded);
    return hdr->e_entry;
}
