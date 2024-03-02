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
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//

#include "dlibc.h"

#include "doomdef.h"
#include "doomkeys.h"
#include "dstrings.h"

#include "d_main.h"
#include "deh_main.h"

#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "r_local.h"

#include "hu_stuff.h"

#include "g_game.h"

#include "m_argv.h"
#include "m_controls.h"
#include "p_saveg.h"

#include "s_sound.h"

#include "doomstat.h"

// Data.
#include "sounds.h"

#include "m_menu.h"

extern patch_t *hu_font[HU_FONTSIZE];
extern boolean message_dontfuckwithme;

extern boolean chat_on; // in heads-up code

//
// defaulted values
//
int mouseSensitivity = 5;

// Show messages has default, 0 = off, 1 = on
int showMessages = 1;

// Blocky mode, has default, 0 = high, 1 = normal
int detailLevel = 0;
int screenblocks = 10;

// temp for screenblocks (0-9)
int screenSize;

// -1 = no quicksave slot picked!
int quickSaveSlot;

// 1 = message to be printed
int messageToPrint;
// ...and here is the message string!
char *messageString;

// message x & y
int messx;
int messy;
int messageLastMenuActive;

// timed message = no input from user
boolean messageNeedsInput;

void (*messageRoutine)(doom_data_t *doom, int response);

char gammamsg[5][26] =
    {
        GAMMALVL0,
        GAMMALVL1,
        GAMMALVL2,
        GAMMALVL3,
        GAMMALVL4};

// we are going to be entering a savegame string
int saveStringEnter;
int saveSlot;      // which slot to save in
int saveCharIndex; // which char we're editing
// old save description before edit
char saveOldString[SAVESTRINGSIZE];

boolean inhelpscreens;
boolean menuactive;

#define SKULLXOFF -32
#define LINEHEIGHT 16

extern boolean sendpause;
char savegamestrings[10][SAVESTRINGSIZE];

char endstring[160];

// static boolean opldev;

//
// MENU TYPEDEFS
//
typedef struct
{
    // 0 = no cursor here, 1 = ok, 2 = arrows ok
    short status;

    char name[10];

    // choice = menu item #.
    // if status = 2,
    //   choice=0:leftarrow,1:rightarrow
    void (*routine)(doom_data_t *doom, int choice);

    // hotkey in menu
    char alphaKey;
} menuitem_t;

typedef struct menu_s
{
    short numitems;          // # of menu items
    struct menu_s *prevMenu; // previous menu
    menuitem_t *menuitems;   // menu items
    void (*routine)(struct doom_data_t_* doom);       // draw routine
    short x;
    short y;      // x,y of menu
    short lastOn; // last item user was on in menu
} menu_t;

short itemOn;           // menu item skull is on
short skullAnimCounter; // skull animation counter
short whichSkull;       // which skull to draw

// graphic name of skulls
// warning: initializer-string for array of chars is too long
char *skullName[2] = {"M_SKULL1", "M_SKULL2"};

// current menudef
menu_t *currentMenu;

//
// PROTOTYPES
//
void M_NewGame(doom_data_t *doom, int choice);
void M_Episode(doom_data_t *doom, int choice);
void M_ChooseSkill(doom_data_t *doom, int choice);
void M_LoadGame(doom_data_t *doom, int choice);
void M_SaveGame(doom_data_t *doom, int choice);
void M_Options(doom_data_t *doom, int choice);
void M_EndGame(doom_data_t *doom, int choice);
void M_ReadThis(doom_data_t *doom, int choice);
void M_ReadThis2(doom_data_t *doom, int choice);
void M_QuitDOOM(doom_data_t *doom, int choice);

void M_ChangeMessages(doom_data_t *doom, int choice);
void M_ChangeSensitivity(doom_data_t *doom, int choice);
void M_SfxVol(doom_data_t *doom, int choice);
void M_MusicVol(doom_data_t *doom, int choice);
void M_ChangeDetail(doom_data_t *doom, int choice);
void M_SizeDisplay(doom_data_t *doom, int choice);
void M_StartGame(doom_data_t *doom, int choice);
void M_Sound(doom_data_t *doom, int choice);

void M_FinishReadThis(doom_data_t *doom, int choice);
void M_LoadSelect(doom_data_t *doom, int choice);
void M_SaveSelect(doom_data_t *doom, int choice);
void M_ReadSaveStrings(doom_data_t *doom);
void M_QuickSave(doom_data_t *doom);
void M_QuickLoad(doom_data_t *doom);

void M_DrawMainMenu(struct doom_data_t_* doom);
void M_DrawReadThis1(struct doom_data_t_* doom);
void M_DrawReadThis2(struct doom_data_t_* doom);
void M_DrawNewGame(struct doom_data_t_* doom);
void M_DrawEpisode(struct doom_data_t_* doom);
void M_DrawOptions(struct doom_data_t_* doom);
void M_DrawSound(struct doom_data_t_* doom);
void M_DrawLoad(struct doom_data_t_* doom);
void M_DrawSave(struct doom_data_t_* doom);

void M_DrawSaveLoadBorder(struct doom_data_t_* doom, int x, int y);
void M_SetupNextMenu(menu_t *menudef);
void M_DrawThermo(struct doom_data_t_* doom, int x, int y, int thermWidth, int thermDot);
void M_DrawEmptyCell(struct doom_data_t_* doom, menu_t *menu, int item);
void M_DrawSelCell(struct doom_data_t_* doom, menu_t *menu, int item);
void M_WriteText(struct doom_data_t_* doom, int x, int y, char *string);
int M_StringWidth(char *string);
int M_StringHeight(char *string);
void M_StartMessage(char *string, void *routine, boolean input);
void M_StopMessage(void);
void M_ClearMenus(void);

