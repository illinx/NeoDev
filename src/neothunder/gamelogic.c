#include <stdlib.h>
#include <video.h>
#include "bullets.h"
#include "enemy.h"
#include "input.h"
#include "screen.h"

// ==== BULLET DEFINITIONS ====

#define FIRST_BULLET_HARDWARE_SPRITE 100
#define MAX_BULLET_HARDWARE_SPRITES 100

// bullet sprites are 100 - 199
//
bullet_t bulletsprites[MAX_BULLET_HARDWARE_SPRITES];

extern TILEMAP bullet[];


// ==== ENEMY DEFINITIONS ====

#define FIRST_OBSTACLE_HARDWARE_SPRITE 220
#define MAX_OBSTACLE_HARDWARE_SPRITES 50

obstacle_t obstaclesprites[MAX_OBSTACLE_HARDWARE_SPRITES];

// this determine when sprites appear on the screen, and at which y coordinate
#define OBSTACLESPR_ENTRY_COUNT 90

// this dictates the maximum frame when we're still spawning an enemy sprite
// when we reach this, we can reset the frame counter to zero, so we cycle back to the
// beginning
// This number is basically the largest number in the enemyspritespawn_frame array
const int maximum_frame_of_spawn_enemy = 2400;

const int enemyspritespawn_frame[OBSTACLESPR_ENTRY_COUNT] = {150, 180, 180, 300, 330, 330, 400, 400, 430, 430, 500, 500, 500, 580, 580, 580, 700, 700, 700, 700, 750, 750, 750, 1000, 1050, 1100, 1200, 1250, 1300, 1500, 1500, 1530, 1560, 1560, 1700, 1700, 1720, 1800, 1800, 1830, 1830, 1950, 1950, 1970, 1970, 1990, 1990, 2100, 2100, 2130, 2130, 2160, 2160,     2350, 2380, 2410, 2440, 2470, 2500,      2800,       3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300,           3350, 3350, 3350, 3350, 3350, 3350, 3350, 3350, 3350, 3350,        3400, 3400, 3400, 3400, 3400, 3400, 3400, 3400, 3400, 3400,};
const int enemyspritespawn_y[OBSTACLESPR_ENTRY_COUNT] = {104, 84, 124, 104, 84, 124, 20, 204, 40, 184, 30, 50, 70, 180, 160, 140, 50, 100, 150, 200, 60, 120, 180, 100, 60, 20, 110, 150, 190, 80, 140, 102, 80, 140, 80, 84, 124, 104, 40, 170, 20, 30, 50, 80, 100, 150, 170, 180, 160, 120, 100, 70, 50, 74, 134, 74, 134, 74, 134, 104,           10, 30, 50, 70, 90, 110, 130, 150, 170, 190,       10, 30, 50, 70, 90, 110, 130, 150, 170, 190,     10, 30, 50, 70, 90, 110, 130, 150, 170, 190};

const int spawnObstacleEveryNFrames = 120;

// enemy ships
extern TILEMAP enemies[];



#define ABS(a) ((a)<0?-(a):a)



// ================ BULLETS ==================


//see if given bullet has hit an enemy
void register_hit_on_enemy(bullet_t bullet, int index)
{
	int i=0;

	for(i=0; i<MAX_OBSTACLE_HARDWARE_SPRITES; i++)
	{
		if(obstaclesprites[i].state != OBSTACLESPR_STATE_ONSCREEN)
		{
			// bullet can only hit onscreen enemies
			continue;
		}
		
		if(ABS(bullet.x - obstaclesprites[i].x) <= 8 &&
		   ABS(bullet.y - obstaclesprites[i].y) <= 8)
		{
			obstaclesprites[i].state = OBSTACLESPR_STATE_EXPLODING;
			//disable_enemy(i);
			// bullet is destroyed
			disable_bullet(index);
			
		}
	}
}

//see if given bullet has hit the player
void register_hit_on_player(bullet_t bullet, int index, int playerx, int playery, int *shield)
{

	playerx += 12;
	
	if(ABS(bullet.x - playerx) <= 9 &&
	   ABS(bullet.y - playery) <= 4)
	{
		// decrease shield
		if(*shield > 0)
			*shield = *shield - 1;

		// bullet is destroyed
		disable_bullet(index);
		
	}

}

