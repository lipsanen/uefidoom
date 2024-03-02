#include <stdint.h>

#include "m_argv.h"
#include "i_video.h"
#include "doomgeneric.h"
#include "dlibc.h"

uint32_t* DG_ScreenBuffer = 0;

void D_DoomMain (struct doom_data_t_* doom);


void doomgeneric_Create(struct doom_data_t_* doom, int argc, char **argv)
{
	// save arguments
    myargc = argc;
    myargv = argv;

	D_DoomMain (doom);
}

