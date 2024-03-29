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
// DESCRIPTION:  Heads-up displays
//

#include "dlibc.h"

#include "doomdef.h"
#include "doomkeys.h"

#include "z_zone.h"

#include "deh_main.h"
#include "i_swap.h"
#include "i_video.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "m_controls.h"
#include "m_misc.h"
#include "w_wad.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

//
// Locally used constants, shortcuts.
//
#define HU_TITLE (mapnames[(doom->gameepisode - 1) * 9 + doom->gamemap - 1])
#define HU_TITLE2 (mapnames_commercial[doom->gamemap - 1])
#define HU_TITLEP (mapnames_commercial[doom->gamemap - 1 + 32])
#define HU_TITLET (mapnames_commercial[doom->gamemap - 1 + 64])
#define HU_TITLE_CHEX (mapnames[doom->gamemap - 1])
#define HU_TITLEHEIGHT 1
#define HU_TITLEX 0
#define HU_TITLEY (167 - SHORT(doom->hu_font[0]->height))

#define HU_INPUTTOGGLE 't'
#define HU_INPUTX HU_MSGX
#define HU_INPUTY (HU_MSGY + HU_MSGHEIGHT * (SHORT(doom->hu_font[0]->height) + 1))
#define HU_INPUTWIDTH 64
#define HU_INPUTHEIGHT 1

const char *chat_macros[10] =
    {
        HUSTR_CHATMACRO0,
        HUSTR_CHATMACRO1,
        HUSTR_CHATMACRO2,
        HUSTR_CHATMACRO3,
        HUSTR_CHATMACRO4,
        HUSTR_CHATMACRO5,
        HUSTR_CHATMACRO6,
        HUSTR_CHATMACRO7,
        HUSTR_CHATMACRO8,
        HUSTR_CHATMACRO9};

const char *player_names[] =
    {
        HUSTR_PLRGREEN,
        HUSTR_PLRINDIGO,
        HUSTR_PLRBROWN,
        HUSTR_PLRRED};

extern int showMessages;


//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

const char *mapnames[] = // DOOM shareware/registered/retail (Ultimate) names.
    {

        HUSTR_E1M1,
        HUSTR_E1M2,
        HUSTR_E1M3,
        HUSTR_E1M4,
        HUSTR_E1M5,
        HUSTR_E1M6,
        HUSTR_E1M7,
        HUSTR_E1M8,
        HUSTR_E1M9,

        HUSTR_E2M1,
        HUSTR_E2M2,
        HUSTR_E2M3,
        HUSTR_E2M4,
        HUSTR_E2M5,
        HUSTR_E2M6,
        HUSTR_E2M7,
        HUSTR_E2M8,
        HUSTR_E2M9,

        HUSTR_E3M1,
        HUSTR_E3M2,
        HUSTR_E3M3,
        HUSTR_E3M4,
        HUSTR_E3M5,
        HUSTR_E3M6,
        HUSTR_E3M7,
        HUSTR_E3M8,
        HUSTR_E3M9,

        HUSTR_E4M1,
        HUSTR_E4M2,
        HUSTR_E4M3,
        HUSTR_E4M4,
        HUSTR_E4M5,
        HUSTR_E4M6,
        HUSTR_E4M7,
        HUSTR_E4M8,
        HUSTR_E4M9,

        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL",
        "NEWLEVEL"};

// List of names for levels in commercial IWADs
// (doom2.wad, plutonia.wad, tnt.wad).  These are stored in a
// single large array; WADs like pl2.wad have a MAP33, and rely on
// the layout in the Vanilla executable, where it is possible to
// overflow the end of one array into the next.

