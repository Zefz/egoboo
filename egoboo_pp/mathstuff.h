#pragma once

/**> HEADER FILES <**/
//#include "egoboo.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

using std::min;
using std::max;
using std::abs;

#include "egobootypedef.h"

/**> MACROS <**/
#define PI           3.1415926535897932384626433832795
#define TWO_PI       6.283185307179586476925286766559
#define PI_OVER_TWO  0.78539816339744830961566084581988
#define PI_OVER_FOUR 0.78539816339744830961566084581988
#define INV_TWO_PI   0.15915494309189533576888376337251
#define DEG_TO_RAD   0.017453292519943295769236907684886
#define RAD_TO_DEG   57.295779513082320876798154814105
#define THREE_PI_OVER_TWO 4.7123889803846898576939650749193

#define SQRT_TWO      1.4142135623730950488016887242097
#define INV_SQRT_TWO  0.70710678118654752440084436210485

#define TRIG_TO_SHORT (SHORT_SIZE*INV_TWO_PI)
#define SHORT_TO_TRIG (TWO_PI/SHORT_SIZE)

#define RAD_TO_ONE(x)   ((x+PI)*INV_TWO_PI)
#define RAD_TO_SHORT(x) ((x+PI)*TRIG_TO_SHORT)
#define SHORT_TO_RAD(x) (x*SHORT_TO_TRIG - PI)

#define FIXEDPOINT_BITS  8

//#define abs(X)  (((X) > 0) ? (X) : -(X))
#define CLIP(val,minval,maxval) MAX(MIN((val),(maxval)),(minval))

#ifndef ABS
#    define ABS(X)  (((X) > 0) ? (X) : -(X))
#endif

#ifndef SGN
#    define SGN(x) ((x)<0 ? -1 : 1)
#endif

/**> DATA STRUCTURES <**/
#pragma pack(push,1)
struct GLMatrix { float v[16]; float & CNV(int i, int j) { return v[4*i+j];}; };

struct vec2_t
{
  union{ float vals[2]; struct {float x,y; }; struct {float u,v; }; struct {float s,t; }; };
  vec2_t(float _x=0, float _y=0) { x = _x; y = _y;} 
};

struct vec3_t
{ 
  union{ float vals[3]; struct {float x,y,z; }; };   
  vec3_t(float _x=0, float _y=0, float _z=0) { x = _x; y = _y; z = _z; } 
};

struct vec4_t
{ 
  union{ float vals[4]; struct {float x,y,z,w; }; struct {float r,g,b,a; }; };  
  vec4_t(float _x=0, float _y=0, float _z=0, float _w=1) { x = _x; y = _y; z = _z; w = _w; } 
};

struct quaternion_t : public vec4_t
{
  quaternion_t(float _x=0, float _y=0, float _z=0, float _w=1) : vec4_t(_x, _y, _z, _w) {};
};

typedef vec4_t GLVector;
#pragma pack(pop)

template <typename _flt_type, int _bits = 16>
struct trig_table
{
  static const size_t size       = 1<<_bits;
  static const size_t branch_cut = 1<<(_bits-1);
  static const size_t mask       = size - 1;

  trig_table(_flt_type (*pfunc)(_flt_type), _flt_type min_domain = 0, _flt_type max_domain = TWO_PI)
    { initialize(pfunc,min_domain,max_domain); }

  _flt_type & operator [] (Sint32 i) { return _list[ (size_t)((i + branch_cut) & mask) ]; };

private:
  _flt_type _list[size];

  void initialize( _flt_type (*pfunc)(_flt_type), _flt_type min_domain = 0, _flt_type max_domain = TWO_PI);
};

template <typename _flt_type, int _bits = 16>
struct mapped_function_table
{
  static const size_t size       = 1<<_bits;
  static const size_t mask       = size - 1;

protected:
  _flt_type _list[size];
  void initialize( _flt_type (*pfunc)(_flt_type), _flt_type (*pinv_map)(_flt_type), _flt_type min_range = 0, _flt_type max_range = TWO_PI);
};

