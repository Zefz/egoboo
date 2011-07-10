#pragma once

//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************


#include <egolib.h>

#include "cartman_mpd.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_Font;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_ui_state;
typedef struct s_ui_state ui_state_t;

struct s_window;
typedef struct s_window window_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXWIN 8            // Number of windows

#define DEFAULT_WINDOW_W 200
#define DEFAULT_WINDOW_H 200
#define DEFAULT_RESOLUTION 8

#define SCREEN_TO_REAL(VAL,CAM,ZOOM) ( VAL * (float)DEFAULT_RESOLUTION * (float)TILE_SIZE  / (float)DEFAULT_WINDOW_W / ZOOM + CAM );
#define REAL_TO_SCREEN(VAL,CAM,ZOOM) ( ( VAL - CAM ) / (float)DEFAULT_RESOLUTION / (float)TILE_SIZE * (float)DEFAULT_WINDOW_W * ZOOM  );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_window
{
    oglx_texture_t    tex;      // Window images
    Uint8             on;       // Draw it?
    int               x;        // Window position
    int               y;        //
    int               borderx;  // Window border size
    int               bordery;  //
    int               surfacex; // Window surface size
    int               surfacey; //
    Uint16            mode;     // Window display mode
    int               id;
};

//--------------------------------------------------------------------------------------------
struct s_ui_state
{
    int    cur_x;              // Cursor position
    int    cur_y;              //

    bool_t pressed;                //
    bool_t clicked;                //
    bool_t pending_click;

    SDLX_screen_info_t scr;

    bool_t GrabMouse;
    bool_t HideMouse;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern window_t window_lst[MAXWIN];
extern ui_state_t ui;
extern struct s_Font * gFont_ptr;
extern SDL_Surface * bmpcursor;         // Cursor image

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void do_cursor();
void draw_slider( int tlx, int tly, int brx, int bry, int* pvalue, int minvalue, int maxvalue );
void show_name( const char *newloadname, SDL_Color fnt_color );
void load_window( window_t * pwin, int id, char *loadname, int x, int y, int bx, int by, int sx, int sy, Uint16 mode );
window_t * find_window( int x, int y );
