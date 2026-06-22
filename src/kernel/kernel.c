#include "libc.h"
#include "ramfs.h"
#include "keyboard.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEM 0xB8000

#define THEME_NC 0
#define THEME_DOS 1
#define THEME_MATRIX 2
#define THEME_KITTY 3
#define THEME_BRUTAL 4
#define THEME_OCEAN 5
#define THEME_SUNSET 6
int current_theme = THEME_NC;

unsigned short themes[7][6] = {
    {0x1F, 0x3F, 0x70, 0x1E, 0x74, 0x10},
    {0x0F, 0x0A, 0x07, 0x0E, 0x0C, 0x00},
    {0x02, 0x0A, 0x00, 0x0A, 0x0C, 0x00},
    {0x5F, 0x7D, 0x5F, 0x7E, 0x7C, 0x50},
    {0x40, 0x4C, 0x47, 0x4E, 0x4C, 0x00},
    {0x1B, 0x3B, 0x1F, 0x3E, 0x3C, 0x10},
    {0x6E, 0x6C, 0x67, 0x6E, 0x6C, 0x60}
};

volatile unsigned short* vga = (volatile unsigned short*)VGA_MEM;
unsigned short vga_buffer[VGA_WIDTH * VGA_HEIGHT];
int hours = 13, minutes = 37, seconds = 0, pit_ticks = 0;
int boot_done = 0;
int active_term = 0;
int fan_frame = 0;

#define TERM_LINES 500
char term_buf_left[TERM_LINES][40];
char term_buf_right[TERM_LINES][40];
int term_lines_left = 0;
int term_lines_right = 0;
int term_scroll_left = 0;
int term_scroll_right = 0;

char cmd_left[64] = {0};
char cmd_right[64] = {0};
int pos_left = 0, pos_right = 0;

char cwd_left[64] = "/";
char cwd_right[64] = "/";

int game_active = 0;
int game_number = 0;
int game_term = 0;

void outb(unsigned short port, unsigned char val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

unsigned short get_color(int idx) {
    return themes[current_theme][idx];
}

void vga_putc_buf(int x, int y, char c, unsigned char color) {
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT) return;
    vga_buffer[y * VGA_WIDTH + x] = (color << 8) | c;
}

void vga_print_buf(int x, int y, char* s, unsigned char color) {
    while (*s) vga_putc_buf(x++, y, *s++, color);
}

void vga_fill_buf(int x, int y, int w, int h, char c, unsigned char color) {
    for (int iy = 0; iy < h; iy++) {
        for (int ix = 0; ix < w; ix++) {
            vga_putc_buf(x + ix, y + iy, c, color);
        }
    }
}

void vga_flush() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        if (vga[i]!= vga_buffer[i]) vga[i] = vga_buffer[i];
    }
}

void pit_init() {
    outb(0x43, 0x36);
    outb(0x40, 0x9C);
    outb(0x40, 0x2E);
}

void draw_dick_boot() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga_buffer[i] = 0x0F20;
    vga_print_buf(30, 11, "HeavyYaycaOS Loading...", 0x0F);
    vga_putc_buf(19, 13, '[', 0x0F);
    vga_putc_buf(60, 13, ']', 0x0F);
    vga_flush();

    for (int i = 0; i < 39; i++) {
        vga_putc_buf(20, 13, '8', 0x0A);
        for (int j = 0; j < i; j++) vga_putc_buf(21+j, 13, '=', 0x0A);
        vga_putc_buf(21+i, 13, 'g', 0x0A);
        vga_flush();
        for (volatile int k = 0; k < 5000000; k++);
    }
    for (volatile int i = 0; i < 50000000; i++);
}

void draw_header() {
    unsigned short header_color = get_color(4);
    vga_fill_buf(0, 0, VGA_WIDTH, 1, ' ', header_color);
    vga_print_buf(33, 0, "HEAVY-YAYCA", header_color);

    char* frames[] = {"-", "\\", "|", "/"};
    vga_print_buf(76, 0, frames[fan_frame], get_color(3));
    fan_frame = (fan_frame + 1) % 4;

    vga_fill_buf(0, 24, VGA_WIDTH, 1, ' ', header_color);
    vga_print_buf(48, 24, "v1.1 BALLS DELUXE ITT42", header_color);
}

