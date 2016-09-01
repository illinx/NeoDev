/*******************************************
**** VIDEO.C - Video Hardware Emulation ****
*******************************************/

//-- Include Files -----------------------------------------------------------
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "video.h"
#include "../neocd.h"
#include "2xsai.h"


//-- Defines -----------------------------------------------------------------
#define VIDEO_TEXT		0
#define VIDEO_NORMAL	1
#define	VIDEO_SCANLINES	2

#define	LINE_BEGIN	mydword = *((int *)fspr)
#define LINE_MID	mydword = *((int *)fspr+1)
#define PIXEL_LAST	col = (mydword&0x0F); if (col) *bm = paldata[col]
#define PIXEL_R		col = (mydword&0x0F); if (col) *bm = paldata[col]; bm--
#define PIXEL_F		col = (mydword&0x0F); if (col) *bm = paldata[col]; bm++
#define SHIFT7		mydword >>= 28
#define SHIFT6		mydword >>= 24
#define SHIFT5		mydword >>= 20
#define SHIFT4		mydword >>= 16
#define SHIFT3		mydword >>= 12
#define SHIFT2		mydword >>= 8
#define SHIFT1		mydword >>= 4

//-- Imported Variables ------------------------------------------------------
extern SDL_Surface	*loading_pict;


//-- Global Variables --------------------------------------------------------
char *          video_vidram;
unsigned short	*video_paletteram_ng;
unsigned short	video_palette_bank0_ng[4096];
unsigned short	video_palette_bank1_ng[4096];
unsigned short	*video_paletteram_pc;
unsigned short	video_palette_bank0_pc[4096];
unsigned short	video_palette_bank1_pc[4096];
unsigned short	video_color_lut[32768];
Uint32		video_color_long[0x10000];
Uint32      video_scan_long[0x10000];

short			video_modulo;
unsigned short	video_pointer;
int				video_dos_cx;
int				video_dos_cy;
int				video_mode = 0;
unsigned short	*video_line_ptr[224];
unsigned char	video_fix_usage[4096];
unsigned char   rom_fix_usage[4096];

unsigned char	video_spr_usage[32768];
unsigned char   rom_spr_usage[32768];

SDL_Surface		*screen;
SDL_Surface		*video_buffer;
SDL_Surface		*game_title;
Uint8			*SrcPtr;
Uint8			*DestPtr;
unsigned char	video_shrinky[17];
char			full_y_skip[16]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

unsigned int	neogeo_frame_counter = 0;
unsigned int	neogeo_frame_counter_speed = 4;

unsigned int	video_hide_fps=1;
int				video_scanlines_capable = 1;
double			gamma_correction = 1.0;

int				fullscreen_flag=0;
int				display_mode=1;
int				snap_no;
int				frameskip=0;

SDL_Rect		src,dest;

//-- Function Prototypes -----------------------------------------------------
int		video_init(void);
void	video_shutdown(void);
void	video_draw_line(int);
int		video_set_mode(int);
void	video_mode_toggle(void);
void	video_fullscreen_toggle(void);
void	incframeskip(void);
void	video_precalc_lut(void);
void	video_flip_pages(void);
void	video_draw_spr(unsigned int code, unsigned int color, int flipx,
			int flipy, int sx, int sy, int zx, int zy);
void	video_draw_screen1(void);
void	video_draw_screen2(void);
void	snapshot_init(void);
void	video_save_snapshot(void);
void	video_setup(void);
void	normblit(void);
void	doubleblit(void);
void	scanblit(void);
void	scan50(void);
void	(*blitter)(void);


