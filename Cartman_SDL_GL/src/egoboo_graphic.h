#pragma once

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include <SDL_opengl.h>

#define MAXCHR 350
#define MAXWAVE 16

#define TILESOUNDTIME 16
#define TILEREAFFIRMAND  3

struct s_camera
{
    float x;       // the position of the center of the window
    float y;       //

    float w;       // the size of the window
    float h;       //

    float swing;
    float swingrate;
    float swingamp;

    GLfloat mModelview[16];
    GLfloat mProjection[16];
};
typedef struct s_camera camera_t;

extern camera_t cam;

struct s_simple_vertex
{
    GLfloat x, y, z;
    GLfloat s, t;
    GLfloat r, g, b, a;

    GLfloat l;
};
typedef struct s_simple_vertex simple_vertex_t;


#define MAXMESHRENDER                   1024          // Max number of tiles to draw

struct s_renderlist
{

    int             all_count;                               // Number to render, total
    Uint32          all[MAXMESHRENDER];                      // List of which to render, total

    int             ref_count;                               // Number to render, reflective
    Uint32          ref[MAXMESHRENDER];                      // List of which to render, reflective

    int             sha_count;                               // Number to render, shadow
    Uint32          sha[MAXMESHRENDER];                      // List of which to render, shadow

    int             dref_count;                              // Number to render, mirror
    Uint32          dref[MAXMESHRENDER];                     // List of which to render, mirror

    int             anim_count;                              // Number to render, animated
    Uint32          anim[MAXMESHRENDER];                     // List of which to render, animated

    int             water_count;                             // Number to render, water
    Uint32          water[MAXMESHRENDER];                    // List of which to render, water

    int             wall_count;                              // Number to render, wall
    Uint32          wall[MAXMESHRENDER];                     // List of which to render, wall

    int             impass_count;                            // Number to render, impass
    Uint32          impass[MAXMESHRENDER];                   // List of which to render, impass

    int             damage_count;                            // Number to render, damage
    Uint32          damage[MAXMESHRENDER];                   // List of which to render, damage

    int             slippy_count;                            // Number to render, slippy
    Uint32          slippy[MAXMESHRENDER];                   // List of which to render, slippy
};

typedef struct s_renderlist renderlist_t;

extern renderlist_t renderlist;

struct s_dolist
{
    int count;                         // How many in the list
    Uint16 which[MAXCHR];              // List of which characters to draw
};
typedef struct s_dolist dolist_t;

extern dolist_t dolist;

/*Special Textures*/
typedef enum e_tx_type
{
    TX_PARTICLE = 0,
    TX_TILE_0,
    TX_TILE_1,
    TX_TILE_2,
    TX_TILE_3,
    TX_WATER_TOP,
    TX_WATER_LOW,
    TX_PHONG,
    TX_LAST
} TX_TYPE;

// Weather and water gfx
#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME 512                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)             //
#define WATERPOINTS   4                               // Points in a water fan
#define WATERMODE     4                                 // Ummm...  For making it work, yeah...


extern int                     weatheroverwater;       // Only spawn over water?
extern int                     weathertimereset;          // Rate at which weather particles spawn
extern int                     weathertime;                // 0 is no weather
extern int                     weatherplayer;
extern int                     numwaterlayer;              // Number of layers
extern float                   watersurfacelevel;          // Surface level for water striders
extern float                   waterdouselevel;            // Surface level for torches
extern Uint8                   waterlight;                 // Is it light ( default is alpha )
extern Uint8                   waterspekstart;           // Specular begins at which light value
extern Uint8                   waterspeklevel;           // General specular amount (0-255)
extern Uint8                   wateriswater;          // Is it water?  ( Or lava... )
extern Uint8                   waterlightlevel[MAXWATERLAYER]; // General light amount (0-63)
extern Uint8                   waterlightadd[MAXWATERLAYER];   // Ambient light amount (0-63)
extern float                   waterlayerz[MAXWATERLAYER];     // Base height of water
extern Uint8                   waterlayeralpha[MAXWATERLAYER]; // Transparency
extern float                   waterlayeramp[MAXWATERLAYER];   // Amplitude of waves
extern float                   waterlayeru[MAXWATERLAYER];     // Coordinates of texture
extern float                   waterlayerv[MAXWATERLAYER];     //
extern float                   waterlayeruadd[MAXWATERLAYER];  // Texture movement
extern float                   waterlayervadd[MAXWATERLAYER];  //
extern float                   waterlayerzadd[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
extern Uint8                   waterlayercolor[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
extern Uint16                  waterlayerframe[MAXWATERLAYER];         // Frame
extern Uint16                  waterlayerframeadd[MAXWATERLAYER];      // Speed
extern float                   waterlayerdistx[MAXWATERLAYER];         // For distant backgrounds
extern float                   waterlayerdisty[MAXWATERLAYER];         //
extern Uint32                  waterspek[256];                         // Specular highlights
extern float                   foregroundrepeat;                       //
extern float                   backgroundrepeat;                       //

// Fog stuff
extern bool_t                  fogallowed;         //
extern bool_t                  fogon;              // Do ground fog?
extern float                   fogbottom;          //
extern float                   fogtop;             //
extern float                   fogdistance;        //
extern Uint8                   fogred;             //  Fog collour
extern Uint8                   foggrn;             //
extern Uint8                   fogblu;             //
extern Uint8                   fogaffectswater;

extern bool_t overlayon;
extern bool_t clearson;        // Do we clear every time?
extern bool_t backgroundon;    // Do we clear every time?
extern bool_t wateron;
extern bool_t twolayerwateron;        // Two layer water?
extern bool_t usefaredge;
extern GLenum shading;

void render_fan( int fan );
void render_water_fan( int tile, Uint8 layer );
void read_wawalite( char *modname );

extern float           hillslide;                      //
extern float           slippyfriction;                 // Friction
extern float           airfriction;                    //
extern float           waterfriction;                  //
extern float           noslipfriction;                 //
extern float           platstick;                      //
extern float           gravity;                        // Gravitational accel

extern float kMd2Normals[][3];