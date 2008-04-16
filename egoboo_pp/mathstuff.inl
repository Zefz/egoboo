#include "Physics.h"

//--------------------------------------------------------------------------------------------
template <typename _flt_type, int _bits>
void trig_table<_flt_type, _bits>::initialize( _flt_type (*pfunc)(_flt_type), _flt_type min_domain, _flt_type max_domain)
{
    // ZZ> This function makes the lookup table for chrturn...
    size_t cnt;
    _flt_type tmp;

    for(cnt=0; cnt<size; cnt++)
    {
      tmp = _flt_type(max_domain-min_domain)*_flt_type(cnt + branch_cut)/_flt_type(size) + _flt_type(min_domain);
      _list[cnt] = (*pfunc)( tmp );
    }
};

//--------------------------------------------------------------------------------------------
template <typename _flt_type, int _bits>
void mapped_function_table<_flt_type, _bits>::initialize( _flt_type (*pfunc)(_flt_type), _flt_type (*pinv_map)(_flt_type), _flt_type min_range, _flt_type max_range)
{
   // BB> This function prepares a lookup-table / approximant pair for fast function lookup.
   //     Intended to smoothly map functions with infinite domains onto a finite-domain lookup table.
   //     The approximant function should be single-valued and invertable over the domain of interest.
   
    size_t cnt;
    _flt_type range_val, domain_val;

    for(cnt=0; cnt<size; cnt++)
    {
      range_val = _flt_type(max_range-min_range)*_flt_type(cnt)/_flt_type(size) + _flt_type(min_range);
      domain_val = (*pinv_map)( range_val );
      _list[cnt] = (*pfunc)( domain_val );
    }
};




//--------------------------------------------------------------------------------------------
inline GLVector vsub(GLVector & A, GLVector & B)
{
  GLVector tmp;
  tmp.x=A.x-B.x; tmp.y=A.y-B.y; tmp.z=A.z-B.z;
  return(tmp);
}

//--------------------------------------------------------------------------------------------
//inline GLVector Normalize(GLVector & vec)
//{
//  GLVector tmp=vec;
//  float len;
//  len= (float)sqrtf(vec.x*vec.x+vec.y*vec.y+vec.z*vec.z);
//  tmp.x/=len;
//  tmp.y/=len;
//  tmp.z/=len;
//  return(tmp);
//}

//--------------------------------------------------------------------------------------------
inline GLVector CrossProduct(GLVector & A, GLVector & B)
{
  GLVector tmp;
  tmp.x = A.y*B.z - A.z*B.y;
  tmp.y = A.z*B.x - A.x*B.z;
  tmp.z = A.x*B.y - A.y*B.x;
  return(tmp);
}

//--------------------------------------------------------------------------------------------
inline float DotProduct(GLVector & A, GLVector & B)
{ return A.x*B.x+A.y*B.y+A.z*B.z; }

//---------------------------------------------------------------------------------------------
//Math Stuff-----------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//inline D3DMATRIX IdentityMatrix()
inline GLMatrix IdentityMatrix()
{
  GLMatrix tmp;

  tmp.CNV(0,0)=1; tmp.CNV(1,0)=0; tmp.CNV(2,0)=0; tmp.CNV(3,0)=0;
  tmp.CNV(0,1)=0; tmp.CNV(1,1)=1; tmp.CNV(2,1)=0; tmp.CNV(3,1)=0;
  tmp.CNV(0,2)=0; tmp.CNV(1,2)=0; tmp.CNV(2,2)=1; tmp.CNV(3,2)=0;
  tmp.CNV(0,3)=0; tmp.CNV(1,3)=0; tmp.CNV(2,3)=0; tmp.CNV(3,3)=1;
  return(tmp);
}

//--------------------------------------------------------------------------------------------
//inline D3DMATRIX ZeroMatrix(void)  // initializes matrix to zero
inline GLMatrix ZeroMatrix(void)
{
  GLMatrix ret;
  int i,j;

  for (i=0; i<4; i++)
    for (j=0; j<4; j++)
    {
      ret.CNV(i,j)=0;
    };

  return ret;
}

