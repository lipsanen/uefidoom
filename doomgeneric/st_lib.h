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
// 	The status bar widget code.
//

#ifndef __STLIB__
#define __STLIB__

// We are referring to patches.
#include "doomdef.h"
#include "r_defs.h"

//
// Typedefs of widgets
//

// Number widget

struct doom_data_t_;


//
// Widget creation, access, and update routines
//

// Initializes widget library.
// More precisely, initialize STMINUS,
//  everything else is done somewhere else.
//
void STlib_init(struct doom_data_t_ *doom);

// Number widget routines
void STlib_initNum(struct doom_data_t_ *doom, st_number_t *n,
                   int x,
                   int y,
                   patch_t **pl,
                   int *num,
                   boolean *on,
                   int width);

void STlib_updateNum(struct doom_data_t_ *doom, st_number_t *n,
                     boolean refresh);

// Percent widget routines
void STlib_initPercent(struct doom_data_t_ *doom, st_percent_t *p,
                       int x,
                       int y,
                       patch_t **pl,
                       int *num,
                       boolean *on,
                       patch_t *percent);

void STlib_updatePercent(struct doom_data_t_ *doom, st_percent_t *per,
                         int refresh);

// Multiple Icon widget routines
void STlib_initMultIcon(struct doom_data_t_ *doom, st_multicon_t *mi,
                        int x,
                        int y,
                        patch_t **il,
                        int *inum,
                        boolean *on);

void STlib_updateMultIcon(struct doom_data_t_ *doom, st_multicon_t *mi,
                          boolean refresh);

// Binary Icon widget routines

void STlib_initBinIcon(struct doom_data_t_ *doom, st_binicon_t *b,
                       int x,
                       int y,
                       patch_t *i,
                       boolean *val,
                       boolean *on);

void STlib_updateBinIcon(struct doom_data_t_ *doom,
                         st_binicon_t *bi,
                         boolean refresh);

#endif
