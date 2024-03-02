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
//

#include "dlibc.h"

#include "doomtype.h"
#include "i_system.h"
#include "m_misc.h"
#include "m_argv.h" // haleyjd 20110212: warning fix

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
//

int M_CheckParmWithArgs(struct doom_data_t_* doom, char *check, int num_args)
{
    int i;

    for (i = 1; i < doom->myargc - num_args; i++)
    {
        if (!d_stricmp(check, doom->myargv[i]))
            return i;
    }

    return 0;
}

//
// M_ParmExists
//
// Returns true if the given parameter exists in the program's command
// line arguments, false if not.
//

boolean M_ParmExists(struct doom_data_t_* doom, char *check)
{
    return M_CheckParm(doom, check) != 0;
}

int M_CheckParm(struct doom_data_t_* doom, char *check)
{
    return M_CheckParmWithArgs(doom, check, 0);
}

#define MAXARGVS 100

// Return the name of the executable used to start the program:

char *M_GetExecutableName(struct doom_data_t_* doom)
{
    char *sep;

    sep = d_strchr(doom->myargv[0], DIR_SEPARATOR);

    if (sep == NULL)
    {
        return doom->myargv[0];
    }
    else
    {
        return sep + 1;
    }
}
