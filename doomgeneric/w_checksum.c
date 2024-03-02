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
//       Generate a checksum of the WAD directory.
//

#include "doomdef.h"
#include "z_zone.h"
#include "m_misc.h"
#include "sha1.h"
#include "w_checksum.h"
#include "w_wad.h"

static int GetFileNumber(doom_data_t* doom, wad_file_t *handle)
{
    int i;
    int result;

    for (i = 0; i < doom->num_open_wadfiles; ++i)
    {
        if (doom->open_wadfiles[i] == handle)
        {
            return i;
        }
    }

    // Not found in list.  This is a new file we haven't seen yet.
    // Allocate another slot for this file.

    void *new_wadfiles = Z_Malloc(sizeof(wad_file_t *) * (doom->num_open_wadfiles + 1), PU_STATIC, NULL);
    if (doom->open_wadfiles)
    {
        d_memcpy(new_wadfiles, doom->open_wadfiles, sizeof(wad_file_t *) * (doom->num_open_wadfiles));
        Z_Free(doom->open_wadfiles);
    }

    doom->open_wadfiles = new_wadfiles;
    doom->open_wadfiles[doom->num_open_wadfiles] = handle;

    result = doom->num_open_wadfiles;
    ++doom->num_open_wadfiles;

    return result;
}

static void ChecksumAddLump(doom_data_t* doom, sha1_context_t *sha1_context, lumpinfo_t *lump)
{
    char buf[9];

    M_StringCopy(buf, lump->name, sizeof(buf));
    SHA1_UpdateString(sha1_context, buf);
    SHA1_UpdateInt32(sha1_context, GetFileNumber(doom, lump->wad_file));
    SHA1_UpdateInt32(sha1_context, lump->position);
    SHA1_UpdateInt32(sha1_context, lump->size);
}

void W_Checksum(struct doom_data_t_* doom, sha1_digest_t digest)
{
    sha1_context_t sha1_context;
    unsigned int i;

    SHA1_Init(&sha1_context);

    doom->num_open_wadfiles = 0;

    // Go through each entry in the WAD directory, adding information
    // about each entry to the SHA1 hash.

    for (i = 0; i < doom->numlumps; ++i)
    {
        ChecksumAddLump(doom, &sha1_context, &doom->lumpinfo[i]);
    }

    SHA1_Final(digest, &sha1_context);
}
