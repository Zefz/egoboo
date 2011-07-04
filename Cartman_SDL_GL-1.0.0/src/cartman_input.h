#pragma once

#include "egoboo_typedef.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_window;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_keyboard;
typedef struct s_keyboard keyboard_t;

struct s_mouse;
typedef struct s_mouse mouse_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MOUSE_PRESSED( BUTTON ) HAS_BITS( mos.b, SDL_BUTTON( BUTTON ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_mouse
{
    bool_t on;

    int   x, y;
    int   x_old, y_old;
    int   b;

    bool_t relative;
    int   cx, cy;

    bool_t            drag, drag_begin;
    struct s_window * drag_window;
    int               drag_mode;
    int               tlx, tly, brx, bry;
};

mouse_t * mouse_ctor( mouse_t * );

//--------------------------------------------------------------------------------------------

struct s_keyboard
{
    bool_t   on;                // Is the keyboard alive?
    int      count;
    int      delay;

    bool_t   needs_update;
    Uint8  * sdlbuffer;
    Uint8    state;
    SDLMod   mod;
};

keyboard_t * keyboard_ctor( keyboard_t * );

//KMOD_NONE No modifiers applicable 
//KMOD_NUM Numlock is down 
//KMOD_CAPS Capslock is down 
//KMOD_LCTRL Left Control is down 
//KMOD_RCTRL Right Control is down 
//KMOD_RSHIFT Right Shift is down 
//KMOD_LSHIFT Left Shift is down 
//KMOD_RALT Right Alt is down 
//KMOD_LALT Left Alt is down 
//KMOD_CTRL A Control key is down 
//KMOD_SHIFT A Shift key is down 
//KMOD_ALT An Alt key is down 

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern mouse_t      mos;
extern keyboard_t   key;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define SDLKEYDOWN(k)  ( (!key.on || (k >= key.count) || (NULL == key.sdlbuffer)) ? bfalse : (0 != key.sdlbuffer[k]))     // Helper for gettin' em
#define SDLKEYMOD(m)   ( key.on && (NULL != key.sdlbuffer) && (0 != (key.state & (m))) )
#define SDLKEYDOWN_MOD(k,m) ( SDLKEYDOWN(k) && (0 != (key.state & (m))) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void   check_input();
bool_t check_keys( Uint32 resolution );
void   update_mouse();