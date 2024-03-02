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
//	Status bar code.
//	Does the face/direction indicator animatin.
//	Does palette indicators as well (red pain/berserk, bright pickup)
//

#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "m_misc.h"
#include "m_random.h"
#include "w_wad.h"

#include "deh_main.h"
#include "deh_misc.h"
#include "doomdef.h"
#include "doomkeys.h"

#include "g_game.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"

#include "p_local.h"
#include "p_inter.h"

#include "am_map.h"
#include "m_cheat.h"

#include "s_sound.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"
#include "d_player.h"

//
// STATUS BAR DATA
//

// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS 1
#define STARTBONUSPALS 9
#define NUMREDPALS 8
#define NUMBONUSPALS 4
// Radiation suit, green shift.
#define RADIATIONPAL 13

// N/256*100% probability
//  that the normal face state will change
#define ST_FACEPROBABILITY 96

// For Responder
#define ST_TOGGLECHAT KEY_ENTER

// Location of status bar
#define ST_X 0
#define ST_X2 104

#define ST_FX 143
#define ST_FY 169

// Should be set to patch width
//  for tall numbers later on
#define ST_TALLNUMWIDTH (tallnum[0]->width)

#define ST_TURNOFFSET (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE (ST_NUMPAINFACES * ST_FACESTRIDE)
#define ST_DEADFACE (ST_GODFACE + 1)

#define ST_FACESX 143
#define ST_FACESY 168

#define ST_EVILGRINCOUNT (2 * TICRATE)
#define ST_STRAIGHTFACECOUNT (TICRATE / 2)
#define ST_TURNCOUNT (1 * TICRATE)
#define ST_OUCHCOUNT (1 * TICRATE)
#define ST_RAMPAGEDELAY (2 * TICRATE)

#define ST_MUCHPAIN 20

// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?

// AMMO number pos.
#define ST_AMMOWIDTH 3
#define ST_AMMOX 44
#define ST_AMMOY 171

// HEALTH number pos.
#define ST_HEALTHWIDTH 3
#define ST_HEALTHX 90
#define ST_HEALTHY 171

// Weapon pos.
#define ST_ARMSX 111
#define ST_ARMSY 172
#define ST_ARMSBGX 104
#define ST_ARMSBGY 168
#define ST_ARMSXSPACE 12
#define ST_ARMSYSPACE 10

// Frags pos.
#define ST_FRAGSX 138
#define ST_FRAGSY 171
#define ST_FRAGSWIDTH 2

// ARMOR number pos.
#define ST_ARMORWIDTH 3
#define ST_ARMORX 221
#define ST_ARMORY 171

// Key icon positions.
#define ST_KEY0WIDTH 8
#define ST_KEY0HEIGHT 5
#define ST_KEY0X 239
#define ST_KEY0Y 171
#define ST_KEY1WIDTH ST_KEY0WIDTH
#define ST_KEY1X 239
#define ST_KEY1Y 181
#define ST_KEY2WIDTH ST_KEY0WIDTH
#define ST_KEY2X 239
#define ST_KEY2Y 191

// Ammunition counter.
#define ST_AMMO0WIDTH 3
#define ST_AMMO0HEIGHT 6
#define ST_AMMO0X 288
#define ST_AMMO0Y 173
#define ST_AMMO1WIDTH ST_AMMO0WIDTH
#define ST_AMMO1X 288
#define ST_AMMO1Y 179
#define ST_AMMO2WIDTH ST_AMMO0WIDTH
#define ST_AMMO2X 288
#define ST_AMMO2Y 191
#define ST_AMMO3WIDTH ST_AMMO0WIDTH
#define ST_AMMO3X 288
#define ST_AMMO3Y 185

// Indicate maximum ammunition.
// Only needed because backpack exists.
#define ST_MAXAMMO0WIDTH 3
#define ST_MAXAMMO0HEIGHT 5
#define ST_MAXAMMO0X 314
#define ST_MAXAMMO0Y 173
#define ST_MAXAMMO1WIDTH ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X 314
#define ST_MAXAMMO1Y 179
#define ST_MAXAMMO2WIDTH ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X 314
#define ST_MAXAMMO2Y 191
#define ST_MAXAMMO3WIDTH ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X 314
#define ST_MAXAMMO3Y 185

// pistol
#define ST_WEAPON0X 110
#define ST_WEAPON0Y 172

// shotgun
#define ST_WEAPON1X 122
#define ST_WEAPON1Y 172

// chain gun
#define ST_WEAPON2X 134
#define ST_WEAPON2Y 172

