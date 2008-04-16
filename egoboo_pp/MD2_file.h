/* Tactics - MD2_Model.h
 * A class for loading/using Quake 2 and Egoboo md2 models.
 *
 * Creating/destroying objects of this class is done in the same fashion as
 * Textures, so see Texture.h for details.
 */

#pragma once

#include "RefCount.h"
#include "id_md2.h"
#include <SDL_opengl.h>

#include <map>
using std::map;

#include <string>
using std::string;

namespace JF
{
  #pragma pack(push,1)
  struct MD2_Vertex
  {
    float x, y, z;
    unsigned normal;	// index to id-normal array
  };

  struct MD2_TexCoord
  {
    float s, t;
  };

  struct MD2_Frame
  {
    char name[16];
    float min[3], max[3];		// axis-aligned bounding box limits
    MD2_Vertex *vertices;
  };
  #pragma pack(pop)

  struct MD2_Triangle : public md2_triangle {};
  struct MD2_SkinName : public md2_skinname {};

  struct MD2_GLCommand
  {
    MD2_GLCommand * next;
    GLenum          gl_mode;
    signed int      command_count;
    md2_gldata    * data;

    MD2_GLCommand()  { next = NULL; data = NULL; }
    ~MD2_GLCommand() { if(NULL!=next) delete next; if(NULL!=data) delete data; };
  };

  class MD2_Model : public RefCount<MD2_Model>
  {
    friend class MD2_Manager;

    public:

      int numVertices()  const { return m_numVertices; }
      int numTexCoords() const { return m_numTexCoords; }
      int numTriangles() const { return m_numTriangles; }
      int numSkins()     const { return m_numSkins; }
      int numFrames()    const { return m_numFrames; }

      const MD2_SkinName  *getSkin(int index) const;
      const MD2_Frame     *getFrame(int index) const;
      const MD2_TexCoord  *getTexCoords() const { return m_texCoords; }
      const MD2_Triangle  *getTriangles() const { return m_triangles; }
      const MD2_GLCommand *getCommands()  const { return m_commands;  }

      MD2_Model & getMD2() { return *this; };

    protected:
      int m_numVertices;
      int m_numTexCoords;
      int m_numTriangles;
      int m_numSkins;
      int m_numFrames;
      int m_numCommands;

      MD2_SkinName  *m_skins;
      MD2_TexCoord  *m_texCoords;
      MD2_Triangle  *m_triangles;
      MD2_Frame     *m_frames;
      MD2_GLCommand *m_commands;

      MD2_Model();
      virtual ~MD2_Model();

      void deallocate();

      static MD2_Model* load(const char *fileName, MD2_Model* mdl = NULL);
  };

  class MD2_Manager
  {
    public:
      static MD2_Model* loadFromFile(const char *fileName, MD2_Model* mdl = NULL);

    private:
      typedef map<string, MD2_Model*> ModelMap;
      static ModelMap modelCache;
  };

}