void draw_clock() {
    if (!boot_done) return;
    char buf[9];
    buf[0] = '0' + hours/10; buf[1] = '0' + hours%10; buf[2] = ':';
    buf[3] = '0' + minutes/10; buf[4] = '0' + minutes%10; buf[5] = ':';
    buf[6] = '0' + seconds/10; buf[7] = '0' + seconds%10; buf[8] = 0;
    vga_print_buf(70, 0, buf, get_color(4));
}

void timer_tick() {
    if (!boot_done) return;
    pit_ticks++;
    if (pit_ticks >= 100) {
        pit_ticks = 0;
        seconds++;
        if (seconds >= 60) { seconds = 0; minutes++; }
        if (minutes >= 60) { minutes = 0; hours++; }
        if (hours >= 24) hours = 0;
    }
}

void term_print(int term, char* str) {
    char (*buf)[40] = term == 0? term_buf_left : term_buf_right;
    int* lines = term == 0? &term_lines_left : &term_lines_right;
    int* scroll = term == 0? &term_scroll_left : &term_scroll_right;

    int len = strlen(str);
    int pos = 0;
    while (pos < len) {
        if (*lines >= TERM_LINES) {
            for (int i = 0; i < TERM_LINES-1; i++) strcpy(buf[i], buf[i+1]);
            (*lines)--;
        }
        int chunk = len - pos;
        if (chunk > 39) chunk = 39;
        for (int i = 0; i < chunk; i++) buf[*lines][i] = str[pos + i];
        buf[*lines][chunk] = 0;
        (*lines)++;
        pos += chunk;
    }
    *scroll = *lines > 18? *lines - 18 : 0;
}

void draw_terminal(int x, int y, int w, int h, int active, char* cmd, int cmd_pos, char buf[][40], int lines, int scroll, char* cwd) {
    unsigned short border = active? get_color(1) : get_color(2);
    unsigned short bg = get_color(0);
    unsigned short shadow = get_color(5);

    vga_fill_buf(x+1, y+h, w, 1, ' ', shadow);
    vga_fill_buf(x+w, y+1, 1, h, ' ', shadow);
    vga_fill_buf(x, y, w, h, ' ', bg);

    for (int i = 0; i < w; i++) { vga_putc_buf(x+i, y, 0xCD, border); vga_putc_buf(x+i, y+h-1, 0xCD, border); }
    for (int i = 1; i < h-1; i++) { vga_putc_buf(x, y+i, 0xBA, border); vga_putc_buf(x+w-1, y+i, 0xBA, border); }
    vga_putc_buf(x, y, 0xC9, border); vga_putc_buf(x+w-1, y, 0xBB, border);
    vga_putc_buf(x, y+h-1, 0xC8, border); vga_putc_buf(x+w-1, y+h-1, 0xBC, border);

    char title[40];
    strcpy(title, " ");
    strcat(title, cwd);
    strcat(title, active? "* " : " ");
    vga_print_buf(x+2, y, title, border);

    int max_lines = h - 3;
    int start = scroll;
    if (start < 0) start = 0;
    if (start > lines - max_lines) start = lines - max_lines > 0? lines - max_lines : 0;

    for (int i = 0; i < max_lines && start + i < lines; i++) {
        vga_print_buf(x+1, y+1+i, buf[start + i], get_color(3));
    }

    char prompt[70];
    strcpy(prompt, cwd);
    strcat(prompt, ">");
    vga_print_buf(x+1, y+h-2, prompt, active? get_color(3) : get_color(2));
    vga_print_buf(x+1+strlen(prompt), y+h-2, cmd, active? get_color(3) : get_color(2));
    if (active) vga_putc_buf(x+1+strlen(prompt)+cmd_pos, y+h-2, '_', get_color(3));
}

void draw_desktops() {
    for (int y = 1; y < 24; y++) {
        unsigned short shade = get_color(0);
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putc_buf(x, y, ' ', shade);
        }
    }

    draw_terminal(0, 1, 40, 22, active_term == 0, cmd_left, pos_left, term_buf_left, term_lines_left, term_scroll_left, cwd_left);
    draw_terminal(40, 1, 40, 22, active_term == 1, cmd_right, pos_right, term_buf_right, term_lines_right, term_scroll_right, cwd_right);
}

