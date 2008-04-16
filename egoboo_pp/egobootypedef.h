// Egoboo
// egobootypedef.h
// Defines some basic types that are used throughout the game code
//
// Re-written to clarify it, and make multiple targets easier to maintain

#ifndef Egoboo_egobootypedef_h
#define Egoboo_egobootypedef_h

#include <assert.h>
#include <cmath>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN       // Speeds up compile times a bit.  We don't need everything in windows.h
#pragma warning(disable : 4305)                     // Turn off silly warnings
#pragma warning(disable : 4244)                     //
#pragma warning(disable : 4554)
#pragma warning(disable : 4761)
#pragma warning(disable : 4996)      //'sprintf' was declared deprecated
#endif

#include <stdexcept>

#include <string>
using std::string;

#include <algorithm>
using std::min;
using std::max;

#include <fstream>
using std::ifstream;

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#include <vector>
using std::vector;

#include <map>
using std::map;




#include <SDL.h>
#include <SDL_endian.h>
#include <SDL_mixer.h>

#include <map>
#include <deque>

#ifdef _WIN32
#define snprintf _snprintf
#endif


typedef struct rect_t
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
}rect_t;

struct IDSZ
{
  const static IDSZ NONE;
  static char strval[5];
  Uint32 value;

  IDSZ()         { value = NONE.value; }
  IDSZ(Uint32 v) { value = v;  };

  IDSZ(const char *str)
  {
    if (NULL==str)
      value = NONE.value;
    else
    {
      value = 0;
      value |= ((str[0]-'A')<<15);
      value |= ((str[1]-'A')<<10);
      value |= ((str[2]-'A')<< 5);
      value |= ((str[3]-'A')<< 0);
    }
  };

  operator Uint32() { return value; }
  operator char*() { convert(*this); return (char *)(&strval); };

  static char * convert(IDSZ & id)
  {
    if (id == IDSZ::NONE)
    {
      sprintf(strval, "NONE");
    }
    else
    {
      strval[0] = ((id.value>>15)&0x1F) + 'A';
      strval[1] = ((id.value>>10)&0x1F) + 'A';
      strval[2] = ((id.value>> 5)&0x1F) + 'A';
      strval[3] = ((id.value>> 0)&0x1F) + 'A';
      strval[4] = 0;
    }
    return (char*)(&strval);
  }

  bool operator == (IDSZ rhs) { return value == rhs.value; }
  bool operator != (IDSZ rhs) { return value != rhs.value; }
  bool operator >= (IDSZ rhs) { return value >= rhs.value; }
  bool operator <= (IDSZ rhs) { return value <= rhs.value; }
  bool operator > (IDSZ rhs) { return value >  rhs.value; }
  bool operator < (IDSZ rhs) { return value <  rhs.value; }
};

struct REGION
{
  float left, top, width, height;
  REGION(float l=0, float t=0, float w=0, float h=0)
  {
    left   = l;
    top    = t;
    width  = w;
    height = h;
  }
};


template <class _c>
struct GID
{
  typedef size_t ID;

  const static ID BAD_ID = (ID)(-1);

  static const ID new_ID() { return _gid++; }

private:
  static ID _gid;
};

template <class _c>
typename GID<_c>::ID GID<_c>::_gid = 0;

struct Latch
{
  float           x;          // Character latches
  float           y;          //
  Uint32          button;     // Button latches

  void clear() { x = 0.0; y = 0.0; button = 0; }

  Latch diff(Latch & rhs, int bit_resolution)
  {
    const int resolution = 1<<bit_resolution;

    Latch tmp;

    tmp.x      = int((x - rhs.x)*resolution) / float(resolution);
    tmp.y      = int((y - rhs.y)*resolution) / float(resolution);
    tmp.button = button ^ rhs.button;

    return tmp;
  };


  bool is_null(int bit_resolution = 0)
  {
    const float resolution = 1.0f / (1<<bit_resolution);
    bool retval = false;

    if(0==bit_resolution)
      retval = (button==0) &&  (x == 0.0f) &&  (y == 0.0f);
    else
     retval = (button==0) && (fabs(x)< resolution) && (fabs(y)< resolution);

    return retval;
  }
};

template <typename _ty, size_t _sz>
struct TList
{
  typedef TList<_ty,_sz> my_list_type;

  const static size_t INVALID = _sz;
  const static size_t SIZE    = _sz;


  _ty & operator [] (int i) { return _list[i]; }

protected:
  _ty    _list[_sz];
  size_t _index;
};

template <typename _ty, size_t _sz> struct TAllocList;

template <typename _ty, size_t _sz>
struct TAllocClient
{
  friend struct TAllocList<_ty, _sz>;
  typedef TAllocClient<_ty, _sz> my_aclient_type;

  TAllocClient() { _allocated = false; }

  my_aclient_type & getAClient() { return *this; }

  bool allocated() { return _allocated; }

private:
  bool _allocated;
};

template <typename _ty, size_t _sz>
struct TAllocList : public TList<_ty,_sz>
{
  typedef TAllocList<_ty,_sz> my_alist_type;

