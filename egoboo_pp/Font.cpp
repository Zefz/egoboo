// Egoboo - gr_text.c
// True-type font drawing functionality.  Uses Freetype 2 & OpenGL
// to do it's business.

#include "proto.h"
#include "Font.h"
#include "graphic.h"
#include "egoboo.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <SDL.h>
#include <SDL_opengl.h>

Font_Manager::namelst_t Font_Manager::m_namelist;

//--------------------------------------------------------------------------------------------
Font_TTF* Font_TTF::loadFont(const char *fileName, int pointSize)
{
  Font_TTF *newFont;
  TTF_Font *ttfFont;

  // Make sure the TTF library was initialized
  if (!TTF_WasInit())
  {
    if (TTF_Init() != -1)
    {
      atexit(TTF_Quit);
    }
    else
    {
      printf("Font_TTF::loadFont: Could not initialize SDL_TTF!\n");
      return NULL;
    }
  }

  // Try and open the font
  ttfFont = TTF_OpenFont(fileName, pointSize);
  if (!ttfFont)
  {
    // couldn't open it, for one reason or another
    return NULL;
  }

  // Everything looks good
  newFont = new Font_TTF;
  newFont->ttfFont = ttfFont;
  newFont->texture = 0;
  newFont->size    = pointSize;

  return newFont;
}

//--------------------------------------------------------------------------------------------
void Font_TTF::freeFont(Font_TTF *font)
{
  if (font)
  {
    TTF_CloseFont(font->ttfFont);
    glDeleteTextures(1, &font->texture);
    free(font);
  }
}

//--------------------------------------------------------------------------------------------
void Font_TTF::drawText(int x, int y, const char *text)
{
  SDL_Surface *textSurf;
  SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };

  Locker_2DMode locker_2d;
  locker_2d.begin();

  {
    // Let TTF render the text
    textSurf = TTF_RenderText_Blended(this->ttfFont, text, color);

    // Does this font already have a texture?  If not, allocate it here
    if (this->texture == 0)
    {
      glGenTextures(1, &this->texture);
    }

    // Copy the surface to the texture
    if (copySurfaceToTexture(textSurf, this->texture, this->texCoords))
    {
      // And draw the darn thing
      glBegin(GL_TRIANGLE_STRIP);
      glTexCoord2f(this->texCoords[0], this->texCoords[1]);
      glVertex2i(x, y);
      glTexCoord2f(this->texCoords[2], this->texCoords[1]);
      glVertex2i(x + textSurf->w, y);
      glTexCoord2f(this->texCoords[0], this->texCoords[3]);
      glVertex2i(x, y + textSurf->h);
      glTexCoord2f(this->texCoords[2], this->texCoords[3]);
      glVertex2i(x + textSurf->w, y + textSurf->h);
      glEnd();
    }

    // Done with the surface
    SDL_FreeSurface(textSurf);
  }
}

void Font_TTF::getTextSize(const char *text, int &width, int &height)
{
  if (this)
  {
    TTF_SizeText(this->ttfFont, text, &width, &height);
  }
}

//--------------------------------------------------------------------------------------------
/** font_drawTextBox
 * Draws a text string into a box, splitting it into lines according to newlines in the string.
 * NOTE: Doesn't pay attention to the width/height arguments yet.
 *
 * font    - The font to draw with
 * text    - The text to draw
 * x       - The x position to start drawing at
 * y       - The y position to start drawing at
 * width   - Maximum width of the box (not implemented)
 * height  - Maximum height of the box (not implemented)
 * spacing - Amount of space to move down between lines. (usually close to your font size)
 */
int Font_TTF::drawTextBox(const char *text, REGION r, int spacing)
{
  int len;
  char *buffer, *line;

  // If text is empty, there's nothing to draw
  if (!text || !text[0]) return 0;

  // Split the passed in text into separate lines
  len = strlen(text);
  buffer = (char*)calloc(1, len + 1);
  strncpy(buffer, text, len);

  line = strtok(buffer, "\n\10\13");
  while (line != NULL)
  {
    drawText(r.left, r.top, line);
    r.top += spacing;
    line = strtok(NULL, "\n");
  }

  free(buffer);

  return r.top;
}

//--------------------------------------------------------------------------------------------
void Font_TTF::getTextBoxSize(const char *text, int spacing, int &width, int &height)
{
  char *buffer, *line;
  int len;
  int tmp_w, tmp_h;

  if (!this) return;
  if (!text || !text[0]) return;

  // Split the passed in text into separate lines
  len = strlen(text);
  buffer = (char*)calloc(1, len + 1);
  strncpy(buffer, text, len);

  line = strtok(buffer, "\n");
  height = 0;
  while (line != NULL)
  {
    TTF_SizeText(this->ttfFont, line, &tmp_w, &tmp_h);
    width = MAX(width, tmp_w);
    height += spacing;

    line = strtok(NULL, "\n");
  }
  free(buffer);
}


