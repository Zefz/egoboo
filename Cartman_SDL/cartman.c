#include <stdio.h>			// For printf and such
#include <fcntl.h>			// For fast file i/o
#include <math.h>
#include <assert.h>

#include <SDL.h>			  // SDL header
#include <SDL_opengl.h>
#include <SDL_image.h>

#include "Log.h"
#include "ConfigFile.h"
#include "Font.h"
#include "SDL_Pixel.h"
#include "egoboo_endian.h"

#ifndef ABS
#    define ABS(X)  (((X) > 0) ? (X) : -(X))
#endif

#ifndef SGN
#    define SGN(X)  (((X) >= 0) ? 1 : -1)
#endif

#ifndef MIN

#    define MIN(x, y)  (((x) > (y)) ? (y) : (x))
#endif

#ifndef MAX
#    define MAX(x, y)  (((x) > (y)) ? (x) : (y))
#endif

#ifndef CLIP
#    define CLIP(A,B,C) MIN(MAX(A,B),C)
#endif


#define MAXMESSAGE          6                       // Number of messages
#define TOTALMAXDYNA                    64          // Absolute max number of dynamic lights
#define TOTALMAXPRT             2048                // True max number of particles


TTF_Font * gFont = NULL;

bool_t                  keyon = btrue;                // Is the keyboard alive?
int                     keycount = 0;
Uint8  *                keysdlbuffer = NULL;
Uint8                   keystate = 0;
#define SDLKEYDOWN(k)  ( ((k >= keycount) || (NULL == keysdlbuffer)) ? bfalse : (0 != keysdlbuffer[k]))     // Helper for gettin' em
#define SDLKEYMOD(m)   ( (NULL != keysdlbuffer) && (0 != (keystate & (m))) )
#define SDLKEYDOWN_MOD(k,m) ( SDLKEYDOWN(k) && (0 != (keystate & (m))) )

STRING egoboo_path;

#define HAS_BITS(A, B) ( 0 != ((A)&(B)) )

#define BOOL_TO_BIT(X) (X ? 1 : 0 )
#define BIT_TO_BOOL(X) ((X==1) ? btrue : bfalse )

// OpenGL Texture filtering
typedef enum e_tx_filters
{
  TX_UNFILTERED,
  TX_LINEAR,
  TX_MIPMAP,
  TX_BILINEAR,
  TX_TRILINEAR_1,
  TX_TRILINEAR_2,
  TX_ANISOTROPIC
} TX_FILTERS;

// OpenGL graphics info
struct s_glInfo
{
  // stack depths
  GLint max_modelview_stack_depth;     // Maximum modelview-matrix stack depth
  GLint max_projection_stack_depth;    // Maximum projection-matrix stack depth
  GLint max_texture_stack_depth;       // Maximum depth of texture matrix stack
  GLint max_name_stack_depth;          // Maximum selection-name stack depth
  GLint max_attrib_stack_depth;        // Maximum depth of the attribute stack
  GLint max_client_attrib_stack_depth; // Maximum depth of the client attribute stack

  // Antialiasing settings
  GLint   subpixel_bits;           // Number of bits of subpixel precision in x and y
  GLfloat point_size_range[2];     // Range (low to high) of antialiased point sizes
  GLfloat point_size_granularity;  // Antialiased point-size granularity
  GLfloat line_width_range[2];     // Range (low to high) of antialiased line widths
  GLfloat line_width_granularity;  // Antialiased line-width granularity

  // display settings
  GLint     max_viewport_dims[2];  // Maximum viewport dimensions
  GLboolean aux_buffers;           // Number of auxiliary buffers
  GLboolean rgba_mode;             // True if color buffers store RGBA
  GLboolean index_mode;            // True if color buffers store indices
  GLboolean doublebuffer;          // True if front and back buffers exist
  GLboolean stereo;                // True if left and right buffers exist
  GLint     red_bits;              // Number of bits per red component in color buffers
  GLint     green_bits;            // Number of bits per green component in color buffers
  GLint     blue_bits;             // Number of bits per blue component in color buffers
  GLint     alpha_bits;            // Number of bits per alpha component in color buffers
  GLint     index_bits;            // Number of bits per index in color buffers
  GLint     depth_bits;            // Number of depth-buffer bitplanes
  GLint     stencil_bits;          // Number of stencil bitplanes
  GLint     accum_red_bits;        // umber of bits per red component in the accumulation buffer
  GLint     accum_green_bits;      // umber of bits per green component in the accumulation buffer
  GLint     accum_blue_bits;       // umber of bits per blue component in the accumulation buffer
  GLint     accum_alpha_bits;      // umber of bits per blue component in the accumulation buffer

  // Misc
  GLint max_lights;                    // Maximum number of lights
  GLint max_clip_planes;               // Maximum number of user clipping planes
  GLint max_texture_size;              // See discussion in "Texture Proxy" in Chapter 9

  GLint max_pixel_map_table;           // Maximum size of a glPixelMap() translation table
  GLint max_list_nesting;              // Maximum display-list call nesting
  GLint max_eval_order;                // Maximum evaluator polynomial order
};
typedef struct s_glInfo glInfo_t;

// SDL graphics info
struct s_screen_info
{
  int    d;               // Screen bit depth
  int    z;               // Screen z-buffer depth ( 8 unsupported )
  int    x;               // Screen X size
  int    y;               // Screen Y size

  Uint8  alpha;

  glInfo_t gl_info;

  // SDL OpenGL attributes
  int red_d;  // SDL_GL_RED_SIZE Size of the framebuffer red component, in bits
  int grn_d;  // SDL_GL_GREEN_SIZE Size of the framebuffer green component, in bits
  int blu_d;  // SDL_GL_BLUE_SIZE Size of the framebuffer blue component, in bits
  int alp_d;  // SDL_GL_ALPHA_SIZE Size of the framebuffer alpha component, in bits
  int dbuff;  // SDL_GL_DOUBLEBUFFER 0 or 1, enable or disable double buffering
  int buf_d;  // SDL_GL_BUFFER_SIZE Size of the framebuffer, in bits
  int zbf_d;  // SDL_GL_DEPTH_SIZE Size of the depth buffer, in bits
  int stn_d;  // SDL_GL_STENCIL_SIZE Size of the stencil buffer, in bits
  int acr_d;  // SDL_GL_ACCUM_RED_SIZE Size of the accumulation buffer red component, in bits
  int acg_d;  // SDL_GL_ACCUM_GREEN_SIZE Size of the accumulation buffer green component, in bits
  int acb_d;  // SDL_GL_ACCUM_BLUE_SIZE Size of the accumulation buffer blue component, in bits
  int aca_d;  // SDL_GL_ACCUM_ALPHA_SIZE Size of the accumulation buffer alpha component, in bits


  // selected SDL bitfields
  unsigned hw_available:1;
  unsigned wm_available:1;
  unsigned blit_hw:1;
  unsigned blit_hw_CC:1;
  unsigned blit_hw_A:1;
  unsigned blit_sw:1;
  unsigned blit_sw_CC:1;
  unsigned blit_sw_A:1;

  unsigned is_sw:1;           // SDL_SWSURFACE Surface is stored in system memory
  unsigned is_hw:1;           // SDL_HWSURFACE Surface is stored in video memory
  unsigned use_asynch_blit:1; // SDL_ASYNCBLIT Surface uses asynchronous blits if possible
  unsigned use_anyformat:1;   // SDL_ANYFORMAT Allows any pixel-format (Display surface)
  unsigned use_hwpalette:1;     // SDL_HWPALETTE Surface has exclusive palette
  unsigned is_doublebuf:1;     // SDL_DOUBLEBUF Surface is double buffered (Display surface)
  unsigned is_fullscreen:1;    // SDL_FULLSCREEN Surface is full screen (Display Surface)
  unsigned use_opengl:1;        // SDL_OPENGL Surface has an OpenGL context (Display Surface)
  unsigned use_openglblit:1;    // SDL_OPENGLBLIT Surface supports OpenGL blitting (Display Surface)
  unsigned sdl_resizable:1;     // SDL_RESIZABLE Surface is resizable (Display Surface)
  unsigned use_hwaccel:1;       // SDL_HWACCEL Surface blit uses hardware acceleration
  unsigned has_srccolorkey:1;   // SDL_SRCCOLORKEY Surface use colorkey blitting
  unsigned use_rleaccel:1;      // SDL_RLEACCEL Colorkey blitting is accelerated with RLE
  unsigned use_srcalpha:1;      // SDL_SRCALPHA Surface blit uses alpha blending
  unsigned is_prealloc:1;      // SDL_PREALLOC Surface uses preallocated memory
};
typedef struct s_screen_info screen_info_t;

struct s_ui_state
{
  int    cur_x;              // Cursor position
  int    cur_y;              //

  bool_t pressed;                //
  bool_t clicked;                //
  bool_t pending_click;

  screen_info_t scr;

  bool_t GrabMouse;
  bool_t HideMouse;
};
typedef struct s_ui_state ui_state_t;
ui_state_t ui;

struct s_config_data
{
  Uint8    reffadeor;        // 255 = Don't fade reflections

  Uint16   messagetime;
  int      maxtotalmeshvertices;  // max number of kilobytes of vertices

  bool_t   messageon;        // Messages?
  int      maxmessage;       //

  bool_t   overlayon;        // Draw overlay?
  bool_t   perspective;      // Perspective correct textures?
  bool_t   dither;           // Dithering?
  GLuint   shading;          // Gourad shading?
  bool_t   antialiasing;     // Antialiasing?
  bool_t   refon;            // Reflections?
  bool_t   shaon;            // Shadows?
  int      texturefilter;    // Texture filtering?
  bool_t   wateron;          // Water overlays?
  bool_t   shasprite;        // Shadow sprites?
  bool_t   zreflect;         // Reflection z buffering?

  int      maxsoundchannel;      // Max number of sounds playing at the same time
  int      buffersize;          // Buffer size set in setup.txt

  bool_t   fullscreen;       // Start in fullscreen?
  bool_t   backgroundon;    // Do we clear every time?
  bool_t   soundon;              // Is the sound alive?
  bool_t   staton;               // Draw the status bars?
  bool_t   phongon;              // Do phong overlay?
  bool_t   networkon;            // Try to connect?
  bool_t   serviceon;            // Do I need to free the interface?
  bool_t   twolayerwateron;      // Two layer water?

  Uint8    camautoturn;          // Type of camera control...
  bool_t   fogallowed;           //
  bool_t   fpson;                // FPS displayed?
  int      frame_limit;

  bool_t   backgroundvalid;      // Allow large background?
  bool_t   overlayvalid;         // Allow large overlay?

  int      lag;                  // Lag tolerance
  int      orderlag;             // Lag tolerance for RTS games
  size_t   mesh_vert_count;

  screen_info_t    scr;       // Requested screen parameters

  int    maxlights;	          // Max number of lights to draw
  Uint16 maxparticles;        // max number of particles

  bool_t soundvalid;          // Allow sound?
  Sint32 soundvolume;         // The sound volume

  bool_t musicvalid;          // Allow music and loops?
  Sint32 musicvolume;         // The sound volume of music

  char   nethostname[64];                            // Name for hosting session
  char   netmessagename[64];                         // Name for messages

  int    wraptolerance;        // Status bar

  bool_t GrabMouse;
  bool_t HideMouse;
  bool_t DevMode;
};
typedef struct s_config_data config_data_t;
config_data_t cfg;

struct s_mouse
{
  int   x, y, x_old, y_old, b, cx, cy;
  int		tlx, tly, brx, bry;
  int		speed;
  bool_t relative;
  Uint8 button[4];             // Mouse button states
};

typedef struct s_mouse mouse_t;

mouse_t mos =
{
  0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0,
  2
};

#define NEARLOW  0.0 //16.0		// For autoweld
#define NEARHI 128.0 //112.0		//
#define BARRIERHEIGHT 14.0		//

#define MAKE_BGR(BMP,BB,GG,RR)     SDL_MapRGBA(BMP->format, (RR)<<3, (GG)<<3, (BB)<<3, 0xFF)
#define MAKE_ABGR(BMP,AA,BB,GG,RR) SDL_MapRGBA(BMP->format, (RR)<<3, (GG)<<3, (BB)<<3, AA)

SDL_Color MAKE_SDLCOLOR(Uint8 BB, Uint8 RR, Uint8 GG) { SDL_Color tmp; tmp.r = RR<<3; tmp.g = GG<<3; tmp.b = BB<<3; return tmp; }

typedef int            int32;
typedef short          int16;
typedef char           int08;

#ifdef _WIN32
#    define SLASH_STR "\\"
#    define SLASH_CHR '\\'
#else
#    define SLASH_STR "/"
#    define SLASH_CHR '/'
#endif


#define MAXLIGHT 100
#define MAXRADIUS 500*FOURNUM
#define MINRADIUS 50*FOURNUM
#define MAXLEVEL 255
#define MINLEVEL 50
size_t numwritten = 0;
size_t numattempt = 0;

int addinglight;

int numlight;
struct s_light
{
  int           x;
  int           y;
  Uint8 level;
  int           radius;
};
typedef struct s_light light_t;
light_t light_lst[MAXLIGHT];

int ambi = 22;
int ambicut = 1;
int direct = 16;

#define VERSION 005			// Version number
#define YEAR 1999			// Year
#define NAME "Cartman"			// Program name
#define KEYDELAY 12			// Delay for keyboard
#define MAXTILE 256			//
#define TINYX 4 //8				// Plan tiles
#define TINYY 4 //8				//
#define SMALLX 32			// Small tiles
#define SMALLY 32			//
#define BIGX 64				// Big tiles
#define BIGY 64				//
#define CAMRATE 8			// Arrow key movement rate
#define MAXSELECT 2560			// Max points that can be select_vertsed
#define FOURNUM 4.137			// Magic number
#define FIXNUM  4.125 // 4.150		// Magic number
#define MAPID 0x4470614d		// The string... MapD

#define FADEBORDER 64			// Darkness at the edge of map
#define SLOPE 50			// Twist stuff

#define MAXWIN 8			// Number of windows

#define WINMODE_NOTHING 0			// Window display mode
#define WINMODE_TILE 1			//
#define WINMODE_VERTEX 2			//
#define WINMODE_SIDE 4			//
#define WINMODE_FX 8				//

//#define ONSIZE 600			// Max size of raise mesh
#define ONSIZE 264			// Max size of raise mesh

#define MAXMESHTYPE 64			// Number of mesh types
#define MAXMESHLINE 64			// Number of lines in a fan schematic
#define MAXMESHVERTICES 16		// Max number of vertices in a fan
#define MAXMESHFAN (512*512)		// Size of map in fans
#define MAXTOTALMESHVERTICES (MAXMESHFAN*MAXMESHVERTICES)
#define MAXMESHSIZEY 1024		// Max fans in y direction
#define MAXMESHTYPE 64			// Number of vertex configurations
#define FANOFF 0xffff			// Don't draw
#define CHAINEND 0xffffffff		// End of vertex chain
#define VERTEXUNUSED 0			// Check mesh.vrta to see if used
#define MAXPOINTS 20480			// Max number of points to draw

#define MPDFX_REF 0			   // MeshFX
#define MPDFX_SHA 1			   //
#define MPDFX_DRAWREF 2		 //
#define MPDFX_ANIM 4			 //
#define MPDFX_WATER 8			 //
#define MPDFX_WALL 16			 //
#define MPDFX_IMPASS 32			//
#define MPDFX_DAMAGE 64			//
#define MPDFX_SLIPPY 128		//

Uint16 animtileframeand = 3;                // Only 4 frames
Uint16 animtilebaseand = 0xfffc;            //
Uint16 biganimtileframeand = 7;             // For big tiles
Uint16 biganimtilebaseand = 0xfff8;         //
Uint16 animtileframeadd = 0;                // Current frame

char  loadname[256];		// Text

int		brushsize = 3;		// Size of raise/lower terrain brush
int		brushamount = 50;	// Amount of raise/lower

#define MAXPOINTSIZE 16

SDL_Surface	*theSurface = NULL;
SDL_Surface	*imgcursor;		// Cursor image
SDL_Surface	*imgpoint[MAXPOINTSIZE];		// Vertex image
SDL_Surface	*imgpointon[MAXPOINTSIZE];	// Vertex image ( select_vertsed )
SDL_Surface	*imgref;		// Meshfx images
SDL_Surface	*imgdrawref;		//
SDL_Surface	*imganim;		//
SDL_Surface	*imgwater;		//
SDL_Surface	*imgwall;		//
SDL_Surface	*imgimpass;		//
SDL_Surface	*imgdamage;		//
SDL_Surface	*imgslippy;		//

SDL_Surface		*bmphitemap;		// Heightmap image
SDL_Surface		*bmpsmalltile[MAXTILE];	// Tiles
SDL_Surface		*bmpbigtile[MAXTILE];	//
SDL_Surface		*bmptinysmalltile[MAXTILE];	// Plan tiles
SDL_Surface		*bmptinybigtile[MAXTILE];	//
SDL_Surface		*bmpfanoff;		//
int		numsmalltile = 0;	//
int		numbigtile = 0;		//

int		numpointsonscreen = 0;
Uint32	pointsonscreen[MAXPOINTS];

int		numselect_verts = 0;
Uint32	select_verts[MAXSELECT];

float		debugx = -1;		// Blargh
float		debugy = -1;		//

struct s_mouse_data
{
  int		          which_win;	// More mouse_ data
  int		          x;	      //
  int		          y;	      //
  Uint16	mode;	    // Window mode
  int	            onfan;	  // Fan mouse_ is on
  Uint16	tile;	    // Tile
  Uint16	presser;	// Random add for tiles
  Uint8	  type;	    // Fan type
  Uint8	  fx;
  int		          rect;	    // Rectangle drawing
  int		          rectx;	  //
  int		          recty;	  //
};

typedef struct s_mouse_data mouse_data_t;

mouse_data_t mdata =
{
  -1,	        // which_win
    -1,	        // x
    -1,	        // y
    0,	        // mode
    0,	        // onfan
    0,	        // tile
    0,	        // presser
    0,	        // type
    MPDFX_SHA, // fx
    0,	        // rect
    0,	        // rectx
    0 	        // recty
};

struct s_window
{
  SDL_Surface		     *bmp;	// Window images
  Uint8	  on;	// Draw it?
  int		          x;	// Window position
  int		          y;	//
  int		          borderx;	// Window border size
  int		          bordery;	//
  int		          surfacex;	// Window surface size
  int		          surfacey;	//
  Uint16	mode;	// Window display mode
};
typedef struct s_window window_t;

static window_t window_lst[MAXWIN];

int		colordepth = 32;		// 256 colors
int		keydelay = 0;		//

struct s_camera
{
  int	x;			//
  int	y;			//
};
typedef struct s_camera camera_t;
camera_t cam = {0,0};

Uint32	atvertex = 0;			// Current vertex check for new
Uint32	numfreevertices = 0;		// Number of free vertices

struct s_command
{
  Uint8   numvertices;                // Number of vertices
  Uint8   ref[MAXMESHVERTICES];       // Lighting references
  int             x[MAXMESHVERTICES];         // Vertex texture posi
  int             y[MAXMESHVERTICES];         //
};
typedef struct s_command command_t;

struct s_line_data
{
  Uint8	  start[MAXMESHTYPE];
  Uint8	  end[MAXMESHTYPE];
};
typedef struct s_line_data line_data_t;

struct s_mesh
{
  int		          sizex;			// Size of mesh
  int		          sizey;			//
  int		          edgex;			// Borders of mesh
  int		          edgey;			//
  int		          edgez;			//

  Uint32	fanstart[MAXMESHSIZEY];	// Y to fan number
  Uint8	  type[MAXMESHFAN];		// Command type
  Uint8	  fx[MAXMESHFAN];		// Special effects flags
  Uint16	tile[MAXMESHFAN];		// Get texture from this
  Uint8	  twist[MAXMESHFAN];		// Surface normal

