/* Tactics - MPD_Mesh.cpp
*/

#include "proto.h"
#include "MPD_file.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <SDL_endian.h>

namespace JF
{
  // Private structures & constants to help in loading files
  #pragma pack(push,1)
  struct MPD_Header
  {
    Uint32 magicNumber;
    Uint32 numVerts;
    Uint32 x_size;
    Uint32 y_size;
  };
  #pragma pack(pop)
};

using namespace JF;

MPD_FanType MPD_Mesh::m_fan_types[MPD_MaxFanTypes];
MPD_Manager::MeshMap MPD_Manager::meshCache;

MPD_Mesh::MPD_Mesh()
{	
  m_numVertices = 0;
  m_fansWide = 0;
  m_fansHigh = 0;
  m_waterShift = 0;
  m_width = 0;
  m_height = 0;

  m_vertices = NULL;
  m_fans = NULL;

  m_prepared = false;
}

MPD_Mesh::~MPD_Mesh()
{
  if (m_vertices != NULL)
  {
    delete[] m_vertices;
  }

  if (m_fans != NULL)
  {
    delete[] m_fans;
  }
}

float MPD_Mesh::getHeight(float x, float y) const
{
  if (m_prepared == false) return 0.0f;

  float z0, z1, z2, z3;         // Height of each fan corner
  float zleft, zright, zdone;    // Weighted height of each side
  int   ix, iy;
  float dx, dy;
  int first_vert;

  if(!check_bound_pos(x,y)) return 0.0f;

  ix = int(x) >> JF::MPD_bits;
  iy = int(y) >> JF::MPD_bits;
  int fan = iy * m_fansWide + ix; assert(check_fan(fan));
  first_vert = m_fans[fan].firstVertex;

  dx = (x - (ix<<MPD_bits)) / float(MPD_Resolution);
  dy = (y - (iy<<MPD_bits)) / float(MPD_Resolution);

  z0 = m_vertices[first_vert + 0].z;
  z1 = m_vertices[first_vert + 1].z;
  z2 = m_vertices[first_vert + 2].z;
  z3 = m_vertices[first_vert + 3].z;

  zleft  = (z0 * (1.0f - dy) + z3 * dy);
  zright = (z1 * (1.0f - dy) + z2 * dy);
  zdone  = (zleft * (1.0f - dx) + zright * dx);
  return zdone;
}

bool MPD_Mesh::check_fan(Uint32 index) const
{
  return index >= 0 && index < (m_fansWide*m_fansHigh);
}

bool MPD_Mesh::check_bound_tile(int ix, int iy) const
{
  return ix>=0 && iy>=0 && ix<m_fansWide && iy<m_fansHigh;
}

bool MPD_Mesh::check_bound_pos(float x, float y) const
{
  return x>0 && y>0 && x<m_width && y<m_height;
}

Uint32 MPD_Mesh::get_index_pos(float x, float y) const
{
  int ix = int(x) >> JF::MPD_bits, iy=int(y) >> JF::MPD_bits;
  return ix + iy*m_fansWide;
}

Uint32 MPD_Mesh::get_index_tile(int ix, int iy) const
{
  return ix + iy*m_fansWide;
}


Uint32 MPD_Mesh::getIndexPos(float x, float y) const
{
  if(!check_bound_pos(x,y)) return -1;

  int ix = int(x) >> JF::MPD_bits, iy=int(y) >> JF::MPD_bits;
  return ix + iy*m_fansWide;
}

Uint32 MPD_Mesh::getIndexTile(int ix, int iy) const
{
  if(!check_bound_tile(ix,iy)) return -1;

  return ix + iy*m_fansWide;
}

const MPD_Fan* MPD_Mesh::getFan(Uint32 index) const
{
  if (!check_fan(index)) return NULL;

  return &m_fans[index];
}

const MPD_Fan* MPD_Mesh::getFanPos(float x, float y) const
{
  if(!check_bound_pos(x,y)) return NULL;

  return &m_fans[get_index_pos(x,y)];
}

const MPD_Fan* MPD_Mesh::getFanTile(int ix, int iy) const
{
  if(!check_bound_tile(ix,iy)) return NULL;

  return &m_fans[get_index_tile(ix,iy)];
}

const MPD_FanType* MPD_Mesh::getFanType(Uint32 index, MPD_FanType *ftypes) const
{
  const MPD_Fan* pfan = getFan(index);
  if(NULL==pfan) return NULL;

  if(NULL==ftypes) ftypes = m_fan_types;
  return &ftypes[pfan->type];
}

const MPD_FanType* MPD_Mesh::getFanTypePos(float x, float y, MPD_FanType *ftypes) const
{
  const MPD_Fan* pfan = getFanPos(x,y);
  if(NULL==pfan) return NULL;

  if(NULL==ftypes) ftypes = m_fan_types;
  return &ftypes[pfan->type];
}

const MPD_FanType* MPD_Mesh::getFanTypeTile(int ix, int iy, MPD_FanType *ftypes) const
{
  const MPD_Fan* pfan = getFanTile(ix,iy);
  if(NULL==pfan) return NULL;

  if(NULL==ftypes) ftypes = m_fan_types;
  return &ftypes[pfan->type];
}

