#include "cartman.h"

#include "ogl_texture.h"
#include "Log.h"
#include "Font.h"
#include "SDL_Pixel.h"
#include "egoboo_endian.h"
#include "SDL_GL_extensions.h"
#include "egoboo_setup.h"
#include "egoboo_graphic.h"
#include "egoboo_fileutil.h"

#include <SDL.h>              // SDL header
#include <SDL_opengl.h>
#include <SDL_image.h>

#include <stdio.h>          // For printf and such
#include <fcntl.h>          // For fast file i/o
#include <math.h>
#include <assert.h>

Font * gFont = NULL;

enum
{
    CORNER_TL,
    CORNER_TR,
    CORNER_BL,
    CORNER_BR,
    CORNER_COUNT
};

enum { kX, kY, kZ };

typedef float cart_vec_t[3];

bool_t                  keyon = btrue;                // Is the keyboard alive?
int                     keycount = 0;
Uint8  *                keysdlbuffer = NULL;
Uint8                   keystate = 0;

Sint16          damagetileparttype;
short           damagetilepartand;
short           damagetilesound;
short           damagetilesoundtime;
Uint16          damagetilemindistance;
int             damagetileamount = 256;                           // Amount of damage
Uint8           damagetiletype  = DAMAGE_FIRE;                      // Type of damage

SDLX_video_parameters_t sdl_vparam;
oglx_video_parameters_t ogl_vparam;

#define SDLKEYDOWN(k)  ( ((k >= keycount) || (NULL == keysdlbuffer)) ? bfalse : (0 != keysdlbuffer[k]))     // Helper for gettin' em
#define SDLKEYMOD(m)   ( (NULL != keysdlbuffer) && (0 != (keystate & (m))) )
#define SDLKEYDOWN_MOD(k,m) ( SDLKEYDOWN(k) && (0 != (keystate & (m))) )

#define DEFAULT_TILE 62

STRING egoboo_path;

#define HAS_BITS(A, B) ( 0 != ((A)&(B)) )

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
typedef struct s_ui_state ui_state_t;
ui_state_t ui;

config_data_t cfg;
ConfigFilePtr_t cfg_file;

struct s_mouse
{
    int   x, y, x_old, y_old, b, cx, cy;
    int     tlx, tly, brx, bry;
    int     speed;
    bool_t relative;
    Uint8 button[4];             // Mouse button states
};

typedef struct s_mouse mouse_t;

mouse_t mos =
{
    0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0,
    2
};

#define NEARLOW  0.0 //16.0     // For autoweld
#define NEARHI 128.0 //112.0        //
#define BARRIERHEIGHT 14.0      //

#define OGL_MAKE_COLOR_3(COL, BB,GG,RR) { COL[0] = RR / 32.0f; COL[1] = GG / 32.0f; COL[2] = BB / 32.0f; }
#define OGL_MAKE_COLOR_4(COL, AA,BB,GG,RR) { COL[0] = RR / 32.0f; COL[1] = GG / 32.0f; COL[2] = BB / 32.0f; COL[3] = AA / 32.0f; }

#define MAKE_BGR(BMP,BB,GG,RR)     SDL_MapRGBA(BMP->format, (RR)<<3, (GG)<<3, (BB)<<3, 0xFF)
#define MAKE_ABGR(BMP,AA,BB,GG,RR) SDL_MapRGBA(BMP->format, (RR)<<3, (GG)<<3, (BB)<<3, AA)

SDL_Color MAKE_SDLCOLOR(Uint8 BB, Uint8 RR, Uint8 GG) { SDL_Color tmp; tmp.r = RR << 3; tmp.g = GG << 3; tmp.b = BB << 3; return tmp; }

#ifdef _WIN32
#    define SLASH_STR "\\"
#    define SLASH_CHR '\\'
#else
#    define SLASH_STR "/"
#    define SLASH_CHR '/'
#endif


size_t numwritten = 0;
size_t numattempt = 0;

int addinglight;

int numlight;
struct s_light
{
    int           x;
    int           y;
    Uint8 level;
    int           radius;
};
typedef struct s_light light_t;
light_t light_lst[MAXLIGHT];

int ambi = 22;
int ambicut = 1;
int direct = 16;

#define POINT_SIZE(X) ( ( (X) >> 1 ) + 4 )

int    animtileupdateand   = 7;
Uint16 animtileframeand    = 3;
Uint16 animtilebaseand     = (Uint16)(~3);
Uint16 biganimtileframeand = 7;
Uint16 biganimtilebaseand  = (Uint16)(~7);
Uint16 animtileframeadd    = 0;

char  loadname[256];        // Text

int     brushsize = 3;      // Size of raise/lower terrain brush
int     brushamount = 50;   // Amount of raise/lower

#define MAXPOINTSIZE 16

SDL_Surface * theSurface = NULL;
SDL_Surface * bmphitemap;        // Heightmap image
SDL_Surface * bmpcursor;         // Cursor image

glTexture     tx_point;      // Vertex image
glTexture     tx_pointon;    // Vertex image ( select_vertsed )
glTexture     tx_ref;        // Meshfx images
glTexture     tx_drawref;    //
glTexture     tx_anim;       //
glTexture     tx_water;      //
glTexture     tx_wall;       //
glTexture     tx_impass;     //
glTexture     tx_damage;     //
glTexture     tx_slippy;     //

glTexture     tx_smalltile[MAXTILE]; // Tiles
glTexture     tx_bigtile[MAXTILE];   //
glTexture     tx_tinysmalltile[MAXTILE]; // Plan tiles
glTexture     tx_tinybigtile[MAXTILE];   //

int     numsmalltile = 0;   //
int     numbigtile = 0;     //

int     numpointsonscreen = 0;
Uint32  pointsonscreen[MAXPOINTS];

int     numselect_verts = 0;
Uint32  select_verts[MAXSELECT];

float   debugx = -1;        // Blargh
float   debugy = -1;        //

struct s_mouse_data
{
    int     which_win;    // More mouse_ data
    int     x;          //
    int     y;          //

    Uint16  mode;       // Window mode
    int     onfan;      // Fan mouse is on
    Uint8   tx;         // Tile texture
    Uint8   fx;         // Tile effects
    Uint8   type;       // Tile fantype
    Uint8   upper;      // Tile upper bits
    Uint16  presser;    // Random add for tiles

    int     rect;       // Rectangle drawing
    int     rectx;      //
    int     recty;      //
};
typedef struct s_mouse_data mouse_data_t;

mouse_data_t mdata =
{
    -1,         // which_win
    -1,         // x
    -1,         // y
    0,          // mode
    0,          // onfan
    0,          // tile
    0,          // presser
    0,          // type
    MPDFX_SHA, // fx
    0,          // rect
    0,          // rectx
    0           // recty
};

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
typedef struct s_window window_t;

static window_t window_lst[MAXWIN];

int     keydelay = 0;       //


Uint32  atvertex = 0;           // Current vertex check for new
Uint32  numfreevertices = 0;        // Number of free vertices

mesh_t mesh;

#include "standard.c"           // Some functions that I always use

void sdlinit( int argc, char **argv );
void read_mouse();
void draw_sprite(SDL_Surface * dst, SDL_Surface * sprite, int x, int y );
void line(ogl_surface * surf, int x0, int y0, int x1, int y1, Uint32 c);
bool_t SDL_RectIntersect(SDL_Rect * src, SDL_Rect * dst, SDL_Rect * isect );

int           cartman_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
int           cartman_BlitScreen(SDL_Surface * bmp, SDL_Rect * rect);
SDL_Surface * cartman_CreateSurface(int w, int h);
SDL_Surface * cartman_LoadIMG(const char * szName);

void draw_top_tile( float x, float y, int fan, glTexture * tx_tile, bool_t draw_tile );

void ogl_draw_box( int x, int y, int w, int h, float color[] );

void ogl_beginFrame();
void ogl_endFrame();
void ogl_draw_sprite( glTexture * sprite, int x, int y, int width, int height );

void cartman_begin_ortho_camera( window_t * pwin, camera_t * pcam  );
void cartman_end_ortho_camera( );


//--------------------------------------------------------------------------------------------
void do_cursor()
{
    // This function implements a mouse cursor
    ui.cur_x = mos.x;  if ( ui.cur_x < 6 )  ui.cur_x = 6;  if ( ui.cur_x > ui.scr.x - 6 )  ui.cur_x = ui.scr.x - 6;
    ui.cur_y = mos.y;  if ( ui.cur_y < 6 )  ui.cur_y = 6;  if ( ui.cur_y > ui.scr.y - 6 )  ui.cur_y = ui.scr.y - 6;
    ui.clicked = bfalse;
    if ( mos.button[0] && !ui.pressed )
    {
        ui.clicked = btrue;
    }
    ui.pressed = mos.button[0];
}

//------------------------------------------------------------------------------
void add_light(int x, int y, int radius, int level)
{
    if (numlight >= MAXLIGHT)  numlight = MAXLIGHT - 1;

    light_lst[numlight].x = x;
    light_lst[numlight].y = y;
    light_lst[numlight].radius = radius;
    light_lst[numlight].level = level;
    numlight++;
}

//------------------------------------------------------------------------------
void alter_light(int x, int y)
{
    int radius, level;

    numlight--;
    if (numlight < 0)  numlight = 0;
    radius = abs(light_lst[numlight].x - x);
    level = abs(light_lst[numlight].y - y);
    if (radius > MAXRADIUS)  radius = MAXRADIUS;
    if (radius < MINRADIUS)  radius = MINRADIUS;
    light_lst[numlight].radius = radius;
    if (level > MAXLEVEL) level = MAXLEVEL;
    if (level < MINLEVEL) level = MINLEVEL;
    light_lst[numlight].level = level;
    numlight++;
}

//------------------------------------------------------------------------------
void draw_light( int number, window_t * pwin )
{
    int xdraw, ydraw, radius;
    Uint8 color;

    xdraw = (light_lst[number].x / FOURNUM) - cam.x + (pwin->surfacex >> 1) - SMALLXY;
    ydraw = (light_lst[number].y / FOURNUM) - cam.y + (pwin->surfacey >> 1) - SMALLXY;
    radius = abs(light_lst[number].radius) / FOURNUM;
    color = light_lst[number].level >> 3;

    //color = MAKE_BGR(pwin->bmp, color, color, color);
    //circle(pwin->bmp, xdraw, ydraw, radius, color);
}

//------------------------------------------------------------------------------
int dist_from_border(int x, int y)
{
    if (x > (mesh.edgex >> 1))
        x = mesh.edgex - x - 1;
    if (y > (mesh.edgey >> 1))
        y = mesh.edgey - y - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x < y)
        return x;
    return y;
}

//------------------------------------------------------------------------------
int dist_from_edge(int x, int y)
{
    if (x > (mesh.tiles_x >> 1))
        x = mesh.tiles_x - x - 1;
    if (y > (mesh.tiles_y >> 1))
        y = mesh.tiles_y - y - 1;
    if (x < y)
        return x;
    return y;
}

//------------------------------------------------------------------------------
int get_fan(int x, int y)
{
    int fan = -1;
    if (y >= 0 && y < MAXMESHTILEY && y < mesh.tiles_y)
    {
        if (x >= 0 && x < MAXMESHTILEY && x < mesh.tiles_x)
        {
            fan = mesh.fanstart[y] + x;
        }
    }
    return fan;
};

//------------------------------------------------------------------------------
int fan_is_floor(int x, int y)
{
    int fan = get_fan(x, y);
    if (-1 == fan) return 0;

    return 0 == (mesh.fx[fan]&48);
}

//------------------------------------------------------------------------------
void set_barrier_height(int x, int y, int bits)
{
    Uint32 fantype, fan, vert, vert_count;
    int cnt, noedges;
    float bestprox, prox, tprox, scale, max_height, min_height;

    bool_t floor_mx, floor_px, floor_my, floor_py;
    bool_t floor_mxmy, floor_mxpy, floor_pxmy, floor_pxpy;

    fan = get_fan(x, y);
    if (-1 == fan) return;

    // bust be a MPDFX_WALL
    if ( 0 == (mesh.fx[fan]&bits) ) return;

    floor_mx   = fan_is_floor(x - 1, y);
    floor_px   = fan_is_floor(x + 1, y);
    floor_my   = fan_is_floor(x, y - 1);
    floor_py   = fan_is_floor(x, y + 1);
    noedges = !(floor_mx || floor_px || floor_my || floor_py);

    floor_mxmy = fan_is_floor(x - 1, y - 1);
    floor_mxpy = fan_is_floor(x - 1, y + 1);
    floor_pxmy = fan_is_floor(x + 1, y - 1);
    floor_pxpy = fan_is_floor(x + 1, y + 1);

    fantype    = mesh.fantype[fan];
    vert_count = mesh.command[fantype].numvertices;

    vert       = mesh.vrtstart[fan];

    min_height = mesh.vrtz[vert];
    max_height = mesh.vrtz[vert];
    vert       = mesh.vrtnext[vert];
    for (cnt = 1; cnt < vert_count; cnt++ )
    {
        min_height = MIN( min_height, mesh.vrtz[vert] );
        max_height = MAX( max_height, mesh.vrtz[vert] );
        vert       = mesh.vrtnext[vert];
    }

    vert = mesh.vrtstart[fan];
    for (cnt = 0; cnt < vert_count; cnt++ )
    {
        float ftmp;

        bestprox = NEARHI; // 2.0f / 3.0f * (NEARHI - NEARLOW);
        if (floor_px)
        {
            prox = NEARHI - mesh.command[fantype].x[cnt];
            if (prox < bestprox) bestprox = prox;
        }
        if (floor_py)
        {
            prox = NEARHI - mesh.command[fantype].y[cnt];
            if (prox < bestprox) bestprox = prox;
        }
        if (floor_mx)
        {
            prox = mesh.command[fantype].x[cnt] - NEARLOW;
            if (prox < bestprox) bestprox = prox;
        }
        if (floor_my)
        {
            prox = mesh.command[fantype].y[cnt] - NEARLOW;
            if (prox < bestprox) bestprox = prox;
        }
        if (noedges)
        {
            // Surrounded by walls on all 4 sides, but it may be a corner piece
            if (floor_pxpy)
            {
                prox  = NEARHI - mesh.command[fantype].x[cnt];
                tprox = NEARHI - mesh.command[fantype].y[cnt];
                if (tprox > prox) prox = tprox;
                if (prox < bestprox) bestprox = prox;
            }
            if (floor_pxmy)
            {
                prox = NEARHI - mesh.command[fantype].x[cnt];
                tprox = mesh.command[fantype].y[cnt] - NEARLOW;
                if (tprox > prox) prox = tprox;
                if (prox < bestprox) bestprox = prox;
            }
            if (floor_mxpy)
            {
                prox = mesh.command[fantype].x[cnt] - NEARLOW;
                tprox = NEARHI - mesh.command[fantype].y[cnt];
                if (tprox > prox) prox = tprox;
                if (prox < bestprox) bestprox = prox;
            }
            if (floor_mxmy)
            {
                prox = mesh.command[fantype].x[cnt] - NEARLOW;
                tprox = mesh.command[fantype].y[cnt] - NEARLOW;
                if (tprox > prox) prox = tprox;
                if (prox < bestprox) bestprox = prox;
            }
        }
        //scale = window_lst[mdata.which_win].surfacey - (mdata.y / FOURNUM);
        //bestprox = bestprox * scale * BARRIERHEIGHT / window_lst[mdata.which_win].surfacey;

        //if (bestprox > mesh.edgez) bestprox = mesh.edgez;
        //if (bestprox < 0) bestprox = 0;

        ftmp = bestprox / 128.0f;
        ftmp = 1.0f - ftmp;
        ftmp *= ftmp * ftmp;
        ftmp = 1.0f - ftmp;

        mesh.vrtz[vert] = ftmp * (max_height - min_height) + min_height;
        vert = mesh.vrtnext[vert];
    }
}

//------------------------------------------------------------------------------
void fix_walls()
{
    int x, y;

    for (y = 0; y < mesh.tiles_y; y++)
    {
        for (x = 0; x < mesh.tiles_x; x++)
        {
            set_barrier_height(x, y, MPDFX_WALL | MPDFX_IMPASS);
        }
    }
}

//------------------------------------------------------------------------------
void impass_edges(int amount)
{
    int x, y;
    int fan;

    for (y = 0; y < mesh.tiles_y; y++)
    {
        for(x = 0; x < mesh.tiles_x; x++)
        {
            if (dist_from_edge(x, y) < amount)
            {
                fan = get_fan(x, y);
                if (-1 == fan) continue;

                mesh.fx[fan] |= MPDFX_IMPASS;
            }
        }
    }
}

//------------------------------------------------------------------------------
Uint8 get_twist(int x, int y)
{
    Uint8 twist;

    // x and y should be from -7 to 8
    if (x < -7) x = -7;
    if (x > 8) x = 8;
    if (y < -7) y = -7;
    if (y > 8) y = 8;

    // Now between 0 and 15
    x = x + 7;
    y = y + 7;
    twist = (y << 4) + x;

    return twist;
}

//------------------------------------------------------------------------------
Uint8 get_fan_twist(Uint32 fan)
{
    int zx, zy, vt0, vt1, vt2, vt3;
    Uint8 twist;

    vt0 = mesh.vrtstart[fan];
    vt1 = mesh.vrtnext[vt0];
    vt2 = mesh.vrtnext[vt1];
    vt3 = mesh.vrtnext[vt2];

    zx = (mesh.vrtz[vt0] + mesh.vrtz[vt3] - mesh.vrtz[vt1] - mesh.vrtz[vt2]) / SLOPE;
    zy = (mesh.vrtz[vt2] + mesh.vrtz[vt3] - mesh.vrtz[vt0] - mesh.vrtz[vt1]) / SLOPE;

    twist = get_twist(zx, zy);

    return twist;
}

//------------------------------------------------------------------------------
int get_level(int x, int y)
{
    int fan;
    int z0, z1, z2, z3;         // Height of each fan corner
    int zleft, zright, zdone;   // Weighted height of each side

    zdone = 0;
    fan = get_fan(x >> 7, y >> 7);
    if (-1 != fan)
    {
        x = x & 127;
        y = y & 127;

        z0 = mesh.vrtz[mesh.vrtstart[fan] + 0];
        z1 = mesh.vrtz[mesh.vrtstart[fan] + 1];
        z2 = mesh.vrtz[mesh.vrtstart[fan] + 2];
        z3 = mesh.vrtz[mesh.vrtstart[fan] + 3];

        zleft = (z0 * (128 - y) + z3 * y) >> 7;
        zright = (z1 * (128 - y) + z2 * y) >> 7;
        zdone = (zleft * (128 - x) + zright * x) >> 7;
    }

    return (zdone);
}

