//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//      Miscellaneous.
//

#include "dlibc.h"

#include "doomtype.h"

#include "deh_str.h"

#include "i_swap.h"
#include "i_system.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

//
// Create a directory
//

void M_MakeDirectory(char *path)
{
    d_mkdir(path);
}

// Check if a file exists

boolean M_FileExists(char *filename)
{
    FILE *fstream;

    fstream = d_fopen(filename, "r");

    if (fstream != NULL)
    {
        d_fclose(fstream);
        return true;
    }
    else
    {
        return false;
    }
}

//
// Determine the length of an open file.
//

long M_FileLength(FILE *handle)
{
    long savedpos;
    long length;

    // save the current position in the file
    savedpos = d_ftell(handle);

    // jump to the end and find the length
    d_fseek(handle, 0, SEEK_END);
    length = d_ftell(handle);

    // go back to the old location
    d_fseek(handle, savedpos, SEEK_SET);

    return length;
}

//
// M_WriteFile
//

boolean M_WriteFile(char *name, void *source, int length)
{
    FILE *handle;
    int count;

    handle = d_fopen(name, "wb");

    if (handle == NULL)
        return false;

    count = d_fwrite(source, 1, length, handle);
    d_fclose(handle);

    if (count < length)
        return false;

    return true;
}

//
// M_ReadFile
//

int M_ReadFile(char *name, byte **buffer)
{
    FILE *handle;
    int count, length;
    byte *buf;

    handle = d_fopen(name, "rb");
    if (handle == NULL)
        I_Error("Couldn't read file %s", name);

    // find the size of the file by seeking to the end and
    // reading the current position

    length = M_FileLength(handle);

    buf = Z_Malloc(length, PU_STATIC, NULL);
    count = d_fread(buf, 1, length, handle);
    d_fclose(handle);

    if (count < length)
        I_Error("Couldn't read file %s", name);

    *buffer = buf;
    return length;
}

// Returns the path to a temporary file of the given name, stored
// inside the system temporary directory.
//
// The returned value must be freed with Z_Free after use.

char *M_TempFile(char *s)
{
    return M_StringJoin(d_tmpdir(), DIR_SEPARATOR_S, s, NULL);
}

boolean M_StrToInt(const char *str, int *result)
{
    return d_sscanf(str, " 0x%x", result) == 1 || d_sscanf(str, " 0X%x", result) == 1 || d_sscanf(str, " 0%o", result) == 1 || d_sscanf(str, " %d", result) == 1;
}

void M_ExtractFileBase(const char *path, char *dest)
{
    const char *src;
    const char *filename;
    int length;

    src = path + d_strlen(path) - 1;

    // back up until a \ or the start
    while (src != path && *(src - 1) != DIR_SEPARATOR)
    {
        src--;
    }

    filename = src;

    // Copy up to eight characters
    // Note: Vanilla Doom exits with an error if a filename is specified
    // with a base of more than eight characters.  To remove the 8.3
    // filename limit, instead we simply truncate the name.

    length = 0;
    d_memset(dest, 0, 8);

    while (*src != '\0' && *src != '.')
    {
        if (length >= 8)
        {
            d_printf("Warning: Truncated '%s' lump name to '%.8s'.\n",
                     filename, dest);
            break;
        }

        dest[length++] = d_toupper((int)*src++);
    }
}

//---------------------------------------------------------------------------
//
// PROC M_ForceUppercase
//
// Change string to uppercase.
//
//---------------------------------------------------------------------------

void M_ForceUppercase(char *text)
{
    char *p;

    for (p = text; *p != '\0'; ++p)
    {
        *p = d_toupper(*p);
    }
}

//
// M_StrCaseStr
//
// Case-insensitive version of strstr()
//