const char *mapnames_commercial[] =
    {
        // DOOM 2 map names.

        HUSTR_1,
        HUSTR_2,
        HUSTR_3,
        HUSTR_4,
        HUSTR_5,
        HUSTR_6,
        HUSTR_7,
        HUSTR_8,
        HUSTR_9,
        HUSTR_10,
        HUSTR_11,

        HUSTR_12,
        HUSTR_13,
        HUSTR_14,
        HUSTR_15,
        HUSTR_16,
        HUSTR_17,
        HUSTR_18,
        HUSTR_19,
        HUSTR_20,

        HUSTR_21,
        HUSTR_22,
        HUSTR_23,
        HUSTR_24,
        HUSTR_25,
        HUSTR_26,
        HUSTR_27,
        HUSTR_28,
        HUSTR_29,
        HUSTR_30,
        HUSTR_31,
        HUSTR_32,

        // Plutonia WAD map names.

        PHUSTR_1,
        PHUSTR_2,
        PHUSTR_3,
        PHUSTR_4,
        PHUSTR_5,
        PHUSTR_6,
        PHUSTR_7,
        PHUSTR_8,
        PHUSTR_9,
        PHUSTR_10,
        PHUSTR_11,

        PHUSTR_12,
        PHUSTR_13,
        PHUSTR_14,
        PHUSTR_15,
        PHUSTR_16,
        PHUSTR_17,
        PHUSTR_18,
        PHUSTR_19,
        PHUSTR_20,

        PHUSTR_21,
        PHUSTR_22,
        PHUSTR_23,
        PHUSTR_24,
        PHUSTR_25,
        PHUSTR_26,
        PHUSTR_27,
        PHUSTR_28,
        PHUSTR_29,
        PHUSTR_30,
        PHUSTR_31,
        PHUSTR_32,

        // TNT WAD map names.

        THUSTR_1,
        THUSTR_2,
        THUSTR_3,
        THUSTR_4,
        THUSTR_5,
        THUSTR_6,
        THUSTR_7,
        THUSTR_8,
        THUSTR_9,
        THUSTR_10,
        THUSTR_11,

        THUSTR_12,
        THUSTR_13,
        THUSTR_14,
        THUSTR_15,
        THUSTR_16,
        THUSTR_17,
        THUSTR_18,
        THUSTR_19,
        THUSTR_20,

        THUSTR_21,
        THUSTR_22,
        THUSTR_23,
        THUSTR_24,
        THUSTR_25,
        THUSTR_26,
        THUSTR_27,
        THUSTR_28,
        THUSTR_29,
        THUSTR_30,
        THUSTR_31,
        THUSTR_32};

void HU_Init(struct doom_data_t_* doom)
{

    int i;
    int j;
    char buffer[9];

    // load the heads-up font
    j = HU_FONTSTART;
    for (i = 0; i < HU_FONTSIZE; i++)
    {
        d_snprintf(buffer, 9, "STCFN%.3d", j++);
        doom->hu_font[i] = (patch_t *)W_CacheLumpName(doom, buffer, PU_STATIC);
    }
}

void HU_Stop(struct doom_data_t_ *doom)
{
    doom->headsupactive = false;
}

void HU_Start(struct doom_data_t_ *doom)
{

    int i;
    const char *s;

    if (doom->headsupactive)
        HU_Stop(doom);

    doom->hustuff_plr = &doom->players[doom->consoleplayer];
    doom->message_on = false;
    doom->message_dontfuckwithme = false;
    doom->message_nottobefuckedwith = false;
    doom->chat_on = false;

    // create the message widget
    HUlib_initSText(doom, &doom->w_message,
                    HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
                    doom->hu_font,
                    HU_FONTSTART, &doom->message_on);

    // create the map title widget
    HUlib_initTextLine(doom, &doom->w_title,
                       HU_TITLEX, HU_TITLEY,
                       doom->hu_font,
                       HU_FONTSTART);

    switch (logical_gamemission)
    {
    case doom1:
        s = HU_TITLE;
        break;
    case doom2:
        s = HU_TITLE2;
        break;
    case pack_plut:
        s = HU_TITLEP;
        break;
    case pack_tnt:
        s = HU_TITLET;
        break;
    default:
        s = "Unknown level";
        break;
    }

    // Chex.exe always uses the episode 1 level title
    // eg. E2M1 gives the title for E1M1

    if (doom->gameversion == exe_chex)
    {
        s = HU_TITLE_CHEX;
    }

    // dehacked substitution to get modified level name

    s = DEH_String(s);

    while (*s)
        HUlib_addCharToTextLine(doom, &doom->w_title, *(s++));

    // create the chat widget
    HUlib_initIText(doom, &doom->w_chat,
                    HU_INPUTX, HU_INPUTY,
                    doom->hu_font,
                    HU_FONTSTART, &doom->chat_on);

    // create the inputbuffer widgets
    for (i = 0; i < MAXPLAYERS; i++)
        HUlib_initIText(doom, &doom->w_inputbuffer[i], 0, 0, 0, 0, &doom->always_off);

    doom->headsupactive = true;
}