// missile launcher
#define ST_WEAPON3X 110
#define ST_WEAPON3Y 181

// plasma gun
#define ST_WEAPON4X 122
#define ST_WEAPON4Y 181

// bfg
#define ST_WEAPON5X 134
#define ST_WEAPON5Y 181

// WPNS title
#define ST_WPNSX 109
#define ST_WPNSY 191

// DETH title
#define ST_DETHX 109
#define ST_DETHY 191

// Incoming messages window location
// UNUSED
//  #define ST_MSGTEXTX	   (viewwindowx)
//  #define ST_MSGTEXTY	   (viewwindowy+viewheight-18)
#define ST_MSGTEXTX 0
#define ST_MSGTEXTY 0
// Dimensions given in characters.
#define ST_MSGWIDTH 52
// Or shall I say, in lines?
#define ST_MSGHEIGHT 1

#define ST_OUTTEXTX 0
#define ST_OUTTEXTY 6

// Width, in characters again.
#define ST_OUTWIDTH 52
// Height, in lines.
#define ST_OUTHEIGHT 1

#define ST_MAPTITLEX \
    (SCREENWIDTH - ST_MAPWIDTH * ST_CHATFONTWIDTH)

#define ST_MAPTITLEY 0
#define ST_MAPHEIGHT 1

//
// STATUS BAR CODE
//
void ST_Stop(struct doom_data_t_* doom);

void ST_refreshBackground(struct doom_data_t_* doom)
{

    if (doom->st_statusbaron)
    {
        V_UseBuffer(doom, doom->st_backing_screen);

        V_DrawPatch(doom, ST_X, 0, doom->sbar);

        if (doom->netgame)
            V_DrawPatch(doom, ST_FX, 0, doom->faceback);

        V_RestoreBuffer(doom);

        V_CopyRect(doom, ST_X, 0, doom->st_backing_screen, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y);
    }
}

