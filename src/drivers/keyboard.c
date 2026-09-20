#include "keyboard.h"
#define KBD_DATA   0x60
#define BUF_SIZE   256

static const char kbd_map[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,  '*', 0,  ' ',
};

static volatile char     buffer[BUF_SIZE];
static volatile uint32_t buf_head = 0;
static volatile uint32_t buf_tail = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void keyboard_init(void) { buf_head = 0; buf_tail = 0; }

void keyboard_handler(void) {
    uint8_t sc = inb(KBD_DATA);
    if (sc & 0x80) return;
    if (sc < 128) {
        char c = kbd_map[sc];
        if (c) {
            uint32_t next = (buf_head + 1) % BUF_SIZE;
            if (next != buf_tail) {
                buffer[buf_head] = c;
                buf_head = next;
            }
        }
    }
}

int keyboard_has_key(void) { return buf_head != buf_tail; }

char keyboard_getchar(void) {
    while (buf_head == buf_tail) __asm__ volatile ("hlt");
    char c = buffer[buf_tail];
    buf_tail = (buf_tail + 1) % BUF_SIZE;
    return c;
}