void resolve_path(char* cwd, char* name, char* out) {
    if (name[0] == '/') {
        strcpy(out, name);
        return;
    }
    if (strcmp(name, "..") == 0) {
        strcpy(out, cwd);
        int len = strlen(out);
        if (len > 1) {
            out[len-1] = 0;
            while (len > 1 && out[len-1]!= '/') len--;
            out[len] = 0;
        }
        if (out[0] == 0) strcpy(out, "/");
        return;
    }
    strcpy(out, cwd);
    if (out[strlen(out)-1]!= '/') strcat(out, "/");
    strcat(out, name);
}

void cmd_tuta(int term) {
    extern RamFile files[MAX_FILES];
    char* cwd = term == 0? cwd_left : cwd_right;
    int count = 0;
    for (int i = 0; i < MAX_FILES && count < 15; i++) {
        if (files[i].used) {
            if (strncmp(files[i].name, cwd, strlen(cwd)) == 0) {
                char* rest = files[i].name + strlen(cwd);
                if (strchr(rest, '/') == NULL || strchr(rest, '/') == rest + strlen(rest) - 1) {
                    char buf[40];
                    strcpy(buf, rest);
                    if (files[i].name[strlen(files[i].name)-1] == '/') strcat(buf, " [DIR]");
                    else {
                        strcat(buf, " ");
                        char size[12];
                        int s = files[i].size, idx = 0;
                        if (s == 0) size[idx++] = '0';
                        while (s > 0) { size[idx++] = '0' + (s % 10); s /= 10; }
                        size[idx] = 0;
                        for (int j = 0; j < idx/2; j++) { char t = size[j]; size[j] = size[idx-1-j]; size[idx-1-j] = t; }
                        strcat(buf, size);
                    }
                    term_print(term, buf);
                    count++;
                }
            }
        }
    }
    if (count == 0) term_print(term, "Pusto");
}

void cmd_faylblyat(int term, char* name) {
    char* cwd = term == 0? cwd_left : cwd_right;
    char path[128];
    resolve_path(cwd, name, path);
    if (ramfs_create(path) == 0) term_print(term, "Sozdan");
    else term_print(term, "Oshibka");
}

void cmd_papka(int term, char* name) {
    char* cwd = term == 0? cwd_left : cwd_right;
    char path[128];
    resolve_path(cwd, name, path);
    int len = strlen(path);
    if (path[len-1]!= '/') { path[len] = '/'; path[len+1] = 0; }
    if (ramfs_create(path) == 0) term_print(term, "Papka sozdana");
    else term_print(term, "Oshibka");
}

void cmd_cd(int term, char* name) {
    char* cwd = term == 0? cwd_left : cwd_right;
    char path[128];
    resolve_path(cwd, name, path);

    if (strcmp(name, "/") == 0) {
        strcpy(cwd, "/");
        term_print(term, "Korень");
        return;
    }

    int len = strlen(path);
    if (path[len-1]!= '/') { path[len] = '/'; path[len+1] = 0; }

    extern RamFile files[MAX_FILES];
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, path) == 0) {
            strcpy(cwd, path);
            term_print(term, "Pereshel");
            return;
        }
    }
    term_print(term, "Net takoy papki");
}

void cmd_vysri(int term, char* name) {
    char* cwd = term == 0? cwd_left : cwd_right;
    char path[128];
    resolve_path(cwd, name, path);
    char buf[256];
    int n = ramfs_read(path, buf);
    if (n > 0) {
        buf[n > 250? 250 : n] = 0;
        term_print(term, buf);
    } else term_print(term, "Net fayla");
}

void cmd_napishi(int term, char* text, char* filename) {
    char* cwd = term == 0? cwd_left : cwd_right;
    char path[128];
    resolve_path(cwd, filename, path);
    if (ramfs_create(path) == 0 || ramfs_write(path, text, strlen(text)) == strlen(text)) {
        term_print(term, "Zapisano");
    } else term_print(term, "Oshibka zapisi");
}