// Respond to keyboard input events,
//  intercept cheats.
boolean
ST_Responder(doom_data_t *doom, event_t *ev)
{
    int i;

    // Filter automap on/off.
    if (ev->type == ev_keyup && ((ev->data1 & 0xffff0000) == AM_MSGHEADER))
    {
        switch (ev->data1)
        {
        case AM_MSGENTERED:
            doom->st_gamestate = AutomapState;
            doom->st_firsttime = true;
            break;

        case AM_MSGEXITED:
            //	d_printf( "AM exited\n");
            doom->st_gamestate = FirstPersonState;
            break;
        }
    }

    // if a user keypress...
    else if (ev->type == ev_keydown)
    {
        if (!doom->netgame && doom->gameskill != sk_nightmare)
        {
            // 'dqd' cheat for toggleable god mode
            if (cht_CheckCheat(&doom->cheat_god, ev->data2))
            {
                doom->plyr->cheats ^= CF_GODMODE;
                if (doom->plyr->cheats & CF_GODMODE)
                {
                    if (doom->plyr->mo)
                        doom->plyr->mo->health = 100;

                    doom->plyr->health = deh_god_mode_health;
                    doom->plyr->message = DEH_String(STSTR_DQDON);
                }
                else
                    doom->plyr->message = DEH_String(STSTR_DQDOFF);
            }
            // 'fa' cheat for killer fucking arsenal
            else if (cht_CheckCheat(&doom->cheat_ammonokey, ev->data2))
            {
                doom->plyr->armorpoints = deh_idfa_armor;
                doom->plyr->armortype = deh_idfa_armor_class;

                for (i = 0; i < NUMWEAPONS; i++)
                    doom->plyr->weaponowned[i] = true;

                for (i = 0; i < NUMAMMO; i++)
                    doom->plyr->ammo[i] = doom->plyr->maxammo[i];

                doom->plyr->message = DEH_String(STSTR_FAADDED);
            }
            // 'kfa' cheat for key full ammo
            else if (cht_CheckCheat(&doom->cheat_ammo, ev->data2))
            {
                doom->plyr->armorpoints = deh_idkfa_armor;
                doom->plyr->armortype = deh_idkfa_armor_class;

                for (i = 0; i < NUMWEAPONS; i++)
                    doom->plyr->weaponowned[i] = true;

                for (i = 0; i < NUMAMMO; i++)
                    doom->plyr->ammo[i] = doom->plyr->maxammo[i];

                for (i = 0; i < NUMCARDS; i++)
                    doom->plyr->cards[i] = true;

                doom->plyr->message = DEH_String(STSTR_KFAADDED);
            }
            // 'mus' cheat for changing music
            else if (cht_CheckCheat(&doom->cheat_mus, ev->data2))
            {

                char buf[3];
                int musnum;

                doom->plyr->message = DEH_String(STSTR_MUS);
                cht_GetParam(&doom->cheat_mus, buf);

                // Note: The original v1.9 had a bug that tried to play back
                // the Doom II music regardless of gamemode.  This was fixed
                // in the Ultimate Doom executable so that it would work for
                // the Doom 1 music as well.

                if (doom->gamemode == commercial || doom->gameversion < exe_ultimate)
                {
                    musnum = mus_runnin + (buf[0] - '0') * 10 + buf[1] - '0' - 1;

                    if (((buf[0] - '0') * 10 + buf[1] - '0') > 35)
                        doom->plyr->message = DEH_String(STSTR_NOMUS);
                    else
                        S_ChangeMusic(doom, musnum, 1);
                }
                else
                {
                    musnum = mus_e1m1 + (buf[0] - '1') * 9 + (buf[1] - '1');

                    if (((buf[0] - '1') * 9 + buf[1] - '1') > 31)
                        doom->plyr->message = DEH_String(STSTR_NOMUS);
                    else
                        S_ChangeMusic(doom, musnum, 1);
                }
            }
            else if ((logical_gamemission == doom1 && cht_CheckCheat(&doom->cheat_noclip, ev->data2)) || (logical_gamemission != doom1 && cht_CheckCheat(&doom->cheat_commercial_noclip, ev->data2)))
            {
                // Noclip cheat.
                // For Doom 1, use the idspipsopd cheat; for all others, use
                // idclip

                doom->plyr->cheats ^= CF_NOCLIP;

                if (doom->plyr->cheats & CF_NOCLIP)
                    doom->plyr->message = DEH_String(STSTR_NCON);
                else
                    doom->plyr->message = DEH_String(STSTR_NCOFF);
            }
            // 'behold?' power-up cheats
            for (i = 0; i < 6; i++)
            {
                if (cht_CheckCheat(&doom->cheat_powerup[i], ev->data2))
                {
                    if (!doom->plyr->powers[i])
                        P_GivePower(doom->plyr, i);
                    else if (i != pw_strength)
                        doom->plyr->powers[i] = 1;
                    else
                        doom->plyr->powers[i] = 0;

                    doom->plyr->message = DEH_String(STSTR_BEHOLDX);
                }
            }

            // 'behold' power-up menu
            if (cht_CheckCheat(&doom->cheat_powerup[6], ev->data2))
            {
                doom->plyr->message = DEH_String(STSTR_BEHOLD);
            }
            // 'choppers' invulnerability & chainsaw
            else if (cht_CheckCheat(&doom->cheat_choppers, ev->data2))
            {
                doom->plyr->weaponowned[wp_chainsaw] = true;
                doom->plyr->powers[pw_invulnerability] = true;
                doom->plyr->message = DEH_String(STSTR_CHOPPERS);
            }
            // 'mypos' for player position
            else if (cht_CheckCheat(&doom->cheat_mypos, ev->data2))
            {
                static char buf[ST_MSGWIDTH];
                d_snprintf(buf, sizeof(buf), "ang=0x%x;x,y=(0x%x,0x%x)",
                           doom->players[doom->consoleplayer].mo->angle,
                           doom->players[doom->consoleplayer].mo->x,
                           doom->players[doom->consoleplayer].mo->y);
                doom->plyr->message = buf;
            }
        }

        // 'clev' change-level cheat
        if (!doom->netgame && cht_CheckCheat(&doom->cheat_clev, ev->data2))
        {
            char buf[3];
            int epsd;
            int map;

            cht_GetParam(&doom->cheat_clev, buf);

            if (doom->gamemode == commercial)
            {
                epsd = 1;
                map = (buf[0] - '0') * 10 + buf[1] - '0';
            }
            else
            {
                epsd = buf[0] - '0';
                map = buf[1] - '0';
            }

            // Chex.exe always warps to episode 1.

            if (doom->gameversion == exe_chex)
            {
                epsd = 1;
            }

            // Catch invalid maps.
            if (epsd < 1)
                return false;

            if (map < 1)
                return false;

            // Ohmygod - this is not going to work.
            if ((doom->gamemode == retail) && ((epsd > 4) || (map > 9)))
                return false;

            if ((doom->gamemode == registered) && ((epsd > 3) || (map > 9)))
                return false;

            if ((doom->gamemode == shareware) && ((epsd > 1) || (map > 9)))
                return false;

            // The source release has this check as map > 34. However, Vanilla
            // Doom allows IDCLEV up to MAP40 even though it normally crashes.
            if ((doom->gamemode == commercial) && ((epsd > 1) || (map > 40)))
                return false;

            // So be it.
            doom->plyr->message = DEH_String(STSTR_CLEV);
            G_DeferedInitNew(doom, doom->gameskill, epsd, map);
        }
    }
    return false;
}

