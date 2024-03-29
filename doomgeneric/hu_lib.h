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

#ifndef __HULIB__
#define __HULIB__

// font stuff
#define HU_CHARERASE	KEY_BACKSPACE

#define HU_MAXLINES		4

#define HU_MAXLINELENGTH	80

//
// Typedefs of widgets
//

struct patch_s;

// Text Line widget
//  (parent of Scrolling Text and Input Text widgets)
typedef struct
{
    // left-justified position of scrolling text window
    int		x;
    int		y;
    
    struct patch_s**	f;			// font
    int		sc;			// start character
    char	l[HU_MAXLINELENGTH+1];	// line of text
    int		len;		      	// current line length

    // whether this line needs to be udpated
    int		needsupdate;	      

} hu_textline_t;



// Scrolling Text window widget
//  (child of Text Line widget)
typedef struct
{
    hu_textline_t	l[HU_MAXLINES];	// text lines to draw
    int			h;		// height in lines
    int			cl;		// current line number

    // pointer to boolean stating whether to update window
    boolean*		on;
    boolean		laston;		// last value of *->on.

} hu_stext_t;



// Input Text Line widget
//  (child of Text Line widget)
typedef struct
{
    hu_textline_t	l;		// text line to input on

     // left margin past which I am not to delete characters
    int			lm;

    // pointer to boolean stating whether to update window
    boolean*		on; 
    boolean		laston; // last value of *->on;

} hu_itext_t;


//
// Widget creation, access, and update routines
//

struct doom_data_t_;

// initializes heads-up widget library
void HUlib_init(struct doom_data_t_* doom);

//
// textline code
//

// clear a line of text
void	HUlib_clearTextLine(struct doom_data_t_* doom, hu_textline_t *t);

void	HUlib_initTextLine(struct doom_data_t_* doom, hu_textline_t *t, int x, int y, struct patch_s **f, int sc);

// returns success
boolean HUlib_addCharToTextLine(struct doom_data_t_* doom, hu_textline_t *t, char ch);

// returns success
boolean HUlib_delCharFromTextLine(struct doom_data_t_* doom, hu_textline_t *t);

// draws tline
void	HUlib_drawTextLine(struct doom_data_t_* doom, hu_textline_t *l, boolean drawcursor);

// erases text line
void	HUlib_eraseTextLine(struct doom_data_t_* doom, hu_textline_t *l); 


//
// Scrolling Text window widget routines
//

// ?
void
HUlib_initSText
( struct doom_data_t_* doom,
  hu_stext_t*	s,
  int		x,
  int		y,
  int		h,
  struct patch_s**	font,
  int		startchar,
  boolean*	on );

// add a new line
void HUlib_addLineToSText(struct doom_data_t_* doom, hu_stext_t* s);  

// ?
void
HUlib_addMessageToSText
( struct doom_data_t_* doom,
  hu_stext_t*	s,
  const char*		prefix,
  const char*		msg );

// draws stext
void HUlib_drawSText(struct doom_data_t_* doom, hu_stext_t* s);

// erases all stext lines
void HUlib_eraseSText(struct doom_data_t_* doom, hu_stext_t* s); 

// Input Text Line widget routines
void
HUlib_initIText
( struct doom_data_t_* doom, 
  hu_itext_t*	it,
  int		x,
  int		y,
  struct patch_s**	font,
  int		startchar,
  boolean*	on );

// enforces left margin
void HUlib_delCharFromIText(struct doom_data_t_* doom, hu_itext_t* it);

// enforces left margin
void HUlib_eraseLineFromIText(struct doom_data_t_* doom, hu_itext_t* it);

// resets line and left margin
void HUlib_resetIText(struct doom_data_t_* doom, hu_itext_t* it);

// left of left-margin
void
HUlib_addPrefixToIText
( struct doom_data_t_* doom, 
  hu_itext_t*	it,
  char*		str );

// whether eaten
boolean
HUlib_keyInIText
( struct doom_data_t_* doom, 
  hu_itext_t*	it,
  unsigned char ch );

void HUlib_drawIText(struct doom_data_t_* doom, hu_itext_t* it);

// erases all itext lines
void HUlib_eraseIText(struct doom_data_t_* doom, hu_itext_t* it); 

#endif