void cmd_blyat(int term, char* name) {
    char* cwd = term == 0? cwd_left : cwd_right;
    char path[128];
    resolve_path(cwd, name, path);
    if (ramfs_delete(path) == 0) term_print(term, "Udaleno");
    else term_print(term, "Net takogo");
}

void cmd_clear(int term) {
    if (term == 0) term_lines_left = 0;
    else term_lines_right = 0;
}

void cmd_vremya(int term, char* argv[], int argc) {
    if (argc == 4) {
        hours = atoi(argv[1]) % 24;
        minutes = atoi(argv[2]) % 60;
        seconds = atoi(argv[3]) % 60;
        term_print(term, "Vremya ustanovleno");
    } else {
        char buf[20] = "Time: ";
        buf[6] = '0' + hours/10; buf[7] = '0' + hours%10; buf[8] = ':';
        buf[9] = '0' + minutes/10; buf[10] = '0' + minutes%10; buf[11] = ':';
        buf[12] = '0' + seconds/10; buf[13] = '0' + seconds%10; buf[14] = 0;
        term_print(term, buf);
    }
}

void cmd_yaycc(int term, char* filename) {
    char* cwd = term == 0? cwd_left : cwd_right;
    char path[128];
    resolve_path(cwd, filename, path);
    char src[1024];
    int n = ramfs_read(path, src);
    if (n < 0) { term_print(term, "Net fayla"); return; }
    src[n] = 0;
void cmd_yaycc(int term, char* filename) {
    char* cwd = term == 0? cwd_left : cwd_right;
    char path[128];
    resolve_path(cwd, filename, path);
    char src[1024];
    int n = ramfs_read(path, src);
    if (n < 0) { term_print(term, "Net fayla"); return; }
    src[n] = 0;
    char* p = src;
    int vars[26] = {0};

    while (*p) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
        if (!*p) break;

        if (strncmp(p, "ZALUPA", 6) == 0) {
            p += 6;
            while (*p == ' ') p++;
            if (*p == '"') {
                p++;
                char out[40];
                int i = 0;
                while (*p && *p!= '"' && i < 39) out[i++] = *p++;
                out[i] = 0;
                term_print(term, out);
                if (*p == '"') p++;
            }
        }
        else if (strncmp(p, "PISKA", 5) == 0) {
            p += 5;
            while (*p == ' ') p++;
            if (*p >= 'a' && *p <= 'z') {
                int var = *p - 'a';
                p++;
                while (*p == ' ') p++;
                if (*p == '=') {
                    p++;
                    while (*p == ' ') p++;
                    vars[var] = atoi(p);
                    while (*p >= '0' && *p <= '9') p++;
                }
            }
        }
        else if (strncmp(p, "GOVNO", 5) == 0) {
            p += 5;
            while (*p == ' ') p++;
            if (*p >= 'a' && *p <= 'z') {
                int var = *p - 'a';
                char buf[12];
                int v = vars[var];
                int idx = 0;
                if (v < 0) { buf[idx++] = '-'; v = -v; }
                if (v == 0) buf[idx++] = '0';
                while (v > 0) { buf[idx++] = '0' + (v % 10); v /= 10; }
                buf[idx] = 0;
                int start = buf[0] == '-'? 1 : 0;
                for (int j = start; j < (idx + start) / 2; j++) {
                    char t = buf[j];
                    buf[j] = buf[idx - 1 - j + start];
                    buf[idx - 1 - j + start] = t;
                }
                term_print(term, buf);
                p++;
            }
        }
        else if (strncmp(p, "PLUS", 4) == 0) {
            p += 4;
            while (*p == ' ') p++;
            if (*p >= 'a' && *p <= 'z') {
                int v1 = *p - 'a';
                p++;
                while (*p == ' ') p++;
                if (*p >= 'a' && *p <= 'z') {
                    int v2 = *p - 'a';
 vars[v1] += vars[v2];
                    p++;
                }
            }
        }
        else if (strncmp(p, "MINUS", 5) == 0) {
            p += 5;
            while (*p == ' ') p++;
            if (*p >= 'a' && *p <= 'z') {
                int v1 = *p - 'a';
                p++;
                while (*p == ' ') p++;
                if (*p >= 'a' && *p <= 'z') {
                    int v2 = *p - 'a';
 vars[v1] -= vars[v2];
                    p++;
                }
            }
        }
        else if (strncmp(p, "ESLI", 4) == 0) {
            p += 4;
            while (*p == ' ') p++;
            if (*p >= 'a' && *p <= 'z') {
                int v1 = *p - 'a';
                p++;
                while (*p == ' ') p++;
                char op = *p++;
                while (*p == ' ') p++;
                int v2 = atoi(p);
                while (*p >= '0' && *p <= '9') p++;
                while (*p == ' ') p++;
                if (strncmp(p, "TO", 2) == 0) {
                    p += 2;
                    int cond = 0;
 if (op == '=') cond = vars[v1] == v2;
 else if (op == '>') cond = vars[v1] > v2;
 else if (op == '<') cond = vars[v1] < v2;
                    if (!cond) {
                        while (*p && strncmp(p, "KONEC", 5)!= 0) p++;
                        if (strncmp(p, "KONEC", 5) == 0) p += 5;
                    }
                }
            }
        }
        else if (strncmp(p, "POKA", 4) == 0) {
            p += 4;
            while (*p == ' ') p++;
            if (*p >= 'a' && *p <= 'z') {
                int v1 = *p - 'a';
                p++;
                while (*p == ' ') p++;
                char op = *p++;
                while (*p == ' ') p++;
                int v2 = atoi(p);
                while (*p >= '0' && *p <= '9') p++;
                while (*p == ' ') p++;
                if (strncmp(p, "DELAY", 5) == 0) {
                    p += 5;
                    char* loop_start = p;
                    while (1) {
                        int cond = 0;
 if (op == '=') cond = vars[v1] == v2;
 else if (op == '>') cond = vars[v1] > v2;
 else if (op == '<') cond = vars[v1] < v2;
                        if (!cond) break;

                        char* tmp = loop_start;
                        while (*tmp && strncmp(tmp, "KONEC", 5)!= 0) {
                            if (strncmp(tmp, "MINUS", 5) == 0) {
                                tmp += 5; while (*tmp == ' ') tmp++;
                                if (*tmp >= 'a' && *tmp <= 'z') {
                                    int vx = *tmp - 'a'; tmp++;
                                    while (*tmp == ' ') tmp++;
                                    if (*tmp >= 'a' && *tmp <= 'z') {
                                        int vy = *tmp - 'a';
                                        vars[vx] -= vars[vy];
                                        tmp++;
                                    }
                                }
                            } else tmp++;
                        }
 if (vars[v1] < 0) vars[v1] = 0;
                    }
                    while (*p && strncmp(p, "KONEC", 5)!= 0) p++;
                    if (strncmp(p, "KONEC", 5) == 0) p += 5;
                }
            }
        }
        else p++;
    }
}
}

