/**************************************
****   INPUT.C  -  Input devices   ****
**************************************/

/*-- Include Files ---------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include "../neocd.h"


/* Joystick definitions */
#define NUMJOYSTICKS    2
/* axes */
#define NUMAXES         2
#define AXISMIN         0
#define AXISMAX         65536
#define AXISCENTRE      AXISMAX / 2
#define AXISTHRESHOLD   AXISCENTRE / 10   /* 10% joystick threshold */
/* buttons */
#define NUMJOYBUTTONS   10


/*--------------------------------------------------------------------------*/
#define P1UP    0x00000001
#define P1DOWN  0x00000002
#define P1LEFT  0x00000004
#define P1RIGHT 0x00000008
#define P1A     0x00000010
#define P1B     0x00000020
#define P1C     0x00000040
#define P1D     0x00000080

#define P2UP    0x00000100
#define P2DOWN  0x00000200
#define P2LEFT  0x00000400
#define P2RIGHT 0x00000800
#define P2A     0x00001000
#define P2B     0x00002000
#define P2C     0x00004000
#define P2D     0x00008000

#define P1START 0x00010000
#define P1SEL   0x00020000
#define P2START 0x00040000
#define P2SEL   0x00080000

#define SPECIAL 0x01000000


/*--------------------------------------------------------------------------*/
Uint32 keys   =~0;
Uint32 keyup  [SDLK_LAST];
Uint32 keydown[SDLK_LAST];

SDL_Joystick *joystick[NUMJOYSTICKS];
Uint32 joydown[NUMJOYSTICKS][NUMJOYBUTTONS];
Uint32 joyup  [NUMJOYSTICKS][NUMJOYBUTTONS];

Uint32 joymask[NUMJOYSTICKS][NUMAXES][AXISMAX];
Uint32 joyset [NUMJOYSTICKS][NUMAXES][AXISMAX];

            
/*--------------------------------------------------------------------------*/
void input_init(void) {
    int i;
    
    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
    
    /* Player 1 */
    keyup[SDLK_UP]    =P1UP;    
    keyup[SDLK_DOWN]  =P1DOWN;
    keyup[SDLK_LEFT]  =P1LEFT;
    keyup[SDLK_RIGHT] =P1RIGHT;
    keyup[SDLK_LCTRL] =P1A;            joyup[0][0]=P1A;
    keyup[SDLK_LALT]  =P1B;            joyup[0][1]=P1B;
    keyup[SDLK_SPACE] =P1C;            joyup[0][2]=P1C;
    keyup[SDLK_LSHIFT]=P1D;            joyup[0][3]=P1D;
    keyup[SDLK_1]     =P1START;        joyup[0][8]=P1START;
    keyup[SDLK_5]     =P1SEL;          joyup[0][9]=P1SEL;
    
    /* Player 2 */
    keyup[SDLK_r]     =P2UP;
    keyup[SDLK_f]     =P2DOWN;
    keyup[SDLK_d]     =P2LEFT;
    keyup[SDLK_g]     =P2RIGHT;
    keyup[SDLK_a]     =P2A;            joyup[1][0]=P2A;
    keyup[SDLK_s]     =P2B;            joyup[1][1]=P2B;
    keyup[SDLK_q]     =P2C;            joyup[1][2]=P2C;
    keyup[SDLK_w]     =P2D;            joyup[1][3]=P2D;
    keyup[SDLK_2]     =P2START;        joyup[1][8]=P2START;
    keyup[SDLK_6]     =P2SEL;          joyup[1][9]=P2SEL;
    
    /* Special */
    keyup[SDLK_F1]    =SPECIAL;
    keyup[SDLK_F2]    =SPECIAL;
    keyup[SDLK_F3]    =SPECIAL;
    keyup[SDLK_F4]    =SPECIAL;
    keyup[SDLK_F12]   =SPECIAL;
    keyup[SDLK_ESCAPE]=SPECIAL;

    /* set key down mask */
    for(i=0;i<SDLK_LAST;i++) {
        keydown[i]=~keyup[i];
    }
    
    /* set joy button down mask */
    for(i=0; i<NUMJOYBUTTONS; i++) {
        joydown[0][i]=~joyup[0][i];
        joydown[1][i]=~joyup[1][i];
    }
    
    /* configure joystick axes */
    /* left and up */
    for(i=AXISMIN; i<AXISCENTRE-AXISTHRESHOLD; i++) {
        joymask[0][0][i]=~P1LEFT;
        joymask[1][0][i]=~P2LEFT;
        joymask[0][1][i]=~P1UP;
        joymask[1][1][i]=~P2UP;
        joyset[0][0][i]=P1RIGHT;
        joyset[1][0][i]=P2RIGHT;
        joyset[0][1][i]=P1DOWN;
        joyset[1][1][i]=P2DOWN;        
    }
    
    /* centre */
    for(i=AXISCENTRE-AXISTHRESHOLD; i<AXISCENTRE+AXISTHRESHOLD; i++) {
        joymask[0][0][i]=~0;
        joymask[1][0][i]=~0;
        joymask[0][1][i]=~0;
        joymask[1][1][i]=~0;
        joyset[0][0][i]=P1RIGHT|P1LEFT;
        joyset[1][0][i]=P2RIGHT|P2LEFT;
        joyset[0][1][i]=P1DOWN|P1UP;
        joyset[1][1][i]=P2DOWN|P2UP;        
    }
    
    /* right and down */
    for(i=AXISCENTRE+AXISTHRESHOLD; i<AXISMAX; i++) {
        joymask[0][0][i]=~P1RIGHT;
        joymask[1][0][i]=~P2RIGHT;
        joymask[0][1][i]=~P1DOWN;
        joymask[1][1][i]=~P2DOWN;
        joyset[0][0][i]=P1LEFT;
        joyset[1][0][i]=P2LEFT;
        joyset[0][1][i]=P1UP;
        joyset[1][1][i]=P2UP;            
    }
        
    
    /* open joysticks */
    for(i=0; i<SDL_NumJoysticks() && i<NUMJOYSTICKS; i++) {
        joystick[i]=SDL_JoystickOpen(i);
        if(joystick[i]!=NULL) {
            printf("\nOpened Joystick %d\n",i);
            printf("Name: %s\n", SDL_JoystickName(i));
            printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joystick[i]));
            printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joystick[i]));
        }
    }
}


