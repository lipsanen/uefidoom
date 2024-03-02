#include <stdint.h>

#include "m_argv.h"
#include "i_video.h"
#include "doomgeneric.h"
#include "dlibc.h"
#include "doomdef.h"

void D_DoomMain (struct doom_data_t_* doom);


void doomgeneric_Create(struct doom_data_t_* doom, int argc, char **argv)
{
    // save arguments
    doom->myargc = argc;
    doom->myargv = argv;

    D_DoomMain (doom);
}

