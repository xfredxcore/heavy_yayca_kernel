#include "editor.h"
#include "ramfs.h"
#include "vga.h"
#include "keyboard.h"
#include "libc.h"

extern volatile unsigned char* vga;
extern unsigned int cursor_x, cursor_y;

void editor_run(char* filename) {
    char buf[4096];
    int size = 0;
    int pos = 0;
    int saved = 0;

    // читаем файл если есть
    int n = ramfs_read(filename, buf);
    if (n > 0) size = n;

    vga_clear_theme(0xF0); // белый фон
    vga_draw_box(0, 0, 80, 25, filename);
    vga_print_at(2, 24, "F2=Save F10=Exit Alt+Shift=RU/EN");

    cursor_x = 1; cursor_y = 1;

    while (1) {
        // рендер текста
        for (int i = 0; i < size; i++) {
            int x = 1 + (i % 78);
            int y = 1 + (i / 78);
            if (y >= 23) break;
            vga_putc_at(x, y, buf[i]);
        }

        // курсор
        int cx = 1 + (pos % 78);
        int cy = 1 + (pos / 78);
        cursor_x = cx; cursor_y = cy;

        unsigned char sc = get_key();
        if (sc == 0x44) { // F10
            break;
        }
        if (sc == 0x3C) { // F2 save
            ramfs_create(filename);
            ramfs_write(filename, buf, size);
            vga_print_at(30, 24, "SAVED!");
            saved = 1;
            continue;
        }
        if (sc == 0x0E && pos > 0) { // backspace
            pos--; size--;
            for (int i = pos; i < size; i++) buf[i] = buf[i+1];
            continue;
        }
        if (sc == 0x1C) { // enter
            if (size < 4095) {
                for (int i = size; i > pos; i--) buf[i] = buf[i-1];
                buf[pos++] = '\n';
                size++;
            }
            continue;
        }

        char c = scancode_to_ascii(sc);
        if (c && size < 4095) {
            for (int i = size; i > pos; i--) buf[i] = buf[i-1];
            buf[pos++] = c;
            size++;
        }
    }

    vga_clear_theme(0xF0);
}
