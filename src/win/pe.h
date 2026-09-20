#ifndef PE_H
#define PE_H

#include <stdint.h>

#define PE_DOS_MAGIC   0x5A4D
#define PE_SIG         0x00004550
#define PE_MACHINE_X64 0x8664
#define PE_MAGIC_32PLUS 0x20B

/* Tipos de relocation */
#define IMAGE_REL_BASED_ABSOLUTE  0
#define IMAGE_REL_BASED_HIGHLOW   3
#define IMAGE_REL_BASED_DIR64     10

typedef struct __attribute__((packed)) {
    uint16_t machine;
    uint16_t num_sections;
    uint32_t timestamp;
    uint32_t sym_ptr;
    uint32_t num_symbols;
    uint16_t size_opt_header;
    uint16_t characteristics;
} pe_coff_t;

typedef struct __attribute__((packed)) {
    uint32_t rva;
    uint32_t size;
} pe_data_dir_t;

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  linker_major;
    uint8_t  linker_minor;
    uint32_t size_code;
    uint32_t size_init;
    uint32_t size_uninit;
    uint32_t entry_point;
    uint32_t base_code;
    uint64_t image_base;
    uint32_t section_align;
    uint32_t file_align;
    uint16_t os_major, os_minor;
    uint16_t img_major, img_minor;
    uint16_t subsys_major, subsys_minor;
    uint32_t win32_version;
    uint32_t size_image;
    uint32_t size_headers;
    uint32_t checksum;
    uint16_t subsystem;
    uint16_t dll_characteristics;
    uint64_t stack_reserve;
    uint64_t stack_commit;
    uint64_t heap_reserve;
    uint64_t heap_commit;
    uint32_t loader_flags;
    uint32_t num_rva;
    pe_data_dir_t data_dir[16];
} pe_opt64_t;

typedef struct __attribute__((packed)) {
    uint8_t  name[8];
    uint32_t virtual_size;
    uint32_t vaddr;
    uint32_t size_raw;
    uint32_t ptr_raw;
    uint32_t ptr_reloc;
    uint32_t ptr_lineno;
    uint16_t num_reloc;
    uint16_t num_lineno;
    uint32_t characteristics;
} pe_section_t;

typedef struct __attribute__((packed)) {
    uint32_t orig_first_thunk;
    uint32_t timestamp;
    uint32_t forwarder;
    uint32_t name;
    uint32_t first_thunk;
} pe_import_t;

typedef struct __attribute__((packed)) {
    uint16_t hint;
    char     name[];
} pe_hint_t;

typedef struct __attribute__((packed)) {
    uint32_t page_rva;
    uint32_t block_size;
} pe_reloc_block_t;

uint64_t pe_load(const void *data, uint64_t size);
uint64_t pe_load_at(const void *data, uint64_t size, uint64_t load_addr);

#endif
