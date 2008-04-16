/* Tactics - MD2_Model.cpp
*/

#include "MD2_file.h"
#include "id_md2.h"

#include <map>
#include <string>
#include <SDL_endian.h>		// TODO: Roll my own endian stuff so that I don't have to include
                          // SDL outside of the stuff that touches video/audio/input/etc.
                          // Not a high priority

extern float kMd2Normals[][3] =
{
#include "id_normals.inl"
,
 {0,0,0}  // Spikey mace
};


#define EGOBOO_MD2_SCALE 1.0f

using namespace std;
using namespace JF;

MD2_Manager::ModelMap MD2_Manager::modelCache;

MD2_Model::MD2_Model()
{
  m_numVertices  = 0;
  m_numTexCoords = 0;
  m_numTriangles = 0;
  m_numSkins     = 0;
  m_numFrames    = 0;

  m_skins     = NULL;
  m_texCoords = NULL;
  m_triangles = NULL;
  m_frames    = NULL;
  m_commands  = NULL;
}

void MD2_Model::deallocate()
{
  if(m_skins != NULL)
  {
    delete [] m_skins;
    m_skins = NULL;
    m_numSkins = 0;
  }

  if(m_texCoords != NULL)
  {
    delete[] m_texCoords;
    m_texCoords = NULL;
    m_numTexCoords = 0;
  }

  if(m_triangles != NULL)
  {
    delete[] m_triangles;
    m_triangles = NULL;
    m_numTriangles = 0;
  }

  if(m_frames != NULL)
  {
    for(int i = 0;i < m_numFrames;i++)
    {
      if(m_frames[i].vertices != NULL)
      {
        delete[] m_frames[i].vertices;
      }
    }
    delete[] m_frames;
    m_frames = NULL;
    m_numFrames = 0;
  }

  if(m_commands!=NULL)
  {
    delete m_commands;
    m_commands = NULL;
  }
};

MD2_Model::~MD2_Model()
{
  deallocate();
}

