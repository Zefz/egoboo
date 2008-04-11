/* Egoboo - graphic.c
* All sorts of stuff related to drawing the game, and all sorts of other stuff
* (such as data loading) that really should not be in here.
*/

/*
This file is part of Egoboo.

Egoboo is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Egoboo is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "mathstuff.h"
#include "Log.h"
#include "Ui.h"
#include "Client.h"
#include "Server.h"

#include <assert.h>
#include <stdarg.h>

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

#ifdef __unix__
#include <unistd.h>
#endif

// Defined in egoboo.h
SDL_Surface *displaySurface = NULL;
bool_t gTextureOn = bfalse;

int doIngameMenu(float deltaTime);
void render_particles();

FOG_INFO GFog = {bfalse, 0.0f, 100, 100, 255, 255, 255, btrue};


//--------------------------------------------------------------------------------------------
//void EnableTexturing()
//{
//  //if ( !gTextureOn )
//  //{
//  //  glEnable( GL_TEXTURE_2D );
//  //  gTextureOn = btrue;
//  //}
//}

//--------------------------------------------------------------------------------------------
//void DisableTexturing()
//{
//  //if ( gTextureOn )
//  //{
//  //    glDisable( GL_TEXTURE_2D );
//  //  gTextureOn = bfalse;
//  //}
//}



//--------------------------------------------------------------------------------------------
// This needs work
static GLint threeDmode_begin_level = 0;
void Begin3DMode()
{
  assert(0 == threeDmode_begin_level);

  ATTRIB_GUARD_OPEN(threeDmode_begin_level);
  ATTRIB_PUSH("Begin3DMode", GL_TRANSFORM_BIT | GL_CURRENT_BIT);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadMatrixf(mProjection.v);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadMatrixf(mView.v);
  glMultMatrixf(mWorld.v);

  glColor4f(1, 1, 1, 1);
}

//--------------------------------------------------------------------------------------------
void End3DMode()
{
  GLint threeDmode_end_level;

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  ATTRIB_POP("End3DMode");

  ATTRIB_GUARD_CLOSE(threeDmode_begin_level, threeDmode_end_level);
  threeDmode_begin_level = 0;
}

//--------------------------------------------------------------------------------------------
static GLint twoD_begin_level = 0;

void Begin2DMode(void)
{
  assert(0 == twoD_begin_level);

  ATTRIB_GUARD_OPEN(twoD_begin_level);
  ATTRIB_PUSH("Begin2DMode", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_TRANSFORM_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();         // Reset The Projection Matrix
  glOrtho(0, CData.scrx, 0, CData.scry, 1, -1);     // Set up an orthogonal projection

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glDisable(GL_DEPTH_TEST);

  glColor4f(1, 1, 1, 1);
};

//--------------------------------------------------------------------------------------------
void End2DMode(void)
{
  GLint twoD_end_level;

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  ATTRIB_POP("End2DMode");

  ATTRIB_GUARD_CLOSE(twoD_begin_level, twoD_end_level);
  twoD_begin_level = 0;
}

//--------------------------------------------------------------------------------------------
static GLint text_begin_level = 0;
void BeginText(void)
{
  assert(0 == text_begin_level);

  ATTRIB_GUARD_OPEN(text_begin_level);
  ATTRIB_PUSH("BeginText", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT);

  GLTexture_Bind(&TxFont, CData.texturefilter);

  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  glColor4f(1, 1, 1, 1);
}

//--------------------------------------------------------------------------------------------
void EndText()
{
  GLint text_end_level;

  ATTRIB_POP("EndText");

  ATTRIB_GUARD_CLOSE(text_begin_level, text_end_level);
  text_begin_level = 0;
}


//---------------------------------------------------------------------------------------------
int get_level(Uint8 x, Uint8 y, Uint32 fan, Uint8 waterwalk)
{
  // ZZ> This function returns the height of a point within a mesh fan, precise
  //     If waterwalk is nonzero and the fan is watery, then the level returned is the
  //     level of the water.
  int z0, z1, z2, z3;         // Height of each fan corner
  int zleft, zright, zdone;   // Weighted height of each side

  x = x & 127;
  y = y & 127;
  z0 = Mesh.vrtz[Mesh.fanlist[fan].vrtstart + 0];
  z1 = Mesh.vrtz[Mesh.fanlist[fan].vrtstart + 1];
  z2 = Mesh.vrtz[Mesh.fanlist[fan].vrtstart + 2];
  z3 = Mesh.vrtz[Mesh.fanlist[fan].vrtstart + 3];

  zleft = (z0 * (128 - y) + z3 * y) >> 7;
  zright = (z1 * (128 - y) + z2 * y) >> 7;
  zdone = (zleft * (128 - x) + zright * x) >> 7;
  if (waterwalk)
  {
    if (watersurfacelevel > zdone && (Mesh.fanlist[fan].fx&MESHFXWATER) && wateriswater)
    {
      return watersurfacelevel;
    }
  }
  return zdone;
}

//---------------------------------------------------------------------------------------------
void initialize_texture_free_list()
{
  int cnt;

  for (cnt = 0; cnt < MAXTEXTURE; cnt++)
  {
    txFree[cnt] = (MAXTEXTURE-1) - cnt;
  };

  tx_index = MAXTEXTURE-1;
};

//---------------------------------------------------------------------------------------------
Uint32 get_free_texture()
{
  if(tx_index<0) return MAXTEXTURE;

  return txFree[tx_index--];
};

//---------------------------------------------------------------------------------------------
void return_texture(Uint32 index)
{
  if(MAXTEXTURE != index)
  {
    GLTexture_Release(&txTexture[index]);
    txFree[++tx_index] = index;
  };

  assert(tx_index<MAXTEXTURE);
};

//---------------------------------------------------------------------------------------------
void release_all_textures()
{
  // ZZ> This function clears out all of the textures
  int cnt;

  for (cnt = 0; cnt < MAXTEXTURE; cnt++)
  {
    GLTexture_Release(&txTexture[cnt]);
  };

  initialize_texture_free_list();
}



//--------------------------------------------------------------------------------------------
void load_one_icon(char *szLoadName)
{
  // ZZ> This function is used to load an icon.  Most icons are loaded
  //     without this function though...

  if (INVALID_TEXTURE != GLTexture_Load(GL_TEXTURE_2D,  &TxIcon[globalnumicon], szLoadName, INVALID_KEY))
  {
    /* PORT
    DDSetColorKey(lpDDSIcon[globalnumicon], 0); */
  }
  globalnumicon++;

}

//---------------------------------------------------------------------------------------------
void prime_titleimage(MOD_INFO * mi_ary, size_t mi_count)
{
  // ZZ> This function sets the title image pointers to NULL
  int cnt;

  for (cnt = 0; cnt < MAXMODULE; cnt++)
  {
    GLTexture_Release(&TxTitleImage[cnt]);
  };

  for (cnt = 0; cnt < mi_count; cnt++)
  {
    mi_ary[cnt].texture_idx = MAXMODULE;
  };
}

//---------------------------------------------------------------------------------------------
void prime_icons()
{
  // ZZ> This function sets the icon pointers to NULL
  int cnt;

  for (cnt = 0; cnt < MAXTEXTURE + 1; cnt++)
  {
    //lpDDSIcon[cnt]=NULL;
    TxIcon[cnt].textureID = INVALID_TEXTURE;
    skintoicon[cnt] = 0;
  }
  iconrect.left = 0;
  iconrect.right = 32;
  iconrect.top = 0;
  iconrect.bottom = 32;
  globalnumicon = 0;


  nullicon = 0;
  GKeyb.icon = 0;
  GMous.icon = 0;
  GJoy[0].icon = 0;
  GJoy[1].icon = 0;
  bookicon = 0;


}

//---------------------------------------------------------------------------------------------
void release_all_icons()
{
  // ZZ> This function clears out all of the icons
  int cnt;

  for (cnt = 0; cnt < MAXTEXTURE + 1; cnt++)
    GLTexture_Release(&TxIcon[cnt]);

  prime_icons(); /* Do we need this? */
}

//---------------------------------------------------------------------------------------------
void release_all_titleimages()
{
  // ZZ> This function clears out all of the title images
  int cnt;

  for (cnt = 0; cnt < MAXMODULE; cnt++)
    GLTexture_Release(&TxTitleImage[cnt]);
}

//---------------------------------------------------------------------------------------------
void release_all_models()
{
  // ZZ> This function clears out all of the models
  int cnt;
  for (cnt = 0; cnt < MAXMODEL; cnt++)
  {
    CapList[cnt].classname[0] = 0;
    MadList[cnt].used = bfalse;
    MadList[cnt].name[0] = '*';
    MadList[cnt].name[1] = 'N';
    MadList[cnt].name[2] = 'O';
    MadList[cnt].name[3] = 'N';
    MadList[cnt].name[4] = 'E';
    MadList[cnt].name[5] = '*';
    MadList[cnt].name[6] = 0;
  }
  madloadframe = 0;
}


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

  GLTexture_Release(&TxMap);
}

//--------------------------------------------------------------------------------------------
static void write_debug_message(const char *format, va_list args)
{
  // ZZ> This function sticks a message in the display queue and sets its timer

  STRING buffer;
  int slot = get_free_message();

  // print the formatted messafe into the buffer
  vsnprintf(buffer, sizeof(buffer) - 1, format, args);

  // Copy the message
  strncpy(GMsg.list[slot].textdisplay, buffer, sizeof(GMsg.list[slot].textdisplay));
  GMsg.list[slot].time = MESSAGETIME;
}

//--------------------------------------------------------------------------------------------
void debug_message(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  write_debug_message(format, args);
  va_end(args);
};


//--------------------------------------------------------------------------------------------
void reset_end_text()
{
  // ZZ> This function resets the end-module text
  if (numpla > 1)
  {
    snprintf(endtext, sizeof(endtext), "Sadly, they were never heard from again...");
    endtextwrite = 42;  // Where to append further text
  }
  else
  {
    if (numpla == 0)
    {
      // No players???  RTS module???
      snprintf(endtext, sizeof(endtext), "The game has ended...");
      endtextwrite = 21;
    }
    else
    {
      // One player
      snprintf(endtext, sizeof(endtext), "Sadly, no trace was ever found...");
      endtextwrite = 33;  // Where to append further text
    }
  }
}

//--------------------------------------------------------------------------------------------
void append_end_text(int message, Uint16 character)
{
  // ZZ> This function appends a message to the end-module text
  int read, cnt;
  char *eread;
  STRING szTmp;
  char cTmp, lTmp;
  Uint16 target, owner;

  target = ChrList[character].aitarget;
  owner = ChrList[character].aiowner;
  if (message < GMsg.total)
  {
    // Copy the message
    read = GMsg.index[message];
    cnt = 0;
    cTmp = GMsg.text[read];  read++;
    while (cTmp != 0)
    {
      if (cTmp == '%')
      {
        // Escape sequence
        eread = szTmp;
        szTmp[0] = 0;
        cTmp = GMsg.text[read];  read++;
        if (cTmp == 'n') // Name
        {
          if (ChrList[character].nameknown)
            strncpy(szTmp, ChrList[character].name, sizeof(STRING));
          else
          {
            lTmp = CapList[ChrList[character].model].classname[0];
            if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
              snprintf(szTmp, sizeof(szTmp), "an %s", CapList[ChrList[character].model].classname);
            else
              snprintf(szTmp, sizeof(szTmp), "a %s", CapList[ChrList[character].model].classname);
          }
          if (cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
        }
        if (cTmp == 'c') // Class name
        {
          eread = CapList[ChrList[character].model].classname;
        }
        if (cTmp == 't') // Target name
        {
          if (ChrList[target].nameknown)
            strncpy(szTmp, ChrList[target].name, sizeof(STRING));
          else
          {
            lTmp = CapList[ChrList[target].model].classname[0];
            if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
              snprintf(szTmp, sizeof(szTmp), "an %s", CapList[ChrList[target].model].classname);
            else
              snprintf(szTmp, sizeof(szTmp), "a %s", CapList[ChrList[target].model].classname);
          }
          if (cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
        }
        if (cTmp == 'o') // Owner name
        {
          if (ChrList[owner].nameknown)
            strncpy(szTmp, ChrList[owner].name, sizeof(STRING));
          else
          {
            lTmp = CapList[ChrList[owner].model].classname[0];
            if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
              snprintf(szTmp, sizeof(szTmp), "an %s", CapList[ChrList[owner].model].classname);
            else
              snprintf(szTmp, sizeof(szTmp), "a %s", CapList[ChrList[owner].model].classname);
          }
          if (cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
        }
        if (cTmp == 's') // Target class name
        {
          eread = CapList[ChrList[target].model].classname;
        }
        if (cTmp >= '0' && cTmp <= '0' + (MAXSKIN - 1))  // Target's skin name
        {
          eread = CapList[ChrList[target].model].skinname[cTmp-'0'];
        }
        if (cTmp == 'd') // tmpdistance value
        {
          snprintf(szTmp, sizeof(szTmp), "%d", valuetmpdistance);
        }
        if (cTmp == 'x') // tmpx value
        {
          snprintf(szTmp, sizeof(szTmp), "%d", valuetmpx);
        }
        if (cTmp == 'y') // tmpy value
        {
          snprintf(szTmp, sizeof(szTmp), "%d", valuetmpy);
        }
        if (cTmp == 'D') // tmpdistance value
        {
          snprintf(szTmp, sizeof(szTmp), "%2d", valuetmpdistance);
        }
        if (cTmp == 'X') // tmpx value
        {
          snprintf(szTmp, sizeof(szTmp), "%2d", valuetmpx);
        }
        if (cTmp == 'Y') // tmpy value
        {
          snprintf(szTmp, sizeof(szTmp), "%2d", valuetmpy);
        }
        if (cTmp == 'a') // Character's ammo
        {
          if (ChrList[character].ammoknown)
            snprintf(szTmp, sizeof(szTmp), "%d", ChrList[character].ammo);
          else
            snprintf(szTmp, sizeof(szTmp), "?");
        }
        if (cTmp == 'k') // Kurse state
        {
          if (ChrList[character].iskursed)
            snprintf(szTmp, sizeof(szTmp), "kursed");
          else
            snprintf(szTmp, sizeof(szTmp), "unkursed");
        }
        if (cTmp == 'p') // Character's possessive
        {
          if (ChrList[character].gender == GENFEMALE)
          {
            snprintf(szTmp, sizeof(szTmp), "her");
          }
          else
          {
            if (ChrList[character].gender == GENMALE)
            {
              snprintf(szTmp, sizeof(szTmp), "his");
            }
            else
            {
              snprintf(szTmp, sizeof(szTmp), "its");
            }
          }
        }
        if (cTmp == 'm') // Character's gender
        {
          if (ChrList[character].gender == GENFEMALE)
          {
            snprintf(szTmp, sizeof(szTmp), "female ");
          }
          else
          {
            if (ChrList[character].gender == GENMALE)
            {
              snprintf(szTmp, sizeof(szTmp), "male ");
            }
            else
            {
              snprintf(szTmp, sizeof(szTmp), " ");
            }
          }
        }
        if (cTmp == 'g') // Target's possessive
        {
          if (ChrList[target].gender == GENFEMALE)
          {
            snprintf(szTmp, sizeof(szTmp), "her");
          }
          else
          {
            if (ChrList[target].gender == GENMALE)
            {
              snprintf(szTmp, sizeof(szTmp), "his");
            }
            else
            {
              snprintf(szTmp, sizeof(szTmp), "its");
            }
          }
        }
        // Copy the generated text
        cTmp = *eread;  eread++;
        while (cTmp != 0 && endtextwrite < MAXENDTEXT - 1)
        {
          endtext[endtextwrite] = cTmp;
          cTmp = *eread;  eread++;
          endtextwrite++;
        }
      }
      else
      {
        // Copy the letter
        if (endtextwrite < MAXENDTEXT - 1)
        {
          endtext[endtextwrite] = cTmp;
          endtextwrite++;
        }
      }
      cTmp = GMsg.text[read];  read++;
      cnt++;
    }
  }
  endtext[endtextwrite] = 0;
}

//--------------------------------------------------------------------------------------------
void make_textureoffset(void)
{
  // ZZ> This function sets up for moving textures
  int cnt;
  for (cnt = 0; cnt < 256; cnt++)
    textureoffset[cnt] = cnt * 1.0 / 256;
}

//--------------------------------------------------------------------------------------------
void create_szfpstext(int frames)
{
  // ZZ> This function fills in the number of frames in "000 Frames per Second"
  frames = frames & 511;
  szfpstext[0] = '0' + (frames / 100);
  szfpstext[1] = '0' + ((frames / 10) % 10);
  szfpstext[2] = '0' + (frames % 10);
}

//--------------------------------------------------------------------------------------------
void make_renderlist()
{
  // ZZ> This function figures out which mesh fans to draw
  int cnt, fan, fanx, fany;
  int row, run, numrow;
  int xlist[4], ylist[4];
  int leftnum, leftlist[4];
  int rightnum, rightlist[4];
  int fanrowstart[128], fanrowrun[128];
  int x, stepx, divx, basex;
  int from, to;


  // Clear old render lists
  cnt = 0;
  while (cnt < Mesh.numrenderlist_all)
  {
    fan = Mesh.renderlist[cnt].all;
    Mesh.fanlist[fan].inrenderlist = bfalse;
    cnt++;
  }
  Mesh.numrenderlist_all = 0;
  Mesh.numrenderlist_ref = 0;
  Mesh.numrenderlist_sha = 0;

  // Make sure it doesn't die ugly !!!BAD!!!

  // It works better this way...
  cornery[cornerlistlowtohighy[3]] += 256;

  // Make life simpler
  xlist[0] = cornerx[cornerlistlowtohighy[0]];
  xlist[1] = cornerx[cornerlistlowtohighy[1]];
  xlist[2] = cornerx[cornerlistlowtohighy[2]];
  xlist[3] = cornerx[cornerlistlowtohighy[3]];
  ylist[0] = cornery[cornerlistlowtohighy[0]];
  ylist[1] = cornery[cornerlistlowtohighy[1]];
  ylist[2] = cornery[cornerlistlowtohighy[2]];
  ylist[3] = cornery[cornerlistlowtohighy[3]];

  // Find the center line
  divx = ylist[3] - ylist[0]; if (divx < 1) return;
  stepx = xlist[3] - xlist[0];
  basex = xlist[0];


  // Find the points in each edge
  leftlist[0] = 0;  leftnum = 1;
  rightlist[0] = 0;  rightnum = 1;
  if (xlist[1] < (stepx*(ylist[1] - ylist[0]) / divx) + basex)
  {
    leftlist[leftnum] = 1;  leftnum++;
    cornerx[1] -= 512;
  }
  else
  {
    rightlist[rightnum] = 1;  rightnum++;
    cornerx[1] += 512;
  }
  if (xlist[2] < (stepx*(ylist[2] - ylist[0]) / divx) + basex)
  {
    leftlist[leftnum] = 2;  leftnum++;
    cornerx[2] -= 512;
  }
  else
  {
    rightlist[rightnum] = 2;  rightnum++;
    cornerx[2] += 512;
  }
  leftlist[leftnum] = 3;  leftnum++;
  rightlist[rightnum] = 3;  rightnum++;


  // Make the left edge ( rowstart )
  fany = ylist[0] >> 7;
  row = 0;
  cnt = 1;
  while (cnt < leftnum)
  {
    from = leftlist[cnt-1];  to = leftlist[cnt];
    x = xlist[from];
    divx = ylist[to] - ylist[from];
    stepx = 0;
    if (divx > 0)
    {
      stepx = ((xlist[to] - xlist[from]) << 7) / divx;
    }
    x -= 256;
    run = ylist[to] >> 7;
    while (fany < run)
    {
      if (fany >= 0 && fany < Mesh.sizey)
      {
        fanx = x >> 7;
        if (fanx < 0)  fanx = 0;
        if (fanx >= Mesh.sizex)  fanx = Mesh.sizex - 1;
        fanrowstart[row] = fanx;
        row++;
      }
      x += stepx;
      fany++;
    }
    cnt++;
  }
  numrow = row;


  // Make the right edge ( rowrun )
  fany = ylist[0] >> 7;
  row = 0;
  cnt = 1;
  while (cnt < rightnum)
  {
    from = rightlist[cnt-1];  to = rightlist[cnt];
    x = xlist[from];
    //x+=128;
    divx = ylist[to] - ylist[from];
    stepx = 0;
    if (divx > 0)
    {
      stepx = ((xlist[to] - xlist[from]) << 7) / divx;
    }
    run = ylist[to] >> 7;
    while (fany < run)
    {
      if (fany >= 0 && fany < Mesh.sizey)
      {
        fanx = x >> 7;
        if (fanx < 0)  fanx = 0;
        if (fanx >= Mesh.sizex - 1)  fanx = Mesh.sizex - 1;//-2
        fanrowrun[row] = ABS(fanx - fanrowstart[row]) + 1;
        row++;
      }
      x += stepx;
      fany++;
    }
    cnt++;
  }

  if (numrow != row)
  {
    log_error("ROW error (%i, %i)\n", numrow, row);
  }

  // Fill 'em up again
  fany = ylist[0] >> 7;
  if (fany < 0) fany = 0;
  if (fany >= Mesh.sizey) fany = Mesh.sizey - 1;
  row = 0;
  while (row < numrow)
  {
    cnt = Mesh.fanstart[fany] + fanrowstart[row];
    run = fanrowrun[row];
    fanx = 0;
    while (fanx < run)
    {
      if (Mesh.numrenderlist_all < MAXMESHRENDER)
      {
        // Put each tile in basic list
        Mesh.fanlist[cnt].inrenderlist = btrue;
        Mesh.renderlist[Mesh.numrenderlist_all].all = cnt;
        Mesh.numrenderlist_all++;
        // Put each tile in one other list, for shadows and relections
        if (Mesh.fanlist[cnt].fx&MESHFXSHA)
        {
          Mesh.renderlist[Mesh.numrenderlist_sha].sha = cnt;
          Mesh.numrenderlist_sha++;
        }
        else
        {
          Mesh.renderlist[Mesh.numrenderlist_ref].ref = cnt;
          Mesh.numrenderlist_ref++;
        }
      }
      cnt++;
      fanx++;
    }
    row++;
    fany++;
  }
}

//--------------------------------------------------------------------------------------------
void figure_out_what_to_draw(GAME_STATE * gs)
{
  // ZZ> This function determines the things that need to be drawn

  // Find the render area corners
  project_view();
  // Make the render list for the mesh
  make_renderlist();

  GCamera.turnleftrightone = (GCamera.turnleftright) / (TWO_PI);
  GCamera.turnleftrightshort = GCamera.turnleftrightone * 65536;

  // Request matrices needed for local machine
  make_dolist(gs);
  order_dolist();
}

//--------------------------------------------------------------------------------------------
void animate_tiles()
{
  // This function changes the animated tile frame
  if ((wldframe & GTile_Anim.updateand) == 0)
  {
    GTile_Anim.frameadd = (GTile_Anim.frameadd + 1) & GTile_Anim.frameand;
  }
}

enum TX_TEXTURES
{
  TX_PARTICLE = 0,
  TX_TILE0,
  TX_TILE1,
  TX_TILE2,
  TX_TILE3,
  TX_WATERTOP,
  TX_WATERLOW,
  TX_PHONG,
  TX_SPECIAL_COUNT
};

//--------------------------------------------------------------------------------------------
void load_basic_textures(char *modname)
{
  // ZZ> This function loads the standard textures for a module
  // BB> In each case, try to load one stored with the module first.

  // Particle sprites
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.particle_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_PARTICLE], CStringTmp1, TRANSCOLOR))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.particle_bitmap);
    if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_PARTICLE], CStringTmp1, TRANSCOLOR))
    {
      log_warning("!!!!Particle bitmap could not be found!!!! Missing File = \"%s\"/n", CStringTmp1);
    }
  };

  // Module background tiles
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.tile0_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_TILE0], CStringTmp1, TRANSCOLOR))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.tile0_bitmap);
    if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_TILE0], CStringTmp1, TRANSCOLOR))
    {
      log_warning("Tile 0 could not be found. Missing File = \"%s\"\n", CData.tile0_bitmap);
    }
  };

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.tile1_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,   &txTexture[TX_TILE1], CStringTmp1, TRANSCOLOR))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.tile1_bitmap);
    if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_TILE1], CStringTmp1, TRANSCOLOR))
    {
      log_warning("Tile 1 could not be found. Missing File = \"%s\"\n", CData.tile1_bitmap);
    }
  };

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.tile2_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_TILE2], CStringTmp1, TRANSCOLOR))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.tile2_bitmap);
    if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_TILE2], CStringTmp1, TRANSCOLOR))
    {
      log_warning("Tile 2 could not be found. Missing File = \"%s\"\n", CData.tile2_bitmap);
    }
  };

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.tile3_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_TILE3], CStringTmp1, TRANSCOLOR))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.tile3_bitmap);
    if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_TILE3], CStringTmp1, TRANSCOLOR))
    {
      log_warning("Tile 3 could not be found. Missing File = \"%s\"\n", CData.tile3_bitmap);
    }
  };


  // Water textures
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.watertop_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_WATERTOP], CStringTmp1, INVALID_KEY))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.watertop_bitmap);
    if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_WATERTOP], CStringTmp1, TRANSCOLOR))
    {
      log_warning("Water Layer 1 could not be found. Missing File = \"%s\"\n", CData.watertop_bitmap);
    }
  };

  // This is also used as far background
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.waterlow_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_WATERLOW], CStringTmp1, INVALID_KEY))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.waterlow_bitmap);
    if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_WATERLOW], CStringTmp1, TRANSCOLOR))
    {
      log_warning("Water Layer 0 could not be found. Missing File = \"%s\"\n", CData.waterlow_bitmap);
    }
  };


  // BB > this is handled differently now and is not needed
  // Texture 7 is the phong map
  //snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.phong_bitmap);
  //if(INVALID_TEXTURE==GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_PHONG], CStringTmp1, INVALID_KEY))
  //{
  //  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.phong_bitmap);
  //  GLTexture_Load(GL_TEXTURE_2D,  &txTexture[TX_PHONG], CStringTmp1, TRANSCOLOR );
  //  {
  //    log_warning("Phong Bitmap could not be found. Missing File = \"%s\"", CData.phong_bitmap);
  //  }
  //};


}

