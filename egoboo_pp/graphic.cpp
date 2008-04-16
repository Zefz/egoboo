// Egoboo, Copyright (C) 2000 Aaron Bishop



#include "graphic.h"
#include "Input.h"
#include "mesh.h"
#include "character.h"
#include "network.h"
#include "Particle.h"
#include "Mad.h"
#include "MD2_file.h"
#include "Camera.h"
#include "Font.h"
#include "UI.h"
#include "Texture.h"
#include "Passage.h"
#include "Profile.h"
#include "egoboo.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Water_List WaterList;
Fog        GFog;
Weather    GWeather;

Icon_List  IconList;
Tx_List    TxList;

GLTexture TxTrimX;                                        /* trim */
GLTexture TxTrimY;                                        /* trim */
GLTexture TxTrim;
GLTexture TxBars;          //OpenGL status bar surface
GLTexture TxBlip;          //OpenGL you are here surface
GLTexture TxMap;          //OpenGL map surface

bool sdl_initialized   = false;
bool ogl_initialized   = false;
bool video_initialized = false;

Uint16 Blip::numblip      = 0;
Uint8  Blip::mapon        = false;
Uint8  Blip::youarehereon = false;
rect_t Blip::rect;           // The blip rectangles

Tile_Dam  GTile_Dam;
Tile_Anim GTile_Anim;

static Uint16          dolist[CHR_COUNT];             // List of which characters to draw
static Uint16          numdolist;                  // How many in the list

Locker_GraphicsMode::eMode Locker_GraphicsMode::mode = Locker_GraphicsMode::MODE_UNKNOWN;

//--------------------------------------------------------------------------------------------
static bool load_one_title_image(int titleimage, char *szLoadName);
static void load_menu();


static void release_all_titleimages();
static void release_all_models();

//static void make_renderlist();

// DOLIST
static void add_to_dolist(int cnt);
static void order_dolist(void);
static void make_dolist(void);


//TILES
static void set_fan_light(int fanx, int fany, Dyna_Info & particle);

//WATER
static void make_water();

//RENDERER
static void Reshape3D(int w, int h);
//static void Begin2DMode(void);
//static void End2DMode(void);

static void render_background(TEX_REF & texture);
static void render_foreground_overlay(TEX_REF & texture);
static void render_shadow(Character & chr, Particle_List & plist, int type);
static void render_bad_shadow(int character);
static void render_water(Camera & cam);

static void light_characters(Character_List & clist, Mesh & msh);
static void light_particles(Particle_List & plist, Mesh & msh);
static void do_dyna_light_mesh(Mesh & msh, Particle_List & plist);
static void draw_scene_sadreflection(Camera & cam);
static void draw_scene_zreflection(Camera & cam);
static void draw_one_icon(ICON_REF & icontype, int x, int y, Uint8 sparkle);
static void draw_map(int x, int y);
static int  draw_one_bar(int bartype, int x, int y, int ticks, int maxticks);

static void draw_scene(Camera & cam);

static int  draw_status(Uint16 character, int x, int y);
static void draw_blip(Uint8 color, int x, int y);
static void draw_trimx(int x, int y, int length);
static void draw_trimy(int x, int y, int length);
static void draw_trim_box(int left, int top, int right, int bottom);
static void draw_trim_box_opening(int left, int top, int right, int bottom, float amount);





//RTS
//static void build_select(float tlx, float tly, float brx, float bry, Uint8 team);
//static Uint16 build_select_target(float tlx, float tly, float brx, float bry, Uint8 team);
//static void move_rtsxy();
//static void do_cursor_rts();
//static void draw_titleimage(int image, int x, int y);


// Defined in egoboo.h
SDL_Surface *displaySurface = NULL;
bool gTextureOn = false;

//--------------------------------------------------------------------------------------------
void EnableTexturing()
{
  //if (!gTextureOn)
  //{
    glEnable(GL_TEXTURE_2D);
    gTextureOn = true;
  //}
}

//--------------------------------------------------------------------------------------------
void DisableTexturing()
{
  if (gTextureOn)
  {
    glDisable(GL_TEXTURE_2D);
    gTextureOn = false;
  }
}

//--------------------------------------------------------------------------------------------
// This needs work
void Begin3DMode(Camera & cam)
{
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(cam.mProjection.v);

  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(cam.mView.v);
  //glMultMatrixf(cam.mWorld.v);
}

//--------------------------------------------------------------------------------------------
void End3DMode()
{
}

//--------------------------------------------------------------------------------------------
/********************> Begin2DMode() <*****/
//void Begin2DMode(void)
//{
//  glMatrixMode(GL_PROJECTION);
//  glLoadIdentity();         // Reset The Projection Matrix
//  glOrtho(0, scrx, 0, scry, 1, -1);     // Set up an orthogonal projection
//
//  glMatrixMode(GL_MODELVIEW);
//  glLoadIdentity();
//
//  glDisable(GL_DEPTH_TEST);
//  glDisable(GL_CULL_FACE);
//}

//--------------------------------------------------------------------------------------------
/********************> End2DMode() <*****/
void End2DMode(void)
{
  glDisable(GL_CULL_FACE); // glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
}




//--------------------------------------------------------------------------------------------
/********************> Reshape3D() <*****/
void Reshape3D(int w, int h)
{
  glViewport(0, 0, w, h);
}

//--------------------------------------------------------------------------------------------
int glinit(int argc, char **argv)
{
  GLfloat intensity[] = {1.0,1.0,1.0,1.0};

  /* initialize the matrices */
  glMatrixMode(GL_TEXTURE);
  glLoadIdentity();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /* Depth testing stuff */
  glClearDepth(1.0);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);

  //fill mode
  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK,  GL_FILL);

  /* Enable a single OpenGL light. */
  //glLightfv(GL_LIGHT0, GL_SPECULAR, light_diffuse);
  //glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  //glEnable(GL_LIGHT0);
  glDisable(GL_LIGHTING);
  //glLightModelfv(GL_LIGHT_MODEL_AMBIENT,intensity);

  /* Backface culling */
  glDisable(GL_CULL_FACE); // glEnable(GL_CULL_FACE);    // This seems implied - DDOI
  glCullFace(GL_BACK);

  glEnable(GL_COLOR_MATERIAL); // Need this for color + lighting

  EnableTexturing();  // Enable texture mapping

  ogl_initialized = true;

  return 1;
}

//--------------------------------------------------------------------------------------------
void sdlinit(int argc, char **argv)
{
  SDL_Surface *theSurface;

  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK) < 0)
  {
    fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  atexit(SDL_Quit);

  /* Setup the cute windows manager icon */
  theSurface =SDL_LoadBMP("basicdat/icon.bmp");
  if (theSurface == NULL)
  {
    fprintf(stderr, "Unable to load icon\n");
    exit(1);
  }
  SDL_WM_SetIcon(theSurface, NULL);


  // Set the window name
  SDL_WM_SetCaption("Egoboo ", "Egoboo");

  if (gGrabMouse)
  {
    SDL_WM_GrabInput(SDL_GRAB_ON);
  }

  if (gHideMouse)
  {
    SDL_ShowCursor(0); // Hide the mouse cursor
  }

  sdl_initialized = true;
}

//---------------------------------------------------------------------------------------------
void set_video_options()
{
  int   colordepth;

  colordepth=scrd/3;

  /* Set the OpenGL Attributes */
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, colordepth);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, colordepth);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  colordepth);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, scrd);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  displaySurface = SDL_SetVideoMode(scrx, scry, scrd, SDL_DOUBLEBUF | SDL_OPENGL | (fullscreen ? SDL_FULLSCREEN : 0));
  if (displaySurface == NULL)
  {
    fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
    exit(1);
  }

  reset_graphics();

  Reshape3D(scrx, scry);

  video_initialized = true;
}


//---------------------------------------------------------------------------------------------
void Tx_List::release_all_textures()
{
  // ZZ> This function clears out all of the textures
  int cnt;

  for (cnt = 0; cnt < SIZE; cnt++) GLTexture::Release(&_list[cnt]);

  _setup();
}

//--------------------------------------------------------------------------------------------
Tx_List::index_t Tx_List::load_one(char *szLoadName, Uint32 key, Uint32 force)
{
  // ZZ> This function is used to load a texture.  

  // get a free texture
  Uint32 idx = get_free(force);
  if(INVALID==idx) return index_t(INVALID);

  // try to load the teture
  if(GLTexture::INVALID == GLTexture::Load(&_list[idx], szLoadName, key) )
  {
    return_one(idx);
    return index_t(INVALID);
  };

  return index_t(idx);
}

//--------------------------------------------------------------------------------------------
Icon_List::index_t Icon_List::load_one_icon(char *szLoadName, Uint32 force)
{
  // ZZ> This function is used to load an icon.  Most icons are loaded
  //     without this function though...

  // get a free texture
  Uint32 idx = get_free(force);
  if(INVALID==idx) return index_t(INVALID);

  // try to load the teture
  if(GLTexture::INVALID == GLTexture::Load(&_list[idx], szLoadName) )
  {
    return_one(idx);
    return index_t(INVALID);
  };

  return index_t(idx);
}

//---------------------------------------------------------------------------------------------
void prime_titleimage()
{
  // ZZ> This function sets the title image pointers to NULL
  int cnt;

  for (cnt = 0; cnt < MAXMODULE; cnt++)
    TxTitleImage[cnt].textureID = GLTexture::INVALID;

  //titlerect.x = 0;
  //titlerect.w = TITLESIZE;
  //titlerect.y = 0;
  //titlerect.h = TITLESIZE;
}

//---------------------------------------------------------------------------------------------
void Icon_List::prime_icons()
{
  // ZZ> This function sets the icon pointers to NULL
  int cnt;

  for (cnt = 0; cnt < SIZE; cnt++)
  {
    _list[cnt].textureID = GLTexture::INVALID;
  }

  //clear out the book icons
  book_count = 0;
  for(cnt=0;cnt<4;cnt++)
    book_icon[cnt] = INVALID;

  null = INVALID;
  keyb = INVALID;
  mous = INVALID;
  joya = INVALID;
  joyb = INVALID;

  ProfileList.clear_icons();

  rect.left = 0;
  rect.right = 0x20;
  rect.top = 0;
  rect.bottom = 0x20;

  _setup();
}

//---------------------------------------------------------------------------------------------
void Icon_List::release_all_icons()
{
  // ZZ> This function clears out all of the icons
  int cnt;

  for (cnt = 0; cnt < SIZE; cnt++)
    GLTexture::Release(&_list[cnt]);

  prime_icons();
}

//---------------------------------------------------------------------------------------------
//void release_all_titleimages()
//{
// // ZZ> This function clears out all of the title images
// int cnt;
//
// for (cnt = 0; cnt < MAXMODULE; cnt++)
//  GLTexture::Release( &TxTitleImage[cnt] );
//}
//
//
//
//--------------------------------------------------------------------------------------------
void release_grfx(void)
{
  // ZZ> This function frees up graphics/input/sound resources.

  SDL_Quit();
}

//--------------------------------------------------------------------------------------------
void release_map()
{
  // ZZ> This function releases all the map images

  GLTexture::Release(&TxMap);
}

//--------------------------------------------------------------------------------------------
void debug_message(const char *format, ...)
{
  va_list args;

  Uint32 slot = GMsg.get_free();
  if(Msg::INVALID==slot) return;

  // fprint the message into the message buffer
  va_start(args, format);
  snprintf(GMsg.textdisplay[slot], 79, format, args);
  va_end(args);

  GMsg.time[slot] = MESSAGETIME;
}

//void debug_message(char *text)
//{
//  // ZZ> This function sticks a message in the display queue and sets its timer
//  Uint32 slot = GMsg.get_free();
//  if(Msg::INVALID==slot) return;
//
//  // Copy the message
//  int write = 0;
//  int read = 0;
//  char cTmp = text[read];  read++;
//  GMsg.time[slot] = MESSAGETIME;
//
//  while (cTmp != 0)
//  {
//    GMsg.textdisplay[slot][write] = cTmp;
//    write++;
//    cTmp = text[read];  read++;
//  }
//  GMsg.textdisplay[slot][write] = 0;
//}

//--------------------------------------------------------------------------------------------
void reset_end_text()
{
  // ZZ> This function resets the end-module text
  if (PlaList.count_total > 1)
  {
    sprintf(endtext, "Sadly, they were never heard from again...");
    endtextwrite = 42;  // Where to append further text
  }
  else
  {
    if (PlaList.count_total == 0)
    {
      // No players???  RTS module???
      sprintf(endtext, "The game has ended...");
      endtextwrite = 21;
    }
    else
    {
      // One player
      sprintf(endtext, "Sadly, no trace was ever found...");
      endtextwrite = 33;  // Where to append further text
    }
  }
}

//--------------------------------------------------------------------------------------------
void append_end_text(int message, Uint16 character)
{
  // ZZ> This function appends a message to the end-module text
  endtext[0] = 0;

  if (message>GMsg.total) return;

  expand_message( endtext, &GMsg.text[GMsg.index[message]], character );
}

//--------------------------------------------------------------------------------------------
void create_szfpstext(int frames)
{
  // ZZ> This function fills in the number of frames in "000 Frames per Second"
  frames = frames&0x01FF;
  szfpstext[0] = '0'+(frames/100);
  szfpstext[1] = '0'+((frames/10)%10);
  szfpstext[2] = '0'+(frames%10);
}

//--------------------------------------------------------------------------------------------
//void make_renderlist()
//{
//  // ZZ> This function figures out which mesh fans to draw
//  int cnt, fan, fanx, fany;
//  int row, run, numrow;
//  int xlist[4], ylist[4];
//  int leftnum, leftlist[4];
//  int rightnum, rightlist[4];
//  int fanrowstart[0x80], fanrowrun[0x80];
//  int x, stepx, divx, basex;
//  int from, to;
//
//  // Clear old render lists
//  cnt = 0;
//  while (cnt < GMesh.numrenderlist)
//  {
//    fan = GMesh.renderlist[cnt];
//    GMesh.fan_info[fan].inrenderlist = false;
//    cnt++;
//  }
//  GMesh.numrenderlist = 0;
//  GMesh.numrenderlistref = 0;
//  GMesh.numrenderlistsha = 0;
//
//  // Make sure it doesn't die ugly !!!BAD!!!
//
//  // It works better this way...
//  GCamera.corner_y[GCamera.corner_listlowtohigh_y[3]]+=0x0100;
//
//  // Make life simpler
//  xlist[0] = GCamera.corner_x[GCamera.corner_listlowtohigh_y[0]];
//  xlist[1] = GCamera.corner_x[GCamera.corner_listlowtohigh_y[1]];
//  xlist[2] = GCamera.corner_x[GCamera.corner_listlowtohigh_y[2]];
//  xlist[3] = GCamera.corner_x[GCamera.corner_listlowtohigh_y[3]];
//  ylist[0] = GCamera.corner_y[GCamera.corner_listlowtohigh_y[0]];
//  ylist[1] = GCamera.corner_y[GCamera.corner_listlowtohigh_y[1]];
//  ylist[2] = GCamera.corner_y[GCamera.corner_listlowtohigh_y[2]];
//  ylist[3] = GCamera.corner_y[GCamera.corner_listlowtohigh_y[3]];
//
//  // Find the center line
//  divx = ylist[3]-ylist[0]; if (divx < 1) return;
//  stepx = xlist[3]-xlist[0];
//  basex = xlist[0];
//
//  // Find the points in each edge
//  leftlist[0] = 0;  leftnum = 1;
//  rightlist[0] = 0;  rightnum = 1;
//  if (xlist[1] < (stepx*(ylist[1]-ylist[0])/divx)+basex)
//  {
//    leftlist[leftnum] = 1;  leftnum++;
//    GCamera.corner_x[1]-=0x0200;
//  }
//  else
//  {
//    rightlist[rightnum] = 1;  rightnum++;
//    GCamera.corner_x[1]+=0x0200;
//  }
//  if (xlist[2] < (stepx*(ylist[2]-ylist[0])/divx)+basex)
//  {
//    leftlist[leftnum] = 2;  leftnum++;
//    GCamera.corner_x[2]-=0x0200;
//  }
//  else
//  {
//    rightlist[rightnum] = 2;  rightnum++;
//    GCamera.corner_x[2]+=0x0200;
//  }
//  leftlist[leftnum] = 3;  leftnum++;
//  rightlist[rightnum] = 3;  rightnum++;
//
//  // Make the left edge ( rowstart )
//  fany = ylist[0]>>JF::MPD_bits;
//  row = 0;
//  cnt = 1;
//  while (cnt < leftnum)
//  {
//    from = leftlist[cnt-1];  to = leftlist[cnt];
//    x = xlist[from];
//    divx = ylist[to]-ylist[from];
//    stepx = 0;
//    if (divx > 0)
//    {
//      stepx = ((xlist[to]-xlist[from])<<JF::MPD_bits)/divx;
//    }
//    x-=0x0100;
//    run = ylist[to]>>JF::MPD_bits;
//    while (fany < run)
//    {
//      if (fany >= 0 && fany < GMesh.fansHigh())
//      {
//        fanx = x>>JF::MPD_bits;
//        if (fanx < 0)  fanx = 0;
//        if (fanx >= GMesh.fansWide())  fanx = GMesh.fansWide()-1;
//        fanrowstart[row] = fanx;
//        row++;
//      }
//      x+=stepx;
//      fany++;
//    }
//    cnt++;
//  }
//  numrow = row;
//
//  // Make the right edge ( rowrun )
//  fany = ylist[0]>>JF::MPD_bits;
//  row = 0;
//  cnt = 1;
//  while (cnt < rightnum)
//  {
//    from = rightlist[cnt-1];  to = rightlist[cnt];
//    x = xlist[from];
//    //x+=0x80;
//    divx = ylist[to]-ylist[from];
//    stepx = 0;
//    if (divx > 0)
//    {
//      stepx = ((xlist[to]-xlist[from])<<JF::MPD_bits)/divx;
//    }
//    run = ylist[to]>>JF::MPD_bits;
//    while (fany < run)
//    {
//      if (fany >= 0 && fany < GMesh.fansHigh())
//      {
//        fanx = x>>JF::MPD_bits;
//        if (fanx < 0)  fanx = 0;
//        if (fanx >= GMesh.fansWide()-1)  fanx = GMesh.fansWide()-1;//-2
//        fanrowrun[row] = ABS(fanx-fanrowstart[row])+1;
//        row++;
//      }
//      x+=stepx;
//      fany++;
//    }
//    cnt++;
//  }
//
//  if (numrow != row)
//  {
//    general_error(numrow, row, "ROW");
//  }
//
//  // Fill 'em up again
//  fany = ylist[0]>>JF::MPD_bits;
//  if (fany < 0) fany = 0;
//  if (fany >= GMesh.fansHigh()) fany = GMesh.fansHigh()-1;
//  row = 0;
//  while (row < numrow)
//  {
//    cnt = GMesh.getIndexTile(row,fany);
//    run = fanrowrun[row];
//    fanx = 0;
//    while (fanx < run)
//    {
//      if (GMesh.numrenderlist < MAXMESHRENDER)
//      {
//        // Put each tile in basic list
//        GMesh.fan_info[cnt].inrenderlist = true;
//        GMesh.renderlist[GMesh.numrenderlist] = cnt;
//        GMesh.numrenderlist++;
//        // Put each tile in one other list, for shadows and relections
//        if (GMesh.has_flags(cnt, MESHFX_SHA))
//        {
//          GMesh.renderlistsha[GMesh.numrenderlistsha] = cnt;
//          GMesh.numrenderlistsha++;
//        }
//        else
//        {
//          GMesh.renderlistref[GMesh.numrenderlistref] = cnt;
//          GMesh.numrenderlistref++;
//        }
//      }
//      cnt++;
//      fanx++;
//    }
//    row++;
//    fany++;
//  }
//}

//--------------------------------------------------------------------------------------------
void figure_out_what_to_draw()
{
  // ZZ> This function determines the things that need to be drawn

  // test the mesh blocks vs. the view frustum
  if(wldframe%0x07==0)
  {
    int fan_count = GMesh.fansWide() * GMesh.fansHigh();
    for(int i=0; i<fan_count; i++)
    {
      GMesh.m_tile_ex[i].in_view = GCamera.frustum.BBoxInFrustum(GMesh.m_tile_ex[i].bbox.minvals.vals, GMesh.m_tile_ex[i].bbox.maxvals.vals);
    };
  };

  // Request matrices needed for local machine
  make_dolist();
  order_dolist();
}

//--------------------------------------------------------------------------------------------
void animate_tiles()
{
  // This function changes the animated tile frame
  if ((wldframe & GTile_Anim.updateand)==0)
  {
    GTile_Anim.frameadd = (GTile_Anim.frameadd + 1) & GTile_Anim.frameand;
  }
}

