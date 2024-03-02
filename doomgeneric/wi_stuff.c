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
//	Intermission screens.
//

#include "z_zone.h"

#include "m_misc.h"
#include "m_random.h"

#include "deh_main.h"
#include "i_swap.h"
#include "i_system.h"

#include "w_wad.h"

#include "g_game.h"

#include "r_local.h"
#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

// Needs access to LFB.
#include "v_video.h"

#include "wi_stuff.h"

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//

//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.
#define NUMEPISODES 4
#define NUMMAPS 9

// in tics
// U #define PAUSELEN		(TICRATE*2)
// U #define SCORESTEP		100
// U #define ANIMPERIOD		32
// pixel distance from "(YOU)" to "PLAYER N"
// U #define STARDIST		10
// U #define WK 1

// GLOBAL LOCATIONS
#define WI_TITLEY 2
#define WI_SPACINGY 33

// SINGPLE-PLAYER STUFF
#define SP_STATSX 50
#define SP_STATSY 50

#define SP_TIMEX 16
#define SP_TIMEY (SCREENHEIGHT - 32)

// NET GAME STUFF
#define NG_STATSY 50
#define NG_STATSX (32 + SHORT(doom->star->width) / 2 + 32 * !dofrags)

#define NG_SPACINGX 64

// DEATHMATCH STUFF
#define DM_MATRIXX 42
#define DM_MATRIXY 68

#define DM_SPACINGX 40

#define DM_TOTALSX 269

#define DM_KILLERSX 10
#define DM_KILLERSY 100
#define DM_VICTIMSX 5
#define DM_VICTIMSY 50

typedef enum
{
	ANIM_ALWAYS,
	ANIM_RANDOM,
	ANIM_LEVEL

} animenum_t;

typedef struct
{
	int x;
	int y;

} point_t;

//
// Animation.
// There is another anim_t used in p_spec.
//
typedef struct
{
	animenum_t type;

	// period in tics between animations
	int period;

	// number of animation frames
	int nanims;

	// location of animation
	point_t loc;

	// ALWAYS: n/a,
	// RANDOM: period deviation (<256),
	// LEVEL: level
	int data1;

	// ALWAYS: n/a,
	// RANDOM: random base period,
	// LEVEL: n/a
	int data2;

	// actual graphics for frames of animations
	patch_t *p[3];

	// following must be initialized to zero before use!

	// next value of bcnt (used in conjunction with period)
	int nexttic;

	// last drawn animation frame
	int lastdrawn;

	// next frame number to animate
	int ctr;

	// used by RANDOM and LEVEL when animating
	int state;

} anim_t;

static const point_t lnodes[NUMEPISODES][NUMMAPS] =
	{
		// Episode 0 World Map
		{
			{185, 164}, // location of level 0 (CJ)
			{148, 143}, // location of level 1 (CJ)
			{69, 122},	// location of level 2 (CJ)
			{209, 102}, // location of level 3 (CJ)
			{116, 89},	// location of level 4 (CJ)
			{166, 55},	// location of level 5 (CJ)
			{71, 56},	// location of level 6 (CJ)
			{135, 29},	// location of level 7 (CJ)
			{71, 24}	// location of level 8 (CJ)
		},

		// Episode 1 World Map should go here
		{
			{254, 25},	// location of level 0 (CJ)
			{97, 50},	// location of level 1 (CJ)
			{188, 64},	// location of level 2 (CJ)
			{128, 78},	// location of level 3 (CJ)
			{214, 92},	// location of level 4 (CJ)
			{133, 130}, // location of level 5 (CJ)
			{208, 136}, // location of level 6 (CJ)
			{148, 140}, // location of level 7 (CJ)
			{235, 158}	// location of level 8 (CJ)
		},

		// Episode 2 World Map should go here
		{
			{156, 168}, // location of level 0 (CJ)
			{48, 154},	// location of level 1 (CJ)
			{174, 95},	// location of level 2 (CJ)
			{265, 75},	// location of level 3 (CJ)
			{130, 48},	// location of level 4 (CJ)
			{279, 23},	// location of level 5 (CJ)
			{198, 48},	// location of level 6 (CJ)
			{140, 25},	// location of level 7 (CJ)
			{281, 136}	// location of level 8 (CJ)
		}

};

//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//

#define ANIM(type, period, nanims, x, y, nexttic)          \
	{                                                      \
		(type), (period), (nanims), {(x), (y)}, (nexttic), \
			0, {NULL, NULL, NULL}, 0, 0, 0, 0              \
	}

static anim_t epsd0animinfo[] =
	{
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 224, 104, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 184, 160, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 112, 136, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 72, 112, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 88, 96, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 64, 48, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 192, 40, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 136, 16, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 80, 16, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 64, 24, 0),
};

static anim_t epsd1animinfo[] =
	{
		ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 1),
		ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 2),
		ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 3),
		ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 4),
		ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 5),
		ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 6),
		ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 7),
		ANIM(ANIM_LEVEL, TICRATE / 3, 3, 192, 144, 8),
		ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 8),
};

static anim_t epsd2animinfo[] =
	{
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 104, 168, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 40, 136, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 160, 96, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 104, 80, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 120, 32, 0),
		ANIM(ANIM_ALWAYS, TICRATE / 4, 3, 40, 0, 0),
};