//--------------------------------------------------------------------------------------------
Uint16 action_number()
{
  // ZZ> This function returns the number of the action in cFrameName, or
  //     it returns NOACTION if it could not find a match
  int cnt;
  char first, second;


  first = cFrameName[0];
  second = cFrameName[1];
  for (cnt = 0; cnt < MAXACTION; cnt++)
  {
    if (first == cActionName[cnt][0] && second == cActionName[cnt][1])
      return cnt;
  }

  return NOACTION;
}

//--------------------------------------------------------------------------------------------
Uint16 action_frame()
{
  // ZZ> This function returns the frame number in the third and fourth characters
  //     of cFrameName
  int number;
  sscanf(&cFrameName[2], "%d", &number);
  return number;
}

//--------------------------------------------------------------------------------------------
Uint16 test_frame_name(char letter)
{
  // ZZ> This function returns btrue if the 4th, 5th, 6th, or 7th letters
  //     of the frame name matches the input argument
  if (cFrameName[4] == letter) return btrue;
  if (cFrameName[4] == 0) return bfalse;
  if (cFrameName[5] == letter) return btrue;
  if (cFrameName[5] == 0) return bfalse;
  if (cFrameName[6] == letter) return btrue;
  if (cFrameName[6] == 0) return bfalse;
  if (cFrameName[7] == letter) return btrue;
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void action_copy_correct(int object, Uint16 actiona, Uint16 actionb)
{
  // ZZ> This function makes sure both actions are valid if either of them
  //     are valid.  It will copy start and ends to mirror the valid action.
  if (MadList[object].actionvalid[actiona] == MadList[object].actionvalid[actionb])
  {
    // They are either both valid or both invalid, in either case we can't help
  }
  else
  {
    // Fix the invalid one
    if (MadList[object].actionvalid[actiona] == bfalse)
    {
      // Fix actiona
      MadList[object].actionvalid[actiona] = btrue;
      MadList[object].actionstart[actiona] = MadList[object].actionstart[actionb];
      MadList[object].actionend[actiona] = MadList[object].actionend[actionb];
    }
    else
    {
      // Fix actionb
      MadList[object].actionvalid[actionb] = btrue;
      MadList[object].actionstart[actionb] = MadList[object].actionstart[actiona];
      MadList[object].actionend[actionb] = MadList[object].actionend[actiona];
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_walk_frame(int object, int lip, int action)
{
  // ZZ> This helps make walking look right
  int frame = 0;
  int framesinaction = MadList[object].actionend[action] - MadList[object].actionstart[action];

  while (frame < 16)
  {
    int framealong = 0;
    if (framesinaction > 0)
    {
      framealong = ((frame * framesinaction / 16) + 2) % framesinaction;
    }
    MadList[object].frameliptowalkframe[lip][frame] = MadList[object].actionstart[action] + framealong;
    frame++;
  }
}

//--------------------------------------------------------------------------------------------
void get_framefx(int frame)
{
  // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
  //     Drop timings
  Uint16 fx = 0;

  if (test_frame_name('I'))
    fx = fx | MADFXINVICTUS;
  if (test_frame_name('L'))
  {
    if (test_frame_name('A'))
      fx = fx | MADFXACTLEFT;
    if (test_frame_name('G'))
      fx = fx | MADFXGRABLEFT;
    if (test_frame_name('D'))
      fx = fx | MADFXDROPLEFT;
    if (test_frame_name('C'))
      fx = fx | MADFXCHARLEFT;
  }
  if (test_frame_name('R'))
  {
    if (test_frame_name('A'))
      fx = fx | MADFXACTRIGHT;
    if (test_frame_name('G'))
      fx = fx | MADFXGRABRIGHT;
    if (test_frame_name('D'))
      fx = fx | MADFXDROPRIGHT;
    if (test_frame_name('C'))
      fx = fx | MADFXCHARRIGHT;
  }
  if (test_frame_name('S'))
    fx = fx | MADFXSTOP;
  if (test_frame_name('F'))
    fx = fx | MADFXFOOTFALL;
  if (test_frame_name('P'))
    fx = fx | MADFXPOOF;

  MadFrame[frame].framefx = fx;
}

//--------------------------------------------------------------------------------------------
void make_framelip(int object, int action)
{
  // ZZ> This helps make walking look right
  int frame, framesinaction;

  if (MadList[object].actionvalid[action])
  {
    framesinaction = MadList[object].actionend[action] - MadList[object].actionstart[action];
    frame = MadList[object].actionstart[action];
    while (frame < MadList[object].actionend[action])
    {
      MadFrame[frame].framelip = (frame - MadList[object].actionstart[action]) * 15 / framesinaction;
      MadFrame[frame].framelip = (MadFrame[frame].framelip) & 15;
      frame++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_actions(int object)
{
  // ZZ> This function creates the frame lists for each action based on the
  //     name of each md2 frame in the model
  int frame, framesinaction;
  int action, lastaction;


  // Clear out all actions and reset to invalid
  action = 0;
  while (action < MAXACTION)
  {
    MadList[object].actionvalid[action] = bfalse;
    action++;
  }


  // Set the primary dance action to be the first frame, just as a default
  MadList[object].actionvalid[ACTIONDA] = btrue;
  MadList[object].actionstart[ACTIONDA] = MadList[object].framestart;
  MadList[object].actionend[ACTIONDA] = MadList[object].framestart + 1;


  // Now go huntin' to see what each frame is, look for runs of same action
  //printf("DIAG: ripping md2\n");
  rip_md2_frame_name(0);
  //printf("DIAG: done ripping md2\n");
  lastaction = action_number();  framesinaction = 0;
  frame = 0;
  while (frame < MadList[object].frames)
  {
    rip_md2_frame_name(frame);
    action = action_number();
    if (lastaction == action)
    {
      framesinaction++;
    }
    else
    {
      // Write the old action
      if (lastaction < MAXACTION)
      {
        MadList[object].actionvalid[lastaction] = btrue;
        MadList[object].actionstart[lastaction] = MadList[object].framestart + frame - framesinaction;
        MadList[object].actionend[lastaction] = MadList[object].framestart + frame;
      }
      framesinaction = 1;
      lastaction = action;
    }
    get_framefx(MadList[object].framestart + frame);
    frame++;
  }
  // Write the old action
  if (lastaction < MAXACTION)
  {
    MadList[object].actionvalid[lastaction] = btrue;
    MadList[object].actionstart[lastaction] = MadList[object].framestart + frame - framesinaction;
    MadList[object].actionend[lastaction] = MadList[object].framestart + frame;
  }

  // Make sure actions are made valid if a similar one exists
  action_copy_correct(object, ACTIONDA, ACTIONDB);  // All dances should be safe
  action_copy_correct(object, ACTIONDB, ACTIONDC);
  action_copy_correct(object, ACTIONDC, ACTIONDD);
  action_copy_correct(object, ACTIONDB, ACTIONDC);
  action_copy_correct(object, ACTIONDA, ACTIONDB);
  action_copy_correct(object, ACTIONUA, ACTIONUB);
  action_copy_correct(object, ACTIONUB, ACTIONUC);
  action_copy_correct(object, ACTIONUC, ACTIONUD);
  action_copy_correct(object, ACTIONTA, ACTIONTB);
  action_copy_correct(object, ACTIONTC, ACTIONTD);
  action_copy_correct(object, ACTIONCA, ACTIONCB);
  action_copy_correct(object, ACTIONCC, ACTIONCD);
  action_copy_correct(object, ACTIONSA, ACTIONSB);
  action_copy_correct(object, ACTIONSC, ACTIONSD);
  action_copy_correct(object, ACTIONBA, ACTIONBB);
  action_copy_correct(object, ACTIONBC, ACTIONBD);
  action_copy_correct(object, ACTIONLA, ACTIONLB);
  action_copy_correct(object, ACTIONLC, ACTIONLD);
  action_copy_correct(object, ACTIONXA, ACTIONXB);
  action_copy_correct(object, ACTIONXC, ACTIONXD);
  action_copy_correct(object, ACTIONFA, ACTIONFB);
  action_copy_correct(object, ACTIONFC, ACTIONFD);
  action_copy_correct(object, ACTIONPA, ACTIONPB);
  action_copy_correct(object, ACTIONPC, ACTIONPD);
  action_copy_correct(object, ACTIONZA, ACTIONZB);
  action_copy_correct(object, ACTIONZC, ACTIONZD);
  action_copy_correct(object, ACTIONWA, ACTIONWB);
  action_copy_correct(object, ACTIONWB, ACTIONWC);
  action_copy_correct(object, ACTIONWC, ACTIONWD);
  action_copy_correct(object, ACTIONDA, ACTIONWD);  // All walks should be safe
  action_copy_correct(object, ACTIONWC, ACTIONWD);
  action_copy_correct(object, ACTIONWB, ACTIONWC);
  action_copy_correct(object, ACTIONWA, ACTIONWB);
  action_copy_correct(object, ACTIONJA, ACTIONJB);
  action_copy_correct(object, ACTIONJB, ACTIONJC);
  action_copy_correct(object, ACTIONDA, ACTIONJC);    // All jumps should be safe
  action_copy_correct(object, ACTIONJB, ACTIONJC);
  action_copy_correct(object, ACTIONJA, ACTIONJB);
  action_copy_correct(object, ACTIONHA, ACTIONHB);
  action_copy_correct(object, ACTIONHB, ACTIONHC);
  action_copy_correct(object, ACTIONHC, ACTIONHD);
  action_copy_correct(object, ACTIONHB, ACTIONHC);
  action_copy_correct(object, ACTIONHA, ACTIONHB);
  action_copy_correct(object, ACTIONKA, ACTIONKB);
  action_copy_correct(object, ACTIONKB, ACTIONKC);
  action_copy_correct(object, ACTIONKC, ACTIONKD);
  action_copy_correct(object, ACTIONKB, ACTIONKC);
  action_copy_correct(object, ACTIONKA, ACTIONKB);
  action_copy_correct(object, ACTIONMH, ACTIONMI);
  action_copy_correct(object, ACTIONDA, ACTIONMM);
  action_copy_correct(object, ACTIONMM, ACTIONMN);


  // Create table for doing transition from one type of walk to another...
  // Clear 'em all to start
  for (frame = 0; frame < MadList[object].frames; frame++)
    MadFrame[frame+MadList[object].framestart].framelip = 0;

  // Need to figure out how far into action each frame is
  make_framelip(object, ACTIONWA);
  make_framelip(object, ACTIONWB);
  make_framelip(object, ACTIONWC);
  // Now do the same, in reverse, for walking animations
  get_walk_frame(object, LIPDA, ACTIONDA);
  get_walk_frame(object, LIPWA, ACTIONWA);
  get_walk_frame(object, LIPWB, ACTIONWB);
  get_walk_frame(object, LIPWC, ACTIONWC);
}

//--------------------------------------------------------------------------------------------
void make_mad_equally_lit(int model)
{
  // ZZ> This function makes ultra low poly models look better
  int frame, cnt, vert;

  if (MadList[model].used)
  {
    frame = MadList[model].framestart;
    for (cnt = 0; cnt < MadList[model].frames; cnt++)
    {
      vert = 0;
      while (vert < MAXVERTICES)
      {
        MadFrame[frame].vrta[vert] = EQUALLIGHTINDEX;
        vert++;
      }
      frame++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void check_copy(char* loadname, int object)
{
  // ZZ> This function copies a model's actions
  FILE *fileread;
  int actiona, actionb;
  char szOne[16], szTwo[16];


  MadList[object].msg_start = 0;
  fileread = fopen(loadname, "r");
  if (fileread)
  {
    while (goto_colon_yesno(fileread))
    {
      fscanf(fileread, "%s%s", szOne, szTwo);
      actiona = what_action(szOne[0]);
      actionb = what_action(szTwo[0]);
      action_copy_correct(object, actiona, actionb);
      action_copy_correct(object, actiona + 1, actionb + 1);
      action_copy_correct(object, actiona + 2, actionb + 2);
      action_copy_correct(object, actiona + 3, actionb + 3);
    }
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
int load_one_object(int skin, char* tmploadname)
{
  // ZZ> This function loads one object and returns the number of skins
  int object;
  int numskins, numicon, skin_index;
  STRING newloadname, loc_loadpath, wavename;
  int cnt;

  //printf(" DIAG: entered load_one_object\n");
  // Load the object data file and get the object number
  snprintf(newloadname, sizeof(newloadname), "%s/%s", tmploadname, CData.data_file);
  object = load_one_character_profile(newloadname);


  //printf(" DIAG: making up model name\n");
  // Make up a name for the model...  IMPORT\TEMP0000.OBJ
  strncpy(MadList[object].name, tmploadname, sizeof(MadList[object].name));

  //printf(" DIAG: appending slash\n");
  // Append a slash to the tmploadname
  strncpy(loc_loadpath, tmploadname, sizeof(loc_loadpath));
  strncat(loc_loadpath, "/", sizeof(STRING));

  // Load the AI script for this object
  snprintf(newloadname, sizeof(newloadname), "%s%s", loc_loadpath, CData.script_file);
  if (load_ai_script(newloadname))
  {
    // Create a reference to the one we just loaded
    MadList[object].ai = iNumAis - 1;
  }


  //printf(" DIAG: load object model\n");
  // Load the object model
  make_newloadname(loc_loadpath, "tris.md2", newloadname);

#ifdef __unix__
  // unix is case sensitive, but sometimes this file is called tris.MD2
  if (access(newloadname, R_OK))
  {
    make_newloadname(loc_loadpath, "tris.MD2", newloadname);
    // still no luck !
    if (access(newloadname, R_OK))
    {
      fprintf(stderr, "ERROR: cannot open: %s\n", newloadname);
      SDL_Quit();
      exit(1);
    }
  }
#endif

  load_one_md2(newloadname, object);
  md2_Models[object] = md2_loadFromFile(newloadname);


  //printf(" DIAG: fixing lighting\n");
  // Fix lighting if need be
  if (CapList[object].uniformlit)
  {
    make_mad_equally_lit(object);
  }


  //printf(" DIAG: creating actions\n");
  // Create the actions table for this object
  get_actions(object);


  //printf(" DIAG: copy actions\n");
  // Copy entire actions to save frame space COPY.TXT
  snprintf(newloadname, sizeof(newloadname), "%s%s", loc_loadpath, CData.copy_file);
  check_copy(newloadname, object);


  //printf(" DIAG: loading messages\n");
  // Load the messages for this object
  make_newloadname(loc_loadpath, CData.message_file, newloadname);
  load_all_messages(newloadname, object);


  //printf(" DIAG: doing random naming\n");
  // Load the random naming table for this object
  make_newloadname(loc_loadpath, CData.naming_file, newloadname);
  read_naming(object, newloadname);


  //printf(" DIAG: loading particles\n");
  // Load the particles for this object
  for (cnt = 0; cnt < MAXPRTPIPPEROBJECT; cnt++)
  {
    snprintf(newloadname, sizeof(newloadname), "%spart%d.txt", loc_loadpath, cnt);
    load_one_particle(newloadname, object, cnt);
  }


  //printf(" DIAG: loading waves\n");
  // Load the waves for this object
  for (cnt = 0; cnt < MAXWAVE; cnt++)
  {
    snprintf(wavename, sizeof(wavename), "%ssound%d.wav", loc_loadpath, cnt);
    CapList[object].waveindex[cnt] = Mix_LoadWAV(wavename);
  }


  //printf(" DIAG: loading enchantments\n");
  // Load the enchantment for this object
  make_newloadname(loc_loadpath, CData.enchant_file, newloadname);
  load_one_enchant_type(newloadname, object);


  //printf(" DIAG: loading skins and icons (PORTED EXCEPT ALPHA)\n");
  // Load the skins and icons
  MadList[object].skinstart = skin;
  numskins = 0;
  numicon = 0;


  for (skin_index = 0; skin_index < MAXSKIN; skin_index++)
  {
    snprintf(newloadname, sizeof(newloadname), "%stris%d.bmp", loc_loadpath, skin_index);
    if (INVALID_TEXTURE != GLTexture_Load(GL_TEXTURE_2D,  &txTexture[skin+numskins], newloadname, TRANSCOLOR))
    {
      numskins++;
      snprintf(newloadname, sizeof(newloadname), "%sicon%d.bmp", loc_loadpath, skin_index);
      if (INVALID_TEXTURE != GLTexture_Load(GL_TEXTURE_2D,  &TxIcon[globalnumicon], newloadname, INVALID_KEY))
      {
        if (object == SPELLBOOK && bookicon == 0) 
          bookicon = globalnumicon;

        while (numicon < numskins)
        {
          skintoicon[skin+numicon] = globalnumicon;
          numicon++;
        }
        globalnumicon++;
      }
    }
  }

  MadList[object].skins = numskins;
  if (numskins == 0)
  {
    // If we didn't get a skin, set it to the water texture
    MadList[object].skinstart = 5;
    MadList[object].skins = 1;
  }


  //printf(" DIAG: leaving load_one_obj\n");
  return numskins;
}

//--------------------------------------------------------------------------------------------
void load_all_objects(GAME_STATE * gs, char *modname)
{
  // ZZ> This function loads a module's objects
  const char *filehandle;
  bool_t keeplooking;
  FILE* fileread;
  STRING newloadname, filename;
  int cnt;
  int skin;
  int importplayer;

  // Clear the import slots...
  //printf(" DIAG: Clearing import slots\n");
  for (cnt = 0; cnt < MAXMODEL; cnt++)
    CapList[cnt].importslot = 10000;

  // Load the import directory
  //printf(" DIAG: loading inport dir\n");
  importplayer = -1;
  skin = 8;  // Character skins start at 8...  Trust me
  if (gs->modstate.importvalid)
  {
    for (cnt = 0; cnt < MAXIMPORT; cnt++)
    {
      snprintf(filename, sizeof(filename), "%s/temp%04d.obj", CData.import_dir, cnt);
      // Make sure the object exists...
      snprintf(newloadname, sizeof(newloadname), "%s/%s", filename, CData.data_file);
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
        importobject = ((importplayer) * 9) + (cnt % 9);
        CapList[importobject].importslot = cnt;
        skin += load_one_object(skin, filename);
      }
    }
  }
  //printf(" DIAG: emptying directory\n");
  //empty_import_directory();  // Free up that disk space...

  // Search for .obj directories and load them
  //printf(" DIAG: Searching for .objs\n");
  importobject = -100;
  snprintf(newloadname, sizeof(newloadname), "%s%s/", modname, CData.objects_dir);
  filehandle = fs_findFirstFile(newloadname, "obj");

  keeplooking = 1;
  if (filehandle != NULL)
  {
    while (keeplooking)
    {
      //printf(" DIAG: keeplooking\n");
      snprintf(filename, sizeof(filename), "%s%s", newloadname, filehandle);
      fprintf(stdout, "loading %s\n", filehandle);
      skin += load_one_object(skin, filename);

      filehandle = fs_findNextFile();

      keeplooking = (filehandle != NULL);
    }
  }
  fs_findClose();
  //printf(" DIAG: Done Searching for .objs\n");
}

//--------------------------------------------------------------------------------------------
bool_t load_bars(char* szBitmap)
{
  // ZZ> This function loads the status bar bitmap
  int cnt;

  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D, &TxBars, szBitmap, 0))
  {
	  return bfalse;
  }
    

  // Make the blit rectangles
  for (cnt = 0; cnt < NUMBAR; cnt++)
  {
    tabrect[cnt].left = 0;
    tabrect[cnt].right = TABX;
    tabrect[cnt].top = cnt * BARY;
    tabrect[cnt].bottom = (cnt + 1) * BARY;
    barrect[cnt].left = TABX;
    barrect[cnt].right = BARX;  // This is reset whenever a bar is drawn
    barrect[cnt].top = tabrect[cnt].top;
    barrect[cnt].bottom = tabrect[cnt].bottom;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
void load_map(char* szModule)
{
  // ZZ> This function loads the map bitmap and the blip bitmap

  // Turn it all off
  mapon = bfalse;
  youarehereon = bfalse;
  numblip = 0;

  // Load the images
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", szModule, CData.gamedat_dir, CData.plan_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D, &TxMap, CStringTmp1, INVALID_KEY))
    log_warning("Cannot load map: %s\n", CStringTmp1);

  // Set up the rectangles
  maprect.left   = 0;
  maprect.right  = MAPSIZE;
  maprect.top    = 0;
  maprect.bottom = MAPSIZE;

}

//--------------------------------------------------------------------------------------------
bool_t load_font(char* szBitmap, char* szSpacing)
{
  // ZZ> This function loads the font bitmap and sets up the coordinates
  //     of each font on that bitmap...  Bitmap must have 16x6 fonts
  int cnt, i, y, xsize, ysize, xdiv, ydiv;
  int xstt, ystt;
  int xspacing, yspacing;
  Uint8 cTmp;
  FILE *fileread;


  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D, &TxFont, szBitmap, 0)) return bfalse;


  // Clear out the conversion table
  for (cnt = 0; cnt < 256; cnt++)
    asciitofont[cnt] = 0;


  // Get the size of the bitmap
  xsize = GLTexture_GetImageWidth(&TxFont);
  ysize = GLTexture_GetImageHeight(&TxFont);
  if (xsize == 0 || ysize == 0)
    log_warning("Bad font size! (basicdat/%s) - X size: %i , Y size: %i\n", szBitmap, xsize, ysize);


  // Figure out the general size of each font
  ydiv = ysize / NUMFONTY;
  xdiv = xsize / NUMFONTX;


  // Figure out where each font is and its spacing
  fileread = fopen(szSpacing, "r");
  if (fileread == NULL) return bfalse;

  globalname = szSpacing;
  cnt = 0;
  y = 0;

  xstt = 0;
  ystt = 0;

  // Uniform font height is at the top
  goto_colon(fileread);
  fscanf(fileread, "%d", &yspacing);
  fontoffset = CData.scry - yspacing;

  // Mark all as unused
  for (i = 0; i < 255; i++)
    asciitofont[i] = 255;

  for (i = 0; i < 96; i++)
  {
    goto_colon(fileread);
    fscanf(fileread, "%c%d", &cTmp, &xspacing);
    if (asciitofont[cTmp] == 255) asciitofont[cTmp] = cnt;
    if (xstt + xspacing + 1 > 255)
    {
      xstt = 0;
      ystt += yspacing;
    }
    fontrect[cnt].x = xstt;
    fontrect[cnt].w = xspacing;
    fontrect[cnt].y = ystt;
    fontrect[cnt].h = yspacing - 2;
    fontxspacing[cnt] = xspacing + 1;
    xstt += xspacing + 1;
    cnt++;
  }
  fclose(fileread);


  // Space between lines
  fontyspacing = (yspacing >> 1) + FONTADD;

  return btrue;
}

//--------------------------------------------------------------------------------------------
void make_water()
{
  // ZZ> This function sets up water movements
  int layer, frame, point, mode, cnt;
  float tmp_sin, tmp_cos, tmp;
  Uint8 spek;


  layer = 0;
  while (layer < numwaterlayer)
  {
    waterlayeru[layer] = 0;
    waterlayerv[layer] = 0;
    frame = 0;
    while (frame < MAXWATERFRAME)
    {
      // Do first mode
      mode = 0;
      for (point = 0; point < WATERPOINTS; point++)
      {
        tmp_sin = sin((frame * TWO_PI / MAXWATERFRAME) + (PI * point / WATERPOINTS) + (PI_OVER_TWO * layer / MAXWATERLAYER));
        tmp_cos = cos((frame * TWO_PI / MAXWATERFRAME) + (PI * point / WATERPOINTS) + (PI_OVER_TWO * layer / MAXWATERLAYER));
        waterlayerzadd[layer][frame][mode][point]  = tmp_sin * waterlayeramp[layer];
      }

      // Now mirror and copy data to other three modes
      mode++;
      waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][1];
      //waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][1];
      waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][0];
      //waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][0];
      waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][3];
      //waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][3];
      waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][2];
      //waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][2];
      mode++;

      waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][3];
      //waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][3];
      waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][2];
      //waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][2];
      waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][1];
      //waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][1];
      waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][0];
      //waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][0];
      mode++;

      waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][2];
      //waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][2];
      waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][3];
      //waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][3];
      waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][0];
      //waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][0];
      waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][1];
      //waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][1];
      frame++;
    }
    layer++;
  }


  // Calculate specular highlights
  spek = 0;
  for (cnt = 0; cnt < 256; cnt++)
  {
    tmp = (float)cnt / 255.0f;
    spek = 255 * tmp * tmp;

    waterspek[cnt] = spek;

    // [claforte] Probably need to replace this with a
    //            glColor4f(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
  }
}