//--------------------------------------------------------------------------------------------
void load_basic_textures(char *modname)
{
  // ZZ> This function loads the standard textures for a module
  char newloadname[0x0100];

  // Particle sprites
  TxList.load_one("basicdat/prt512.png", COLORKEY_INVALID, TX_PRT);

  // Module background tiles
  make_newloadname(modname, "gamedat/tile0.bmp", newloadname);
  TxList.load_one(newloadname, COLORKEY_TRANS, TX_TILE0);

  make_newloadname(modname, "gamedat/tile1.bmp", newloadname);
  TxList.load_one(newloadname, COLORKEY_TRANS, TX_TILE1);

  make_newloadname(modname, "gamedat/tile2.bmp", newloadname);
  TxList.load_one(newloadname, COLORKEY_TRANS, TX_TILE2);

  make_newloadname(modname, "gamedat/tile3.bmp", newloadname);
  TxList.load_one(newloadname, COLORKEY_TRANS, TX_TILE3);

  // Water textures
  make_newloadname(modname, "gamedat/watertop.bmp", newloadname);
  TxList.load_one(newloadname, COLORKEY_TRANS, TX_WATERTOP);            // This is also used as foreground

  make_newloadname(modname, "gamedat/waterlow.bmp", newloadname);
  TxList.load_one(newloadname, COLORKEY_TRANS, TX_WATERLOW);            // This is also used as far background

  //if (phongon)
  //{
    // phong map
    make_newloadname(modname, "gamedat/phong.bmp", newloadname);
    TxList.load_one(newloadname, COLORKEY_TRANS, TX_PHONG);
  //}

}


//--------------------------------------------------------------------------------------------
Uint32 Profile_List::load_one_object(char* tmploadname, int & skin)
{
  // ZZ> This function loads one model and returns the number of skins
  int newskins, newicons;
  char newloadname[0x0100];
  char wavename[0x0100];
  int cnt;

  //we have a bit of a KLUDGE here. Should be reading the object type from spawn.txt
  // instead of using slots in this way...

  Uint32 prof_idx = get_free();
  if(INVALID == prof_idx) return prof_idx;

  Profile & rprof = ProfileList[prof_idx];
  strncpy(rprof.filename, tmploadname, 0x0100);

  // Load the model data file and get the model number
  make_newloadname(tmploadname, "/data.txt", newloadname);
  rprof.cap_ref = CapList.load_one_cap(newloadname);
  if(Cap_List::INVALID!=rprof.cap_ref.index)
  {
    strncpy(CapList[rprof.cap_ref].filename, tmploadname, 0x0100);
    CapList[rprof.cap_ref].loaded = true;
  }

  Sint32 slot = CapList[rprof.cap_ref].slot;

  if(slot < 0)
  {
    return_one(prof_idx);
    CapList.return_one(rprof.cap_ref.index);
    return INVALID;
  }

  // link a given "slot" to a given profile for spawning
  slot_list[slot] = prof_idx;


  // Make up a name for the model...  IMPORT\TEMP0000.OBJ
  strncpy(rprof.name, tmploadname, 126);

  // Append a slash to the tmploadname
  sprintf(newloadname, "%s", tmploadname);
  sprintf(tmploadname, "%s/", newloadname);

  // Load the AI script for this model
  make_newloadname(tmploadname, "script.txt", newloadname);
  rprof.scr_ref = ScrList.load_ai_script(newloadname);

  //printf(" DIAG: load model model\n");
  // Load the model model
  rprof.mad_ref = MadList.load_one_mad(tmploadname);
  if(Mad_List::INVALID!=rprof.mad_ref.index)
  {
    strncpy(MadList[rprof.mad_ref].filename, tmploadname, 0x0100);
    MadList[rprof.mad_ref].loaded = true;
  }

  //printf(" DIAG: fixing lighting\n");
  // Fix lighting if need be
  if (CapList[rprof.cap_ref].uniformlit)
  {
    make_md2_equally_lit(rprof.mad_ref);
  }

  // Load the messages for this model
  make_newloadname(tmploadname, "message.txt", newloadname);
  rprof.load_all_messages(newloadname);

  //printf(" DIAG: doing random naming\n");
  // Load the random naming table for this model
  rprof.read_naming(tmploadname);

  // Load the particles for this model
  rprof.load_pips(tmploadname);


  //printf(" DIAG: loading waves\n");
  // Load the waves for this model
  for (cnt = 0; VALID_WAVE_RANGE(cnt); cnt++)
  {
    sprintf(wavename, "sound%d.wav", cnt);
    make_newloadname(tmploadname, wavename, newloadname);
    rprof.waveindex[cnt] = Mix_LoadWAV(newloadname);
  }

  //printf(" DIAG: loading enchantments\n");
  // Load the enchantment for this model
  rprof.eve_ref = EveList.load_one_eve(tmploadname, rprof.cap_ref.index);
  if(Eve_List::INVALID!=rprof.eve_ref.index)
  {
    strncpy(EveList[rprof.eve_ref].filename, tmploadname, 0x0100);
    EveList[rprof.eve_ref].loaded = true;
  }

  //printf(" DIAG: loading skins and icons (PORTED EXCEPT ALPHA)\n");
  // Load the skins and icons

  // blank the skins and icons
  //for(int i=0; i<4; i++)
  //{
  //  rprof.skin_idx[i] = Tx_List::INVALID;
  //  rprof.icon_ref[i] = Icon_List::INVALID;
  //};

  //grab the existing icons, etc.
  newskins = 0;
  newicons = 0;
  Uint32 lastskin = TX_WATERTOP;
  Uint32 lasticon = IconList.null.index;
  for(int i=0; i<4; i++)
  {
    sprintf(newloadname, "%stris%d.bmp", tmploadname, i);
    TEX_REF tx_ref = TxList.load_one(newloadname, COLORKEY_TRANS);
    if ( Tx_List::INVALID != tx_ref.index)
    {
      lastskin = tx_ref.index;
      while (newskins <= i)
      {
        rprof.skin_ref[newskins] = tx_ref;
        newskins++;
      }
    }

    sprintf(newloadname, "%sicon%d.bmp", tmploadname, i);
    ICON_REF icon_ref = IconList.load_one_icon(newloadname);
    if ( Icon_List::INVALID != icon_ref.index)
    {
      lasticon = icon_ref.index;
      if (slot==SPELLBOOK) IconList.book_icon[IconList.book_count++] = icon_ref;

      while (newicons <= i)
      {
        rprof.icon_ref[newicons] = icon_ref;
        newicons++;
      }
    }
  }

  while (newskins < 4)
  {
    rprof.skin_ref[newskins] = lastskin;
    newskins++;
  }

  while (newicons < 4)
  {
    rprof.icon_ref[newicons] = lasticon;
    newicons++;
  }

  rprof.loaded = true;

  return prof_idx;
}


//--------------------------------------------------------------------------------------------
void Cap_List::free_import_slots()
{
  for (int cnt = 0; cnt<MODEL_COUNT; cnt++)
    m_slotlist[cnt] = 10000;
}

//--------------------------------------------------------------------------------------------
void Profile_List::load_all_objects(char *modname)
{
  // ZZ> This function loads a module's objects
  const char *filehandle;
  bool keeplooking;
  FILE* fileread;
  char newloadname[0x0100];
  char filename[0x0100];
  int cnt;
  int skin;
  int importplayer;

  // Log all of the script errors
  //printf(" DIAG: opening ParseErr\n");
  globalparseerr = fopen("basicdat/ParseErr.txt", "w");
  parseerror = false;
  fprintf(globalparseerr, "This file documents typos found in the AI scripts...\n");

  // Clear the import slots...
  CapList.free_import_slots();

  // Load the import directory
  //printf(" DIAG: loading inport dir\n");
  importplayer = -1;
  skin = 8;  // Character skins start at 8...  Trust me
  if (importvalid)
  {
    for (cnt = 0; cnt < MAXIMPORT; cnt++)
    {
      sprintf(filename, "import/temp%04d.obj", cnt);
      // Make sure the object exists...
      sprintf(newloadname, "%s/data.txt", filename);
      fileread = fopen(newloadname, "r");
      if (fileread)
      {
        //printf("Found import slot %04d\n", cnt);

        fclose(fileread);
        // Load it...
        if ((cnt % 9) == 0)
        {
          importplayer++;
        }
        CapList.set_import( ((importplayer)*9)+(cnt%9), cnt );
        ProfileList.load_one_object(filename, skin);
      }
    }
  }
  //printf(" DIAG: emptying directory\n");
  //empty_import_directory();  // Free up that disk space...

  // Search for .obj directories and load them
  //printf(" DIAG: Searching for .objs\n");
  CapList.m_importobject = -100;
  make_newloadname(modname, "objects/", newloadname);
  filehandle= fs_findFirstFile(newloadname, "obj");

  keeplooking = 1;
  if (filehandle!= NULL)
  {
    while (keeplooking)
    {
      //printf(" DIAG: keeplooking\n");
      sprintf(filename, "%s%s", newloadname, filehandle);
      ProfileList.load_one_object(filename, skin);

      filehandle = fs_findNextFile();

      keeplooking = (filehandle != NULL);
    }
  }
  fs_findClose();
  //printf(" DIAG: Done Searching for .objs\n");
  fclose(globalparseerr);
}

//--------------------------------------------------------------------------------------------
void load_bars(char* szBitmap)
{
  // ZZ> This function loads the status bar bitmap
  int cnt;

  GLTexture::Load(&TxBars, szBitmap, 0);
  if (&TxBars == NULL)
    general_error(0, 0, "NO BARS!!!");


  // Make the blit rectangles
  for (cnt = 0; cnt < NUMBAR; cnt++)
  {
    tabrect[cnt].left = 0;
    tabrect[cnt].right = TABX;
    tabrect[cnt].top = cnt*BARY;
    tabrect[cnt].bottom = (cnt+1)*BARY;
    barrect[cnt].left = TABX;
    barrect[cnt].right = BARX;  // This is reset whenever a bar is drawn
    barrect[cnt].top = tabrect[cnt].top;
    barrect[cnt].bottom = tabrect[cnt].bottom;
  }

  // Set the transparent color
  //DDSetColorKey(lpDDSBars, 0); port to new alpha code
}

//--------------------------------------------------------------------------------------------
void load_map(char* szModule, int sysmem)
{
  // ZZ> This function loads the map bitmap and the blip bitmap
  char szMap[0x0100];

  // Turn it all off
  Blip::mapon = false;
  Blip::youarehereon = false;
  Blip::numblip = 0;

  // Load the images
  sprintf(szMap, "%sgamedat/plan.bmp", szModule);
  if (GLTexture::INVALID == GLTexture::Load(&TxMap, szMap))
    general_error(0, 0, "NO MAP!!!");

  // Set up the rectangles
  maprect.left   = 0;
  maprect.right  = MAPSIZE;
  maprect.top    = 0;
  maprect.bottom = MAPSIZE;

}

//--------------------------------------------------------------------------------------------
void make_water()
{
  // ZZ> This function sets up water movements
  int layer, frame, cnt;
  float temp;


  for (layer = 0; layer < WaterList.layer_count; layer++)
  {
    if (WaterList.is_light)  WaterList[layer].alpha = 0xFF; // Some cards don't support alpha lights...
    WaterList[layer].off = vec2_t(0,0);

    for (frame = 0; frame < MAXWATERFRAME; frame++)
    {
      for (int ix=0; ix<2; ix++)
      {
        for (int iy=0; iy<2; iy++)
        {
          temp = sin((frame*2*PI/MAXWATERFRAME)+(PI*ix/WATERPOINTS)+(PI*iy/WATERPOINTS)+(PI*layer/MAXWATERLAYER));
          WaterList[layer].z_add[frame][ix][iy] = temp*WaterList[layer].amp;
          WaterList[layer].color[frame][ix][iy] = (WaterList[layer].light_level*(temp+1.0f)/2.0f) + WaterList[layer].light_add;
        }
      }
    };
  }

  // Calculate specular highlights
  // [claforte] Probably need to replace this with a
  //            glColor4f(spek/float(0x0100), spek/float(0x0100), spek/float(0x0100), 1.0f) call:

  if (shading == GL_FLAT)
  {
    for (cnt = 0; cnt < 0x0100; cnt++)
    {
      WaterList.spek[cnt] = 0;
    }
  }
  else
  {
    for (cnt = 0; cnt < 0x0100; cnt++)
    {
      if (cnt > WaterList.spek_start)
      {
        temp = (cnt-WaterList.spek_start)/(0x0100-WaterList.spek_start);
        WaterList.spek[cnt] = temp*temp*WaterList.spek_level;
      }
      else
      {
        WaterList.spek[cnt] = 0;
      };
    };
  }
}

//--------------------------------------------------------------------------------------------
void read_wawalite(char *modname)
{
  // ZZ> This function sets up water and lighting for the module
  char newloadname[0x0100];
  FILE* fileread;
  int iTmp;

  make_newloadname(modname, "gamedat/wawalite.txt", newloadname);
  fileread = fopen(newloadname, "r");
  if (fileread)
  {
    goto_colon(fileread);
    //  !!!BAD!!!
    //  Random map...
    //  If someone else wants to handle this, here are some thoughts for approaching
    //  it.  The .MPD file for the level should give the basic size of the map.  Use
    //  a standard tile set like the Palace modules.  Only use objects that are in
    //  the module's object directory, and only use some of them.  Imagine several Rock
    //  Moles eating through a stone filled level to make a path from the entrance to
    //  the exit.  Door placement will be difficult.
    //  !!!BAD!!!

    // Read water data first
    WaterList.layer_count = get_next_int(fileread);
    WaterList.spek_start = get_next_int(fileread);
    WaterList.spek_level = get_next_int(fileread);
    WaterList.level_douse = get_next_int(fileread);
    WaterList.level_surface = get_next_int(fileread);
    WaterList.is_light = get_next_bool(fileread);
    WaterList.is_water = get_next_bool(fileread);
    overlay_on = get_next_bool(fileread);
    clearson = !get_next_bool(fileread);
    WaterList[0].dist.x = get_next_float(fileread);
    WaterList[0].dist.y = get_next_float(fileread);
    WaterList[1].dist.x = get_next_float(fileread);
    WaterList[1].dist.y = get_next_float(fileread);
    WaterList.foreground_repeat = get_next_int(fileread);
    WaterList.background_repeat = get_next_int(fileread);

    WaterList[0].z = get_next_int(fileread);
    WaterList[0].alpha = get_next_int(fileread);
    WaterList[0].frame_add = get_next_int(fileread);
    WaterList[0].light_level = get_next_int(fileread);
    WaterList[0].light_add = get_next_int(fileread);
    WaterList[0].amp = get_next_float(fileread);
    WaterList[0].off_add.s = get_next_float(fileread);
    WaterList[0].off_add.t = get_next_float(fileread);

    WaterList[1].z = get_next_int(fileread);
    WaterList[1].alpha = get_next_int(fileread);
    WaterList[1].frame_add = get_next_int(fileread);
    WaterList[1].light_level = get_next_int(fileread);
    WaterList[1].light_add = get_next_int(fileread);
    WaterList[1].amp = get_next_float(fileread);
    WaterList[1].off_add.s = get_next_float(fileread);
    WaterList[1].off_add.t = get_next_float(fileread);

    WaterList[0].off =vec2_t(0,0);
    WaterList[1].off =vec2_t(0,0);
    WaterList[0].frame = rand()&WATERFRAMEAND;
    WaterList[1].frame = rand()&WATERFRAMEAND;

    // Read light data second
    WaterList.lx = get_next_float(fileread);
    WaterList.ly = get_next_float(fileread);
    WaterList.lz = get_next_float(fileread);
    WaterList.la = get_next_float(fileread);

    //float norm = sqrtf(WaterList.lx*WaterList.lx + WaterList.ly*WaterList.ly + WaterList.lz*WaterList.lz);
    //WaterList.lx /= norm;
    //WaterList.ly /= norm;
    //WaterList.lz /= norm;

    // Read tile data third
    GPhys.fric_hill = get_next_float(fileread);
    GPhys.fric_slip = get_next_float(fileread);
    GPhys.fric_air = get_next_float(fileread);
    GPhys.fric_h2o = get_next_float(fileread);
    GPhys.fric_stick = get_next_float(fileread);
    GPhys.gravity = get_next_float(fileread);
    GTile_Anim.updateand = get_next_int(fileread);
    GTile_Anim.frameand = get_next_int(fileread);
    GTile_Anim.bigframeand = (GTile_Anim.updateand<<1)+1;
    GTile_Dam.amount = get_next_int(fileread);
    goto_colon(fileread);  GTile_Dam.type = get_damage_type(fileread);

    // Read weather data fourth
    GWeather.overwater = get_next_bool(fileread);
    GWeather.time_reset = get_next_int(fileread);
    GWeather.time = GWeather.time_reset;
    GWeather.player = 0;
    // Read extra data
    GMesh.exploremode = get_next_bool(fileread);
    use_faredge = get_next_bool(fileread);
    GCamera.swing = 0;
    GCamera.swing_rate = get_next_float(fileread);
    GCamera.swing_amp = get_next_float(fileread);

    // Read unnecessary data...  Only read if it exists...
    GFog.on = false;
    GFog.affectswater = true;
    GFog.top = 100;
    GFog.bottom = 0;
    GFog.distance = 100;
    GFog.red = 0xFF;
    GFog.grn = 0xFF;
    GFog.blu = 0xFF;
    GTile_Dam.parttype = -1;
    GTile_Dam.partand = 0xFF;
    GTile_Dam.sound = -1;
    GTile_Dam.soundtime = TILESOUNDTIME;
    GTile_Dam.mindistance = 9999;
    if (goto_colon_yesno(fileread))
    {
      GFog.on = GFog.allowed;
      GFog.top = get_float(fileread);
      GFog.bottom = get_next_float(fileread);
      GFog.red = 0xFF*get_next_float(fileread);
      GFog.grn = 0xFF*get_next_float(fileread);
      GFog.blu = 0xFF*get_next_float(fileread);
      GFog.affectswater = get_next_bool(fileread);
      GFog.distance = (GFog.top-GFog.bottom);
      if (GFog.distance < 1.0)  GFog.on = false;

      // Read extra stuff for damage tile particles...
      if (goto_colon_yesno(fileread))
      {
        GTile_Dam.parttype = get_int(fileread);
        GTile_Dam.partand  = get_next_int(fileread);
        GTile_Dam.sound    = get_next_int(fileread);
      }
    }

    // Allow slow machines to ignore the fancy stuff
    if (!twolayerwateron && WaterList.layer_count > 1)
    {
      WaterList.layer_count = 1;
      iTmp = WaterList[0].alpha;
      iTmp = ((WaterList[1].alpha*iTmp)>>FIXEDPOINT_BITS) + iTmp;
      if (iTmp > 0xFF) iTmp = 0xFF;
      WaterList[0].alpha = iTmp;
    }

    fclose(fileread);

    // Do it
    make_lighttospek();
    make_water();
  }
  else
  {
    general_error(0, 0, "WAWALITE.TXT NOT READ");
  }
}

//--------------------------------------------------------------------------------------------
void render_background(TEX_REF & texture)
{
  // ZZ> This function draws the large background
  GLVertex vtlist[4];
  Uint32 light;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  vec3_t pos;
  vec2_t off;
  int i;

  // Flat shade this
  //if(shading != D3DSHADE_FLAT)
  //    lpD3DDDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
  // Wait until there's a way to check current - DDOI
  //    glShadeModel (GL_FLAT);

  TxList[texture].Bind(GL_TEXTURE_2D);

  // Figure out the screen coordinates of its corners
  pos.x = scrx/2.0;
  pos.y = scry/2.0;
  pos.z = .99999;
  off = WaterList[1].off;
  rotate = 0x4000 + 0x2000 - (int(GCamera.turn_lr)<<2);
  rotate = rotate>>2;
  size = pos.x + pos.y + 1;
  sinsize = sin_tab[rotate]*size;
  cossize = cos_tab[rotate]*size;

  light = (0xffffffff);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  vtlist[0].pos.x = pos.x + cossize;
  vtlist[0].pos.y = pos.y - sinsize;
  vtlist[0].pos.z = pos.z;
  //vtlist[0].dcSpecular = 0;
  vtlist[0].txcoord.s = 0+off.s;
  vtlist[0].txcoord.t = 0+off.t;

  vtlist[1].pos.x = pos.x + sinsize;
  vtlist[1].pos.y = pos.y + cossize;
  vtlist[1].pos.z = pos.z;
  //vtlist[1].dcSpecular = 0;
  vtlist[1].txcoord.s = WaterList.background_repeat+off.s;
  vtlist[1].txcoord.t = 0+off.t;

  vtlist[2].pos.x = pos.x - cossize;
  vtlist[2].pos.y = pos.y + sinsize;
  vtlist[2].pos.z = pos.z;
  //vtlist[2].dcSpecular = 0;
  vtlist[2].txcoord.s = WaterList.background_repeat+off.s;
  vtlist[2].txcoord.t = WaterList.background_repeat+off.t;

  vtlist[3].pos.x = pos.x - sinsize;
  vtlist[3].pos.y = pos.y - cossize;
  vtlist[3].pos.z = pos.z;
  //vtlist[3].dcSpecular = 0;
  vtlist[3].txcoord.s = 0+off.s;
  vtlist[3].txcoord.t = WaterList.background_repeat+off.t;

  glBegin(GL_TRIANGLE_FAN);
  for (i = 0; i < 4; i++)
  {
    glTexCoord2fv(vtlist[i].txcoord.vals);
    glVertex3fv(vtlist[i].pos.vals);
  }
  glEnd();
}

