#include "dlibc.h"
#include <stdbool.h>
#include "doom1wad.h"

struct _IO_FILE {
    long offset;
};

struct _IO_FILE doom1;
static bool doom1_open = false;

int d_fseek(FILE *file, long offset, int origin ) {
    if(file == NULL) {
        return -1;
    }

    if(origin == SEEK_SET) {
        file->offset = offset;
    } else if(origin == SEEK_CUR) {
        file->offset += offset;
    } else if(origin == SEEK_END) {
        file->offset = doom1_wad_len + offset;
    } else {
        return -1;
    }
    return 0;
}

int d_fclose(FILE* file) { 
    if(file != &doom1) {
        return -1;
    }

    doom1_open = false; 
    return 0;
}

FILE* d_fopen(const char* filepath, const char* modes) { 
    if(doom1_open || d_strcmp(filepath, "doom1.wad") != 0) {
        return NULL;
    }

    doom1_open = true;
    doom1.offset = 0;

    return &doom1;
}

int d_fread(void* dest, size_t size, size_t count, FILE* file) {
    size_t bytesLeft = doom1_wad_len - doom1.offset;
    size_t bytesReq = size * count;
    size_t bytesRead = bytesLeft < bytesReq ? bytesLeft : bytesReq;

    int elems = bytesRead / size;
    bytesRead = size * elems;
    d_memcpy(dest, doom1_wad + doom1.offset, bytesRead);
    doom1.offset += bytesRead;
    return elems;
}

int d_fwrite(const void* src, size_t size, size_t count, FILE* file) { return -1; }
size_t d_ftell(FILE* file) { return doom1.offset; }
int d_remove( const char* fname ) { return -1; }
int d_rename(const char* old, const char* newname ) { return -1; }

int d_mkdir(const char *path)
{
    return -1;
}

const char *d_tmpdir()
{
    return "/";
}

