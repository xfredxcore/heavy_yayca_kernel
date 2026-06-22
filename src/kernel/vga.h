#ifndef VGA_H
#define VGA_H

#define WHITE_THEME 0xF0
#define BLACK_THEME 0x0F

void vga_set_theme(unsigned char theme);
void vga_draw_box(int x, int y, int w, int h, char* title);
void vga_draw_window(int x, int y, int w, int h, char* title);
void vga_load_cp866();
void vga_clear_theme(unsigned char theme);

#endif
