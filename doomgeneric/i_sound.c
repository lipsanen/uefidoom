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

#include "dlibc.h"
#include "config.h"
#include "doomfeatures.h"
#include "doomtype.h"
#include "i_sound.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"


void I_InitSound(struct doom_data_t_* doom, boolean use_sfx_prefix)
{

}

void I_ShutdownSound(void)
{

}

int I_GetSfxLumpNum(sfxinfo_t *sfxinfo)
{

}

void I_UpdateSound(void)
{

}

void I_UpdateSoundParams(int channel, int vol, int sep)
{

}

int I_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep)
{

}

void I_StopSound(int channel)
{

}

boolean I_SoundIsPlaying(int channel)
{
    return false;
}

void I_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{

}

void I_InitMusic(void)
{

}

void I_ShutdownMusic(void)
{
}

void I_SetMusicVolume(int volume)
{

}

void I_PauseSong(void)
{

}

void I_ResumeSong(void)
{

}

void *I_RegisterSong(void *data, int len)
{

}

void I_UnRegisterSong(void *handle)
{

}

void I_PlaySong(void *handle, boolean looping)
{

}

void I_StopSong(void)
{

}

boolean I_MusicIsPlaying(void)
{
    return false;
}

void I_BindSoundVariables(void)
{

}
