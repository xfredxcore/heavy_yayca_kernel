#include "vga.h"
#include "libc.h"

extern volatile unsigned char* vga;
extern unsigned int cursor_x, cursor_y;

unsigned char current_theme = 0xF0;

void outb(unsigned short port, unsigned char val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void vga_set_theme(unsigned char theme) {
    current_theme = theme;
}

void vga_clear_theme(unsigned char theme) {
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = theme;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_putc_at(int x, int y, char c) {
    int pos = (y * 80 + x) * 2;
    vga[pos] = c;
    vga[pos + 1] = current_theme;
}

void vga_print_at(int x, int y, const char* str) {
    while (*str) {
        vga_putc_at(x++, y, *str++);
    }
}

// CP866: 0xC9=╔ 0xCD=═ 0xBB=╗ 0xBA=║ 0xC8=╚ 0xBC=╝
void vga_draw_box(int x, int y, int w, int h, char* title) {
    // углы
    vga_putc_at(x, y, 0xC9); // ╔
    vga_putc_at(x + w - 1, y, 0xBB); // ╗
    vga_putc_at(x, y + h - 1, 0xC8); // ╚
    vga_putc_at(x + w - 1, y + h - 1, 0xBC); // ╝

    // горизонтали
    for (int i = 1; i < w - 1; i++) {
        vga_putc_at(x + i, y, 0xCD); // ═
        vga_putc_at(x + i, y + h - 1, 0xCD);
    }

    // вертикали
    for (int i = 1; i < h - 1; i++) {
        vga_putc_at(x, y + i, 0xBA); // ║
        vga_putc_at(x + w - 1, y + i, 0xBA);
    }

    // заголовок
    if (title) {
        int len = strlen(title);
        int tx = x + (w - len) / 2;
        vga_print_at(tx, y, title);
    }

    // очистка внутри
    for (int iy = 1; iy < h - 1; iy++) {
        for (int ix = 1; ix < w - 1; ix++) {
            vga_putc_at(x + ix, y + iy, ' ');
        }
    }
}

// Грузим CP866 в VGA. Спизжено из Linux.
void vga_load_cp866() {
    // Разрешаем запись в font RAM
    outb(0x3C4, 0x00); outb(0x3C5, 0x01); // reset
    outb(0x3C4, 0x02); outb(0x3C5, 0x04); // write plane 2
    outb(0x3C4, 0x04); outb(0x3C5, 0x07); // memory mode
    outb(0x3CE, 0x05); outb(0x3CF, 0x00); // graphics mode
    outb(0x3CE, 0x06); outb(0x3CF, 0x04); // misc

    // Тут должен быть массив font_cp866[4096], но для краткости хуй
    // В реале надо 256 символов * 16 байт. Спизди из Linux kernel
    // drivers/video/console/font_8x16.c

    // Возвращаем обычный режим
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x02); outb(0x3C5, 0x03);
    outb(0x3C4, 0x04); outb(0x3C5, 0x03);
    outb(0x3CE, 0x05); outb(0x3CF, 0x10);
    outb(0x3CE, 0x06); outb(0x3CF, 0x0E);
}
