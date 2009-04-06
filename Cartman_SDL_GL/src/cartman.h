#pragma once

#include <SDL_opengl.h>

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

#define VERSION 005         // Version number
#define YEAR 1999           // Year
#define NAME "Cartman"          // Program name
#define KEYDELAY 12         // Delay for keyboard
#define MAXTILE 256         //
#define TINYX 4 //8             // Plan tiles
#define TINYY 4 //8             //
#define SMALLX 32           // Small tiles
#define SMALLY 32           //
#define BIGX 64             // Big tiles
#define BIGY 64             //
#define CAMRATE 8           // Arrow key movement rate
#define MAXSELECT 2560          // Max points that can be select_vertsed
#define FOURNUM 4.137           // Magic number
#define FIXNUM  4.125 // 4.150      // Magic number
#define MAPID 0x4470614d        // The string... MapD

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

#define MAXMESHTYPE 64          // Number of mesh types
#define MAXMESHLINE 64          // Number of lines in a fan schematic
#define MAXMESHVERTICES 16      // Max number of vertices in a fan
#define MAXMESHFAN (512*512)        // Size of map in fans
#define MAXTOTALMESHVERTICES (MAXMESHFAN*MAXMESHVERTICES)
#define MAXMESHSIZEY 1024       // Max fans in y direction
#define MAXMESHTYPE 64          // Number of vertex configurations
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
