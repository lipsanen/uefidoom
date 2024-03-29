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
//
// DESCRIPTION:  the automap code
//

#include "dlibc.h"

#include "deh_main.h"

#include "z_zone.h"
#include "doomkeys.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"

#include "m_cheat.h"
#include "m_controls.h"
#include "m_misc.h"
#include "i_system.h"

// Needs access to LFB.
#include "v_video.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "dstrings.h"

#include "am_map.h"

// For use if I do walls with outsides/insides
#define REDS (256 - 5 * 16)
#define REDRANGE 16
#define BLUES (256 - 4 * 16 + 8)
#define BLUERANGE 8
#define GREENS (7 * 16)
#define GREENRANGE 16
#define GRAYS (6 * 16)
#define GRAYSRANGE 16
#define BROWNS (4 * 16)
#define BROWNRANGE 16
#define YELLOWS (256 - 32 + 7)
#define YELLOWRANGE 1
#define BLACK 0
#define WHITE (256 - 47)

// Automap colors
#define BACKGROUND BLACK
#define YOURCOLORS WHITE
#define YOURRANGE 0
#define WALLCOLORS REDS
#define WALLRANGE REDRANGE
#define TSWALLCOLORS GRAYS
#define TSWALLRANGE GRAYSRANGE
#define FDWALLCOLORS BROWNS
#define FDWALLRANGE BROWNRANGE
#define CDWALLCOLORS YELLOWS
#define CDWALLRANGE YELLOWRANGE
#define THINGCOLORS GREENS
#define THINGRANGE GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE WALLRANGE
#define GRIDCOLORS (GRAYS + GRAYSRANGE / 2)
#define GRIDRANGE 0
#define XHAIRCOLORS GRAYS

// drawing stuff

// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC 4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN ((int)(1.02 * FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT ((int)(FRACUNIT / 1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x) << 16), doom->scale_ftom)
#define MTOF(x) (FixedMul((x), doom->scale_mtof) >> 16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x) (doom->f_x + MTOF((x)-doom->m_x))
#define CYMTOF(y) (doom->f_y + (doom->f_h - MTOF((y)-doom->m_y)))

// the following is crap
#define LINE_NEVERSEE ML_DONTDRAW

//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8 * PLAYERRADIUS) / 7)
static const mline_t player_arrow[] = {
    {{-R + R / 8, 0}, {R, 0}},    // -----
    {{R, 0}, {R - R / 2, R / 4}}, // ----->
    {{R, 0}, {R - R / 2, -R / 4}},
    {{-R + R / 8, 0}, {-R - R / 8, R / 4}}, // >---->
    {{-R + R / 8, 0}, {-R - R / 8, -R / 4}},
    {{-R + 3 * R / 8, 0}, {-R + R / 8, R / 4}}, // >>--->
    {{-R + 3 * R / 8, 0}, {-R + R / 8, -R / 4}}};
#undef R

#define R ((8 * PLAYERRADIUS) / 7)
static const mline_t cheat_player_arrow[] = {
    {{-R + R / 8, 0}, {R, 0}},    // -----
    {{R, 0}, {R - R / 2, R / 6}}, // ----->
    {{R, 0}, {R - R / 2, -R / 6}},
    {{-R + R / 8, 0}, {-R - R / 8, R / 6}}, // >----->
    {{-R + R / 8, 0}, {-R - R / 8, -R / 6}},
    {{-R + 3 * R / 8, 0}, {-R + R / 8, R / 6}}, // >>----->
    {{-R + 3 * R / 8, 0}, {-R + R / 8, -R / 6}},
    {{-R / 2, 0}, {-R / 2, -R / 6}}, // >>-d--->
    {{-R / 2, -R / 6}, {-R / 2 + R / 6, -R / 6}},
    {{-R / 2 + R / 6, -R / 6}, {-R / 2 + R / 6, R / 4}},
    {{-R / 6, 0}, {-R / 6, -R / 6}}, // >>-dd-->
    {{-R / 6, -R / 6}, {0, -R / 6}},
    {{0, -R / 6}, {0, R / 4}},
    {{R / 6, R / 4}, {R / 6, -R / 7}}, // >>-ddt->
    {{R / 6, -R / 7}, {R / 6 + R / 32, -R / 7 - R / 32}},
    {{R / 6 + R / 32, -R / 7 - R / 32}, {R / 6 + R / 10, -R / 7}}};
#undef R

