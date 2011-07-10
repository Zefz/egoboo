#include "cartman.h"

#include "cartman_mpd.h"
#include "cartman_functions.h"
#include "cartman_input.h"
#include "cartman_gui.h"
#include "cartman_gfx.h"
#include "cartman_math.inl"

#include <egolib.h>

#include <stdio.h>          // For printf and such
#include <fcntl.h>          // For fast file i/o
#include <math.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_cart_mouse_data;
typedef struct s_cart_mouse_data cart_mouse_data_t;

struct s_light;
typedef struct s_light light_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_light
{
    int           x;
    int           y;
    Uint8 level;
    int           radius;
};

//--------------------------------------------------------------------------------------------
struct s_cart_mouse_data
{
    // click/drag window
    int     win_id;
    Uint16  win_mode;

    // click location
    float   xpos, ypos;
    int     onfan;
    int     xfan, yfan;

    // click data
    Uint8   type;       // Tile fantype
    Uint8   fx;         // Tile effects
    Uint8   tx;         // Tile texture
    Uint8   upper;      // Tile upper bits
    Uint16  presser;    // Random add for tiles

    // Rectangle drawing
    int     rect_draw;   // draw it
    int     rect_drag;   // which window id
    int     rect_done;   // which window id
    float   rect_x0;     //
    float   rect_y0;     //
    float   rect_x1;     //
    float   rect_y1;     //
};

static cart_mouse_data_t * cart_mouse_data_ctor( cart_mouse_data_t * );
static void cart_mouse_data_toggle_fx( int fxmask );

// helper functions
static void cart_mouse_data_mesh_set_tile( Uint16 tiletoset );
static void cart_mouse_data_flatten_mesh();
static void cart_mouse_data_clear_mesh();
static void cart_mouse_data_three_e_mesh();
static void cart_mouse_data_mesh_replace_tile( bool_t tx_only, bool_t at_floor_level );
static void cart_mouse_data_mesh_set_fx();
static void cart_mouse_data_rect_select();
static void cart_mouse_data_rect_unselect();
static void cart_mouse_data_mesh_replace_fx();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

STRING egoboo_path = { "" };

int     onscreen_count = 0;
Uint32  onscreen_vert[MAXPOINTS];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static cart_mouse_data_t mdata = { -1 };

static float cartman_zoom_hrz = 1.0f;
static float cartman_zoom_vrt = 1.0f;

static STRING  loadname;        // Text

static int     brushsize = 3;      // Size of raise/lower terrain brush
static int     brushamount = 50;   // Amount of raise/lower

static float   debugx = -1;        // Blargh
static float   debugy = -1;        //

static bool_t addinglight = bfalse;

static int numlight;
static light_t light_lst[MAXLIGHT];

static int ambi = 22;
static int ambicut = 1;
static int direct = 16;

static bool_t _sdl_atexit_registered = bfalse;
static bool_t _ttf_atexit_registered = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// ui functions
static void draw_cursor_in_window( window_t * pwin );
static void unbound_mouse();
static void bound_mouse();
static bool_t cartman_check_keys( const char *modname );
static bool_t cartman_check_mouse( const char *modname );
static void   cartman_check_input( const char *modname );

// loading
static void load_module( const char *modname );
static void load_basic_textures( const char *modname );
void cartman_create_mesh( void );

// saving
static void cartman_save_mesh( const char *modname );

// gfx functions
static void load_all_windows( void );

