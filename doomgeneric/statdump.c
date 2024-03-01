 /*

 Copyright(C) 2005-2014 Simon Howard

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 --

 Functions for presenting the information captured from the statistics
 buffer to a file.

 */

#include "dlibc.h"

#include "d_player.h"
#include "d_mode.h"
#include "m_argv.h"

#include "statdump.h"

/* Par times for E1M1-E1M9. */
static const int doom1_par_times[] =
{
    30, 75, 120, 90, 165, 180, 180, 30, 165,
};

/* Par times for MAP01-MAP09. */
static const int doom2_par_times[] =
{
    30, 90, 120, 120, 90, 150, 120, 120, 270,
};

#if ORIGCODE

/* Player colors. */
static const char *player_colors[] =
{
    "Green", "Indigo", "Brown", "Red"
};

#endif

// Array of end-of-level statistics that have been captured.

#define MAX_CAPTURES 32
static wbstartstruct_t captured_stats[MAX_CAPTURES];
static int num_captured_stats = 0;

#if ORIGCODE
static GameMission_t discovered_gamemission = none;
#endif

#if ORIGCODE

/* Try to work out whether this is a Doom 1 or Doom 2 game, by looking
 * at the episode and map, and the par times.  This is used to decide
 * how to format the level name.  Unfortunately, in some cases it is
 * impossible to determine whether this is Doom 1 or Doom 2. */

static void DiscoverGamemode(wbstartstruct_t *stats, int num_stats)
{
    int partime;
    int level;
    int i;

    if (discovered_gamemission != none)
    {
        return;
    }

    for (i=0; i<num_stats; ++i)
    {
        level = stats[i].last;

        /* If episode 2, 3 or 4, this is Doom 1. */

        if (stats[i].epsd > 0)
        {
            discovered_gamemission = doom;
            return;
        }

        /* This is episode 1.  If this is level 10 or higher,
           it must be Doom 2. */

        if (level >= 9)
        {
            discovered_gamemission = doom2;
            return;
        }

        /* Try to work out if this is Doom 1 or Doom 2 by looking
           at the par time. */

        partime = stats[i].partime;

        if (partime == doom1_par_times[level] * TICRATE
         && partime != doom2_par_times[level] * TICRATE)
        {
            discovered_gamemission = doom;
            return;
        }

        if (partime != doom1_par_times[level] * TICRATE
         && partime == doom2_par_times[level] * TICRATE)
        {
            discovered_gamemission = doom2;
            return;
        }
    }
}

#endif

#if ORIGCODE

/* Returns the number of players active in the given stats buffer. */

static int GetNumPlayers(wbstartstruct_t *stats)
{
    int i;
    int num_players = 0;

    for (i=0; i<MAXPLAYERS; ++i)
    {
        if (stats->plyr[i].in)
        {
            ++num_players;
        }
    }

    return num_players;
}

#endif

#if ORIGCODE

static void PrintBanner(FILE *stream)
{
    d_fprintf(stream, "===========================================\n");
}

static void PrintPercentage(FILE *stream, int amount, int total)
{
    if (total == 0)
    {
        d_fprintf(stream, "0");
    }
    else
    {
        d_fprintf(stream, "%i / %i", amount, total);

        // statdump.exe is a 16-bit program, so very occasionally an
        // integer overflow can occur when doing this calculation with
        // a large value. Therefore, cast to short to give the same
        // output.

        d_fprintf(stream, " (%i%%)", (short) (amount * 100) / total);
    }
}

#endif

#if ORIGCODE

/* Display statistics for a single player. */

static void PrintPlayerStats(FILE *stream, wbstartstruct_t *stats,
        int player_num)
{
    wbplayerstruct_t *player = &stats->plyr[player_num];

    d_fprintf(stream, "Player %i (%s):\n", player_num + 1,
            player_colors[player_num]);

    /* Kills percentage */

    d_fprintf(stream, "\tKills: ");
    PrintPercentage(stream, player->skills, stats->maxkills);
    d_fprintf(stream, "\n");

    /* Items percentage */

    d_fprintf(stream, "\tItems: ");
    PrintPercentage(stream, player->sitems, stats->maxitems);
    d_fprintf(stream, "\n");

    /* Secrets percentage */

    d_fprintf(stream, "\tSecrets: ");
    PrintPercentage(stream, player->ssecret, stats->maxsecret);
    d_fprintf(stream, "\n");
}

#endif

#if ORIGCODE

/* Frags table for multiplayer games. */

static void PrintFragsTable(FILE *stream, wbstartstruct_t *stats)
{
    int x, y;

    d_fprintf(stream, "Frags:\n");

    /* Print header */

    d_fprintf(stream, "\t\t");

    for (x=0; x<MAXPLAYERS; ++x)
    {

        if (!stats->plyr[x].in)
        {
            continue;
        }

        d_fprintf(stream, "%s\t", player_colors[x]);
    }

    d_fprintf(stream, "\n");

    d_fprintf(stream, "\t\t-------------------------------- VICTIMS\n");

    /* Print table */

    for (y=0; y<MAXPLAYERS; ++y)
    {
        if (!stats->plyr[y].in)
        {
            continue;
        }

        d_fprintf(stream, "\t%s\t|", player_colors[y]);

        for (x=0; x<MAXPLAYERS; ++x)
        {
            if (!stats->plyr[x].in)
            {
                continue;
            }

            d_fprintf(stream, "%i\t", stats->plyr[y].frags[x]);
        }

        d_fprintf(stream, "\n");
    }

    d_fprintf(stream, "\t\t|\n");
    d_fprintf(stream, "\t     KILLERS\n");
}

#endif

#if ORIGCODE

/* Displays the level name: MAPxy or ExMy, depending on game mode. */

static void PrintLevelName(FILE *stream, int episode, int level)
{
    PrintBanner(stream);

    switch (discovered_gamemission)
    {

        case doom:
            d_fprintf(stream, "E%iM%i\n", episode + 1, level + 1);
            break;
        case doom2:
            d_fprintf(stream, "MAP%02i\n", level + 1);
            break;
        default:
        case none:
            d_fprintf(stream, "E%iM%i / MAP%02i\n", 
                    episode + 1, level + 1, level + 1);
            break;
    }

    PrintBanner(stream);
}

#endif

#if ORIGCODE

/* Print details of a statistics buffer to the given file. */

static void PrintStats(FILE *stream, wbstartstruct_t *stats)
{
    int leveltime, partime;
    int i;

    PrintLevelName(stream, stats->epsd, stats->last);
    d_fprintf(stream, "\n");

    leveltime = stats->plyr[0].stime / TICRATE;
    partime = stats->partime / TICRATE;
    d_fprintf(stream, "Time: %i:%02i", leveltime / 60, leveltime % 60);
    d_fprintf(stream, " (par: %i:%02i)\n", partime / 60, partime % 60);
    d_fprintf(stream, "\n");

    for (i=0; i<MAXPLAYERS; ++i)
    {
        if (stats->plyr[i].in)
        {
            PrintPlayerStats(stream, stats, i);
        }
    }

    if (GetNumPlayers(stats) >= 2)
    {
        PrintFragsTable(stream, stats);
    }

    d_fprintf(stream, "\n");
}

#endif

void StatCopy(wbstartstruct_t *stats)
{
    if (M_ParmExists("-statdump") && num_captured_stats < MAX_CAPTURES)
    {
        d_memcpy(&captured_stats[num_captured_stats], stats,
               sizeof(wbstartstruct_t));
        ++num_captured_stats;
    }
}

