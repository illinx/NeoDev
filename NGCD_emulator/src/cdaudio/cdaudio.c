/**************************************
****   CDAUDIO.C  -  CD-DA Player  ****
**************************************/

//-- Include files -----------------------------------------------------------
#include <SDL.h>
#include <stdlib.h>
#include <string.h>

#include "../neocd.h"
#include "cdaudio.h"


//-- Private Variables -------------------------------------------------------
static int			cdda_min_track;
static int			cdda_max_track;
static int			cdda_disk_length;
static int			cdda_track_end;
static int			cdda_loop_counter;
static SDL_CD			*cdrom;

//-- Public Variables --------------------------------------------------------
int			cdda_first_drive=0;
int			cdda_current_drive=0;
int			cdda_current_track=0;
int			cdda_current_frame=0;
int			cdda_playing=0;
int			cdda_autoloop=0;
int			cdda_volume=0;
int			cdda_disabled=0;

//-- Function Prototypes -----------------------------------------------------
int			cdda_init(void);
int			cdda_play(int);
void			cdda_stop(void);
void			cdda_resume(void);
void			cdda_shutdown(void);
void			cdda_loop_check(void);
int 			cdda_get_disk_info(void);




//----------------------------------------------------------------------------
int	cdda_init(void)
{
	
	cdda_min_track = cdda_max_track = 0;
	cdda_current_track = 0;
	cdda_playing = 0;
	cdda_loop_counter = 0;

	/* Open the default drive */
	cdrom=SDL_CDOpen(cdda_current_drive);

	/* Did if open? Check if cdrom is NULL */
	if(cdrom == NULL){
		printf("Couldn't open drive %s for audio.  %s\n", SDL_CDName(cdda_current_drive), SDL_GetError());
		cdda_disabled=1;
		return 1;
	} else {
		cdda_disabled=0;
		printf("CD Audio OK!\n");
	}

	cdda_get_disk_info();
	return	1;
}

//----------------------------------------------------------------------------
int cdda_get_disk_info(void)
{
    if(cdda_disabled) return 1;

    if( CD_INDRIVE(SDL_CDStatus(cdrom)) ) {
        cdda_min_track = 0;
        cdda_max_track = cdrom->numtracks;
        cdda_disk_length = cdrom->numtracks;
        return 1;
    }
    else
    {
        printf("Error: No Disc in drive\n");
        cdda_disabled=1;
        return 1;
    }
}


//----------------------------------------------------------------------------
int cdda_play(int track)
{
    if(cdda_disabled) return 1;
    
    if(cdda_playing && cdda_current_track==track) return 1;

    if( CD_INDRIVE(SDL_CDStatus(cdrom)) ) {
    	SDL_CDPlayTracks(cdrom, track-1, 0, 1, 0);
    	cdda_current_track = track;
    	cdda_loop_counter=0;
    	cdda_track_end=(cdrom->track[track-1].length*60)/CD_FPS;//Length in 1/60s of second
    	cdda_playing = 1;
    	return 1;
    } 
    else
    { 
        cdda_disabled = 1;
        return 1;
    }
}

//----------------------------------------------------------------------------
void	cdda_pause(void)
{
	if(cdda_disabled) return;
	SDL_CDPause(cdrom);
	cdda_playing = 0;
}


void	cdda_stop(void)
{
	if(cdda_disabled) return;
	SDL_CDStop(cdrom);
	cdda_playing = 0;
}

//----------------------------------------------------------------------------
void	cdda_resume(void)
{
	if(cdda_disabled || cdda_playing) return;
	SDL_CDResume(cdrom);	
	cdda_playing = 1;
}

//----------------------------------------------------------------------------
void	cdda_shutdown(void)
{
	if(cdda_disabled) return;
	SDL_CDStop(cdrom);
	SDL_CDClose(cdrom);
}

//----------------------------------------------------------------------------
void	cdda_loop_check(void)
{
	if(cdda_disabled) return;
	if (cdda_playing==1) {
		cdda_loop_counter++;
		if (cdda_loop_counter>=cdda_track_end) {
			if (cdda_autoloop)
				cdda_play(cdda_current_track);
			else
				cdda_stop();
		}
	}
}

