#include "player.h"
#include "screen.h"

player_t PLAYER_DATA;
joystick_t JOYSTICK_DATA;

void initialize_player()
{
	PLAYER_DATA.playerFlapping = 0;
	PLAYER_DATA.playerWasFlapping = 0;
	PLAYER_DATA.playerYAxisVelocity = 0.0f;
	PLAYER_DATA.playerYAxisAcceleration = 0.0f;
	PLAYER_DATA.playerMoving = 0;
	
	PLAYER_DATA.posX = 0;
	PLAYER_DATA.posY = 100;
}

int is_player_moving()
{
	return PLAYER_DATA.playerMoving;
}

void update_player()
{
	//static const float fPlayerMass = 10.0f;

	PLAYER_DATA.playerMoving = 0;
	PLAYER_DATA.playerWasFlapping = PLAYER_DATA.playerFlapping;
	PLAYER_DATA.playerFlapping = 0;


	update_controls();

	// 		if (i & JOY_UP)
	// 		{
	// 			if(y>-8)
	// 				y-=playerspeed;
	// 			playermoving = 1;
	// 		}
	// 		
	// 		if (i & JOY_DOWN)
	// 		{
	// 			if(y<216)
	// 				y+=playerspeed;
	// 			playermoving = 1;
	// 		}
	// 		
	// 		if (i & JOY_LEFT)
	// 		{
	// 			if(x>-24)
	// 				x-=playerspeed;
	// 			playermoving = 1;
	// 		}
	// 		
	// 		if (i & JOY_RIGHT)
	// 		{
	// 			if(x<304)
	// 				x+=playerspeed;
	// 			playermoving = 1;
	// 		}

	if (JOYSTICK_DATA.i & JOY_START)
	{
		PLAYER_DATA.posX = PLAYER_DATA.posY = 0;
	}

	//i = poll_joystick(PORT1, READ_BIOS);

	// 		if (i & JOY_A)
	// 		{
	// 			fire_new_bullet(x+24, y+2, 5, 0, BULLET_TYPE_PLAYER );
	// 		}

	//are we flapping?
	if (JOYSTICK_DATA.i & JOY_A)
	{
		PLAYER_DATA.playerFlapping = 1;
	}

	//physics update
	PLAYER_DATA.playerYAxisAcceleration = (fGravity * fGravityInvert);

	//integrate
	PLAYER_DATA.playerYAxisVelocity += PLAYER_DATA.playerYAxisAcceleration;
	if (PLAYER_DATA.playerFlapping && !PLAYER_DATA.playerWasFlapping)
	{
		PLAYER_DATA.playerYAxisVelocity = (fFlapVelocity * fGravityInvert);
		// 			playerYAxisVelocity += fFlapVelocity;
		// 			if (playerYAxisVelocity < fFlapVelocity)
		// 			{
		// 				playerYAxisVelocity = fFlapVelocity;
		// 			}
	}
	PLAYER_DATA.playerYAxisVelocity *= fAirDrag;
	PLAYER_DATA.posY += PLAYER_DATA.playerYAxisVelocity;
	//y += (playerYAxisVelocity * fAirDrag);

	//clamp to top of screen, don't kill
	if (PLAYER_DATA.posY <= SCREEN_BORDER_TOP)
	{
		PLAYER_DATA.posY = SCREEN_BORDER_TOP;
	}

	if (PLAYER_DATA.playerYAxisVelocity <= 0.0f)
	{
		PLAYER_DATA.playerMoving = 1;
	}
}

void update_controls()
{
	JOYSTICK_DATA.i = poll_joystick(PORT1, READ_BIOS);
	//JOYSTICK_DATA.joy2 = JOYSTICK_DATA.i;
}

DWORD get_controls_direct()
{
	return poll_joystick(PORT1, READ_DIRECT);
}

int get_player_pos_x()
{
	return PLAYER_DATA.posX;
}

int get_player_pos_y()
{
	return PLAYER_DATA.posY;
}