/* TODO: Implement this below
//--------------------------------------------------------------------------------------------
void render_foreground_overlay(Uint16 texture)
{
//  GLVertex vtlist[4];
float size;
float sinsize, cossize;
float x, y, z, u, v;
int i;

float loc_foregroundrepeat;

// Figure out the screen coordinates of its corners
x = scrx/2.0;
y = scry/2.0;
z = .99999;
u = WaterList[1].u;
v = WaterList[1].v;
size = x + y + 1;
sinsize = sin_tab[rotate]*size;
cossize = cos_tab[rotate]*size;

loc_foregroundrepeat = WaterList.foreground_repeat * min(x/scrx, y/scrx) / 4.0;

vtlist[0].x = x + cossize;
vtlist[0].y = y - sinsize;
vtlist[0].z = z;
vtlist[0].s = 0+u;
vtlist[0].t = 0+v;

vtlist[1].x = x + sinsize;
vtlist[1].y = y + cossize;
vtlist[1].z = z;
vtlist[1].s = loc_foregroundrepeat+u;
vtlist[1].t = v;

vtlist[2].x = x - cossize;
vtlist[2].y = y + sinsize;
vtlist[2].z = z;
vtlist[2].s = loc_foregroundrepeat+u;
vtlist[2].t = loc_foregroundrepeat+v;

vtlist[3].x = x - sinsize;
vtlist[3].y = y - cossize;
vtlist[3].z = z;
vtlist[3].s = u;
vtlist[3].t = loc_foregroundrepeat+v;

//-------------------------------------------------
glPushAttrib(GL_LIGHTING_BIT|GL_DEPTH_BUFFER_BIT|GL_HINT_BIT);

glHint(GL_POINT_SMOOTH_HINT, GL_NICEST); // make sure that the texture is as smooth as possible

glShadeModel(GL_FLAT); // Flat shade this

glDepthMask (GL_FALSE); // do not write into the depth buffer
glDepthFunc(GL_ALWAYS); // make it appear over the top of everything

glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_COLOR); // make the texture a filter

TxList[texture].Bind(GL_TEXTURE_2D);

glBegin(GL_TRIANGLE_FAN);
glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
for (i = 0; i < 4; i++)
{
glTexCoord2f(vtlist[i].s, vtlist[i].t);

glVertex3f (vtlist[i].x, vtlist[i].y, vtlist[i].z);
}
glEnd();

glPopAttrib();
//-------------------------------------------------
}*/

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_shadow(Character & chr, Particle_List & plist, int type)
{
  // ZZ> This function draws a NIFTY shadow
  GLVertex v[4];

  vec3_t pos;
  float height, size_umbra, size_penumbra;
  float alpha_umbra, alpha_penumbra;
  signed char hide;
  int i;

  hide = chr.getCap().hide_state;
  if(hide == NOHIDE || hide != chr.ai.state)
  {
    // Original points
    pos.z = chr.level;
    pos.z += SHADOWRAISE;
    height = chr.matrix.CNV(3,2) - chr.level;
    if(height<0) height = 0;

    float radius = chr.calc_bump_size/2.0;

    size_umbra    = 1.5*(radius - height/30.0);
    size_penumbra = 1.5*(radius + height/30.0);

    if(height>0)
    {
      float factor_umbra = 1.5*radius/size_umbra;
      alpha_umbra = 0.3/factor_umbra/factor_umbra/1.5;

      float factor_penumbra = 1.5*radius/size_penumbra;
      alpha_penumbra = 0.3/factor_penumbra/factor_penumbra/1.5;
    }
    else
    {
      alpha_umbra    = 0.3;
      alpha_penumbra = 0.3;
    };

    /* PORT:if(g_foginfo.on)
    {
    alpha = (alpha>>1) + 0x40;
    z = (chr.level);
    if(z < g_foginfo.top)
    {
    if(z < g_foginfo.bottom)
    {
    alpha += 80;
    }
    else
    {
    z = 1.0 - ((z-g_foginfo.bottom)/g_foginfo.distance);
    alpha += (80 * z);
    }
    }
    }*/

    pos.x = chr.matrix.CNV(3,0);
    pos.y = chr.matrix.CNV(3,1);

    // Choose texture.
    TxList[plist.texture].Bind(GL_TEXTURE_2D);

    if(type == 0)
    {
      //BAD SHADOW
      v[0].txcoord.s = plist.txcoord[236][0].s;
      v[0].txcoord.t = plist.txcoord[236][0].t;

      v[1].txcoord.s = plist.txcoord[253][1].s;
      v[1].txcoord.t = plist.txcoord[236][0].t;

      v[2].txcoord.s = plist.txcoord[253][1].s;
      v[2].txcoord.t = plist.txcoord[253][1].t;

      v[3].txcoord.s = plist.txcoord[236][0].s;
      v[3].txcoord.t = plist.txcoord[253][1].t;
    }
    else
    {
      //GOOD SHADOW

      v[0].txcoord.s = plist.txcoord[238][0].s;
      v[0].txcoord.t = plist.txcoord[238][0].t;

      v[1].txcoord.s = plist.txcoord[255][1].s;
      v[1].txcoord.t = plist.txcoord[238][0].t;

      v[2].txcoord.s = plist.txcoord[255][1].s;
      v[2].txcoord.t = plist.txcoord[255][1].t;

      v[3].txcoord.s = plist.txcoord[238][0].s;
      v[3].txcoord.t = plist.txcoord[255][1].t;
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    glDepthMask(GL_FALSE);

    if(size_penumbra>0)
    {
      v[0].pos.x = pos.x+size_penumbra;
      v[0].pos.y = pos.y-size_penumbra;
      v[0].pos.z = pos.z;

      v[1].pos.x = pos.x+size_penumbra;
      v[1].pos.y = pos.y+size_penumbra;
      v[1].pos.z = pos.z;

      v[2].pos.x = pos.x-size_penumbra;
      v[2].pos.y = pos.y+size_penumbra;
      v[2].pos.z = pos.z;

      v[3].pos.x = pos.x-size_penumbra;
      v[3].pos.y = pos.y-size_penumbra;
      v[3].pos.z = pos.z;

      glBegin(GL_TRIANGLE_FAN);
      glColor4f(alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2fv (v[i].txcoord.vals);
        glVertex3fv (v[i].pos.vals);
      }
      glEnd();
    };

    if(size_umbra>0)
    {
      v[0].pos.x = pos.x+size_umbra;
      v[0].pos.y = pos.y-size_umbra;
      v[0].pos.z = pos.z+0.1;

      v[1].pos.x = pos.x+size_umbra;
      v[1].pos.y = pos.y+size_umbra;
      v[1].pos.z = pos.z+0.1;

      v[2].pos.x = pos.x-size_umbra;
      v[2].pos.y = pos.y+size_umbra;
      v[2].pos.z = pos.z+0.1;

      v[3].pos.x = pos.x-size_umbra;
      v[3].pos.y = pos.y-size_umbra;
      v[3].pos.z = pos.z+0.1;

      glBegin(GL_TRIANGLE_FAN);
      glColor4f(alpha_umbra, alpha_umbra, alpha_umbra, 1.0);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2fv (v[i].txcoord.vals);
        glVertex3fv (v[i].pos.vals);
      }
      glEnd();
    };

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
  };
}

//--------------------------------------------------------------------------------------------
void render_bad_shadow(int character)
{
  // ZZ> This function draws a sprite shadow
  GLVertex v[4];
  float size;
  vec3_t pos;
  Uint8 ambi;
  int height;
  Sint8 hide;
  Uint8 trans;
  int i;

  hide = ChrList[character].getCap().hide_state;
  if (hide == NOHIDE || hide != ChrList[character].ai.state)
  {
    // Original points
    pos.z = ChrList[character].level;
    pos.z += SHADOWRAISE;
    height = ChrList[character].matrix.CNV(3,2) - pos.z;
    if (height > 0xFF)  return;
    if (height < 0) height = 0;
    size = ChrList[character].shadow_size-((height*ChrList[character].shadow_size)>>FIXEDPOINT_BITS);
    if (size < 1) return;
    ambi = ChrList[character].light_level>>4;  // LUL >>3;
    trans = ((0xFF-height)>>1)+0x40;
    /*      if(GFog.on)
    {
    z = (ChrList[character].level);
    if(z < GFog.top)
    {
    if(z > GFog.bottom)
    {
    z = ((z-GFog.bottom)/GFog.distance);
    trans = (trans * z);
    if(trans < 0x40)  trans = 0x40;
    }
    else
    {
    trans = 0x40;
    }
    }
    }*/
    //light = (trans<<24) | (ambi<<16) | (ambi<<8) | ambi;
    glColor4f(ambi/float(0xFF), ambi/float(0xFF), ambi/float(0xFF), trans/float(0xFF));

    pos.x = ChrList[character].matrix.CNV(3,0);
    pos.y = ChrList[character].matrix.CNV(3,1);
    v[0].pos.x = (float) pos.x+size;
    v[0].pos.y = (float) pos.y-size;
    v[0].pos.z = (float) pos.z;

    v[1].pos.x = (float) pos.x+size;
    v[1].pos.y = (float) pos.y+size;
    v[1].pos.z = (float) pos.z;

    v[2].pos.x = (float) pos.x-size;
    v[2].pos.y = (float) pos.y+size;
    v[2].pos.z = (float) pos.z;

    v[3].pos.x = (float) pos.x-size;
    v[3].pos.y = (float) pos.y-size;
    v[3].pos.z = (float) pos.z;

    // Choose texture and matrix
    TxList[PrtList.texture].Bind(GL_TEXTURE_2D);

    v[0].txcoord.s = PrtList.txcoord[236][0].s;
    v[0].txcoord.t = PrtList.txcoord[236][0].t;

    v[1].txcoord.s = PrtList.txcoord[253][1].s;
    v[1].txcoord.t = PrtList.txcoord[236][0].t;

    v[2].txcoord.s = PrtList.txcoord[253][1].s;
    v[2].txcoord.t = PrtList.txcoord[253][1].t;

    v[3].txcoord.s = PrtList.txcoord[236][0].s;
    v[3].txcoord.t = PrtList.txcoord[253][1].t;

    glBegin(GL_TRIANGLE_FAN);
    for (i = 0; i < 4; i++)
    {
      glTexCoord2fv(v[i].txcoord.vals);
      glVertex3fv(v[i].pos.vals);
    }
    glEnd();
  }
}

//--------------------------------------------------------------------------------------------

void light_characters(Character_List & clist, Mesh & msh)
{
  // ZZ> This function figures out character lighting
  int cnt, tnc;
  Sint16 tl, tr, bl, br;

  for (cnt = 0; cnt < numdolist; cnt++)
  {
    tnc = dolist[cnt];

    if( INVALID_CHR(tnc) ) continue;

    Character & rchr = clist[tnc];

    int fan  = GMesh.getIndexPos(rchr.pos.x,rchr.pos.y);
    if(JF::MPD_IndexInvalid == fan) continue;

    rchr.onwhichfan = fan;

    int vert = msh.getFan(fan)->firstVertex;
    JF::MPD_Vertex * vlist = (JF::MPD_Vertex *)msh.getVertices();

    if(msh.exploremode)
    {
      tl = vlist[vert+0].ambient;
      tr = vlist[vert+1].ambient;
      br = vlist[vert+2].ambient;
      bl = vlist[vert+3].ambient;
    }
    else
    {
      tl = vlist[vert+0].light;
      tr = vlist[vert+1].light;
      br = vlist[vert+2].light;
      bl = vlist[vert+3].light;
    }

    int tmp = MIN(MIN(tl,tr),MIN(br,bl));

    rchr.light_x = ((tr+br) - (tl+bl))/2;
    rchr.light_y = ((tl+tr) - (bl+br))/2;
    rchr.light_a = tmp + MAX(0,(tl + tr + bl + br)/4 - ABS(rchr.light_x) - ABS(rchr.light_y) );
  }
}

void light_character(Character & rchr, Mesh & msh)
{
  // ZZ> This function figures out character lighting
  int cnt, tnc;
  Sint16 tl, tr, bl, br;

  rchr.light_x = 0;
  rchr.light_y = 0;
  rchr.light_a = 0;

  int fan  = GMesh.getIndexPos(rchr.pos.x,rchr.pos.y);
  if(JF::MPD_IndexInvalid == fan) return;
  rchr.onwhichfan = fan;

  int vert = msh.getFan(fan)->firstVertex;
  JF::MPD_Vertex * vlist = (JF::MPD_Vertex *)msh.getVertices();

  if(msh.exploremode)
  {
    tl = vlist[vert+0].ambient;
    tr = vlist[vert+1].ambient;
    br = vlist[vert+2].ambient;
    bl = vlist[vert+3].ambient;
  }
  else
  {
    tl = vlist[vert+0].light;
    tr = vlist[vert+1].light;
    br = vlist[vert+2].light;
    bl = vlist[vert+3].light;
  }

  int tmp = MIN(MIN(tl,tr),MIN(br,bl));

  rchr.light_x = ((tr+br) - (tl+bl))>>1;
  rchr.light_y = ((tl+tr) - (bl+br))>>1;
  rchr.light_a = tmp + MAX(0,((tl + tr + bl + br)>>2) - ABS(rchr.light_x) - ABS(rchr.light_y) );
}

//--------------------------------------------------------------------------------------------
void light_particles(Particle_List & plist, Mesh & msh)
{
  // ZZ> This function figures out particle lighting
  int cnt;

  const JF::MPD_Fan * pfan;
  JF::MPD_Vertex    * vlist = (JF::MPD_Vertex *)msh.getVertices();

  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    pfan = msh.getFanPos( rprt_cnt.pos.x, rprt_cnt.pos.y );
    if(NULL!=pfan)
    {
      int vert = pfan->firstVertex;

      rprt_cnt.light  = vlist[vert+0].light;
      rprt_cnt.light += vlist[vert+1].light;
      rprt_cnt.light += vlist[vert+2].light;
      rprt_cnt.light += vlist[vert+3].light;

      Uint32 character = rprt_cnt.attachedtocharacter;
      if (VALID_CHR(character))
      {
        rprt_cnt.light += ChrList[character].light_level;
      }
    };

  }SCAN_PRT_END;
}

//--------------------------------------------------------------------------------------------
void do_dyna_light_mesh(Mesh & msh, Particle_List & plist)
{
  // ZZ> This function does dynamic lighting of visible fans

  int cnt, lastvertex, vertex, fan, entry, fanx, fany, addx, addy;
  float light;

  JF::MPD_Vertex * vrtlist = (JF::MPD_Vertex *)msh.getVertices();

  //Don't need to do every frame
  if((allframe & 7)!=0) return;

  // Do each floor tile
  if (msh.exploremode)
  {
    // Set base light level in explore mode...  
    {

      for (cnt = 0; cnt < DynaList.count; cnt++)
      {
        fanx = int(DynaList[cnt].pos.x)>>JF::MPD_bits;
        fany = int(DynaList[cnt].pos.y)>>JF::MPD_bits;

        for (addy = -DYNAFANS; addy <= DYNAFANS; addy++)
        {
          for (addx = -DYNAFANS; addx <= DYNAFANS; addx++)
          {
            set_fan_light(fanx+addx, fany+addy, DynaList[cnt]);
          }
        }
      };
    }
  }
  else
  {
    //Prime the tiles
    for (entry = 0; entry < GRenderlist.all_count; entry++)
    {
      fan = GRenderlist[entry].all;
      vertex = msh.getFan(fan)->firstVertex;

      lastvertex = vertex + msh.getFanType(fan)->numVertices;
      for ( /* nothing */; vertex < lastvertex; vertex++)
      {
        vrtlist[vertex].light = vrtlist[vertex].ambient;
      }
    }

    // do the dynamic lights
    for (cnt = 0; cnt < DynaList.count; cnt++)
    {
      fanx = int(DynaList[cnt].pos.x)>>JF::MPD_bits;
      fany = int(DynaList[cnt].pos.y)>>JF::MPD_bits;

      for (addy = -DYNAFANS; addy <= DYNAFANS; addy++)
      {
        for (addx = -DYNAFANS; addx <= DYNAFANS; addx++)
        {
          set_fan_light(fanx+addx, fany+addy, DynaList[cnt]);
        }
      }
    };
  }

}

//--------------------------------------------------------------------------------------------
void render_water(Camera & cam)
{
  // ZZ> This function draws all of the water fans

  int cnt;

  //// Set the transformation thing
  //glLoadMatrixf(cam.mView.v);
  //glMultMatrixf(cam.mWorld.v);

  if(!overlay_on)
  {
    // render bottom to top
    for(int i = WaterList.layer_count; i>=0; i--)
    {
      for (cnt = 0; cnt < GRenderlist.wat_count; cnt++)
      {
        int fan = GRenderlist[cnt].wat;
        render_water_fan(GMesh, fan, i);
      }
    }
  }
  else
  {
    //      render_foreground_overlay(TX_WATERTOP);  // Texture 5 is watertop.bmp
  }
}