//
// DOOM MENU
//
enum
{
    newgame = 0,
    options,
    loadgame,
    savegame,
    readthis,
    quitdoom,
    main_end
} main_e;

menuitem_t MainMenu[] =
    {
        {1, "M_NGAME", M_NewGame, 'n'},
        {1, "M_OPTION", M_Options, 'o'},
        {1, "M_LOADG", M_LoadGame, 'l'},
        {1, "M_SAVEG", M_SaveGame, 's'},
        // Another hickup with Special edition.
        {1, "M_RDTHIS", M_ReadThis, 'r'},
        {1, "M_QUITG", M_QuitDOOM, 'q'}};

menu_t MainDef =
    {
        main_end,
        NULL,
        MainMenu,
        M_DrawMainMenu,
        97, 64,
        0};

//
// EPISODE SELECT
//
enum
{
    ep1,
    ep2,
    ep3,
    ep4,
    ep_end
} episodes_e;

menuitem_t EpisodeMenu[] =
    {
        {1, "M_EPI1", M_Episode, 'k'},
        {1, "M_EPI2", M_Episode, 't'},
        {1, "M_EPI3", M_Episode, 'i'},
        {1, "M_EPI4", M_Episode, 't'}};

menu_t EpiDef =
    {
        ep_end,        // # of menu items
        &MainDef,      // previous menu
        EpisodeMenu,   // menuitem_t ->
        M_DrawEpisode, // drawing routine ->
        48, 63,        // x,y
        ep1            // lastOn
};

//
// NEW GAME
//
enum
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
} newgame_e;

menuitem_t NewGameMenu[] =
    {
        {1, "M_JKILL", M_ChooseSkill, 'i'},
        {1, "M_ROUGH", M_ChooseSkill, 'h'},
        {1, "M_HURT", M_ChooseSkill, 'h'},
        {1, "M_ULTRA", M_ChooseSkill, 'u'},
        {1, "M_NMARE", M_ChooseSkill, 'n'}};

menu_t NewDef =
    {
        newg_end,      // # of menu items
        &EpiDef,       // previous menu
        NewGameMenu,   // menuitem_t ->
        M_DrawNewGame, // drawing routine ->
        48, 63,        // x,y
        hurtme         // lastOn
};

//
// OPTIONS MENU
//
enum
{
    endgame,
    messages,
    detail,
    scrnsize,
    option_empty1,
    mousesens,
    option_empty2,
    soundvol,
    opt_end
} options_e;

menuitem_t OptionsMenu[] =
    {
        {1, "M_ENDGAM", M_EndGame, 'e'},
        {1, "M_MESSG", M_ChangeMessages, 'm'},
        {1, "M_DETAIL", M_ChangeDetail, 'g'},
        {2, "M_SCRNSZ", M_SizeDisplay, 's'},
        {-1, "", 0, '\0'},
        {2, "M_MSENS", M_ChangeSensitivity, 'm'},
        {-1, "", 0, '\0'},
        {1, "M_SVOL", M_Sound, 's'}};

menu_t OptionsDef =
    {
        opt_end,
        &MainDef,
        OptionsMenu,
        M_DrawOptions,
        60, 37,
        0};

//
// Read This! MENU 1 & 2
//
enum
{
    rdthsempty1,
    read1_end
} read_e;

menuitem_t ReadMenu1[] =
    {
        {1, "", M_ReadThis2, 0}};

menu_t ReadDef1 =
    {
        read1_end,
        &MainDef,
        ReadMenu1,
        M_DrawReadThis1,
        280, 185,
        0};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

menuitem_t ReadMenu2[] =
    {
        {1, "", M_FinishReadThis, 0}};

menu_t ReadDef2 =
    {
        read2_end,
        &ReadDef1,
        ReadMenu2,
        M_DrawReadThis2,
        330, 175,
        0};

//
// SOUND VOLUME MENU
//
enum
{
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    sound_end
} sound_e;

menuitem_t SoundMenu[] =
    {
        {2, "M_SFXVOL", M_SfxVol, 's'},
        {-1, "", 0, '\0'},
        {2, "M_MUSVOL", M_MusicVol, 'm'},
        {-1, "", 0, '\0'}};

menu_t SoundDef =
    {
        sound_end,
        &OptionsDef,
        SoundMenu,
        M_DrawSound,
        80, 64,
        0};

//
// LOAD GAME MENU
//
enum
{
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load_end
} load_e;

menuitem_t LoadMenu[] =
    {
        {1, "", M_LoadSelect, '1'},
        {1, "", M_LoadSelect, '2'},
        {1, "", M_LoadSelect, '3'},
        {1, "", M_LoadSelect, '4'},
        {1, "", M_LoadSelect, '5'},
        {1, "", M_LoadSelect, '6'}};

menu_t LoadDef =
    {
        load_end,
        &MainDef,
        LoadMenu,
        M_DrawLoad,
        80, 54,
        0};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[] =
    {
        {1, "", M_SaveSelect, '1'},
        {1, "", M_SaveSelect, '2'},
        {1, "", M_SaveSelect, '3'},
        {1, "", M_SaveSelect, '4'},
        {1, "", M_SaveSelect, '5'},
        {1, "", M_SaveSelect, '6'}};

