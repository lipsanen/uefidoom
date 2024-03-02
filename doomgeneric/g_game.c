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
// DESCRIPTION:  none
//

#include "g_game.h"
#include "dlibc.h"

#include "doomdef.h"
#include "doomkeys.h"
#include "doomstat.h"

#include "deh_main.h"
#include "deh_misc.h"

#include "z_zone.h"
#include "f_finale.h"
#include "m_argv.h"
#include "m_controls.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"

#include "d_main.h"

#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"

#include "p_local.h"

#include "s_sound.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_sky.h"

#include "g_game.h"

#define SAVEGAMESIZE 0x2c000

void G_ReadDemoTiccmd(doom_data_t *doom, ticcmd_t *cmd);
void G_WriteDemoTiccmd(doom_data_t *doom, ticcmd_t *cmd);
void G_PlayerReborn(struct doom_data_t_* doom, int player);

void G_DoReborn(doom_data_t *doom, int playernum);

void G_DoLoadLevel(doom_data_t *doom);
void G_DoNewGame(doom_data_t *doom);
void G_DoPlayDemo(doom_data_t *doom);
void G_DoCompleted(doom_data_t *doom);
void G_DoVictory(void);
void G_DoWorldDone(doom_data_t *doom);
void G_DoSaveGame(struct doom_data_t_* doom);

#define MAXPLMOVE (doom->forwardmove[1])

#define TURBOTHRESHOLD 0x32


static const int *weapon_keys[] = {
    &key_weapon1,
    &key_weapon2,
    &key_weapon3,
    &key_weapon4,
    &key_weapon5,
    &key_weapon6,
    &key_weapon7,
    &key_weapon8};


// Used for prev/next weapon keys.
static const struct
{
    weapontype_t weapon;
    weapontype_t weapon_num;
} weapon_order_table[] = {
    {wp_fist, wp_fist},
    {wp_chainsaw, wp_fist},
    {wp_pistol, wp_pistol},
    {wp_shotgun, wp_shotgun},
    {wp_supershotgun, wp_shotgun},
    {wp_chaingun, wp_chaingun},
    {wp_missile, wp_missile},
    {wp_plasma, wp_plasma},
    {wp_bfg, wp_bfg}};

int G_CmdChecksum(ticcmd_t *cmd)
{
    size_t i;
    int sum = 0;

    for (i = 0; i < sizeof(*cmd) / 4 - 1; i++)
        sum += ((int *)cmd)[i];

    return sum;
}

static boolean WeaponSelectable(doom_data_t *doom, weapontype_t weapon)
{
    // Can't select the super shotgun in Doom 1.

    if (weapon == wp_supershotgun && logical_gamemission == doom1)
    {
        return false;
    }

    // These weapons aren't available in shareware.

    if ((weapon == wp_plasma || weapon == wp_bfg) && doom->gamemission == doom1 && doom->gamemode == shareware)
    {
        return false;
    }

    // Can't select a weapon if we don't own it.

    if (!doom->players[doom->consoleplayer].weaponowned[weapon])
    {
        return false;
    }

    // Can't select the fist if we have the chainsaw, unless
    // we also have the berserk pack.

    if (weapon == wp_fist && doom->players[doom->consoleplayer].weaponowned[wp_chainsaw] && !doom->players[doom->consoleplayer].powers[pw_strength])
    {
        return false;
    }

    return true;
}