static void render_tile_window( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void render_fx_window( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void render_vertex_window( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void render_side_window( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void render_window( window_t * pwin );
static void render_all_windows( void );

static void draw_window( window_t * pwin );
static void draw_all_windows( void );
static void draw_lotsa_stuff( void );

static void draw_main( void );

// camera stuff
static void move_camera();
static void bound_camera( void );

// misc
static void mesh_calc_vrta();
static void fan_calc_vrta( int fan );
static int  vertex_calc_vrta( Uint32 vert );
static void make_onscreen();
static void onscreen_add_fan( Uint32 fan );
static void ease_up_mesh( float zoom_vrt );

// cartman versions of these functions
static int cartman_get_vertex( int x, int y, int num );

// vertex selection functions
void select_add_rect();
void select_remove_rect();

// light functions
static void add_light( int x, int y, int radius, int level );
static void alter_light( int x, int y );
static void draw_light( int number, window_t * pwin, float zoom_hrz );

static void cartman_check_mouse_side( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void cartman_check_mouse_tile( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void cartman_check_mouse_fx( window_t * pwin, float zoom_hrz, float zoom_vrt );
static void cartman_check_mouse_vertex( window_t * pwin, float zoom_hrz, float zoom_vrt );

// vfs support functions
void setup_egoboo_paths_vfs();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "standard.c"           // Some functions that I always use

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void add_light( int x, int y, int radius, int level )
{
    if ( numlight >= MAXLIGHT )  numlight = MAXLIGHT - 1;

    light_lst[numlight].x = x;
    light_lst[numlight].y = y;
    light_lst[numlight].radius = radius;
    light_lst[numlight].level = level;
    numlight++;
}

//--------------------------------------------------------------------------------------------
void alter_light( int x, int y )
{
    int radius, level;

    numlight--;
    if ( numlight < 0 )  numlight = 0;

    level = abs( light_lst[numlight].y - y );

    radius = abs( light_lst[numlight].x - x );
    if ( radius > MAXRADIUS / cartman_zoom_hrz )  radius = MAXRADIUS / cartman_zoom_hrz;
    if ( radius < MINRADIUS / cartman_zoom_hrz )  radius = MINRADIUS / cartman_zoom_hrz;

    light_lst[numlight].radius = radius;
    if ( level > MAP_MAXLEVEL ) level = MAP_MAXLEVEL;
    if ( level < MAP_MINLEVEL ) level = MAP_MINLEVEL;
    light_lst[numlight].level = level;

    numlight++;
}

//--------------------------------------------------------------------------------------------
void draw_light( int number, window_t * pwin, float zoom_hrz )
{
    int xdraw, ydraw, radius;
    Uint8 color;

    xdraw = ( light_lst[number].x / FOURNUM * zoom_hrz ) - cam.x + ( pwin->surfacex >> 1 ) - SMALLXY;
    ydraw = ( light_lst[number].y / FOURNUM * zoom_hrz ) - cam.y + ( pwin->surfacey >> 1 ) - SMALLXY;
    radius = abs( light_lst[number].radius ) / FOURNUM * zoom_hrz;
    color = light_lst[number].level >> 3;

    //color = MAKE_BGR(pwin->bmp, color, color, color);
    //circle(pwin->bmp, xdraw, ydraw, radius, color);
}

//--------------------------------------------------------------------------------------------
void draw_cursor_in_window( window_t * pwin )
{
    int x, y;

    if ( NULL == pwin || !pwin->on ) return;

    if ( -1 != mdata.win_id && pwin->id != mdata.win_id )
    {
        int size = POINT_SIZE( 10 );

        x = pwin->x + ( mos.x - window_lst[mdata.win_id].x );
        y = pwin->y + ( mos.y - window_lst[mdata.win_id].y );

        ogl_draw_sprite_2d( &tx_pointon, x - size / 2, y - size / 2, size, size );
    }

}

//--------------------------------------------------------------------------------------------
int cartman_get_vertex( int x, int y, int num )
{
    int vert = get_vertex( x, y, num );

    if ( vert == -1 )
    {
        return vert;
        printf( "BAD GET_VERTEX NUMBER(2nd), %d at %d, %d...\n", num, x, y );
        exit( -1 );
    }

    return vert;
}

//--------------------------------------------------------------------------------------------
void onscreen_add_fan( Uint32 fan )
{
    // ZZ> This function flags a fan's points as being "onscreen"
    int cnt;
    Uint32 vert, fan_type, vert_count;

    fan_type    = mesh.fantype[fan];
    vert_count  = mesh.command[fan_type].numvertices;

    for ( cnt = 0, vert = mesh.vrtstart[fan];
          cnt < vert_count && CHAINEND != vert;
          cnt++, vert = mesh.vrtnext[vert] )
    {
        if ( vert >= MAXTOTALMESHVERTICES ) break;
        if ( onscreen_count >= MAXPOINTS ) break;

        if ( VERTEXUNUSED == mesh.vrta[vert] ) continue;

        onscreen_vert[onscreen_count] = vert;
        onscreen_count++;
    }
}

//--------------------------------------------------------------------------------------------
void make_onscreen()
{
    int mapx, mapy;
    int mapxstt, mapystt;
    int mapxend, mapyend;
    int fan;

    mapxstt = floor(( cam.x - cam.w  * 0.5f ) / TILE_SIZE ) - 1.0f;
    mapystt = floor(( cam.y - cam.h  * 0.5f ) / TILE_SIZE ) - 1.0f;

    mapxend = ceil(( cam.x + cam.w  * 0.5f ) / TILE_SIZE ) + 1.0f;
    mapyend = ceil(( cam.y + cam.h  * 0.5f ) / TILE_SIZE ) + 1.0f;

    onscreen_count = 0;
    for ( mapy = mapystt; mapy <= mapyend; mapy++ )
    {
        if ( mapy < 0 || mapy >= mesh.tiles_y ) continue;

        for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
        {
            if ( mapx < 0 || mapx >= mesh.tiles_x ) continue;

            fan = mesh_get_fan( mapx, mapy );
            if ( -1 == fan ) continue;

            onscreen_add_fan( fan );
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_basic_textures( const char *modname )
{
    // ZZ> This function loads the standard textures for a module
    STRING newloadname;
    SDL_Surface *bmptemp;       // A temporary bitmap

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "tile0.bmp", newloadname );
    bmptemp = cartman_LoadIMG( newloadname );
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "tile1.bmp", newloadname );
    bmptemp = cartman_LoadIMG( newloadname );
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "tile2.bmp", newloadname );
    bmptemp = cartman_LoadIMG( newloadname );
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "tile3.bmp", newloadname );
    bmptemp = cartman_LoadIMG( newloadname );
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );
}

//--------------------------------------------------------------------------------------------
void load_module( const char *modname )
{
    STRING mod_path;

    sprintf( mod_path, "%s" SLASH_STR "modules" SLASH_STR "%s", egoboo_path, modname );

    //  show_name(mod_path);
    load_basic_textures( mod_path );
    if ( !load_mesh( mod_path ) )
    {
        cartman_create_mesh();
    }

    //read_wawalite( mod_path );

    numlight = 0;
    addinglight = 0;
}

//--------------------------------------------------------------------------------------------
void render_tile_window( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    oglx_texture_t * tx_tile;
    float x, y;
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera_hrz( pwin, &cam, zoom_hrz, zoom_hrz );
        {
            mapxstt = floor(( cam.x - cam.w  * 0.5f ) / TILE_SIZE ) - 1.0f;
            mapystt = floor(( cam.y - cam.h  * 0.5f ) / TILE_SIZE ) - 1.0f;

            mapxend = ceil(( cam.x + cam.w  * 0.5f ) / TILE_SIZE ) + 1.0f;
            mapyend = ceil(( cam.y + cam.h  * 0.5f ) / TILE_SIZE ) + 1.0f;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                if ( mapy < 0 || mapy >= mesh.tiles_y ) continue;
                y = mapy * TILE_SIZE;

                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    if ( mapx < 0 || mapx >= mesh.tiles_x ) continue;
                    x = mapx * TILE_SIZE;

                    fan     = fan_at( mapx, mapy );

                    tx_tile = NULL;
                    if ( -1 != fan )
                    {
                        tx_tile = tile_at( fan );
                    }

                    tx_tile = NULL;
                    if ( -1 != fan )
                    {
                        tx_tile = tile_at( fan );
                    }

                    if ( NULL != tx_tile )
                    {
                        draw_top_tile( x, y, fan, tx_tile, bfalse );
                    }
                }
            }
        }
        cartman_end_ortho_camera();

        // force OpenGL to execute these commands
        glFlush();

        //cnt = 0;
        //while (cnt < numlight)
        //{
        //    draw_light(cnt, pwin);
        //    cnt++;
        //}
    }
    glPopAttrib();

}

//--------------------------------------------------------------------------------------------
void render_fx_window( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    oglx_texture_t * tx_tile;
    float x, y;
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;
    float zoom_fx;

    zoom_fx = ( zoom_hrz < 1.0f ) ? zoom_hrz : 1.0f;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera_hrz( pwin, &cam, zoom_hrz, zoom_hrz );
        {
            mapxstt = floor(( cam.x - cam.w  * 0.5f ) / TILE_SIZE ) - 1.0f;
            mapystt = floor(( cam.y - cam.h  * 0.5f ) / TILE_SIZE ) - 1.0f;

            mapxend = ceil(( cam.x + cam.w  * 0.5f ) / TILE_SIZE ) + 1.0f;
            mapyend = ceil(( cam.y + cam.h  * 0.5f ) / TILE_SIZE ) + 1.0f;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                if ( mapy < 0 || mapy >= mesh.tiles_y ) continue;
                y = mapy * TILE_SIZE;

                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    if ( mapx < 0 || mapx >= mesh.tiles_x ) continue;
                    x = mapx * TILE_SIZE;

                    fan     = fan_at( mapx, mapy );

                    tx_tile = NULL;
                    if ( -1 != fan )
                    {
                        tx_tile = tile_at( fan );
                    }

                    if ( NULL != tx_tile )
                    {
                        ogl_draw_sprite_2d( tx_tile, x, y, TILE_SIZE, TILE_SIZE );

                        // water is whole tile
                        draw_tile_fx( x, y, mesh.fx[fan], 4.0f * zoom_fx );
                    }
                }
            }
        }
        cartman_end_ortho_camera();

    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void render_vertex_window( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera_hrz( pwin, &cam, zoom_hrz, zoom_hrz );
        {
            mapxstt = floor(( cam.x - cam.w * 0.5f ) / TILE_SIZE ) - 1.0f;
            mapystt = floor(( cam.y - cam.h * 0.5f ) / TILE_SIZE ) - 1.0f;

            mapxend = ceil(( cam.x + cam.w * 0.5f ) / TILE_SIZE ) + 1;
            mapyend = ceil(( cam.y + cam.h * 0.5f ) / TILE_SIZE ) + 1;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                if ( mapy < 0 || mapy >= mesh.tiles_y ) continue;

                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    if ( mapx < 0 || mapx >= mesh.tiles_x ) continue;

                    fan = mesh_get_fan( mapx, mapy );
                    if ( -1 != fan )
                    {
                        draw_top_fan( pwin, fan, zoom_hrz );
                    }
                }
            }

            if ( mdata.rect_draw && pwin->id == mdata.rect_drag )
            {
                float color[4];
                float x_min, x_max;
                float y_min, y_max;

                OGL_MAKE_COLOR_4( color, 0x3F, 16 + ( timclock&15 ), 16 + ( timclock&15 ), 0 );

                x_min = mdata.rect_x0;
                x_max = mdata.rect_x1;
                if ( x_min > x_max ) { float tmp = x_max; x_max = x_min; x_min = tmp; }

                y_min = mdata.rect_y0;
                y_max = mdata.rect_y1;
                if ( y_min > y_max ) { float tmp = y_max; y_max = y_min; y_min = tmp; }

                ogl_draw_box( x_min, y_min, x_max - x_min, y_max - y_min, color );
            }
        }
        cartman_end_ortho_camera();

        // force OpenGL to execute these commands
        glFlush();
    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void render_side_window( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera_vrt( pwin, &cam, zoom_hrz, zoom_vrt / 3.0f );
        {
            mapxstt = floor(( cam.x - cam.w * 0.5f ) / TILE_SIZE ) - 1.0f;
            mapystt = floor(( cam.y - cam.h * 0.5f ) / TILE_SIZE ) - 1.0f;

            mapxend = ceil(( cam.x + cam.w * 0.5f ) / TILE_SIZE ) + 1;
            mapyend = ceil(( cam.y + cam.h * 0.5f ) / TILE_SIZE ) + 1;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                if ( mapy < 0 || mapy >= mesh.tiles_y ) continue;

                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    if ( mapx < 0 || mapx >= mesh.tiles_x ) continue;

                    fan = mesh_get_fan( mapx, mapy );
                    if ( -1 == fan ) continue;

                    draw_side_fan( pwin, fan, zoom_hrz, zoom_vrt );
                }
            }

            if ( mdata.rect_draw && pwin->id == mdata.rect_drag )
            {
                float color[4];
                float x_min, x_max;
                float y_min, y_max;

                OGL_MAKE_COLOR_4( color, 0x3F, 16 + ( timclock&15 ), 16 + ( timclock&15 ), 0 );

                x_min = mdata.rect_x0;
                x_max = mdata.rect_x1;
                if ( x_min > x_max ) { float tmp = x_max; x_max = x_min; x_min = tmp; }

                y_min = mdata.rect_y0;
                y_max = mdata.rect_y1;
                if ( y_min > y_max ) { float tmp = y_max; y_max = y_min; y_min = tmp; }

                ogl_draw_box( x_min, y_min, x_max - x_min, y_max - y_min, color );
            }
        }
        cartman_end_ortho_camera();

        // force OpenGL to execute these commands
        glFlush();
    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void render_window( window_t * pwin )
{
    if ( NULL == pwin ||  !pwin->on ) return;

    glPushAttrib( GL_SCISSOR_BIT );
    {
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - ( pwin->y + pwin->surfacey ), pwin->surfacex, pwin->surfacey );

        make_onscreen();

        if ( HAS_BITS( pwin->mode, WINMODE_TILE ) )
        {
            render_tile_window( pwin, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( pwin->mode, WINMODE_VERTEX ) )
        {
            render_vertex_window( pwin, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( pwin->mode, WINMODE_SIDE ) )
        {
            render_side_window( pwin, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( pwin->mode, WINMODE_FX ) )
        {
            render_fx_window( pwin, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        draw_cursor_in_window( pwin );

    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void render_all_windows( void )
{
    int cnt;

    for ( cnt = 0; cnt < MAXWIN; cnt++ )
    {
        render_window( window_lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void load_all_windows( void )
{
    int cnt;

    for ( cnt = 0; cnt < MAXWIN; cnt++ )
    {
        window_lst[cnt].on = bfalse;
        oglx_texture_Release( &( window_lst[cnt].tex ) );
    }

    load_window( window_lst + 0, 0, "data" SLASH_STR "window.png", 180, 16,  7, 9, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, WINMODE_VERTEX );
    load_window( window_lst + 1, 1, "data" SLASH_STR "window.png", 410, 16,  7, 9, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, WINMODE_TILE );
    load_window( window_lst + 2, 2, "data" SLASH_STR "window.png", 180, 248, 7, 9, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, WINMODE_SIDE );
    load_window( window_lst + 3, 3, "data" SLASH_STR "window.png", 410, 248, 7, 9, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, WINMODE_FX );
}

//--------------------------------------------------------------------------------------------
void draw_window( window_t * pwin )
{
    if ( NULL == pwin || !pwin->on ) return;

    ogl_draw_sprite_2d( &( pwin->tex ), pwin->x, pwin->y, pwin->surfacex, pwin->surfacey );
}

//--------------------------------------------------------------------------------------------
void draw_all_windows( void )
{
    int cnt;

    for ( cnt = 0; cnt < MAXWIN; cnt++ )
    {
        draw_window( window_lst + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void bound_camera( void )
{
    if ( cam.x < 0 )
    {
        cam.x = 0;
    }
    else if ( cam.x > mesh.edgex )
    {
        cam.x = mesh.edgex;
    }

    if ( cam.y < 0 )
    {
        cam.y = 0;
    }
    else if ( cam.y > mesh.edgey )
    {
        cam.y = mesh.edgey;
    }

    if ( cam.z < -mesh.edgez )
    {
        cam.z = -mesh.edgez;
    }
    else if ( cam.y > mesh.edgey )
    {
        cam.y = mesh.edgez;
    }

}

//--------------------------------------------------------------------------------------------
void unbound_mouse()
{
    if ( !mos.drag )
    {
        mos.tlx = 0;
        mos.tly = 0;
        mos.brx = ui.scr.x - 1;
        mos.bry = ui.scr.y - 1;
    }
}

//--------------------------------------------------------------------------------------------
void bound_mouse()
{
    if ( mdata.win_id != -1 )
    {
        mos.tlx = window_lst[mdata.win_id].x + window_lst[mdata.win_id].borderx;
        mos.tly = window_lst[mdata.win_id].y + window_lst[mdata.win_id].bordery;
        mos.brx = mos.tlx + window_lst[mdata.win_id].surfacex - 1;
        mos.bry = mos.tly + window_lst[mdata.win_id].surfacey - 1;
    }
}

//--------------------------------------------------------------------------------------------
int vertex_calc_vrta( Uint32 vert )
{
    int newa, cnt;
    float x, y, z;
    float brx, bry, brz, deltaz, dist;
    float newlevel, distance, disx, disy;

    if ( CHAINEND == vert || vert >= MAXTOTALMESHVERTICES ) return ~0;

    if ( VERTEXUNUSED == mesh.vrta[vert] ) return ~0;

    // To make life easier
    x = mesh.vrtx[vert];
    y = mesh.vrty[vert];
    z = mesh.vrtz[vert];

    // Directional light
    brx = x + 64;
    bry = y + 64;
    brz = get_level( brx, y ) +
          get_level( x, bry ) +
          get_level( x + 46, y + 46 );
    if ( z < -128 ) z = -128;
    if ( brz < -128*3 ) brz = -128 * 3;
    deltaz = z + z + z - brz;
    newa = ( deltaz * direct / 256.0f );

    // Point lights !!!BAD!!!
    newlevel = 0;
    cnt = 0;
    while ( cnt < numlight )
    {
        disx = x - light_lst[cnt].x;
        disy = y - light_lst[cnt].y;
        distance = sqrt(( float )( disx * disx + disy * disy ) );
        if ( distance < light_lst[cnt].radius )
        {
            newlevel += (( light_lst[cnt].level * ( light_lst[cnt].radius - distance ) ) / light_lst[cnt].radius );
        }
        cnt++;
    }
    newa += newlevel;

    // Bounds
    if ( newa < -ambicut ) newa = -ambicut;
    newa += ambi;
    mesh.vrta[vert] = CLIP( newa, 1, 255 );

    // Edge fade
    //dist = dist_from_border( mesh.vrtx[vert], mesh.vrty[vert] );
    //if ( dist <= FADEBORDER )
    //{
    //    newa = newa * dist / FADEBORDER;
    //    if ( newa == VERTEXUNUSED )  newa = 1;
    //    mesh.vrta[vert] = newa;
    //}

    return newa;
}

//--------------------------------------------------------------------------------------------
void fan_calc_vrta( int fan )
{
    int num, cnt;
    Uint8 type;
    Uint32 vert;

    if ( -1 == fan ) return;

    type = mesh.fantype[fan];
    if ( type > MAXMESHTYPE ) return;

    num = mesh.command[type].numvertices;
    if ( 0 == num ) return;

    for ( cnt = 0, vert = mesh.vrtstart[fan];
          cnt < num && CHAINEND != vert;
          cnt++, vert = mesh.vrtnext[vert] )
    {
        vertex_calc_vrta( vert );
    }
}

//--------------------------------------------------------------------------------------------
void mesh_calc_vrta()
{
    int mapx, mapy;
    int fan;

    for ( mapy = 0; mapy < mesh.tiles_y; mapy++ )
    {
        for ( mapx = 0; mapx < mesh.tiles_x; mapx++ )
        {
            fan = mesh_get_fan( mapx, mapy );

            fan_calc_vrta( fan );
        }
    }
}

//--------------------------------------------------------------------------------------------
void move_camera()
{
    if (( -1 != mdata.win_id ) && ( MOUSE_PRESSED( SDL_BUTTON_MIDDLE ) || CART_KEYDOWN( SDLK_m ) ) )
    {
        cam.x += mos.x - mos.x_old;
        cam.y += mos.y - mos.y_old;

        mos.x = mos.x_old;
        mos.y = mos.y_old;

        bound_camera();
    }
}

//--------------------------------------------------------------------------------------------
void cartman_check_mouse_side( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int    mpix_x, mpix_z;
    float  mpos_x, mpos_z;
    bool_t inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_SIDE ) ) return;

    mpix_x = mos.x - ( pwin->x + pwin->borderx + pwin->surfacex / 2 );
    mpix_z = mos.y - ( pwin->y + pwin->bordery + pwin->surfacey / 2 );

    inside = ( mpix_x >= -( pwin->surfacex / 2 ) ) && ( mpix_x <= ( pwin->surfacex / 2 ) ) &&
             ( mpix_z >= -( pwin->surfacey / 2 ) ) && ( mpix_z <= ( pwin->surfacey / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_z = SCREEN_TO_REAL( mpix_z, cam.z, zoom_vrt );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_z = 0;

        if ( mpix_x < - pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x + pwin->surfacex / 2;
        }
        else if ( mpix_x > pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x - pwin->surfacex / 2;
        }

        if ( mpix_z < - pwin->surfacex / 2 )
        {
            dmpix_z = mpix_z + pwin->surfacey / 2;
        }
        else if ( mpix_z > pwin->surfacey / 2 )
        {
            dmpix_z = mpix_z - pwin->surfacey / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_z )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.z += dmpix_z * FOURNUM / zoom_vrt;

            bound_camera();
        }
    }
    else if ( inside )
    {
        mdata.win_id = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.xpos      = mpos_x;
        mdata.ypos      = mpos_z;
        mdata.xfan      = floor( mpix_x / ( float )TILE_SIZE );
        mdata.yfan      = -1;

        debugx = mpos_x;
        debugy = mpos_z;

        if ( MOUSE_PRESSED( SDL_BUTTON_LEFT ) )
        {
            if ( -1 == mdata.rect_drag )
            {
                mdata.rect_draw = btrue;
                mdata.rect_drag = pwin->id;
                mdata.rect_done = -1;

                mdata.rect_x0 = mdata.rect_x1 = mpos_x;
                mdata.rect_y0 = mdata.rect_y1 = mpos_z;
            }
            else if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_x1 = mpos_x;
                mdata.rect_y1 = mpos_z;
            }
        }
        else
        {
            if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_drag = -1;
                mdata.rect_done = pwin->id;
            }
        }

        if ( pwin->id == mdata.rect_done )
        {
            if ( select_count() > 0 && !CART_KEYMOD( KMOD_ALT ) && !CART_KEYDOWN( SDLK_MODE ) &&
                 !CART_KEYMOD( KMOD_LCTRL ) && !CART_KEYMOD( KMOD_RCTRL ) )
            {
                select_clear();
            }

            if ( CART_KEYMOD( KMOD_ALT ) || CART_KEYDOWN( SDLK_MODE ) )
            {
                cart_mouse_data_rect_unselect();
            }
            else
            {
                cart_mouse_data_rect_select();
            }

            mdata.rect_draw = bfalse;
            mdata.rect_drag   = -1;
            mdata.rect_done = -1;
        }

        if ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) )
        {
            move_select( mos.cx / zoom_hrz, 0, - mos.cy / zoom_vrt );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_y ) )
        {
            move_select( 0, 0, -mos.cy / zoom_vrt );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_u ) )
        {
            if ( mdata.type >= ( MAXMESHTYPE >> 1 ) )
            {
                move_mesh_z( -mos.cy / zoom_vrt, mdata.tx, 0xC0 );
            }
            else
            {
                move_mesh_z( -mos.cy / zoom_vrt, mdata.tx, 0xF0 );
            }
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_n ) )
        {
            if ( CART_KEYDOWN( SDLK_RSHIFT ) )
            {
                // Move the first 16 up and down
                move_mesh_z( -mos.cy / zoom_vrt, 0, 0xF0 );
            }
            else
            {
                // Move the entire mesh up and down
                move_mesh_z( -mos.cy / zoom_vrt, 0, 0 );
            }
            bound_mouse();
        }
    }
}

//--------------------------------------------------------------------------------------------
void cartman_check_mouse_tile( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int vert, fan_tmp;
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    bool_t inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_TILE ) ) return;

    mpix_x = mos.x - ( pwin->x + pwin->borderx + pwin->surfacex / 2 );
    mpix_y = mos.y - ( pwin->y + pwin->bordery + pwin->surfacey / 2 );

    inside = ( mpix_x >= -( pwin->surfacex / 2 ) ) && ( mpix_x <= ( pwin->surfacex / 2 ) ) &&
             ( mpix_y >= -( pwin->surfacey / 2 ) ) && ( mpix_y <= ( pwin->surfacey / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x + pwin->surfacex / 2;
        }
        else if ( mpix_x > pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x - pwin->surfacex / 2;
        }

        if ( mpix_y < - pwin->surfacex / 2 )
        {
            dmpix_y = mpix_y + pwin->surfacey / 2;
        }
        else if ( mpix_y > pwin->surfacey / 2 )
        {
            dmpix_y = mpix_y - pwin->surfacey / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x  * FOURNUM / zoom_hrz;
            cam.y += dmpix_y  * FOURNUM / zoom_hrz;

            bound_camera();
        }
    }
    else if ( inside )
    {
        mdata.win_id    = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.xpos      = mpos_x;
        mdata.ypos      = mpos_y;
        mdata.xfan      = floor( mpos_x / ( float )TILE_SIZE );
        mdata.yfan      = floor( mpos_y / ( float )TILE_SIZE );

        debugx = mpos_x;
        debugy = mpos_y;

        // update mdata.onfan only if the tile is valid
        fan_tmp = mesh_get_fan( mdata.xfan, mdata.yfan );
        if ( -1 != fan_tmp ) mdata.onfan = fan_tmp;

        if ( MOUSE_PRESSED( SDL_BUTTON_LEFT ) )
        {
            cart_mouse_data_mesh_replace_tile( CART_KEYDOWN( SDLK_t ), CART_KEYDOWN( SDLK_v ) );
        }

        if ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) )
        {
            // force an update of mdata.onfan
            mdata.onfan = fan_tmp;

            if ( mdata.onfan >= 0 && mdata.onfan < MAXMESHFAN )
            {
                mdata.type  = mesh.fantype[mdata.onfan];
                mdata.tx    = TILE_GET_LOWER_BITS( mesh.tx_bits[mdata.onfan] );
                mdata.upper = TILE_GET_UPPER_BITS( mesh.tx_bits[mdata.onfan] );
            }
            else
            {
                mdata.type  = 0;
                mdata.tx    = TILE_GET_LOWER_BITS( FANOFF );
                mdata.upper = TILE_GET_UPPER_BITS( FANOFF );
            }
        }

        if ( !CART_KEYDOWN( SDLK_k ) )
        {
            addinglight = bfalse;
        }
        if ( CART_KEYDOWN( SDLK_k ) && !addinglight )
        {
            add_light( mdata.xpos, mdata.ypos, MINRADIUS / zoom_hrz, MAP_MAXLEVEL / zoom_hrz );
            addinglight = btrue;
        }
        if ( addinglight )
        {
            alter_light( mdata.xpos, mdata.ypos );
        }
    }
}

