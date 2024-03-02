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
//  Refresh module, data I/O, caching, retrieval of graphics
//  by name.
//


#ifndef __R_DATA__
#define __R_DATA__

#include "r_defs.h"
#include "r_state.h"


// Retrieve column data for span blitting.
byte*
R_GetColumn
( struct doom_data_t_* doom, 
  int		tex,
  int		col );

struct doom_data_t_;


// I/O, setting up the stuff.
void R_InitData (struct doom_data_t_* doom);
void R_PrecacheLevel (struct doom_data_t_* doom);


// Retrieval.
// Floor/ceiling opaque texture tiles,
// lookup by name. For animation?
int R_FlatNumForName (struct doom_data_t_* doom, char* name);


// Called by P_Ticker for switches and animations,
// returns the texture number for the texture name.
int R_TextureNumForName (char *name);
int R_CheckTextureNumForName (char *name);

#endif