struct arctan_table : public mapped_function_table<float, 14>
{
  arctan_table()
  {
    initialize( atanf, _inverse_approximant, 0, PI_OVER_TWO );
  };

  float lookup(float tangent)
  {
    Uint32 value = _approximant(tangent)*size;
    return _list[size_t(value & mask)];
  };


  float lookup(float x, float y)
  {
    if(x == 0) return y<0 ? -PI_OVER_TWO : PI_OVER_TWO;

    float tangent = y/x;
    float arctangent = lookup(tangent);

    if(x<0) arctangent += PI;

    return arctangent;
  };

private:

  static float _approximant(float value)
  {
    return PI_OVER_TWO*value/(1 + fabsf(value));
  };

  static float _inverse_approximant(float value)
  {
    return value/(PI_OVER_TWO-fabsf(value));
  };

};

/**> GLOBAL VAR_IABLES <**/
extern trig_table<float, 14> sin_tab;  // Convert chrturn>>2...  to sine
extern trig_table<float, 14> cos_tab;  // Convert chrturn>>2...  to cosine
extern arctan_table atan_tab;

/**> FUNCTION PROTOTYPES <**/
inline GLVector vsub(GLVector & A, GLVector & B);
//inline GLVector Normalize(GLVector & vec);
inline GLVector CrossProduct(GLVector & A, GLVector & B);
inline float DotProduct(GLVector & A, GLVector & B);
inline GLMatrix IdentityMatrix(void);
inline GLMatrix ZeroMatrix(void);
inline GLMatrix MatrixMult(GLMatrix & a, GLMatrix & b);
inline GLMatrix Translate(const float dx, const float dy, const float dz);
inline GLMatrix RotateX(const float rads);
inline GLMatrix RotateY(const float rads);
inline GLMatrix RotateZ(const float rads);
inline GLMatrix ScaleXYZ(const float size_x, const float size_y, const float size_z);
inline GLMatrix FourPoints(vec3_t & ori, vec3_t & wid, vec3_t & forw, vec3_t & up, float scale);
//inline GLMatrix ViewMatrix(const GLVector & from, const GLVector & at, const GLVector & world_up, const float roll);
//inline GLMatrix ProjectionMatrix(const float near_plane, const float far_plane, const float fov);
inline void  TransformVertices( GLMatrix & Matrix, GLVector *pSourceV, GLVector *pDestV, Uint32 pNumVertor );
inline void CopyMatrix( GLMatrix & MatrixSource, GLMatrix & MatrixDest ) { memcpy( &MatrixDest, &MatrixSource, sizeof(GLMatrix) ); };

//----------------------------------------------------------------------------------------
inline vec2_t operator - (vec2_t & rhs);
inline vec2_t operator * (vec2_t & lhs, float rhs);
inline vec2_t operator * (float lhs, vec2_t & rhs);
inline vec2_t operator / (vec2_t & lhs, float rhs);
inline vec2_t operator - (vec2_t & lhs, vec2_t & rhs);
inline vec2_t operator + (vec2_t & lhs, vec2_t & rhs);
inline vec2_t operator * (vec2_t & lhs, vec2_t & rhs);
inline vec2_t operator / (vec2_t & lhs, vec2_t & rhs);
inline vec2_t & operator += (vec2_t & lhs, vec2_t & rhs);
inline vec2_t & operator -= (vec2_t & lhs, vec2_t & rhs);

inline vec2_t & operator *= (vec2_t & lhs, vec2_t & rhs);
inline vec2_t & operator /= (vec2_t & lhs, vec2_t & rhs);
inline vec2_t & operator *= (vec2_t & lhs, float rhs);
inline vec2_t & operator /= (vec2_t & lhs, float rhs);