void disable_bullet(int which)
{
	bulletsprites[which].state = BULLET_STATE_NOTUSED;
	// move it offscreen somewhere ...
	change_sprite_pos(which+FIRST_BULLET_HARDWARE_SPRITE, -16, -16, 1);
}

void update_bullets(int playerx, int playery, int *shield)
{
	int i=0;

	// calculate new positions for onscreen bullets	
	for(i=0; i<MAX_BULLET_HARDWARE_SPRITES; i++)
	{
		if(bulletsprites[i].state == BULLET_STATE_ONSCREEN)
		{
			bulletsprites[i].x += bulletsprites[i].speedx;
			bulletsprites[i].y += bulletsprites[i].speedy;
	
			// check if this bullet hit an enemy
			if(bulletsprites[i].type == BULLET_TYPE_PLAYER)
			{
				register_hit_on_enemy(bulletsprites[i], i);
			}
			else if(bulletsprites[i].type == BULLET_TYPE_ENEMY)
			{
				register_hit_on_player(bulletsprites[i], i, playerx, playery, shield);
			}
		}
		
		// kill bullet if it is off-screen by at least 10 pixels
		if(bulletsprites[i].x <= -16 || bulletsprites[i].x >= 330 ||
		   bulletsprites[i].y <= -16 || bulletsprites[i].y >= 234)
		{
			disable_bullet(i);
		}
	}	

	// and finally, move all active bullet sprites on screen
	for(i=0; i<MAX_BULLET_HARDWARE_SPRITES; i++)
	{
		if(bulletsprites[i].state == BULLET_STATE_ONSCREEN)
		{
			change_sprite_pos(i+FIRST_BULLET_HARDWARE_SPRITE, bulletsprites[i].x, bulletsprites[i].y, 1);
		}
	}	
}

void fire_new_bullet(int initx, int inity, int speedx, int speedy, int type)
{
	int j;

	//find first unused bullet sprite
	for(j=0; j<MAX_BULLET_HARDWARE_SPRITES; j++)
	{
		if(bulletsprites[j].state == BULLET_STATE_NOTUSED)
		{
			// found one!
			bulletsprites[j].state = BULLET_STATE_ONSCREEN;
			bulletsprites[j].x = initx;
			bulletsprites[j].y = inity;
			bulletsprites[j].tile = 1+type;

			bulletsprites[j].type = type;

			bulletsprites[j].speedx = speedx;
			bulletsprites[j].speedy = speedy;
			
			// write sprite data too
			set_current_sprite(j+FIRST_BULLET_HARDWARE_SPRITE);
			write_sprite_data(initx, inity, 15, 255, 1, 1, (const PTILEMAP)&bullet[bulletsprites[j].tile]);

			// and stop searching, since we found one
			break;
		}
	}
}

void initialize_bullets()
{
	int i;
	
	for(i=0; i<MAX_BULLET_HARDWARE_SPRITES; i++)
	{
		bulletsprites[i].state = BULLET_STATE_NOTUSED;
		bulletsprites[i].x = 500;
		bulletsprites[i].y = 500;
		bulletsprites[i].speedx = 0;
		bulletsprites[i].speedy = 0;
		bulletsprites[i].tile = 0;
		bulletsprites[i].type = BULLET_TYPE_ENEMY;
	}
}

int bullet_sprites_onscreen()
{
	int i, count=0;
	
	for(i=0; i<MAX_BULLET_HARDWARE_SPRITES; i++)
	{
		if(bulletsprites[i].state == BULLET_STATE_ONSCREEN) count++;
	}
	return count;
}



// =================== ENEMY SHIPS =====================




void disable_enemy(int which)
{
	obstaclesprites[which].state = OBSTACLESPR_STATE_NOTUSED;
	// move it offscreen somewhere ...
	change_sprite_pos(which+FIRST_OBSTACLE_HARDWARE_SPRITE, -16, -16, 1);
}

