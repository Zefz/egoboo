#pragma once

#include <SDL_opengl.h>
#include <assert.h>

#include "egobootypedef.h"
#include "mathstuff.h"
#include "id_md2.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct Camera;
struct Character;

#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME (1<<9)                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)             //
#define WATERPOINTS 4                               // Points in a water fan
#define WATERMODE   4                                 // Ummm...  For making it work, yeah...


//--------------------------------------------------------------------------------------------
typedef struct glvertex_t
{
  vec3_t pos;
  vec4_t fcolor;
  Uint32 icolor;   // can replace r,g,b,a with a call to glColor4ubv
  vec2_t txcoord;

  void convert_fcolor()
  {
    // icolor is ABGR
    icolor  = (int(fcolor.w*0xFF)<<24);
    icolor |= (int(fcolor.z*0xFF)<<16);
    icolor |= (int(fcolor.y*0xFF)<< 8);
    icolor |= (int(fcolor.x*0xFF)<< 0);
  };

  void convert_icolor()
  {
    fcolor.x = float((icolor >> 0) & 0xFF) / float(0xFF);
    fcolor.y = float((icolor >> 8) & 0xFF) / float(0xFF);
    fcolor.z = float((icolor >>16) & 0xFF) / float(0xFF);
    fcolor.w = float((icolor >>24) & 0xFF) / float(0xFF);
  };

} GLVertex;

//--------------------------------------------------------------------------------------------
struct Weather
{
  int    overwater;         // Only spawn over water?
  int    time_reset;         // Rate at which weather particles spawn
  int    time;              // 0 is no weather
  int    player;

  Weather()
  {
    overwater = false;
    time_reset = 10;
    time      = 0;
    player;
  }
};

//--------------------------------------------------------------------------------------------
struct Water
{
  Uint8           light_level; // General light amount (0-0x3F)
  Uint8           light_add;   // Ambient light amount (0-0x3F)
  float           z;     // Base height of water
  Uint8           alpha; // Transparency
  float           amp;   // Amplitude of waves
  vec2_t          off;     // Coordinates of texture
  vec2_t          off_add;  // Texture movement
  float           z_add[MAXWATERFRAME][2][2];
  Uint8           color[MAXWATERFRAME][2][2];
  Uint16          frame; // Frame
  Uint16          frame_add;      // Speed
  vec3_t          dist;         // For distant backgrounds
};

//--------------------------------------------------------------------------------------------
struct Water_List : public TList<Water, MAXWATERLAYER>
{
  int      layer_count;           // Number of layers
  float    level_surface;         // Surface level for water striders
  float    level_douse;           // Surface level for torches
  bool     is_light;              // Is it light ( render as a sprite )
  bool     is_water;              // Is it water?  ( Or lava... )
  Uint8    spek_start;            // Specular begins at which light value
  Uint8    spek_level;            // General specular amount (0-0xFF)
  Uint32   spek[0x0100];             // Specular highlights
  float    foreground_repeat;     //
  float    background_repeat;     //
  Uint8    shift;
  float    lx, ly, lz, la;

  Water_List()
  {
    layer_count         = 0;         // Number of layers
    level_surface     = 0;         // Surface level for water striders
    level_douse       = 0;         // Surface level for torches
    is_light          = false;         // Is it light ( default is alpha )
    is_water          = true;      // Is it water?  ( Or lava... )
    foreground_repeat = 1;         //
    background_repeat = 1;         //
    shift            = 3;
    spek_start        = 0x80;       // Specular begins at which light value
    spek_level        = 0x80;       // General specular amount (0-0xFF)

    lx = ly = lz = la = 0;
  };
};


//--------------------------------------------------------------------------------------------
//Fog stuff
struct Fog
{
  bool            allowed;           //
  Uint8           on;                // Do ground fog?
  float           bottom;            //
  float           top;               //
  float           distance;          //
  Uint8           red;               //
  Uint8           grn;               //
  Uint8           blu;               //
  Uint8           affectswater;

  Fog()
  {
    allowed   = true;
    on        = false;
    bottom    = 0.0;
    top       = 100;
    distance  = 100;
    red       = 0xFF;
    grn       = 0xFF;
    blu       = 0xFF;
  }

};

//--------------------------------------------------------------------------------------------
struct Tile_Dam
{
  short parttype;
  short partand;
  short sound;
  short soundtime;
  int   mindistance;

  int   amount;                         // Amount of damage
  Uint8 type;                           // Type of damage

  Tile_Dam();
};

extern Tile_Dam GTile_Dam;

//--------------------------------------------------------------------------------------------
struct Tile_Anim
{
  int    updateand;          // New tile every 7 frames
  Uint16 frameadd;           // Current frame

  Uint16 frameand;           // Only 4 frames
  Uint16 baseand;            //

  Uint16 bigframeand;        // For big tiles
  Uint16 bigbaseand;         //

  Tile_Anim();
};

extern Tile_Anim GTile_Anim;

//--------------------------------------------------------------------------------------------

template <typename _child_ty>
struct TLocker
{
  int  my_depth;
  static int depth;

  bool my_begun, my_ended;
  TLocker()  { my_begun = false; my_ended = true; };
  ~TLocker() { end(); };