//--------------------------------------------------------------------------------------------
void cartman_check_mouse_fx( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    bool_t inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_FX ) ) return;

    mpix_x = mos.x - ( pwin->x + pwin->borderx + pwin->surfacex / 2 );
    mpix_y = mos.y - ( pwin->y + pwin->bordery + pwin->surfacey / 2 );

    inside = ( mpix_x >= -( pwin->surfacex / 2 ) ) && ( mpix_x <= ( pwin->surfacex / 2 ) ) &&
             ( mpix_y >= -( pwin->surfacey / 2 ) ) && ( mpix_y <= ( pwin->surfacey / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x + pwin->surfacex / 2;
        }
        else if ( mpix_x > pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x - pwin->surfacex / 2;
        }

        if ( mpix_y < - pwin->surfacex / 2 )
        {
            dmpix_y = mpix_y + pwin->surfacey / 2;
        }
        else if ( mpix_y > pwin->surfacey / 2 )
        {
            dmpix_y = mpix_y - pwin->surfacey / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.y += dmpix_y * FOURNUM / zoom_hrz;

            bound_camera();
        }
    }
    else if ( inside )
    {
        int fantmp;

        mdata.win_id    = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.xpos      = mpos_x;
        mdata.ypos      = mpos_y;
        mdata.xfan      = floor( mpos_x / ( float )TILE_SIZE );
        mdata.yfan      = floor( mpos_y / ( float )TILE_SIZE );

        debugx = mpos_x;
        debugy = mpos_y;

        fantmp = mesh_get_fan( mdata.xfan, mdata.yfan );
        if ( -1 != fantmp ) mdata.onfan = fantmp;

        if ( MOUSE_PRESSED( SDL_BUTTON_LEFT ) )
        {
            if ( !CART_KEYDOWN( SDLK_LSHIFT ) )
            {
                cart_mouse_data_mesh_set_fx();
            }
            else
            {
                cart_mouse_data_mesh_replace_fx();
            }
        }

        if ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) )
        {
            mdata.onfan = fantmp;

            if ( mdata.onfan >= 0 && mdata.onfan < MAXMESHFAN )
            {
                mdata.fx = mesh.fx[mdata.onfan];
            }
            else
            {
                mdata.fx = MPDFX_WALL | MPDFX_IMPASS;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void cartman_check_mouse_vertex( window_t * pwin, float zoom_hrz, float zoom_vrt )
{
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    bool_t inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_VERTEX ) ) return;

    mpix_x = mos.x - ( pwin->x + pwin->surfacex / 2 );
    mpix_y = mos.y - ( pwin->y + pwin->surfacey / 2 );

    inside = ( mpix_x >= -( pwin->surfacex / 2 ) ) && ( mpix_x <= ( pwin->surfacex / 2 ) ) &&
             ( mpix_y >= -( pwin->surfacey / 2 ) ) && ( mpix_y <= ( pwin->surfacey / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x + pwin->surfacex / 2;
        }
        else if ( mpix_x > pwin->surfacex / 2 )
        {
            dmpix_x = mpix_x - pwin->surfacex / 2;
        }

        if ( mpix_y < - pwin->surfacex / 2 )
        {
            dmpix_y = mpix_y + pwin->surfacey / 2;
        }
        else if ( mpix_y > pwin->surfacey / 2 )
        {
            dmpix_y = mpix_y - pwin->surfacey / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.y += dmpix_y * FOURNUM / zoom_hrz;

            bound_camera();
        }
    }
    else if ( inside )
    {
        mdata.win_id    = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.xpos      = mpos_x;
        mdata.ypos      = mpos_y;
        mdata.xfan      = floor( mpos_x / ( float )TILE_SIZE );
        mdata.yfan      = floor( mpos_y / ( float )TILE_SIZE );

        debugx = mpos_x;
        debugy = mpos_y;

        if ( MOUSE_PRESSED( SDL_BUTTON_LEFT ) )
        {
            if ( -1 == mdata.rect_drag )
            {
                mdata.rect_draw = btrue;
                mdata.rect_drag = pwin->id;
                mdata.rect_done = -1;

                mdata.rect_x0 = mdata.rect_x1 = mpos_x;
                mdata.rect_y0 = mdata.rect_y1 = mpos_y;
            }
            else if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_x1 = mpos_x;
                mdata.rect_y1 = mpos_y;
            }
        }
        else
        {
            if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_drag = -1;
                mdata.rect_done = pwin->id;
            }
        }

        if ( pwin->id == mdata.rect_done )
        {
            if ( select_count() > 0 && !CART_KEYMOD( KMOD_ALT ) && !CART_KEYDOWN( SDLK_MODE ) &&
                 !CART_KEYMOD( KMOD_LCTRL ) && !CART_KEYMOD( KMOD_RCTRL ) )
            {
                select_clear();
            }
            if ( CART_KEYMOD( KMOD_ALT ) || CART_KEYDOWN( SDLK_MODE ) )
            {
                cart_mouse_data_rect_unselect();
            }
            else
            {
                cart_mouse_data_rect_select();
            }

            mdata.rect_draw = bfalse;
            mdata.rect_drag = -1;
            mdata.rect_done = -1;
        }

        if ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) )
        {
            move_select( mos.cx / zoom_vrt, mos.cy / zoom_vrt, 0 );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_f ) )
        {
            //    fix_corners(mdata.xpos>>7, mdata.ypos>>7);
            fix_vertices( floor( mdata.xpos / ( float )TILE_SIZE ), floor( mdata.ypos / ( float )TILE_SIZE ) );
        }

        if ( CART_KEYDOWN( SDLK_p ) || ( MOUSE_PRESSED( SDL_BUTTON_RIGHT ) && 0 == select_count() ) )
        {
            raise_mesh( onscreen_vert, onscreen_count, mdata.xpos, mdata.ypos, brushamount, brushsize );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t cartman_check_mouse( const char * modulename )
{
    int cnt;

    if ( !mos.on ) return bfalse;

    unbound_mouse();
    move_camera();

    // place this after move_camera()
    update_mouse();

    // handle all window-specific commands
    //if( mos.drag && NULL != mos.drag_window )
    //{
    //    // we are dragging something in a specific window
    //    cartman_check_mouse_tile( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_vertex( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_side( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_fx( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //}
    //else
    {
        mdata.win_id = -1;

        for ( cnt = 0; cnt < MAXWIN; cnt++ )
        {
            window_t * pwin = window_lst + cnt;

            cartman_check_mouse_tile( pwin, cartman_zoom_hrz, 1.0f / 16.0f );
            cartman_check_mouse_vertex( pwin, cartman_zoom_hrz, 1.0f / 16.0f );
            cartman_check_mouse_side( pwin, cartman_zoom_hrz, 1.0f / 16.0f );
            cartman_check_mouse_fx( pwin, cartman_zoom_hrz, 1.0f / 16.0f );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void ease_up_mesh( float zoom_vrt )
{
    // ZZ> This function lifts the entire mesh

    mos.y = mos.y_old;
    mos.x = mos.x_old;

    mesh_move( 0, 0, -mos.cy / zoom_vrt );
}

//--------------------------------------------------------------------------------------------
bool_t cartman_check_keys( const char * modname )
{
    if ( !check_keys( 20 ) ) return bfalse;

    // Hurt
    if ( CART_KEYDOWN( SDLK_h ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_DAMAGE );
        key.delay = KEYDELAY;
    }
    // Impassable
    if ( CART_KEYDOWN( SDLK_i ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_IMPASS );
        key.delay = KEYDELAY;
    }
    // Barrier
    if ( CART_KEYDOWN( SDLK_b ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_WALL );
        key.delay = KEYDELAY;
    }
    // Overlay
    if ( CART_KEYDOWN( SDLK_o ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_WATER );
        key.delay = KEYDELAY;
    }
    // Reflective
    if ( CART_KEYDOWN( SDLK_r ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_SHA );
        key.delay = KEYDELAY;
    }
    // Draw reflections
    if ( CART_KEYDOWN( SDLK_d ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_DRAWREF );
        key.delay = KEYDELAY;
    }
    // Animated
    if ( CART_KEYDOWN( SDLK_a ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_ANIM );
        key.delay = KEYDELAY;
    }
    // Slippy
    if ( CART_KEYDOWN( SDLK_s ) )
    {
        cart_mouse_data_toggle_fx( MPDFX_SLIPPY );
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_g ) )
    {
        fix_mesh();
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_z ) )
    {
        if ( mdata.onfan >= 0 || mdata.onfan < MAXMESHFAN )
        {
            Uint16 tx_bits = mesh.tx_bits[mdata.onfan];
            cart_mouse_data_mesh_set_tile( tx_bits );
        }
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_x ) )
    {
        if ( mdata.onfan >= 0 || mdata.onfan < MAXMESHFAN )
        {
            Uint8  type    = mesh.fantype[mdata.onfan];
            Uint16 tx_bits = mesh.tx_bits[mdata.onfan];

            if ( type >= ( MAXMESHTYPE >> 1 ) )
            {
                trim_mesh_tile( tx_bits, 0xC0 );
            }
            else
            {
                trim_mesh_tile( tx_bits, 0xF0 );
            }
        }

        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_e ) )
    {
        ease_up_mesh( cartman_zoom_vrt );
    }

    if ( CART_KEYDOWN( SDLK_LEFTBRACKET ) || CART_KEYDOWN( SDLK_RIGHTBRACKET ) )
    {
        select_verts_connected();
    }
    if ( CART_KEYDOWN( SDLK_8 ) )
    {
        cart_mouse_data_three_e_mesh();
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_j ) )
    {
        if ( 0 == select_count() ) { jitter_mesh(); }
        else { jitter_select(); }
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_w ) )
    {
        //impass_edges(2);
        mesh_calc_vrta();
        cartman_save_mesh( modname );
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_SPACE ) )
    {
        weld_select();
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_INSERT ) )
    {
        mdata.type = ( mdata.type - 1 ) % MAXMESHTYPE;
        while ( 0 == mesh.numline[mdata.type] )
        {
            mdata.type = ( mdata.type - 1 ) % MAXMESHTYPE;
        }
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_DELETE ) )
    {
        mdata.type = ( mdata.type + 1 ) % MAXMESHTYPE;
        while ( 0 == mesh.numline[mdata.type] )
        {
            mdata.type = ( mdata.type + 1 ) % MAXMESHTYPE;
        }
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_KP_PLUS ) )
    {
        mdata.tx = ( mdata.tx + 1 ) & 0xFF;
        key.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_KP_MINUS ) )
    {
        mdata.tx = ( mdata.tx - 1 ) & 0xFF;
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_UP ) || CART_KEYDOWN( SDLK_LEFT ) || CART_KEYDOWN( SDLK_DOWN ) || CART_KEYDOWN( SDLK_RIGHT ) )
    {
        if ( CART_KEYDOWN( SDLK_RIGHT ) )
        {
            cam.x += 8 * CAMRATE;
        }
        if ( CART_KEYDOWN( SDLK_LEFT ) )
        {
            cam.x -= 8 * CAMRATE;
        }

        if ( WINMODE_SIDE == mdata.win_mode )
        {
            if ( CART_KEYDOWN( SDLK_DOWN ) )
            {
                cam.z += 8 * CAMRATE * ( mesh.edgez / DEFAULT_Z_SIZE );
            }
            if ( CART_KEYDOWN( SDLK_UP ) )
            {
                cam.z -= 8 * CAMRATE * ( mesh.edgez / DEFAULT_Z_SIZE );
            }
        }
        else
        {
            if ( CART_KEYDOWN( SDLK_DOWN ) )
            {
                cam.y += 8 * CAMRATE;
            }
            if ( CART_KEYDOWN( SDLK_UP ) )
            {
                cam.y -= 8 * CAMRATE;
            }
        }
        bound_camera();
    }

    if ( CART_KEYDOWN( SDLK_PLUS ) || CART_KEYDOWN( SDLK_EQUALS ) )
    {
        cartman_zoom_hrz *= 2;
        if ( cartman_zoom_hrz > 4 )
        {
            cartman_zoom_hrz = 4;
        }
        else
        {
            key.delay = KEYDELAY;
        }
    }

    if ( CART_KEYDOWN( SDLK_MINUS ) || CART_KEYDOWN( SDLK_UNDERSCORE ) )
    {
        cartman_zoom_hrz /= 2;
        if ( cartman_zoom_hrz < 0.25f )
        {
            cartman_zoom_hrz = 0.25f;
        }
        else
        {
            key.delay = KEYDELAY;
        }
    }

    //------------------
    // from cartman_check_mouse_side() and cartman_check_mouse_tile() functions
    if ( CART_KEYDOWN( SDLK_f ) )
    {
        cart_mouse_data_flatten_mesh();
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_q ) )
    {
        fix_walls();
        key.delay = KEYDELAY;
    }

    //------------------
    // "fixed" jeys

    if ( CART_KEYDOWN( SDLK_5 ) )
    {
        set_select_no_bound_z( -8000 * 4 );
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_6 ) )
    {
        set_select_no_bound_z( -127 * 4 );
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_7 ) )
    {
        set_select_no_bound_z( 127 * 4 );
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN_MOD( SDLK_c, KMOD_SHIFT ) )
    {
        cart_mouse_data_clear_mesh();
        key.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN_MOD( SDLK_l, KMOD_SHIFT ) )
    {
        level_vrtz();
    }

    // brush size
    if ( CART_KEYDOWN( SDLK_END ) || CART_KEYDOWN( SDLK_KP1 ) )
    {
        brushsize = 0;
    }
    if ( CART_KEYDOWN( SDLK_PAGEDOWN ) || CART_KEYDOWN( SDLK_KP3 ) )
    {
        brushsize = 1;
    }
    if ( CART_KEYDOWN( SDLK_HOME ) || CART_KEYDOWN( SDLK_KP7 ) )
    {
        brushsize = 2;
    }
    if ( CART_KEYDOWN( SDLK_PAGEUP ) || CART_KEYDOWN( SDLK_KP9 ) )
    {
        brushsize = 3;
    }

    // presser
    if ( CART_KEYDOWN( SDLK_1 ) )
    {
        mdata.presser = 0;
    }
    if ( CART_KEYDOWN( SDLK_2 ) )
    {
        mdata.presser = 1;
    }
    if ( CART_KEYDOWN( SDLK_3 ) )
    {
        mdata.presser = 2;
    }
    if ( CART_KEYDOWN( SDLK_4 ) )
    {
        mdata.presser = 3;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t check_input_mouse( SDL_Event * pevt )
{
    bool_t handled = bfalse;

    if ( NULL == pevt || !mos.on ) return bfalse;

    if ( 0 == mos.b )
    {
        mos.drag = bfalse;
        mos.drag_begin = bfalse;

        // set mdata??
    }

    switch ( pevt->type )
    {
        case SDL_MOUSEBUTTONDOWN:
            switch ( pevt->button.button )
            {
                case SDL_BUTTON_LEFT:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    mos.b |= SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = btrue;
            handled = btrue;
            break;

        case SDL_MOUSEBUTTONUP:
            switch ( pevt->button.button )
            {
                case SDL_BUTTON_LEFT:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_LEFT );
                    break;

                case SDL_BUTTON_MIDDLE:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_MIDDLE );
                    break;

                case SDL_BUTTON_RIGHT:
                    mos.b &= ~SDL_BUTTON( SDL_BUTTON_RIGHT );
                    break;
            }
            ui.pending_click = bfalse;
            handled = btrue;
            break;

        case SDL_MOUSEMOTION:
            mos.b = pevt->motion.state;
            if ( mos.drag )
            {
                if ( 0 != mos.b )
                {
                    mos.brx = pevt->motion.x;
                    mos.bry = pevt->motion.y;
                }
                else
                {
                    mos.drag = bfalse;
                }
            }

            if ( mos.relative )
            {
                mos.cx = pevt->motion.xrel;
                mos.cy = pevt->motion.yrel;
            }
            else
            {
                mos.x = pevt->motion.x;
                mos.y = pevt->motion.y;
            }
            break;
    }

    if ( 0 != mos.b )
    {
        if ( mos.drag_begin )
        {
            // start dragging
            mos.drag = btrue;
        }
        else if ( !mos.drag )
        {
            // set the dragging to begin the next mouse time the mouse moves
            mos.drag_begin = btrue;

            // initialize the drag rect
            mos.tlx = mos.x;
            mos.tly = mos.y;

            mos.brx = mos.x;
            mos.bry = mos.y;

            // set the drag window
            mos.drag_window = find_window( mos.x, mos.y );
            mos.drag_mode   = ( NULL == mos.drag_window ) ? 0 : mos.drag_mode;
        }
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
bool_t check_input_keyboard( SDL_Event * pevt )
{
    bool_t handled = bfalse;

    if ( NULL == pevt || !key.on ) return bfalse;

    switch ( pevt->type )
    {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            key.state = pevt->key.state;
            key.needs_update = btrue;
            handled = btrue;
            break;
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
void draw_lotsa_stuff( void )
{
    int x, cnt, todo, tile, add;

#if defined(_DEBUG)
    // Tell which tile we're in
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, 226, NULL,
                    "X = %6.2f", debugx );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, 234, NULL,
                    "Y = %6.2f", debugy );
#endif

    // Tell user what keys are important
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 120, NULL,
                    "O = Overlay (Water)" );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 112, NULL,
                    "R = Reflective" );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 104, NULL,
                    "D = Draw Reflection" );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 96, NULL,
                    "A = Animated" );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 88, NULL,
                    "B = Barrier (Slit)" );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 80, NULL,
                    "I = Impassable (Wall)" );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 72, NULL,
                    "H = Hurt" );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 64, NULL,
                    "S = Slippy" );

    // Vertices left
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 56, NULL,
                    "Vertices %d", numfreevertices );

    // Misc data
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 40, NULL,
                    "Ambient   %d", ambi );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 32, NULL,
                    "Ambicut   %d", ambicut );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 24, NULL,
                    "Direct    %d", direct );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 16, NULL,
                    "Brush amount %d", brushamount );
    fnt_drawText_OGL( gFont_ptr, cart_white, 0, ui.scr.y - 8, NULL,
                    "Brush size   %d", brushsize );

    // Cursor
    //if (mos.x >= 0 && mos.x < ui.scr.x && mos.y >= 0 && mos.y < ui.scr.y)
    //{
    //    draw_sprite(theSurface, bmpcursor, mos.x, mos.y);
    //}

    // Tile picks
    todo = 0;
    tile = 0;
    add  = 1;
    if ( mdata.tx < MAXTILE )
    {
        switch ( mdata.presser )
        {
            case 0:
                todo = 1;
                tile = mdata.tx;
                add = 1;
                break;
            case 1:
                todo = 2;
                tile = mdata.tx & 0xFE;
                add = 1;
                break;
            case 2:
                todo = 4;
                tile = mdata.tx & 0xFC;
                add = 1;
                break;
            case 3:
                todo = 4;
                tile = mdata.tx & 0xF8;
                add = 2;
                break;
        }

        x = 0;
        for ( cnt = 0; cnt < todo; cnt++ )
        {
            if ( mdata.type >= ( MAXMESHTYPE >> 1 ) )
            {
                ogl_draw_sprite_2d( tx_bigtile + tile, x, 0, SMALLXY, SMALLXY );
            }
            else
            {
                ogl_draw_sprite_2d( tx_smalltile + tile, x, 0, SMALLXY, SMALLXY );
            }
            x += SMALLXY;
            tile += add;
        }

        fnt_drawText_OGL( gFont_ptr, cart_white, 0, 32, NULL,
                        "Tile 0x%02x 0x%02x", mdata.upper, mdata.tx );
        fnt_drawText_OGL( gFont_ptr, cart_white, 0, 40, NULL,
                        "Eats %d verts", mesh.command[mdata.type].numvertices );
        if ( mdata.type >= ( MAXMESHTYPE >> 1 ) )
        {
            fnt_drawText_OGL( gFont_ptr, cart_white, 0, 56, NULL,
                            "63x63 Tile" );
        }
        else
        {
            fnt_drawText_OGL( gFont_ptr, cart_white, 0, 56, NULL,
                            "31x31 Tile" );
        }
        draw_schematic( NULL, mdata.type, 0, 64 );
    }

    // FX selection
    draw_tile_fx( 0, 193, mdata.fx, 1.0f );

    if ( numattempt > 0 )
    {
        fnt_drawText_OGL( gFont_ptr, cart_white, NULL, 0, 0,
                        "numwritten %d/%d", numwritten, numattempt );
    }