  Uint32	vrtstart[MAXMESHFAN];	// Which vertex to start at

  Uint16	vrtx[MAXTOTALMESHVERTICES];   // Vertex position
  Uint16	vrty[MAXTOTALMESHVERTICES];	   //
  Sint16	vrtz[MAXTOTALMESHVERTICES];  	 // Vertex elevation
  Uint8	  vrta[MAXTOTALMESHVERTICES];	   // Vertex base light, 0=unused
  Uint32	vrtnext[MAXTOTALMESHVERTICES]; // Next vertex in fan

  Uint32	     numline[MAXMESHTYPE];	// Number of lines to draw
  line_data_t  line[MAXMESHLINE];
  command_t    command[MAXMESHTYPE];
};
typedef struct s_mesh mesh_t;

mesh_t mesh;

#include "standard.c"			// Some functions that I always use
//#include "scale.c"			// Scaling functions
//#include "mouse3.c"			// The mouse functions...

//   mos.x is mouse x position
//   mos.y is mouse y position
//   mos.cx is mouse x counter
//   mos.cy is mouse y counter
//   mos.b is mouse button state

void sdlinit( int argc, char **argv );
void read_setup( config_data_t * pc, char* filename );
int           cartman_BlitScreen(SDL_Surface * bmp, SDL_Rect * rect);
SDL_Surface * cartman_CreateSurface(int w, int h);
SDL_Surface * cartman_LoadIMG(const char * szName);
void read_mouse();
void draw_sprite(SDL_Surface * dst, SDL_Surface * sprite, int x, int y );
void line(SDL_Surface * surf, int x0, int y0, int x1, int y1, Uint32 c);
bool_t SDL_RectIntersect(SDL_Rect * src, SDL_Rect * dst, SDL_Rect * isect );
int cartman_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);


//--------------------------------------------------------------------------------------------
void do_cursor()
{
  // This function implements a mouse cursor
  ui.cur_x = mos.x;  if ( ui.cur_x < 6 )  ui.cur_x = 6;  if ( ui.cur_x > ui.scr.x - 6 )  ui.cur_x = ui.scr.x - 6;
  ui.cur_y = mos.y;  if ( ui.cur_y < 6 )  ui.cur_y = 6;  if ( ui.cur_y > ui.scr.y - 6 )  ui.cur_y = ui.scr.y - 6;
  ui.clicked = bfalse;
  if ( mos.button[0] && !ui.pressed )
  {
    ui.clicked = btrue;
  }
  ui.pressed = mos.button[0];
}

//------------------------------------------------------------------------------
void add_light(int x, int y, int radius, int level)
{
  if(numlight >= MAXLIGHT)  numlight = MAXLIGHT-1;

  light_lst[numlight].x = x;
  light_lst[numlight].y = y;
  light_lst[numlight].radius = radius;
  light_lst[numlight].level = level;
  numlight++;
}

//------------------------------------------------------------------------------
void alter_light(int x, int y)
{
  int radius, level;

  numlight--;
  if(numlight < 0)  numlight = 0;
  radius = abs(light_lst[numlight].x - x);
  level = abs(light_lst[numlight].y - y);
  if(radius > MAXRADIUS)  radius = MAXRADIUS;
  if(radius < MINRADIUS)  radius = MINRADIUS;
  light_lst[numlight].radius = radius;
  if(level > MAXLEVEL) level = MAXLEVEL;
  if(level < MINLEVEL) level = MINLEVEL;
  light_lst[numlight].level = level;
  numlight++;
}

//------------------------------------------------------------------------------
void draw_light(int number, SDL_Surface* window_surface, int window)
{
  int xdraw, ydraw, radius;
  Uint8 color;

  xdraw = (light_lst[number].x/FOURNUM) - cam.x + (window_lst[window].surfacex>>1) - SMALLX;
  ydraw = (light_lst[number].y/FOURNUM) - cam.y + (window_lst[window].surfacey>>1) - SMALLY;
  radius = abs(light_lst[number].radius)/FOURNUM;
  color = light_lst[number].level>>3;
  color = MAKE_BGR(window_surface,color,color,color);
  //circle(window_surface, xdraw, ydraw, radius, color);
}

//------------------------------------------------------------------------------
int dist_from_border(int x, int y)
{
  if(x > (mesh.edgex>>1))
    x = mesh.edgex-x-1;
  if(y > (mesh.edgey>>1))
    y = mesh.edgey-y-1;
  if(x < 0) x = 0;
  if(y < 0) y = 0;
  if(x < y)
    return x;
  return y;
}

//------------------------------------------------------------------------------
int dist_from_edge(int x, int y)
{
  if(x > (mesh.sizex>>1))
    x = mesh.sizex-x-1;
  if(y > (mesh.sizey>>1))
    y = mesh.sizey-y-1;
  if(x < y)
    return x;
  return y;
}

//------------------------------------------------------------------------------
int get_fan(int x, int y)
{
  int fan = -1;
  if(y >=0 && y < MAXMESHSIZEY && y < mesh.sizey)
  {
    if(x>=0 && x<mesh.sizex)
    {
      fan = mesh.fanstart[y]+x;
    }
  }
  return fan;
};

//------------------------------------------------------------------------------
int fan_is_floor(int x, int y)
{
  int fan;

  fan = get_fan(x, y);
  if(fan != -1)
  {
    if((mesh.fx[fan]&48)==0)
    {
      return 1;
    }
  }
  return 0;
}

