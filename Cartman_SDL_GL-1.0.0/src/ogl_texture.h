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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

///
/// @file
/// @brief Basic OpenGL Wrapper
/// @details Basic definitions for loading and managing OpenGL textures in Egoboo.
///   Uses SDL_image to load .tif, .png, .bmp, .dib, .xpm, and other formats into
///   OpenGL texures

//#include "egoboo_memory.h"

#include "ogl_include.h"
#include "ogl_debug.h"
#include "egoboo_typedef.h"

#include <SDL.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define INVALID_TX_ID  ( (GLuint) (~0) )
#define INVALID_KEY    ( (Uint32) (~0) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_glTexture
{
    gl_texture_t base;

    STRING        name;       // the name of the original file
    SDL_Surface * surface;    // the original texture data
    int           imgW, imgH; // the height & width of the texture data
    GLfloat       alpha;      // the alpha for the texture
};
typedef struct s_glTexture glTexture;

Uint32  glTexture_Convert( GLenum tx_target, glTexture *texture, SDL_Surface * image, Uint32 key );
Uint32  glTexture_Load( GLenum tx_target, glTexture *texture, const char *filename, Uint32 key );
GLuint  glTexture_GetTextureID( glTexture *texture );
GLsizei glTexture_GetImageHeight( glTexture *texture );
GLsizei glTexture_GetImageWidth( glTexture *texture );
GLsizei glTexture_GetTextureWidth( glTexture *texture );
GLsizei glTexture_GetTextureHeight( glTexture *texture );
void    glTexture_SetAlpha( glTexture *texture, GLfloat alpha );
GLfloat glTexture_GetAlpha( glTexture *texture );
void    glTexture_Release( glTexture *texture );

void    glTexture_Bind( glTexture * texture );

glTexture * glTexture_new( glTexture * texture );
void        glTexture_delete( glTexture * texture );
