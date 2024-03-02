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
#include "hu_lib.h"
#include "m_cheat.h"
#include "m_fixed.h"
#include "v_patch.h"
#include "i_timer.h"
#include "d_mode.h"
#include "p_pspr.h"

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
//
// Globally visible constants.
//
#define HU_FONTSTART	'!'	// the first font characters
#define HU_FONTEND	'_'	// the last font characters

// Calculate # of glyphs in font.
#define HU_FONTSIZE	(HU_FONTEND - HU_FONTSTART + 1)	


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


// States for status bar code.
typedef enum
{
    AutomapState,
    FirstPersonState
    
} st_stateenum_t;


// States for the chat code.
typedef enum
{
    StartChatState,
    WaitDestState,
    GetChatState
    
} st_chatstateenum_t;

struct loop_interface_t_;
struct lumpinfo_s;
struct wbplayerstruct_s;
struct wbstartstruct_s;
struct _wad_file_s;
struct player_s;
struct mobj_s;

typedef struct
{
    // upper right-hand corner
    //  of the number (right-justified)
    int x;
    int y;

    // max # of digits in number
    int width;

    // last number value
    int oldnum;

    // pointer to current value
    int *num;

    // pointer to boolean stating
    //  whether to update number
    boolean *on;

    // list of patches for 0-9
    patch_t **p;

    // user data
    int data;

} st_number_t;

// Percent widget ("child" of number widget,
//  or, more precisely, contains a number widget.)
typedef struct
{
    // number information
    st_number_t n;

    // percent sign graphic
    patch_t *p;

} st_percent_t;

// Multiple Icon widget
typedef struct
{
    // center-justified location of icons
    int x;
    int y;

    // last icon number
    int oldinum;

    // pointer to current icon
    int *inum;

    // pointer to boolean stating
    //  whether to update icon
    boolean *on;

    // list of icons
    patch_t **p;

    // user data
    int data;

} st_multicon_t;

// Binary Icon widget

typedef struct
{
    // center-justified location of icon
    int x;
    int y;

    // last icon value
    boolean oldval;

    // pointer to current icon status
    boolean *val;

    // pointer to boolean
    //  stating whether to update icon
    boolean *on;

    patch_t *p; // icon
    int data;   // user data

} st_binicon_t;


typedef boolean (*vpatchclipfunc_t)(patch_t *, int, int);

// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS 4

// Number of status faces.
#define ST_NUMPAINFACES 5
#define ST_NUMSTRAIGHTFACES 3
#define ST_NUMTURNFACES 2
#define ST_NUMSPECIALFACES 3