#define R (FRACUNIT)
static const mline_t triangle_guy[] = {
    {{(fixed_t)(-.867 * R), (fixed_t)(-.5 * R)}, {(fixed_t)(.867 * R), (fixed_t)(-.5 * R)}},
    {{(fixed_t)(.867 * R), (fixed_t)(-.5 * R)}, {(fixed_t)(0), (fixed_t)(R)}},
    {{(fixed_t)(0), (fixed_t)(R)}, {(fixed_t)(-.867 * R), (fixed_t)(-.5 * R)}}};
#undef R

#define R (FRACUNIT)
static const mline_t thintriangle_guy[] = {
    {{(fixed_t)(-.5 * R), (fixed_t)(-.7 * R)}, {(fixed_t)(R), (fixed_t)(0)}},
    {{(fixed_t)(R), (fixed_t)(0)}, {(fixed_t)(-.5 * R), (fixed_t)(.7 * R)}},
    {{(fixed_t)(-.5 * R), (fixed_t)(.7 * R)}, {(fixed_t)(-.5 * R), (fixed_t)(-.7 * R)}}};
#undef R

// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.

void AM_getIslope(doom_data_t *doom,
                  mline_t *ml,
                  islope_t *is)
{
    int dx, dy;

    dy = ml->a.y - ml->b.y;
    dx = ml->b.x - ml->a.x;
    if (!dy)
        is->islp = (dx < 0 ? -INT_MAX : INT_MAX);
    else
        is->islp = FixedDiv(dx, dy);
    if (!dx)
        is->slp = (dy < 0 ? -INT_MAX : INT_MAX);
    else
        is->slp = FixedDiv(dy, dx);
}

//
//
//
void AM_activateNewScale(doom_data_t *doom)
{
    doom->m_x += doom->m_w / 2;
    doom->m_y += doom->m_h / 2;
    doom->m_w = FTOM(doom->f_w);
    doom->m_h = FTOM(doom->f_h);
    doom->m_x -= doom->m_w / 2;
    doom->m_y -= doom->m_h / 2;
    doom->m_x2 = doom->m_x + doom->m_w;
    doom->m_y2 = doom->m_y + doom->m_h;
}

//
//
//
void AM_saveScaleAndLoc(doom_data_t *doom)
{
    doom->old_m_x = doom->m_x;
    doom->old_m_y = doom->m_y;
    doom->old_m_w = doom->m_w;
    doom->old_m_h = doom->m_h;
}

//
//
//
void AM_restoreScaleAndLoc(doom_data_t *doom)
{

    doom->m_w = doom->old_m_w;
    doom->m_h = doom->old_m_h;
    if (!doom->followplayer)
    {
        doom->m_x = doom->old_m_x;
        doom->m_y = doom->old_m_y;
    }
    else
    {
        doom->m_x = doom->plr->mo->x - doom->m_w / 2;
        doom->m_y = doom->plr->mo->y - doom->m_h / 2;
    }
    doom->m_x2 = doom->m_x + doom->m_w;
    doom->m_y2 = doom->m_y + doom->m_h;

    // Change the scaling multipliers
    doom->scale_mtof = FixedDiv(doom->f_w << FRACBITS, doom->m_w);
    doom->scale_ftom = FixedDiv(FRACUNIT, doom->scale_mtof);
}

//
// adds a marker at the current location
//
void AM_addMark(doom_data_t *doom)
{
    doom->markpoints[doom->markpointnum].x = doom->m_x + doom->m_w / 2;
    doom->markpoints[doom->markpointnum].y = doom->m_y + doom->m_h / 2;
    doom->markpointnum = (doom->markpointnum + 1) % AM_NUMMARKPOINTS;
}

//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
void AM_findMinMaxBoundaries(doom_data_t *doom)
{
    int i;
    fixed_t a;
    fixed_t b;

    doom->min_x = doom->min_y = INT_MAX;
    doom->max_x = doom->max_y = -INT_MAX;

    for (i = 0; i < numvertexes; i++)
    {
        if (vertexes[i].x < doom->min_x)
            doom->min_x = vertexes[i].x;
        else if (vertexes[i].x > doom->max_x)
            doom->max_x = vertexes[i].x;

        if (vertexes[i].y < doom->min_y)
            doom->min_y = vertexes[i].y;
        else if (vertexes[i].y > doom->max_y)
            doom->max_y = vertexes[i].y;
    }

    doom->max_w = doom->max_x - doom->min_x;
    doom->max_h = doom->max_y - doom->min_y;

    doom->min_w = 2 * PLAYERRADIUS; // const? never changed?
    doom->min_h = 2 * PLAYERRADIUS;

    a = FixedDiv(doom->f_w << FRACBITS, doom->max_w);
    b = FixedDiv(doom->f_h << FRACBITS, doom->max_h);

    doom->min_scale_mtof = a < b ? a : b;
    doom->max_scale_mtof = FixedDiv(doom->f_h << FRACBITS, 2 * PLAYERRADIUS);
}

