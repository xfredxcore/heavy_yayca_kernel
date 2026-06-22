#include "libc.h"
#include "vga.h"
#include "yayca_lang.h"

int vars[26]; // A-Z

void yayca_run(char* code) {
    char* line = code;
    while (*line) {
        char* next = line;
        while (*next && *next!= '\n') next++;
        char old = *next;
        *next = 0;

        // PRINT "хуй"
        if (line[0]=='P' && line[1]=='R' && line[2]=='I' && line[3]=='N' && line[4]=='T') {
            char* s = line + 6;
            if (*s == '"') {
                s++;
                char* end = s;
                while (*end && *end!= '"') end++;
                *end = 0;
                vga_draw_string(0, 0, s, 15);
            }
        }
        // LET A = 5
        else if (line[0]=='L' && line[1]=='E' && line[2]=='T') {
            char var = line[4];
            int val = atoi(line + 8);
            if (var >= 'A' && var <= 'Z') vars[var - 'A'] = val;
        }

        *next = old;
        line = next;
        if (*line) line++;
    }
}