int ST_calcPainOffset(struct doom_data_t_* doom)
{
    int health;

    health = doom->plyr->health > 100 ? 100 : doom->plyr->health;

    if (health != doom->oldhealth)
    {
        doom->lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
        doom->oldhealth = health;
    }
    return doom->lastcalc;
}

//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
void ST_updateFaceWidget(struct doom_data_t_* doom)
{
    int i;
    angle_t badguyangle;
    angle_t diffang;
    boolean doevilgrin;

    if (doom->priority < 10)
    {
        // dead
        if (!doom->plyr->health)
        {
            doom->priority = 9;
            doom->st_faceindex = ST_DEADFACE;
            doom->st_facecount = 1;
        }
    }

    if (doom->priority < 9)
    {
        if (doom->plyr->bonuscount)
        {
            // picking up bonus
            doevilgrin = false;

            for (i = 0; i < NUMWEAPONS; i++)
            {
                if (doom->oldweaponsowned[i] != doom->plyr->weaponowned[i])
                {
                    doevilgrin = true;
                    doom->oldweaponsowned[i] = doom->plyr->weaponowned[i];
                }
            }
            if (doevilgrin)
            {
                // evil grin if just picked up weapon
                doom->priority = 8;
                doom->st_facecount = ST_EVILGRINCOUNT;
                doom->st_faceindex = ST_calcPainOffset(doom) + ST_EVILGRINOFFSET;
            }
        }
    }

    if (doom->priority < 8)
    {
        if (doom->plyr->damagecount && doom->plyr->attacker && doom->plyr->attacker != doom->plyr->mo)
        {
            // being attacked
            doom->priority = 7;

            if (doom->plyr->health - doom->st_oldhealth > ST_MUCHPAIN)
            {
                doom->st_facecount = ST_TURNCOUNT;
                doom->st_faceindex = ST_calcPainOffset(doom) + ST_OUCHOFFSET;
            }
            else
            {
                badguyangle = R_PointToAngle2(doom->plyr->mo->x,
                                              doom->plyr->mo->y,
                                              doom->plyr->attacker->x,
                                              doom->plyr->attacker->y);

                if (badguyangle > doom->plyr->mo->angle)
                {
                    // whether right or left
                    diffang = badguyangle - doom->plyr->mo->angle;
                    i = diffang > ANG180;
                }
                else
                {
                    // whether left or right
                    diffang = doom->plyr->mo->angle - badguyangle;
                    i = diffang <= ANG180;
                } // confusing, aint it?

                doom->st_facecount = ST_TURNCOUNT;
                doom->st_faceindex = ST_calcPainOffset(doom);

                if (diffang < ANG45)
                {
                    // head-on
                    doom->st_faceindex += ST_RAMPAGEOFFSET;
                }
                else if (i)
                {
                    // turn face right
                    doom->st_faceindex += ST_TURNOFFSET;
                }
                else
                {
                    // turn face left
                    doom->st_faceindex += ST_TURNOFFSET + 1;
                }
            }
        }
    }

    if (doom->priority < 7)
    {
        // getting hurt because of your own damn stupidity
        if (doom->plyr->damagecount)
        {
            if (doom->plyr->health - doom->st_oldhealth > ST_MUCHPAIN)
            {
                doom->priority = 7;
                doom->st_facecount = ST_TURNCOUNT;
                doom->st_faceindex = ST_calcPainOffset(doom) + ST_OUCHOFFSET;
            }
            else
            {
                doom->priority = 6;
                doom->st_facecount = ST_TURNCOUNT;
                doom->st_faceindex = ST_calcPainOffset(doom) + ST_RAMPAGEOFFSET;
            }
        }
    }

    if (doom->priority < 6)
    {
        // rapid firing
        if (doom->plyr->attackdown)
        {
            if (doom->lastattackdown == -1)
                doom->lastattackdown = ST_RAMPAGEDELAY;
            else if (!--doom->lastattackdown)
            {
                doom->priority = 5;
                doom->st_faceindex = ST_calcPainOffset(doom) + ST_RAMPAGEOFFSET;
                doom->st_facecount = 1;
                doom->lastattackdown = 1;
            }
        }
        else
            doom->lastattackdown = -1;
    }

    if (doom->priority < 5)
    {
        // invulnerability
        if ((doom->plyr->cheats & CF_GODMODE) || doom->plyr->powers[pw_invulnerability])
        {
            doom->priority = 4;

            doom->st_faceindex = ST_GODFACE;
            doom->st_facecount = 1;
        }
    }

    // look left or look right if the facecount has timed out
    if (!doom->st_facecount)
    {
        doom->st_faceindex = ST_calcPainOffset(doom) + (doom->st_randomnumber % 3);
        doom->st_facecount = ST_STRAIGHTFACECOUNT;
        doom->priority = 0;
    }

    doom->st_facecount--;
}