//--------------------------------------------------------------------------------------------
void draw_scene_sadreflection(Camera & cam)
{
  // ZZ> This function draws 3D objects
  Uint16 cnt, tnc;
  Uint8 trans;
  rect_t rect;// = {0, 0, scrx, scry}; // Don't know why this isn't working on the Mac, it should

  rect.left = 0;
  rect.right = 0;
  rect.top = scrx;
  rect.bottom = scry;

  // ZB> Clear the z-buffer
  glClear(GL_DEPTH_BUFFER_BIT);

  // Clear the image if need be
  if (clearson)
    glClear(GL_COLOR_BUFFER_BIT);
  else
  {
    // Render the background
    render_background( TEX_REF(TX_WATERLOW) );  // 6 is the texture for waterlow.bmp
  }

  // Render the reflective floors
  glDisable(GL_CULL_FACE); // glEnable(GL_CULL_FACE);
  glFrontFace(GL_CW);
  GMesh.txref_last = Tx_List::INVALID;
  for (cnt = 0; cnt < GRenderlist.ref_count; cnt++)
  {
    if(GMesh.m_tile_ex[GRenderlist[cnt].ref].in_view)
      render_fan(GMesh, GRenderlist[cnt].ref );
  }

  if (refon)
  {
    // Render reflections of characters

    glDisable(GL_CULL_FACE); // glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (cnt = 0; cnt < numdolist; cnt++)
    {
      tnc = dolist[cnt];
      if (GMesh.has_flags(ChrList[tnc].onwhichfan, MESHFX_DRAWREF))
      {
        render_refmad(tnc, ChrList[tnc].alpha&ChrList[tnc].light);
      }
    }

    // Render the reflected sprites
    glFrontFace(GL_CW);
    render_refprt();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
  }

  // Render the shadow floors
  GMesh.txref_last = Tx_List::INVALID;

  for (cnt = 0; cnt < GRenderlist.sha_count; cnt++)
  {
    if(GMesh.m_tile_ex[GRenderlist[cnt].sha].in_view)
      render_fan(GMesh, GRenderlist[cnt].sha );
  }

  // Render the shadows
  if (shaon)
  {
    if (shasprite)
    {
      // Bad shadows
      glDepthMask(GL_FALSE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      for (cnt = 0; cnt < numdolist; cnt++)
      {
        tnc = dolist[cnt];
        if (INVALID_CHR(ChrList[tnc].held_by))
        {
          if (((ChrList[tnc].light==0xFF && ChrList[tnc].alpha==0xFF) || ChrList[tnc].getCap().forceshadow) && ChrList[tnc].shadow_size!=0)
          {
            render_bad_shadow(tnc);
          }
        }
      }

      glDisable(GL_BLEND);
      glDepthMask(GL_TRUE);
    }
    else
    {
      // Good shadows for me
      glDepthMask(GL_FALSE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_COLOR, GL_ZERO);
      for (cnt = 0; cnt < numdolist; cnt++)
      {
        tnc = dolist[cnt];
        if (INVALID_CHR(ChrList[tnc].held_by))
        {
          if (((ChrList[tnc].light==0xFF && ChrList[tnc].alpha==0xFF) || ChrList[tnc].getCap().forceshadow) && ChrList[tnc].shadow_size!=0)
          {
            render_shadow(ChrList[tnc], PrtList, 0);
          }
        }
      }
      glDisable(GL_BLEND);
      glDepthMask(GL_TRUE);
    }
  }

  glAlphaFunc(GL_GREATER, 0);
  glEnable(GL_ALPHA_TEST);
  glDisable(GL_CULL_FACE);

  // Render the normal characters
  for (cnt = 0; cnt < numdolist; cnt++)
  {
    tnc = dolist[cnt];
    if (ChrList[tnc].alpha==0xFF && ChrList[tnc].light==0xFF)
      render_mad(tnc, 0xFF);
  }

  // Render the sprites
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);

  // Now render the transparent characters
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for (cnt = 0; cnt < numdolist; cnt++)
  {
    tnc = dolist[cnt];
    if (ChrList[tnc].alpha!=0xFF && ChrList[tnc].light==0xFF)
    {
      trans = ChrList[tnc].alpha;
      if (trans < SEEINVISIBLE && (localseeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
      render_mad(tnc, trans);
    }
  }

  // Alpha water
  if (!WaterList.is_light)  render_water(cam);

  // Then do the light characters
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  for (cnt = 0; cnt < numdolist; cnt++)
  {
    tnc = dolist[cnt];
    if (ChrList[tnc].light!=0xFF)
    {
      trans = ChrList[tnc].light;
      if (trans < SEEINVISIBLE && (localseeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
      render_mad(tnc, trans);
    }

    // Do phong highlights
    if (phongon && ChrList[tnc].alpha==0xFF && ChrList[tnc].light==0xFF && !ChrList[tnc].enviro && ChrList[tnc].sheen > 0)
    {
      TEX_REF texturesave;
      ChrList[tnc].enviro = true;
      texturesave = ChrList[tnc].texture;
      ChrList[tnc].texture = TX_PHONG;  // The phong map texture...
      render_mad(tnc, ChrList[tnc].sheen<<4);
      ChrList[tnc].texture = texturesave;
      ChrList[tnc].enviro = false;
    }
  }

  // Light water
  if (WaterList.is_light)  render_water(cam);

  // Turn Z buffer back on, alphablend off
  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);
  glEnable(GL_ALPHA_TEST);
  render_prt();
  glDisable(GL_ALPHA_TEST);

  glDepthMask(GL_TRUE);
  glDisable(GL_BLEND);

  // Done rendering
}


//--------------------------------------------------------------------------------------------
void draw_scene_zreflection(Camera & cam)
{
  // ZZ> This function draws 3D objects
  unsigned short cnt, tnc;
  unsigned char trans;

  //if(refon)
  //{
  //  //---------------------------------------------------
  //  // Render reflections of characters
  //  glPushAttrib(GL_ENABLE_BIT);
  //    //glFrontFace(GL_CCW);
  //    glEnable(GL_BLEND);
  //    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  //    glDepthFunc(GL_LEQUAL);

  //    for (cnt = 0; cnt < numdolist; cnt++)
  //    {
  //      tnc = dolist[cnt];
  //      if((GMesh[ChrList[tnc].onwhichfan].fx&MPDFX_DRAWREF))
  //        render_refmad(tnc, ChrList[tnc].alpha&ChrList[tnc].light);
  //    }
  //  glPopAttrib();
  //  //---------------------------------------------------

  //  //---------------------------------------------------
  //  // Render the reflected sprites
  //  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
  //    glDisable(GL_DEPTH_TEST);
  //    glDepthMask(GL_FALSE);
  //    render_refprt(cam, PrtList);
  //  glPopAttrib();
  //  //---------------------------------------------------

  //  //---------------------------------------------------
  //  // Render the reflective floors i.e. they are partially transparent
  //  // to the reflected objects
  //  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
  //    glEnable(GL_BLEND);
  //    glBlendFunc(GL_ONE, GL_ONE);
  //    glDepthMask(GL_FALSE);

  //    GMesh.txref_last = Tx_List::INVALID;
  //    for (cnt = 0; cnt < GRenderlist.drf_count; cnt++)
  //    {
  //      if(GMesh.m_tile_ex[GRenderlist[cnt].drf].in_view)
  //        render_fan(GRenderlist[cnt].drf);
  //    };
  //  glPopAttrib();
  //  //---------------------------------------------------
  //}
  //else
  //{
  //  //---------------------------------------------------
  //  // Render the now "non-reflective" floors
  //  glPushAttrib(GL_ENABLE_BIT);
  //    GMesh.txref_last = Tx_List::INVALID;
  //    for (cnt = 0; cnt < GRenderlist.drf_count; cnt++)
  //    {
  //      if(GMesh.m_tile_ex[GRenderlist[cnt].drf].in_view)
  //        render_fan(GRenderlist[cnt].drf);
  //    }
  //  glPopAttrib();
  //  //---------------------------------------------------
  //};

  //---------------------------------------------------
  // Render the "draw me first" tiles
    if(GRenderlist.ref_count > 0)
    {
      //dump_gl_state("First-Open");

      glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER,0);

        GMesh.txref_last = Tx_List::INVALID;
        for (cnt = 0; cnt < GRenderlist.ref_count; cnt++)
        {
          if(GMesh.m_tile_ex[GRenderlist[cnt].ref].in_view)
            render_fan(GMesh, GRenderlist[cnt].ref);
        }

      //flip_pages();
      glPopAttrib();

      //dump_gl_state("First-Close");
    };
  //---------------------------------------------------

  //---------------------------------------------------
  // Render the "draw me second" tiles
    if(GRenderlist.sha_count > 0)
    {
      //dump_gl_state("Second-Open");

      glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER,0);

        GMesh.txref_last = Tx_List::INVALID;
        for (cnt = 0; cnt < GRenderlist.sha_count; cnt++)
        {
          if(GMesh.m_tile_ex[GRenderlist[cnt].sha].in_view)
            render_fan(GMesh, GRenderlist[cnt].sha);
        };

      //flip_pages();
      glPopAttrib();

      //dump_gl_state("Second-Close");
    }
  //---------------------------------------------------


  //---------------------------------------------------
  // Render the shadow floors - i.e. cover up the mesh for
  // a "fog of war" type effect for the RTS maps
    if(GRenderlist.sha_count>0)
    {
      //dump_gl_state("FOW-Open");

      glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_FALSE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER,0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_DST_COLOR, GL_ZERO);

        GMesh.txref_last = Tx_List::INVALID;
        for (cnt = 0; cnt < GRenderlist.sha_count; cnt++)
        {
          if(GMesh.m_tile_ex[GRenderlist[cnt].sha].in_view)
            render_fan(GMesh, GRenderlist[cnt].sha);
        }

      //flip_pages();
      glPopAttrib();

      //dump_gl_state("FOW-Close");
    }
  //---------------------------------------------------


  //---------------------------------------------------
  // Render the shadows
    //if(numdolist>0)
    {
      ////dump_gl_state("Shadow-Open");

      glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_FALSE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER,0);

        if (shaon)
        {
          for (cnt = 0; cnt < numdolist; cnt++)
          {
            tnc = dolist[cnt];
            if(ChrList[tnc].held_by == Character_List::INVALID)
            {
              if(((ChrList[tnc].light==0xFF && ChrList[tnc].alpha==0xFF) || ChrList[tnc].getCap().forceshadow) && ChrList[tnc].shadow_size!=0)
              {
                if(shasprite)
                  render_shadow(ChrList[tnc], PrtList, 0);
                else
                  render_shadow(ChrList[tnc], PrtList, 1);
              };
            }
          }
        }

      //flip_pages();
      glPopAttrib();

      ////dump_gl_state("Shadow-Close");
    }
  //---------------------------------------------------

  //---------------------------------------------------
  if(!WaterList.is_light)
  {
    //dump_gl_state("Water2-Open");

    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      glDepthMask(GL_FALSE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);

      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GREATER,0);

      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);

      render_water(cam);
      //flip_pages();
    glPopAttrib();

    //dump_gl_state("Water2-Close");
  }
  //---------------------------------------------------

  //---------------------------------------------------
  // Render the normal characters

    if(numdolist>0)
    {
      //dump_gl_state("Character-Open");

      glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER,0);

        for (cnt = 0; cnt < numdolist; cnt++)
        {
          tnc = dolist[cnt];
          if(ChrList[tnc].alpha==0xFF && ChrList[tnc].light==0xFF)
            render_mad(tnc, 0xFF);
        }
        //flip_pages();
      glPopAttrib();

      //dump_gl_state("Character-Close");
    }
  //---------------------------------------------------

  //---------------------------------------------------
  //if(!WaterList.is_light)
  //{
  //  ////dump_gl_state("Water3-Open");

  //  glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  //    glDepthMask(GL_FALSE);
  //    glEnable(GL_DEPTH_TEST);
  //    glDepthFunc(GL_LESS);

  //    glEnable(GL_ALPHA_TEST);
  //    glAlphaFunc(GL_GREATER,0);

  //    glEnable(GL_BLEND);
  //    glBlendFunc(GL_ONE, GL_ONE);

  //    render_water();
  //    //flip_pages();
  //  glPopAttrib();

  //  ////dump_gl_state("Water3-Close");
  //}
  //---------------------------------------------------


  //---------------------------------------------------
  // Render the sprites
  //---------------------------------------------------


  //---------------------------------------------------
  // Now render the transparent characters
  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
    glDepthMask ( GL_FALSE );
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (cnt = 0; cnt < numdolist; cnt++)
    {
      tnc = dolist[cnt];
      if(ChrList[tnc].alpha!=0xFF && ChrList[tnc].light==0xFF)
      {
        trans = ChrList[tnc].alpha;
        if(trans < SEEINVISIBLE && (localseeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
        render_mad(tnc, trans);
      }
    }
  glPopAttrib();
  //---------------------------------------------------


  //---------------------------------------------------
  // Render transparent water tiles
  if(!WaterList.is_light)
  {
    //dump_gl_state("Water4-Open");

    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      glDepthMask(GL_FALSE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);

      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GREATER,0);

      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

      render_water(cam);
      //flip_pages();
    glPopAttrib();

    //dump_gl_state("Water4-Close");
  }
  //---------------------------------------------------


  //---------------------------------------------------
  // Then do the light characters
  // i.e. acid bloba, etc.
    //glPushAttrib(GL_ENABLE_BIT);
    //  glEnable(GL_BLEND);
    //  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //  for (cnt = 0; cnt < numdolist; cnt++)
    //  {
    //    tnc = dolist[cnt];
    //    if(ChrList[tnc].light!=0xFF)
    //    {
    //      trans = ChrList[tnc].light;
    //      if(trans < SEEINVISIBLE && (localseeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
    //        render_mad(tnc, trans);
    //    }

    //    // Do phong highlights
    //    if(phongon && ChrList[tnc].sheen > 0)
    //    {
    //      bool enviro_save = ChrList[tnc].enviro;
    //      unsigned short texture_save = ChrList[tnc].texture;
    //      ChrList[tnc].enviro = TRUE;
    //      ChrList[tnc].texture = TX_PHONG;  // The phong map texture...
    //      render_mad(tnc, ChrList[tnc].sheen<<4);
    //      ChrList[tnc].texture = texture_save;
    //      ChrList[tnc].enviro = enviro_save;
    //    }
    //  }
    //glPopAttrib();
  //---------------------------------------------------

  //---------------------------------------------------
  // Do light water
  if(WaterList.is_light)
  {
    glPushAttrib(GL_ENABLE_BIT);
      render_water(cam);
    glPopAttrib();
  };
  //---------------------------------------------------

  //---------------------------------------------------
  // do particles
  // Turn Z buffer back on, alphablend off
  {
    //dump_gl_state("Particles-Open");

    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      glDepthMask(GL_FALSE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);

      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GREATER,0);

      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_SRC_COLOR);

      render_prt();

    //flip_pages();
    glPopAttrib();

    //dump_gl_state("Particles-Close");
  }
  //---------------------------------------------------

  // Done rendering
}

//--------------------------------------------------------------------------------------------
//void draw_scene_zreflection(Camera & cam)
//{
//  // ZZ> This function draws 3D objects
//  Uint16 cnt, tnc;
//  Uint8 trans;
//
//  // Clear the image if need be
//  // PORT: I don't think this is needed if(clearson) { clear_surface(lpDDSBack); }
//  // Zbuffer is cleared later
//
//  // Render the reflective floors
//  //glDisable(GL_DEPTH_TEST);
//  //glDepthMask(GL_FALSE);
//
//  // Render the background
//  //if (!clearson)
//  //  render_background(TX_WATERLOW);  // 6 is the texture for waterlow.bmp
//
//  // if(GRenderlist.ref_count > 0)
//  // {
//  //    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
//  //      glDepthMask(GL_TRUE);
//  //      glEnable(GL_DEPTH_TEST);
//  //      glDepthFunc(GL_LEQUAL);
//
//  //      glEnable(GL_ALPHA_TEST);
//  //      glAlphaFunc(GL_GREATER,0);
//
//  //      GMesh.txref_last = Tx_List::INVALID;
//  //      for (cnt = 0; cnt < GRenderlist.ref_count; cnt++)
//  //      {
//  //        if(GMesh.m_tile_ex[GRenderlist[cnt].ref].in_view)
//  //          render_fan(GMesh, GRenderlist[cnt].ref);
//  //      }
//
//  //      //flip_pages();
//  //    glPopAttrib();
//  //  }
//
//  //// BAD: DRAW SHADOW STUFF TOO
//  // if(GRenderlist.sha_count > 0)
//  // {
//  //    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
//  //      glDepthMask(GL_TRUE);
//  //      glEnable(GL_DEPTH_TEST);
//  //      glDepthFunc(GL_LEQUAL);
//
//  //      glEnable(GL_ALPHA_TEST);
//  //      glAlphaFunc(GL_GREATER,0);
//
//  //      GMesh.txref_last = Tx_List::INVALID;
//  //      for (cnt = 0; cnt < GRenderlist.sha_count; cnt++)
//  //      {  
//  //        if(GMesh.m_tile_ex[GRenderlist[cnt].sha].in_view)
//  //          render_fan(GMesh, GRenderlist[cnt].sha);
//  //      }
//
//  //    flip_pages();
//  //    glPopAttrib();
//  //  }
//
//  glEnable(GL_DEPTH_TEST);
//  glDepthMask(GL_TRUE);
//  if (refon)
//  {
//    // Render reflections of characters
//    glFrontFace(GL_CCW);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//    glDepthFunc(GL_LEQUAL);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if (GMesh.has_flags(ChrList[tnc].onwhichfan, MESHFX_DRAWREF))
//        render_refmad(tnc, ChrList[tnc].alpha&ChrList[tnc].light);
//    }
//
//    // [claforte] I think this is wrong... I think we should choose some other depth func.
//    glDepthFunc(GL_ALWAYS);
//
//    // Render the reflected sprites
//    glDisable(GL_DEPTH_TEST);
//    glDepthMask(GL_FALSE);
//    glFrontFace(GL_CW);
//    render_refprt();
//
//    glDisable(GL_BLEND);
//    glDepthFunc(GL_LEQUAL);
//    glEnable(GL_DEPTH_TEST);
//    glDepthMask(GL_TRUE);
//  }
//
//  // Clear the Zbuffer at a bad time...  But hey, reflections work with Voodoo
//  //lpD3DVViewport->Clear(1, &rect, D3DCLEAR_ZBUFFER);
//  // Not sure if this is cool or not - DDOI
//  // glClear ( GL_DEPTH_BUFFER_BIT );
//
//  // Render the shadow floors
//  GMesh.txref_last = Tx_List::INVALID;
//
//  for (cnt = 0; cnt < GRenderlist.sha_count; cnt++)
//  {  
//    if(GMesh.m_tile_ex[GRenderlist[cnt].sha].in_view)
//      render_fan(GMesh, GRenderlist[cnt].sha );
//  }
//
//  // Render the shadows
//  if (shaon)
//  {
//    if (shasprite)
//    {
//      // Bad shadows
//      glDepthMask(GL_FALSE);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if (INVALID_CHR(ChrList[tnc].held_by))
//        {
//          if (((ChrList[tnc].light==0xFF && ChrList[tnc].alpha==0xFF) || ChrList[tnc].getCap().forceshadow) && ChrList[tnc].shadow_size!=0)
//            render_bad_shadow(tnc);
//        }
//      }
//      glDisable(GL_BLEND);
//      glDepthMask(GL_TRUE);
//    }
//    else
//    {
//      // Good shadows for me
//      glDepthMask(GL_FALSE);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_COLOR, GL_ZERO);
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if (INVALID_CHR(ChrList[tnc].held_by))
//        {
//          if (((ChrList[tnc].light==0xFF && ChrList[tnc].alpha==0xFF) || ChrList[tnc].getCap().forceshadow) && ChrList[tnc].shadow_size!=0)
//            render_shadow(tnc);
//        }
//      }
//
//      glDisable(GL_BLEND);
//      glDepthMask(GL_TRUE);
//    }
//  }
//
//  glAlphaFunc(GL_GREATER, 0);
//  glEnable(GL_ALPHA_TEST);
//
//  // Render the normal characters
//  for (cnt = 0; cnt < numdolist; cnt++)
//  {
//    tnc = dolist[cnt];
//    if (ChrList[tnc].alpha==0xFF && ChrList[tnc].light==0xFF)
//      render_mad(tnc, 0xFF);
//  }
//
//  // Render the sprites
//  glDepthMask(GL_FALSE);
//  glEnable(GL_BLEND);
//
//  // Now render the transparent characters
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//  for (cnt = 0; cnt < numdolist; cnt++)
//  {
//    tnc = dolist[cnt];
//    if (ChrList[tnc].alpha!=0xFF && ChrList[tnc].light==0xFF)
//    {
//      trans = ChrList[tnc].alpha;
//      if (trans < SEEINVISIBLE && (localseeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//      render_mad(tnc, trans);
//    }
//  }
//
//  // And alpha water floors
//  if (!WaterList.is_light)
//    render_water(cam);
//
//  // Then do the light characters
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//
//  for (cnt = 0; cnt < numdolist; cnt++)
//  {
//    tnc = dolist[cnt];
//    if (ChrList[tnc].light!=0xFF)
//    {
//      trans = ChrList[tnc].light;
//      if (trans < SEEINVISIBLE && (localseeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//      render_mad(tnc, trans);
//    }
//
//    // Do phong highlights
//    if (phongon && ChrList[tnc].alpha==0xFF && ChrList[tnc].light==0xFF && !ChrList[tnc].enviro && ChrList[tnc].sheen > 0)
//    {
//      Uint16 texturesave;
//      ChrList[tnc].enviro = true;
//      texturesave = ChrList[tnc].texture;
//      ChrList[tnc].texture = TX_PHONG;  // The phong map texture...
//      render_mad(tnc, ChrList[tnc].sheen<<4);
//      ChrList[tnc].texture = texturesave;
//      ChrList[tnc].enviro = false;
//    }
//  }
//
//  // Do light water
//  if (WaterList.is_light)
//    render_water(cam);
//
//  // Turn Z buffer back on, alphablend off
//  glDepthMask(GL_TRUE);
//  glDisable(GL_BLEND);
//  glEnable(GL_ALPHA_TEST);
//  render_prt();
//  glDisable(GL_ALPHA_TEST);
//
//  glDepthMask(GL_TRUE);
//
//  glDisable(GL_BLEND);
//
//  // Done rendering
//}
//
//
//
//--------------------------------------------------------------------------------------------
void draw_blip(Uint8 color, int x, int y)
{
  float xl,xr,yt,yb;
  int width, height;

  // ZZ> This function draws a blip
  if (x > 0 && y > 0)
  {
    EnableTexturing();
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glNormal3f(0.0f, 0.0f, 1.0f);

    TxBlip.Bind(GL_TEXTURE_2D);
    xl = ((float)BlipList[color].rect.left)/0x20;
    xr = ((float)BlipList[color].rect.right)/0x20;
    yt = ((float)BlipList[color].rect.top)/4;
    yb = ((float)BlipList[color].rect.bottom)/4;
    width = BlipList[color].rect.right-BlipList[color].rect.left; height=BlipList[color].rect.bottom-BlipList[color].rect.top;

    glBegin(GL_QUADS);
    glTexCoord2f(xl, yb);   glVertex2i(x-1,       y+1+height);
    glTexCoord2f(xr, yb);   glVertex2i(x-1+width, y+1+height);
    glTexCoord2f(xr, yt);   glVertex2i(x-1+width, y+1);
    glTexCoord2f(xl, yt);   glVertex2i(x-1,       y+1);
    glEnd();

  }
}

//--------------------------------------------------------------------------------------------
void draw_one_icon(ICON_REF & icontype, int x, int y, Uint8 sparkle)
{
  // ZZ> This function draws an icon
  int position, blipx, blipy;
  float xl,xr,yt,yb;
  int width, height;

  if(Icon_List::INVALID == icontype.index) return;

  Icon & ricon = IconList[icontype];

  if ( ricon.Valid() )
  {
    EnableTexturing();  // Enable texture mapping
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    //lpDDSBack->BltFast(x, y, lpDDSIcon[icontype], &iconrect, DDBLTFAST_NOCOLORKEY);
    ricon.Bind(GL_TEXTURE_2D);
    xl=((float)IconList.rect.left)/0x20;
    xr=((float)IconList.rect.right)/0x20;
    yt=((float)IconList.rect.top)/0x20;
    yb=((float)IconList.rect.bottom)/0x20;
    width  = IconList.rect.right - IconList.rect.left;
    height = IconList.rect.bottom - IconList.rect.top;

    glBegin(GL_QUADS);
    glTexCoord2f(xl, yb);   glVertex2i(x,       y+height);
    glTexCoord2f(xr, yb);   glVertex2i(x+width, y+height);
    glTexCoord2f(xr, yt);   glVertex2i(x+width, y);
    glTexCoord2f(xl, yt);   glVertex2i(x,       y);
    glEnd();
  }

  if (sparkle != NOSPARKLE)
  {
    position = wldframe&0x1F;
    position = (SPARKLESIZE * position >> 5);

    blipx = x + SPARKLEADD + position;
    blipy = y + SPARKLEADD;
    draw_blip(sparkle, blipx, blipy);

    blipx = x + SPARKLEADD + SPARKLESIZE;
    blipy = y + SPARKLEADD + position;
    draw_blip(sparkle, blipx, blipy);

    blipx = blipx - position;
    blipy = y + SPARKLEADD + SPARKLESIZE;
    draw_blip(sparkle, blipx, blipy);

    blipx = x + SPARKLEADD;
    blipy = blipy - position;
    draw_blip(sparkle, blipx, blipy);
  }
}