static int G_NextWeapon(doom_data_t *doom, int direction)
{
    weapontype_t weapon;
    int start_i, i;

    // Find index in the table.

    if (doom->players[doom->consoleplayer].pendingweapon == wp_nochange)
    {
        weapon = doom->players[doom->consoleplayer].readyweapon;
    }
    else
    {
        weapon = doom->players[doom->consoleplayer].pendingweapon;
    }

    for (i = 0; i < arrlen(weapon_order_table); ++i)
    {
        if (weapon_order_table[i].weapon == weapon)
        {
            break;
        }
    }

    // Switch weapon. Don't loop forever.
    start_i = i;
    do
    {
        i += direction;
        i = (i + arrlen(weapon_order_table)) % arrlen(weapon_order_table);
    } while (i != start_i && !WeaponSelectable(doom, weapon_order_table[i].weapon));

    return weapon_order_table[i].weapon_num;
}

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd(doom_data_t *doom, ticcmd_t *cmd, int maketic)
{
    int i;
    boolean strafe;
    boolean bstrafe;
    int speed;
    int tspeed;
    int forward;
    int side;

    d_memset(cmd, 0, sizeof(ticcmd_t));

    cmd->consistancy =
        doom->consistancy[doom->consoleplayer][maketic % BACKUPTICS];

    strafe = doom->gamekeydown[key_strafe] || doom->mousebuttons[mousebstrafe] || doom->joybuttons[joybstrafe];

    // fraggle: support the old "joyb_speed = 31" hack which
    // allowed an autorun effect

    speed = key_speed >= NUMKEYS || joybspeed >= MAX_JOY_BUTTONS || doom->gamekeydown[key_speed] || doom->joybuttons[joybspeed];

    forward = side = 0;

    // use two stage accelerative turning
    // on the keyboard and joystick
    if (doom->joyxmove < 0 || doom->joyxmove > 0 || doom->gamekeydown[key_right] || doom->gamekeydown[key_left])
        doom->turnheld += doom->ticdup;
    else
        doom->turnheld = 0;

    if (doom->turnheld < SLOWTURNTICS)
        tspeed = 2; // slow turn
    else
        tspeed = speed;

    // let movement keys cancel each other out
    if (strafe)
    {
        if (doom->gamekeydown[key_right])
        {
            // d_printf( "strafe right\n");
            side += doom->sidemove[speed];
        }
        if (doom->gamekeydown[key_left])
        {
            //	d_printf( "strafe left\n");
            side -= doom->sidemove[speed];
        }
        if (doom->joyxmove > 0)
            side += doom->sidemove[speed];
        if (doom->joyxmove < 0)
            side -= doom->sidemove[speed];
    }
    else
    {
        if (doom->gamekeydown[key_right])
            cmd->angleturn -= doom->angleturn[tspeed];
        if (doom->gamekeydown[key_left])
            cmd->angleturn += doom->angleturn[tspeed];
        if (doom->joyxmove > 0)
            cmd->angleturn -= doom->angleturn[tspeed];
        if (doom->joyxmove < 0)
            cmd->angleturn += doom->angleturn[tspeed];
    }

    if (doom->gamekeydown[key_up])
    {
        // d_printf( "up\n");
        forward += doom->forwardmove[speed];
    }
    if (doom->gamekeydown[key_down])
    {
        // d_printf( "down\n");
        forward -= doom->forwardmove[speed];
    }

    if (doom->joyymove < 0)
        forward += doom->forwardmove[speed];
    if (doom->joyymove > 0)
        forward -= doom->forwardmove[speed];

    if (doom->gamekeydown[key_strafeleft] || doom->joybuttons[joybstrafeleft] || doom->mousebuttons[mousebstrafeleft] || doom->joystrafemove < 0)
    {
        side -= doom->sidemove[speed];
    }

    if (doom->gamekeydown[key_straferight] || doom->joybuttons[joybstraferight] || doom->mousebuttons[mousebstraferight] || doom->joystrafemove > 0)
    {
        side += doom->sidemove[speed];
    }

    // buttons
    cmd->chatchar = HU_dequeueChatChar();

    if (doom->gamekeydown[key_fire] || doom->mousebuttons[mousebfire] || doom->joybuttons[joybfire])
        cmd->buttons |= BT_ATTACK;

    if (doom->gamekeydown[key_use] || doom->joybuttons[joybuse] || doom->mousebuttons[mousebuse])
    {
        cmd->buttons |= BT_USE;
        // clear double clicks if hit use button
        doom->dclicks = 0;
    }

    // If the previous or next weapon button is pressed, the
    // next_weapon variable is set to change weapons when
    // we generate a ticcmd.  Choose a new weapon.

    if (doom->gamestate == GS_LEVEL && doom->next_weapon != 0)
    {
        i = G_NextWeapon(doom, doom->next_weapon);
        cmd->buttons |= BT_CHANGE;
        cmd->buttons |= i << BT_WEAPONSHIFT;
    }
    else
    {
        // Check weapon keys.

        for (i = 0; i < arrlen(weapon_keys); ++i)
        {
            int key = *weapon_keys[i];

            if (doom->gamekeydown[key])
            {
                cmd->buttons |= BT_CHANGE;
                cmd->buttons |= i << BT_WEAPONSHIFT;
                break;
            }
        }
    }

    doom->next_weapon = 0;

    // mouse
    if (doom->mousebuttons[mousebforward])
    {
        forward += doom->forwardmove[speed];
    }
    if (doom->mousebuttons[mousebbackward])
    {
        forward -= doom->forwardmove[speed];
    }

    if (dclick_use)
    {
        // forward double click
        if (doom->mousebuttons[mousebforward] != doom->dclickstate && doom->dclicktime > 1)
        {
            doom->dclickstate = doom->mousebuttons[mousebforward];
            if (doom->dclickstate)
                doom->dclicks++;
            if (doom->dclicks == 2)
            {
                cmd->buttons |= BT_USE;
                doom->dclicks = 0;
            }
            else
                doom->dclicktime = 0;
        }
        else
        {
            doom->dclicktime += doom->ticdup;
            if (doom->dclicktime > 20)
            {
                doom->dclicks = 0;
                doom->dclickstate = 0;
            }
        }

        // strafe double click
        bstrafe =
            doom->mousebuttons[mousebstrafe] || doom->joybuttons[joybstrafe];
        if (bstrafe != doom->dclickstate2 && doom->dclicktime2 > 1)
        {
            doom->dclickstate2 = bstrafe;
            if (doom->dclickstate2)
                doom->dclicks2++;
            if (doom->dclicks2 == 2)
            {
                cmd->buttons |= BT_USE;
                doom->dclicks2 = 0;
            }
            else
                doom->dclicktime2 = 0;
        }
        else
        {
            doom->dclicktime2 += doom->ticdup;
            if (doom->dclicktime2 > 20)
            {
                doom->dclicks2 = 0;
                doom->dclickstate2 = 0;
            }
        }
    }

    forward += doom->mousey;

    if (strafe)
        side += doom->mousex * 2;
    else
        cmd->angleturn -= doom->mousex * 0x8;

    if (doom->mousex == 0)
    {
        // No movement in the previous frame

        doom->testcontrols_mousespeed = 0;
    }

    doom->mousex = doom->mousey = 0;

    if (forward > MAXPLMOVE)
        forward = MAXPLMOVE;
    else if (forward < -MAXPLMOVE)
        forward = -MAXPLMOVE;
    if (side > MAXPLMOVE)
        side = MAXPLMOVE;
    else if (side < -MAXPLMOVE)
        side = -MAXPLMOVE;

    cmd->forwardmove += forward;
    cmd->sidemove += side;

    // special buttons
    if (doom->sendpause)
    {
        doom->sendpause = false;
        cmd->buttons = BT_SPECIAL | BTS_PAUSE;
    }

    if (doom->sendsave)
    {
        doom->sendsave = false;
        cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (doom->savegameslot << BTS_SAVESHIFT);
    }

    // low-res turning

    if (doom->lowres_turn)
    {
        static signed short carry = 0;
        signed short desired_angleturn;

        desired_angleturn = cmd->angleturn + carry;

        // round angleturn to the nearest 256 unit boundary
        // for recording demos with single byte values for turn

        cmd->angleturn = (desired_angleturn + 128) & 0xff00;

        // Carry forward the error from the reduced resolution to the
        // next tic, so that successive small movements can accumulate.

        carry = desired_angleturn - cmd->angleturn;
    }
}

//
// G_DoLoadLevel
//
void G_DoLoadLevel(doom_data_t *doom)
{
    int i;

    // Set the sky map.
    // First thing, we have a dummy sky texture name,
    //  a flat. The data is in the WAD only because
    //  we look for an actual index, instead of simply
    //  setting one.

    skyflatnum = R_FlatNumForName(doom, DEH_String(SKYFLATNAME));

    // The "Sky never changes in Doom II" bug was fixed in
    // the id Anthology version of doom2.exe for Final Doom.
    if ((doom->gamemode == commercial) && (doom->gameversion == exe_final2 || doom->gameversion == exe_chex))
    {
        char *skytexturename;

        if (doom->gamemap < 12)
        {
            skytexturename = "SKY1";
        }
        else if (doom->gamemap < 21)
        {
            skytexturename = "SKY2";
        }
        else
        {
            skytexturename = "SKY3";
        }

        skytexturename = DEH_String(skytexturename);

        skytexture = R_TextureNumForName(skytexturename);
    }

    doom->levelstarttic = doom->gametic; // for time calculation

    if (doom->wipegamestate == GS_LEVEL)
        doom->wipegamestate = -1; // force a wipe

    doom->gamestate = GS_LEVEL;

    for (i = 0; i < MAXPLAYERS; i++)
    {
        doom->turbodetected[i] = false;
        if (doom->playeringame[i] && doom->players[i].playerstate == PST_DEAD)
            doom->players[i].playerstate = PST_REBORN;
        d_memset(doom->players[i].frags, 0, sizeof(doom->players[i].frags));
    }

    P_SetupLevel(doom, doom->gameepisode, doom->gamemap, 0, doom->gameskill);
    doom->displayplayer = doom->consoleplayer; // view the guy you are playing
    doom->gameaction = ga_nothing;
    Z_CheckHeap();

    // clear cmd building stuff

    d_memset(doom->gamekeydown, 0, sizeof(doom->gamekeydown));
    doom->joyxmove = doom->joyymove = doom->joystrafemove = 0;
    doom->mousex = doom->mousey = 0;
    doom->sendpause = doom->sendsave = doom->paused = false;
    d_memset(doom->mousearray, 0, sizeof(doom->mousearray));
    d_memset(doom->joyarray, 0, sizeof(doom->joyarray));

    if (doom->testcontrols)
    {
        doom->players[doom->consoleplayer].message = "Press escape to quit.";
    }
}