void ST_updateWidgets(struct doom_data_t_* doom)
{
    int i;

    // must redirect the pointer if the ready weapon has changed.
    //  if (w_ready.data != plyr->readyweapon)
    //  {
    if (weaponinfo[doom->plyr->readyweapon].ammo == am_noammo)
        doom->w_ready.num = &doom->largeammo;
    else
        doom->w_ready.num = &doom->plyr->ammo[weaponinfo[doom->plyr->readyweapon].ammo];
    //{
    // static int tic=0;
    // static int dir=-1;
    // if (!(tic&15))
    //   plyr->ammo[weaponinfo[plyr->readyweapon].ammo]+=dir;
    // if (plyr->ammo[weaponinfo[plyr->readyweapon].ammo] == -100)
    //   dir = 1;
    // tic++;
    // }
    doom->w_ready.data = doom->plyr->readyweapon;

    // if (*w_ready.on)
    //  STlib_updateNum(&w_ready, true);
    // refresh weapon change
    //  }

    // update keycard multiple widgets
    for (i = 0; i < 3; i++)
    {
        doom->keyboxes[i] = doom->plyr->cards[i] ? i : -1;

        if (doom->plyr->cards[i + 3])
            doom->keyboxes[i] = i + 3;
    }

    // refresh everything if this is him coming back to life
    ST_updateFaceWidget(doom);

    // used by the w_armsbg widget
    doom->st_notdeathmatch = !doom->deathmatch;

    // used by w_arms[] widgets
    doom->st_armson = doom->st_statusbaron && !doom->deathmatch;

    // used by w_frags widget
    doom->st_fragson = doom->deathmatch && doom->st_statusbaron;
    doom->st_fragscount = 0;

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (i != doom->consoleplayer)
            doom->st_fragscount += doom->plyr->frags[i];
        else
            doom->st_fragscount -= doom->plyr->frags[i];
    }

    // get rid of chat window if up because of message
    if (!--doom->st_msgcounter)
        doom->st_chat = doom->st_oldchat;
}

void ST_Ticker(struct doom_data_t_* doom)
{

    doom->st_clock++;
    doom->st_randomnumber = M_Random();
    ST_updateWidgets(doom);
    doom->st_oldhealth = doom->plyr->health;
}

void ST_doPaletteStuff(doom_data_t *doom)
{

    int palette;
    byte *pal;
    int cnt;
    int bzc;

    cnt = doom->plyr->damagecount;

    if (doom->plyr->powers[pw_strength])
    {
        // slowly fade the berzerk out
        bzc = 12 - (doom->plyr->powers[pw_strength] >> 6);

        if (bzc > cnt)
            cnt = bzc;
    }

    if (cnt)
    {
        palette = (cnt + 7) >> 3;

        if (palette >= NUMREDPALS)
            palette = NUMREDPALS - 1;

        palette += STARTREDPALS;
    }

    else if (doom->plyr->bonuscount)
    {
        palette = (doom->plyr->bonuscount + 7) >> 3;

        if (palette >= NUMBONUSPALS)
            palette = NUMBONUSPALS - 1;

        palette += STARTBONUSPALS;
    }

    else if (doom->plyr->powers[pw_ironfeet] > 4 * 32 || doom->plyr->powers[pw_ironfeet] & 8)
        palette = RADIATIONPAL;
    else
        palette = 0;

    // In Chex Quest, the player never sees red.  Instead, the
    // radiation suit palette is used to tint the screen green,
    // as though the player is being covered in goo by an
    // attacking flemoid.

    if (doom->gameversion == exe_chex && palette >= STARTREDPALS && palette < STARTREDPALS + NUMREDPALS)
    {
        palette = RADIATIONPAL;
    }

    if (palette != doom->st_palette)
    {
        doom->st_palette = palette;
        pal = (byte *)W_CacheLumpNum(doom, doom->lu_palette, PU_CACHE) + palette * 768;
        I_SetPalette(pal);
    }
}