MD2_Model* MD2_Model::load(const char *fileName, MD2_Model* mdl)
{
  md2_header header;
  FILE *f;

  if(fileName == NULL) return NULL;

  // Open up the file, and make sure it's a MD2 model
  f = fopen(fileName, "rb");
  if (f == NULL) return NULL;

  fread(&header, sizeof(header), 1, f);

  // Convert the byte ordering in the header, if we need to
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  header.magic            = SDL_Swap32BE(header.magic);
  header.version          = SDL_Swap32BE(header.version);
  header.skinWidth        = SDL_Swap32BE(header.skinWidth);
  header.skinHeight       = SDL_Swap32BE(header.skinHeight);
  header.frameSize        = SDL_Swap32BE(header.frameSize);
  header.numSkins         = SDL_Swap32BE(header.numSkins);
  header.numVertices      = SDL_Swap32BE(header.numVertices);
  header.numTexCoords     = SDL_Swap32BE(header.numTexCoords);
  header.numTriangles     = SDL_Swap32BE(header.numTriangles);
  header.numGlCommands    = SDL_Swap32BE(header.numGlCommands);
  header.numFrames        = SDL_Swap32BE(header.numFrames);
  header.offsetSkins      = SDL_Swap32BE(header.offsetSkins);
  header.offsetTexCoords  = SDL_Swap32BE(header.offsetTexCoords);
  header.offsetTriangles  = SDL_Swap32BE(header.offsetTriangles);
  header.offsetFrames     = SDL_Swap32BE(header.offsetFrames);
  header.offsetGlCommands = SDL_Swap32BE(header.offsetGlCommands);
  header.offsetEnd        = SDL_Swap32BE(header.offsetEnd);
#endif

  if(header.magic != MD2_MAGIC_NUMBER || header.version != MD2_VERSION)
  {
    fclose(f);
    return NULL;
  }

  // Allocate a MD2_Model to hold all this stuff
  MD2_Model *model = (NULL==mdl) ? new MD2_Model() : mdl;
  model->m_numVertices = header.numVertices;
  model->m_numTexCoords = header.numTexCoords;
  model->m_numTriangles = header.numTriangles;
  model->m_numSkins = header.numSkins;
  model->m_numFrames = header.numFrames;

  model->m_texCoords = new MD2_TexCoord[header.numTexCoords];
  model->m_triangles = new MD2_Triangle[header.numTriangles];
  model->m_skins     = new MD2_SkinName[header.numSkins];
  model->m_frames    = new MD2_Frame[header.numFrames];

  for (int i = 0;i < header.numFrames; i++)
  {
    model->m_frames[i].vertices = new MD2_Vertex[header.numVertices];
  }

  // Load the texture coordinates from the file, normalizing them as we go
  fseek(f, header.offsetTexCoords, SEEK_SET);
  for (int i = 0;i < header.numTexCoords; i++)
  {
    md2_texcoord tc;
    fread(&tc, sizeof(tc), 1, f);

    // Convert the byte order of the texture coordinates, if we need to
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    tc.s = SDL_Swap16(tc.s);
    tc.t = SDL_Swap16(tc.t);
#endif
    model->m_texCoords[i].s = tc.s / (float)header.skinWidth;
    model->m_texCoords[i].t = tc.t / (float)header.skinHeight;
  }

  // Load triangles from the file.  I use the same memory layout as the file
  // on a little endian machine, so they can just be read directly
  fseek(f, header.offsetTriangles, SEEK_SET);
  fread(model->m_triangles, sizeof(md2_triangle), header.numTriangles, f);

  // Convert the byte ordering on the triangles, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  for(int i = 0;i < header.numTriangles;i++)
  {
    for(v = 0;v < 3;v++)
    {
      model->m_triangles[i].vertexIndices[v]   = SDL_Swap16(model->triangles[i].vertexIndices[v]);
      model->m_triangles[i].texCoordIndices[v] = SDL_Swap16(model->triangles[i].texCoordIndices[v]);
    }
  }
#endif

  // Load the skin names.  Again, I can load them directly
  fseek(f, header.offsetSkins, SEEK_SET);
  fread(model->m_skins, sizeof(struct md2_skinname), header.numSkins, f);

  // Load the frames of animation
  fseek(f, header.offsetFrames, SEEK_SET);
  for(int i = 0;i < header.numFrames;i++)
  {
    char frame_buffer[MD2_MAX_FRAMESIZE];
    md2_frame *frame;

    // read the current frame
    fread(frame_buffer, header.frameSize, 1, f);
    frame = (struct md2_frame*)frame_buffer;

    // Convert the byte ordering on the scale & translate vectors, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    frame->scale[0] = swapFloat(frame->scale[0]);
    frame->scale[1] = swapFloat(frame->scale[1]);
    frame->scale[2] = swapFloat(frame->scale[2]);

    frame->translate[0] = swapFloat(frame->translate[0]);
    frame->translate[1] = swapFloat(frame->translate[1]);
    frame->translate[2] = swapFloat(frame->translate[2]);
#endif

    // unpack the md2 vertices from this frame
    for(int v = 0;v < header.numVertices;v++)
    {
      model->m_frames[i].vertices[v].x = frame->vertices[v].vertex[0] * frame->scale[0] + frame->translate[0];
      model->m_frames[i].vertices[v].y = frame->vertices[v].vertex[1] * frame->scale[1] + frame->translate[1];
      model->m_frames[i].vertices[v].z = frame->vertices[v].vertex[2] * frame->scale[2] + frame->translate[2];

      model->m_frames[i].vertices[v].normal = frame->vertices[v].lightNormalIndex;
    }

    // Calculate the bounding box for this frame
    model->m_frames[i].min[0] = frame->translate[0];
    model->m_frames[i].min[1] = frame->translate[1];
    model->m_frames[i].min[2] = frame->translate[2];
    model->m_frames[i].max[0] = frame->translate[0] + (frame->scale[0] * float(0xFF));
    model->m_frames[i].max[1] = frame->translate[1] + (frame->scale[1] * float(0xFF));
    model->m_frames[i].max[2] = frame->translate[2] + (frame->scale[2] * float(0xFF));

    //make sure to copy the frame name!
    strncpy(model->m_frames[i].name, frame->name, 16);
  }

  //Load up the pre-computed OpenGL optimizations
  if(header.numGlCommands > 0)
  {
    Uint32          cmd_cnt = 0;
    MD2_GLCommand * cmd     = NULL;
    fseek(f, header.offsetGlCommands, SEEK_SET);

    //count the commands
    while( cmd_cnt < header.numGlCommands )
    {
      //read the number of commands in the strip
      Sint32 commands;
      fread(&commands, sizeof(Sint32), 1, f);
      commands = SDL_SwapLE32(commands);
      if(commands==0)
        break;

      cmd = new MD2_GLCommand;
      cmd->command_count = commands;

      //set the GL drawing mode
      if(cmd->command_count > 0)
      {
        cmd->gl_mode = GL_TRIANGLE_STRIP;
      }
      else
      {
        cmd->command_count = -cmd->command_count;
        cmd->gl_mode = GL_TRIANGLE_FAN;
      }

      //allocate the data
      cmd->data = new md2_gldata[cmd->command_count];

      //read in the data
      fread(cmd->data, sizeof(md2_gldata), cmd->command_count, f);

      //translate the data, if necessary
#if SDL_BYTEORDER != SDL_LIL_ENDIAN
      for(int i=0; i<cmd->command_count; i++)
      {
        cmd->data[i].index = SDL_swap32(cmd->data[i].s);
        cmd->data[i].s     = swapFloat(cmd->data[i].s);
        cmd->data[i].t     = swapFloat(cmd->data[i].t);
      };
#endif

      // attach it to the command list
      cmd->next         = model->m_commands;
      model->m_commands = cmd;

      cmd_cnt += cmd->command_count;
    };
  }


  // Close the file, we're done with it
  fclose(f);

  return model;
}

const MD2_SkinName *MD2_Model::getSkin(int index) const
{
  if(index >= 0 && index < m_numSkins)
  {
    return &m_skins[index];
  }
  return NULL;
}

const MD2_Frame *MD2_Model::getFrame(int index) const
{
  if(index >= 0 && index < m_numFrames)
  {
    return &m_frames[index];
  }
  return NULL;
}


MD2_Model* MD2_Manager::loadFromFile(const char *fileName, MD2_Model* mdl)
{
  // ignore garbage input
  if (!fileName || !fileName[0]) return NULL;

  // See if it's already been loaded
  ModelMap::iterator i = modelCache.find(string(fileName));
  if (i != modelCache.end())
  {
    return i->second->retain();
  }

  // No?  Try loading it
  MD2_Model *model = MD2_Model::load(fileName, mdl);
  if (model == NULL)
  {
    // no luck
    return NULL;
  }

  modelCache[string(fileName)] = model;
  return model;
}
