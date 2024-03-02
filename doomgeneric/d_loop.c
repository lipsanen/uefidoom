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
//     Main loop code.
//

#include "dlibc.h"
#include "d_loop.h"
#include <stddef.h>
#include "doomfeatures.h"

#include "d_event.h"
#include "d_loop.h"
#include "d_ticcmd.h"

#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "m_argv.h"
#include "m_fixed.h"

#include "net_client.h"
#include "net_gui.h"
#include "net_io.h"
#include "net_query.h"
#include "net_server.h"
#include "net_sdl.h"
#include "net_loop.h"

static boolean BuildNewTic(struct doom_data_t_ *doom)
{
    int gameticdiv;
    ticcmd_t cmd;

    gameticdiv = doom->gametic / doom->ticdup;

    I_StartTic(doom);
    doom->loop_interface->ProcessEvents(doom);

    // Always run the menu

    doom->loop_interface->RunMenu();

    if (drone)
    {
        // In drone mode, do not generate any ticcmds.

        return false;
    }

    if (doom->new_sync)
    {
        // If playing single player, do not allow tics to buffer
        // up very far

        if (!net_client_connected && doom->maketic - gameticdiv > 2)
            return false;

        // Never go more than ~200ms ahead

        if (doom->maketic - gameticdiv > 8)
            return false;
    }
    else
    {
        if (doom->maketic - gameticdiv >= 5)
            return false;
    }

    // d_printf ("mk:%i ",maketic);
    d_memset(&cmd, 0, sizeof(ticcmd_t));
    doom->loop_interface->BuildTiccmd(doom, &cmd, doom->maketic);

#ifdef FEATURE_MULTIPLAYER

    if (net_client_connected)
    {
        NET_CL_SendTiccmd(&cmd, maketic);
    }

#endif
    doom->ticdata[doom->maketic % BACKUPTICS].cmds[doom->localplayer] = cmd;
    doom->ticdata[doom->maketic % BACKUPTICS].ingame[doom->localplayer] = true;

    ++doom->maketic;

    return true;
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//

void NetUpdate(struct doom_data_t_ *doom)
{
    BuildNewTic(doom);
}

static void D_Disconnected(void)
{
    // In drone mode, the game cannot continue once disconnected.

    if (drone)
    {
        I_Error("Disconnected from server in drone mode.");
    }

    // disconnected from server

    d_printf("Disconnected from server.\n");
}

//
// Invoked by the network engine when a complete set of ticcmds is
// available.
//

void D_ReceiveTic(doom_data_t *doom, ticcmd_t *ticcmds, boolean *players_mask)
{
    int i;

    // Disconnected from server?

    if (ticcmds == NULL && players_mask == NULL)
    {
        D_Disconnected();
        return;
    }

    for (i = 0; i < NET_MAXPLAYERS; ++i)
    {
        if (!drone && i == doom->localplayer)
        {
            // This is us.  Don't overwrite it.
        }
        else
        {
            doom->ticdata[doom->recvtic % BACKUPTICS].cmds[i] = ticcmds[i];
            doom->ticdata[doom->recvtic % BACKUPTICS].ingame[i] = players_mask[i];
        }
    }

    ++doom->recvtic;
}

//
// Start game loop
//
// Called after the screen is set but before the game starts running.
//

void D_StartGameLoop(void)
{
}

void D_StartNetGame(doom_data_t *doom, net_gamesettings_t *settings,
                    netgame_startup_callback_t callback)
{
    settings->consoleplayer = 0;
    settings->num_players = 1;
    settings->player_classes[0] = doom->player_class;
    settings->new_sync = 0;
    settings->extratics = 1;
    settings->ticdup = 1;

    doom->ticdup = settings->ticdup;
    doom->new_sync = settings->new_sync;
}

boolean D_InitNetGame(doom_data_t *doom, net_connect_data_t *connect_data)
{
    boolean result = false;
#ifdef FEATURE_MULTIPLAYER
    net_addr_t *addr = NULL;
    int i;
#endif

    // Call D_QuitNetGame on exit:

    I_AtExit(D_QuitNetGame, true);

    doom->player_class = connect_data->player_class;

#ifdef FEATURE_MULTIPLAYER

    //!
    // @category net
    //
    // Start a multiplayer server, listening for connections.
    //

    if (M_CheckParm("-server") > 0 || M_CheckParm("-privateserver") > 0)
    {
        NET_SV_Init();
        NET_SV_AddModule(&net_loop_server_module);
        NET_SV_AddModule(&net_sdl_module);
        NET_SV_RegisterWithMaster();

        net_loop_client_module.InitClient();
        addr = net_loop_client_module.ResolveAddress(NULL);
    }
    else
    {
        //!
        // @category net
        //
        // Automatically search the local LAN for a multiplayer
        // server and join it.
        //

        i = M_CheckParm("-autojoin");

        if (i > 0)
        {
            addr = NET_FindLANServer();

            if (addr == NULL)
            {
                I_Error("No server found on local LAN");
            }
        }

        //!
        // @arg <address>
        // @category net
        //
        // Connect to a multiplayer server running on the given
        // address.
        //

        i = M_CheckParmWithArgs("-connect", 1);

        if (i > 0)
        {
            net_sdl_module.InitClient();
            addr = net_sdl_module.ResolveAddress(myargv[i + 1]);

            if (addr == NULL)
            {
                I_Error("Unable to resolve '%s'\n", myargv[i + 1]);
            }
        }
    }

    if (addr != NULL)
    {
        if (M_CheckParm("-drone") > 0)
        {
            connect_data->drone = true;
        }

        if (!NET_CL_Connect(addr, connect_data))
        {
            I_Error("D_InitNetGame: Failed to connect to %s\n",
                    NET_AddrToString(addr));
        }

        d_printf("D_InitNetGame: Connected to %s\n", NET_AddrToString(addr));

        // Wait for launch message received from server.

        NET_WaitForLaunch();

        result = true;
    }
#endif

    return result;
}

//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame(doom_data_t *doom)
{
#ifdef FEATURE_MULTIPLAYER
    NET_SV_Shutdown();
    NET_CL_Disconnect();
#endif
}

static int GetLowTic(doom_data_t *doom)
{
    int lowtic;

    lowtic = doom->maketic;

#ifdef FEATURE_MULTIPLAYER
    if (net_client_connected)
    {
        if (drone || recvtic < lowtic)
        {
            lowtic = recvtic;
        }
    }
#endif

    return lowtic;
}

static int frameon;
static int frameskip[4];
static int oldnettics;

static void OldNetSync(doom_data_t *doom)
{
    unsigned int i;
    int keyplayer = -1;

    frameon++;

    // ideally maketic should be 1 - 3 tics above lowtic
    // if we are consistantly slower, speed up time

    for (i = 0; i < NET_MAXPLAYERS; i++)
    {
        if (doom->local_playeringame[i])
        {
            keyplayer = i;
            break;
        }
    }

    if (keyplayer < 0)
    {
        // If there are no players, we can never advance anyway

        return;
    }

    if (doom->localplayer == keyplayer)
    {
        // the key player does not adapt
    }
    else
    {
        if (doom->maketic <= doom->recvtic)
        {
            // d_printf ("-");
        }

        frameskip[frameon & 3] = oldnettics > doom->recvtic;
        oldnettics = doom->maketic;

        if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3])
        {
            doom->skiptics = 1;
            // d_printf ("+");
        }
    }
}