//--------------------------------------------------------------------------------------------
Font_BMP * Font_BMP::loadFont(const char* szFilename)
{
  // ZZ> This function loads the font bitmap and sets up the coordinates
  //     of each font on that bitmap...  Bitmap must have 16x6 fonts

  int cnt, i, y, xsize, ysize, xdiv, ydiv;
  int stt_x, stt_y;
  int xspacing, yspacing;
  Uint8 cTmp;
  FILE *fileread;
  char bitmapname[0x0100];
  char spacingname[0x0100];
  GLTexture TxTmp;

  strcpy(bitmapname, szFilename);
  strcat(bitmapname,".bmp");

  strcpy(spacingname, szFilename);
  strcat(spacingname,".txt");


  fileread = fopen(spacingname, "r");
  if (fileread==NULL)
    return NULL;

  if (GLTexture::INVALID == GLTexture::Load(&TxTmp, bitmapname, 0))
    return NULL;

  Font_BMP * pfnt = new Font_BMP;
  if(NULL==pfnt) return NULL;

  pfnt->texture = TxTmp;

  // Mark all as unused
  for (i=0; i < 0xFF; i++)
  {
    pfnt->ascii_map[i] = 0xFF;
    pfnt->spacing_x[i] = 0;
  }


  // Get the size of the bitmap
  xsize = GLTexture::GetImageWidth(&pfnt->texture);
  ysize = GLTexture::GetImageHeight(&pfnt->texture);
  if (xsize == 0 || ysize == 0)
  {
    GLTexture::Release(&TxTmp);
    return NULL;
  };


  // Figure out the general size of each font
  ydiv = ysize/NUMFONTY;
  xdiv = xsize/NUMFONTX;

  cnt = 0;
  y = 0;

  stt_x = 0;
  stt_y = 0;

  // Uniform font height is at the top
  yspacing = get_next_int(fileread);
  //pfnt->offset = scry - yspacing;

  cnt = 0;
  while( goto_colon_yesno(fileread) && cnt<=0xFF )
  {
    fscanf(fileread, "%c%d", &cTmp, &xspacing);
    if (pfnt->ascii_map[cTmp] == 0xFF) 
      pfnt->ascii_map[cTmp] = cnt;

    if (stt_x+xspacing+1 > 0xFF)
    {
      stt_x = 0;
      stt_y += yspacing;
    }

    pfnt->rect[cnt].x = stt_x;
    pfnt->rect[cnt].w = xspacing;
    pfnt->rect[cnt].y = stt_y;
    pfnt->rect[cnt].h = yspacing-2;
    pfnt->spacing_x[cnt] = xspacing+1;
    stt_x += xspacing+1;

    cnt++;
  }
  fclose(fileread);

  // Space between lines
  pfnt->spacing_y = yspacing;
  pfnt->size      = yspacing;

  return pfnt;
}

//--------------------------------------------------------------------------------------------
Font * Font_Manager::loadFont(const char *fileName, int pointSize)
{
  Font * retval = NULL;

  if(NULL==fileName || 0==fileName[0]) return retval;

  // check to see if we have a registered font with that name
  namelst_it fn_it = m_namelist.find( string(fileName) );
  if(fn_it != m_namelist.end()) return fn_it->second;

  // this is a new font. find the type
  char * pos = strrchr(fileName, '.');

  if(NULL == pos)
  {
    // no extension == bitmap font
    retval = Font_BMP::loadFont(fileName);
  }
  else if( strncmp(pos+1, "ttf", 3) || strncmp(pos+1, "TTF", 3)  )
  {
    // ttf == bitmap font
    retval = Font_TTF::loadFont(fileName, pointSize);
  };

  if (NULL != retval)
  {
    m_namelist[ string(fileName) ] = retval;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
void Font_Manager::freeFont(Font * pfnt)
{
  if(NULL==pfnt) return;

  //find and remove the matching element from the m_namelist
  namelst_it nam_it = m_namelist.begin();
  for( /* nothing */; nam_it != m_namelist.end(); nam_it++)
  {
    if(nam_it->second == pfnt) { m_namelist.erase(nam_it); break; }
  };

  delete pfnt;
};

//--------------------------------------------------------------------------------------------
void Font_BMP::drawText(int x, int y, const char *szText)
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer
  Uint8 cTmp = szText[0];
  int cnt = 1;

  if(NULL==szText || 0x00 == szText[0]) return;

  Locker_2DMode loc_locker_2d;
  loc_locker_2d.begin();

  texture.Bind(GL_TEXTURE_2D);

  while (cTmp != 0)
  {
    // Convert ASCII to our own little font
    if (cTmp == '~')
    {
      // Use squiggle for tab
      x = x & (~TABAND);
      x+=TABAND+1;
    }
    else if (cTmp == '\n')
    {
      break;
    }
    else
    {
      // Normal letter
      cTmp =  ascii_map[cTmp];
      draw_one(cTmp, x, y);
      x += spacing_x[cTmp];
    }
    cTmp=szText[cnt];
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
int Font_BMP::length_of_word(const char *szText)
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
      x += spacing_x[ascii_map[cTmp]];
    }
    else if (cTmp == '~')
    {
      x += TABAND+1;
    }
    cnt++;
    cTmp = szText[cnt];
  }

  while (cTmp != ' ' && cTmp != '~' && cTmp != '\n' && cTmp != 0)
  {
    x += spacing_x[ascii_map[cTmp]];
    cnt++;
    cTmp = szText[cnt];
  }
  return x;
}