void HU_Drawer(doom_data_t *doom)
{
    HUlib_drawSText(doom, &doom->w_message);
    HUlib_drawIText(doom, &doom->w_chat);
    if (doom->automapactive)
        HUlib_drawTextLine(doom, &doom->w_title, false);
}

void HU_Erase(doom_data_t *doom)
{

    HUlib_eraseSText(doom, &doom->w_message);
    HUlib_eraseIText(doom, &doom->w_chat);
    HUlib_eraseTextLine(doom, &doom->w_title);
}

void HU_Ticker(struct doom_data_t_ *doom)
{

    int i, rc;
    char c;

    // tick down message counter if message is up
    if (doom->message_counter && !--doom->message_counter)
    {
        doom->message_on = false;
        doom->message_nottobefuckedwith = false;
    }

    if (showMessages || doom->message_dontfuckwithme)
    {

        // display message if necessary
        if ((doom->hustuff_plr->message && !doom->message_nottobefuckedwith) || (doom->hustuff_plr->message && doom->message_dontfuckwithme))
        {
            HUlib_addMessageToSText(doom, &doom->w_message, 0, doom->hustuff_plr->message);
            doom->hustuff_plr->message = 0;
            doom->message_on = true;
            doom->message_counter = HU_MSGTIMEOUT;
            doom->message_nottobefuckedwith = doom->message_dontfuckwithme;
            doom->message_dontfuckwithme = 0;
        }

    } // else message_on = false;

    // check for incoming chat characters
    if (doom->netgame)
    {
        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (!doom->playeringame[i])
                continue;
            if (i != doom->consoleplayer && (c = doom->players[i].cmd.chatchar))
            {
                if (c <= HU_BROADCAST)
                    doom->chat_dest[i] = c;
                else
                {
                    rc = HUlib_keyInIText(doom, &doom->w_inputbuffer[i], c);
                    if (rc && c == KEY_ENTER)
                    {
                        if (doom->w_inputbuffer[i].l.len && (doom->chat_dest[i] == doom->consoleplayer + 1 || doom->chat_dest[i] == HU_BROADCAST))
                        {
                            HUlib_addMessageToSText(doom, &doom->w_message,
                                                    DEH_String(player_names[i]),
                                                    doom->w_inputbuffer[i].l.l);

                            doom->message_nottobefuckedwith = true;
                            doom->message_on = true;
                            doom->message_counter = HU_MSGTIMEOUT;
                            if (doom->gamemode == commercial)
                                S_StartSound(doom, 0, sfx_radio);
                            else
                                S_StartSound(doom, 0, sfx_tink);
                        }
                        HUlib_resetIText(doom, &doom->w_inputbuffer[i]);
                    }
                }
                doom->players[i].cmd.chatchar = 0;
            }
        }
    }
}

void HU_queueChatChar(struct doom_data_t_* doom, char c)
{
    if (((doom->head + 1) & (QUEUESIZE - 1)) == doom->tail)
    {
        doom->hustuff_plr->message = DEH_String(HUSTR_MSGU);
    }
    else
    {
        doom->chatchars[doom->head] = c;
        doom->head = (doom->head + 1) & (QUEUESIZE - 1);
    }
}

char HU_dequeueChatChar(struct doom_data_t_* doom)
{
    char c;

    if (doom->head != doom->tail)
    {
        c = doom->chatchars[doom->tail];
        doom->tail = (doom->tail + 1) & (QUEUESIZE - 1);
    }
    else
    {
        c = 0;
    }

    return c;
}