menu_t SaveDef =
    {
        load_end,
        &MainDef,
        SaveMenu,
        M_DrawSave,
        80, 54,
        0};

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings(doom_data_t *doom)
{
    FILE *handle;
    int i;
    char name[256];

    for (i = 0; i < load_end; i++)
    {
        M_StringCopy(name, P_SaveGameFile(doom, i), sizeof(name));

        handle = d_fopen(name, "rb");
        if (handle == NULL)
        {
            M_StringCopy(savegamestrings[i], EMPTYSTRING, SAVESTRINGSIZE);
            LoadMenu[i].status = 0;
            continue;
        }
        d_fread(&savegamestrings[i], 1, SAVESTRINGSIZE, handle);
        d_fclose(handle);
        LoadMenu[i].status = 1;
    }
}

//
// M_LoadGame & Cie.
//
void M_DrawLoad(struct doom_data_t_* doom)
{
    int i;

    V_DrawPatchDirect(doom, 72, 28,
                      W_CacheLumpName(doom, DEH_String("M_LOADG"), PU_CACHE));

    for (i = 0; i < load_end; i++)
    {
        M_DrawSaveLoadBorder(doom, LoadDef.x, LoadDef.y + LINEHEIGHT * i);
        M_WriteText(doom, LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
    }
}

//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(struct doom_data_t_* doom, int x, int y)
{
    int i;

    V_DrawPatchDirect(doom, x - 8, y + 7,
                      W_CacheLumpName(doom, DEH_String("M_LSLEFT"), PU_CACHE));

    for (i = 0; i < 24; i++)
    {
        V_DrawPatchDirect(doom, x, y + 7,
                          W_CacheLumpName(doom, DEH_String("M_LSCNTR"), PU_CACHE));
        x += 8;
    }

    V_DrawPatchDirect(doom, x, y + 7,
                      W_CacheLumpName(doom, DEH_String("M_LSRGHT"), PU_CACHE));
}

//
// User wants to load this game
//
void M_LoadSelect(doom_data_t *doom, int choice)
{
    char name[256];

    M_StringCopy(name, P_SaveGameFile(doom, choice), sizeof(name));

    G_LoadGame(doom, name);
    M_ClearMenus();
}

//
// Selected from DOOM menu
//
void M_LoadGame(doom_data_t *doom, int choice)
{
    if (doom->netgame)
    {
        M_StartMessage(DEH_String(LOADNET), NULL, false);
        return;
    }

    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings(doom);
}

//
//  M_SaveGame & Cie.
//
void M_DrawSave(struct doom_data_t_* doom)
{
    int i;

    V_DrawPatchDirect(doom, 72, 28, W_CacheLumpName(doom, DEH_String("M_SAVEG"), PU_CACHE));
    for (i = 0; i < load_end; i++)
    {
        M_DrawSaveLoadBorder(doom, LoadDef.x, LoadDef.y + LINEHEIGHT * i);
        M_WriteText(doom, LoadDef.x, LoadDef.y + LINEHEIGHT * i, savegamestrings[i]);
    }

    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[saveSlot]);
        M_WriteText(doom, LoadDef.x + i, LoadDef.y + LINEHEIGHT * saveSlot, "_");
    }
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(struct doom_data_t_* doom, int slot)
{
    G_SaveGame(doom, slot, savegamestrings[slot]);
    M_ClearMenus();

    // PICK QUICKSAVE SLOT YET?
    if (quickSaveSlot == -2)
        quickSaveSlot = slot;
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(doom_data_t *doom, int choice)
{
    // we are going to be intercepting all chars
    saveStringEnter = 1;

    saveSlot = choice;
    M_StringCopy(saveOldString, savegamestrings[choice], SAVESTRINGSIZE);
    if (!d_strcmp(savegamestrings[choice], EMPTYSTRING))
        savegamestrings[choice][0] = 0;
    saveCharIndex = d_strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
void M_SaveGame(doom_data_t *doom, int choice)
{
    if (!doom->usergame)
    {
        M_StartMessage(DEH_String(SAVEDEAD), NULL, false);
        return;
    }

    if (doom->gamestate != GS_LEVEL)
        return;

    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings(doom);
}

//
//      M_QuickSave
//
char tempstring[80];

void M_QuickSaveResponse(struct doom_data_t_* doom, int key)
{
    if (key == key_menu_confirm)
    {
        M_DoSave(doom, quickSaveSlot);
        S_StartSound(doom, NULL, sfx_swtchx);
    }
}

void M_QuickSave(doom_data_t *doom)
{
    if (!doom->usergame)
    {
        S_StartSound(doom, NULL, sfx_oof);
        return;
    }

    if (doom->gamestate != GS_LEVEL)
        return;

    if (quickSaveSlot < 0)
    {
        M_StartControlPanel();
        M_ReadSaveStrings(doom);
        M_SetupNextMenu(&SaveDef);
        quickSaveSlot = -2; // means to pick a slot now
        return;
    }
    d_snprintf(tempstring, 80, QSPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickSaveResponse, true);
}

//
// M_QuickLoad
//
void M_QuickLoadResponse(doom_data_t *doom, int key)
{
    if (key == key_menu_confirm)
    {
        M_LoadSelect(doom, quickSaveSlot);
        S_StartSound(doom, NULL, sfx_swtchx);
    }
}

void M_QuickLoad(doom_data_t *doom)
{
    if (doom->netgame)
    {
        M_StartMessage(DEH_String(QLOADNET), NULL, false);
        return;
    }

    if (quickSaveSlot < 0)
    {
        M_StartMessage(DEH_String(QSAVESPOT), NULL, false);
        return;
    }
    d_snprintf(tempstring, 80, QLPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickLoadResponse, true);
}

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(doom_data_t *doom)
{
    char *lumpname = "CREDIT";
    int skullx = 330, skully = 175;

    inhelpscreens = true;

    // Different versions of Doom 1.9 work differently

    switch (doom->gameversion)
    {
    case exe_doom_1_666:
    case exe_doom_1_7:
    case exe_doom_1_8:
    case exe_doom_1_9:
    case exe_hacx:

        if (doom->gamemode == commercial)
        {
            // Doom 2

            lumpname = "HELP";

            skullx = 330;
            skully = 165;
        }
        else
        {
            // Doom 1
            // HELP2 is the first screen shown in Doom 1

            lumpname = "HELP2";

            skullx = 280;
            skully = 185;
        }
        break;

    case exe_ultimate:
    case exe_chex:

        // Ultimate Doom always displays "HELP1".

        // Chex Quest version also uses "HELP1", even though it is based
        // on Final Doom.

        lumpname = "HELP1";

        break;

    case exe_final:
    case exe_final2:

        // Final Doom always displays "HELP".

        lumpname = "HELP";

        break;

    default:
        I_Error("Unhandled game version");
        break;
    }

    lumpname = DEH_String(lumpname);

    V_DrawPatchDirect(doom, 0, 0, W_CacheLumpName(doom, lumpname, PU_CACHE));

    ReadDef1.x = skullx;
    ReadDef1.y = skully;
}

//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(struct doom_data_t_* doom)
{
    inhelpscreens = true;

    // We only ever draw the second page if this is
    // gameversion == exe_doom_1_9 and gamemode == registered

    V_DrawPatchDirect(doom, 0, 0, W_CacheLumpName(doom, DEH_String("HELP1"), PU_CACHE));
}

//
// Change Sfx & Music volumes
//
void M_DrawSound(struct doom_data_t_* doom)
{
    V_DrawPatchDirect(doom, 60, 38, W_CacheLumpName(doom, DEH_String("M_SVOL"), PU_CACHE));

    M_DrawThermo(doom, SoundDef.x, SoundDef.y + LINEHEIGHT * (sfx_vol + 1),
                 16, sfxVolume);

    M_DrawThermo(doom, SoundDef.x, SoundDef.y + LINEHEIGHT * (music_vol + 1),
                 16, musicVolume);
}

void M_Sound(doom_data_t *doom, int choice)
{
    M_SetupNextMenu(&SoundDef);
}

void M_SfxVol(doom_data_t *doom, int choice)
{
    switch (choice)
    {
    case 0:
        if (sfxVolume)
            sfxVolume--;
        break;
    case 1:
        if (sfxVolume < 15)
            sfxVolume++;
        break;
    }

    S_SetSfxVolume(sfxVolume * 8);
}

void M_MusicVol(doom_data_t *doom, int choice)
{
    switch (choice)
    {
    case 0:
        if (musicVolume)
            musicVolume--;
        break;
    case 1:
        if (musicVolume < 15)
            musicVolume++;
        break;
    }

    S_SetMusicVolume(musicVolume * 8);
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu(struct doom_data_t_* doom)
{
    V_DrawPatchDirect(doom, 94, 2,
                      W_CacheLumpName(doom, DEH_String("M_DOOM"), PU_CACHE));
}

//
// M_NewGame
//
void M_DrawNewGame(struct doom_data_t_* doom)
{
    V_DrawPatchDirect(doom, 96, 14, W_CacheLumpName(doom, DEH_String("M_NEWG"), PU_CACHE));
    V_DrawPatchDirect(doom, 54, 38, W_CacheLumpName(doom, DEH_String("M_SKILL"), PU_CACHE));
}

void M_NewGame(doom_data_t *doom, int choice)
{
    if (doom->netgame && !doom->demoplayback)
    {
        M_StartMessage(DEH_String(NEWGAME), NULL, false);
        return;
    }

    // Chex Quest disabled the episode select screen, as did Doom II.

    if (doom->gamemode == commercial || doom->gameversion == exe_chex)
        M_SetupNextMenu(&NewDef);
    else
        M_SetupNextMenu(&EpiDef);
}

//
//      M_Episode
//
int epi;

void M_DrawEpisode(struct doom_data_t_* doom)
{
    V_DrawPatchDirect(doom, 54, 38, W_CacheLumpName(doom, DEH_String("M_EPISOD"), PU_CACHE));
}

void M_VerifyNightmare(doom_data_t* doom, int key)
{
    if (key != key_menu_confirm)
        return;

    G_DeferedInitNew(doom, nightmare, epi + 1, 1);
    M_ClearMenus();
}

void M_ChooseSkill(doom_data_t *doom, int choice)
{
    if (choice == nightmare)
    {
        M_StartMessage(DEH_String(NIGHTMARE), M_VerifyNightmare, true);
        return;
    }

    G_DeferedInitNew(doom, choice, epi + 1, 1);
    M_ClearMenus();
}

void M_Episode(doom_data_t *doom, int choice)
{
    if ((doom->gamemode == shareware) && choice)
    {
        M_StartMessage(DEH_String(SWSTRING), NULL, false);
        M_SetupNextMenu(&ReadDef1);
        return;
    }

    // Yet another hack...
    if ((doom->gamemode == registered) && (choice > 2))
    {
        d_printf(
            "M_Episode: 4th episode requires UltimateDOOM\n");
        choice = 0;
    }

    epi = choice;
    M_SetupNextMenu(&NewDef);
}

//
// M_Options
//
static char *detailNames[2] = {"M_GDHIGH", "M_GDLOW"};
static char *msgNames[2] = {"M_MSGOFF", "M_MSGON"};

void M_DrawOptions(struct doom_data_t_* doom)
{
    V_DrawPatchDirect(doom, 108, 15, W_CacheLumpName(doom, DEH_String("M_OPTTTL"), PU_CACHE));

    V_DrawPatchDirect(doom, OptionsDef.x + 175, OptionsDef.y + LINEHEIGHT * detail,
                      W_CacheLumpName(doom, DEH_String(detailNames[detailLevel]),
                                      PU_CACHE));

    V_DrawPatchDirect(doom, OptionsDef.x + 120, OptionsDef.y + LINEHEIGHT * messages,
                      W_CacheLumpName(doom, DEH_String(msgNames[showMessages]),
                                      PU_CACHE));

    M_DrawThermo(doom, OptionsDef.x, OptionsDef.y + LINEHEIGHT * (mousesens + 1),
                 10, mouseSensitivity);

    M_DrawThermo(doom, OptionsDef.x, OptionsDef.y + LINEHEIGHT * (scrnsize + 1),
                 9, screenSize);
}

void M_Options(doom_data_t *doom, int choice)
{
    M_SetupNextMenu(&OptionsDef);
}

//
//      Toggle messages on/off
//
void M_ChangeMessages(doom_data_t *doom, int choice)
{
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;

    if (!showMessages)
        doom->players[doom->consoleplayer].message = DEH_String(MSGOFF);
    else
        doom->players[doom->consoleplayer].message = DEH_String(MSGON);

    message_dontfuckwithme = true;
}

//
// M_EndGame
//
void M_EndGameResponse(doom_data_t *doom, int key)
{
    if (key != key_menu_confirm)
        return;

    currentMenu->lastOn = itemOn;
    M_ClearMenus();
    D_StartTitle(doom);
}

void M_EndGame(doom_data_t *doom, int choice)
{
    choice = 0;
    if (!doom->usergame)
    {
        S_StartSound(doom, NULL, sfx_oof);
        return;
    }

    if (doom->netgame)
    {
        M_StartMessage(DEH_String(NETEND), NULL, false);
        return;
    }

    M_StartMessage(DEH_String(ENDGAME), M_EndGameResponse, true);
}

//
// M_ReadThis
//
void M_ReadThis(doom_data_t *doom, int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(doom_data_t *doom, int choice)
{
    // Doom 1.9 had two menus when playing Doom 1
    // All others had only one

    if (doom->gameversion <= exe_doom_1_9 && doom->gamemode != commercial)
    {
        choice = 0;
        M_SetupNextMenu(&ReadDef2);
    }
    else
    {
        // Close the menu

        M_FinishReadThis(doom, 0);
    }
}

void M_FinishReadThis(doom_data_t *doom, int choice)
{
    choice = 0;
    M_SetupNextMenu(&MainDef);
}

//
// M_QuitDOOM
//
int quitsounds[8] =
    {
        sfx_pldeth,
        sfx_dmpain,
        sfx_popain,
        sfx_slop,
        sfx_telept,
        sfx_posit1,
        sfx_posit3,
        sfx_sgtatk};

int quitsounds2[8] =
    {
        sfx_vilact,
        sfx_getpow,
        sfx_boscub,
        sfx_slop,
        sfx_skeswg,
        sfx_kntdth,
        sfx_bspact,
        sfx_sgtatk};

void M_QuitResponse(doom_data_t *doom, int key)
{
    if (key != key_menu_confirm)
        return;
    if (!doom->netgame)
    {
        if (doom->gamemode == commercial)
            S_StartSound(doom, NULL, quitsounds2[(doom->gametic >> 2) & 7]);
        else
            S_StartSound(doom, NULL, quitsounds[(doom->gametic >> 2) & 7]);
    }
    I_Quit(doom);
}

static char *M_SelectEndMessage(doom_data_t *doom)
{
    char **endmsg;

    if (logical_gamemission == doom1)
    {
        // Doom 1

        endmsg = doom1_endmsg;
    }
    else
    {
        // Doom 2

        endmsg = doom2_endmsg;
    }

    return endmsg[doom->gametic % NUM_QUITMESSAGES];
}

void M_QuitDOOM(doom_data_t *doom, int choice)
{
    d_snprintf(endstring, sizeof(endstring), "%s\n\n" DOSY,
               DEH_String(M_SelectEndMessage(doom)));

    M_StartMessage(endstring, M_QuitResponse, true);
}

void M_ChangeSensitivity(doom_data_t *doom, int choice)
{
    switch (choice)
    {
    case 0:
        if (mouseSensitivity)
            mouseSensitivity--;
        break;
    case 1:
        if (mouseSensitivity < 9)
            mouseSensitivity++;
        break;
    }
}

void M_ChangeDetail(doom_data_t *doom, int choice)
{
    choice = 0;
    detailLevel = 1 - detailLevel;

    R_SetViewSize(screenblocks, detailLevel);

    if (!detailLevel)
        doom->players[doom->consoleplayer].message = DEH_String(DETAILHI);
    else
        doom->players[doom->consoleplayer].message = DEH_String(DETAILLO);
}

void M_SizeDisplay(doom_data_t *doom, int choice)
{
    switch (choice)
    {
    case 0:
        if (screenSize > 0)
        {
            screenblocks--;
            screenSize--;
        }
        break;
    case 1:
        if (screenSize < 8)
        {
            screenblocks++;
            screenSize++;
        }
        break;
    }

    R_SetViewSize(screenblocks, detailLevel);
}

//
//      Menu Functions
//
void M_DrawThermo(struct doom_data_t_ *doom,
                  int x,
                  int y,
                  int thermWidth,
                  int thermDot)
{
    int xx;
    int i;

    xx = x;
    V_DrawPatchDirect(doom, xx, y, W_CacheLumpName(doom, DEH_String("M_THERML"), PU_CACHE));
    xx += 8;
    for (i = 0; i < thermWidth; i++)
    {
        V_DrawPatchDirect(doom, xx, y, W_CacheLumpName(doom, DEH_String("M_THERMM"), PU_CACHE));
        xx += 8;
    }
    V_DrawPatchDirect(doom, xx, y, W_CacheLumpName(doom, DEH_String("M_THERMR"), PU_CACHE));

    V_DrawPatchDirect(doom, (x + 8) + thermDot * 8, y,
                      W_CacheLumpName(doom, DEH_String("M_THERMO"), PU_CACHE));
}

void M_DrawEmptyCell(struct doom_data_t_ *doom, menu_t *menu,
                     int item)
{
    V_DrawPatchDirect(doom, menu->x - 10, menu->y + item * LINEHEIGHT - 1,
                      W_CacheLumpName(doom, DEH_String("M_CELL1"), PU_CACHE));
}

void M_DrawSelCell(struct doom_data_t_ *doom, menu_t *menu,
                   int item)
{
    V_DrawPatchDirect(doom, menu->x - 10, menu->y + item * LINEHEIGHT - 1,
                      W_CacheLumpName(doom, DEH_String("M_CELL2"), PU_CACHE));
}

void M_StartMessage(char *string,
                    void *routine,
                    boolean input)
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = string;
    messageRoutine = routine;
    messageNeedsInput = input;
    menuactive = true;
    return;
}

void M_StopMessage(void)
{
    menuactive = messageLastMenuActive;
    messageToPrint = 0;
}

//
// Find string width from hu_font chars
//
int M_StringWidth(char *string)
{
    size_t i;
    int w = 0;
    int c;

    for (i = 0; i < d_strlen(string); i++)
    {
        c = d_toupper(string[i]) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
            w += 4;
        else
            w += SHORT(hu_font[c]->width);
    }

    return w;
}

//
//      Find string height from hu_font chars
//
int M_StringHeight(char *string)
{
    size_t i;
    int h;
    int height = SHORT(hu_font[0]->height);

    h = height;
    for (i = 0; i < d_strlen(string); i++)
        if (string[i] == '\n')
            h += height;

    return h;
}

//
//      Write a string using the hu_font
//
void M_WriteText(struct doom_data_t_* doom,
                 int x,
                 int y,
                 char *string)
{
    int w;
    char *ch;
    int c;
    int cx;
    int cy;

    ch = string;
    cx = x;
    cy = y;

    while (1)
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = x;
            cy += 12;
            continue;
        }

        c = d_toupper(c) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        w = SHORT(hu_font[c]->width);
        if (cx + w > SCREENWIDTH)
            break;
        V_DrawPatchDirect(doom, cx, cy, hu_font[c]);
        cx += w;
    }
}

// These keys evaluate to a "null" key in Vanilla Doom that allows weird
// jumping in the menus. Preserve this behavior for accuracy.

static boolean IsNullKey(int key)
{
    return key == KEY_PAUSE || key == KEY_CAPSLOCK || key == KEY_SCRLCK || key == KEY_NUMLOCK;
}

//
// CONTROL PANEL
//

//
// M_Responder
//
boolean M_Responder(doom_data_t *doom, event_t *ev)
{
    int key;
    int i;
    static int mousey = 0;
    static int lasty = 0;
    static int mousex = 0;
    static int lastx = 0;

    // In testcontrols mode, none of the function keys should do anything
    // - the only key is escape to quit.

    if (doom->testcontrols)
    {
        if (ev->type == ev_quit || (ev->type == ev_keydown && (ev->data1 == key_menu_activate || ev->data1 == key_menu_quit)))
        {
            I_Quit(doom);
            return true;
        }

        return false;
    }

    // "close" button pressed on window?
    if (ev->type == ev_quit)
    {
        // First click on close button = bring up quit confirm message.
        // Second click on close button = confirm quit

        if (menuactive && messageToPrint && messageRoutine == M_QuitResponse)
        {
            M_QuitResponse(doom, key_menu_confirm);
        }
        else
        {
            S_StartSound(doom, NULL, sfx_swtchn);
            M_QuitDOOM(doom, 0);
        }

        return true;
    }

    // key is the key pressed, ch is the actual character typed

    key = -1;

    if (ev->type == ev_joystick)
    {
        if (ev->data3 < 0)
        {
            key = key_menu_up;
        }
        else if (ev->data3 > 0)
        {
            key = key_menu_down;
        }

        if (ev->data2 < 0)
        {
            key = key_menu_left;
        }
        else if (ev->data2 > 0)
        {
            key = key_menu_right;
        }

        if (ev->data1 & 1)
        {
            key = key_menu_forward;
        }
        if (ev->data1 & 2)
        {
            key = key_menu_back;
        }
        if (joybmenu >= 0 && (ev->data1 & (1 << joybmenu)) != 0)
        {
            key = key_menu_activate;
        }
    }
    else
    {
        if (ev->type == ev_mouse)
        {
            mousey += ev->data3;
            mousex += ev->data2;

            if (ev->data1 & 1)
            {
                key = key_menu_forward;
            }

            if (ev->data1 & 2)
            {
                key = key_menu_back;
            }
        }
        else
        {
            if (ev->type == ev_keydown)
            {
                key = ev->data1;
            }
        }
    }

    if (key == -1)
        return false;

    // Save Game string input
    if (saveStringEnter)
    {
        switch (key)
        {
        case KEY_BACKSPACE:
            if (saveCharIndex > 0)
            {
                saveCharIndex--;
                savegamestrings[saveSlot][saveCharIndex] = 0;
            }
            break;

        case KEY_ESCAPE:
            saveStringEnter = 0;
            M_StringCopy(savegamestrings[saveSlot], saveOldString,
                         SAVESTRINGSIZE);
            break;

        case KEY_ENTER:
            saveStringEnter = 0;
            if (savegamestrings[saveSlot][0])
                M_DoSave(doom, saveSlot);
            break;

        default:

            int ch = d_toupper(key);

            if (ch != ' ' && (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE))
            {
                break;
            }

            if (ch >= 32 && ch <= 127 &&
                saveCharIndex < SAVESTRINGSIZE - 1 &&
                M_StringWidth(savegamestrings[saveSlot]) <
                    (SAVESTRINGSIZE - 2) * 8)
            {
                savegamestrings[saveSlot][saveCharIndex++] = ch;
                savegamestrings[saveSlot][saveCharIndex] = 0;
            }
            break;
        }
        return true;
    }

    // Take care of any messages that need input
    if (messageToPrint)
    {
        if (messageNeedsInput)
        {
            if (key != ' ' && key != KEY_ESCAPE && key != key_menu_confirm && key != key_menu_abort)
            {
                return false;
            }
        }

        menuactive = messageLastMenuActive;
        messageToPrint = 0;
        if (messageRoutine)
            messageRoutine(doom, key);

        menuactive = false;
        S_StartSound(doom, NULL, sfx_swtchx);
        return true;
    }

    if ((doom->devparm && key == key_menu_help) ||
        (key != 0 && key == key_menu_screenshot))
    {
        G_ScreenShot(doom);
        return true;
    }

    // F-Keys
    if (!menuactive)
    {
        if (key == key_menu_decscreen) // Screen size down
        {
            if (doom->automapactive || chat_on)
                return false;
            M_SizeDisplay(doom, 0);
            S_StartSound(doom, NULL, sfx_stnmov);
            return true;
        }
        else if (key == key_menu_incscreen) // Screen size up
        {
            if (doom->automapactive || chat_on)
                return false;
            M_SizeDisplay(doom, 1);
            S_StartSound(doom, NULL, sfx_stnmov);
            return true;
        }
        else if (key == key_menu_help) // Help key
        {
            M_StartControlPanel();

            if (doom->gamemode == retail)
                currentMenu = &ReadDef2;
            else
                currentMenu = &ReadDef1;

            itemOn = 0;
            S_StartSound(doom, NULL, sfx_swtchn);
            return true;
        }
        else if (key == key_menu_save) // Save
        {
            M_StartControlPanel();
            S_StartSound(doom, NULL, sfx_swtchn);
            M_SaveGame(doom, 0);
            return true;
        }
        else if (key == key_menu_load) // Load
        {
            M_StartControlPanel();
            S_StartSound(doom, NULL, sfx_swtchn);
            M_LoadGame(doom, 0);
            return true;
        }
        else if (key == key_menu_volume) // Sound Volume
        {
            M_StartControlPanel();
            currentMenu = &SoundDef;
            itemOn = sfx_vol;
            S_StartSound(doom, NULL, sfx_swtchn);
            return true;
        }
        else if (key == key_menu_detail) // Detail toggle
        {
            M_ChangeDetail(doom, 0);
            S_StartSound(doom, NULL, sfx_swtchn);
            return true;
        }
        else if (key == key_menu_qsave) // Quicksave
        {
            S_StartSound(doom, NULL, sfx_swtchn);
            M_QuickSave(doom);
            return true;
        }
        else if (key == key_menu_endgame) // End game
        {
            S_StartSound(doom, NULL, sfx_swtchn);
            M_EndGame(doom, 0);
            return true;
        }
        else if (key == key_menu_messages) // Toggle messages
        {
            M_ChangeMessages(doom, 0);
            S_StartSound(doom, NULL, sfx_swtchn);
            return true;
        }
        else if (key == key_menu_qload) // Quickload
        {
            S_StartSound(doom, NULL, sfx_swtchn);
            M_QuickLoad(doom);
            return true;
        }
        else if (key == key_menu_quit) // Quit DOOM
        {
            S_StartSound(doom, NULL, sfx_swtchn);
            M_QuitDOOM(doom, 0);
            return true;
        }
        else if (key == key_menu_gamma) // gamma toggle
        {
            usegamma++;
            if (usegamma > 4)
                usegamma = 0;
            doom->players[doom->consoleplayer].message = DEH_String(gammamsg[usegamma]);
            I_SetPalette(W_CacheLumpName(doom, DEH_String("PLAYPAL"), PU_CACHE));
            return true;
        }
    }

    // Pop-up menu?
    if (!menuactive)
    {
        if (key == key_menu_activate)
        {
            M_StartControlPanel();
            S_StartSound(doom, NULL, sfx_swtchn);
            return true;
        }
        return false;
    }

    // Keys usable within menu

    if (key == key_menu_down)
    {
        // Move down to next item

        do
        {
            if (itemOn + 1 > currentMenu->numitems - 1)
                itemOn = 0;
            else
                itemOn++;
            S_StartSound(doom, NULL, sfx_pstop);
        } while (currentMenu->menuitems[itemOn].status == -1);

        return true;
    }
    else if (key == key_menu_up)
    {
        // Move back up to previous item

        do
        {
            if (!itemOn)
                itemOn = currentMenu->numitems - 1;
            else
                itemOn--;
            S_StartSound(doom, NULL, sfx_pstop);
        } while (currentMenu->menuitems[itemOn].status == -1);

        return true;
    }
    else if (key == key_menu_left)
    {
        // Slide slider left

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(doom, NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(doom, 0);
        }
        return true;
    }
    else if (key == key_menu_right)
    {
        // Slide slider right

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(doom, NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(doom, 1);
        }
        return true;
    }
    else if (key == key_menu_forward)
    {
        // Activate menu item

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status)
        {
            currentMenu->lastOn = itemOn;
            if (currentMenu->menuitems[itemOn].status == 2)
            {
                currentMenu->menuitems[itemOn].routine(doom, 1); // right arrow
                S_StartSound(doom, NULL, sfx_stnmov);
            }
            else
            {
                currentMenu->menuitems[itemOn].routine(doom, itemOn);
                S_StartSound(doom, NULL, sfx_pistol);
            }
        }
        return true;
    }
    else if (key == key_menu_activate)
    {
        // Deactivate menu

        currentMenu->lastOn = itemOn;
        M_ClearMenus();
        S_StartSound(doom, NULL, sfx_swtchx);
        return true;
    }
    else if (key == key_menu_back)
    {
        // Go back to previous menu

        currentMenu->lastOn = itemOn;
        if (currentMenu->prevMenu)
        {
            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
            S_StartSound(doom, NULL, sfx_swtchn);
        }
        return true;
    }

    // Keyboard shortcut?
    // Vanilla Doom has a weird behavior where it jumps to the scroll bars
    // when the certain keys are pressed, so emulate this.

    else if (IsNullKey(key))
    {
        for (i = itemOn + 1; i < currentMenu->numitems; i++)
        {

            itemOn = i;
            S_StartSound(doom, NULL, sfx_pstop);
            return true;
        }

        for (i = 0; i <= itemOn; i++)
        {
            itemOn = i;
            S_StartSound(doom, NULL, sfx_pstop);
            return true;
        }
    }

    return false;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
    // intro might call this repeatedly
    if (menuactive)
        return;

    menuactive = 1;
    currentMenu = &MainDef;       // JDC
    itemOn = currentMenu->lastOn; // JDC
}

