#ifndef GUI_H
#define GUI_H

void gui_init();
void gui_draw_desktop();
void gui_draw_taskbar();
void gui_draw_cursor(int x, int y);
void gui_draw_dickbar(int progress); // 0-100
void gui_draw_clock();

#endif