//--------------------------------------------------------------------------------------------
void read_wawalite(char *modname, Uint32 * seed)
{
  // ZZ> This function sets up water and lighting for the module
  FILE* fileread;
  float fTmp;
  char cTmp;
  int iTmp;

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.wawalite_file);
  fileread = fopen(CStringTmp1, "r");
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
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  numwaterlayer = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterspekstart = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterspeklevel = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterdouselevel = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  watersurfacelevel = iTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if (cTmp == 'T' || cTmp == 't')  waterlight = btrue;
    else waterlight = bfalse;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    wateriswater = bfalse;
    if (cTmp == 'T' || cTmp == 't')  wateriswater = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if ((cTmp == 'T' || cTmp == 't') && CData.overlayvalid)  CData.overlayon = btrue;
    else CData.overlayon = bfalse;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if ((cTmp == 'T' || cTmp == 't') && CData.backgroundvalid)  clearson = bfalse;
    else clearson = btrue;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayerdistx[0] = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayerdisty[0] = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayerdistx[1] = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayerdisty[1] = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  foregroundrepeat = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  backgroundrepeat = iTmp;


    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayerz[0] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayeralpha[0] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayerframeadd[0] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlightlevel[0] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlightadd[0] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayeramp[0] = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayeruadd[0] = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayervadd[0] = fTmp;

    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayerz[1] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayeralpha[1] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlayerframeadd[1] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlightlevel[1] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  waterlightadd[1] = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayeramp[1] = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayeruadd[1] = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterlayervadd[1] = fTmp;

    waterlayeru[0] = 0;
    waterlayerv[0] = 0;
    waterlayeru[1] = 0;
    waterlayerv[1] = 0;
    waterlayerframe[0] = ego_rand(&seed) & WATERFRAMEAND;
    waterlayerframe[1] = ego_rand(&seed) & WATERFRAMEAND;
    // Read light data second
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  lightspekx = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  lightspeky = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  lightspekz = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  lightambi  = fTmp;

    lightspek = sqrt(lightspekx * lightspekx + lightspeky * lightspeky + lightspekz * lightspekz);
    if (0 != lightspek)
    {
      lightspekx /= lightspek;
      lightspeky /= lightspek;
      lightspekz /= lightspek;
      lightspek *= lightambi;
    }

    // Read tile data third
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  hillslide = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  slippyfriction = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  airfriction = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  waterfriction = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  noslipfriction = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  gravity = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  GTile_Anim.updateand = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  GTile_Anim.frameand = iTmp;
    GTile_Anim.frameand_big = (iTmp << 1) + 1;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  GTile_Dam.amount = iTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if (cTmp == 'S' || cTmp == 's')  GTile_Dam.type = DAMAGESLASH;
    if (cTmp == 'C' || cTmp == 'c')  GTile_Dam.type = DAMAGECRUSH;
    if (cTmp == 'P' || cTmp == 'p')  GTile_Dam.type = DAMAGEPOKE;
    if (cTmp == 'H' || cTmp == 'h')  GTile_Dam.type = DAMAGEHOLY;
    if (cTmp == 'E' || cTmp == 'e')  GTile_Dam.type = DAMAGEEVIL;
    if (cTmp == 'F' || cTmp == 'f')  GTile_Dam.type = DAMAGEFIRE;
    if (cTmp == 'I' || cTmp == 'i')  GTile_Dam.type = DAMAGEICE;
    if (cTmp == 'Z' || cTmp == 'z')  GTile_Dam.type = DAMAGEZAP;
    // Read weather data fourth
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    GWeather.overwater = bfalse;
    if (cTmp == 'T' || cTmp == 't')  GWeather.overwater = btrue;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  GWeather.timereset = iTmp;
    GWeather.time = GWeather.timereset;
    GWeather.player = 0;
    // Read extra data
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    Mesh.exploremode = bfalse;
    if (cTmp == 'T' || cTmp == 't')  Mesh.exploremode = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    usefaredge = bfalse;
    if (cTmp == 'T' || cTmp == 't')  usefaredge = btrue;
    GCamera.swing = 0;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  GCamera.swingrate = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  GCamera.swingamp = fTmp;


    // Read unnecessary data...  Only read if it exists...
    GFog.on = bfalse;
    GFog.affectswater = btrue;
    GFog.top = 100;
    GFog.bottom = 0;
    GFog.distance = 100;
    GFog.red = 255;
    GFog.grn = 255;
    GFog.blu = 255;
    GTile_Dam.parttype = -1;
    GTile_Dam.partand = 255;
    GTile_Dam.sound = -1;
    GTile_Dam.soundtime = TILESOUNDTIME;
    GTile_Dam.mindistance = 9999;
    if (goto_colon_yesno(fileread))
    {
      GFog.on = CData.fogallowed;
      fscanf(fileread, "%f", &fTmp);  GFog.top = fTmp;
      goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  GFog.bottom = fTmp;
      goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  GFog.red = fTmp * 255;
      goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  GFog.grn = fTmp * 255;
      goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  GFog.blu = fTmp * 255;
      goto_colon(fileread);  cTmp = get_first_letter(fileread);
      if (cTmp == 'F' || cTmp == 'f')  GFog.affectswater = bfalse;
      GFog.distance = (GFog.top - GFog.bottom);
      if (GFog.distance < 1.0)  GFog.on = bfalse;


      // Read extra stuff for damage tile particles...
      if (goto_colon_yesno(fileread))
      {
        fscanf(fileread, "%d", &iTmp);  GTile_Dam.parttype = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
        GTile_Dam.partand = iTmp;
        goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
        GTile_Dam.sound = iTmp;
      }
    }

    // Allow slow machines to ignore the fancy stuff
    if (CData.twolayerwateron == bfalse && numwaterlayer > 1)
    {
      numwaterlayer = 1;
      iTmp = waterlayeralpha[0];
      iTmp = ((waterlayeralpha[1] * iTmp) >> 8) + iTmp;
      if (iTmp > 255) iTmp = 255;
      waterlayeralpha[0] = iTmp;
    }


    fclose(fileread);
    // Do it
    //printf("entering light stuff\n");
    make_spektable(lightspekx, lightspeky, lightspekz);
    make_lighttospek();
    make_water();
  }
  else

  {
    log_error("Could not read (%s/%s/%s) could not be read!\n", modname, CData.gamedat_dir, CData.wawalite_file);
  }
}