//------------------------------------------------------------------------------
void set_barrier_height(int x, int y)
{
  Uint32 type, fan, vert;
  int cnt, noedges;
  float bestprox, prox, tprox, scale;

  fan = get_fan(x, y);
  if(fan != -1)
  {
    if(mesh.fx[fan]&MPDFX_WALL)
    {
      type = mesh.type[fan];
      noedges = btrue;
      vert = mesh.vrtstart[fan];
      cnt = 0;
      while(cnt < mesh.command[type].numvertices)
      {
        bestprox = 2*(NEARHI-NEARLOW)/3.0;
        if(fan_is_floor(x+1, y))
        {
          prox = NEARHI-mesh.command[type].x[cnt];
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(x, y+1))
        {
          prox = NEARHI-mesh.command[type].y[cnt];
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(x-1, y))
        {
          prox = mesh.command[type].x[cnt]-NEARLOW;
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(x, y-1))
        {
          prox = mesh.command[type].y[cnt]-NEARLOW;
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(noedges)
        {
          // Surrounded by walls on all 4 sides, but it may be a corner piece
          if(fan_is_floor(x+1, y+1))
          {
            prox = NEARHI-mesh.command[type].x[cnt];
            tprox = NEARHI-mesh.command[type].y[cnt];
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(x+1, y-1))
          {
            prox = NEARHI-mesh.command[type].x[cnt];
            tprox = mesh.command[type].y[cnt]-NEARLOW;
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(x-1, y+1))
          {
            prox = mesh.command[type].x[cnt]-NEARLOW;
            tprox = NEARHI-mesh.command[type].y[cnt];
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(x-1, y-1))
          {
            prox = mesh.command[type].x[cnt]-NEARLOW;
            tprox = mesh.command[type].y[cnt]-NEARLOW;
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
        }
        scale = window_lst[mdata.which_win].surfacey-(mdata.y/FOURNUM);
        bestprox = bestprox*scale*BARRIERHEIGHT/window_lst[mdata.which_win].surfacey;
        if(bestprox > mesh.edgez) bestprox = mesh.edgez;
        if(bestprox < 0) bestprox = 0;
        mesh.vrtz[vert] = bestprox;
        vert = mesh.vrtnext[vert];
        cnt++;
      }
    }
  }
}

//------------------------------------------------------------------------------
void fix_walls()
{
  int x, y;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      set_barrier_height(x, y);
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void impass_edges(int amount)
{
  int x, y;
  int fan;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      if(dist_from_edge(x, y) < amount)
      {
        fan = get_fan(x,y);
        if(fan != -1)
        {
          mesh.fx[fan] = mesh.fx[fan]|MPDFX_IMPASS;
        };
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
Uint8 get_twist(int x, int y)
{
  Uint8 twist;

  // x and y should be from -7 to 8
  if(x < -7) x = -7;
  if(x > 8) x = 8;
  if(y < -7) y = -7;
  if(y > 8) y = 8;
  // Now between 0 and 15
  x = x+7;
  y = y+7;
  twist = (y<<4)+x;
  return twist;
}

//------------------------------------------------------------------------------
Uint8 get_fan_twist(Uint32 fan)
{
  int zx, zy, vt0, vt1, vt2, vt3;
  Uint8 twist;

  vt0 = mesh.vrtstart[fan];
  vt1 = mesh.vrtnext[vt0];
  vt2 = mesh.vrtnext[vt1];
  vt3 = mesh.vrtnext[vt2];
  zx = (mesh.vrtz[vt0]+mesh.vrtz[vt3]-mesh.vrtz[vt1]-mesh.vrtz[vt2])/SLOPE;
  zy = (mesh.vrtz[vt2]+mesh.vrtz[vt3]-mesh.vrtz[vt0]-mesh.vrtz[vt1])/SLOPE;
  twist = get_twist(zx, zy);

  return twist;
}

//------------------------------------------------------------------------------
int get_level(int x, int y)
{
  int fan;
  int z0, z1, z2, z3;         // Height of each fan corner
  int zleft, zright,zdone;    // Weighted height of each side

  zdone = 0;
  fan = get_fan(x>>7, y>>7);
  if(fan != -1)
  {
    x = x&127;
    y = y&127;
    z0 = mesh.vrtz[mesh.vrtstart[fan]+0];
    z1 = mesh.vrtz[mesh.vrtstart[fan]+1];
    z2 = mesh.vrtz[mesh.vrtstart[fan]+2];
    z3 = mesh.vrtz[mesh.vrtstart[fan]+3];

    zleft = (z0*(128-y)+z3*y)>>7;
    zright = (z1*(128-y)+z2*y)>>7;
    zdone = (zleft*(128-x)+zright*x)>>7;
  }

  return (zdone);
}

//------------------------------------------------------------------------------
void make_hitemap(void)
{
  int x, y, pixx, pixy, level, fan;

  if(bmphitemap) SDL_FreeSurface(bmphitemap);

  bmphitemap = cartman_CreateSurface(mesh.sizex<<2, mesh.sizey<<2);
  if(NULL == bmphitemap) return;

  y = 16;
  pixy = 0;
  while(pixy < (mesh.sizey<<2))
  {
    x = 16;
    pixx = 0;
    while(pixx < (mesh.sizex<<2))
    {
      level=(get_level(x, y)*255/mesh.edgez);  // level is 0 to 255
      if(level > 252) level = 252;
      fan = get_fan(pixx>>2, pixy);
      level = 255;
      if(fan != -1)
      {
        if(mesh.fx[fan]&16) level = 253;         // Wall
        if(mesh.fx[fan]&32) level = 254;         // Impass
        if((mesh.fx[fan]&48)==48) level = 255;   // Both
      }
      SDL_PutPixel(bmphitemap, pixx, pixy, level);
      x+=32;
      pixx++;
    }
    y+=32;
    pixy++;
  }
}

//------------------------------------------------------------------------------
SDL_Surface *tiny_tile_at(int x, int y)
{
  Uint16 tile, basetile;
  Uint8 type, fx;
  int fan;

  if(x < 0 || x >= mesh.sizex || y < 0 || y >= mesh.sizey)
  {
    return bmpfanoff;
  }

  tile = 0;
  type = 0;
  fx = 0;
  fan = get_fan(x,y);
  if(fan != -1)
  {
    if(mesh.tile[fan]==FANOFF)
    {
      return bmpfanoff;
    }
    tile = mesh.tile[fan];
    type = mesh.type[fan];
    fx = mesh.fx[fan];
  }

  if(fx&MPDFX_ANIM)
  {
    animtileframeadd = (timclock>>3)&3;
    if(type >= (MAXMESHTYPE>>1))
    {
      // Big tiles
      basetile = tile&biganimtilebaseand;// Animation set
      tile+=(animtileframeadd<<1);         // Animated tile
      tile = (tile&biganimtileframeand)+basetile;
    }
    else
    {
      // Small tiles
      basetile = tile&animtilebaseand;// Animation set
      tile+=animtileframeadd;         // Animated tile
      tile = (tile&animtileframeand)+basetile;
    }
  }
  if(type >= (MAXMESHTYPE>>1))
  {
    return bmptinybigtile[tile];
  }
  return bmptinysmalltile[tile];
}

//------------------------------------------------------------------------------
void make_planmap(void)
{
  int x, y, putx, puty;
  SDL_Surface* bmptemp;

  bmptemp = cartman_CreateSurface(64, 64);
  if(NULL != bmptemp)  return;

  if(NULL == bmphitemap) SDL_FreeSurface(bmphitemap);
  bmphitemap = cartman_CreateSurface(mesh.sizex*TINYX, mesh.sizey*TINYY);
  if(NULL == bmphitemap) return;

  SDL_FillRect(bmphitemap, NULL, MAKE_BGR(bmphitemap,0,0,0));
  puty = 0;
  y = 0;
  while(y < mesh.sizey)
  {
    putx = 0;
    x = 0;
    while(x < mesh.sizex)
    {
      {
        SDL_Rect dst = {putx, puty, TINYX, TINYY};
        cartman_BlitSurface(tiny_tile_at(x, y), NULL, bmphitemap, &dst);
      }
      putx+=TINYX;
      x++;
    }
    puty+=TINYY;
    y++;
  }

  SDL_SoftStretch(bmphitemap, NULL, bmptemp, NULL);
  SDL_FreeSurface(bmphitemap);

  bmphitemap = bmptemp;
}

//------------------------------------------------------------------------------
void draw_cursor_in_window(SDL_Surface *window_surface, int win)
{
  int x, y;

  if(mdata.which_win!=-1)
  {
    if(window_lst[win].on && win != mdata.which_win)
    {
      if((window_lst[mdata.which_win].mode&WINMODE_SIDE) == (window_lst[win].mode&WINMODE_SIDE))
      {
        x = mos.x-window_lst[mdata.which_win].x-window_lst[mdata.which_win].borderx;
        y = mos.y-window_lst[mdata.which_win].y-window_lst[mdata.which_win].bordery;
        draw_sprite(window_surface, imgpointon[10], x-4, y-4);
      }
    }
  }
}

//------------------------------------------------------------------------------
int get_vertex(int x, int y, int num)
{
  // ZZ> This function gets a vertex number or -1
  int vert, cnt;
  int fan;

  vert = -1;
  fan = get_fan(x,y);
  if(fan != -1)
  {
    if(mesh.command[mesh.type[fan]].numvertices>num)
    {
      vert = mesh.vrtstart[fan];
      cnt = 0;
      while(cnt < num)
      {
        vert = mesh.vrtnext[vert];
        if(vert==-1)
        {
          printf("BAD GET_VERTEX NUMBER(2nd), %d at %d, %d...\n", num, x, y);
          printf("%d VERTICES ALLOWED...\n\n", mesh.command[mesh.type[fan]].numvertices);
          exit(-1);
        }
        cnt++;
      }
    }
  }

  return vert;
}

//------------------------------------------------------------------------------
int nearest_vertex(int x, int y, float nearx, float neary)
{
  // ZZ> This function gets a vertex number or -1
  int vert, bestvert, cnt;
  int fan;
  int num;
  float prox, proxx, proxy, bestprox;

  bestvert = -1;
  fan = get_fan(x,y);
  if(fan != -1)
  {
    num = mesh.command[mesh.type[fan]].numvertices;
    vert = mesh.vrtstart[fan];
    vert = mesh.vrtnext[vert];
    vert = mesh.vrtnext[vert];
    vert = mesh.vrtnext[vert];
    vert = mesh.vrtnext[vert];
    bestprox = 9000;
    cnt = 4;
    while(cnt < num)
    {
      proxx = mesh.command[mesh.type[fan]].x[cnt]-nearx;
      proxy = mesh.command[mesh.type[fan]].y[cnt]-neary;
      if(proxx < 0) proxx=-proxx;
      if(proxy < 0) proxy=-proxy;
      prox = proxx+proxy;
      if(prox < bestprox)
      {
        bestvert = vert;
        bestprox = prox;
      }
      vert = mesh.vrtnext[vert];
      cnt++;
    }
  }
  return bestvert;
}

//------------------------------------------------------------------------------
void weld_select()
{
  // ZZ> This function welds the highlighted vertices
  int cnt, x, y, z, a;
  Uint32 vert;

  if(numselect_verts > 1)
  {
    x = 0;
    y = 0;
    z = 0;
    a = 0;
    cnt = 0;
    while(cnt < numselect_verts)
    {
      vert = select_verts[cnt];
      x+=mesh.vrtx[vert];
      y+=mesh.vrty[vert];
      z+=mesh.vrtz[vert];
      a+=mesh.vrta[vert];
      cnt++;
    }
    x+=cnt>>1;  y+=cnt>>1;
    x=x/numselect_verts;
    y=y/numselect_verts;
    z=z/numselect_verts;
    a=a/numselect_verts;
    cnt = 0;
    while(cnt < numselect_verts)
    {
      vert = select_verts[cnt];
      mesh.vrtx[vert]=x;
      mesh.vrty[vert]=y;
      mesh.vrtz[vert]=z;
      mesh.vrta[vert]=a;
      cnt++;
    }
  }
}

//------------------------------------------------------------------------------
void add_select(int vert)
{
  // ZZ> This function highlights a vertex
  int cnt, found;

  if(numselect_verts < MAXSELECT && vert >= 0)
  {
    found = bfalse;
    cnt = 0;
    while(cnt < numselect_verts && !found)
    {
      if(select_verts[cnt]==vert)
      {
        found=btrue;
      }
      cnt++;
    }
    if(!found)
    {
      select_verts[numselect_verts] = vert;
      numselect_verts++;
    }
  }
}

//------------------------------------------------------------------------------
void clear_select(void)
{
  // ZZ> This function unselects all vertices
  numselect_verts = 0;
}

//------------------------------------------------------------------------------
int vert_selected(int vert)
{
  // ZZ> This function returns btrue if the vertex has been highlighted by user
  int cnt;

  cnt = 0;
  while(cnt < numselect_verts)
  {
    if(vert==select_verts[cnt])
    {
      return btrue;
    }
    cnt++;
  }

  return bfalse;
}

//------------------------------------------------------------------------------
void remove_select(int vert)
{
  // ZZ> This function makes sure the vertex is not highlighted
  int cnt, stillgoing;

  cnt = 0;
  stillgoing = btrue;
  while(cnt < numselect_verts && stillgoing)
  {
    if(vert==select_verts[cnt])
    {
      stillgoing = bfalse;
    }
    cnt++;
  }
  if(stillgoing == bfalse)
  {
    while(cnt < numselect_verts)
    {
      select_verts[cnt-1] = select_verts[cnt];
      cnt++;
    }
    numselect_verts--;
  }
}

//------------------------------------------------------------------------------
void fan_onscreen(Uint32 fan)
{
  // ZZ> This function flags a fan's points as being "onscreen"
  int cnt;
  Uint32 vert;

  vert = mesh.vrtstart[fan];
  cnt = 0;
  while(cnt < mesh.command[mesh.type[fan]].numvertices)
  {
    pointsonscreen[numpointsonscreen] = vert;  numpointsonscreen++;
    vert = mesh.vrtnext[vert];
    cnt++;
  }
}

//------------------------------------------------------------------------------
void make_onscreen(void)
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  int fan;

  numpointsonscreen = 0;
  mapxstt = (cam.x-(ONSIZE>>1))/31;
  mapystt = (cam.y-(ONSIZE>>1))/31;
  numx = (ONSIZE/SMALLX)+3;
  numy = (ONSIZE/SMALLY)+3;
  x = -cam.x+(ONSIZE>>1)-SMALLX;
  y = -cam.y+(ONSIZE>>1)-SMALLY;

  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    if(mapy>=0 && mapy<mesh.sizey)
    {
      mapx = mapxstt;
      cntx = 0;
      while(cntx < numx)
      {
        fan = get_fan(mapx, mapy);
        if(fan != -1)
        {
          fan_onscreen(fan);
        }
        mapx++;
        cntx++;
      }
    }
    mapy++;
    cnty++;
  }
}

//------------------------------------------------------------------------------
void draw_top_fan(SDL_Surface *bmp, int fan, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     A wireframe tile from a vertex connection window
  Uint32 faketoreal[MAXMESHVERTICES];
  int fantype;
  int cnt, stt, end, vert;
  Uint32 color;
  int size;

  fantype = mesh.type[fan];
  color = MAKE_BGR(bmp,16,16,31);
  if(fantype>=MAXMESHTYPE/2)
  {
    color = MAKE_BGR(bmp,31,16,16);
  }

  vert = mesh.vrtstart[fan];
  cnt = 0;
  while(cnt < mesh.command[fantype].numvertices)
  {
    faketoreal[cnt] = vert;
    vert = mesh.vrtnext[vert];
    cnt++;
  }

  cnt = 0;
  while(cnt < mesh.numline[fantype])
  {
    stt = faketoreal[mesh.line[fantype].start[cnt]];
    end = faketoreal[mesh.line[fantype].end[cnt]];
    line(bmp, mesh.vrtx[stt]+x, mesh.vrty[stt]+y,
      mesh.vrtx[end]+x, mesh.vrty[end]+y,
      color);
    cnt++;
  }

  cnt = 0;
  while(cnt < mesh.command[fantype].numvertices)
  {
    vert = faketoreal[cnt];
    size = (mesh.vrtz[vert]<<4)/(mesh.edgez+1);
    if(size<0) size = 0;
    if(size>=MAXPOINTSIZE) size = 15;
    if(mesh.vrtz[vert] >= 0)
    {
      if(vert_selected(vert))
      {
        draw_sprite(bmp, imgpointon[size],
          mesh.vrtx[vert]+x-(imgpointon[size]->w >> 1),
          mesh.vrty[vert]+y-(imgpointon[size]->h >> 1));
      }
      else
      {
        draw_sprite(bmp, imgpoint[size],
          mesh.vrtx[vert]+x-(imgpointon[size]->w >> 1),
          mesh.vrty[vert]+y-(imgpointon[size]->h >> 1));
      }
    }
    cnt++;
  }
}

//------------------------------------------------------------------------------
void draw_side_fan(SDL_Surface *bmp, int fan, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     A wireframe tile from a vertex connection window ( Side view )
  Uint32 faketoreal[MAXMESHVERTICES];
  int fantype;
  int cnt, stt, end, vert;
  Uint32 color;
  int size;

  fantype = mesh.type[fan];
  color = MAKE_BGR(bmp,16,16,31);
  if(fantype>=MAXMESHTYPE/2)
  {
    color = MAKE_BGR(bmp,31,16,16);
  }

  vert = mesh.vrtstart[fan];
  cnt = 0;
  while(cnt < mesh.command[fantype].numvertices)
  {
    faketoreal[cnt] = vert;
    vert = mesh.vrtnext[vert];
    cnt++;
  }

  cnt = 0;
  while(cnt < mesh.numline[fantype])
  {
    stt = faketoreal[mesh.line[fantype].start[cnt]];
    end = faketoreal[mesh.line[fantype].end[cnt]];
    if(mesh.vrtz[stt] >= 0 && mesh.vrtz[end] >= 0)
    {
      line(bmp, mesh.vrtx[stt]+x, -(mesh.vrtz[stt]>>4)+y,
        mesh.vrtx[end]+x, -(mesh.vrtz[end]>>4)+y,
        color);
    }
    cnt++;
  }

  size = 5;
  cnt = 0;
  while(cnt < mesh.command[fantype].numvertices)
  {
    vert = faketoreal[cnt];
    if(mesh.vrtz[vert] >= 0)
    {
      if(vert_selected(vert))
      {
        draw_sprite(bmp, imgpointon[size],
          mesh.vrtx[vert]+x-(imgpointon[size]->w >> 1),
          -(mesh.vrtz[vert]>>4)+y-(imgpointon[size]->h >> 1));
      }
      else
      {
        draw_sprite(bmp, imgpoint[size],
          mesh.vrtx[vert]+x-(imgpoint[size]->w >> 1),
          -(mesh.vrtz[vert]>>4)+y-(imgpoint[size]->h >> 1) );
      }
    }
    cnt++;
  }
}

//------------------------------------------------------------------------------
void draw_schematic(SDL_Surface *bmp, int fantype, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     The wireframe on the left side of the theSurface.
  int cnt, stt, end;
  Uint32 color;

  color = MAKE_BGR(bmp,16,16,31);
  if(mdata.type>=MAXMESHTYPE/2)  color = MAKE_BGR(bmp,31,16,16);
  cnt = 0;
  while(cnt < mesh.numline[fantype])
  {
    stt = mesh.line[fantype].start[cnt];
    end = mesh.line[fantype].end[cnt];
    line(bmp, mesh.command[fantype].x[stt]+x, mesh.command[fantype].y[stt]+y,
      mesh.command[fantype].x[end]+x, mesh.command[fantype].y[end]+y,
      color);
    cnt++;
  }
}

//------------------------------------------------------------------------------
void add_line(int fantype, int start, int end)
{
  // ZZ> This function adds a line to the vertex schematic
  int cnt;

  // Make sure line isn't already in list
  cnt = 0;
  while(cnt < mesh.numline[fantype])
  {
    if((mesh.line[fantype].start[cnt]==start &&
      mesh.line[fantype].end[cnt]==end) ||
      (mesh.line[fantype].end[cnt]==start &&
      mesh.line[fantype].start[cnt]==end))
    {
      return;
    }
    cnt++;
  }

  // Add it in
  mesh.line[fantype].start[cnt]=start;
  mesh.line[fantype].end[cnt]=end;
  mesh.numline[fantype]++;
}

//------------------------------------------------------------------------------
void goto_colon(FILE* fileread)
{
  // ZZ> This function moves a file read pointer to the next colon
  char cTmp;

  fscanf(fileread, "%c", &cTmp);
  while(cTmp != ':')
  {
    fscanf(fileread, "%c", &cTmp);
  }
}

//------------------------------------------------------------------------------
void load_mesh_fans()
{
  // ZZ> This function loads fan types for the mesh...  Starting vertex
  //     positions and number of vertices
  int cnt, entry;
  int numfantype, fantype, vertices;
  int numcommand, command, command_size;
  int fancenter, ilast;
  int itmp;
  float ftmp;
  FILE* fileread;
  STRING fname;

  // Initialize all mesh types to 0
  entry = 0;
  while(entry < MAXMESHTYPE)
  {
    mesh.numline[entry] = 0;
    mesh.command[entry].numvertices = 0;
    entry++;
  }

  // Open the file and go to it
  sprintf( fname, "%s" SLASH_STR "basicdat" SLASH_STR "fans.txt", egoboo_path );
  fileread = fopen( fname, "r");
  if(NULL == fileread)
  {
    log_error("load_mesh_fans() - Cannot find fans.txt file\n");
  }

  if(fileread)
  {
    goto_colon(fileread);
    fscanf(fileread, "%d", &numfantype);
    fantype = 0;
    while(fantype < numfantype)
    {
      goto_colon(fileread);
      fscanf(fileread, "%d", &vertices);
      mesh.command[fantype].numvertices = vertices;
      mesh.command[fantype+MAXMESHTYPE/2].numvertices = vertices;  // DUPE
      cnt = 0;
      while(cnt < vertices)
      {
        goto_colon(fileread);
        fscanf(fileread, "%d", &itmp);
        mesh.command[fantype].ref[cnt] = itmp;
        mesh.command[fantype+MAXMESHTYPE/2].ref[cnt] = itmp;  // DUPE
        goto_colon(fileread);
        fscanf(fileread, "%f", &ftmp);
        //        mesh.command[fantype].x[cnt] = (ftmp*.75+.5*.25)*128;
        //        mesh.command.x[fantype+MAXMESHTYPE/2][cnt] = (ftmp*.75+.5*.25)*128;  // DUPE
        mesh.command[fantype].x[cnt] = (ftmp)*128;
        mesh.command[fantype+MAXMESHTYPE/2].x[cnt] = (ftmp)*128;  // DUPE
        goto_colon(fileread);
        fscanf(fileread, "%f", &ftmp);
        //        mesh.command[fantype].y[cnt] = (ftmp*.75+.5*.25)*128;
        //        mesh.command.y[fantype+MAXMESHTYPE/2][cnt] = (ftmp*.75+.5*.25)*128;  // DUPE
        mesh.command[fantype].y[cnt] = (ftmp)*128;
        mesh.command[fantype+MAXMESHTYPE/2].y[cnt] = (ftmp)*128;  // DUPE
        cnt++;
      }

      // Get the vertex connections
      goto_colon(fileread);
      fscanf(fileread, "%d", &numcommand);
      entry = 0;
      command = 0;
      while(command < numcommand)
      {
        goto_colon(fileread);
        fscanf(fileread, "%d", &command_size);
        goto_colon(fileread);
        fscanf(fileread, "%d", &fancenter);
        goto_colon(fileread);
        fscanf(fileread, "%d", &itmp);
        cnt = 2;
        while(cnt < command_size)
        {
          ilast=itmp;
          goto_colon(fileread);
          fscanf(fileread, "%d", &itmp);
          add_line(fantype, fancenter, itmp);
          add_line(fantype, fancenter, ilast);
          add_line(fantype, ilast, itmp);

          add_line(fantype+MAXMESHTYPE/2, fancenter, itmp);  // DUPE
          add_line(fantype+MAXMESHTYPE/2, fancenter, ilast);  // DUPE
          add_line(fantype+MAXMESHTYPE/2, ilast, itmp);  // DUPE
          entry++;
          cnt++;
        }
        command++;
      }
      fantype++;
    }
    fclose(fileread);
  }
}

//------------------------------------------------------------------------------
void free_vertices()
{
  // ZZ> This function sets all vertices to unused
  int cnt;

  cnt = 0;
  while(cnt < MAXTOTALMESHVERTICES)
  {
    mesh.vrta[cnt] = VERTEXUNUSED;
    cnt++;
  }
  atvertex = 0;
  numfreevertices=MAXTOTALMESHVERTICES;
}

//------------------------------------------------------------------------------
int get_free_vertex()
{
  // ZZ> This function returns btrue if it can find an unused vertex, and it
  // will set atvertex to that vertex index.  bfalse otherwise.
  int cnt;

  if(numfreevertices!=0)
  {
    cnt = 0;
    while(cnt < MAXTOTALMESHVERTICES && mesh.vrta[atvertex]!=VERTEXUNUSED)
    {
      atvertex++;
      if(atvertex == MAXTOTALMESHVERTICES)
      {
        atvertex = 0;
      }
      cnt++;
    }
    if(mesh.vrta[atvertex]==VERTEXUNUSED)
    {
      mesh.vrta[atvertex]=60;
      return btrue;
    }
  }
  return bfalse;
}

//------------------------------------------------------------------------------
void remove_fan(int fan)
{
  // ZZ> This function removes a fan's vertices from usage and sets the fan
  //     to not be drawn
  int cnt, vert;
  Uint32 numvert;

  numvert = mesh.command[mesh.type[fan]].numvertices;
  vert = mesh.vrtstart[fan];
  cnt = 0;
  while(cnt < numvert)
  {
    mesh.vrta[vert] = VERTEXUNUSED;
    numfreevertices++;
    vert = mesh.vrtnext[vert];
    cnt++;
  }
  mesh.type[fan] = 0;
  mesh.fx[fan] = MPDFX_SHA;
}

//------------------------------------------------------------------------------
int add_fan(int fan, int x, int y)
{
  // ZZ> This function allocates the vertices needed for a fan
  int cnt;
  int numvert;
  Uint32 vertex;
  Uint32 vertexlist[17];

  numvert = mesh.command[mesh.type[fan]].numvertices;
  if(numfreevertices >= numvert)
  {
    mesh.fx[fan]=MPDFX_SHA;
    cnt = 0;
    while(cnt < numvert)
    {
      if(get_free_vertex()==bfalse)
      {
        // Reset to unused
        numvert = cnt;
        cnt = 0;
        while(cnt < numvert)
        {
          mesh.vrta[vertexlist[cnt]]=60;
          cnt++;
        }
        return bfalse;
      }
      vertexlist[cnt] = atvertex;
      cnt++;
    }
    vertexlist[cnt] = CHAINEND;

    cnt = 0;
    while(cnt < numvert)
    {
      vertex = vertexlist[cnt];
      mesh.vrtx[vertex] = x+(mesh.command[mesh.type[fan]].x[cnt]>>2);
      mesh.vrty[vertex] = y+(mesh.command[mesh.type[fan]].y[cnt]>>2);
      mesh.vrtz[vertex] = 0;
      mesh.vrtnext[vertex] = vertexlist[cnt+1];
      cnt++;
    }
    mesh.vrtstart[fan] = vertexlist[0];
    numfreevertices-=numvert;
    return btrue;
  }
  return bfalse;
}

//------------------------------------------------------------------------------
void num_free_vertex()
{
  // ZZ> This function counts the unused vertices and sets numfreevertices
  int cnt, num;

  num = 0;
  cnt = 0;
  while(cnt < MAXTOTALMESHVERTICES)
  {
    if(mesh.vrta[cnt]==VERTEXUNUSED)
    {
      num++;
    }
    cnt++;
  }
  numfreevertices=num;
}

//------------------------------------------------------------------------------
void make_fanstart()
{
  // ZZ> This function builds a look up table to ease calculating the
  //     fan number given an x,y pair
  int cnt;

  cnt = 0;
  while(cnt < mesh.sizey)
  {
    mesh.fanstart[cnt] = mesh.sizex*cnt;
    cnt++;
  }
}

//------------------------------------------------------------------------------
SDL_Surface *tile_at(int x, int y)
{
  Uint16 tile, basetile;
  Uint8 type, fx;
  int fan;

  fan = get_fan(x,y);
  if(fan==-1 || mesh.tile[fan]==FANOFF)
  {
    return bmpfanoff;
  }

  tile = mesh.tile[fan];
  type = mesh.type[fan];
  fx = mesh.fx[fan];
  if(fx&MPDFX_ANIM)
  {
    animtileframeadd = (timclock>>3)&3;
    if(type >= (MAXMESHTYPE>>1))
    {
      // Big tiles
      basetile = tile&biganimtilebaseand;// Animation set
      tile+=(animtileframeadd<<1);         // Animated tile
      tile = (tile&biganimtileframeand)+basetile;
    }
    else
    {
      // Small tiles
      basetile = tile&animtilebaseand;// Animation set
      tile+=animtileframeadd;         // Animated tile
      tile = (tile&animtileframeand)+basetile;
    }
  }
  if(type >= (MAXMESHTYPE>>1))
  {
    return bmpbigtile[tile];
  }
  return bmpsmalltile[tile];
}

//------------------------------------------------------------------------------
int fan_at(int x, int y)
{
  return get_fan(x,y);
}

//------------------------------------------------------------------------------
void weld_0(int x, int y)
{
  clear_select();
  add_select(get_vertex(x, y, 0));
  add_select(get_vertex(x-1, y, 1));
  add_select(get_vertex(x, y-1, 3));
  add_select(get_vertex(x-1, y-1, 2));
  weld_select();
  clear_select();
}

//------------------------------------------------------------------------------
void weld_1(int x, int y)
{
  clear_select();
  add_select(get_vertex(x, y, 1));
  add_select(get_vertex(x+1, y, 0));
  add_select(get_vertex(x, y-1, 2));
  add_select(get_vertex(x+1, y-1, 3));
  weld_select();
  clear_select();
}

//------------------------------------------------------------------------------
void weld_2(int x, int y)
{
  clear_select();
  add_select(get_vertex(x, y, 2));
  add_select(get_vertex(x+1, y, 3));
  add_select(get_vertex(x, y+1, 1));
  add_select(get_vertex(x+1, y+1, 0));
  weld_select();
  clear_select();
}

//------------------------------------------------------------------------------
void weld_3(int x, int y)
{
  clear_select();
  add_select(get_vertex(x, y, 3));
  add_select(get_vertex(x-1, y, 2));
  add_select(get_vertex(x, y+1, 0));
  add_select(get_vertex(x-1, y+1, 1));
  weld_select();
  clear_select();
}

//------------------------------------------------------------------------------
void weld_cnt(int x, int y, int cnt, Uint32 fan)
{
  if(mesh.command[mesh.type[fan]].x[cnt] < NEARLOW+1 ||
    mesh.command[mesh.type[fan]].y[cnt] < NEARLOW+1 ||
    mesh.command[mesh.type[fan]].x[cnt] > NEARHI-1 ||
    mesh.command[mesh.type[fan]].y[cnt] > NEARHI-1)
  {
    clear_select();
    add_select(get_vertex(x, y, cnt));
    if(mesh.command[mesh.type[fan]].x[cnt] < NEARLOW+1)
      add_select(nearest_vertex(x-1, y, NEARHI, mesh.command[mesh.type[fan]].y[cnt]));
    if(mesh.command[mesh.type[fan]].y[cnt] < NEARLOW+1)
      add_select(nearest_vertex(x, y-1, mesh.command[mesh.type[fan]].x[cnt], NEARHI));
    if(mesh.command[mesh.type[fan]].x[cnt] > NEARHI-1)
      add_select(nearest_vertex(x+1, y, NEARLOW, mesh.command[mesh.type[fan]].y[cnt]));
    if(mesh.command[mesh.type[fan]].y[cnt] > NEARHI-1)
      add_select(nearest_vertex(x, y+1, mesh.command[mesh.type[fan]].x[cnt], NEARLOW));
    weld_select();
    clear_select();
  }
}

//------------------------------------------------------------------------------
void fix_corners(int x, int y)
{
  int fan;

  fan = get_fan(x,y);
  if(fan != -1)
  {
    weld_0(x, y);
    weld_1(x, y);
    weld_2(x, y);
    weld_3(x, y);
  }
}

//------------------------------------------------------------------------------
void fix_vertices(int x, int y)
{
  int fan;
  int cnt;

  fix_corners(x, y);
  fan = get_fan(x,y);
  if( fan != -1 )
  {
    cnt = 4;
    while(cnt < mesh.command[mesh.type[fan]].numvertices)
    {
      weld_cnt(x, y, cnt, fan);
      cnt++;
    }
  }
}

//------------------------------------------------------------------------------
void fix_mesh(void)
{
  // ZZ> This function corrects corners across entire mesh
  int x, y;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      //      fix_corners(x, y);
      fix_vertices(x, y);
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
char tile_is_different(int x, int y, Uint16 tileset,
                       Uint16 tileand)
{
  // ZZ> bfalse if of same set, btrue if different
  int fan;

  fan = get_fan(x,y);
  if( fan != -1 )
  {
    return bfalse;
  }
  else
  {
    if(tileand == 192)
    {
      if(mesh.tile[fan] >= 48) return bfalse;
    }
    if((mesh.tile[fan]&tileand) == tileset)
    {
      return bfalse;
    }
  }
  return btrue;
}

//------------------------------------------------------------------------------
Uint16 trim_code(int x, int y, Uint16 tileset)
{
  // ZZ> This function returns the standard tile set value thing...  For
  //     Trimming tops of walls and floors

  Uint16 code;

  if(tile_is_different(x, y-1, tileset, 240))
  {
    // Top
    code = 0;
    if(tile_is_different(x-1, y, tileset, 240))
    {
      // Left
      code = 8;
    }
    if(tile_is_different(x+1, y, tileset, 240))
    {
      // Right
      code = 9;
    }
    return code;
  }
  if(tile_is_different(x, y+1, tileset, 240))
  {
    // Bottom
    code = 1;
    if(tile_is_different(x-1, y, tileset, 240))
    {
      // Left
      code = 10;
    }
    if(tile_is_different(x+1, y, tileset, 240))
    {
      // Right
      code = 11;
    }
    return code;
  }
  if(tile_is_different(x-1, y, tileset, 240))
  {
    // Left
    code = 2;
    return code;
  }
  if(tile_is_different(x+1, y, tileset, 240))
  {
    // Right
    code = 3;
    return code;
  }

  if(tile_is_different(x+1, y+1, tileset, 240))
  {
    // Bottom Right
    code = 4;
    return code;
  }
  if(tile_is_different(x-1, y+1, tileset, 240))
  {
    // Bottom Left
    code = 5;
    return code;
  }
  if(tile_is_different(x+1, y-1, tileset, 240))
  {
    // Top Right
    code = 6;
    return code;
  }
  if(tile_is_different(x-1, y-1, tileset, 240))
  {
    // Top Left
    code = 7;
    return code;
  }

  code = 255;
  return code;
}

//------------------------------------------------------------------------------
Uint16 wall_code(int x, int y, Uint16 tileset)
{
  // ZZ> This function returns the standard tile set value thing...  For
  //     Trimming tops of walls and floors

  Uint16 code;

  if(tile_is_different(x, y-1, tileset, 192))
  {
    // Top
    code = (rand()&2) + 20;
    if(tile_is_different(x-1, y, tileset, 192))
    {
      // Left
      code = 48;
    }
    if(tile_is_different(x+1, y, tileset, 192))
    {
      // Right
      code = 50;
    }
    return code;
  }
  if(tile_is_different(x, y+1, tileset, 192))
  {
    // Bottom
    code = (rand()&2);
    if(tile_is_different(x-1, y, tileset, 192))
    {
      // Left
      code = 52;
    }
    if(tile_is_different(x+1, y, tileset, 192))
    {
      // Right
      code = 54;
    }
    return code;
  }
  if(tile_is_different(x-1, y, tileset, 192))
  {
    // Left
    code = (rand()&2) + 16;
    return code;
  }
  if(tile_is_different(x+1, y, tileset, 192))
  {
    // Right
    code = (rand()&2) + 4;
    return code;
  }

  if(tile_is_different(x+1, y+1, tileset, 192))
  {
    // Bottom Right
    code = 32;
    return code;
  }
  if(tile_is_different(x-1, y+1, tileset, 192))
  {
    // Bottom Left
    code = 34;
    return code;
  }
  if(tile_is_different(x+1, y-1, tileset, 192))
  {
    // Top Right
    code = 36;
    return code;
  }
  if(tile_is_different(x-1, y-1, tileset, 192))
  {
    // Top Left
    code = 38;
    return code;
  }

  code = 255;
  return code;
}

//------------------------------------------------------------------------------
void trim_mesh_tile(Uint16 tileset, Uint16 tileand)
{
  // ZZ> This function trims walls and floors and tops automagically
  int fan;
  int x, y, code;

  tileset = tileset&tileand;
  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if(fan != -1 && (mesh.tile[fan]&tileand) == tileset)
      {
        if(tileand == 192)
        {
          code = wall_code(x, y, tileset);
        }
        else
        {
          code = trim_code(x, y, tileset);
        }
        if(code != 255)
        {
          mesh.tile[fan] = tileset + code;
        }
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void fx_mesh_tile(Uint16 tileset, Uint16 tileand, Uint8 fx)
{
  // ZZ> This function sets the fx for a group of tiles
  int fan;
  int x, y;

  tileset = tileset&tileand;
  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if(fan != -1 && (mesh.tile[fan]&tileand) == tileset)
      {
        mesh.fx[fan] = fx;
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void set_mesh_tile(Uint16 tiletoset)
{
  // ZZ> This function sets one tile type to another
  int fan;
  int x, y;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if(fan != -1 && mesh.tile[fan] == tiletoset)
      {
        switch(mdata.presser)
        {
        case 0:
          mesh.tile[fan]=mdata.tile;
          break;
        case 1:
          mesh.tile[fan]=(mdata.tile&0xfffe)+(rand()&1);
          break;
        case 2:
          mesh.tile[fan]=(mdata.tile&0xfffc)+(rand()&3);
          break;
        case 3:
          mesh.tile[fan]=(mdata.tile&0xfff0)+(rand()&6);
          break;
        }
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void create_mesh(void)
{
  // ZZ> This function makes the mesh
  int x, y, fan, tile;

  free_vertices();
  printf("Mesh file not found, so creating a new one...\n");
  printf("Number of tiles in X direction ( 32-512 ):  ");
  scanf("%d", &mesh.sizex);
  printf("Number of tiles in Y direction ( 32-512 ):  ");
  scanf("%d", &mesh.sizey);
  mesh.edgex = (mesh.sizex*SMALLX)-1;
  mesh.edgey = (mesh.sizey*SMALLY)-1;
  mesh.edgez = 180<<4;

  fan = 0;
  y = 0;
  tile = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      mesh.type[fan] = 2+0;
      mesh.tile[fan] = (((x&1)+(y&1))&1)+62;
      if(add_fan(fan, x*31, y*31)==bfalse)
      {
        printf("NOT ENOUGH VERTICES!!!\n\n");
        exit(-1);
      }
      fan++;
      x++;
    }
    y++;
  }

  make_fanstart();
  fix_mesh();
}

//------------------------------------------------------------------------------
void get_small_tiles(SDL_Surface* bmpload)
{
  int x, y, x1, y1;
  int sz_x = bmpload->w;
  int sz_y = bmpload->h;
  int step_x = sz_x >> 3;
  int step_y = sz_y >> 3;

  if(step_x == 0) step_x = 1;
  if(step_y == 0) step_y = 1;

  y1 = 0;
  y = 0;
  while(y < sz_y && y1 < 256)
  {
    x1 = 0;
    x = 0;
    while(x < sz_x && x1 < 256)
    {
      SDL_Rect src1 = { x, y, (step_x-1), (step_y-1) };

      bmpsmalltile[numsmalltile] = cartman_CreateSurface(SMALLX, SMALLY);
      SDL_FillRect(bmpsmalltile[numsmalltile], NULL, MAKE_BGR(bmpsmalltile[numsmalltile],0,0,0));

      bmptinysmalltile[numsmalltile] = cartman_CreateSurface(TINYX, TINYY);
      SDL_FillRect(bmptinysmalltile[numsmalltile], NULL, MAKE_BGR(bmptinysmalltile[numsmalltile],0,0,0));

      SDL_SoftStretch(bmpload, &src1, bmpsmalltile[numsmalltile], NULL);
      SDL_SoftStretch(bmpsmalltile[numsmalltile], NULL, bmptinysmalltile[numsmalltile], NULL );

      numsmalltile++;
      x+=step_x;
      x1 += 32;
    }
    y+=step_y;
    y1 += 32;
  }
}

//------------------------------------------------------------------------------
void get_big_tiles(SDL_Surface* bmpload)
{
  int x, y, x1, y1;
  int sz_x = bmpload->w;
  int sz_y = bmpload->h;
  int step_x = sz_x >> 3;
  int step_y = sz_y >> 3;

  if(step_x == 0) step_x = 1;
  if(step_y == 0) step_y = 1;

  y1 = 0;
  y = 0;
  while(y < sz_y)
  {
    x1 = 0;
    x = 0;
    while(x < sz_x)
    {
      int wid, hgt;

      SDL_Rect src1;

      bmpbigtile[numbigtile] = cartman_CreateSurface(SMALLX, SMALLY);
      SDL_FillRect(bmpbigtile[numbigtile], NULL, MAKE_BGR(bmpbigtile[numbigtile],0,0,0));

      bmptinybigtile[numbigtile] = cartman_CreateSurface(TINYX, TINYY);
      SDL_FillRect(bmptinybigtile[numbigtile], NULL, MAKE_BGR(bmptinybigtile[numbigtile],0,0,0));

      wid = (2*step_x-1);
      if(x + wid > bmpload->w) wid = bmpload->w - x;

      hgt = (2*step_y-1);
      if(y + hgt > bmpload->h) hgt = bmpload->h - y;

      src1.x = x;
      src1.y = y;
      src1.w = wid;
      src1.h = hgt;

      SDL_SoftStretch(bmpload, &src1, bmpbigtile[numbigtile], NULL);
      SDL_SoftStretch(bmpbigtile[numbigtile], NULL, bmptinybigtile[numbigtile], NULL);

      numbigtile++;
      x+=step_x;
      x1 += 32;
    }
    y+=step_y;
    y1 += 32;
  }
}

//------------------------------------------------------------------------------
void get_tiles(SDL_Surface* bmpload)
{
  get_small_tiles(bmpload);
  get_big_tiles(bmpload);
}

//------------------------------------------------------------------------------
void make_newloadname(char *modname, char *appendname, char *newloadname)
{
  // ZZ> This function takes some names and puts 'em together
  int cnt, tnc;
  char ctmp;

  cnt = 0;
  ctmp = modname[cnt];
  while(ctmp != 0)
  {
    newloadname[cnt] = ctmp;
    cnt++;
    ctmp = modname[cnt];
  }
  tnc = 0;
  ctmp = appendname[tnc];
  while(ctmp != 0)
  {
    newloadname[cnt] = ctmp;
    cnt++;
    tnc++;
    ctmp = appendname[tnc];
  }
  newloadname[cnt] = 0;
}

//------------------------------------------------------------------------------
void load_basic_textures(char *modname)
{
  // ZZ> This function loads the standard textures for a module
  char newloadname[256];
  SDL_Surface	*bmptemp;		// A temporary bitmap

  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "tile0.bmp", newloadname);
  bmptemp = cartman_LoadIMG(newloadname);
  get_tiles(bmptemp);
  SDL_FreeSurface(bmptemp);

  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "tile1.bmp", newloadname);
  bmptemp = cartman_LoadIMG(newloadname);
  get_tiles(bmptemp);
  SDL_FreeSurface(bmptemp);

  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "tile2.bmp", newloadname);
  bmptemp = cartman_LoadIMG(newloadname);
  get_tiles(bmptemp);
  SDL_FreeSurface(bmptemp);

  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "tile3.bmp", newloadname);
  bmptemp = cartman_LoadIMG(newloadname);
  get_tiles(bmptemp);
  SDL_FreeSurface(bmptemp);

  bmpfanoff = cartman_CreateSurface(SMALLX, SMALLY);
  SDL_FillRect(bmpfanoff, NULL, MAKE_BGR(bmpfanoff, 0, 0, 0));
}

//------------------------------------------------------------------------------
void show_name(char *newloadname)
{
  fnt_printf(theSurface, gFont, 0, ui.scr.y-16, MAKE_SDLCOLOR(31,31,31), newloadname);
}

//------------------------------------------------------------------------------
void make_twist()
{
  Uint32 fan, numfan;

  numfan = mesh.sizex*mesh.sizey;
  fan = 0;
  while(fan < numfan)
  {
    mesh.twist[fan]=get_fan_twist(fan);
    fan++;
  }
}

//------------------------------------------------------------------------------
int count_vertices()
{
  int fan, x, y, cnt, num, totalvert;
  Uint32 vert;

  totalvert = 0;
  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if( fan != -1 )
      {
        num = mesh.command[mesh.type[fan]].numvertices;
        vert = mesh.vrtstart[fan];
        cnt = 0;
        while(cnt < num)
        {
          totalvert++;
          vert = mesh.vrtnext[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
  return totalvert;
}

//------------------------------------------------------------------------------
void save_mesh(char *modname)
{
#define SAVE numwritten+=fwrite(&itmp, 4, 1, filewrite); numattempt++
#define SAVEF numwritten+=fwrite(&ftmp, 4, 1, filewrite); numattempt++
  FILE* filewrite;
  char newloadname[256];
  int itmp;
  float ftmp;
  int fan, x, y, cnt, num;
  Uint32 vert;
  Uint8 ctmp;

  numwritten = 0;
  numattempt = 0;
  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "plan.bmp", newloadname);
  make_planmap();
  if(bmphitemap)
  {
    //    save_pcx(newloadname, bmphitemap);
    SDL_SaveBMP(bmphitemap, newloadname);
  }

  //  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "level.pcx", newloadname);
  //  make_hitemap();
  //  if(bmphitemap)
  //  {
  //    make_graypal();
  //    save_pcx(newloadname, bmphitemap);
  //  }
  make_twist();

  sprintf( newloadname, "%s" SLASH_STR "modules" SLASH_STR "%s" SLASH_STR "gamedat" SLASH_STR "level.mpd", egoboo_path, modname );

  show_name(newloadname);
  filewrite = fopen(newloadname, "wb");
  if(filewrite)
  {
    itmp=MAPID;  SAVE;
    //    This didn't work for some reason...
    //    itmp=MAXTOTALMESHVERTICES-numfreevertices;  SAVE;
    itmp = count_vertices();  SAVE;
    itmp=mesh.sizex;  SAVE;
    itmp=mesh.sizey;  SAVE;

    // Write tile data
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if(fan != -1)
        {
          itmp = (mesh.type[fan]<<24)+(mesh.fx[fan]<<16)+mesh.tile[fan];  SAVE;
        }
        x++;
      }
      y++;
    }
    // Write twist data
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if(fan != -1)
        {
          ctmp = mesh.twist[fan];  numwritten+=fwrite(&ctmp, 1, 1, filewrite);
        }
        numattempt++;
        x++;
      }
      y++;
    }

    // Write x vertices
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if( fan != -1 )
        {
          num = mesh.command[mesh.type[fan]].numvertices;
          vert = mesh.vrtstart[fan];
          cnt = 0;
          while(cnt < num)
          {
            ftmp = mesh.vrtx[vert]*FIXNUM;  SAVEF;
            vert = mesh.vrtnext[vert];
            cnt++;
          }
        }
        x++;
      }
      y++;
    }

    // Write y vertices
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if( fan != -1)
        {
          num = mesh.command[mesh.type[fan]].numvertices;
          vert = mesh.vrtstart[fan];
          cnt = 0;
          while(cnt < num)
          {
            ftmp = mesh.vrty[vert]*FIXNUM;  SAVEF;
            vert = mesh.vrtnext[vert];
            cnt++;
          }
        }
        x++;
      }
      y++;
    }

    // Write z vertices
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if( fan != -1)
        {
          num = mesh.command[mesh.type[fan]].numvertices;
          vert = mesh.vrtstart[fan];
          cnt = 0;
          while(cnt < num)
          {
            ftmp = mesh.vrtz[vert]*FIXNUM;  SAVEF;
            vert = mesh.vrtnext[vert];
            cnt++;
          }
        }
        x++;
      }
      y++;
    }

    // Write a vertices
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if(fan != -1)
        {
          num = mesh.command[mesh.type[fan]].numvertices;
          vert = mesh.vrtstart[fan];
          cnt = 0;
          while(cnt < num)
          {
            ctmp = mesh.vrta[vert];  numwritten+=fwrite(&ctmp, 1, 1, filewrite);
            numattempt++;
            vert = mesh.vrtnext[vert];
            cnt++;
          }
        }
        x++;
      }
      y++;
    }
  }
}