//--------------------------------------------------------------------------------------------
void draw_map(int x, int y)
{
  // ZZ> This function draws the map

  //printf("draw map getting called\n");

  EnableTexturing();
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  //glNormal3f( 0, 0, 1 );

  TxMap.Bind(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 1.0); glVertex2i(x,  y+MAPSIZE);
  glTexCoord2f(1.0, 1.0); glVertex2i(x+MAPSIZE, y+MAPSIZE);
  glTexCoord2f(1.0, 0.0); glVertex2i(x+MAPSIZE, y);
  glTexCoord2f(0.0, 0.0); glVertex2i(x,  y);
  glEnd();
}

//--------------------------------------------------------------------------------------------
int draw_one_bar(int bartype, int x, int y, int ticks, int maxticks)
{
  // ZZ> This function draws a bar and returns the y position for the next one
  int noticks;
  float xl, xr, yt, yb;
  int width, height;

  EnableTexturing();               // Enable texture mapping
  glColor4f(1, 1, 1, 1);

  if (maxticks>0 && ticks >= 0)
  {
    // Draw the tab
    TxBars.Bind(GL_TEXTURE_2D);
    xl=((float)tabrect[bartype].left)/0x80;
    xr=((float)tabrect[bartype].right)/0x80;
    yt=((float)tabrect[bartype].top)/0x80;
    yb=((float)tabrect[bartype].bottom)/0x80;
    width=tabrect[bartype].right-tabrect[bartype].left; height=tabrect[bartype].bottom-tabrect[bartype].top;

    glBegin(GL_QUADS);
      glTexCoord2f(xl, yb);   glVertex2i(x,       y+height);
      glTexCoord2f(xr, yb);   glVertex2i(x+width, y+height);
      glTexCoord2f(xr, yt);   glVertex2i(x+width, y);
      glTexCoord2f(xl, yt);   glVertex2i(x,       y);
    glEnd();

    // Error check
    if (maxticks>MAXTICK) maxticks = MAXTICK;
    if (ticks>maxticks) ticks = maxticks;

    // Draw the full rows of ticks
    x+=TABX;
    while (ticks >= NUMTICK)
    {
      barrect[bartype].right = BARX;
      TxBars.Bind(GL_TEXTURE_2D);
      xl=((float)barrect[bartype].left)/0x80;
      xr=((float)barrect[bartype].right)/0x80;
      yt=((float)barrect[bartype].top)/0x80;
      yb=((float)barrect[bartype].bottom)/0x80;
      width=barrect[bartype].right-barrect[bartype].left; height=barrect[bartype].bottom-barrect[bartype].top;
      glBegin(GL_QUADS);
      glTexCoord2f(xl, yb);   glVertex2i(x,       y+height);
      glTexCoord2f(xr, yb);   glVertex2i(x+width, y+height);
      glTexCoord2f(xr, yt);   glVertex2i(x+width, y);
      glTexCoord2f(xl, yt);   glVertex2i(x,       y);
      glEnd();
      y+=BARY;
      ticks-=NUMTICK;
      maxticks-=NUMTICK;
    }

    // Draw any partial rows of ticks
    if (maxticks > 0)
    {
      // Draw the filled ones
      barrect[bartype].right = (ticks<<3)+TABX;
      TxBars.Bind(GL_TEXTURE_2D);
      xl=((float)barrect[bartype].left)/0x80;
      xr=((float)barrect[bartype].right)/0x80;
      yt=((float)barrect[bartype].top)/0x80;
      yb=((float)barrect[bartype].bottom)/0x80;
      width=barrect[bartype].right-barrect[bartype].left; height=barrect[bartype].bottom-barrect[bartype].top;
      glBegin(GL_QUADS);
      glTexCoord2f(xl, yb);   glVertex2i(x,       y+height);
      glTexCoord2f(xr, yb);   glVertex2i(x+width, y+height);
      glTexCoord2f(xr, yt);   glVertex2i(x+width, y);
      glTexCoord2f(xl, yt);   glVertex2i(x,       y);
      glEnd();

      // Draw the empty ones
      noticks = maxticks-ticks;
      if (noticks > (NUMTICK-ticks)) noticks = (NUMTICK-ticks);
      barrect[0].right = (noticks<<3)+TABX;
      TxBars.Bind(GL_TEXTURE_2D);
      xl=((float)barrect[0].left)/0x80;
      xr=((float)barrect[0].right)/0x80;
      yt=((float)barrect[0].top)/0x80;
      yb=((float)barrect[0].bottom)/0x80;
      width=barrect[0].right-barrect[0].left; height=barrect[0].bottom-barrect[0].top;
      glBegin(GL_QUADS);
      glTexCoord2f(xl, yb);   glVertex2i((ticks<<3)+x,       y+height);
      glTexCoord2f(xr, yb);   glVertex2i((ticks<<3)+x+width, y+height);
      glTexCoord2f(xr, yt);   glVertex2i((ticks<<3)+x+width, y);
      glTexCoord2f(xl, yt);   glVertex2i((ticks<<3)+x,       y);
      glEnd();
      maxticks-=NUMTICK;
      y+=BARY;
    }

    // Draw full rows of empty ticks
    while (maxticks >= NUMTICK)
    {
      barrect[0].right = BARX;
      TxBars.Bind(GL_TEXTURE_2D);
      xl=((float)barrect[0].left)/0x80;
      xr=((float)barrect[0].right)/0x80;
      yt=((float)barrect[0].top)/0x80;
      yb=((float)barrect[0].bottom)/0x80;
      width=barrect[0].right-barrect[0].left; height=barrect[0].bottom-barrect[0].top;
      glBegin(GL_QUADS);
      glTexCoord2f(xl, yb);   glVertex2i(x,       y+height);
      glTexCoord2f(xr, yb);   glVertex2i(x+width, y+height);
      glTexCoord2f(xr, yt);   glVertex2i(x+width, y);
      glTexCoord2f(xl, yt);   glVertex2i(x,       y);
      glEnd();
      y+=BARY;
      maxticks-=NUMTICK;
    }

    // Draw the last of the empty ones
    if (maxticks > 0)
    {
      barrect[0].right = (maxticks<<3)+TABX;
      TxBars.Bind(GL_TEXTURE_2D);
      xl=((float)barrect[0].left)/0x80;
      xr=((float)barrect[0].right)/0x80;
      yt=((float)barrect[0].top)/0x80;
      yb=((float)barrect[0].bottom)/0x80;
      width=barrect[0].right-barrect[0].left; height=barrect[0].bottom-barrect[0].top;

      glBegin(GL_QUADS);
        glTexCoord2f(xl, yb);   glVertex2i(x,       y+height);
        glTexCoord2f(xr, yb);   glVertex2i(x+width, y+height);
        glTexCoord2f(xr, yt);   glVertex2i(x+width, y);
        glTexCoord2f(xl, yt);   glVertex2i(x,       y);
      glEnd();
      y+=BARY;
    }
  }

  return y;
}

//--------------------------------------------------------------------------------------------
void BeginText(void)
{
  //EnableTexturing();  // Enable texture mapping
  //GFont.texture.Bind(GL_TEXTURE_2D);
  //glAlphaFunc(GL_GREATER,0);
  //glEnable(GL_ALPHA_TEST);
  //glDisable(GL_DEPTH_TEST);
  //glDisable(GL_CULL_FACE);
}

//--------------------------------------------------------------------------------------------
void EndText()
{
  //glDisable(GL_BLEND);
  //glDisable(GL_ALPHA_TEST);
}

//--------------------------------------------------------------------------------------------
int draw_status(Uint16 character, int x, int y)
{
  // ZZ> This function shows a character's icon, status and inventory
  //     The x,y coordinates are the top left point of the image to draw
  Uint16 item;
  char cTmp;
  char *readtext;

  int life = ChrList[character].life>>FIXEDPOINT_BITS;
  int lifemax = ChrList[character].lifemax>>FIXEDPOINT_BITS;
  int mana = ChrList[character].mana>>FIXEDPOINT_BITS;
  int manamax = ChrList[character].manamax>>FIXEDPOINT_BITS;
  int cnt = lifemax;

  Font * fnt = UI::bmp_fnt;

  // Write the character's first name
  if (ChrList[character].nameknown)
    readtext = ChrList[character].name;
  else
    readtext = ChrList[character].getCap().classname;

  //for (cnt = 0; cnt < 6; cnt++)
  //{
  //  cTmp = readtext[cnt];
  //  if (cTmp == ' ' || cTmp == 0)
  //  {
  //    generictext[cnt] = 0;
  //    break;
  //  }
  //  else
  //    generictext[cnt] = cTmp;
  //}
  //generictext[6] = 0;
  fnt->drawText(x+8, y, readtext); y += fnt->getSize()/2;

  // Write the character's money
  sprintf(generictext, "$%4d", ChrList[character].money);
  fnt->drawText(x+8, y, generictext); y += fnt->getSize()/2+8;

  // Draw the icons
  draw_one_icon(ProfileList[ChrList[character].model].icon_ref[ChrList[character].skin], x+40, y, ChrList[character].sparkle);

  item = ChrList[character].holding_which[SLOT_LEFT];
  if (VALID_CHR(item))
  {
    if (ChrList[item].show_icon)
    {
      draw_one_icon(ProfileList[ChrList[item].model].icon_ref[ChrList[item].skin], x+8, y, ChrList[item].sparkle);
      if (ChrList[item].ammomax!=0 && ChrList[item].ammoknown)
      {
        if (!ChrList[item].getCap().isstackable || ChrList[item].ammo>1)
        {
          // Show amount of ammo left
          sprintf(generictext, "%2d", ChrList[item].ammo);
          fnt->drawText(x+8, y-8, generictext);
        }
      }
    }
    else
      draw_one_icon(IconList.book_icon[ChrList[item].money&3], x+8, y, ChrList[item].sparkle);
  }
  else
    draw_one_icon(IconList.null, x+8, y, NOSPARKLE);

  item = ChrList[character].holding_which[SLOT_RIGHT];
  if (VALID_CHR(item))
  {
    if (ChrList[item].show_icon)
    {
      draw_one_icon(ProfileList[ChrList[item].model].icon_ref[ChrList[item].skin], x+72, y, ChrList[item].sparkle);
      if (ChrList[item].ammomax!=0 && ChrList[item].ammoknown)
      {
        if (!ChrList[item].getCap().isstackable || ChrList[item].ammo>1)
        {
          // Show amount of ammo left
          sprintf(generictext, "%2d", ChrList[item].ammo);
          fnt->drawText(x+72, y-8, generictext);
        }
      }
    }
    else
      draw_one_icon(IconList.book_icon[ChrList[item].money&3], x+72, y, ChrList[item].sparkle);
  }
  else
    draw_one_icon(IconList.null, x+72, y, NOSPARKLE);

  y+=0x20;

  // Draw the bars
  if (ChrList[character].alive)
    y = draw_one_bar(ChrList[character].lifecolor, x, y, life, lifemax);
  else
    y = draw_one_bar(0, x, y, 0, lifemax);  // Draw a black bar

  y = draw_one_bar(ChrList[character].manacolor, x, y, mana, manamax);
  return y;
}

//--------------------------------------------------------------------------------------------
void draw_text()
{
  // ZZ> This function spits out some words
  char text[0x0200];
  int y, cnt, tnc, fifties, seconds, minutes;
  Locker_2DMode locker_2d;
  Font * fnt = UI::bmp_fnt;

  locker_2d.begin();

  // Status bars
  y = 0;
  if (staton)
  {
    for (cnt = 0; cnt < numstat && y < scry; cnt++)
      y = draw_status(statlist[cnt], scrx-BARX, y);
  }

  // Map display
  if (Blip::mapon)
  {
    draw_map(0, scry-MAPSIZE);

    for (cnt = 0; cnt < Blip::numblip; cnt++)
      draw_blip(BlipList[cnt].c, BlipList[cnt].x, BlipList[cnt].y+scry-MAPSIZE);

    if (Blip::youarehereon && (wldframe&8))
    {
      for (cnt = 0;  cnt<Player_List::SIZE; cnt++)
      {
        if ( VALID_PLAYER(cnt) )
        {
          tnc = PlaList[cnt].index;
          if (ChrList[tnc].alive)
            draw_blip(0, ChrList[tnc].pos.x*MAPSIZE/GMesh.width(), (ChrList[tnc].pos.y*MAPSIZE/GMesh.height())+scry-MAPSIZE);
        }
      }
    }
  }

  // FPS text
  y = 0;
  if (outofsync)
  {
    sprintf(text, "OUT OF SYNC, TRY RTS...");
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
  }

  if (parseerror)
  {
    sprintf(text, "SCRIPT ERROR ( SEE PARSEERR.TXT )");
    fnt->drawText(0, y, text);
    y += fnt->getSize() ;
  }

  if (fpson)
  {
    fnt->drawText(0, y, szfpstext);
    y += fnt->getSize() ;
  }

  if (GKeyb.pressed(SDLK_F1))
  {
    // In-Game help
    sprintf(text, "!!!MOUSE HELP!!!");
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    if (GRTS.on)
    {
      if (allselect)
      {
        sprintf(text, "  Left Click to order units");
        fnt->drawText(0, y, text);  y += fnt->getSize() ;
      }
      else
      {
        sprintf(text, "  Left Drag to select units");
        fnt->drawText(0, y, text);  y += fnt->getSize() ;
        sprintf(text, "  Left Click to order them");
        fnt->drawText(0, y, text);  y += fnt->getSize() ;
      }
    }
    else
    {
      sprintf(text, "  Edit CONTROLS.TXT to change");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  Left Click to use an item");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  Left and Right Click to grab");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  Middle Click to jump");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  A and S keys do stuff");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
    }
    sprintf(text, "  Right Drag to move camera");
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
  }
  if (GKeyb.pressed(SDLK_F2))
  {
    // In-Game help
    sprintf(text, "!!!JOYSTICK HELP!!!");
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    if (GRTS.on)
    {
      sprintf(text, "  Device_Joystick not available");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
    }
    else
    {
      sprintf(text, "  Edit CONTROLS.TXT to change");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  Hit the buttons");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  You'll figure it out");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
    }
  }
  if (GKeyb.pressed(SDLK_F3))
  {
    // In-Game help
    sprintf(text, "!!!KEYBOARD HELP!!!");
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    if (GRTS.on)
    {
      sprintf(text, "  Device_Keyboard not available");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
    }
    else
    {
      sprintf(text, "  Edit CONTROLS.TXT to change");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  TGB control one hand");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  YHN control the other");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  Keypad to move and jump");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
      sprintf(text, "  Number keys for stats");
      fnt->drawText(0, y, text);  y += fnt->getSize() ;
    }
  }
  if (GKeyb.pressed(SDLK_F5))
  {
    // Debug information
    sprintf(text, "!!!DEBUG MODE-5!!!");
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  CAM %f %f", GCamera.pos.x, GCamera.pos.y);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    tnc = PlaList[0].index;
    sprintf(text, "  PLA0DEF %d %d %d %d %d %d %d %d",
            ChrList[tnc].damagemodifier[0]&3,
            ChrList[tnc].damagemodifier[1]&3,
            ChrList[tnc].damagemodifier[2]&3,
            ChrList[tnc].damagemodifier[3]&3,
            ChrList[tnc].damagemodifier[4]&3,
            ChrList[tnc].damagemodifier[5]&3,
            ChrList[tnc].damagemodifier[6]&3,
            ChrList[tnc].damagemodifier[7]&3);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    tnc = PlaList[0].index;
    sprintf(text, "  PLA0 %5.1f %5.1f", ChrList[tnc].pos.x/float(0x80), ChrList[tnc].pos.y/float(0x80));
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    tnc = PlaList[1].index;
    sprintf(text, "  PLA1 %5.1f %5.1f", ChrList[tnc].pos.x/float(0x80), ChrList[tnc].pos.y/float(0x80));
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
  }
  if (GKeyb.pressed(SDLK_F6))
  {
    // More debug information
    sprintf(text, "!!!DEBUG MODE-6!!!");
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  FREEPRT %d", PrtList.count_free());
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  FREECHR %d",  ChrList.count_free());
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  MACHINE %d", localmachine);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  EXPORT %d", exportvalid);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  FOGAFF %d", GFog.affectswater);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  PASS %d/%d", ShopList.count_used(), PassList.count_used());
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  NETPLAYERS %d", GNet.numplayer);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "  DAMAGE_PART %d", GTile_Dam.parttype);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
  }

  if (GKeyb.pressed(SDLK_F7))
  {
    // White debug mode
    sprintf(text, "!!!DEBUG MODE-7!!!");
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "CAM %f %f %f %f", GCamera.mView.CNV(0,0), GCamera.mView.CNV(1,0), GCamera.mView.CNV(2,0), GCamera.mView.CNV(3,0));
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "CAM %f %f %f %f", GCamera.mView.CNV(0,1), GCamera.mView.CNV(1,1), GCamera.mView.CNV(2,1), GCamera.mView.CNV(3,1));
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "CAM %f %f %f %f", GCamera.mView.CNV(0,2), GCamera.mView.CNV(1,2), GCamera.mView.CNV(2,2), GCamera.mView.CNV(3,2));
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "CAM %f %f %f %f", GCamera.mView.CNV(0,3), GCamera.mView.CNV(1,3), GCamera.mView.CNV(2,3), GCamera.mView.CNV(3,3));
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "x %f", GCamera.center.x);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "y %f", GCamera.center.y);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
    sprintf(text, "turn %d %d", cam_autoturn, cam_turn_time);
    fnt->drawText(0, y, text);  y += fnt->getSize() ;
  }
  if (GKeyb.pressed(SDLK_F8) && !GNet.on)
  {
    // Pause debug mode...  Don't show text, so screen captures look better
    //        sprintf(text, "!!!DEBUG MODE-8!!!");
    //        fnt->drawText(0, y, text);  y += fnt->getSize() ;
  }
  if (timeron)
  {
    fifties = (timervalue%50)<<1;
    seconds = ((timervalue/50)%60);
    minutes = (timervalue/3000);
    sprintf(text, "=%d:%02d:%02d=", minutes, seconds, fifties);
    fnt->drawText(0, y, text);
    y += fnt->getSize() ;
  }
  if (waitingforplayers)
  {
    sprintf(text, "Waiting for players...");
    fnt->drawText(0, y, text);
    y += fnt->getSize() ;
  }
  if (!GRTS.on)
  {
    if (alllocalpladead || respawnanytime)
    {
      if (respawn_mode)
      {
        sprintf(text, "PRESS SPACE TO RESPAWN");
      }
      else
      {
        sprintf(text, "PRESS ESCAPE TO QUIT");
      }
      fnt->drawText(0, y, text);
      y += fnt->getSize() ;
    }
    else
    {
      if (beatmodule)
      {
        sprintf(text, "VICTORY!  PRESS ESCAPE");
        fnt->drawText(0, y, text);
        y += fnt->getSize() ;
      }
    }
  }

  // Network message input
  if (GNetMsg.mode)
  {
    y = fnt->drawTextBox(GNetMsg.buffer, REGION(0, y, scrx-wraptolerance, 0), fnt->getSize() );
  }

  // Messages
  if (messageon)
  {
    // Display the messages
    tnc = GMsg.start;

    for (cnt = 0; cnt < maxmessage; cnt++)
    {
      if (GMsg.time[tnc]>0)
      {
        y = fnt->drawTextBox(GMsg.textdisplay[tnc], REGION(0, y, scrx-wraptolerance,0), fnt->getSize() );
        GMsg.time[tnc] -= GMsg.timechange;
      }
      tnc++;
      tnc = tnc % maxmessage;
    }
  }
}

//--------------------------------------------------------------------------------------------
void flip_pages()
{
  SDL_GL_SwapBuffers();
}

//--------------------------------------------------------------------------------------------
void draw_scene(Camera & cam)
{
  Locker_3DMode loc_locker_3d;

  loc_locker_3d.begin(cam);

  DynaList.make(PrtList);
  do_dyna_light_mesh(GMesh, PrtList);
  //light_characters(ChrList, GMesh);
  light_particles(PrtList, GMesh);

  glClear(GL_DEPTH_BUFFER_BIT);
  glDepthMask(GL_TRUE);

  //if (zreflect) //DO REFLECTIONS
    draw_scene_zreflection(cam);
  //else
  //  draw_scene_sadreflection(cam);

  //Foreground overlay
  //if (overlay_on)
  //{
  //    render_foreground_overlay(TX_WATERTOP);  // Texture 5 is watertop.bmp
  //}

  //End3DMode();
}

//--------------------------------------------------------------------------------------------
bool load_one_title_image(int titleimage, char *szLoadName)
{
  // ZZ> This function loads a title in the specified image slot, forcing it into
  //     system memory.  Returns true if it worked

  return GLTexture::INVALID != GLTexture::Load(&TxTitleImage[titleimage], szLoadName);
}