void ST_drawWidgets(struct doom_data_t_* doom, boolean refresh)
{
    int i;

    // used by w_arms[] widgets
    doom->st_armson = doom->st_statusbaron && !doom->deathmatch;

    // used by w_frags widget
    doom->st_fragson = doom->deathmatch && doom->st_statusbaron;

    STlib_updateNum(doom, &doom->w_ready, refresh);

    for (i = 0; i < 4; i++)
    {
        STlib_updateNum(doom, &doom->w_ammo[i], refresh);
        STlib_updateNum(doom, &doom->w_maxammo[i], refresh);
    }

    STlib_updatePercent(doom, &doom->w_health, refresh);
    STlib_updatePercent(doom, &doom->w_armor, refresh);

    STlib_updateBinIcon(doom, &doom->w_armsbg, refresh);

    for (i = 0; i < 6; i++)
        STlib_updateMultIcon(doom, &doom->w_arms[i], refresh);

    STlib_updateMultIcon(doom, &doom->w_faces, refresh);

    for (i = 0; i < 3; i++)
        STlib_updateMultIcon(doom, &doom->w_keyboxes[i], refresh);

    STlib_updateNum(doom, &doom->w_frags, refresh);
}

void ST_doRefresh(struct doom_data_t_* doom)
{

    doom->st_firsttime = false;

    // draw status bar background to off-screen buff
    ST_refreshBackground(doom);

    // and refresh all widgets
    ST_drawWidgets(doom, true);
}

void ST_diffDraw(struct doom_data_t_* doom)
{
    // update all widgets
    ST_drawWidgets(doom, false);
}

void ST_Drawer(doom_data_t *doom, boolean fullscreen, boolean refresh)
{

    doom->st_statusbaron = (!fullscreen) || doom->automapactive;
    doom->st_firsttime = doom->st_firsttime || refresh;

    // Do red-/gold-shifts from damage/items
    ST_doPaletteStuff(doom);

    // If just after ST_Start(), refresh all
    if (doom->st_firsttime)
        ST_doRefresh(doom);
    // Otherwise, update as little as possible
    else
        ST_diffDraw(doom);
}

typedef void (*load_callback_t)(struct doom_data_t_* doom, char *lumpname, patch_t **variable);

// Iterates through all graphics to be loaded or unloaded, along with
// the variable they use, invoking the specified callback function.

static void ST_loadUnloadGraphics(struct doom_data_t_* doom, load_callback_t callback)
{

    int i;
    int j;
    int facenum;

    char namebuf[9];

    // Load the numbers, tall and short
    for (i = 0; i < 10; i++)
    {
        d_snprintf(namebuf, 9, "STTNUM%d", i);
        callback(doom, namebuf, &doom->tallnum[i]);

        d_snprintf(namebuf, 9, "STYSNUM%d", i);
        callback(doom, namebuf, &doom->shortnum[i]);
    }

    // Load percent key.
    // Note: why not load STMINUS here, too?

    callback(doom, DEH_String("STTPRCNT"), &doom->tallpercent);

    // key cards
    for (i = 0; i < NUMCARDS; i++)
    {
        d_snprintf(namebuf, 9, "STKEYS%d", i);
        callback(doom, namebuf, &doom->keys[i]);
    }

    // arms background
    callback(doom, DEH_String("STARMS"), &doom->armsbg);

    // arms ownership widgets
    for (i = 0; i < 6; i++)
    {
        d_snprintf(namebuf, 9, "STGNUM%d", i + 2);

        // gray #
        callback(doom, namebuf, &doom->arms[i][0]);

        // yellow #
        doom->arms[i][1] = doom->shortnum[i + 2];
    }

    // face backgrounds for different color players
    d_snprintf(namebuf, 9, "STFB%d", doom->consoleplayer);
    callback(doom, namebuf, &doom->faceback);

    // status bar background bits
    callback(doom, DEH_String("STBAR"), &doom->sbar);

    // face states
    facenum = 0;
    for (i = 0; i < ST_NUMPAINFACES; i++)
    {
        for (j = 0; j < ST_NUMSTRAIGHTFACES; j++)
        {
            d_snprintf(namebuf, 9, "STFST%d%d", i, j);
            callback(doom, namebuf, &doom->faces[facenum]);
            ++facenum;
        }
        d_snprintf(namebuf, 9, "STFTR%d0", i); // turn right
        callback(doom, namebuf, &doom->faces[facenum]);
        ++facenum;
        d_snprintf(namebuf, 9, "STFTL%d0", i); // turn left
        callback(doom, namebuf, &doom->faces[facenum]);
        ++facenum;
        d_snprintf(namebuf, 9, "STFOUCH%d", i); // ouch!
        callback(doom, namebuf, &doom->faces[facenum]);
        ++facenum;
        d_snprintf(namebuf, 9, "STFEVL%d", i); // evil grin ;)
        callback(doom, namebuf, &doom->faces[facenum]);
        ++facenum;
        d_snprintf(namebuf, 9, "STFKILL%d", i); // pissed off
        callback(doom, namebuf, &doom->faces[facenum]);
        ++facenum;
    }

    callback(doom, DEH_String("STFGOD0"), &doom->faces[facenum]);
    ++facenum;
    callback(doom, DEH_String("STFDEAD0"), &doom->faces[facenum]);
    ++facenum;
}