static void SetJoyButtons(doom_data_t* doom, unsigned int buttons_mask)
{
    int i;

    for (i = 0; i < MAX_JOY_BUTTONS; ++i)
    {
        int button_on = (buttons_mask & (1 << i)) != 0;

        // Detect button press:

        if (!doom->joybuttons[i] && button_on)
        {
            // Weapon cycling:

            if (i == joybprevweapon)
            {
                doom->next_weapon = -1;
            }
            else if (i == joybnextweapon)
            {
                doom->next_weapon = 1;
            }
        }

        doom->joybuttons[i] = button_on;
    }
}

static void SetMouseButtons(doom_data_t* doom, unsigned int buttons_mask)
{
    int i;

    for (i = 0; i < MAX_MOUSE_BUTTONS; ++i)
    {
        unsigned int button_on = (buttons_mask & (1 << i)) != 0;

        // Detect button press:

        if (!doom->mousebuttons[i] && button_on)
        {
            if (i == mousebprevweapon)
            {
                doom->next_weapon = -1;
            }
            else if (i == mousebnextweapon)
            {
                doom->next_weapon = 1;
            }
        }

        doom->mousebuttons[i] = button_on;
    }
}

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
boolean G_Responder(doom_data_t *doom, event_t *ev)
{
    // allow spy mode changes even during the demo
    if (doom->gamestate == GS_LEVEL && ev->type == ev_keydown && ev->data1 == key_spy && (doom->singledemo || !doom->deathmatch))
    {
        // spy mode
        do
        {
            doom->displayplayer++;
            if (doom->displayplayer == MAXPLAYERS)
                doom->displayplayer = 0;
        } while (!doom->playeringame[doom->displayplayer] && doom->displayplayer != doom->consoleplayer);
        return true;
    }

    // any other key pops up menu if in demos
    if (doom->gameaction == ga_nothing && !doom->singledemo &&
        (doom->demoplayback || doom->gamestate == GS_DEMOSCREEN))
    {
        if (ev->type == ev_keydown ||
            (ev->type == ev_mouse && ev->data1) ||
            (ev->type == ev_joystick && ev->data1))
        {
            M_StartControlPanel();
            return true;
        }
        return false;
    }

    if (doom->gamestate == GS_LEVEL)
    {
#if 0 
	if (devparm && ev->type == ev_keydown && ev->data1 == ';') 
	{ 
	    G_DeathMatchSpawnPlayer (0); 
	    return true; 
	}
#endif
        if (HU_Responder(doom, ev))
            return true; // chat ate the event
        if (ST_Responder(doom, ev))
            return true; // status window ate it
        if (AM_Responder(doom, ev))
            return true; // automap ate it
    }

    if (doom->gamestate == GS_FINALE)
    {
        if (F_Responder(doom, ev))
            return true; // finale ate the event
    }

    if (doom->testcontrols && ev->type == ev_mouse)
    {
        // If we are invoked by setup to test the controls, save the
        // mouse speed so that we can display it on-screen.
        // Perform a low pass filter on this so that the thermometer
        // appears to move smoothly.

       doom-> testcontrols_mousespeed = d_abs(ev->data2);
    }

    // If the next/previous weapon keys are pressed, set the next_weapon
    // variable to change weapons when the next ticcmd is generated.

    if (ev->type == ev_keydown && ev->data1 == key_prevweapon)
    {
        doom->next_weapon = -1;
    }
    else if (ev->type == ev_keydown && ev->data1 == key_nextweapon)
    {
        doom->next_weapon = 1;
    }

    switch (ev->type)
    {
    case ev_keydown:
        if (ev->data1 == key_pause)
        {
            doom->sendpause = true;
        }
        else if (ev->data1 < NUMKEYS)
        {
            doom->gamekeydown[ev->data1] = true;
        }

        return true; // eat key down events

    case ev_keyup:
        if (ev->data1 < NUMKEYS)
            doom->gamekeydown[ev->data1] = false;
        return false; // always let key up events filter down

    case ev_mouse:
        SetMouseButtons(doom, ev->data1);
        doom->mousex = ev->data2 * (mouseSensitivity + 5) / 10;
        doom->mousey = ev->data3 * (mouseSensitivity + 5) / 10;
        return true; // eat events

    case ev_joystick:
        SetJoyButtons(doom, ev->data1);
        doom->joyxmove = ev->data2;
        doom->joyymove = ev->data3;
        doom->joystrafemove = ev->data4;
        return true; // eat events

    default:
        break;
    }

    return false;
}

