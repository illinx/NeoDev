/*

		NEO THUNDER
			
					by Sebastian Mihai, 2011

		After working with the low-capabilities Gameboy, and the horrible lcc compiler,
		man, was this a breeze of fresh air.

		Sleek compiler and libraries, a powerhouse of a cpu, everything worked nicely!
		My approach was a bit different with this one. Instead of working out a nice engine and then
		deciding to move on to a different console (like I did with Burly Bear vs. The Mean Foxes rather than balancing the game, 
		I've written this one as quickly as I could and then spent time to polish the gameplay.

		If you are finding this game too hard, I've done my job right. It should be crazy hard... with
		a million bullets on the screen...

		You may find the code a bit hard to read, and it surely is messy. Email me if you need help understanding it. My website is

											http://www.sebastianmihai.com

		(operational as of right now, February 2011). You can find a way to contact me there.
		I would be thrilled if this code was used as a basis for your own Neogeo project. That was half of my purpose, to be
		able to provide a nice starter game, along with a development environment for anyone to pick up and improve.

		Send me your homebrew games!

		Long live the Neogeo!
*/

#include <stdlib.h>
#include <video.h>
#include <input.h>
#include "background.h"
#include "bullets.h"
#include "enemy.h"
#include "player.h"
#include "screen.h"

// pallete information
extern PALETTE	palettes[];

extern TILEMAP	playership[];

int lastscore;

void render_playership(int x, int y, int playermoving)
{
	set_current_sprite(201);
	write_sprite_data(x+16, y, 15, 255, 1, 1, (const PTILEMAP)&playership[3]);		

	set_current_sprite(200);
	if(playermoving == 1)
	{
// 		if (joy2 & JOY_LEFT)
// 		{
// 			write_sprite_data(x, y, 15, 255, 1, 1, (const PTILEMAP)&playership[0]);
// 			set_current_sprite(201);
// 			if(_vbl_count % 2 == 0)
// 				write_sprite_data(x+16, y, 15, 255, 1, 1, (const PTILEMAP)&playership[3]);
// 			else
// 				write_sprite_data(x+16, y, 15, 255, 1, 1, (const PTILEMAP)&playership[4]);
// 		}
// 		else
		{
			if(_vbl_count % 2 == 0)
				write_sprite_data(x, y, 15, 255, 1, 1, (const PTILEMAP)&playership[2]);
			else
				write_sprite_data(x, y, 15, 255, 1, 1, (const PTILEMAP)&playership[0]);
		}
	}
	else
	{
		
		if(_vbl_count % 2 == 0)
			write_sprite_data(x, y, 15, 255, 1, 1, (const PTILEMAP)&playership[1]);
		else
			write_sprite_data(x, y, 15, 255, 1, 1, (const PTILEMAP)&playership[0]);
	}

	//change_sprite_pos(200, x, y, 1);

	change_sprite_pos(201, x+16, y, 1);
}

void menu()
{
	DWORD control_state = 0;

	setpalette(0, 2, (const PPALETTE)&palettes);
	clear_fix();
	clear_spr();

	_vbl_count = 0;

	do
	{
		control_state = get_controls_direct();		
		textoutf(13,6, 0, 0, "FLAPPY FISH");
		textoutf(8,9, 0, 0, "a game by Jonathan Martin");
		textoutf(10, 12, 0, 0, "and Angele Poisson!");
		textoutf(11,15, 0, 0, "Press A or B to start!");

// 			if(lastscore >= 31)
// 				textoutf(0,27, 0, 0, "Last attempt: COMPLETED GAME!!");
// 			else
			textoutf(0,27, 0, 0, "Last attempt: %d fishes traveled", lastscore);

		wait_vbl();

		set_current_sprite(379);
		if(_vbl_count % 2 == 0)
			write_sprite_data(132, 160, 15, 255, 1, 1, (const PTILEMAP)&playership[0]);
		else
			write_sprite_data(132, 160, 15, 255, 1, 1, (const PTILEMAP)&playership[2]);
		set_current_sprite(380);
		write_sprite_data(148, 160, 15, 255, 1, 1, (const PTILEMAP)&playership[3]);

	}while(!(control_state & (JOY_A | JOY_B)));

	srand(_vbl_count);

	_vbl_count = 0;
	clear_fix();
	clear_spr();
}

