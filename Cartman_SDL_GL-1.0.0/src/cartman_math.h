#pragma once

#include "egoboo_typedef.h"

#include <SDL.h>

#include <math.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#ifndef TWO_PI
#    define TWO_PI 3.1415926535897932384626433832795f
#endif

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

#ifndef HAS_BITS
#    define HAS_BITS(A, B) ( 0 != ((A)&(B)) )
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef float cart_vec_t[3];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t SDL_RectIntersect( SDL_Rect * src, SDL_Rect * dst, SDL_Rect * isect );