//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker(doom_data_t *doom)
{
    int i;
    int buf;
    ticcmd_t *cmd;

    // do player reborns if needed
    for (i = 0; i < MAXPLAYERS; i++)
        if (doom->playeringame[i] && doom->players[i].playerstate == PST_REBORN)
            G_DoReborn(doom, i);

    // do things to change the game state
    while (doom->gameaction != ga_nothing)
    {
        switch (doom->gameaction)
        {
        case ga_loadlevel:
            G_DoLoadLevel(doom);
            break;
        case ga_newgame:
            G_DoNewGame(doom);
            break;
        case ga_loadgame:
            G_DoLoadGame(doom);
            break;
        case ga_savegame:
            G_DoSaveGame(doom);
            break;
        case ga_playdemo:
            G_DoPlayDemo(doom);
            break;
        case ga_completed:
            G_DoCompleted(doom);
            break;
        case ga_victory:
            F_StartFinale();
            break;
        case ga_worlddone:
            G_DoWorldDone(doom);
            break;
        case ga_screenshot:
            V_ScreenShot(doom, "DOOM%02i.%s");
            doom->players[doom->consoleplayer].message = DEH_String("screen shot");
            doom->gameaction = ga_nothing;
            break;
        case ga_nothing:
            break;
        }
    }

    // get commands, check consistancy,
    // and build new consistancy check
    buf = (doom->gametic / doom->ticdup) % BACKUPTICS;

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (doom->playeringame[i])
        {
            cmd = &doom->players[i].cmd;

            d_memcpy(cmd, &doom->netcmds[i], sizeof(ticcmd_t));

            if (doom->demoplayback)
                G_ReadDemoTiccmd(doom, cmd);
            if (doom->demorecording)
                G_WriteDemoTiccmd(doom, cmd);

            // check for turbo cheats

            // check ~ 4 seconds whether to display the turbo message.
            // store if the turbo threshold was exceeded in any tics
            // over the past 4 seconds.  offset the checking period
            // for each player so messages are not displayed at the
            // same time.

            if (cmd->forwardmove > TURBOTHRESHOLD)
            {
                doom->turbodetected[i] = true;
            }

            if ((doom->gametic & 31) == 0 && ((doom->gametic >> 5) % MAXPLAYERS) == i && doom->turbodetected[i])
            {
                static char turbomessage[80];
                extern char *player_names[4];
                d_snprintf(turbomessage, sizeof(turbomessage),
                           "%s is turbo!", player_names[i]);
                doom->players[doom->consoleplayer].message = turbomessage;
                doom->turbodetected[i] = false;
            }

            if (doom->netgame && !doom->netdemo && !(doom->gametic % doom->ticdup))
            {
                if (doom->gametic > BACKUPTICS && doom->consistancy[i][buf] != cmd->consistancy)
                {
                    I_Error("consistency failure (%i should be %i)",
                            cmd->consistancy, doom->consistancy[i][buf]);
                }
                if (doom->players[i].mo)
                    doom->consistancy[i][buf] = doom->players[i].mo->x;
                else
                    doom->consistancy[i][buf] = rndindex;
            }
        }
    }

    // check for special buttons
    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (doom->playeringame[i])
        {
            if (doom->players[i].cmd.buttons & BT_SPECIAL)
            {
                switch (doom->players[i].cmd.buttons & BT_SPECIALMASK)
                {
                case BTS_PAUSE:
                    doom->paused ^= 1;
                    if (doom->paused)
                        S_PauseSound();
                    else
                        S_ResumeSound();
                    break;

                case BTS_SAVEGAME:
                    if (!doom->savedescription[0])
                    {
                        M_StringCopy(doom->savedescription, "NET GAME",
                                     sizeof(doom->savedescription));
                    }

                    doom->savegameslot =
                        (doom->players[i].cmd.buttons & BTS_SAVEMASK) >> BTS_SAVESHIFT;
                    doom->gameaction = ga_savegame;
                    break;
                }
            }
        }
    }

    // Have we just finished displaying an intermission screen?

    if (doom->game_oldgamestate == GS_INTERMISSION && doom->gamestate != GS_INTERMISSION)
    {
        WI_End(doom);
    }

    doom->game_oldgamestate = doom->gamestate;

    // do main actions
    switch (doom->gamestate)
    {
    case GS_LEVEL:
        P_Ticker(doom);
        ST_Ticker(doom);
        AM_Ticker(doom);
        HU_Ticker(doom);
        break;

    case GS_INTERMISSION:
        WI_Ticker(doom);
        break;

    case GS_FINALE:
        F_Ticker();
        break;

    case GS_DEMOSCREEN:
        D_PageTicker(doom);
        break;
    }
}

