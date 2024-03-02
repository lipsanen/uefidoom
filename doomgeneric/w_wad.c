//
// Copyright(C) 1993-1996 Id Software, Inc.
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
//	Handles WAD file header, directory, lump I/O.
//

#include "dlibc.h"
#include "doomtype.h"

#include "config.h"
#include "d_iwad.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_video.h"
#include "m_misc.h"
#include "z_zone.h"

#include "w_wad.h"

typedef struct
{
    // Should be "IWAD" or "PWAD".
    char identification[4];
    int numlumps;
    int infotableofs;
} PACKEDATTR wadinfo_t;

typedef struct
{
    int filepos;
    int size;
    char name[8];
} PACKEDATTR filelump_t;

// Hash function used for lump names.

unsigned int W_LumpNameHash(const char *s)
{
    // This is the djb2 string hash function, modded to work on strings
    // that have a maximum length of 8.

    unsigned int result = 5381;
    unsigned int i;

    for (i = 0; i < 8 && s[i] != '\0'; ++i)
    {
        result = ((result << 5) ^ result) ^ d_toupper((int)s[i]);
    }

    return result;
}

// Increase the size of the lumpinfo[] array to the specified size.
static void ExtendLumpInfo(doom_data_t* doom, int newnumlumps)
{
    lumpinfo_t *newlumpinfo;
    unsigned int i;
    size_t lumpsize = newnumlumps * sizeof(lumpinfo_t);

    newlumpinfo = Z_Malloc(lumpsize, PU_STATIC, NULL);
    d_memset(newlumpinfo, 0, lumpsize);

    if (newlumpinfo == NULL)
    {
        I_Error("Couldn't realloc lumpinfo");
    }

    // Copy over lumpinfo_t structures from the old array. If any of
    // these lumps have been cached, we need to update the user
    // pointers to the new location.
    for (i = 0; i < doom->numlumps && i < newnumlumps; ++i)
    {
        d_memcpy(&newlumpinfo[i], &doom->lumpinfo[i], sizeof(lumpinfo_t));

        if (newlumpinfo[i].cache != NULL)
        {
            Z_ChangeUser(newlumpinfo[i].cache, &newlumpinfo[i].cache);
        }

        // We shouldn't be generating a hash table until after all WADs have
        // been loaded, but just in case...
        if (doom->lumpinfo[i].next != NULL)
        {
            int nextlumpnum = doom->lumpinfo[i].next - doom->lumpinfo;
            newlumpinfo[i].next = &newlumpinfo[nextlumpnum];
        }
    }

    // All done.
    if (doom->lumpinfo != NULL)
        Z_Free(doom->lumpinfo);
    doom->lumpinfo = newlumpinfo;
    doom->numlumps = newnumlumps;
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.

wad_file_t *W_AddFile(doom_data_t* doom, const char *filename)
{
    wadinfo_t header;
    lumpinfo_t *lump_p;
    unsigned int i;
    wad_file_t *wad_file;
    int length;
    int startlump;
    filelump_t *fileinfo;
    filelump_t *filerover;
    int newnumlumps;

    // open the file and add to directory

    wad_file = W_OpenFile(doom, filename);

    if (wad_file == NULL)
    {
        d_printf(" couldn't open %s\n", filename);
        return NULL;
    }

    newnumlumps = doom->numlumps;

    if (d_stricmp(filename + d_strlen(filename) - 3, "wad"))
    {
        // single lump file

        // fraggle: Swap the filepos and size here.  The WAD directory
        // parsing code expects a little-endian directory, so will swap
        // them back.  Effectively we're constructing a "fake WAD directory"
        // here, as it would appear on disk.

        fileinfo = Z_Malloc(sizeof(filelump_t), PU_STATIC, 0);
        fileinfo->filepos = LONG(0);
        fileinfo->size = LONG(wad_file->length);

        // Name the lump after the base of the filename (without the
        // extension).

        M_ExtractFileBase(filename, fileinfo->name);
        newnumlumps++;
    }
    else
    {
        // WAD file
        W_Read(wad_file, 0, &header, sizeof(header));

        if (d_strncmp(header.identification, "IWAD", 4))
        {
            // Homebrew levels?
            if (d_strncmp(header.identification, "PWAD", 4))
            {
                I_Error("Wad file %s doesn't have IWAD "
                        "or PWAD id\n",
                        filename);
            }

            // ???modifiedgame = true;
        }

        header.numlumps = LONG(header.numlumps);
        header.infotableofs = LONG(header.infotableofs);
        length = header.numlumps * sizeof(filelump_t);
        fileinfo = Z_Malloc(length, PU_STATIC, 0);

        W_Read(wad_file, header.infotableofs, fileinfo, length);
        newnumlumps += header.numlumps;
    }

    // Increase size of numlumps array to accomodate the new file.
    startlump = doom->numlumps;
    ExtendLumpInfo(doom, newnumlumps);

    lump_p = &doom->lumpinfo[startlump];

    filerover = fileinfo;

    for (i = startlump; i < doom->numlumps; ++i)
    {
        lump_p->wad_file = wad_file;
        lump_p->position = LONG(filerover->filepos);
        lump_p->size = LONG(filerover->size);
        lump_p->cache = NULL;
        d_strncpy(lump_p->name, filerover->name, 8);

        ++lump_p;
        ++filerover;
    }

    Z_Free(fileinfo);

    if (doom->lumphash != NULL)
    {
        Z_Free(doom->lumphash);
        doom->lumphash = NULL;
    }

    return wad_file;
}

//
// W_NumLumps
//
int W_NumLumps(doom_data_t* doom)
{
    return doom->numlumps;
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName(doom_data_t* doom, const char *name)
{
    lumpinfo_t *lump_p;
    int i;

    // Do we have a hash table yet?

    if (doom->lumphash != NULL)
    {
        int hash;

        // We do! Excellent.

        hash = W_LumpNameHash(name) % doom->numlumps;

        for (lump_p = doom->lumphash[hash]; lump_p != NULL; lump_p = lump_p->next)
        {
            if (!d_strnicmp(lump_p->name, name, 8))
            {
                return lump_p - doom->lumpinfo;
            }
        }
    }
    else
    {
        // We don't have a hash table generate yet. Linear search :-(
        //
        // scan backwards so patch lump files take precedence

        for (i = doom->numlumps - 1; i >= 0; --i)
        {
            if (!d_strnicmp(doom->lumpinfo[i].name, name, 8))
            {
                return i;
            }
        }
    }

    // TFB. Not found.

    return -1;
}

//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName(doom_data_t* doom, const char *name)
{
    int i;

    i = W_CheckNumForName(doom, name);

    if (i < 0)
    {
        I_Error("W_GetNumForName: %s not found!", name);
    }

    return i;
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength(doom_data_t* doom, unsigned int lump)
{
    if (lump >= doom->numlumps)
    {
        I_Error("W_LumpLength: %i >= numlumps", lump);
    }

    return doom->lumpinfo[lump].size;
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(doom_data_t* doom, unsigned int lump, void *dest)
{
    int c;
    lumpinfo_t *l;

    if (lump >= doom->numlumps)
    {
        I_Error("W_ReadLump: %i >= numlumps", lump);
    }

    l = doom->lumpinfo + lump;

    I_BeginRead();

    c = W_Read(l->wad_file, l->position, dest, l->size);

    if (c < l->size)
    {
        I_Error("W_ReadLump: only read %i of %i on lump %i",
                c, l->size, lump);
    }

    I_EndRead();
}

//
// W_CacheLumpNum
//
// Load a lump into memory and return a pointer to a buffer containing
// the lump data.
//
// 'tag' is the type of zone memory buffer to allocate for the lump
// (usually PU_STATIC or PU_CACHE).  If the lump is loaded as
// PU_STATIC, it should be released back using W_ReleaseLumpNum
// when no longer needed (do not use Z_ChangeTag).
//

void *W_CacheLumpNum(doom_data_t* doom, int lumpnum, int tag)
{
    byte *result;
    lumpinfo_t *lump;

    if ((unsigned)lumpnum >= doom->numlumps)
    {
        I_Error("W_CacheLumpNum: %i >= numlumps", lumpnum);
    }

    lump = &doom->lumpinfo[lumpnum];

    // Get the pointer to return.  If the lump is in a memory-mapped
    // file, we can just return a pointer to within the memory-mapped
    // region.  If the lump is in an ordinary file, we may already
    // have it cached; otherwise, load it into memory.

    if (lump->wad_file->mapped != NULL)
    {
        // Memory mapped file, return from the mmapped region.

        result = lump->wad_file->mapped + lump->position;
    }
    else if (lump->cache != NULL)
    {
        // Already cached, so just switch the zone tag.

        result = lump->cache;
        Z_ChangeTag(lump->cache, tag);
    }
    else
    {
        // Not yet loaded, so load it now

        lump->cache = Z_Malloc(W_LumpLength(doom, lumpnum), tag, &lump->cache);
        W_ReadLump(doom, lumpnum, lump->cache);
        result = lump->cache;
    }

    return result;
}

//
// W_CacheLumpName
//
void *W_CacheLumpName(doom_data_t* doom, const char *name, int tag)
{
    return W_CacheLumpNum(doom, W_GetNumForName(doom, name), tag);
}

//
// Release a lump back to the cache, so that it can be reused later
// without having to read from disk again, or alternatively, discarded
// if we run out of memory.
//
// Back in Vanilla Doom, this was just done using Z_ChangeTag
// directly, but now that we have WAD mmap, things are a bit more
// complicated ...
//

void W_ReleaseLumpNum(doom_data_t* doom, int lumpnum)
{
    lumpinfo_t *lump;

    if ((unsigned)lumpnum >= doom->numlumps)
    {
        I_Error("W_ReleaseLumpNum: %i >= numlumps", lumpnum);
    }

    lump = &doom->lumpinfo[lumpnum];

    if (lump->wad_file->mapped != NULL)
    {
        // Memory-mapped file, so nothing needs to be done here.
    }
    else
    {
        Z_ChangeTag(lump->cache, PU_CACHE);
    }
}

void W_ReleaseLumpName(doom_data_t* doom, const char *name)
{
    W_ReleaseLumpNum(doom, W_GetNumForName(doom, name));
}

// Generate a hash table for fast lookups

void W_GenerateHashTable(doom_data_t* doom)
{
    unsigned int i;

    // Free the old hash table, if there is one

    if (doom->lumphash != NULL)
    {
        Z_Free(doom->lumphash);
    }

    // Generate hash table
    if (doom->numlumps > 0)
    {
        doom->lumphash = Z_Malloc(sizeof(lumpinfo_t *) * doom->numlumps, PU_STATIC, NULL);
        d_memset(doom->lumphash, 0, sizeof(lumpinfo_t *) * doom->numlumps);

        for (i = 0; i < doom->numlumps; ++i)
        {
            unsigned int hash;

            hash = W_LumpNameHash(doom->lumpinfo[i].name) % doom->numlumps;

            // Hook into the hash table

            doom->lumpinfo[i].next = doom->lumphash[hash];
            doom->lumphash[hash] = &doom->lumpinfo[i];
        }
    }

    // All done!
}

// Lump names that are unique to particular game types. This lets us check
// the user is not trying to play with the wrong executable, eg.
// chocolate-doom -iwad hexen.wad.
static const struct
{
    GameMission_t mission;
    char *lumpname;
} unique_lumps[] = {
    {doom1, "POSSA1"},
    {heretic, "IMPXA1"},
    {hexen, "ETTNA1"},
    {strife, "AGRDA1"},
};

void W_CheckCorrectIWAD(doom_data_t* doom, GameMission_t mission)
{
    int i;
    int lumpnum;

    for (i = 0; i < arrlen(unique_lumps); ++i)
    {
        if (mission != unique_lumps[i].mission)
        {
            lumpnum = W_CheckNumForName(doom, unique_lumps[i].lumpname);

            if (lumpnum >= 0)
            {
                I_Error("\nYou are trying to use a %s IWAD file with "
                        "the %s%s binary.\nThis isn't going to work.\n"
                        "You probably want to use the %s%s binary.",
                        D_SuggestGameName(unique_lumps[i].mission,
                                          indetermined),
                        PROGRAM_PREFIX,
                        D_GameMissionString(mission),
                        PROGRAM_PREFIX,
                        D_GameMissionString(unique_lumps[i].mission));
            }
        }
    }
}
