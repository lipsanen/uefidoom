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
//  Intermission.
//

#ifndef __WI_STUFF__
#define __WI_STUFF__

//#include "v_video.h"

#include "doomdef.h"

// States for the intermission

// Called by main loop, animate the intermission.
void WI_Ticker (doom_data_t* doom);

// Called by main loop,
// draws the intermission directly into the screen buffer.
void WI_Drawer (doom_data_t* doom);

// Setup for an intermission screen.
void WI_Start(doom_data_t* doom, wbstartstruct_t*	 wbstartstruct);

// Shut down the intermission screen
void WI_End(doom_data_t* doom);

#endif
