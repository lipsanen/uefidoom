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
// DESCRIPTION:  Head up display
//

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"

#define HU_BROADCAST	5

#define HU_MSGX		0
#define HU_MSGY		0
#define HU_MSGWIDTH	64	// in characters
#define HU_MSGHEIGHT	1	// in lines

#define HU_MSGTIMEOUT	(4*TICRATE)

//
// HEADS UP TEXT
//

void HU_Init(struct doom_data_t_* doom);
void HU_Start(struct doom_data_t_* doom);
struct doom_data_t_;

boolean HU_Responder(struct doom_data_t_* doom, event_t* ev);

void HU_Ticker(struct doom_data_t_* doom);
void HU_Drawer(struct doom_data_t_* doom);
char HU_dequeueChatChar(struct doom_data_t_* doom);
void HU_Erase(struct doom_data_t_* doom);

extern const char *chat_macros[10];

#endif