static void ST_loadCallback(struct doom_data_t_* doom, char *lumpname, patch_t **variable)
{
    *variable = W_CacheLumpName(doom, lumpname, PU_STATIC);
}

void ST_loadGraphics(struct doom_data_t_* doom)
{
    ST_loadUnloadGraphics(doom, ST_loadCallback);
}

void ST_loadData(struct doom_data_t_* doom)
{
    doom->lu_palette = W_GetNumForName(doom, DEH_String("PLAYPAL"));
    ST_loadGraphics(doom);
}

static void ST_unloadCallback(struct doom_data_t_* doom, char *lumpname, patch_t **variable)
{
    W_ReleaseLumpName(doom, lumpname);
    *variable = NULL;
}

void ST_unloadGraphics(struct doom_data_t_* doom)
{
    ST_loadUnloadGraphics(doom, ST_unloadCallback);
}

void ST_unloadData(struct doom_data_t_* doom)
{
    ST_unloadGraphics(doom);
}

void ST_initData(doom_data_t *doom)
{

    int i;

    doom->st_firsttime = true;
    doom->plyr = &doom->players[doom->consoleplayer];

    doom->st_clock = 0;
    doom->st_chatstate = StartChatState;
    doom->st_gamestate = FirstPersonState;

    doom->st_statusbaron = true;
    doom->st_oldchat = doom->st_chat = false;
    doom->st_cursoron = false;

    doom->st_faceindex = 0;
    doom->st_palette = -1;

    doom->st_oldhealth = -1;

    for (i = 0; i < NUMWEAPONS; i++)
        doom->oldweaponsowned[i] = doom->plyr->weaponowned[i];

    for (i = 0; i < 3; i++)
        doom->keyboxes[i] = -1;

    STlib_init(doom);
}

