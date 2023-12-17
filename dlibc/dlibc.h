#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// Replacements for libc functions
int d_atoi (const char *__nptr);
int d_abs(int __x);
int d_isdigit(int c);
int d_isspace(char c);
void* d_memset( void* dest, int ch, size_t count );
void* d_memcpy( void* dest, const void* src, size_t count );
void* d_memmove( void* dest, const void* src, size_t count );
size_t d_strlen(const char* str);
int d_remove( const char* fname );
int d_rename(const char* old, const char* newname );
int d_sprintf(char* buffer, const char* format, ...);
int d_snprintf(char* buffer, size_t buffer_size, const char* format, ...);
int d_strcmp(const char* str1, const char* str2);
int d_strncmp(const char* str1, const char* str2, size_t size);
int d_stricmp(const char* str1, const char* str2);
int d_strnicmp(const char* str1, const char* str2, size_t size);
int d_vprintf(const char* format, va_list args);
int d_vsnprintf(char* buf, size_t size, const char* format, va_list args);
char *d_strchr( const char *str, int ch );
int d_putchar(int c);
int d_mkdir(const char* path);
void d_strcpy(char* dest, const char* src);
char* d_strncpy(char* dest, const char* src, size_t size);
char* d_strstr(const char* haystack, const char* needle);
int d_file_exists(const char* path);
int d_islower(int c);
void* d_realloc(void* ptr, size_t newsize);
void* d_memchr(const void *src, int c, size_t n);

struct _IO_FILE;
typedef struct _IO_FILE FILE;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int d_fclose(FILE* file);
int d_fprintf(FILE* file, const char* format, ...);
int d_fread(void* dest, size_t size, size_t count, FILE* file);
int d_fseek(FILE *stream, long offset, int origin );
int d_fwrite(const void* src, size_t size, size_t count, FILE* file);
FILE* d_fopen(const char* filepath, const char* modes);
size_t d_ftell(FILE* file);
int d_printf(const char* format, ...);
int d_tolower (int c);
int d_toupper (int c);

int d_sscanf(const char* src, const char* format, ...);
int d_vsscanf(const char* src, const char* fmt, va_list ap);

const char* d_tmpdir();
typedef int (*d_cmpfun)(const void *, const void *);
void d_qsort(void *base, size_t nel, size_t width, d_cmpfun cmp);
// Returns 1 if wider than 16:10, 0 if 16:10 and -1 if taller than 16:10
int d_aspect_ratio_comparison(uint32_t width, uint32_t height);
void d_get_screen_params(uint32_t width, uint32_t height, uint32_t* skip_x, uint32_t* skip_y);

#ifdef __cplusplus
}
#endif
