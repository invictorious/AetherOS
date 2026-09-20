#ifndef EXEC_H
#define EXEC_H

#include <stdint.h>

void     exec_run(const char *path) __attribute__((noreturn));
uint64_t exec_load_elf(const char *path);   /* devuelve entry point o 0 */

#endif