//--------------------------------------------------------------------------------------------
int Font_BMP::drawTextBox(const char *szText, REGION box, int spacing)
{
  // ZZ> This function spits a line of null terminated text onto the backbuffer,
  //     wrapping over the right side and returning the new y value

  int x = box.left;
  int y = box.top;
  int maxx = box.width;
  int stt_x = x;

  int newy = y+spacing;
  Uint8 newword = true;
  int cnt = 1;

  // If text is empty, there's nothing to draw
  if (!szText || !szText[0]) return 0;

  Locker_2DMode loc_locker_2d;
  loc_locker_2d.begin();

  texture.Bind(GL_TEXTURE_2D);

  maxx += stt_x;

  char cTmp = szText[0];
  while (cTmp != 0)
  {
    // Check each new word for wrapping
    if (newword)
    {
      int end_x = x + length_of_word(szText + cnt - 1);

      newword = false;
      if (end_x > maxx)
      {
        // Wrap the end and cut off spaces and tabs
        x     = stt_x + spacing_y;
        y    += spacing;
        newy += spacing;
        while (cTmp == ' ' || cTmp == '~')
        {
          cTmp=szText[cnt];
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
        x += TABAND+1;
      }
      else if (cTmp == '\n')
      {
        x     = stt_x;
        y    += spacing;
        newy += spacing;
      }
      else
      {
        // Normal letter
        cTmp = ascii_map[cTmp];
        draw_one(cTmp, x, y);
        x += spacing_x[cTmp];
      }
      cTmp = szText[cnt];
      if (cTmp == '~' || cTmp == ' ')
      {
        newword = true;
      }
      cnt++;
    }
  }

  return newy;
}

//--------------------------------------------------------------------------------------------
void Font_BMP::getTextSize(const char *szText, int &width, int &height)
{
  // ZZ> This function returns the number of pixels the
  //     next word will take on screen in the width direction

  // Count all preceeding spaces
  width = 0;

  int cnt = 0;
  Uint8 cTmp = szText[cnt];
  while (cTmp == ' ' || cTmp == '~' || cTmp == '\n')
  {
    if (cTmp == ' ')
    {
      width += spacing_x[ascii_map[cTmp]];
    }
    else if (cTmp == '~')
    {
      width += TABAND+1;
    }
    cnt++;
    cTmp = szText[cnt];
  }

  while (cTmp != ' ' && cTmp != '~' && cTmp != '\n' && cTmp != 0)
  {
    width += spacing_x[ascii_map[cTmp]];
    cnt++;
    cTmp = szText[cnt];
  }

  height = this->spacing_y;
}

//--------------------------------------------------------------------------------------------
void Font_BMP::getTextBoxSize(const char *szText, int spacing, int &width, int &height)
{
  // ZZ> This function estimates the screen size the
  //     Text Box will occupy

  int w = 0;
  width = 0;
  height = spacing_y;

  int cnt = 0;
  Uint8 cTmp = szText[cnt];
  while (cTmp == 0x00)
  {
    switch(cTmp)
    {
      case '~':
        w += TABAND+1;
        break;

      case '\n':
        width   = MAX(width, w);
        height += spacing_y;
        w = 0;
        break;

      default:
        w += spacing_x[ascii_map[cTmp]];
        break;
    };

    cnt++;
    cTmp = szText[cnt];
  }
}

//--------------------------------------------------------------------------------------------
void Font_BMP::draw_one(char c, int x1, int y1)
{
  // ZZ> This function draws a letter or number
  // GAC> Very nasty version for starters.  Lots of room for improvement.
  GLfloat dx,dy,fx1,fx2,fy1,fy2;
  GLuint x2,y2;

  texture.Bind(GL_TEXTURE_2D);

  y1 += spacing_y;

  x2 = x1 + rect[c].w;
  y2 = y1 - rect[c].h;

  dx = 1.0/0x0100;
  dy = 1.0/0x0100;
  fx1 = rect[c].x*dx + 0.001;
  fx2 = (rect[c].x+rect[c].w)*dx - 0.001;
  fy1 = rect[c].y*dy + 0.001;
  fy2 = (rect[c].y+rect[c].h)*dy;

  glBegin(GL_QUADS);
    glTexCoord2f(fx1,fy2);   glVertex2i(x1, y1);
    glTexCoord2f(fx2,fy2);   glVertex2i(x2, y1);
    glTexCoord2f(fx2,fy1);   glVertex2i(x2, y2);
    glTexCoord2f(fx1,fy1);   glVertex2i(x1, y2);
  glEnd();
}