void cmd_uchebnik(int term) {
    term_print(term, "YAYCA++ v3:");
    term_print(term, "ZALUPA \"text\" - vyvod");
    term_print(term, "PISKA a = 42 - peremennaya");
    term_print(term, "GOVNO a - vyvod peremennoy");
    term_print(term, "PLUS a b - a = a + b");
    term_print(term, "MINUS a b - a = a - b");
    term_print(term, "ESLI a > 10 TO... KONEC");
    term_print(term, "POKA a > 0 DELAY... KONEC");
    term_print(term, "Primer cikla:");
    term_print(term, "PISKA x=3");
    term_print(term, "PISKA y=1");
    term_print(term, "POKA x > 0 DELAY");
    term_print(term, "ZALUPA \"BALLS\"");
    term_print(term, "MINUS x y");
    term_print(term, "KONEC");
}

void cmd_skazhi(int term, char* argv[], int argc) {
    char output[128] = {0};
    for (int i = 1; i < argc; i++) {
        strcat(output, argv[i]);
        if (i < argc - 1) strcat(output, " ");
    }
    term_print(term, output);
}

void cmd_mudro(int term) {
    char* quotes[] = {
        "U vorobya dva BALLS - poetomu dva terminala",
        "Vorobey bez BALLS - eto golub",
        "Ne trogay BALLS vorobya - klyunet",
        "Kto rano vstaet - tomu vorobey BALLS otdavit",
        "V chuzhoe gnezdo so svoimi BALLS ne lezut",
        "Vorobey s BALLS - eto uzhe orel",
        "BALLS vorobya vidny v polet",
        "Luchshe sinica v rukah, chem BALLS vorobya",
        "Vorobey na BALLS ne serdit",
        "Mal vorobey da BALLS bolshie",
        "BALLS ne meshok - za spinoi ne nosish",
        "Vorobey s BALLS v pole voin",
        "U horoshego vorobya BALLS vsegda pri nem",
        "BALLS dorogi k obedu",
        "Vorobey stary - BALLS molody",
        "Ne v BALLS schastye vorobya",
        "BALLS vorobya ne kormyat",
        "Na chuzhie BALLS rot ne razevaj",
        "BALLS k BALLS ne prikladyvay",
        "S vorobyinymi BALLS v orlinoe gnezdo"
    };
    term_print(term, quotes[pit_ticks % 20]);
}