void check_collision_with_player(int i, int playerx, int playery, int *shield)
{
	playerx+=11;
	if(ABS(playerx - obstaclesprites[i].x) <= 16 &&
	   ABS(playery - obstaclesprites[i].y) <= 11)
		{
			//obstaclesprites[i].state = OBSTACLESPR_STATE_EXPLODING;
			//*shield = *shield - 2;
			*shield = 0;	//insta blow up
		}
}

void update_obstacles(int frames, int freeSpace, int playerx, int playery, int *shield, int* playerScore)
{
	int i, j, p;
	//int explosionindex=0;
	
	for(i=0; i<MAX_OBSTACLE_HARDWARE_SPRITES; i++)
	{
		if(obstaclesprites[i].state == OBSTACLESPR_STATE_ONSCREEN)
		{
			// first thing we do is shift all enemy sprites to the left by one pixel
			obstaclesprites[i].x--;
	
			// then we see if any have reached an x of -16, which means they're now fully off-screen and can be
			// freed up (or de-allocated, in a way)		
			if(obstaclesprites[i].x == -16)
			{
				obstaclesprites[i].state = OBSTACLESPR_STATE_NOTUSED;
				// move it offscreen somewhere ...
				change_sprite_pos(i+FIRST_OBSTACLE_HARDWARE_SPRITE, -16, 0, 1);
			}
			else
			{
				//enemy_open_fire(i, playerx, playery);
				check_collision_with_player(i, playerx, playery, shield);

				if (obstaclesprites[i].obstacleType == OBSTACLE_TYPE_CAP_DOWN
					&& obstaclesprites[i].playerHasPassed == 0
					&& obstaclesprites[i].x < playerx)
				{
					obstaclesprites[i].playerHasPassed = 1;
					*playerScore = *playerScore + 1;
				}
			}
		}
	}

	if (frames % spawnObstacleEveryNFrames == 0)
	{
		// empirical value....after 9070 we don't spawn any more enemies, because player has won
		if(_vbl_count <= 9070)
		{
			const int obstacleTopCapPosition = rand() % (SCREEN_HEIGHT - freeSpace - SPRITE_SPACING);
			const int obstacleBottomCapPosition = obstacleTopCapPosition + freeSpace;

			//textoutf(8,9, 0, 0, "y pos: %d", obstacleTopCapPosition);
			//clear_fix();
			//textoutf(8,9, 0, 0, "frames: %d", frames);

			//find first unused enemy sprite, draw top cap
			for(j=0; j<MAX_OBSTACLE_HARDWARE_SPRITES; j++)
			{
				if(obstaclesprites[j].state == OBSTACLESPR_STATE_NOTUSED)
				{
					// found one!
					obstaclesprites[j].state = OBSTACLESPR_STATE_ONSCREEN;
					obstaclesprites[j].x = 319;
					obstaclesprites[j].y = obstacleTopCapPosition;
					//obstaclesprites[j].expframes = 50;
					obstaclesprites[j].obstacleType = OBSTACLE_TYPE_CAP_DOWN;	//top cap points down
					obstaclesprites[j].playerHasPassed = 0;

					// write sprite data too
					set_current_sprite(j+FIRST_OBSTACLE_HARDWARE_SPRITE);
					write_sprite_data(obstaclesprites[j].x, obstaclesprites[j].y, 15, 255, 1, 1, (const PTILEMAP)&enemies[1]);

					// and stop searching, since we found one
					break;
				}
			}

			//find first unused enemy sprite, draw bottom cap
			for(j=0; j<MAX_OBSTACLE_HARDWARE_SPRITES; j++)
			{
				if(obstaclesprites[j].state == OBSTACLESPR_STATE_NOTUSED)
				{
					// found one!
					obstaclesprites[j].state = OBSTACLESPR_STATE_ONSCREEN;
					obstaclesprites[j].x = 319;
					obstaclesprites[j].y = obstacleBottomCapPosition;
					//obstaclesprites[j].expframes = 50;
					obstaclesprites[j].obstacleType = OBSTACLE_TYPE_CAP_UP;	//bottom cap points up
					obstaclesprites[j].playerHasPassed = 0;

					// write sprite data too
					set_current_sprite(j+FIRST_OBSTACLE_HARDWARE_SPRITE);
					write_sprite_data(obstaclesprites[j].x, obstaclesprites[j].y, 15, 255, 1, 1, (const PTILEMAP)&enemies[1]);

					// and stop searching, since we found one
					break;
				}
			}

			//now draw the rest of the pipe from the top cap up
			for (p=obstacleTopCapPosition-SPRITE_SPACING; p > (SCREEN_BORDER_TOP-SPRITE_SPACING); p -=SPRITE_SPACING)
			{
				for(j=0; j<MAX_OBSTACLE_HARDWARE_SPRITES; j++)
				{
					if(obstaclesprites[j].state == OBSTACLESPR_STATE_NOTUSED)
					{
						// found one!
						obstaclesprites[j].state = OBSTACLESPR_STATE_ONSCREEN;
						obstaclesprites[j].x = 319;
						obstaclesprites[j].y = p;
						//obstaclesprites[j].expframes = 50;
						obstaclesprites[j].obstacleType = OBSTACLE_TYPE_REGULAR;	//bottom cap points up
						obstaclesprites[j].playerHasPassed = 0;

						// write sprite data too
						set_current_sprite(j+FIRST_OBSTACLE_HARDWARE_SPRITE);
						write_sprite_data(obstaclesprites[j].x, obstaclesprites[j].y, 15, 255, 1, 1, (const PTILEMAP)&enemies[4]);

						// and stop searching, since we found one
						break;
					}
				}
			}

			//now from the bottom cap down
			//now draw the rest of the pipe from the top cap up
			for (p=obstacleBottomCapPosition+SPRITE_SPACING; p < (SCREEN_BORDER_BOTTOM+SPRITE_SPACING); p +=SPRITE_SPACING)
			{
				for(j=0; j<MAX_OBSTACLE_HARDWARE_SPRITES; j++)
				{
					if(obstaclesprites[j].state == OBSTACLESPR_STATE_NOTUSED)
					{
						// found one!
						obstaclesprites[j].state = OBSTACLESPR_STATE_ONSCREEN;
						obstaclesprites[j].x = 319;
						obstaclesprites[j].y = p;
						//obstaclesprites[j].expframes = 50;
						obstaclesprites[j].obstacleType = OBSTACLE_TYPE_REGULAR;	//bottom cap points up
						obstaclesprites[j].playerHasPassed = 0;

						// write sprite data too
						set_current_sprite(j+FIRST_OBSTACLE_HARDWARE_SPRITE);
						write_sprite_data(obstaclesprites[j].x, obstaclesprites[j].y, 15, 255, 1, 1, (const PTILEMAP)&enemies[4]);

						// and stop searching, since we found one
						break;
					}
				}
			}
		}
	}

	// now we check to see if we're supposed to add new sprites for this frame
// 	for(i=0; i<OBSTACLESPR_ENTRY_COUNT; i++)
// 	{
// 		// empirical value....after 9070 we don't spawn any more enemies, because player has won
// 		if(_vbl_count > 9070) break;
// 		
//  		if(enemyspritespawn_frame[i] == frames)
//  		{
//  			//find first unused enemy sprite
//  			for(j=0; j<MAX_OBSTACLE_HARDWARE_SPRITES; j++)
//  			{
//  				if(obstaclesprites[j].state == OBSTACLESPR_STATE_NOTUSED)
//  				{
//  					// found one!
//  					obstaclesprites[j].state = OBSTACLESPR_STATE_ONSCREEN;
//  					obstaclesprites[j].x = 319;
//  					obstaclesprites[j].y = enemyspritespawn_y[i];
//  					obstaclesprites[j].expframes = 50;
//  					
//  					// write sprite data too
//  					set_current_sprite(j+FIRST_OBSTACLE_HARDWARE_SPRITE);
//  					write_sprite_data(obstaclesprites[j].x, obstaclesprites[j].y, 15, 255, 1, 1, (const PTILEMAP)&enemies[1]);
//  
//  					// and stop searching, since we found one
//  					break;
//  				}
//  			}
//  		}
// 	}

	// and finally, move all active enemy sprites on screen
	for(i=0; i<MAX_OBSTACLE_HARDWARE_SPRITES; i++)
	{
		if(obstaclesprites[i].state == OBSTACLESPR_STATE_ONSCREEN)
		{
			change_sprite_pos(i+FIRST_OBSTACLE_HARDWARE_SPRITE, obstaclesprites[i].x, obstaclesprites[i].y, 1);
		}
	}

	// show explosion for exploding enemies
// 	for(i=0; i<MAX_OBSTACLE_HARDWARE_SPRITES; i++)
// 	{
// 		if(obstaclesprites[i].state == OBSTACLESPR_STATE_EXPLODING)
// 		{
// 			if(obstaclesprites[i].expframes == 0)
// 			{
// 				disable_enemy(i);
// 				continue;
// 			}
// 			if(obstaclesprites[i].expframes < 10) explosionindex = 2;
// 			else if(obstaclesprites[i].expframes >= 10 && obstaclesprites[i].expframes < 20) explosionindex = 3;
// 			else if(obstaclesprites[i].expframes >= 20 && obstaclesprites[i].expframes < 30) explosionindex = 4;
// 			else if(obstaclesprites[i].expframes >= 30 && obstaclesprites[i].expframes < 40) explosionindex = 3;
// 			else if(obstaclesprites[i].expframes >= 40 && obstaclesprites[i].expframes < 50) explosionindex = 2;
// 			set_current_sprite(i+FIRST_OBSTACLE_HARDWARE_SPRITE);
// 			write_sprite_data(obstaclesprites[i].x, obstaclesprites[i].y, 15, 255, 1, 1, (const PTILEMAP)&enemies[explosionindex]);
// 			obstaclesprites[i].expframes--;
// 		}
// 	}
}