//--------------------------------------------------------------------------------------------
void render_background(Uint16 texture)
{
  // ZZ> This function draws the large background
  GLVERTEX vtlist[4];
  float size;
  float sinsize, cossize;
  float x, y, z, u, v;
  int i;

  float loc_backgroundrepeat;


  // Flat shade this?
  if (CData.shading) glShadeModel(GL_FLAT);

  // Figure out the screen coordinates of its corners
  x = CData.scrx << 6;
  y = CData.scry << 6;
  z = .99999;
  u = waterlayeru[1];
  v = waterlayerv[1];
  size = x + y + 1;
  sinsize = turntosin[(3*2047) & TRIGTABLE_MASK] * size;  // why 3/8 of a turn???
  cossize = turntosin[(3*2047 + 4096) & TRIGTABLE_MASK] * size;  // why 3/8 of a turn???
  loc_backgroundrepeat = backgroundrepeat * MIN(x / CData.scrx, y / CData.scrx);


  vtlist[0].x = x + cossize;
  vtlist[0].y = y - sinsize;
  vtlist[0].z = z;
  vtlist[0].s = 0 + u;
  vtlist[0].t = 0 + v;

  vtlist[1].x = x + sinsize;
  vtlist[1].y = y + cossize;
  vtlist[1].z = z;
  vtlist[1].s = loc_backgroundrepeat + u;
  vtlist[1].t = 0 + v;

  vtlist[2].x = x - cossize;
  vtlist[2].y = y + sinsize;
  vtlist[2].z = z;
  vtlist[2].s = loc_backgroundrepeat + u;
  vtlist[2].t = loc_backgroundrepeat + v;

  vtlist[3].x = x - sinsize;
  vtlist[3].y = y - cossize;
  vtlist[3].z = z;
  vtlist[3].s = 0 + u;
  vtlist[3].t = loc_backgroundrepeat + v;

  //-------------------------------------------------
  ATTRIB_PUSH("render_background", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
  {
    glShadeModel(GL_FLAT); // Flat shade this
    glDepthMask(GL_FALSE);

    GLTexture_Bind(&txTexture[texture], CData.texturefilter);

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    for (i = 0; i < 4; i++)
    {
      glTexCoord2f(vtlist[i].s, vtlist[i].t);

      glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
    }
    glEnd();
  };
  ATTRIB_POP("render_background");
  //-------------------------------------------------
}



//--------------------------------------------------------------------------------------------
void render_foreground_overlay(Uint16 texture)
{
  GLVERTEX vtlist[4];
  float size;
  float sinsize, cossize;
  float x, y, z, u, v;
  int i;
  Uint16 rotate;
  float loc_foregroundrepeat;

  // Figure out the screen coordinates of its corners
  x = CData.scrx << 6;
  y = CData.scry << 6;
  z = 0;
  u = waterlayeru[1];
  v = waterlayerv[1];
  size = x + y + 1;
  rotate = 16384 + 8192 - GCamera.turnleftrightshort;
  rotate = rotate >> 2;
  sinsize = turntosin[rotate & TRIGTABLE_MASK] * size;
  cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK] * size;

  loc_foregroundrepeat = foregroundrepeat * MIN(x / CData.scrx, y / CData.scrx) / 4.0;


  vtlist[0].x = x + cossize;
  vtlist[0].y = y - sinsize;
  vtlist[0].z = z;
  vtlist[0].s = 0 + u;
  vtlist[0].t = 0 + v;

  vtlist[1].x = x + sinsize;
  vtlist[1].y = y + cossize;
  vtlist[1].z = z;
  vtlist[1].s = loc_foregroundrepeat + u;
  vtlist[1].t = v;

  vtlist[2].x = x - cossize;
  vtlist[2].y = y + sinsize;
  vtlist[2].z = z;
  vtlist[2].s = loc_foregroundrepeat + u;
  vtlist[2].t = loc_foregroundrepeat + v;

  vtlist[3].x = x - sinsize;
  vtlist[3].y = y - cossize;
  vtlist[3].z = z;
  vtlist[3].s = u;
  vtlist[3].t = loc_foregroundrepeat + v;

  //-------------------------------------------------
  ATTRIB_PUSH("render_forground_overlay", GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TEXTURE_BIT | GL_HINT_BIT | GL_CURRENT_BIT);
  {
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST); // make sure that the texture is as smooth as possible

    glShadeModel(GL_FLAT); // Flat shade this

    glDepthMask(GL_FALSE);  // do not write into the depth buffer
    glDepthFunc(GL_ALWAYS); // make it appear over the top of everything

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR); // make the texture a filter

    GLTexture_Bind(&txTexture[texture], CData.texturefilter);

    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
    glBegin(GL_TRIANGLE_FAN);
    for (i = 0; i < 4; i++)
    {
      glTexCoord2f(vtlist[i].s, vtlist[i].t);
      glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
    }
    glEnd();
  }
  ATTRIB_POP("render_forground_overlay");
  //-------------------------------------------------
}

