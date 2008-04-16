#pragma once

#include "MPD_file.h"
#include "egobootypedef.h"
#include "mathstuff.h"
#include "Texture.h"

struct vec3_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXMESHFAN                      (0x0200*0x0200)   // Terrain mesh size
#define MAXMESHSIZEY                    0x0400        // Max fans in y direction
#define BYTESFOREACHVERTEX              14          // 14 bytes each
#define MAXMESHVERTICES                 16          // Fansquare vertices
#define MAXMESHTYPE                     0x40          // Number of fansquare command types
#define MAXMESHCOMMAND                  4           // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           0x20          // Fansquare command list size
#define MAXMESHCOMMANDSIZE              0x20          // Max trigs in each command
#define MAXTILETYPE                     0x0100         // Max number of tile images
#define MAXMESHRENDER                   0x0400        // Max number of tiles to draw

enum MESHFX_BITS
{
  MESHFX_REF                       =      0,           // 0 This tile is drawn 1st
  MESHFX_SHA                       = 1 << 0,           // 0 This tile is drawn 2nd
  MESHFX_DRAWREF                   = 1 << 1,           // 1 Draw reflection of characters
  MESHFX_ANIM                      = 1 << 2,           // 2 Animated tile ( 4 frame )
  MESHFX_WATER                     = 1 << 3,           // 3 Render water above surface ( Water details are set per module )
  MESHFX_WALL                      = 1 << 4,           // 4 Wall ( Passable by ghosts, particles )
  MESHFX_IMPASS                    = 1 << 5,           // 5 Impassable
  MESHFX_DAMAGE                    = 1 << 6,           // 6 Damage
  MESHFX_SLIPPY                    = 1 << 7            // 7 Ice or normal
};

//--------------------------------------------------------------------------------------------
struct renderlist_element
{
  size_t    all;                   // Which to render, total
  size_t    ref;                   // ..., reflective
  size_t    sha;                   // ..., shadow
  size_t    drf;
  size_t    ani;
  size_t    wat;
  size_t    wal;
  size_t    imp;
  size_t    dam;
  size_t    slp;
};

//--------------------------------------------------------------------------------------------

struct Mesh;

struct Render_List : public TList<renderlist_element, 16*MAXMESHRENDER>
{
  size_t   all_count;             // Number to render, total
  size_t   ref_count;             // ..., reflective
  size_t   sha_count;             // ..., shadow
  size_t   drf_count;
  size_t   ani_count;
  size_t   wat_count;
  size_t   wal_count;
  size_t   imp_count;
  size_t   dam_count;
  size_t   slp_count;

  Render_List()
  {
    all_count = ref_count = sha_count = drf_count =
    ani_count = wat_count = wal_count = imp_count =
    dam_count = slp_count = 0;
  };

  void make(Mesh & mlist);
  void reset(Mesh & mlist);

  void make(JF::MPD_Mesh * pmesh);
  void reset(JF::MPD_Mesh * pmesh);
};


extern Render_List GRenderlist;

//--------------------------------------------------------------------------------------------
//struct Fan_Info
//{
//  Uint8   type;                               // Command type
//  Uint8   fx;                                 // Special effects flags
//  Uint8   twist;                              //
//  Uint8   inrenderlist;                       //
//  Uint16  tile;                               // Get texture from this
//  Uint32  vrtstart;                           // Which vertex to start at
//};

struct Bump_Info
{
  Uint16  chr;                     // For character collisions
  Uint16  chr_count;               // Number on the block
  Uint16  prt;                     // For particle collisions
  Uint16  prt_count;               // Number on the block
};

struct Bump_List : TList<Bump_Info, MAXMESHFAN/16>
{
  Uint32 count;

  Bump_List() { count = 0; }

  void clear();
};


extern Bump_List BumpList;

struct Bump_List_Client
{
  bool   in_bumplist;
  Uint32 bump_next;                     // Next item on fanblock
};

//--------------------------------------------------------------------------------------------
struct AA_BBOX
{
  vec3_t minvals, maxvals;
  bool   initialized;

  AA_BBOX() { initialized=false; };

  AA_BBOX(vec3_t & corner1,  vec3_t &corner2)
  {
    set(corner1, corner2);
  }

  AA_BBOX(const AA_BBOX & rhs) { set(rhs); }

  void set(vec3_t & corner1,  vec3_t &corner2)
  {
    minvals.x = MIN(corner1.x, corner2.x);
    minvals.y = MIN(corner1.y, corner2.y);
    minvals.z = MIN(corner1.z, corner2.z);

    maxvals.x = MAX(corner1.x, corner2.x);
    maxvals.y = MAX(corner1.y, corner2.y);
    maxvals.z = MAX(corner1.z, corner2.z);

    initialized = true;
  }

  void set(const AA_BBOX & rhs)
  {
    *this = rhs;
    initialized = true;
  };