static const int NUMANIMS[NUMEPISODES] =
	{
		arrlen(epsd0animinfo),
		arrlen(epsd1animinfo),
		arrlen(epsd2animinfo),
};

static anim_t *anims[NUMEPISODES] =
	{
		epsd0animinfo,
		epsd1animinfo,
		epsd2animinfo};

//
// GENERAL DATA
//

//
// Locally used stuff.
//

// States for single-player
#define SP_KILLS 0
#define SP_ITEMS 2
#define SP_SECRET 4
#define SP_FRAGS 6
#define SP_TIME 8
#define SP_PAR ST_TIME

#define SP_PAUSE 1

// in seconds
#define SHOWNEXTLOCDELAY 4
//#define SHOWLASTLOCDELAY	SHOWNEXTLOCDELAY

// used to accelerate or skip a stage

//
// CODE
//

// slam background
void WI_slamBackground(doom_data_t *doom)
{
	V_DrawPatch(0, 0, doom->background);
}

// The ticker is used to detect keys
//  because of timing issues in netgames.
boolean WI_Responder(event_t *ev)
{
	return false;
}

// Draws "<Levelname> Finished!"
void WI_drawLF(doom_data_t *doom)
{
	int y = WI_TITLEY;

	if (doom->gamemode != commercial || doom->wbs->last < doom->NUMCMAPS)
	{
		// draw <LevelName>
		V_DrawPatch((SCREENWIDTH - SHORT(doom->lnames[doom->wbs->last]->width)) / 2,
					y, doom->lnames[doom->wbs->last]);

		// draw "Finished!"
		y += (5 * SHORT(doom->lnames[doom->wbs->last]->height)) / 4;

		V_DrawPatch((SCREENWIDTH - SHORT(doom->finished->width)) / 2, y, doom->finished);
	}
	else if (doom->wbs->last == doom->NUMCMAPS)
	{
		// MAP33 - nothing is displayed!
	}
	else if (doom->wbs->last > doom->NUMCMAPS)
	{
		// > MAP33.  Doom bombs out here with a Bad V_DrawPatch error.
		// I'm pretty sure that doom2.exe is just reading into random
		// bits of memory at this point, but let's try to be accurate
		// anyway.  This deliberately triggers a V_DrawPatch error.

		patch_t tmp;
		d_memset(&tmp, 0, sizeof(patch_t));
		tmp.width = SCREENWIDTH;
		tmp.height = SCREENHEIGHT;
		tmp.leftoffset = 1;
		tmp.topoffset = 1;

		V_DrawPatch(0, y, &tmp);
	}
}

// Draws "Entering <LevelName>"
void WI_drawEL(doom_data_t *doom)
{
	int y = WI_TITLEY;

	// draw "Entering"
	V_DrawPatch((SCREENWIDTH - SHORT(doom->entering->width)) / 2,
				y,
				doom->entering);

	// draw level
	y += (5 * SHORT(doom->lnames[doom->wbs->next]->height)) / 4;

	V_DrawPatch((SCREENWIDTH - SHORT(doom->lnames[doom->wbs->next]->width)) / 2,
				y,
				doom->lnames[doom->wbs->next]);
}

void WI_drawOnLnode(doom_data_t *doom,
					int n,
					patch_t *c[])
{

	int i;
	int left;
	int top;
	int right;
	int bottom;
	boolean fits = false;

	i = 0;
	do
	{
		left = lnodes[doom->wbs->epsd][n].x - SHORT(c[i]->leftoffset);
		top = lnodes[doom->wbs->epsd][n].y - SHORT(c[i]->topoffset);
		right = left + SHORT(c[i]->width);
		bottom = top + SHORT(c[i]->height);

		if (left >= 0 && right < SCREENWIDTH && top >= 0 && bottom < SCREENHEIGHT)
		{
			fits = true;
		}
		else
		{
			i++;
		}
	} while (!fits && i != 2 && c[i] != NULL);

	if (fits && i < 2)
	{
		V_DrawPatch(lnodes[doom->wbs->epsd][n].x,
					lnodes[doom->wbs->epsd][n].y,
					c[i]);
	}
	else
	{
		// DEBUG
		d_printf("Could not place patch on level %d", n + 1);
	}
}

void WI_initAnimatedBack(doom_data_t *doom)
{
	int i;
	anim_t *a;

	if (doom->gamemode == commercial)
		return;

	if (doom->wbs->epsd > 2)
		return;

	for (i = 0; i < NUMANIMS[doom->wbs->epsd]; i++)
	{
		a = &anims[doom->wbs->epsd][i];

		// init variables
		a->ctr = -1;

		// specify the next time to draw it
		if (a->type == ANIM_ALWAYS)
			a->nexttic = doom->bcnt + 1 + (M_Random() % a->period);
		else if (a->type == ANIM_RANDOM)
			a->nexttic = doom->bcnt + 1 + a->data2 + (M_Random() % a->data1);
		else if (a->type == ANIM_LEVEL)
			a->nexttic = doom->bcnt + 1;
	}
}

