// Egoboo - Font.h
// True-type text drawing interface.

#pragma once

#define egoboo_Font_h

#include "egobootypedef.h"
#include <SDL_opengl.h>
#include <SDL_ttf.h>

#include <map>
using std::map;

#include <string>
using std::string;


struct Font;
struct Font_TTF;
struct Font_BMP;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct Font_Manager
{
  static Font * loadFont(const char *fileName, int pointSize = 10);
  static void   freeFont(Font * pf);

  static Font * getFont(const char * name);

protected:
  typedef map<string, Font*>  namelst_t;
  typedef namelst_t::iterator namelst_it;

  static namelst_t  m_namelist;
};

//--------------------------------------------------------------------------------------------

struct IFont
{
  virtual void  drawText(int x, int y, const char *text) = 0;
  virtual int   drawTextBox(const char *text, REGION r, int spacing) = 0;

  // Only works properly on a single line of text
  virtual void  getTextSize(const char *text, int &width, int &height) = 0;

  // Works for multiple-line strings, using the user-supplied spacing
  virtual void  getTextBoxSize(const char *text, int spacing, int &width, int &height) = 0;

  virtual Font_TTF * getTTF() = 0;
  virtual Font_BMP * getBMP() = 0;
};


//--------------------------------------------------------------------------------------------
struct Font : public IFont
{
  virtual Font_TTF * getTTF() { return NULL; };
  virtual Font_BMP * getBMP() { return NULL; };

  virtual ~Font() {};

  int getSize() { return size + 2; };

protected:
  int size;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define FNT_NUM_FONT_CHARACTERS 94
#define FNT_SMALL_FONT_SIZE 12
#define FNT_NORMAL_FONT_SIZE 16
#define FNT_LARGE_FONT_SIZE 20
#define FNT_MAX_FONTS 8

struct Font_TTF : public Font
{
  friend struct Font_Manager;
  virtual Font_TTF * getTTF() { return this; };

  virtual void  drawText(int x, int y, const char *text);
  virtual int   drawTextBox(const char *text, REGION r, int spacing);

  // Only works properly on a single line of text
  virtual void  getTextSize(const char *text, int &width, int &height);

  // Works for multiple-line strings, using the user-supplied spacing
  virtual void  getTextBoxSize(const char *text, int spacing, int &width, int &height);


protected:
  TTF_Font *ttfFont;

  GLuint  texture;
  GLfloat texCoords[4];

  static Font_TTF * loadFont(const char *fileName, int pointSize);
  static void       freeFont(Font_TTF *font);
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "gltexture.h"

#define NUMFONTX                        16          // Number of fonts in the bitmap
#define NUMFONTY                        6           //
#define FONT_BMP_COUNT                  (NUMFONTX*NUMFONTY)
#define FONTADD                         4           // Gap between letters
#define TABAND              0x1F                      // Tab size

struct Font_BMP : public Font
{
  friend struct Font_Manager;
  virtual Font_BMP * getBMP() { return this; };

  virtual void  drawText(int x, int y, const char *text);
  virtual int   drawTextBox(const char *text, REGION r, int spacing);

  // Only works properly on a single line of text
  virtual void  getTextSize(const char *text, int &width, int &height);

  // Works for multiple-line strings, using the user-supplied spacing
  virtual void  getTextBoxSize(const char *text, int spacing, int &width, int &height);

  void draw_one(char c, int x, int y);

protected:
  int           offset;                     // Line up fonts from top of screen
  SDL_Rect      rect[FONT_BMP_COUNT];       // The font rectangles
  int           spacing_x[0xFF];             // The spacing stuff
  int           spacing_y;                  //
  GLTexture     texture;                    // OpenGL font surface
  Uint8         ascii_map[0x0100];             // Conversion table

  static Font_BMP *loadFont(const char *fileName);
  static void      freeFont(Font_BMP *font);

  int length_of_word(const char *szText);
};