//----------------------------------------------------------------------------
int	video_init(void)
{
	int		y,i;
	unsigned short	*ptr;

	video_precalc_lut();

	video_vidram = calloc(1, 131072);

	if (video_vidram==NULL) {
		strcpy(global_error, "VIDEO: Could not allocate vidram (128k)");
		return	0;
	}

	memset(video_palette_bank0_ng, 0, 8192);
	memset(video_palette_bank1_ng, 0, 8192);
	memset(video_palette_bank0_pc, 0, 8192);
	memset(video_palette_bank1_pc, 0, 8192);

	video_paletteram_ng = video_palette_bank0_ng;
	video_paletteram_pc = video_palette_bank0_pc;
	video_modulo = 0;
	video_pointer = 0;

        for(i=0;i<32768;i++)
            video_spr_usage[i]=1;

	if (video_set_mode(VIDEO_NORMAL)==0)
		return 0;

	video_buffer = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 226, 16, 0xF800, 0x07E0, 0x001F, 0x0000);
	game_title   = SDL_CreateRGBSurface(SDL_SWSURFACE, 144,  80, 16, 0xF800, 0x07E0, 0x001F, 0x0000);

	ptr = (unsigned short *)(video_buffer->pixels);

	for(y=0;y<224;y++) {
		video_line_ptr[y] = ptr;
		ptr += 320;
	}

	blitter=scanblit;

	//dest={0,7,320,240};
	src.x=8;
	src.y=0;
	src.w=304;
	src.h=224;

	dest.x=166;
	dest.y=128;
	dest.w=304;
	dest.h=224;

	snapshot_init();

	// 2xSaI/Eagle Init
	Init_2xSaI (565);

	return 1;
}

//----------------------------------------------------------------------------
void	video_shutdown(void)
{

	//if (video_buffer != NULL) SDL_FreeSurface( video_buffer );
	//if (loading_pict != NULL) SDL_FreeSurface( loading_pict );
	//if (game_title != NULL)   SDL_FreeSurface( game_title );

	//free(video_vidram);
	
}

//----------------------------------------------------------------------------
int	video_set_mode(int mode)
{
	if ((screen=SDL_SetVideoMode(640, 480, 16, SDL_HWPALETTE|SDL_HWSURFACE))==NULL) {
		fprintf( stderr, "Could not set video mode: %s\n", SDL_GetError() );
        SDL_Quit();
        exit( -1 );
		return 0;
	}

	video_mode = VIDEO_NORMAL;
	
	return 1;
}


void video_fullscreen_toggle(void)
{
	/*SDL_WM_ToggleFullScreen(screen);
	return;*/
	
	if(screen != NULL) SDL_FreeSurface(screen);

	if(fullscreen_flag)
	{
		if(0)
			screen=SDL_SetVideoMode(320, 240, 16, SDL_HWPALETTE);
		else
			screen=SDL_SetVideoMode(640, 480, 16, SDL_HWPALETTE);
		fullscreen_flag=0;
	} else {
		if(0)
			screen=SDL_SetVideoMode(320, 240, 16, SDL_FULLSCREEN|SDL_HWPALETTE);
		else
			screen=SDL_SetVideoMode(640, 480, 16, SDL_FULLSCREEN|SDL_HWPALETTE);
		fullscreen_flag=1;
	}

	if (screen==NULL) {
		fprintf( stderr, "Could not set video mode: %s\n", SDL_GetError() );
        SDL_Quit();
        exit( -1 );
		return;
	}

	//Make sure cursor is still hidden
	SDL_ShowCursor(SDL_DISABLE);

	//clear screen and screen changes
	SDL_FillRect(screen,NULL,0);
	SDL_FillRect(delta,NULL,0);

}

void video_mode_toggle(void)
{
	switch(display_mode)
	{
	case	0:
		blitter=scanblit;
		break;
	case	1:
		blitter=doubleblit;
		break;
	case	2:
		blitter=scan50;
		break;
	case	3:
		blitter=Super2xSaI;
		break;
	case	4:
		blitter=SuperEagle;
		break;
	case	5:
		blitter=normblit;
		break;
	}



	if(++display_mode==6)
		display_mode=0;

	//reset frameskip
	frameskip=0;

	//Clear screen and screen changes
	SDL_FillRect(screen,NULL,0);
	SDL_FillRect(delta,NULL,0);

}


void incframeskip(void)
{
	frameskip++;
    frameskip=frameskip%12;
}
//----------------------------------------------------------------------------
void	video_precalc_lut(void)
{
	int	ndx, rr, rg, rb;
	
	for(rr=0;rr<32;rr++) {
		for(rg=0;rg<32;rg++) {
			for(rb=0;rb<32;rb++) {
				ndx = ((rr&1)<<14)|((rg&1)<<13)|((rb&1)<<12)|((rr&30)<<7)
					|((rg&30)<<3)|((rb&30)>>1);
				video_color_lut[ndx] =
				     ((int)( 31 * pow( (double)rb / 31, 1 / gamma_correction ) )<<0)
					|((int)( 63 * pow( (double)rg / 31, 1 / gamma_correction ) )<<5)
					|((int)( 31 * pow( (double)rr / 31, 1 / gamma_correction ) )<<11);
			}
		}
	}
	
	for(ndx=0;ndx<0x10000;ndx++) {
		video_color_long[ndx]=ndx|(ndx<<16);
		video_scan_long[ndx]=(video_color_long[ndx]>>1)&0x7bef7bef;
	}
	
}

