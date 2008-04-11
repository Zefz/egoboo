/* Egoboo - Ui.c
 * A basic library for implementing user interfaces, based off of Casey Muratori's
 * IMGUI.  (https://mollyrocket.com/forums/viewtopic.php?t=134)
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

#include "Ui.h"
#include "Log.h"
#include "egoboo.h"
#include <string.h>
#include <SDL_opengl.h>
#include <assert.h>

struct UiContext
{
  // Tracking control focus stuff
  UI_ID active;
  UI_ID hot;

  // Basic mouse state
  int mouseX, mouseY;
  int mouseReleased;
  int mousePressed;

  Font *defaultFont;
  Font *activeFont;
};

static struct UiContext ui_context;

//--------------------------------------------------------------------------------------------
// Core functions
int ui_initialize(const char *default_font, int default_font_size)
{
  memset(&ui_context, 0, sizeof(ui_context));
  ui_context.active = ui_context.hot = UI_Nothing;

  ui_context.defaultFont = fnt_loadFont(default_font, default_font_size);
  return 1;
}

void ui_shutdown()
{
  if (ui_context.defaultFont)
  {
    fnt_freeFont(ui_context.defaultFont);
  }

  memset(&ui_context, 0, sizeof(ui_context));
}

void ui_handleSDLEvent(SDL_Event *evt)
{
  if (evt)
  {
    switch (evt->type)
    {
    case SDL_MOUSEBUTTONDOWN:
      ui_context.mouseReleased = 0;
      ui_context.mousePressed = 1;

      break;

    case SDL_MOUSEBUTTONUP:
      ui_context.mousePressed = 0;
      ui_context.mouseReleased = 1;

      break;

    case SDL_MOUSEMOTION:
      ui_context.mouseX = evt->motion.x;
      ui_context.mouseY = evt->motion.y;

      break;
    }
  }
}

static ui_begin_level = 0;
bool_t ui_frame_enabled = bfalse;
void ui_beginFrame(float deltaTime)
{
  SDL_Surface *screen;

  screen = SDL_GetVideoSurface();

  ATTRIB_GUARD_OPEN(ui_begin_level);
  ATTRIB_PUSH("ui_beginFrame", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT | GL_VIEWPORT_BIT | GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);

  glShadeModel(GL_FLAT);                                    /* GL_LIGHTING_BIT */

  glDepthMask(GL_FALSE);                                    /* GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT */
  glDisable(GL_DEPTH_TEST);                                 /* GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT */
  glDisable(GL_CULL_FACE);                                  /* GL_ENABLE_BIT|GL_POLYGON_BIT */
  glEnable(GL_TEXTURE_2D);                                  /* GL_ENABLE_BIT|GL_TEXTURE_BIT */

  //glEnable(GL_ALPHA_TEST);                                  /* GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT */
  //glAlphaFunc(GL_GREATER, 0);                               /* GL_COLOR_BUFFER_BIT */

  glEnable(GL_BLEND);                                       /* GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT */
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);        /* GL_COLOR_BUFFER_BIT */

  glViewport(0, 0, screen->w, screen->h);                   /* GL_VIEWPORT_BIT */

  // Set up an ortho projection for the gui to use.  Controls are free to modify this
  // later, but most of them will need this, so it's done by default at the beginning
  // of a frame
  glMatrixMode(GL_PROJECTION);                              /* GL_TRANSFORM_BIT */
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, screen->w, screen->h, 0, -1, 1);

  glMatrixMode(GL_MODELVIEW);                               /* GL_TRANSFORM_BIT */
  glPushMatrix();
  glLoadIdentity();

  // hotness gets reset at the start of each frame
  ui_context.hot = UI_Nothing;

  ui_frame_enabled = btrue;
}

void ui_endFrame()
{
  GLint ui_end_level;

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  // Re-enable any states disabled by gui_beginFrame
  ATTRIB_POP("ui_endFrame"); /* GL_ENABLE_BIT|GL_TRANSFORM_BIT|GL_VIEWPORT_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_POLYGON_BIT|GL_TEXTURE_BIT */
  ATTRIB_GUARD_CLOSE(ui_begin_level, ui_end_level);

  // Clear input states at the end of the frame
  ui_context.mousePressed = ui_context.mouseReleased = 0;

  ui_frame_enabled = bfalse;

  ui_DoCursor();
}

