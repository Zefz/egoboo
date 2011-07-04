#pragma once

#include "egoboo_typedef.h"

#include "cartman_mpd.h"

#include "SDL_extensions.h"
#include "ogl_texture.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct Font;

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
    glTexture         tex;      // Window images
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
extern struct Font * gFont;
extern SDL_Surface * bmpcursor;         // Cursor image

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void do_cursor();
void draw_slider( int tlx, int tly, int brx, int bry, int* pvalue, int minvalue, int maxvalue );
void show_name( const char *newloadname );
void load_window( window_t * pwin, int id, char *loadname, int x, int y, int bx, int by, int sx, int sy, Uint16 mode );
window_t * find_window( int x, int y );
