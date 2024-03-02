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
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//

#include "dlibc.h"
#include "doomgeneric.h"
#include "config.h"
#include "deh_main.h"
#include "doomdef.h"
#include "doomstat.h"

#include "dstrings.h"
#include "doomfeatures.h"
#include "sounds.h"

#include "d_iwad.h"

#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"

#include "f_finale.h"
#include "f_wipe.h"

#include "m_argv.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_misc.h"
#include "m_menu.h"
#include "p_saveg.h"

#include "i_endoom.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "g_game.h"

#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "net_client.h"
#include "net_dedicated.h"
#include "net_query.h"

#include "p_setup.h"
#include "r_local.h"

#include "d_main.h"

//
// D-DoomLoop()
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//
void D_DoomLoop(struct doom_data_t_ *doom);

extern boolean inhelpscreens;
extern boolean setsizeneeded;
extern int showMessages;

void R_ExecuteSetViewSize(void);
void D_ConnectNetGame(doom_data_t *doom);
void D_CheckNetGame(doom_data_t *doom);

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents(doom_data_t *doom)
{
    event_t *ev;

    // IF STORE DEMO, DO NOT ACCEPT INPUT
    if (doom->storedemo)
        return;

    while ((ev = D_PopEvent(doom)) != NULL)
    {
        if (M_Responder(doom, ev))
            continue; // menu ate the event
        G_Responder(doom, ev);
    }
}

