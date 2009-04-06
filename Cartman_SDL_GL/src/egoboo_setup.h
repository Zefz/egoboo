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

#include "egoboo_typedef.h"
#include "SDL_extensions.h"
#include "configfile.h"

#include <SDL_opengl.h>

// OpenGL Texture filtering
typedef enum e_tx_filters
{
    TX_UNFILTERED,
    TX_LINEAR,
    TX_MIPMAP,
    TX_BILINEAR,
    TX_TRILINEAR_1,
    TX_TRILINEAR_2,
    TX_ANISOTROPIC
} TX_FILTERS;

struct s_config_data
{
    Uint8    reffadeor;        // 255 = Don't fade reflections

    Uint16   messagetime;
    int      maxtotalmeshvertices;  // max number of kilobytes of vertices

    bool_t   messageon;        // Messages?
    int      maxmessage;       //

    bool_t   overlayon;        // Draw overlay?
    bool_t   perspective;      // Perspective correct textures?
    bool_t   dither;           // Dithering?
    GLuint   shading;          // Gourad shading?
    bool_t   antialiasing;     // Antialiasing?
    bool_t   refon;            // Reflections?
    bool_t   shaon;            // Shadows?
    int      texturefilter;    // Texture filtering?
    bool_t   wateron;          // Water overlays?
    bool_t   shasprite;        // Shadow sprites?
    bool_t   zreflect;         // Reflection z buffering?

    int      maxsoundchannel;      // Max number of sounds playing at the same time
    int      buffersize;          // Buffer size set in setup.txt

    bool_t   fullscreen;       // Start in fullscreen?
    bool_t   backgroundon;    // Do we clear every time?
    bool_t   soundon;              // Is the sound alive?
    bool_t   staton;               // Draw the status bars?
    bool_t   phongon;              // Do phong overlay?
    bool_t   networkon;            // Try to connect?
    bool_t   serviceon;            // Do I need to free the interface?
    bool_t   twolayerwateron;      // Two layer water?

    Uint8    camautoturn;          // Type of camera control...
    bool_t   fogallowed;           //
    bool_t   fpson;                // FPS displayed?
    int      frame_limit;

    bool_t   backgroundvalid;      // Allow large background?
    bool_t   overlayvalid;         // Allow large overlay?

    int      lag;                  // Lag tolerance
    int      orderlag;             // Lag tolerance for RTS games
    size_t   mesh_vert_count;

    SDLX_screen_info_t scr;        // Requested screen parameters

    int    maxlights;              // Max number of lights to draw
    Uint16 maxparticles;           // max number of particles

    bool_t soundvalid;          // Allow sound?
    Sint32 soundvolume;         // The sound volume

    bool_t musicvalid;          // Allow music and loops?
    Sint32 musicvolume;         // The sound volume of music

    char   nethostname[64];                            // Name for hosting session
    char   netmessagename[64];                         // Name for messages

    int    wraptolerance;        // Status bar

    bool_t GrabMouse;
    bool_t HideMouse;
    bool_t DevMode;
};
typedef struct s_config_data config_data_t;

ConfigFilePtr_t setup_read( char* filename );
bool_t setup_write( ConfigFilePtr_t cfg_file);
bool_t setup_quit( ConfigFilePtr_t cfg_file );

bool_t setup_download(ConfigFilePtr_t cfg_file, config_data_t * pc);
bool_t setup_upload(ConfigFilePtr_t cfg_file, config_data_t * pc);