//------------------------------------------------------------------------------
int load_mesh(char *modname)
{
  FILE* fileread;
  char  newloadname[256];
  Uint32  uiTmp32;
  int32   iTmp32;
  Uint8  uiTmp8;
  int num, cnt;
  float ftmp;
  int fan;
  Uint32 numvert, numfan;
  Uint32 vert;
  int x, y;

  sprintf( newloadname, "%s" SLASH_STR "gamedat" SLASH_STR "level.mpd", modname );

  fileread = fopen(newloadname, "rb");
  if(NULL == fileread)
  {
    log_warning("load_mesh() - Cannot find mesh for module \"%s\"\n", modname);
  }

  if(fileread)
  {
    free_vertices();

    fread( &uiTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32(uiTmp32); if ( uiTmp32 != MAPID ) return bfalse;
    fread( &uiTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32(iTmp32); numvert = uiTmp32;
    fread( &iTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32(iTmp32); mesh.sizex = iTmp32;
    fread( &iTmp32, 4, 1, fileread );  iTmp32 = ENDIAN_INT32(iTmp32); mesh.sizey = iTmp32;

    numfan = mesh.sizex*mesh.sizey;
    mesh.edgex = (mesh.sizex*SMALLX)-1;
    mesh.edgey = (mesh.sizey*SMALLY)-1;
    mesh.edgez = 180<<4;
    numfreevertices = MAXTOTALMESHVERTICES-numvert;

    // Load fan data
    fan = 0;
    while ( fan < numfan )
    {
      fread( &uiTmp32, 4, 1, fileread );
      uiTmp32 = ENDIAN_INT32(uiTmp32);

      mesh.type[fan] = (uiTmp32 >> 24) & 0x00FF;
      mesh.fx[fan]   = (uiTmp32 >> 16) & 0x00FF;
      mesh.tile[fan] = (uiTmp32 >>  0) & 0xFFFF;

      fan++;
    }

    // Load normal data
    // Load fan data
    fan = 0;
    while ( fan < numfan )
    {
      fread( &uiTmp8, 1, 1, fileread );
      mesh.twist[fan] = uiTmp8;

      fan++;
    }

    // Load vertex x data
    cnt = 0;
    while(cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread); ftmp = ENDIAN_FLOAT( ftmp );
      mesh.vrtx[cnt] = ftmp/FIXNUM;
      cnt++;
    }

    // Load vertex y data
    cnt = 0;
    while(cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread); ftmp = ENDIAN_FLOAT( ftmp );
      mesh.vrty[cnt] = ftmp/FIXNUM;
      cnt++;
    }

    // Load vertex z data
    cnt = 0;
    while(cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread); ftmp = ENDIAN_FLOAT( ftmp );
      mesh.vrtz[cnt] = ftmp/FIXNUM;
      cnt++;
    }

    // Load vertex a data
    cnt = 0;
    while(cnt < numvert)
    {
      fread(&uiTmp8, 1, 1, fileread);
      mesh.vrta[cnt] = uiTmp8;  // !!!BAD!!!
      cnt++;
    }

    make_fanstart();
    vert = 0;
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if( fan != -1)
        {
          int type = mesh.type[fan];
          if( type >= 0 && type < MAXMESHTYPE)
          {
            num = mesh.command[type].numvertices;
            mesh.vrtstart[fan] = vert;
            cnt = 0;
            while(cnt < num)
            {
              mesh.vrtnext[vert] = vert+1;
              vert++;
              cnt++;
            }
          }
          else
          {
            assert(0);
          }
        }
        mesh.vrtnext[vert-1] = CHAINEND;
        x++;
      }
      y++;
    }
    return btrue;
  }
  return bfalse;
}

//------------------------------------------------------------------------------
void move_select(int x, int y, int z)
{
  int vert, cnt, newx, newy, newz;

  cnt = 0;
  while(cnt < numselect_verts)
  {
    vert = select_verts[cnt];
    newx = mesh.vrtx[vert]+x;
    newy = mesh.vrty[vert]+y;
    newz = mesh.vrtz[vert]+z;
    if(newx<0)  x=0-mesh.vrtx[vert];
    if(newx>mesh.edgex) x=mesh.edgex-mesh.vrtx[vert];
    if(newy<0)  y=0-mesh.vrty[vert];
    if(newy>mesh.edgey) y=mesh.edgey-mesh.vrty[vert];
    if(newz<0)  z=0-mesh.vrtz[vert];
    if(newz>mesh.edgez) z=mesh.edgez-mesh.vrtz[vert];
    cnt++;
  }

  cnt = 0;
  while(cnt < numselect_verts)
  {
    vert = select_verts[cnt];
    newx = mesh.vrtx[vert]+x;
    newy = mesh.vrty[vert]+y;
    newz = mesh.vrtz[vert]+z;

    if(newx<0)  newx=0;
    if(newx>mesh.edgex)  newx=mesh.edgex;
    if(newy<0)  newy=0;
    if(newy>mesh.edgey)  newy=mesh.edgey;
    if(newz<0)  newz=0;
    if(newz>mesh.edgez)  newz=mesh.edgez;

    mesh.vrtx[vert]=newx;
    mesh.vrty[vert]=newy;
    mesh.vrtz[vert]=newz;
    cnt++;
  }
}

