#include "libc.h"
#include "libc.h"
#include "ramfs.h"

RamFile files[MAX_FILES];



void ramfs_init() {
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
    }
}

RamFile* ramfs_find(char* name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].name, name) == 0) {
            return &files[i];
        }
    }
    return 0;
}

int ramfs_create(char* name) {
    if (ramfs_find(name)) return -1; // уже есть
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].size = 0;
            strcpy(files[i].name, name);
            return 0;
        }
    }
    return -2; // нет места
}

int ramfs_write(char* name, char* data, int size) {
    RamFile* f = ramfs_find(name);
    if (!f) return -1;
    if (size > MAX_FILE_SIZE) size = MAX_FILE_SIZE;
    for (int i = 0; i < size; i++) f->data[i] = data[i];
    f->size = size;
    return size;
}

int ramfs_read(char* name, char* buf) {
    RamFile* f = ramfs_find(name);
    if (!f) return -1;
    for (int i = 0; i < f->size; i++) buf[i] = f->data[i];
    return f->size;
}

int ramfs_delete(char* name) {
    RamFile* f = ramfs_find(name);
    if (!f) return -1;
    f->used = 0;
    f->size = 0;
    return 0;
}