//----------------------------------------------------------------------------
void video_draw_screen1()
{
	static unsigned int	fc;
	static int	pass1_start;
	int			sx =0,sy =0,oy =0,my =0,zx = 1, rzy = 1;
	int			offs,i,count,y;
	int			tileno,tileatr,t1,t2,t3;
	char		fullmode=0;
	int			ddax=0,dday=0,rzx=15,yskip=0;

	if(SDL_MUSTLOCK(video_buffer))
		SDL_LockSurface(video_buffer);

	//fill background colour
	SDL_FillRect(video_buffer,NULL,video_paletteram_pc[4095]);

	if(SDL_MUSTLOCK(video_buffer))
		SDL_UnlockSurface(video_buffer);

	
	t1 = *((unsigned short *)( &video_vidram[0x10400 + 4] ));

	if ((t1 & 0x40) == 0)
	{
		for (pass1_start=6;pass1_start<0x300;pass1_start+=2)
		{
			t1 = *((unsigned short *)( &video_vidram[0x10400 + pass1_start] ));

			if ((t1 & 0x40) == 0)
				break;
		}
		
		if (pass1_start == 6)
			pass1_start = 0;
	}
	else
		pass1_start = 0;	

	for (count=pass1_start;count<0x300;count+=2) {
		t3 = *((unsigned short *)( &video_vidram[0x10000 + count] ));
		t1 = *((unsigned short *)( &video_vidram[0x10400 + count] ));
		t2 = *((unsigned short *)( &video_vidram[0x10800 + count] ));

		// If this bit is set this new column is placed next to last one
		if (t1 & 0x40) {
			sx += (rzx + 1);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			// Get new zoom for this column
			zx = (t3 >> 8)&0x0F;

			sy = oy;
		} else {	// nope it is a new block
			// Sprite scaling
			zx = (t3 >> 8)&0x0F;

			rzy = t3 & 0xff;

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			// Number of tiles in this strip
			my = t1 & 0x3f;
			if (my == 0x20)
				fullmode = 1;
			else if (my >= 0x21)
				fullmode = 2;	// most games use 0x21, but
			else 
				fullmode = 0;	// Alpha Mission II uses 0x3f

			sy = 0x1F0 - (t1 >> 7);
			if (sy > 0x100) sy -= 0x200;
			
			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				while (sy < -16) sy += 2 * (rzy + 1);
			}
			oy = sy;

		  	if(my==0x21) my=0x20;
			else if(rzy!=0xff && my!=0)
				my=((my*16*256)/(rzy+1) + 15)/16;

            		if(my>0x20) my=0x20;

			ddax=0;	// setup x zoom
		}

		rzx = zx;

		// No point doing anything if tile strip is 0
		if ((my==0)||(sx>311))
			continue;

		// Setup y zoom
		if(rzy==255)
			yskip=16;
		else
			dday=0;	// =256; NS990105 mslug fix

		offs = count<<6;

		// my holds the number of tiles in each vertical multisprite block
		for (y=0; y < my ;y++) {
			tileno  = *((unsigned short *)(&video_vidram[offs]));
			offs+=2;
			tileatr = *((unsigned short *)(&video_vidram[offs]));
			offs+=2;

			if (tileatr&0x8)
				tileno = (tileno&~7)|(neogeo_frame_counter&7);
			else if (tileatr&0x4)
				tileno = (tileno&~3)|(neogeo_frame_counter&3);
				
//			tileno &= 0x7FFF;
			if (tileno>0x7FFF)
				continue;

			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				if (sy >= 224) sy -= 2 * (rzy + 1);
			}
			else if (fullmode == 1)
			{
				if (y == 0x10) sy -= 2 * (rzy + 1);
			}
			else if (sy > 0x100) sy -= 0x200;

			if(rzy!=255)
			{
				yskip=0;
				video_shrinky[0]=0;
				for(i=0;i<16;i++)
				{
					video_shrinky[i+1]=0;
					dday-=rzy+1;
					if(dday<=0)
					{
						dday+=256;
						yskip++;
						video_shrinky[yskip]++;
					}
					else
						video_shrinky[yskip]++;
				}
			}

            if (((tileatr>>8)||(tileno!=0))&&(sy<224))
			{
				video_draw_spr(
					tileno,
					tileatr >> 8,
					tileatr & 0x01,tileatr & 0x02,
					sx,sy,rzx,yskip);
			}

			sy +=yskip;
		}  // for y
	}  // for count

	for (count=0;count<pass1_start;count+=2) {
		t3 = *((unsigned short *)( &video_vidram[0x10000 + count] ));
		t1 = *((unsigned short *)( &video_vidram[0x10400 + count] ));
		t2 = *((unsigned short *)( &video_vidram[0x10800 + count] ));

		// If this bit is set this new column is placed next to last one
		if (t1 & 0x40) {
			sx += (rzx + 1);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			// Get new zoom for this column
			zx = (t3 >> 8)&0x0F;

			sy = oy;
		} else {	// nope it is a new block
			// Sprite scaling
			zx = (t3 >> 8)&0x0F;

			rzy = t3 & 0xff;

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			// Number of tiles in this strip
			my = t1 & 0x3f;
			if (my == 0x20)
				fullmode = 1;
			else if (my >= 0x21)
				fullmode = 2;	// most games use 0x21, but
			else 
				fullmode = 0;	// Alpha Mission II uses 0x3f

			sy = 0x1F0 - (t1 >> 7);
			if (sy > 0x100) sy -= 0x200;
			
			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				while (sy < -16) sy += 2 * (rzy + 1);
			}
			oy = sy;

		  	if(my==0x21) my=0x20;
			else if(rzy!=0xff && my!=0)
				my=((my*16*256)/(rzy+1) + 15)/16;

            		if(my>0x20) my=0x20;

			ddax=0;	// setup x zoom
		}

		rzx = zx;

		// No point doing anything if tile strip is 0
		if ((my==0)||(sx>311))
			continue;

		// Setup y zoom
		if(rzy==255)
			yskip=16;
		else
			dday=0;	// =256; NS990105 mslug fix

		offs = count<<6;

		// my holds the number of tiles in each vertical multisprite block
		for (y=0; y < my ;y++) {
			tileno  = *((unsigned short *)(&video_vidram[offs]));
			offs+=2;
			tileatr = *((unsigned short *)(&video_vidram[offs]));
			offs+=2;

			if (tileatr&0x8)
				tileno = (tileno&~7)|(neogeo_frame_counter&7);
			else if (tileatr&0x4)
				tileno = (tileno&~3)|(neogeo_frame_counter&3);
				
//			tileno &= 0x7FFF;
			if (tileno>0x7FFF)
				continue;

			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				if (sy >= 248) sy -= 2 * (rzy + 1);
			}
			else if (fullmode == 1)
			{
				if (y == 0x10) sy -= 2 * (rzy + 1);
			}
			else if (sy > 0x110) sy -= 0x200;

			if(rzy!=255)
			{
				yskip=0;
				video_shrinky[0]=0;
				for(i=0;i<16;i++)
				{
					video_shrinky[i+1]=0;
					dday-=rzy+1;
					if(dday<=0)
					{
						dday+=256;
						yskip++;
						video_shrinky[yskip]++;
					}
					else
						video_shrinky[yskip]++;
				}
			}

            if (((tileatr>>8)||(tileno!=0))&&(sy<224))
			{
				video_draw_spr(
					tileno,
					tileatr >> 8,
					tileatr & 0x01,tileatr & 0x02,
					sx,sy,rzx,yskip);
			}

			sy +=yskip;
		}  // for y
	}  // for count

	if (fc >= neogeo_frame_counter_speed) {
		neogeo_frame_counter++;
		fc=0;
	}

	fc++;

    video_draw_fix();
	(*blitter)();

}

