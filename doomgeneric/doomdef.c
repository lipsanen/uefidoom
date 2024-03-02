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

// scale on entry
#define INITSCALEMTOF (.2*FRACUNIT)

static const cheatseq_t cheat_amap = CHEAT("iddt", 0);
static const cheatseq_t cheat_mus = CHEAT("idmus", 2);
static const cheatseq_t cheat_god = CHEAT("iddqd", 0);
static const cheatseq_t cheat_ammo = CHEAT("idkfa", 0);
static const cheatseq_t cheat_ammonokey = CHEAT("idfa", 0);
static const cheatseq_t cheat_noclip = CHEAT("idspispopd", 0);
static const cheatseq_t cheat_commercial_noclip = CHEAT("idclip", 0);

static const cheatseq_t cheat_powerup[7] =
    {
        CHEAT("idbeholdv", 0),
        CHEAT("idbeholds", 0),
        CHEAT("idbeholdi", 0),
        CHEAT("idbeholdr", 0),
        CHEAT("idbeholda", 0),
        CHEAT("idbeholdl", 0),
        CHEAT("idbehold", 0),
};

static const cheatseq_t cheat_choppers = CHEAT("idchoppers", 0);
static const cheatseq_t cheat_clev = CHEAT("idclev", 2);
static const cheatseq_t cheat_mypos = CHEAT("idmypos", 0);

void doomdata_init(doom_data_t* doom) {
    d_memset(doom, 0, sizeof(doom_data_t));

#define COPY_VAR(var) d_memcpy(&doom->var, &var, sizeof(var));

    COPY_VAR(cheat_amap);
    COPY_VAR(cheat_mus);
    COPY_VAR(cheat_god);
    COPY_VAR(cheat_ammo);
    COPY_VAR(cheat_ammonokey);
    COPY_VAR(cheat_noclip);
    COPY_VAR(cheat_commercial_noclip);
    COPY_VAR(cheat_powerup);
    COPY_VAR(cheat_choppers);
    COPY_VAR(cheat_clev);
    COPY_VAR(cheat_mypos);

    doom->leveljuststarted = 1;
    doom->markpointnum = 0; // next point to be assigned
    doom->followplayer = 1; // specifies whether to follow the player around
    doom->stopped = true;
    doom->scale_mtof = (fixed_t)INITSCALEMTOF;
    doom->new_sync = true;
    doom->ticdup = 1;
    doom->show_endoom = 1;
    doom->wipegamestate = GS_DEMOSCREEN;
    doom->oldgamestate = -1;

    // Game Mode - identify IWAD as shareware, retail etc.
    doom->gamemode = indetermined;
    doom->gamemission = doom1;
    doom->gameversion = exe_final2;
    doom->lastepisode = -1;
    doom->lastlevel = -1;
    doom->st_stopped = true;
    doom->st_oldhealth = -1;
    doom->largeammo = 1994; // means "n/a"
    doom->lastattackdown = -1;
    doom->oldhealth = -1;
}

// Location for any defines turned variables.

// None.