//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer
// Called at the start.
// Called by the game initialization functions.
//
void G_InitPlayer(struct doom_data_t_* doom, int player)
{
    // clear everything else to defaults
    G_PlayerReborn(doom, player);
}

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel(struct doom_data_t_* doom, int player)
{
    player_t *p;

    p = &doom->players[player];

    d_memset(p->powers, 0, sizeof(p->powers));
    d_memset(p->cards, 0, sizeof(p->cards));
    p->mo->flags &= ~MF_SHADOW; // cancel invisibility
    p->extralight = 0;          // cancel gun flashes
    p->fixedcolormap = 0;       // cancel ir gogles
    p->damagecount = 0;         // no palette changes
    p->bonuscount = 0;
}

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn(struct doom_data_t_* doom, int player)
{
    player_t *p;
    int i;
    int frags[MAXPLAYERS];
    int killcount;
    int itemcount;
    int secretcount;

    d_memcpy(frags, doom->players[player].frags, sizeof(frags));
    killcount = doom->players[player].killcount;
    itemcount = doom->players[player].itemcount;
    secretcount = doom->players[player].secretcount;

    p = &doom->players[player];
    d_memset(p, 0, sizeof(*p));

    d_memcpy(doom->players[player].frags, frags, sizeof(doom->players[player].frags));
    doom->players[player].killcount = killcount;
    doom->players[player].itemcount = itemcount;
    doom->players[player].secretcount = secretcount;

    p->usedown = p->attackdown = true; // don't do anything immediately
    p->playerstate = PST_LIVE;
    p->health = deh_initial_health; // Use dehacked value
    p->readyweapon = p->pendingweapon = wp_pistol;
    p->weaponowned[wp_fist] = true;
    p->weaponowned[wp_pistol] = true;
    p->ammo[am_clip] = deh_initial_bullets;

    for (i = 0; i < NUMAMMO; i++)
        p->maxammo[i] = maxammo[i];
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
void P_SpawnPlayer(mapthing_t *mthing);

boolean
G_CheckSpot(doom_data_t *doom,
            int playernum,
            mapthing_t *mthing)
{
    fixed_t x;
    fixed_t y;
    subsector_t *ss;
    mobj_t *mo;
    int i;

    if (!doom->players[playernum].mo)
    {
        // first spawn of level, before corpses
        for (i = 0; i < playernum; i++)
            if (doom->players[i].mo->x == mthing->x << FRACBITS && doom->players[i].mo->y == mthing->y << FRACBITS)
                return false;
        return true;
    }

    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;

    if (!P_CheckPosition(doom, doom->players[playernum].mo, x, y))
        return false;

    // flush an old corpse if needed
    if (doom->bodyqueslot >= BODYQUESIZE)
        P_RemoveMobj(doom->bodyque[doom->bodyqueslot % BODYQUESIZE]);
    doom->bodyque[doom->bodyqueslot % BODYQUESIZE] = doom->players[playernum].mo;
    doom->bodyqueslot++;

    // spawn a teleport fog
    ss = R_PointInSubsector(x, y);

    // The code in the released source looks like this:
    //
    //    an = ( ANG45 * (((unsigned int) mthing->angle)/45) )
    //         >> ANGLETOFINESHIFT;
    //    mo = P_SpawnMobj (x+20*finecosine[an], y+20*finesine[an]
    //                     , ss->sector->floorheight
    //                     , MT_TFOG);
    //
    // But 'an' can be a signed value in the DOS version. This means that
    // we get a negative index and the lookups into finecosine/finesine
    // end up dereferencing values in finetangent[].
    // A player spawning on a deathmatch start facing directly west spawns
    // "silently" with no spawn fog. Emulate this.
    //
    // This code is imported from PrBoom+.

    {
        fixed_t xa, ya;
        signed int an;

        // This calculation overflows in Vanilla Doom, but here we deliberately
        // avoid integer overflow as it is undefined behavior, so the value of
        // 'an' will always be positive.
        an = (ANG45 >> ANGLETOFINESHIFT) * ((signed int)mthing->angle / 45);

        switch (an)
        {
        case 4096:                  // -4096:
            xa = finetangent[2048]; // finecosine[-4096]
            ya = finetangent[0];    // finesine[-4096]
            break;
        case 5120:                  // -3072:
            xa = finetangent[3072]; // finecosine[-3072]
            ya = finetangent[1024]; // finesine[-3072]
            break;
        case 6144:                  // -2048:
            xa = finesine[0];       // finecosine[-2048]
            ya = finetangent[2048]; // finesine[-2048]
            break;
        case 7168:                  // -1024:
            xa = finesine[1024];    // finecosine[-1024]
            ya = finetangent[3072]; // finesine[-1024]
            break;
        case 0:
        case 1024:
        case 2048:
        case 3072:
            xa = finecosine[an];
            ya = finesine[an];
            break;
        default:
            I_Error("G_CheckSpot: unexpected angle %d\n", an);
            xa = ya = 0;
            break;
        }
        mo = P_SpawnMobj(doom, x + 20 * xa, y + 20 * ya,
                         ss->sector->floorheight, MT_TFOG);
    }

    if (doom->players[doom->consoleplayer].viewz != 1)
        S_StartSound(doom, mo, sfx_telept); // don't start sound on first frame

    return true;
}

//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//
void G_DeathMatchSpawnPlayer(doom_data_t *doom, int playernum)
{
    int i, j;
    int selections;

    selections = deathmatch_p - deathmatchstarts;
    if (selections < 4)
        I_Error("Only %i deathmatch spots, 4 required", selections);

    for (j = 0; j < 20; j++)
    {
        i = P_Random() % selections;
        if (G_CheckSpot(doom, playernum, &deathmatchstarts[i]))
        {
            deathmatchstarts[i].type = playernum + 1;
            P_SpawnPlayer(&deathmatchstarts[i]);
            return;
        }
    }

    // no good spot, so the player will probably get stuck
    P_SpawnPlayer(&playerstarts[playernum]);
}

//
// G_DoReborn
//
void G_DoReborn(doom_data_t *doom, int playernum)
{
    int i;

    if (!doom->netgame)
    {
        // reload the level from scratch
        doom->gameaction = ga_loadlevel;
    }
    else
    {
        // respawn at the start

        // first dissasociate the corpse
        doom->players[playernum].mo->player = NULL;

        // spawn at random spot if in death match
        if (doom->deathmatch)
        {
            G_DeathMatchSpawnPlayer(doom, playernum);
            return;
        }

        if (G_CheckSpot(doom, playernum, &playerstarts[playernum]))
        {
            P_SpawnPlayer(&playerstarts[playernum]);
            return;
        }

        // try to spawn at one of the other players spots
        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (G_CheckSpot(doom, playernum, &playerstarts[i]))
            {
                playerstarts[i].type = playernum + 1; // fake as other player
                P_SpawnPlayer(&playerstarts[i]);
                playerstarts[i].type = i + 1; // restore
                return;
            }
            // he's going to be inside something.  Too bad.
        }
        P_SpawnPlayer(&playerstarts[playernum]);
    }
}

void G_ScreenShot(struct doom_data_t_* doom)
{
    doom->gameaction = ga_screenshot;
}

// DOOM Par Times
int pars[4][10] =
    {
        {0},
        {0, 30, 75, 120, 90, 165, 180, 180, 30, 165},
        {0, 90, 90, 90, 120, 90, 360, 240, 30, 170},
        {0, 90, 45, 90, 150, 90, 90, 165, 30, 135}};

// DOOM II Par Times
int cpars[32] =
    {
        30, 90, 120, 120, 90, 150, 120, 120, 270, 90,     //  1-10
        210, 150, 150, 150, 210, 150, 420, 150, 210, 150, // 11-20
        240, 150, 180, 150, 150, 300, 330, 420, 300, 180, // 21-30
        120, 30                                           // 31-32
};

//
// G_DoCompleted
//
boolean secretexit;
extern char *pagename;

void G_ExitLevel(struct doom_data_t_* doom)
{
    secretexit = false;
    doom->gameaction = ga_completed;
}

// Here's for the german edition.
void G_SecretExitLevel(doom_data_t *doom)
{
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ((doom->gamemode == commercial) && (W_CheckNumForName(doom, "map31") < 0))
        secretexit = false;
    else
        secretexit = true;
    doom->gameaction = ga_completed;
}