//       Without  flip    	    With Flip
// 01: X0000000 00000000	00000000 0000000X
// 02: X0000000 X0000000	0000000X 0000000X
// 03: X0000X00 00X00000	00000X00 00X0000X
// 04: X000X000 X000X000	000X000X 000X000X
// 05: X00X00X0 0X00X000	000X00X0 0X00X00X
// 06: X0X00X00 X0X00X00	00X00X0X 00X00X0X
// 07: X0X0X0X0 0X0X0X00	00X0X0X0 0X0X0X0X
// 08: X0X0X0X0 X0X0X0X0	0X0X0X0X 0X0X0X0X
// 09: XX0X0X0X X0X0X0X0	0X0X0X0X X0X0X0XX
// 10: XX0XX0X0 XX0XX0X0	0X0XX0XX 0X0XX0XX
// 11: XXX0XX0X X0XX0XX0	0XX0XX0X X0XX0XXX
// 12: XXX0XXX0 XXX0XXX0	0XXX0XXX 0XXX0XXX
// 13: XXXXX0XX XX0XXXX0	0XXXX0XX XX0XXXXX
// 14: XXXXXXX0 XXXXXXX0	0XXXXXXX 0XXXXXXX
// 15: XXXXXXXX XXXXXXX0	0XXXXXXX XXXXXXXX
// 16: XXXXXXXX XXXXXXXX	XXXXXXXX XXXXXXXX