#if defined(_DEBUG)
    fnt_drawText_OGL( gFont_ptr, cart_white, NULL, 0, 0,
                    "<%f, %f>", mos.x, mos.y );
#endif

}

//--------------------------------------------------------------------------------------------
void draw_main( void )
{
    bool_t recalc_lighting = bfalse;

    glClear( GL_COLOR_BUFFER_BIT );

    ogl_beginFrame();
    {
        int itmp;

        render_all_windows();

        draw_all_windows();

        itmp = ambi;
        draw_slider( 0, 250, 19, 350, &ambi,          0, 200 );
        if ( itmp != ambi ) recalc_lighting = btrue;

        itmp = ambicut;
        draw_slider( 20, 250, 39, 350, &ambicut,       0, ambi );
        if ( itmp != ambicut ) recalc_lighting = btrue;

        itmp = direct;
        draw_slider( 40, 250, 59, 350, &direct,        0, 100 );
        if ( itmp != direct ) recalc_lighting = btrue;

        draw_slider( 60, 250, 79, 350, &brushamount, -50,  50 );

        draw_lotsa_stuff();
    }
    ogl_endFrame();

    if ( recalc_lighting )
    {
        mesh_calc_vrta();
    }

    dunframe++;
    secframe++;

    SDL_GL_SwapBuffers();
}