void cmd_theme(int term, char* argv[], int argc) {
    if (argc == 2) {
        if (strcmp(argv[1], "nc") == 0) { current_theme = THEME_NC; term_print(term, "Tema: Norton"); }
        else if (strcmp(argv[1], "dos") == 0) { current_theme = THEME_DOS; term_print(term, "Tema: DOS"); }
        else if (strcmp(argv[1], "matrix") == 0) { current_theme = THEME_MATRIX; term_print(term, "Tema: Matrix"); }
        else if (strcmp(argv[1], "kitty") == 0) { current_theme = THEME_KITTY; term_print(term, "Tema: Hello Kitty"); }
        else if (strcmp(argv[1], "brutal") == 0) { current_theme = THEME_BRUTAL; term_print(term, "Tema: BRUTAL"); }
        else if (strcmp(argv[1], "ocean") == 0) { current_theme = THEME_OCEAN; term_print(term, "Tema: Ocean"); }
        else if (strcmp(argv[1], "sunset") == 0) { current_theme = THEME_SUNSET; term_print(term, "Tema: Sunset"); }
        else term_print(term, "Temy: nc dos matrix kitty brutal ocean sunset");
    } else term_print(term, "theme nc|dos|matrix|kitty|brutal|ocean|sunset");
}

void cmd_ugaday(int term) {
    game_active = 1;
    game_term = term;
    game_number = pit_ticks % 11;
    term_print(term, "Ugada y chislo ot 0 do 10");
}

void cmd_help(int term) {
    term_print(term, "tuta - spisok faylov/papok");
    term_print(term, "faylblyat <name> - sozdat fayl");
    term_print(term, "papka <name> - sozdat papku");
    term_print(term, "cd <name> - voyti v papku");
    term_print(term, "cd.. - vyiti iz papki");
    term_print(term, "vysri <name> - prochitat fayl");
    term_print(term, "napishi <text> <file> - zapisat");
    term_print(term, "blyat <name> - udalit fayl/papku");
    term_print(term, "clear - ochistit terminal");
    term_print(term, "vremya [HH MM SS] - pokazat/ustanovit");
    term_print(term, "yaycc <file> - YAYCA++ v3");
    term_print(term, "uchebnik - obuchenie YAYCA++");
    term_print(term, "skazhi <text> - vyvesti text");
    term_print(term, "mudro - citata pro vorobya");
    term_print(term, "theme <name> - smena temy");
    term_print(term, "ugaday - igra ugaday chislo");
    term_print(term, "rkn - vyklyuchit");
    term_print(term, "TAB - pereklyuchit terminal");
    term_print(term, "Up/Down - prokrutka 500 strok");
}

