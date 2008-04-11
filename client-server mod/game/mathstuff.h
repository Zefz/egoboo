/* Egoboo - mathstuff.h
 * The name's pretty self explanatory, doncha think?
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

#ifndef _MATHSTUFF_H_
#define _MATHSTUFF_H_


/**> HEADER FILES <**/
//#include "egoboo.h"
#include <math.h>
#include "egobootypedef.h"

#define PI                  3.1415926535897932384626433832795f
#define TWO_PI              6.283185307179586476925286766559f
#define PI_OVER_TWO         1.5707963267948966192313216916398f
#define PI_OVER_FOUR        0.78539816339744830961566084581988f
#define RAD_TO_SHORT        ((float)65535 / TWO_PI)
#define RAD_TO_BYTE         ((float)256   / TWO_PI)
#define DEG_TO_RAD          0.017453292519943295769236907684886f
#define RAD_TO_DEG          57.295779513082320876798154814105

#define RAD_TO_TURN(XX)     ((Uint16)((XX + PI) * RAD_TO_SHORT))

#define ABS(X)  (((X) > 0) ? (X) : -(X))

/* Neither Linux nor Mac OS X seem to have MIN and MAX defined, so if they
 * haven't already been found, define them here. */
#ifndef MAX
#define MAX(a,b) ( ((a)>(b))? (a):(b) )
#endif

#ifndef MIN
#define MIN(a,b) ( ((a)>(b))? (b):(a) )
#endif

/**> MACROS <**/
#define _CNV(i,j) .v[4*i+j]
#define CopyMatrix( pMatrixSource, pMatrixDest ) memcpy( (pMatrixDest), (pMatrixSource), sizeof( GLMATRIX ) )


/**> DATA STRUCTURES <**/
typedef struct glmatrix { float v[16]; } GLMATRIX;
typedef struct glvector { float x, y, z, w; } GLVECTOR;


/**> GLOBAL VARIABLES <**/
#define TRIGTABLE_SIZE (1<<14)
#define TRIGTABLE_MASK (TRIGTABLE_SIZE-1)
extern float turntosin[TRIGTABLE_SIZE];           // Convert chrturn>>2...  to sine


/**> FUNCTION PROTOTYPES <**/
GLVECTOR vsub(GLVECTOR A, GLVECTOR B);
GLVECTOR Normalize(GLVECTOR vec);
GLVECTOR CrossProduct(GLVECTOR A, GLVECTOR B);
float DotProduct(GLVECTOR A, GLVECTOR B);
GLMATRIX IdentityMatrix(void);
GLMATRIX ZeroMatrix(void);
GLMATRIX MatrixMult(const GLMATRIX a, const GLMATRIX b);
GLMATRIX Translate(const float dx, const float dy, const float dz);
GLMATRIX RotateX(const float rads);
GLMATRIX RotateY(const float rads);
GLMATRIX RotateZ(const float rads);
GLMATRIX ScaleXYZ(const float sizex, const float sizey, const float sizez);
GLMATRIX ScaleXYZRotateXYZTranslate(const float sizex, const float sizey, const float sizez, Uint16 turnz, Uint16 turnx, Uint16 turny, float tx, float ty, float tz);
GLMATRIX FourPoints(float orix, float oriy, float oriz, float widx, float widy, float widz, float forx, float fory, float forz, float upx,  float upy,  float upz, float scale);
GLMATRIX ViewMatrix(const GLVECTOR from, const GLVECTOR at, const GLVECTOR world_up, const float roll);
GLMATRIX ProjectionMatrix(const float near_plane, const float far_plane, const float fov);
void TransformVertices(GLMATRIX *pMatrix, GLVECTOR *pSourceV, GLVECTOR *pDestV, Uint32 pNumVertor);


// My lil' random number table
#define RANDIE_BITS  12
#define RANDIE_MAX  (1<<RANDIE_BITS)
#define RANDIE_MASK ((1<<RANDIE_BITS)-1)
extern Uint16 randie_table[RANDIE_MAX];
extern  Uint16 randie_index;
#define RANDIE(index) (index=(index+1)&RANDIE_MASK, randie_table[index])
void make_randie();

extern Uint32 ego_rand_seed;
Uint32 ego_rand(Uint32 * seed);

#endif

