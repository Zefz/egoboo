#pragma once

#include "egobootypedef.h"
#include "StateMachine.h"
#include "GLTexture.h"
#include "Ui.h"
#include "graphic.h"

#include <deque>
using std::deque;

struct UiContext;
struct Font;

struct Widget;
struct Menu;

extern GLfloat tint_select[4];
extern GLfloat tint_mouseover[4];
extern GLfloat tint_none[4];


struct IWidget
{
  virtual Widget  * get_Widget() = 0;
  virtual Menu    * get_Menu()   = 0;

  virtual int       mouseInside(UiContext * c) = 0;
  virtual int       Behavior(UiContext * c)    = 0;

  virtual void      Draw(float deltaTime) = 0;
  virtual void      DrawSub(float deltaTime) = 0;
  virtual void      Handle_Lerp(float deltaTime) = 0;
};

struct Widget : public IWidget, public StateMachine
{
  //list management
  typedef deque<Widget*>        widgetdeq_t;
  typedef widgetdeq_t::iterator widgetdeq_it;
  widgetdeq_t subwidgets;

  //widget states
  bool     active;
  bool     hot;
  GLfloat  tint[4];
  float    lerp;
  float    dx;
  bool     region_on;

  virtual ~Widget();

  Widget(const char * TaskName, UI::ID i, const StateMachine * parent = NULL);

  void AddSubWidget(Widget * sub);

  virtual Widget  * get_Widget() { return this; };
  virtual Menu    * get_Menu()   { return NULL; };

  virtual void Draw(float deltaTime)
  {
    Handle_Lerp(deltaTime);

    if(region_on)
    {
      if (active && hot)
        memcpy(tint,tint_select,sizeof(tint));
      else if (hot)
        memcpy(tint,tint_mouseover,sizeof(tint));
      else
        memcpy(tint,tint_none,sizeof(tint));

      UI::drawRegion(_region, tint);
    };
  };

  virtual void DrawSub(float deltaTime)
  {
    //draw all of the sub widgets
    widgetdeq_it i;
    for(i = subwidgets.begin(); i!=subwidgets.end(); i++)
    {
      if((*i)->isRunning()) (*i)->Draw(deltaTime);
    };
  };

  virtual void Handle_Lerp(float deltaTime)
    {
      lerp += deltaTime;
      if(lerp<0.0f) lerp = 0.0f;
      if(lerp>1.0f) lerp = 1.0f;
    };

  virtual int  mouseInside(UiContext * c);
  virtual int  Behavior(UiContext * c);

  void SetRegion(REGION & r) { _region = r; }

  const UI::ID getID() { return _id; }

protected:
  const UI::ID  _id;
  REGION        _region;

protected:

  StateMachine * child, * sibling;

  //virtual void run(float deltaTime);

  //virtual void Begin(float deltaTime) ;
  //virtual void Enter(float deltaTime) ;
    virtual void Run(float deltaTime)   ;
  //virtual void Leave(float deltaTime) ;
  //virtual void Finish(float deltaTime);
};


struct ContainerImage
{
  ContainerImage(GLTexture * tx = NULL);

  virtual void Draw(float lerp);

  void SetImageRegion(REGION & r)
  { _image_region = r; }

  void SetImage(GLTexture & i)
  { _background = i; }

  void ClearImage()
  { _background.textureID = GLTexture::INVALID; }

protected:
  REGION     _image_region;
  GLTexture  _background;
};

struct ContainerText
{

  ContainerText(const char * t = NULL, Font * fnt = NULL)
  {
    _text = (char *)t;
    _font = fnt;
  }

  Font * getFont()
  {
    Font * retval = _font;

    if(NULL==retval)
      retval = GUI.getFont();

    return retval;
  };

  void SetText(const char * t)   { _text = (char *) t; };
  void ClearText()               { _text = NULL; };

  void SetTextRegion(REGION & r) { _text_region = r; };

  virtual void Draw(float lerp);

protected:
  REGION  _text_region;
  char  * _text;
  Font  * _font;
};


struct WidgetImage : public Widget, public ContainerImage
{
  WidgetImage(const char * TaskName, UI::ID i, GLTexture * tx = NULL, StateMachine * parent = NULL) :
    Widget(TaskName, i, parent),
    ContainerImage(tx) {}

  void SetRegion(REGION & r)
  {
    _region = r;
    _image_region = REGION( r.left+5, r.top+5, r.width-10, r.height-10 );

    if(_image_region.width  < 0 ) _image_region.width  = 0;
    if(_image_region.height < 0 ) _image_region.height = 0;

    if(_image_region.left   > r.left+r.width) _image_region.width = 0;
    if(_image_region.height > r.top+r.height) _image_region.height = 0;
  };

  void SetImageRegion(REGION & r)
  {
    _image_region = r;
    _region = REGION( r.left-5, r.top-5, r.width+10, r.height+10 );
  };

  virtual void Draw(float deltaTime) { Widget::Draw(deltaTime); ContainerImage::Draw(lerp); };

protected:
  //virtual void run(float deltaTime);

  //virtual void Begin(float deltaTime) ;
  //virtual void Enter(float deltaTime) ;
  //virtual void Run(float deltaTime)   ;
  //virtual void Leave(float deltaTime) ;
  //virtual void Finish(float deltaTime);
};

struct WidgetTextBox : public Widget, public ContainerText
{
  WidgetTextBox(const char * TaskName, UI::ID i, const char * t = NULL, Font * fnt = NULL, StateMachine * parent = NULL) :
    Widget(TaskName, i, parent),
    ContainerText(t, fnt) { region_on = true; }

  virtual void Draw(float deltaTime) { Widget::Draw(deltaTime); ContainerText::Draw(lerp); };

protected:
  //virtual void run(float deltaTime);

  //virtual void Begin(float deltaTime) ;
  //virtual void Enter(float deltaTime) ;
  //virtual void Run(float deltaTime)   ;
  //virtual void Leave(float deltaTime) ;
  //virtual void Finish(float deltaTime);
};


struct WidgetButton : public Widget, public ContainerImage, public ContainerText
{
  WidgetButton(const char * TaskName, UI::ID i, const char * t = NULL, GLTexture * tx=NULL, Font * fnt = NULL, StateMachine * parent = NULL) :
    Widget(TaskName, i, parent),
    ContainerImage(tx),
    ContainerText(t, fnt) { region_on = true; };

  virtual void Draw(float deltaTime)
  {
    float x_save1 = _region.left;
    _region.left -= dx  * lerp;

    float x_save2 = _text_region.left;
    _text_region.left -= dx  * lerp;

    float x_save3 = _image_region.left;
    _image_region.left -= dx  * lerp;

    Widget::Draw(deltaTime);
    ContainerImage::Draw(lerp);
    ContainerText::Draw(lerp);

    DrawSub(deltaTime);

    _region.left       = x_save1;
    _text_region.left  = x_save2;
    _image_region.left = x_save3;
  };


protected:
  //virtual void run(float deltaTime);

  //virtual void Begin(float deltaTime) ;
  //virtual void Enter(float deltaTime) ;
  //virtual void Run(float deltaTime)   ;
  //virtual void Leave(float deltaTime) ;
  //virtual void Finish(float deltaTime);

};