#include "keyboard.h"

#define KBD_DATA 0x60
#define BUF_SIZE 256

/* Tabla sin Shift */
static const char kbd_lower[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,  '*', 0,  ' ',
};

/* Tabla con Shift */
static const char kbd_upper[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?',
    0,  '*', 0,  ' ',
};

static volatile char     buffer[BUF_SIZE];
static volatile uint32_t buf_head = 0;
static volatile uint32_t buf_tail = 0;

static int shift_pressed = 0;
static int caps_lock     = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void keyboard_init(void) {
    buf_head = 0;
    buf_tail = 0;
    shift_pressed = 0;
    caps_lock = 0;
}

void keyboard_handler(void) {
    uint8_t sc = inb(KBD_DATA);

    /* Shift (0x2A left, 0x36 right) */
    if (sc == 0x2A || sc == 0x36) { shift_pressed = 1; return; }
    if (sc == 0xAA || sc == 0xB6) { shift_pressed = 0; return; }

    /* Caps Lock (0x3A) */
    if (sc == 0x3A) { caps_lock = !caps_lock; return; }

    /* Ignorar teclas soltadas */
    if (sc & 0x80) return;
    if (sc >= 128) return;

    char c = shift_pressed ? kbd_upper[sc] : kbd_lower[sc];

    /* Caps Lock solo afecta a letras */
    if (caps_lock && c >= 'a' && c <= 'z') c -= 32;
    else if (caps_lock && shift_pressed && c >= 'A' && c <= 'Z') c += 32;

    if (c) {
        uint32_t next = (buf_head + 1) % BUF_SIZE;
        if (next != buf_tail) {
            buffer[buf_head] = c;
            buf_head = next;
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