//------------------------------------------------------------------------------
void make_hitemap(void)
{
    int x, y, pixx, pixy, level, fan;

    if (bmphitemap) SDL_FreeSurface(bmphitemap);

    bmphitemap = cartman_CreateSurface(mesh.tiles_x << 2, mesh.tiles_y << 2);
    if (NULL == bmphitemap) return;

    y = 16;
    pixy = 0;
    while (pixy < (mesh.tiles_y << 2))
    {
        x = 16;
        pixx = 0;
        while (pixx < (mesh.tiles_x << 2))
        {
            level = (get_level(x, y) * 255 / mesh.edgez);  // level is 0 to 255
            if (level > 252) level = 252;
            fan = get_fan(pixx >> 2, pixy);
            level = 255;
            if (-1 != fan)
            {
                if (mesh.fx[fan]&16) level = 253;        // Wall
                if (mesh.fx[fan]&32) level = 254;        // Impass
                if ((mesh.fx[fan]&48) == 48) level = 255;   // Both
            }
            SDL_PutPixel(bmphitemap, pixx, pixy, level);
            x += 32;
            pixx++;
        }
        y += 32;
        pixy++;
    }
}

//------------------------------------------------------------------------------
glTexture * tiny_tile_at(int x, int y)
{
    Uint16 tx_bits, basetile;
    Uint8 fantype, fx;
    int fan;

    if (x < 0 || x >= mesh.tiles_x || y < 0 || y >= mesh.tiles_y)
    {
        return NULL;
    }

    tx_bits = 0;
    fantype = 0;
    fx = 0;
    fan = get_fan(x, y);
    if (-1 != fan)
    {
        if ( TILE_IS_FANOFF(mesh.tx_bits[fan]) )
        {
            return NULL;
        }
        tx_bits = mesh.tx_bits[fan];
        fantype = mesh.fantype[fan];
        fx      = mesh.fx[fan];
    }

    if (fx&MPDFX_ANIM)
    {
        animtileframeadd = (timclock >> 3) & 3;
        if (fantype >= (MAXMESHTYPE >> 1))
        {
            // Big tiles
            basetile = tx_bits & biganimtilebaseand;// Animation set
            tx_bits += (animtileframeadd << 1);     // Animated tx_bits
            tx_bits = (tx_bits & biganimtileframeand) + basetile;
        }
        else
        {
            // Small tiles
            basetile = tx_bits & animtilebaseand;// Animation set
            tx_bits += animtileframeadd;       // Animated tx_bits
            tx_bits = (tx_bits & animtileframeand) + basetile;
        }
    }

    // remove any of the upper bit information
    tx_bits &= 0xFF;

    if (fantype >= (MAXMESHTYPE >> 1))
    {
        return tx_bigtile + tx_bits;
    }
    else
    {
        return tx_smalltile + tx_bits;
    }
}

//------------------------------------------------------------------------------
void make_planmap(void)
{
    int x, y, putx, puty;
    //SDL_Surface* bmptemp;

    //bmptemp = cartman_CreateSurface(64, 64);
    //if(NULL != bmptemp)  return;

    if (NULL == bmphitemap) SDL_FreeSurface(bmphitemap);
    bmphitemap = cartman_CreateSurface(mesh.tiles_x * TINYXY, mesh.tiles_y * TINYXY);
    if (NULL == bmphitemap) return;

    SDL_FillRect(bmphitemap, NULL, MAKE_BGR(bmphitemap, 0, 0, 0));

    puty = 0;
    for ( y = 0; y < mesh.tiles_y; y++ )
    {
        putx = 0;
        for (x = 0; x < mesh.tiles_x; x++)
        {
            glTexture * tx_tile;
            tx_tile = tiny_tile_at(x, y);

            if( NULL != tx_tile )
            {
                SDL_Rect dst = {putx, puty, TINYXY, TINYXY};
                cartman_BlitSurface( tx_tile->surface, NULL, bmphitemap, &dst);
            }
            putx += TINYXY;
        }
        puty += TINYXY;
    }

    //SDL_SoftStretch(bmphitemap, NULL, bmptemp, NULL);
    //SDL_FreeSurface(bmphitemap);

    //bmphitemap = bmptemp;
}

//------------------------------------------------------------------------------
void draw_cursor_in_window( window_t * pwin )
{
    int x, y;

    if ( NULL == pwin || !pwin->on ) return;

    if ( -1 != mdata.which_win && pwin->id != mdata.which_win )
    {
        //if ( (window_lst[mdata.which_win].mode & WINMODE_SIDE) == (pwin->mode & WINMODE_SIDE) )
        //{
        int size = POINT_SIZE(10);

        x = pwin->x + (mos.x - window_lst[mdata.which_win].x);
        y = pwin->y + (mos.y - window_lst[mdata.which_win].y);

        ogl_draw_sprite( &tx_pointon, x - size / 2, y - size / 2, size, size );
        //}
    }

}

//------------------------------------------------------------------------------
int get_vertex(int x, int y, int num)
{
    // ZZ> This function gets a vertex number or -1
    int vert, cnt;
    int fan;

    vert = -1;
    fan = get_fan(x, y);
    if (-1 != fan)
    {
        if (mesh.command[mesh.fantype[fan]].numvertices > num)
        {
            vert = mesh.vrtstart[fan];
            cnt = 0;
            while (cnt < num)
            {
                vert = mesh.vrtnext[vert];
                if (vert == -1)
                {
                    printf("BAD GET_VERTEX NUMBER(2nd), %d at %d, %d...\n", num, x, y);
                    printf("%d VERTICES ALLOWED...\n\n", mesh.command[mesh.fantype[fan]].numvertices);
                    exit(-1);
                }
                cnt++;
            }
        }
    }

    return vert;
}

//------------------------------------------------------------------------------
int nearest_vertex(int x, int y, float nearx, float neary)
{
    // ZZ> This function gets a vertex number or -1
    int vert, bestvert, cnt;
    int fan;
    int num;
    float prox, proxx, proxy, bestprox;

    bestvert = -1;
    fan = get_fan(x, y);
    if (-1 != fan)
    {
        num = mesh.command[mesh.fantype[fan]].numvertices;
        vert = mesh.vrtstart[fan];
        vert = mesh.vrtnext[vert];
        vert = mesh.vrtnext[vert];
        vert = mesh.vrtnext[vert];
        vert = mesh.vrtnext[vert];
        bestprox = 9000;
        cnt = 4;
        while (cnt < num)
        {
            proxx = mesh.command[mesh.fantype[fan]].x[cnt] - nearx;
            proxy = mesh.command[mesh.fantype[fan]].y[cnt] - neary;
            if (proxx < 0) proxx = -proxx;
            if (proxy < 0) proxy = -proxy;
            prox = proxx + proxy;
            if (prox < bestprox)
            {
                bestvert = vert;
                bestprox = prox;
            }
            vert = mesh.vrtnext[vert];
            cnt++;
        }
    }
    return bestvert;
}

//------------------------------------------------------------------------------
void weld_select()
{
    // ZZ> This function welds the highlighted vertices
    int cnt, x, y, z, a;
    Uint32 vert;

    if (numselect_verts > 1)
    {
        x = 0;
        y = 0;
        z = 0;
        a = 0;
        cnt = 0;
        while (cnt < numselect_verts)
        {
            vert = select_verts[cnt];
            x += mesh.vrtx[vert];
            y += mesh.vrty[vert];
            z += mesh.vrtz[vert];
            a += mesh.vrta[vert];
            cnt++;
        }
        x += cnt >> 1;  y += cnt >> 1;
        x = x / numselect_verts;
        y = y / numselect_verts;
        z = z / numselect_verts;
        a = a / numselect_verts;
        cnt = 0;
        while (cnt < numselect_verts)
        {
            vert = select_verts[cnt];
            mesh.vrtx[vert] = x;
            mesh.vrty[vert] = y;
            mesh.vrtz[vert] = z;
            mesh.vrta[vert] = a;
            cnt++;
        }
    }
}

//------------------------------------------------------------------------------
void add_select(int vert)
{
    // ZZ> This function highlights a vertex
    int cnt, found;

    if (numselect_verts < MAXSELECT && vert >= 0)
    {
        found = bfalse;
        cnt = 0;
        while (cnt < numselect_verts && !found)
        {
            if (select_verts[cnt] == vert)
            {
                found = btrue;
            }
            cnt++;
        }
        if (!found)
        {
            select_verts[numselect_verts] = vert;
            numselect_verts++;
        }
    }
}

//------------------------------------------------------------------------------
void clear_select(void)
{
    // ZZ> This function unselects all vertices
    numselect_verts = 0;
}

//------------------------------------------------------------------------------
int vert_selected(int vert)
{
    // ZZ> This function returns btrue if the vertex has been highlighted by user
    int cnt;

    cnt = 0;
    while (cnt < numselect_verts)
    {
        if (vert == select_verts[cnt])
        {
            return btrue;
        }
        cnt++;
    }

    return bfalse;
}

//------------------------------------------------------------------------------
void remove_select(int vert)
{
    // ZZ> This function makes sure the vertex is not highlighted
    int cnt, stillgoing;

    cnt = 0;
    stillgoing = btrue;
    while (cnt < numselect_verts && stillgoing)
    {
        if (vert == select_verts[cnt])
        {
            stillgoing = bfalse;
        }
        cnt++;
    }
    if (stillgoing == bfalse)
    {
        while (cnt < numselect_verts)
        {
            select_verts[cnt-1] = select_verts[cnt];
            cnt++;
        }
        numselect_verts--;
    }
}

//------------------------------------------------------------------------------
void fan_onscreen(Uint32 fan)
{
    // ZZ> This function flags a fan's points as being "onscreen"
    int cnt;
    Uint32 vert;

    vert = mesh.vrtstart[fan];
    cnt = 0;
    while (cnt < mesh.command[mesh.fantype[fan]].numvertices)
    {
        pointsonscreen[numpointsonscreen] = vert;  numpointsonscreen++;
        vert = mesh.vrtnext[vert];
        cnt++;
    }
}

//------------------------------------------------------------------------------
void make_onscreen(void)
{
    int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
    int fan;

    numpointsonscreen = 0;
    mapxstt = (cam.x - (ONSIZE >> 1)) / TILEDIV;
    mapystt = (cam.y - (ONSIZE >> 1)) / TILEDIV;
    numx = (ONSIZE / TILEDIV) + 3;
    numy = (ONSIZE / TILEDIV) + 3;
    x = -cam.x + (ONSIZE >> 1) - TILEDIV;
    y = -cam.y + (ONSIZE >> 1) - TILEDIV;

    mapy = mapystt;
    cnty = 0;
    while (cnty < numy)
    {
        if (mapy >= 0 && mapy < mesh.tiles_y)
        {
            mapx = mapxstt;
            cntx = 0;
            while (cntx < numx)
            {
                fan = get_fan(mapx, mapy);
                if (-1 != fan)
                {
                    fan_onscreen(fan);
                }
                mapx++;
                cntx++;
            }
        }
        mapy++;
        cnty++;
    }
}

//------------------------------------------------------------------------------
void draw_top_fan( window_t * pwin, int fan, int x, int y )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     A wireframe tile from a vertex connection window
    Uint32 faketoreal[MAXMESHVERTICES];
    int fantype;
    int cnt, stt, end, vert;
    float color[4];
    int size;

    if ( -1 == fan ) return;

    fantype = mesh.fantype[fan];

    OGL_MAKE_COLOR_4(color, 32, 16, 16, 31);
    if (fantype >= MAXMESHTYPE / 2)
    {
        OGL_MAKE_COLOR_4(color, 32, 31, 16, 16);
    }

    vert = mesh.vrtstart[fan];
    for (cnt = 0; cnt < mesh.command[fantype].numvertices; cnt++)
    {
        faketoreal[cnt] = vert;
        vert = mesh.vrtnext[vert];
    }

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {
            for (cnt = 0; cnt < mesh.numline[fantype]; cnt++)
            {
                stt = faketoreal[mesh.line[fantype].start[cnt]];
                end = faketoreal[mesh.line[fantype].end[cnt]];

                glVertex2f( mesh.vrtx[stt] + x, mesh.vrty[stt] + y );
                glVertex2f( mesh.vrtx[end] + x, mesh.vrty[end] + y );

            }
        }
        glEnd();
    }
    glPopAttrib();

    glColor4f( 1, 1, 1, 1 );
    for (cnt = 0; cnt < mesh.command[fantype].numvertices; cnt++)
    {
        int point_size;

        vert = faketoreal[cnt];

        size = (mesh.vrtz[vert] << 4) / (mesh.edgez + 1);
        if (size < 0) size = 0;
        if (size >= MAXPOINTSIZE) size = 15;

        point_size = POINT_SIZE( size );

        if (point_size > 0)
        {
            glTexture * tx_tmp;

            if ( vert_selected(vert) )
            {
                tx_tmp = &tx_pointon;
            }
            else
            {
                tx_tmp = &tx_point;
            }

            ogl_draw_sprite( tx_tmp, mesh.vrtx[vert] + x - point_size / 2, mesh.vrty[vert] + y - point_size / 2, point_size, point_size );
        }
    }
}

//------------------------------------------------------------------------------
void draw_side_fan(window_t * pwin, int fan, int x, int y)
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     A wireframe tile from a vertex connection window ( Side view )
    Uint32 faketoreal[MAXMESHVERTICES];
    int fantype;
    int cnt, stt, end, vert;
    float color[4];
    int size;
    int point_size;

    fantype = mesh.fantype[fan];

    OGL_MAKE_COLOR_4(color, 32, 16, 16, 31);
    if (fantype >= MAXMESHTYPE / 2)
    {
        OGL_MAKE_COLOR_4(color, 32, 31, 16, 16);
    }

    vert = mesh.vrtstart[fan];
    cnt = 0;
    while (cnt < mesh.command[fantype].numvertices)
    {
        faketoreal[cnt] = vert;
        vert = mesh.vrtnext[vert];
        cnt++;
    }

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {
            for (cnt = 0; cnt < mesh.numline[fantype]; cnt++)
            {
                stt = faketoreal[mesh.line[fantype].start[cnt]];
                end = faketoreal[mesh.line[fantype].end[cnt]];


                glVertex2i( mesh.vrtx[stt] + x, -(mesh.vrtz[stt] >> 4) + y );
                glVertex2i( mesh.vrtx[end] + x, -(mesh.vrtz[end] >> 4) + y );

            }
        }
        glEnd();
    }
    glPopAttrib();

    size = 5;
    point_size = POINT_SIZE(size);

    glColor4f( 1, 1, 1, 1 );

    for (cnt = 0; cnt < mesh.command[fantype].numvertices; cnt++ )
    {
        glTexture * ptmp = NULL;

        vert = faketoreal[cnt];

        if (vert_selected(vert))
        {
            ptmp = &tx_pointon;
        }
        else
        {
            ptmp = &tx_point;
        }

        ogl_draw_sprite( ptmp, mesh.vrtx[vert] + x - point_size / 2,
                         -(mesh.vrtz[vert] >> 4) + y - point_size / 2, point_size, point_size);

    }
}

//------------------------------------------------------------------------------
void draw_schematic(window_t * pwin, int fantype, int x, int y)
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     The wireframe on the left side of the theSurface.
    int cnt, stt, end;
    float color[4];

    OGL_MAKE_COLOR_4(color, 32, 16, 16, 31);
    if (fantype >= MAXMESHTYPE / 2)
    {
        OGL_MAKE_COLOR_4(color, 32, 31, 16, 16);
    };

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {

            for ( cnt = 0; cnt < mesh.numline[fantype]; cnt++ )
            {
                stt = mesh.line[fantype].start[cnt];
                end = mesh.line[fantype].end[cnt];

                glVertex2i( mesh.command[fantype].x[stt] + x, mesh.command[fantype].y[stt] + y );
                glVertex2i( mesh.command[fantype].x[end] + x, mesh.command[fantype].y[end] + y );

            }
        }
        glEnd();
    }
    glPopAttrib();
}

//------------------------------------------------------------------------------
void add_line(int fantype, int start, int end)
{
    // ZZ> This function adds a line to the vertex schematic
    int cnt;

    if( fantype < 0 || fantype >= MAXMESHTYPE ) return;

    if( mesh.numline[fantype] >= MAXMESHTYPE ) return;

    // Make sure line isn't already in list
    for (cnt = 0; cnt < mesh.numline[fantype]; cnt++)
    {
        if ((mesh.line[fantype].start[cnt] == start && mesh.line[fantype].end[cnt] == end) ||
            (mesh.line[fantype].end[cnt] == start && mesh.line[fantype].start[cnt] == end))
        {
            return;
        }
    }

    // Add it in
    cnt = mesh.numline[fantype];
    mesh.line[fantype].start[cnt] = start;
    mesh.line[fantype].end[cnt]   = end;
    mesh.numline[fantype]++;
}