void G_DoCompleted(doom_data_t *doom)
{
    int i;

    doom->gameaction = ga_nothing;

    for (i = 0; i < MAXPLAYERS; i++)
        if (doom->playeringame[i])
            G_PlayerFinishLevel(doom, i); // take away cards and stuff

    if (doom->automapactive)
        AM_Stop(doom);

    if (doom->gamemode != commercial)
    {
        // Chex Quest ends after 5 levels, rather than 8.

        if (doom->gameversion == exe_chex)
        {
            if (doom->gamemap == 5)
            {
                doom->gameaction = ga_victory;
                return;
            }
        }
        else
        {
            switch (doom->gamemap)
            {
            case 8:
                doom->gameaction = ga_victory;
                return;
            case 9:
                for (i = 0; i < MAXPLAYERS; i++)
                    doom->players[i].didsecret = true;
                break;
            }
        }
    }

    //#if 0  Hmmm - why?
    if ((doom->gamemap == 8) && (doom->gamemode != commercial))
    {
        // victory
        doom->gameaction = ga_victory;
        return;
    }

    if ((doom->gamemap == 9) && (doom->gamemode != commercial))
    {
        // exit secret level
        for (i = 0; i < MAXPLAYERS; i++)
            doom->players[i].didsecret = true;
    }
    //#endif

    doom->wminfo.didsecret = doom->players[doom->consoleplayer].didsecret;
    doom->wminfo.epsd = doom->gameepisode - 1;
    doom->wminfo.last = doom->gamemap - 1;

    // wminfo.next is 0 biased, unlike gamemap
    if (doom->gamemode == commercial)
    {
        if (secretexit)
            switch (doom->gamemap)
            {
            case 15:
                doom->wminfo.next = 30;
                break;
            case 31:
                doom->wminfo.next = 31;
                break;
            }
        else
            switch (doom->gamemap)
            {
            case 31:
            case 32:
                doom->wminfo.next = 15;
                break;
            default:
                doom->wminfo.next = doom->gamemap;
            }
    }
    else
    {
        if (secretexit)
            doom->wminfo.next = 8; // go to secret level
        else if (doom->gamemap == 9)
        {
            // returning from secret level
            switch (doom->gameepisode)
            {
            case 1:
                doom->wminfo.next = 3;
                break;
            case 2:
                doom->wminfo.next = 5;
                break;
            case 3:
                doom->wminfo.next = 6;
                break;
            case 4:
                doom->wminfo.next = 2;
                break;
            }
        }
        else
            doom->wminfo.next = doom->gamemap; // go to next level
    }

    doom->wminfo.maxkills = doom->totalkills;
    doom->wminfo.maxitems = doom->totalitems;
    doom->wminfo.maxsecret = doom->totalsecret;
    doom->wminfo.maxfrags = 0;

    // Set par time. Doom episode 4 doesn't have a par time, so this
    // overflows into the cpars array. It's necessary to emulate this
    // for statcheck regression testing.
    if (doom->gamemode == commercial)
        doom->wminfo.partime = TICRATE * cpars[doom->gamemap - 1];
    else if (doom->gameepisode < 4)
        doom->wminfo.partime = TICRATE * pars[doom->gameepisode][doom->gamemap];
    else
        doom->wminfo.partime = TICRATE * cpars[doom->gamemap];

    doom->wminfo.pnum = doom->consoleplayer;

    for (i = 0; i < MAXPLAYERS; i++)
    {
        doom->wminfo.plyr[i].in = doom->playeringame[i];
        doom->wminfo.plyr[i].skills = doom->players[i].killcount;
        doom->wminfo.plyr[i].sitems = doom->players[i].itemcount;
        doom->wminfo.plyr[i].ssecret = doom->players[i].secretcount;
        doom->wminfo.plyr[i].stime = leveltime;
        d_memcpy(doom->wminfo.plyr[i].frags, doom->players[i].frags, sizeof(doom->wminfo.plyr[i].frags));
    }

    doom->gamestate = GS_INTERMISSION;
    doom->viewactive = false;
    doom->automapactive = false;

    WI_Start(doom, &doom->wminfo);
}

//
// G_WorldDone
//
void G_WorldDone(doom_data_t *doom)
{
    doom->gameaction = ga_worlddone;

    if (secretexit)
        doom->players[doom->consoleplayer].didsecret = true;

    if (doom->gamemode == commercial)
    {
        switch (doom->gamemap)
        {
        case 15:
        case 31:
            if (!secretexit)
                break;
        case 6:
        case 11:
        case 20:
        case 30:
            F_StartFinale();
            break;
        }
    }
}

void G_DoWorldDone(doom_data_t *doom)
{
    doom->gamestate = GS_LEVEL;
    doom->gamemap = doom->wminfo.next + 1;
    G_DoLoadLevel(doom);
    doom->gameaction = ga_nothing;
    doom->viewactive = true;
}

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
extern boolean setsizeneeded;
void R_ExecuteSetViewSize(void);

char savename[256];

void G_LoadGame(struct doom_data_t_* doom, char *name)
{
    M_StringCopy(savename, name, sizeof(savename));
    doom->gameaction = ga_loadgame;
}

#define VERSIONSIZE 16

void G_DoLoadGame(doom_data_t *doom)
{
    int savedleveltime;

    doom->gameaction = ga_nothing;

    save_stream = d_fopen(savename, "rb");

    if (save_stream == NULL)
    {
        return;
    }

    savegame_error = false;

    if (!P_ReadSaveGameHeader(doom))
    {
        d_fclose(save_stream);
        return;
    }

    savedleveltime = leveltime;

    // load a base level
    G_InitNew(doom, doom->gameskill, doom->gameepisode, doom->gamemap);

    leveltime = savedleveltime;

    // dearchive all the modifications
    P_UnArchivePlayers(doom);
    P_UnArchiveWorld();
    P_UnArchiveThinkers(doom);
    P_UnArchiveSpecials();

    if (!P_ReadSaveGameEOF())
        I_Error("Bad savegame");

    d_fclose(save_stream);

    if (setsizeneeded)
        R_ExecuteSetViewSize();

    // draw the pattern into the back screen
    R_FillBackScreen(doom);
}

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void G_SaveGame(doom_data_t* doom, int slot,
                char *description)
{
    doom->savegameslot = slot;
    M_StringCopy(doom->savedescription, description, sizeof(doom->savedescription));
    doom->sendsave = true;
}

void G_DoSaveGame(struct doom_data_t_* doom)
{
    doom->gameaction = ga_nothing;
}

//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.
//
skill_t d_skill;
int d_episode;
int d_map;

void G_DeferedInitNew(struct doom_data_t_* doom, skill_t skill,
                      int episode,
                      int map)
{
    d_skill = skill;
    d_episode = episode;
    d_map = map;
    doom->gameaction = ga_newgame;
}

void G_DoNewGame(doom_data_t *doom)
{
    doom->demoplayback = false;
    doom->netdemo = false;
    doom->netgame = false;
    doom->deathmatch = false;
    doom->playeringame[1] = doom->playeringame[2] = doom->playeringame[3] = 0;
    doom->respawnparm = false;
    doom->fastparm = false;
    doom->nomonsters = false;
    doom->consoleplayer = 0;
    G_InitNew(doom, d_skill, d_episode, d_map);
    doom->gameaction = ga_nothing;
}