  void begin()
  {
    my_begun = false;
    if(my_ended)
    {
      my_depth = depth;
      depth++;
      my_begun = true;
      my_ended = false;
    };
  }

  bool end()
  {
    my_ended = false;
    if(my_begun)
    {
      my_depth = depth;
      depth++;
      my_begun = false;
      my_ended = true;
    }

    return my_ended;
  }

};


template <typename _child_ty>
int TLocker<_child_ty>::depth = 0;

//--------------------------------------------------------------------------------------------
struct Locker_GraphicsMode : public TLocker<Locker_GraphicsMode>
{
  typedef TLocker<Locker_GraphicsMode> parent_type;

  enum eMode
  {
    MODE_UNKNOWN,
    MODE_2D,
    MODE_3D,
    MODE_TEXT
  };


  GLint matrix_mode;
  eMode old_mode;
  static eMode mode;

  bool my_begun, my_ended;
  Locker_GraphicsMode()  { my_begun = false; my_ended = true; };
  ~Locker_GraphicsMode()  { end(); };

  void begin() { parent_type::begin(); my_begin(); };
  bool end()
  {
    bool tmp, retval=false;
    tmp = parent_type::end(); retval = retval || tmp;
    tmp = my_end(); retval = retval || tmp;
    return retval;
  };

protected:
  void my_begin()
  {
    my_begun = false;
    if(my_ended)
    {
      glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);
      old_mode = mode;
      my_begun = true;
      my_ended = false;
    };
  };

  bool my_end()
  {
    my_ended = false;
    if(my_begun)
    {
      glMatrixMode(matrix_mode);
      mode = old_mode;
      my_begun = false;
      my_ended = true;
    };

    return my_ended;
  };
};

//--------------------------------------------------------------------------------------------
struct Locker_3DMode : public Locker_GraphicsMode
{
  typedef Locker_GraphicsMode parent_type;

  bool my_begun, my_ended;
  Locker_3DMode()  { my_begun = false; my_ended = true; };
  ~Locker_3DMode() { end(); };

  void begin()             { assert(false); throw Ego_Exception( "Locker_3DMode::begin() - invalid initializer"); };
  void begin(Camera & cam) { parent_type::begin(); my_begin(cam); };
  bool end()
  {
    bool tmp, retval=false;
    tmp = parent_type::end(); retval = retval || tmp;
    tmp = my_end(); retval = retval || tmp;
    return retval;
  };

protected:
  void my_begin(Camera & cam);
  bool my_end();
};

//--------------------------------------------------------------------------------------------
struct Locker_2DMode : public Locker_GraphicsMode
{
  typedef Locker_GraphicsMode parent_type;

  bool my_begun, my_ended;
  Locker_2DMode()  { my_begun = false; my_ended = true; };
  ~Locker_2DMode() { end(); };

  void begin() { parent_type::begin(); my_begin(); };
  bool end()
  {
    bool tmp, retval=false;
    tmp = parent_type::end(); retval = retval || tmp;
    tmp = my_end(); retval = retval || tmp;
    return retval;
  };

protected:
  void my_begin();
  bool my_end();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void Begin3DMode(Camera & cam);
void End3DMode();

void load_basic_textures(char *modname);
void load_bars(char* szBitmap);
void load_map(char* szModule, int sysmem);
//void load_one_icon(char *szLoadName);
void load_all_titleimages();
void load_blip_bitmap();

void prime_titleimage();

void release_grfx(void);
void release_map();

void create_szfpstext(int frames);

void figure_out_what_to_draw();
void debug_message(const char *format, ...);

void reset_end_text();
void append_end_text(int message, Uint16 character);

//TILES
void animate_tiles();

//WATER

void read_wawalite(char *modname);

//RENDERER
extern bool sdl_initialized;
extern bool ogl_initialized;
extern bool video_initialized;
int glinit(int argc, char **argv);
void sdlinit(int argc, char **argv);
void set_video_options();

void draw_main(Camera & cam);

//FONT
void load_font(char* szBitmap, char* szSpacing, int sysmem);

//UTILITY
// The next two functions are borrowed from the gl_font.c test program from SDL_ttf
int powerOfTwo(int input);
int copySurfaceToTexture(SDL_Surface *surface, GLuint texture, GLfloat *texCoords);


// JF - Added so that the video mode might be determined outside of the graphics code
extern SDL_Surface *displaySurface;

//FONT
void BeginText(void);
void EndText();
void draw_one_font(int fonttype, int x, int y);


//RENDERER
void EnableTexturing();
void DisableTexturing();

void dump_gl_state(char * str = NULL, bool modelview=false, bool projection=false, bool texture=false);

void reset_graphics();

void blend_md2_vertices(Character & chr, Uint32 vrtmin=0, Uint32 vrtmax = (Uint32)(-1));
extern GLfloat md2_blendedVertices[MD2_MAX_VERTICES][3];
extern GLfloat md2_blendedNormals[MD2_MAX_VERTICES][3];

void flip_pages();

void light_character(Character & rchr, struct Mesh & msh);

extern Water_List WaterList;
extern Fog        GFog;
extern Weather    GWeather;