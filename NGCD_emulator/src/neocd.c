/**************************************
****  NEOCD.C  - Main Source File  ****
**************************************/

//-- Include Files -----------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <SDL.h>
#include <ctype.h>
#include <string.h>

#include "neocd.h"
#include "icon.h"

#define REGION_JAPAN  0
#define REGION_USA    1
#define REGION_EUROPE 2

#define REGION REGION_USA


//-- Private variables -------------------------------------------------------
static char		neocd_wm_title[255]=VERSION1;

//-- Global Variables --------------------------------------------------------
char			*neogeo_rom_memory = NULL;
char			*neogeo_prg_memory = NULL;
char			*neogeo_fix_memory = NULL;
char			*neogeo_spr_memory = NULL;
char			*neogeo_pcm_memory = NULL;

char			global_error[80];
unsigned char		neogeo_memorycard[8192];
int			neogeo_prio_mode = 0;
int			neogeo_ipl_done = 0;
char			neogeo_region=REGION;
unsigned char		config_game_name[80];

Uint32		neocd_time; /* next time marker */


//-- 68K Core related stuff --------------------------------------------------
int				mame_debug = 0;
int				previouspc = 0;
int				ophw = 0;
int				cur_mrhard = 0;


//-- Function Prototypes -----------------------------------------------------
void	neogeo_init(void);
void	neogeo_reset(void);
void	neogeo_hreset(void);
void	neogeo_shutdown(void);
void	MC68000_Cause_Interrupt(int);
void	neogeo_exception(void);
void	neogeo_run(void);
void	draw_main(void);
void	neogeo_quit(void);
void	not_implemented(void);
void	neogeo_machine_settings(void);
void	neogeo_debug_mode(void);
void	neogeo_cdda_check(void);
void	neogeo_cdda_control(void);
void	neogeo_do_cdda( int command, int trck_number_bcd);
void	neogeo_read_gamename(void);