//--------------------------------------------------------------------------------------------
void render_shadow(int character)
{
  // ZZ> This function draws a NIFTY shadow
  GLVERTEX v[4];

  float x, y;
  float level;
  float height, size_umbra, size_penumbra;
  float height_factor, ambient_factor, tile_factor;
  float alpha_umbra, alpha_penumbra, alpha_character, light_character;
  Sint8 hide;
  int i;

  hide = CapList[ChrList[character].model].hidestate;
  if (hide != NOHIDE && hide == ChrList[character].aistate) return;

  // Original points
  level = ChrList[character].level;
  level += SHADOWRAISE;
  height = (ChrList[character].matrix)_CNV(3, 2) - level;
  if (height < 0) height = 0;

  tile_factor = (0 == Mesh.fanlist[ChrList[character].onwhichfan].fx & MESHFXWATER) ? 1.0 : 0.5;

  height_factor   = MAX(MIN((5 * ChrList[character].bumpsize / height), 1), 0);
  ambient_factor  = (float)ChrList[character].lightspek / (float)(ChrList[character].lightambi + ChrList[character].lightspek);
  ambient_factor  = 0.5f * (ambient_factor + lightspek / (lightambi + lightspek));
  alpha_character = ChrList[character].alpha / 255.0f;
  if (ChrList[character].light == 255)
  {
    light_character = 1.0f;
  }
  else
  {
    light_character = (float)ChrList[character].lightspek / (float)ChrList[character].light;
    light_character =  MIN(1, MAX(0, light_character));
  };


  size_umbra    = (ChrList[character].bumpsize - height / 30.0);
  size_penumbra = (ChrList[character].bumpsize + height / 30.0);

  alpha_umbra    = alpha_character * height_factor * ambient_factor * light_character * tile_factor;
  alpha_penumbra = alpha_character * height_factor * ambient_factor * light_character * tile_factor;

  if ((int)(alpha_umbra*255) == 0 && (int)(alpha_umbra*255) == 0) return;

  x = (ChrList[character].matrix)_CNV(3, 0);
  y = (ChrList[character].matrix)_CNV(3, 1);

  //GOOD SHADOW
  v[0].s = CALCULATE_PRT_U0(238);
  v[0].t = CALCULATE_PRT_V0(238);

  v[1].s = CALCULATE_PRT_U1(255);
  v[1].t = CALCULATE_PRT_V0(238);

  v[2].s = CALCULATE_PRT_U1(255);
  v[2].t = CALCULATE_PRT_V1(255);

  v[3].s = CALCULATE_PRT_U0(238);
  v[3].t = CALCULATE_PRT_V1(255);

  if (size_penumbra > 0)
  {
    v[0].x = x + size_penumbra;
    v[0].y = y - size_penumbra;
    v[0].z = level;

    v[1].x = x + size_penumbra;
    v[1].y = y + size_penumbra;
    v[1].z = level;

    v[2].x = x - size_penumbra;
    v[2].y = y + size_penumbra;
    v[2].z = level;

    v[3].x = x - size_penumbra;
    v[3].y = y - size_penumbra;
    v[3].z = level;

    ATTRIB_PUSH("render_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

      glDepthMask(GL_FALSE);
      glDepthFunc(GL_LEQUAL);

      // Choose texture.
      GLTexture_Bind(&txTexture[TX_PARTICLE], CData.texturefilter);

      glBegin(GL_TRIANGLE_FAN);
      glColor4f(alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2f(v[i].s, v[i].t);
        glVertex3f(v[i].x, v[i].y, v[i].z);
      }
      glEnd();
    }
    ATTRIB_POP("render_shadow");
  };

  if (size_umbra > 0)
  {
    v[0].x = x + size_umbra;
    v[0].y = y - size_umbra;
    v[0].z = level + 0.1;

    v[1].x = x + size_umbra;
    v[1].y = y + size_umbra;
    v[1].z = level + 0.1;

    v[2].x = x - size_umbra;
    v[2].y = y + size_umbra;
    v[2].z = level + 0.1;

    v[3].x = x - size_umbra;
    v[3].y = y - size_umbra;
    v[3].z = level + 0.1;

    ATTRIB_PUSH("render_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT);
    {
      glDisable(GL_CULL_FACE);

      glEnable(GL_BLEND);
      glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

      glDepthMask(GL_FALSE);
      glDepthFunc(GL_LEQUAL);

      // Choose texture.
      GLTexture_Bind(&txTexture[TX_PARTICLE], CData.texturefilter);

      glBegin(GL_TRIANGLE_FAN);
      glColor4f(alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2f(v[i].s, v[i].t);
        glVertex3f(v[i].x, v[i].y, v[i].z);
      }
      glEnd();
    }
    ATTRIB_POP("render_shadow");
  };
};

//--------------------------------------------------------------------------------------------
void render_bad_shadow(int character)
{
  // ZZ> This function draws a sprite shadow
  GLVERTEX v[4];
  float size, x, y;
  Uint8 ambi;
  //DWORD light;
  float level; //, z;
  int height;
  Sint8 hide;
  Uint8 trans;
  int i;


  hide = CapList[ChrList[character].model].hidestate;
  if (hide == NOHIDE || hide != ChrList[character].aistate)
  {
    // Original points
    level = ChrList[character].level;
    level += SHADOWRAISE;
    height = (ChrList[character].matrix)_CNV(3, 2) - level;
    if (height > 255)  return;
    if (height < 0) height = 0;
    size = ChrList[character].shadowsize - ((height * ChrList[character].shadowsize) >> 8);
    if (size < 1) return;
    ambi = ChrList[character].lightspek >> 4;  // LUL >>3;
    trans = ((255 - height) >> 1) + 64;

    x = (ChrList[character].matrix)_CNV(3, 0);
    y = (ChrList[character].matrix)_CNV(3, 1);
    v[0].x = (float) x + size;
    v[0].y = (float) y - size;
    v[0].z = (float) level;

    v[1].x = (float) x + size;
    v[1].y = (float) y + size;
    v[1].z = (float) level;

    v[2].x = (float) x - size;
    v[2].y = (float) y + size;
    v[2].z = (float) level;

    v[3].x = (float) x - size;
    v[3].y = (float) y - size;
    v[3].z = (float) level;


    v[0].s = CALCULATE_PRT_U0(236);
    v[0].t = CALCULATE_PRT_V0(236);

    v[1].s = CALCULATE_PRT_U1(253);
    v[1].t = CALCULATE_PRT_V0(236);

    v[2].s = CALCULATE_PRT_U1(253);
    v[2].t = CALCULATE_PRT_V1(253);

    v[3].s = CALCULATE_PRT_U0(236);
    v[3].t = CALCULATE_PRT_V1(253);

    ATTRIB_PUSH("render_bad_shadow", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
    {

      glDisable(GL_CULL_FACE);

      //glEnable(GL_ALPHA_TEST);
      //glAlphaFunc(GL_GREATER, 0);

      glEnable(GL_BLEND);
      glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

      glDepthMask(GL_FALSE);
      glDepthFunc(GL_LEQUAL);

      // Choose texture.
      GLTexture_Bind(&txTexture[TX_PARTICLE], CData.texturefilter);

      glColor4f(ambi / 255.0, ambi / 255.0, ambi / 255.0, trans / 255.0);
      glBegin(GL_TRIANGLE_FAN);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2f(v[i].s, v[i].t);
        glVertex3f(v[i].x, v[i].y, v[i].z);
      }
      glEnd();
    }
    ATTRIB_POP("render_bad_shadow");
  }
}




//--------------------------------------------------------------------------------------------
void light_characters()
{
  // ZZ> This function figures out character lighting
  int cnt, tnc, x, y;
  Uint16 tl, tr, bl, br;
  Uint16 spek, ambi;

  cnt = 0;
  while (cnt < numdolist)
  {
    tnc = dolist[cnt];
    x = ChrList[tnc].xpos;
    y = ChrList[tnc].ypos;
    x = (x & 127) >> 5;  // From 0 to 3
    y = (y & 127) >> 5;  // From 0 to 3
    spek = 0;
    tl = Mesh.vrtl[Mesh.fanlist[ChrList[tnc].onwhichfan].vrtstart + 0];
    tr = Mesh.vrtl[Mesh.fanlist[ChrList[tnc].onwhichfan].vrtstart + 1];
    br = Mesh.vrtl[Mesh.fanlist[ChrList[tnc].onwhichfan].vrtstart + 2];
    bl = Mesh.vrtl[Mesh.fanlist[ChrList[tnc].onwhichfan].vrtstart + 3];

    ambi = MIN(MIN(tl, tr), MIN(bl, br));

    // Interpolate lighting level using tile corners
    switch (x)
    {
    case 0:
      spek += (tl - ambi) << 1;
      spek += (bl - ambi) << 1;
      break;

    case 1:
    case 2:
      spek += (tl - ambi);
      spek += (tr - ambi);
      spek += (bl - ambi);
      spek += (br - ambi);
      break;

    case 3:
      spek += (tr - ambi) << 1;
      spek += (br - ambi) << 1;
      break;

    }
    switch (y)
    {
    case 0:
      spek += (tl - ambi) << 1;
      spek += (tr - ambi) << 1;
      break;

    case 1:
    case 2:
      spek += (tl - ambi);
      spek += (tr - ambi);
      spek += (bl - ambi);
      spek += (br - ambi);
      break;

    case 3:
      spek += (bl - ambi) << 1;
      spek += (br - ambi) << 1;
      break;

    }
    spek = spek >> 3;
    ChrList[tnc].lightambi = ambi;
    ChrList[tnc].lightspek = spek;


    if (Mesh.exploremode == bfalse)
    {
      // Look up spek direction using corners again
      tl = ((tl & 0xf0) << 8) & 0xf000;
      tr = ((tr & 0xf0) << 4) & 0x0f00;
      br = ((br & 0xf0) << 0) & 0x00f0;
      bl = ((bl & 0xf0) >> 4) & 0x000f;
      tl = tl | tr | br | bl;
      ChrList[tnc].lightturnleftright = (lightdirectionlookup[tl] << 8);
    }
    else
    {
      ChrList[tnc].lightturnleftright = 0;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void light_particles()
{
  // ZZ> This function figures out particle lighting
  int cnt;
  int character;

  cnt = 0;
  while (cnt < MAXPRT)
  {
    if (PrtList[cnt].on)
    {
      character = PrtList[cnt].attachedtocharacter;
      if (character != MAXCHR)
      {
        PrtList[cnt].light = ChrList[character].lightspek;
      }
      else
      {
        PrtList[cnt].light = Mesh.vrtl[Mesh.fanlist[PrtList[cnt].onwhichfan].vrtstart];
      }
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void set_fan_light(int fanx, int fany, Uint16 particle)
{
  // ZZ> This function is a little helper, lighting the selected fan
  //     with the chosen particle
  float x, y, z;
  int fan, vertex, lastvertex;
  float level;
  float light;


  if (fanx >= 0 && fanx < Mesh.sizex && fany >= 0 && fany < Mesh.sizey)
  {
    fan = fanx + Mesh.fanstart[fany];
    vertex = Mesh.fanlist[fan].vrtstart;
    lastvertex = vertex + Mesh.tilelist[Mesh.fanlist[fan].type].commandnumvertices;
    while (vertex < lastvertex)
    {
      light = Mesh.vrta[vertex];
      x = PrtList[particle].xpos - Mesh.vrtx[vertex];
      y = PrtList[particle].ypos - Mesh.vrty[vertex];
      z = PrtList[particle].zpos - Mesh.vrtz[vertex];
      level = PrtList[particle].dyna_lightlevel;
      level *= 16 * PrtList[particle].dyna_lightfalloff / (16 * PrtList[particle].dyna_lightfalloff + x * x + y * y + z * z);
      level = MIN(255, level * 255);

      Mesh.vrtl[vertex] = 0.9 * Mesh.vrtl[vertex] + 0.1 * (light + level);
      if (Mesh.exploremode && level > light) Mesh.vrta[vertex] = 0.9 * Mesh.vrta[vertex] + 0.1 * level;

      vertex++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void do_dynalight()
{
  // ZZ> This function does GDyna.mic lighting of visible fans

  int cnt, lastvertex, vertex, fan, entry, fanx, fany, addx, addy;
  float x, y, z;
  float level;
  float light;


  // Do each floor tile
  if (Mesh.exploremode)
  {
    // Set base light level in explore mode...  Don't need to do every frame
    if ((allframe & 7) == 0)
    {
      cnt = 0;
      while (cnt < MAXPRT)
      {
        if (PrtList[cnt].on && PrtList[cnt].dyna_lighton)
        {
          fanx = PrtList[cnt].xpos;
          fany = PrtList[cnt].ypos;
          fanx = fanx >> 7;
          fany = fany >> 7;
          addy = -DYNAFANS;
          while (addy <= DYNAFANS)
          {
            addx = -DYNAFANS;
            while (addx <= DYNAFANS)
            {
              set_fan_light(fanx + addx, fany + addy, cnt);
              addx++;
            }
            addy++;
          }
        }
        cnt++;
      }
    }
  }
  else
  {
    if (CData.shading != GL_FLAT)
    {
      // Add to base light level in normal mode
      entry = 0;
      while (entry < Mesh.numrenderlist_all)
      {
        fan = Mesh.renderlist[entry].all;
        vertex = Mesh.fanlist[fan].vrtstart;
        lastvertex = vertex + Mesh.tilelist[Mesh.fanlist[fan].type].commandnumvertices;
        while (vertex < lastvertex)
        {
          // Do light particles
          light = Mesh.vrta[vertex];
          cnt = 0;
          while (cnt < numdynalight)
          {
            x = GDyna.lightlistx[cnt] - Mesh.vrtx[vertex];
            y = GDyna.lightlisty[cnt] - Mesh.vrty[vertex];
            z = GDyna.lightlistz[cnt] - Mesh.vrtz[vertex];
            level = GDyna.lightlevel[cnt];
            level *= 16 * GDyna.lightfalloff[cnt] / (16 * GDyna.lightfalloff[cnt] + x * x + y * y + z * z);
            light += level * 255;
            cnt++;
          }
          if (light > 255) light = 255;
          if (light < 0) light = 0;
          Mesh.vrtl[vertex] = 0.9 * Mesh.vrtl[vertex] + 0.1 * light;
          if (Mesh.exploremode && light > Mesh.vrta[vertex]) Mesh.vrta[vertex] = 0.9 * Mesh.vrta[vertex] + 0.1 * light;

          vertex++;
        }
        entry++;
      }
    }
    else
    {
      entry = 0;
      while (entry < Mesh.numrenderlist_all)
      {
        fan = Mesh.renderlist[entry].all;
        vertex = Mesh.fanlist[fan].vrtstart;
        lastvertex = vertex + Mesh.tilelist[Mesh.fanlist[fan].type].commandnumvertices;
        while (vertex < lastvertex)
        {
          // Do light particles
          Mesh.vrtl[vertex] = Mesh.vrta[vertex];

          vertex++;
        }
        entry++;
      }
    }
  }
};

//--------------------------------------------------------------------------------------------
void render_water()
{
  // ZZ> This function draws all of the water fans

  int cnt;

  // Bottom layer first
  if (clearson && numwaterlayer > 1)
  {
    cnt = 0;
    while (cnt < Mesh.numrenderlist_all)
    {
      if (Mesh.fanlist[Mesh.renderlist[cnt].all].fx&MESHFXWATER)
      {
        // !!!BAD!!! Water will get screwed up if Mesh.sizex is odd
        render_water_fan(Mesh.renderlist[cnt].all, 1, ((Mesh.renderlist[cnt].all >> watershift)&2) + (Mesh.renderlist[cnt].all&1));
      }
      cnt++;
    }
  }

  // Top layer second
  if (!CData.overlayon && numwaterlayer > 0)
  {
    cnt = 0;
    while (cnt < Mesh.numrenderlist_all)
    {
      if (Mesh.fanlist[Mesh.renderlist[cnt].all].fx&MESHFXWATER)
      {
        // !!!BAD!!! Water will get screwed up if Mesh.sizex is odd
        render_water_fan(Mesh.renderlist[cnt].all, 0, ((Mesh.renderlist[cnt].all >> watershift)&2) + (Mesh.renderlist[cnt].all&1));
      }
      cnt++;
    }
  }
}

void render_water_lit()
{
  // BB> This function draws the hilites for water tiles using global lighting

  int cnt;

  // Bottom layer first
  if (clearson && numwaterlayer > 1)
  {
    float ambi_level = (waterlightadd[1] + waterlightlevel[1]) / 255.0;
    float spek_level =  waterspeklevel / 255.0;
    float spekularity = MIN(40, spek_level / ambi_level) + 2;
    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if (waterlight)
    {
      // self-lit water provides its own light
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_ambient);
    }
    else
    {
      // non-self-lit water needs an external lightsource
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_none);
    }

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

    cnt = 0;
    while (cnt < Mesh.numrenderlist_all)
    {
      if (Mesh.fanlist[Mesh.renderlist[cnt].all].fx&MESHFXWATER)
      {
        // !!!BAD!!! Water will get screwed up if Mesh.sizex is odd
        render_water_fan_lit(Mesh.renderlist[cnt].all, 1, ((Mesh.renderlist[cnt].all >> watershift)&2) + (Mesh.renderlist[cnt].all&1));
      }
      cnt++;
    }
  }

  // Top layer second
  if (!CData.overlayon && numwaterlayer > 0)
  {
    float ambi_level = (waterlightadd[1] + waterlightlevel[1]) / 255.0;
    float spek_level =  waterspeklevel / 255.0;
    float spekularity = MIN(40, spek_level / ambi_level) + 2;
    GLfloat mat_none[]      = {0, 0, 0, 0};
    GLfloat mat_ambient[]   = { ambi_level, ambi_level, ambi_level, 1.0 };
    GLfloat mat_diffuse[]   = { spek_level, spek_level, spek_level, 1.0 };
    GLfloat mat_shininess[] = {spekularity};

    if (waterlight)
    {
      // self-lit water provides its own light
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_ambient);
    }
    else
    {
      // non-self-lit water needs an external lightsource
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_none);
    }

    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,  mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

    cnt = 0;
    while (cnt < Mesh.numrenderlist_all)
    {
      if (Mesh.fanlist[Mesh.renderlist[cnt].all].fx&MESHFXWATER)
      {
        // !!!BAD!!! Water will get screwed up if Mesh.sizex is odd
        render_water_fan_lit(Mesh.renderlist[cnt].all, 0, ((Mesh.renderlist[cnt].all >> watershift)&2) + (Mesh.renderlist[cnt].all&1));
      }
      cnt++;
    }
  }

}

//--------------------------------------------------------------------------------------------
void render_good_shadows()
{
  int cnt, tnc;

  // Good shadows for me
  ATTRIB_PUSH("render_good_shadows", GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT);
  {
    glDisable(GL_ALPHA_TEST);
    //glAlphaFunc(GL_GREATER, 0);

    glDisable(GL_CULL_FACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);

    for (cnt = 0; cnt < numdolist; cnt++)
    {
      tnc = dolist[cnt];
      //if(ChrList[tnc].attachedto == MAXCHR)
      //{
      if (ChrList[tnc].shadowsize != 0 || CapList[ChrList[tnc].model].forceshadow && (0 == Mesh.fanlist[ChrList[tnc].onwhichfan].fx&MESHFXDRAWREF))
        render_shadow(tnc);
      //}
    }
  }
  ATTRIB_POP("render_good_shadows");
}

//--------------------------------------------------------------------------------------------
void render_bad_shadows()
{
  int cnt, tnc;

  // Bad shadows
  ATTRIB_PUSH("render_bad_shadows", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  {
    glDepthMask(GL_FALSE);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (cnt = 0; cnt < numdolist; cnt++)
    {
      tnc = dolist[cnt];
      //if(ChrList[tnc].attachedto == MAXCHR)
      //{
      if (ChrList[tnc].shadowsize != 0 || CapList[ChrList[tnc].model].forceshadow && (0 == Mesh.fanlist[ChrList[tnc].onwhichfan].fx&MESHFXDRAWREF))
        render_bad_shadow(tnc);
      //}
    }
  }
  ATTRIB_POP("render_bad_shadows");
}




//--------------------------------------------------------------------------------------------
//void draw_scene_sadreflection()
//{
//  // ZZ> This function draws 3D objects
//  Uint16 cnt, tnc;
//  Uint8 trans;
//  rect_t rect;// = {0, 0, CData.scrx, CData.scry}; // Don't know why this isn't working on the Mac, it should
//
//  rect.left = 0;
//  rect.right = 0;
//  rect.top = CData.scrx;
//  rect.bottom = CData.scry;
//
//  // ZB> Clear the z-buffer
//  glClear(GL_DEPTH_BUFFER_BIT);
//
//  // Clear the image if need be
//  if (clearson)
//    glClear(GL_COLOR_BUFFER_BIT);
//  else
//  {
//    // Render the background
//    render_background(6);  // 6 is the texture for waterlow.bmp
//  }
//
//
//  // Render the reflective floors
//  glEnable(GL_CULL_FACE);
//  glFrontFace(GL_CW);
//  Mesh.lasttexture = 0;
//  for (cnt = 0; cnt < Mesh.numrenderlist_ref; cnt++)
//    render_fan(Mesh.renderlist[cnt].ref);
//
//  if(CData.refon)
//  {
//    // Render reflections of characters
//
//    glEnable(GL_CULL_FACE);
//    glFrontFace(GL_CCW);
//
//    glDisable(GL_DEPTH_TEST);
//    glDepthMask(GL_FALSE);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if((Mesh.fanlist[ChrList[tnc].onwhichfan].fx&MESHFXDRAWREF))
//      {
//        render_refmad(tnc, ChrList[tnc].alpha&ChrList[tnc].light);
//      }
//    }
//
//    // Render the reflected sprites
//    glFrontFace(GL_CW);
//    render_refprt();
//
//    glDisable(GL_BLEND);
//    glEnable(GL_DEPTH_TEST);
//    glDepthMask(GL_TRUE);
//  }
//
//  // Render the shadow floors
//  Mesh.lasttexture = 0;
//  for (cnt = 0; cnt < Mesh.numrenderlist_sha; cnt++)
//    render_fan(Mesh.renderlist[cnt].sha);
//
//  // Render the shadows
//  if(CData.shaon)
//  {
//    if(CData.shasprite)
//    {
//      // Bad shadows
//      glDepthMask(GL_FALSE);
//      glEnable(GL_BLEND);
//      glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if(ChrList[tnc].attachedto == MAXCHR)
//        {
//          if(((ChrList[tnc].light==255 && ChrList[tnc].alpha==255) || CapList[ChrList[tnc].model].forceshadow) && ChrList[tnc].shadowsize!=0)
//          {
//            render_bad_shadow(tnc);
//          }
//        }
//      }
//
//      glDisable(GL_BLEND);
//      glDepthMask(GL_TRUE);
//    }
//    else
//    {
//      // Good shadows for me
//      glDepthMask(GL_FALSE);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_COLOR, GL_ZERO);
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if(ChrList[tnc].attachedto == MAXCHR)
//        {
//          if(((ChrList[tnc].light==255 && ChrList[tnc].alpha==255) || CapList[ChrList[tnc].model].forceshadow) && ChrList[tnc].shadowsize!=0)
//          {
//            render_shadow(tnc);
//          }
//        }
//      }
//      glDisable(GL_BLEND);
//      glDepthMask(GL_TRUE);
//    }
//  }
//
//  glAlphaFunc(GL_GREATER, 0);
//  glEnable(GL_ALPHA_TEST);
//  glDisable(GL_CULL_FACE);
//
//  // Render the normal characters
//  for (cnt = 0; cnt < numdolist; cnt++)
//  {
//    tnc = dolist[cnt];
//    if(ChrList[tnc].alpha==255 && ChrList[tnc].light==255)
//      render_mad(tnc, 255);
//  }
//
//  // Render the sprites
//  glDepthMask(GL_FALSE);
//  glEnable(GL_BLEND);
//
//  // Now render the transparent characters
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//  for (cnt = 0; cnt < numdolist; cnt++)
//  {
//    tnc = dolist[cnt];
//    if(ChrList[tnc].alpha!=255 && ChrList[tnc].light==255)
//    {
//      trans = ChrList[tnc].alpha;
//      if(trans < SEEINVISIBLE && (ClientState.seeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//      render_mad(tnc, trans);
//    }
//  }
//
//  // Alpha water
//  if(!waterlight)  render_water();
//
//  // Then do the light characters
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//  for (cnt = 0; cnt < numdolist; cnt++)
//  {
//    tnc = dolist[cnt];
//    if(ChrList[tnc].light!=255)
//    {
//      trans = ChrList[tnc].light;
//      if(trans < SEEINVISIBLE && (ClientState.seeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//      render_mad(tnc, trans);
//    }
//
//    // Do phong highlights
//    if(CData.phongon && ChrList[tnc].alpha==255 && ChrList[tnc].light==255 && ChrList[tnc].enviro==bfalse && ChrList[tnc].sheen > 0)
//    {
//      Uint16 texturesave, envirosave;
//      envirosave = ChrList[tnc].enviro;
//      texturesave = ChrList[tnc].texture;
//      ChrList[tnc].enviro = btrue;
//      ChrList[tnc].texture = 7;  // The phong map texture...
//      render_mad(tnc, ChrList[tnc].sheen<<4);
//      ChrList[tnc].texture = texturesave;
//      ChrList[tnc].enviro = envirosave;
//    }
//  }
//
//  // Light water
//  if(waterlight)  render_water();
//
//  // Turn Z buffer back on, alphablend off
//  glDepthMask(GL_TRUE);
//  glDisable(GL_BLEND);
//  glEnable(GL_ALPHA_TEST);
//  render_prt();
//  glDisable(GL_ALPHA_TEST);
//
//  glDepthMask(GL_TRUE);
//  glDisable(GL_BLEND);
//
//  // Done rendering
//}

//--------------------------------------------------------------------------------------------
void render_character_reflections()
{
  int cnt, tnc;

  // Render reflections of characters
  ATTRIB_PUSH("render_character_reflections", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
  {
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    for (cnt = 0; cnt < numdolist; cnt++)
    {
      tnc = dolist[cnt];
      if ((Mesh.fanlist[ChrList[tnc].onwhichfan].fx & MESHFXDRAWREF))
        render_refmad(tnc, ChrList[tnc].alpha / 2);
    }
  }
  ATTRIB_POP("render_character_reflections");

};

//--------------------------------------------------------------------------------------------
void render_ref_fans()
{
  int cnt, tnc, fan, texture;

  ATTRIB_PUSH("render_ref_fans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
  {
    // depth buffer stuff
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // CData.shading stuff
    glShadeModel(CData.shading);

    // alpha stuff
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    // backface culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    for (texture = TX_TILE0; texture <= TX_TILE3; texture++)
    {
      Mesh.lasttexture = texture;
      GLTexture_Bind(&txTexture[texture], CData.texturefilter);
      for (tnc = 0; tnc < Mesh.numrenderlist_ref; tnc++)
      {
        fan = Mesh.renderlist[tnc].ref;
        if (0 == (Mesh.fanlist[fan].fx&MESHFXDRAWREF))
          render_fan(fan, texture);
      };
    }
  }
  ATTRIB_POP("render_ref_fans");
};


//--------------------------------------------------------------------------------------------
void render_sha_fans()
{
  int cnt, tnc, fan, texture;

  // Render the shadow floors
  ATTRIB_PUSH("render_sha_fans", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
  {
    // depth buffer stuff
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // CData.shading stuff
    glShadeModel(CData.shading);

    // alpha stuff
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    // backface culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    for (texture = TX_TILE0; texture <= TX_TILE3; texture++)
    {
      Mesh.lasttexture = texture;
      GLTexture_Bind(&txTexture[texture], CData.texturefilter);
      for (tnc = 0; tnc < Mesh.numrenderlist_sha; tnc++)
      {
        fan = Mesh.renderlist[tnc].sha;
        if (0 == (Mesh.fanlist[fan].fx&MESHFXDRAWREF))
          render_fan(fan, texture);
      };
    }
  }
  ATTRIB_POP("render_sha_fans");
};

//--------------------------------------------------------------------------------------------
void render_ref_fans_ref()
{
  int cnt, tnc, fan, texture;

  ATTRIB_PUSH("render_ref_fans_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
  {
    // depth buffer stuff
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // CData.shading stuff
    glShadeModel(CData.shading);

    // alpha stuff
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    // backface culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    for (texture = TX_TILE0; texture <= TX_TILE3; texture++)
    {
      Mesh.lasttexture = texture;
      GLTexture_Bind(&txTexture[texture], CData.texturefilter);
      for (tnc = 0; tnc < Mesh.numrenderlist_ref; tnc++)
      {
        fan = Mesh.renderlist[tnc].ref;
        if (0 != (Mesh.fanlist[fan].fx&MESHFXDRAWREF))
          render_fan(fan, texture);
      };
    }
  }
  ATTRIB_POP("render_ref_fans_ref");
};

//--------------------------------------------------------------------------------------------
void render_sha_fans_ref()
{
  int cnt, tnc, fan, texture;

  // Render the shadow floors
  ATTRIB_PUSH("render_sha_fans_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
  {
    // depth buffer stuff
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // CData.shading stuff
    glShadeModel(CData.shading);

    // alpha stuff
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    // backface culling
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    for (texture = TX_TILE0; texture <= TX_TILE3; texture++)
    {
      Mesh.lasttexture = texture;
      GLTexture_Bind(&txTexture[texture], CData.texturefilter);
      for (tnc = 0; tnc < Mesh.numrenderlist_sha; tnc++)
      {
        fan = Mesh.renderlist[tnc].sha;
        if (0 != (Mesh.fanlist[fan].fx&MESHFXDRAWREF))
          render_fan(fan, texture);
      };
    }

  }
  ATTRIB_POP("render_sha_fans_ref");
};

//--------------------------------------------------------------------------------------------
void render_solid_characters()
{
  int cnt, tnc;

  // Render the normal characters
  ATTRIB_PUSH("render_solid_characters", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
  {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    // must be here even for solid characters because of stuff like the spikemace
    // and a lot of other items that just use a simple texturemapped plane with
    // transparency for their shape.  Maybe it should be converted to blend?
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    for (cnt = 0; cnt < numdolist; cnt++)
    {
      tnc = dolist[cnt];
      if (ChrList[tnc].alpha==255 && ChrList[tnc].light==255)
        render_mad(tnc, 255);
    }
  }
  ATTRIB_POP("render_solid_characters");

};

//--------------------------------------------------------------------------------------------
void render_alpha_characters(GAME_STATE * gs)
{
  int cnt, tnc;
  Uint8 trans;

  // Now render the transparent characters
  ATTRIB_PUSH("render_alpha_characters", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
  {
    glDepthMask(GL_FALSE);

    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

    for (cnt = 0; cnt < numdolist; cnt++)
    {
      tnc = dolist[cnt];
      if (ChrList[tnc].alpha != 255)
      {
        trans = ChrList[tnc].alpha;

        if ((ChrList[tnc].alpha + ChrList[tnc].light) < SEEINVISIBLE &&  gs->cs->seeinvisible && ChrList[tnc].islocalplayer)
          trans = SEEINVISIBLE - ChrList[tnc].light;

        if (trans > 0)
        {
          render_mad(tnc, trans);
        };
      }
    }

  }
  ATTRIB_POP("render_alpha_characters");

};

//--------------------------------------------------------------------------------------------
// render the water hilights, etc. using global lighting
void render_water_highlights()
{
  ATTRIB_PUSH("render_water_highlights", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT);
  {
    GLfloat light_position[] = { 10000*lightspekx, 10000*lightspeky, 10000*lightspekz, 1.0 };
    GLfloat lmodel_ambient[] = { lightambi, lightambi, lightambi, 1.0 };
    GLfloat light_diffuse[]  = { lightspek, lightspek, lightspek, 1.0 };

    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    //glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    render_water_lit();
  }
  ATTRIB_POP("render_water_highlights");
};

//--------------------------------------------------------------------------------------------
void render_alpha_water()
{

  ATTRIB_PUSH("render_alpha_water", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_POLYGON_BIT);
  {
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    render_water();
  }
  ATTRIB_POP("render_alpha_water");
};

//--------------------------------------------------------------------------------------------
void render_light_water()
{
  ATTRIB_PUSH("render_light_water", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT);
  {
    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    render_water();
  }
  ATTRIB_POP("render_light_water");
};

//--------------------------------------------------------------------------------------------
void render_character_highlights()
{
  int cnt, tnc;

  ATTRIB_PUSH("render_character_highlights", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
  {
    GLfloat light_none[]     = {0, 0, 0, 0};
    GLfloat light_position[] = { 10000*lightspekx, 10000*lightspeky, 10000*lightspekz, 1.0 };
    GLfloat lmodel_ambient[] = { lightambi, lightambi, lightambi, 1.0 };
    GLfloat light_specular[] = { lightspek, lightspek, lightspek, 1.0 };

    glDisable(GL_CULL_FACE);

    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_LESS, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glLightfv(GL_LIGHT1, GL_AMBIENT,  light_none);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  light_none);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT1, GL_POSITION, light_position);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light_none);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    glEnable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glEnable(GL_LIGHT1);

    for (cnt = 0; cnt < numdolist; cnt++)
    {
      tnc = dolist[cnt];

      if (ChrList[tnc].sheen == 0 && ChrList[tnc].light == 255 && ChrList[tnc].alpha == 255) continue;

      render_mad_lit(tnc);
    }
  }
  ATTRIB_POP("render_character_highlights");
};

//--------------------------------------------------------------------------------------------
GLint inp_attrib_stack, out_attrib_stack;
void draw_scene_zreflection(GAME_STATE * gs)
{
  // ZZ> This function draws 3D objects
  // do all the rendering of reflections
  if (gs->cd->refon)
  {
    ATTRIB_GUARD_OPEN(inp_attrib_stack);
    render_character_reflections();
    ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);

    ATTRIB_GUARD_OPEN(inp_attrib_stack);
    render_particle_reflections();
    ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);
  }


  //---- render the non-reflective fans ----
  {
    ATTRIB_GUARD_OPEN(inp_attrib_stack);
    render_ref_fans();
    ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);

    ATTRIB_GUARD_OPEN(inp_attrib_stack);
    render_sha_fans();
    ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);
  }

  //---- render the reflective fans ----
  {
    ATTRIB_GUARD_OPEN(inp_attrib_stack);
    render_ref_fans_ref();
    ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);

    ATTRIB_GUARD_OPEN(inp_attrib_stack);
    render_sha_fans_ref();
    ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);
  }

  // Render the shadows
  if (gs->cd->shaon)
  {
    if (gs->cd->shasprite)
    {
      ATTRIB_GUARD_OPEN(inp_attrib_stack);
      render_bad_shadows();
      ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);
    }
    else
    {
      ATTRIB_GUARD_OPEN(inp_attrib_stack);
      render_good_shadows();
      ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);
    }
  }


  ATTRIB_GUARD_OPEN(inp_attrib_stack);
  render_solid_characters();
  ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);

  ////---- Render transparent objects ----
  {
    render_alpha_characters(gs);

    // And alpha water
    if (!waterlight)
    {
      ATTRIB_GUARD_OPEN(inp_attrib_stack);
      render_alpha_water();
      ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);
    };

    // Do self-lit water
    if (waterlight)
    {
      ATTRIB_GUARD_OPEN(inp_attrib_stack);
      render_light_water();
      ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);
    };

  };

  // do highlights
  ATTRIB_GUARD_OPEN(inp_attrib_stack);
  render_character_highlights();
  ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);

  ATTRIB_GUARD_OPEN(inp_attrib_stack);
  render_water_highlights();
  ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);

  // render the sprites
  ATTRIB_GUARD_OPEN(inp_attrib_stack);
  render_particles();
  ATTRIB_GUARD_CLOSE(inp_attrib_stack, out_attrib_stack);
};


