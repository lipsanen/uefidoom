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
//	DOOM Network game communication and protocol,
//	all OS independend parts.
//

#include "dlibc.h"
#include "doomfeatures.h"

#include "d_main.h"
#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "w_checksum.h"
#include "w_wad.h"

#include "deh_main.h"

#include "d_loop.h"

// Called when a player leaves the game

static void PlayerQuitGame(doom_data_t* doom, player_t *player)
{
    unsigned int player_num;

    player_num = player - doom->players;

    // Do this the same way as Vanilla Doom does, to allow dehacked
    // replacements of this message

    M_StringCopy(doom->exitmsg, DEH_String("Player 1 left the game"),
                 sizeof(doom->exitmsg));

    doom->exitmsg[7] += player_num;

    doom->playeringame[player_num] = false;
    doom->players[doom->consoleplayer].message = doom->exitmsg;

    // TODO: check if it is sensible to do this:

    if (doom->demorecording) 
    {
        G_CheckDemoStatus (doom);
    }
}

static void RunTic(doom_data_t* doom, ticcmd_t *cmds, boolean *ingame)
{
    unsigned int i;

    // Check for player quits.

    for (i = 0; i < MAXPLAYERS; ++i)
    {
        if (!doom->demoplayback && doom->playeringame[i] && !ingame[i])
        {
            PlayerQuitGame(doom, &doom->players[i]);
        }
    }

    doom->netcmds = cmds;

    // check that there are players in the game.  if not, we cannot
    // run a tic.

    if (doom->advancedemo)
        D_DoAdvanceDemo (doom);

    G_Ticker (doom);
}

static loop_interface_t doom_loop_interface = {
    D_ProcessEvents,
    G_BuildTiccmd,
    RunTic,
    M_Ticker
};


// Load game settings from the specified structure and
// set global variables.

static void LoadGameSettings(doom_data_t* doom, net_gamesettings_t *settings)
{
    unsigned int i;

    doom->deathmatch = settings->deathmatch;
    doom->startepisode = settings->episode;
    doom->startmap = settings->map;
    doom->startskill = settings->skill;
    doom->startloadgame = settings->loadgame;
    doom->lowres_turn = settings->lowres_turn;
    doom->nomonsters = settings->nomonsters;
    doom->fastparm = settings->fast_monsters;
    doom->respawnparm = settings->respawn_monsters;
    doom->timelimit = settings->timelimit;
    doom->consoleplayer = settings->consoleplayer;

    if (doom->lowres_turn)
    {
        d_printf("NOTE: Turning resolution is reduced; this is probably "
               "because there is a client recording a Vanilla demo.\n");
    }

    for (i = 0; i < MAXPLAYERS; ++i)
    {
        doom->playeringame[i] = i < settings->num_players;
    }
}

// Save the game settings from global variables to the specified
// game settings structure.

static void SaveGameSettings(doom_data_t* doom, net_gamesettings_t *settings)
{
    // Fill in game settings structure with appropriate parameters
    // for the new game

    settings->deathmatch = doom->deathmatch;
    settings->episode = doom->startepisode;
    settings->map = doom->startmap;
    settings->skill = doom->startskill;
    settings->loadgame = doom->startloadgame;
    settings->gameversion = doom->gameversion;
    settings->nomonsters = doom->nomonsters;
    settings->fast_monsters = doom->fastparm;
    settings->respawn_monsters = doom->respawnparm;
    settings->timelimit = doom->timelimit;

    settings->lowres_turn = M_CheckParm(doom, "-record") > 0
                         && M_CheckParm(doom, "-longtics") == 0;
}

static void InitConnectData(doom_data_t* doom, net_connect_data_t *connect_data)
{
    connect_data->max_players = MAXPLAYERS;
    connect_data->drone = false;

    //!
    // @category net
    //
    // Run as the left screen in three screen mode.
    //

    if (M_CheckParm(doom, "-left") > 0)
    {
        viewangleoffset = ANG90;
        connect_data->drone = true;
    }

    //! 
    // @category net
    //
    // Run as the right screen in three screen mode.
    //

    if (M_CheckParm(doom, "-right") > 0)
    {
        viewangleoffset = ANG270;
        connect_data->drone = true;
    }

    //
    // Connect data
    //

    // Game type fields:

    connect_data->gamemode = doom->gamemode;
    connect_data->gamemission = doom->gamemission;

    // Are we recording a demo? Possibly set lowres turn mode

    connect_data->lowres_turn = M_CheckParm(doom, "-record") > 0
                             && M_CheckParm(doom, "-longtics") == 0;

    // Read checksums of our WAD directory and dehacked information

    W_Checksum(doom, connect_data->wad_sha1sum);

    // Are we playing with the Freedoom IWAD?

    connect_data->is_freedoom = W_CheckNumForName(doom, "FREEDOOM") >= 0;
}

void D_ConnectNetGame(struct doom_data_t_* doom)
{
    net_connect_data_t connect_data;

    InitConnectData(doom, &connect_data);
    doom->netgame = D_InitNetGame(doom, &connect_data);

    //!
    // @category net
    //
    // Start the game playing as though in a netgame with a single
    // player.  This can also be used to play back single player netgame
    // demos.
    //

    if (M_CheckParm(doom, "-solo-net") > 0)
    {
        doom->netgame = true;
    }
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
void D_CheckNetGame (struct doom_data_t_* doom)
{
    net_gamesettings_t settings;

    if (doom->netgame)
    {
        doom->autostart = true;
    }

    D_RegisterLoopCallbacks(doom, &doom_loop_interface);

    SaveGameSettings(doom, &settings);
    D_StartNetGame(doom, &settings, NULL);
    LoadGameSettings(doom, &settings);

    d_printf("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
               doom->startskill, doom->deathmatch, doom->startmap, doom->startepisode);

    d_printf("player %i of %i (%i nodes)\n",
               doom->consoleplayer+1, settings.num_players, settings.num_players);

    // Show players here; the server might have specified a time limit

    if (doom->timelimit > 0 && doom->deathmatch)
    {
        // Gross hack to work like Vanilla:

        if (doom->timelimit == 20 && M_CheckParm(doom, "-avg"))
        {
            d_printf("Austin Virtual Gaming: Levels will end "
                           "after 20 minutes\n");
        }
        else
        {
            d_printf("Levels will end after %d minute", doom->timelimit);
            if (doom->timelimit > 1)
                d_printf("s");
            d_printf(".\n");
        }
    }
}

