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
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef Egoboo_egobootypedef_h
#define Egoboo_egobootypedef_h

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN    // Speeds up compile times a bit.  We don't need everything
// in windows.h

// Turn off warnings that we don't care about.
#pragma warning(disable : 4305) // truncation from 'double' to 'float'
#pragma warning(disable : 4244) // conversion from 'double' to 'float'
#pragma warning(disable : 4554) // possibly operator precendence error
//#pragma warning(disable : 4761)

// Windows defines snprintf as _snprintf; that's kind of a pain, so redefine it here
#define snprintf _snprintf

#endif

#include <SDL.h>
#include <SDL_endian.h>

extern float LoadFloatByteswapped(float *ptr);

typedef struct rect_t
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
} rect_t;

typedef enum bool_e
{
  btrue  = (1==1),
  bfalse = (!btrue)
} bool_t;

typedef enum retval_e
{
  rv_error    = -1,
  rv_fail     = bfalse,
  rv_succeed  = btrue,
  rv_waiting  = 2
} retval_t;


typedef char STRING[256];

typedef Uint32 IDSZ;

#ifndef MAKE_IDSZ
#define MAKE_IDSZ(idsz) ((IDSZ)((((idsz)[0]-'A') << 15) | (((idsz)[1]-'A') << 10) | (((idsz)[2]-'A') << 5) | (((idsz)[3]-'A') << 0)))
#endif


#endif // include guard