//--------------------------------------------------------------------------------------------
void load_all_titleimages()
{
  // ZZ> This function loads the title image for each module.  Modules without a
  //     title are marked as invalid

  char searchname[15];
  char loadname[0x0100];
  const char *FileName;
  FILE* filesave;

  // Convert searchname
  strcpy(searchname, "modules/*.mod");

  // Log a directory list
  filesave = fopen("basicdat/modules.txt", "w");
  if (filesave != NULL)
  {
    fprintf(filesave, "This file logs all of the modules found\n");
    fprintf(filesave, "** Denotes an invalid module (Or unlocked)\n\n");
  }

  // Search for .mod directories
  FileName = fs_findFirstFile("modules", "mod");
  Module::globalnum = 0;
  while (FileName && Module::globalnum < MAXMODULE)
  {
    sprintf(ModList[Module::globalnum].loadname, "%s", FileName);
    sprintf(loadname, "modules/%s/gamedat/menu.txt", FileName);
    if (get_module_data(Module::globalnum, loadname))
    {
      sprintf(loadname, "modules/%s/gamedat/title.bmp", FileName);
      if (load_one_title_image(Module::globalnum, loadname))
      {
        fprintf(filesave, "%02d.  %s\n", Module::globalnum, ModList[Module::globalnum].longname);
        Module::globalnum++;
      }
      else
      {
        fprintf(filesave, "**.  %s\n", FileName);
      }
    }
    else
    {
      fprintf(filesave, "**.  %s\n", FileName);
    }
    FileName = fs_findNextFile();
  }
  fs_findClose();

  if (filesave != NULL) fclose(filesave);
}

//--------------------------------------------------------------------------------------------
void load_blip_bitmap()
{
  //This function loads the blip bitmaps
  int cnt;

  GLTexture::Load(&TxBlip, "basicdat/blip.bmp");

  // Set up the rectangles
  for (cnt = 0; cnt < NUMBAR; cnt++)
  {
    BlipList[cnt].rect.left   = cnt * BLIPSIZE;
    BlipList[cnt].rect.right  = (cnt * BLIPSIZE) + BLIPSIZE;
    BlipList[cnt].rect.top    = 0;
    BlipList[cnt].rect.bottom = BLIPSIZE;
  }
}

//--------------------------------------------------------------------------------------------
//void draw_trimx(int x, int y, int length)
//{
// // ZZ> This function draws a horizontal trim bar
// GLfloat txWidth, txHeight, txLength;
//
// if ( TxTrim.Valid() )
// {
//  /*while( length > 0 )
//        {
//   trimrect.right = length;
//   if(length > TRIMX)  trimrect.right = TRIMX;
//   trimrect.bottom = 4;
//   lpDDSBack->BltFast(x, y, lpDDSTrimX, &trimrect, DDBLTFAST_NOCOLORKEY);
//   length-=TRIMX;
//   x+=TRIMX;
//  }*/
//
//  /* Calculate the texture width, height, and length */
//  txWidth = ( GLfloat )( GLTexture::GetImageWidth( &TxTrim )/GLTexture::GetDimensions( &TxTrim ) );
//  txHeight = ( GLfloat )( GLTexture::GetImageHeight( &TxTrim )/GLTexture::GetDimensions( &TxTrim ) );
//  txLength = ( GLfloat )( length/GLTexture::GetImageWidth( &TxTrim ) );
//
//
//  /* Bind our texture */
//  TxTrim.Bind(GL_TEXTURE_2D);
//
//  /* Draw the trim */
//  glColor4f( 1, 1, 1, 1 );
//  glBegin( GL_QUADS );
//   glTexCoord2f( 0, 1 ); glVertex2f( x, scry - y );
//   glTexCoord2f( 0, 1 - txHeight ); glVertex2f( x, scry - y - GLTexture::GetImageHeight( &TxTrim ) );
//   glTexCoord2f( txWidth*txLength, 1 - txHeight ); glVertex2f( x + length, scry - y - GLTexture::GetImageHeight( &TxTrim ) );
//   glTexCoord2f( txWidth*txLength, 1 ); glVertex2f( x + length, scry - y );
//  glEnd();
// }
//}

//--------------------------------------------------------------------------------------------
//void draw_trimy(int x, int y, int length)
//{
// // ZZ> This function draws a vertical trim bar
// GLfloat txWidth, txHeight, txLength;
//
// if ( TxTrim.Valid() )
// {
//  /*while(length > 0)
//  {
//   trimrect.bottom = length;
//   if(length > TRIMY)  trimrect.bottom = TRIMY;
//   trimrect.right = 4;
//   lpDDSBack->BltFast(x, y, lpDDSTrimY, &trimrect, DDBLTFAST_NOCOLORKEY);
//   length-=TRIMY;
//   y+=TRIMY;
//  }*/
//
//  /* Calculate the texture width, height, and length */
//  txWidth = ( GLfloat )( GLTexture::GetImageWidth( &TxTrim )/GLTexture::GetDimensions( &TxTrim ) );
//  txHeight = ( GLfloat )( GLTexture::GetImageHeight( &TxTrim )/GLTexture::GetDimensions( &TxTrim ) );
//  txLength = ( GLfloat )( length/GLTexture::GetImageHeight( &TxTrim ) );
//
//  /* Bind our texture */
//  TxTrim.Bind(GL_TEXTURE_2D);
//
//  /* Draw the trim */
//  glColor4f( 1, 1, 1, 1 );
//  glBegin( GL_QUADS );
//   glTexCoord2f( 0, 1 ); glVertex2f( x, scry - y );
//   glTexCoord2f( 0, 1 - txHeight*txLength ); glVertex2f( x, scry - y - length );
//   glTexCoord2f( txWidth, 1 - txHeight*txLength ); glVertex2f( x + GLTexture::GetImageWidth( &TxTrim ), scry - y - length );
//   glTexCoord2f( txWidth, 1 ); glVertex2f( x + GLTexture::GetImageWidth( &TxTrim ), scry - y );
//  glEnd();
// }
//}

//--------------------------------------------------------------------------------------------
//void draw_trim_box(int left, int top, int right, int bottom)
//{
//    // ZZ> This function draws a trim rectangle
//    float l,t,r,b;
//    l=((float)left)/scrx;
//    r=((float)right)/scrx;
//    t=((float)top)/scry;
//    b=((float)bottom)/scry;
//
// Locker_2DMode locker_2d;
// locker_2d.begin();
//
// draw_trimx(left, top, right-left);
// draw_trimx(left, bottom-4, right-left);
// draw_trimy(left, top, bottom-top);
// draw_trimy(right-4, top, bottom-top);
//
//}

//--------------------------------------------------------------------------------------------
//void draw_trim_box_opening(int left, int top, int right, int bottom, float amount)
//{
//    // ZZ> This function draws a trim rectangle, scaled around its center
//    int x = (left + right)>>1;
//    int y = (top + bottom)>>1;
//    left   = (x * (1.0-amount)) + (left * amount);
//    right  = (x * (1.0-amount)) + (right * amount);
//    top    = (y * (1.0-amount)) + (top * amount);
//    bottom = (y * (1.0-amount)) + (bottom * amount);
//    draw_trim_box(left, top, right, bottom);
//}

//--------------------------------------------------------------------------------------------
void draw_titleimage(int image, int x, int y)
{
  // ZZ> This function draws a title image on the backbuffer
  GLfloat txWidth, txHeight;

  if (! TxTitleImage[image].Valid() ) return;

  Locker_2DMode locker_2d;
  locker_2d.begin();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    //Begin2DMode();

    /* Calculate the texture width & height */
    txWidth  = (GLfloat)(GLTexture::GetImageWidth(&TxTitleImage[image])/GLTexture::GetTextureWidth(&TxTitleImage[image]));
    txHeight = (GLfloat)(GLTexture::GetImageHeight(&TxTitleImage[image])/GLTexture::GetTextureHeight(&TxTitleImage[image]));

    /* Bind the texture */
    TxTitleImage[image].Bind(GL_TEXTURE_2D);

    /* Draw the quad */
    glBegin(GL_QUADS);
      glTexCoord2f(0, 1);                glVertex2f(x,                                                  y + GLTexture::GetImageHeight(&TxTitleImage[image]));
      glTexCoord2f(txWidth, 1);          glVertex2f(x + GLTexture::GetImageWidth(&TxTitleImage[image]), y + GLTexture::GetImageHeight(&TxTitleImage[image]));
      glTexCoord2f(txWidth, 1-txHeight); glVertex2f(x + GLTexture::GetImageWidth(&TxTitleImage[image]), y);
      glTexCoord2f(0, 1-txHeight);       glVertex2f(x,                                                  y);
    glEnd();


}


//--------------------------------------------------------------------------------------------
void build_select(float tlx, float tly, float brx, float bry, Uint8 team)
{
  // ZZ> This function checks which characters are in the selection rectangle
  /*PORT
  D3DLVERTEX v[PRT_COUNT];
  D3DTLVERTEX vt[PRT_COUNT];
  int numbertocheck, character, cnt, first, sound;

  // Unselect old ones
  clear_select();

  // Figure out who to check
  numbertocheck = 0;
  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if(rchr_cnt.team == team && !rchr_cnt.is_inpack)
    {
      v[numbertocheck].x = (D3DVALUE) rchr_cnt.pos.x;
      v[numbertocheck].y = (D3DVALUE) rchr_cnt.pos.y;
      v[numbertocheck].z = (D3DVALUE) rchr_cnt.pos.z;
      v[numbertocheck].color = cnt;  // Store an index in the color slot...
      v[numbertocheck].dwReserved = 0;
      numbertocheck++;
    }

  } SCAN_CHR_END;

  // Figure out where the points go onscreen
  lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &GCamera.mWorld);
  transform_vertices(numbertocheck, v, vt);

  first = true;
  cnt = 0;
  while(cnt < numbertocheck)
  {
  // Only check if in front of camera
  if(vt[cnt].dvRHW > 0)
  {
  // Check the rectangle
  if(vt[cnt].dvSX > tlx && vt[cnt].dvSX < brx)
  {
  if(vt[cnt].dvSY > tly && vt[cnt].dvSY < bry)
  {
  // Select the character
  character = v[cnt].color;
  add_select(character);
  if(first)
  {
  // Play the select speech for the first one picked
  sound = ChrList[character].wavespeech[SPEECH_SELECT];
  if( VALID_WAVE_RANGE(sound) )
  //play_sound_pvf(ChrList[character].getCap().waveindex[sound], PANMID, VOLMAX, 11025);
  first = false;
  }
  }
  }
  }
  cnt++;
  }
  */
}

//--------------------------------------------------------------------------------------------
Uint16 build_select_target(float tlx, float tly, float brx, float bry, Uint8 team)
{
  // ZZ> This function checks which characters are in the selection rectangle,
  //     and returns the first one found

  //D3DLVERTEX v[PRT_COUNT];
  //   D3DTLVERTEX vt[PRT_COUNT];
  //   int numbertocheck, character, cnt;

  //   // Figure out who to check
  //   numbertocheck = 0;
  //   // Add enemies first
  //   SCAN_CHR_BEGIN(cnt, rchr_cnt)
  //   {
  //       if(TeamList[team].hatesteam[rchr_cnt.team] && !rchr_cnt.is_inpack)
  //       {
  //           v[numbertocheck].x = (D3DVALUE) rchr_cnt.pos.x;
  //           v[numbertocheck].y = (D3DVALUE) rchr_cnt.pos.y;
  //           v[numbertocheck].z = (D3DVALUE) rchr_cnt.pos.z;
  //           v[numbertocheck].color = cnt;  // Store an index in the color slot...
  //           v[numbertocheck].dwReserved = 0;
  //           numbertocheck++;
  //       }
  //   } SCAN_CHR_END;
  //
  //   // Add allies next
  //   SCAN_CHR_BEGIN(cnt, rchr_cnt)
  //   {
  //       if(!TeamList[team].hatesteam[rchr_cnt.team] && rchr_cnt.on && !rchr_cnt.is_inpack)
  //       {
  //           v[numbertocheck].x = (D3DVALUE) rchr_cnt.pos.x;
  //           v[numbertocheck].y = (D3DVALUE) rchr_cnt.pos.y;
  //           v[numbertocheck].z = (D3DVALUE) rchr_cnt.pos.z;
  //           v[numbertocheck].color = cnt;  // Store an index in the color slot...
  //           v[numbertocheck].dwReserved = 0;
  //           numbertocheck++;
  //       }
  //   } SCAN_CHR_END;

  //   // Figure out where the points go onscreen
  //   lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &GCamera.mWorld);
  //   transform_vertices(numbertocheck, v, vt);

  //   cnt = 0;
  //   while(cnt < numbertocheck)
  //   {
  //       // Only check if in front of camera
  //       if(vt[cnt].dvRHW > 0)
  //       {
  //           // Check the rectangle
  //           if(vt[cnt].dvSX > tlx && vt[cnt].dvSX < brx)
  //           {
  //               if(vt[cnt].dvSY > tly && vt[cnt].dvSY < bry)
  //               {
  //                   // Select the character
  //                   character = v[cnt].color;
  //                   return character;
  //               }
  //           }
  //       }
  //       cnt++;
  //   }
  //   return Character_List::INVALID;

  return 0;
}

//--------------------------------------------------------------------------------------------
void move_rtsxy()
{
  // ZZ> This function iteratively transforms the cursor back to world coordinates

  //D3DLVERTEX v[1];
  //   D3DTLVERTEX vt[1];
  //   int numbertocheck, x, y, fan;
  //   float sin, cos, trailrate, level;

  //   // Figure out where the GRTS.xy is at on the screen
  //   fan = GMesh.getIndexPos(GRTS.x, GRTS.y);
  //   level = mesh_get_level(GRTS.x, GRTS.y, false);
  //   v[0].x = (D3DVALUE) GRTS.x;
  //   v[0].y = (D3DVALUE) GRTS.y;
  //   v[0].z = level;
  //   v[0].color = 0;
  //   v[0].dwReserved = 0;
  //   numbertocheck = 1;

  //   // Figure out where the points go onscreen
  //   lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &GCamera.mWorld);
  //   transform_vertices(numbertocheck, v, vt);

  //   if(vt[0].dvRHW < 0)
  //   {
  //       // Move it to camera_trackxy if behind the camera
  //       GRTS.x = GCamera.track_x;
  //       GRTS.y = GCamera.track_y;
  //   }
  //   else
  //   {
  //       // Move it to closer to the onscreen cursor
  //       trailrate = ABS(GUI.cursorx-vt[0].dvSX) + ABS(GUI.cursory-vt[0].dvSY);
  //       trailrate *= GRTS.trailrate;
  //       sin = sin_tab[GCamera.turn_lr]*trailrate;
  //       cos = cos_tab[GCamera.turn_lr]*trailrate;
  //       if(vt[0].dvSX < GUI.cursorx)
  //       {
  //           GRTS.x += cos;
  //           GRTS.y -= sin;
  //       }
  //       else
  //       {
  //           GRTS.x -= cos;
  //           GRTS.y += sin;
  //       }

  //       if(vt[0].dvSY < GUI.cursory)
  //       {
  //           GRTS.x += sin;
  //           GRTS.y += cos;
  //       }
  //       else
  //       {
  //           GRTS.x -= sin;
  //           GRTS.y -= cos;
  //       }
  //   }

}

//--------------------------------------------------------------------------------------------
void do_cursor_rts()
{
  //    // This function implements the RTS mouse cursor
  //    int stt_x, stt_y, end_x, end_y, target, leader;
  //    Sint16 sound;
  //
  //    if(GMous.button[1] == 0)
  //    {
  //        GUI.cursorx+=GMous.x;
  //        GUI.cursory+=GMous.y;
  //    }
  //    if(GUI.cursorx < 6)  GUI.cursorx = 6;  if (GUI.cursorx > scrx-16)  GUI.cursorx = scrx-16;
  //    if(GUI.cursory < 8)  GUI.cursory = 8;  if (GUI.cursory > scry-24)  GUI.cursory = scry-24;
  //    move_rtsxy();
  //    if(GMous.button[0])
  //    {
  //        // Moving the end select point
  //        GUI.pressed = true;
  //        GRTS.end_x = GUI.cursorx+5;
  //        GRTS.end_y = GUI.cursory+7;
  //
  //        // Draw the selection rectangle
  //        if(!allselect)
  //        {
  //            stt_x = GRTS.stt_x;  end_x = GRTS.end_x;  if(stt_x > end_x)  {  stt_x = GRTS.end_x;  end_x = GRTS.stt_x; }
  //            stt_y = GRTS.stt_y;  end_y = GRTS.end_y;  if(stt_y > end_y)  {  stt_y = GRTS.end_y;  end_y = GRTS.stt_y; }
  //            draw_trim_box(stt_x, stt_y, end_x, end_y);
  //        }
  //    }
  //    else
  //    {
  //        if(GUI.pressed)
  //        {
  //            // See if we selected anyone
  //            if((ABS(GRTS.stt_x - GRTS.end_x) + ABS(GRTS.stt_y - GRTS.end_y)) > 10 && !allselect)
  //            {
  //                // We drew a box alright
  //                stt_x = GRTS.stt_x;  end_x = GRTS.end_x;  if(stt_x > end_x)  {  stt_x = GRTS.end_x;  end_x = GRTS.stt_x; }
  //                stt_y = GRTS.stt_y;  end_y = GRTS.end_y;  if(stt_y > end_y)  {  stt_y = GRTS.end_y;  end_y = GRTS.stt_y; }
  //                build_select(stt_x, stt_y, end_x, end_y, GRTS.team_local);
  //            }
  //            else
  //            {
  //                // We want to issue an order
  //                if(GRTS.select_count > 0)
  //                {
  //                    leader = GRTS.select[0];
  //                    stt_x = GRTS.stt_x-20;  end_x = GRTS.stt_x+20;
  //                    stt_y = GRTS.stt_y-20;  end_y = GRTS.stt_y+20;
  //                    target = build_select_target(stt_x, stt_y, end_x, end_y, GRTS.team_local);
  //                    if( INVALID_CHR(target) )
  //                    {
  //                        // No target...
  //                        if(GKeyb.pressed(SDLK_LSHIFT) || GKeyb.pressed(SDLK_RSHIFT))
  //                        {
  //                            send_rts_order(GRTS.x, GRTS.y, RTSTERRAIN, target);
  //                            sound = ChrList[leader].wavespeech[SPEECH_TERRAIN];
  //                        }
  //                        else
  //                        {
  //                            send_rts_order(GRTS.x, GRTS.y, RTSMOVE, target);
  //                            sound = wldframe&1;  // Move or MoveAlt
  //                            sound = ChrList[leader].wavespeech[sound];
  //                        }
  //                    }
  //                    else
  //                    {
  //                        if(TeamList[GRTS.team_local].hatesteam[ChrList[target].team])
  //                        {
  //                            // Target is an enemy, so issue an attack order
  //                            send_rts_order(GRTS.x, GRTS.y, RTSATTACK, target);
  //                            sound = ChrList[leader].wavespeech[SPEECH_ATTACK];
  //                        }
  //                        else
  //                        {
  //                            // Target is a friend, so issue an assist order
  //                            send_rts_order(GRTS.x, GRTS.y, RTSASSIST, target);
  //                            sound = ChrList[leader].wavespeech[SPEECH_ASSIST];
  //                        }
  //                    }
  //                    // Do unit speech at 11025 KHz
  //                    if( VALID_WAVE_RANGE(sound) )
  //                    {
  ////REMOVE?      channel = Mix_PlayChannel(-1, ChrList[leader].getCap().waveindex[sound], 0);
  ////REMOVE?                        Mix_SetPosition(channel, 0, 0);
  //                        //WRONG FUNCTION. IF YOU WANT THIS TO WORK RIGHT, CONVERT TO //play_sound
  //                    }
  //                }
  //            }
  //            GUI.pressed = false;
  //        }
  //
  //        // Moving the select point
  //        GRTS.stt_x = GUI.cursorx+5;
  //        GRTS.stt_y = GUI.cursory+7;
  //        GRTS.end_x = GUI.cursorx+5;
  //        GRTS.end_y = GUI.cursory+7;
  //    }
  //
  //    // GAC - Don't forget to BeginText() and EndText();
  //    BeginText();
  //    draw_one_font(11, GUI.cursorx-5, GUI.cursory-7);
  //    EndText ();
}

//--------------------------------------------------------------------------------------------
void draw_main(Camera & cam)
{
  // ZZ> This function does all the drawing stuff
  //printf("DIAG: Drawing scene GMesh.numrenderlistref=%d\n",GMesh.numrenderlistref);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  draw_scene(cam);
  draw_text();
  flip_pages();
  allframe++;
  fpsframe++;
}


//--------------------------------------------------------------------------------------------
// The next two functions are borrowed from the gl_font.c test program from SDL_ttf
int powerOfTwo(int input)
{
  int value = 1;

  while (value < input)
  {
    value <<= 1;
  }
  return value;
}