//------------------------------------------------------------------------------
void set_select_no_bound_z(int z)
{
  int vert, cnt;

  cnt = 0;
  while(cnt < numselect_verts)
  {
    vert = select_verts[cnt];
    mesh.vrtz[vert]=z;
    cnt++;
  }
}

//------------------------------------------------------------------------------
void move_mesh_z(int z, Uint16 tiletype, Uint16 tileand)
{
  int vert, cnt, newz, x, y, totalvert;
  int fan;

  tiletype = tiletype & tileand;
  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if(fan != -1)
      {
        if((mesh.tile[fan]&tileand) == tiletype)
        {
          vert = mesh.vrtstart[fan];
          totalvert = mesh.command[mesh.type[fan]].numvertices;
          cnt = 0;
          while(cnt < totalvert)
          {
            newz = mesh.vrtz[vert]+z;
            if(newz<0)  newz=0;
            if(newz>mesh.edgez) newz=mesh.edgez;
            mesh.vrtz[vert] = newz;
            vert = mesh.vrtnext[vert];
            cnt++;
          }
        }
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void move_vert(int vert, int x, int y, int z)
{
  int newx, newy, newz;

  newx = mesh.vrtx[vert]+x;
  newy = mesh.vrty[vert]+y;
  newz = mesh.vrtz[vert]+z;

  if(newx<0)  newx=0;
  if(newx>mesh.edgex)  newx=mesh.edgex;
  if(newy<0)  newy=0;
  if(newy>mesh.edgey)  newy=mesh.edgey;
  if(newz<0)  newz=0;
  if(newz>mesh.edgez)  newz=mesh.edgez;

  mesh.vrtx[vert]=newx;
  mesh.vrty[vert]=newy;
  mesh.vrtz[vert]=newz;
}

//------------------------------------------------------------------------------
void raise_mesh(int x, int y, int amount, int size)
{
  int disx, disy, dis, cnt, newamount;
  Uint32 vert;

  cnt = 0;
  while(cnt < numpointsonscreen)
  {
    vert = pointsonscreen[cnt];
    disx = mesh.vrtx[vert]-(x/FOURNUM);
    disy = mesh.vrty[vert]-(y/FOURNUM);
    dis = sqrt(disx*disx+disy*disy);

    newamount = abs(amount)-((dis<<1)>>size);
    if(newamount < 0) newamount = 0;
    if(amount < 0)  newamount = -newamount;
    move_vert(vert, 0, 0, newamount);

    cnt++;
  }
}

//------------------------------------------------------------------------------
void load_module(char *modname)
{
  char newloadname[256];

  sprintf( newloadname, "%s" SLASH_STR "modules" SLASH_STR "%s", egoboo_path, modname);

  //  show_name(newloadname);
  load_basic_textures(newloadname);
  if(!load_mesh(newloadname))
  {
    create_mesh();
  }
  numlight = 0;
  addinglight = 0;
}

//------------------------------------------------------------------------------
void render_tile_window(int window, SDL_Surface *window_surface)
{
  SDL_Surface *bmptile;
  int x, y, xstt, ystt, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  int cnt;

  mapxstt = (cam.x-(window_lst[window].surfacex>>1))/31;
  mapystt = (cam.y-(window_lst[window].surfacey>>1))/31;
  numx = (window_lst[window].surfacex/SMALLX)+3;
  numy = (window_lst[window].surfacey/SMALLY)+3;
  xstt = -((cam.x-(window_lst[window].surfacex>>1))%31)-(SMALLX);
  ystt = -((cam.y-(window_lst[window].surfacey>>1))%31)-(SMALLY);

  y = ystt;
  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    x = xstt;
    mapx = mapxstt;
    cntx = 0;
    while(cntx < numx)
    {
      bmptile = tile_at(mapx, mapy);
      {
        SDL_Rect dst = {x, y, SMALLX, SMALLY};
        cartman_BlitSurface(bmptile, NULL, window_surface, &dst);
      }
      mapx++;
      cntx++;
      x+=31;
    }
    mapy++;
    cnty++;
    y+=31;
  }

  cnt = 0;
  while(cnt < numlight)
  {
    draw_light(cnt, window_surface, window);
    cnt++;
  }
}

//------------------------------------------------------------------------------
void render_fx_window(int window, SDL_Surface *window_surface)
{
  SDL_Surface *bmptile;
  int x, y, xstt, ystt, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  int fan;

  mapxstt = (cam.x-(window_lst[window].surfacex>>1))/31;
  mapystt = (cam.y-(window_lst[window].surfacey>>1))/31;
  numx = (window_lst[window].surfacex/SMALLX)+3;
  numy = (window_lst[window].surfacey/SMALLY)+3;
  xstt = -((cam.x-(window_lst[window].surfacex>>1))%31)-(SMALLX);
  ystt = -((cam.y-(window_lst[window].surfacey>>1))%31)-(SMALLY);

  y = ystt;
  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    x = xstt;
    mapx = mapxstt;
    cntx = 0;
    while(cntx < numx)
    {
      bmptile = tile_at(mapx, mapy);
      {
        SDL_Rect dst = {x, y, SMALLX-1, SMALLY-1};
        cartman_BlitSurface(bmptile, NULL, window_surface, &dst);
      }
      fan = fan_at(mapx, mapy);
      if(fan!=-1)
      {
        if(!(mesh.fx[fan]&MPDFX_SHA))
          draw_sprite(window_surface, imgref, x, y);
        if(mesh.fx[fan]&MPDFX_DRAWREF)
          draw_sprite(window_surface, imgdrawref, x+16, y);
        if(mesh.fx[fan]&MPDFX_ANIM)
          draw_sprite(window_surface, imganim, x, y+16);
        if(mesh.fx[fan]&MPDFX_WALL)
          draw_sprite(window_surface, imgwall, x+15, y+15);
        if(mesh.fx[fan]&MPDFX_IMPASS)
          draw_sprite(window_surface, imgimpass, x+15+8, y+15);
        if(mesh.fx[fan]&MPDFX_DAMAGE)
          draw_sprite(window_surface, imgdamage, x+15, y+15+8);
        if(mesh.fx[fan]&MPDFX_SLIPPY)
          draw_sprite(window_surface, imgslippy, x+15+8, y+15+8);
        if(mesh.fx[fan]&MPDFX_WATER)
          draw_sprite(window_surface, imgwater, x, y);
      }
      mapx++;
      cntx++;
      x+=31;
    }
    mapy++;
    cnty++;
    y+=31;
  }
}

//------------------------------------------------------------------------------
void render_vertex_window(int window, SDL_Surface *window_surface)
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  int fan;

  //  numpointsonscreen = 0;
  mapxstt = (cam.x-(window_lst[window].surfacex>>1))/31;
  mapystt = (cam.y-(window_lst[window].surfacey>>1))/31;
  numx = (window_lst[window].surfacex/SMALLX)+3;
  numy = (window_lst[window].surfacey/SMALLY)+3;
  x = -cam.x+(window_lst[window].surfacey>>1)-SMALLX;
  y = -cam.y+(window_lst[window].surfacey>>1)-SMALLY;

  for(cnty = 0, mapy = mapystt; cnty < numy; cnty++, mapy++)
  {
    if(mapy<0 || mapy>=mesh.sizey) continue;

    for(cntx = 0, mapx = mapxstt; cntx < numx; cntx++, mapx++)
    {
      fan = get_fan(mapx, mapy);
      if( fan != -1 )
      {
        draw_top_fan(window_surface, fan, x, y);
      }
    }
  }

  if(mdata.rect && mdata.mode==WINMODE_VERTEX)
  {
    SDL_Rect rtmp;

    rtmp.x = (mdata.rectx/FOURNUM)+x;
    rtmp.y = (mdata.recty/FOURNUM)+y;
    rtmp.w = (mdata.x/FOURNUM) - (mdata.rectx/FOURNUM);
    rtmp.h = (mdata.y/FOURNUM) - (mdata.recty/FOURNUM);

    SDL_FillRect(window_surface, &rtmp, MAKE_ABGR(window_surface,0x3F, 16+(timclock&15), 16+(timclock&15), 0));
  }

  if((SDLKEYDOWN(SDLK_p) || ((mos.b&2) && numselect_verts == 0)) && mdata.mode==WINMODE_VERTEX)
  {
    raise_mesh(mdata.x, mdata.y, brushamount, brushsize);
  }
}

//------------------------------------------------------------------------------
void render_side_window(int window, SDL_Surface *window_surface)
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  int fan;

  mapxstt = (cam.x-(window_lst[window].surfacex>>1))/31;
  mapystt = (cam.y-(window_lst[window].surfacey>>1))/31;
  numx = (window_lst[window].surfacex/SMALLX)+3;
  numy = (window_lst[window].surfacey/SMALLY)+3;
  x = -cam.x+(window_lst[window].surfacey>>1)-SMALLX;
  y = (*window_surface).h-10;

  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    if(mapy>=0 && mapy<mesh.sizey)
    {
      mapx = mapxstt;
      cntx = 0;
      while(cntx < numx)
      {
        fan = get_fan(mapx, mapy);
        if(fan != -1)
        {
          draw_side_fan(window_surface, fan, x, y);
        }
        mapx++;
        cntx++;
      }
    }
    mapy++;
    cnty++;
  }

  if(mdata.rect && mdata.mode==WINMODE_SIDE)
  {
    SDL_Rect rtmp;
    rtmp.x = (mdata.rectx/FOURNUM)+x;
    rtmp.y = (mdata.recty/FOURNUM);
    rtmp.w = (mdata.x/FOURNUM) - (mdata.rectx/FOURNUM) + 1;
    rtmp.h = (mdata.recty/FOURNUM) - (mdata.y/FOURNUM) + 1;

    SDL_FillRect(window_surface, &rtmp, MAKE_ABGR(window_surface, 0x3F, 16+(timclock&15), 16+(timclock&15), 0));
  }
}

//------------------------------------------------------------------------------
void render_window(int window)
{
  SDL_Surface *window_surface = NULL;
  window_t * pwin;
  SDL_Rect rwin;
  SDL_Rect rtmp;

  if(window < 0 || window >= MAXWIN) return;

  pwin = window_lst + window;
  if(!pwin->on) return;

  make_onscreen();

  // save the old clipping rectangle
  SDL_GetClipRect(pwin->bmp, &rwin);

  // set a new clipping rectangle that masks out the borders
  rtmp.x = pwin->borderx;
  rtmp.y = pwin->bordery;
  rtmp.w = pwin->surfacex;
  rtmp.h = pwin->surfacey;
  SDL_SetClipRect(pwin->bmp, &rtmp);

  window_surface = pwin->bmp;

  if(pwin->mode&WINMODE_TILE)
  {
    render_tile_window(window, window_surface);
  }
  else
  {
    // Untiled bitmaps clear
    SDL_FillRect(window_surface, &(window_surface->clip_rect), MAKE_BGR(window_surface,0,0,0));
  }

  if(pwin->mode&WINMODE_FX)
  {
    render_fx_window(window, window_surface);
  }

  if(pwin->mode&WINMODE_VERTEX)
  {
    render_vertex_window(window, window_surface);
  }

  if(pwin->mode&WINMODE_SIDE)
  {
    render_side_window(window, window_surface);
  }

  draw_cursor_in_window(window_surface, window);

  // restore the old clipping rectangle
  SDL_SetClipRect(pwin->bmp, &rwin);
}

//------------------------------------------------------------------------------
void load_window(int window, char *loadname, int x, int y, int bx, int by,
                 int sx, int sy, Uint16 mode)
{
  window_lst[window].bmp = cartman_LoadIMG(loadname);
  window_lst[window].x = x;
  window_lst[window].y = y;
  window_lst[window].borderx = bx;
  window_lst[window].bordery = by;
  window_lst[window].surfacex = sx;
  window_lst[window].surfacey = sy;
  window_lst[window].on = btrue;
  window_lst[window].mode = mode;
}

//------------------------------------------------------------------------------
void render_all_windows(void)
{
  int cnt;

  for(cnt = 0; cnt < MAXWIN; cnt++)
  {
    render_window(cnt);
  }
}

//------------------------------------------------------------------------------
void load_all_windows(void)
{
  int cnt;

  cnt = 0;
  while(cnt < MAXWIN)
  {
    window_lst[cnt].on = bfalse;
    cnt++;
  }

  load_window(0, "window.pcx", 180, 16,  7, 9, 200, 200, WINMODE_VERTEX);
  load_window(1, "window.pcx", 410, 16,  7, 9, 200, 200, WINMODE_TILE);
  load_window(2, "window.pcx", 180, 248, 7, 9, 200, 200, WINMODE_SIDE);
  load_window(3, "window.pcx", 410, 248, 7, 9, 200, 200, WINMODE_FX);
}

//------------------------------------------------------------------------------
void draw_window(int window)
{
  if(window_lst[window].on)
  {
    SDL_Rect dst = {window_lst[window].x, window_lst[window].y, (*window_lst[window].bmp).w, (*window_lst[window].bmp).h};
    cartman_BlitScreen(window_lst[window].bmp, &dst);
  }
}

//------------------------------------------------------------------------------
void draw_all_windows(void)
{
  int cnt;
  cnt = 0;
  while(cnt < MAXWIN)
  {
    draw_window(cnt);
    cnt++;
  }
}

//------------------------------------------------------------------------------
void bound_camera(void)
{
  if(cam.x < 0)
  {
    cam.x = 0;
  }
  if(cam.y < 0)
  {
    cam.y = 0;
  }
  if(cam.x > mesh.sizex*SMALLX)
  {
    cam.x = mesh.sizex*SMALLX;
  }
  if(cam.y > mesh.sizey*SMALLY)
  {
    cam.y = mesh.sizey*SMALLY;
  }
}

//------------------------------------------------------------------------------
void unbound_mouse()
{
  mos.tlx = 0;
  mos.tly = 0;
  mos.brx = ui.scr.x-1;
  mos.bry = ui.scr.y-1;
}

//------------------------------------------------------------------------------
void bound_mouse()
{
  if(mdata.which_win != -1)
  {
    mos.tlx = window_lst[mdata.which_win].x + window_lst[mdata.which_win].borderx;
    mos.tly = window_lst[mdata.which_win].y + window_lst[mdata.which_win].bordery;
    mos.brx = mos.tlx + window_lst[mdata.which_win].surfacex-1;
    mos.bry = mos.tly + window_lst[mdata.which_win].surfacey-1;
  }
}

//------------------------------------------------------------------------------
void rect_select(void)
{
  // ZZ> This function checks the rectangular select_vertsion
  int cnt;
  Uint32 vert;
  int tlx, tly, brx, bry;
  int y;

  if(mdata.mode == WINMODE_VERTEX)
  {
    tlx = mdata.rectx/FOURNUM;
    brx = mdata.x/FOURNUM;
    tly = mdata.recty/FOURNUM;
    bry = mdata.y/FOURNUM;

    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }

    cnt = 0;
    while(cnt < numpointsonscreen && numselect_verts<MAXSELECT)
    {
      vert = pointsonscreen[cnt];
      if(mesh.vrtx[vert]>=tlx &&
        mesh.vrtx[vert]<=brx &&
        mesh.vrty[vert]>=tly &&
        mesh.vrty[vert]<=bry)
      {
        add_select(vert);
      }
      cnt++;
    }
  }
  if(mdata.mode == WINMODE_SIDE)
  {
    tlx = mdata.rectx/FOURNUM;
    brx = mdata.x/FOURNUM;
    tly = mdata.recty/FOURNUM;
    bry = mdata.y/FOURNUM;

    y = 190;//((*(window_lst[mdata.which_win].bmp)).h-10);

    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }

    cnt = 0;
    while(cnt < numpointsonscreen && numselect_verts<MAXSELECT)
    {
      vert = pointsonscreen[cnt];
      if(mesh.vrtx[vert]>=tlx &&
        mesh.vrtx[vert]<=brx &&
        -(mesh.vrtz[vert]>>4)+y>=tly &&
        -(mesh.vrtz[vert]>>4)+y<=bry)
      {
        add_select(vert);
      }
      cnt++;
    }
  }
}

//------------------------------------------------------------------------------
void rect_unselect(void)
{
  // ZZ> This function checks the rectangular select_vertsion, and removes any fans
  //     in the select_vertsion area
  int cnt;
  Uint32 vert;
  int tlx, tly, brx, bry;
  int y;

  if(mdata.mode == WINMODE_VERTEX)
  {
    tlx = mdata.rectx/FOURNUM;
    brx = mdata.x/FOURNUM;
    tly = mdata.recty/FOURNUM;
    bry = mdata.y/FOURNUM;

    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }

    cnt = 0;
    while(cnt < numpointsonscreen && numselect_verts<MAXSELECT)
    {
      vert = pointsonscreen[cnt];
      if(mesh.vrtx[vert]>=tlx &&
        mesh.vrtx[vert]<=brx &&
        mesh.vrty[vert]>=tly &&
        mesh.vrty[vert]<=bry)
      {
        remove_select(vert);
      }
      cnt++;
    }
  }
  if(mdata.mode == WINMODE_SIDE)
  {
    tlx = mdata.rectx/FOURNUM;
    brx = mdata.x/FOURNUM;
    tly = mdata.recty/FOURNUM;
    bry = mdata.y/FOURNUM;

    y = 190;//((*(window_lst[mdata.which_win].bmp)).h-10);

    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }

    cnt = 0;
    while(cnt < numpointsonscreen && numselect_verts<MAXSELECT)
    {
      vert = pointsonscreen[cnt];
      if(mesh.vrtx[vert]>=tlx &&
        mesh.vrtx[vert]<=brx &&
        -(mesh.vrtz[vert]>>4)+y>=tly &&
        -(mesh.vrtz[vert]>>4)+y<=bry)
      {
        remove_select(vert);
      }
      cnt++;
    }
  }
}

//------------------------------------------------------------------------------
int set_vrta(Uint32 vert)
{
  int newa, x, y, z, brx, bry, brz, deltaz, dist, cnt;
  int newlevel, distance, disx, disy;

  // To make life easier
  x = mesh.vrtx[vert]*FOURNUM;
  y = mesh.vrty[vert]*FOURNUM;
  z = mesh.vrtz[vert];

  // Directional light
  brx = x+64;
  bry = y+64;
  brz = get_level(brx, y) +
    get_level(x, bry) +
    get_level(x+46, y+46);
  if(z < -128) z = -128;
  if(brz < -128) brz = -128;
  deltaz = z+z+z-brz;
  newa = (deltaz*direct>>8);

  // Point lights !!!BAD!!!
  newlevel = 0;
  cnt = 0;
  while(cnt < numlight)
  {
    disx = x-light_lst[cnt].x;
    disy = y-light_lst[cnt].y;
    distance = sqrt(disx*disx + disy*disy);
    if(distance < light_lst[cnt].radius)
    {
      newlevel += ((light_lst[cnt].level*(light_lst[cnt].radius-distance))/light_lst[cnt].radius);
    }
    cnt++;
  }
  newa += newlevel;

  // Bounds
  if(newa < -ambicut) newa = -ambicut;
  newa+=ambi;
  if(newa <= 0) newa = 1;
  if(newa > 255) newa = 255;
  mesh.vrta[vert]=newa;

  // Edge fade
  dist = dist_from_border(mesh.vrtx[vert], mesh.vrty[vert]);
  if(dist <= FADEBORDER)
  {
    newa = newa*dist/FADEBORDER;
    if(newa==VERTEXUNUSED)  newa=1;
    mesh.vrta[vert]=newa;
  }

  return newa;
}