void WI_updateAnimatedBack(doom_data_t *doom)
{
	int i;
	anim_t *a;

	if (doom->gamemode == commercial)
		return;

	if (doom->wbs->epsd > 2)
		return;

	for (i = 0; i < NUMANIMS[doom->wbs->epsd]; i++)
	{
		a = &anims[doom->wbs->epsd][i];

		if (doom->bcnt == a->nexttic)
		{
			switch (a->type)
			{
			case ANIM_ALWAYS:
				if (++a->ctr >= a->nanims)
					a->ctr = 0;
				a->nexttic = doom->bcnt + a->period;
				break;

			case ANIM_RANDOM:
				a->ctr++;
				if (a->ctr == a->nanims)
				{
					a->ctr = -1;
					a->nexttic = doom->bcnt + a->data2 + (M_Random() % a->data1);
				}
				else
					a->nexttic = doom->bcnt + a->period;
				break;

			case ANIM_LEVEL:
				// gawd-awful hack for level anims
				if (!(doom->state == StatCount && i == 7) && doom->wbs->next == a->data1)
				{
					a->ctr++;
					if (a->ctr == a->nanims)
						a->ctr--;
					a->nexttic = doom->bcnt + a->period;
				}
				break;
			}
		}
	}
}

void WI_drawAnimatedBack(doom_data_t *doom)
{
	int i;
	anim_t *a;

	if (doom->gamemode == commercial)
		return;

	if (doom->wbs->epsd > 2)
		return;

	for (i = 0; i < NUMANIMS[doom->wbs->epsd]; i++)
	{
		a = &anims[doom->wbs->epsd][i];

		if (a->ctr >= 0)
			V_DrawPatch(a->loc.x, a->loc.y, a->p[a->ctr]);
	}
}

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//

int WI_drawNum(doom_data_t *doom,
			   int x,
			   int y,
			   int n,
			   int digits)
{

	int fontwidth = SHORT(doom->num[0]->width);
	int neg;
	int temp;

	if (digits < 0)
	{
		if (!n)
		{
			// make variable-length zeros 1 digit long
			digits = 1;
		}
		else
		{
			// figure out # of digits in #
			digits = 0;
			temp = n;

			while (temp)
			{
				temp /= 10;
				digits++;
			}
		}
	}

	neg = n < 0;
	if (neg)
		n = -n;

	// if non-number, do not draw it
	if (n == 1994)
		return 0;

	// draw the new number
	while (digits--)
	{
		x -= fontwidth;
		V_DrawPatch(x, y, doom->num[n % 10]);
		n /= 10;
	}

	// draw a minus sign if necessary
	if (neg)
		V_DrawPatch(x -= 8, y, doom->wiminus);

	return x;
}

void WI_drawPercent(doom_data_t *doom,
					int x,
					int y,
					int p)
{
	if (p < 0)
		return;

	V_DrawPatch(x, y, doom->percent);
	WI_drawNum(doom, x, y, p, -1);
}

//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
void WI_drawTime(doom_data_t *doom,
				 int x,
				 int y,
				 int t)
{

	int div;
	int n;

	if (t < 0)
		return;

	if (t <= 61 * 59)
	{
		div = 1;

		do
		{
			n = (t / div) % 60;
			x = WI_drawNum(doom, x, y, n, 2) - SHORT(doom->colon->width);
			div *= 60;

			// draw
			if (div == 60 || t / div)
				V_DrawPatch(x, y, doom->colon);

		} while (t / div);
	}
	else
	{
		// "sucks"
		V_DrawPatch(x - SHORT(doom->sucks->width), y, doom->sucks);
	}
}

void WI_End(doom_data_t *doom)
{
	void WI_unloadData(doom_data_t * doom);
	WI_unloadData(doom);
}

void WI_initNoState(doom_data_t *doom)
{
	doom->state = NoState;
	doom->acceleratestage = 0;
	doom->cnt = 10;
}

void WI_updateNoState(doom_data_t *doom)
{

	WI_updateAnimatedBack(doom);

	if (!--doom->cnt)
	{
		// Don't call WI_End yet.  G_WorldDone doesnt immediately
		// change gamestate, so WI_Drawer is still going to get
		// run until that happens.  If we do that after WI_End
		// (which unloads all the graphics), we're in trouble.
		// WI_End();
		G_WorldDone(doom);
	}
}

static boolean snl_pointeron = false;

void WI_initShowNextLoc(doom_data_t *doom)
{
	doom->state = ShowNextLoc;
	doom->acceleratestage = 0;
	doom->cnt = SHOWNEXTLOCDELAY * TICRATE;

	WI_initAnimatedBack(doom);
}

void WI_updateShowNextLoc(doom_data_t *doom)
{
	WI_updateAnimatedBack(doom);

	if (!--doom->cnt || doom->acceleratestage)
		WI_initNoState(doom);
	else
		snl_pointeron = (doom->cnt & 31) < 20;
}