char *M_StrCaseStr(char *haystack, char *needle)
{
    unsigned int haystack_len;
    unsigned int needle_len;
    unsigned int len;
    unsigned int i;

    haystack_len = d_strlen(haystack);
    needle_len = d_strlen(needle);

    if (haystack_len < needle_len)
    {
        return NULL;
    }

    len = haystack_len - needle_len;

    for (i = 0; i <= len; ++i)
    {
        if (!d_strnicmp(haystack + i, needle, needle_len))
        {
            return haystack + i;
        }
    }

    return NULL;
}

//
// String replace function.
//

char *M_StringReplace(const char *haystack, const char *needle,
                      const char *replacement)
{
    char *result, *dst;
    const char *p;
    size_t needle_len = d_strlen(needle);
    size_t result_len, dst_len;

    // Iterate through occurrences of 'needle' and calculate the size of
    // the new string.
    result_len = d_strlen(haystack) + 1;
    p = haystack;

    for (;;)
    {
        p = d_strstr(p, needle);
        if (p == NULL)
        {
            break;
        }

        p += needle_len;
        result_len += d_strlen(replacement) - needle_len;
    }

    // Construct new string.

    result = Z_Malloc(result_len, PU_STATIC, NULL);
    if (result == NULL)
    {
        I_Error("M_StringReplace: Failed to allocate new string");
        return NULL;
    }

    dst = result;
    dst_len = result_len;
    p = haystack;

    while (*p != '\0')
    {
        if (!d_strncmp(p, needle, needle_len))
        {
            M_StringCopy(dst, replacement, dst_len);
            p += needle_len;
            dst += d_strlen(replacement);
            dst_len -= d_strlen(replacement);
        }
        else
        {
            *dst = *p;
            ++dst;
            --dst_len;
            ++p;
        }
    }

    *dst = '\0';

    return result;
}

// Safe string copy function that works like OpenBSD's strlcpy().
// Returns true if the string was not truncated.

boolean M_StringCopy(char *dest, const char *src, size_t dest_size)
{
    size_t len;

    if (dest_size >= 1)
    {
        dest[dest_size - 1] = '\0';
        d_strncpy(dest, src, dest_size - 1);
    }
    else
    {
        return false;
    }

    len = d_strlen(dest);
    return src[len] == '\0';
}

// Safe string concat function that works like OpenBSD's strlcat().
// Returns true if string not truncated.

boolean M_StringConcat(char *dest, const char *src, size_t dest_size)
{
    size_t offset;

    offset = d_strlen(dest);
    if (offset > dest_size)
    {
        offset = dest_size;
    }

    return M_StringCopy(dest + offset, src, dest_size - offset);
}

// Returns true if 's' begins with the specified prefix.

boolean M_StringStartsWith(const char *s, const char *prefix)
{
    return d_strlen(s) > d_strlen(prefix) && d_strncmp(s, prefix, d_strlen(prefix)) == 0;
}

// Returns true if 's' ends with the specified suffix.

boolean M_StringEndsWith(const char *s, const char *suffix)
{
    return d_strlen(s) >= d_strlen(suffix) && d_strcmp(s + d_strlen(s) - d_strlen(suffix), suffix) == 0;
}

// Return a newly-malloced string with all the strings given as arguments
// concatenated together.

char *M_StringJoin(const char *s, ...)
{
    char *result;
    const char *v;
    va_list args;
    size_t result_len;

    result_len = d_strlen(s) + 1;

    va_start(args, s);
    for (;;)
    {
        v = va_arg(args, const char *);
        if (v == NULL)
        {
            break;
        }

        result_len += d_strlen(v);
    }
    va_end(args);

    result = Z_Malloc(result_len, PU_STATIC, NULL);

    if (result == NULL)
    {
        I_Error("M_StringJoin: Failed to allocate new string.");
        return NULL;
    }

    M_StringCopy(result, s, result_len);

    va_start(args, s);
    for (;;)
    {
        v = va_arg(args, const char *);
        if (v == NULL)
        {
            break;
        }

        M_StringConcat(result, v, result_len);
    }
    va_end(args);

    return result;
}
