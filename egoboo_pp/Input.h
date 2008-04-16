#pragma once

#include "StateMachine.h"

//----------------------------------------------------------------------------

struct IDevice
{
  virtual void read() = 0;
};

struct Device : public IDevice
{
  bool    on;

  Latch   latch, latch_old;

  float dx,dy;

  Uint32  player;

  float   sustain ;          // Falloff rate for old movement
  float   cover;             // For falloff

  virtual void read()
  {
    latch.x = latch.x*cover + latch_old.x*sustain;
    latch.y = latch.y*cover + latch_old.y*sustain;

    dx = latch.x - latch_old.x;
    dy = latch.y - latch_old.y;

    latch_old = latch;
  };

  Device()
  {
    on         = false;

    latch_old.clear();
    latch.clear();

    player     = 0;

    sustain    = .50;
    cover      = .50;
  };

};

//----------------------------------------------------------------------------
struct Device_Mouse : public Device
{
  bool    on;                // Is the mouse alive?
  float   sense;             // Sensitivity threshold

  Sint32  w;                 // Device_Mouse wheel movement counter
  Uint8   button[4];         // Device_Mouse button states

  virtual void read();

  Device_Mouse()
  {
    on         = true;

    sense      = 6;
    w          = 0;

    player     = 0;
  };
};

extern Device_Mouse GMous;

//----------------------------------------------------------------------------
struct Device_Keyboard : public Device
{
  bool    on;                  // Is the keyboard alive?
  int     player;

  Uint8 * buffer;

  virtual void read();
  void reset_press();

  bool pressed(Uint32 k) { if(NULL==buffer) return false; else return (0!=buffer[k]); }

  void press(Uint32 k)  { if(NULL!=buffer) buffer[k] = 1; }

  Device_Keyboard()
  {
    on     = true;
    player = 0;
  }
};

extern Device_Keyboard GKeyb;

//----------------------------------------------------------------------------

#define JOYBUTTON           8                       // Maximum number of joystick buttons

struct Device_Joystick : public Device
{
  Uint8   button[JOYBUTTON];      //

  // SDL specific declarations
  SDL_Joystick * sdlptr;

  virtual void read();

  Device_Joystick()
  {
    sdlptr = NULL;
  }

};

#define MAXJOY 2
extern Device_Joystick GJoy[MAXJOY];

//----------------------------------------------------------------------------
struct Machine_Input : public StateMachine
{
  //virtual void run(float deltaTime);

  int   joy_count;

  Machine_Input(const JF::Scheduler * s) :
    StateMachine(s, "Device Handling Task") { joy_count = 0; };

  Machine_Input(const StateMachine * parent) :
    StateMachine("Device Handling Task", parent)  { joy_count = 0; };

protected:
  virtual void Begin(float deltaTime);
  //virtual void Enter(float deltaTime);
  virtual void Run(float deltaTime);
  //virtual void Leave(float deltaTime);
  //virtual void Finish(float deltaTime);
};