boolean HU_Responder(struct doom_data_t_* doom, event_t *ev)
{
    const char *macromessage;
    boolean eatkey = false;
    unsigned char c;
    int i;
    int numplayers;

    numplayers = 0;
    for (i = 0; i < MAXPLAYERS; i++)
        numplayers += doom->playeringame[i];

    if (ev->data1 == KEY_RSHIFT)
    {
        return false;
    }
    else if (ev->data1 == KEY_RALT || ev->data1 == KEY_LALT)
    {
        doom->altdown = ev->type == ev_keydown;
        return false;
    }

    if (ev->type != ev_keydown)
        return false;

    if (!doom->chat_on)
    {
        if (ev->data1 == key_message_refresh)
        {
            doom->message_on = true;
            doom->message_counter = HU_MSGTIMEOUT;
            eatkey = true;
        }
        else if (doom->netgame && ev->data2 == key_multi_msg)
        {
            eatkey = doom->chat_on = true;
            HUlib_resetIText(doom, &doom->w_chat);
            HU_queueChatChar(doom, HU_BROADCAST);
        }
        else if (doom->netgame && numplayers > 2)
        {
            for (i = 0; i < MAXPLAYERS; i++)
            {
                if (ev->data2 == key_multi_msgplayer[i])
                {
                    if (doom->playeringame[i] && i != doom->consoleplayer)
                    {
                        eatkey = doom->chat_on = true;
                        HUlib_resetIText(doom, &doom->w_chat);
                        HU_queueChatChar(doom, i + 1);
                        break;
                    }
                    else if (i == doom->consoleplayer)
                    {
                        doom->num_nobrainers++;
                        if (doom->num_nobrainers < 3)
                            doom->hustuff_plr->message = DEH_String(HUSTR_TALKTOSELF1);
                        else if (doom->num_nobrainers < 6)
                            doom->hustuff_plr->message = DEH_String(HUSTR_TALKTOSELF2);
                        else if (doom->num_nobrainers < 9)
                            doom->hustuff_plr->message = DEH_String(HUSTR_TALKTOSELF3);
                        else if (doom->num_nobrainers < 32)
                            doom->hustuff_plr->message = DEH_String(HUSTR_TALKTOSELF4);
                        else
                            doom->hustuff_plr->message = DEH_String(HUSTR_TALKTOSELF5);
                    }
                }
            }
        }
    }
    else
    {
        // send a macro
        if (doom->altdown)
        {
            c = ev->data1 - '0';
            if (c > 9)
                return false;
            // d_printf( "got here\n");
            macromessage = chat_macros[c];

            // kill last message with a '\n'
            HU_queueChatChar(doom, KEY_ENTER); // DEBUG!!!

            // send the macro message
            while (*macromessage)
                HU_queueChatChar(doom, *macromessage++);
            HU_queueChatChar(doom, KEY_ENTER);

            // leave chat mode and notify that it was sent
            doom->chat_on = false;
            M_StringCopy(doom->lastmessage, chat_macros[c], sizeof(doom->lastmessage));
            doom->hustuff_plr->message = doom->lastmessage;
            eatkey = true;
        }
        else
        {
            c = ev->data2;

            eatkey = HUlib_keyInIText(doom, &doom->w_chat, c);
            if (eatkey)
            {
                // static unsigned char buf[20]; // DEBUG
                HU_queueChatChar(doom, c);

                // d_snprintf(buf, sizeof(buf), "KEY: %d => %d", ev->data1, c);
                //        plr->message = buf;
            }
            if (c == KEY_ENTER)
            {
                doom->chat_on = false;
                if (doom->w_chat.l.len)
                {
                    M_StringCopy(doom->lastmessage, doom->w_chat.l.l, sizeof(doom->lastmessage));
                    doom->hustuff_plr->message = doom->lastmessage;
                }
            }
            else if (c == KEY_ESCAPE)
                doom->chat_on = false;
        }
    }

    return eatkey;
}