void WI_drawShowNextLoc(doom_data_t *doom)
{

	int i;
	int last;

	WI_slamBackground(doom);

	// draw animated background
	WI_drawAnimatedBack(doom);

	if (doom->gamemode != commercial)
	{
		if (doom->wbs->epsd > 2)
		{
			WI_drawEL(doom);
			return;
		}

		last = (doom->wbs->last == 8) ? doom->wbs->next - 1 : doom->wbs->last;

		// draw a splat on taken cities.
		for (i = 0; i <= last; i++)
			WI_drawOnLnode(doom, i, doom->splat);

		// splat the secret level?
		if (doom->wbs->didsecret)
			WI_drawOnLnode(doom, 8, doom->splat);

		// draw flashing ptr
		if (snl_pointeron)
			WI_drawOnLnode(doom, doom->wbs->next, doom->yah);
	}

	// draws which level you are entering..
	if ((doom->gamemode != commercial) || doom->wbs->next != 30)
		WI_drawEL(doom);
}

void WI_drawNoState(doom_data_t *doom)
{
	snl_pointeron = true;
	WI_drawShowNextLoc(doom);
}

int WI_fragSum(doom_data_t* doom, int playernum)
{
	int i;
	int frags = 0;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i] && i != playernum)
		{
			frags += doom->plrs[playernum].frags[i];
		}
	}

	// JDC hack - negative frags.
	frags -= doom->plrs[playernum].frags[playernum];
	// UNUSED if (frags < 0)
	// 	frags = 0;

	return frags;
}

static int dm_state;
static int dm_frags[MAXPLAYERS][MAXPLAYERS];
static int dm_totals[MAXPLAYERS];

void WI_initDeathmatchStats(doom_data_t *doom)
{

	int i;
	int j;

	doom->state = StatCount;
	doom->acceleratestage = 0;
	dm_state = 1;

	doom->cnt_pause = TICRATE;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			for (j = 0; j < MAXPLAYERS; j++)
				if (playeringame[j])
					dm_frags[i][j] = 0;

			dm_totals[i] = 0;
		}
	}

	WI_initAnimatedBack(doom);
}

void WI_updateDeathmatchStats(doom_data_t *doom)
{

	int i;
	int j;

	boolean stillticking;

	WI_updateAnimatedBack(doom);

	if (doom->acceleratestage && dm_state != 4)
	{
		doom->acceleratestage = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				for (j = 0; j < MAXPLAYERS; j++)
					if (playeringame[j])
						dm_frags[i][j] = doom->plrs[i].frags[j];

				dm_totals[i] = WI_fragSum(doom, i);
			}
		}

		S_StartSound(0, sfx_barexp);
		dm_state = 4;
	}

	if (dm_state == 2)
	{
		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		stillticking = false;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (playeringame[i])
			{
				for (j = 0; j < MAXPLAYERS; j++)
				{
					if (playeringame[j] && dm_frags[i][j] != doom->plrs[i].frags[j])
					{
						if (doom->plrs[i].frags[j] < 0)
							dm_frags[i][j]--;
						else
							dm_frags[i][j]++;

						if (dm_frags[i][j] > 99)
							dm_frags[i][j] = 99;

						if (dm_frags[i][j] < -99)
							dm_frags[i][j] = -99;

						stillticking = true;
					}
				}
				dm_totals[i] = WI_fragSum(doom, i);

				if (dm_totals[i] > 99)
					dm_totals[i] = 99;

				if (dm_totals[i] < -99)
					dm_totals[i] = -99;
			}
		}
		if (!stillticking)
		{
			S_StartSound(0, sfx_barexp);
			dm_state++;
		}
	}
	else if (dm_state == 4)
	{
		if (doom->acceleratestage)
		{
			S_StartSound(0, sfx_slop);

			if (doom->gamemode == commercial)
				WI_initNoState(doom);
			else
				WI_initShowNextLoc(doom);
		}
	}
	else if (dm_state & 1)
	{
		if (!--doom->cnt_pause)
		{
			dm_state++;
			doom->cnt_pause = TICRATE;
		}
	}
}

void WI_drawDeathmatchStats(doom_data_t *doom)
{

	int i;
	int j;
	int x;
	int y;
	int w;

	WI_slamBackground(doom);

	// draw animated background
	WI_drawAnimatedBack(doom);
	WI_drawLF(doom);

	// draw stat titles (top line)
	V_DrawPatch(DM_TOTALSX - SHORT(doom->total->width) / 2,
				DM_MATRIXY - WI_SPACINGY + 10,
				doom->total);

	V_DrawPatch(DM_KILLERSX, DM_KILLERSY, doom->killers);
	V_DrawPatch(DM_VICTIMSX, DM_VICTIMSY, doom->victims);

	// draw P?
	x = DM_MATRIXX + DM_SPACINGX;
	y = DM_MATRIXY;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (playeringame[i])
		{
			V_DrawPatch(x - SHORT(doom->p[i]->width) / 2,
						DM_MATRIXY - WI_SPACINGY,
						doom->p[i]);

			V_DrawPatch(DM_MATRIXX - SHORT(doom->p[i]->width) / 2,
						y,
						doom->p[i]);

			if (i == doom->me)
			{
				V_DrawPatch(x - SHORT(doom->p[i]->width) / 2,
							DM_MATRIXY - WI_SPACINGY,
							doom->bstar);

				V_DrawPatch(DM_MATRIXX - SHORT(doom->p[i]->width) / 2,
							y,
							doom->star);
			}
		}
		else
		{
			// V_DrawPatch(x-SHORT(bp[i]->width)/2,
			//   DM_MATRIXY - WI_SPACINGY, bp[i]);
			// V_DrawPatch(DM_MATRIXX-SHORT(bp[i]->width)/2,
			//   y, bp[i]);
		}
		x += DM_SPACINGX;
		y += WI_SPACINGY;
	}

	// draw stats
	y = DM_MATRIXY + 10;
	w = SHORT(doom->num[0]->width);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		x = DM_MATRIXX + DM_SPACINGX;

		if (playeringame[i])
		{
			for (j = 0; j < MAXPLAYERS; j++)
			{
				if (playeringame[j])
					WI_drawNum(doom, x + w, y, dm_frags[i][j], 2);

				x += DM_SPACINGX;
			}
			WI_drawNum(doom, DM_TOTALSX + w, y, dm_totals[i], 2);
		}
		y += WI_SPACINGY;
	}
}