//--------------------------------------------------------------------------------------------
// Utility functions
int ui_mouseInside(int x, int y, int width, int height)
{
  int right, bottom;
  right = x + width;
  bottom = y + height;

  if (x <= ui_context.mouseX && y <= ui_context.mouseY && ui_context.mouseX <= right && ui_context.mouseY <= bottom)
  {
    return 1;
  }

  return 0;
}

void ui_setactive(UI_ID id)
{
  ui_context.active = id;
}

void ui_sethot(UI_ID id)
{
  // Only allow hotness to be set if this control, or no control is active
  if (ui_context.active == id || ui_context.active == UI_Nothing)
  {
    ui_context.hot = id;
  }
}

Font* ui_getFont()
{
  return (ui_context.activeFont != NULL) ? ui_context.activeFont : ui_context.defaultFont;
}

//--------------------------------------------------------------------------------------------
// Behaviors
int ui_buttonBehavior(UI_ID id, int x, int y, int width, int height)
{
  int result = 0;

  // If the mouse is over the button, try and set hotness so that it can be clicked
  if (ui_mouseInside(x, y, width, height))
  {
    ui_sethot(id);
  }

  // Check to see if the button gets clicked on
  if (ui_context.active == id)
  {
    if (ui_context.mouseReleased == 1)
    {
      if (ui_context.hot == id) result = 1;

      ui_setactive(UI_Nothing);
    }
  }
  else if (ui_context.hot == id)
  {
    if (ui_context.mousePressed == 1)
    {
      ui_setactive(id);
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------
// Drawing
void ui_drawButton(UI_ID id, int x, int y, int width, int height)
{
  if (!ui_frame_enabled) return;

  // Draw the button
  ATTRIB_PUSH("ui_drawButton", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
  {
    glDisable(GL_TEXTURE_2D);                                  /* GL_ENABLE_BIT|GL_TEXTURE_BIT */

    glBegin(GL_QUADS);

    if (ui_context.active != UI_Nothing && ui_context.active == id && ui_context.hot == id)
      glColor4f(0, 0, 0.9f, 0.6f);
    else if (ui_context.hot != UI_Nothing && ui_context.hot == id)
      glColor4f(0.54f, 0, 0, 1.0f);
    else
      //glColor4f(0.6f, 0, 0, 0.6f);
      glColor4f(0.66f, 0, 0, 0.6f);

    glVertex2i(x, y);
    glVertex2i(x, y + height);
    glVertex2i(x + width, y + height);
    glVertex2i(x + width, y);
    glEnd();
  }
  ATTRIB_POP("ui_drawButton");
}

void ui_drawImage(UI_ID id, GLTexture *img, int x, int y, int width, int height)
{
  int w, h;
  float x1, y1;

  if (NULL == img || !ui_frame_enabled) return;

  ATTRIB_PUSH("ui_drawImage", GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
  {

    if (width == 0 || height == 0)
    {
      w = GLTexture_GetImageWidth(img);
      h = GLTexture_GetImageHeight(img);
    }
    else
    {
      w = width;
      h = height;
    }

    x1 = (GLfloat)GLTexture_GetImageWidth(img)  / (GLfloat)GLTexture_GetTextureWidth(img);
    y1 = (GLfloat)GLTexture_GetImageHeight(img) / (GLfloat)GLTexture_GetTextureHeight(img);

    // Draw the image
    GLTexture_Bind(img, TX_UNFILTERED);
    glColor4f(1, 1, 1, GLTexture_GetAlpha(img));

    glBegin(GL_QUADS);
    glTexCoord2f(0,  0); glVertex2i(x,     y);
    glTexCoord2f(0, y1); glVertex2i(x,     y + h);
    glTexCoord2f(x1, y1); glVertex2i(x + w, y + h);
    glTexCoord2f(x1,  0); glVertex2i(x + w, y);
    glEnd();

    //glBegin(GL_TRIANGLE_STRIP);
    // glTexCoord2f(0, 0);  glVertex2i(x, y);
    // glTexCoord2f(x1, 0); glVertex2i(x + w, y);
    // glTexCoord2f(0, y1); glVertex2i(x, y + h);
    // glTexCoord2f(x1, y1); glVertex2i(x + w, y + h);
    //glEnd();
  };
  ATTRIB_POP("ui_drawImage");
}

/** ui_drawTextBox
 * Draws a text string into a box, splitting it into lines according to newlines in the string.
 * NOTE: Doesn't pay attention to the width/height arguments yet.
 *
 * text    - The text to draw
 * x       - The x position to start drawing at
 * y       - The y position to start drawing at
 * width   - Maximum width of the box (not implemented)
 * height  - Maximum height of the box (not implemented)
 * spacing - Amount of space to move down between lines. (usually close to your font size)
 */
void ui_drawTextBox(const char *text, int x, int y, int width, int height, int spacing)
{
  Font *font = ui_getFont();
  fnt_drawTextBox(font, text, x, y, width, height, spacing);
}

//--------------------------------------------------------------------------------------------
// Controls
int ui_doButton(UI_ID id, const char *text, int x, int y, int width, int height)
{
  int result;
  int text_w, text_h;
  int text_x, text_y;
  Font *font;

  // Do all the logic type work for the button
  result = ui_buttonBehavior(id, x, y, width, height);

  // Draw the button part of the button
  ui_drawButton(id, x, y, width, height);

  // And then draw the text that goes on top of the button
  font = ui_getFont();
  if (font)
  {
    // find the width & height of the text to be drawn, so that it can be centered inside
    // the button
    fnt_getTextSize(font, text, &text_w, &text_h);

    text_x = (width - text_w) / 2 + x;
    text_y = (height - text_h) / 2 + y;

    glColor3f(1, 1, 1);
    fnt_drawText(font, text_x, text_y, text);
  }

  return result;
}

int ui_doImageButton(UI_ID id, GLTexture *img, int x, int y, int width, int height)
{
  int result;

  // Do all the logic type work for the button
  result = ui_buttonBehavior(id, x, y, width, height);

  // Draw the button part of the button
  ui_drawButton(id, x, y, width, height);

  // And then draw the image on top of it
  glColor3f(1, 1, 1);
  ui_drawImage(id, img, x + 5, y + 5, width - 10, height - 10);

  return result;
}

int ui_doImageButtonWithText(UI_ID id, GLTexture *img, const char *text, int x, int y, int width, int height)
{
  int result;
  Font *font;
  int text_x, text_y;
  int text_w, text_h;

  // Do all the logic type work for the button
  result = ui_buttonBehavior(id, x, y, width, height);

  // Draw the button part of the button
  ui_drawButton(id, x, y, width, height);

  // Draw the image part
  glColor3f(1, 1, 1);
  ui_drawImage(id, img, x + 5, y + 5, 0, 0);

  // And draw the text next to the image
  // And then draw the text that goes on top of the button
  font = ui_getFont();
  if (font)
  {
    // find the width & height of the text to be drawn, so that it can be centered inside
    // the button
    fnt_getTextSize(font, text, &text_w, &text_h);

    text_x = GLTexture_GetImageWidth(img) + 10 + x;
    text_y = (height - text_h) / 2 + y;

    glColor3f(1, 1, 1);
    fnt_drawText(font, text_x, text_y, text);
  }

  return result;
}


void ui_DoCursor()
{
  // BB > Use the "normal" Egoboo cursor drawing routines to handle the cursor
  //      Turn off the mouse as a Game Controller while the menu is on.
  // !!!! This is a pretty ugly patch !!!!
  
  // must use Begin2DMode() and BeginText() to get OpenGL in the right state
  read_mouse();
  Begin2DMode();
  BeginText();
  draw_one_font(95, GMous.x - 5, GMous.y - 7);
  EndText();
  End2DMode();
};
