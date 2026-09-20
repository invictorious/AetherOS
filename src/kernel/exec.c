#include "exec.h"
#include "console.h"
#include "../fs/fat32.h"

#define USER_LOAD_ADDR  0x500000
#define USER_STACK_TOP  0x700000
#define USER_STACK_SIZE 0x10000   /* 64 KB */

/* Pila del kernel para cuando el binario haga syscall */
extern void enter_usermode(uint64_t entry, uint64_t user_stack) __attribute__((noreturn));
extern void tss_set_kernel_stack(uint64_t rsp0);

static uint8_t kstack_for_user[16384] __attribute__((aligned(16)));

void exec_run(const char *path) {
    /* 1. Buscar el archivo */
    fat32_dir_entry_t entry;
    if (fat32_open(path, &entry) != 0) {
        console_write("[EXEC] No existe: ");
        console_write(path);
        console_write("\n");
        /* Volvemos al shell, no al kernel */
        for (;;) __asm__ volatile ("hlt");
    }

    if (entry.size == 0 || entry.size > 0x100000) {
        console_write("[EXEC] Tamano invalido\n");
        for (;;) __asm__ volatile ("hlt");
    }

    /* 2. Cargar el binario a USER_LOAD_ADDR */
    uint8_t *dst = (uint8_t*)USER_LOAD_ADDR;
    uint32_t n = fat32_read_file(&entry, dst, entry.size);

    console_printf("[EXEC] %s cargado (%u bytes) en 0x%x\n",
                   path, n, (unsigned int)USER_LOAD_ADDR);

    /* 3. Configurar la pila del kernel para syscalls desde el nuevo binario */
    tss_set_kernel_stack((uint64_t)(kstack_for_user + sizeof(kstack_for_user)));

    /* 4. Saltar al binario en ring 3 */
    uint64_t user_stack_top = USER_STACK_TOP + USER_STACK_SIZE;
    enter_usermode(USER_LOAD_ADDR, user_stack_top);

    for (;;) __asm__ volatile ("hlt");
}
