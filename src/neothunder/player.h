#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <input.h>

static const float fAirDrag = 0.99f;
static const float fWaterDrag = 0.75f;
static const float fGravity = 0.2f;
static const float fFlapVelocity = -3.0f;
static const float fGravityInvert = 1.0f;

typedef struct
{
	int playerMoving;
	int playerFlapping;
	int playerWasFlapping;
	int posX;
	int posY;
	float playerYAxisVelocity;
	float playerYAxisAcceleration;
} player_t;

typedef struct  
{
	// used to read in the joystick
	DWORD i;
	//DWORD joy2;
} joystick_t;

int is_player_moving();
void initialize_player();
void update_player();
void update_controls();
int get_player_pos_x();
int get_player_pos_y();
DWORD get_controls_direct();

#endif //__PLAYER_H__