void G_InitNew(doom_data_t *doom,
               skill_t skill,
               int episode,
               int map)
{
    char *skytexturename;
    int i;

    if (doom->paused)
    {
        doom->paused = false;
        S_ResumeSound();
    }

    /*
    // Note: This commented-out block of code was added at some point
    // between the DOS version(s) and the Doom source release. It isn't
    // found in disassemblies of the DOS version and causes IDCLEV and
    // the -warp command line parameter to behave differently.
    // This is left here for posterity.

    // This was quite messy with SPECIAL and commented parts.
    // Supposedly hacks to make the latest edition work.
    // It might not work properly.
    if (episode < 1)
      episode = 1;

    if ( gamemode == retail )
    {
      if (episode > 4)
    episode = 4;
    }
    else if ( gamemode == shareware )
    {
      if (episode > 1)
       episode = 1;	// only start episode 1 on shareware
    }
    else
    {
      if (episode > 3)
    episode = 3;
    }
    */

    if (skill > sk_nightmare)
        skill = sk_nightmare;

    if (doom->gameversion >= exe_ultimate)
    {
        if (episode == 0)
        {
            episode = 4;
        }
    }
    else
    {
        if (episode < 1)
        {
            episode = 1;
        }
        if (episode > 3)
        {
            episode = 3;
        }
    }

    if (episode > 1 && doom->gamemode == shareware)
    {
        episode = 1;
    }

    if (map < 1)
        map = 1;

    if ((map > 9) && (doom->gamemode != commercial))
        map = 9;

    M_ClearRandom();

    if (skill == sk_nightmare || doom->respawnparm)
        doom->respawnmonsters = true;
    else
        doom->respawnmonsters = false;

    if (doom->fastparm || (skill == sk_nightmare && doom->gameskill != sk_nightmare))
    {
        for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
            states[i].tics >>= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 20 * FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 20 * FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 20 * FRACUNIT;
    }
    else if (skill != sk_nightmare && doom->gameskill == sk_nightmare)
    {
        for (i = S_SARG_RUN1; i <= S_SARG_PAIN2; i++)
            states[i].tics <<= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 15 * FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 10 * FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 10 * FRACUNIT;
    }

    // force players to be initialized upon first level load
    for (i = 0; i < MAXPLAYERS; i++)
        doom->players[i].playerstate = PST_REBORN;

    doom->usergame = true; // will be set false if a demo
    doom->paused = false;
    doom->demoplayback = false;
    doom->automapactive = false;
    doom->viewactive = true;
    doom->gameepisode = episode;
    doom->gamemap = map;
    doom->gameskill = skill;

    // Set the sky to use.
    //
    // Note: This IS broken, but it is how Vanilla Doom behaves.
    // See http://doomwiki.org/wiki/Sky_never_changes_in_Doom_II.
    //
    // Because we set the sky here at the start of a game, not at the
    // start of a level, the sky texture never changes unless we
    // restore from a saved game.  This was fixed before the Doom
    // source release, but this IS the way Vanilla DOS Doom behaves.

    if (doom->gamemode == commercial)
    {
        if (doom->gamemap < 12)
            skytexturename = "SKY1";
        else if (doom->gamemap < 21)
            skytexturename = "SKY2";
        else
            skytexturename = "SKY3";
    }
    else
    {
        switch (doom->gameepisode)
        {
        default:
        case 1:
            skytexturename = "SKY1";
            break;
        case 2:
            skytexturename = "SKY2";
            break;
        case 3:
            skytexturename = "SKY3";
            break;
        case 4: // Special Edition sky
            skytexturename = "SKY4";
            break;
        }
    }

    skytexturename = DEH_String(skytexturename);

    skytexture = R_TextureNumForName(skytexturename);

    G_DoLoadLevel(doom);
}

//
// DEMO RECORDING
//
#define DEMOMARKER 0x80

void G_ReadDemoTiccmd(doom_data_t *doom, ticcmd_t *cmd)
{
    if (*doom->demo_p == DEMOMARKER)
    {
        // end of demo data stream
        G_CheckDemoStatus(doom);
        return;
    }
    cmd->forwardmove = ((signed char)*doom->demo_p++);
    cmd->sidemove = ((signed char)*doom->demo_p++);

    // If this is a longtics demo, read back in higher resolution

    if (doom->longtics)
    {
        cmd->angleturn = *doom->demo_p++;
        cmd->angleturn |= (*doom->demo_p++) << 8;
    }
    else
    {
        cmd->angleturn = ((unsigned char)*doom->demo_p++) << 8;
    }

    cmd->buttons = (unsigned char)*doom->demo_p++;
}

// Increase the size of the demo buffer to allow unlimited demos

static void IncreaseDemoBuffer(doom_data_t* doom)
{
    int current_length;
    byte *new_demobuffer;
    byte *new_demop;
    int new_length;

    // Find the current size

    current_length = doom->demoend - doom->demobuffer;

    // Generate a new buffer twice the size
    new_length = current_length * 2;

    new_demobuffer = Z_Malloc(new_length, PU_STATIC, 0);
    new_demop = new_demobuffer + (doom->demo_p - doom->demobuffer);

    // Copy over the old data

    d_memcpy(new_demobuffer, doom->demobuffer, current_length);

    // Free the old buffer and point the demo pointers at the new buffer.

    Z_Free(doom->demobuffer);

    doom->demobuffer = new_demobuffer;
    doom->demo_p = new_demop;
    doom->demoend = doom->demobuffer + new_length;
}

void G_WriteDemoTiccmd(doom_data_t *doom, ticcmd_t *cmd)
{
    byte *demo_start;

    if (doom->gamekeydown[key_demo_quit]) // press q to end demo recording
        G_CheckDemoStatus(doom);

    demo_start = doom->demo_p;

    *doom->demo_p++ = cmd->forwardmove;
    *doom->demo_p++ = cmd->sidemove;

    // If this is a longtics demo, record in higher resolution

    if (doom->longtics)
    {
        *doom->demo_p++ = (cmd->angleturn & 0xff);
        *doom->demo_p++ = (cmd->angleturn >> 8) & 0xff;
    }
    else
    {
        *doom->demo_p++ = cmd->angleturn >> 8;
    }

    *doom->demo_p++ = cmd->buttons;

    // reset demo pointer back
    doom->demo_p = demo_start;

    if (doom->demo_p > doom->demoend - 16)
    {
        if (doom->vanilla_demo_limit)
        {
            // no more space
            G_CheckDemoStatus(doom);
            return;
        }
        else
        {
            // Vanilla demo limit disabled: unlimited
            // demo lengths!

            IncreaseDemoBuffer(doom);
        }
    }

    G_ReadDemoTiccmd(doom, cmd); // make SURE it is exactly the same
}

//
// G_RecordDemo
//
void G_RecordDemo(struct doom_data_t_* doom, char *name)
{
    size_t demoname_size;
    int i;
    int maxsize;

    doom->usergame = false;
    demoname_size = d_strlen(name) + 5;
    doom->demoname = Z_Malloc(demoname_size, PU_STATIC, NULL);
    d_snprintf(doom->demoname, demoname_size, "%s.lmp", name);
    maxsize = 0x20000;

    //!
    // @arg <size>
    // @category demo
    // @vanilla
    //
    // Specify the demo buffer size (KiB)
    //

    i = M_CheckParmWithArgs(doom, "-maxdemo", 1);
    if (i)
        maxsize = d_atoi(doom->myargv[i + 1]) * 1024;
    doom->demobuffer = Z_Malloc(maxsize, PU_STATIC, NULL);
    doom->demoend = doom->demobuffer + maxsize;

    doom->demorecording = true;
}

