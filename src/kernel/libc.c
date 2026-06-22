#include "libc.h"

void* memcpy(void* dest, const void* src, int n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (int i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

void* memset(void* s, int c, int n) {
    char* p = (char*)s;
    for (int i = 0; i < n; i++) p[i] = c;
    return s;
}

int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i]!= s2[i]) return *(unsigned char*)(s1+i) - *(unsigned char*)(s2+i);
        if (s1[i] == 0) return 0;
    }
    return 0;
}

void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

void strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while ((*dest++ = *src++));
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == c) return (char*)s;
        s++;
    }
    return NULL;
}

int atoi(const char* s) {
    int res = 0, sign = 1;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res * sign;
}
