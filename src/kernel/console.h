#ifndef CONSOLE_H
#define CONSOLE_H

#include <stddef.h>
#include <stdint.h>

int  console_init(void);
void console_clear(void);
void console_putchar(char c);
void console_set_silent(int s);
void console_write(const char *s);
void console_printf(const char *fmt, ...);
void console_put_at(int x, int y, char c, uint8_t color);

#endif