//
//
//
void AM_changeWindowLoc(doom_data_t *doom)
{
    if (doom->m_paninc.x || doom->m_paninc.y)
    {
        doom->followplayer = 0;
        doom->f_oldloc.x = INT_MAX;
    }

    doom->m_x += doom->m_paninc.x;
    doom->m_y += doom->m_paninc.y;

    if (doom->m_x + doom->m_w / 2 > doom->max_x)
        doom->m_x = doom->max_x - doom->m_w / 2;
    else if (doom->m_x + doom->m_w / 2 < doom->min_x)
        doom->m_x = doom->min_x - doom->m_w / 2;

    if (doom->m_y + doom->m_h / 2 > doom->max_y)
        doom->m_y = doom->max_y - doom->m_h / 2;
    else if (doom->m_y + doom->m_h / 2 < doom->min_y)
        doom->m_y = doom->min_y - doom->m_h / 2;

    doom->m_x2 = doom->m_x + doom->m_w;
    doom->m_y2 = doom->m_y + doom->m_h;
}

//
//
//
void AM_initVariables(doom_data_t *doom)
{
    int pnum;
    event_t st_notify = {ev_keyup, AM_MSGENTERED, 0, 0};

    doom->automapactive = true;
    doom->fb = I_VideoBuffer;

    doom->f_oldloc.x = INT_MAX;
    doom->amclock = 0;
    doom->lightlev = 0;

    doom->m_paninc.x = doom->m_paninc.y = 0;
    doom->ftom_zoommul = FRACUNIT;
    doom->mtof_zoommul = FRACUNIT;

    doom->m_w = FTOM(doom->f_w);
    doom->m_h = FTOM(doom->f_h);

    // find player to center on initially
    if (doom->playeringame[doom->consoleplayer])
    {
        doom->plr = &doom->players[doom->consoleplayer];
    }
    else
    {
        doom->plr = &doom->players[0];

        for (pnum = 0; pnum < MAXPLAYERS; pnum++)
        {
            if (doom->playeringame[pnum])
            {
                doom->plr = &doom->players[pnum];
                break;
            }
        }
    }

    doom->m_x = doom->plr->mo->x - doom->m_w / 2;
    doom->m_y = doom->plr->mo->y - doom->m_h / 2;
    AM_changeWindowLoc(doom);

    // for saving & restoring
    doom->old_m_x = doom->m_x;
    doom->old_m_y = doom->m_y;
    doom->old_m_w = doom->m_w;
    doom->old_m_h = doom->m_h;

    // inform the status bar of the change
    ST_Responder(doom, &st_notify);
}

//
//
//
void AM_loadPics(doom_data_t *doom)
{
    int i;
    char namebuf[9];

    for (i = 0; i < 10; i++)
    {
        d_snprintf(namebuf, 9, "AMMNUM%d", i);
        doom->marknums[i] = W_CacheLumpName(doom, namebuf, PU_STATIC);
    }
}

void AM_unloadPics(doom_data_t *doom)
{
    int i;
    char namebuf[9];

    for (i = 0; i < 10; i++)
    {
        d_snprintf(namebuf, 9, "AMMNUM%d", i);
        W_ReleaseLumpName(doom, namebuf);
    }
}

void AM_clearMarks(doom_data_t *doom)
{
    int i;

    for (i = 0; i < AM_NUMMARKPOINTS; i++)
        doom->markpoints[i].x = -1; // means empty
    doom->markpointnum = 0;
}

//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(doom_data_t *doom)
{
    doom->leveljuststarted = 0;

    doom->f_x = doom->f_y = 0;
    doom->f_w = SCREENWIDTH;
    doom->f_h = SCREENHEIGHT - 32;

    AM_clearMarks(doom);

    AM_findMinMaxBoundaries(doom);
    doom->scale_mtof = FixedDiv(doom->min_scale_mtof, (int)(0.7 * FRACUNIT));
    if (doom->scale_mtof > doom->max_scale_mtof)
        doom->scale_mtof = doom->min_scale_mtof;
    doom->scale_ftom = FixedDiv(FRACUNIT, doom->scale_mtof);
}

