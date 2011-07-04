#pragma once

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_line_data;
typedef struct s_line_data line_data_t;

struct s_command;
typedef struct s_command command_t;

struct s_mesh;
typedef struct s_mesh mesh_t;

struct s_mesh_info;
typedef struct s_mesh_info mesh_info_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

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

#define MPDFX_REF 0            // MeshFX
#define MPDFX_SHA 1            //
#define MPDFX_DRAWREF 2      //
#define MPDFX_ANIM 4             //
#define MPDFX_WATER 8            //
#define MPDFX_WALL 16            //
#define MPDFX_IMPASS 32         //
#define MPDFX_DAMAGE 64         //
#define MPDFX_SLIPPY 128        //

#define INVALID_BLOCK ((Uint32)(~0))
#define INVALID_TILE  ((Uint32)(~0))

#define TINYXY   4              // Plan tiles
#define SMALLXY 32              // Small tiles
#define BIGXY   (2 * SMALLXY)   // Big tiles
#define MAPID     0x4470614d        // The string... MapD

#define FIXNUM    4 // 4.129           // 4.150
#define TILE_SIZE 128
#define FOURNUM   ( (float)TILE_SIZE / (float)SMALLXY )          // Magic number

#define DEFAULT_TILE 62

#define SLOPE 50            // Twist stuff

#define TILE_IS_FANOFF(XX)              ( FANOFF == (XX) )

// handle the upper and lower bits for the tile image
#define TILE_UPPER_SHIFT                8
#define TILE_LOWER_MASK                 ((1 << TILE_UPPER_SHIFT)-1)
#define TILE_UPPER_MASK                 (~TILE_LOWER_MASK)

#define TILE_GET_LOWER_BITS(XX)         ( TILE_LOWER_MASK & (XX) )

#define TILE_GET_UPPER_BITS(XX)         (( TILE_UPPER_MASK & (XX) ) >> TILE_UPPER_SHIFT )
#define TILE_SET_UPPER_BITS(XX)         (( (XX) << TILE_UPPER_SHIFT ) & TILE_UPPER_MASK )
#define TILE_SET_BITS(HI,LO)            (TILE_SET_UPPER_BITS(HI) | TILE_GET_LOWER_BITS(LO))

#define TILE_HAS_INVALID_IMAGE(XX)      HAS_SOME_BITS( TILE_UPPER_MASK, (XX).img )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_mesh_info
{
    int tiles_x, tiles_y;
};

//--------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------
struct s_line_data
{
    Uint8     start[MAXMESHTYPE];
    Uint8     end[MAXMESHTYPE];
};

//--------------------------------------------------------------------------------------------
struct s_mesh
{
    bool_t exploremode;

    int     tiles_x;                  // Size of mesh
    int     tiles_y;                  //
    Uint32  fanstart[MAXMESHTILEY];   // Y to fan number

    int     blocks_x;
    int     blocks_y;
    Uint32  blockstart[MAXMESHTILEY >> 2];

    float   edgex;            // Borders of mesh
    float   edgey;            //
    float   edgez;            //

    Uint8   fantype[MAXMESHFAN];        // Tile fan type
    Uint8   fx[MAXMESHFAN];             // Rile special effects flags
    Uint16  tx_bits[MAXMESHFAN];        // Tile texture bits and special tile bits
    Uint8   twist[MAXMESHFAN];          // Surface normal

    Uint32  vrtstart[MAXMESHFAN];     // Which vertex to start at
    Uint32  vrtnext[MAXTOTALMESHVERTICES];   // Next vertex in fan

    float   vrtx[MAXTOTALMESHVERTICES];      // Vertex position
    float   vrty[MAXTOTALMESHVERTICES];      //
    float   vrtz[MAXTOTALMESHVERTICES];      // Vertex elevation
    Uint8   vrta[MAXTOTALMESHVERTICES];      // Vertex base light, VERTEXUNUSED == unused

    Uint32       numline[MAXMESHTYPE];       // Number of lines to draw
    line_data_t  line[MAXMESHLINE];
    command_t    command[MAXMESHTYPE];
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern mesh_t mesh;
extern size_t numwritten;
extern size_t numattempt;
extern Uint32 numfreevertices;        // Number of free vertices

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void create_mesh( mesh_info_t * pinfo );
int  load_mesh( const char *modname );
void save_mesh( const char *modname );

void make_twist();
void load_mesh_fans();
void make_fanstart();

int  mesh_get_fan( int x, int y );
void num_free_vertex();
int  count_vertices();
void add_line( int fantype, int start, int end );

void free_vertices();
int get_free_vertex();

Uint8 get_twist( int x, int y );
Uint8 get_fan_twist( Uint32 fan );
int get_level( int x, int y );
int get_vertex( int x, int y, int num );
int fan_at( int x, int y );

void remove_fan( int fan );
int add_fan( int fan, float x, float y );
