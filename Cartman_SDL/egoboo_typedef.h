#pragma once

/* Egoboo - egobootypedef.h
 * Defines some basic types that are used throughout the game code.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo_config.h"

#include <SDL_types.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// pseudo functions for adding C++-like new and delete to C

#if defined(__cplusplus)
#    define EGOBOO_NEW( TYPE ) new TYPE
#    define EGOBOO_NEW_ARY( TYPE, COUNT ) new TYPE [ COUNT ]
#    define EGOBOO_DELETE(PTR) if(NULL != PTR) { delete PTR; PTR = NULL; }
#    define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { delete [] PTR; PTR = NULL; }
#else
//#    if USE_DEBUG
#        define EGOBOO_NEW( TYPE ) (TYPE *)calloc(1, sizeof(TYPE))
#        define EGOBOO_NEW_ARY( TYPE, COUNT ) (TYPE *)calloc(COUNT, sizeof(TYPE))
#        define EGOBOO_DELETE(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
#        define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
//#    else
//#        define EGOBOO_NEW( TYPE ) (TYPE *)malloc( sizeof(TYPE) )
//#        define EGOBOO_NEW_ARY( TYPE, COUNT ) (TYPE *)malloc(COUNT * sizeof(TYPE))
//#        define EGOBOO_DELETE(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
//#        define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
//#    endif
#endif

#define CLIP_UINT08(I) (Uint8 )((I) & 0xFF )
#define CLIP_UINT16(I) (Uint16)((I) & 0xFFFF )
#define CLIP_UINT32(I) (Uint32)((I) & 0xFFFFFFFF )

// RECTANGLE
typedef struct s_rect
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
} rect_t;

// BOOLEAN
typedef int bool_t;
enum
{
  btrue = ( 1 == 1 ),
  bfalse = !btrue
};

// IDSZ
typedef Uint32     IDSZ_t;
#define IDSZ_NONE  Make_IDSZ("NONE")
#ifndef Make_IDSZ
#define Make_IDSZ(string) ((IDSZ_t)((((string)[0]-'A') << 15) | (((string)[1]-'A') << 10) | (((string)[2]-'A') << 5) | (((string)[3]-'A') << 0)))
#endif

//STRING
typedef char STRING[256];

typedef Uint16 UFixedPt_t;
typedef Sint16 SFixedPt_t;

//FAST CONVERSIONS
#define FP8_ONE            0x0100
#define FP8_TO_FLOAT(XX)   ( (float)(XX)/(float)(1<<8) )
#define FLOAT_TO_FP8(XX)   ( (Uint32)((XX)*(float)(1<<8)) )
#define FP8_TO_INT(XX)     ( (XX) >> 8 )                      // fast version of XX / 256
#define INT_TO_FP8(XX)     ( (XX) << 8 )                      // fast version of XX * 256
#define FP8_MUL(XX, YY)    ( ((XX)*(YY)) >> 8 )
#define FP8_DIV(XX, YY)    ( ((XX)<<8) / (YY) )
