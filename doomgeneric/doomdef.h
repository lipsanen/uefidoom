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
//  Internally used data structures for virtually everything,
//   lots of other stuff.
//

#ifndef __DOOMDEF__
#define __DOOMDEF__

#include <stdint.h>
#include "doomtype.h"
#include "net_defs.h"
#include "d_event.h"
#include "m_cheat.h"
#include "m_fixed.h"
#include "v_patch.h"
#include "i_timer.h"
#include "d_mode.h"

typedef struct
{
    fixed_t x, y;
} mpoint_t;

typedef struct
{
    int x, y;
} fpoint_t;

typedef struct
{
    fpoint_t a, b;
} fline_t;

typedef struct
{
    mpoint_t a, b;
} mline_t;

typedef struct
{
    fixed_t slp, islp;
} islope_t;

#define AM_NUMMARKPOINTS 10
#define MAXEVENTS 64

struct player_s;

typedef struct
{
    ticcmd_t cmds[NET_MAXPLAYERS];
    boolean ingame[NET_MAXPLAYERS];
} ticcmd_set_t;

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo.
typedef enum
{
    GS_LEVEL,
    GS_INTERMISSION,
    GS_FINALE,
    GS_DEMOSCREEN,
} gamestate_t;

struct loop_interface_t_;
struct lumpinfo_s;
struct wbplayerstruct_s;
struct wbstartstruct_s;
struct _wad_file_s;

typedef boolean (*vpatchclipfunc_t)(patch_t *, int, int);

// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS 4

typedef enum
{
    NoState = -1,
    StatCount,
    ShowNextLoc,
} stateenum_t;

struct doom_data_t_
{
    int leveljuststarted; // kluge until AM_LevelInit() is called
    cheatseq_t cheat_amap;
    int cheating;
    mpoint_t m_paninc;
    patch_t *marknums[10];
    byte *fb; // pseudo-frame buffer
    uint8_t should_quit;
    mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
    int markpointnum;                      // next point to be assigned
    int followplayer;                      // specifies whether to follow the player around
    boolean stopped;
    int grid;
    struct player_s *plr; // the player represented by an arrow

    // used by MTOF to scale from map-to-frame-buffer coords
    fixed_t scale_mtof;
    // used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
    fixed_t scale_ftom;

    // location of window on screen
    int f_x;
    int f_y;

    // size of window on screen
    int f_w;
    int f_h;

    int lightlev; // used for funky strobing effect
    int amclock;

    fixed_t mtof_zoommul; // how far the window zooms in each tic (map coords)
    fixed_t ftom_zoommul; // how far the window zooms in each tic (fb coords)

    fixed_t m_x, m_y;   // LL x,y where the window is on the map (map coords)
    fixed_t m_x2, m_y2; // UR x,y where the window is on the map (map coords)

    //
    // width/height of window on map (map coords)
    //
    fixed_t m_w;
    fixed_t m_h;

    // based on level size
    fixed_t min_x;
    fixed_t min_y;
    fixed_t max_x;
    fixed_t max_y;

    fixed_t max_w; // max_x-min_x,
    fixed_t max_h; // max_y-min_y

    // based on player size
    fixed_t min_w;
    fixed_t min_h;

    fixed_t min_scale_mtof; // used to tell when to stop zooming out
    fixed_t max_scale_mtof; // used to tell when to stop zooming in

    // old stuff for recovery later
    fixed_t old_m_w, old_m_h;
    fixed_t old_m_x, old_m_y;

    // old location used by the Follower routine
    mpoint_t f_oldloc;
    boolean automapactive;

    event_t events[MAXEVENTS];
    int eventhead;
    int eventtail;

    // The complete set of data for a particular tic.

    //
    // gametic is the tic about to (or currently being) run
    // maketic is the tic that hasn't had control made for it yet
    // recvtic is the latest tic received from the server.
    //
    // a gametic cannot be run until ticcmds are received for it
    // from all players.
    //
    ticcmd_set_t ticdata[BACKUPTICS];

    // The index of the next tic to be made (with a call to BuildTiccmd).
    int maketic;

    // The number of complete tics received from the server so far.
    int recvtic;

