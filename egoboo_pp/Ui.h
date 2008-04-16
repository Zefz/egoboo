// Egoboo - Ui.h
// The immediate-mode gui system again, refactored a bit.  I'm removing the ability
// to have more than one UI context, as there isn't any point for the game to have it,
// and instead offering a global interface to one ui.  Switching between fonts is
// also now an option.

#ifndef egoboo_ui_h
#define egoboo_ui_h

#include "Font.h"
#include "GLTexture.h"
#include "StateMachine.h"
#include "JF_Scheduler.h"
#include "JF_Task.h"
#include <SDL.h>

struct Widget;
struct WidgetButton;
struct UI;

struct UiContext : public GID<UI>
{
  typedef GID<UI> GID_Server;
  typedef GID_Server::ID ID;

  // Tracking control focus stuff
  ID active;
  ID hot;

  // Basic mouse state
  int mouseX, mouseY;
  int mouseReleased;
  int mousePressed;

  Font *defaultFont;
  Font *activeFont;

  void sethot(Widget * w);
  void unsethot(Widget * w);

  void setactive(Widget * w);
  void unsetactive(Widget * w);

  UiContext & GetContext() { return *this; }

  UiContext()
  {
    defaultFont = NULL;
    activeFont  = NULL;
    active      = GID_Server::BAD_ID;
    hot         = GID_Server::BAD_ID;
  };
};

struct RTS_Info;
struct Widget;

struct UI : public UiContext, public StateMachine
{
  typedef GID_Server::ID ID;

  static const ID Nothing = GID_Server::BAD_ID;

  bool       pending_click;
  bool       wheel_event;

  static Font * bmp_fnt;

  // Initialize or shut down the ui system
  static int  initialize(UI * ctxt, const char *default_font, int default_font_size);
  static void shutdown(UI * ctxt);

  // Pass input data from SDL to the ui
  void handleSDLEvent(SDL_Event *evt);

  // Allow the ui to do work that needs to be done before and after each frame
  static void beginFrame(float deltaTime);
  static void endFrame();

  // Utility functions
  Font* getFont();

  // UI functions
  void do_cursor();

  /*****************************************************************************/
  // Most users won't need to worry about stuff below here; it's mostly for
  // implementing new controls.
  /*****************************************************************************/

  // Behaviors
  int  buttonBehavior(UI::ID id, int x, int y, int width, int height);

  // Drawing
  static void drawRegion(REGION & r, GLfloat col[] );
  static void drawImage(GLTexture *img, REGION r);
  void drawTextBox(const char *text, REGION r, int spacing);

  UI() : StateMachine(&s_scheduler, "UI loop")
  {
    pending_click = false;
  }

  static const JF::Scheduler * getUIScheduler()           { return &s_scheduler; }
  static const addTask(JF::Task * tsk, JF::Task * parent) { s_scheduler.addTask(tsk, parent); }

protected:

  static JF::Scheduler s_scheduler;

  // UI functions
  void do_cursor_rts(UiContext & c, RTS_Info & r);
  void do_cursor_2d(UiContext & c);

  virtual void Begin(float deltaTime);
  //virtual void Enter(float deltaTime)  { result =  0; state = SM_Running; };
  virtual void Run(float deltaTime);
  //virtual void Leave(float deltaTime)  { result =  0; state = SM_Finish;  };
  virtual void Finish(float deltaTime);
};

extern UI GUI;

#endif // include guard