// Get the demo version code appropriate for the version set in gameversion.
int G_VanillaVersionCode(doom_data_t *doom)
{
    switch (doom->gameversion)
    {
    case exe_doom_1_2:
        I_Error("Doom 1.2 does not have a version code!");
    case exe_doom_1_666:
        return 106;
    case exe_doom_1_7:
        return 107;
    case exe_doom_1_8:
        return 108;
    case exe_doom_1_9:
    default: // All other versions are variants on v1.9:
        return 109;
    }
}

void G_BeginRecording(doom_data_t *doom)
{
    int i;

    //!
    // @category demo
    //
    // Record a high resolution "Doom 1.91" demo.
    //

    doom->longtics = M_CheckParm(doom, "-longtics") != 0;

    // If not recording a longtics demo, record in low res

    doom->lowres_turn = !doom->longtics;

    doom->demo_p = doom->demobuffer;

    // Save the right version code for this demo

    if (doom->longtics)
    {
        *doom->demo_p++ = DOOM_191_VERSION;
    }
    else
    {
        *doom->demo_p++ = G_VanillaVersionCode(doom);
    }

    *doom->demo_p++ = doom->gameskill;
    *doom->demo_p++ = doom->gameepisode;
    *doom->demo_p++ = doom->gamemap;
    *doom->demo_p++ = doom->deathmatch;
    *doom->demo_p++ = doom->respawnparm;
    *doom->demo_p++ = doom->fastparm;
    *doom->demo_p++ = doom->nomonsters;
    *doom->demo_p++ = doom->consoleplayer;

    for (i = 0; i < MAXPLAYERS; i++)
        *doom->demo_p++ = doom->playeringame[i];
}

//
// G_PlayDemo
//

char *defdemoname;

void G_DeferedPlayDemo(struct doom_data_t_* doom, char *name)
{
    defdemoname = name;
    doom->gameaction = ga_playdemo;
}

// Generate a string describing a demo version

static char *DemoVersionDescription(int version)
{
    static char resultbuf[16];

    switch (version)
    {
    case 104:
        return "v1.4";
    case 105:
        return "v1.5";
    case 106:
        return "v1.6/v1.666";
    case 107:
        return "v1.7/v1.7a";
    case 108:
        return "v1.8";
    case 109:
        return "v1.9";
    default:
        break;
    }

    // Unknown version.  Perhaps this is a pre-v1.4 IWAD?  If the version
    // byte is in the range 0-4 then it can be a v1.0-v1.2 demo.

    if (version >= 0 && version <= 4)
    {
        return "v1.0/v1.1/v1.2";
    }
    else
    {
        d_snprintf(resultbuf, sizeof(resultbuf),
                   "%i.%i (unknown)", version / 100, version % 100);
        return resultbuf;
    }
}

void G_DoPlayDemo(doom_data_t *doom)
{
    skill_t skill;
    int i, episode, map;
    int demoversion;

    doom->gameaction = ga_nothing;
    doom->demobuffer = doom->demo_p = W_CacheLumpName(doom, defdemoname, PU_STATIC);

    demoversion = *doom->demo_p++;

    if (demoversion == G_VanillaVersionCode(doom))
    {
        doom->longtics = false;
    }
    else if (demoversion == DOOM_191_VERSION)
    {
        // demo recorded with cph's modified "v1.91" doom exe
        doom->longtics = true;
    }
    else
    {
        char *message = "Demo is from a different game version!\n"
                        "(read %i, should be %i)\n"
                        "\n"
                        "*** You may need to upgrade your version "
                        "of Doom to v1.9. ***\n"
                        "    See: https://www.doomworld.com/classicdoom"
                        "/info/patches.php\n"
                        "    This appears to be %s.";

        // I_Error(message, demoversion, G_VanillaVersionCode(),
        d_printf(message, demoversion, G_VanillaVersionCode(doom),
                 DemoVersionDescription(demoversion));
    }

    skill = *doom->demo_p++;
    episode = *doom->demo_p++;
    map = *doom->demo_p++;
    doom->deathmatch = *doom->demo_p++;
    doom->respawnparm = *doom->demo_p++;
    doom->fastparm = *doom->demo_p++;
    doom->nomonsters = *doom->demo_p++;
    doom->consoleplayer = *doom->demo_p++;

    for (i = 0; i < MAXPLAYERS; i++)
        doom->playeringame[i] = *doom->demo_p++;

    if (doom->playeringame[1] || M_CheckParm(doom, "-solo-net") > 0 || M_CheckParm(doom, "-netdemo") > 0)
    {
        doom->netgame = true;
        doom->netdemo = true;
    }

    // don't spend a lot of time in loadlevel
    doom->precache = false;
    G_InitNew(doom, skill, episode, map);
    doom->precache = true;

    doom->usergame = false;
    doom->demoplayback = true;
}

//
// G_TimeDemo
//
void G_TimeDemo(doom_data_t *doom, char *name)
{
    //!
    // @vanilla
    //
    // Disable rendering the screen entirely.
    //

    doom->nodrawers = M_CheckParm(doom, "-nodraw");

    doom->timingdemo = true;
    doom->singletics = true;

    defdemoname = name;
   doom-> gameaction = ga_playdemo;
}

/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

boolean G_CheckDemoStatus(doom_data_t *doom)
{
    int endtime;

    if (doom->demoplayback)
    {
        W_ReleaseLumpName(doom, defdemoname);
        doom->demoplayback = false;
        doom->netdemo = false;
        doom->netgame = false;
        doom->deathmatch = false;
        doom->playeringame[1] = doom->playeringame[2] = doom->playeringame[3] = 0;
        doom->respawnparm = false;
        doom->fastparm = false;
        doom->nomonsters = false;
        doom->consoleplayer = 0;

        if (doom->singledemo)
            I_Quit(doom);
        else
            D_AdvanceDemo(doom);

        return true;
    }

    if (doom->demorecording)
    {
        *doom->demo_p++ = DEMOMARKER;
        M_WriteFile(doom->demoname, doom->demobuffer, doom->demo_p - doom->demobuffer);
        Z_Free(doom->demobuffer);
        doom->demorecording = false;
        I_Error("Demo %s recorded", doom->demoname);
    }

    return false;
}