    // The number of tics that have been run (using RunTic) so far.
    int gametic;

    // When set to true, a single tic is run each time TryRunTics() is called.
    // This is used for -timedemo mode.
    boolean singletics;

    // Index of the local player.
    int localplayer;

    // Used for original sync code.
    int skiptics;

    // Reduce the bandwidth needed by sampling game input less and transmitting
    // less.  If ticdup is 2, sample half normal, 3 = one third normal, etc.
    int ticdup;

    // Amount to offset the timer for game sync.
    fixed_t offsetms;

    // Use new client syncronisation code
    boolean new_sync;

    // Callback functions for loop code.
    struct loop_interface_t_ *loop_interface;

    // Current players in the multiplayer game.
    // This is distinct from playeringame[] used by the game code, which may
    // modify playeringame[] when playing back multiplayer demos.
    boolean local_playeringame[NET_MAXPLAYERS];

    // Requested player class "sent" to the server on connect.
    // If we are only doing a single player game then this needs to be remembered
    // and saved in the game settings.
    int player_class;

    // Location where savegames are stored

    char *savegamedir;

    // location of IWAD and WAD files

    const char *iwadfile;

    boolean devparm;     // started game with -devparm
    boolean nomonsters;  // checkparm of -nomonsters
    boolean respawnparm; // checkparm of -respawn
    boolean fastparm;    // checkparm of -fast

    skill_t startskill;
    int startepisode;
    int startmap;
    boolean autostart;
    int startloadgame;

    boolean advancedemo;

    // Store demo, do not accept any inputs
    boolean storedemo;

    // "BFG Edition" version of doom2.wad does not include TITLEPIC.
    boolean bfgedition;

    // If true, the main game loop has started.
    boolean main_loop_started;

    char wadfile[1024]; // primary wad file
    char mapdir[1024];  // directory of development maps

    int show_endoom;

    // D_Display
    //  draw current display, possibly wiping it from the previous
    //

    // wipegamestate can be set to -1 to force a wipe on the next draw
    gamestate_t wipegamestate;
    boolean wipe;

    //
    //  DEMO LOOP
    //
    int demosequence;
    int pagetic;
    char *pagename;

    boolean viewactivestate;
    boolean menuactivestate;
    boolean inhelpscreensstate;
    boolean fullscreen;
    gamestate_t oldgamestate;
    int borderdrawcount;
    ticcmd_t *netcmds;

    // Game Mode - identify IWAD as shareware, retail etc.
    GameMode_t gamemode;
    GameMission_t gamemission;
    GameVersion_t gameversion;
    char *gamedescription;

    // Set if homebrew PWAD stuff has been added.
    boolean modifiedgame;

    int st_palette;

    //
    // GLOBALS
    //

    // Location of each lump on disk.
    struct lumpinfo_s *lumpinfo;
    unsigned int numlumps;

    // Hash table for fast lookups

    struct lumpinfo_s **lumphash;

    uint32_t *DG_ScreenBuffer;

    int myargc;
    char **myargv;

    boolean drone;

    int lastlevel, lastepisode;

    int nexttic;
    int litelevelscnt;
    char exitmsg[80];

    int acceleratestage;

    // wbs->pnum
    int me;

    // specifies current state
    stateenum_t state;

    // contains information passed into intermission
    struct wbstartstruct_s *wbs;

    struct wbplayerstruct_s *plrs; // wbs->plyr[]

    // used for general timing
    int cnt;

    // used for timing of background animation
    int bcnt;

    // signals to refresh everything for one frame
    int firstrefresh;

    int cnt_kills[MAXPLAYERS];
    int cnt_items[MAXPLAYERS];
    int cnt_secret[MAXPLAYERS];
    int cnt_time;
    int cnt_par;
    int cnt_pause;

    // # of commercial levels
    int NUMCMAPS;

    //
    //	GRAPHICS
    //

    // You Are Here graphic
    patch_t *yah[3];

    // splat
    patch_t *splat[2];

    // %, : graphics
    patch_t *percent;
    patch_t *colon;

    // 0-9 graphic
    patch_t *num[10];

