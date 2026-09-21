#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void     mouse_init(void);
void     mouse_handler(void);      /* llamado desde IRQ12 */
int      mouse_x(void);
int      mouse_y(void);
uint8_t  mouse_buttons(void);
int      mouse_changed(void);
uint32_t mouse_get_irq_count(void);
uint32_t mouse_get_byte_count(void);
uint32_t mouse_get_packet_count(void);
int      mouse_get_mode(void);
int      mouse_raw_x(void);
int      mouse_raw_y(void);      /* 1 si hubo movimiento o clic desde el ultimo check */

#endif