//------------------------------------------------------------------------------
void calc_vrta()
{
  int x, y, fan, num, cnt;
  Uint32 vert;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if( fan != -1)
      {
        vert = mesh.vrtstart[fan];
        num = mesh.command[mesh.type[fan]].numvertices;
        cnt = 0;
        while(cnt < num)
        {
          set_vrta(vert);
          vert = mesh.vrtnext[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void level_vrtz()
{
  int x, y, fan, num, cnt;
  Uint32 vert;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if( fan != -1)
      {
        vert = mesh.vrtstart[fan];
        num = mesh.command[mesh.type[fan]].numvertices;
        cnt = 0;
        while(cnt < num)
        {
          mesh.vrtz[vert] = 0;
          vert = mesh.vrtnext[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void jitter_select()
{
  int cnt;
  Uint32 vert;

  cnt = 0;
  while(cnt < numselect_verts)
  {
    vert = select_verts[cnt];
    move_vert(vert, (rand()%3)-1, (rand()%3)-1, 0);
    cnt++;
  }
}

//------------------------------------------------------------------------------
void jitter_mesh()
{
  int x, y, fan, num, cnt;
  Uint32 vert;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if(fan != -1)
      {
        vert = mesh.vrtstart[fan];
        num = mesh.command[mesh.type[fan]].numvertices;
        cnt = 0;
        while(cnt < num)
        {
          clear_select();
          add_select(vert);
          //        srand(mesh.vrtx[vert]+mesh.vrty[vert]+dunframe);
          move_select((rand()&7)-3,(rand()&7)-3,(rand()&63)-32);
          vert = mesh.vrtnext[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
  clear_select();
}

//------------------------------------------------------------------------------
void flatten_mesh()
{
  int x, y, fan, num, cnt;
  Uint32 vert;
  int height;

  height = (780 - (mdata.y)) * 4;
  if(height < 0)  height = 0;
  if(height > mesh.edgez) height = mesh.edgez;
  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if(fan != -1)
      {
        vert = mesh.vrtstart[fan];
        num = mesh.command[mesh.type[fan]].numvertices;
        cnt = 0;
        while(cnt < num)
        {
          if(mesh.vrtz[vert] > height - 50)
            if(mesh.vrtz[vert] < height + 50)
              mesh.vrtz[vert] = height;
          vert = mesh.vrtnext[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
  clear_select();
}

//------------------------------------------------------------------------------
void move_camera()
{
  if(((mos.b&4) || SDLKEYDOWN(SDLK_m)) && mdata.which_win!=-1)
  {
    cam.x+=mos.cx;
    cam.y+=mos.cy;
    bound_camera();
    mos.x=mos.x_old;
    mos.y=mos.y_old;
  }
}

//------------------------------------------------------------------------------
void mouse_side(int cnt)
{
  mdata.x = mos.x - window_lst[cnt].x-window_lst[cnt].borderx+cam.x-69;
  mdata.y = mos.y-window_lst[cnt].y-window_lst[cnt].bordery;
  mdata.x = mdata.x*FOURNUM;
  mdata.y = mdata.y*FOURNUM;
  if(SDLKEYDOWN(SDLK_f))
  {
    flatten_mesh();
  }
  if(mos.b&1)
  {
    if(mdata.rect==bfalse)
    {
      mdata.rect=btrue;
      mdata.rectx=mdata.x;
      mdata.recty=mdata.y;
    }
  }
  else
  {
    if(mdata.rect==btrue)
    {
      if(numselect_verts!=0 && !SDLKEYMOD(KMOD_ALT) && !SDLKEYDOWN(SDLK_MODE) &&
        !SDLKEYMOD(KMOD_LCTRL) && !SDLKEYMOD(KMOD_RCTRL))
      {
        clear_select();
      }
      if( SDLKEYMOD(KMOD_ALT) || SDLKEYDOWN(SDLK_MODE))
      {
        rect_unselect();
      }
      else
      {
        rect_select();
      }
      mdata.rect = bfalse;
    }
  }
  if(mos.b&2)
  {
    move_select(mos.cx, 0, -(mos.cy<<4));
    bound_mouse();
  }
  if(SDLKEYDOWN(SDLK_y))
  {
    move_select(0, 0, -(mos.cy<<4));
    bound_mouse();
  }
  if(SDLKEYDOWN(SDLK_5))
  {
    set_select_no_bound_z(-8000<<2);
  }
  if(SDLKEYDOWN(SDLK_6))
  {
    set_select_no_bound_z(-127<<2);
  }
  if(SDLKEYDOWN(SDLK_7))
  {
    set_select_no_bound_z(127<<2);
  }
  if(SDLKEYDOWN(SDLK_u))
  {
    if(mdata.type >= (MAXMESHTYPE>>1))
    {
      move_mesh_z(-(mos.cy<<4), mdata.tile, 192);
    }
    else
    {
      move_mesh_z(-(mos.cy<<4), mdata.tile, 240);
    }
    bound_mouse();
  }
  if(SDLKEYDOWN(SDLK_n))
  {
    if(SDLKEYDOWN(SDLK_RSHIFT))
    {
      // Move the first 16 up and down
      move_mesh_z(-(mos.cy<<4), 0, 240);
    }
    else
    {
      // Move the entire mesh up and down
      move_mesh_z(-(mos.cy<<4), 0, 0);
    }
    bound_mouse();
  }
  if(SDLKEYDOWN(SDLK_q))
  {
    fix_walls();
  }
}

//------------------------------------------------------------------------------
void mouse_tile(int cnt)
{
  int x, y, keyt, vert, keyv;
  float tl, tr, bl, br;

  tl = tr = bl = br = 0.0f;
  mdata.x = mos.x - window_lst[cnt].x-window_lst[cnt].borderx+cam.x-69;
  mdata.y = mos.y - window_lst[cnt].y-window_lst[cnt].bordery+cam.y-69;
  if(mdata.x < 0 ||
    mdata.x >= SMALLX*mesh.sizex ||
    mdata.y < 0 ||
    mdata.y >= SMALLY*mesh.sizey)
  {
    mdata.x = mdata.x*FOURNUM;
    mdata.y = mdata.y*FOURNUM;
    if(mos.b&2)
    {
      mdata.type = 0+0;
      mdata.tile = 0xffff;
    }
  }
  else
  {
    mdata.x = mdata.x*FOURNUM;
    mdata.y = mdata.y*FOURNUM;
    if(mdata.x >= (mesh.sizex<<7))  mdata.x = (mesh.sizex<<7)-1;
    if(mdata.y >= (mesh.sizey<<7))  mdata.y = (mesh.sizey<<7)-1;
    debugx = mdata.x/128.0;
    debugy = mdata.y/128.0;
    x = mdata.x>>7;
    y = mdata.y>>7;
    mdata.onfan = get_fan(x,y);
    if(mdata.onfan == -1) mdata.onfan = 0;

    if(!SDLKEYDOWN(SDLK_k))
    {
      addinglight = bfalse;
    }
    if(SDLKEYDOWN(SDLK_k)&&addinglight==bfalse)
    {
      add_light(mdata.x, mdata.y, MINRADIUS, MAXLEVEL);
      addinglight = btrue;
    }
    if(addinglight)
    {
      alter_light(mdata.x, mdata.y);
    }
    if(mos.b&1)
    {
      keyt = SDLKEYDOWN(SDLK_t);
      keyv = SDLKEYDOWN(SDLK_v);
      if(!keyt)
      {
        if(!keyv)
        {
          // Save corner heights
          vert = mesh.vrtstart[mdata.onfan];
          tl = mesh.vrtz[vert];
          vert = mesh.vrtnext[vert];
          tr = mesh.vrtz[vert];
          vert = mesh.vrtnext[vert];
          br = mesh.vrtz[vert];
          vert = mesh.vrtnext[vert];
          bl = mesh.vrtz[vert];
        }
        remove_fan(mdata.onfan);
      }
      switch(mdata.presser)
      {
      case 0:
        mesh.tile[mdata.onfan]=mdata.tile;
        break;
      case 1:
        mesh.tile[mdata.onfan]=(mdata.tile&0xfffe)+(rand()&1);
        break;
      case 2:
        mesh.tile[mdata.onfan]=(mdata.tile&0xfffc)+(rand()&3);
        break;
      case 3:
        mesh.tile[mdata.onfan]=(mdata.tile&0xfff0)+(rand()&6);
        break;
      }
      if(!keyt)
      {
        mesh.type[mdata.onfan]=mdata.type;
        add_fan(mdata.onfan, (mdata.x>>7)*31, (mdata.y>>7)*31);
        mesh.fx[mdata.onfan]=mdata.fx;
        if(!keyv)
        {
          // Return corner heights
          vert = mesh.vrtstart[mdata.onfan];
          mesh.vrtz[vert] = tl;
          vert = mesh.vrtnext[vert];
          mesh.vrtz[vert] = tr;
          vert = mesh.vrtnext[vert];
          mesh.vrtz[vert] = br;
          vert = mesh.vrtnext[vert];
          mesh.vrtz[vert] = bl;
        }
      }
    }
    if(mos.b&2)
    {
      mdata.type = mesh.type[mdata.onfan];
      mdata.tile = mesh.tile[mdata.onfan];
    }
  }
}

//------------------------------------------------------------------------------
void mouse_fx(int cnt)
{
  int x, y;

  mdata.x = mos.x - window_lst[cnt].x-window_lst[cnt].borderx+cam.x-69;
  mdata.y = mos.y - window_lst[cnt].y-window_lst[cnt].bordery+cam.y-69;
  if(mdata.x < 0 ||
    mdata.x >= SMALLX*mesh.sizex ||
    mdata.y < 0 ||
    mdata.y >= SMALLY*mesh.sizey)
  {
  }
  else
  {
    mdata.x = mdata.x*FOURNUM;
    mdata.y = mdata.y*FOURNUM;
    if(mdata.x >= (mesh.sizex<<7))  mdata.x = (mesh.sizex<<7)-1;
    if(mdata.y >= (mesh.sizey<<7))  mdata.y = (mesh.sizey<<7)-1;
    debugx = mdata.x/128.0;
    debugy = mdata.y/128.0;
    x = mdata.x>>7;
    y = mdata.y>>7;
    mdata.onfan = get_fan(x,y);
    if( mdata.onfan == -1 ) mdata.onfan = 0;

    if(mos.b&1)
    {
      mesh.fx[mdata.onfan] = mdata.fx;
    }
    if(mos.b&2)
    {
      mdata.fx = mesh.fx[mdata.onfan];
    }
  }
}

//------------------------------------------------------------------------------
void mouse_vertex(int cnt)
{
  mdata.x = mos.x - window_lst[cnt].x-window_lst[cnt].borderx+cam.x-69;
  mdata.y = mos.y - window_lst[cnt].y-window_lst[cnt].bordery+cam.y-69;
  mdata.x = mdata.x*FOURNUM;
  mdata.y = mdata.y*FOURNUM;
  if(SDLKEYDOWN(SDLK_f))
  {
    //    fix_corners(mdata.x>>7, mdata.y>>7);
    fix_vertices(mdata.x>>7, mdata.y>>7);
  }
  if(SDLKEYDOWN(SDLK_5))
  {
    set_select_no_bound_z(-8000<<2);
  }
  if(SDLKEYDOWN(SDLK_6))
  {
    set_select_no_bound_z(-127<<2);
  }
  if(SDLKEYDOWN(SDLK_7))
  {
    set_select_no_bound_z(127<<2);
  }
  if(mos.b&1)
  {
    if(mdata.rect==bfalse)
    {
      mdata.rect=btrue;
      mdata.rectx=mdata.x;
      mdata.recty=mdata.y;
    }
  }
  else
  {
    if(mdata.rect==btrue)
    {
      if(numselect_verts!=0 && !SDLKEYMOD(KMOD_ALT) && !SDLKEYDOWN(SDLK_MODE) &&
        !SDLKEYMOD(KMOD_LCTRL) && !SDLKEYMOD(KMOD_RCTRL))
      {
        clear_select();
      }
      if( SDLKEYMOD(KMOD_ALT) || SDLKEYDOWN(SDLK_MODE))
      {
        rect_unselect();
      }
      else
      {
        rect_select();
      }
      mdata.rect = bfalse;
    }
  }
  if(mos.b&2)
  {
    move_select(mos.cx, mos.cy, 0);
    bound_mouse();
  }
}

//------------------------------------------------------------------------------
void check_mouse(void)
{
  int cnt;

  debugx = -1;
  debugy = -1;

  unbound_mouse();
  move_camera();
  mdata.which_win = -1;
  mdata.x = -1;
  mdata.y = -1;
  mdata.mode = 0;
  cnt = 0;
  while(cnt < MAXWIN)
  {
    if(window_lst[cnt].on)
    {
      if(mos.x >= window_lst[cnt].x+window_lst[cnt].borderx &&
        mos.x <  window_lst[cnt].x+window_lst[cnt].borderx+window_lst[cnt].surfacex &&
        mos.y >= window_lst[cnt].y+window_lst[cnt].bordery &&
        mos.y <  window_lst[cnt].y+window_lst[cnt].bordery+window_lst[cnt].surfacey)
      {
        mdata.which_win = cnt;
        mdata.mode = window_lst[cnt].mode;
        if(mdata.mode==WINMODE_TILE)
        {
          mouse_tile(cnt);
        }
        if(mdata.mode==WINMODE_VERTEX)
        {
          mouse_vertex(cnt);
        }
        if(mdata.mode==WINMODE_SIDE)
        {
          mouse_side(cnt);
        }
        if(mdata.mode==WINMODE_FX)
        {
          mouse_fx(cnt);
        }
      }
    }
    cnt++;
  }
}

//------------------------------------------------------------------------------
void clear_mesh()
{
  int x, y;
  int fan;

  if(mdata.tile != FANOFF)
  {
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if(fan != -1)
        {
          remove_fan(fan);
          switch(mdata.presser)
          {
          case 0:
            mesh.tile[fan]=mdata.tile;
            break;
          case 1:
            mesh.tile[fan]=(mdata.tile&0xfffe)+(rand()&1);
            break;
          case 2:
            if(mdata.type >= 32)
              mesh.tile[fan]=(mdata.tile&0xfff8)+(rand()&6);
            else
              mesh.tile[fan]=(mdata.tile&0xfffc)+(rand()&3);
            break;
          case 3:
            mesh.tile[fan]=(mdata.tile&0xfff0)+(rand()&6);
            break;
          }
          mesh.type[fan]=mdata.type;
          if(mdata.type<=1) mesh.type[fan] = rand()&1;
          if(mdata.type == 32 || mdata.type == 33)
            mesh.type[fan] = 32 + (rand()&1);
          add_fan(fan, x*31, y*31);
        }
        x++;
      }
      y++;
    }
  }
}

//------------------------------------------------------------------------------
void three_e_mesh()
{
  // ZZ> Replace all 3F tiles with 3E tiles...
  int x, y;
  int fan;

  if(mdata.tile != FANOFF)
  {
    y = 0;
    while(y < mesh.sizey)
    {
      x = 0;
      while(x < mesh.sizex)
      {
        fan = get_fan(x,y);
        if(fan != -1)
        {
          if(mesh.tile[fan]==0x3F)  mesh.tile[fan]=0x3E;
        }
        x++;
      }
      y++;
    }
  }
}

//------------------------------------------------------------------------------
void toggle_fx(int fxmask)
{
  if(mdata.fx&fxmask)
  {
    mdata.fx-=fxmask;
  }
  else
  {
    mdata.fx+=fxmask;
  }
}

//------------------------------------------------------------------------------
void ease_up_mesh()
{
  // ZZ> This function lifts the entire mesh
  int x, y, cnt, zadd;
  Uint32 fan, vert;

  mos.y=mos.y_old;
  mos.x=mos.x_old;
  zadd = -mos.cy;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      if(fan != -1)
      {
        vert = mesh.vrtstart[fan];
        cnt = 0;
        while(cnt < mesh.command[mesh.type[fan]].numvertices)
        {
          move_vert(vert, 0, 0, zadd);
          vert = mesh.vrtnext[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void select_verts_connected()
{
  int vert, cnt, tnc, x, y, totalvert = 0;
  int fan;
  Uint8 found, select_vertsfan;

  y = 0;
  while(y < mesh.sizey)
  {
    x = 0;
    while(x < mesh.sizex)
    {
      fan = get_fan(x,y);
      select_vertsfan = bfalse;
      if(fan != -1)
      {
        totalvert = mesh.command[mesh.type[fan]].numvertices;
        cnt = 0;
        vert = mesh.vrtstart[fan];
        while(cnt < totalvert)
        {

          found = bfalse;
          tnc = 0;
          while(tnc < numselect_verts && !found)
          {
            if(select_verts[tnc]==vert)
            {
              found=btrue;
            }
            tnc++;
          }
          if(found) select_vertsfan = btrue;
          vert = mesh.vrtnext[vert];
          cnt++;
        }
      }

      if(select_vertsfan)
      {
        cnt = 0;
        vert = mesh.vrtstart[fan];
        while(cnt < totalvert)
        {
          add_select(vert);
          vert = mesh.vrtnext[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void check_keys(char *modname)
{
  char newloadname[256];

  keysdlbuffer = SDL_GetKeyState( &keycount );

  if(keydelay <= 0)
  {
    // Hurt
    if(SDLKEYDOWN(SDLK_h))
    {
      toggle_fx(MPDFX_DAMAGE);
      keydelay=KEYDELAY;
    }
    // Impassable
    if(SDLKEYDOWN(SDLK_i))
    {
      toggle_fx(MPDFX_IMPASS);
      keydelay=KEYDELAY;
    }
    // Barrier
    if(SDLKEYDOWN(SDLK_b))
    {
      toggle_fx(MPDFX_WALL);
      keydelay=KEYDELAY;
    }
    // Overlay
    if(SDLKEYDOWN(SDLK_o))
    {
      toggle_fx(MPDFX_WATER);
      keydelay=KEYDELAY;
    }
    // Reflective
    if(SDLKEYDOWN(SDLK_y))
    {
      toggle_fx(MPDFX_SHA);
      keydelay=KEYDELAY;
    }
    // Draw reflections
    if(SDLKEYDOWN(SDLK_d))
    {
      toggle_fx(MPDFX_DRAWREF);
      keydelay=KEYDELAY;
    }
    // Animated
    if(SDLKEYDOWN(SDLK_a))
    {
      toggle_fx(MPDFX_ANIM);
      keydelay=KEYDELAY;
    }
    // Slippy
    if(SDLKEYDOWN(SDLK_s))
    {
      toggle_fx(MPDFX_SLIPPY);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_g))
    {
      fix_mesh();
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_z))
    {
      set_mesh_tile(mesh.tile[mdata.onfan]);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_LSHIFT))
    {
      if(mesh.type[mdata.onfan] >= (MAXMESHTYPE>>1))
      {
        fx_mesh_tile(mesh.tile[mdata.onfan], 192, mdata.fx);
      }
      else
      {
        fx_mesh_tile(mesh.tile[mdata.onfan], 240, mdata.fx);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_x))
    {
      if(mesh.type[mdata.onfan] >= (MAXMESHTYPE>>1))
      {
        trim_mesh_tile(mesh.tile[mdata.onfan], 192);
      }
      else
      {
        trim_mesh_tile(mesh.tile[mdata.onfan], 240);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_e))
    {
      ease_up_mesh();
    }
    if(SDLKEYDOWN(SDLK_c))
    {
      clear_mesh();
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_LEFTBRACKET) || SDLKEYDOWN(SDLK_RIGHTBRACKET))
    {
      select_verts_connected();
    }
    if(SDLKEYDOWN(SDLK_8))
    {
      three_e_mesh();
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_j))
    {
      if(numselect_verts == 0) { jitter_mesh(); }
      else { jitter_select(); }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_l))
    {
      level_vrtz();
    }
    if(SDLKEYDOWN(SDLK_w))
    {
      //impass_edges(2);
      calc_vrta();
      sprintf( newloadname, "%s" SLASH_STR "modules" SLASH_STR "%s", egoboo_path, modname);
      save_mesh(newloadname);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_SPACE))
    {
      weld_select();
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_INSERT))
    {
      mdata.type=(mdata.type-1)&(MAXMESHTYPE-1);
      while(mesh.numline[mdata.type]==0)
      {
        mdata.type=(mdata.type-1)&(MAXMESHTYPE-1);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_DELETE))
    {
      mdata.type=(mdata.type+1)&(MAXMESHTYPE-1);
      while(mesh.numline[mdata.type]==0)
      {
        mdata.type=(mdata.type+1)&(MAXMESHTYPE-1);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_KP_PLUS))
    {
      mdata.tile=(mdata.tile+1)&255;
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_KP_MINUS))
    {
      mdata.tile=(mdata.tile-1)&255;
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_UP) || SDLKEYDOWN(SDLK_LEFT) || SDLKEYDOWN(SDLK_DOWN) || SDLKEYDOWN(SDLK_RIGHT))
    {
      if(SDLKEYDOWN(SDLK_UP))
      {
        cam.y-=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_LEFT))
      {
        cam.x-=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_DOWN))
      {
        cam.y+=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_RIGHT))
      {
        cam.x+=CAMRATE;
      }
      bound_camera();
    }
    if(SDLKEYDOWN(SDLK_END))
    {
      brushsize = 0;
    }
    if(SDLKEYDOWN(SDLK_PAGEDOWN))
    {
      brushsize = 1;
    }
    if(SDLKEYDOWN(SDLK_HOME))
    {
      brushsize = 2;
    }
    if(SDLKEYDOWN(SDLK_PAGEUP))
    {
      brushsize = 3;
    }
    if(SDLKEYDOWN(SDLK_1))
    {
      mdata.presser = 0;
    }
    if(SDLKEYDOWN(SDLK_2))
    {
      mdata.presser = 1;
    }
    if(SDLKEYDOWN(SDLK_3))
    {
      mdata.presser = 2;
    }
    if(SDLKEYDOWN(SDLK_4))
    {
      mdata.presser = 3;
    }
  }
}

void check_input(char * modulename)
{
  // ZZ> This function gets all the current player input states
  SDL_Event evt;

  while ( SDL_PollEvent( &evt ) )
  {

    switch ( evt.type )
    {
      case SDL_MOUSEBUTTONDOWN:
        ui.pending_click = btrue;
        break;

      case SDL_MOUSEBUTTONUP:
        ui.pending_click = bfalse;
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP:
        keystate = evt.key.state;
        break;
    }
  }

  read_mouse();

  check_keys(modulename);				//
  check_mouse();					//
};

//------------------------------------------------------------------------------
void create_imgcursor(void)
{
  int x, y;
  Uint32 col, loc, clr;
  SDL_Rect rtmp;

  imgcursor = cartman_CreateSurface(8, 8);
  col = MAKE_BGR(imgcursor,31,31,31);			// White color
  loc = MAKE_BGR(imgcursor,3,3,3);				// Gray color
  clr = MAKE_ABGR(imgcursor,0,0,0,8);

  // Simple triangle
  rtmp.x = 0;
  rtmp.y = 0;
  rtmp.w = 8;
  rtmp.h = 1;
  SDL_FillRect(imgcursor, &rtmp, loc);

  for(y=0; y<8; y++)
  {
    for(x=0; x<8; x++)
    {
      if(x+y < 8) SDL_PutPixel(imgcursor, x, y, col);
      else SDL_PutPixel(imgcursor, x, y, clr);
    }
  }
}

//------------------------------------------------------------------------------
void load_img(void)
{
  int cnt;
  SDL_Surface *bmptemp;

  bmptemp = cartman_LoadIMG("point.pcx");
  for(cnt = 0; cnt < MAXPOINTSIZE; cnt++)
  {
    imgpoint[cnt] = cartman_CreateSurface((cnt>>1)+4, (cnt>>1)+4);
    SDL_SoftStretch(bmptemp, NULL, imgpoint[cnt], NULL);
  }
  SDL_FreeSurface(bmptemp);

  bmptemp = cartman_LoadIMG("pointon.pcx");
  for(cnt = 0; cnt < MAXPOINTSIZE; cnt++)
  {
    imgpointon[cnt] = cartman_CreateSurface((cnt>>1)+4, (cnt>>1)+4);
    SDL_SoftStretch(bmptemp, NULL, imgpointon[cnt], NULL);
  }
  SDL_FreeSurface(bmptemp);

  imgref     = cartman_LoadIMG("ref.pcx");
  imgdrawref = cartman_LoadIMG("drawref.pcx");
  imganim    = cartman_LoadIMG("anim.pcx");
  imgwater   = cartman_LoadIMG("water.pcx");
  imgwall    = cartman_LoadIMG("slit.pcx");
  imgimpass  = cartman_LoadIMG("impass.pcx");
  imgdamage  = cartman_LoadIMG("damage.pcx");
  imgslippy  = cartman_LoadIMG("slippy.pcx");
}

//------------------------------------------------------------------------------
void draw_lotsa_stuff(void)
{
  int x, y, cnt, todo, tile, add;

  // Tell which tile we're in
  x = debugx * 128;
  y = debugy * 128;
  fnt_printf(theSurface, gFont, 0, 226, MAKE_SDLCOLOR(31,31,31),
    "X = %6.2f (%d)", debugx, x);
  fnt_printf(theSurface, gFont, 0, 234, MAKE_SDLCOLOR(31,31,31),
    "Y = %6.2f (%d)", debugy, y);

  // Tell user what keys are important
  fnt_printf(theSurface, gFont, 0, ui.scr.y-120, MAKE_SDLCOLOR(31,31,31),
    "O = Overlay (Water)");
  fnt_printf(theSurface, gFont, 0, ui.scr.y-112, MAKE_SDLCOLOR(31,31,31),
    "R = Reflective");
  fnt_printf(theSurface, gFont, 0, ui.scr.y-104, MAKE_SDLCOLOR(31,31,31),
    "D = Draw Reflection");
  fnt_printf(theSurface, gFont, 0, ui.scr.y-96, MAKE_SDLCOLOR(31,31,31),
    "A = Animated");
  fnt_printf(theSurface, gFont, 0, ui.scr.y-88, MAKE_SDLCOLOR(31,31,31),
    "B = Barrier (Slit)");
  fnt_printf(theSurface, gFont, 0, ui.scr.y-80, MAKE_SDLCOLOR(31,31,31),
    "I = Impassable (Wall)");
  fnt_printf(theSurface, gFont, 0, ui.scr.y-72, MAKE_SDLCOLOR(31,31,31),
    "H = Hurt");
  fnt_printf(theSurface, gFont, 0, ui.scr.y-64, MAKE_SDLCOLOR(31,31,31),
    "S = Slippy");

  // Vertices left
  fnt_printf(theSurface, gFont, 0, ui.scr.y-56, MAKE_SDLCOLOR(31,31,31),
    "Vertices %d", numfreevertices);

  // Misc data
  fnt_printf(theSurface, gFont, 0, ui.scr.y-40, MAKE_SDLCOLOR(31,31,31),
    "Ambient   %d", ambi);
  fnt_printf(theSurface, gFont, 0, ui.scr.y-32, MAKE_SDLCOLOR(31,31,31),
    "Ambicut   %d", ambicut);
  fnt_printf(theSurface, gFont, 0, ui.scr.y-24, MAKE_SDLCOLOR(31,31,31),
    "Direct    %d", direct);
  fnt_printf(theSurface, gFont, 0, ui.scr.y-16, MAKE_SDLCOLOR(31,31,31),
    "Brush amount %d", brushamount);
  fnt_printf(theSurface, gFont, 0, ui.scr.y-8, MAKE_SDLCOLOR(31,31,31),
    "Brush size   %d", brushsize);

  // Cursor
  if(mos.x >= 0 && mos.x < ui.scr.x && mos.y >= 0 && mos.y < ui.scr.y)
  {
    draw_sprite(theSurface, imgcursor, mos.x, mos.y);
  }

  // Tile picks
  todo = 0;
  tile = 0;
  add  = 1;
  if(mdata.tile<=MAXTILE)
  {
    switch(mdata.presser)
    {
    case 0:
      todo = 1;
      tile = mdata.tile;
      add = 1;
      break;
    case 1:
      todo = 2;
      tile = mdata.tile&0xfffe;
      add = 1;
      break;
    case 2:
      todo = 4;
      tile = mdata.tile&0xfffc;
      add = 1;
      break;
    case 3:
      todo = 4;
      tile = mdata.tile&0xfff0;
      add = 2;
      break;
    }
    x = 0;
    cnt = 0;
    while(cnt < todo)
    {
      SDL_Rect dst = {x, 0, SMALLX, SMALLY};
      if(mdata.type&32)
      {
        cartman_BlitScreen(bmpbigtile[tile], &dst);
      }
      else
      {
        cartman_BlitScreen(bmpsmalltile[tile], &dst);
      }
      x+=SMALLX;
      tile+=add;
      cnt++;
    }
    fnt_printf(theSurface, gFont, 0, 32, MAKE_SDLCOLOR(31,31,31),
      "Tile 0x%02x", mdata.tile);
    fnt_printf(theSurface, gFont, 0, 40, MAKE_SDLCOLOR(31,31,31),
      "Eats %d verts", mesh.command[mdata.type].numvertices);
    if(mdata.type>=MAXMESHTYPE/2)
    {
      fnt_printf(theSurface, gFont, 0, 56, MAKE_SDLCOLOR(31,16,16),
        "63x63 Tile");
    }
    else
    {
      fnt_printf(theSurface, gFont, 0, 56, MAKE_SDLCOLOR(16,16,31),
        "31x31 Tile");
    }
    draw_schematic(theSurface, mdata.type, 0, 64);
  }

  // FX select_vertsion
  if(!(mdata.fx&MPDFX_SHA))
    draw_sprite(theSurface, imgref, 0, 200);
  if(mdata.fx&MPDFX_DRAWREF)
    draw_sprite(theSurface, imgdrawref, 16, 200);
  if(mdata.fx&MPDFX_ANIM)
    draw_sprite(theSurface, imganim, 0, 216);
  if(mdata.fx&MPDFX_WALL)
    draw_sprite(theSurface, imgwall, 15, 215);
  if(mdata.fx&MPDFX_IMPASS)
    draw_sprite(theSurface, imgimpass, 15+8, 215);
  if(mdata.fx&MPDFX_DAMAGE)
    draw_sprite(theSurface, imgdamage, 15, 215+8);
  if(mdata.fx&MPDFX_SLIPPY)
    draw_sprite(theSurface, imgslippy, 15+8, 215+8);
  if(mdata.fx&MPDFX_WATER)
    draw_sprite(theSurface, imgwater, 0, 200);

  if(numattempt > 0)
    fnt_printf(theSurface, gFont, 0, 0, MAKE_SDLCOLOR(31,31,31),
    "numwritten %d/%d", numwritten, numattempt);

  fnt_printf(theSurface, gFont, 0, 0, MAKE_SDLCOLOR(31,31,31),
    "<%f, %f>", mos.x, mos.y );
}

//------------------------------------------------------------------------------
void draw_slider(int tlx, int tly, int brx, int bry, int* pvalue,
                 int minvalue, int maxvalue)
{
  int cnt;
  int value;
  SDL_Rect rtmp;

  // Pick a new value
  value = *pvalue;
  if(mos.x >= tlx && mos.x <= brx && mos.y >= tly && mos.y <= bry && mos.b)
  {
    value = (((mos.y - tly)*(maxvalue-minvalue))/(bry - tly)) + minvalue;
  }
  if(value < minvalue) value = minvalue;
  if(value > maxvalue) value = maxvalue;
  *pvalue = value;

  // Draw it
  if(maxvalue != 0)
  {
    cnt = ((value-minvalue)*20/(maxvalue-minvalue))+11;

    rtmp.x = tlx;
    rtmp.y = (((value-minvalue)*(bry-tly)/(maxvalue-minvalue)))+tly;
    rtmp.w = brx - tlx + 1;
    rtmp.h = 1;
    SDL_FillRect(theSurface, &rtmp, MAKE_BGR(theSurface,cnt,cnt,cnt));
  }

  rtmp.x = tlx;
  rtmp.y = tly;
  rtmp.h = brx-tlx+1;
  rtmp.w = bry-tly+1;
  SDL_FillRect(theSurface, &rtmp, MAKE_BGR(theSurface,31,31,31));

}

//------------------------------------------------------------------------------
void draw_main(void)
{
  SDL_FillRect(theSurface, NULL, MAKE_BGR(theSurface, 0, 0, 0));

  draw_all_windows();

  draw_slider( 0, 250, 19, 350, &ambi,          0, 200);
  draw_slider(20, 250, 39, 350, &ambicut,       0, ambi);
  draw_slider(40, 250, 59, 350, &direct,        0, 100);
  draw_slider(60, 250, 79, 350, &brushamount, -50,  50);

  draw_lotsa_stuff();

  dunframe++;
  secframe++;

  SDL_Flip(theSurface);
}

//------------------------------------------------------------------------------
int main(int argcnt, char* argtext[])
{
  char modulename[100];
  STRING fname;
  char *blah[3];

  blah[0] = malloc(256); strcpy(blah[0], "");
  blah[1] = malloc(256); strcpy(blah[1], "/home/bgbirdsey/egoboo");
  blah[2] = malloc(256); strcpy(blah[2], "advent" );

  argcnt = 3;
  argtext = blah;

  // register the logging code
  log_init();
  atexit( log_shutdown );

  show_info();						// Text title
  if(argcnt < 2 || argcnt > 3)
  {
    printf("USAGE: CARTMAN [PATH] MODULE ( without .MOD )\n\n");
    exit(0);
  }
  else if(argcnt < 3)
  {
    sprintf(egoboo_path, "%s", ".");
    sprintf(modulename, "%s.mod", argtext[1]);
  }
  else if(argcnt < 4)
  {
    size_t len = strlen(argtext[1]);
    char * pstr = argtext[1];
    if(pstr[0] == '\"')
    {
      pstr[len-1] = '\0';
      pstr++;
    }
    sprintf(egoboo_path, "%s", pstr);
    sprintf(modulename, "%s.mod", argtext[2]);
  }

  sprintf(fname, "%s" SLASH_STR "setup.txt", egoboo_path );
  read_setup( &cfg, fname );

  // initialize the SDL elements
  sdlinit( argcnt, argtext );
  gFont = TTF_OpenFont( "pc8x8.fon", 12 );

  make_randie();					    // Random number table
  fill_fpstext();					    // Make the FPS text

  load_all_windows();					// Load windows
  create_imgcursor();					// Make cursor image
  load_img();						      // Load other images
  load_mesh_fans();					  // Get fan data
  load_module(modulename);		// Load the module

  dunframe   = 0;						  // Timer resets
  worldclock = 0;
  timclock   = 0;
  while(!SDLKEYDOWN(SDLK_ESCAPE) && !SDLKEYDOWN(SDLK_F1))			// Main loop
  {
    check_input(modulename);

    render_all_windows();
    draw_main();

    SDL_Delay(1);
  }

  show_info();				// Ending statistics
  exit(0);						// End
}

//------------------------------------------------------------------------------
void GetScreen_Info( screen_info_t * psi, SDL_Surface * ps )
{
  const SDL_VideoInfo * pvi = SDL_GetVideoInfo();
  glInfo_t * gl_info = &(psi->gl_info);

  memset(psi, 0, sizeof(screen_info_t));

  psi->is_sw           = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_SWSURFACE ) );
  psi->is_hw           = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_HWSURFACE) );
  psi->use_asynch_blit = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_ASYNCBLIT) );
  psi->use_anyformat   = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_ANYFORMAT) );
  psi->use_hwpalette   = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_HWPALETTE) );
  psi->is_doublebuf    = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_DOUBLEBUF) );
  psi->is_fullscreen   = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_FULLSCREEN) );
  psi->use_opengl      = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_OPENGL) );
  psi->use_openglblit  = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_OPENGLBLIT) );
  psi->sdl_resizable   = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_RESIZABLE) );
  psi->use_hwaccel     = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_HWACCEL) );
  psi->has_srccolorkey = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_SRCCOLORKEY) );
  psi->use_rleaccel    = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_RLEACCEL) );
  psi->use_srcalpha    = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_SRCALPHA) );
  psi->is_prealloc     = BOOL_TO_BIT( HAS_BITS(ps->flags, SDL_PREALLOC) );

  psi->hw_available = pvi->hw_available;
  psi->wm_available = pvi->wm_available;
  psi->blit_hw      = pvi->blit_hw;
  psi->blit_hw_CC   = pvi->blit_hw_CC;
  psi->blit_hw_A    = pvi->blit_hw_A;
  psi->blit_sw      = pvi->blit_sw;
  psi->blit_sw_CC   = pvi->blit_sw_CC;
  psi->blit_sw_A    = pvi->blit_sw_A;

  // get any SDL-OpenGL info
  if( 1 == psi->use_opengl)
  {
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE,         &psi->red_d);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE,       &psi->grn_d);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE,        &psi->blu_d);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE,       &psi->alp_d);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER,     &psi->dbuff);
    SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE,      &psi->buf_d);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE,       &psi->zbf_d);
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE,     &psi->stn_d);
    SDL_GL_GetAttribute(SDL_GL_ACCUM_RED_SIZE,   &psi->acr_d);
    SDL_GL_GetAttribute(SDL_GL_ACCUM_GREEN_SIZE, &psi->acg_d);
    SDL_GL_GetAttribute(SDL_GL_ACCUM_BLUE_SIZE,  &psi->acb_d);
    SDL_GL_GetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, &psi->aca_d);
  }

  psi->d = ps->format->BitsPerPixel;
  psi->x = ps->w;
  psi->y = ps->h;
  psi->z = psi->zbf_d;

  // get any pure OpenGL device caps
  if( 1 == psi->use_opengl)
  {
    glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH,     &gl_info->max_modelview_stack_depth);
    glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH,    &gl_info->max_projection_stack_depth);
    glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH,       &gl_info->max_texture_stack_depth);
    glGetIntegerv(GL_MAX_NAME_STACK_DEPTH,          &gl_info->max_name_stack_depth);
    glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH,        &gl_info->max_attrib_stack_depth);
    glGetIntegerv(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, &gl_info->max_client_attrib_stack_depth);

    glGetIntegerv(GL_SUBPIXEL_BITS,        &gl_info->subpixel_bits);
    glGetFloatv(GL_POINT_SIZE_RANGE,        gl_info->point_size_range);
    glGetFloatv(GL_POINT_SIZE_GRANULARITY, &gl_info->point_size_granularity);
    glGetFloatv(GL_LINE_WIDTH_RANGE,        gl_info->line_width_range);
    glGetFloatv(GL_LINE_WIDTH_GRANULARITY, &gl_info->line_width_granularity);

    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, gl_info->max_viewport_dims);
    glGetBooleanv(GL_AUX_BUFFERS,      &gl_info->aux_buffers);
    glGetBooleanv(GL_RGBA_MODE,        &gl_info->rgba_mode);
    glGetBooleanv(GL_INDEX_MODE,       &gl_info->index_mode);
    glGetBooleanv(GL_DOUBLEBUFFER,     &gl_info->doublebuffer);
    glGetBooleanv(GL_STEREO,           &gl_info->stereo);
    glGetIntegerv(GL_RED_BITS,         &gl_info->red_bits);
    glGetIntegerv(GL_GREEN_BITS,       &gl_info->green_bits);
    glGetIntegerv(GL_BLUE_BITS,        &gl_info->blue_bits);
    glGetIntegerv(GL_ALPHA_BITS,       &gl_info->alpha_bits);
    glGetIntegerv(GL_INDEX_BITS,       &gl_info->index_bits);
    glGetIntegerv(GL_DEPTH_BITS,       &gl_info->depth_bits);
    glGetIntegerv(GL_STENCIL_BITS,     &gl_info->stencil_bits);
    glGetIntegerv(GL_ACCUM_RED_BITS,   &gl_info->accum_red_bits);
    glGetIntegerv(GL_ACCUM_GREEN_BITS, &gl_info->accum_green_bits);
    glGetIntegerv(GL_ACCUM_BLUE_BITS,  &gl_info->accum_blue_bits);
    glGetIntegerv(GL_ACCUM_ALPHA_BITS, &gl_info->accum_alpha_bits);

    glGetIntegerv(GL_MAX_LIGHTS,       &gl_info->max_lights);
    glGetIntegerv(GL_MAX_CLIP_PLANES,  &gl_info->max_clip_planes);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gl_info->max_texture_size);

    glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &gl_info->max_pixel_map_table);
    glGetIntegerv(GL_MAX_LIST_NESTING,    &gl_info->max_list_nesting);
    glGetIntegerv(GL_MAX_EVAL_ORDER,      &gl_info->max_eval_order);
  }
}

