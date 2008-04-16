/* Tactics -JF_MPD_Mesh.h
 * Egoboo MPD mesh loader.
 */

#pragma once

#include "RefCount.h"

#include <SDL_types.h>
#include <stdlib.h>

#include <map>
using std::map;

#include <string>
using std::string;

namespace JF
{
  enum MPD_Constant
  {
    MPD_FileID		        = 0x4470614d,	// The string... MapD
    MPD_NumTextures	      = 4,
    MPD_MaxCommands	      = 4,
    MPD_MaxCommandEntries = 0x20,
    MPD_MaxVertices	      = 16,
    MPD_MaxFanTypes	      = 0x40,
    MPD_MaxTextureTiles	  = 0x0100,
    MPD_MaxFans		        = (0x0200*0x0200),
    MPD_MaxSizeY	        = 0x0400,
    MPD_FanOff		        = 0xffff,	 // an empty fan
    MPD_IndexInvalid      = (Uint32)(-1),
    MPD_bits              = 7,
    MPD_Resolution        = (1<<MPD_bits)
  };

  #pragma pack(push,1)
  struct MPD_Vertex
  {
    float  x, y, z;		// position data
    Uint32 color;	// packed rgb color
    float  s, t;			// texture coordinates
    Uint8  ambient, light;	// Lighting data
    Uint8  reserved[6];		// Pack structure to 0x20 bytes
  };
  #pragma pack(pop)

  struct MPD_FanType
  {
    int    numCommands;
    Uint32 numCommandEntries[MPD_MaxCommands];
    Uint32 numVertices;
    Uint16 vertexIndices[MPD_MaxCommandEntries];
    float  texCoords[MPD_MaxVertices][2];
  };

  struct MPD_Fan
  {
    Uint8  type;		// Index into FanType array
    Uint8  flags;	// Fan property flags
    Uint32 textureTile;		// Tile texture the fan uses
    Uint8  twist;	// ?
    Uint32 firstVertex;	// This fans first vertex in the mesh's vertex array
  };


  class MPD_Mesh : public RefCount<MPD_Mesh>
  {
    friend class MPD_Manager;

    public:
      virtual ~MPD_Mesh();

      static bool loadFanTypes(const char *fileName, MPD_FanType *fanTypes = NULL);

      // prepareVertices calculates the "firstVertex" member of each vertex.  It needs
      // to be called before you can draw the mesh, or call getHeight
      void prepareVertices(const MPD_FanType *fanTypes = NULL);

      // Return the mesh's height at the given point.
      float getHeight(float x, float y) const;

      // Retrieve info about the mesh
      bool   prepared()    const { return m_prepared; }
      Uint32 numVertices() const { return m_numVertices; }
      Uint32 fansWide()    const { return m_fansWide; }
      Uint32 fansHigh()    const { return m_fansHigh; }
      float  width()       const { return m_width; }
      float  height()      const { return m_height; }
      Uint32 numFans()     const { return (m_fansWide * m_fansHigh); }

      const MPD_Vertex*  getVertices()            const { return m_vertices; }

      const MPD_Fan* getFan(Uint32 index) const;
      const MPD_Fan* getFanPos(float x, float y) const;
      const MPD_Fan* getFanTile(int ix, int iy) const;

      const MPD_FanType* getFanType(Uint32 index, MPD_FanType *ftypes = NULL) const;
      const MPD_FanType* getFanTypePos(float x, float y, MPD_FanType *ftypes = NULL) const;
      const MPD_FanType* getFanTypeTile(int ix, int iy, MPD_FanType *ftypes = NULL) const;

      bool has_flags(Uint32 fan, Uint32 fx);
      bool has_flags(float x, float y,Uint32 fx);

      void add_flags(Uint32 fan, Uint32 fx);
      void add_flags(float x, float y, Uint32 fx);

      void remove_flags(Uint32 fan, Uint32 fx);
      void remove_flags(float x, float y, Uint32 fx);

      Uint32 getIndexPos(float x, float y) const;
      Uint32 getIndexTile(int x, int y)    const;

      static const Uint32 INVALID_INDEX = (Uint32)(-1);

    protected:

      bool check_fan(Uint32 fan) const;
      bool check_bound_tile(int ix, int iy) const;
      bool check_bound_pos(float x, float y) const;

      Uint32 get_index_pos(float x, float y) const;  // not error trapped, so be careful
      Uint32 get_index_tile(int x, int y)    const;  // not error trapped, so be careful

      Uint32 m_numVertices;
      Uint32 m_fansWide;
      Uint32 m_fansHigh;
      Uint32 m_waterShift;
      float  m_width, m_height;

      MPD_Vertex *m_vertices;
      MPD_Fan    *m_fans;

      static MPD_FanType m_fan_types[MPD_MaxFanTypes];

      static MPD_Mesh *load(const char *fileName, MPD_Mesh *msh = NULL);

      MPD_Mesh();

    private:

      bool m_prepared;
  };

  class MPD_Manager
  {
    public:
      static MPD_Mesh *loadFromFile(const char *fileName, MPD_Mesh *msh = NULL);

    private:
      typedef map<string, MPD_Mesh*> MeshMap;

      static MeshMap meshCache;
  };

};