//--------------------------------------------------------------------------------------------
int copySurfaceToTexture(SDL_Surface *surface, GLuint texture, GLfloat *texCoords)
{
  int w, h;
  SDL_Surface *image;
  SDL_Rect area;
  Uint32 saved_flags;
  Uint8 saved_alpha;

  // Use the surface width & height expanded to the next powers of two
  w = powerOfTwo(surface->w);
  h = powerOfTwo(surface->h);
  texCoords[0] = 0.0f;
  texCoords[1] = 0.0f;
  texCoords[2] = (GLfloat)surface->w / w;
  texCoords[3] = (GLfloat)surface->h / h;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 0x20, 0x0000ff, 0x00ff00, 0xff0000, 0xff000000);
#else
  image = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 0x20, 0xff000000, 0xff0000, 0x00ff00, 0x0000ff);
#endif

  if (image == NULL)
  {
    return 0;
  }

  // Save the alpha blending attributes
  saved_flags = surface->flags&(SDL_SRCALPHA|SDL_RLEACCELOK);
  saved_alpha = surface->format->alpha;
  if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA)
  {
    SDL_SetAlpha(surface, 0, 0);
  }

  // Copy the surface into the texture image
  area.x = 0;
  area.y = 0;
  area.w = surface->w;
  area.h = surface->h;
  SDL_BlitSurface(surface, &area, image, &area);

  // Restore the blending attributes
  if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA)
  {
    SDL_SetAlpha(surface, saved_flags, saved_alpha);
  }

  // Send the texture to OpenGL
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);

  // Don't need the extra image anymore
  SDL_FreeSurface(image);

  return 1;
}

//--------------------------------------------------------------------------------------------
void add_to_dolist(int cnt)
{
  // This function puts a character in the list
  int fan;
  const JF::MPD_Fan    * pfan;
  const JF::MPD_Vertex * vlst = GMesh.getVertices();

  if (!ChrList[cnt].indolist)
  {
    fan = GMesh.getIndexPos(ChrList[cnt].pos.x, ChrList[cnt].pos.y);

    ChrList[cnt].onwhichfan = fan;
    pfan = GMesh.getFan(fan);

    if(NULL!=pfan)
    {
      Uint32 vert = pfan->firstVertex;
      ChrList[cnt].light_level = (vlst[vert+0].light + vlst[vert+1].light + vlst[vert+2].light + vlst[vert+3].light)/4;
    }
    else
    {
      ChrList[cnt].light_level = 0;
    }

    {
      dolist[numdolist]     = cnt;
      ChrList[cnt].indolist = true;
      numdolist++;

      // Do flashing
      if ((allframe&ChrList[cnt].flashand) == 0 && ChrList[cnt].flashand!=DONTFLASH)
      {
        flash_character(cnt, 0xFF);
      }

      // Do blacking
      if ((allframe&SEEKURSEAND) == 0 && localseekurse && ChrList[cnt].iskursed)
      {
        flash_character(cnt, 0);
      }

    }
    //else
    //{
    //  // Double check for large/special objects
    //  if (ChrList[cnt].getCap().alwaysdraw)
    //  {
    //    dolist[numdolist] = cnt;
    //    ChrList[cnt].indolist = true;
    //    numdolist++;
    //  }
    //}

    // Add its weapons too
    if (VALID_CHR(ChrList[cnt].holding_which[SLOT_LEFT]))
      add_to_dolist(ChrList[cnt].holding_which[SLOT_LEFT]);
    if (VALID_CHR(ChrList[cnt].holding_which[SLOT_RIGHT]))
      add_to_dolist(ChrList[cnt].holding_which[SLOT_RIGHT]);
  }
}

//--------------------------------------------------------------------------------------------
void order_dolist(void)
{
  // ZZ> This function orders the dolist based on distance from camera,
  //     which is needed for reflections to properly clip themselves.
  //     Order from closest to farthest
  int cnt, tnc, character, order;
  int dist[CHR_COUNT];
  Uint16 olddolist[CHR_COUNT];

  // Figure the distance of each
  cnt = 0;
  while (cnt < numdolist)
  {
    character = dolist[cnt];  olddolist[cnt] = character;
    if (ChrList[character].light != 0xFF || ChrList[character].alpha != 0xFF)
    {
      // This makes stuff inside an invisible character visible...
      // A key inside a Jellcube, for example
      dist[cnt] = 0x7fffffff;
    }
    else
    {
      dist[cnt] = diff_abs_horiz(ChrList[character].pos,GCamera.pos);
    }
    cnt++;
  }

  // Put em in the right order
  cnt = 0;
  while (cnt < numdolist)
  {
    character = olddolist[cnt];
    order = 0;  // Assume this character is closest
    tnc = 0;
    while (tnc < numdolist)
    {
      // For each one closer, increment the order
      order += (dist[cnt] > dist[tnc]);
      order += (dist[cnt] == dist[tnc]) && (cnt < tnc);
      tnc++;
    }
    dolist[order] = character;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_dolist(void)
{
  // ZZ> This function finds the characters that need to be drawn and puts them in the list
  int cnt, character;

  // Remove everyone from the dolist
  cnt = 0;
  while (cnt < numdolist)
  {
    character = dolist[cnt];
    ChrList[character].indolist = false;
    cnt++;
  }
  numdolist = 0;

  // Now fill it up again
  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if ( rchr_cnt.is_inpack ) continue;

    add_to_dolist(cnt);      // Add the character

  } SCAN_CHR_END;
}


//--------------------------------------------------------------------------------------------
Tile_Dam::Tile_Dam()
{
  amount  = 0x0100;                               // Amount of damage
  type    = DAMAGE_FIRE;                        // Type of damage
};

//--------------------------------------------------------------------------------------------
Tile_Anim::Tile_Anim()
{
  updateand    = (1<<3)-1;                          // New tile every 7 frames
  frameadd     = 0;                // Current frame

  frameand     = (1<<2)-1;             // Only 4 frames
  baseand      = ~frameand;            //

  bigframeand  = (1<<3)-1;             // For big tiles
  bigbaseand   = ~bigframeand;         //
};

//---------------------------------------------------------------------------------------------
void Locker_3DMode::my_begin(Camera & cam)
{
  my_begun = false;
  if(my_ended)
  {
    mode = MODE_3D;

    glPushAttrib(GL_TRANSFORM_BIT|GL_ENABLE_BIT);

      glMatrixMode(GL_PROJECTION);
      glLoadMatrixf(cam.mProjection.v);

      glMatrixMode(GL_MODELVIEW);
      glLoadMatrixf(cam.mView.v);

      glEnable(GL_TEXTURE_2D);

    my_begun = true;
    my_ended = false;
  };
};

//---------------------------------------------------------------------------------------------
bool Locker_3DMode::my_end()
{
  my_ended = false;
  if(my_begun)
  {
    glPopAttrib();

    my_begun = false;
    my_ended = true;
  };

  return my_ended;
};


//---------------------------------------------------------------------------------------------
void Locker_2DMode::my_begin()
{
  my_begun = false;
  if(my_ended)
  {
    mode     = MODE_2D;

    glPushAttrib(GL_TRANSFORM_BIT|GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT);

      // Set up an orthogonal projection
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, scrx, scry, 0, -1, 1);

      // "disable" the modelview mode
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      glEnable(GL_TEXTURE_2D);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glAlphaFunc(GL_GREATER,0);
      glEnable(GL_ALPHA_TEST);

      glColor4f(1,1,1,1);
      glNormal3f(0, 0, 1);   // glNormal3f( 0, 1, 0 );

    my_begun = true;
    my_ended = false;
  };

};

//---------------------------------------------------------------------------------------------
bool Locker_2DMode::my_end()
{
  my_ended = false;
  if(my_begun)
  {
    glPopAttrib();

    my_begun = false;
    my_ended = true;
  };

  return my_ended;
};

