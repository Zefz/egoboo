
#include "Widget.h"
#include "Ui.h"
#include "graphic.h"

GLfloat tint_select[4]     = {0.0f, 0.0f, 0.9f, 0.6f};
GLfloat tint_mouseover[4]  = {0.9f, 0.0f, 0.0f, 0.6f};
GLfloat tint_none[4]       = {0.4f, 0.0f, 0.0f, 1.0f};


void Widget::AddSubWidget(Widget * sub)
{
  // append the widgets at the end so that we can add them in an "intuitive" way
  if (NULL==sub) return;

  subwidgets.push_back(sub);
}

//--------------------------------------------------------------------------------------------
// Behaviors
int Widget::Behavior(UiContext * c)
{
  int result = 0;

  // If the mouse is over the button, try and set hotness so that it can be GUI.clicked
  if (mouseInside(c))
  {
    c->sethot(this);

    if (c->mousePressed == 1)
    {
      c->setactive(this);
      result = 1;
    }
  }
  else
  {
    c->unsethot(this);

    if (c->mouseReleased == 1)
    {
      c->unsetactive(this);
    }
  }

  return result;
}

//--------------------------------------------------------------------------------------------
int Widget::mouseInside(UiContext * c)
{
  float right, bottom;
  right  = _region.left + _region.width;
  bottom = _region.top  + _region.height;

  return (_region.left <= c->mouseX) && 
         (_region.top  <= c->mouseY) && 
         (c->mouseX <= right) && (c->mouseY <= bottom);
}

Widget::Widget(const char * TaskName, UI::ID i, const StateMachine * parent) :
StateMachine(UI::getUIScheduler(), TaskName, parent), _id(i)
{
  active      = false;
  hot         = false;
  lerp        = 0.0f;
  dx          = 0.0f;

  region_on   = false;

  SetRegion( REGION(0, 0, displaySurface->w, displaySurface->h) );
};

Widget::~Widget()
{
  // we should only have gotten here if the
  // scheduler for the sub-tasks is empty
  assert(GetSubScheduler()->isEmpty());

  // also, we should NECESSARILY be in the state SM_Unknown
  assert(SM_Unknown == state);

  // If this case, all of the sub-widgets and sub-tasks are scheduled to be
  // destroyed at a later time.

  //Just clear out the list. probably not necessary
  subwidgets.clear();

}

//--------------------------------------------------------------------------------------------
// Controls

void Widget::Run(float dtime)
{
  // Do all the logic type work for the button
  result = Behavior(&GUI.GetContext());
};


//void WidgetButton::Draw(float dtime)
//{
//  int text_w, text_h;
//  int text_x, text_y;
//  Font *fnt;
//
//  // Draw any background image
//  Widget::Draw_Image();
//
//  // Draw the button part of the button
//  UI::drawButton(this);
//
//  Widget::Draw_Text(getFont());
//}

//void WidgetButton::Draw_Image(float dtime)
//{
//  // Draw the button part of the button
//  UI::drawButton(this);
//
//  // And then draw the image on top of it
//  glColor3f(1, 1, 1);
//  UI::drawImage(&background, REGION(region.left + 5, region.top + 5, region.width - 10, region.height - 10));
//}
//
//void WidgetButton::Draw_All(float dtime)
//{
//  Font *getFont();
//  int text_x, text_y;
//  int text_w, text_h;
//
//  // Draw the image part
//  glColor3f(1, 1, 1);
//  UI::drawImage(&background, REGION(region.left + 5, region.top + 5, 0, 0));
//
//  // And draw the text next to the image
//  // And then draw the text that goes on top of the button
//  getFont() = GUI.getFont();
//  if (getFont())
//  {
//    // find the region.width & region.height of the text to be drawn, so that it can be centered inside
//    // the button
//    Font::getTextSize(getFont(), text, &text_w, &text_h);
//
//    text_x = background.imgW + 10 + region.left;
//    text_y = (region.height - text_h) / 2 + region.top;
//
//    glColor3f(1, 1, 1);
//    Font::drawText(getFont(), text_x, text_y, text);
//  }
//
//}

void ContainerImage::Draw(float lerp)
{
  if( !_background.Valid() ) return;

  glColor4f(1, 1, 1, 1-lerp);
  UI::drawImage(&_background, _image_region);
};

void ContainerText::Draw(float lerp)
{
  //check for something to draw
  if (NULL==_text || _text[0]==0x00) return;

  //check for valid font
  Font * fnt = getFont();
  if (NULL==fnt) return;

  int text_w, text_h;
  int text_x, text_y;

  // find the region.width & region.height of the text to be drawn, so that it can be centered inside
  // the button
  fnt->getTextBoxSize(_text, 20, text_w, text_h);

  // Justification == centered
  text_x = (_text_region.width  - text_w) / 2 + _text_region.left;
  text_y = (_text_region.height - text_h) / 2 + _text_region.top;

  glColor3f(1, 1, 1);
  fnt->drawTextBox(_text, _text_region, 20);
}

ContainerImage::ContainerImage(GLTexture * tx)
{
  if(NULL==tx || !tx->Valid()) return;

  //copy the image info
  _background = *tx;

  //center the region by default
  _image_region = REGION(
      (displaySurface->w-_background.imgW)/2,
      (displaySurface->h-_background.imgH)/2,
      _background.imgW, _background.imgH);

}