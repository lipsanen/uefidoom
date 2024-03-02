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
//   Duh.
// 


#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "d_event.h"
#include "d_ticcmd.h"


//
// GAME
//
void G_DeathMatchSpawnPlayer (doom_data_t* doom, int playernum);

void G_InitNew (doom_data_t* doom, skill_t skill, int episode, int map);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew (doom_data_t* doom, skill_t skill, int episode, int map);

void G_DeferedPlayDemo (struct doom_data_t_* doom, char* demo);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame (doom_data_t* doom, char* name);

void G_DoLoadGame (doom_data_t* doom);

// Called by M_Responder.
void G_SaveGame (struct doom_data_t_* doom, int slot, char* description);

// Only called by startup code.
void G_RecordDemo (struct doom_data_t_* doom, char* name);

void G_BeginRecording (doom_data_t* doom);

void G_PlayDemo (char* name);
void G_TimeDemo (doom_data_t* doom, char* name);
boolean G_CheckDemoStatus (doom_data_t* doom);

void G_ExitLevel (doom_data_t* doom);
void G_SecretExitLevel (doom_data_t* doom);

void G_WorldDone (doom_data_t* doom);

// Read current data from inputs and build a player movement command.

void G_BuildTiccmd (doom_data_t* data, ticcmd_t *cmd, int maketic); 

void G_Ticker (doom_data_t* data);
boolean G_Responder (doom_data_t* data, event_t*	ev);

void G_ScreenShot (struct doom_data_t_* doom);

void G_DrawMouseSpeedBox(void);
int G_VanillaVersionCode(doom_data_t* doom);

extern int vanilla_savegame_limit;
extern int vanilla_demo_limit;
#endif