static int cnt_frags[MAXPLAYERS];
static int dofrags;
static int ng_state;

void WI_initNetgameStats(doom_data_t *doom)
{

	int i;

	doom->state = StatCount;
	doom->acceleratestage = 0;
	ng_state = 1;

	doom->cnt_pause = TICRATE;

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		doom->cnt_kills[i] = doom->cnt_items[i] = doom->cnt_secret[i] = cnt_frags[i] = 0;

		dofrags += WI_fragSum(doom, i);
	}

	dofrags = !!dofrags;

	WI_initAnimatedBack(doom);
}

void WI_updateNetgameStats(doom_data_t *doom)
{

	int i;
	int fsum;

	boolean stillticking;

	WI_updateAnimatedBack(doom);

	if (doom->acceleratestage && ng_state != 10)
	{
		doom->acceleratestage = 0;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			doom->cnt_kills[i] = (doom->plrs[i].skills * 100) / doom->wbs->maxkills;
			doom->cnt_items[i] = (doom->plrs[i].sitems * 100) / doom->wbs->maxitems;
			doom->cnt_secret[i] = (doom->plrs[i].ssecret * 100) / doom->wbs->maxsecret;

			if (dofrags)
				cnt_frags[i] = WI_fragSum(doom, i);
		}
		S_StartSound(0, sfx_barexp);
		ng_state = 10;
	}

	if (ng_state == 2)
	{
		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		stillticking = false;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			doom->cnt_kills[i] += 2;

			if (doom->cnt_kills[i] >= (doom->plrs[i].skills * 100) / doom->wbs->maxkills)
				doom->cnt_kills[i] = (doom->plrs[i].skills * 100) / doom->wbs->maxkills;
			else
				stillticking = true;
		}

		if (!stillticking)
		{
			S_StartSound(0, sfx_barexp);
			ng_state++;
		}
	}
	else if (ng_state == 4)
	{
		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		stillticking = false;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			doom->cnt_items[i] += 2;
			if (doom->cnt_items[i] >= (doom->plrs[i].sitems * 100) / doom->wbs->maxitems)
				doom->cnt_items[i] = (doom->plrs[i].sitems * 100) / doom->wbs->maxitems;
			else
				stillticking = true;
		}
		if (!stillticking)
		{
			S_StartSound(0, sfx_barexp);
			ng_state++;
		}
	}
	else if (ng_state == 6)
	{
		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		stillticking = false;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			doom->cnt_secret[i] += 2;

			if (doom->cnt_secret[i] >= (doom->plrs[i].ssecret * 100) / doom->wbs->maxsecret)
				doom->cnt_secret[i] = (doom->plrs[i].ssecret * 100) / doom->wbs->maxsecret;
			else
				stillticking = true;
		}

		if (!stillticking)
		{
			S_StartSound(0, sfx_barexp);
			ng_state += 1 + 2 * !dofrags;
		}
	}
	else if (ng_state == 8)
	{
		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		stillticking = false;

		for (i = 0; i < MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;

			cnt_frags[i] += 1;

			if (cnt_frags[i] >= (fsum = WI_fragSum(doom, i)))
				cnt_frags[i] = fsum;
			else
				stillticking = true;
		}

		if (!stillticking)
		{
			S_StartSound(0, sfx_pldeth);
			ng_state++;
		}
	}
	else if (ng_state == 10)
	{
		if (doom->acceleratestage)
		{
			S_StartSound(0, sfx_sgcock);
			if (doom->gamemode == commercial)
				WI_initNoState(doom);
			else
				WI_initShowNextLoc(doom);
		}
	}
	else if (ng_state & 1)
	{
		if (!--doom->cnt_pause)
		{
			ng_state++;
			doom->cnt_pause = TICRATE;
		}
	}
}

