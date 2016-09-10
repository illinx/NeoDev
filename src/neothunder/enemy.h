#ifndef __ENEMY_H__
#define __ENEMY_H__

#include <video.h>


#define OBSTACLESPR_STATE_EXPLODING 2
#define OBSTACLESPR_STATE_ONSCREEN 1
#define OBSTACLESPR_STATE_NOTUSED 0

#define OBSTACLE_TYPE_REGULAR 0
#define OBSTACLE_TYPE_CAP_DOWN 1
#define OBSTACLE_TYPE_CAP_UP 2

typedef struct
{
	// where the sprite is placed on the screen
	int x, y;
	
	// enemy tile number
	int tile;

	// either it's on screen, or it's not used yet
	int state;

	// explosion frames left -- this counts down to 0
	//int expframes;

	//type of obstacle to draw: regular, top cap, or bottom cap
	int obstacleType;

	//has the player flown by us yet?
	int playerHasPassed;
} obstacle_t;

//set all sprites to notused, called once at beginning
void initialize_obstacles();

void update_obstacles(int frames, int freeSpace, int playerx, int playery, int *shield, int* playerScore);

int obstacle_sprites_onscreen();
int maximum_frame_enemy_sprite();

void disable_enemy(int which);
void enemy_open_fire(int which, int playerx, int playery);

#endif