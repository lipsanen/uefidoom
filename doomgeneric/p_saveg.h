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
//	Savegame I/O, archiving, persistence.
//


#ifndef __P_SAVEG__
#define __P_SAVEG__

#include "dlibc.h"

// maximum size of a savegame description

#define SAVESTRINGSIZE 24

struct doom_data_t_;

// temporary filename to use while saving.

char *P_TempSaveGameFile(struct doom_data_t_* doom);

// filename to use for a savegame slot

char *P_SaveGameFile(struct doom_data_t_* doom, int slot);

// Savegame file header read/write functions

boolean P_ReadSaveGameHeader(doom_data_t* doom);
void P_WriteSaveGameHeader(doom_data_t* doom, char *description);

// Savegame end-of-file read/write functions

boolean P_ReadSaveGameEOF(void);
void P_WriteSaveGameEOF(void);

// Persistent storage/archiving.
// These are the load / save game routines.
void P_ArchivePlayers (void);
void P_UnArchivePlayers (void);
void P_ArchiveWorld (void);
void P_UnArchiveWorld (void);
void P_ArchiveThinkers (void);
void P_UnArchiveThinkers (void);
void P_ArchiveSpecials (void);
void P_UnArchiveSpecials (void);

extern FILE *save_stream;
extern boolean savegame_error;


#endif