void ST_createWidgets(struct doom_data_t_* doom)
{

    int i;

    // ready weapon ammo
    STlib_initNum(doom, &doom->w_ready,
                  ST_AMMOX,
                  ST_AMMOY,
                  doom->tallnum,
                  &doom->plyr->ammo[weaponinfo[doom->plyr->readyweapon].ammo],
                  &doom->st_statusbaron,
                  ST_AMMOWIDTH);

    // the last weapon type
    doom->w_ready.data = doom->plyr->readyweapon;

    // health percentage
    STlib_initPercent(doom, &doom->w_health,
                      ST_HEALTHX,
                      ST_HEALTHY,
                      doom->tallnum,
                      &doom->plyr->health,
                      &doom->st_statusbaron,
                      doom->tallpercent);

    // arms background
    STlib_initBinIcon(doom, &doom->w_armsbg,
                      ST_ARMSBGX,
                      ST_ARMSBGY,
                      doom->armsbg,
                      &doom->st_notdeathmatch,
                      &doom->st_statusbaron);

    // weapons owned
    for (i = 0; i < 6; i++)
    {
        STlib_initMultIcon(doom, &doom->w_arms[i],
                           ST_ARMSX + (i % 3) * ST_ARMSXSPACE,
                           ST_ARMSY + (i / 3) * ST_ARMSYSPACE,
                           doom->arms[i], (int *)&doom->plyr->weaponowned[i + 1],
                           &doom->st_armson);
    }

    // frags sum
    STlib_initNum(doom, &doom->w_frags,
                  ST_FRAGSX,
                  ST_FRAGSY,
                  doom->tallnum,
                  &doom->st_fragscount,
                  &doom->st_fragson,
                  ST_FRAGSWIDTH);

    // faces
    STlib_initMultIcon(doom, &doom->w_faces,
                       ST_FACESX,
                       ST_FACESY,
                       doom->faces,
                       &doom->st_faceindex,
                       &doom->st_statusbaron);

    // armor percentage - should be colored later
    STlib_initPercent(doom, &doom->w_armor,
                      ST_ARMORX,
                      ST_ARMORY,
                      doom->tallnum,
                      &doom->plyr->armorpoints,
                      &doom->st_statusbaron, doom->tallpercent);

    // keyboxes 0-2
    STlib_initMultIcon(doom, &doom->w_keyboxes[0],
                       ST_KEY0X,
                       ST_KEY0Y,
                       doom->keys,
                       &doom->keyboxes[0],
                       &doom->st_statusbaron);

    STlib_initMultIcon(doom, &doom->w_keyboxes[1],
                       ST_KEY1X,
                       ST_KEY1Y,
                       doom->keys,
                       &doom->keyboxes[1],
                       &doom->st_statusbaron);

    STlib_initMultIcon(doom, &doom->w_keyboxes[2],
                       ST_KEY2X,
                       ST_KEY2Y,
                       doom->keys,
                       &doom->keyboxes[2],
                       &doom->st_statusbaron);

    // ammo count (all four kinds)
    STlib_initNum(doom, &doom->w_ammo[0],
                  ST_AMMO0X,
                  ST_AMMO0Y,
                  doom->shortnum,
                  &doom->plyr->ammo[0],
                  &doom->st_statusbaron,
                  ST_AMMO0WIDTH);

    STlib_initNum(doom, &doom->w_ammo[1],
                  ST_AMMO1X,
                  ST_AMMO1Y,
                  doom->shortnum,
                  &doom->plyr->ammo[1],
                  &doom->st_statusbaron,
                  ST_AMMO1WIDTH);

    STlib_initNum(doom, &doom->w_ammo[2],
                  ST_AMMO2X,
                  ST_AMMO2Y,
                  doom->shortnum,
                  &doom->plyr->ammo[2],
                  &doom->st_statusbaron,
                  ST_AMMO2WIDTH);

    STlib_initNum(doom, &doom->w_ammo[3],
                  ST_AMMO3X,
                  ST_AMMO3Y,
                  doom->shortnum,
                  &doom->plyr->ammo[3],
                  &doom->st_statusbaron,
                  ST_AMMO3WIDTH);

    // max ammo count (all four kinds)
    STlib_initNum(doom, &doom->w_maxammo[0],
                  ST_MAXAMMO0X,
                  ST_MAXAMMO0Y,
                  doom->shortnum,
                  &doom->plyr->maxammo[0],
                  &doom->st_statusbaron,
                  ST_MAXAMMO0WIDTH);

    STlib_initNum(doom, &doom->w_maxammo[1],
                  ST_MAXAMMO1X,
                  ST_MAXAMMO1Y,
                  doom->shortnum,
                  &doom->plyr->maxammo[1],
                  &doom->st_statusbaron,
                  ST_MAXAMMO1WIDTH);

    STlib_initNum(doom, &doom->w_maxammo[2],
                  ST_MAXAMMO2X,
                  ST_MAXAMMO2Y,
                  doom->shortnum,
                  &doom->plyr->maxammo[2],
                  &doom->st_statusbaron,
                  ST_MAXAMMO2WIDTH);

    STlib_initNum(doom, &doom->w_maxammo[3],
                  ST_MAXAMMO3X,
                  ST_MAXAMMO3Y,
                  doom->shortnum,
                  &doom->plyr->maxammo[3],
                  &doom->st_statusbaron,
                  ST_MAXAMMO3WIDTH);
}

void ST_Start(doom_data_t *doom)
{
    if (!doom->st_stopped)
        ST_Stop(doom);

    ST_initData(doom);
    ST_createWidgets(doom);
    doom->st_stopped = false;
}

void ST_Stop(struct doom_data_t_* doom)
{
    if (doom->st_stopped)
        return;

    I_SetPalette(W_CacheLumpNum(doom, doom->lu_palette, PU_CACHE));
    doom->st_stopped = true;
}

void ST_Init(struct doom_data_t_* doom)
{
    ST_loadData(doom);
    doom->st_backing_screen = (byte *)Z_Malloc(ST_WIDTH * ST_HEIGHT, PU_STATIC, 0);
}