//----------------------------------------------------------------------------------------
inline vec3_t operator - (vec3_t & rhs);
inline vec3_t operator * (vec3_t & lhs, float rhs);
inline vec3_t operator * (float lhs, vec3_t & rhs);
inline vec3_t operator / (vec3_t & lhs, float rhs);
inline vec3_t operator - (vec3_t & lhs, vec3_t & rhs);
inline vec3_t operator + (vec3_t & lhs, vec3_t & rhs);
inline vec3_t operator * (vec3_t & lhs, vec3_t & rhs);
inline vec3_t operator / (vec3_t & lhs, vec3_t & rhs);
inline vec3_t & operator += (vec3_t & lhs, vec3_t & rhs);
inline vec3_t & operator -= (vec3_t & lhs, vec3_t & rhs);

inline vec3_t & operator *= (vec3_t & lhs, vec3_t & rhs);
inline vec3_t & operator /= (vec3_t & lhs, vec3_t & rhs);
inline vec3_t & operator *= (vec3_t & lhs, float rhs);
inline vec3_t & operator /= (vec3_t & lhs, float rhs);

//----------------------------------------------------------------------------------------
inline vec3_t parallel_normalized(vec3_t & direction, vec3_t & vec);
inline vec3_t parallel(vec3_t & direction, vec3_t & vec);
inline float dot_product(vec3_t & lhs, vec3_t & rhs);
inline vec3_t cross_product(vec3_t & lhs, vec3_t & rhs);

inline float dist_abs(vec3_t & vec);
inline float dist_abs_horiz(vec3_t & vec);

inline float dist_squared(vec3_t & vec);
inline float dist_squared_horiz(vec3_t & vec);

inline float diff_abs(vec3_t & lhs, vec3_t & rhs);
inline float diff_abs_horiz(vec3_t & lhs, vec3_t & rhs);

inline float diff_squared(vec3_t & lhs, vec3_t & rhs);
inline float diff_squared_horiz(vec3_t & lhs, vec3_t & rhs);

inline Uint16 vals_to_turn(float xval, float yval);
inline Uint16 vec_to_turn(vec3_t & vec);
inline Uint16 diff_to_turn(vec3_t & lhs, vec3_t & rhs);

inline vec3_t turn_to_vec(Uint16 turn);

inline vec3_t matrix_mult(GLMatrix & mat, vec3_t & vec);
inline vec3_t matrix_mult_simple(GLMatrix & mat, vec3_t & vec);

inline bool normalize_iterative(vec3_t & vec, float tolerance = 1e-6, int max_iterations = 5);


//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
inline quaternion_t operator - (quaternion_t & rhs);
inline quaternion_t operator * (quaternion_t & lhs, float rhs);
inline quaternion_t operator * (float lhs, quaternion_t & rhs);
inline quaternion_t operator / (quaternion_t & lhs, float rhs);
inline quaternion_t operator - (quaternion_t & lhs, quaternion_t & rhs);
inline quaternion_t operator + (quaternion_t & lhs, quaternion_t & rhs);
inline quaternion_t operator * (quaternion_t & lhs, quaternion_t & rhs);
inline quaternion_t operator / (quaternion_t & lhs, quaternion_t & rhs);
inline quaternion_t & operator += (quaternion_t & lhs, quaternion_t & rhs);
inline quaternion_t & operator -= (quaternion_t & lhs, quaternion_t & rhs);

inline quaternion_t & operator *= (quaternion_t & lhs, quaternion_t & rhs);
inline quaternion_t & operator /= (quaternion_t & lhs, quaternion_t & rhs);
inline quaternion_t & operator *= (quaternion_t & lhs, float rhs);
inline quaternion_t & operator /= (quaternion_t & lhs, float rhs);

inline GLMatrix create_matrix(quaternion_t & rhs);
inline GLMatrix create_rotate_around_normalized(vec3_t & axis, float angle_rad);

#include "mathstuff.inl"
