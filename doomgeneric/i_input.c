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


#include "dlibc.h"

#include "config.h"
#include "deh_str.h"
#include "doomtype.h"
#include "doomkeys.h"
#include "i_system.h"
#include "i_swap.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"
#include "tables.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "doomgeneric.h"

int vanilla_keyboard_mapping = 1;

static unsigned char TranslateKey(unsigned char key)
{
	return key;
}

// Get the equivalent ASCII (Unicode?) character for a keypress.

static unsigned char GetTypedChar(unsigned char key)
{
    return TranslateKey(key);
}


void I_GetEvent(doom_data_t* doom)
{
    event_t event;
    int pressed;
    unsigned char key;

    
	while (DG_GetKey(&pressed, &key))
    {
        // process event
        
        if (pressed)
        {
            // data1 has the key pressed, data2 has the character
            // (shift-translated, etc)
            event.type = ev_keydown;
            event.data1 = TranslateKey(key);
            event.data2 = GetTypedChar(key);

            if (event.data1 != 0)
            {
                D_PostEvent(doom, &event);
            }
        }
        else
        {
            event.type = ev_keyup;
            event.data1 = TranslateKey(key);

            // data2 is just initialized to zero for ev_keyup.
            // For ev_keydown it's the shifted Unicode character
            // that was typed, but if something wants to detect
            // key releases it should do so based on data1
            // (key ID), not the printable char.

            event.data2 = 0;

            if (event.data1 != 0)
            {
                D_PostEvent(doom, &event);
            }
            break;
        }
    }


                /*
            case SDL_MOUSEMOTION:
                event.type = ev_mouse;
                event.data1 = mouse_button_state;
                event.data2 = AccelerateMouse(sdlevent.motion.xrel);
                event.data3 = -AccelerateMouse(sdlevent.motion.yrel);
                D_PostEvent(&event);
                break;
                */
}

void I_InitInput(void)
{
}