bool MPD_Mesh::has_flags(Uint32 fan, Uint32 fx)
{
  if(!check_fan(fan)) return false;

  return 0 != (m_fans[fan].flags&fx);
}

bool MPD_Mesh::has_flags(float x, float y, Uint32 fx)
{
  if(!check_bound_pos(x,y)) return false;

  return 0 != (m_fans[get_index_pos(x,y)].flags&fx);
}

void MPD_Mesh::add_flags(Uint32 fan, Uint32 fx)
{
  if(check_fan(fan))
    m_fans[fan].flags |= fx;
}

void MPD_Mesh::add_flags(float x, float y,Uint32 fx)
{
  if(check_bound_pos(x,y))
    m_fans[get_index_pos(x,y)].flags |= fx;
}

void MPD_Mesh::remove_flags(Uint32 fan, Uint32 fx)
{
  if(check_fan(fan))
    m_fans[fan].flags &= ~fx;
}

void MPD_Mesh::remove_flags(float x, float y,Uint32 fx)
{
  if(check_bound_pos(x,y))
    m_fans[get_index_pos(x,y)].flags &= ~fx;
}

bool MPD_Mesh::loadFanTypes(const char *fileName, MPD_FanType * fanTypes)
{
  FILE *infile;
  int tmp;
  int bigfanoffset = MPD_MaxFanTypes/2;
  int total_fans;

  if(fileName == NULL) return false;

  if(NULL==fanTypes) fanTypes = m_fan_types;

  infile = fopen(fileName, "rt");
  if(infile == NULL)
  {
    return false;
  }

  // find out how many fans we're going to be reading
  goto_colon_yesno(infile);
  fscanf(infile, "%d", &tmp);
  total_fans = tmp;

  memset(fanTypes, 0, sizeof(MPD_FanType) * MPD_MaxFanTypes);

  // read in the list of fans
  for(tmp = 0;tmp < total_fans;tmp++)
  {
    int num_verts, c, num_commands, entry;

    num_verts = get_next_int(infile);
    fanTypes[tmp].numVertices = num_verts;
    fanTypes[tmp + bigfanoffset].numVertices = num_verts;

    // read in the texture coordinates for this fan
    for(c = 0;c < num_verts;c++)
    {
      int throwaway;
      float tc;

      // the first value, Ref, gets thrown away
      throwaway = get_next_int(infile);

      // read the two texture coordinates
      tc = get_next_float(infile);
      fanTypes[tmp].texCoords[c][0] = tc;
      fanTypes[tmp + bigfanoffset].texCoords[c][0] = tc;

      tc = get_next_float(infile);
      fanTypes[tmp].texCoords[c][1] = tc;
      fanTypes[tmp + bigfanoffset].texCoords[c][1] = tc;
    }

    // read in the drawing commands for this fan
    num_commands = get_next_int(infile);
    fanTypes[tmp].numCommands = num_commands;
    fanTypes[tmp + bigfanoffset].numCommands = num_commands;
    entry = 0;
    for(c = 0;c < num_commands;c++)
    {
      int i, command_size;

      command_size = get_next_int(infile);
      fanTypes[tmp].numCommandEntries[c] = command_size;
      fanTypes[tmp + bigfanoffset].numCommandEntries[c] = command_size;

      for(i = 0;i < command_size;i++)
      {
        int itmp;

        itmp = get_next_int(infile);
        fanTypes[tmp].vertexIndices[entry] = itmp;
        fanTypes[tmp + bigfanoffset].vertexIndices[entry] = itmp;
        entry++;
      } // for i < command_size
    } // for c < num_commands
  } // for tmp < num_fans

  // don't need the file open anymore
  fclose(infile);

  // fix texture coordinates for seamless tiling
  for(tmp = 0;tmp < (MPD_MaxFanTypes / 2);tmp++)
  {
    for(Uint32 i = 0; i < fanTypes[tmp].numVertices;i++)
    {
      fanTypes[tmp].texCoords[i][0] = ((.6f/0x20)+(fanTypes[tmp].texCoords[i][0]*30.8f/0x20))/8;
      fanTypes[tmp].texCoords[i][1] = ((.6f/0x20)+(fanTypes[tmp].texCoords[i][1]*30.8f/0x20))/8;
    }
  }

  // and for the bigger tiles
  for(tmp;tmp < MPD_MaxFanTypes;tmp++)
  {
    for(Uint32 i = 0; i < fanTypes[tmp].numVertices;i++)
    {
      fanTypes[tmp].texCoords[i][0] = ((.6f/0x40)+(fanTypes[tmp].texCoords[i][0]*62.8f/0x40))/4;
      fanTypes[tmp].texCoords[i][1] = ((.6f/0x40)+(fanTypes[tmp].texCoords[i][1]*62.8f/0x40))/4;
    }
  }

  return true;
}