//------------------------------------------------------------------------------
void load_mesh_fans()
{
    // ZZ> This function loads fan types for the mesh...  Starting vertex
    //     positions and number of vertices
    int cnt, entry;
    int numfantype, fantype, bigfantype, vertices;
    int numcommand, command, command_size;
    int itmp;
    float ftmp;
    FILE* fileread;
    STRING fname;

    // Initialize all mesh types to 0
    for (entry = 0; entry < MAXMESHTYPE; entry++)
    {
        mesh.numline[entry] = 0;
        mesh.command[entry].numvertices = 0;
    }

    // Open the file and go to it
    sprintf( fname, "%s" SLASH_STR "basicdat" SLASH_STR "fans.txt", egoboo_path );
    fileread = fopen( fname, "r");
    if (NULL == fileread)
    {
        log_error("load_mesh_fans() - Cannot find fans.txt file\n");
    }

    goto_colon(fileread);
    fscanf(fileread, "%d", &numfantype);

    for (fantype = 0, bigfantype = MAXMESHTYPE / 2; fantype < numfantype; fantype++, bigfantype++ )
    {
        goto_colon(fileread);
        fscanf(fileread, "%d", &vertices);
        mesh.command[fantype].numvertices = vertices;
        mesh.command[fantype+MAXMESHTYPE/2].numvertices = vertices;  // DUPE

        for ( cnt = 0; cnt < vertices; cnt++ )
        {
            // lighting "ref" data
            goto_colon(fileread);
            fscanf(fileread, "%d", &itmp);
            mesh.command[fantype].ref[cnt] = itmp;
            mesh.command[fantype+MAXMESHTYPE/2].ref[cnt] = itmp;  // DUPE

            // texure u data and mesh x data
            goto_colon(fileread);
            fscanf(fileread, "%f", &ftmp);

            mesh.command[fantype].u[cnt] = ftmp;
            mesh.command[fantype+MAXMESHTYPE/2].u[cnt] = ftmp;  // DUPE

            mesh.command[fantype].x[cnt] = (ftmp) * 128;
            mesh.command[fantype+MAXMESHTYPE/2].x[cnt] = (ftmp) * 128;  // DUPE

            // texure v data and mesh y data
            goto_colon(fileread);
            fscanf(fileread, "%f", &ftmp);
            mesh.command[fantype].v[cnt] = ftmp;
            mesh.command[fantype+MAXMESHTYPE/2].v[cnt] = ftmp;  // DUPE

            mesh.command[fantype].y[cnt] = (ftmp) * 128;
            mesh.command[fantype+MAXMESHTYPE/2].y[cnt] = (ftmp) * 128;  // DUPE
        }


        // Get the vertex connections
        goto_colon(fileread);
        fscanf(fileread, "%d", &numcommand);
        mesh.command[fantype].count = numcommand;
        mesh.command[bigfantype].count = numcommand;  // Dupe

        entry = 0;
        for ( command = 0; command < numcommand; command++ )
        {
            // grab the fan vertex data
            goto_colon(fileread);
            fscanf(fileread, "%d", &command_size);
            mesh.command[fantype].size[command] = command_size;
            mesh.command[bigfantype].size[command] = command_size;  // Dupe

            for ( cnt = 0; cnt < command_size; cnt++ )
            {
                goto_colon( fileread );
                fscanf( fileread, "%d", &itmp );
                mesh.command[fantype].vrt[entry] = itmp;
                mesh.command[bigfantype].vrt[entry] = itmp;  // Dupe
                entry++;
            }
        }
    }
    fclose(fileread);

    for (fantype = 0, bigfantype = MAXMESHTYPE / 2; fantype < numfantype; fantype++, bigfantype++ )
    {
        int inow, ilast, fancenter;

        entry = 0;
        numcommand = mesh.command[fantype].count;
        for ( command = 0; command < numcommand; command++ )
        {
            command_size = mesh.command[fantype].size[command];

            // convert the fan data into lines representing the fan edges
            fancenter = mesh.command[fantype].vrt[entry++];
            inow      = mesh.command[fantype].vrt[entry++];
            for (cnt = 2; cnt < command_size; cnt++, entry++)
            {
                ilast = inow;
                inow = mesh.command[fantype].vrt[entry];

                add_line(fantype, fancenter, inow);
                add_line(fantype, fancenter, ilast);
                add_line(fantype, ilast,     inow);

                add_line(bigfantype, fancenter, inow);
                add_line(bigfantype, fancenter, ilast);
                add_line(bigfantype, ilast,     inow);
            }
        }
    }


    // Correct all of them silly texture positions for seamless tiling
    for ( entry = 0; entry < MAXMESHTYPE / 2; entry++  )
    {
        for ( cnt = 0; cnt < mesh.command[entry].numvertices; cnt++ )
        {
            mesh.command[entry].u[cnt] = ( 0.6f / SMALLXY ) + ( mesh.command[entry].u[cnt] * (SMALLXY - 2 * 0.6f) / SMALLXY );
            mesh.command[entry].v[cnt] = ( 0.6f / SMALLXY ) + ( mesh.command[entry].v[cnt] * (SMALLXY - 2 * 0.6f) / SMALLXY );
        }
    }

    // Do for big tiles too
    for ( /*nothing*/; entry < MAXMESHTYPE; entry++ )
    {
        for ( cnt = 0; cnt < mesh.command[entry].numvertices; cnt++ )
        {
            mesh.command[entry].u[cnt] = ( 0.6f / BIGXY ) + ( mesh.command[entry].u[cnt] * (BIGXY - 2 * 0.6f) / BIGXY );
            mesh.command[entry].v[cnt] = ( 0.6f / BIGXY ) + ( mesh.command[entry].v[cnt] * (BIGXY - 2 * 0.6f) / BIGXY );
        }
    }
}

//------------------------------------------------------------------------------
void free_vertices()
{
    // ZZ> This function sets all vertices to unused
    int cnt;

    cnt = 0;
    while (cnt < MAXTOTALMESHVERTICES)
    {
        mesh.vrta[cnt] = VERTEXUNUSED;
        cnt++;
    }
    atvertex = 0;
    numfreevertices = MAXTOTALMESHVERTICES;
}

//------------------------------------------------------------------------------
int get_free_vertex()
{
    // ZZ> This function returns btrue if it can find an unused vertex, and it
    // will set atvertex to that vertex index.  bfalse otherwise.
    int cnt;

    if (numfreevertices != 0)
    {
        cnt = 0;
        while (cnt < MAXTOTALMESHVERTICES && mesh.vrta[atvertex] != VERTEXUNUSED)
        {
            atvertex++;
            if (atvertex == MAXTOTALMESHVERTICES)
            {
                atvertex = 0;
            }
            cnt++;
        }
        if (mesh.vrta[atvertex] == VERTEXUNUSED)
        {
            mesh.vrta[atvertex] = 60;
            return btrue;
        }
    }
    return bfalse;
}

//------------------------------------------------------------------------------
void remove_fan(int fan)
{
    // ZZ> This function removes a fan's vertices from usage and sets the fan
    //     to not be drawn
    int cnt, vert;
    Uint32 numvert;

    numvert = mesh.command[mesh.fantype[fan]].numvertices;
    vert = mesh.vrtstart[fan];
    cnt = 0;
    while (cnt < numvert)
    {
        mesh.vrta[vert] = VERTEXUNUSED;
        numfreevertices++;
        vert = mesh.vrtnext[vert];
        cnt++;
    }
    mesh.fantype[fan] = 0;
    mesh.fx[fan] = MPDFX_SHA;
}

//------------------------------------------------------------------------------
int add_fan(int fan, int x, int y)
{
    // ZZ> This function allocates the vertices needed for a fan
    int cnt;
    int numvert;
    Uint32 vertex;
    Uint32 vertexlist[17];

    numvert = mesh.command[mesh.fantype[fan]].numvertices;
    if (numfreevertices >= numvert)
    {
        mesh.fx[fan] = MPDFX_SHA;
        cnt = 0;
        while (cnt < numvert)
        {
            if (get_free_vertex() == bfalse)
            {
                // Reset to unused
                numvert = cnt;
                cnt = 0;
                while (cnt < numvert)
                {
                    mesh.vrta[vertexlist[cnt]] = 60;
                    cnt++;
                }
                return bfalse;
            }
            vertexlist[cnt] = atvertex;
            cnt++;
        }
        vertexlist[cnt] = CHAINEND;

        cnt = 0;
        while (cnt < numvert)
        {
            vertex = vertexlist[cnt];
            mesh.vrtx[vertex] = x + (mesh.command[mesh.fantype[fan]].x[cnt] >> 2);
            mesh.vrty[vertex] = y + (mesh.command[mesh.fantype[fan]].y[cnt] >> 2);
            mesh.vrtz[vertex] = 0;
            mesh.vrtnext[vertex] = vertexlist[cnt+1];
            cnt++;
        }
        mesh.vrtstart[fan] = vertexlist[0];
        numfreevertices -= numvert;
        return btrue;
    }
    return bfalse;
}

//------------------------------------------------------------------------------
void num_free_vertex()
{
    // ZZ> This function counts the unused vertices and sets numfreevertices
    int cnt, num;

    num = 0;
    cnt = 0;
    while (cnt < MAXTOTALMESHVERTICES)
    {
        if (mesh.vrta[cnt] == VERTEXUNUSED)
        {
            num++;
        }
        cnt++;
    }
    numfreevertices = num;
}

//------------------------------------------------------------------------------
void make_fanstart()
{
    // ZZ> This function builds a look up table to ease calculating the
    //     fan number given an x,y pair
    int cnt;

    cnt = 0;
    while (cnt < mesh.tiles_y)
    {
        mesh.fanstart[cnt] = mesh.tiles_x * cnt;
        cnt++;
    }
}

//------------------------------------------------------------------------------
glTexture *tile_at( int fan )
{
    Uint16 tx_bits, basetile;
    Uint8 fantype, fx;

    if (fan == -1 || TILE_IS_FANOFF(mesh.tx_bits[fan]))
    {
        return NULL;
    }

    tx_bits = mesh.tx_bits[fan];
    fantype = mesh.fantype[fan];
    fx = mesh.fx[fan];
    if (fx&MPDFX_ANIM)
    {
        animtileframeadd = (timclock >> 3) & 3;
        if (fantype >= (MAXMESHTYPE >> 1))
        {
            // Big tiles
            basetile = tx_bits & biganimtilebaseand;// Animation set
            tx_bits += (animtileframeadd << 1);     // Animated tx_bits
            tx_bits = (tx_bits & biganimtileframeand) + basetile;
        }
        else
        {
            // Small tiles
            basetile = tx_bits & animtilebaseand;  // Animation set
            tx_bits += animtileframeadd;           // Animated tx_bits
            tx_bits = (tx_bits & animtileframeand) + basetile;
        }
    }

    // remove any of the upper bit information
    tx_bits &= 0xFF;

    if (fantype >= (MAXMESHTYPE >> 1))
    {
        return tx_bigtile + tx_bits;
    }
    else
    {
        return tx_smalltile + tx_bits;
    }
}

//------------------------------------------------------------------------------
int fan_at(int x, int y)
{
    return get_fan(x, y);
}

//------------------------------------------------------------------------------
void weld_0(int x, int y)
{
    clear_select();
    add_select(get_vertex(x, y, 0));
    add_select(get_vertex(x - 1, y, 1));
    add_select(get_vertex(x, y - 1, 3));
    add_select(get_vertex(x - 1, y - 1, 2));
    weld_select();
    clear_select();
}

//------------------------------------------------------------------------------
void weld_1(int x, int y)
{
    clear_select();
    add_select(get_vertex(x, y, 1));
    add_select(get_vertex(x + 1, y, 0));
    add_select(get_vertex(x, y - 1, 2));
    add_select(get_vertex(x + 1, y - 1, 3));
    weld_select();
    clear_select();
}

//------------------------------------------------------------------------------
void weld_2(int x, int y)
{
    clear_select();
    add_select(get_vertex(x, y, 2));
    add_select(get_vertex(x + 1, y, 3));
    add_select(get_vertex(x, y + 1, 1));
    add_select(get_vertex(x + 1, y + 1, 0));
    weld_select();
    clear_select();
}

//------------------------------------------------------------------------------
void weld_3(int x, int y)
{
    clear_select();
    add_select(get_vertex(x, y, 3));
    add_select(get_vertex(x - 1, y, 2));
    add_select(get_vertex(x, y + 1, 0));
    add_select(get_vertex(x - 1, y + 1, 1));
    weld_select();
    clear_select();
}

//------------------------------------------------------------------------------
void weld_cnt(int x, int y, int cnt, Uint32 fan)
{
    if (mesh.command[mesh.fantype[fan]].x[cnt] < NEARLOW + 1 ||
        mesh.command[mesh.fantype[fan]].y[cnt] < NEARLOW + 1 ||
        mesh.command[mesh.fantype[fan]].x[cnt] > NEARHI - 1 ||
        mesh.command[mesh.fantype[fan]].y[cnt] > NEARHI - 1)
    {
        clear_select();
        add_select(get_vertex(x, y, cnt));
        if (mesh.command[mesh.fantype[fan]].x[cnt] < NEARLOW + 1)
            add_select(nearest_vertex(x - 1, y, NEARHI, mesh.command[mesh.fantype[fan]].y[cnt]));
        if (mesh.command[mesh.fantype[fan]].y[cnt] < NEARLOW + 1)
            add_select(nearest_vertex(x, y - 1, mesh.command[mesh.fantype[fan]].x[cnt], NEARHI));
        if (mesh.command[mesh.fantype[fan]].x[cnt] > NEARHI - 1)
            add_select(nearest_vertex(x + 1, y, NEARLOW, mesh.command[mesh.fantype[fan]].y[cnt]));
        if (mesh.command[mesh.fantype[fan]].y[cnt] > NEARHI - 1)
            add_select(nearest_vertex(x, y + 1, mesh.command[mesh.fantype[fan]].x[cnt], NEARLOW));
        weld_select();
        clear_select();
    }
}

//------------------------------------------------------------------------------
void fix_corners(int x, int y)
{
    int fan;

    fan = get_fan(x, y);
    if (-1 != fan)
    {
        weld_0(x, y);
        weld_1(x, y);
        weld_2(x, y);
        weld_3(x, y);
    }
}

//------------------------------------------------------------------------------
void fix_vertices(int x, int y)
{
    int fan;
    int cnt;

    fix_corners(x, y);
    fan = get_fan(x, y);
    if ( -1 != fan )
    {
        cnt = 4;
        while (cnt < mesh.command[mesh.fantype[fan]].numvertices)
        {
            weld_cnt(x, y, cnt, fan);
            cnt++;
        }
    }
}

//------------------------------------------------------------------------------
void fix_mesh(void)
{
    // ZZ> This function corrects corners across entire mesh
    int x, y;

    for (y = 0; y < mesh.tiles_y; y++)
    {
        
        for (x = 0; x < mesh.tiles_x; x++)
        {
            //      fix_corners(x, y);
            fix_vertices(x, y);   
        }
    }
}

//------------------------------------------------------------------------------
//char tile_is_different(int x, int y, Uint16 tileset, Uint16 tileand)
//{
//    // ZZ> bfalse if of same set, btrue if different
//    int fan;
//
//    fan = get_fan(x, y);
//    if ( -1 != fan )
//    {
//        return bfalse;
//    }
//    else
//    {
//        if (tileand == 0xC0)
//        {
//            if (mesh.tx_bits[fan] >= 48) return bfalse;
//        }
//
//        if ( tileset == (mesh.tx_bits[fan]&tileand) )
//        {
//            return bfalse;
//        }
//    }
//    return btrue;
//}

//------------------------------------------------------------------------------
//Uint16 trim_code(int x, int y, Uint16 tileset)
//{
//    // ZZ> This function returns the standard tile set value thing...  For
//    //     Trimming tops of walls and floors
//
//    Uint16 code;
//
//    if (tile_is_different(x, y - 1, tileset, 0xF0))
//    {
//        // Top
//        code = 0;
//        if (tile_is_different(x - 1, y, tileset, 0xF0))
//        {
//            // Left
//            code = 8;
//        }
//        if (tile_is_different(x + 1, y, tileset, 0xF0))
//        {
//            // Right
//            code = 9;
//        }
//        return code;
//    }
//
//    if (tile_is_different(x, y + 1, tileset, 0xF0))
//    {
//        // Bottom
//        code = 1;
//        if (tile_is_different(x - 1, y, tileset, 0xF0))
//        {
//            // Left
//            code = 10;
//        }
//        if (tile_is_different(x + 1, y, tileset, 0xF0))
//        {
//            // Right
//            code = 11;
//        }
//        return code;
//    }
//
//    if (tile_is_different(x - 1, y, tileset, 0xF0))
//    {
//        // Left
//        code = 2;
//        return code;
//    }
//    if (tile_is_different(x + 1, y, tileset, 0xF0))
//    {
//        // Right
//        code = 3;
//        return code;
//    }
//
//    if (tile_is_different(x + 1, y + 1, tileset, 0xF0))
//    {
//        // Bottom Right
//        code = 4;
//        return code;
//    }
//    if (tile_is_different(x - 1, y + 1, tileset, 0xF0))
//    {
//        // Bottom Left
//        code = 5;
//        return code;
//    }
//    if (tile_is_different(x + 1, y - 1, tileset, 0xF0))
//    {
//        // Top Right
//        code = 6;
//        return code;
//    }
//    if (tile_is_different(x - 1, y - 1, tileset, 0xF0))
//    {
//        // Top Left
//        code = 7;
//        return code;
//    }
//
//    code = 255;
//    return code;
//}

//------------------------------------------------------------------------------
//Uint16 wall_code(int x, int y, Uint16 tileset)
//{
//    // ZZ> This function returns the standard tile set value thing...  For
//    //     Trimming tops of walls and floors
//
//    Uint16 code;
//
//    if (tile_is_different(x, y - 1, tileset, 0xC0))
//    {
//        // Top
//        code = (rand() & 2) + 20;
//        if (tile_is_different(x - 1, y, tileset, 0xC0))
//        {
//            // Left
//            code = 48;
//        }
//        if (tile_is_different(x + 1, y, tileset, 0xC0))
//        {
//            // Right
//            code = 50;
//        }
//
//        return code;
//    }
//
//    if (tile_is_different(x, y + 1, tileset, 0xC0))
//    {
//        // Bottom
//        code = (rand() & 2);
//        if (tile_is_different(x - 1, y, tileset, 0xC0))
//        {
//            // Left
//            code = 52;
//        }
//        if (tile_is_different(x + 1, y, tileset, 0xC0))
//        {
//            // Right
//            code = 54;
//        }
//        return code;
//    }
//
//    if (tile_is_different(x - 1, y, tileset, 0xC0))
//    {
//        // Left
//        code = (rand() & 2) + 16;
//        return code;
//    }
//
//    if (tile_is_different(x + 1, y, tileset, 0xC0))
//    {
//        // Right
//        code = (rand() & 2) + 4;
//        return code;
//    }
//
//    if (tile_is_different(x + 1, y + 1, tileset, 0xC0))
//    {
//        // Bottom Right
//        code = 32;
//        return code;
//    }
//    if (tile_is_different(x - 1, y + 1, tileset, 0xC0))
//    {
//        // Bottom Left
//        code = 34;
//        return code;
//    }
//    if (tile_is_different(x + 1, y - 1, tileset, 0xC0))
//    {
//        // Top Right
//        code = 36;
//        return code;
//    }
//    if (tile_is_different(x - 1, y - 1, tileset, 0xC0))
//    {
//        // Top Left
//        code = 38;
//        return code;
//    }
//
//    code = 255;
//    return code;
//}
//
//------------------------------------------------------------------------------
//void trim_mesh_tile(Uint16 tileset, Uint16 tileand)
//{
//    // ZZ> This function trims walls and floors and tops automagically
//    int fan;
//    int x, y, code;
//
//    tileset = tileset & tileand;
//    
//    for (y = 0; y < mesh.tiles_y; y++)
//    {
//        for (x = 0; x < mesh.tiles_x; x++)
//        {
//            fan = get_fan(x, y);
//            if (-1 == fan) continue;
//            
//            if( tileset == (mesh.tx_bits[fan]&tileand) )
//            {
//                if (tileand == 0xC0)
//                {
//                    code = wall_code(x, y, tileset);
//                }
//                else
//                {
//                    code = trim_code(x, y, tileset);
//                }
//
//                if (code != 255)
//                {
//                    mesh.tx_bits[fan] = tileset + code;
//                }
//            }
//        }
//    }
//}