#define ST_FACESTRIDE \
    (ST_NUMSTRAIGHTFACES + ST_NUMTURNFACES + ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES 2

#define ST_NUMFACES \
    (ST_FACESTRIDE * ST_NUMPAINFACES + ST_NUMEXTRAFACES)

typedef enum
{
    NoState = -1,
    StatCount,
    ShowNextLoc,
} stateenum_t;

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

#define MAX_MOUSE_BUTTONS 8

typedef enum
{
    F_STAGE_TEXT,
    F_STAGE_ARTSCREEN,
    F_STAGE_CAST,
} finalestage_t;

//
// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct wbplayerstruct_s
{
    boolean	in;	// whether the player is in game
    
    // Player stats, kills, collected items etc.
    int		skills;
    int		sitems;
    int		ssecret;
    int		stime; 
    int		frags[4];
    int		score;	// current score on entry, modified on return
  
} wbplayerstruct_t;

typedef struct wbstartstruct_s
{
    int		epsd;	// episode # (0-2)

    // if true, splash the secret level
    boolean	didsecret;
    
    // previous and next levels, origin 0
    int		last;
    int		next;	
    
    int		maxkills;
    int		maxitems;
    int		maxsecret;
    int		maxfrags;

    // the par time
    int		partime;
    
    // index of this player in game
    int		pnum;	

    wbplayerstruct_t	plyr[MAXPLAYERS];

} wbstartstruct_t;


//
// Player states.
//
typedef enum
{
    // Playing or camping.
    PST_LIVE,
    // Dead on the ground, view follows killer.
    PST_DEAD,
    // Ready to restart/respawn???
    PST_REBORN		

} playerstate_t;


//
// Player internal flags, for cheats and debug.
//
typedef enum
{
    // No clipping, walk through barriers.
    CF_NOCLIP		= 1,
    // No damage, no health loss.
    CF_GODMODE		= 2,
    // Not really a cheat, just a debug aid.
    CF_NOMOMENTUM	= 4

} cheat_t;


//
// Extended player object info: player_t
//
typedef struct player_s
{
    struct mobj_s*		mo;
    playerstate_t	playerstate;
    ticcmd_t		cmd;

    // Determine POV,
    //  including viewpoint bobbing during movement.
    // Focal origin above r.z
    fixed_t		viewz;
    // Base height above floor for viewz.
    fixed_t		viewheight;
    // Bob/squat speed.
    fixed_t         	deltaviewheight;
    // bounded/scaled total momentum.
    fixed_t         	bob;	

    // This is only used between levels,
    // mo->health is used during levels.
    int			health;	
    int			armorpoints;
    // Armor type is 0-2.
    int			armortype;	

    // Power ups. invinc and invis are tic counters.
    int			powers[NUMPOWERS];
    boolean		cards[NUMCARDS];
    boolean		backpack;
    
    // Frags, kills of other players.
    int			frags[MAXPLAYERS];
    weapontype_t	readyweapon;
    
    // Is wp_nochange if not changing.
    weapontype_t	pendingweapon;

    boolean		weaponowned[NUMWEAPONS];
    int			ammo[NUMAMMO];
    int			maxammo[NUMAMMO];

    // True if button down last tic.
    int			attackdown;
    int			usedown;

    // Bit flags, for cheats and debug.
    // See cheat_t, above.
    int			cheats;		

    // Refired shots are less accurate.
    int			refire;		

     // For intermission stats.
    int			killcount;
    int			itemcount;
    int			secretcount;

    // Hint messages.
    char*		message;	
    
    // For screen flashing (red or bright).
    int			damagecount;
    int			bonuscount;

    // Who did damage (NULL for floors/ceilings).
     struct mobj_s*		attacker;
    
    // So gun flashes light up areas.
    int			extralight;

    // Current PLAYPAL, ???
    //  can be set to REDCOLORMAP for pain, etc.
    int			fixedcolormap;

    // Player skin colorshift,
    //  0-3 for which color to draw player.
    int			colormap;	

    // Overlay view sprites (gun, etc).
    pspdef_t		psprites[NUMPSPRITES];

    // True if secret level has been done.
    boolean		didsecret;	

} player_t;

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

    boolean st_stopped;

    // graphics are drawn to a backing screen and blitted to the real screen
    byte *st_backing_screen;

    // main player in game
    struct player_s *plyr;

    // ST_Start() has just been called
    boolean st_firsttime;

    // lump number for PLAYPAL
    int lu_palette;

    // used for timing
    unsigned int st_clock;

    // used for making messages go away
    int st_msgcounter;

    // used when in chat
    st_chatstateenum_t st_chatstate;

    // whether in automap or first-person
    st_stateenum_t st_gamestate;

    // whether left-side main status bar is active
    boolean st_statusbaron;

    // whether status bar chat is active
    boolean st_chat;

    // value of st_chat before message popped up
    boolean st_oldchat;

    // whether chat window has the cursor on
    boolean st_cursoron;

    // !deathmatch
    boolean st_notdeathmatch;

    // !deathmatch && st_statusbaron
    boolean st_armson;

    // !deathmatch
    boolean st_fragson;

    // main bar left
    patch_t *sbar;

    // 0-9, tall numbers
    patch_t *tallnum[10];

    // tall % sign
    patch_t *tallpercent;

    // 0-9, short, yellow (,different!) numbers
    patch_t *shortnum[10];

    // 3 key-cards, 3 skulls
    patch_t *keys[NUMCARDS];

    // face status patches
    patch_t *faces[ST_NUMFACES];

    // face background
    patch_t *faceback;

    // main bar right
    patch_t *armsbg;

    // weapon ownership patches
    patch_t *arms[6][2];

    // ready-weapon widget
    st_number_t w_ready;

    // in deathmatch only, summary of frags stats
    st_number_t w_frags;

    // health widget
    st_percent_t w_health;

    // arms background
    st_binicon_t w_armsbg;

    // weapon ownership widgets
    st_multicon_t w_arms[6];

    // face status widget
    st_multicon_t w_faces;

    // keycard widgets
    st_multicon_t w_keyboxes[3];

    // armor widget
    st_percent_t w_armor;

    // ammo widgets
    st_number_t w_ammo[4];

    // max ammo widgets
    st_number_t w_maxammo[4];

    // number of frags so far in deathmatch
    int st_fragscount;

    // used to use appopriately pained face
    int st_oldhealth;

    // used for evil grin
    boolean oldweaponsowned[NUMWEAPONS];

    // count until face changes
    int st_facecount;

    // current face index, used by w_faces
    int st_faceindex;

    // holds key-type for each key box on bar
    int keyboxes[3];

    // a random number per tick
    int st_randomnumber;

    int largeammo;

    int lastattackdown;
    int priority;

    int lastcalc;
    int oldhealth;

    cheatseq_t cheat_mus;
    cheatseq_t cheat_god;
    cheatseq_t cheat_ammo;
    cheatseq_t cheat_ammonokey;
    cheatseq_t cheat_noclip;
    cheatseq_t cheat_commercial_noclip;

    cheatseq_t cheat_powerup[7];
    cheatseq_t cheat_choppers;
    cheatseq_t cheat_clev;
    cheatseq_t cheat_mypos;

    patch_t *sttminus;

    // Stage of animation:
    finalestage_t finalestage;

    unsigned int finalecount;
    const char*	finaletext;
    const char*	finaleflat;

    //
    //                       SCREEN WIPE PACKAGE
    //
    boolean wipe_go;
    byte *wipe_scr_start;
    byte *wipe_scr_end;
    byte *wipe_scr;
    int *wipe_y;

    // g_game.c starts

    // Gamestate the last time G_Ticker was called.
    gamestate_t game_oldgamestate;
    gameaction_t gameaction;
    gamestate_t gamestate;
    skill_t gameskill;
    boolean respawnmonsters;
    int gameepisode;
    int gamemap;

    // If non-zero, exit the level after this number of minutes.

    int timelimit;

    boolean paused;
    boolean sendpause; // send a pause event next tic
    boolean sendsave;  // send a save event next tic
    boolean usergame;  // ok to save / end game

    boolean timingdemo; // if true, exit with report on completion
    boolean nodrawers;  // for comparative timing purposes

    boolean viewactive;

    int deathmatch;  // only if started as net death
    boolean netgame; // only true if packets are broadcast
    boolean playeringame[MAXPLAYERS];
    player_t players[MAXPLAYERS];

    boolean turbodetected[MAXPLAYERS];

    int consoleplayer;                       // player taking events and displaying
    int displayplayer;                       // view being displayed
    int levelstarttic;                       // gametic at level start
    int totalkills, totalitems, totalsecret; // for intermission

    char *demoname;
    boolean demorecording;
    boolean longtics;    // cph's doom 1.91 longtics hack
    boolean lowres_turn; // low resolution turning for longtics
    boolean demoplayback;
    boolean netdemo;
    byte *demobuffer;
    byte *demo_p;
    byte *demoend;
    boolean singledemo; // quit after playing a demo from cmdline

    boolean precache; // if true, load all graphics at start

    boolean testcontrols; // Invoked by setup to test controls
    int testcontrols_mousespeed;

    wbstartstruct_t wminfo; // parms for world map / intermission

    byte consistancy[MAXPLAYERS][BACKUPTICS];

    fixed_t forwardmove[2];
    fixed_t sidemove[2];
    fixed_t angleturn[3]; // + slow turn

    #define SLOWTURNTICS 6

    #define NUMKEYS 256
    #define MAX_JOY_BUTTONS 20

    boolean gamekeydown[NUMKEYS];
    int turnheld; // for accelerative turning

    boolean mousearray[MAX_MOUSE_BUTTONS + 1];
    boolean *mousebuttons; // allow [-1]

    // mouse values are used once
    int mousex;
    int mousey;

    int dclicktime;
    boolean dclickstate;
    int dclicks;
    int dclicktime2;
    boolean dclickstate2;
    int dclicks2;

    // joystick values are repeated
    int joyxmove;
    int joyymove;
    int joystrafemove;
    boolean joyarray[MAX_JOY_BUTTONS + 1];
    boolean *joybuttons; // allow [-1]

    int savegameslot;
    char savedescription[32];

    #define BODYQUESIZE 32

    struct mobj_s *bodyque[BODYQUESIZE];
    int bodyqueslot;

    int vanilla_savegame_limit;
    int vanilla_demo_limit;

    int next_weapon;

    unsigned short angleturn_carry;

    // g_game.c ends

    // hu_stuff.c
    int num_nobrainers;
    boolean altdown;
    char lastmessage[HU_MAXLINELENGTH + 1];
    int message_counter;
    player_t *hustuff_plr;
    patch_t *hu_font[HU_FONTSIZE];
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

//
// Difficulty/skill settings/filters.
//

// Skill flags.
#define MTF_EASY 1
#define MTF_NORMAL 2
#define MTF_HARD 4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH 8

#endif // __DOOMDEF__