void execute_cmd(int term, char* buf) {
    char* argv[8];
    int argc = 0;
    char* p = buf;
    while (*p && argc < 8) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p!= ' ') p++;
        if (*p) *p++ = 0;
    }
    if (argc == 0) return;

    if (game_active && term == game_term) {
        int guess = atoi(argv[0]);
        if (guess == game_number) {
            term_print(term, "UGADAL! Vorobey dovolen");
            game_active = 0;
        } else if (guess < game_number) term_print(term, "Bolshe");
        else term_print(term, "Menshe");
        return;
    }

    if (strcmp(argv[0], "tuta") == 0) cmd_tuta(term);
    else if (strcmp(argv[0], "faylblyat") == 0 && argc > 1) cmd_faylblyat(term, argv[1]);
    else if (strcmp(argv[0], "papka") == 0 && argc > 1) cmd_papka(term, argv[1]);
    else if (strcmp(argv[0], "cd") == 0 && argc > 1) cmd_cd(term, argv[1]);
    else if (strcmp(argv[0], "vysri") == 0 && argc > 1) cmd_vysri(term, argv[1]);
    else if (strcmp(argv[0], "napishi") == 0 && argc > 2) cmd_napishi(term, argv[1], argv[2]);
    else if (strcmp(argv[0], "blyat") == 0 && argc > 1) cmd_blyat(term, argv[1]);
    else if (strcmp(argv[0], "clear") == 0) cmd_clear(term);
    else if (strcmp(argv[0], "vremya") == 0) cmd_vremya(term, argv, argc);
    else if (strcmp(argv[0], "yaycc") == 0 && argc > 1) cmd_yaycc(term, argv[1]);
    else if (strcmp(argv[0], "uchebnik") == 0) cmd_uchebnik(term);
    else if (strcmp(argv[0], "skazhi") == 0) cmd_skazhi(term, argv, argc);
    else if (strcmp(argv[0], "mudro") == 0) cmd_mudro(term);
    else if (strcmp(argv[0], "theme") == 0) cmd_theme(term, argv, argc);
    else if (strcmp(argv[0], "ugaday") == 0) cmd_ugaday(term);
    else if (strcmp(argv[0], "help") == 0) cmd_help(term);
    else if (strcmp(argv[0], "rkn") == 0) asm volatile("cli; hlt");
    else term_print(term, "Huy znayet takuyu komandu");
}

void yayca_shell() {
    pit_init();
    ramfs_init();
    ramfs_create("/README");
    ramfs_write("/README", "ZALUPA \"Privet BALLS\"", 20);
    ramfs_create("/test.yay");
    ramfs_write("/test.yay", "PISKA x=10\nPISKA y=5\nPLUS x y\nZALUPA \"BALLS:\"\nGOVNO x", 58);
    ramfs_create("/papka/");

    term_print(0, "Levoe BALL vorobya");
    term_print(1, "Pravoe BALL vorobya");
    term_print(0, "TAB - pereklyuchenie");
    term_print(1, "help - komandy");

    while (1) {
        timer_tick();
        draw_header();
        draw_clock();
        draw_desktops();
        vga_flush();

        if (inb(0x64) & 1) {
            unsigned char sc = inb(0x60);
            char c = scancode_to_ascii(sc);
            char* cur_cmd = active_term == 0? cmd_left : cmd_right;
            int* cur_pos = active_term == 0? &pos_left : &pos_right;
            int* cur_scroll = active_term == 0? &term_scroll_left : &term_scroll_right;
            int* cur_lines = active_term == 0? &term_lines_left : &term_lines_right;
            char* cwd = active_term == 0? cwd_left : cwd_right;

            if (sc == 0x1C) {
                char prompt[70];
                strcpy(prompt, cwd);
                strcat(prompt, ">");
                term_print(active_term, prompt);
                term_print(active_term, cur_cmd);
                execute_cmd(active_term, cur_cmd);
                *cur_pos = 0;
                for (int i = 0; i < 64; i++) cur_cmd[i] = 0;
            }
            else if (sc == 0x0E && *cur_pos > 0) {
                (*cur_pos)--;
                cur_cmd[*cur_pos] = 0;
            }
            else if (sc == 0x48) {
                if (*cur_scroll > 0) (*cur_scroll)--;
            }
            else if (sc == 0x50) {
                if (*cur_scroll < *cur_lines - 18) (*cur_scroll)++;
            }
            else if (sc == 0x0F) {
                active_term =!active_term;
            }
            else if (c && *cur_pos < 60) {
                cur_cmd[(*cur_pos)++] = c;
                cur_cmd[*cur_pos] = 0;
            }
        }
        for (volatile int i = 0; i < 10000; i++);
    }
}

void kernel_main() {
    draw_dick_boot();
    boot_done = 1;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) vga_buffer[i] = 0x1F20;
    vga_flush();
    yayca_shell();
}
