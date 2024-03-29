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
//	The status bar widget code.
//

#include "deh_main.h"
#include "doomdef.h"

#include "z_zone.h"
#include "v_video.h"

#include "i_swap.h"
#include "i_system.h"

#include "w_wad.h"

#include "st_stuff.h"
#include "st_lib.h"
#include "r_local.h"

//
// Hack display negative frags.
//  Loads and store the stminus lump.
//

void STlib_init(struct doom_data_t_ *doom)
{
    doom->sttminus = (patch_t *)W_CacheLumpName(doom, DEH_String("STTMINUS"), PU_STATIC);
}

// ?
void STlib_initNum(struct doom_data_t_* doom, st_number_t *n,
                   int x,
                   int y,
                   patch_t **pl,
                   int *num,
                   boolean *on,
                   int width)
{
    n->x = x;
    n->y = y;
    n->oldnum = 0;
    n->width = width;
    n->num = num;
    n->on = on;
    n->p = pl;
}

//
// A fairly efficient way to draw a number
//  based on differences from the old number.
// Note: worth the trouble?
//
void STlib_drawNum(struct doom_data_t_ *doom, st_number_t *n,
                   boolean refresh)
{

    int numdigits = n->width;
    int num = *n->num;

    int w = SHORT(n->p[0]->width);
    int h = SHORT(n->p[0]->height);
    int x = n->x;

    int neg;

    n->oldnum = *n->num;

    neg = num < 0;

    if (neg)
    {
        if (numdigits == 2 && num < -9)
            num = -9;
        else if (numdigits == 3 && num < -99)
            num = -99;

        num = -num;
    }

    // clear the area
    x = n->x - numdigits * w;

    if (n->y - ST_Y < 0)
        I_Error("drawNum: n->y - ST_Y < 0");

    V_CopyRect(doom, x, n->y - ST_Y, doom->st_backing_screen, w * numdigits, h, x, n->y);

    // if non-number, do not draw it
    if (num == 1994)
        return;

    x = n->x;

    // in the special case of 0, you draw 0
    if (!num)
        V_DrawPatch(doom, x - w, n->y, n->p[0]);

    // draw the new number
    while (num && numdigits--)
    {
        x -= w;
        V_DrawPatch(doom, x, n->y, n->p[num % 10]);
        num /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
        V_DrawPatch(doom, x - 8, n->y, doom->sttminus);
}

//
void STlib_updateNum(struct doom_data_t_ *doom, st_number_t *n,
                     boolean refresh)
{
    if (*n->on)
        STlib_drawNum(doom, n, refresh);
}

//
void STlib_initPercent(struct doom_data_t_* doom, st_percent_t *p,
                       int x,
                       int y,
                       patch_t **pl,
                       int *num,
                       boolean *on,
                       patch_t *percent)
{
    STlib_initNum(doom, &p->n, x, y, pl, num, on, 3);
    p->p = percent;
}

void STlib_updatePercent(struct doom_data_t_ *doom, st_percent_t *per,
                         int refresh)
{
    if (refresh && *per->n.on)
        V_DrawPatch(doom, per->n.x, per->n.y, per->p);

    STlib_updateNum(doom, &per->n, refresh);
}

void STlib_initMultIcon(struct doom_data_t_ *doom,
                        st_multicon_t *i,
                        int x,
                        int y,
                        patch_t **il,
                        int *inum,
                        boolean *on)
{
    i->x = x;
    i->y = y;
    i->oldinum = -1;
    i->inum = inum;
    i->on = on;
    i->p = il;
}

void STlib_updateMultIcon(struct doom_data_t_ *doom, st_multicon_t *mi,
                          boolean refresh)
{
    int w;
    int h;
    int x;
    int y;

    if (*mi->on && (mi->oldinum != *mi->inum || refresh) && (*mi->inum != -1))
    {
        if (mi->oldinum != -1)
        {
            x = mi->x - SHORT(mi->p[mi->oldinum]->leftoffset);
            y = mi->y - SHORT(mi->p[mi->oldinum]->topoffset);
            w = SHORT(mi->p[mi->oldinum]->width);
            h = SHORT(mi->p[mi->oldinum]->height);

            if (y - ST_Y < 0)
                I_Error("updateMultIcon: y - ST_Y < 0");

            V_CopyRect(doom, x, y - ST_Y, doom->st_backing_screen, w, h, x, y);
        }
        V_DrawPatch(doom, mi->x, mi->y, mi->p[*mi->inum]);
        mi->oldinum = *mi->inum;
    }
}

void STlib_initBinIcon(struct doom_data_t_ *doom, st_binicon_t *b,
                       int x,
                       int y,
                       patch_t *i,
                       boolean *val,
                       boolean *on)
{
    b->x = x;
    b->y = y;
    b->oldval = false;
    b->val = val;
    b->on = on;
    b->p = i;
}

void STlib_updateBinIcon(struct doom_data_t_ *doom, st_binicon_t *bi,
                         boolean refresh)
{
    int x;
    int y;
    int w;
    int h;

    if (*bi->on && (bi->oldval != *bi->val || refresh))
    {
        x = bi->x - SHORT(bi->p->leftoffset);
        y = bi->y - SHORT(bi->p->topoffset);
        w = SHORT(bi->p->width);
        h = SHORT(bi->p->height);

        if (y - ST_Y < 0)
            I_Error("updateBinIcon: y - ST_Y < 0");

        if (*bi->val)
            V_DrawPatch(doom, bi->x, bi->y, bi->p);
        else
            V_CopyRect(doom, x, y - ST_Y, doom->st_backing_screen, w, h, x, y);

        bi->oldval = *bi->val;
    }
}
