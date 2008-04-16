// Egoboo - Menu.h
// Interface to the pre-game (stuff that happens outside of gameplay) menus

#ifndef egoboo_Menu_h
#define egoboo_Menu_h

#include "StateMachine.h"
#include "Ui.h"
#include "Widget.h"

struct Menu;

struct IMenu
{
  virtual void    run(float deltaTime) = 0;
  virtual Menu  * get_Menu() = 0;
};

struct Menu : public IMenu, public WidgetButton
{
  static Font* default_font;
  static bool  font_loaded;
  static bool  initialized;

  static bool  initialize(Font * fnt = NULL);
  static void  shutdown();

  /* Persistent menu data */

  float buttonLeft, buttonTop;
  char * text;
  int   textLeft, textTop;
  int   buttonCount;
  int    menuChoice;
  double frameDuration;

  Menu(const char * TaskName, UI::ID i, StateMachine * parent=NULL, Font * fnt=NULL) :
  WidgetButton(TaskName, i, NULL, NULL, fnt, parent)
  {
    lerp         = 0;
    menuChoice   = 0;
    buttonCount  = 0;
    text         = NULL;
    region_on    = false;
  };

  ~Menu() { }

  int  Handle_Buttons(float deltaTime);
  bool SetupAuto();
  WidgetButton * AddButtonAuto(int cnt, const char * t=NULL, GLTexture * bg=NULL);
  WidgetButton * AddButton(float l, REGION & r, const char * t=NULL, GLTexture * bg=NULL);
  void           menu_frameStep() {};

  virtual void   run(float deltaTime);
  virtual Menu * get_Menu()           { return this; };

  static void BeginGraphics();
  static void EndGraphics();

protected:
  //virtual void Begin(float deltaTime);
  virtual void Enter(float deltaTime);
  virtual void Run(float deltaTime);
  virtual void Leave(float deltaTime);
  //virtual void Finish(float deltaTime);

};

struct Menu_Main : public Menu
{
  Menu_Main(UI::ID i, StateMachine * parent, Font*fnt = NULL) :
    Menu("Main Menu", i, parent, fnt) {};

  virtual void run(float deltaTime);

protected:
  virtual void Begin(float deltaTime)  ;
  //virtual void Enter(float deltaTime)  ;
  //virtual void Run(float deltaTime)    ;
  //virtual void Leave(float deltaTime)  ;
  virtual void Finish(float deltaTime) ;
};

//int doMenu(float dt, Font * fnt = NULL);

//void menu_frameStep();

#endif // include guard
