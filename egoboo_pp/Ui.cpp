// Egoboo - Ui.c

#pragma once

#include "Ui.h"
#include "Widget.h"
#include "Graphic.h"
#include "Input.h"
#include "egoboo.h"
#include <string.h>
#include <SDL_opengl.h>
#include <assert.h>

UI GUI;
Font * UI::bmp_fnt = NULL;
JF::Scheduler UI::s_scheduler;

//Font * ContainerText::_default_font = NULL;

//--------------------------------------------------------------------------------------------
// Core functions
int UI::initialize(UI * ctxt, const char *default_font, int default_font_size)
{
  memset(&ctxt->GetContext(), 0, sizeof(UiContext));
  ctxt->active = ctxt->hot = UI::Nothing;

  ctxt->defaultFont = Font_Manager::loadFont(default_font, default_font_size);

  return 1;
}

void UI::shutdown(UI * ctxt)
{
  Font_Manager::freeFont(ctxt->defaultFont);
  memset(&ctxt->GetContext(), 0, sizeof(UiContext));
}

void UI::handleSDLEvent(SDL_Event *evt)
{
  if (evt)
  {
    switch (evt->type)
    {
      case SDL_MOUSEBUTTONDOWN:
        if(evt->button.which == 0)
        {
          mouseReleased = 0;
          mousePressed = 1;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if(evt->button.which == 0)
        {
          mousePressed = 0;
          mouseReleased = 1;
        }
        break;

      case SDL_MOUSEMOTION:
        mouseX = evt->motion.x;
        mouseY = evt->motion.y;

        break;
    }
  }
}

void UI::beginFrame(float deltaTime)
{
  SDL_Surface *screen = SDL_GetVideoSurface();

  EnableTexturing();
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);


  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport(0, 0, screen->w, screen->h);

  // Set up an ortho projection for the gui to use.  Controls are free to modify this
  // later, but most of them will need this, so it's done by default at the beginning
  // of a frame
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, screen->w, screen->h, 0, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // hotness gets reset at the start of each frame
  GUI.hot = UI::Nothing;
}

void UI::endFrame()
{
  // Restore the OpenGL matrices to what they were
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Re-enable any states disabled by gui_beginFrame
  glPopAttrib();

  // Clear input states at the end of the frame
  GUI.mousePressed = GUI.mouseReleased = 0;
}

//--------------------------------------------------------------------------------------------
// Utility functions

void UiContext::sethot(Widget * w)
{
  assert(NULL!=w);

  hot = w->getID();
  w->hot = true;
}

void UiContext::unsethot(Widget * w)
{
  assert(NULL!=w);

  if (active == w->getID())
    hot = UI::Nothing;

  w->hot = false;
}

void UiContext::setactive(Widget * w)
{
  assert(NULL!=w);

  if (hot == w->getID() || active == UI::Nothing)
  {
    // Only allow hotness to be set if this control, or no control is active
    active = w->getID();
    w->active = true;
  }
}

void UiContext::unsetactive(Widget * w)
{
  assert(NULL!=w);

  if ( active == w->getID() )
  {
    // Only allow hotness to be set if this control, or no control is active
    active = UI::Nothing;
  }

  w->hot = false;
}

Font* UI::getFont()
{
  return (GUI.activeFont != NULL) ? GUI.activeFont : GUI.defaultFont;
}

//--------------------------------------------------------------------------------------------
// Drawing
void UI::drawRegion(REGION & r, GLfloat col[])
{
  // Draw the button
  DisableTexturing();
  glBegin(GL_QUADS);

    glColor4fv(col);
    glVertex2i(r.left, r.top);
    glVertex2i(r.left, r.top + r.height);
    glVertex2i(r.left + r.width, r.top + r.height);
    glVertex2i(r.left + r.width, r.top);

  glEnd();
  EnableTexturing();
}