//
//
//
void AM_Stop(doom_data_t *doom)
{
    event_t st_notify = {0, ev_keyup, AM_MSGEXITED, 0};

    AM_unloadPics(doom);
    doom->automapactive = false;
    ST_Responder(doom, &st_notify);
    doom->stopped = true;
}

//
//
//
void AM_Start(doom_data_t *doom)
{
    if (!doom->stopped)
        AM_Stop(doom);
    doom->stopped = false;
    if (doom->lastlevel != doom->gamemap || doom->lastepisode != doom->gameepisode)
    {
        AM_LevelInit(doom);
        doom->lastlevel = doom->gamemap;
        doom->lastepisode = doom->gameepisode;
    }
    AM_initVariables(doom);
    AM_loadPics(doom);
}

//
// set the window scale to the maximum size
//
static void AM_minOutWindowScale(doom_data_t *doom)
{
    doom->scale_mtof = doom->min_scale_mtof;
    doom->scale_ftom = FixedDiv(FRACUNIT, doom->scale_mtof);
    AM_activateNewScale(doom);
}

//
// set the window scale to the minimum size
//
static void AM_maxOutWindowScale(doom_data_t *doom)
{
    doom->scale_mtof = doom->max_scale_mtof;
    doom->scale_ftom = FixedDiv(FRACUNIT, doom->scale_mtof);
    AM_activateNewScale(doom);
}

//
// Handle events (user inputs) in automap mode
//
boolean
AM_Responder(doom_data_t *doom, event_t *ev)
{
    int rc;
    int bigstate = 0;
    char buffer[20];
    int key;

    rc = false;

    if (!doom->automapactive)
    {
        if (ev->type == ev_keydown && ev->data1 == key_map_toggle)
        {
            AM_Start(doom);
            doom->viewactive = false;
            rc = true;
        }
    }
    else if (ev->type == ev_keydown)
    {
        rc = true;
        key = ev->data1;

        if (key == key_map_east) // pan right
        {
            if (!doom->followplayer)
                doom->m_paninc.x = FTOM(F_PANINC);
            else
                rc = false;
        }
        else if (key == key_map_west) // pan left
        {
            if (!doom->followplayer)
                doom->m_paninc.x = -FTOM(F_PANINC);
            else
                rc = false;
        }
        else if (key == key_map_north) // pan up
        {
            if (!doom->followplayer)
                doom->m_paninc.y = FTOM(F_PANINC);
            else
                rc = false;
        }
        else if (key == key_map_south) // pan down
        {
            if (!doom->followplayer)
                doom->m_paninc.y = -FTOM(F_PANINC);
            else
                rc = false;
        }
        else if (key == key_map_zoomout) // zoom out
        {
            doom->mtof_zoommul = M_ZOOMOUT;
            doom->ftom_zoommul = M_ZOOMIN;
        }
        else if (key == key_map_zoomin) // zoom in
        {
            doom->mtof_zoommul = M_ZOOMIN;
            doom->ftom_zoommul = M_ZOOMOUT;
        }
        else if (key == key_map_toggle)
        {
            bigstate = 0;
            doom->viewactive = true;
            AM_Stop(doom);
        }
        else if (key == key_map_maxzoom)
        {
            bigstate = !bigstate;
            if (bigstate)
            {
                AM_saveScaleAndLoc(doom);
                AM_minOutWindowScale(doom);
            }
            else
                AM_restoreScaleAndLoc(doom);
        }
        else if (key == key_map_follow)
        {
            doom->followplayer = !doom->followplayer;
            doom->f_oldloc.x = INT_MAX;
            if (doom->followplayer)
                doom->plr->message = DEH_String(AMSTR_FOLLOWON);
            else
                doom->plr->message = DEH_String(AMSTR_FOLLOWOFF);
        }
        else if (key == key_map_grid)
        {
            doom->grid = !doom->grid;
            if (doom->grid)
                doom->plr->message = DEH_String(AMSTR_GRIDON);
            else
                doom->plr->message = DEH_String(AMSTR_GRIDOFF);
        }
        else if (key == key_map_mark)
        {
            d_snprintf(buffer, sizeof(buffer), "%s %d",
                       DEH_String(AMSTR_MARKEDSPOT), doom->markpointnum);
            doom->plr->message = buffer;
            AM_addMark(doom);
        }
        else if (key == key_map_clearmark)
        {
            AM_clearMarks(doom);
            doom->plr->message = DEH_String(AMSTR_MARKSCLEARED);
        }
        else
        {
            rc = false;
        }

        if (!doom->deathmatch && cht_CheckCheat(&doom->cheat_amap, ev->data2))
        {
            rc = false;
            doom->cheating = (doom->cheating + 1) % 3;
        }
    }
    else if (ev->type == ev_keyup)
    {
        rc = false;
        key = ev->data1;

        if (key == key_map_east)
        {
            if (!doom->followplayer)
                doom->m_paninc.x = 0;
        }
        else if (key == key_map_west)
        {
            if (!doom->followplayer)
                doom->m_paninc.x = 0;
        }
        else if (key == key_map_north)
        {
            if (!doom->followplayer)
                doom->m_paninc.y = 0;
        }
        else if (key == key_map_south)
        {
            if (!doom->followplayer)
                doom->m_paninc.y = 0;
        }
        else if (key == key_map_zoomout || key == key_map_zoomin)
        {
            doom->mtof_zoommul = FRACUNIT;
            doom->ftom_zoommul = FRACUNIT;
        }
    }

    return rc;
}