  bool encloses(vec3_t & pos)
  {
    if(pos.x<minvals.x) return false;
    if(pos.x>maxvals.x) return false;

    if(pos.y<minvals.y) return false;
    if(pos.y>maxvals.y) return false;

    if(pos.z<minvals.z) return false;
    if(pos.z>maxvals.z) return false;

    return true;
  }

  void do_merge(vec3_t & pos)
  {
    if(!initialized)
    {
      minvals = pos;
      maxvals = pos;
      initialized = true;
    }
    else
    {
      minvals.x = MIN(minvals.x, pos.x);
      minvals.y = MIN(minvals.y, pos.y);
      minvals.z = MIN(minvals.z, pos.z);

      maxvals.x = MAX(maxvals.x, pos.x);
      maxvals.y = MAX(maxvals.y, pos.y);
      maxvals.z = MAX(maxvals.z, pos.z);
    }
  };

  void do_merge(AA_BBOX & rhs)
  {
    if(!initialized)
    {
      set(rhs);
    }
    else
    {
      minvals.x = MIN(minvals.x, rhs.minvals.x);
      minvals.y = MIN(minvals.y, rhs.minvals.y);
      minvals.z = MIN(minvals.z, rhs.minvals.z);

      maxvals.x = MAX(maxvals.x, rhs.maxvals.x);
      maxvals.y = MAX(maxvals.y, rhs.maxvals.y);
      maxvals.z = MAX(maxvals.z, rhs.maxvals.z);
    }
  };

};


//--------------------------------------------------------------------------------------------
template <typename _ty>
struct Dynamic_Array
{
  Dynamic_Array()  { _list = NULL; _allocated = 0; }

  ~Dynamic_Array() { Deallocate(); };

  void Allocate(size_t sz)
  {
    Deallocate();
    _list = new _ty[sz];
    if(NULL!=_list) _allocated = sz;
  }

  void Deallocate()
  {
    if(NULL!=_list)
    {
      delete [] _list;
      _list = NULL;
    }
    _allocated = 0;
  }

  _ty & operator [] (size_t index) { if(index>_allocated) assert(false); return _list[index]; };

protected:
  size_t    _allocated;
  _ty * _list;
};

struct Mesh_Info_Item
{
  AA_BBOX bbox;
  bool    in_view;

  Mesh_Info_Item() { in_view = false; };
};

struct Mesh_Extra_Info : public Dynamic_Array<Mesh_Info_Item>
{
};

//--------------------------------------------------------------------------------------------
struct Mesh : public JF::MPD_Mesh
{
  bool     exploremode;                                   // Explore mode?
  TEX_REF  txref_last;                                    // Last texture used

  static const Uint32 Block_bits       = JF::MPD_bits + 2;
  static const Uint32 Block_resolution = (1<<Block_bits);

  Mesh()
  {
    exploremode = false;
    _cached_normals        = NULL;
    _cached_normals_smooth = NULL;
    m_blocksWide = m_blocksHigh = 0;
  }

  ~Mesh()
  {
    if(NULL!=_cached_normals) 
      { delete [] _cached_normals; _cached_normals = NULL; }

    if(NULL!=_cached_normals_smooth) 
      { delete [] _cached_normals_smooth; _cached_normals_smooth = NULL; }
  }

  void getRegionFromFan(Uint32 fan, REGION & r) const;
  void getRegionFromFanblock(Uint32 block, REGION & r) const;

  Uint32 getFanblockPos(vec3_t & pos) const;
  Uint32 getFanblockBlock(int ix, int iy) const;

  bool load(char *modname);

  bool simple_normal(vec3_t & pos, vec3_t & normal) const;
  bool simple_normal(Uint32 fan, vec3_t & normal) const;
  bool smoothed_normal(vec3_t & pos, vec3_t & normal) const;
  bool smoothed_normal(Uint32 fan, vec3_t & normal) const;

  Mesh_Extra_Info m_fan_ex;
  Mesh_Extra_Info m_tile_ex;
  Mesh_Extra_Info m_block_ex;

protected:
  bool check_fanblock_block(int ix, int iy) const;

  Uint32 get_fanblock_pos(vec3_t & pos) const;
  Uint32 get_fanblock_block(int x, int y)   const;

private:
  void do_bboxes();
  void do_normals();
  void do_normals_smooth();

  bool do_simple_normal(Uint32 fan, vec3_t & normal) const;

  vec3_t * _cached_normals;
  vec3_t * _cached_normals_smooth;

  Uint32 m_blocksWide, m_blocksHigh;

};

extern Mesh GMesh;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//MESH
int load_mesh(Mesh & msh, char *modname);
//bool get_mesh_memory();
int  mesh_get_level(vec3_t & pos, Uint8 waterwalk);

//void load_mesh_fans();
void make_fanstart();
//void make_twist();

bool mesh_is_over_water(vec3_t & pos);