void WI_drawNetgameStats(doom_data_t *doom)
{
	int i;
	int x;
	int y;
	int pwidth = SHORT(doom->percent->width);

	WI_slamBackground(doom);

	// draw animated background
	WI_drawAnimatedBack(doom);

	WI_drawLF(doom);

	// draw stat titles (top line)
	V_DrawPatch(NG_STATSX + NG_SPACINGX - SHORT(doom->kills->width),
				NG_STATSY, doom->kills);

	V_DrawPatch(NG_STATSX + 2 * NG_SPACINGX - SHORT(doom->items->width),
				NG_STATSY, doom->items);

	V_DrawPatch(NG_STATSX + 3 * NG_SPACINGX - SHORT(doom->secret->width),
				NG_STATSY, doom->secret);

	if (dofrags)
		V_DrawPatch(NG_STATSX + 4 * NG_SPACINGX - SHORT(doom->frags->width),
					NG_STATSY, doom->frags);

	// draw stats
	y = NG_STATSY + SHORT(doom->kills->height);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		if (!playeringame[i])
			continue;

		x = NG_STATSX;
		V_DrawPatch(x - SHORT(doom->p[i]->width), y, doom->p[i]);

		if (i == doom->me)
			V_DrawPatch(x - SHORT(doom->p[i]->width), y, doom->star);

		x += NG_SPACINGX;
		WI_drawPercent(doom, x - pwidth, y + 10, doom->cnt_kills[i]);
		x += NG_SPACINGX;
		WI_drawPercent(doom, x - pwidth, y + 10, doom->cnt_items[i]);
		x += NG_SPACINGX;
		WI_drawPercent(doom, x - pwidth, y + 10, doom->cnt_secret[i]);
		x += NG_SPACINGX;

		if (dofrags)
			WI_drawNum(doom, x, y + 10, cnt_frags[i], -1);

		y += WI_SPACINGY;
	}
}

static int sp_state;

void WI_initStats(doom_data_t *doom)
{
	doom->state = StatCount;
	doom->acceleratestage = 0;
	sp_state = 1;
	doom->cnt_kills[0] = doom->cnt_items[0] = doom->cnt_secret[0] = -1;
	doom->cnt_time = doom->cnt_par = -1;
	doom->cnt_pause = TICRATE;

	WI_initAnimatedBack(doom);
}