//
// Zooming
//
static void AM_changeWindowScale(doom_data_t *doom)
{
    // Change the scaling multipliers
    doom->scale_mtof = FixedMul(doom->scale_mtof, doom->mtof_zoommul);
    doom->scale_ftom = FixedDiv(FRACUNIT, doom->scale_mtof);

    if (doom->scale_mtof < doom->min_scale_mtof)
        AM_minOutWindowScale(doom);
    else if (doom->scale_mtof > doom->max_scale_mtof)
        AM_maxOutWindowScale(doom);
    else
        AM_activateNewScale(doom);
}

//
//
//
static void AM_doFollowPlayer(doom_data_t *doom)
{

    if (doom->f_oldloc.x != doom->plr->mo->x || doom->f_oldloc.y != doom->plr->mo->y)
    {
        doom->m_x = FTOM(MTOF(doom->plr->mo->x)) - doom->m_w / 2;
        doom->m_y = FTOM(MTOF(doom->plr->mo->y)) - doom->m_h / 2;
        doom->m_x2 = doom->m_x + doom->m_w;
        doom->m_y2 = doom->m_y + doom->m_h;
        doom->f_oldloc.x = doom->plr->mo->x;
        doom->f_oldloc.y = doom->plr->mo->y;

        //  doom->m_x = FTOM(MTOF(plr->mo->x - doom->m_w/2));
        //  doom->m_y = FTOM(MTOF(plr->mo->y - doom->m_h/2));
        //  doom->m_x = plr->mo->x - doom->m_w/2;
        //  doom->m_y = plr->mo->y - doom->m_h/2;
    }
}

//
//
//
static void AM_updatelightlev(doom_data_t *doom)
{
    // static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
    static const int litelevels[] = {0, 4, 7, 10, 12, 14, 15, 15};

    // Change light level
    if (doom->amclock > doom->nexttic)
    {
        doom->lightlev = litelevels[doom->litelevelscnt++];
        if (doom->litelevelscnt == arrlen(litelevels))
            doom->litelevelscnt = 0;
        doom->nexttic = doom->amclock + 6 - (doom->amclock % 6);
    }
}

//
// Updates on Game Tick
//
void AM_Ticker(doom_data_t *doom)
{

    if (!doom->automapactive)
        return;

    doom->amclock++;

    if (doom->followplayer)
        AM_doFollowPlayer(doom);

    // Change the zoom if necessary
    if (doom->ftom_zoommul != FRACUNIT)
        AM_changeWindowScale(doom);

    // Change x,y location
    if (doom->m_paninc.x || doom->m_paninc.y)
        AM_changeWindowLoc(doom);

    // Update light level
    // AM_updatedoom->lightlev();
}

//
// Clear automap frame buffer.
//
static void AM_clearFB(doom_data_t *doom, int color)
{
    d_memset(doom->fb, color, doom->f_w * doom->f_h);
}

