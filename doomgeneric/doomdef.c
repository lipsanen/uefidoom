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
//  DoomDef - basic defines for DOOM, e.g. Version, game mode
//   and skill level, and display parameters.
//

#include "dlibc.h"
#include "doomdef.h"

void doomdata_init(doom_data_t* doom) {
    d_strcpy(doom->cheat_amap.sequence, "iddt");
    doom->cheat_amap.sequence_len = d_strlen(doom->cheat_amap.sequence);
    d_strcpy(doom->cheat_amap.parameter_buf, "");
    doom->cheat_amap.chars_read = 0;
    doom->cheat_amap.parameter_chars = 0;
    doom->cheat_amap.param_chars_read = 0;
}

// Location for any defines turned variables.

// None.