//------------------------------------------------------------------------------
void sdlinit( int argc, char **argv )
{
  int          colordepth;

  if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK ) < 0 )
  {
    log_error( "Unable to initialize SDL: %s\n", SDL_GetError() );
  }
  atexit( SDL_Quit );

  // start the font handler
  if( TTF_Init() < 0)
  {
    log_error( "Unable to load the font handler: %s\n", SDL_GetError() );
  }
  atexit( TTF_Quit );

#ifdef __unix__
  /* GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
     will cause SDL_SetVideoMode to fail with:
     "Unable to set video mode: Couldn't find matching GLX visual" */
  if ( cfg.scr.d == 32 ) cfg.scr.d = 24;
#endif

  colordepth = cfg.scr.d / 3;

#ifndef __APPLE__
  /* Setup the cute windows manager icon */
  theSurface = SDL_LoadBMP( "basicdat" SLASH_STR "icon.bmp" );
  if ( theSurface == NULL )
  {
    log_warning( "Unable to load icon (basicdat" SLASH_STR "icon.bmp)\n" );
  }
  else
  {
    SDL_WM_SetIcon( theSurface, NULL );
  }
#endif

  if( cfg.fullscreen )
  {
    theSurface = SDL_SetVideoMode( cfg.scr.x, cfg.scr.y, cfg.scr.d, SDL_SWSURFACE | SDL_FULLSCREEN );
  }
  else
  {
    theSurface = SDL_SetVideoMode( cfg.scr.x, cfg.scr.y, cfg.scr.d, SDL_ANYFORMAT | SDL_SWSURFACE );
  };

  if ( theSurface == NULL )
  {
    log_error( "Unable to set video mode: %s\n", SDL_GetError() );
  }

  GetScreen_Info( &(ui.scr), theSurface );

  // Set the window name
  SDL_WM_SetCaption( NAME, NAME );

  if ( ui.GrabMouse )
  {
    SDL_WM_GrabInput ( SDL_GRAB_ON );
  }

  if ( ui.HideMouse )
  {
    SDL_ShowCursor( 0 );  // Hide the mouse cursor
  }
}