void	video_draw_spr(unsigned int code, unsigned int color, int flipx,
			int flipy, int sx, int sy, int zx, int zy)
{
	int						oy, ey, y, dy;
	unsigned short			*bm;
	int						col;
	int						l;
	int						mydword;
	unsigned char			*fspr = 0;
	char					*l_y_skip;
	const unsigned short	*paldata;


	// Check for total transparency, no need to draw
	//if (video_spr_usage[code] == 0)
    	//	return;

	if (sx <= -8)
		return;

	if(SDL_MUSTLOCK(video_buffer))
		SDL_LockSurface(video_buffer);

   	if(zy == 16)
		 l_y_skip = full_y_skip;
	else
		 l_y_skip = video_shrinky;

	fspr = neogeo_spr_memory;

	// Mish/AJP - Most clipping is done in main loop
	oy = sy;
  	ey = sy + zy -1; 	// Clip for size of zoomed object

	if (sy < 0)
		sy = 0;
	if (ey >= 224)
		ey = 223;

	if (flipy)	// Y flip
	{
		dy = -8;
		fspr += (code+1)*128 - 8 - (sy-oy)*8;
	}
	else		// normal
	{
		dy = 8;
		fspr += code*128 + (sy-oy)*8;
	}

	paldata = &video_paletteram_pc[color*16];
	
	if (flipx)	// X flip
	{
		l=0;
    		switch(zx) {
		case	0:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_MID;
				SHIFT7;
				PIXEL_LAST;
				l++;
			}
			break;
		case	1:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 1;
				LINE_BEGIN;
				SHIFT7;
				PIXEL_R;
				LINE_MID;
				SHIFT7;
				PIXEL_LAST;
				l++;
			}
			break;
		case	2:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 2;
				LINE_BEGIN;
				SHIFT5;
				PIXEL_R;
				LINE_MID;
				SHIFT2;
				PIXEL_R;
				SHIFT5;
				PIXEL_LAST;
				l++;
			}
			break;
		case	3:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 3;
				LINE_BEGIN;
				SHIFT3;
				PIXEL_R;
				SHIFT4;
				PIXEL_R;
				LINE_MID;
				SHIFT3;
				PIXEL_R;
				SHIFT4;
				PIXEL_LAST;
				l++;
			}
			break;
		case	4:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 4;
				LINE_BEGIN;
				SHIFT3;
				PIXEL_R;
				SHIFT3;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT3;
				PIXEL_R;
				SHIFT3;
				PIXEL_LAST;
				l++;
			}
			break;
		case	5:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 5;
				LINE_BEGIN;
				SHIFT2;
				PIXEL_R;
				SHIFT3;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				SHIFT2;
				PIXEL_R;
				SHIFT3;
				PIXEL_R;
				SHIFT2;
				PIXEL_LAST;
				l++;
			}
			break;		
		case	6:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 6;
				LINE_BEGIN;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_LAST;
				l++;
			}
			break;
		case	7:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 7;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_LAST;
				l++;
			}
			break;		
		case	8:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 8;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;
		case	9:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 9;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;		
		case	10:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 10;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;		
		case	11:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 11;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;	
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;	
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;		
		case	12:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 12;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT2;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;		
		case	13:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 13;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;		
		case	14:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 14;
				LINE_BEGIN;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;
		case	15:
			for (y = sy;y <= ey;y++)
			{
				fspr += l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx + 15;
				LINE_BEGIN;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				LINE_MID;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_R;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;		
		}
	}
	else		// normal
	{
		l=0;
    		switch(zx) {
		case	0:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_LAST;
				l++;
			}
			break;
		case	1:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				LINE_MID;
				PIXEL_LAST;
				l++;
			}
			break;
		case	2:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT5;
				PIXEL_F;
				LINE_MID;
				SHIFT2;
				PIXEL_LAST;
				l++;
			}
			break;
		case	3:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT4;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT4;
				PIXEL_LAST;
				l++;
			}
			break;
		case	4:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT3;
				PIXEL_F;
				SHIFT3;
				PIXEL_F;
				LINE_MID;
				SHIFT1;
				PIXEL_F;
				SHIFT3;
				PIXEL_LAST;
				l++;
			}
			break;
		case	5:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT3;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT3;
				PIXEL_LAST;
				l++;
			}
			break;
		case	6:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_LAST;
				l++;
			}
			break;
		case	7:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_LAST;
				l++;
			}
			break;
		case	8:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT2;
				PIXEL_LAST;
				l++;
			}
			break;
		case	9:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_LAST;
				l++;
			}
			break;
		case	10:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;
		case	11:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;	
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;	
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;
		case	12:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT2;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;
		case	13:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;
		case	14:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;
		case	15:
			for (y = sy ;y <= ey;y++)
			{
				fspr+=l_y_skip[l]*dy;
				bm  = (video_line_ptr[y]) + sx;
				LINE_BEGIN;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				LINE_MID;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_F;
				SHIFT1;
				PIXEL_LAST;
				l++;
			}
			break;
		}
	}
	if(SDL_MUSTLOCK(video_buffer))
		SDL_UnlockSurface(video_buffer);
}