//----------------------------------------------------------------------------
int	main(int argc, char* argv[])
{
	FILE			*fp;
	SDL_RWops	*iconfp;
	SDL_Surface *icon;
	int				result;
	//int				a, b, c,w,h,i;
	
	char * fixtmp;

	// Initialise SDL
	if((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_CDROM|SDL_INIT_JOYSTICK|SDL_INIT_NOPARACHUTE)==-1)) { 
            printf("Could not initialize SDL: %s.\n", SDL_GetError());
            exit(-1);
        }

	//set up window icon
	iconfp=SDL_RWFromMem((void*)rawicon, sizeof(rawicon));
	icon=SDL_LoadBMP_RW(iconfp,0);
	SDL_WM_SetIcon(icon,NULL);

	// Displays version number, date and time
	puts(VERSION1);
	puts(VERSION2);

	// Allocate needed memory buffers
	
	puts("NEOGEO: Allocating memory...");

	printf("PRG (2MB) ... ");
	neogeo_prg_memory = calloc(1, 0x200000);
	if (neogeo_prg_memory==NULL) {
		puts("failed !");
		return 0;
	}
	puts("OK!");

	printf("SPR (4MB) ... ");
	neogeo_spr_memory = calloc(1, 0x400000);
	if (neogeo_spr_memory==NULL) {
		puts("failed !");
		return 0;
	}
	puts("OK!");
	
	printf("ROM (512kb) ... ");
	neogeo_rom_memory = calloc(1, 0x80000);
	if (neogeo_rom_memory==NULL) {
		puts("failed !");
		return 0;
	}
	puts("OK!");

	printf("FIX (128kb) ... ");
	neogeo_fix_memory = calloc(1, 0x20000);
	if (neogeo_fix_memory==NULL) {
		puts("failed !");
		return 0;
	}
	puts("OK!");

	printf("PCM (1Mb) ... ");
	neogeo_pcm_memory = calloc(1, 0x100000);
	if (neogeo_pcm_memory==NULL) {
		puts("failed !");
		return 0;
	}
	puts("OK!");
	
	// Initialize Memory Mapping
	initialize_memmap();

	// Read memory card
	fp = fopen("memcard.bin", "rb");
	if (fp!=NULL) {
		fread(neogeo_memorycard, 1, 8192, fp);
		fclose(fp);
	}

	// Register exit procedure
	atexit(neogeo_shutdown);
	
	// Video init
	result = video_init();
	if (!result)
	{
		video_mode = 0;
		puts(global_error);
		return 0;
	}

	//set window title
	SDL_WM_SetCaption(neocd_wm_title,neocd_wm_title);	


	//Hide Mouse Pointer
	SDL_ShowCursor(SDL_DISABLE);

	// Load BIOS
	fp = fopen("neocd.bin", "rb");
	if (fp==NULL) {
		printf("Fatal Error: Could not load NEOCD.BIN\n");
		return 0;
	}
	fread(neogeo_rom_memory, 1, 0x80000, fp);
	fclose(fp);

	fixtmp=calloc(1, 65536);
	memcpy(fixtmp,&neogeo_rom_memory[458752],65536);
	
	fix_conv(fixtmp,&neogeo_rom_memory[458752],65536,rom_fix_usage);
	/*swab(neogeo_rom_memory, neogeo_rom_memory, 131072);*/
	free(fixtmp);
	fixtmp=NULL;

	// Load startup RAM
	fp = fopen("startup.bin", "rb");
	if (fp==NULL) {
		printf("Fatal Error: Could not load STARTUP.BIN\n");
		return 0;
	}
	fread(neogeo_prg_memory + 0x10F300, 1, 3328, fp);
	fclose(fp);
	swab(neogeo_prg_memory + 0x10F300, neogeo_prg_memory + 0x10F300, 3328);


	// Check BIOS validity
	if (*((short*)(neogeo_rom_memory+0xA822)) != 0x4BF9)
	{
		printf("Fatal Error: Invalid BIOS file.");
		return 0;
	}


	/*** Patch BIOS load files w/ now loading message ***/
	*((short*)(neogeo_rom_memory+0x552)) = 0xFABF;
	*((short*)(neogeo_rom_memory+0x554)) = 0x4E75;
	/*** Patch BIOS load files w/out now loading ***/
	*((short*)(neogeo_rom_memory+0x564)) = 0xFAC0;
	*((short*)(neogeo_rom_memory+0x566)) = 0x4E75;
	/*** Patch BIOS CDROM Check ***/
	*((short*)(neogeo_rom_memory+0xB040)) = 0x4E71;
	*((short*)(neogeo_rom_memory+0xB042)) = 0x4E71;
	/*** Patch BIOS upload command ***/
	*((short*)(neogeo_rom_memory+0x546)) = 0xFAC1;
	*((short*)(neogeo_rom_memory+0x548)) = 0x4E75;

	/*** Patch BIOS CDDA check ***/
	*((short*)(neogeo_rom_memory+0x56A)) = 0xFAC3;
	*((short*)(neogeo_rom_memory+0x56C)) = 0x4E75;

	/*** Full reset, please ***/
	*((short*)(neogeo_rom_memory+0xA87A)) = 0x4239;
	*((short*)(neogeo_rom_memory+0xA87C)) = 0x0010;
	*((short*)(neogeo_rom_memory+0xA87E)) = 0xFDAE;

	/*** Trap exceptions ***/
	*((short*)(neogeo_rom_memory+0xA5B6)) = 0x4AFC;


	// Initialise input
	input_init();

	// Initialize CD-ROM
	cdrom_init1();
	cdda_current_drive=cdrom_current_drive;
	cdda_init();
	
	// Sound init
	init_sdl_audio();

	// Initialize everything
	neogeo_init();
	pd4990a_init();
	neogeo_run();
	
	return 0;
}

//----------------------------------------------------------------------------
void	neogeo_init(void)
{
	m68k_pulse_reset();
}

//----------------------------------------------------------------------------
void	neogeo_hreset(void)
{

	FILE * fp;

	/* read game name */
	neogeo_read_gamename();

	/* Special patch for Samurai Spirits RPG */
	if (strcmp(config_game_name, "TEST PROGRAM USA") == 0)
	{
		strcpy(config_game_name, "SAMURAI SPIRITS RPG");
		
		fp = fopen("patch.prg", "rb");
		if (fp == NULL) {
			printf("Fatal Error: Couldnt open patch.prg.\n");
			exit(1);
		}
		
		fread(neogeo_prg_memory + 0x132000, 1, 112, fp);
		fclose(fp);
		swab(neogeo_prg_memory + 0x132000, neogeo_prg_memory + 0x132000, 112);
	
	}
	
	/* update window title with game name */
	strcat(neocd_wm_title," - ");
	strcat(neocd_wm_title,config_game_name);
	SDL_WM_SetCaption(neocd_wm_title,neocd_wm_title);

	// First time init
	m68k_pulse_reset();
	m68k_set_reg(M68K_REG_PC,0xc0a822);
	m68k_set_reg(M68K_REG_SR,0x2700);
	m68k_set_reg(M68K_REG_A7,0x10F300);
	m68k_set_reg(M68K_REG_ISP,0x10F300);
	m68k_set_reg(M68K_REG_USP,0x10F300);
	
	m68k_write_memory_32(0x10F6EE, m68k_read_memory_32(0x68L)); // $68 *must* be copied at 10F6EE

	if (m68k_read_memory_8(0x107)&0x7E)
	{
		if (m68k_read_memory_16(0x13A))
		{
			m68k_write_memory_32(0x10F6EA, (m68k_read_memory_16(0x13A)<<1) + 0xE00000);
		}
		else
		{
			m68k_write_memory_32(0x10F6EA, 0);
			m68k_write_memory_8(0x00013B, 0x01);
		}
	}
	else
		m68k_write_memory_32(0x10F6EA, 0xE1FDF0);

	/* Set System Region */
	m68k_write_memory_8(0x10FD83,neogeo_region);

	cdda_current_track = 0;
	cdda_get_disk_info();

	z80_init();
}	

