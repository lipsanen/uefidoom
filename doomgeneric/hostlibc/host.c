#include "dlibc.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

int d_fseek(FILE *stream, long offset, int origin ) { return fseek(stream, offset, origin); }
int d_fclose(FILE* file) { return fclose(file); }
int d_fread(void* dest, size_t size, size_t count, FILE* file) { return fread(dest, size, count, file); }
int d_fwrite(const void* src, size_t size, size_t count, FILE* file) { return fwrite(src, size, count, file); }
FILE* d_fopen(const char* filepath, const char* modes) { return fopen(filepath, modes); }
size_t d_ftell(FILE* file) { return ftell(file); }
int d_putchar(int c) { return putchar(c); }
int d_remove( const char* fname ) { return remove(fname); }
int d_rename(const char* old, const char* newname ) { return rename(old, newname); }