void snapshot_init(void)
{
	/* This function sets up snapshot number */
	char name[30];
	FILE *fp;

	snap_no=0;

	sprintf(name,"snap%04d.bmp", snap_no);

	while((fp=fopen(name,"r"))!=NULL)
	{
		fclose(fp);
		snap_no++;
		sprintf(name,"snap%04d.bmp", snap_no);
	}
}




void video_save_snapshot(void)
{
	char name[30];
	sprintf(name,"snap%04d.bmp", snap_no++);
	SDL_SaveBMP(screen,name);
}


//------------------------------------------------------------------------
// Blitters
//------------------------------------------------------------------------


/* 1x centred */
void normblit(void) {
	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);
	SDL_BlitSurface(video_buffer,&src,screen,&dest);
	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}

/* 2x with scanlines */
void scanblit(void)
{

	Uint16 *pixelsrc;
	Uint32 *pixeldest;
	Uint16 x,y;

	pixelsrc =(Uint16 *)video_buffer->pixels+8;
	pixeldest=(Uint32 *)screen->pixels+4480+8;

	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

	for(y=0;y<224;y++) {
		for(x=0;x<304;x++) {
			*pixeldest++=video_color_long[*pixelsrc++];
		}
		pixeldest+=640-304;
		pixelsrc+=16;
	}

	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}


/* 2x scaled */
void doubleblit(void)
{
	Uint16 *pixelsrc;
	Uint32 *pixeldest;
	Uint16 x,y;

	pixelsrc =(Uint16 *)video_buffer->pixels+8;
	pixeldest=(Uint32 *)screen->pixels+4480;

	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

	for(y=0;y<224;y++) {
		for(x=8;x<312;x++) {
			pixeldest[x+320]=pixeldest[x]=video_color_long[*pixelsrc++];
		}
		pixeldest+=640;
		pixelsrc+=16;
	}

	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}


/* 2x with 50% scanlines */
void scan50(void)
{
	Uint16 *pixelsrc;
	Uint32 *pixeldest;
	Uint16 x,y;

	pixelsrc =(Uint16 *)video_buffer->pixels+8;
	pixeldest=(Uint32 *)screen->pixels+4480;

	if(SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

	for(y=0;y<224;y++) {
		for(x=8;x<312;x++) {
			pixeldest[x]=video_color_long[*pixelsrc];
			pixeldest[x+320]=video_scan_long[*pixelsrc];
			pixelsrc++;
		}
		pixeldest+=640;
		pixelsrc+=16;
	}

	if(SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);
	SDL_Flip(screen);
}