//----------------------------------------------------------------------------
void	neogeo_reset(void)
{

	m68k_pulse_reset();
	m68k_set_reg(M68K_REG_PC,0x122);
	m68k_set_reg(M68K_REG_SR,0x2700);
	m68k_set_reg(M68K_REG_A7,0x10F300);
	m68k_set_reg(M68K_REG_ISP,0x10F300);
	m68k_set_reg(M68K_REG_USP,0x10F300);

	m68k_write_memory_8(0x10FD80, 0x82);
	m68k_write_memory_8(0x10FDAF, 0x01);
	m68k_write_memory_8(0x10FEE1, 0x0A);
	m68k_write_memory_8(0x10F675, 0x01);
	m68k_write_memory_8(0x10FEBF, 0x00);
	m68k_write_memory_32(0x10FDB6, 0);
	m68k_write_memory_32(0x10FDBA, 0);

	/* System Region */
	m68k_write_memory_8(0x10FD83,neogeo_region);

	cdda_current_track = 0;
	z80_init();
}

//----------------------------------------------------------------------------
void	neogeo_shutdown(void)
{
	FILE	*fp;
	
	// Close everything and free memory
	cdda_shutdown();
	cdrom_shutdown();
	sound_shutdown();
	input_shutdown();
	video_shutdown();

	printf("NEOGEO: System Shutdown.\n");

	fp = fopen("memcard.bin", "wb");
	if (fp != NULL) {	
		fwrite(neogeo_memorycard, 1, 8192, fp);
		fclose(fp);
	} else {
		printf("Error: Couldn't open memcard.bin for writing.\n");
	}

	free(neogeo_prg_memory);
	free(neogeo_rom_memory);
	free(neogeo_spr_memory);
	free(neogeo_fix_memory);
	free(neogeo_pcm_memory);
	
	SDL_Quit();
	
	return;
}

//----------------------------------------------------------------------------
void	neogeo_exception(void)
{
	printf("NEOGEO: Exception Trapped at %08x !\n", previouspc);
	exit(0);
}	

//----------------------------------------------------------------------------
void MC68000_Cause_Interrupt(int level)
{
	m68k_set_irq(level);
}

//----------------------------------------------------------------------------
void	neogeo_exit(void)
{
	puts("NEOGEO: Exit requested by software...");
	exit(0);
}

//----------------------------------------------------------------------------
void	neogeo_run(void)
{
	Uint32 now;

    int	i;

	// If IPL.TXT not loaded, load it !
	if (!neogeo_ipl_done)
	{
		// Display Title
		cdrom_load_title();

		// Process IPL.TXT
		if (!cdrom_process_ipl()) {
			printf("Error: Error while processing IPL.TXT.\n");
			return;
		}
		
		// Reset everything
		neogeo_ipl_done = 1;
		neogeo_hreset();
	}

	/* get time for speed throttle */
    neocd_time=SDL_GetTicks()+REFRESHTIME;
	
	// Main loop
	my_timer();
	z80_cycles = Z80_VBL_CYCLES/256;
	while(1)
	{
		// Execute Z80 timeslice (one VBL)
		mz80int(0);
		
		if (SDL_GetAudioStatus()==SDL_AUDIO_PLAYING) {
		    for (i = 0; i < 256; i++) {
			if (z80_cycles>0) {
			    mz80exec(z80_cycles);
			    z80_cycles=0;
			    my_timer();
			}
			z80_cycles += Z80_VBL_CYCLES/256;
		    }
		}
		
		// One-vbl timeslice
		m68k_execute(200000);
		m68k_set_irq(2);
		
		/* update pd4990a */
		pd4990a_addretrace();
		
		/* check the watchdog */
		if (watchdog_counter > 0) {
		    if (--watchdog_counter == 0) {
			//logerror("reset caused by the watchdog\n");
			neogeo_reset();
		    }
		}
		
		/* check for memcard writes */
		if (memcard_write > 0) {
		   memcard_write--;
		   if(memcard_write==0) {
		     /* write memory card here */
			 /* if you need to keep file up to date */
		   }
		}

		// Call display routine
		video_draw_screen1();

		// Check if there are pending commands for CDDA
		neogeo_cdda_check();
		cdda_loop_check();

		/* Update keys and Joystick */
		processEvents();
		
		/* Speed Throttle */	
		now=SDL_GetTicks();
		if (now < neocd_time) {
			SDL_Delay(neocd_time-now);
		}
		neocd_time+=REFRESHTIME;
	}

	// Stop CDDA
	cdda_stop();
		
	return;
}