//------------------------------------------------------------------------------
void fx_mesh_tile(Uint16 tileset, Uint16 tileand, Uint8 fx)
{
    // ZZ> This function sets the fx for a group of tiles
    int fan;
    int x, y;

    tileset = tileset & tileand;
    for(y = 0; y < mesh.tiles_y; y++)
    { 
        for (x = 0; x < mesh.tiles_x; x++)
        {
            fan = get_fan(x, y);
            if (-1 == fan) continue;
            
            if( tileset == (mesh.tx_bits[fan]&tileand) )
            {
                mesh.fx[fan] = fx;
            }
        }
    }
}

//------------------------------------------------------------------------------
void set_mesh_tile(Uint16 tiletoset)
{
    // ZZ> This function sets one tile type to another
    int fan;
    int x, y;

    for (y = 0; y < mesh.tiles_y; y++)
    {
        for (x = 0; x < mesh.tiles_x; x++)
        {
            fan = get_fan(x, y);
            if( -1 == fan ) continue;

            if( TILE_IS_FANOFF(mesh.tx_bits[fan]) ) continue;

            if ( tiletoset == mesh.tx_bits[fan] )
            {
                int tx_bits;

                tx_bits = TILE_SET_UPPER_BITS( mdata.upper );
                switch (mdata.presser)
                {
                    case 0:
                        tx_bits |= mdata.tx & 0xFF;
                        break;

                    case 1:
                        tx_bits |= (mdata.tx & 0xFE) + (rand() & 1);
                        break;

                    case 2:
                        tx_bits |= (mdata.tx & 0xFC) + (rand() & 3);
                        break;

                    case 3:
                        tx_bits |= (mdata.tx & 0xF8) + (rand() & 7);
                        break;

                    default:
                        tx_bits = mesh.tx_bits[fan];
                }
                mesh.tx_bits[fan] = tx_bits;                
            }
        }
    }
}

//------------------------------------------------------------------------------
void create_mesh(void)
{
    // ZZ> This function makes the mesh
    int x, y, fan, tile;

    free_vertices();
    printf("Mesh file not found, so creating a new one...\n");

    printf("Number of tiles in X direction ( 32-512 ):  ");
    scanf("%d", &mesh.tiles_x);

    printf("Number of tiles in Y direction ( 32-512 ):  ");
    scanf("%d", &mesh.tiles_y);

    mesh.edgex = (mesh.tiles_x * TILEDIV) - 1;
    mesh.edgey = (mesh.tiles_y * TILEDIV) - 1;
    mesh.edgez = 180 << 4;

    fan = 0;
    
    tile = 0;
    for (y = 0; y < mesh.tiles_y; y++)
    {
        for (x = 0; x < mesh.tiles_x; x++)
        {
            mesh.fantype[fan] = 0;
            mesh.tx_bits[fan] = (((x & 1) + (y & 1)) & 1) + DEFAULT_TILE;

            if ( !add_fan(fan, x*TILEDIV, y*TILEDIV) )
            {
                printf("NOT ENOUGH VERTICES!!!\n\n");
                exit(-1);
            }
            fan++;
        }
    }

    make_fanstart();
    fix_mesh();
}

//------------------------------------------------------------------------------
void get_small_tiles(SDL_Surface* bmpload)
{
    SDL_Surface * image;

    int x, y, x1, y1;
    int sz_x = bmpload->w;
    int sz_y = bmpload->h;
    int step_x = sz_x >> 3;
    int step_y = sz_y >> 3;

    if (step_x == 0) step_x = 1;
    if (step_y == 0) step_y = 1;

    y1 = 0;
    y = 0;
    while (y < sz_y && y1 < 256)
    {
        x1 = 0;
        x = 0;
        while (x < sz_x && x1 < 256)
        {
            SDL_Rect src1 = { x, y, (step_x - 1), (step_y - 1) };

            glTexture_new( tx_smalltile + numsmalltile );

            image = cartman_CreateSurface(TILEDIV, TILEDIV);
            SDL_FillRect(image, NULL, MAKE_BGR(image, 0, 0, 0));
            SDL_SoftStretch(bmpload, &src1, image, NULL);

            glTexture_Convert(GL_TEXTURE_2D, tx_smalltile + numsmalltile, image, INVALID_KEY);

            numsmalltile++;
            x += step_x;
            x1 += 32;
        }
        y += step_y;
        y1 += 32;
    }
}

//------------------------------------------------------------------------------
void get_big_tiles(SDL_Surface* bmpload)
{
    SDL_Surface * image;

    int x, y, x1, y1;
    int sz_x = bmpload->w;
    int sz_y = bmpload->h;
    int step_x = sz_x >> 3;
    int step_y = sz_y >> 3;

    if (step_x == 0) step_x = 1;
    if (step_y == 0) step_y = 1;

    y1 = 0;
    y = 0;
    while (y < sz_y)
    {
        x1 = 0;
        x = 0;
        while (x < sz_x)
        {
            int wid, hgt;

            SDL_Rect src1;

            wid = (2 * step_x - 1);
            if (x + wid > bmpload->w) wid = bmpload->w - x;

            hgt = (2 * step_y - 1);
            if (y + hgt > bmpload->h) hgt = bmpload->h - y;

            src1.x = x;
            src1.y = y;
            src1.w = wid;
            src1.h = hgt;

            glTexture_new( tx_bigtile + numbigtile );

            image = cartman_CreateSurface(TILEDIV, TILEDIV);
            SDL_FillRect(image, NULL, MAKE_BGR(image, 0, 0, 0));

            SDL_SoftStretch(bmpload, &src1, image, NULL);

            glTexture_Convert(GL_TEXTURE_2D, tx_bigtile + numbigtile, image, INVALID_KEY);

            numbigtile++;
            x += step_x;
            x1 += 32;
        }
        y += step_y;
        y1 += 32;
    }
}

//------------------------------------------------------------------------------
void get_tiles(SDL_Surface* bmpload)
{
    get_small_tiles(bmpload);
    get_big_tiles(bmpload);
}

//------------------------------------------------------------------------------
void load_basic_textures(char *modname)
{
    // ZZ> This function loads the standard textures for a module
    char newloadname[256];
    SDL_Surface *bmptemp;       // A temporary bitmap

    make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "tile0.bmp", newloadname);
    bmptemp = cartman_LoadIMG(newloadname);
    get_tiles(bmptemp);
    SDL_FreeSurface(bmptemp);

    make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "tile1.bmp", newloadname);
    bmptemp = cartman_LoadIMG(newloadname);
    get_tiles(bmptemp);
    SDL_FreeSurface(bmptemp);

    make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "tile2.bmp", newloadname);
    bmptemp = cartman_LoadIMG(newloadname);
    get_tiles(bmptemp);
    SDL_FreeSurface(bmptemp);

    make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "tile3.bmp", newloadname);
    bmptemp = cartman_LoadIMG(newloadname);
    get_tiles(bmptemp);
    SDL_FreeSurface(bmptemp);
}

//------------------------------------------------------------------------------
void show_name(char *newloadname)
{
    fnt_printf_OGL( gFont, 0, ui.scr.y - 16, newloadname);
}

//------------------------------------------------------------------------------
void make_twist()
{
    Uint32 fan, numfan;

    numfan = mesh.tiles_x * mesh.tiles_y;
    fan = 0;
    while (fan < numfan)
    {
        mesh.twist[fan] = get_fan_twist(fan);
        fan++;
    }
}