// if we're at the right place, fire at the player!
void enemy_open_fire(int which, int playerx, int playery)
{
	int x=0, y=0;
	float slope;

	if(obstaclesprites[which].x == 290 || obstaclesprites[which].x == 220 || obstaclesprites[which].x == 150 )
	{
		//figure out trajectory
		slope = (float)(ABS(obstaclesprites[which].y - playery)) / ( (float)(ABS(obstaclesprites[which].x - playerx)) + 0.1f );

		if(slope >= 0.0f && slope <= 0.3f)
		{
			x = 2; y = 0;
		}
		else if(slope >= 0.3f && slope <= 0.75f)
		{
			x = 2; y = 1;
		}
		else if(slope > 0.75f && slope <= 1.25f)
		{
			x = 2; y = 2;
		}
		else if(slope > 1.25f && slope <= 2.00f)
		{
			x = 1; y = 2;
		}
		else if(slope > 2.00f)
		{
			x = 0; y = 2;
		}

		if(obstaclesprites[which].y > playery)
			y = 0 - y;
		if(obstaclesprites[which].x > playerx)
			x = 0 - x;

		fire_new_bullet(obstaclesprites[which].x - 8, obstaclesprites[which].y+1, x, y, BULLET_TYPE_ENEMY );
	}
}

void initialize_obstacles()
{
	int i;
	
	for(i=0; i<MAX_OBSTACLE_HARDWARE_SPRITES; i++)
	{
		obstaclesprites[i].state = OBSTACLESPR_STATE_NOTUSED;
		obstaclesprites[i].x = 500;
		obstaclesprites[i].y = 500;
		obstaclesprites[i].tile = 0;
		//obstaclesprites[i].expframes = 50;
		obstaclesprites[i].playerHasPassed = 0;
	}
}

int obstacle_sprites_onscreen()
{
	int i, count=0;
	
	for(i=0; i<MAX_OBSTACLE_HARDWARE_SPRITES; i++)
	{
		if(obstaclesprites[i].state == OBSTACLESPR_STATE_ONSCREEN) count++;
	}
	return count;
}

int maximum_frame_enemy_sprite()
{
	return maximum_frame_of_spawn_enemy;
}