//----------------------------------------------------------------------------
// This is a really dirty hack to make SAMURAI SPIRITS RPG work
void	neogeo_prio_switch(void)
{
	if (m68k_get_reg(NULL,M68K_REG_D7) == 0xFFFF)
		return;
	
	if (m68k_get_reg(NULL,M68K_REG_D7) == 9 && 
	    m68k_get_reg(NULL,M68K_REG_A3) == 0x10DED9 &&
		(m68k_get_reg(NULL,M68K_REG_A2) == 0x1081d0 ||
		(m68k_get_reg(NULL,M68K_REG_A2)&0xFFF000) == 0x102000)) {
		neogeo_prio_mode = 0;
		return;
	}
	
	if (m68k_get_reg(NULL,M68K_REG_D7) == 8 && 
	    m68k_get_reg(NULL,M68K_REG_A3) == 0x10DEC7 && 
		m68k_get_reg(NULL,M68K_REG_A2) == 0x102900) {
		neogeo_prio_mode = 0;
		return;
	}
	
	if (m68k_get_reg(NULL,M68K_REG_A7) == 0x10F29C)
	{
		if ((m68k_get_reg(NULL,M68K_REG_D4)&0x4010) == 0x4010)
		{
			neogeo_prio_mode = 0;
			return;
		}
		
		neogeo_prio_mode = 1;
	}
	else
	{
		if (m68k_get_reg(NULL,M68K_REG_A3) == 0x5140)
		{
			neogeo_prio_mode = 1;
			return;
		}

		if ( (m68k_get_reg(NULL,M68K_REG_A3)&~0xF) == (m68k_get_reg(NULL,M68K_REG_A4)&~0xF) )
			neogeo_prio_mode = 1;
		else
			neogeo_prio_mode = 0;
	}
}

//----------------------------------------------------------------------------
void	not_implemented(void)
{
		printf("Error: This function isn't implemented.");
}

//----------------------------------------------------------------------------
void	neogeo_quit(void)
{
		exit(0);
}

//----------------------------------------------------------------------------
void neogeo_cdda_check(void)
{
	int		Offset;
	
	Offset = m68k_read_memory_32(0x10F6EA);
	if (Offset < 0xE00000)	// Invalid addr
		return;

	Offset -= 0xE00000;
	Offset >>= 1;
	
	neogeo_do_cdda(subcpu_memspace[Offset], subcpu_memspace[Offset+1]);
}

//----------------------------------------------------------------------------
void neogeo_cdda_control(void)
{
	neogeo_do_cdda( (m68k_get_reg(NULL,M68K_REG_D0)>>8)&0xFF, 
	                 m68k_get_reg(NULL,M68K_REG_D0)&0xFF );
}

//----------------------------------------------------------------------------
void neogeo_do_cdda( int command, int track_number_bcd)
{
	int		track_number;
	int		offset;

	if ((command == 0)&&(track_number_bcd == 0))
		return;

	m68k_write_memory_8(0x10F64B, track_number_bcd);
	m68k_write_memory_8(0x10F6F8, track_number_bcd);
	m68k_write_memory_8(0x10F6F7, command);
	m68k_write_memory_8(0x10F6F6, command);

	offset = m68k_read_memory_32(0x10F6EA);

	if (offset)
	{
		offset -= 0xE00000;
		offset >>= 1;

		m68k_write_memory_8(0x10F678, 1);

		subcpu_memspace[offset] = 0;
		subcpu_memspace[offset+1] = 0;
	}

	switch( command )
	{
		case	0:
		case	1:
		case	5:
		case	4:
		case	3:
		case	7:
			track_number = ((track_number_bcd>>4)*10) + (track_number_bcd&0x0F);
			if ((track_number == 0)&&(!cdda_playing))
			{
				//sound_mute();
				cdda_resume();
			}
			else if ((track_number>1)&&(track_number<99))
			{
				//sound_mute();
				cdda_play(track_number);
				cdda_autoloop = !(command&1);
			}
			break;
		case	6:
		case	2:
			if (cdda_playing)
			{
				//sound_mute();
				cdda_pause();
			}
			break;
	}
}
//----------------------------------------------------------------------------
void neogeo_read_gamename(void)
{

	unsigned char	*Ptr;
	int				temp;

	Ptr = neogeo_prg_memory + m68k_read_memory_32(0x11A);
	swab(Ptr, config_game_name, 80);

	for(temp=0;temp<80;temp++) {
		if (!isprint(config_game_name[temp])) {
			config_game_name[temp]=0;
			break;
		}
	}
}