//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
static boolean
AM_clipMline(doom_data_t *doom,
             mline_t *ml,
             fline_t *fl)
{
    enum
    {
        LEFT = 1,
        RIGHT = 2,
        BOTTOM = 4,
        TOP = 8
    };

    register int outcode1 = 0;
    register int outcode2 = 0;
    register int outside;

    fpoint_t tmp;
    int dx;
    int dy;

#define DOOUTCODE(oc, mx, my)   \
    (oc) = 0;                   \
    if ((my) < 0)               \
        (oc) |= TOP;            \
    else if ((my) >= doom->f_h) \
        (oc) |= BOTTOM;         \
    if ((mx) < 0)               \
        (oc) |= LEFT;           \
    else if ((mx) >= doom->f_w) \
        (oc) |= RIGHT;

    // do trivial rejects and outcodes
    if (ml->a.y > doom->m_y2)
        outcode1 = TOP;
    else if (ml->a.y < doom->m_y)
        outcode1 = BOTTOM;

    if (ml->b.y > doom->m_y2)
        outcode2 = TOP;
    else if (ml->b.y < doom->m_y)
        outcode2 = BOTTOM;

    if (outcode1 & outcode2)
        return false; // trivially outside

    if (ml->a.x < doom->m_x)
        outcode1 |= LEFT;
    else if (ml->a.x > doom->m_x2)
        outcode1 |= RIGHT;

    if (ml->b.x < doom->m_x)
        outcode2 |= LEFT;
    else if (ml->b.x > doom->m_x2)
        outcode2 |= RIGHT;

    if (outcode1 & outcode2)
        return false; // trivially outside

    // transform to frame-buffer coordinates.
    fl->a.x = CXMTOF(ml->a.x);
    fl->a.y = CYMTOF(ml->a.y);
    fl->b.x = CXMTOF(ml->b.x);
    fl->b.y = CYMTOF(ml->b.y);

    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    DOOUTCODE(outcode2, fl->b.x, fl->b.y);

    if (outcode1 & outcode2)
        return false;

    while (outcode1 | outcode2)
    {
        // may be partially inside box
        // find an outside point
        if (outcode1)
            outside = outcode1;
        else
            outside = outcode2;

        // clip to each side
        if (outside & TOP)
        {
            dy = fl->a.y - fl->b.y;
            dx = fl->b.x - fl->a.x;
            tmp.x = fl->a.x + (dx * (fl->a.y)) / dy;
            tmp.y = 0;
        }
        else if (outside & BOTTOM)
        {
            dy = fl->a.y - fl->b.y;
            dx = fl->b.x - fl->a.x;
            tmp.x = fl->a.x + (dx * (fl->a.y - doom->f_h)) / dy;
            tmp.y = doom->f_h - 1;
        }
        else if (outside & RIGHT)
        {
            dy = fl->b.y - fl->a.y;
            dx = fl->b.x - fl->a.x;
            tmp.y = fl->a.y + (dy * (doom->f_w - 1 - fl->a.x)) / dx;
            tmp.x = doom->f_w - 1;
        }
        else if (outside & LEFT)
        {
            dy = fl->b.y - fl->a.y;
            dx = fl->b.x - fl->a.x;
            tmp.y = fl->a.y + (dy * (-fl->a.x)) / dx;
            tmp.x = 0;
        }
        else
        {
            tmp.x = 0;
            tmp.y = 0;
        }

        if (outside == outcode1)
        {
            fl->a = tmp;
            DOOUTCODE(outcode1, fl->a.x, fl->a.y);
        }
        else
        {
            fl->b = tmp;
            DOOUTCODE(outcode2, fl->b.x, fl->b.y);
        }

        if (outcode1 & outcode2)
            return false; // trivially outside
    }

    return true;
}
#undef DOOUTCODE

//
// Classic Bresenham w/ whatever optimizations needed for speed
//
static void
AM_drawFline(doom_data_t *doom,
             fline_t *fl,
             int color)
{
    register int x;
    register int y;
    register int dx;
    register int dy;
    register int sx;
    register int sy;
    register int ax;
    register int ay;
    register int d;

    static int fuck = 0;

    // For debugging only
    if (fl->a.x < 0 || fl->a.x >= doom->f_w || fl->a.y < 0 || fl->a.y >= doom->f_h || fl->b.x < 0 || fl->b.x >= doom->f_w || fl->b.y < 0 || fl->b.y >= doom->f_h)
    {
        d_printf("fuck %d \r", fuck++);
        return;
    }

#define PUTDOT(xx, yy, cc) doom->fb[(yy)*doom->f_w + (xx)] = (cc)

    dx = fl->b.x - fl->a.x;
    ax = 2 * (dx < 0 ? -dx : dx);
    sx = dx < 0 ? -1 : 1;

    dy = fl->b.y - fl->a.y;
    ay = 2 * (dy < 0 ? -dy : dy);
    sy = dy < 0 ? -1 : 1;

    x = fl->a.x;
    y = fl->a.y;

    if (ax > ay)
    {
        d = ay - ax / 2;
        while (1)
        {
            PUTDOT(x, y, color);
            if (x == fl->b.x)
                return;
            if (d >= 0)
            {
                y += sy;
                d -= ax;
            }
            x += sx;
            d += ay;
        }
    }
    else
    {
        d = ax - ay / 2;
        while (1)
        {
            PUTDOT(x, y, color);
            if (y == fl->b.y)
                return;
            if (d >= 0)
            {
                x += sx;
                d -= ay;
            }
            y += sy;
            d += ax;
        }
    }
}