void get_ready()
{
		setpalette(0, 2, (const PPALETTE)&palettes);
		clear_fix();
		clear_spr();
		_vbl_count = 0;
		do
		{
			textoutf(13,12, 0, 0, "Get ready!");
			textoutf(17,13, 0, 0, "%d", 5 - _vbl_count/40);
			wait_vbl();

		}while(_vbl_count < 200);

		_vbl_count = 0;
		clear_fix();
		clear_spr();
}

void game_over()
{
	setpalette(0, 2, (const PPALETTE)&palettes);
	clear_fix();
	clear_spr();

	srand(_vbl_count);

	_vbl_count = 0;
	do
	{
		textoutf(13,12, 0, 0, "Game over");
		textoutf(6,16, 0, 0, "You traveled %d fish years", lastscore);
		textoutf(9,17, 0, 0, "before you got owned");
		wait_vbl();

	}while(_vbl_count < 300 && ((get_controls_direct() & JOY_A) == 0));

	_vbl_count = 0;
	clear_fix();
	clear_spr();
}

void game()
{
	int playershield = 1;	
	int current_background_frame = 0;
	int current_obstacle_frame = 0;
	//int playermoving = 0;
	//int playerspeed = 1;
	int stage = 1;
	int playerX = 0;
	int playerY = 0;

	const int freeSpaceAllowedMax = 96;
	const int freeSpaceAllowedMin = 32;
	const int freeSpaceAllowedMinusPerStage = 16;
	int freeSpaceAllowed = freeSpaceAllowedMax;

	initialize_player();

	//char c = 177;
	//int count = 0;
	lastscore = 0;
	
	// Copy our palette into video hardware, palette no 1
	setpalette(0, 2, (const PPALETTE)&palettes);

	//==============

	set_current_sprite(200);
	write_sprite_data(0, 100, 15, 255, 1, 1, (const PTILEMAP)&playership[2]);
	set_current_sprite(201);
	write_sprite_data(0, 100, 15, 255, 1, 1, (const PTILEMAP)&playership[3]);

	//rand_init();
	
	initialize_background();
	//initialize_bullets();
	initialize_obstacles();

	while(1)
	{

		wait_vbl();

		update_player();
		playerX = get_player_pos_x();
		playerY = get_player_pos_y();

		render_playership(playerX, playerY, is_player_moving());

		// increment background frame
		current_background_frame++;
		if(current_background_frame > maximum_frame_background_sprite()) current_background_frame=0;
		 
		update_background(current_background_frame);
		//update_bullets(x, y, &playershield);

		current_obstacle_frame+=1;	//stage
		if(current_obstacle_frame > maximum_frame_enemy_sprite())
		{
			current_obstacle_frame=1;
			stage++;
			freeSpaceAllowed -= freeSpaceAllowedMinusPerStage;
			freeSpaceAllowed = max(freeSpaceAllowed, freeSpaceAllowedMin);
		}
		update_obstacles(current_obstacle_frame, freeSpaceAllowed, playerX, playerY, &playershield, &lastscore);

		// output stats
		textoutf(18,27, 0, 0, "stage: %d", stage);
		//textoutf(0,25, 0, 0, "VBL:%03d bgframe:%03d bgspr:%03d ", _vbl_count, current_background_frame, background_sprites_onscreen());
		//textoutf(0,26, 0, 0, "bullets:%03d enemies:%03d shield:%d", bullet_sprites_onscreen(), enemy_sprites_onscreen(), playershield);
// 		textoutf(0,27, 0, 0, "SHIELD: ");
// 		for(count=0;count<playershield;count++)textoutf(8+count,27, 0, 0, "%c", c);
// 		for(count=playershield;count<10;count++)textoutf(8+count,27, 0, 0, " ");
		//lastscore = _vbl_count/300;
		textoutf(0,27, 0, 0, "SCORE: %d", lastscore);

		// lose?
		if (playerY > SCREEN_BORDER_BOTTOM)
		{
			playershield = 0;
		}

		if(playershield <= 0) 
		{
			return;
		}

// 		if( lastscore >= 32 )
// 		{
// 			// win!
// 			textoutf(5,3, 0, 0, "Grats, you beat the game!");
// 			// keep writing on screen for a bit longer
// 			if(lastscore == 33)
// 			{
// 				return;
// 			}
// 		}
	}
	
	
	return;
}

int	main(void)
{	
	//char* author = "Sebastian Mihai, 2011";
	while(1)
	{
		menu();
		get_ready();
		game();
		//if(lastscore < 32)
			game_over();
	}
	
	return 0;
}
