/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

  
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "../neocd.h"
#include "streams.h"

int sound=1;
SDL_AudioSpec desired;

#include "sound.h"

#define MIXER_MAX_CHANNELS 16

//#define CPU_FPS 60
#define BUFFER_LEN 16384
extern Uint16 play_buffer[BUFFER_LEN];

//#define NB_SAMPLES 512 /* better resolution */
#define NB_SAMPLES 1024

void update_sdl_stream(void *userdata, Uint8 * stream, int len)
{
    streamupdate(len);
    memcpy(stream, (Uint8 *) play_buffer, len);
}

int init_sdl_audio(void)
{

    if(SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    	fprintf(stderr,"Couldn't Start Sound.\n");
    }

    desired.freq = SAMPLE_RATE;
    desired.samples = NB_SAMPLES;
    
#ifdef WORDS_BIGENDIAN
    desired.format = AUDIO_S16MSB;
#else	/* */
    desired.format = AUDIO_S16;
#endif	/* */
    desired.channels = 2;
    desired.callback = update_sdl_stream;
    desired.userdata = NULL;
    SDL_OpenAudio(&desired, NULL);
    
    //init rest of audio
    streams_sh_start();
	YM2610_sh_start();
	SDL_PauseAudio(0);
    
    return 1;
}

void sound_toggle(void) {
	SDL_PauseAudio(sound);
	sound^=1;
}

void sound_shutdown(void) {
	SDL_PauseAudio(1);
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    
    streams_sh_stop();
    YM2610_sh_stop();
    
}



