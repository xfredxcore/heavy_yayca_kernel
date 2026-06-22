#ifndef LIBC_H
#define LIBC_H

#define NULL ((void*)0)

void* memcpy(void* dest, const void* src, int n);
void* memset(void* s, int c, int n);
int strlen(const char* s);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, int n);
void strcpy(char* dest, const char* src);
void strcat(char* dest, const char* src);
char* strchr(const char* s, int c);
int atoi(const char* s);

#endif