//--------------------------------------------------------------------------------------------
//void draw_scene_zreflection()
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
//  glDisable(GL_DEPTH_TEST);
//  glDepthMask(GL_FALSE);
//  glDisable(GL_BLEND);
//
//  // Renfer ref
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  Mesh.lasttexture = 0;
//  for (cnt = 0; cnt < Mesh.numrenderlist_ref; cnt++)
//    render_fan(Mesh.renderlist[cnt].ref);
//
//  // Renfer sha
//  // BAD: DRAW SHADOW STUFF TOO
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  for (cnt = 0; cnt < Mesh.numrenderlist_sha; cnt++)
//    render_fan(Mesh.renderlist[cnt].sha);
//
//  glEnable(GL_DEPTH_TEST);
//  glDepthMask(GL_TRUE);
//  if(CData.refon)
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
//      if((Mesh.fanlist[ChrList[tnc].onwhichfan].fx&MESHFXDRAWREF))
//        render_refmad(tnc, ChrList[tnc].alpha&ChrList[tnc].light);
//    }
//
//    // [claforte] I think this is wrong... I think we should choose some other depth func.
//    glDepthFunc(GL_ALWAYS);
//
//    // Render the reflected sprites
//    render_particle_reflections();
//  }
//
//  // Render the shadow floors
//  Mesh.lasttexture = 0;
//
//  glEnable(GL_ALPHA_TEST);
//  glAlphaFunc(GL_GREATER, 0);
//  for (cnt = 0; cnt < Mesh.numrenderlist_sha; cnt++)
//    render_fan(Mesh.renderlist[cnt].sha);
//
//  // Render the shadows
//  if (CData.shaon)
//  {
//    if (CData.shasprite)
//    {
//      // Bad shadows
//      glDepthMask(GL_FALSE);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if(ChrList[tnc].attachedto == MAXCHR)
//        {
//          if(((ChrList[tnc].light==255 && ChrList[tnc].alpha==255) || CapList[ChrList[tnc].model].forceshadow) && ChrList[tnc].shadowsize!=0)
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
//      glDepthFunc(GL_LEQUAL);
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_SRC_COLOR, GL_ZERO);
//
//      for (cnt = 0; cnt < numdolist; cnt++)
//      {
//        tnc = dolist[cnt];
//        if(ChrList[tnc].attachedto == MAXCHR)
//        {
//          if(((ChrList[tnc].light==255 && ChrList[tnc].alpha==255) || CapList[ChrList[tnc].model].forceshadow) && ChrList[tnc].shadowsize!=0)
//            render_shadow(tnc);
//        }
//      }
//
//      glDisable(GL_BLEND);
//      glDepthMask ( GL_TRUE );
//    }
//  }
//
//  // Render the normal characters
//  ATTRIB_PUSH("zref",GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_TRUE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LESS);
//
//    glDisable(GL_BLEND);
//
//    glEnable(GL_ALPHA_TEST);
//    glAlphaFunc(GL_GREATER, 0);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(ChrList[tnc].alpha==255 && ChrList[tnc].light==255)
//        render_mad(tnc, 255);
//    }
//  }
//  ATTRIB_POP("zref");
//
//  //// Render the sprites
//  glDepthMask ( GL_FALSE );
//  glEnable(GL_BLEND);
//
//  // Now render the transparent characters
//  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_FALSE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(ChrList[tnc].alpha!=255 && ChrList[tnc].light==255)
//      {
//        trans = ChrList[tnc].alpha;
//        if(trans < SEEINVISIBLE && (ClientState.seeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//        render_mad(tnc, trans);
//      }
//    }
//
//  }
//
//  // And alpha water floors
//  if(!waterlight)
//    render_water();
//
//  // Then do the light characters
//  glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//  {
//    glDepthMask ( GL_FALSE );
//    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_LEQUAL);
//
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//
//    for (cnt = 0; cnt < numdolist; cnt++)
//    {
//      tnc = dolist[cnt];
//      if(ChrList[tnc].light!=255)
//      {
//        trans = (ChrList[tnc].light * ChrList[tnc].alpha) >> 9;
//        if(trans < SEEINVISIBLE && (ClientState.seeinvisible || ChrList[tnc].islocalplayer))  trans = SEEINVISIBLE;
//        render_mad(tnc, trans);
//      }
//    }
//  }
//
//  // Do phong highlights
//  if(CData.phongon && ChrList[tnc].sheen > 0)
//  {
//    Uint16 texturesave, envirosave;
//
//    ATTRIB_PUSH("zref", GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT);
//    {
//      glDepthMask ( GL_FALSE );
//      glEnable(GL_DEPTH_TEST);
//      glDepthFunc(GL_LEQUAL);
//
//      glEnable(GL_BLEND);
//      glBlendFunc(GL_ONE, GL_ONE);
//
//      envirosave = ChrList[tnc].enviro;
//      texturesave = ChrList[tnc].texture;
//      ChrList[tnc].enviro = btrue;
//      ChrList[tnc].texture = 7;  // The phong map texture...
//      render_enviromad(tnc, (ChrList[tnc].alpha * spek_global[ChrList[tnc].sheen][ChrList[tnc].light]) / 2, GL_TEXTURE_2D);
//      ChrList[tnc].texture = texturesave;
//      ChrList[tnc].enviro = envirosave;
//    };
//    ATTRIB_POP("zref");
//  }
//
//
//  // Do light water
//  if(waterlight)
//    render_water();
//
//  // Turn Z buffer back on, alphablend off
//  render_particles();
//
//  // Done rendering
//};

//--------------------------------------------------------------------------------------------
bool_t get_mesh_memory()
{
  // ZZ> This function gets a load of memory for the terrain mesh
  Mesh.floatmemory = (float *) malloc(CData.maxtotalmeshvertices * BYTESFOREACHVERTEX);
  if (Mesh.floatmemory == NULL)
    return bfalse;

  Mesh.vrtx = &Mesh.floatmemory[0];
  Mesh.vrty = &Mesh.floatmemory[1*CData.maxtotalmeshvertices];
  Mesh.vrtz = &Mesh.floatmemory[2*CData.maxtotalmeshvertices];
  Mesh.vrta = (Uint8 *) & Mesh.floatmemory[3*CData.maxtotalmeshvertices];
  Mesh.vrtl = &Mesh.vrta[CData.maxtotalmeshvertices];
  return btrue;
}

//--------------------------------------------------------------------------------------------
void draw_blip(Uint8 color, int x, int y)
{
  float xl, xr, yt, yb;
  int width, height;

  // ZZ> This function draws a blip
  if (x < 4 || x > CData.scrx + 4 || y < -4 || y > CData.scry + 4) return;


  ATTRIB_PUSH("draw_blip", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    // depth buffer stuff
    glDepthMask(GL_FALSE);

    // CData.shading stuff
    glShadeModel(GL_FLAT);

    // alpha stuff
    glDisable(GL_ALPHA_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDisable(GL_ALPHA_TEST);

    // backface culling
    glDisable(GL_CULL_FACE);

    GLTexture_Bind(&TxBlip, CData.texturefilter);
    xl = ((float)BlipList[color].rect.left) / 32;
    xr = ((float)BlipList[color].rect.right) / 32;
    yt = ((float)BlipList[color].rect.top) / 4;
    yb = ((float)BlipList[color].rect.bottom) / 4;
    width = BlipList[color].rect.right - BlipList[color].rect.left; height = BlipList[color].rect.bottom - BlipList[color].rect.top;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(xl, yb);   glVertex2i(x - 1,       CData.scry - y - 1 - height);
    glTexCoord2f(xr, yb);   glVertex2i(x - 1 + width, CData.scry - y - 1 - height);
    glTexCoord2f(xr, yt);   glVertex2i(x - 1 + width, CData.scry - y - 1);
    glTexCoord2f(xl, yt);   glVertex2i(x - 1,       CData.scry - y - 1);
    glEnd();
  }
  ATTRIB_POP("draw_blip");
}

//--------------------------------------------------------------------------------------------
void draw_one_icon(int icontype, int x, int y, Uint8 sparkle)
{
  // ZZ> This function draws an icon
  int position, blipx, blipy;
  float xl, xr, yt, yb;
  int width, height;

  if (INVALID_TEXTURE == TxIcon[icontype].textureID) return;

  ATTRIB_PUSH("draw_one_icon", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    // depth buffer stuff
    glDepthMask(GL_FALSE);

    // CData.shading stuff
    glShadeModel(GL_FLAT);

    // alpha stuff
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);

    // backface culling
    glDisable(GL_CULL_FACE);

    GLTexture_Bind(&TxIcon[icontype], CData.texturefilter);

    xl = ((float)iconrect.left) / 32;
    xr = ((float)iconrect.right) / 32;
    yt = ((float)iconrect.top) / 32;
    yb = ((float)iconrect.bottom) / 32;
    width = iconrect.right - iconrect.left; height = iconrect.bottom - iconrect.top;

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(xl, yb);   glVertex2i(x,       CData.scry - y - height);
    glTexCoord2f(xr, yb);   glVertex2i(x + width, CData.scry - y - height);
    glTexCoord2f(xr, yt);   glVertex2i(x + width, CData.scry - y);
    glTexCoord2f(xl, yt);   glVertex2i(x,       CData.scry - y);
    glEnd();
  }
  ATTRIB_POP("draw_one_icon");

  if (sparkle != NOSPARKLE)
  {
    position = wldframe & 31;
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
void draw_one_font(int fonttype, int x, int y)
{
  // ZZ> This function draws a letter or number
  // GAC> Very nasty version for starters.  Lots of room for improvement.
  GLfloat dx, dy, fx1, fx2, fy1, fy2;
  GLuint x2, y2;

  y = fontoffset - y;
  x2 = x + fontrect[fonttype].w;
  y2 = y + fontrect[fonttype].h;
  dx = 2.0 / 512;
  dy = 1.0 / 256;
  fx1 = fontrect[fonttype].x * dx + 0.001;
  fx2 = (fontrect[fonttype].x + fontrect[fonttype].w) * dx - 0.001;
  fy1 = fontrect[fonttype].y * dy + 0.001;
  fy2 = (fontrect[fonttype].y + fontrect[fonttype].h) * dy;

  glBegin(GL_QUADS);
  glTexCoord2f(fx1, fy2);   glVertex2i(x, y);
  glTexCoord2f(fx2, fy2);   glVertex2i(x2, y);
  glTexCoord2f(fx2, fy1);   glVertex2i(x2, y2);
  glTexCoord2f(fx1, fy1);   glVertex2i(x, y2);
  glEnd();
}

//--------------------------------------------------------------------------------------------
void draw_map(int x, int y)
{
  // ZZ> This function draws the map

  //printf("draw map getting called\n");

  ATTRIB_PUSH("draw_map", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT);
  {
    // depth buffer stuff
    glDepthMask(GL_FALSE);

    // CData.shading stuff
    glShadeModel(GL_FLAT);

    // alpha stuff
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);

    // backface culling
    glDisable(GL_CULL_FACE);

    GLTexture_Bind(&TxMap, CData.texturefilter);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 1.0); glVertex2i(x,  CData.scry - y - MAPSIZE);
    glTexCoord2f(1.0, 1.0); glVertex2i(x + MAPSIZE, CData.scry - y - MAPSIZE);
    glTexCoord2f(1.0, 0.0); glVertex2i(x + MAPSIZE, CData.scry - y);
    glTexCoord2f(0.0, 0.0); glVertex2i(x,  CData.scry - y);
    glEnd();
  }
  ATTRIB_POP("draw_map");
}

