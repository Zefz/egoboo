/* Egoboo - input.c
 * Keyboard, mouse, and joystick handling code.
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
#include "Ui.h"

MOUSE    GMous   = {btrue,0,0,6,.5f,.5f,0,0,0,0,0,0,0};
KEYBOARD GKeyb   = {btrue, 0, 0};
JOYSTICK GJoy[2] = {{bfalse,0,0,0,0},{bfalse,0,0,0,0}};



//--------------------------------------------------------------------------------------------
void read_mouse()
{
  int b = SDL_GetMouseState(&GMous.x, &GMous.y);
  
  if(GMous.on)
  {
    SDL_GetRelativeMouseState(&GMous.dx, &GMous.dy);
  }
  else
  {
    GMous.dx = GMous.dy = 0;
  };

  GMous.button[0] = (b & SDL_BUTTON(1)) ? 1 : 0;
  GMous.button[1] = (b & SDL_BUTTON(3)) ? 1 : 0;
  GMous.button[2] = (b & SDL_BUTTON(2)) ? 1 : 0; // Middle is 2 on SDL
  GMous.button[3] = (b & SDL_BUTTON(4)) ? 1 : 0;
}

//--------------------------------------------------------------------------------------------
void read_key()
{
  sdlkeybuffer = SDL_GetKeyState(NULL);
}

//--------------------------------------------------------------------------------------------
void read_joystick()
{
  int button;
  float jx, jy;
  const float jthresh = .5;

  if((!GJoy[0].on || NULL==sdljoya) && (!GJoy[1].on || NULL==sdljoyb))
  {
    GJoy[0].x = GJoy[0].y = 0.0f;
    GJoy[0].b = 0;

    GJoy[1].x = GJoy[1].y = 0.0f;
    GJoy[1].b = 0;
    return;
  };
  
  
  SDL_JoystickUpdate();

  if (GJoy[0].on)
  {
    jx = (float)SDL_JoystickGetAxis(sdljoya, 0) / (float)(1<<15);
    jy = (float)SDL_JoystickGetAxis(sdljoya, 1) / (float)(1<<15);

    GJoy[0].x = GMous.sustain*GJoy[0].x + GMous.cover*(jx - jthresh*jx/(jthresh + fabs(jx)))*(jthresh + 1.0f);
    GJoy[0].y = GMous.sustain*GJoy[0].y + GMous.cover*(jy - jthresh*jy/(jthresh + fabs(jy)))*(jthresh + 1.0f);

    button = SDL_JoystickNumButtons(sdljoya);
    while (button >= 0)
    {
      GJoy[0].button[button] = SDL_JoystickGetButton(sdljoya, button);
      button--;
    }
  }

  if (GJoy[1].on)
  {
    jx = (float)SDL_JoystickGetAxis(sdljoyb, 0) / (float)(1<<15);
    jy = (float)SDL_JoystickGetAxis(sdljoyb, 1) / (float)(1<<15);

    GJoy[1].x = GMous.sustain*GJoy[1].x + GMous.cover*(jx - jthresh*jx/(jthresh + fabs(jx)))*(jthresh + 1.0f);
    GJoy[1].y = GMous.sustain*GJoy[1].y + GMous.cover*(jy - jthresh*jy/(jthresh + fabs(jy)))*(jthresh + 1.0f);

    button = SDL_JoystickNumButtons(sdljoyb);
    while (button >= 0)
    {
      GJoy[1].button[button] = SDL_JoystickGetButton(sdljoyb, button);
      button--;
    }
  }
}

//--------------------------------------------------------------------------------------------
void reset_press()
{
  // ZZ> This function resets key press information
  /*PORT
      int cnt;
      cnt = 0;
      while(cnt < 256)
      {
          GKeyb.press[cnt] = bfalse;
          cnt++;
      }
  */
}

//--------------------------------------------------------------------------------------------
void read_input()
{
  // ZZ> This function gets all the current player input states
  int cnt;
  SDL_Event evt;

  // Run through SDL's event loop to get info in the way that we want
  // it for the Gui code
  while (SDL_PollEvent(&evt))
  {
    ui_handleSDLEvent(&evt);

    switch (evt.type)
    {
    case SDL_MOUSEBUTTONDOWN:
      pending_click = btrue;
      break;

    case SDL_MOUSEBUTTONUP:
      pending_click = bfalse;
      break;

    }
  }

  // Get immediate mode state for the rest of the game
  read_key();
  read_mouse();
  read_joystick();


  // Set up for button masks
  GJoy[0].b = 0;
  GJoy[1].b = 0;
  GMous.b = 0;


  // Joystick mask
  cnt = 0;
  while (cnt < JOYBUTTON)
  {
    GJoy[0].b |= (GJoy[0].button[cnt] << cnt);
    GJoy[1].b |= (GJoy[1].button[cnt] << cnt);
    cnt++;
  }


  // Mouse mask
  GMous.b = (GMous.button[3] << 3) | (GMous.button[2] << 2) | (GMous.button[1] << 1) | (GMous.button[0] << 0);
}

