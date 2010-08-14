#pragma once

#include <SDL_opengl.h>

struct s_glTexture;
typedef struct s_glTexture glTexture;

#define TWO_PI 3.1415926535897932384626433832795f

#ifndef ABS
#    define ABS(X)  (((X) > 0) ? (X) : -(X))
#endif

#ifndef SGN
#    define SGN(X)  (((X) >= 0) ? 1 : -1)
#endif

#ifndef MIN

#    define MIN(x, y)  (((x) > (y)) ? (y) : (x))
#endif

#ifndef MAX
#    define MAX(x, y)  (((x) > (y)) ? (x) : (y))
#endif

#ifndef CLIP
#    define CLIP(A,B,C) MIN(MAX(A,B),C)
#endif

struct s_ogl_surface
{
    GLint viewport[4];
};
typedef struct s_ogl_surface ogl_surface;

#define MAXMESSAGE          6                       // Number of messages
#define TOTALMAXDYNA                    64          // Absolute max number of dynamic lights
#define TOTALMAXPRT             2048                // True max number of particles


#define MAXLIGHT 100
#define MAXRADIUS 500*FOURNUM
#define MINRADIUS 50*FOURNUM
#define MAXLEVEL 255
#define MINLEVEL 50

#define VERSION 005             // Version number
#define YEAR 1999               // Year
#define NAME "Cartman"          // Program name
#define KEYDELAY 12             // Delay for keyboard
#define MAXTILE 256             //
#define TINYXY   4              // Plan tiles
#define SMALLXY 32              // Small tiles
#define BIGXY   64              // Big tiles
#define CAMRATE 8               // Arrow key movement rate
#define MAXSELECT 2560          // Max points that can be select_vertsed
#define FOURNUM   ((1<<7)/((float)(SMALLXY)))          // Magic number
#define FIXNUM    4 // 4.129           // 4.150
#define TILEDIV   SMALLXY
#define MAPID     0x4470614d        // The string... MapD

#define FADEBORDER 64           // Darkness at the edge of map
#define SLOPE 50            // Twist stuff

#define MAXWIN 8            // Number of windows

#define WINMODE_NOTHING 0           // Window display mode
#define WINMODE_TILE 1          //
#define WINMODE_VERTEX 2            //
#define WINMODE_SIDE 4          //
#define WINMODE_FX 8                //

//#define ONSIZE 600            // Max size of raise mesh
#define ONSIZE 264          // Max size of raise mesh


#define MAXMESHTYPE                     64          // Number of mesh types
#define MAXMESHLINE                     64          // Number of lines in a fan schematic
#define MAXMESHVERTICES                 16      // Max number of vertices in a fan
#define MAXMESHFAN                      (512*512)        // Size of map in fans
#define MAXTOTALMESHVERTICES            (MAXMESHFAN*MAXMESHVERTICES)
#define MAXMESHTILEY                    1024       // Max fans in y direction
#define MAXMESHBLOCKY                   (( MAXMESHTILEY >> 2 )+1)  // max blocks in the y direction
#define MAXMESHCOMMAND                  4             // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32            // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32            // Max trigs in each command

#define FANOFF   0xFFFF         // Don't draw
#define CHAINEND 0xFFFFFFFF     // End of vertex chain

#define VERTEXUNUSED 0          // Check mesh.vrta to see if used
#define MAXPOINTS 20480         // Max number of points to draw

#define MPDFX_REF 0            // MeshFX
#define MPDFX_SHA 1            //
#define MPDFX_DRAWREF 2      //
#define MPDFX_ANIM 4             //
#define MPDFX_WATER 8            //
#define MPDFX_WALL 16            //
#define MPDFX_IMPASS 32         //
#define MPDFX_DAMAGE 64         //
#define MPDFX_SLIPPY 128        //

#include "SDL_extensions.h"
#include "ogl_extensions.h"
#include "egoboo_setup.h"

extern config_data_t cfg;
extern SDLX_video_parameters_t sdl_vparam;
extern oglx_video_parameters_t ogl_vparam;

extern int    animtileupdateand;                      // New tile every ( (1 << n) - 1 ) frames
extern Uint16 animtileframeand;                       // 1 << n frames
extern Uint16 animtilebaseand;
extern Uint16 biganimtileframeand;                    // 1 << n frames
extern Uint16 biganimtilebaseand;
extern Uint16 animtileframeadd;


struct s_command
{
    Uint8   numvertices;                // Number of vertices