//--------------------------------------------------------------------------------------------
int draw_one_bar(int bartype, int x, int y, int ticks, int maxticks)
{
  // ZZ> This function draws a bar and returns the y position for the next one
  int noticks;
  float xl, xr, yt, yb;
  int width, height;

  ATTRIB_PUSH("draw_one_bar", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    // depth buffer stuff
    glDepthMask(GL_FALSE);

    // CData.shading stuff
    glShadeModel(GL_FLAT);

    // alpha stuff
    glDisable(GL_ALPHA_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // backface culling
    glDisable(GL_CULL_FACE);

    if (maxticks > 0 && ticks >= 0)
    {
      // Draw the tab
      GLTexture_Bind(&TxBars, CData.texturefilter);
      xl = ((float)tabrect[bartype].left) / 128;
      xr = ((float)tabrect[bartype].right) / 128;
      yt = ((float)tabrect[bartype].top) / 128;
      yb = ((float)tabrect[bartype].bottom) / 128;
      width = tabrect[bartype].right - tabrect[bartype].left; height = tabrect[bartype].bottom - tabrect[bartype].top;


      glBegin(GL_QUADS);
      glTexCoord2f(xl, yb);   glVertex2i(x,       CData.scry - y - height);
      glTexCoord2f(xr, yb);   glVertex2i(x + width, CData.scry - y - height);
      glTexCoord2f(xr, yt);   glVertex2i(x + width, CData.scry - y);
      glTexCoord2f(xl, yt);   glVertex2i(x,       CData.scry - y);
      glEnd();

      // Error check
      if (maxticks > MAXTICK) maxticks = MAXTICK;
      if (ticks > maxticks) ticks = maxticks;

      // Draw the full rows of ticks
      x += TABX;
      while (ticks >= NUMTICK)
      {
        barrect[bartype].right = BARX;
        GLTexture_Bind(&TxBars, CData.texturefilter);
        xl = ((float)barrect[bartype].left) / 128;
        xr = ((float)barrect[bartype].right) / 128;
        yt = ((float)barrect[bartype].top) / 128;
        yb = ((float)barrect[bartype].bottom) / 128;
        width = barrect[bartype].right - barrect[bartype].left; height = barrect[bartype].bottom - barrect[bartype].top;

        glBegin(GL_QUADS);
        glTexCoord2f(xl, yb);   glVertex2i(x,       CData.scry - y - height);
        glTexCoord2f(xr, yb);   glVertex2i(x + width, CData.scry - y - height);
        glTexCoord2f(xr, yt);   glVertex2i(x + width, CData.scry - y);
        glTexCoord2f(xl, yt);   glVertex2i(x,       CData.scry - y);
        glEnd();
        y += BARY;
        ticks -= NUMTICK;
        maxticks -= NUMTICK;
      }


      // Draw any partial rows of ticks
      if (maxticks > 0)
      {
        // Draw the filled ones
        barrect[bartype].right = (ticks << 3) + TABX;
        GLTexture_Bind(&TxBars, CData.texturefilter);
        xl = ((float)barrect[bartype].left) / 128;
        xr = ((float)barrect[bartype].right) / 128;
        yt = ((float)barrect[bartype].top) / 128;
        yb = ((float)barrect[bartype].bottom) / 128;
        width = barrect[bartype].right - barrect[bartype].left; height = barrect[bartype].bottom - barrect[bartype].top;

        glBegin(GL_QUADS);
        glTexCoord2f(xl, yb);   glVertex2i(x,       CData.scry - y - height);
        glTexCoord2f(xr, yb);   glVertex2i(x + width, CData.scry - y - height);
        glTexCoord2f(xr, yt);   glVertex2i(x + width, CData.scry - y);
        glTexCoord2f(xl, yt);   glVertex2i(x,       CData.scry - y);
        glEnd();

        // Draw the empty ones
        noticks = maxticks - ticks;
        if (noticks > (NUMTICK - ticks)) noticks = (NUMTICK - ticks);
        barrect[0].right = (noticks << 3) + TABX;
        GLTexture_Bind(&TxBars, CData.texturefilter);
        xl = ((float)barrect[0].left) / 128;
        xr = ((float)barrect[0].right) / 128;
        yt = ((float)barrect[0].top) / 128;
        yb = ((float)barrect[0].bottom) / 128;
        width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;

        glColor4f(1, 1, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2f(xl, yb);   glVertex2i((ticks << 3) + x,       CData.scry - y - height);
        glTexCoord2f(xr, yb);   glVertex2i((ticks << 3) + x + width, CData.scry - y - height);
        glTexCoord2f(xr, yt);   glVertex2i((ticks << 3) + x + width, CData.scry - y);
        glTexCoord2f(xl, yt);   glVertex2i((ticks << 3) + x,       CData.scry - y);
        glEnd();
        maxticks -= NUMTICK;
        y += BARY;
      }


      // Draw full rows of empty ticks
      while (maxticks >= NUMTICK)
      {
        barrect[0].right = BARX;
        GLTexture_Bind(&TxBars, CData.texturefilter);
        xl = ((float)barrect[0].left) / 128;
        xr = ((float)barrect[0].right) / 128;
        yt = ((float)barrect[0].top) / 128;
        yb = ((float)barrect[0].bottom) / 128;
        width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;

        glBegin(GL_QUADS);
        glTexCoord2f(xl, yb);   glVertex2i(x,       CData.scry - y - height);
        glTexCoord2f(xr, yb);   glVertex2i(x + width, CData.scry - y - height);
        glTexCoord2f(xr, yt);   glVertex2i(x + width, CData.scry - y);
        glTexCoord2f(xl, yt);   glVertex2i(x,       CData.scry - y);
        glEnd();
        y += BARY;
        maxticks -= NUMTICK;
      }


      // Draw the last of the empty ones
      if (maxticks > 0)
      {
        barrect[0].right = (maxticks << 3) + TABX;
        GLTexture_Bind(&TxBars, CData.texturefilter);
        xl = ((float)barrect[0].left) / 128;
        xr = ((float)barrect[0].right) / 128;
        yt = ((float)barrect[0].top) / 128;
        yb = ((float)barrect[0].bottom) / 128;
        width = barrect[0].right - barrect[0].left; height = barrect[0].bottom - barrect[0].top;

        glColor4f(1, 1, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2f(xl, yb);   glVertex2i(x,       CData.scry - y - height);
        glTexCoord2f(xr, yb);   glVertex2i(x + width, CData.scry - y - height);
        glTexCoord2f(xr, yt);   glVertex2i(x + width, CData.scry - y);
        glTexCoord2f(xl, yt);   glVertex2i(x,       CData.scry - y);
        glEnd();
        y += BARY;
      }
    }
  }
  ATTRIB_POP("draw_one_bar");

  return y;
}

//--------------------------------------------------------------------------------------------
void draw_string(char *szText, int x, int y)
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer
  Uint8 cTmp = szText[0];
  int cnt = 1;

  BeginText();
  while (cTmp != 0)
  {
    // Convert ASCII to our own little font
    if (cTmp == '~')
    {
      // Use squiggle for tab
      x = x & (~TABAND);
      x += TABAND + 1;
    }
    else if (cTmp == '\n')
    {
      break;

    }
    else
    {
      // Normal letter
      cTmp = asciitofont[cTmp];
      draw_one_font(cTmp, x, y);
      x += fontxspacing[cTmp];
    }
    cTmp = szText[cnt];
    cnt++;
  }
  EndText();
}

//--------------------------------------------------------------------------------------------
int length_of_word(char *szText)
{
  // ZZ> This function returns the number of pixels the
  //     next word will take on screen in the x direction

  // Count all preceeding spaces
  int x = 0;
  int cnt = 0;
  Uint8 cTmp = szText[cnt];

  while (cTmp == ' ' || cTmp == '~' || cTmp == '\n')
  {
    if (cTmp == ' ')
    {
      x += fontxspacing[asciitofont[cTmp]];
    }
    else if (cTmp == '~')
    {
      x += TABAND + 1;
    }
    cnt++;
    cTmp = szText[cnt];
  }


  while (cTmp != ' ' && cTmp != '~' && cTmp != '\n' && cTmp != 0)
  {
    x += fontxspacing[asciitofont[cTmp]];
    cnt++;
    cTmp = szText[cnt];
  }
  return x;
}

//--------------------------------------------------------------------------------------------
int draw_wrap_string(char *szText, int x, int y, int maxx)
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer,
  //     wrapping over the right side and returning the new y value
  int sttx = x;
  Uint8 cTmp = szText[0];
  int newy = y + fontyspacing;
  Uint8 newword = btrue;
  int cnt = 1;

  BeginText();

  maxx = maxx + sttx;

  while (cTmp != 0)
  {
    // Check each new word for wrapping
    if (newword)
    {
      int endx = x + length_of_word(szText + cnt - 1);

      newword = bfalse;
      if (endx > maxx)
      {
        // Wrap the end and cut off spaces and tabs
        x = sttx + fontyspacing;
        y += fontyspacing;
        newy += fontyspacing;
        while (cTmp == ' ' || cTmp == '~')
        {
          cTmp = szText[cnt];
          cnt++;
        }
      }
    }
    else
    {
      if (cTmp == '~')
      {
        // Use squiggle for tab
        x = x & (~TABAND);
        x += TABAND + 1;
      }
      else if (cTmp == '\n')
      {
        x = sttx;
        y += fontyspacing;
        newy += fontyspacing;
      }
      else
      {
        // Normal letter
        cTmp = asciitofont[cTmp];
        draw_one_font(cTmp, x, y);
        x += fontxspacing[cTmp];
      }
      cTmp = szText[cnt];
      if (cTmp == '~' || cTmp == ' ')
      {
        newword = btrue;
      }
      cnt++;
    }
  }
  EndText();
  return newy;
}

//--------------------------------------------------------------------------------------------
int draw_status(Uint16 character, int x, int y)
{
  // ZZ> This function shows a character's icon, status and inventory
  //     The x,y coordinates are the top left point of the image to draw
  Uint16 item;
  char cTmp;
  char *readtext;

  int life = ChrList[character].life >> 8;
  int lifemax = ChrList[character].lifemax >> 8;
  int mana = ChrList[character].mana >> 8;
  int manamax = ChrList[character].manamax >> 8;
  int cnt = lifemax;

  /* [claforte] This can be removed
  Note: This implies that the status line assumes that the MAX life and mana
  representable is 50.

  if(cnt > 50) cnt = 50;
  if(cnt == 0) cnt = -9;
  cnt = manamax;
  if(cnt > 50) cnt = 50;
  if(cnt == 0) cnt = -9;
  */

  // Write the character's first name
  if (ChrList[character].nameknown)
    readtext = ChrList[character].name;
  else
    readtext = CapList[ChrList[character].model].classname;

  for (cnt = 0; cnt < 6; cnt++)
  {
    cTmp = readtext[cnt];
    if (cTmp == ' ' || cTmp == 0)
    {
      generictext[cnt] = 0;
      break;

    }
    else
      generictext[cnt] = cTmp;
  }
  generictext[6] = 0;
  draw_string(generictext, x + 8, y); y += fontyspacing;


  // Write the character's money
  snprintf(generictext, sizeof(generictext), "$%4d", ChrList[character].money);
  draw_string(generictext, x + 8, y); y += fontyspacing + 8;


  // Draw the icons
  draw_one_icon(skintoicon[ChrList[character].texture], x + 40, y, ChrList[character].sparkle);
  item = ChrList[character].holdingwhich[0];
  if (item != MAXCHR)
  {
    if (ChrList[item].icon)
    {
      draw_one_icon(skintoicon[ChrList[item].texture], x + 8, y, ChrList[item].sparkle);
      if (ChrList[item].ammomax != 0 && ChrList[item].ammoknown)
      {
        if (CapList[ChrList[item].model].isstackable == bfalse || ChrList[item].ammo > 1)
        {
          // Show amount of ammo left
          snprintf(generictext, sizeof(generictext), "%2d", ChrList[item].ammo);
          draw_string(generictext, x + 8, y - 8);
        }
      }
    }
    else
      draw_one_icon(bookicon + (ChrList[item].money % MAXSKIN), x + 8, y, ChrList[item].sparkle);
  }
  else
    draw_one_icon(nullicon, x + 8, y, NOSPARKLE);

  item = ChrList[character].holdingwhich[1];
  if (item != MAXCHR)
  {
    if (ChrList[item].icon)
    {
      draw_one_icon(skintoicon[ChrList[item].texture], x + 72, y, ChrList[item].sparkle);
      if (ChrList[item].ammomax != 0 && ChrList[item].ammoknown)
      {
        if (CapList[ChrList[item].model].isstackable == bfalse || ChrList[item].ammo > 1)
        {
          // Show amount of ammo left
          snprintf(generictext, sizeof(generictext), "%2d", ChrList[item].ammo);
          draw_string(generictext, x + 72, y - 8);
        }
      }
    }
    else
      draw_one_icon(bookicon + (ChrList[item].money % MAXSKIN), x + 72, y, ChrList[item].sparkle);
  }
  else
    draw_one_icon(nullicon, x + 72, y, NOSPARKLE);

  y += 32;

  // Draw the bars
  if (ChrList[character].alive)
    y = draw_one_bar(ChrList[character].lifecolor, x, y, life, lifemax);
  else
    y = draw_one_bar(0, x, y, 0, lifemax);  // Draw a black bar

  y = draw_one_bar(ChrList[character].manacolor, x, y, mana, manamax);
  return y;
}

//--------------------------------------------------------------------------------------------
void draw_text(GAME_STATE * gs)
{
  // ZZ> This function spits out some words
  char text[512];
  int y, cnt, tnc, fifties, seconds, minutes;

  if(NULL==gs) return;

  Begin2DMode();
  // Status bars
  y = 0;
  if (gs->cd->staton)
  {
    for (cnt = 0; cnt < numstat && y < gs->cd->scry; cnt++)
      y = draw_status(statlist[cnt], gs->cd->scrx - BARX, y);
  }

  // Map display
  if (mapon)
  {
    draw_map(0, gs->cd->scry - MAPSIZE);

    for (cnt = 0; cnt < numblip; cnt++)
      draw_blip(BlipList[cnt].c, BlipList[cnt].x, BlipList[cnt].y + gs->cd->scry - MAPSIZE);

    if (youarehereon && (wldframe&8))
    {
      for (cnt = 0; cnt < MAXPLAYER; cnt++)
      {
        if (PlaList[cnt].valid && PlaList[cnt].device != INPUTNONE)
        {
          tnc = PlaList[cnt].index;
          if (ChrList[tnc].alive)
            draw_blip(0, ChrList[tnc].xpos*MAPSIZE / Mesh.edgex, (ChrList[tnc].ypos*MAPSIZE / Mesh.edgey) + gs->cd->scry - MAPSIZE);
        }
      }
    }
  }


  // FPS text
  y = 0;
  if (outofsync)
  {
    snprintf(text, sizeof(text), "OUT OF SYNC, TRY RTS...");
    draw_string(text, 0, y);  y += fontyspacing;
  }

  if (parseerror && gs->cd->DevMode)
  {
    snprintf(text, sizeof(text), "SCRIPT ERROR ( SEE LOG.TXT )");
    draw_string(text, 0, y);
    y += fontyspacing;
  }

  if (gs->cd->fpson)
  {
    draw_string(szfpstext, 0, y);
    y += fontyspacing;
  }

  if (SDLKEYDOWN(SDLK_F1))
  {
    // In-Game help
    snprintf(text, sizeof(text), "!!!MOUSE HELP!!!");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Edit CONTROLS.TXT to change");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Left Click to use an item");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Left and Right Click to grab");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Middle Click to jump");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  A and S GKeyb.s do stuff");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Right Drag to move camera");
    draw_string(text, 0, y);  y += fontyspacing;
  }

  if (SDLKEYDOWN(SDLK_F2))
  {
    // In-Game help
    snprintf(text, sizeof(text), "!!!JOYSTICK HELP!!!");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Edit CONTROLS.TXT to change");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Hit the buttons");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  You'll figure it out");
    draw_string(text, 0, y);  y += fontyspacing;
  }

  if (SDLKEYDOWN(SDLK_F3))
  {
    // In-Game help
    snprintf(text, sizeof(text), "!!!KEYBOARD HELP!!!");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Edit CONTROLS.TXT to change");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  TGB control one hand");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  YHN control the other");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Keypad to move and jump");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  Number GKeyb.s for stats");
    draw_string(text, 0, y);  y += fontyspacing;
  }

  if (SDLKEYDOWN(SDLK_F5) && gs->cd->DevMode)
  {
    // Debug information
    snprintf(text, sizeof(text), "!!!DEBUG MODE-5!!!");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  CAM %f %f", GCamera.x, GCamera.y);
    draw_string(text, 0, y);  y += fontyspacing;
    tnc = PlaList[0].index;
    snprintf(text, sizeof(text), "  PLA0DEF %d %d %d %d %d %d %d %d",
             ChrList[tnc].damagemodifier[0]&3,
             ChrList[tnc].damagemodifier[1]&3,
             ChrList[tnc].damagemodifier[2]&3,
             ChrList[tnc].damagemodifier[3]&3,
             ChrList[tnc].damagemodifier[4]&3,
             ChrList[tnc].damagemodifier[5]&3,
             ChrList[tnc].damagemodifier[6]&3,
             ChrList[tnc].damagemodifier[7]&3);
    draw_string(text, 0, y);  y += fontyspacing;
    tnc = PlaList[0].index;
    snprintf(text, sizeof(text), "  PLA0 %5.1f %5.1f", ChrList[tnc].xpos / 128.0, ChrList[tnc].ypos / 128.0);
    draw_string(text, 0, y);  y += fontyspacing;
    tnc = PlaList[1].index;
    snprintf(text, sizeof(text), "  PLA1 %5.1f %5.1f", ChrList[tnc].xpos / 128.0, ChrList[tnc].ypos / 128.0);
    draw_string(text, 0, y);  y += fontyspacing;
  }

  if (SDLKEYDOWN(SDLK_F6) && gs->cd->DevMode)
  {
    // More debug information
    snprintf(text, sizeof(text), "!!!DEBUG MODE-6!!!");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  FREEPRT %d", numfreeprt);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  FREECHR %d",  numfreechr);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  MACHINE %d", localmachine);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  EXPORT %d", gs->modstate.exportvalid);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  FOGAFF %d", GFog.affectswater);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  PASS %d/%d", numshoppassage, numpassage);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  NETPLAYERS %d", gs->ss->num_loaded);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "  DAMAGEPART %d", GTile_Dam.parttype);
    draw_string(text, 0, y);  y += fontyspacing;
  }

  if (SDLKEYDOWN(SDLK_F7) && gs->cd->DevMode)
  {
    // White debug mode
    snprintf(text, sizeof(text), "!!!DEBUG MODE-7!!!");
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "CAM %f %f %f %f", (mView)_CNV(0, 0), (mView)_CNV(1, 0), (mView)_CNV(2, 0), (mView)_CNV(3, 0));
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "CAM %f %f %f %f", (mView)_CNV(0, 1), (mView)_CNV(1, 1), (mView)_CNV(2, 1), (mView)_CNV(3, 1));
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "CAM %f %f %f %f", (mView)_CNV(0, 2), (mView)_CNV(1, 2), (mView)_CNV(2, 2), (mView)_CNV(3, 2));
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "CAM %f %f %f %f", (mView)_CNV(0, 3), (mView)_CNV(1, 3), (mView)_CNV(2, 3), (mView)_CNV(3, 3));
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "x %f", GCamera.centerx);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "y %f", GCamera.centery);
    draw_string(text, 0, y);  y += fontyspacing;
    snprintf(text, sizeof(text), "turn %d %d", gs->cd->autoturncamera, doturntime);
    draw_string(text, 0, y);  y += fontyspacing;
  }

  //Draw paused text
  if (gs->paused && !SDLKEYDOWN(SDLK_F11))
  {
    snprintf(text, sizeof(text), "GAME PAUSED");
    draw_string(text, -90 + gs->cd->scrx / 2, 0 + gs->cd->scry / 2);
  }

  //Pressed panic button
  if (SDLKEYDOWN(SDLK_q) && SDLKEYDOWN(SDLK_LCTRL))
  {
    log_info("User pressed escape button (LCTRL+Q)... Quitting game.\n");
    gs->Active = bfalse;
    gs->moduleActive = bfalse;
  }

  if (timeron)
  {
    fifties = (timervalue % 50) << 1;
    seconds = ((timervalue / 50) % 60);
    minutes = (timervalue / 3000);
    snprintf(text, sizeof(text), "=%d:%02d:%02d=", minutes, seconds, fifties);
    draw_string(text, 0, y);
    y += fontyspacing;
  }
  if (gs->cs->waiting)
  {
    snprintf(text, sizeof(text), "Waiting for players...");
    draw_string(text, 0, y);
    y += fontyspacing;
  }
  if (!gs->modstate.rts_control)
  {
    if (gs->cs->allpladead || gs->modstate.respawnanytime)
    {
      if (gs->modstate.respawnvalid)
      {
        snprintf(text, sizeof(text), "PRESS SPACE TO RESPAWN");
      }
      else
      {
        snprintf(text, sizeof(text), "PRESS ESCAPE TO QUIT");
      }
      draw_string(text, 0, y);
      y += fontyspacing;
    }
    else
    {
      if (gs->modstate.beat)
      {
        snprintf(text, sizeof(text), "VICTORY!  PRESS ESCAPE");
        draw_string(text, 0, y);
        y += fontyspacing;
      }
    }
  }


  // Network message input
  if (gs->modstate.net_messagemode)
  {
    y = draw_wrap_string(GNetMsg.buffer, 0, y, gs->cd->scrx - gs->cd->wraptolerance);
  }


  // Messages
  if (gs->cd->messageon)
  {
    // Display the messages
    tnc = GMsg.start;

    for (cnt = 0; cnt < gs->cd->maxmessage; cnt++)
    {
      if (GMsg.list[tnc].time > 0)
      {
        y = draw_wrap_string(GMsg.list[tnc].textdisplay, 0, y, gs->cd->scrx - gs->cd->wraptolerance);
        GMsg.list[tnc].time -= gs->cs->msg_timechange;
      }
      tnc++;
      tnc = tnc % gs->cd->maxmessage;
    }
  }
  End2DMode();
}

//--------------------------------------------------------------------------------------------
void flip_pages()
{
  SDL_GL_SwapBuffers();
}

//--------------------------------------------------------------------------------------------
void draw_scene(GAME_STATE * gs)
{
  Begin3DMode();

  make_prtlist();
  do_dynalight();
  light_characters();
  light_particles();

  // Render the background
  if (clearson == bfalse)
  {
    render_background(TX_WATERLOW);  // 6 is the texture for waterlow.bmp
  }

  //if(gs->cd->zreflect) //DO REFLECTIONS
  draw_scene_zreflection(gs);
  //else
  //  draw_scene_sadreflection();

  //Foreground overlay
  if (gs->cd->overlayon)
  {
    render_foreground_overlay(TX_WATERTOP);  // Texture 5 is watertop.bmp
  }

  End3DMode();
}

//--------------------------------------------------------------------------------------------
void draw_ingameMenu(GAME_STATE * gs, float frameDuration)
{
  int menuResult;

  ui_beginFrame(frameDuration);

  menuResult = doIngameMenu((float)frameDuration);
  switch (menuResult)
  {
    case 1: /* nothing */
      break;

    case - 1:
      // The user selected "Quit"
      gs->ingameMenuActive = bfalse;
      SDL_WM_GrabInput(SDL_GRAB_OFF);
      GMous.on = btrue;
      break;
  }

  ui_endFrame();
};

//--------------------------------------------------------------------------------------------
void draw_main(GAME_STATE * gs, float frameDuration)
{
  // ZZ> This function does all the drawing stuff
  //printf("DIAG: Drawing scene Mesh.numrenderlist_ref=%d\n",Mesh.numrenderlist_ref);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  draw_scene(gs);
  draw_text(gs);
  //if (gs->modstate.rts_control)
  //  do_cursor_rts();

  /* In Game Menu */
  if (gs->ingameMenuActive)
  {
    draw_ingameMenu(gs, frameDuration);
  };

  request_pageflip();
}

//--------------------------------------------------------------------------------------------
Uint32 load_one_module_image(int titleimage, char *szLoadName)
{
  // ZZ> This function loads a title in the specified image slot, forcing it into
  //     system memory.  Returns btrue if it worked
  Uint32 retval = MAXMODULE;

  if(INVALID_TEXTURE != GLTexture_Load(GL_TEXTURE_2D,  &TxTitleImage[titleimage], szLoadName, INVALID_KEY))
  {
    retval = titleimage;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
size_t load_all_module_data(MOD_INFO * modinfo, size_t infocount)
{
  // ZZ> This function loads the title image for each module.  Modules without a
  //     title are marked as invalid

  char searchname[15];
  STRING loadname;
  const char *FileName;
  FILE* filesave;
  size_t modcount;

  // Convert searchname
  strcpy(searchname, "modules/*.mod");

  // Log a directory list
  filesave = fopen(CData.modules_file, "w");
  if (filesave != NULL)
  {
    fprintf(filesave, "This file logs all of the modules found\n");
    fprintf(filesave, "** Denotes an invalid module (Or locked)\n\n");
  }
  else log_warning("Could not write to modules.txt\n");

  // Search for .mod directories
  FileName = fs_findFirstFile(CData.modules_dir, "mod");
  modcount = 0;
  while (FileName && modcount < infocount)
  {
    strncpy(modinfo[modcount].loadname, FileName, sizeof(modinfo[modcount].loadname));
    snprintf(loadname, sizeof(loadname), "%s/%s/%s/%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.menu_file);
    if (rv_succeed == get_module_data(modcount, loadname, modinfo, infocount))
    {
      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s/%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.title_bitmap);
      modinfo[modcount].texture_idx = load_one_module_image(modcount, CStringTmp1);
      if (MAXMODULE != modinfo[modcount].texture_idx)
      {
        fprintf(filesave, "%02d.  %s\n", modcount, modinfo[modcount].longname);
        modcount++;
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

  return modcount;
}

//--------------------------------------------------------------------------------------------
void load_blip_bitmap(char * modname)
{
  //This function loads the blip bitmaps
  int cnt;

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.blip_bitmap);
  if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &TxBlip, CStringTmp1, INVALID_KEY))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.blip_bitmap);
    if (INVALID_TEXTURE == GLTexture_Load(GL_TEXTURE_2D,  &TxBlip, CStringTmp1, INVALID_KEY))
    {
      log_warning("Blip bitmap not loaded. Missing file = \"%s\"\n", CStringTmp1);
    }
  };

  // Set up the rectangles
  for (cnt = 0; cnt < NUMBAR; cnt++)
  {
    BlipList[cnt].rect.left   = cnt * BLIPSIZE;
    BlipList[cnt].rect.right  = (cnt * BLIPSIZE) + BLIPSIZE;
    BlipList[cnt].rect.top    = 0;
    BlipList[cnt].rect.bottom = BLIPSIZE;
  }
}