void UI::drawImage(GLTexture *img, REGION r)
{
  int w, h;
  float x1, y1;

  if (NULL==img) return;

  if (r.width == 0 || r.height == 0)
  {
    w = img->imgW;
    h = img->imgH;
  }
  else
  {
    w = r.width;
    h = r.height;
  }

  x1 = (float)img->imgW / img->txW;
  y1 = (float)img->imgH / img->txH;

  // Draw the image
  img->Bind(GL_TEXTURE_2D);
  glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0, 0);  glVertex2i(r.left, r.top);
    glTexCoord2f(x1, 0); glVertex2i(r.left + w, r.top);
    glTexCoord2f(0, y1); glVertex2i(r.left, r.top + h);
    glTexCoord2f(x1, y1); glVertex2i(r.left + w, r.top + h);
  glEnd();

}

/** UI::drawTextBox
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
void UI::drawTextBox(const char *text, REGION r, int spacing)
{
  Font *font = getFont();
  if(NULL!=font)
    font->drawTextBox(text, r, spacing);
}

//--------------------------------------------------------------------------------------------
void UI::do_cursor_rts(UiContext & c, RTS_Info & r)
{
  //    // This function implements the RTS mouse cursor
  //    int stt_x, stt_y, end_x, end_y, target, leader;
  //    Sint16 sound;
  //
  //    if(GMous.button[1] == 0)
  //    {
  //        c.mouseX+=GMous.x;
  //        c.mouseY+=GMous.y;
  //    }
  //    if(c.mouseX < 6)  c.mouseX = 6;  if (c.mouseX > scrx-16)  c.mouseX = scrx-16;
  //    if(c.mouseY < 8)  c.mouseY = 8;  if (c.mouseY > scry-24)  c.mouseY = scry-24;
  //    move_rtsxy();
  //    if(GMous.button[0])
  //    {
  //        // Moving the end select point
  //        c.mousePressed = true;
  //        r.end_x = c.mouseX+5;
  //        r.end_y = c.mouseY+7;
  //
  //        // Draw the selection rectangle
  //        if(!allselect)
  //        {
  //            stt_x = r.stt_x;  end_x = r.end_x;  if(stt_x > end_x)  {  stt_x = r.end_x;  end_x = r.stt_x; }
  //            stt_y = r.stt_y;  end_y = r.end_y;  if(stt_y > end_y)  {  stt_y = r.end_y;  end_y = r.stt_y; }
  //            draw_trim_box(stt_x, stt_y, end_x, end_y);
  //        }
  //    }
  //    else
  //    {
  //        if(c.mousePressed)
  //        {
  //            // See if we selected anyone
  //            if((ABS(r.stt_x - r.end_x) + ABS(r.stt_y - r.end_y)) > 10 && !allselect)
  //            {
  //                // We drew a box alright
  //                stt_x = r.stt_x;  end_x = r.end_x;  if(stt_x > end_x)  {  stt_x = r.end_x;  end_x = r.stt_x; }
  //                stt_y = r.stt_y;  end_y = r.end_y;  if(stt_y > end_y)  {  stt_y = r.end_y;  end_y = r.stt_y; }
  //                build_select(stt_x, stt_y, end_x, end_y, r.team_local);
  //            }
  //            else
  //            {
  //                // We want to issue an order
  //                if(r.select_count > 0)
  //                {
  //                    leader = r.select[0];
  //                    stt_x = r.stt_x-20;  end_x = r.stt_x+20;
  //                    stt_y = r.stt_y-20;  end_y = r.stt_y+20;
  //                    target = build_select_target(stt_x, stt_y, end_x, end_y, r.team_local);
  //                    if( INVALID_CHR(target) )
  //                    {
  //                        // No target...
  //                        if(GKeyb.pressed(SDLK_LSHIFT) || GKeyb.pressed(SDLK_RSHIFT))
  //                        {
  //                            send_rts_order(r.x, r.y, RTSTERRAIN, target);
  //                            sound = ChrList[leader].wavespeech[SPEECH_TERRAIN];
  //                        }
  //                        else
  //                        {
  //                            send_rts_order(r.x, r.y, RTSMOVE, target);
  //                            sound = wldframe&1;  // Move or MoveAlt
  //                            sound = ChrList[leader].wavespeech[sound];
  //                        }
  //                    }
  //                    else
  //                    {
  //                        if(TeamList[r.team_local].hatesteam[ChrList[target].team])
  //                        {
  //                            // Target is an enemy, so issue an attack order
  //                            send_rts_order(r.x, r.y, RTSATTACK, target);
  //                            sound = ChrList[leader].wavespeech[SPEECH_ATTACK];
  //                        }
  //                        else
  //                        {
  //                            // Target is a friend, so issue an assist order
  //                            send_rts_order(r.x, r.y, RTSASSIST, target);
  //                            sound = ChrList[leader].wavespeech[SPEECH_ASSIST];
  //                        }
  //                    }
  //                    // Do unit speech at 11025 KHz
  //                    if(sound >= 0 && sound < MAXWAVE)
  //                    {
  ////REMOVE?      channel = Mix_PlayChannel(-1, ChrList[leader].getCap().waveindex[sound], 0);
  ////REMOVE?                        Mix_SetPosition(channel, 0, 0);
  //                        //WRONG FUNCTION. IF YOU WANT THIS TO WORK RIGHT, CONVERT TO //play_sound
  //                    }
  //                }
  //            }
  //            c.mousePressed = false;
  //        }
  //
  //        // Moving the select point
  //        r.stt_x = c.mouseX+5;
  //        r.stt_y = c.mouseY+7;
  //        r.end_x = c.mouseX+5;
  //        r.end_y = c.mouseY+7;
  //    }
  //
  //    // GAC - Don't forget to BeginText() and EndText();
  //    BeginText();
  //    draw_one_font(11, c.mouseX-5, c.mouseY-7);
  //    EndText ();
}

//--------------------------------------------------------------------------------------------
void UI::do_cursor_2d(UiContext & c)
{
  c.mouseX=GMous.latch.x;  if (c.mouseX < 6)  c.mouseX = 6;  if (c.mouseX > scrx-16)  c.mouseX = scrx-16;
  c.mouseY=GMous.latch.y;  if (c.mouseY < 8)  c.mouseY = 8;  if (c.mouseY > scry-24)  c.mouseY = scry-24;
  mouseReleased = false;

  if (GMous.button[0] && !mousePressed)
  {
    mouseReleased = true;
  }
  mousePressed = (0 != GMous.button[0]);

  if(NULL==UI::bmp_fnt) return;

  Font_BMP * pfnt = UI::bmp_fnt->getBMP();
  if(NULL==pfnt) return;

  Locker_2DMode loc_locker_2d;
  loc_locker_2d.begin();

  //bfnt->draw_one(11, c.mouseX-5, c.mouseY-7);
  pfnt->draw_one(95, c.mouseX, c.mouseY);

  flip_pages();
}

void UI::do_cursor()
{
  // This function implements a mouse cursor

  if(!menuactive) return;

  if (GRTS.on)
  {
    do_cursor_rts(GetContext(), GRTS);
  }
  else
  {
    do_cursor_2d(GetContext());
  };

}

void UI::Begin(float deltaTime)
{
  //wait to do anything until sdl has begun
  if(video_initialized)
  {
    // prepare the UI for business
    UI::bmp_fnt = Font_Manager::loadFont("basicdat/font");
    load_all_titleimages();

    StateMachine::Begin(deltaTime);
  };
};

void UI::Run(float deltaTime)
{
  do_cursor();
};

void UI::Finish(float deltaTime)
{
  for (int cnt = 0; cnt < MAXMODULE; cnt++)
    GLTexture::Release( &TxTitleImage[cnt] );

  Font_Manager::freeFont(UI::bmp_fnt);
  StateMachine::Finish(deltaTime);
};