    Uint8   ref[MAXMESHVERTICES];       // Lighting references

    int     x[MAXMESHVERTICES];         // Vertex texture posi
    int     y[MAXMESHVERTICES];         //

    float   u[MAXMESHVERTICES];         // Vertex texture posi
    float   v[MAXMESHVERTICES];         //

    int     count;                      // how many commands
    int     size[MAXMESHCOMMAND];      // how many command entries
    int     vrt[MAXMESHCOMMANDENTRIES];       // which vertex for each command entry
};
typedef struct s_command command_t;

#define INVALID_BLOCK ((Uint32)(~0))
#define INVALID_TILE  ((Uint32)(~0))


struct s_mesh
{
    bool_t exploremode;

    int               tiles_x;           // Size of mesh
    int               tiles_y;           //
    Uint32  fanstart[MAXMESHTILEY];           // Y to fan number

    int               blocksx;          // Size of mesh
    int               blocksy;          //
    Uint32  blockstart[(MAXMESHTILEY >> 4) + 1];

    int               edgex;            // Borders of mesh
    int               edgey;            //
    int               edgez;            //

    Uint8   fantype[MAXMESHFAN];        // Tile fan type
    Uint8   fx[MAXMESHFAN];             // Rile special effects flags
    Uint16  tx_bits[MAXMESHFAN];        // Tile texture bits and special tile bits
    Uint8   twist[MAXMESHFAN];          // Surface normal

    Uint32  vrtstart[MAXMESHFAN];     // Which vertex to start at
    Uint32  vrtnext[MAXTOTALMESHVERTICES];   // Next vertex in fan

    Uint16  vrtx[MAXTOTALMESHVERTICES];      // Vertex position
    Uint16  vrty[MAXTOTALMESHVERTICES];      //
    Sint16  vrtz[MAXTOTALMESHVERTICES];      // Vertex elevation
    Uint8   vrta[MAXTOTALMESHVERTICES];      // Vertex base light, 0=unused

    Uint32       numline[MAXMESHTYPE];       // Number of lines to draw
    line_data_t  line[MAXMESHLINE];
    command_t    command[MAXMESHTYPE];
};
typedef struct s_mesh mesh_t;


extern mesh_t mesh;

glTexture * tile_at( int fan );

#define DAMAGENULL          255                        //

enum e_damage_type
{
    DAMAGE_SLASH = 0,                        //
    DAMAGE_CRUSH,                            //
    DAMAGE_POKE,                             //
    DAMAGE_HOLY,                             // (Most invert Holy damage )
    DAMAGE_EVIL,                             //
    DAMAGE_FIRE,                             //
    DAMAGE_ICE,                              //
    DAMAGE_ZAP,                              //
    DAMAGE_COUNT                             // Damage types
};

#define DAMAGECHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGEINVERT        4                       // 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       // 000000xx Resistance ( 1 is common )
#define DAMAGETILETIME      32                      // Invincibility time
#define DAMAGETIME          16                      // Invincibility time
#define DEFENDTIME          16                      // Invincibility time


extern Sint16          damagetileparttype;
extern short           damagetilepartand;
extern short           damagetilesound;
extern short           damagetilesoundtime;
extern Uint16          damagetilemindistance;
extern int             damagetileamount;                           // Amount of damage
extern Uint8           damagetiletype;                      // Type of damage

// handle the upper and lower bits for the tile image
#define TILE_UPPER_SHIFT                8
#define TILE_LOWER_MASK                 ((1 << TILE_UPPER_SHIFT)-1)
#define TILE_UPPER_MASK                 (~TILE_LOWER_MASK)

#define TILE_GET_LOWER_BITS(XX)         ( TILE_LOWER_MASK & (XX) )

#define TILE_GET_UPPER_BITS(XX)         (( TILE_UPPER_MASK & (XX) ) >> TILE_UPPER_SHIFT )
#define TILE_SET_UPPER_BITS(XX)         (( (XX) << TILE_UPPER_SHIFT ) & TILE_UPPER_MASK )
#define TILE_SET_BITS(HI,LO)            (TILE_SET_UPPER_BITS(HI) | TILE_GET_LOWER_BITS(LO))

#define TILE_IS_FANOFF(XX)              ( FANOFF == (XX) )

#define TILE_HAS_INVALID_IMAGE(XX)      HAS_SOME_BITS( TILE_UPPER_MASK, (XX).img )