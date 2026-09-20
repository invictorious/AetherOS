#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELF_MAGIC       0x464C457F   /* 0x7F 'E' 'L' 'F' */
#define ELF_CLASS_64    2
#define ELF_DATA_LE     1
#define ELF_TYPE_EXEC   2
#define ELF_MACHINE_X86_64 0x3E

#define PT_LOAD         1

typedef struct __attribute__((packed)) {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_header_t;

typedef struct __attribute__((packed)) {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_phdr_t;

#define PF_X 1
#define PF_W 2
#define PF_R 4

/* Devuelve la direccion de entrada, o 0 si fallo */
uint64_t elf_load(const void *data, uint64_t size);

#endif