static void Do_Wipe(doom_data_t *doom)
{
    // wipe update
    int nowtime;
    int tics;
    boolean done;

    tics = 1;
    done = wipe_ScreenWipe(doom, wipe_Melt, 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
    I_UpdateNoBlit();
    M_Drawer(doom);       // menu is drawn even on top of wipes
    I_FinishUpdate(doom); // page flip or blit buffer

    if (done)
    {
        doom->wipe = false;
    }
}

void D_Display(struct doom_data_t_ *doom)
{
    int y;
    boolean redrawsbar;

    if (doom->wipe)
    {
        Do_Wipe(doom);
        return;
    }

    if (doom->nodrawers)
        return; // for comparative timing / profiling

    redrawsbar = false;

    // change the view size if needed
    if (setsizeneeded)
    {
        R_ExecuteSetViewSize();
        doom->oldgamestate = -1; // force background redraw
        doom->borderdrawcount = 3;
    }

    // save the current screen if about to wipe
    if (doom->gamestate != doom->wipegamestate)
    {
        doom->wipe = true;
        wipe_StartScreen(doom, 0, 0, SCREENWIDTH, SCREENHEIGHT);
    }
    else
        doom->wipe = false;

    if (doom->gamestate == GS_LEVEL && doom->gametic)
        HU_Erase(doom);

    // do buffered drawing
    switch (doom->gamestate)
    {
    case GS_LEVEL:
        if (!doom->gametic)
            break;
        if (doom->automapactive)
            AM_Drawer(doom);
        if (doom->wipe || (viewheight != 200 && doom->fullscreen))
            redrawsbar = true;
        if (doom->inhelpscreensstate && !inhelpscreens)
            redrawsbar = true; // just put away the help screen
        ST_Drawer(doom, viewheight == 200, redrawsbar);
        doom->fullscreen = viewheight == 200;
        break;

    case GS_INTERMISSION:
        WI_Drawer(doom);
        break;

    case GS_FINALE:
        F_Drawer();
        break;

    case GS_DEMOSCREEN:
        D_PageDrawer(doom);
        break;
    }

    // draw buffered stuff to screen
    I_UpdateNoBlit();

    // draw the view directly
    if (doom->gamestate == GS_LEVEL && !doom->automapactive && doom->gametic)
        R_RenderPlayerView(doom, &doom->players[doom->displayplayer]);

    if (doom->gamestate == GS_LEVEL && doom->gametic)
        HU_Drawer(doom);

    // clean up border stuff
    if (doom->gamestate != doom->oldgamestate && doom->gamestate != GS_LEVEL)
        I_SetPalette(W_CacheLumpName(doom, DEH_String("PLAYPAL"), PU_CACHE));

    // see if the border needs to be initially drawn
    if (doom->gamestate == GS_LEVEL && doom->oldgamestate != GS_LEVEL)
    {
        doom->viewactivestate = false; // view was not active
        R_FillBackScreen(doom);        // draw the pattern into the back screen
    }

    // see if the border needs to be updated to the screen
    if (doom->gamestate == GS_LEVEL && !doom->automapactive && scaledviewwidth != SCREENWIDTH)
    {
        if (menuactive || doom->menuactivestate || !doom->viewactivestate)
            doom->borderdrawcount = 3;
        if (doom->borderdrawcount)
        {
            R_DrawViewBorder(doom); // erase old menu stuff
            doom->borderdrawcount--;
        }
    }

    if (doom->testcontrols)
    {
        // Box showing current mouse speed

        V_DrawMouseSpeedBox(doom, doom->testcontrols_mousespeed);
    }

    doom->menuactivestate = menuactive;
    doom->viewactivestate = doom->viewactive;
    doom->inhelpscreensstate = inhelpscreens;
    doom->oldgamestate = doom->wipegamestate = doom->gamestate;

    // draw pause pic
    if (doom->paused)
    {
        if (doom->automapactive)
            y = 4;
        else
            y = viewwindowy + 4;
        V_DrawPatchDirect(doom, viewwindowx + (scaledviewwidth - 68) / 2, y,
                          W_CacheLumpName(doom, DEH_String("M_PAUSE"), PU_CACHE));
    }

    // menus go directly to the screen
    M_Drawer(doom);      // menu is drawn even on top of everything
    NetUpdate(doom); // send out any new accumulation

    // normal update
    if (!doom->wipe)
    {
        I_FinishUpdate(doom); // page flip or blit buffer
        return;
    }

    wipe_EndScreen(doom, 0, 0, SCREENWIDTH, SCREENHEIGHT);
}

//
// Add configuration file variable bindings.
//

void D_BindVariables(doom_data_t *doom)
{
    int i;

    M_ApplyPlatformDefaults();

    I_BindVideoVariables();
    I_BindSoundVariables();

    M_BindBaseControls();
    M_BindWeaponControls();
    M_BindMapControls();
    M_BindMenuControls();
    M_BindChatControls(MAXPLAYERS);

    key_multi_msgplayer[0] = HUSTR_KEYGREEN;
    key_multi_msgplayer[1] = HUSTR_KEYINDIGO;
    key_multi_msgplayer[2] = HUSTR_KEYBROWN;
    key_multi_msgplayer[3] = HUSTR_KEYRED;

#ifdef FEATURE_MULTIPLAYER
    NET_BindVariables();
#endif

    M_BindVariable("mouse_sensitivity", &mouseSensitivity);
    M_BindVariable("sfx_volume", &sfxVolume);
    M_BindVariable("music_volume", &musicVolume);
    M_BindVariable("show_messages", &showMessages);
    M_BindVariable("screenblocks", &screenblocks);
    M_BindVariable("detaillevel", &detailLevel);
    M_BindVariable("snd_channels", &snd_channels);
    M_BindVariable("vanilla_savegame_limit", &doom->vanilla_savegame_limit);
    M_BindVariable("vanilla_demo_limit", &doom->vanilla_demo_limit);
    M_BindVariable("show_endoom", &doom->show_endoom);

    // Multiplayer chat macros

    for (i = 0; i < 10; ++i)
    {
        char buf[12];

        d_snprintf(buf, sizeof(buf), "chatmacro%i", i);
        M_BindVariable(buf, &chat_macros[i]);
    }
}

//
// D_GrabMouseCallback
//
// Called to determine whether to grab the mouse pointer
//

boolean D_GrabMouseCallback(doom_data_t *doom)
{
    // Drone players don't need mouse focus

    if (doom->drone)
        return false;

    // when menu is active or game is paused, release the mouse

    if (menuactive || doom->paused)
        return false;

    // only grab mouse when playing levels (but not demos)

    return (doom->gamestate == GS_LEVEL) && !doom->demoplayback && !doom->advancedemo;
}

void doomgeneric_Tick(struct doom_data_t_ *doom)
{
    // frame syncronous IO operations
    I_StartFrame();

    TryRunTics(doom); // will run at least one tic

    S_UpdateSounds(doom, doom->players[doom->consoleplayer].mo); // move positional sounds

    // Update display, next frame, with current state.
    if (screenvisible)
    {
        D_Display(doom);
    }
}

//
//  D_DoomLoop
//
void D_DoomLoop(struct doom_data_t_ *doom)
{
    if (doom->bfgedition &&
        (doom->demorecording || (doom->gameaction == ga_playdemo) || doom->netgame))
    {
        d_printf(" WARNING: You are playing using one of the Doom Classic\n"
                 " IWAD files shipped with the Doom 3: BFG Edition. These are\n"
                 " known to be incompatible with the regular IWAD files and\n"
                 " may cause demos and network games to get out of sync.\n");
    }

    if (doom->demorecording)
        G_BeginRecording(doom);

    doom->main_loop_started = true;

    TryRunTics(doom);

    I_GraphicsCheckCommandLine();
    I_SetGrabMouseCallback(D_GrabMouseCallback);
    I_InitGraphics(doom);
    I_EnableLoadingDisk();

    V_RestoreBuffer(doom);
    R_ExecuteSetViewSize();

    D_StartGameLoop();

    if (doom->testcontrols)
    {
        doom->wipegamestate = doom->gamestate;
    }

    doomgeneric_Tick(doom);
}

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(doom_data_t *doom)
{
    if (--doom->pagetic < 0)
        D_AdvanceDemo(doom);
}

//
// D_PageDrawer
//
void D_PageDrawer(doom_data_t *doom)
{
    V_DrawPatch(doom, 0, 0, W_CacheLumpName(doom, doom->pagename, PU_CACHE));
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo(doom_data_t *doom)
{
    doom->advancedemo = true;
}

//
// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
//
void D_DoAdvanceDemo(doom_data_t *doom)
{
    doom->players[doom->consoleplayer].playerstate = PST_LIVE; // not reborn
    doom->advancedemo = false;
    doom->usergame = false; // no save / end game here
    doom->paused = false;
    doom->gameaction = ga_nothing;

    // The Ultimate Doom executable changed the demo sequence to add
    // a DEMO4 demo.  Final Doom was based on Ultimate, so also
    // includes this change; however, the Final Doom IWADs do not
    // include a DEMO4 lump, so the game bombs out with an error
    // when it reaches this point in the demo sequence.

    // However! There is an alternate version of Final Doom that
    // includes a fixed executable.

    if (doom->gameversion == exe_ultimate || doom->gameversion == exe_final)
        doom->demosequence = (doom->demosequence + 1) % 7;
    else
        doom->demosequence = (doom->demosequence + 1) % 6;

    switch (doom->demosequence)
    {
    case 0:
        if (doom->gamemode == commercial)
            doom->pagetic = TICRATE * 11;
        else
            doom->pagetic = 170;
        doom->gamestate = GS_DEMOSCREEN;
        doom->pagename = DEH_String("TITLEPIC");
        if (doom->gamemode == commercial)
            S_StartMusic(doom, mus_dm2ttl);
        else
            S_StartMusic(doom, mus_intro);
        break;
    case 1:
        G_DeferedPlayDemo(doom, DEH_String("demo1"));
        break;
    case 2:
        doom->pagetic = 200;
        doom->gamestate = GS_DEMOSCREEN;
        doom->pagename = DEH_String("CREDIT");
        break;
    case 3:
        G_DeferedPlayDemo(doom, DEH_String("demo2"));
        break;
    case 4:
        doom->gamestate = GS_DEMOSCREEN;
        if (doom->gamemode == commercial)
        {
            doom->pagetic = TICRATE * 11;
            doom->pagename = DEH_String("TITLEPIC");
            S_StartMusic(doom, mus_dm2ttl);
        }
        else
        {
            doom->pagetic = 200;

            if (doom->gamemode == retail)
                doom->pagename = DEH_String("CREDIT");
            else
                doom->pagename = DEH_String("HELP2");
        }
        break;
    case 5:
        G_DeferedPlayDemo(doom, DEH_String("demo3"));
        break;
        // THE DEFINITIVE DOOM Special Edition demo
    case 6:
        G_DeferedPlayDemo(doom, DEH_String("demo4"));
        break;
    }

    // The Doom 3: BFG Edition version of doom2.wad does not have a
    // TITLETPIC lump. Use INTERPIC instead as a workaround.
    if (doom->bfgedition && !d_stricmp(doom->pagename, "TITLEPIC") && W_CheckNumForName(doom, "titlepic") < 0)
    {
        doom->pagename = DEH_String("INTERPIC");
    }
}

//
// D_StartTitle
//
void D_StartTitle(doom_data_t *doom)
{
    doom->gameaction = ga_nothing;
    doom->demosequence = -1;
    D_AdvanceDemo(doom);
}

// Strings for dehacked replacements of the startup banner
//
// These are from the original source: some of them are perhaps
// not used in any dehacked patches

static char *banners[] =
    {
        // doom2.wad
        "                         "
        "DOOM 2: Hell on Earth v%i.%i"
        "                           ",
        // doom1.wad
        "                            "
        "DOOM Shareware Startup v%i.%i"
        "                           ",
        // doom.wad
        "                            "
        "DOOM Registered Startup v%i.%i"
        "                           ",
        // Registered DOOM uses this
        "                          "
        "DOOM System Startup v%i.%i"
        "                          ",
        // doom.wad (Ultimate DOOM)
        "                         "
        "The Ultimate DOOM Startup v%i.%i"
        "                        ",
        // tnt.wad
        "                     "
        "DOOM 2: TNT - Evilution v%i.%i"
        "                           ",
        // plutonia.wad
        "                   "
        "DOOM 2: Plutonia Experiment v%i.%i"
        "                           ",
};

//
// Get game name: if the startup banner has been replaced, use that.
// Otherwise, use the name given
//

static char *GetGameName(doom_data_t *doom, char *gamename)
{
    size_t i;
    char *deh_sub;

    for (i = 0; i < arrlen(banners); ++i)
    {
        // Has the banner been replaced?

        deh_sub = DEH_String(banners[i]);

        if (deh_sub != banners[i])
        {
            size_t gamename_size;
            int version;

            // Has been replaced.
            // We need to expand via printf to include the Doom version number
            // We also need to cut off spaces to get the basic name

            gamename_size = d_strlen(deh_sub) + 10;
            gamename = Z_Malloc(gamename_size, PU_STATIC, 0);
            version = G_VanillaVersionCode(doom);
            d_snprintf(gamename, gamename_size, deh_sub,
                       version / 100, version % 100);

            while (gamename[0] != '\0' && d_isspace((int)gamename[0]))
            {
                d_memmove(gamename, gamename + 1, gamename_size - 1);
            }

            while (gamename[0] != '\0' && d_isspace((int)gamename[d_strlen(gamename) - 1]))
            {
                gamename[d_strlen(gamename) - 1] = '\0';
            }

            return gamename;
        }
    }

    return gamename;
}

static void SetMissionForPackName(doom_data_t *doom, char *pack_name)
{
    int i;
    static const struct
    {
        char *name;
        int mission;
    } packs[] = {
        {"doom2", doom2},
        {"tnt", pack_tnt},
        {"plutonia", pack_plut},
    };

    for (i = 0; i < arrlen(packs); ++i)
    {
        if (!d_stricmp(pack_name, packs[i].name))
        {
            doom->gamemission = packs[i].mission;
            return;
        }
    }

    d_printf("Valid mission packs are:\n");

    for (i = 0; i < arrlen(packs); ++i)
    {
        d_printf("\t%s\n", packs[i].name);
    }

    I_Error("Unknown mission pack name: %s", pack_name);
}

//
// Find out what version of Doom is playing.
//

void D_IdentifyVersion(doom_data_t *doom)
{
    // gamemission is set up by the D_FindIWAD function.  But if
    // we specify '-iwad', we have to identify using
    // IdentifyIWADByName.  However, if the iwad does not match
    // any known IWAD name, we may have a dilemma.  Try to
    // identify by its contents.

    if (doom->gamemission == none)
    {
        unsigned int i;

        for (i = 0; i < doom->numlumps; ++i)
        {
            if (!d_strnicmp(doom->lumpinfo[i].name, "MAP01", 8))
            {
                doom->gamemission = doom2;
                break;
            }
            else if (!d_strnicmp(doom->lumpinfo[i].name, "E1M1", 8))
            {
                doom->gamemission = doom1;
                break;
            }
        }

        if (doom->gamemission == none)
        {
            // Still no idea.  I don't think this is going to work.

            I_Error("Unknown or invalid IWAD file.");
        }
    }

    // Make sure gamemode is set up correctly

    if (logical_gamemission == doom1)
    {
        // Doom 1.  But which version?

        if (W_CheckNumForName(doom, "E4M1") > 0)
        {
            // Ultimate Doom

            doom->gamemode = retail;
        }
        else if (W_CheckNumForName(doom, "E3M1") > 0)
        {
            doom->gamemode = registered;
        }
        else
        {
            doom->gamemode = shareware;
        }
    }
    else
    {
        int p;

        // Doom 2 of some kind.
        doom->gamemode = commercial;

        // We can manually override the gamemission that we got from the
        // IWAD detection code. This allows us to eg. play Plutonia 2
        // with Freedoom and get the right level names.

        //!
        // @arg <pack>
        //
        // Explicitly specify a Doom II "mission pack" to run as, instead of
        // detecting it based on the filename. Valid values are: "doom2",
        // "tnt" and "plutonia".
        //
        p = M_CheckParmWithArgs(doom, "-pack", 1);
        if (p > 0)
        {
            SetMissionForPackName(doom, doom->myargv[p + 1]);
        }
    }
}

// Set the gamedescription string

void D_SetGameDescription(doom_data_t *doom)
{
    boolean is_freedoom = W_CheckNumForName(doom, "FREEDOOM") >= 0,
            is_freedm = W_CheckNumForName(doom, "FREEDM") >= 0;

    doom->gamedescription = "Unknown";

    if (logical_gamemission == doom1)
    {
        // Doom 1.  But which version?

        if (is_freedoom)
        {
            doom->gamedescription = GetGameName(doom, "Freedoom: Phase 1");
        }
        else if (doom->gamemode == retail)
        {
            // Ultimate Doom

            doom->gamedescription = GetGameName(doom, "The Ultimate DOOM");
        }
        else if (doom->gamemode == registered)
        {
            doom->gamedescription = GetGameName(doom, "DOOM Registered");
        }
        else if (doom->gamemode == shareware)
        {
            doom->gamedescription = GetGameName(doom, "DOOM Shareware");
        }
    }
    else
    {
        // Doom 2 of some kind.  But which mission?

        if (is_freedoom)
        {
            if (is_freedm)
            {
                doom->gamedescription = GetGameName(doom, "FreeDM");
            }
            else
            {
                doom->gamedescription = GetGameName(doom, "Freedoom: Phase 2");
            }
        }
        else if (logical_gamemission == doom2)
        {
            doom->gamedescription = GetGameName(doom, "DOOM 2: Hell on Earth");
        }
        else if (logical_gamemission == pack_plut)
        {
            doom->gamedescription = GetGameName(doom, "DOOM 2: Plutonia Experiment");
        }
        else if (logical_gamemission == pack_tnt)
        {
            doom->gamedescription = GetGameName(doom, "DOOM 2: TNT - Evilution");
        }
    }
}

static boolean D_AddFile(struct doom_data_t_* doom, char *filename)
{
    wad_file_t *handle;

    d_printf(" adding %s\n", filename);
    handle = W_AddFile(doom, filename);

    return handle != NULL;
}

// Copyright message banners
// Some dehacked mods replace these.  These are only displayed if they are
// replaced by dehacked.

static char *copyright_banners[] =
    {
        "===========================================================================\n"
        "ATTENTION:  This version of DOOM has been modified.  If you would like to\n"
        "get a copy of the original game, call 1-800-IDGAMES or see the readme file.\n"
        "        You will not receive technical support for modified games.\n"
        "                      press enter to continue\n"
        "===========================================================================\n",

        "===========================================================================\n"
        "                 Commercial product - do not distribute!\n"
        "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
        "===========================================================================\n",

        "===========================================================================\n"
        "                                Shareware!\n"
        "===========================================================================\n"};

// Prints a message only if it has been modified by dehacked.

void PrintDehackedBanners(void)
{
    size_t i;

    for (i = 0; i < arrlen(copyright_banners); ++i)
    {
        char *deh_s;

        deh_s = DEH_String(copyright_banners[i]);

        if (deh_s != copyright_banners[i])
        {
            d_printf("%s", deh_s);

            // Make sure the modified banner always ends in a newline character.
            // If it doesn't, add a newline.  This fixes av.wad.

            if (deh_s[d_strlen(deh_s) - 1] != '\n')
            {
                d_printf("\n");
            }
        }
    }
}

static const struct
{
    char *description;
    char *cmdline;
    GameVersion_t version;
} gameversions[] = {
    {"Doom 1.666", "1.666", exe_doom_1_666},
    {"Doom 1.7/1.7a", "1.7", exe_doom_1_7},
    {"Doom 1.8", "1.8", exe_doom_1_8},
    {"Doom 1.9", "1.9", exe_doom_1_9},
    {"Hacx", "hacx", exe_hacx},
    {"Ultimate Doom", "ultimate", exe_ultimate},
    {"Final Doom", "final", exe_final},
    {"Final Doom (alt)", "final2", exe_final2},
    {"Chex Quest", "chex", exe_chex},
    {NULL, NULL, 0},
};

// Initialize the game version

static void InitGameVersion(doom_data_t *doom)
{
    int p;
    int i;

    //!
    // @arg <version>
    // @category compat
    //
    // Emulate a specific version of Doom.  Valid values are "1.9",
    // "ultimate", "final", "final2", "hacx" and "chex".
    //

    p = M_CheckParmWithArgs(doom, "-gameversion", 1);

    if (p)
    {
        for (i = 0; gameversions[i].description != NULL; ++i)
        {
            if (!d_strcmp(doom->myargv[p + 1], gameversions[i].cmdline))
            {
                doom->gameversion = gameversions[i].version;
                break;
            }
        }

        if (gameversions[i].description == NULL)
        {
            d_printf("Supported game versions:\n");

            for (i = 0; gameversions[i].description != NULL; ++i)
            {
                d_printf("\t%s (%s)\n", gameversions[i].cmdline,
                         gameversions[i].description);
            }

            I_Error("Unknown game version '%s'", doom->myargv[p + 1]);
        }
    }
    else
    {
        // Determine automatically

        if (doom->gamemission == pack_chex)
        {
            // chex.exe - identified by iwad filename

            doom->gameversion = exe_chex;
        }
        else if (doom->gamemission == pack_hacx)
        {
            // hacx.exe: identified by iwad filename

            doom->gameversion = exe_hacx;
        }
        else if (doom->gamemode == shareware || doom->gamemode == registered)
        {
            // original

            doom->gameversion = exe_doom_1_9;

            // TODO: Detect IWADs earlier than Doom v1.9.
        }
        else if (doom->gamemode == retail)
        {
            doom->gameversion = exe_ultimate;
        }
        else if (doom->gamemode == commercial)
        {
            if (doom->gamemission == doom2)
            {
                doom->gameversion = exe_doom_1_9;
            }
            else
            {
                // Final Doom: tnt or plutonia
                // Defaults to emulating the first Final Doom executable,
                // which has the crash in the demo loop; however, having
                // this as the default should mean that it plays back
                // most demos correctly.

                doom->gameversion = exe_final;
            }
        }
    }

    // The original exe does not support retail - 4th episode not supported

    if (doom->gameversion < exe_ultimate && doom->gamemode == retail)
    {
        doom->gamemode = registered;
    }

    // EXEs prior to the Final Doom exes do not support Final Doom.

    if (doom->gameversion < exe_final && doom->gamemode == commercial && (doom->gamemission == pack_tnt || doom->gamemission == pack_plut))
    {
        doom->gamemission = doom2;
    }
}

void PrintGameVersion(doom_data_t *doom)
{
    int i;

    for (i = 0; gameversions[i].description != NULL; ++i)
    {
        if (gameversions[i].version == doom->gameversion)
        {
            d_printf("Emulating the behavior of the "
                     "'%s' executable.\n",
                     gameversions[i].description);
            break;
        }
    }
}

// Function called at exit to display the ENDOOM screen

static void D_Endoom(doom_data_t *doom)
{
    byte *endoom;

    // Don't show ENDOOM if we have it disabled, or we're running
    // in screensaver or control test mode. Only show it once the
    // game has actually started.

    if (!doom->show_endoom || !doom->main_loop_started || screensaver_mode || M_CheckParm(doom, "-testcontrols") > 0)
    {
        return;
    }

    endoom = W_CacheLumpName(doom, DEH_String("ENDOOM"), PU_STATIC);

    // exit(0);
}

static void Init_ScreenBuffer(doom_data_t* doom)
{
    uint32_t width, height;
    doomgeneric_Res(&width, &height);
    doom->DG_ScreenBuffer = Z_Malloc(width * height * 4, PU_STATIC, NULL);
}

//
// D_DoomMain
//
void D_DoomMain(struct doom_data_t_ *doom)
{
    int p;
    char file[256];
    char demolumpname[9];

    I_AtExit(D_Endoom, false);

    // print banner

    I_PrintBanner(PACKAGE_STRING);

    d_printf("Z_Init: Init zone memory allocation daemon. \n");
    Z_Init(doom);
    Init_ScreenBuffer(doom);

#ifdef FEATURE_MULTIPLAYER
    //!
    // @category net
    //
    // Start a dedicated server, routing packets but not participating
    // in the game itself.
    //

    if (M_CheckParm("-dedicated") > 0)
    {
        d_printf("Dedicated server mode.\n");
        NET_DedicatedServer();

        // Never returns
    }

    //!
    // @category net
    //
    // Query the Internet master server for a global list of active
    // servers.
    //

    if (M_CheckParm("-search"))
    {
        NET_MasterQuery();
        exit(0);
    }

    //!
    // @arg <address>
    // @category net
    //
    // Query the status of the server running on the given IP
    // address.
    //

    p = M_CheckParmWithArgs("-query", 1);

    if (p)
    {
        NET_QueryAddress(myargv[p + 1]);
        exit(0);
    }

    //!
    // @category net
    //
    // Search the local LAN for running servers.
    //

    if (M_CheckParm("-localsearch"))
    {
        NET_LANQuery();
        exit(0);
    }

#endif

    //!
    // @vanilla
    //
    // Disable monsters.
    //

    doom->nomonsters = M_CheckParm(doom, "-nomonsters");

    //!
    // @vanilla
    //
    // Monsters respawn after being killed.
    //

    doom->respawnparm = M_CheckParm(doom, "-respawn");

    //!
    // @vanilla
    //
    // Monsters move faster.
    //

    doom->fastparm = M_CheckParm(doom, "-fast");

    //!
    // @vanilla
    //
    // Developer mode.  F1 saves a screenshot in the current working
    // directory.
    //

    doom->devparm = M_CheckParm(doom, "-devparm");

    I_DisplayFPSDots(doom->devparm);

    //!
    // @category net
    // @vanilla
    //
    // Start a deathmatch game.
    //

    if (M_CheckParm(doom, "-deathmatch"))
        doom->deathmatch = 1;

    //!
    // @category net
    // @vanilla
    //
    // Start a deathmatch 2.0 game.  Weapons do not stay in place and
    // all items respawn after 30 seconds.
    //

    if (M_CheckParm(doom, "-altdeath"))
        doom->deathmatch = 2;

    if (doom->devparm)
        d_printf(D_DEVSTR);

    // find which dir to use for config files

    M_SetConfigDir(NULL);

    //!
    // @arg <x>
    // @vanilla
    //
    // Turbo mode.  The player's speed is multiplied by x%.  If unspecified,
    // x defaults to 200.  Values are rounded up to 10 and down to 400.
    //

    if ((p = M_CheckParm(doom, "-turbo")))
    {
        int scale = 200;

        if (p < doom->myargc - 1)
            scale = d_atoi(doom->myargv[p + 1]);
        if (scale < 10)
            scale = 10;
        if (scale > 400)
            scale = 400;
        d_printf("turbo scale: %i%%\n", scale);
        doom->forwardmove[0] = doom->forwardmove[0] * scale / 100;
        doom->forwardmove[1] = doom->forwardmove[1] * scale / 100;
        doom->sidemove[0] = doom->sidemove[0] * scale / 100;
        doom->sidemove[1] = doom->sidemove[1] * scale / 100;
    }

    // init subsystems
    d_printf("V_Init: allocate screens.\n");
    V_Init(doom);

    // Load configuration files before initialising other subsystems.
    d_printf("M_LoadDefaults: Load system defaults.\n");
    M_SetConfigFilenames("default.cfg", PROGRAM_PREFIX "doom.cfg");
    D_BindVariables(doom);
    M_LoadDefaults(doom);

    // Find main IWAD file and load it.
    doom->iwadfile = D_FindIWAD();

    // None found?

    if (doom->iwadfile == NULL)
    {
        I_Error("Game mode indeterminate.  No IWAD file was found.  Try\n"
                "specifying one with the '-iwad' command line parameter.\n");
    }

    doom->modifiedgame = false;

    d_printf("W_Init: Init WADfiles.\n");
    D_AddFile(doom, (char *)doom->iwadfile);

    W_CheckCorrectIWAD(doom, doom1);

    // Now that we've loaded the IWAD, we can figure out what gamemission
    // we're playing and which version of Vanilla Doom we need to emulate.
    D_IdentifyVersion(doom);
    InitGameVersion(doom);

    // Doom 3: BFG Edition includes modified versions of the classic
    // IWADs which can be identified by an additional DMENUPIC lump.
    // Furthermore, the M_GDHIGH lumps have been modified in a way that
    // makes them incompatible to Vanilla Doom and the modified version
    // of doom2.wad is missing the TITLEPIC lump.
    // We specifically check for DMENUPIC here, before PWADs have been
    // loaded which could probably include a lump of that name.

    if (W_CheckNumForName(doom, "dmenupic") >= 0)
    {
        d_printf("BFG Edition: Using workarounds as needed.\n");
        doom->bfgedition = true;

        // BFG Edition changes the names of the secret levels to
        // censor the Wolfenstein references. It also has an extra
        // secret level (MAP33). In Vanilla Doom (meaning the DOS
        // version), MAP33 overflows into the Plutonia level names
        // array, so HUSTR_33 is actually PHUSTR_1.

        DEH_AddStringReplacement(HUSTR_31, "level 31: idkfa");
        DEH_AddStringReplacement(HUSTR_32, "level 32: keen");
        DEH_AddStringReplacement(PHUSTR_1, "level 33: betray");

        // The BFG edition doesn't have the "low detail" menu option (fair
        // enough). But bizarrely, it reuses the M_GDHIGH patch as a label
        // for the options menu (says "Fullscreen:"). Why the perpetrators
        // couldn't just add a new graphic lump and had to reuse this one,
        // I don't know.
        //
        // The end result is that M_GDHIGH is too wide and causes the game
        // to crash. As a workaround to get a minimum level of support for
        // the BFG edition IWADs, use the "ON"/"OFF" graphics instead.

        DEH_AddStringReplacement("M_GDHIGH", "M_MSGON");
        DEH_AddStringReplacement("M_GDLOW", "M_MSGOFF");
    }

    // Load PWAD files.
    doom->modifiedgame = false;

    // Debug:
    //    W_PrintDirectory();

    //!
    // @arg <demo>
    // @category demo
    // @vanilla
    //
    // Play back the demo named demo.lmp.
    //

    p = M_CheckParmWithArgs(doom, "-playdemo", 1);

    if (!p)
    {
        //!
        // @arg <demo>
        // @category demo
        // @vanilla
        //
        // Play back the demo named demo.lmp, determining the framerate
        // of the screen.
        //
        p = M_CheckParmWithArgs(doom, "-timedemo", 1);
    }

    if (p)
    {
        // With Vanilla you have to specify the file without extension,
        // but make that optional.
        if (M_StringEndsWith(doom->myargv[p + 1], ".lmp"))
        {
            M_StringCopy(file, doom->myargv[p + 1], sizeof(file));
        }
        else
        {
            d_snprintf(file, sizeof(file), "%s.lmp", doom->myargv[p + 1]);
        }

        if (D_AddFile(doom, file))
        {
            M_StringCopy(demolumpname, doom->lumpinfo[doom->numlumps - 1].name,
                         sizeof(demolumpname));
        }
        else
        {
            // If file failed to load, still continue trying to play
            // the demo in the same way as Vanilla Doom.  This makes
            // tricks like "-playdemo demo1" possible.

            M_StringCopy(demolumpname, doom->myargv[p + 1], sizeof(demolumpname));
        }

        d_printf("Playing demo %s.\n", file);
    }

    I_AtExit((atexit_func_t)G_CheckDemoStatus, true);

    // Generate the WAD hash table.  Speed things up a bit.
    W_GenerateHashTable(doom);

    // Set the gamedescription string. This is only possible now that
    // we've finished loading Dehacked patches.
    D_SetGameDescription(doom);

    doom->savegamedir = M_GetSaveGameDir(D_SaveGameIWADName(doom->gamemission));

    // Check for -file in shareware
    if (doom->modifiedgame)
    {
        // These are the lumps that will be checked in IWAD,
        // if any one is not present, execution will be aborted.
        static char name[23][8] =
            {
                "e2m1", "e2m2", "e2m3", "e2m4", "e2m5", "e2m6", "e2m7", "e2m8", "e2m9",
                "e3m1", "e3m3", "e3m3", "e3m4", "e3m5", "e3m6", "e3m7", "e3m8", "e3m9",
                "dphoof", "bfgga0", "heada1", "cybra1", "spida1d1"};
        int i;

        if (doom->gamemode == shareware)
            I_Error(DEH_String("\nYou cannot -file with the shareware "
                               "version. Register!"));

        // Check for fake IWAD with right name,
        // but w/o all the lumps of the registered version.
        if (doom->gamemode == registered)
            for (i = 0; i < 23; i++)
                if (W_CheckNumForName(doom, name[i]) < 0)
                    I_Error(DEH_String("\nThis is not the registered version."));
    }

    if (W_CheckNumForName(doom, "SS_START") >= 0 || W_CheckNumForName(doom, "FF_END") >= 0)
    {
        I_PrintDivider();
        d_printf(" WARNING: The loaded WAD file contains modified sprites or\n"
                 " floor textures.  You may want to use the '-merge' command\n"
                 " line option instead of '-file'.\n");
    }

    I_PrintStartupBanner(doom->gamedescription);
    PrintDehackedBanners();

    // Freedoom's IWADs are Boom-compatible, which means they usually
    // don't work in Vanilla (though FreeDM is okay). Show a warning
    // message and give a link to the website.
    if (W_CheckNumForName(doom, "FREEDOOM") >= 0 && W_CheckNumForName(doom, "FREEDM") < 0)
    {
        d_printf(" WARNING: You are playing using one of the Freedoom IWAD\n"
                 " files, which might not work in this port. See this page\n"
                 " for more information on how to play using Freedoom:\n"
                 "   http://www.chocolate-doom.org/wiki/index.php/Freedoom\n");
        I_PrintDivider();
    }

    d_printf("I_Init: Setting up machine state.\n");
    I_CheckIsScreensaver();
    I_InitSound(doom, true);
    I_InitMusic();
    // Initial netgame startup. Connect to server etc.
    D_ConnectNetGame(doom);

    // get skill / episode / map from parms
    doom->startskill = sk_medium;
    doom->startepisode = 1;
    doom->startmap = 1;
    doom->autostart = false;

    //!
    // @arg <skill>
    // @vanilla
    //
    // Set the game skill, 1-5 (1: easiest, 5: hardest).  A skill of
    // 0 disables all monsters.
    //

    p = M_CheckParmWithArgs(doom, "-skill", 1);

    if (p)
    {
        doom->startskill = doom->myargv[p + 1][0] - '1';
        doom->autostart = true;
    }

    //!
    // @arg <n>
    // @vanilla
    //
    // Start playing on episode n (1-4)
    //

    p = M_CheckParmWithArgs(doom, "-episode", 1);

    if (p)
    {
        doom->startepisode = doom->myargv[p + 1][0] - '0';
        doom->startmap = 1;
        doom->autostart = true;
    }

    doom->timelimit = 0;

    //!
    // @arg <n>
    // @category net
    // @vanilla
    //
    // For multiplayer games: exit each level after n minutes.
    //

    p = M_CheckParmWithArgs(doom, "-timer", 1);

    if (p)
    {
        doom->timelimit = d_atoi(doom->myargv[p + 1]);
    }

    //!
    // @category net
    // @vanilla
    //
    // Austin Virtual Gaming: end levels after 20 minutes.
    //

    p = M_CheckParm(doom, "-avg");

    if (p)
    {
        doom->timelimit = 20;
    }

    //!
    // @arg [<x> <y> | <xy>]
    // @vanilla
    //
    // Start a game immediately, warping to ExMy (Doom 1) or MAPxy
    // (Doom 2)
    //

    p = M_CheckParmWithArgs(doom, "-warp", 1);

    if (p)
    {
        if (doom->gamemode == commercial)
            doom->startmap = d_atoi(doom->myargv[p + 1]);
        else
        {
            doom->startepisode = doom->myargv[p + 1][0] - '0';

            if (p + 2 < doom->myargc)
            {
                doom->startmap = doom->myargv[p + 2][0] - '0';
            }
            else
            {
                doom->startmap = 1;
            }
        }
        doom->autostart = true;
    }

    // Undocumented:
    // Invoked by setup to test the controls.

    p = M_CheckParm(doom, "-testcontrols");

    if (p > 0)
    {
        doom->startepisode = 1;
        doom->startmap = 1;
        doom->autostart = true;
        doom->testcontrols = true;
    }

    // Check for load game parameter
    // We do this here and save the slot number, so that the network code
    // can override it or send the load slot to other players.

    //!
    // @arg <s>
    // @vanilla
    //
    // Load the game in slot s.
    //

    p = M_CheckParmWithArgs(doom, "-loadgame", 1);

    if (p)
    {
        doom->startloadgame = d_atoi(doom->myargv[p + 1]);
    }
    else
    {
        // Not loading a game
        doom->startloadgame = -1;
    }

    d_printf("M_Init: Init miscellaneous info.\n");
    M_Init(doom);

    d_printf("R_Init: Init DOOM refresh daemon - ");
    R_Init(doom);

    d_printf("\nP_Init: Init Playloop state.\n");
    P_Init(doom);

    d_printf("S_Init: Setting up sound.\n");
    S_Init(sfxVolume * 8, musicVolume * 8);

    d_printf("D_CheckNetGame: Checking network game status.\n");
    D_CheckNetGame(doom);

    PrintGameVersion(doom);

    d_printf("HU_Init: Setting up heads up display.\n");
    HU_Init(doom);

    d_printf("ST_Init: Init status bar.\n");
    ST_Init(doom);

    // If Doom II without a MAP01 lump, this is a store demo.
    // Moved this here so that MAP01 isn't constantly looked up
    // in the main loop.

    if (doom->gamemode == commercial && W_CheckNumForName(doom, "map01") < 0)
        doom->storedemo = true;

    //!
    // @arg <x>
    // @category demo
    // @vanilla
    //
    // Record a demo named x.lmp.
    //

    p = M_CheckParmWithArgs(doom, "-record", 1);

    if (p)
    {
        G_RecordDemo(doom, doom->myargv[p + 1]);
        doom->autostart = true;
    }

    p = M_CheckParmWithArgs(doom, "-playdemo", 1);
    if (p)
    {
        doom->singledemo = true; // quit after one demo
        G_DeferedPlayDemo(doom, demolumpname);
        D_DoomLoop(doom);
        return;
    }

    p = M_CheckParmWithArgs(doom, "-timedemo", 1);
    if (p)
    {
        G_TimeDemo(doom, demolumpname);
        D_DoomLoop(doom);
        return;
    }

    if (doom->startloadgame >= 0)
    {
        M_StringCopy(file, P_SaveGameFile(doom, doom->startloadgame), sizeof(file));
        G_LoadGame(doom, file);
    }

    if (doom->gameaction != ga_loadgame)
    {
        if (doom->autostart || doom->netgame)
            G_InitNew(doom, doom->startskill, doom->startepisode, doom->startmap);
        else
            D_StartTitle(doom); // start up intro loop
    }

    D_DoomLoop(doom);
}