// Display OPL debug messages - hack for GENMIDI development.

#if 0
static void M_DrawOPLDev(void)
{
    extern void I_OPL_DevMessages(char *, size_t);
    char debug[1024];
    char *curr, *p;
    int line;

    //XXX I_OPL_DevMessages(debug, sizeof(debug));
    curr = debug;
    line = 0;

    for (;;)
    {
        p = d_strchr(curr, '\n');

        if (p != NULL)
        {
            *p = '\0';
        }

        M_WriteText(0, line * 8, curr);
        ++line;

        if (p == NULL)
        {
            break;
        }

        curr = p + 1;
    }
}
#endif

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(struct doom_data_t_ *doom)
{
    static short x;
    static short y;
    unsigned int i;
    unsigned int max;
    char string[80];
    char *name;
    int start;

    inhelpscreens = false;

    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
        start = 0;
        y = SCREENHEIGHT / 2 - M_StringHeight(messageString) / 2;
        while (messageString[start] != '\0')
        {
            int foundnewline = 0;

            for (i = 0; i < d_strlen(messageString + start); i++)
            {
                if (messageString[start + i] == '\n')
                {
                    M_StringCopy(string, messageString + start,
                                 sizeof(string));
                    if (i < sizeof(string))
                    {
                        string[i] = '\0';
                    }

                    foundnewline = 1;
                    start += i + 1;
                    break;
                }
            }

            if (!foundnewline)
            {
                M_StringCopy(string, messageString + start, sizeof(string));
                start += d_strlen(string);
            }

            x = SCREENWIDTH / 2 - M_StringWidth(string) / 2;
            M_WriteText(doom, x, y, string);
            y += SHORT(hu_font[0]->height);
        }

        return;
    }

    // if (opldev)
    //{
    //     M_DrawOPLDev();
    // }

    if (!menuactive)
        return;

    if (currentMenu->routine)
        currentMenu->routine(doom); // call Draw routine

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;
    max = currentMenu->numitems;

    for (i = 0; i < max; i++)
    {
        name = DEH_String(currentMenu->menuitems[i].name);

        if (name[0])
        {
            V_DrawPatchDirect(doom, x, y, W_CacheLumpName(doom, name, PU_CACHE));
        }
        y += LINEHEIGHT;
    }

    // DRAW SKULL
    V_DrawPatchDirect(doom, x + SKULLXOFF, currentMenu->y - 5 + itemOn * LINEHEIGHT,
                      W_CacheLumpName(doom, DEH_String(skullName[whichSkull]),
                                      PU_CACHE));
}

