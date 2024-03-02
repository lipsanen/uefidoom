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
//	WAD I/O functions.
//


#ifndef __W_WAD__
#define __W_WAD__

#include "doomtype.h"
#include "d_mode.h"

#include "w_file.h"


//
// TYPES
//

//
// WADFILE I/O related stuff.
//

typedef struct lumpinfo_s lumpinfo_t;

struct lumpinfo_s
{
    char	name[8];
    wad_file_t *wad_file;
    int		position;
    int		size;
    void       *cache;

    // Used for hash table lookups

    lumpinfo_t *next;
};

struct doom_data_t_;

wad_file_t *W_AddFile (struct doom_data_t_* doom, const char *filename);

int	W_CheckNumForName (struct doom_data_t_* doom, const char* name);
int	W_GetNumForName (struct doom_data_t_* doom, const char* name);

int	W_LumpLength (struct doom_data_t_* doom, unsigned int lump);
void    W_ReadLump (struct doom_data_t_* doom, unsigned int lump, void *dest);

void*	W_CacheLumpNum (struct doom_data_t_* doom, int lump, int tag);
void*	W_CacheLumpName (struct doom_data_t_* doom, const char* name, int tag);

void    W_GenerateHashTable(struct doom_data_t_* doom);

extern unsigned int W_LumpNameHash(const char *s);

void    W_ReleaseLumpNum(struct doom_data_t_* doom, int lump);
void    W_ReleaseLumpName(struct doom_data_t_* doom, const char *name);

struct doom_data_t_;

void W_CheckCorrectIWAD(struct doom_data_t_* doom, GameMission_t mission);

#endif