void dump_gl_state(char * str, bool modelview, bool projection, bool texture)
{
  GLint     iTmp[16];
  GLboolean bTmp[16];
  GLfloat   fTmp[16];

  cout << endl << "==============================" << endl;
  if(str!=NULL)
  {
    cout << "----" << str << "----" << endl << endl;
  };

  //----------------------------------------
  // Transformation

  cout << "---- transformation ----" << endl;

  cout << "  GL_MODELVIEW_STACK_DEPTH  ";
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH,iTmp);
    cout << iTmp[0] << endl;

  if(modelview)
  {
    glGetFloatv(GL_MODELVIEW_MATRIX, fTmp);
    cout << "  <" << endl;
    for(int i=0, k=0; i<4; i++)
    {
      cout << "    <";
      for(int j=0; j<4; j++, k++)
      {
        cout << fTmp[k] << ", ";
      }
      cout << ">" << endl;
    }
    cout << "  >" << endl << endl;
  };

  cout << "  GL_PROJECTION_STACK_DEPTH ";
    glGetIntegerv(GL_PROJECTION_STACK_DEPTH,iTmp);
    cout  << iTmp[0] << endl;

  if(projection)
  {
    glGetFloatv(GL_PROJECTION_MATRIX, fTmp);
    cout << "  <" << endl;
    for(int i=0, k=0; i<4; i++)
    {
      cout << "    <";
      for(int j=0; j<4; j++, k++)
      {
        cout << fTmp[k] << ", ";
      }
      cout << ">" << endl;
    }
    cout << "  >" << endl << endl;
  };

  cout << "  GL_TEXTURE_STACK_DEPTH    ";
    glGetIntegerv(GL_TEXTURE_STACK_DEPTH,iTmp);
    cout << iTmp[0] << endl;

  if(texture)
  {
    glGetFloatv(GL_TEXTURE_MATRIX, fTmp);
    cout << "  <" << endl;
    for(int i=0, k=0; i<4; i++)
    {
      cout << "    <";
      for(int j=0; j<4; j++, k++)
      {
        cout << fTmp[k] << ", ";
      }
      cout << ">" << endl;
    }
    cout << "  >" << endl << endl;
  };

  cout << "  GL_MATRIX_MODE            ";
    glGetIntegerv(GL_MATRIX_MODE,iTmp);
    switch(iTmp[0])
    {
      case GL_MODELVIEW : cout << "GL_MODELVIEW"; break;
      case GL_PROJECTION: cout << "GL_PROJECTION"; break;
      case GL_TEXTURE   : cout << "GL_TEXTURE"; break;
      default: cout << "UNKNOWN";
    };
    cout << endl;

  cout << "  GL_NORMALIZE              ";
    iTmp[0] = glIsEnabled(GL_NORMALIZE);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;


  cout << "  GL_DEPTH_RANGE            ";
    glGetFloatv(GL_DEPTH_RANGE,fTmp);
    cout << fTmp[0] << ", " << fTmp[1]  << endl;

  //GL_VIEWPORT Viewport origin	and extent	viewport - glGetIntegerv
  //GL_CLIP_PLANEi User clipping	plane coefficients	transform 0, 0, 0, 0 glGetClipPlane
  //GL_CLIP_PLANEi ith user clipping	transform/	GL_FALSE glIsEnabled

  //----------------------------------------
  // Coloring

  cout << "---- coloring ----" << endl;

  cout << "  GL_SHADE_MODEL            ";
    glGetIntegerv(GL_SHADE_MODEL, iTmp);
    switch(iTmp[0])
    {
      case GL_SMOOTH: cout << "GL_SMOOTH"; break;
      case GL_FLAT: cout << "GL_FLAT"; break;
      default: cout << "UNKNOWN";
    };
    cout << endl;

  //GL_FOG_COLOR Fog color fog 0, 0, 0, 0 glGetFloatv()
  //GL_FOG_INDEX Fog index fog 0 glGetFloatv()
  //GL_FOG_DENSITY Exponential fog density fog 1.0 glGetFloatv()
  //GL_FOG_START Linear fog start fog 0.0 glGetFloatv()
  //GL_FOG_END Linear fog end fog 1.0 glGetFloatv()
  //GL_FOG_MODE Fog mode fog GL_EXP glGetIntegerv()
  //GL_FOG True if fog enabled fog/enable GL_FALSE glIsEnabled()

  //----------------------------------------
  // Rasterization

  cout << "---- rasterization ----" << endl;

  cout << "  GL_LINE_SMOOTH            ";
    iTmp[0] = glIsEnabled(GL_LINE_SMOOTH);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  cout << "  GL_POLYGON_SMOOTH         ";
    iTmp[0] = glIsEnabled(GL_POLYGON_SMOOTH);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  cout << "  GL_POLYGON_MODE           ";
    glGetIntegerv(GL_POLYGON_MODE, iTmp);
    switch(iTmp[0])
    {
      case GL_FRONT_AND_BACK: cout << "GL_FRONT_AND_BACK"; break;
      case GL_FRONT: cout << "GL_FRONT"; break;
      case GL_BACK: cout << "GL_BACK"; break;
      case GL_POINT: cout << "GL_POINT"; break;
      case GL_LINE: cout << "GL_LINE"; break;
      case GL_FILL: cout << "GL_FILL"; break;
      default: cout << "UNKNOWN"; break;
    };
    cout << endl;

  cout << "  GL_CULL_FACE              ";
    iTmp[0] = glIsEnabled(GL_CULL_FACE);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  if(iTmp[0]==GL_TRUE)
  {
    cout << "  GL_CULL_FACE_MODE       ";
      glGetIntegerv(GL_CULL_FACE_MODE,iTmp);
      switch(iTmp[0])
      {
        case GL_FRONT: cout << "GL_FRONT"; break;
        case GL_BACK: cout << "GL_BACK"; break;
        case GL_FRONT_AND_BACK: cout << "GL_FRONT_AND_BACK"; break;
        default: cout << "UNKNOWN";
      };
      cout  << endl;

    cout << "  GL_FRONT_FACE            ";
      glGetIntegerv(GL_CULL_FACE_MODE,iTmp);
      cout << ( iTmp[0]==GL_CW ? "GL_CW" : "GL_CCW" ) << endl;
  }

  //GL_POINT_SIZE Point size point 1.0 glGetFloatv()
  //GL_LINE_WIDTH Line width line 1.0 glGetFloatv()
  //GL_LINE_STIPPLE_PATTERN Line stipple line 1's glGetIntegerv()
  //GL_LINE_STIPPLE_REPEAT Line stipple repeat line 1 glGetIntegerv()
  //GL_LINE_STIPPLE Line stipple enable line/enable GL_FALSE glIsEnabled()
  //GL_POLYGON_OFFSET_FACTOR Polygon offset factor polygon 0 glGetFloatv()
  //GL_POLYGON_OFFSET_BIAS Polygon offset bias polygon 0 glGetFloatv()
  //GL_POLYGON_OFFSET_POINT Polygon offset enable	for GL_POINT mode	rasterization	polygon/enable GL_FALSE glIsEnabled()
  //GL_POLYGON_OFFSET_LINE Polygon offset enable	for GL_LINE mode	rasterization	polygon/enable GL_FALSE glIsEnabled()
  //GL_POLYGON_OFFSET_FILL Polygon offset enable	for GL_FILL mode	rasterization	polygon/enable GL_FALSE glIsEnabled()
  //GL_POLYGON_STIPPLE Polygon stipple enable polygon/enable GL_FALSE glIsEnabled()
  //- Polygon stipple pattern polygon-stipple 1's glGetPolygon-	Stipple()

  //----------------------------------------
  // Texturing

  cout << "---- texturing ----" << endl;

  cout << "  GL_TEXTURE_1D             ";
    iTmp[0] = glIsEnabled(GL_TEXTURE_1D);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  if(iTmp[0]==GL_TRUE)
  {
    cout << "    GL_TEXTURE_BINDING_1D   ";
      glGetIntegerv(GL_TEXTURE_BINDING_1D, iTmp);
      cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;
  };

  cout << "  GL_TEXTURE_2D             ";
    iTmp[0] = glIsEnabled(GL_TEXTURE_2D);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  if(iTmp[0]==GL_TRUE)
  {
    cout << "    GL_TEXTURE_BINDING_2D   ";
      glGetIntegerv(GL_TEXTURE_BINDING_2D, iTmp);
      cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;
  };

  //GL_TEXTURE x-D texture image	at level of detail i	- - glGetTexImage()
  //GL_TEXTURE_WIDTH x-D texture image	i's width	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_HEIGHT x-D texture image	i's height	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_BORDER x-D texture image	i's border width	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_INTERNAL	_FORMAT	x-D texture image	i's internal image	format	- 1 glGetTexLevelParameter*()
  //GL_TEXTURE_RED_SIZE x-D texture image	i's red resolution	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_GREEN_SIZE x-D texture image	i's green	resolution	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_BLUE_SIZE x-D texture image	i's blue resolution	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_ALPHA_SIZE x-D texture image	i's alpha	resolution	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_LUMINANCE_SIZE x-D texture image	i's luminance	resolution	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_INTENSITY_SIZE x-D texture image	i's intensity	resolution	- 0 glGetTexLevelParameter*()
  //GL_TEXTURE_BORDER_COLOR Texture border	color	texture 0, 0, 0, 0 glGetTexParameter*()
  //GL_TEXTURE_MIN_FILTER Texture	minification	function	texture GL_	NEAREST_	MIPMAP_	LINEAR	glGetTexParameter*()
  //GL_TEXTURE_MAG_FILTER Texture	magnification	function	texture GL_LINEAR glGetTexParameter*()
  //GL_TEXTURE_WRAP_x Texture wrap	mode (x is S or T)	texture GL_REPEAT glGetTexParameter*()
  //GL_TEXTURE_PRIORITY Texture object	priority	texture 1 glGetTexParameter*()
  //GL_TEXTURE_RESIDENCY Texture residency texture GL_FALSE glGetTexParameteriv()
  //GL_TEXTURE_ENV_MODE Texture	application	function	texture GL_	MODULATE	glGetTexEnviv()
  //GL_TEXTURE_ENV_COLOR Texture	environment color	texture 0, 0, 0, 0 glGetTexEnvfv()
  //GL_TEXTURE_GEN_x Texgen enabled	(x is S, T, R, or	Q)	texture/e	nable	GL_FALSE glIsEnabled()
  //GL_EYE_PLANE Texgen plane	equation	coefficients	texture - glGetTexGenfv()
  //GL_OBJECT_PLANE Texgen object	linear coefficients	texture - glGetTexGenfv()
  //GL_TEXTURE_GEN_MODE Function used for	texgen	texture GL_EYE_	LINEAR	glGetTexGeniv()

  //----------------------------------------
  // Pixel Operations

  cout << "---- pixel operations ----" << endl;

  cout << "  GL_SCISSOR_TEST           ";
    iTmp[0] = glIsEnabled(GL_DEPTH_TEST);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  if(iTmp[0])
  {
    cout << "    GL_SCISSOR_BOX          ";
      glGetIntegerv(GL_SCISSOR_BOX, iTmp);
      cout << "<" << iTmp[0] << ", " << iTmp[1] << ", " << iTmp[2] << ", " << iTmp[3] << ">" << endl;
  }

  cout << "  GL_ALPHA_TEST             ";
    iTmp[0] = glIsEnabled(GL_ALPHA_TEST);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  if(iTmp[0]==GL_TRUE)
  {
    cout << "    GL_ALPHA_TEST_FUNC      ";
      glGetIntegerv(GL_ALPHA_TEST_FUNC, iTmp);
      switch(iTmp[0])
      {
        case GL_NEVER: cout << "GL_NEVER"; break;
        case GL_ALWAYS: cout << "GL_ALWAYS"; break;
        case GL_LESS: cout << "GL_LESS"; break;
        case GL_LEQUAL: cout << "GL_LEQUAL"; break;
        case GL_EQUAL: cout << "GL_EQUAL"; break;
        case GL_GEQUAL: cout << "GL_GEQUAL"; break;
        case GL_GREATER: cout << "GL_EQUAL"; break;
        case GL_NOTEQUAL: cout << "GL_GEQUAL"; break;
        default: cout << "UNKNOWN";
      };
      glGetIntegerv(GL_ALPHA_TEST_REF, iTmp);
      cout << " " << iTmp[0] << endl;
  };

  cout << "  GL_STENCIL_TEST           ";
    iTmp[0] = glIsEnabled(GL_STENCIL_TEST);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  if(iTmp[0]==GL_TRUE)
  {
    cout << "    GL_STENCIL_FUNC    ";
      glGetIntegerv(GL_STENCIL_FUNC, iTmp);
      switch(iTmp[0])
      {
        case GL_NEVER: cout << "GL_NEVER"; break;
        case GL_ALWAYS: cout << "GL_ALWAYS"; break;
        case GL_LESS: cout << "GL_LESS"; break;
        case GL_LEQUAL: cout << "GL_LEQUAL"; break;
        case GL_EQUAL: cout << "GL_EQUAL"; break;
        case GL_GEQUAL: cout << "GL_GEQUAL"; break;
        case GL_GREATER: cout << "GL_EQUAL"; break;
        case GL_NOTEQUAL: cout << "GL_GEQUAL"; break;
        default: cout << "UNKNOWN";
      };
      cout << " ";
      cout << "  GL_STENCIL_REF      ";
        glGetIntegerv(GL_STENCIL_REF, iTmp);
        cout << iTmp[0] << endl;

    cout << "    GL_STENCIL_VALUE_MASK  ";
      glGetIntegerv(GL_STENCIL_VALUE_MASK, iTmp);
      cout << iTmp[0] << endl;

    cout << "    GL_STENCIL_FAIL    ";
      glGetIntegerv(GL_STENCIL_FAIL, iTmp);
      switch(iTmp[0])
      {
        case GL_KEEP: cout << "GL_KEEP"; break;
        case GL_ZERO: cout << "GL_ZERO"; break;
        case GL_REPLACE: cout << "GL_REPLACE"; break;
        case GL_INCR: cout << "GL_INCR"; break;
        case GL_DECR: cout << "GL_DECR"; break;
        case GL_INVERT: cout << "GL_INVERT"; break;
        default: cout << "UNKNOWN";
      };

    cout << "    GL_STENCIL_PASS_DEPTH_FAIL    ";
      glGetIntegerv(GL_STENCIL_FAIL, iTmp);
      switch(iTmp[0])
      {
        case GL_KEEP: cout << "GL_KEEP"; break;
        case GL_ZERO: cout << "GL_ZERO"; break;
        case GL_REPLACE: cout << "GL_REPLACE"; break;
        case GL_INCR: cout << "GL_INCR"; break;
        case GL_DECR: cout << "GL_DECR"; break;
        case GL_INVERT: cout << "GL_INVERT"; break;
        default: cout << "UNKNOWN";
      };

    cout << "    GL_STENCIL_PASS_DEPTH_PASS    ";
      glGetIntegerv(GL_STENCIL_FAIL, iTmp);
      switch(iTmp[0])
      {
        case GL_KEEP: cout << "GL_KEEP"; break;
        case GL_ZERO: cout << "GL_ZERO"; break;
        case GL_REPLACE: cout << "GL_REPLACE"; break;
        case GL_INCR: cout << "GL_INCR"; break;
        case GL_DECR: cout << "GL_DECR"; break;
        case GL_INVERT: cout << "GL_INVERT"; break;
        default: cout << "UNKNOWN";
      };

  };

  cout << "  GL_DEPTH_TEST             ";
    iTmp[0] = glIsEnabled(GL_DEPTH_TEST);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  if(iTmp[0]==GL_TRUE)
  {
    cout << "    GL_DEPTH_FUNC           ";
      glGetIntegerv(GL_DEPTH_FUNC, iTmp);
      switch(iTmp[0])
      {
        case GL_NEVER: cout << "GL_NEVER"; break;
        case GL_ALWAYS: cout << "GL_ALWAYS"; break;
        case GL_LESS: cout << "GL_LESS"; break;
        case GL_LEQUAL: cout << "GL_LEQUAL"; break;
        case GL_EQUAL: cout << "GL_EQUAL"; break;
        case GL_GEQUAL: cout << "GL_GEQUAL"; break;
        case GL_GREATER: cout << "GL_EQUAL"; break;
        case GL_NOTEQUAL: cout << "GL_GEQUAL"; break;
        default: cout << "UNKNOWN";
      };
      cout << endl;
  };

  cout << "  GL_BLEND                  ";
    iTmp[0] = glIsEnabled(GL_BLEND);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  if(iTmp[0]==GL_TRUE)
  {
    cout << "    GL_BLEND_SRC            ";
      glGetIntegerv(GL_BLEND_SRC, iTmp);
      switch(iTmp[0])
      {
        case GL_ZERO: cout << "GL_ZERO"; break;
        case GL_ONE: cout << "GL_ONE"; break;
        case GL_DST_COLOR: cout << "GL_DST_COLOR"; break;
        case GL_SRC_COLOR: cout << "GL_SRC_COLOR"; break;
        case GL_ONE_MINUS_DST_COLOR: cout << "GL_ONE_MINUS_DST_COLOR"; break;
        case GL_ONE_MINUS_SRC_COLOR: cout << "GL_ONE_MINUS_SRC_COLOR"; break;
        case GL_SRC_ALPHA: cout << "GL_SRC_ALPHA"; break;
        case GL_ONE_MINUS_SRC_ALPHA: cout << "GL_ONE_MINUS_SRC_ALPHA"; break;
        case GL_DST_ALPHA: cout << "GL_DST_ALPHA"; break;
        case GL_ONE_MINUS_DST_ALPHA: cout << "GL_ONE_MINUS_DST_ALPHA"; break;
        case GL_SRC_ALPHA_SATURATE: cout << "GL_SRC_ALPHA_SATURATE"; break;

        default: cout << "UNKNOWN";
      };
      cout << endl;

    cout << "    GL_BLEND_DST            ";
      glGetIntegerv(GL_BLEND_DST, iTmp);
      switch(iTmp[0])
      {
        case GL_ZERO: cout << "GL_ZERO"; break;
        case GL_ONE: cout << "GL_ONE"; break;
        case GL_DST_COLOR: cout << "GL_DST_COLOR"; break;
        case GL_SRC_COLOR: cout << "GL_SRC_COLOR"; break;
        case GL_ONE_MINUS_DST_COLOR: cout << "GL_ONE_MINUS_DST_COLOR"; break;
        case GL_ONE_MINUS_SRC_COLOR: cout << "GL_ONE_MINUS_SRC_COLOR"; break;
        case GL_SRC_ALPHA: cout << "GL_SRC_ALPHA"; break;
        case GL_ONE_MINUS_SRC_ALPHA: cout << "GL_ONE_MINUS_SRC_ALPHA"; break;
        case GL_DST_ALPHA: cout << "GL_DST_ALPHA"; break;
        case GL_ONE_MINUS_DST_ALPHA: cout << "GL_ONE_MINUS_DST_ALPHA"; break;
        case GL_SRC_ALPHA_SATURATE: cout << "GL_SRC_ALPHA_SATURATE"; break;

        default: cout << "UNKNOWN";
      };
       cout << endl;
  };

  cout << "  GL_DITHER                 ";
    iTmp[0] = glIsEnabled(GL_DITHER);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  cout << "  GL_INDEX_LOGIC_OP         ";
    iTmp[0] = glIsEnabled(GL_INDEX_LOGIC_OP);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  cout << "  GL_COLOR_LOGIC_OP         ";
    iTmp[0] = glIsEnabled(GL_COLOR_LOGIC_OP);
    cout << ( iTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  cout << "    GL_LOGIC_OP_MODE        ";
    glGetIntegerv(GL_LOGIC_OP_MODE, iTmp);
    switch(iTmp[0])
    {
      case GL_CLEAR: cout << "GL_CLEAR"; break;
      case GL_AND: cout << "GL_AND"; break;
      case GL_COPY: cout << "GL_COPY"; break;
      case GL_OR: cout << "GL_OR"; break;
      case GL_NOOP: cout << "GL_NOOP"; break;
      case GL_NAND: cout << "GL_NAND"; break;
      case GL_SET: cout << "GL_SET"; break;
      case GL_NOR: cout << "GL_NOR"; break;
      case GL_COPY_INVERTED: cout << "GL_COPY_INVERTED"; break;
      case GL_XOR: cout << "GL_XOR"; break;
      case GL_INVERT: cout << "GL_INVERT"; break;
      case GL_EQUIV: cout << "GL_EQUIV"; break;
      case GL_AND_REVERSE: cout << "GL_AND_REVERSE"; break;
      case GL_AND_INVERTED: cout << "GL_AND_INVERTED"; break;
      case GL_OR_REVERSE: cout << "GL_OR_REVERSE"; break;
      case GL_OR_INVERTED: cout << "GL_OR_INVERTED"; break;

      default: cout << "UNKNOWN";
    };
    cout << endl;

  //----------------------------------------
  // Framebuffer Control

  cout << "---- framebuffer control ----" << endl;

  cout << "  GL_DRAW_BUFFER          ";
    glGetIntegerv(GL_DRAW_BUFFER, iTmp);
    cout << iTmp[0] << endl;

  cout << "  GL_STENCIL_WRITEMASK    ";
    glGetIntegerv(GL_STENCIL_WRITEMASK, iTmp);
    cout << iTmp[0] << endl;

  cout << "  GL_STENCIL_CLEAR_VALUE  ";
    glGetIntegerv(GL_STENCIL_WRITEMASK, iTmp);
    cout << iTmp[0] << endl;


  cout << "  GL_DEPTH_WRITEMASK      ";
    glGetBooleanv(GL_DEPTH_WRITEMASK, bTmp);
    cout << ( bTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  cout << "  GL_DEPTH_CLEAR_VALUE    ";
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, fTmp);
    cout << fTmp[0] << endl;


  cout << "  GL_INDEX_WRITEMASK      ";
    glGetIntegerv(GL_INDEX_WRITEMASK, iTmp);
    cout << iTmp[0] << endl;

  cout << "  GL_INDEX_CLEAR_VALUE    ";
    glGetFloatv(GL_INDEX_CLEAR_VALUE, fTmp);
    cout << fTmp[0] << endl;


  cout << "  GL_COLOR_WRITEMASK      ";
    glGetBooleanv(GL_COLOR_WRITEMASK, bTmp);
    cout << ( bTmp[0]==GL_TRUE ? "GL_TRUE" : "GL_FALSE" ) << endl;

  cout << "  GL_COLOR_CLEAR_VALUE    ";
    glGetFloatv(GL_COLOR_CLEAR_VALUE, fTmp);
    cout << "<" << fTmp[0] << ", " << fTmp[1] << ", " << fTmp[2] << ", " << fTmp[3] << ">" << endl;


  cout << "  GL_ACCUM_CLEAR_VALUE    ";
    glGetFloatv(GL_ACCUM_CLEAR_VALUE, fTmp);
    cout << "<" << fTmp[0] << ", " << fTmp[1] << ", " << fTmp[2] << ", " << fTmp[3] << ">" << endl;

  //----------------------------------------
  // Pixels

  cout << "---- pixels ----" << endl;

  cout << "  color scale             ";
    cout << "<";
    glGetFloatv(GL_RED_SCALE, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_GREEN_SCALE, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_BLUE_SCALE, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_ALPHA_SCALE, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_DEPTH_SCALE, fTmp);
    cout << fTmp[0] << ">" << endl;

  cout << "  color bias              ";
    cout << "<";
    glGetFloatv(GL_RED_BIAS, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_GREEN_BIAS, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_BLUE_BIAS, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_ALPHA_BIAS, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_DEPTH_BIAS, fTmp);
    cout << fTmp[0] << ">" << endl;

  cout << "  zoom                    ";
    cout << "<";
    glGetFloatv(GL_ZOOM_X, fTmp);
    cout << fTmp[0] << ", ";
    glGetFloatv(GL_ZOOM_Y, fTmp);
    cout << fTmp[0] << ">" << endl;

  //GL_UNPACK_SWAP_BYTES Value of	GL_UNPACK_SWAP_BYTES	pixel-store GL_FALSE glGetBooleanv()
  //GL_UNPACK_LSB_FIRST Value of	GL_UNPACK_LSB_FIRST	pixel-store GL_FALSE glGetBooleanv()
  //GL_UNPACK_ROW_LENGTH Value of	GL_UNPACK_ROW_LENGTH	pixel-store 0 glGetIntegerv()
  //GL_UNPACK_SKIP_ROWS Value of	GL_UNPACK_SKIP_ROWS	pixel-store 0 glGetIntegerv()
  //GL_UNPACK_SKIP_PIXELS Value of	GL_UNPACK_SKIP_PIXELS	pixel-store 0 glGetIntegerv()
  //GL_UNPACK_ALIGNMENT Value of	GL_UNPACK_ALIGNMENT	pixel-store 4 glGetIntegerv()
  //GL_PACK_SWAP_BYTES Value of	GL_PACK_SWAP_BYTES	pixel-store GL_FALSE glGetBooleanv()
  //GL_PACK_LSB_FIRST Value of	GL_PACK_LSB_FIRST	pixel-store GL_FALSE glGetBooleanv()
  //GL_PACK_ROW_LENGTH Value of	GL_PACK_ROW_LENGTH	pixel-store 0 glGetIntegerv()
  //GL_PACK_SKIP_ROWS Value of	GL_PACK_SKIP_ROWS pixel-store 0 glGetIntegerv()
  //GL_PACK_SKIP_PIXELS Value of	GL_PACK_SKIP_PIXELS	pixel-store 0 glGetIntegerv()
  //GL_PACK_ALIGNMENT Value of	GL_PACK_ALIGNMENT	pixel-store 4 glGetIntegerv()
  //GL_MAP_COLOR True if colors are mapped pixel GL_FALSE glGetBooleanv()
  //GL_MAP_STENCIL True if stencil values are mapped pixel GL_FALSE glGetBooleanv()
  //GL_INDEX_SHIFT Value of GL_INDEX_SHIFT pixel 0 glGetIntegerv()
  //GL_INDEX_OFFSET Value of GL_INDEX_OFFSET pixel 0 glGetIntegerv()
  //GL_x glPixelMap() translation tables; x	is a map name from Table 8-1	- 0's glGetPixelMap*()
  //GL_x_SIZE Size of table x - 1 glGetIntegerv()
  //GL_READ_BUFFER Read source buffer pixel - glGetIntegerv()

  //----------------------------------------
  // Hints

  cout << "---- hints ----" << endl;

  cout << "  GL_PERSPECTIVE_CORRECTION_HINT ";
    glGetIntegerv(GL_PERSPECTIVE_CORRECTION_HINT, iTmp);
    switch(iTmp[0])
    {
      case GL_FASTEST: cout << "GL_FASTEST"; break;
      case GL_NICEST: cout << "GL_NICEST"; break;
      case GL_DONT_CARE: cout << "GL_DONT_CARE"; break;
      default: cout << "UNKNOWN";
    };
    cout << endl;

  cout << "  GL_POINT_SMOOTH_HINT           ";
    glGetIntegerv(GL_POINT_SMOOTH_HINT, iTmp);
    switch(iTmp[0])
    {
      case GL_FASTEST: cout << "GL_FASTEST"; break;
      case GL_NICEST: cout << "GL_NICEST"; break;
      case GL_DONT_CARE: cout << "GL_DONT_CARE"; break;
      default: cout << "UNKNOWN";
    };
    cout << endl;

  cout << "  GL_LINE_SMOOTH_HINT            ";
    glGetIntegerv(GL_LINE_SMOOTH_HINT, iTmp);
    switch(iTmp[0])
    {
      case GL_FASTEST: cout << "GL_FASTEST"; break;
      case GL_NICEST: cout << "GL_NICEST"; break;
      case GL_DONT_CARE: cout << "GL_DONT_CARE"; break;
      default: cout << "UNKNOWN";
    };
    cout << endl;

  cout << "  GL_POLYGON_SMOOTH_HINT         ";
    glGetIntegerv(GL_POLYGON_SMOOTH_HINT, iTmp);
    switch(iTmp[0])
    {
      case GL_FASTEST: cout << "GL_FASTEST"; break;
      case GL_NICEST: cout << "GL_NICEST"; break;
      case GL_DONT_CARE: cout << "GL_DONT_CARE"; break;
      default: cout << "UNKNOWN";
    };
    cout << endl;

  cout << "  GL_FOG_HINT                    ";
    glGetIntegerv(GL_FOG_HINT, iTmp);
    switch(iTmp[0])
    {
      case GL_FASTEST: cout << "GL_FASTEST"; break;
      case GL_NICEST: cout << "GL_NICEST"; break;
      case GL_DONT_CARE: cout << "GL_DONT_CARE"; break;
      default: cout << "UNKNOWN";
    };
    cout << endl;

  //----------------------------------------
  // Miscellaneous

  cout << "---- miscellaneous ----" << endl;

  cout << "  GL_CLIENT_ATTRIB_STACK_DEPTH  ";
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH, iTmp);
    cout << iTmp[0] << endl;

  cout << "  GL_ATTRIB_STACK_DEPTH  ";
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH, iTmp);
    cout << iTmp[0] << endl;

  cout << "  GL_RENDER_MODE         ";
    glGetIntegerv(GL_RENDER_MODE, iTmp);
    switch(iTmp[0])
    {
      case GL_RENDER: cout << "GL_RENDER"; break;
      case GL_SELECT: cout << "GL_SELECT"; break;
      case GL_FEEDBACK: cout << "GL_FEEDBACK"; break;
      default: cout << "UNKNOWN";
    };
    cout << endl;

  GLenum errCode = glGetError();
  if (errCode != GL_NO_ERROR)
  {
    cout << "GL Error : " << gluErrorString(errCode) << endl;
  }

  //GL_LIST_BASE Setting of glListBase() list 0 glGetIntegerv()
  //GL_LIST_INDEX Number of display list	under construction; 0	if none	- 0 glGetIntegerv()
  //GL_LIST_MODE Mode of display list	under construction;	undefined if none	- 0 glGetIntegerv()
  //GL_NAME_STACK_DEPTH Name stack depth - 0 glGetIntegerv()
  //GL_SELECTION_BUFFER_POINTER Pointer to selection	buffer	select 0 glGetPointerv()
  //GL_SELECTION_BUFFER_SIZE Size of selection	buffer	select 0 glGetIntegerv()
  //GL_FEEDBACK_BUFFER_POINTER Pointer to feedback	buffer	feedback 0 glGetPointerv()
  //GL_FEEDBACK_BUFFER_SIZE Size of feedback	buffer	feedback 0 glGetIntegerv()
  //GL_FEEDBACK_BUFFER_TYPE Type of feedback	buffer	feedback GL_2D glGetIntegerv()

  if(str!=NULL)
  {
    cout << endl << "----" << str << "----" << endl;
  };
  cout << "==============================" << endl << endl;

};


void reset_graphics()
{
  /* Depth testing stuff */
  glDepthMask(GL_FALSE);
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LESS );

  // glDepthFunc() valid values GL_NEVER, GL_ALWAYS, GL_LESS, GL_LEQUAL,
  //                            GL_EQUAL, GL_GEQUAL, GL_GREATER, or GL_NOTEQUAL.

  /* Enable a single OpenGL light. */
  //GLfloat intensity[] = {1.0,1.0,1.0,1.0};
  //glLightfv(GL_LIGHT0, GL_SPECULAR, light_diffuse);
  //glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  //glEnable(GL_LIGHT0);
  //glDisable(GL_LIGHTING);
  //glLightModelfv(GL_LIGHT_MODEL_AMBIENT,intensity);
  //glEnable(GL_COLOR_MATERIAL);  // Need this for color + lighting

  glEnable(GL_TEXTURE_2D);    // Enable texture mapping
  glShadeModel( shading );

  //perspective correction
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, perspective);

  //antialiasing
  glHint(GL_POLYGON_SMOOTH_HINT, antialias );
  glHint(GL_LINE_SMOOTH_HINT,    antialias );
  glHint(GL_POINT_SMOOTH_HINT,   antialias );
  if(antialias != GL_FASTEST) glEnable(GL_POLYGON_SMOOTH);

  //fill mode
  glPolygonMode(GL_FRONT, fillmode_front);
  glPolygonMode(GL_BACK,  fillmode_back );

  //dithering
  if(dither)
    glEnable(GL_DITHER);
  else
    glDisable(GL_DITHER);

  /* Backface culling */
  //glEnable (GL_CULL_FACE);   // This seems implied - DDOI
  //glCullFace( GL_BACK );

};


void Profile::load_pips(const char * pathname)
{
  char newloadname[0x0100];

  for (int cnt = 0; cnt < PRTPIP_COUNT; cnt++)
  {
    sprintf(newloadname, "%spart%d.txt", pathname, cnt);
    prtpip[cnt] = PipList.load_one_pip(newloadname);

    if(Pip_List::INVALID!=prtpip[cnt].index)
    {
      strncpy(PipList[prtpip[cnt]].filename, newloadname, 0x0100);
      PipList[prtpip[cnt]].loaded = true;
    }
  }
};

//--------------------------------------------------------------------------------------------
void set_fan_light(int fanx, int fany, Dyna_Info & dinfo)
{
  // ZZ> This function is a little helper, lighting the selected fan
  //     with the chosen particle

  int   vertex, lastvertex;
  float level, light;

  Uint32 fan = fanx + fany*GMesh.fansWide();
  const JF::MPD_Fan     * pfan     = GMesh.getFan(fan);
  const JF::MPD_FanType * pfantype = GMesh.getFanType(fan);
  JF::MPD_Vertex * vrtlist = (JF::MPD_Vertex *)GMesh.getVertices();

  if(NULL==pfan || NULL == pfantype)  return;

  vec3_t normal;
  GMesh.simple_normal(fan, normal);

  vertex = pfan->firstVertex;
  lastvertex = vertex + pfantype->numVertices;
  for ( /* nothing */; vertex < lastvertex; vertex++)
  {
    // Do light particles
    light = vrtlist[vertex].light;

    vec3_t diff = vec3_t( dinfo.pos.x - vrtlist[vertex].x,
                          dinfo.pos.y - vrtlist[vertex].y,
                          dinfo.pos.z - vrtlist[vertex].z);

    float normal_factor = dot_product(diff, normal);
    if(normal_factor>0)
    {
      float len2 = dist_squared(diff);
      normal_factor = normal_factor*normal_factor/len2;
      float falloff2 = (dinfo.falloff * dinfo.falloff)/9; //scaling by 9 makes factor = 0.1 at the dynalist range
      float factor = normal_factor*falloff2 / (falloff2 + len2);
      light += (0xFF*dinfo.level) * factor;

      if (light > 0xFF) light = 0xFF;
      if (light < 0) light = 0;

      vrtlist[vertex].light = light;
    };
  };
}
