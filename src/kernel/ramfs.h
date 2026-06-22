#ifndef RAMFS_H
#define RAMFS_H

#define MAX_FILES 64
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 4096

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    int size;
    int used;
} RamFile;

void ramfs_init();
int ramfs_create(char* name);
int ramfs_write(char* name, char* data, int size);
int ramfs_read(char* name, char* buf);
int ramfs_delete(char* name);
void ramfs_list();
RamFile* ramfs_find(char* name);

#endif