//------------------------------------------------------------------------------
int count_vertices()
{
    int fan, x, y, cnt, num, totalvert;
    Uint32 vert;

    totalvert = 0;
    y = 0;
    while (y < mesh.tiles_y)
    {
        x = 0;
        while (x < mesh.tiles_x)
        {
            fan = get_fan(x, y);
            if ( -1 != fan )
            {
                num = mesh.command[mesh.fantype[fan]].numvertices;
                vert = mesh.vrtstart[fan];
                cnt = 0;
                while (cnt < num)
                {
                    totalvert++;
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
    return totalvert;
}

//------------------------------------------------------------------------------
void save_mesh(char *modname)
{
#define SAVE numwritten+=fwrite(&itmp, 4, 1, filewrite); numattempt++
#define SAVEF numwritten+=fwrite(&ftmp, 4, 1, filewrite); numattempt++
    FILE* filewrite;
    char newloadname[256];
    int itmp;
    float ftmp;
    int fan, x, y, cnt, num;
    Uint32 vert;
    Uint8 ctmp;

    numwritten = 0;
    numattempt = 0;
    sprintf( newloadname, "%s" SLASH_STR "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "plan.bmp", egoboo_path, modname );
    make_planmap();
    if (bmphitemap)
    {
        SDL_SaveBMP(bmphitemap, newloadname);
    }

    //  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "level.png", newloadname);
    //  make_hitemap();
    //  if(bmphitemap)
    //  {
    //    make_graypal();
    //    save_pcx(newloadname, bmphitemap);
    //  }
    make_twist();

    sprintf( newloadname, "%s" SLASH_STR "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "level.mpd", egoboo_path, modname );

    show_name(newloadname);
    filewrite = fopen(newloadname, "wb");
    if (filewrite)
    {
        itmp = MAPID;  SAVE;
        //    This didn't work for some reason...
        //    itmp=MAXTOTALMESHVERTICES-numfreevertices;  SAVE;
        itmp = count_vertices();  SAVE;
        itmp = mesh.tiles_x;  SAVE;
        itmp = mesh.tiles_y;  SAVE;

        // Write tile data
        y = 0;
        while (y < mesh.tiles_y)
        {
            x = 0;
            while (x < mesh.tiles_x)
            {
                fan = get_fan(x, y);
                if (-1 != fan)
                {
                    itmp = (mesh.fantype[fan] << 24) + (mesh.fx[fan] << 16) + mesh.tx_bits[fan];  SAVE;
                }
                x++;
            }
            y++;
        }

        // Write twist data
        y = 0;
        while (y < mesh.tiles_y)
        {
            x = 0;
            while (x < mesh.tiles_x)
            {
                fan = get_fan(x, y);
                if (-1 != fan)
                {
                    ctmp = mesh.twist[fan];  numwritten += fwrite(&ctmp, 1, 1, filewrite);
                }
                numattempt++;
                x++;
            }
            y++;
        }

        // Write x vertices
        y = 0;
        while (y < mesh.tiles_y)
        {
            x = 0;
            while (x < mesh.tiles_x)
            {
                fan = get_fan(x, y);
                if ( -1 != fan )
                {
                    num = mesh.command[mesh.fantype[fan]].numvertices;
                    vert = mesh.vrtstart[fan];
                    cnt = 0;
                    while (cnt < num)
                    {
                        ftmp = mesh.vrtx[vert] * FIXNUM;  SAVEF;
                        vert = mesh.vrtnext[vert];
                        cnt++;
                    }
                }
                x++;
            }
            y++;
        }

        // Write y vertices
        y = 0;
        while (y < mesh.tiles_y)
        {
            x = 0;
            while (x < mesh.tiles_x)
            {
                fan = get_fan(x, y);
                if ( -1 != fan)
                {
                    num = mesh.command[mesh.fantype[fan]].numvertices;
                    vert = mesh.vrtstart[fan];
                    cnt = 0;
                    while (cnt < num)
                    {
                        ftmp = mesh.vrty[vert] * FIXNUM;  SAVEF;
                        vert = mesh.vrtnext[vert];
                        cnt++;
                    }
                }
                x++;
            }
            y++;
        }

        // Write z vertices
        y = 0;
        while (y < mesh.tiles_y)
        {
            x = 0;
            while (x < mesh.tiles_x)
            {
                fan = get_fan(x, y);
                if ( -1 != fan)
                {
                    num = mesh.command[mesh.fantype[fan]].numvertices;
                    vert = mesh.vrtstart[fan];
                    cnt = 0;
                    while (cnt < num)
                    {
                        ftmp = mesh.vrtz[vert] * FIXNUM;  SAVEF;
                        vert = mesh.vrtnext[vert];
                        cnt++;
                    }
                }
                x++;
            }
            y++;
        }

        // Write a vertices
        y = 0;
        while (y < mesh.tiles_y)
        {
            x = 0;
            while (x < mesh.tiles_x)
            {
                fan = get_fan(x, y);
                if (-1 != fan)
                {
                    num = mesh.command[mesh.fantype[fan]].numvertices;
                    vert = mesh.vrtstart[fan];
                    cnt = 0;
                    while (cnt < num)
                    {
                        ctmp = mesh.vrta[vert];  numwritten += fwrite(&ctmp, 1, 1, filewrite);
                        numattempt++;
                        vert = mesh.vrtnext[vert];
                        cnt++;
                    }
                }
                x++;
            }
            y++;
        }
    }
}

//------------------------------------------------------------------------------
int load_mesh(char *modname)
{
    FILE* fileread;
    char  newloadname[256];
    Uint32  uiTmp32;
    Sint32   iTmp32;
    Uint8  uiTmp8;
    int num, cnt;
    float ftmp;
    int fan;
    Uint32 numvert, numfan;
    Uint32 vert;
    int x, y;

    sprintf( newloadname, "%s" SLASH_STR "gamedat" SLASH_STR "level.mpd", modname );

    fileread = fopen(newloadname, "rb");
    if (NULL == fileread)
    {
        log_warning("load_mesh() - Cannot find mesh for module \"%s\"\n", modname);
    }

    if (fileread)
    {
        free_vertices();

        fread( &uiTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32(uiTmp32); if ( uiTmp32 != MAPID ) return bfalse;
        fread( &uiTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32(iTmp32); numvert = uiTmp32;
        fread( &iTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32(iTmp32); mesh.tiles_x = iTmp32;
        fread( &iTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32(iTmp32); mesh.tiles_y = iTmp32;

        numfan = mesh.tiles_x * mesh.tiles_y;
        mesh.edgex = (mesh.tiles_x * TILEDIV) - 1;
        mesh.edgey = (mesh.tiles_y * TILEDIV) - 1;
        mesh.edgez = 180 << 4;
        numfreevertices = MAXTOTALMESHVERTICES - numvert;

        // Load fan data
        fan = 0;
        while ( fan < numfan )
        {
            fread( &uiTmp32, 4, 1, fileread );
            uiTmp32 = ENDIAN_INT32(uiTmp32);

            mesh.fantype[fan] = (uiTmp32 >> 24) & 0x00FF;
            mesh.fx[fan]      = (uiTmp32 >> 16) & 0x00FF;
            mesh.tx_bits[fan] = (uiTmp32 >>  0) & 0xFFFF;

            fan++;
        }

        // Load normal data
        // Load fan data
        fan = 0;
        while ( fan < numfan )
        {
            fread( &uiTmp8, 1, 1, fileread );
            mesh.twist[fan] = uiTmp8;

            fan++;
        }

        // Load vertex x data
        cnt = 0;
        while (cnt < numvert)
        {
            fread(&ftmp, 4, 1, fileread); ftmp = ENDIAN_FLOAT( ftmp );
            mesh.vrtx[cnt] = ftmp / FIXNUM;
            cnt++;
        }

        // Load vertex y data
        cnt = 0;
        while (cnt < numvert)
        {
            fread(&ftmp, 4, 1, fileread); ftmp = ENDIAN_FLOAT( ftmp );
            mesh.vrty[cnt] = ftmp / FIXNUM;
            cnt++;
        }

        // Load vertex z data
        cnt = 0;
        while (cnt < numvert)
        {
            fread(&ftmp, 4, 1, fileread); ftmp = ENDIAN_FLOAT( ftmp );
            mesh.vrtz[cnt] = ftmp / FIXNUM;
            cnt++;
        }

        // Load vertex a data
        cnt = 0;
        while (cnt < numvert)
        {
            fread(&uiTmp8, 1, 1, fileread);
            mesh.vrta[cnt] = uiTmp8;  // !!!BAD!!!
            cnt++;
        }

        make_fanstart();

        // store the vertices in the vertex chain for editing
        vert = 0;
        y = 0;
        while (y < mesh.tiles_y)
        {
            x = 0;
            while (x < mesh.tiles_x)
            {
                fan = get_fan(x, y);
                if ( -1 != fan)
                {
                    int type = mesh.fantype[fan];
                    if ( type >= 0 && type < MAXMESHTYPE)
                    {
                        num = mesh.command[type].numvertices;
                        mesh.vrtstart[fan] = vert;
                        cnt = 0;
                        while (cnt < num)
                        {
                            mesh.vrtnext[vert] = vert + 1;
                            vert++;
                            cnt++;
                        }
                    }
                    else
                    {
                        assert(0);
                    }
                }
                mesh.vrtnext[vert-1] = CHAINEND;
                x++;
            }
            y++;
        }
        return btrue;
    }
    return bfalse;
}

//------------------------------------------------------------------------------
void move_select(int x, int y, int z)
{
    int vert, cnt, newx, newy, newz;

    cnt = 0;
    while (cnt < numselect_verts)
    {
        vert = select_verts[cnt];
        newx = mesh.vrtx[vert] + x;
        newy = mesh.vrty[vert] + y;
        newz = mesh.vrtz[vert] + z;
        if (newx < 0)  x = 0 - mesh.vrtx[vert];
        if (newx > mesh.edgex) x = mesh.edgex - mesh.vrtx[vert];
        if (newy < 0)  y = 0 - mesh.vrty[vert];
        if (newy > mesh.edgey) y = mesh.edgey - mesh.vrty[vert];
        if (newz < 0)  z = 0 - mesh.vrtz[vert];
        if (newz > mesh.edgez) z = mesh.edgez - mesh.vrtz[vert];
        cnt++;
    }

    cnt = 0;
    while (cnt < numselect_verts)
    {
        vert = select_verts[cnt];
        newx = mesh.vrtx[vert] + x;
        newy = mesh.vrty[vert] + y;
        newz = mesh.vrtz[vert] + z;

        if (newx < 0)  newx = 0;
        if (newx > mesh.edgex)  newx = mesh.edgex;
        if (newy < 0)  newy = 0;
        if (newy > mesh.edgey)  newy = mesh.edgey;
        if (newz < 0)  newz = 0;
        if (newz > mesh.edgez)  newz = mesh.edgez;

        mesh.vrtx[vert] = newx;
        mesh.vrty[vert] = newy;
        mesh.vrtz[vert] = newz;
        cnt++;
    }
}

//------------------------------------------------------------------------------
void set_select_no_bound_z(int z)
{
    int vert, cnt;

    cnt = 0;
    while (cnt < numselect_verts)
    {
        vert = select_verts[cnt];
        mesh.vrtz[vert] = z;
        cnt++;
    }
}

//------------------------------------------------------------------------------
void move_mesh_z(int z, Uint16 tiletype, Uint16 tileand)
{
    int vert, cnt, newz, x, y, totalvert;
    int fan;

    tiletype = tiletype & tileand;
    
    for (y = 0; y < mesh.tiles_y; y++)
    {
        for (x = 0; x < mesh.tiles_x; x++)
        {
            fan = get_fan(x, y);
            if (-1 == fan) continue;

            if (tiletype == (mesh.tx_bits[fan]&tileand))
            {
                vert = mesh.vrtstart[fan];
                totalvert = mesh.command[mesh.fantype[fan]].numvertices;
                
                for (cnt = 0; cnt < totalvert; cnt++)
                {
                    newz = mesh.vrtz[vert] + z;
                    if (newz < 0)  newz = 0;
                    if (newz > mesh.edgez) newz = mesh.edgez;
                    mesh.vrtz[vert] = newz;
                    vert = mesh.vrtnext[vert];
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void move_vert(int vert, int x, int y, int z)
{
    int newx, newy, newz;

    newx = mesh.vrtx[vert] + x;
    newy = mesh.vrty[vert] + y;
    newz = mesh.vrtz[vert] + z;

    if (newx < 0)  newx = 0;
    if (newx > mesh.edgex)  newx = mesh.edgex;
    if (newy < 0)  newy = 0;
    if (newy > mesh.edgey)  newy = mesh.edgey;
    if (newz < 0)  newz = 0;
    if (newz > mesh.edgez)  newz = mesh.edgez;

    mesh.vrtx[vert] = newx;
    mesh.vrty[vert] = newy;
    mesh.vrtz[vert] = newz;
}

//------------------------------------------------------------------------------
void raise_mesh(int x, int y, int amount, int size)
{
    int disx, disy, dis, cnt, newamount;
    Uint32 vert;

    cnt = 0;
    while (cnt < numpointsonscreen)
    {
        vert = pointsonscreen[cnt];
        disx = mesh.vrtx[vert] - (x / FOURNUM);
        disy = mesh.vrty[vert] - (y / FOURNUM);
        dis = sqrt(disx * disx + disy * disy);

        newamount = abs(amount) - ((dis << 1) >> size);
        if (newamount < 0) newamount = 0;
        if (amount < 0)  newamount = -newamount;
        move_vert(vert, 0, 0, newamount);

        cnt++;
    }
}

//------------------------------------------------------------------------------
void load_module(char *modname)
{
    char mod_path[256];

    sprintf( mod_path, "%s" SLASH_STR "modules" SLASH_STR "%s", egoboo_path, modname);

    //  show_name(mod_path);
    load_basic_textures(mod_path);
    if (!load_mesh(mod_path))
    {
        create_mesh();
    }

    read_wawalite( mod_path );

    numlight = 0;
    addinglight = 0;
}

//------------------------------------------------------------------------------
void draw_top_tile( float x, float y, int fan, glTexture * tx_tile, bool_t draw_tile )
{
    int cnt;
    Uint32 vert;
    float max_s, max_t, dst;
    simple_vertex_t vrt[4];

    if ( -1 == fan || TILE_IS_FANOFF(fan) ) return;

    if( NULL == tx_tile ) return;

    glTexture_Bind( tx_tile );

    dst = 1.0 / 64.0;

    max_s = -dst + ( float ) glTexture_GetImageWidth( tx_tile )  / ( float ) glTexture_GetTextureWidth( tx_tile );
    max_t = -dst + ( float ) glTexture_GetImageHeight( tx_tile )  / ( float ) glTexture_GetTextureHeight( tx_tile );

    // set the texture coordinates
    vrt[0].s = dst;
    vrt[0].t = dst;

    vrt[1].s = max_s;
    vrt[1].t = dst;

    vrt[2].s = max_s;
    vrt[2].t = max_t;

    vrt[3].s = dst;
    vrt[3].t = max_t;

    // set the tile corners
    if ( draw_tile )
    {
        // draw the tile on a 31x31 grix, using the values of x,y

        vert = mesh.vrtstart[fan];

        // Top Left
        vrt[0].x = x;
        vrt[0].y = y;
        vrt[0].z = 0;
        vrt[0].l = mesh.vrta[vert] / 255.0f;
        vert = mesh.vrtnext[vert];

        // Top Right
        vrt[1].x = x + TILEDIV;
        vrt[1].y = y;
        vrt[1].z = 0;
        vrt[1].l = mesh.vrta[vert] / 255.0f;
        vert = mesh.vrtnext[vert];

        // Bottom Right
        vrt[2].x = x + TILEDIV;
        vrt[2].y = y + TILEDIV;
        vrt[2].z = 0;
        vrt[2].l = mesh.vrta[vert] / 255.0f;
        vert = mesh.vrtnext[vert];

        // Bottom Left
        vrt[3].x = x;
        vrt[3].y = y + TILEDIV;
        vrt[3].z = 0;
        vrt[3].l = mesh.vrta[vert] / 255.0f;
    }
    else
    {
        // draw the tile using the actual values of the coordinates

        int cnt;

        vert = mesh.vrtstart[fan];
        for (cnt = 0; cnt < 4; cnt++)
        {
            vrt[cnt].x = mesh.vrtx[vert];
            vrt[cnt].y = mesh.vrty[vert];
            vrt[cnt].z = mesh.vrtz[vert];
            vrt[cnt].l = mesh.vrta[vert] / 255.0f;

            vert = mesh.vrtnext[vert];
        }
    }

    // Draw A Quad
    glBegin(GL_QUADS);
    {
        for (cnt = 0; cnt < 4; cnt++)
        {
            glColor3f( vrt[cnt].l,  vrt[cnt].l,  vrt[cnt].l );
            glTexCoord2f(vrt[cnt].s, vrt[cnt].t);
            glVertex3f( vrt[cnt].x, vrt[cnt].y, vrt[cnt].z );
        };
    }
    glEnd();


}

//------------------------------------------------------------------------------
void render_tile_window( window_t * pwin )
{
    glTexture * tx_tile;
    float x, y;
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - (pwin->y + pwin->surfacey), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - (pwin->y + pwin->surfacey), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera( pwin, &cam );
        {
            int fan;

            mapxstt = ( cam.x - cam.w / 2) / TILEDIV - 1;
            mapystt = ( cam.y - cam.h / 2) / TILEDIV - 1;

            mapxend = ( cam.x + cam.w / 2) / TILEDIV + 1;
            mapyend = ( cam.y + cam.h / 2) / TILEDIV + 1;

            // make sure that the texturing is turned on for drawing the tile
            glEnable( GL_TEXTURE_2D );

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                y = mapy * TILEDIV;
                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    x = mapx * TILEDIV;

                    fan     = fan_at(mapx, mapy);
                    tx_tile = tile_at(fan);

                    draw_top_tile( x, y, fan, tx_tile, bfalse );
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

//------------------------------------------------------------------------------
void render_fx_window( window_t * pwin )
{
    glTexture * tx_tile;
    int x, y;
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    glPushAttrib( GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT );
    {
        // set the viewport transformation
        glViewport( pwin->x, sdl_scr.y - (pwin->y + pwin->surfacey), pwin->surfacex, pwin->surfacey );

        // clip the viewport
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - (pwin->y + pwin->surfacey), pwin->surfacex, pwin->surfacey );

        cartman_begin_ortho_camera( pwin, &cam );
        {

            mapxstt = ( cam.x - cam.w / 2) / TILEDIV - 1;
            mapystt = ( cam.y - cam.h / 2) / TILEDIV - 1;

            mapxend = ( cam.x + cam.w / 2) / TILEDIV + 1;
            mapyend = ( cam.y + cam.h / 2) / TILEDIV + 1;

            for ( mapy = mapystt; mapy <= mapyend; mapy++ )
            {
                y = mapy * TILEDIV;
                for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
                {
                    x = mapx * TILEDIV;

                    fan     = fan_at(mapx, mapy);
                    if (-1 != fan)
                    {
                        tx_tile = tile_at(fan);

                        ogl_draw_sprite( tx_tile, x, y, TILEDIV, TILEDIV );

                        if (mesh.fx[fan]&MPDFX_WATER)
                            ogl_draw_sprite( &tx_water, x, y, 0, 0);

                        if (!(mesh.fx[fan]&MPDFX_SHA))
                            ogl_draw_sprite( &tx_ref, x, y, 0, 0);

                        if (mesh.fx[fan]&MPDFX_DRAWREF)
                            ogl_draw_sprite( &tx_drawref, x + 16, y, 0, 0);

                        if (mesh.fx[fan]&MPDFX_ANIM)
                            ogl_draw_sprite( &tx_anim, x, y + 16, 0, 0);

                        if (mesh.fx[fan]&MPDFX_WALL)
                            ogl_draw_sprite( &tx_wall, x + 15, y + 15, 0, 0);

                        if (mesh.fx[fan]&MPDFX_IMPASS)
                            ogl_draw_sprite( &tx_impass, x + 15 + 8, y + 15, 0, 0);

                        if (mesh.fx[fan]&MPDFX_DAMAGE)
                            ogl_draw_sprite( &tx_damage, x + 15, y + 15 + 8, 0, 0);

                        if (mesh.fx[fan]&MPDFX_SLIPPY)
                            ogl_draw_sprite( &tx_slippy, x + 15 + 8, y + 15 + 8, 0, 0);
                    }
                }
            }
        }
        cartman_end_ortho_camera();

    }
    glPopAttrib();
}

//------------------------------------------------------------------------------
void render_vertex_window( window_t * pwin )
{
    int x, y;
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    glPushAttrib( GL_SCISSOR_BIT );
    {
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - (pwin->y + pwin->surfacey), pwin->surfacex, pwin->surfacey );

        mapxstt = (cam.x - (pwin->surfacex / 2)) / TILEDIV;
        mapystt = (cam.y - (pwin->surfacey / 2)) / TILEDIV;

        mapxend = (cam.x + (pwin->surfacex / 2)) / TILEDIV + 1;
        mapyend = (cam.y + (pwin->surfacey / 2)) / TILEDIV + 1;

        x = pwin->x + ( pwin->surfacex / 2 - cam.x );
        y = pwin->y + ( pwin->surfacey / 2 - cam.y );

        for ( mapy = mapystt; mapy <= mapyend; mapy++ )
        {
            for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
            {
                fan = get_fan(mapx, mapy);
                if ( -1 != fan )
                {
                    draw_top_fan( pwin, fan, x, y );
                }
            }
        }

        if (mdata.rect && mdata.mode == WINMODE_VERTEX)
        {
            float color[4];

            OGL_MAKE_COLOR_4(color, 0x3F, 16 + (timclock&15), 16 + (timclock&15), 0 );

            ogl_draw_box( (mdata.rectx / FOURNUM) + x,
                          (mdata.recty / FOURNUM) + y,
                          (mdata.x / FOURNUM) - (mdata.rectx / FOURNUM),
                          (mdata.y / FOURNUM) - (mdata.recty / FOURNUM),
                          color);
        }

        if ((SDLKEYDOWN(SDLK_p) || ((mos.b&2) && numselect_verts == 0)) && mdata.mode == WINMODE_VERTEX)
        {
            raise_mesh(mdata.x, mdata.y, brushamount, brushsize);
        }
    }
    glPopAttrib();
}

//------------------------------------------------------------------------------
void render_side_window( window_t * pwin )
{
    int x, y;
    int mapx, mapxstt, mapxend;
    int mapy, mapystt, mapyend;
    int fan;

    glPushAttrib( GL_SCISSOR_BIT );
    {
        glEnable( GL_SCISSOR_TEST );
        glScissor( pwin->x, sdl_scr.y - (pwin->y + pwin->surfacey), pwin->surfacex, pwin->surfacey );

        mapxstt = (cam.x - (pwin->surfacex / 2)) / TILEDIV;
        mapystt = (cam.y - (pwin->surfacey / 2)) / TILEDIV;

        mapxend = (cam.x + (pwin->surfacex / 2)) / TILEDIV + 1;
        mapyend = (cam.y + (pwin->surfacey / 2)) / TILEDIV + 1;

        x = pwin->x + ( pwin->surfacex / 2 - cam.x);
        y = pwin->y + pwin->surfacey - 10;

        for ( mapy = mapystt; mapy <= mapyend; mapy++ )
        {
            for ( mapx = mapxstt; mapx <= mapxend; mapx++ )
            {
                fan = get_fan(mapx, mapy);
                if (-1 != fan)
                {
                    draw_side_fan(pwin, fan, x, y);
                }
            }
        }

        if (mdata.rect && mdata.mode == WINMODE_SIDE)
        {
            float color[4];

            OGL_MAKE_COLOR_4(color, 0x3F, 16 + (timclock&15), 16 + (timclock&15), 0 );

            ogl_draw_box( (mdata.rectx / FOURNUM) + x,
                          (mdata.recty / FOURNUM),
                          (mdata.x / FOURNUM) - (mdata.rectx / FOURNUM) + 1,
                          (mdata.recty / FOURNUM) - (mdata.y / FOURNUM) + 1,
                          color);
        }
    }
    glPopAttrib();
}

//------------------------------------------------------------------------------
void render_window(window_t * pwin)
{
    if ( NULL == pwin ||  !pwin->on ) return;

    //glPushAttrib( GL_SCISSOR_BIT );
    {
        //glEnable( GL_SCISSOR_TEST );
        //glScissor( pwin->x, sdl_scr.y - (pwin->y + pwin->surfacey), pwin->surfacex, pwin->surfacey );

        make_onscreen();

        if ( 0 != (pwin->mode & WINMODE_TILE) )
        {
            render_tile_window( pwin );
        }

        if ( 0 != (pwin->mode & WINMODE_FX) )
        {
            render_fx_window( pwin );
        }

        if ( 0 != (pwin->mode & WINMODE_VERTEX) )
        {
            render_vertex_window( pwin );
        }

        if ( 0 != (pwin->mode & WINMODE_SIDE) )
        {
            render_side_window( pwin );
        }

        draw_cursor_in_window( pwin );

    }
    //glPopAttrib();
}

//------------------------------------------------------------------------------
void load_window(window_t * pwin, int id, char *loadname, int x, int y, int bx, int by,
                 int sx, int sy, Uint16 mode)
{
    if ( NULL == pwin ) return;

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &(pwin->tex), loadname, INVALID_KEY ) )
    {
        log_warning( "Cannot load \"%s\".\n", loadname);
    }

    pwin->x        = x;
    pwin->y        = y;
    pwin->borderx  = bx;
    pwin->bordery  = by;
    pwin->surfacex = sx;
    pwin->surfacey = sy;
    pwin->on       = btrue;
    pwin->mode     = mode;
    pwin->id       = id;
}

//------------------------------------------------------------------------------
void render_all_windows(void)
{
    int cnt;

    for (cnt = 0; cnt < MAXWIN; cnt++)
    {
        render_window( window_lst + cnt );
    }
}

//------------------------------------------------------------------------------
void load_all_windows(void)
{
    int cnt;

    for (cnt = 0; cnt < MAXWIN; cnt++)
    {
        window_lst[cnt].on = bfalse;
        glTexture_Release( &(window_lst[cnt].tex) );
    }

    load_window(window_lst + 0, 0, "data" SLASH_STR "window.png", 180, 16,  7, 9, 200, 200, WINMODE_VERTEX);
    load_window(window_lst + 1, 1, "data" SLASH_STR "window.png", 410, 16,  7, 9, 200, 200, WINMODE_TILE);
    load_window(window_lst + 2, 2, "data" SLASH_STR "window.png", 180, 248, 7, 9, 200, 200, WINMODE_SIDE);
    load_window(window_lst + 3, 3, "data" SLASH_STR "window.png", 410, 248, 7, 9, 200, 200, WINMODE_FX);
}

//------------------------------------------------------------------------------
void draw_window( window_t * pwin )
{
    if (NULL == pwin || !pwin->on ) return;

    ogl_draw_sprite( &(pwin->tex), pwin->x, pwin->y, pwin->surfacex, pwin->surfacey );
}

//------------------------------------------------------------------------------
void draw_all_windows(void)
{
    int cnt;

    for (cnt = 0; cnt < MAXWIN; cnt++)
    {
        draw_window( window_lst + cnt );
    }
}

//------------------------------------------------------------------------------
void bound_camera(void)
{
    if (cam.x < 0)
    {
        cam.x = 0;
    }
    if (cam.y < 0)
    {
        cam.y = 0;
    }
    if (cam.x > mesh.tiles_x * TILEDIV)
    {
        cam.x = mesh.tiles_x * TILEDIV;
    }
    if (cam.y > mesh.tiles_y * TILEDIV)
    {
        cam.y = mesh.tiles_y * TILEDIV;
    }
}

//------------------------------------------------------------------------------
void unbound_mouse()
{
    mos.tlx = 0;
    mos.tly = 0;
    mos.brx = ui.scr.x - 1;
    mos.bry = ui.scr.y - 1;
}

//------------------------------------------------------------------------------
void bound_mouse()
{
    if (mdata.which_win != -1)
    {
        mos.tlx = window_lst[mdata.which_win].x + window_lst[mdata.which_win].borderx;
        mos.tly = window_lst[mdata.which_win].y + window_lst[mdata.which_win].bordery;
        mos.brx = mos.tlx + window_lst[mdata.which_win].surfacex - 1;
        mos.bry = mos.tly + window_lst[mdata.which_win].surfacey - 1;
    }
}

//------------------------------------------------------------------------------
void rect_select(void)
{
    // ZZ> This function checks the rectangular select_vertsion
    int cnt;
    Uint32 vert;
    int tlx, tly, brx, bry;
    int y;

    if (mdata.mode == WINMODE_VERTEX)
    {
        tlx = mdata.rectx / FOURNUM;
        brx = mdata.x / FOURNUM;
        tly = mdata.recty / FOURNUM;
        bry = mdata.y / FOURNUM;

        if (tlx > brx)  { cnt = tlx;  tlx = brx;  brx = cnt; }
        if (tly > bry)  { cnt = tly;  tly = bry;  bry = cnt; }

        cnt = 0;
        while (cnt < numpointsonscreen && numselect_verts < MAXSELECT)
        {
            vert = pointsonscreen[cnt];
            if (mesh.vrtx[vert] >= tlx &&
                    mesh.vrtx[vert] <= brx &&
                    mesh.vrty[vert] >= tly &&
                    mesh.vrty[vert] <= bry)
            {
                add_select(vert);
            }
            cnt++;
        }
    }
    if (mdata.mode == WINMODE_SIDE)
    {
        tlx = mdata.rectx / FOURNUM;
        brx = mdata.x / FOURNUM;
        tly = mdata.recty / FOURNUM;
        bry = mdata.y / FOURNUM;

        y = 190;//((*(window_lst[mdata.which_win].bmp)).h-10);

        if (tlx > brx)  { cnt = tlx;  tlx = brx;  brx = cnt; }
        if (tly > bry)  { cnt = tly;  tly = bry;  bry = cnt; }

        cnt = 0;
        while (cnt < numpointsonscreen && numselect_verts < MAXSELECT)
        {
            vert = pointsonscreen[cnt];
            if (mesh.vrtx[vert] >= tlx &&
                    mesh.vrtx[vert] <= brx &&
                    -(mesh.vrtz[vert] >> 4) + y >= tly &&
                    -(mesh.vrtz[vert] >> 4) + y <= bry)
            {
                add_select(vert);
            }
            cnt++;
        }
    }
}

//------------------------------------------------------------------------------
void rect_unselect(void)
{
    // ZZ> This function checks the rectangular select_vertsion, and removes any fans
    //     in the select_vertsion area
    int cnt;
    Uint32 vert;
    int tlx, tly, brx, bry;
    int y;

    if (mdata.mode == WINMODE_VERTEX)
    {
        tlx = mdata.rectx / FOURNUM;
        brx = mdata.x / FOURNUM;
        tly = mdata.recty / FOURNUM;
        bry = mdata.y / FOURNUM;

        if (tlx > brx)  { cnt = tlx;  tlx = brx;  brx = cnt; }
        if (tly > bry)  { cnt = tly;  tly = bry;  bry = cnt; }

        cnt = 0;
        while (cnt < numpointsonscreen && numselect_verts < MAXSELECT)
        {
            vert = pointsonscreen[cnt];
            if (mesh.vrtx[vert] >= tlx &&
                    mesh.vrtx[vert] <= brx &&
                    mesh.vrty[vert] >= tly &&
                    mesh.vrty[vert] <= bry)
            {
                remove_select(vert);
            }
            cnt++;
        }
    }
    if (mdata.mode == WINMODE_SIDE)
    {
        tlx = mdata.rectx / FOURNUM;
        brx = mdata.x / FOURNUM;
        tly = mdata.recty / FOURNUM;
        bry = mdata.y / FOURNUM;

        y = 190;//((*(window_lst[mdata.which_win].bmp)).h-10);

        if (tlx > brx)  { cnt = tlx;  tlx = brx;  brx = cnt; }
        if (tly > bry)  { cnt = tly;  tly = bry;  bry = cnt; }

        cnt = 0;
        while (cnt < numpointsonscreen && numselect_verts < MAXSELECT)
        {
            vert = pointsonscreen[cnt];
            if (mesh.vrtx[vert] >= tlx &&
                    mesh.vrtx[vert] <= brx &&
                    -(mesh.vrtz[vert] >> 4) + y >= tly &&
                    -(mesh.vrtz[vert] >> 4) + y <= bry)
            {
                remove_select(vert);
            }
            cnt++;
        }
    }
}

//------------------------------------------------------------------------------
int set_vrta(Uint32 vert)
{
    int newa, x, y, z, brx, bry, brz, deltaz, dist, cnt;
    int newlevel, distance, disx, disy;

    // To make life easier
    x = mesh.vrtx[vert] * FOURNUM;
    y = mesh.vrty[vert] * FOURNUM;
    z = mesh.vrtz[vert];

    // Directional light
    brx = x + 64;
    bry = y + 64;
    brz = get_level(brx, y) +
          get_level(x, bry) +
          get_level(x + 46, y + 46);
    if (z < -128) z = -128;
    if (brz < -128*3) brz = -128 * 3;
    deltaz = z + z + z - brz;
    newa = (deltaz * direct >> 8);

    // Point lights !!!BAD!!!
    newlevel = 0;
    cnt = 0;
    while (cnt < numlight)
    {
        disx = x - light_lst[cnt].x;
        disy = y - light_lst[cnt].y;
        distance = sqrt(disx * disx + disy * disy);
        if (distance < light_lst[cnt].radius)
        {
            newlevel += ((light_lst[cnt].level * (light_lst[cnt].radius - distance)) / light_lst[cnt].radius);
        }
        cnt++;
    }
    newa += newlevel;

    // Bounds
    if (newa < -ambicut) newa = -ambicut;
    newa += ambi;
    if (newa <= 0) newa = 1;
    if (newa > 255) newa = 255;
    mesh.vrta[vert] = newa;

    // Edge fade
    dist = dist_from_border(mesh.vrtx[vert], mesh.vrty[vert]);
    if (dist <= FADEBORDER)
    {
        newa = newa * dist / FADEBORDER;
        if (newa == VERTEXUNUSED)  newa = 1;
        mesh.vrta[vert] = newa;
    }

    return newa;
}

//------------------------------------------------------------------------------
void calc_vrta()
{
    int x, y, fan, num, cnt;
    Uint32 vert;

    y = 0;
    while (y < mesh.tiles_y)
    {
        x = 0;
        while (x < mesh.tiles_x)
        {
            fan = get_fan(x, y);
            if ( -1 != fan)
            {
                vert = mesh.vrtstart[fan];
                num = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                while (cnt < num)
                {
                    set_vrta(vert);
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
}

//------------------------------------------------------------------------------
void level_vrtz()
{
    int x, y, fan, num, cnt;
    Uint32 vert;

    y = 0;
    while (y < mesh.tiles_y)
    {
        x = 0;
        while (x < mesh.tiles_x)
        {
            fan = get_fan(x, y);
            if ( -1 != fan)
            {
                vert = mesh.vrtstart[fan];
                num = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                while (cnt < num)
                {
                    mesh.vrtz[vert] = 0;
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
}

//------------------------------------------------------------------------------
void jitter_select()
{
    int cnt;
    Uint32 vert;

    cnt = 0;
    while (cnt < numselect_verts)
    {
        vert = select_verts[cnt];
        move_vert(vert, (rand() % 3) - 1, (rand() % 3) - 1, 0);
        cnt++;
    }
}

//------------------------------------------------------------------------------
void jitter_mesh()
{
    int x, y, fan, num, cnt;
    Uint32 vert;

    y = 0;
    while (y < mesh.tiles_y)
    {
        x = 0;
        while (x < mesh.tiles_x)
        {
            fan = get_fan(x, y);
            if (-1 != fan)
            {
                vert = mesh.vrtstart[fan];
                num = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                while (cnt < num)
                {
                    clear_select();
                    add_select(vert);
                    //        srand(mesh.vrtx[vert]+mesh.vrty[vert]+dunframe);
                    move_select((rand()&7) - 3, (rand()&7) - 3, (rand()&63) - 32);
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
    clear_select();
}

//------------------------------------------------------------------------------
void flatten_mesh()
{
    int x, y, fan, num, cnt;
    Uint32 vert;
    int height;

    height = (780 - (mdata.y)) * 4;
    if (height < 0)  height = 0;
    if (height > mesh.edgez) height = mesh.edgez;
    y = 0;
    while (y < mesh.tiles_y)
    {
        x = 0;
        while (x < mesh.tiles_x)
        {
            fan = get_fan(x, y);
            if (-1 != fan)
            {
                vert = mesh.vrtstart[fan];
                num = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                while (cnt < num)
                {
                    if (mesh.vrtz[vert] > height - 50)
                        if (mesh.vrtz[vert] < height + 50)
                            mesh.vrtz[vert] = height;
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
    clear_select();
}

//------------------------------------------------------------------------------
void move_camera()
{
    if (((mos.b&4) || SDLKEYDOWN(SDLK_m)) && mdata.which_win != -1)
    {
        cam.x += mos.cx;
        cam.y += mos.cy;
        bound_camera();
        mos.x = mos.x_old;
        mos.y = mos.y_old;
    }
}

//------------------------------------------------------------------------------
void mouse_side( window_t * pwin )
{
    int mpix_x = mos.x - (pwin->x + pwin->borderx + pwin->surfacex / 2) + cam.x;
    int mpix_y = mos.y - (pwin->y + pwin->bordery + pwin->surfacey / 2);

    mdata.y = mpix_y * FOURNUM;

    if ( mpix_x < 0 || mpix_x >= TILEDIV * (mesh.tiles_x + 1) )
    {
        mdata.x = mpix_x * FOURNUM;

        if (mos.b&1)
        {
            if (mdata.rect == bfalse)
            {
                mdata.rect = btrue;
                mdata.rectx = mdata.x;
                mdata.recty = mdata.y;
            }
        }
        else
        {
            if (mdata.rect == btrue)
            {
                if (numselect_verts != 0 && !SDLKEYMOD(KMOD_ALT) && !SDLKEYDOWN(SDLK_MODE) &&
                    !SDLKEYMOD(KMOD_LCTRL) && !SDLKEYMOD(KMOD_RCTRL))
                {
                    clear_select();
                }
                if ( SDLKEYMOD(KMOD_ALT) || SDLKEYDOWN(SDLK_MODE))
                {
                    rect_unselect();
                }
                else
                {
                    rect_select();
                }
                mdata.rect = bfalse;
            }
        }
        if (mos.b&2)
        {
            move_select(mos.cx, 0, -(mos.cy << 4));
            bound_mouse();
        }
        if (SDLKEYDOWN(SDLK_y))
        {
            move_select(0, 0, -(mos.cy << 4));
            bound_mouse();
        }
        if (SDLKEYDOWN(SDLK_u))
        {
            if (mdata.type >= MAXMESHTYPE / 2)
            {
                move_mesh_z(-(mos.cy << 4), mdata.tx, 0xC0);
            }
            else
            {
                move_mesh_z(-(mos.cy << 4), mdata.tx, 0xF0);
            }
            bound_mouse();
        }
        if (SDLKEYDOWN(SDLK_n))
        {
            if (SDLKEYDOWN(SDLK_RSHIFT))
            {
                // Move the first 16 up and down
                move_mesh_z(-(mos.cy << 4), 0, 0xF0);
            }
            else
            {
                // Move the entire mesh up and down
                move_mesh_z(-(mos.cy << 4), 0, 0);
            }
            bound_mouse();
        }
    }

    if (SDLKEYDOWN(SDLK_f))
    {
        flatten_mesh();
    }
    if (SDLKEYDOWN(SDLK_5))
    {
        set_select_no_bound_z(-8000 << 2);
    }
    if (SDLKEYDOWN(SDLK_6))
    {
        set_select_no_bound_z(-127 << 2);
    }
    if (SDLKEYDOWN(SDLK_7))
    {
        set_select_no_bound_z(127 << 2);
    }
    if (SDLKEYDOWN(SDLK_q))
    {
        fix_walls();
    }
}

//------------------------------------------------------------------------------
void mouse_tile( window_t * pwin )
{
    int x, y, keyt, vert, keyv;

    int mpix_x = mos.x - (pwin->x + pwin->borderx + pwin->surfacex / 2) + cam.x;
    int mpix_y = mos.y - (pwin->y + pwin->bordery + pwin->surfacey / 2) + cam.y;

    if ( mpix_x < 0 || mpix_x > TILEDIV * (mesh.tiles_x + 1) ||
         mpix_y < 0 || mpix_y > TILEDIV * (mesh.tiles_y + 1))
    {
        if (mos.b&2)
        {
            mdata.type  = 0;
            mdata.tx    = TILE_GET_LOWER_BITS(FANOFF);
            mdata.upper = TILE_GET_UPPER_BITS(FANOFF);
        }
    }
    else
    {
        cart_vec_t pos[CORNER_COUNT];

        mdata.x = mpix_x * FOURNUM;
        mdata.y = mpix_y * FOURNUM;

        if (mdata.x >= (mesh.tiles_x << 7))  mdata.x = (mesh.tiles_x << 7) - 1;
        if (mdata.y >= (mesh.tiles_y << 7))  mdata.y = (mesh.tiles_y << 7) - 1;

        debugx = mdata.x / 128.0;
        debugy = mdata.y / 128.0;
        x = mdata.x >> 7;
        y = mdata.y >> 7;
        mdata.onfan = get_fan(x, y);
        if (mdata.onfan == -1) mdata.onfan = 0;

        if (!SDLKEYDOWN(SDLK_k))
        {
            addinglight = bfalse;
        }
        if (SDLKEYDOWN(SDLK_k) && addinglight == bfalse)
        {
            add_light(mdata.x, mdata.y, MINRADIUS, MAXLEVEL);
            addinglight = btrue;
        }
        if (addinglight)
        {
            alter_light(mdata.x, mdata.y);
        }
        if (mos.b&1)
        {
            int tx_bits;

            keyt = SDLKEYDOWN(SDLK_t);
            keyv = SDLKEYDOWN(SDLK_v);

            if (!keyt)
            {
                if (!keyv)
                {
                    // Save corner positions
                    vert = mesh.vrtstart[mdata.onfan];
                    pos[CORNER_TL][kX] = mesh.vrtx[vert];
                    pos[CORNER_TL][kY] = mesh.vrty[vert];
                    pos[CORNER_TL][kZ] = mesh.vrtz[vert];

                    vert = mesh.vrtnext[vert];
                    pos[CORNER_TR][kX] = mesh.vrtx[vert];
                    pos[CORNER_TR][kY] = mesh.vrty[vert];
                    pos[CORNER_TR][kZ] = mesh.vrtz[vert];

                    vert = mesh.vrtnext[vert];
                    pos[CORNER_BL][kX] = mesh.vrtx[vert];
                    pos[CORNER_BL][kY] = mesh.vrty[vert];
                    pos[CORNER_BL][kZ] = mesh.vrtz[vert];

                    vert = mesh.vrtnext[vert];
                    pos[CORNER_BR][kX] = mesh.vrtx[vert];
                    pos[CORNER_BR][kY] = mesh.vrty[vert];
                    pos[CORNER_BR][kZ] = mesh.vrtz[vert];
                }
                remove_fan(mdata.onfan);
            }

            tx_bits = TILE_SET_UPPER_BITS( mdata.upper );
            switch (mdata.presser)
            {
                case 0:
                    tx_bits |= mdata.tx & 0xFF;
                    break;
                case 1:
                    tx_bits |= (mdata.tx & 0xFE) | (rand() & 1);
                    break;
                case 2:
                    tx_bits |= (mdata.tx & 0xFC) | (rand() & 3);
                    break;
                case 3:
                    tx_bits |= (mdata.tx & 0xF8) | (rand() & 7);
                    break;
                default:
                    tx_bits = mesh.tx_bits[mdata.onfan];
                    break;
            };
            mesh.tx_bits[mdata.onfan] = tx_bits;

            if (!keyt)
            {
                mesh.fantype[mdata.onfan] = mdata.type;
                add_fan(mdata.onfan, (mdata.x >> 7)*TILEDIV, (mdata.y >> 7)*TILEDIV);
                mesh.fx[mdata.onfan] = mdata.fx;
                if (!keyv)
                {
                    // Return corner positions
                    vert = mesh.vrtstart[mdata.onfan];
                    mesh.vrtx[vert] = pos[CORNER_TL][kX];
                    mesh.vrty[vert] = pos[CORNER_TL][kY];
                    mesh.vrtz[vert] = pos[CORNER_TL][kZ];

                    vert = mesh.vrtnext[vert];
                    mesh.vrtx[vert] = pos[CORNER_TR][kX];
                    mesh.vrty[vert] = pos[CORNER_TR][kY];
                    mesh.vrtz[vert] = pos[CORNER_TR][kZ];

                    vert = mesh.vrtnext[vert];
                    mesh.vrtx[vert] = pos[CORNER_BL][kX];
                    mesh.vrty[vert] = pos[CORNER_BL][kY];
                    mesh.vrtz[vert] = pos[CORNER_BL][kZ];

                    vert = mesh.vrtnext[vert];
                    mesh.vrtx[vert] = pos[CORNER_BR][kX];
                    mesh.vrty[vert] = pos[CORNER_BR][kY];
                    mesh.vrtz[vert] = pos[CORNER_BR][kZ];
                }
            }
        }
        if (mos.b&2)
        {
            mdata.type  = mesh.fantype[mdata.onfan];
            mdata.tx    = TILE_GET_LOWER_BITS(mesh.tx_bits[mdata.onfan]);
            mdata.upper = TILE_GET_UPPER_BITS(mesh.tx_bits[mdata.onfan]);
        }
    }
}

//------------------------------------------------------------------------------
void mouse_fx( window_t * pwin )
{
    int x, y;

    int mpix_x = mos.x - (pwin->x + pwin->borderx + pwin->surfacex / 2) + cam.x;
    int mpix_y = mos.y - (pwin->y + pwin->bordery + pwin->surfacey / 2) + cam.y;

    if ( mpix_x < 0 || mpix_x >= TILEDIV * (mesh.tiles_x + 1) ||
         mpix_y < 0 || mpix_y >= TILEDIV * (mesh.tiles_y + 1))
    {
    }
    else
    {
        mdata.x = mpix_x * FOURNUM;
        mdata.y = mpix_y * FOURNUM;

        if (mdata.x >= (mesh.tiles_x << 7))  mdata.x = (mesh.tiles_x << 7) - 1;
        if (mdata.y >= (mesh.tiles_y << 7))  mdata.y = (mesh.tiles_y << 7) - 1;

        debugx = mdata.x / 128.0;
        debugy = mdata.y / 128.0;
        x = mdata.x >> 7;
        y = mdata.y >> 7;
        mdata.onfan = get_fan(x, y);
        if ( mdata.onfan == -1 ) mdata.onfan = 0;

        if (mos.b&1)
        {
            mesh.fx[mdata.onfan] = mdata.fx;
        }
        if (mos.b&2)
        {
            mdata.fx = mesh.fx[mdata.onfan];
        }
    }
}

//------------------------------------------------------------------------------
void mouse_vertex( window_t * pwin )
{
    int mpix_x = mos.x - (pwin->x + pwin->borderx + pwin->surfacex / 2) + cam.x;
    int mpix_y = mos.y - (pwin->y + pwin->bordery + pwin->surfacey / 2) + cam.y;

    if ( mpix_x < 0 || mpix_x >= TILEDIV * (mesh.tiles_x + 1) ||
         mpix_y < 0 || mpix_y >= TILEDIV * (mesh.tiles_y + 1))
    {
    }
    else
    {
        mdata.x = mpix_x * FOURNUM;
        mdata.y = mpix_y * FOURNUM;

        if (SDLKEYDOWN(SDLK_f))
        {
            //    fix_corners(mdata.x>>7, mdata.y>>7);
            fix_vertices(mdata.x >> 7, mdata.y >> 7);
        }

        if (mos.b&1)
        {
            if (mdata.rect == bfalse)
            {
                mdata.rect = btrue;
                mdata.rectx = mdata.x;
                mdata.recty = mdata.y;
            }
        }
        else
        {
            if (mdata.rect == btrue)
            {
                if (numselect_verts != 0 && !SDLKEYMOD(KMOD_ALT) && !SDLKEYDOWN(SDLK_MODE) &&
                        !SDLKEYMOD(KMOD_LCTRL) && !SDLKEYMOD(KMOD_RCTRL))
                {
                    clear_select();
                }
                if ( SDLKEYMOD(KMOD_ALT) || SDLKEYDOWN(SDLK_MODE))
                {
                    rect_unselect();
                }
                else
                {
                    rect_select();
                }
                mdata.rect = bfalse;
            }
        }
        if (mos.b&2)
        {
            move_select(mos.cx, mos.cy, 0);
            bound_mouse();
        }
    }

    if (SDLKEYDOWN(SDLK_5))
    {
        set_select_no_bound_z(-8000 << 2);
    }
    if (SDLKEYDOWN(SDLK_6))
    {
        set_select_no_bound_z(-127 << 2);
    }
    if (SDLKEYDOWN(SDLK_7))
    {
        set_select_no_bound_z(127 << 2);
    }
}

//------------------------------------------------------------------------------
void check_mouse(void)
{
    int cnt;

    debugx = -1;
    debugy = -1;

    unbound_mouse();
    move_camera();

    mdata.which_win = -1;
    mdata.x         = -1;
    mdata.y         = -1;
    mdata.mode      =  0;

    for (cnt = 0; cnt < MAXWIN; cnt++)
    {
        window_t * pwin = window_lst + cnt;

        if (pwin->on)
        {
            if ( mos.x >= pwin->x + pwin->borderx &&
                    mos.x <= pwin->x + pwin->surfacex - pwin->borderx &&
                    mos.y >= pwin->y + pwin->bordery &&
                    mos.y <= pwin->y + pwin->surfacey - pwin->bordery )
            {
                mdata.which_win = pwin->id;
                mdata.mode      = pwin->mode;

                if ( 0 != (mdata.mode & WINMODE_TILE) )
                {
                    mouse_tile(pwin);
                }

                if ( 0 != (mdata.mode & WINMODE_VERTEX) )
                {
                    mouse_vertex(pwin);
                }

                if ( 0 != (mdata.mode & WINMODE_SIDE) )
                {
                    mouse_side(pwin);
                }

                if ( 0 != (mdata.mode & WINMODE_FX) )
                {
                    mouse_fx(pwin);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void clear_mesh()
{
    int x, y;
    int fan;

    if ( !TILE_IS_FANOFF(TILE_SET_BITS(mdata.upper, mdata.tx)) )
    {
        y = 0;
        while (y < mesh.tiles_y)
        {
            x = 0;
            while (x < mesh.tiles_x)
            {
                fan = get_fan(x, y);
                if (-1 != fan)
                {
                    int tx_bits, type;

                    remove_fan(fan);

                    tx_bits = TILE_SET_UPPER_BITS( mdata.upper );
                    switch (mdata.presser)
                    {
                        case 0:
                            tx_bits |= mdata.tx & 0xFF;
                            break;
                        case 1:
                            tx_bits |= (mdata.tx & 0xFE) | (rand() & 1);
                            break;
                        case 2:
                            tx_bits |= (mdata.tx & 0xFC) | (rand() & 3);
                            break;
                        case 3:
                            tx_bits |= (mdata.tx & 0xF8) | (rand() & 7);
                            break;
                        default:
                            tx_bits = mesh.tx_bits[fan];
                            break;
                    }
                    mesh.tx_bits[fan] = tx_bits;

                    type = mdata.type;
                    if (type <= 1) type = rand() & 1;
                    if (type == 32 || type == 33) type = 32 + (rand() & 1);
                    mesh.fantype[fan] = type;

                    add_fan(fan, x * TILEDIV, y * TILEDIV);
                }
                x++;
            }
            y++;
        }
    }
}

//------------------------------------------------------------------------------
void three_e_mesh()
{
    // ZZ> Replace all 3F tiles with 3E tiles...
    int x, y;
    int fan;

    if (!TILE_IS_FANOFF( TILE_SET_BITS(mdata.upper, mdata.tx) ))
    {
        for (y = 0; y < mesh.tiles_y; y++)
        {
            for (x = 0; x < mesh.tiles_x; x++)
            {
                fan = get_fan(x, y);
                if (-1 == fan) continue;

                if ( mesh.tx_bits[fan] == 0x3F )  mesh.tx_bits[fan] = 0x3E;
            }
        }
    }
}

//------------------------------------------------------------------------------
void toggle_fx(int fxmask)
{
    if (mdata.fx&fxmask)
    {
        mdata.fx -= fxmask;
    }
    else
    {
        mdata.fx += fxmask;
    }
}

//------------------------------------------------------------------------------
void ease_up_mesh()
{
    // ZZ> This function lifts the entire mesh
    int x, y, cnt, zadd;
    Uint32 fan, vert;

    mos.y = mos.y_old;
    mos.x = mos.x_old;
    zadd = -mos.cy;

    y = 0;
    while (y < mesh.tiles_y)
    {
        x = 0;
        while (x < mesh.tiles_x)
        {
            fan = get_fan(x, y);
            if (-1 != fan)
            {
                vert = mesh.vrtstart[fan];
                cnt = 0;
                while (cnt < mesh.command[mesh.fantype[fan]].numvertices)
                {
                    move_vert(vert, 0, 0, zadd);
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
}

//------------------------------------------------------------------------------
void select_verts_connected()
{
    int vert, cnt, tnc, x, y, totalvert = 0;
    int fan;
    Uint8 found, select_vertsfan;

    y = 0;
    while (y < mesh.tiles_y)
    {
        x = 0;
        while (x < mesh.tiles_x)
        {
            fan = get_fan(x, y);
            select_vertsfan = bfalse;
            if (-1 != fan)
            {
                totalvert = mesh.command[mesh.fantype[fan]].numvertices;
                cnt = 0;
                vert = mesh.vrtstart[fan];
                while (cnt < totalvert)
                {

                    found = bfalse;
                    tnc = 0;
                    while (tnc < numselect_verts && !found)
                    {
                        if (select_verts[tnc] == vert)
                        {
                            found = btrue;
                        }
                        tnc++;
                    }
                    if (found) select_vertsfan = btrue;
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }

            if (select_vertsfan)
            {
                cnt = 0;
                vert = mesh.vrtstart[fan];
                while (cnt < totalvert)
                {
                    add_select(vert);
                    vert = mesh.vrtnext[vert];
                    cnt++;
                }
            }
            x++;
        }
        y++;
    }
}

//------------------------------------------------------------------------------
void check_keys(char *modname)
{
    static int last_tick = -1;
    int tick;

    tick = SDL_GetTicks();
    if (tick < last_tick + 20) return;
    last_tick = tick;

    keydelay--;

    keysdlbuffer = SDL_GetKeyState( &keycount );

    if (keydelay <= 0)
    {
        // Hurt
        if (SDLKEYDOWN(SDLK_h))
        {
            toggle_fx(MPDFX_DAMAGE);
            keydelay = KEYDELAY;
        }
        // Impassable
        if (SDLKEYDOWN(SDLK_i))
        {
            toggle_fx(MPDFX_IMPASS);
            keydelay = KEYDELAY;
        }
        // Barrier
        if (SDLKEYDOWN(SDLK_b))
        {
            toggle_fx(MPDFX_WALL);
            keydelay = KEYDELAY;
        }
        // Overlay
        if (SDLKEYDOWN(SDLK_o))
        {
            toggle_fx(MPDFX_WATER);
            keydelay = KEYDELAY;
        }
        // Reflective
        if (SDLKEYDOWN(SDLK_y))
        {
            toggle_fx(MPDFX_SHA);
            keydelay = KEYDELAY;
        }
        // Draw reflections
        if (SDLKEYDOWN(SDLK_d))
        {
            toggle_fx(MPDFX_DRAWREF);
            keydelay = KEYDELAY;
        }
        // Animated
        if (SDLKEYDOWN(SDLK_a))
        {
            toggle_fx(MPDFX_ANIM);
            keydelay = KEYDELAY;
        }
        // Slippy
        if (SDLKEYDOWN(SDLK_s))
        {
            toggle_fx(MPDFX_SLIPPY);
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_g))
        {
            fix_mesh();
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_z))
        {
            set_mesh_tile(mesh.tx_bits[mdata.onfan]);
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_LSHIFT))
        {
            if (mesh.fantype[mdata.onfan] >= (MAXMESHTYPE >> 1))
            {
                fx_mesh_tile(mesh.tx_bits[mdata.onfan], 0xC0, mdata.fx);
            }
            else
            {
                fx_mesh_tile(mesh.tx_bits[mdata.onfan], 0xF0, mdata.fx);
            }
            keydelay = KEYDELAY;
        }
        //if (SDLKEYDOWN(SDLK_x))
        //{
        //    if (mesh.fantype[mdata.onfan] >= (MAXMESHTYPE >> 1))
        //    {
        //        trim_mesh_tile(mesh.tx_bits[mdata.onfan], 0xC0);
        //    }
        //    else
        //    {
        //        trim_mesh_tile(mesh.tx_bits[mdata.onfan], 0xF0);
        //    }
        //    keydelay = KEYDELAY;
        //}
        if (SDLKEYDOWN(SDLK_e))
        {
            ease_up_mesh();
        }
        if (SDLKEYDOWN(SDLK_c))
        {
            clear_mesh();
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_LEFTBRACKET) || SDLKEYDOWN(SDLK_RIGHTBRACKET))
        {
            select_verts_connected();
        }
        if (SDLKEYDOWN(SDLK_8))
        {
            three_e_mesh();
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_j))
        {
            if (numselect_verts == 0) { jitter_mesh(); }
            else { jitter_select(); }
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_l))
        {
            level_vrtz();
        }
        if (SDLKEYDOWN(SDLK_w))
        {
            //impass_edges(2);
            calc_vrta();
            save_mesh(modname);
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_SPACE))
        {
            weld_select();
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_INSERT))
        {
            mdata.type = (mdata.type - 1) & (MAXMESHTYPE - 1);
            while (mesh.numline[mdata.type] == 0)
            {
                mdata.type = (mdata.type - 1) & (MAXMESHTYPE - 1);
            }
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_DELETE))
        {
            mdata.type = (mdata.type + 1) & (MAXMESHTYPE - 1);
            while (mesh.numline[mdata.type] == 0)
            {
                mdata.type = (mdata.type + 1) & (MAXMESHTYPE - 1);
            }
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_KP_PLUS))
        {
            mdata.tx = (mdata.tx + 1) & 0xFF;
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_KP_MINUS))
        {
            mdata.tx = (mdata.tx - 1) & 0xFF;
            keydelay = KEYDELAY;
        }
        if (SDLKEYDOWN(SDLK_UP) || SDLKEYDOWN(SDLK_LEFT) || SDLKEYDOWN(SDLK_DOWN) || SDLKEYDOWN(SDLK_RIGHT))
        {
            if (SDLKEYDOWN(SDLK_UP))
            {
                cam.y -= CAMRATE;
            }
            if (SDLKEYDOWN(SDLK_LEFT))
            {
                cam.x -= CAMRATE;
            }
            if (SDLKEYDOWN(SDLK_DOWN))
            {
                cam.y += CAMRATE;
            }
            if (SDLKEYDOWN(SDLK_RIGHT))
            {
                cam.x += CAMRATE;
            }
            bound_camera();
        }
        if (SDLKEYDOWN(SDLK_END))
        {
            brushsize = 0;
        }
        if (SDLKEYDOWN(SDLK_PAGEDOWN))
        {
            brushsize = 1;
        }
        if (SDLKEYDOWN(SDLK_HOME))
        {
            brushsize = 2;
        }
        if (SDLKEYDOWN(SDLK_PAGEUP))
        {
            brushsize = 3;
        }
        if (SDLKEYDOWN(SDLK_1))
        {
            mdata.presser = 0;
        }
        if (SDLKEYDOWN(SDLK_2))
        {
            mdata.presser = 1;
        }
        if (SDLKEYDOWN(SDLK_3))
        {
            mdata.presser = 2;
        }
        if (SDLKEYDOWN(SDLK_4))
        {
            mdata.presser = 3;
        }
    }
}

void check_input(char * modulename)
{
    // ZZ> This function gets all the current player input states
    SDL_Event evt;

    while ( SDL_PollEvent( &evt ) )
    {

        switch ( evt.type )
        {
            case SDL_MOUSEBUTTONDOWN:
                ui.pending_click = btrue;
                break;

            case SDL_MOUSEBUTTONUP:
                ui.pending_click = bfalse;
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                keystate = evt.key.state;
                break;
        }
    }

    read_mouse();

    check_keys(modulename);             //
    check_mouse();                  //
};

//------------------------------------------------------------------------------
void create_imgcursor(void)
{
    int x, y;
    Uint32 col, loc, clr;
    SDL_Rect rtmp;

    bmpcursor = cartman_CreateSurface(8, 8);
    col = MAKE_BGR(bmpcursor, 31, 31, 31);      // White color
    loc = MAKE_BGR(bmpcursor, 3, 3, 3);             // Gray color
    clr = MAKE_ABGR(bmpcursor, 0, 0, 0, 8);

    // Simple triangle
    rtmp.x = 0;
    rtmp.y = 0;
    rtmp.w = 8;
    rtmp.h = 1;
    SDL_FillRect(bmpcursor, &rtmp, loc);

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            if (x + y < 8) SDL_PutPixel(bmpcursor, x, y, col);
            else SDL_PutPixel(bmpcursor, x, y, clr);
        }
    }
}

//------------------------------------------------------------------------------
void load_img(void)
{
    SDL_Surface *bmptemp;

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_point, "data" SLASH_STR "point.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "point.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_pointon, "data" SLASH_STR "pointon.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "pointon.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_ref, "data" SLASH_STR "pointon.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "ref.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_drawref, "data" SLASH_STR "drawref.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "drawref.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_anim, "data" SLASH_STR "anim.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "anim.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_water, "data" SLASH_STR "water.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "water.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_wall, "data" SLASH_STR "slit.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "slit.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_impass, "data" SLASH_STR "impass.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "impass.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_damage, "data" SLASH_STR "damage.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "damage.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load(GL_TEXTURE_2D, &tx_slippy, "data" SLASH_STR "slippy.png", INVALID_KEY) )
    {
        log_warning( "Cannot load image \"%s\".\n", "slippy.png" );
    }
}

//------------------------------------------------------------------------------
void draw_lotsa_stuff(void)
{
    int x, y, cnt, todo, tile, add;

    // Tell which tile we're in
    x = debugx * 128;
    y = debugy * 128;
    fnt_printf_OGL( gFont, 0, 226,
                    "X = %6.2f (%d)", debugx, x);
    fnt_printf_OGL( gFont, 0, 234,
                    "Y = %6.2f (%d)", debugy, y);

    // Tell user what keys are important
    fnt_printf_OGL( gFont, 0, ui.scr.y - 120,
                    "O = Overlay (Water)");
    fnt_printf_OGL( gFont, 0, ui.scr.y - 112,
                    "R = Reflective");
    fnt_printf_OGL( gFont, 0, ui.scr.y - 104,
                    "D = Draw Reflection");
    fnt_printf_OGL( gFont, 0, ui.scr.y - 96,
                    "A = Animated");
    fnt_printf_OGL( gFont, 0, ui.scr.y - 88,
                    "B = Barrier (Slit)");
    fnt_printf_OGL( gFont, 0, ui.scr.y - 80,
                    "I = Impassable (Wall)");
    fnt_printf_OGL( gFont, 0, ui.scr.y - 72,
                    "H = Hurt");
    fnt_printf_OGL( gFont, 0, ui.scr.y - 64,
                    "S = Slippy");

    // Vertices left
    fnt_printf_OGL( gFont, 0, ui.scr.y - 56,
                    "Vertices %d", numfreevertices);

    // Misc data
    fnt_printf_OGL( gFont, 0, ui.scr.y - 40,
                    "Ambient   %d", ambi);
    fnt_printf_OGL( gFont, 0, ui.scr.y - 32,
                    "Ambicut   %d", ambicut);
    fnt_printf_OGL( gFont, 0, ui.scr.y - 24,
                    "Direct    %d", direct);
    fnt_printf_OGL( gFont, 0, ui.scr.y - 16,
                    "Brush amount %d", brushamount);
    fnt_printf_OGL( gFont, 0, ui.scr.y - 8,
                    "Brush size   %d", brushsize);

    // Cursor
    //if (mos.x >= 0 && mos.x < ui.scr.x && mos.y >= 0 && mos.y < ui.scr.y)
    //{
    //    draw_sprite(theSurface, bmpcursor, mos.x, mos.y);
    //}

    // Tile picks
    todo = 0;
    tile = 0;
    add  = 1;
    if (mdata.tx < MAXTILE)
    {
        switch (mdata.presser)
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
        for (cnt = 0; cnt < todo; cnt++)
        {
            if (mdata.type >= MAXMESHTYPE / 2)
            {
                ogl_draw_sprite( tx_bigtile + tile, x, 0, TILEDIV, TILEDIV );
            }
            else
            {
                ogl_draw_sprite( tx_smalltile + tile, x, 0, TILEDIV, TILEDIV );
            }
            x += TILEDIV;
            tile += add;
        }

        fnt_printf_OGL( gFont, 0, 32,
            "Tile 0x%02x 0x%02x", mdata.upper, mdata.tx);
        fnt_printf_OGL( gFont, 0, 40,
                        "Eats %d verts", mesh.command[mdata.type].numvertices);
        if (mdata.type >= MAXMESHTYPE / 2)
        {
            fnt_printf_OGL( gFont, 0, 56,
                            "63x63 Tile");
        }
        else
        {
            fnt_printf_OGL( gFont, 0, 56,
                            "31x31 Tile");
        }
        draw_schematic(NULL, mdata.type, 0, 64);
    }

    // FX select_vertsion
    if (mdata.fx&MPDFX_WATER)
        ogl_draw_sprite(&tx_water, 0, 200, 0, 0);

    if (!(mdata.fx&MPDFX_SHA))
        ogl_draw_sprite(&tx_ref, 0, 200, 0, 0);

    if (mdata.fx&MPDFX_DRAWREF)
        ogl_draw_sprite(&tx_drawref, 16, 200, 0, 0);

    if (mdata.fx&MPDFX_ANIM)
        ogl_draw_sprite(&tx_anim, 0, 216, 0, 0);

    if (mdata.fx&MPDFX_WALL)
        ogl_draw_sprite(&tx_wall, 15, 215, 0, 0);

    if (mdata.fx&MPDFX_IMPASS)
        ogl_draw_sprite(&tx_impass, 15 + 8, 215, 0, 0);

    if (mdata.fx&MPDFX_DAMAGE)
        ogl_draw_sprite(&tx_damage, 15, 215 + 8, 0, 0);

    if (mdata.fx&MPDFX_SLIPPY)
        ogl_draw_sprite(&tx_slippy, 15 + 8, 215 + 8, 0, 0);

    if (numattempt > 0)
    {
        fnt_printf_OGL( gFont, 0, 0,
                        "numwritten %d/%d", numwritten, numattempt);
    }

    fnt_printf_OGL( gFont, 0, 0,
                    "<%f, %f>", mos.x, mos.y );
}

//------------------------------------------------------------------------------
void draw_slider(int tlx, int tly, int brx, int bry, int* pvalue, int minvalue, int maxvalue)
{
    int cnt;
    int value;

    float color[4] = {1, 1, 1, 1};

    // Pick a new value
    value = *pvalue;
    if (mos.x >= tlx && mos.x <= brx && mos.y >= tly && mos.y <= bry && mos.b)
    {
        value = (((mos.y - tly) * (maxvalue - minvalue)) / (bry - tly)) + minvalue;
    }
    if (value < minvalue) value = minvalue;
    if (value > maxvalue) value = maxvalue;
    *pvalue = value;

    // Draw it
    if ( (maxvalue - minvalue) != 0)
    {
        float amount;
        cnt = ((value - minvalue) * 20 / (maxvalue - minvalue)) + 11;

        amount = (value - minvalue) / (float)(maxvalue - minvalue);

        ogl_draw_box( tlx, amount * (bry - tly) + tly, brx - tlx + 1, 5, color );
    }

}

//------------------------------------------------------------------------------
void draw_main(void)
{
    bool_t recalc_lighting = bfalse;

    glClear ( GL_COLOR_BUFFER_BIT );

    ogl_beginFrame();
    {
        int itmp;

        render_all_windows();

        draw_all_windows();

        itmp = ambi;
        draw_slider( 0, 250, 19, 350, &ambi,          0, 200);
        if ( itmp != ambi ) recalc_lighting = btrue;

        itmp = ambicut;
        draw_slider(20, 250, 39, 350, &ambicut,       0, ambi);
        if ( itmp != ambicut ) recalc_lighting = btrue;

        itmp = direct;
        draw_slider(40, 250, 59, 350, &direct,        0, 100);
        if ( itmp != direct ) recalc_lighting = btrue;

        draw_slider(60, 250, 79, 350, &brushamount, -50,  50);

        draw_lotsa_stuff();
    }
    ogl_endFrame();

    if (recalc_lighting)
    {
        calc_vrta();
    }

    dunframe++;
    secframe++;

    SDL_GL_SwapBuffers();
}

//------------------------------------------------------------------------------
int main(int argcnt, char* argtext[])
{
    char modulename[100];
    STRING fname;
    char *blah[3];

    //blah[0] = malloc(256); strcpy(blah[0], "");
    //blah[1] = malloc(256); strcpy(blah[1], "/home/bgbirdsey/egoboo");
    //blah[2] = malloc(256); strcpy(blah[2], "advent" );

    //argcnt = 3;
    //argtext = blah;

    // register the logging code
    log_init();
    log_setLoggingLevel( 2 );
    atexit( log_shutdown );

    show_info();                        // Text title
    if (argcnt < 2 || argcnt > 3)
    {
        printf("USAGE: CARTMAN [PATH] MODULE ( without .MOD )\n\n");
        exit(0);
    }
    else if (argcnt < 3)
    {
        sprintf(egoboo_path, "%s", ".");
        sprintf(modulename, "%s.mod", argtext[1]);
    }
    else if (argcnt < 4)
    {
        size_t len = strlen(argtext[1]);
        char * pstr = argtext[1];
        if (pstr[0] == '\"')
        {
            pstr[len-1] = '\0';
            pstr++;
        }
        sprintf(egoboo_path, "%s", pstr);
        sprintf(modulename, "%s.mod", argtext[2]);
    }

    sprintf(fname, "%s" SLASH_STR "setup.txt", egoboo_path );
    cfg_file = setup_read( fname );
    if ( NULL == cfg_file )
    {
        log_error( "Cannot load the setup file \"%s\".\n", fname );
    }
    setup_download( cfg_file, &cfg );

    // initialize the SDL elements
    sdlinit( argcnt, argtext );
    gFont = fnt_loadFont( "data" SLASH_STR "pc8x8.fon", 12 );

    glClearColor (0.0, 0.0, 0.0, 0.0);

    make_randie();                      // Random number table
    fill_fpstext();                     // Make the FPS text

    load_all_windows();                 // Load windows
    create_imgcursor();                 // Make cursor image
    load_img();                         // Load other images
    load_mesh_fans();                   // Get fan data
    load_module(modulename);            // Load the module

    dunframe   = 0;                     // Timer resets
    worldclock = 0;
    timclock   = 0;
    while (!SDLKEYDOWN(SDLK_ESCAPE) && !SDLKEYDOWN(SDLK_F1))        // Main loop
    {
        check_input(modulename);

        draw_main();

        SDL_Delay(1);

        timclock = SDL_GetTicks() >> 3;
    }

    show_info();                // Ending statistics
    exit(0);                        // End
}

//------------------------------------------------------------------------------
static bool_t _sdl_atexit_registered = bfalse;
static bool_t _ttf_atexit_registered = bfalse;
void sdlinit( int argc, char **argv )
{

    log_info( "Initializing SDL version %d.%d.%d... ", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
    if ( SDL_Init( 0 ) < 0 )
    {
        log_message( "Failed!\n" );
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

    log_info( "INFO: Initializing SDL video... " );
    if (SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0)
    {
        log_message( "Failed!\n" );
        log_error( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

    log_info( "Initializing SDL timer services... " );
    if (SDL_InitSubSystem( SDL_INIT_TIMER ) < 0)
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
        log_message( "Succeess!\n" );
    }

    // initialize the font handler
    log_info( "Initializing the SDL_ttf font handler... " );
    if ( TTF_Init() < 0)
    {
        log_message( "Failed! Unable to load the font handler: %s\n", SDL_GetError() );
        exit( -1 );
    }
    else
    {
        log_message( "Success!\n" );
        if ( !_ttf_atexit_registered )
        {
            _ttf_atexit_registered = btrue;
            atexit(TTF_Quit);
        }
    }

#ifdef __unix__

    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
    // will cause SDL_SetVideoMode to fail with:
    // Unable to set video mode: Couldn't find matching GLX visual
    if ( cfg.scr.d == 32 ) cfg.scr.d = 24;

#endif

    // the flags to pass to SDL_SetVideoMode
    sdl_vparam.flags          = SDL_OPENGLBLIT;                // enable SDL blitting operations in OpenGL (for the moment)
    sdl_vparam.opengl         = SDL_TRUE;
    sdl_vparam.doublebuffer   = SDL_TRUE;
    sdl_vparam.glacceleration = GL_FALSE;
    sdl_vparam.width          = MIN(640, cfg.scr.x);
    sdl_vparam.height         = MIN(480, cfg.scr.y);
    sdl_vparam.depth          = cfg.scr.d;

    ogl_vparam.dither         = GL_FALSE;
    ogl_vparam.antialiasing   = GL_TRUE;
    ogl_vparam.perspective    = GL_FASTEST;
    ogl_vparam.shading        = GL_SMOOTH;
    ogl_vparam.userAnisotropy = cfg.texturefilter > TX_TRILINEAR_2;

    // Get us a video mode
    if ( NULL == SDL_GL_set_mode(NULL, &sdl_vparam, &ogl_vparam) )
    {
        log_info( "I can't get SDL to set any video mode: %s\n", SDL_GetError() );
        exit(-1);
    }

    glEnable (GL_LINE_SMOOTH);
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
    glLineWidth (1.5);

    //  SDL_WM_SetIcon(tmp_surface, NULL);
    SDL_WM_SetCaption("Egoboo", "Egoboo");

    if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ) < 0 )
    {
        log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
    }
    atexit( SDL_Quit );

    // start the font handler
    if ( TTF_Init() < 0)
    {
        log_error( "Unable to load the font handler: %s\n", SDL_GetError() );
    }
    atexit( TTF_Quit );

#ifdef __unix__
    /* GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
    will cause SDL_SetVideoMode to fail with:
    "Unable to set video mode: Couldn't find matching GLX visual" */
    if ( cfg.scr.d == 32 ) cfg.scr.d = 24;
#endif

#ifndef __APPLE__
    {
        SDL_Surface * tmp_icon;
        /* Setup the cute windows manager icon */
        tmp_icon = SDL_LoadBMP( "data" SLASH_STR "egomap_icon.bmp" );
        if ( tmp_icon == NULL )
        {
            log_warning( "Unable to load icon (data" SLASH_STR "egomap_icon.bmp)\n" );
        }
        else
        {
            SDL_WM_SetIcon( tmp_icon, NULL );
        }
    }
#endif

    SDLX_Get_Screen_Info( &(ui.scr), bfalse );

    theSurface = SDL_GetVideoSurface();

    // Set the window name
    SDL_WM_SetCaption( NAME, NAME );

    if ( ui.GrabMouse )
    {
        SDL_WM_GrabInput ( SDL_GRAB_ON );
    }

    if ( ui.HideMouse )
    {
        SDL_ShowCursor( 0 );  // Hide the mouse cursor
    }
}


//--------------------------------------------------------------------------------------------
SDL_Surface * cartman_CreateSurface(int w, int h)
{
    SDL_PixelFormat   tmpformat;

    if ( NULL == theSurface ) return NULL;

    // expand the screen format to support alpha
    memcpy( &tmpformat, theSurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
    SDLX_ExpandFormat(&tmpformat);

    return SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, tmpformat.BitsPerPixel, tmpformat.Rmask, tmpformat.Gmask, tmpformat.Bmask, tmpformat.Amask );
}

//--------------------------------------------------------------------------------------------
void read_mouse()
{
    int x, y, b;
    if ( mos.relative )
    {
        b = SDL_GetRelativeMouseState( &x, &y );
        mos.cx = x;
        mos.cy = y;
        mos.x += mos.cx;
        mos.y += mos.cy;
    }
    else
    {
        b = SDL_GetMouseState( &x, &y );
        mos.cx = mos.x - x;
        mos.cy = mos.y - y;
        mos.x  = x;
        mos.y  = y;
    }

    mos.button[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
    mos.button[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
    mos.button[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0; // Middle is 2 on SDL
    mos.button[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;

    // Mouse mask
    mos.b = ( mos.button[3] << 3 ) | ( mos.button[2] << 2 ) | ( mos.button[1] << 1 ) | ( mos.button[0] << 0 );
}

void draw_sprite(SDL_Surface * dst, SDL_Surface * sprite, int x, int y )
{
    SDL_Rect rdst;

    if (NULL == dst || NULL == sprite) return;

    rdst.x = x;
    rdst.y = y;
    rdst.w = sprite->w;
    rdst.h = sprite->h;

    cartman_BlitSurface(sprite, NULL, dst, &rdst );
}

//void line(ogl_surface * surf, int x0, int y0, int x1, int y1, Uint32 c)
//{
//    glPushAttrib( GL_VIEWPORT_BIT );
//    {
//        //if( NULL != surf )
//        //{
//        //    glViewport( surf->viewport[0], surf->viewport[1], surf->viewport[2], surf->viewport[3] );
//        //};
//
//        glColor4i( ( c >> 0 ) & 0xFF, ( c >> 8 ) & 0xFF, ( c >> 16 ) & 0xFF, ( c >> 24 ) & 0xFF );
//
//        glBegin(GL_LINE);
//        {
//            glVertex2i( x0, y0 );
//            glVertex2i( x1, y1 );
//        }
//        glEnd();
//    }
//    glPopAttrib();
//}

int cartman_BlitScreen(SDL_Surface * bmp, SDL_Rect * prect)
{
    return cartman_BlitSurface(bmp, NULL, theSurface, prect );
}

bool_t SDL_RectIntersect(SDL_Rect * src, SDL_Rect * dst, SDL_Rect * isect )
{
    Sint16 xmin, xmax, ymin, ymax;

    // should not happen
    if (NULL == src && NULL == dst) return bfalse;

    // null cases
    if (NULL == isect) return bfalse;
    if (NULL == src) { *isect = *dst; return btrue; }
    if (NULL == dst) { *isect = *src; return btrue; }

    xmin = MAX(src->x, dst->x);
    xmax = MIN(src->x + src->w, dst->x + dst->w);

    ymin = MAX(src->y, dst->y);
    ymax = MIN(src->y + src->h, dst->y + dst->h);

    isect->x = xmin;
    isect->w = MAX(0, xmax - xmin);
    isect->y = ymin;
    isect->h = MAX(0, ymax - ymin);

    return btrue;
}

int cartman_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
    // clip the source and destination rectangles

    int retval = -1;
    SDL_Rect rsrc, rdst;

    if ( NULL == src || 0 != ((size_t)src->map & 0x80000000) ) return 0;

    if (NULL == srcrect && NULL == dstrect)
    {
        retval = SDL_BlitSurface(src, NULL, dst, NULL);
        if (retval >= 0)
        {
            SDL_UpdateRect(dst, 0, 0, 0, 0);
        }
    }
    else if (NULL == srcrect)
    {
        SDL_RectIntersect( &(dst->clip_rect), dstrect, &rdst );
        retval = SDL_BlitSurface(src, NULL, dst, &rdst);
        if (retval >= 0)
        {
            SDL_UpdateRect(dst, rdst.x, rdst.y, rdst.w, rdst.h);
        }
    }
    else if (NULL == dstrect)
    {
        SDL_RectIntersect( &(src->clip_rect), srcrect, &rsrc );

        retval = SDL_BlitSurface(src, &rsrc, dst, NULL);
        if (retval >= 0)
        {
            SDL_UpdateRect(dst, 0, 0, 0, 0);
        }
    }
    else
    {
        SDL_RectIntersect( &(src->clip_rect), srcrect, &rsrc );
        SDL_RectIntersect( &(dst->clip_rect), dstrect, &rdst );

        retval = SDL_BlitSurface(src, &rsrc, dst, &rdst);
        if (retval >= 0)
        {
            SDL_UpdateRect(dst, rdst.x, rdst.y, rdst.w, rdst.h);
        }
    }

    return retval;
}


SDL_Surface * cartman_LoadIMG(const char * szName)
{
    SDL_PixelFormat tmpformat;
    SDL_Surface * bmptemp, * bmpconvert;

    // load the bitmap
    bmptemp = IMG_Load(szName);

    // expand the screen format to support alpha
    memcpy( &tmpformat, theSurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
    SDLX_ExpandFormat(&tmpformat);

    // convert it to the same pixel format as the screen surface
    bmpconvert = SDL_ConvertSurface( bmptemp, &tmpformat, SDL_SWSURFACE );
    SDL_FreeSurface(bmptemp);

    return bmpconvert;
}

//--------------------------------------------------------------------------------------------
void ogl_draw_sprite( glTexture * img, int x, int y, int width, int height )
{
    int w, h;
    float x1, y1;

    x1 = 1.0f;
    y1 = 1.0f;

    w = width;
    h = height;

    if ( NULL != img )
    {
        if ( width == 0 || height == 0 )
        {
            w = img->imgW;
            h = img->imgH;
        }

        x1 = ( float ) glTexture_GetImageWidth( img )  / ( float ) glTexture_GetTextureWidth( img );
        y1 = ( float ) glTexture_GetImageHeight( img ) / ( float ) glTexture_GetTextureHeight( img );
    }

    // Draw the image
    glTexture_Bind( img );

    glColor4f(1, 1, 1, 1);

    glBegin( GL_TRIANGLE_STRIP );
    {
        glTexCoord2f(  0,  0 );  glVertex2i( x,     y );
        glTexCoord2f( x1,  0 );  glVertex2i( x + w, y );
        glTexCoord2f(  0, y1 );  glVertex2i( x,     y + h );
        glTexCoord2f( x1, y1 );  glVertex2i( x + w, y + h );
    }
    glEnd();
}




//--------------------------------------------------------------------------------------------
void ogl_draw_box( int x, int y, int w, int h, float color[] )
{
    glPushAttrib( GL_ENABLE_BIT );
    {
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_QUADS );
        {
            glColor4fv( color );

            glVertex2i( x,     y );
            glVertex2i( x,     y + h );
            glVertex2i( x + w, y + h );
            glVertex2i( x + w, y );
        }
        glEnd();
    }
    glPopAttrib();
};


//--------------------------------------------------------------------------------------------
void ogl_beginFrame()
{
    glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glEnable( GL_TEXTURE_2D );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glViewport( 0, 0, theSurface->w, theSurface->h );

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( 0, theSurface->w, theSurface->h, 0, -1, 1 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}

//--------------------------------------------------------------------------------------------
void ogl_endFrame()
{
    // Restore the OpenGL matrices to what they were
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    // Re-enable any states disabled by gui_beginFrame
    glPopAttrib();
}

//------------------------------------------------------------------------------
void cartman_begin_ortho_camera( window_t * pwin, camera_t * pcam  )
{
    float w, h;
    float aspect;
    float left, right, bottom, top;

    w = pwin->surfacex;
    h = pwin->surfacey;

    pcam->w = w;
    pcam->h = h;

    left   = - w / 2;
    right  =   w / 2;
    bottom = - h / 2;
    top    =   h / 2;

    aspect = (GLdouble) w / h;
    if ( aspect < 1.0 )
    {
        // window taller than wide
        bottom /= aspect; top /= aspect;
    }
    else
    {
        left *= aspect;
        right *= aspect;
    }

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(left, right, bottom, top, -(180 << 4), (180 << 4));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    gluLookAt (pcam->x, pcam->y, 0, pcam->x, pcam->y, 180 << 4, 0.0, -1.0, 0.0);
}

//------------------------------------------------------------------------------
void cartman_end_ortho_camera( )
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}