  TAllocList() { _setup(); }

  Uint32 get_free(Uint32 forceval = _sz)
  {
    if(_index == 0) return _sz;

    Uint32 retval = _sz;
    if(forceval<_sz)
    {
      Uint32 i;
      for(i=0; i<_index; i++)
      {
        if(forceval == _free[i]) break;
      }

      if(i<_index)
      {
        Uint32 tmpval   = _free[_index-1];
        _free[_index-1] = _free[i];
        _free[i]        = tmpval;
      }
    }

    _index--;
    retval = _free[_index];
    _list[retval]._allocated = true;

    return retval;
  }

  void return_one(Uint32 i)
  {
    if(_list[i].allocated())
    {
      _list[i]._allocated = false;
      _free[_index] = i;
      _index++;
      assert(_index <= _sz);
    }
  }

  Uint32 count_free() { return _index; }
  Uint32 count_used() { return SIZE-_index; }

protected:

  Uint32 _swap(Uint32 elem1, Uint32 elem2)
  {
    for (Uint32 tnc = 0; tnc<_index; tnc++)
    {
      if (_free[tnc] == elem2)
      {
        _free[tnc] = elem1;
        break;
      }
    }
    return elem2;
  };

  void _setup()
  {
    for(_index=0; _index<_sz; _index++)
    {
      _free[_index]            = _index;
      _list[_index]._allocated = false;
    }

    _index = _sz-1;
  };

private:
  Uint32 _free[_sz];
};


//=======================================================================

template <typename _ty, size_t _sz>
struct TListStrict
{
  typedef TList<_ty,_sz> my_list_type;

  const static Uint32 INVALID = _sz;
  const static size_t SIZE    = _sz;

  typedef struct index_t
  {
    explicit index_t(size_t i = _sz) { index = i; }
    Uint32 index;
    index_t operator = (Uint32 rhs)
      { index = rhs; index_t tmp; tmp.index = rhs; return tmp; };
  } index_t;

  _ty & operator [] (index_t i) { return _list[i.index]; }

protected:
  _ty    _list[_sz];
  size_t _index;
};

template <typename _ty, size_t _sz> struct TAllocListStrict;

template <typename _ty, size_t _sz>
struct TAllocClientStrict
{
  friend struct TAllocListStrict<_ty, _sz>;
  typedef TAllocListStrict<_ty, _sz> my_aclient_type;

  my_aclient_type & getAClient() { return *this; }

  TAllocClientStrict() { _allocated = false; }

  bool allocated() { return _allocated; }

private:
  bool _allocated;
};

template <typename _ty, size_t _sz>
struct TAllocListStrict : public TListStrict<_ty,_sz>
{
  typedef TAllocListStrict<_ty,_sz> my_alist_type;

  TAllocListStrict() { _setup(); }

  Uint32 get_free(Uint32 forceval = _sz)
  {
    if(_index == 0) return _sz;

    Uint32 retval = _sz;
    if(forceval<_sz)
    {
      Uint32 i;
      for(i=0; i<_index; i++)
      {
        if(forceval == _free[i]) break;
      }

      if(i<_index)
      {
        Uint32 tmpval   = _free[_index-1];
        _free[_index-1] = _free[i];
        _free[i]        = tmpval;
      }
    }

    _index--;
    retval = _free[_index];
    _list[retval]._allocated = true;

    return retval;
  }

  void return_one(Uint32 i)
  {
    if(_list[i].allocated())
    {
      _list[i]._allocated = false;
      _free[_index] = i;
      _index++;
      assert(_index <= _sz);
    }
  }

  Uint32 count_free() { return _index; }

protected:

  Uint32 _swap(Uint32 elem1, Uint32 elem2)
  {
    for (Uint32 tnc = 0; tnc<_index; tnc++)
    {
      if (_free[tnc] == elem2)
      {
        _free[tnc] = elem1;
        break;
      }
    }
    return elem2;
  };

  void _setup()
  {
    for(_index=0; _index<_sz; _index++)
    {
      _free[_index]            = _index;
      _list[_index]._allocated = false;
    }

    _index = _sz;
  };

private:
  Uint32 _free[_sz];
};

//=======================================================================

class Ego_Exception : public std::runtime_error
{
public:

  // Constructors
  Ego_Exception (const string &error)
    : std::runtime_error (error) {};

  Ego_Exception (const std::string &error, const std::string &name)
    : std::runtime_error (error), _which (name) {};

  virtual ~Ego_Exception () throw () {};

public:

  // Public interface
  virtual const char *which () const throw ()
    { return _which.c_str(); }

private:
  // Member variables
  string _which;
};

#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
#define SDL_SwapLEFloat(X) X
#else
inline float SDL_SwapLEFloat(float X)
{
  union { float f; Uint32 l; } data;
  data.f = X;
  data.l = SDL_Swap32(data.l);
  return data.f;
};
#endif

#ifndef MAX
#define MAX(a,b) ( (a)>(b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ( (a)>(b) ? (b) : (a))
#endif

#endif // include guard

