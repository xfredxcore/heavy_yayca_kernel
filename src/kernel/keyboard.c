#include "keyboard.h"

static int shift_pressed = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(unsigned short port, unsigned char val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char get_key() {
    unsigned char sc = 0;
    while ((inb(0x64) & 1) == 0);
    sc = inb(0x60);
    return sc;
}

char scancode_to_ascii(unsigned char sc) {
    if (sc == 0x2A || sc == 0x36) { shift_pressed = 1; return 0; }
    if (sc == 0xAA || sc == 0xB6) { shift_pressed = 0; return 0; }
    if (sc == 0x1D) { ctrl_pressed = 1; return 0; }
    if (sc == 0x9D) { ctrl_pressed = 0; return 0; }
    if (sc == 0x38) { alt_pressed = 1; return 0; }
    if (sc == 0xB8) { alt_pressed = 0; return 0; }

    // Ctrl+Alt+Del = reboot
    if (ctrl_pressed && alt_pressed && sc == 0x53) {
        outb(0x64, 0xFE);
        return 0;
    }

    if (sc & 0x80) return 0;
    if (sc >= 128) return 0;

    static char map_en[128] = {
        0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
        'a','s','d','f','g','h','j','k','l',';','\'','`', 0, '\\',
        'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' '
    };

    static char map_en_shift[128] = {
        0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
        '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
        'A','S','D','F','G','H','J','K','L',':','"','~', 0, '|',
        'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' '
    };

    return shift_pressed? map_en_shift[(int)sc] : map_en[(int)sc];
}