//--------------------------------------------------------------------------------------------
//inline D3DMATRIX MatrixMult(const D3DMATRIX a, const D3DMATRIX b)
inline GLMatrix MatrixMult(GLMatrix & a, GLMatrix & b)
{
  int i,j,k;
  GLMatrix ret;

  for (i=0; i<4; i++)
  {
    for (j=0; j<4; j++)
    {
      ret.CNV(i,j) = 0;
      for (k=0; k<4; k++)
      {
        ret.CNV(i,j) += a.CNV(k,j) * b.CNV(i,k);
      }
    }
  }

  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX Translate(const float dx, const float dy, const float dz)
inline GLMatrix Translate(const float dx, const float dy, const float dz)
{
  GLMatrix ret = IdentityMatrix();
  ret.CNV(3,0) = dx;
  ret.CNV(3,1) = dy;
  ret.CNV(3,2) = dz;
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX RotateX(const float rads)
inline GLMatrix RotateX(const float rads)
{
  float cosine = (float)cos(rads);
  float sine = (float)sin(rads);
  GLMatrix ret = IdentityMatrix();
  ret.CNV(1,1) = cosine;
  ret.CNV(2,2) = cosine;
  ret.CNV(1,2) = -sine;
  ret.CNV(2,1) = sine;
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX RotateY(const float rads)
inline GLMatrix RotateY(const float rads)
{
  float cosine = (float)cos(rads);
  float sine = (float)sin(rads);
  GLMatrix ret = IdentityMatrix();
  ret.CNV(0,0) = cosine;      //0,0
  ret.CNV(2,2) = cosine;      //2,2
  ret.CNV(0,2) = sine;      //0,2
  ret.CNV(2,0) = -sine;      //2,0
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX RotateZ(const float rads)
inline GLMatrix RotateZ(const float rads)
{
  float cosine = (float)cos(rads);
  float sine = (float)sin(rads);
  GLMatrix ret = IdentityMatrix();
  ret.CNV(0,0) = cosine;      //0,0
  ret.CNV(1,1) = cosine;      //1,1
  ret.CNV(0,1) = -sine;      //0,1
  ret.CNV(1,0) = sine;      //1,0
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX ScaleXYZ(const float size_x, const float size_y, const float size_z)
inline GLMatrix ScaleXYZ(const float size_x, const float size_y, const float size_z)
{
  GLMatrix ret = IdentityMatrix();
  ret.CNV(0,0) = size_x;      //0,0
  ret.CNV(1,1) = size_y;      //1,1
  ret.CNV(2,2) = size_z;      //2,2
  return ret;
}

//--------------------------------------------------------------------------------------------
//D3DMATRIX FourPoints(float orix, float oriy, float oriz,
inline GLMatrix FourPoints(vec3_t & ori, vec3_t & wid, vec3_t & forw, vec3_t & up, float scale)
{
  GLMatrix tmp;

  wid  -= ori;  // "x"
  forw -= ori;  // "y"
  up   -= ori;  // "z"

  // make sure the vectors are mutually perpendicular
  // assuming that the up vector is pointing in the correct direction

  forw = cross_product(up,wid);
  wid  = cross_product(forw,up);
 
  normalize_iterative(wid);
  wid *=-scale;

  normalize_iterative(forw);
  forw *= scale;

  normalize_iterative(up);
  up *= scale;

  tmp.CNV(0,0) = wid.x;
  tmp.CNV(0,1) = wid.y;
  tmp.CNV(0,2) = wid.z;
  tmp.CNV(0,3) = 0;     
  tmp.CNV(1,0) = forw.x;  
  tmp.CNV(1,1) = forw.y;  
  tmp.CNV(1,2) = forw.z;  
  tmp.CNV(1,3) = 0;     
  tmp.CNV(2,0) = up.x;   
  tmp.CNV(2,1) = up.y;   
  tmp.CNV(2,2) = up.z;   
  tmp.CNV(2,3) = 0;     
  tmp.CNV(3,0) = ori.x;  
  tmp.CNV(3,1) = ori.y;  
  tmp.CNV(3,2) = ori.z;  
  tmp.CNV(3,3) = 1;     

  return tmp;
}

//--------------------------------------------------------------------------------------------
// MN This probably should be replaced by a call to gluLookAt, don't see why we need to make our own...
//

//inline GLMatrix ViewMatrix( GLVector & from,      // Camera location
//                    GLVector & at,        // Camera look-at target
//                    GLVector & world_up,  // world’s up, usually 0, 0, 1
//                    const float roll)     // clockwise roll around viewing direction, in radians
//{
//  GLMatrix view = IdentityMatrix();
//  GLVector up, right, view_dir;
//
//  view_dir = Normalize( vsub(at,from) );
//  right = CrossProduct(world_up, view_dir);
//  up = CrossProduct(view_dir, right);
//  right = Normalize(right);
//  up = Normalize(up);
//  view.CNV(0,0) = right.x;            //0,0
//  view.CNV(1,0) = right.y;            //1,0
//  view.CNV(2,0) = right.z;            //2,0
//  view.CNV(0,1) = up.x;              //0,1
//  view.CNV(1,1) = up.y;              //1,1
//  view.CNV(2,1) = up.z;              //2,1
//  view.CNV(0,2) = view_dir.x;            //0,2
//  view.CNV(1,2) = view_dir.y;            //1,2
//  view.CNV(2,2) = view_dir.z;          //2,2
//  view.CNV(3,0) = -DotProduct(right, from);    //3,0
//  view.CNV(3,1) = -DotProduct(up, from);      //3,1
//  view.CNV(3,2) = -DotProduct(view_dir, from);  //3,2
//
//  if (roll != 0.0f)
//  {
//    // MatrixMult function shown above
//    view = MatrixMult(RotateZ(-roll), view);
//  }
//
//  return view;
//}

//--------------------------------------------------------------------------------------------
// MN Again, there is a gl function for this, glFrustum or gluPerspective... does this account for viewport ratio?
//
//inline D3DMATRIX ProjectionMatrix(const float near_plane,     // distance to near clipping plane
//inline GLMatrix ProjectionMatrix( const float near_plane,     // distance to near clipping plane
//                          const float far_plane,      // distance to far clipping plane
//                          const float fov)            // field of view angle, in radians
//{
//  float c = (float)cos(fov*0.5);
//  float s = (float)sin(fov*0.5);
//  float Q = s/(1.0f - near_plane/far_plane);
//  GLMatrix ret = ZeroMatrix();
//  ret.CNV(0,0) = c;              //0,0
//  ret.CNV(1,1) = c;              //1,1
//  ret.CNV(2,2) = Q;              //2,2
//  ret.CNV(3,2) = -Q*near_plane;        //3,2
//  ret.CNV(2,3) = s;              //2,3
//  return ret;
//}


//----------------------------------------------------
// GS - Normally we souldn't this function but I found it in the rendering of the particules.
//
// This is just a MulVectorMatrix for now. The W division and screen size multiplication
// must be done afterward.
// Isn't tested!!!!
inline void  TransformVertices( GLMatrix & Matrix, GLVector * pSourceV, GLVector * pDestV, Uint32 NumVertor )
{
  while ( NumVertor > 0 )
  {
    pDestV->x = pSourceV->x * Matrix.v[0] + pSourceV->y * Matrix.v[4] + pSourceV->z * Matrix.v[8] + pSourceV->w * Matrix.v[12];
    pDestV->y = pSourceV->x * Matrix.v[1] + pSourceV->y * Matrix.v[5] + pSourceV->z * Matrix.v[9] + pSourceV->w * Matrix.v[13];
    pDestV->z = pSourceV->x * Matrix.v[2] + pSourceV->y * Matrix.v[6] + pSourceV->z * Matrix.v[10] + pSourceV->w * Matrix.v[14];
    pDestV->w = pSourceV->x * Matrix.v[3] + pSourceV->y * Matrix.v[7] + pSourceV->z * Matrix.v[11] + pSourceV->w * Matrix.v[15];
    pDestV++;
    pSourceV++;
    NumVertor--;
  }
}

//----------------------------------------------------
//----------------------------------------------------

inline vec2_t operator - (vec2_t & rhs)
{
  return vec2_t(-rhs.x, -rhs.y);
}

inline vec2_t operator * (vec2_t & lhs, float rhs)
{
  return vec2_t(lhs.x*rhs, lhs.y*rhs);
};

inline vec2_t operator * (float lhs, vec2_t & rhs)
{
  return vec2_t(lhs*rhs.x, lhs*rhs.y);
};

inline vec2_t operator / (vec2_t & lhs, float rhs)
{
  return vec2_t(lhs.x/rhs, lhs.y/rhs);
};

inline vec2_t operator - (vec2_t & lhs, vec2_t & rhs)
{
  return vec2_t(lhs.x-rhs.x, lhs.y-rhs.y);
};

inline vec2_t operator + (vec2_t & lhs, vec2_t & rhs)
{
  return vec2_t(lhs.x+rhs.x, lhs.y+rhs.y);
};

inline vec2_t operator * (vec2_t & lhs, vec2_t & rhs)
{
  return vec2_t(lhs.x*rhs.x, lhs.y*rhs.y);
};

inline vec2_t operator / (vec2_t & lhs, vec2_t & rhs)
{
  return vec2_t(lhs.x/rhs.x, lhs.y/rhs.y);
};

inline vec2_t & operator += (vec2_t & lhs, vec2_t & rhs)
{
  lhs.x += rhs.x; lhs.y += rhs.y;
  return lhs;
};

inline vec2_t & operator -= (vec2_t & lhs, vec2_t & rhs)
{
  lhs.x -= rhs.x; lhs.y -= rhs.y;
  return lhs;
};

inline vec2_t & operator *= (vec2_t & lhs, float rhs)
{
  lhs.x *= rhs; lhs.y *= rhs;
  return lhs;
};

inline vec2_t & operator /= (vec2_t & lhs, float rhs)
{
  lhs.x /= rhs; lhs.y /= rhs;
  return lhs;
};

inline vec2_t & operator *= (vec2_t & lhs, vec2_t & rhs)
{
  lhs.x *= rhs.x; lhs.y *= rhs.y;
  return lhs;
};

inline vec2_t & operator /= (vec2_t & lhs, vec2_t & rhs)
{
  lhs.x /= rhs.x; lhs.y /= rhs.y;
  return lhs;
};

//----------------------------------------------------
//----------------------------------------------------
inline vec3_t operator - (vec3_t & rhs)
{
  return vec3_t(-rhs.x, -rhs.y, -rhs.z);
}


inline vec3_t operator * (vec3_t & lhs, float rhs)
{
  return vec3_t(lhs.x*rhs, lhs.y*rhs, lhs.z*rhs);
};

inline vec3_t operator * (float lhs, vec3_t & rhs)
{
  return vec3_t(lhs*rhs.x, lhs*rhs.y, lhs*rhs.z);
};

inline vec3_t operator / (vec3_t & lhs, float rhs)
{
  return vec3_t(lhs.x/rhs, lhs.y/rhs, lhs.z/rhs);
};

inline vec3_t operator - (vec3_t & lhs, vec3_t & rhs)
{
  return vec3_t(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z);
};

inline vec3_t operator + (vec3_t & lhs, vec3_t & rhs)
{
  return vec3_t(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z);
};

inline vec3_t operator * (vec3_t & lhs, vec3_t & rhs)
{
  return vec3_t(lhs.x*rhs.x, lhs.y*rhs.y, lhs.z*rhs.z);
};

inline vec3_t operator / (vec3_t & lhs, vec3_t & rhs)
{
  return vec3_t(lhs.x/rhs.x, lhs.y/rhs.y, lhs.z/rhs.z);
};

inline vec3_t & operator += (vec3_t & lhs, vec3_t & rhs)
{
  lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z;
  return lhs;
};

inline vec3_t & operator -= (vec3_t & lhs, vec3_t & rhs)
{
  lhs.x -= rhs.x; lhs.y -= rhs.y; lhs.z -= rhs.z;
  return lhs;
};

inline vec3_t & operator *= (vec3_t & lhs, float rhs)
{
  lhs.x *= rhs; lhs.y *= rhs; lhs.z *= rhs;
  return lhs;
};

inline vec3_t & operator /= (vec3_t & lhs, float rhs)
{
  lhs.x /= rhs; lhs.y /= rhs; lhs.z /= rhs;
  return lhs;
};

inline vec3_t & operator *= (vec3_t & lhs, vec3_t & rhs)
{
  lhs.x *= rhs.x; lhs.y *= rhs.y; lhs.z *= rhs.z;
  return lhs;
};

inline vec3_t & operator /= (vec3_t & lhs, vec3_t & rhs)
{
  lhs.x /= rhs.x; lhs.y /= rhs.y; lhs.z /= rhs.z;
  return lhs;
};

//--------------------------------------------------------------------------------------------
inline vec3_t parallel_normalized(vec3_t & direction, vec3_t & vec)
{
  float dotprod = dot_product(direction, vec);

  return direction * dotprod;
}

//--------------------------------------------------------------------------------------------
inline vec3_t parallel(vec3_t & direction, vec3_t & vec)
{
  vec3_t loc_direction = direction;
  normalize_iterative(loc_direction);
  float dotprod2 = dot_product(loc_direction, vec);

  return loc_direction * dotprod2;
}

//--------------------------------------------------------------------------------------------
inline float dot_product(vec3_t & lhs, vec3_t & rhs)
{
  return lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z;
}

//--------------------------------------------------------------------------------------------
inline vec3_t cross_product(vec3_t & lhs, vec3_t & rhs)
{
  vec3_t tmp;
  tmp.x = lhs.y*rhs.z - lhs.z*rhs.y;
  tmp.y = lhs.z*rhs.x - lhs.x*rhs.z;
  tmp.z = lhs.x*rhs.y - lhs.y*rhs.x;
  return(tmp);
}

//--------------------------------------------------------------------------------------------

inline float dist_abs(vec3_t & vec)
{
  return fabsf(vec.x) + fabsf(vec.y) + fabsf(vec.z);
};

inline float dist_abs_horiz(vec3_t & vec)
{
  return fabsf(vec.x) + fabsf(vec.y);
};

inline float dist_squared(vec3_t & vec)
{
  return vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
}

inline float dist_squared_horiz(vec3_t & vec)
{
  return vec.x*vec.x + vec.y*vec.y;
}

inline float diff_abs(vec3_t & lhs, vec3_t & rhs)
{
  return fabsf(lhs.x-rhs.x) + fabsf(lhs.y-rhs.y) + fabsf(lhs.z-rhs.z) ;
}

inline float diff_abs_horiz(vec3_t & lhs, vec3_t & rhs)
{
  return fabsf(lhs.x-rhs.x) + fabsf(lhs.y-rhs.y);
}

inline float diff_squared(vec3_t & lhs, vec3_t & rhs)
{
  return (lhs.x-rhs.x)*(lhs.x-rhs.x) + (lhs.y-rhs.y)*(lhs.y-rhs.y) + (lhs.z-rhs.z)*(lhs.z-rhs.z);
};

inline float diff_squared_horiz(vec3_t & lhs, vec3_t & rhs)
{
  return (lhs.x-rhs.x)*(lhs.x-rhs.x) + (lhs.y-rhs.y)*(lhs.y-rhs.y);
};

inline Uint16 vals_to_turn(float xval, float yval)
{
  return int(atan_tab.lookup(xval,yval)*INV_TWO_PI*((1<<16)-1));
};

inline Uint16 vec_to_turn(vec3_t & vec)
{
  return int(atan_tab.lookup(vec.x, vec.y)*INV_TWO_PI*((1<<16)-1));
};

inline Uint16 diff_to_turn(vec3_t & lhs, vec3_t & rhs)
{
  return int(atan_tab.lookup(lhs.x-rhs.x, lhs.y-rhs.y)*INV_TWO_PI*((1<<16)-1));
};

inline vec3_t turn_to_vec(Uint16 turn)
{
  return vec3_t( cos_tab[ turn>>2 ] , sin_tab[ turn>>2 ], 0);
};

inline vec3_t matrix_mult(GLMatrix & mat, vec3_t & vec)
{
  vec3_t tmp;

  tmp.x = (vec.x*mat.CNV(0,0) + vec.y*mat.CNV(1,0) + vec.z*mat.CNV(2,0));
  tmp.y = (vec.x*mat.CNV(0,1) + vec.y*mat.CNV(1,1) + vec.z*mat.CNV(2,1));
  tmp.z = (vec.x*mat.CNV(0,2) + vec.y*mat.CNV(1,2) + vec.z*mat.CNV(2,2));

  tmp.x += mat.CNV(3,0);
  tmp.y += mat.CNV(3,1);
  tmp.z += mat.CNV(3,2);

  return tmp;
}


inline vec3_t matrix_mult_simple(GLMatrix & mat, vec3_t & vec)
{
  vec3_t tmp;

  tmp.x = (vec.x*mat.CNV(0,0) + vec.y*mat.CNV(1,0) + vec.z*mat.CNV(2,0));
  tmp.y = (vec.x*mat.CNV(0,1) + vec.y*mat.CNV(1,1) + vec.z*mat.CNV(2,1));
  tmp.z = (vec.x*mat.CNV(0,2) + vec.y*mat.CNV(1,2) + vec.z*mat.CNV(2,2));

  return tmp;
}


inline bool normalize_iterative(vec3_t & vec, float tolerance, int max_iterations)
{
  float len2 = dist_squared(vec);

  if(len2==0.0f) return false;

  float norm_factor = 1.0;
  if(len2<20.0f)
  {
    float tmp_factor  = 1.0;
    for(int cnt=0; cnt<max_iterations && ABS(1.0f-len2)>tolerance; cnt++)
    {
      tmp_factor  = 2.0f/(1.0f + len2);
      norm_factor *= tmp_factor;
      len2        *= tmp_factor * tmp_factor;
    };
  }
  else
  {
    norm_factor = 1.0f / sqrtf(len2);
  }

  vec *= norm_factor;
  return true;
};

//----------------------------------------------------
//----------------------------------------------------
inline quaternion_t operator - (quaternion_t & rhs)
{
  return quaternion_t(-rhs.x, -rhs.y, -rhs.z, -rhs.w);
}

inline quaternion_t operator * (quaternion_t & lhs, float rhs)
{
  return quaternion_t(lhs.x*rhs, lhs.y*rhs, lhs.z*rhs, lhs.w*rhs );
};

inline quaternion_t operator * (float lhs, quaternion_t & rhs)
{
  return quaternion_t(lhs*rhs.x, lhs*rhs.y, lhs*rhs.z, lhs*rhs.w);
};

inline quaternion_t operator / (quaternion_t & lhs, float rhs)
{
  return quaternion_t(lhs.x/rhs, lhs.y/rhs, lhs.z/rhs, lhs.w/rhs);
};

inline quaternion_t operator - (quaternion_t & lhs, quaternion_t & rhs)
{
  return quaternion_t(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z, lhs.w-rhs.w);
};

inline quaternion_t operator + (quaternion_t & lhs, quaternion_t & rhs)
{
  return quaternion_t(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z, lhs.w+rhs.w);
};

inline quaternion_t operator * (quaternion_t & lhs, quaternion_t & rhs)
{
  // quaternion multiplication
  // basis is Q = w + x*i + y*j + z*k
  // Qlhs * Qrhs = (lhs.w + lhs.x*i + lhs.y*j + lhs.z*k) * (rhs.w + rhs.x*i + rhs.y*j + rhs.z*k)
  return quaternion_t(
    (lhs.w*rhs.x + lhs.x*rhs.w) + (lhs.y*rhs.z - lhs.z*rhs.y), 
    (lhs.w*rhs.y + lhs.y*rhs.w) + (lhs.z*rhs.x - lhs.x*rhs.z), 
    (lhs.w*rhs.z + lhs.z*rhs.w) + (lhs.x*rhs.y - lhs.y*rhs.x), 
    lhs.w*rhs.w - lhs.x*rhs.x - lhs.y*rhs.y - lhs.z*rhs.z);
};

inline quaternion_t operator / (quaternion_t & lhs, quaternion_t & rhs)
{
  // quaternion division
  // basis is Q = w + x*i + y*j + z*k
  // conjugate(Q) = w - x*i - y*w - z*k
  // Qlhs / Qrhs = (Qlhs * conjugate(Qrhs)) / |Qrhs|^2 = 
  return quaternion_t(
    (-lhs.w*rhs.x + lhs.x*rhs.w) - (lhs.y*rhs.z - lhs.z*rhs.y), 
    (-lhs.w*rhs.y + lhs.y*rhs.w) - (lhs.z*rhs.x - lhs.x*rhs.z), 
    (-lhs.w*rhs.z + lhs.z*rhs.w) - (lhs.x*rhs.y - lhs.y*rhs.x), 
    lhs.w*rhs.w + lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z) / 
    (rhs.w*rhs.w + rhs.x*rhs.x + rhs.y*rhs.y + rhs.z*rhs.z);
};

inline quaternion_t & operator += (quaternion_t & lhs, quaternion_t & rhs)
{
  lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; lhs.w += rhs.w;
  return lhs;
};

inline quaternion_t & operator -= (quaternion_t & lhs, quaternion_t & rhs)
{
  lhs.x -= rhs.x; lhs.y -= rhs.y; lhs.z -= rhs.z; lhs.w -= rhs.w;
  return lhs;
};

inline quaternion_t & operator *= (quaternion_t & lhs, float rhs)
{
  lhs.x *= rhs; lhs.y *= rhs; lhs.z *= rhs; lhs.w *= rhs;
  return lhs;
};

inline quaternion_t & operator /= (quaternion_t & lhs, float rhs)
{
  lhs.x /= rhs; lhs.y /= rhs; lhs.z /= rhs; lhs.w /= rhs; 
  return lhs;
};

inline quaternion_t & operator *= (quaternion_t & lhs, quaternion_t & rhs)
{
  lhs = lhs * rhs;
  return lhs;
};

inline quaternion_t & operator /= (quaternion_t & lhs, quaternion_t & rhs)
{
  lhs = lhs / rhs;
  return lhs;
};

inline GLMatrix create_matrix(quaternion_t & rhs)
{
  GLMatrix mat;

  // generate the basic matrix
  mat.CNV(0,0) = 1 - 2*rhs.y*rhs.y - 2*rhs.z*rhs.z; 
  mat.CNV(0,1) = 2*rhs.x*rhs.y + 2*rhs.z*rhs.w;
  mat.CNV(0,2) = 2*rhs.x*rhs.z - 2*rhs.y*rhs.w;
  mat.CNV(0,3) = 0;

  mat.CNV(1,0) = 2*rhs.x*rhs.y - 2*rhs.z*rhs.w;
  mat.CNV(1,1) = 1 - 2*rhs.x*rhs.x - 2*rhs.z*rhs.z;
  mat.CNV(1,2) = 2*rhs.y*rhs.z + 2*rhs.x*rhs.w;
  mat.CNV(1,3) = 0;

  mat.CNV(2,0) = 2*rhs.x*rhs.z + 2*rhs.y*rhs.w;
  mat.CNV(2,1) = 2*rhs.y*rhs.z - 2*rhs.x*rhs.w;
  mat.CNV(2,2) = 1 - 2*rhs.x*rhs.x - 2*rhs.y*rhs.y;
  mat.CNV(2,3) = 0;

  mat.CNV(3,0) =  0;
  mat.CNV(3,1) =  0;
  mat.CNV(3,2) =  0;
  mat.CNV(3,3) =  1;
};

inline GLMatrix create_rotate_around_normalized(vec3_t & axis, float angle_rad)
{
  GLMatrix mat;
  float sinval = sinf(angle_rad/2.0f);
  float cosval = cosf(angle_rad/2.0f);
  quaternion_t rhs(axis.x*cosval,axis.y*cosval,axis.z*cosval,sinval);

  // generate the basic matrix
  mat.CNV(0,0) = 1 - 2*rhs.y*rhs.y - 2*rhs.z*rhs.z; 
  mat.CNV(0,1) = 2*rhs.x*rhs.y + 2*rhs.z*rhs.w;
  mat.CNV(0,2) = 2*rhs.x*rhs.z - 2*rhs.y*rhs.w;
  mat.CNV(0,3) = 0;

  mat.CNV(1,0) = 2*rhs.x*rhs.y - 2*rhs.z*rhs.w;
  mat.CNV(1,1) = 1 - 2*rhs.x*rhs.x - 2*rhs.z*rhs.z;
  mat.CNV(1,2) = 2*rhs.y*rhs.z + 2*rhs.x*rhs.w;
  mat.CNV(1,3) = 0;

  mat.CNV(2,0) = 2*rhs.x*rhs.z + 2*rhs.y*rhs.w;
  mat.CNV(2,1) = 2*rhs.y*rhs.z - 2*rhs.x*rhs.w;
  mat.CNV(2,2) = 1 - 2*rhs.x*rhs.x - 2*rhs.y*rhs.y;
  mat.CNV(2,3) = 0;

  mat.CNV(3,0) =  0;
  mat.CNV(3,1) =  0;
  mat.CNV(3,2) =  0;
  mat.CNV(3,3) =  1;
};