void input_shutdown(void) {
    int i;
    
    /* Close joysticks */
    for(i=0; i<NUMJOYSTICKS; i++) {
        if(joystick[i]!=NULL) {
            SDL_JoystickClose(joystick[i]);    
            joystick[i]=NULL;
        }
    }
}


INLINE void specialKey (SDLKey key) {
    switch(key) {
        case SDLK_F1: video_fullscreen_toggle(); break;
        case SDLK_F2: video_mode_toggle(); break;
        case SDLK_F3: incframeskip(); break;
        case SDLK_F4: sound_toggle(); break;
        case SDLK_F12: video_save_snapshot(); break;
        case SDLK_ESCAPE: exit(0); break;
        default:
            break;
    }
}


INLINE void keyDown (SDLKey key) {
    if(keyup[key]&SPECIAL) {
        specialKey(key);
    } else {
        keys &= keydown[key];
    }
}


INLINE void keyUp (SDLKey key) {
    keys |= keyup[key];
}


INLINE void joyDown (int which, int button) {
    if (which<NUMJOYSTICKS && button<NUMJOYBUTTONS) {
        keys &= joydown[which][button];
    }
}


INLINE void joyUp (int which, int button) {
    if (which<NUMJOYSTICKS && button<NUMJOYBUTTONS) {
        keys |= joyup[which][button];
    }
}


INLINE void joyMotion (int which, int axis, int value) {
    value+=AXISCENTRE;
    if (which<NUMJOYSTICKS && axis <NUMAXES && value<AXISMAX) {
        keys &= joymask[which][axis][value];
        keys |= joyset[which][axis][value];
    }
}


void processEvents(void) {
    SDL_Event event;
        
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN: keyDown(event.key.keysym.sym); break;
            case SDL_KEYUP:   keyUp(event.key.keysym.sym); break;
            
            case SDL_JOYBUTTONDOWN: joyDown(event.jbutton.which, event.jbutton.button); break;
            case SDL_JOYBUTTONUP:   joyUp(event.jbutton.which, event.jbutton.button); break;
            
            case SDL_JOYAXISMOTION: joyMotion(event.jaxis.which, event.jaxis.axis, event.jaxis.value); break;
            
            case SDL_QUIT:    exit(0); break;

            default:
                break;
        }
    }
}
        
/*--------------------------------------------------------------------------*/
unsigned char read_player1(void) {
    return keys&0xff;
}

/*--------------------------------------------------------------------------*/
unsigned char read_player2(void) {
    return (keys>>8)&0xff;
}

/*--------------------------------------------------------------------------*/
unsigned char read_pl12_startsel(void) {
    return (keys>>16)&0x0f;
}