// Returns true if there are players in the game:

static boolean PlayersInGame(doom_data_t *doom)
{
    boolean result = false;
    unsigned int i;

    // If we are connected to a server, check if there are any players
    // in the game.

    if (net_client_connected)
    {
        for (i = 0; i < NET_MAXPLAYERS; ++i)
        {
            result = result || doom->local_playeringame[i];
        }
    }

    // Whether single or multi-player, unless we are running as a drone,
    // we are in the game.

    if (!drone)
    {
        result = true;
    }

    return result;
}

// When using ticdup, certain values must be cleared out when running
// the duplicate ticcmds.

static void TicdupSquash(ticcmd_set_t *set)
{
    ticcmd_t *cmd;
    unsigned int i;

    for (i = 0; i < NET_MAXPLAYERS; ++i)
    {
        cmd = &set->cmds[i];
        cmd->chatchar = 0;
        if (cmd->buttons & BT_SPECIAL)
            cmd->buttons = 0;
    }
}

// When running in single player mode, clear all the ingame[] array
// except the local player.

static void SinglePlayerClear(doom_data_t *doom, ticcmd_set_t *set)
{
    unsigned int i;

    for (i = 0; i < NET_MAXPLAYERS; ++i)
    {
        if (i != doom->localplayer)
        {
            set->ingame[i] = false;
        }
    }
}

//
// TryRunTics
//

void TryRunTics(struct doom_data_t_ *doom)
{
    int i;
    int counts;

    BuildNewTic(doom);
    ticcmd_set_t *set;

    if (!PlayersInGame(doom))
    {
        return;
    }

    set = &doom->ticdata[(doom->gametic / doom->ticdup) % BACKUPTICS];

    if (!net_client_connected)
    {
        SinglePlayerClear(doom, set);
    }

    for (i = 0; i < doom->ticdup; i++)
    {

        d_memcpy(doom->local_playeringame, set->ingame, sizeof(doom->local_playeringame));

        doom->loop_interface->RunTic(doom, set->cmds, set->ingame);
        doom->gametic++;

        // modify command for duplicated tics

        TicdupSquash(set);
    }

    NetUpdate(doom); // check for new console commands
}

void D_RegisterLoopCallbacks(doom_data_t *doom, loop_interface_t *i)
{
    doom->loop_interface = i;
}