////--------------------------------------------------------------------------------------------
//void load_menu(MOD_INFO * modinfo, size_t infocount)
//{
//  // ZZ> This function loads all of the menu data...  Images are loaded into system
//  // memory
//
//  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.font_bitmap);
//  snprintf(CStringTmp2, sizeof(CStringTmp2), "%s/%s", CData.basicdat_dir, CData.fontdef_file);
//  load_font(CStringTmp1, CStringTmp2);
//  load_all_module_data(modinfo, infocount);
//}

//--------------------------------------------------------------------------------------------
void do_cursor()
{
  // This function implements a mouse cursor
  read_input();
  cursorx = GMous.x;  if (cursorx < 6)  cursorx = 6;  if (cursorx > CData.scrx - 16)  cursorx = CData.scrx - 16;
  cursory = GMous.y;  if (cursory < 8)  cursory = 8;  if (cursory > CData.scry - 24)  cursory = CData.scry - 24;
  clicked = bfalse;
  if (GMous.button[0] && pressed == bfalse)
  {
    clicked = btrue;
  }
  pressed = GMous.button[0];
  BeginText();  // Needed to setup text mode
  //draw_one_font(11, cursorx-5, cursory-7);
  draw_one_font(95, cursorx - 5, cursory - 7);
  EndText();    // Needed when done with text mode
}

/********************> Reshape3D() <*****/
void Reshape3D(int w, int h)
{
  glViewport(0, 0, w, h);
}

int glinit(int argc, char **argv)
{
  // get the maximum anisotropy fupported by the video vard
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
  log2Anisotropy = (maxAnisotropy == 0) ? 0 : floor(log(maxAnisotropy + 1e-6) / log(2.0f));

  if (maxAnisotropy == 0.0f && CData.texturefilter >= TX_ANISOTROPIC)
  {
    CData.texturefilter = TX_TRILINEAR_2;
  }
  userAnisotropy = MIN(maxAnisotropy, userAnisotropy);

  /* Depth testing stuff */
  glClearDepth(1.0);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);

  //Load the current graphical settings
  load_graphics();

  //fill mode
  glPolygonMode(GL_FRONT, GL_FILL);
  glPolygonMode(GL_BACK,  GL_FILL);

  /* Disable OpenGL lighting */
  glDisable(GL_LIGHTING);

  /* Backface culling */
  glEnable(GL_CULL_FACE);    // This seems implied - DDOI
  glCullFace(GL_BACK);

  // set up environment mapping
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP); // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP); // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

  glEnable( GL_TEXTURE_2D );  // Enable texture mapping

  return 1;
}

void sdlinit(int argc, char **argv)
{
  int cnt, colordepth;
  SDL_Surface *theSurface;
  STRING strbuffer = {0};

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
  {
    log_error("Unable to initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  atexit(SDL_Quit);

  // log the video driver info
  SDL_VideoDriverName(strbuffer, 256);
  if(CData.DevMode) log_info("Video Driver - %s\n", strbuffer);

  colordepth = CData.scrd / 3;

  /* Setup the cute windows manager icon */
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.icon_bitmap);
  theSurface = SDL_LoadBMP(CStringTmp1);
  if (theSurface == NULL)
  {
    log_warning("Unable to load icon (basicdat/icon.bmp)\n");
  }
  SDL_WM_SetIcon(theSurface, NULL);

  /* Set the OpenGL Attributes */
#ifndef __unix__
  /* Under Unix we cannot specify these, we just get whatever format
  the framebuffer has, specifying depths > the framebuffer one
  will cause SDL_SetVideoMode to fail with:
  Unable to set video mode: Couldn't find matching GLX visual */
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, colordepth);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, colordepth);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  colordepth);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, CData.scrd);
#endif
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  displaySurface = SDL_SetVideoMode(CData.scrx, CData.scry, CData.scrd, SDL_DOUBLEBUF | SDL_OPENGL | (CData.fullscreen ? SDL_FULLSCREEN : 0));
  if (displaySurface == NULL)
  {
    log_error("Unable to set video mode: %s\n", SDL_GetError());
    exit(1);
  }
  video_mode_chaged = bfalse;

  // grab all the available video modes
  video_mode_list = SDL_ListModes(displaySurface->format, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN | SDL_OPENGL | SDL_HWACCEL | SDL_SRCALPHA);
  log_info("Detecting avalible video modes...\n");
  for (cnt = 0; NULL != video_mode_list[cnt]; ++cnt)
  {
    log_info("Video Mode - %d x %d\n", video_mode_list[cnt]->w, video_mode_list[cnt]->h);
  };

  // Set the window name
  SDL_WM_SetCaption("Egoboo", "Egoboo");

  // set the mouse cursor
  SDL_WM_GrabInput(CData.GrabMouse);
  //if (CData.HideMouse) SDL_ShowCursor(SDL_DISABLE);

  if (SDL_NumJoysticks() > 0)
  {
    sdljoya = SDL_JoystickOpen(0);
    GJoy[0].on = btrue;
  }
}

void load_graphics()
{
  //This function loads all the graphics based on the game settings
  GLenum quality;

  //Check if the computer graphic driver supports anisotropic filtering
  if (CData.texturefilter >= TX_ANISOTROPIC)
  {
    if (!strstr((char*)glGetString(GL_EXTENSIONS), "GL_EXT_texture_filter_anisotropic"))
    {
      log_warning("Your graphics driver does not support anisotropic filtering.\n");
      CData.texturefilter = TX_TRILINEAR_2; //Set filtering to trillienar instead
    }
  }

  //Enable prespective correction?
  if (CData.perspective) quality = GL_NICEST;
  else quality = GL_FASTEST;
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, quality);

  //Enable dithering? (This actually reduces quality but increases preformance)
  if (CData.dither) glEnable(GL_DITHER);
  else glDisable(GL_DITHER);

  //Enable gourad CData.shading? (Important!)
  glShadeModel(CData.shading);

  //Enable CData.antialiasing?
  if (CData.antialiasing)
  {
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,    GL_NICEST);

    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,   GL_NICEST);

    //glEnable(GL_POLYGON_SMOOTH);     //Caused some glitches
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  }
  else
  {
    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_LINE_SMOOTH);
    //glDisable(GL_POLYGON_SMOOTH);
  }
}

/* obsolete graphics functions */
#if 0
////--------------------------------------------------------------------------------------------
//void draw_titleimage(int image, int x, int y)
//{
//  // ZZ> This function draws a title image on the backbuffer
//  GLfloat txWidth, txHeight;
//
//  if ( INVALID_TEXTURE != GLTexture_GetTextureID(&TxTitleImage[image]) )
//  {
//    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
//    Begin2DMode();
//    glNormal3f( 0, 0, 1 ); // glNormal3f( 0, 1, 0 );
//
//    /* Calculate the texture width & height */
//    txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTitleImage[image] )/GLTexture_GetTextureWidth( &TxTitleImage[image] ) );
//    txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTitleImage[image] )/GLTexture_GetTextureHeight( &TxTitleImage[image] ) );
//
//    /* Bind the texture */
//    GLTexture_Bind( &TxTitleImage[image], CData.texturefilter );
//
//    /* Draw the quad */
//    glBegin( GL_QUADS );
//    glTexCoord2f( 0, 1 ); glVertex2f( x, CData.scry - y - GLTexture_GetImageHeight( &TxTitleImage[image] ) );
//    glTexCoord2f( txWidth, 1 ); glVertex2f( x + GLTexture_GetImageWidth( &TxTitleImage[image] ), CData.scry - y - GLTexture_GetImageHeight( &TxTitleImage[image] ) );
//    glTexCoord2f( txWidth, 1-txHeight ); glVertex2f( x + GLTexture_GetImageWidth( &TxTitleImage[image] ), CData.scry - y );
//    glTexCoord2f( 0, 1-txHeight ); glVertex2f( x, CData.scry - y );
//    glEnd();
//
//    End2DMode();
//  }
//}
#endif


//--------------------------------------------------------------------------------------------
//void do_cursor_rts()
//{
//  /*    // This function implements the RTS mouse cursor
//  int sttx, stty, endx, endy, target, leader;
//  Sint16 sound;
//
//
//  if(GMous.button[1] == 0)
//  {
//  cursorx+=GMous.dx;
//  cursory+=GMous.dy;
//  }
//  if(cursorx < 6)  cursorx = 6;  if (cursorx > CData.scrx-16)  cursorx = CData.scrx-16;
//  if(cursory < 8)  cursory = 8;  if (cursory > CData.scry-24)  cursory = CData.scry-24;
//  move_GRTS.xy();
//  if(GMous.button[0])
//  {
//  // Moving the end select point
//  pressed = btrue;
//  GRTS.endx = cursorx+5;
//  GRTS.endy = cursory+7;
//
//  // Draw the selection rectangle
//  if(allselect == bfalse)
//  {
//  sttx = GRTS.sttx;  endx = GRTS.endx;  if(sttx > endx)  {  sttx = GRTS.endx;  endx = GRTS.sttx; }
//  stty = GRTS.stty;  endy = GRTS.endy;  if(stty > endy)  {  stty = GRTS.endy;  endy = GRTS.stty; }
//  draw_trim_box(sttx, stty, endx, endy);
//  }
//  }
//  else
//  {
//  if(pressed)
//  {
//  // See if we selected anyone
//  if((ABS(GRTS.sttx - GRTS.endx) + ABS(GRTS.stty - GRTS.endy)) > 10 && allselect == bfalse)
//  {
//  // We drew a box alright
//  sttx = GRTS.sttx;  endx = GRTS.endx;  if(sttx > endx)  {  sttx = GRTS.endx;  endx = GRTS.sttx; }
//  stty = GRTS.stty;  endy = GRTS.endy;  if(stty > endy)  {  stty = GRTS.endy;  endy = GRTS.stty; }
//  build_select(sttx, stty, endx, endy, GRTS.localteam);
//  }
//  else
//  {
//  // We want to issue an order
//  if(numrtsselect > 0)
//  {
//  leader = GRTS.select[0];
//  sttx = GRTS.sttx-20;  endx = GRTS.sttx+20;
//  stty = GRTS.stty-20;  endy = GRTS.stty+20;
//  target = build_select_target(sttx, stty, endx, endy, GRTS.localteam);
//  if(target == MAXCHR)
//  {
//  // No target...
//  if(SDLKEYDOWN(SDLK_LSHIFT) || SDLKEYDOWN(SDLK_RSHIFT))
//  {
//  send_rts_order(GRTS.x, GRTS.y, RTSTERRAIN, target);
//  sound = ChrList[leader].wavespeech[SPEECHTERRAIN];
//  }
//  else
//  {
//  send_rts_order(GRTS.x, GRTS.y, RTSMOVE, target);
//  sound = wldframe&1;  // Move or MoveAlt
//  sound = ChrList[leader].wavespeech[sound];
//  }
//  }
//  else
//  {
//  if(TeamList[GRTS.localteam].hatesteam[ChrList[target].team])
//  {
//  // Target is an enemy, so issue an attack order
//  send_rts_order(GRTS.x, GRTS.y, RTSATTACK, target);
//  sound = ChrList[leader].wavespeech[SPEECHATTACK];
//  }
//  else
//  {
//  // Target is a friend, so issue an assist order
//  send_rts_order(GRTS.x, GRTS.y, RTSASSIST, target);
//  sound = ChrList[leader].wavespeech[SPEECHASSIST];
//  }
//  }
//  // Do unit speech at 11025 KHz
//  if(sound >= 0 && sound < MAXWAVE)
//  {
//  play_sound_pvf(CapList[ChrList[leader].model].waveindex[sound], PANMID, VOLMAX, 11025);
//  }
//  }
//  }
//  pressed = bfalse;
//  }
//
//
//  // Moving the select point
//  GRTS.sttx = cursorx+5;
//  GRTS.stty = cursory+7;
//  GRTS.endx = cursorx+5;
//  GRTS.endy = cursory+7;
//  }
//
//  // GAC - Don't forget to BeginText() and EndText();
//  BeginText();
//  draw_one_font(11, cursorx-5, cursory-7);
//  EndText ();*/
//}


////--------------------------------------------------------------------------------------------
//void build_select(float tlx, float tly, float brx, float bry, Uint8 team)
//{
//  // ZZ> This function checks which characters are in the selection rectangle
//  /*PORT
//  D3DLVERTEX v[MAXPRT];
//  D3DTLVERTEX vt[MAXPRT];
//  int numbertocheck, character, cnt, first, sound;
//
//
//  // Unselect old ones
//  clear_select();
//
//
//  // Figure out who to check
//  numbertocheck = 0;
//  cnt = 0;
//  while(cnt < MAXCHR)
//  {
//  if(ChrList[cnt].team == team && ChrList[cnt].on && !ChrList[cnt].inpack)
//  {
//  v[numbertocheck].x = (D3DVALUE) ChrList[cnt].xpos;
//  v[numbertocheck].y = (D3DVALUE) ChrList[cnt].ypos;
//  v[numbertocheck].z = (D3DVALUE) ChrList[cnt].zpos;
//  v[numbertocheck].color = cnt;  // Store an index in the color slot...
//  v[numbertocheck].dwReserved = 0;
//  numbertocheck++;
//  }
//  cnt++;
//  }
//
//
//  // Figure out where the points go onscreen
//  lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
//  transform_vertices(numbertocheck, v, vt);
//
//
//  first = btrue;
//  cnt = 0;
//  while(cnt < numbertocheck)
//  {
//  // Only check if in front of camera
//  if(vt[cnt].dvRHW > 0)
//  {
//  // Check the rectangle
//  if(vt[cnt].dvSX > tlx && vt[cnt].dvSX < brx)
//  {
//  if(vt[cnt].dvSY > tly && vt[cnt].dvSY < bry)
//  {
//  // Select the character
//  character = v[cnt].color;
//  add_select(character);
//  if(first)
//  {
//  // Play the select speech for the first one picked
//  sound = ChrList[character].wavespeech[SPEECHSELECT];
//  if(sound >= 0 && sound < MAXWAVE)
//  play_sound_pvf(CapList[ChrList[character].model].waveindex[sound], PANMID, VOLMAX, 11025);
//  first = bfalse;
//  }
//  }
//  }
//  }
//  cnt++;
//  }
//  */
//}

//--------------------------------------------------------------------------------------------
//Uint16 build_select_target(float tlx, float tly, float brx, float bry, Uint8 team)
//{
//  // ZZ> This function checks which characters are in the selection rectangle,
//  //     and returns the first one found
//  /*PORT
//  D3DLVERTEX v[MAXPRT];
//  D3DTLVERTEX vt[MAXPRT];
//  int numbertocheck, character, cnt;
//
//
//  // Figure out who to check
//  numbertocheck = 0;
//  // Add enemies first
//  cnt = 0;
//  while(cnt < MAXCHR)
//  {
//  if(TeamList[team].hatesteam[ChrList[cnt].team] && ChrList[cnt].on && !ChrList[cnt].inpack)
//  {
//  v[numbertocheck].x = (D3DVALUE) ChrList[cnt].xpos;
//  v[numbertocheck].y = (D3DVALUE) ChrList[cnt].ypos;
//  v[numbertocheck].z = (D3DVALUE) ChrList[cnt].zpos;
//  v[numbertocheck].color = cnt;  // Store an index in the color slot...
//  v[numbertocheck].dwReserved = 0;
//  numbertocheck++;
//  }
//  cnt++;
//  }
//  // Add allies next
//  cnt = 0;
//  while(cnt < MAXCHR)
//  {
//  if(TeamList[team].hatesteam[ChrList[cnt].team] == bfalse && ChrList[cnt].on && !ChrList[cnt].inpack)
//  {
//  v[numbertocheck].x = (D3DVALUE) ChrList[cnt].xpos;
//  v[numbertocheck].y = (D3DVALUE) ChrList[cnt].ypos;
//  v[numbertocheck].z = (D3DVALUE) ChrList[cnt].zpos;
//  v[numbertocheck].color = cnt;  // Store an index in the color slot...
//  v[numbertocheck].dwReserved = 0;
//  numbertocheck++;
//  }
//  cnt++;
//  }
//
//
//  // Figure out where the points go onscreen
//  lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
//  transform_vertices(numbertocheck, v, vt);
//
//
//  cnt = 0;
//  while(cnt < numbertocheck)
//  {
//  // Only check if in front of camera
//  if(vt[cnt].dvRHW > 0)
//  {
//  // Check the rectangle
//  if(vt[cnt].dvSX > tlx && vt[cnt].dvSX < brx)
//  {
//  if(vt[cnt].dvSY > tly && vt[cnt].dvSY < bry)
//  {
//  // Select the character
//  character = v[cnt].color;
//  return character;
//  }
//  }
//  }
//  cnt++;
//  }
//  return MAXCHR;
//  */
//  return 0;
//}

//--------------------------------------------------------------------------------------------
//void move_GRTS.xy()
//{
//  // ZZ> This function iteratively transforms the cursor back to world coordinates
//  /*PORT
//  D3DLVERTEX v[1];
//  D3DTLVERTEX vt[1];
//  int numbertocheck, x, y, fan;
//  float sin, cos, trailrate, level;
//
//
//
//  // Figure out where the GRTS.xy is at on the screen
//  x = GRTS.x;
//  y = GRTS.y;
//  fan = Mesh.fanstart[y>>7]+(x>>7);
//  level = get_level(GRTS.x, GRTS.y, fan, bfalse);
//  v[0].x = (D3DVALUE) GRTS.x;
//  v[0].y = (D3DVALUE) GRTS.y;
//  v[0].z = level;
//  v[0].color = 0;
//  v[0].dwReserved = 0;
//  numbertocheck = 1;
//
//
//  // Figure out where the points go onscreen
//  lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
//  transform_vertices(numbertocheck, v, vt);
//
//
//  if(vt[0].dvRHW < 0)
//  {
//  // Move it to GCamera.trackxy if behind the camera
//  GRTS.x = GCamera.trackx;
//  GRTS.y = GCamera.tracky;
//  }
//  else
//  {
//  // Move it to closer to the onscreen cursor
//  trailrate = ABS(cursorx-vt[0].dvSX) + ABS(cursory-vt[0].dvSY);
//  trailrate *= GRTS.trailrate;
//  sin = turntosin[(GCamera.turnleftrightshort>>2) & TRIGTABLE_MASK]*trailrate;
//  cos = turntosin[((GCamera.turnleftrightshort>>2)+4096) & TRIGTABLE_MASK]*trailrate;
//  if(vt[0].dvSX < cursorx)
//  {
//  GRTS.x += cos;
//  GRTS.y -= sin;
//  }
//  else
//  {
//  GRTS.x -= cos;
//  GRTS.y += sin;
//  }
//
//
//
//  if(vt[0].dvSY < cursory)
//  {
//  GRTS.x += sin;
//  GRTS.y += cos;
//  }
//  else
//  {
//  GRTS.x -= sin;
//  GRTS.y -= cos;
//  }
//  }
//  */
//}
//
