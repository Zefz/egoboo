// input.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "egoboo.h"
#include "input.h"
#include "Ui.h"

Device_Mouse    GMous;
Device_Keyboard GKeyb;
Device_Joystick GJoy[MAXJOY];

//--------------------------------------------------------------------------------------------
void Device_Mouse::read()
{
  int lx, ly, lb;

  if(!on)
  {
    GMous.latch.button = 0;
    return;
  }

  lb = SDL_GetMouseState(&lx,&ly);

  latch.x = lx; // GMous.x and GMous.y are the wrong type to use in above call
  latch.y = ly;
  button[0] = (latch.button&SDL_BUTTON(1)) ? 1:0;
  button[1] = (latch.button&SDL_BUTTON(3)) ? 1:0;
  button[2] = (latch.button&SDL_BUTTON(2)) ? 1:0; // Middle is 2 on SDL
  button[3] = (latch.button&SDL_BUTTON(4)) ? 1:0;

  // Device_Mouse mask
  latch.button = (GMous.button[3]<<3)|(GMous.button[2]<<2)|(GMous.button[1]<<1)|(GMous.button[0]<<0);

  Device::read();

  // make a "dead zone" for mouse movement
  if(dx!=0 || dy!=0)
  {
    float tmp = dx*dx + dy*dy;
    float scale = tmp/(sense*sense + tmp);
    dx *= scale;
    dy *= scale;
  };

}

//--------------------------------------------------------------------------------------------
void Device_Keyboard::read()
{
  GKeyb.buffer = SDL_GetKeyState(NULL);
  GKeyb.latch.button   = 0;

  // !!!! NOT IDEAL !!!!
  latch.x = (CtrlList.key_is_pressed(KEY_RIGHT) - CtrlList.key_is_pressed(KEY_LEFT));
  latch.y = (CtrlList.key_is_pressed(KEY_DOWN ) - CtrlList.key_is_pressed(KEY_UP  ));

  Device::read();
}

//--------------------------------------------------------------------------------------------
void Device_Joystick::read()
{
  if (!on || NULL==sdlptr) { latch.button = 0; return; }

  latch.x = SDL_JoystickGetAxis(sdlptr,0) + 750;
  latch.x *= latch.x*latch.x/(750*750 + latch.x*latch.x);
  latch.x /= float(0x8000);

  latch.y = SDL_JoystickGetAxis(sdlptr,1);
  latch.y *= latch.y*latch.y/(750*750 + latch.y*latch.y);
  latch.y /= float(0x8000);

  int button_cnt = SDL_JoystickNumButtons(sdlptr);
  for(int cnt=0; cnt<button_cnt && cnt<JOYBUTTON; cnt++)
  {
    button[cnt] = SDL_JoystickGetButton(sdlptr, cnt);
  }

  latch.button = 0;
  for (cnt = 0; cnt < JOYBUTTON; cnt++)
  {
    latch.button |= (button[cnt]<<cnt);
  }


  Device::read();
}

//--------------------------------------------------------------------------------------------
void Device_Keyboard::reset_press()
{
  // ZZ> This function resets key press information
  /*PORT
      int cnt;
      cnt = 0;
      while(cnt < 0x0100)
      {
          GKeyb.press[cnt] = false;
          cnt++;
      }
  */
}

//--------------------------------------------------------------------------------------------
void Machine_Input::Begin(float dt)
{
  //open the joysticks
  joy_count = SDL_NumJoysticks();

  for(int i=0; i<2 && i<joy_count; i++)
  {
    GJoy[i].sdlptr = SDL_JoystickOpen(i);
    GJoy[i].on     = (NULL!=GJoy[i].sdlptr);
  };

  state = SM_Entering;
}

//--------------------------------------------------------------------------------------------
void Machine_Input::Run(float dt)
{
  // ZZ> This function gets all the current player input states
  SDL_Event evt;

  // Run through SDL's event loop to get info in the way that we want
  // it for the Gui code
  while (SDL_PollEvent(&evt))
  {
    GUI.handleSDLEvent(&evt);

    switch (evt.type)
    {
      case SDL_MOUSEBUTTONDOWN:
        GUI.pending_click = true;
        switch(evt.button.button)
        {
          case SDL_BUTTON_LEFT:
          case SDL_BUTTON_MIDDLE:
          case SDL_BUTTON_RIGHT:
            GUI.pending_click = true;
            break;
          case 4: GMous.w--; GUI.wheel_event = true; break;
          case 5: GMous.w++; GUI.wheel_event = true; break;
        };

      case SDL_MOUSEBUTTONUP:
        GUI.pending_click = false;
        break;
    }
  }

  // Get immediate mode state for the rest of the game
  GMous.read();
  GKeyb.read();
  for(int i=0; i<joy_count && i<MAXJOY; i++)
    GJoy[i].read();
}
