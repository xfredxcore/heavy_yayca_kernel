#include "vga.h"
#include "gui.h"

#define COLOR_BG 2 // зеленый
#define COLOR_TASKBAR 7 // серый
#define COLOR_TEXT 13 // розовый
#define COLOR_DICK 12 // красный хуй
#define COLOR_WHITE 15

int mouse_x = 160, mouse_y = 100;
int clock_tick = 0;

void gui_draw_desktop() {
    vga_clear(COLOR_BG);
    // Обои: ВОРОБЕЙ С ЯЙЦАМИ
    vga_draw_string(80, 80, "VOROBEY S YAYCAMI", COLOR_TEXT);
    vga_draw_string(120, 100, " O ", COLOR_WHITE);
    vga_draw_string(120, 108, "<( )>", COLOR_WHITE);
    vga_draw_string(128, 116, " O O", COLOR_WHITE);
}

void gui_draw_taskbar() {
    // Панель задач снизу
    for (int y = 180; y < 200; y++)
        for (int x = 0; x < 320; x++)
            vga_set_pixel(x, y, COLOR_TASKBAR);

    // Кнопка ПУСК = воробей
    vga_draw_string(4, 184, "[YAYCA]", COLOR_TEXT);
}

void gui_draw_dickbar(int progress) {
    // Член загрузки. 0-100%
    int len = (progress * 200) / 100;
    // Головка
    for (int y = 90; y < 110; y++)
        for (int x = 50 + len; x < 60 + len; x++)
            vga_set_pixel(x, y, COLOR_DICK);
    // Ствол
    for (int y = 95; y < 105; y++)
        for (int x = 50; x < 50 + len; x++)
            vga_set_pixel(x, y, COLOR_DICK);
    // Яйца в начале
    for (int y = 105; y < 115; y++) {
        for (int x = 40; x < 50; x++) vga_set_pixel(x, y, COLOR_DICK);
        for (int x = 30; x < 40; x++) vga_set_pixel(x, y, COLOR_DICK);
    }
    vga_draw_string(50, 120, "ZAGRUZKA...", COLOR_WHITE);
}

void gui_draw_cursor(int x, int y) {
    // Курсор-стрелка
    vga_set_pixel(x, y, COLOR_WHITE);
    vga_set_pixel(x+1, y+1, COLOR_WHITE);
    vga_set_pixel(x+2, y+2, COLOR_WHITE);
    vga_set_pixel(x, y+1, COLOR_WHITE);
    vga_set_pixel(x, y+2, COLOR_WHITE);
}

void gui_draw_clock() {
    clock_tick++;
    int sec = clock_tick / 18; // PIT 18.2 Гц
    char buf[9];
    buf[0] = '0' + (sec / 36000) % 10;
    buf[1] = '0' + (sec / 3600) % 10;
    buf[2] = ':';
    buf[3] = '0' + (sec / 600) % 6;
    buf[4] = '0' + (sec / 60) % 10;
    buf[5] = ':';
    buf[6] = '0' + (sec / 10) % 6;
    buf[7] = '0' + sec % 10;
    buf[8] = 0;
    vga_draw_string(260, 184, buf, 0);
}

void gui_init() {
    vga_init_graphics();
    gui_draw_desktop();
    gui_draw_taskbar();
    for (int i = 0; i <= 100; i++) {
        gui_draw_dickbar(i);
        for (volatile int j = 0; j < 1000000; j++); // задержка
    }
    vga_clear(COLOR_BG);
    gui_draw_desktop();
    gui_draw_taskbar();
}
