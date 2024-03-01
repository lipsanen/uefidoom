//
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
//     Search for and locate an IWAD file, and initialize according
//     to the IWAD type.
//

#include "dlibc.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

const char *D_FindIWAD()
{
    static char* THE_filename = (char*)"doom1.wad";

    if (M_FileExists(THE_filename))
    {
        return THE_filename;
    }

    return NULL;
}

//
// Get the IWAD name used for savegames.
//

char *D_SaveGameIWADName(GameMission_t gamemission)
{
    return "doom1.wad";
}

char *D_SuggestIWADName(GameMission_t mission, GameMode_t mode)
{
    return "doom1.wad";
}

char *D_SuggestGameName(GameMission_t mission, GameMode_t mode)
{
    return "Doom";
}

