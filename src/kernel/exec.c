#include "exec.h"
#include "console.h"
#include "elf.h"
#include "../fs/fat32.h"

#define USER_LOAD_ADDR  0x400000
#define USER_STACK_TOP  0x800000
#define USER_STACK_SIZE 0x80000
#define MAX_BIN_SIZE    0x100000

extern void enter_usermode(uint64_t entry, uint64_t user_stack) __attribute__((noreturn));
extern void tss_set_kernel_stack(uint64_t rsp0);

static uint8_t kstack_for_user[16384] __attribute__((aligned(16)));
static uint8_t file_buffer[MAX_BIN_SIZE] __attribute__((aligned(4096)));

/* Solo carga el ELF, no ejecuta. Devuelve entry point o 0. */
uint64_t exec_load_elf(const char *path) {
    /* Deshabilitar interrupciones durante la carga:
     * el scheduler no debe cambiar de proceso mientras
     * estamos a mitad de una syscall critica */
    extern void scheduler_enter_critical(void);
    extern void scheduler_exit_critical(void);
    scheduler_enter_critical();

    fat32_dir_entry_t entry;
    if (fat32_open(path, &entry) != 0) {
        console_write("[EXEC] No existe: ");
        console_write(path);
        console_write("\n");
        scheduler_exit_critical();
        return 0;
    }
    if (entry.size == 0 || entry.size > MAX_BIN_SIZE) {
        scheduler_exit_critical();
        return 0;
    }

    uint32_t n = fat32_read_file(&entry, file_buffer, MAX_BIN_SIZE);
    console_printf("[EXEC] %s: %u bytes\n", path, n);

    uint32_t magic = *(uint32_t*)file_buffer;

    uint64_t result = 0;
    if (magic == ELF_MAGIC) {
        console_printf("[EXEC] Formato ELF64\n");
        result = elf_load(file_buffer, n);
    } else {
        console_printf("[EXEC] Formato binario plano\n");
        uint8_t *dst = (uint8_t*)USER_LOAD_ADDR;
        for (uint32_t i = 0; i < n; i++) dst[i] = file_buffer[i];
        result = USER_LOAD_ADDR;
    }

    scheduler_exit_critical();
    return result;
}

/* Ejecuta inmediatamente (reemplaza al proceso actual) */
void exec_run(const char *path) {
    uint64_t entry_point = exec_load_elf(path);
    if (entry_point == 0) {
        console_write("[EXEC] Fallo al cargar\n");
        for (;;) __asm__ volatile ("hlt");
    }

    console_printf("[EXEC] Saltando a 0x%lx en ring 3\n",
                   (unsigned long)entry_point);

    tss_set_kernel_stack((uint64_t)(kstack_for_user + sizeof(kstack_for_user)));
    uint64_t user_stack_top = USER_STACK_TOP + USER_STACK_SIZE;
    enter_usermode(entry_point, user_stack_top);

    for (;;) __asm__ volatile ("hlt");
}
