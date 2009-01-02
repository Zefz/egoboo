#include "egoboo_endian.h"

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

union u_convert {float f; Uint32 i;};

typedef union u_convert convert_t;

float ENDIAN_FLOAT(float X)
{
  convert_t utmp;

  utmp.f = X;

  utmp.i = SDL_SwapLE32(utmp.i);

  return utmp.f;
}
