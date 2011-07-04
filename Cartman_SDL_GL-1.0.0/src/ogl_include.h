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
/// @details Basic definitions for using OpenGL in Egoboo

#include <SDL_opengl.h>

#ifdef __cplusplus
#    include <cassert>
#    include <cstdio>
extern "C"
{
#else
#    include <assert.h>
#    include <stdio.h>
#endif

#if defined(DEBUG_ATTRIB) && USE_DEBUG
#    define ATTRIB_PUSH(TXT, BITS)    { GLint xx=0; glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); glPushAttrib(BITS); fprintf( stdout, "INFO: PUSH  ATTRIB: %s before attrib stack push. level == %d\n", TXT, xx); }
#    define ATTRIB_POP(TXT)           { GLint xx=0; glPopAttrib(); glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); fprintf( stdout, "INFO: POP   ATTRIB: %s after attrib stack pop. level == %d\n", TXT, xx); }
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX); fprintf( stdout, "INFO: OPEN ATTRIB_GUARD: before attrib stack push. level == %d\n", XX); }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); if(XX!=YY) { fprintf( stderr, "ERROR: CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); exit(-1); } else fprintf( stdout, "INFO: CLOSE ATTRIB_GUARD: after attrib stack pop. level == %d\n", XX); }
#elif USE_DEBUG
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX);  }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); assert(XX==YY); if(XX!=YY) { fprintf( stderr, "ERROR: CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); exit(-1); }  }
#else
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)
#    define ATTRIB_GUARD_CLOSE(XX,YY)
#endif

//--------------------------------------------------------------------------------------------
    typedef float glMatrix[16];
    typedef float glVector4[4];
    typedef float glVector3[3];
    typedef float glVector2[3];

//--------------------------------------------------------------------------------------------
    struct glVertex
    {
        glVector4 pos;
        glVector3 rt, up;
        GLfloat   dist;
        glVector4 col;
        GLuint    color; // should replace r,g,b,a and be called by glColor4ubv
        glVector2 tx; // u and v in D3D I guess
    };

//--------------------------------------------------------------------------------------------
// generic OpenGL lighting struct
    struct s_glLight
    {
        glVector4 emission, diffuse, specular;
        float     shininess[1];
    };
    typedef struct s_glLight glLight_t;

//--------------------------------------------------------------------------------------------
    GLboolean handle_opengl_error( void );

    void ViewMatrix( glMatrix view,
                     const glVector3 from,      // camera location
                     const glVector3 at,        // camera look-at target
                     const glVector3 world_up,  // world’s up, usually 0, 0, 1
                     const GLfloat roll );      // clockwise roll around viewing direction, in radians

    void ProjectionMatrix( glMatrix proj,
                           const GLfloat near_plane,    // distance to near clipping plane
                           const GLfloat far_plane,     // distance to far clipping plane
                           const GLfloat fov );         // field of view angle, in radians

#ifdef __cplusplus
};
#endif