//--------------------------------------------------------------------------------------------
int main( int argcnt, char* argtext[] )
{
    char modulename[100];
    STRING fname;

    // char *blah[3];

    //blah[0] = malloc(256); strcpy(blah[0], "");
    //blah[1] = malloc(256); strcpy(blah[1], "/home/bgbirdsey/egoboo");
    //blah[2] = malloc(256); strcpy(blah[2], "advent" );

    //argcnt = 3;
    //argtext = blah;

    // construct some global variables
    mouse_ctor( &mos );
    keyboard_ctor( &key );
    cart_mouse_data_ctor( &mdata );

    // initial text for the console
    show_info();

    // grab the egoboo directory and the module name from the command line
    if ( argcnt < 2 || argcnt > 3 )
    {
        printf( "USAGE: CARTMAN [PATH] MODULE ( without .MOD )\n\n" );
        exit( 0 );
    }
    else if ( argcnt < 3 )
    {
        sprintf( egoboo_path, "%s", "." );
        sprintf( modulename, "%s.mod", argtext[1] );
    }
    else if ( argcnt < 4 )
    {
        size_t len = strlen( argtext[1] );
        char * pstr = argtext[1];
        if ( pstr[0] == '\"' )
        {
            pstr[len-1] = '\0';
            pstr++;
        }
        sprintf( egoboo_path, "%s", pstr );
        sprintf( modulename, "%s.mod", argtext[2] );
    }

    // initialize the virtual file system
    vfs_init( egoboo_path );
    setup_egoboo_paths_vfs();

    // register the logging code
    log_init( vfs_resolveWriteFilename( "/debug/log.txt" ) );
    log_setLoggingLevel( 2 );
    atexit( log_shutdown );

    if ( !setup_read_vfs() )
    {
        log_error( "Cannot load the setup file \"%s\".\n", fname );
    }
    setup_download( &cfg );

    // initialize the SDL elements
    cartman_init_SDL_base();
    gfx_system_begin();

    gFont_ptr = fnt_loadFont( "data" SLASH_STR "pc8x8.fon", 12 );

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    make_randie();                      // Random number table
    fill_fpstext();                     // Make the FPS text

    load_all_windows();                 // Load windows
    create_imgcursor();                 // Make cursor image
    load_img();                         // Load other images
    load_mesh_fans();                   // Get fan data
    load_module( modulename );          // Load the module

    dunframe   = 0;                     // Timer resets
    worldclock = 0;
    timclock   = 0;
    while ( btrue )  // Main loop
    {
        if ( CART_KEYDOWN( SDLK_ESCAPE ) || CART_KEYDOWN( SDLK_F1 ) ) break;

        cartman_check_input( modulename );

        draw_main();

        SDL_Delay( 1 );

        timclock = SDL_GetTicks() >> 3;
    }

    show_info();                // Ending statistics
    exit( 0 );                      // End
}

