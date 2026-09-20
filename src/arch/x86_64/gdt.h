#ifndef GDT_H
#define GDT_H

#include <stdint.h>

void gdt_init(void);
void tss_set_kernel_stack(uint64_t rsp0);
uint16_t gdt_user_code_selector(void);
uint16_t gdt_user_data_selector(void);
uint16_t gdt_tss_selector(void);

#endif