void WI_updateStats(doom_data_t *doom)
{

	WI_updateAnimatedBack(doom);

	if (doom->acceleratestage && sp_state != 10)
	{
		doom->acceleratestage = 0;
		doom->cnt_kills[0] = (doom->plrs[doom->me].skills * 100) / doom->wbs->maxkills;
		doom->cnt_items[0] = (doom->plrs[doom->me].sitems * 100) / doom->wbs->maxitems;
		doom->cnt_secret[0] = (doom->plrs[doom->me].ssecret * 100) / doom->wbs->maxsecret;
		doom->cnt_time = doom->plrs[doom->me].stime / TICRATE;
		doom->cnt_par = doom->wbs->partime / TICRATE;
		S_StartSound(0, sfx_barexp);
		sp_state = 10;
	}

	if (sp_state == 2)
	{
		doom->cnt_kills[0] += 2;

		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		if (doom->cnt_kills[0] >= (doom->plrs[doom->me].skills * 100) / doom->wbs->maxkills)
		{
			doom->cnt_kills[0] = (doom->plrs[doom->me].skills * 100) / doom->wbs->maxkills;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}
	else if (sp_state == 4)
	{
		doom->cnt_items[0] += 2;

		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		if (doom->cnt_items[0] >= (doom->plrs[doom->me].sitems * 100) / doom->wbs->maxitems)
		{
			doom->cnt_items[0] = (doom->plrs[doom->me].sitems * 100) / doom->wbs->maxitems;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}
	else if (sp_state == 6)
	{
		doom->cnt_secret[0] += 2;

		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		if (doom->cnt_secret[0] >= (doom->plrs[doom->me].ssecret * 100) / doom->wbs->maxsecret)
		{
			doom->cnt_secret[0] = (doom->plrs[doom->me].ssecret * 100) / doom->wbs->maxsecret;
			S_StartSound(0, sfx_barexp);
			sp_state++;
		}
	}

	else if (sp_state == 8)
	{
		if (!(doom->bcnt & 3))
			S_StartSound(0, sfx_pistol);

		doom->cnt_time += 3;

		if (doom->cnt_time >= doom->plrs[doom->me].stime / TICRATE)
			doom->cnt_time = doom->plrs[doom->me].stime / TICRATE;

		doom->cnt_par += 3;

		if (doom->cnt_par >= doom->wbs->partime / TICRATE)
		{
			doom->cnt_par = doom->wbs->partime / TICRATE;

			if (doom->cnt_time >= doom->plrs[doom->me].stime / TICRATE)
			{
				S_StartSound(0, sfx_barexp);
				sp_state++;
			}
		}
	}
	else if (sp_state == 10)
	{
		if (doom->acceleratestage)
		{
			S_StartSound(0, sfx_sgcock);

			if (doom->gamemode == commercial)
				WI_initNoState(doom);
			else
				WI_initShowNextLoc(doom);
		}
	}
	else if (sp_state & 1)
	{
		if (!--doom->cnt_pause)
		{
			sp_state++;
			doom->cnt_pause = TICRATE;
		}
	}
}

void WI_drawStats(doom_data_t *doom)
{
	// line height
	int lh;

	lh = (3 * SHORT(doom->num[0]->height)) / 2;

	WI_slamBackground(doom);

	// draw animated background
	WI_drawAnimatedBack(doom);

	WI_drawLF(doom);

	V_DrawPatch(SP_STATSX, SP_STATSY, doom->kills);
	WI_drawPercent(doom, SCREENWIDTH - SP_STATSX, SP_STATSY, doom->cnt_kills[0]);

	V_DrawPatch(SP_STATSX, SP_STATSY + lh, doom->items);
	WI_drawPercent(doom, SCREENWIDTH - SP_STATSX, SP_STATSY + lh, doom->cnt_items[0]);

	V_DrawPatch(SP_STATSX, SP_STATSY + 2 * lh, doom->sp_secret);
	WI_drawPercent(doom, SCREENWIDTH - SP_STATSX, SP_STATSY + 2 * lh, doom->cnt_secret[0]);

	V_DrawPatch(SP_TIMEX, SP_TIMEY, doom->timepatch);
	WI_drawTime(doom, SCREENWIDTH / 2 - SP_TIMEX, SP_TIMEY, doom->cnt_time);

	if (doom->wbs->epsd < 3)
	{
		V_DrawPatch(SCREENWIDTH / 2 + SP_TIMEX, SP_TIMEY, doom->par);
		WI_drawTime(doom, SCREENWIDTH - SP_TIMEX, SP_TIMEY, doom->cnt_par);
	}
}

void WI_checkForAccelerate(doom_data_t *doom)
{
	int i;
	player_t *player;

	// check for button presses to skip delays
	for (i = 0, player = players; i < MAXPLAYERS; i++, player++)
	{
		if (playeringame[i])
		{
			if (player->cmd.buttons & BT_ATTACK)
			{
				if (!player->attackdown)
					doom->acceleratestage = 1;
				player->attackdown = true;
			}
			else
				player->attackdown = false;
			if (player->cmd.buttons & BT_USE)
			{
				if (!player->usedown)
					doom->acceleratestage = 1;
				player->usedown = true;
			}
			else
				player->usedown = false;
		}
	}
}

// Updates stuff each tick
void WI_Ticker(doom_data_t *doom)
{
	// counter for general background animation
	doom->bcnt++;

	if (doom->bcnt == 1)
	{
		// intermission music
		if (doom->gamemode == commercial)
			S_ChangeMusic(doom, mus_dm2int, true);
		else
			S_ChangeMusic(doom, mus_inter, true);
	}

	WI_checkForAccelerate(doom);

	switch (doom->state)
	{
	case StatCount:
		if (deathmatch)
			WI_updateDeathmatchStats(doom);
		else if (netgame)
			WI_updateNetgameStats(doom);
		else
			WI_updateStats(doom);
		break;

	case ShowNextLoc:
		WI_updateShowNextLoc(doom);
		break;

	case NoState:
		WI_updateNoState(doom);
		break;
	}
}

typedef void (*load_callback_t)(struct doom_data_t_ *doom, char *lumpname, patch_t **variable);

// Common load/unload function.  Iterates over all the graphics
// lumps to be loaded/unloaded into memory.

static void WI_loadUnloadData(doom_data_t *doom, load_callback_t callback)
{
	int i, j;
	char name[9];
	anim_t *a;

	if (doom->gamemode == commercial)
	{
		for (i = 0; i < doom->NUMCMAPS; i++)
		{
			d_snprintf(name, 9, "CWILV%2.2d", i);
			callback(doom, name, &doom->lnames[i]);
		}
	}
	else
	{
		for (i = 0; i < NUMMAPS; i++)
		{
			d_snprintf(name, 9, "WILV%d%d", doom->wbs->epsd, i);
			callback(doom, name, &doom->lnames[i]);
		}

		// you are here
		callback(doom, DEH_String("WIURH0"), &doom->yah[0]);

		// you are here (alt.)
		callback(doom, DEH_String("WIURH1"), &doom->yah[1]);

		// splat
		callback(doom, DEH_String("WISPLAT"), &doom->splat[0]);

		if (doom->wbs->epsd < 3)
		{
			for (j = 0; j < NUMANIMS[doom->wbs->epsd]; j++)
			{
				a = &anims[doom->wbs->epsd][j];
				for (i = 0; i < a->nanims; i++)
				{
					// MONDO HACK!
					if (doom->wbs->epsd != 1 || j != 8)
					{
						// animations
						d_snprintf(name, 9, "WIA%d%.2d%.2d", doom->wbs->epsd, j, i);
						callback(doom, name, &a->p[i]);
					}
					else
					{
						// HACK ALERT!
						a->p[i] = anims[1][4].p[i];
					}
				}
			}
		}
	}

	// More hacks on minus sign.
	callback(doom, DEH_String("WIMINUS"), &doom->wiminus);

	for (i = 0; i < 10; i++)
	{
		// numbers 0-9
		d_snprintf(name, 9, "WINUM%d", i);
		callback(doom, name, &doom->num[i]);
	}

	// percent sign
	callback(doom, DEH_String("WIPCNT"), &doom->percent);

	// "finished"
	callback(doom, DEH_String("WIF"), &doom->finished);

	// "entering"
	callback(doom, DEH_String("WIENTER"), &doom->entering);

	// "kills"
	callback(doom, DEH_String("WIOSTK"), &doom->kills);

	// "scrt"
	callback(doom, DEH_String("WIOSTS"), &doom->secret);

	// "secret"
	callback(doom, DEH_String("WISCRT2"), &doom->sp_secret);

	// french wad uses WIOBJ (?)
	if (W_CheckNumForName(doom, DEH_String("WIOBJ")) >= 0)
	{
		// "items"
		if (netgame && !deathmatch)
			callback(doom, DEH_String("WIOBJ"), &doom->items);
		else
			callback(doom, DEH_String("WIOSTI"), &doom->items);
	}
	else
	{
		callback(doom, DEH_String("WIOSTI"), &doom->items);
	}

	// "frgs"
	callback(doom, DEH_String("WIFRGS"), &doom->frags);

	// ":"
	callback(doom, DEH_String("WICOLON"), &doom->colon);

	// "time"
	callback(doom, DEH_String("WITIME"), &doom->timepatch);

	// "sucks"
	callback(doom, DEH_String("WISUCKS"), &doom->sucks);

	// "par"
	callback(doom, DEH_String("WIPAR"), &doom->par);

	// "killers" (vertical)
	callback(doom, DEH_String("WIKILRS"), &doom->killers);

	// "victims" (horiz)
	callback(doom, DEH_String("WIVCTMS"), &doom->victims);

	// "total"
	callback(doom, DEH_String("WIMSTT"), &doom->total);

	for (i = 0; i < MAXPLAYERS; i++)
	{
		// "1,2,3,4"
		d_snprintf(name, 9, "STPB%d", i);
		callback(doom, name, &doom->p[i]);

		// "1,2,3,4"
		d_snprintf(name, 9, "WIBP%d", i + 1);
		callback(doom, name, &doom->bp[i]);
	}

	// Background image

	if (doom->gamemode == commercial)
	{
		M_StringCopy(name, DEH_String("INTERPIC"), sizeof(name));
	}
	else if (doom->gamemode == retail && doom->wbs->epsd == 3)
	{
		M_StringCopy(name, DEH_String("INTERPIC"), sizeof(name));
	}
	else
	{
		d_snprintf(name, sizeof(name), "WIMAP%d", doom->wbs->epsd);
	}

	// Draw backdrop and save to a temporary buffer

	callback(doom, name, &doom->background);
}

static void WI_loadCallback(struct doom_data_t_ *doom, char *name, patch_t **variable)
{
	*variable = W_CacheLumpName(doom, name, PU_STATIC);
}

void WI_loadData(doom_data_t *doom)
{
	if (doom->gamemode == commercial)
	{
		doom->NUMCMAPS = 32;
		doom->lnames = (patch_t **)Z_Malloc(sizeof(patch_t *) * doom->NUMCMAPS,
											PU_STATIC, NULL);
	}
	else
	{
		doom->lnames = (patch_t **)Z_Malloc(sizeof(patch_t *) * NUMMAPS,
											PU_STATIC, NULL);
	}

	WI_loadUnloadData(doom, WI_loadCallback);

	// These two graphics are special cased because we're sharing
	// them with the status bar code

	// your face
	doom->star = W_CacheLumpName(doom, DEH_String("STFST01"), PU_STATIC);

	// dead face
	doom->bstar = W_CacheLumpName(doom, DEH_String("STFDEAD0"), PU_STATIC);
}

static void WI_unloadCallback(struct doom_data_t_ *doom, char *name, patch_t **variable)
{
	W_ReleaseLumpName(doom, name);
	*variable = NULL;
}

void WI_unloadData(doom_data_t *doom)
{
	WI_loadUnloadData(doom, WI_unloadCallback);

	// We do not free these lumps as they are shared with the status
	// bar code.

	// W_ReleaseLumpName("STFST01");
	// W_ReleaseLumpName("STFDEAD0");
}

void WI_Drawer(doom_data_t *doom)
{
	switch (doom->state)
	{
	case StatCount:
		if (deathmatch)
			WI_drawDeathmatchStats(doom);
		else if (netgame)
			WI_drawNetgameStats(doom);
		else
			WI_drawStats(doom);
		break;

	case ShowNextLoc:
		WI_drawShowNextLoc(doom);
		break;

	case NoState:
		WI_drawNoState(doom);
		break;
	}
}

void WI_initVariables(doom_data_t *doom, wbstartstruct_t *wbstartstruct)
{

	doom->wbs = wbstartstruct;
	doom->acceleratestage = 0;
	doom->cnt = doom->bcnt = 0;
	doom->firstrefresh = 1;
	doom->me = doom->wbs->pnum;
	doom->plrs = doom->wbs->plyr;

	if (!doom->wbs->maxkills)
		doom->wbs->maxkills = 1;

	if (!doom->wbs->maxitems)
		doom->wbs->maxitems = 1;

	if (!doom->wbs->maxsecret)
		doom->wbs->maxsecret = 1;

	if (doom->gamemode != retail)
		if (doom->wbs->epsd > 2)
			doom->wbs->epsd -= 3;
}

void WI_Start(doom_data_t *doom, wbstartstruct_t *wbstartstruct)
{
	WI_initVariables(doom, wbstartstruct);
	WI_loadData(doom);

	if (deathmatch)
		WI_initDeathmatchStats(doom);
	else if (netgame)
		WI_initNetgameStats(doom);
	else
		WI_initStats(doom);
}