    // minus sign
    patch_t *wiminus;

    // "Finished!" graphics
    patch_t *finished;

    // "Entering" graphic
    patch_t *entering;

    // "secret"
    patch_t *sp_secret;

    // "Kills", "Scrt", "Items", "Frags"
    patch_t *kills;
    patch_t *secret;
    patch_t *items;
    patch_t *frags;

    // Time sucks.
    patch_t *timepatch;
    patch_t *par;
    patch_t *sucks;

    // "killers", "victims"
    patch_t *killers;
    patch_t *victims;

    // "Total", your face, your dead face
    patch_t *total;
    patch_t *star;
    patch_t *bstar;

    // "red P[1..MAXPLAYERS]"
    patch_t *p[MAXPLAYERS];

    // "gray P[1..MAXPLAYERS]"
    patch_t *bp[MAXPLAYERS];

    // Name graphics of each level (centered)
    patch_t **lnames;

    // Buffer storing the backdrop
    patch_t *background;

    boolean snl_pointeron;

    int dm_state;
    int dm_frags[MAXPLAYERS][MAXPLAYERS];
    int dm_totals[MAXPLAYERS];
    int cnt_frags[MAXPLAYERS];
    int dofrags;
    int ng_state;
    int sp_state;

    struct _wad_file_s **open_wadfiles;
    int num_open_wadfiles;

    // Blending table used for fuzzpatch, etc.
    // Only used in Heretic/Hexen

    byte *tinttable;

    // villsa [STRIFE] Blending table used for Strife
    byte *xlatab;

    // The screen buffer that the v_video.c code draws to.

    byte *dest_screen;

    int dirtybox[4];

    // haleyjd 08/28/10: clipping callback function for patches.
    // This is needed for Chocolate Strife, which clips patches to the screen.
    vpatchclipfunc_t patchclip_callback;
};

typedef struct doom_data_t_ doom_data_t;
void doomdata_init(doom_data_t *doom);

//
// Global parameters/defines.
//
// DOOM version
#define DOOM_VERSION 109

// Version code for cph's longtics hack ("v1.91")
#define DOOM_191_VERSION 111

// If rangecheck is undefined,
// most parameter validation debugging code will not be compiled
#define RANGECHECK

typedef enum
{
    ga_nothing,
    ga_loadlevel,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_playdemo,
    ga_completed,
    ga_victory,
    ga_worlddone,
    ga_screenshot
} gameaction_t;

//
// Difficulty/skill settings/filters.
//

// Skill flags.
#define MTF_EASY 1
#define MTF_NORMAL 2
#define MTF_HARD 4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH 8

//
// Key cards.
//
typedef enum
{
    it_bluecard,
    it_yellowcard,
    it_redcard,
    it_blueskull,
    it_yellowskull,
    it_redskull,

    NUMCARDS

} card_t;

// The defined weapons,
//  including a marker indicating
//  user has not changed weapon.
typedef enum
{
    wp_fist,
    wp_pistol,
    wp_shotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg,
    wp_chainsaw,
    wp_supershotgun,

    NUMWEAPONS,

    // No pending weapon change.
    wp_nochange

} weapontype_t;

// Ammunition types defined.
typedef enum
{
    am_clip,  // Pistol / chaingun ammo.
    am_shell, // Shotgun / double barreled shotgun.
    am_cell,  // Plasma rifle, BFG.
    am_misl,  // Missile launcher.
    NUMAMMO,
    am_noammo // Unlimited for chainsaw / fist.

} ammotype_t;

// Power up artifacts.
typedef enum
{
    pw_invulnerability,
    pw_strength,
    pw_invisibility,
    pw_ironfeet,
    pw_allmap,
    pw_infrared,
    NUMPOWERS

} powertype_t;

//
// Power up durations,
//  how many seconds till expiration,
//  assuming TICRATE is 35 ticks/second.
//
typedef enum
{
    INVULNTICS = (30 * TICRATE),
    INVISTICS = (60 * TICRATE),
    INFRATICS = (120 * TICRATE),
    IRONTICS = (60 * TICRATE)

} powerduration_t;

#endif // __DOOMDEF__