void MPD_Mesh::prepareVertices(const MPD_FanType *fanTypes)
{
  if (fanTypes == NULL) fanTypes = m_fan_types;
  if (fanTypes == NULL) return;

  int vert, fan, numFans;

  vert = 0;
  numFans = m_fansWide * m_fansHigh;

  for (fan = 0; fan < numFans; fan++)
  {
    float offu, offv;
    int   fanType;

    fanType = m_fans[fan].type;
    m_fans[fan].firstVertex = vert;

    offu = (m_fans[fan].textureTile % 8) / 8.0f;
    offv = (m_fans[fan].textureTile / 8) / 8.0f;

    for (Uint32 v = 0; v < fanTypes[fanType].numVertices; v++)
    {
      m_vertices[vert].s = fanTypes[fanType].texCoords[v][0] + offu;
      m_vertices[vert].t = fanTypes[fanType].texCoords[v][1] + offv;
      m_vertices[vert].color = 0;
      vert++;
    }
  }

  m_prepared = true;
}

MPD_Mesh *MPD_Mesh::load(const char *fileName, MPD_Mesh *msh)
{
  FILE *file;
  Uint32 numTiles;
  Uint32 fan, v;     // loop counters
  struct MPD_Header header;

  if (!fileName) return NULL;

  file = fopen(fileName, "rb");
  if (file == NULL) return NULL;

  // make sure that this is a valid Mesh file
  fread(&header, sizeof(struct MPD_Header), 1, file);

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  header.magicNumber = SDL_Swap32(header.magicNumber);
  header.numVerts    = SDL_Swap32(header.numVerts);
  header.x_size      = SDL_Swap32(header.x_size);
  header.y_size      = SDL_Swap32(header.y_size);
#endif

  if(header.magicNumber != MPD_FileID)
  {
    // not a valid .mpd file, bail out
    fclose(file);
    return NULL;
  }

  // good mpd file, so allocate space for it
  MPD_Mesh *mesh = (msh==NULL) ? new MPD_Mesh : msh;

  // gather information about the mesh file
  mesh->m_numVertices = header.numVerts;
  mesh->m_fansWide = header.x_size;
  mesh->m_fansHigh = header.y_size;
  mesh->m_width  = mesh->m_fansWide * float(MPD_Resolution);
  mesh->m_height = mesh->m_fansHigh * float(MPD_Resolution);

  numTiles = header.x_size * header.y_size;
  mesh->m_vertices = new MPD_Vertex[header.numVerts];
  mesh->m_fans = new MPD_Fan[numTiles];

  // clear the vertices to 0, just to be safe
#ifndef NDEBUG
  memset(mesh->m_vertices, 0, mesh->m_numVertices * sizeof(MPD_Vertex));
  memset(mesh->m_fans, 0, numTiles * sizeof(MPD_Fan));
#endif

  // Load fan data
  for(fan = 0;fan < numTiles;fan++)
  {
    Uint32 tmp;
    fread(&tmp, sizeof(Uint32), 1, file);  tmp = SDL_SwapLE32(tmp);

    mesh->m_fans[fan].type = (unsigned char)(tmp >> 24);
    mesh->m_fans[fan].flags = (unsigned char)(tmp >> 16);
    mesh->m_fans[fan].textureTile = (unsigned short)tmp;
  }

  // Load "twist" data
  for(fan = 0;fan < numTiles;fan++)
  {
    char tmp;
    fread(&tmp, sizeof(char), 1, file);
    mesh->m_fans[fan].twist = tmp;
  }

  // Load vertex x data
  for(v = 0; v < mesh->m_numVertices; v++)
  {
    float tmp;
    fread(&tmp, sizeof(float), 1, file); tmp = SDL_SwapLEFloat(tmp);
    mesh->m_vertices[v].x = tmp;
  }

  // Load vertex y data
  for(v = 0; v < mesh->m_numVertices; v++)
  {
    float tmp;
    fread(&tmp, sizeof(float), 1, file); tmp = SDL_SwapLEFloat(tmp);
    mesh->m_vertices[v].y = tmp;
  }

  // Load vertex z data
  for(v = 0;v < mesh->m_numVertices;v++)
  {
    float tmp;
    fread(&tmp, sizeof(float), 1, file); tmp = SDL_SwapLEFloat(tmp);
    mesh->m_vertices[v].z = tmp / float(1<<4); // Cartman uses 4 bit fixed point for Z
  }

  // Load lighting data
  for(v = 0; v < mesh->m_numVertices; v++)
  {
    char tmp;
    fread(&tmp, sizeof(char), 1, file);
    mesh->m_vertices[v].ambient = tmp;
    mesh->m_vertices[v].light = 0;
  }

  // done with the file now
  fclose(file);
  return mesh;
}


MPD_Mesh *MPD_Manager::loadFromFile(const char *fileName, MPD_Mesh *msh)
{
  // ignore garbage input
  if (!fileName || !fileName[0]) return NULL;

  // See if it's already been loaded
  MeshMap::iterator i = meshCache.find( string(fileName) );
  if (i != meshCache.end())
  {
    return i->second->retain();
  }

  // No?  Try loading it
  MPD_Mesh *mesh = MPD_Mesh::load(fileName, msh);

  // If we succeeded, cache it
  if (mesh != NULL)
    meshCache[string(fileName)] = mesh;


  return mesh;
}