//
// M_ClearMenus
//
void M_ClearMenus(void)
{
    menuactive = 0;
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}

//
// M_Ticker
//
void M_Ticker(void)
{
    if (--skullAnimCounter <= 0)
    {
        whichSkull ^= 1;
        skullAnimCounter = 8;
    }
}

//
// M_Init
//
void M_Init(doom_data_t *doom)
{
    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    whichSkull = 0;
    skullAnimCounter = 10;
    screenSize = screenblocks - 3;
    messageToPrint = 0;
    messageString = NULL;
    messageLastMenuActive = menuactive;
    quickSaveSlot = -1;

    // Here we could catch other version dependencies,
    //  like HELP1/2, and four episodes.

    switch (doom->gamemode)
    {
    case commercial:
        // Commercial has no "read this" entry.
        MainMenu[readthis] = MainMenu[quitdoom];
        MainDef.numitems--;
        MainDef.y += 8;
        NewDef.prevMenu = &MainDef;
        break;
    case shareware:
        // Episode 2 and 3 are handled,
        //  branching to an ad screen.
    case registered:
        break;
    case retail:
        // We are fine.
    default:
        break;
    }

    // Versions of doom.exe before the Ultimate Doom release only had
    // three episodes; if we're emulating one of those then don't try
    // to show episode four. If we are, then do show episode four
    // (should crash if missing).
    if (doom->gameversion < exe_ultimate)
    {
        EpiDef.numitems--;
    }

    // opldev = M_CheckParm("-opldev") > 0;
}