//--------------------------------------------------------------------------------------------
//Macros for reading values from a file
#define GetBoolean(label, var, default) \
{ \
  if ( GetConfigBooleanValue( lpConfigFile, lszCurrentSection, (label), &lTempBool ) == 0 ) \
  { \
    lTempBool = (default); \
  } \
  (var) = lTempBool; \
}

#define GetInt(label, var, default) \
{ \
  if ( GetConfigIntValue( lpConfigFile, lszCurrentSection, (label), &lTempInt ) == 0 ) \
  { \
    lTempInt = (default); \
  } \
  (var) = lTempInt; \
}

// Don't make len larger than 64
#define GetString(label, var, len, default) \
{ \
  if ( GetConfigValue( lpConfigFile, lszCurrentSection, (label), lTempStr, sizeof( lTempStr ) / sizeof( *lTempStr ) ) == 0 ) \
  { \
    strncpy( lTempStr, (default), sizeof( lTempStr ) / sizeof( *lTempStr ) ); \
  } \
  strncpy( (var), lTempStr, (len) ); \
  (var)[(len) - 1] = '\0'; \
}

//--------------------------------------------------------------------------------------------
void read_setup( config_data_t * pc, char* filename )
{
  // ZZ> This function loads the setup file

  ConfigFilePtr lpConfigFile;
  STRING lszCurrentSection;
  bool_t lTempBool;
  Sint32 lTempInt;
  STRING lTempStr;


  lpConfigFile = OpenConfigFile( filename );
  if ( lpConfigFile == NULL )
  {
    // Major Error
    log_error( "Could not find setup file \"%s\"\n", filename );
  }
  else
  {
    /*********************************************

    GRAPHIC Section

    *********************************************/

    strcpy( lszCurrentSection, "GRAPHIC" );

    // Draw z reflection?
    GetBoolean( "Z_REFLECTION", pc->zreflect, bfalse );

    // Max number of vertrices (Should always be 100!)
    GetInt( "MAX_NUMBER_VERTICES", pc->maxtotalmeshvertices, 100 );
    pc->maxtotalmeshvertices *= 1024;

    // Do fullscreen?
    GetBoolean( "FULLSCREEN", pc->fullscreen, bfalse );

    // Screen Size
    GetInt( "SCREENSIZE_X", pc->scr.x, 640 );
    GetInt( "SCREENSIZE_Y", pc->scr.y, 480 );

    // Color depth
    GetInt( "COLOR_DEPTH", pc->scr.d, 32 );

    // The z depth
    GetInt( "Z_DEPTH", pc->scr.z, 8 );

    // Max number of messages displayed
    GetInt( "MAX_TEXT_MESSAGE", pc->maxmessage, 6 );
    pc->messageon = btrue;
    if ( pc->maxmessage < 1 )  { pc->maxmessage = 1;  pc->messageon = bfalse; }
    if ( pc->maxmessage > MAXMESSAGE )  { pc->maxmessage = MAXMESSAGE; }

    // Max number of messages displayed
    GetInt( "MESSAGE_DURATION", pc->messagetime, 50 );

    // Show status bars? (Life, mana, character icons, etc.)
    GetBoolean( "STATUS_BAR", pc->staton, btrue );
    pc->wraptolerance = pc->staton ? 90 : 32;

    // Perspective correction
    GetBoolean( "PERSPECTIVE_CORRECT", pc->perspective, bfalse );

    // Enable dithering? (Reduces quality but increases preformance)
    GetBoolean( "DITHERING", pc->dither, bfalse );

    // Reflection fadeout
    GetBoolean( "FLOOR_REFLECTION_FADEOUT", lTempBool, bfalse );
    if ( lTempBool )
    {
      pc->reffadeor = 0;
    }
    else
    {
      pc->reffadeor = 255;
    }

    // Draw Reflection?
    GetBoolean( "REFLECTION", pc->refon, bfalse );

    // Draw shadows?
    GetBoolean( "SHADOWS", pc->shaon, bfalse );

    // Draw good shadows?
    GetBoolean( "SHADOW_AS_SPRITE", pc->shasprite, btrue );

    // Draw phong mapping?
    GetBoolean( "PHONG", pc->phongon, btrue );

    // Draw water with more layers?
    GetBoolean( "MULTI_LAYER_WATER", pc->twolayerwateron, bfalse );

    // TODO: This is not implemented
    GetBoolean( "OVERLAY", pc->overlayvalid, bfalse );

    // Allow backgrounds?
    GetBoolean( "BACKGROUND", pc->backgroundvalid, bfalse );

    // Enable fog?
    GetBoolean( "FOG", pc->fogallowed, bfalse );

    // Do gourad shading?
    GetBoolean( "GOURAUD_SHADING", lTempBool, btrue );
    pc->shading = lTempBool ? GL_SMOOTH : GL_FLAT;

    // Enable antialiasing?
    GetBoolean( "ANTIALIASING", pc->antialiasing, bfalse );

    // Do we do texture filtering?
    GetString( "TEXTURE_FILTERING", lTempStr, sizeof(STRING), "LINEAR" );

    if ( lTempStr[0] == 'L' || lTempStr[0] == 'l' )  pc->texturefilter = TX_UNFILTERED;
    if ( lTempStr[0] == 'L' || lTempStr[0] == 'l' )  pc->texturefilter = TX_LINEAR;
	  if ( lTempStr[0] == 'M' || lTempStr[0] == 'm' )  pc->texturefilter = TX_MIPMAP;
    if ( lTempStr[0] == 'B' || lTempStr[0] == 'b' )  pc->texturefilter = TX_BILINEAR;
    if ( lTempStr[0] == 'T' || lTempStr[0] == 't' )  pc->texturefilter = TX_TRILINEAR_1;
    if ( lTempStr[0] == '2'                       )  pc->texturefilter = TX_TRILINEAR_2;
    if ( lTempStr[0] == 'A' || lTempStr[0] == 'a' )  pc->texturefilter = TX_ANISOTROPIC;

    // Max number of lights
	GetInt( "MAX_DYNAMIC_LIGHTS", pc->maxlights, 12 );
	if ( pc->maxlights > TOTALMAXDYNA ) pc->maxlights = TOTALMAXDYNA;

    // Get the FPS limit
	GetInt( "MAX_FPS_LIMIT", pc->frame_limit, 30 );

    // Get the particle limit
	GetInt( "MAX_PARTICLES", pc->maxparticles, 256 );
	if(pc->maxparticles > TOTALMAXPRT) pc->maxparticles = TOTALMAXPRT;

    /*********************************************

    SOUND Section

    *********************************************/

    strcpy( lszCurrentSection, "SOUND" );

    // Enable sound
    GetBoolean( "SOUND", pc->soundvalid, bfalse );

    // Enable music
    GetBoolean( "MUSIC", pc->musicvalid, bfalse );

    // Music volume
    GetInt( "MUSIC_VOLUME", pc->musicvolume, 50 );

    // Sound volume
    GetInt( "SOUND_VOLUME", pc->soundvolume, 75 );

    // Max number of sound channels playing at the same time
    GetInt( "MAX_SOUND_CHANNEL", pc->maxsoundchannel, 16 );
    pc->maxsoundchannel = CLIP(pc->maxsoundchannel, 8, 128);

    // The output buffer size
    GetInt( "OUTPUT_BUFFER_SIZE", pc->buffersize, 2048 );
    pc->buffersize = CLIP(pc->buffersize, 512, 8196);


    /*********************************************

    CONTROL Section

    *********************************************/

    strcpy( lszCurrentSection, "CONTROL" );

    // Camera control mode
    GetString( "AUTOTURN_CAMERA", lTempStr, sizeof(STRING), "GOOD" );

    if ( lTempStr[0] == 'G' || lTempStr[0] == 'g' )  pc->camautoturn = 255;
    if ( lTempStr[0] == 'T' || lTempStr[0] == 't' )  pc->camautoturn = btrue;
    if ( lTempStr[0] == 'F' || lTempStr[0] == 'f' )  pc->camautoturn = bfalse;


    /*********************************************

    NETWORK Section

    *********************************************/

    strcpy( lszCurrentSection, "NETWORK" );

    // Enable networking systems?
    GetBoolean( "NETWORK_ON", pc->networkon, bfalse );

    // Max lag
    GetInt( "LAG_TOLERANCE", pc->lag, 2 );
    GetInt( "RTS_LAG_TOLERANCE", pc->orderlag, 25 );

    // Name or IP of the host or the target to join
    GetString( "HOST_NAME", pc->nethostname, sizeof(STRING), "no host" );

    // Multiplayer name
    GetString( "MULTIPLAYER_NAME", pc->netmessagename, sizeof(STRING), "little Raoul" );


    /*********************************************

    DEBUG Section

    *********************************************/

    strcpy( lszCurrentSection, "DEBUG" );

    // Show the FPS counter?
    GetBoolean( "DISPLAY_FPS", pc->fpson, btrue );
    GetBoolean( "HIDE_MOUSE", pc->HideMouse, btrue );
    GetBoolean( "GRAB_MOUSE", pc->GrabMouse, btrue );
    GetBoolean( "DEV_MODE", pc->DevMode, btrue );

    CloseConfigFile( lpConfigFile );

  }
}

//--------------------------------------------------------------------------------------------
bool_t cartman_ExpandFormat(SDL_PixelFormat * pformat)
{
  // use the basic screen format to create a surface with proper alpha support

  int i;

  if( NULL == pformat ) return bfalse;

  if( pformat->BitsPerPixel > 24 )
  {
    // create the mask
    // this will work if both endian systems think they have "RGBA" graphics
    // if you need a different pixel format (ARGB or BGRA or whatever) this section
    // will have to be changed to reflect that
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
    pformat->Amask = ( Uint32 )( 0xFF << 24 );
    pformat->Bmask = ( Uint32 )( 0xFF << 16 );
    pformat->Gmask = ( Uint32 )( 0xFF <<  8 );
    pformat->Rmask = ( Uint32 )( 0xFF <<  0 );
#else
    pformat->Amask = ( Uint32 )( 0xFF <<  0 );
    pformat->Bmask = ( Uint32 )( 0xFF <<  8 );
    pformat->Gmask = ( Uint32 )( 0xFF << 16 );
    pformat->Rmask = ( Uint32 )( 0xFF << 24 );
#endif

    for ( i = 0; i < cfg.scr.d && ( pformat->Amask & ( 1 << i ) ) == 0; i++ );
    if( 0 == (pformat->Amask & ( 1 << i )) )
    {
      // no alpha bits available
      pformat->Ashift = 0;
      pformat->Aloss  = 8;
    }
    else
    {
      // normal alpha channel
      pformat->Ashift = i;
      pformat->Aloss  = 0;
    }
  }


  return btrue;
}



//--------------------------------------------------------------------------------------------
SDL_Surface * cartman_CreateSurface(int w, int h)
{
  SDL_PixelFormat   tmpformat;

  if( NULL == theSurface ) return NULL;

  // expand the screen format to support alpha
  memcpy( &tmpformat, theSurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
  cartman_ExpandFormat(&tmpformat);

  return SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, tmpformat.BitsPerPixel, tmpformat.Rmask, tmpformat.Gmask, tmpformat.Bmask, tmpformat.Amask );
}

//--------------------------------------------------------------------------------------------
void read_mouse()
{
  int x, y, b;
  if ( mos.relative )
  {
    b = SDL_GetRelativeMouseState( &x, &y );
    mos.cx = x;
    mos.cy = y;
    mos.x += mos.cx;
    mos.y += mos.cy;
  }
  else
  {
    b = SDL_GetMouseState( &x, &y );
    mos.cx = mos.x - x;
    mos.cy = mos.y - y;
    mos.x  = x;
    mos.y  = y;
  }

  mos.button[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
  mos.button[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
  mos.button[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0; // Middle is 2 on SDL
  mos.button[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;

  // Mouse mask
  mos.b = ( mos.button[3] << 3 ) | ( mos.button[2] << 2 ) | ( mos.button[1] << 1 ) | ( mos.button[0] << 0 );
}

void draw_sprite(SDL_Surface * dst, SDL_Surface * sprite, int x, int y )
{
  SDL_Rect rdst;

  if(NULL == dst || NULL == sprite) return;

  rdst.x = x;
  rdst.y = y;
  rdst.w = sprite->w;
  rdst.h = sprite->h;

  cartman_BlitSurface(sprite, NULL, dst, &rdst );
}

void line(SDL_Surface * surf, int x0, int y0, int x1, int y1, Uint32 c)
{
  Uint32 pix = c;
  int dy = y1 - y0;
  int dx = x1 - x0;
  float t = (float) 0.5;                      // offset for rounding

  SDL_PutPixel(surf, x0, y0, pix);
  if (abs(dx) > abs(dy))           // slope < 1
  {
    float m = (float) dy / (float) dx;      // compute slope
    t += y0;
    dx = (dx < 0) ? -1 : 1;
    m *= dx;
    while (x0 != x1)
    {
      x0 += dx;                           // step to next x value
      t += m;                             // add slope to y value
      SDL_PutPixel(surf, x0, (int) t, pix);
    }
  }
  else
  {                                         // slope >= 1
    float m = (float) dx / (float) dy;      // compute slope
    t += x0;
    dy = (dy < 0) ? -1 : 1;
    m *= dy;
    while (y0 != y1)
    {
      y0 += dy;                           // step to next y value
      t += m;                             // add slope to x value
      SDL_PutPixel(surf, (int) t, y0, pix);
    }
  }
}

int cartman_BlitScreen(SDL_Surface * bmp, SDL_Rect * prect)
{
  return cartman_BlitSurface(bmp, NULL, theSurface, prect );
}

bool_t SDL_RectIntersect(SDL_Rect * src, SDL_Rect * dst, SDL_Rect * isect )
{
  Sint16 xmin,xmax, ymin,ymax;

  // should not happen
  if(NULL == src && NULL == dst) return bfalse;

  // null cases
  if(NULL == isect) return bfalse;
  if(NULL == src) { *isect = *dst; return btrue; }
  if(NULL == dst) { *isect = *src; return btrue; }

  xmin = MAX(src->x, dst->x);
  xmax = MIN(src->x + src->w, dst->x + dst->w);

  ymin = MAX(src->y, dst->y);
  ymax = MIN(src->y + src->h, dst->y + dst->h);

  isect->x = xmin;
  isect->w = MAX(0, xmax-xmin);
  isect->y = ymin;
  isect->h = MAX(0, ymax-ymin);

  return btrue;
}

int cartman_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect)
{
  // clip the source and destination rectangles

  int retval = -1;
  SDL_Rect rsrc, rdst;

  if(NULL == srcrect && NULL == dstrect)
  {
    retval = SDL_BlitSurface(src, NULL, dst, NULL);
    if(retval >= 0)
    {
      SDL_UpdateRect(dst, 0,0,0,0);
    }
  }
  else if(NULL == srcrect)
  {
    SDL_RectIntersect( &(dst->clip_rect), dstrect, &rdst );
    retval = SDL_BlitSurface(src, NULL, dst, &rdst);
    if(retval >= 0)
    {
      SDL_UpdateRect(dst, rdst.x,rdst.y,rdst.w,rdst.h);
    }
  }
  else if(NULL == dstrect)
  {
    SDL_RectIntersect( &(src->clip_rect), srcrect, &rsrc );

    retval = SDL_BlitSurface(src, &rsrc, dst, NULL);
    if(retval >= 0)
    {
      SDL_UpdateRect(dst, 0,0,0,0);
    }
  }
  else
  {
    SDL_RectIntersect( &(src->clip_rect), srcrect, &rsrc );
    SDL_RectIntersect( &(dst->clip_rect), dstrect, &rdst );

    retval = SDL_BlitSurface(src, &rsrc, dst, &rdst);
    if(retval >= 0)
    {
      SDL_UpdateRect(dst, rdst.x,rdst.y,rdst.w,rdst.h);
    }
  }

  return retval;
}


SDL_Surface * cartman_LoadIMG(const char * szName)
{
  SDL_PixelFormat tmpformat;
  SDL_Surface * bmptemp, * bmpconvert;

  // load the bitmap
  bmptemp = IMG_Load(szName);

  // expand the screen format to support alpha
  memcpy( &tmpformat, theSurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
  cartman_ExpandFormat(&tmpformat);

  // convert it to the same pixel format as the screen surface
  bmpconvert = SDL_ConvertSurface( bmptemp, &tmpformat, SDL_SWSURFACE );
  SDL_FreeSurface(bmptemp);

  return bmpconvert;
}