//
// Clip lines, draw visible part sof lines.
//
static void
AM_drawMline(doom_data_t *doom,
             mline_t *ml,
             int color)
{
    fline_t fl;

    if (AM_clipMline(doom, ml, &fl))
        AM_drawFline(doom, &fl, color); // draws it on frame buffer using fb coords
}

//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
static void AM_drawGrid(doom_data_t *doom, int color)
{
    fixed_t x, y;
    fixed_t start, end;
    mline_t ml;

    // Figure out start of vertical gridlines
    start = doom->m_x;
    if ((start - bmaporgx) % (MAPBLOCKUNITS << FRACBITS))
        start += (MAPBLOCKUNITS << FRACBITS) - ((start - bmaporgx) % (MAPBLOCKUNITS << FRACBITS));
    end = doom->m_x + doom->m_w;

    // draw vertical gridlines
    ml.a.y = doom->m_y;
    ml.b.y = doom->m_y + doom->m_h;
    for (x = start; x < end; x += (MAPBLOCKUNITS << FRACBITS))
    {
        ml.a.x = x;
        ml.b.x = x;
        AM_drawMline(doom, &ml, color);
    }

    // Figure out start of horizontal gridlines
    start = doom->m_y;
    if ((start - bmaporgy) % (MAPBLOCKUNITS << FRACBITS))
        start += (MAPBLOCKUNITS << FRACBITS) - ((start - bmaporgy) % (MAPBLOCKUNITS << FRACBITS));
    end = doom->m_y + doom->m_h;

    // draw horizontal gridlines
    ml.a.x = doom->m_x;
    ml.b.x = doom->m_x + doom->m_w;
    for (y = start; y < end; y += (MAPBLOCKUNITS << FRACBITS))
    {
        ml.a.y = y;
        ml.b.y = y;
        AM_drawMline(doom, &ml, color);
    }
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
static void AM_drawWalls(doom_data_t *doom)
{
    int i;
    mline_t l;

    for (i = 0; i < numlines; i++)
    {
        l.a.x = lines[i].v1->x;
        l.a.y = lines[i].v1->y;
        l.b.x = lines[i].v2->x;
        l.b.y = lines[i].v2->y;
        if (doom->cheating || (lines[i].flags & ML_MAPPED))
        {
            if ((lines[i].flags & LINE_NEVERSEE) && !doom->cheating)
                continue;
            if (!lines[i].backsector)
            {
                AM_drawMline(doom, &l, WALLCOLORS + doom->lightlev);
            }
            else
            {
                if (lines[i].special == 39)
                { // teleporters
                    AM_drawMline(doom, &l, WALLCOLORS + WALLRANGE / 2);
                }
                else if (lines[i].flags & ML_SECRET) // secret door
                {
                    if (doom->cheating)
                        AM_drawMline(doom, &l, SECRETWALLCOLORS + doom->lightlev);
                    else
                        AM_drawMline(doom, &l, WALLCOLORS + doom->lightlev);
                }
                else if (lines[i].backsector->floorheight != lines[i].frontsector->floorheight)
                {
                    AM_drawMline(doom, &l, FDWALLCOLORS + doom->lightlev); // floor level change
                }
                else if (lines[i].backsector->ceilingheight != lines[i].frontsector->ceilingheight)
                {
                    AM_drawMline(doom, &l, CDWALLCOLORS + doom->lightlev); // ceiling level change
                }
                else if (doom->cheating)
                {
                    AM_drawMline(doom, &l, TSWALLCOLORS + doom->lightlev);
                }
            }
        }
        else if (doom->plr->powers[pw_allmap])
        {
            if (!(lines[i].flags & LINE_NEVERSEE))
                AM_drawMline(doom, &l, GRAYS + 3);
        }
    }
}

//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
static void
AM_rotate(doom_data_t *doom,
          fixed_t *x,
          fixed_t *y,
          angle_t a)
{
    fixed_t tmpx;

    tmpx =
        FixedMul(*x, finecosine[a >> ANGLETOFINESHIFT]) - FixedMul(*y, finesine[a >> ANGLETOFINESHIFT]);

    *y =
        FixedMul(*x, finesine[a >> ANGLETOFINESHIFT]) + FixedMul(*y, finecosine[a >> ANGLETOFINESHIFT]);

    *x = tmpx;
}

static void
AM_drawLineCharacter(doom_data_t *doom,
                     const mline_t *lineguy,
                     int lineguylines,
                     fixed_t scale,
                     angle_t angle,
                     int color,
                     fixed_t x,
                     fixed_t y)
{
    int i;
    mline_t l;

    for (i = 0; i < lineguylines; i++)
    {
        l.a.x = lineguy[i].a.x;
        l.a.y = lineguy[i].a.y;

        if (scale)
        {
            l.a.x = FixedMul(scale, l.a.x);
            l.a.y = FixedMul(scale, l.a.y);
        }

        if (angle)
            AM_rotate(doom, &l.a.x, &l.a.y, angle);

        l.a.x += x;
        l.a.y += y;

        l.b.x = lineguy[i].b.x;
        l.b.y = lineguy[i].b.y;

        if (scale)
        {
            l.b.x = FixedMul(scale, l.b.x);
            l.b.y = FixedMul(scale, l.b.y);
        }

        if (angle)
            AM_rotate(doom, &l.b.x, &l.b.y, angle);

        l.b.x += x;
        l.b.y += y;

        AM_drawMline(doom, &l, color);
    }
}

static void AM_drawPlayers(doom_data_t *doom)
{
    int i;
    player_t *p;
    static const int their_colors[] = {GREENS, GRAYS, BROWNS, REDS};
    int their_color = -1;
    int color;

    if (!doom->netgame)
    {
        if (doom->cheating)
            AM_drawLineCharacter(doom, cheat_player_arrow, arrlen(cheat_player_arrow), 0,
                                 doom->plr->mo->angle, WHITE, doom->plr->mo->x, doom->plr->mo->y);
        else
            AM_drawLineCharacter(doom, player_arrow, arrlen(player_arrow), 0, doom->plr->mo->angle,
                                 WHITE, doom->plr->mo->x, doom->plr->mo->y);
        return;
    }

    for (i = 0; i < MAXPLAYERS; i++)
    {
        their_color++;
        p = &doom->players[i];

        if ((doom->deathmatch && !doom->singledemo) && p != doom->plr)
            continue;

        if (!doom->playeringame[i])
            continue;

        if (p->powers[pw_invisibility])
            color = 246; // *close* to black
        else
            color = their_colors[their_color];

        AM_drawLineCharacter(doom, player_arrow, arrlen(player_arrow), 0, p->mo->angle,
                             color, p->mo->x, p->mo->y);
    }
}

static void
AM_drawThings(doom_data_t *doom,
              int colors,
              int colorrange)
{
    int i;
    mobj_t *t;

    for (i = 0; i < numsectors; i++)
    {
        t = sectors[i].thinglist;
        while (t)
        {
            AM_drawLineCharacter(doom, thintriangle_guy, arrlen(thintriangle_guy),
                                 16 << FRACBITS, t->angle, colors + doom->lightlev, t->x, t->y);
            t = t->snext;
        }
    }
}

static void AM_drawMarks(doom_data_t *doom)
{
    int i, fx, fy, w, h;

    for (i = 0; i < AM_NUMMARKPOINTS; i++)
    {
        if (doom->markpoints[i].x != -1)
        {
            //      w = SHORT(marknums[i]->width);
            //      h = SHORT(marknums[i]->height);
            w = 5; // because something's wrong with the wad, i guess
            h = 6; // because something's wrong with the wad, i guess
            fx = CXMTOF(doom->markpoints[i].x);
            fy = CYMTOF(doom->markpoints[i].y);
            if (fx >= doom->f_x && fx <= doom->f_w - w && fy >= doom->f_y && fy <= doom->f_h - h)
                V_DrawPatch(doom, fx, fy, doom->marknums[i]);
        }
    }
}

static void AM_drawCrosshair(doom_data_t *doom, int color)
{
    doom->fb[(doom->f_w * (doom->f_h + 1)) / 2] = color; // single point for now
}

void AM_Drawer(doom_data_t *doom)
{
    if (!doom->automapactive)
        return;

    AM_clearFB(doom, BACKGROUND);
    if (doom->grid)
        AM_drawGrid(doom, GRIDCOLORS);
    AM_drawWalls(doom);
    AM_drawPlayers(doom);
    if (doom->cheating == 2)
        AM_drawThings(doom, THINGCOLORS, THINGRANGE);
    AM_drawCrosshair(doom, XHAIRCOLORS);

    AM_drawMarks(doom);

    V_MarkRect(doom, doom->f_x, doom->f_y, doom->f_w, doom->f_h);
}