static bool_t _sdl_initialized_base = bfalse;

//--------------------------------------------------------------------------------------------
void cartman_init_SDL_base()
{
    if ( _sdl_initialized_base ) return;

    log_info( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init( 0 ) < 0 )
    {
        log_message( "Failure!\n" );
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    if ( !_sdl_atexit_registered )
    {
        atexit( SDL_Quit );
        _sdl_atexit_registered = bfalse;
    }

    log_info( "Intializing SDL Timing Services... " );
    if ( SDL_InitSubSystem( SDL_INIT_TIMER ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    log_info( "Intializing SDL Event Threading... " );
    if ( SDL_InitSubSystem( SDL_INIT_EVENTTHREAD ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    _sdl_initialized_base = btrue;
}

//--------------------------------------------------------------------------------------------
void cartman_create_mesh( void )
{
    int mapx, mapy, fan;
    int x, y;

    mesh_info_t mpd_info;

    printf( "Mesh file not found, so creating a new one...\n" );

    printf( "Number of tiles in X direction ( 32-512 ):  " );
    scanf( "%d", &( mpd_info.tiles_x ) );

    printf( "Number of tiles in Y direction ( 32-512 ):  " );
    scanf( "%d", &( mpd_info.tiles_y ) );

    create_mesh( &mpd_info );

    fan = 0;
    for ( mapy = 0; mapy < mesh.tiles_y; mapy++ )
    {
        y = mapy * TILE_SIZE;
        for ( mapx = 0; mapx < mesh.tiles_x; mapx++ )
        {
            x = mapx * TILE_SIZE;
            if ( !add_fan( fan, x, y ) )
            {
                printf( "NOT ENOUGH VERTICES!!!\n\n" );
                exit( -1 );
            }

            fan++;
        }
    }

    fix_mesh();
}

//--------------------------------------------------------------------------------------------
void cartman_save_mesh( const char * modname )
{
    STRING newloadname;

    numwritten = 0;
    numattempt = 0;

    sprintf( newloadname, "%s" SLASH_STR "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "plan.bmp", egoboo_path, modname );

    make_planmap();
    if ( bmphitemap )
    {
        SDL_SaveBMP( bmphitemap, newloadname );
    }

    //  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "level.png", newloadname);
    //  make_hitemap();
    //  if(bmphitemap)
    //  {
    //    make_graypal();
    //    save_pcx(newloadname, bmphitemap);
    //  }

    save_mesh( modname );

    show_name( newloadname, cart_white );
}

//--------------------------------------------------------------------------------------------
void cartman_check_input( const char * modulename )
{
    debugx = -1;
    debugy = -1;

    check_input();

    cartman_check_mouse( modulename );
    cartman_check_keys( modulename );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
cart_mouse_data_t * cart_mouse_data_ctor( cart_mouse_data_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    memset( ptr, 0, sizeof( *ptr ) );

    ptr->win_id   = -1;
    ptr->win_mode = -1;
    ptr->xfan     = -1;
    ptr->yfan     = -1;

    ptr->onfan = -1;
    ptr->fx = MPDFX_SHA;

    ptr->rect_drag = -1;
    ptr->rect_done = -1;

    return ptr;
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_mesh_set_tile( Uint16 tiletoset )
{
    mesh_set_tile( tiletoset, mdata.upper, mdata.presser, mdata.tx );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_flatten_mesh()
{
    flatten_mesh( mdata.ypos );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_clear_mesh()
{
    clear_mesh( mdata.upper, mdata.presser, mdata.tx, mdata.type );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_three_e_mesh()
{
    three_e_mesh( mdata.upper, mdata.tx );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_mesh_replace_tile( bool_t tx_only, bool_t at_floor_level )
{
    mesh_replace_tile( mdata.xfan, mdata.yfan, mdata.onfan, mdata.tx, mdata.upper, mdata.fx, mdata.type, mdata.presser, tx_only, at_floor_level );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_mesh_set_fx()
{
    mesh_set_fx( mdata.onfan, mdata.fx );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_toggle_fx( int fxmask )
{
    mdata.fx ^= fxmask;
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_rect_select()
{
    select_add_rect( mdata.rect_x0, mdata.rect_y0, mdata.rect_x1, mdata.rect_y1, mdata.win_mode );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_rect_unselect()
{
    select_add_rect( mdata.rect_x0, mdata.rect_y0, mdata.rect_x1, mdata.rect_y1, mdata.win_mode );
}

//--------------------------------------------------------------------------------------------
void cart_mouse_data_mesh_replace_fx()
{
    Uint8  type;
    Uint16 tx_bits;

    if ( mdata.onfan < 0 || mdata.onfan >= MAXMESHFAN ) return;

    type = mesh.fantype[mdata.onfan];
    if ( type >= MAXMESHTYPE ) return;

    tx_bits = mesh.tx_bits[mdata.onfan];
    if ( TILE_IS_FANOFF( tx_bits ) ) return;

    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
        mesh_replace_fx( tx_bits, 0xC0, mdata.fx );
    }
    else
    {
        mesh_replace_fx( tx_bits, 0xF0, mdata.fx );
    }
}

//--------------------------------------------------------------------------------------------
void setup_egoboo_paths_vfs()
{
    /// @details BB@> set the basic mount points used by the main program

    //---- tell the vfs to add the basic search paths
    vfs_set_base_search_paths();

    //---- mount all of the default global directories

    // mount the global basicdat directory t the beginning of the list
    vfs_add_mount_point( fs_getDataDirectory(), "basicdat", "mp_data", 1 );

    // Create a mount point for the /user/modules directory
    vfs_add_mount_point( fs_getUserDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /data/modules directory
    vfs_add_mount_point( fs_getDataDirectory(), "modules", "mp_modules", 1 );

    // Create a mount point for the /user/players directory
    vfs_add_mount_point( fs_getUserDirectory(), "players", "mp_players", 1 );

    // Create a mount point for the /data/players directory
    //vfs_add_mount_point( fs_getDataDirectory(), "players", "mp_players", 1 );     //ZF> Let's remove the local players folder since it caused so many problems for people

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "import", "mp_import", 1 );

    // Create a mount point for the /user/remote directory
    vfs_add_mount_point( fs_getUserDirectory(), "remote", "mp_remote", 1 );
}

////--------------------------------------------------------------------------------------------
//void sdlinit( int argc, char **argv )
//{
//
//    log_info( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
//    if ( SDL_Init( 0 ) < 0 )
//    {
//        log_message( "Failed!\n" );
//        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
//    }
//    else
//    {
//        log_message( "Success!\n" );
//    }
//
//    if ( !_sdl_atexit_registered )
//    {
//        atexit( SDL_Quit );
//        _sdl_atexit_registered = bfalse;
//    }
//
//    log_info( "INFO: Initializing SDL video... " );
//    if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0 )
//    {
//        log_message( "Failed!\n" );
//        log_error( "SDL error == \"%s\"\n", SDL_GetError() );
//    }
//    else
//    {
//        log_message( "Success!\n" );
//    }
//
//    log_info( "Initializing SDL timer services... " );
//    if ( SDL_InitSubSystem( SDL_INIT_TIMER ) < 0 )
//    {
//        log_message( "Failed!\n" );
//        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
//    }
//    else
//    {
//        log_message( "Success!\n" );
//    }
//
//    log_info( "Intializing SDL Event Threading... " );
//    if ( SDL_InitSubSystem( SDL_INIT_EVENTTHREAD ) < 0 )
//    {
//        log_message( "Failed!\n" );
//        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
//    }
//    else
//    {
//        log_message( "Succeess!\n" );
//    }
//
//    // initialize the font handler
//    log_info( "Initializing the SDL_ttf font handler... " );
//    if ( TTF_Init() < 0 )
//    {
//        log_message( "Failed! Unable to load the font handler: %s\n", SDL_GetError() );
//        exit( -1 );
//    }
//    else
//    {
//        log_message( "Success!\n" );
//        if ( !_ttf_atexit_registered )
//        {
//            _ttf_atexit_registered = btrue;
//            atexit( TTF_Quit );
//        }
//    }
//
//#ifdef __unix__
//
//    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
//    // will cause SDL_SetVideoMode to fail with:
//    // Unable to set video mode: Couldn't find matching GLX visual
//    if ( cfg.scr.d == 32 ) cfg.scr.d = 24;
//
//#endif
//
//    // the flags to pass to SDL_SetVideoMode
//    sdl_vparam.flags          = SDL_OPENGLBLIT;                // enable SDL blitting operations in OpenGL (for the moment)
//    sdl_vparam.opengl         = SDL_TRUE;
//    sdl_vparam.doublebuffer   = SDL_TRUE;
//    sdl_vparam.glacceleration = GL_FALSE;
//    sdl_vparam.width          = MIN( 640, cfg.scrx_req );
//    sdl_vparam.height         = MIN( 480, cfg.scry_req );
//    sdl_vparam.depth          = cfg.scrd_req;
//
//    ogl_vparam.dither         = GL_FALSE;
//    ogl_vparam.antialiasing   = GL_TRUE;
//    ogl_vparam.perspective    = GL_FASTEST;
//    ogl_vparam.shading        = GL_SMOOTH;
//    ogl_vparam.userAnisotropy = cfg.texturefilter > TX_TRILINEAR_2;
//
//    // Get us a video mode
//    if ( NULL == SDL_GL_set_mode( NULL, &sdl_vparam, &ogl_vparam ) )
//    {
//        log_info( "I can't get SDL to set any video mode: %s\n", SDL_GetError() );
//        exit( -1 );
//    }
//
//    glEnable( GL_LINE_SMOOTH );
//    glEnable( GL_BLEND );
//    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
//    glHint( GL_LINE_SMOOTH_HINT, GL_DONT_CARE );
//    glLineWidth( 1.5f );
//
//    //  SDL_WM_SetIcon(tmp_surface, NULL);
//    SDL_WM_SetCaption( "Egoboo", "Egoboo" );
//
//    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ) < 0 )
//    {
//        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
//    }
//    atexit( SDL_Quit );
//
//    // start the font handler
//    if ( TTF_Init() < 0 )
//    {
//        log_error( "Unable to load the font handler: %s\n", SDL_GetError() );
//    }
//    atexit( TTF_Quit );
//
//#ifdef __unix__
//    /* GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
//    will cause SDL_SetVideoMode to fail with:
//    "Unable to set video mode: Couldn't find matching GLX visual" */
//    if ( cfg.scr.d == 32 ) cfg.scr.d = 24;
//#endif
//
//#ifndef __APPLE__
//    {
//        SDL_Surface * tmp_icon;
//        /* Setup the cute windows manager icon */
//        tmp_icon = SDL_LoadBMP( "data" SLASH_STR "egomap_icon.bmp" );
//        if ( tmp_icon == NULL )
//        {
//            log_warning( "Unable to load icon (data" SLASH_STR "egomap_icon.bmp)\n" );
//        }
//        else
//        {
//            SDL_WM_SetIcon( tmp_icon, NULL );
//        }
//    }
//#endif
//
//    SDLX_Get_Screen_Info( &( ui.scr ), ( SDL_bool )bfalse );
//
//    theSurface = SDL_GetVideoSurface();
//
//    // Set the window name
//    SDL_WM_SetCaption( NAME, NAME );
//
//    if ( ui.GrabMouse )
//    {
//        SDL_WM_GrabInput( SDL_GRAB_ON );
//    }
//
//    if ( ui.HideMouse )
//    {
//        SDL_ShowCursor( 0 );  // Hide the mouse cursor
//    }
//}
//
