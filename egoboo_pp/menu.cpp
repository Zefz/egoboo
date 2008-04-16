// menu.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "egoboo.h"
#include "Ui.h"
#include "Menu.h"
#include "Log.h"
#include "Font.h"
#include "JF_Clock.h"
#include <stdlib.h>

// TEMPORARY!
#define NET_DONE_SENDING_FILES 10009
#define NET_NUM_FILES_TO_SEND  10010

#ifdef WIN32
#define snprintf _snprintf
#endif

Font * Menu::default_font = NULL;
bool   Menu::font_loaded = false;
bool   Menu::initialized = false;

static int selectedPlayer = 0;
static int selectedModule = 0;


/* Button labels.  Defined here for consistency's sake, rather than leaving them as constants */
const char *mainMenuButtons[] =
{
  "Single Player",
  "Multi-player",
  "Options",
  "Quit",
  ""
};

const char *singlePlayerButtons[] =
{
  "New Player",
  "Load Saved Player",
  "Back",
  ""
};

const char *optionsButtons[] =
{
  "Audio Options",
  "Input Controls",
  "Video Settings",
  "Back",
  ""
};

const char *choosemoduleButtons[] =
{
  "<-",
  "->",
  "Select Module",
  ""
};

const char *chooseplayerButtons [] =
{
  "Play!",
  "Back",
  ""
};

static bool startNewPlayer = false;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct Menu_SinglePlayer : public Menu
{
  Menu_SinglePlayer(UI::ID i, StateMachine * parent, Font*fnt = NULL) :
    Menu("Single Player Menu",i,parent,fnt)
    {};

  virtual void run(float deltaTime);

protected:
  virtual void Begin(float deltaTime)  ;
  //virtual void Enter(float deltaTime)  ;
  //virtual void Run(float deltaTime)    ;
  //virtual void Leave(float deltaTime)  ;
  virtual void Finish(float deltaTime) ;
};

struct Menu_ChooseModule : public Menu
{
  Menu_ChooseModule(UI::ID i, StateMachine * parent, Font*fnt = NULL) :
    Menu("Choose Module Menu",i,parent,fnt)
    {};

  virtual void run(float deltaTime);

  WidgetButton * Posters[3];
  WidgetButton * Textbox;

protected:
  virtual void Begin(float deltaTime)  ;
  //virtual void Enter(float deltaTime)  ;
  virtual void Run(float deltaTime)    ;
  //virtual void Leave(float deltaTime)  ;
  virtual void Finish(float deltaTime) ;

  int startIndex;
  int validModules[MAXMODULE];
  int numValidModules;

  int moduleMenuOffsetX;
  int moduleMenuOffsetY;
  int clickedModule;
};

struct Menu_ChoosePlayer : public Menu
{
  Menu_ChoosePlayer(UI::ID i, StateMachine * parent, Font*fnt = NULL) :
    Menu("Choose Player Menu", i, parent, fnt)
    {};

  virtual void run(float deltaTime);

protected:
  virtual void Begin(float deltaTime)  ;
  virtual void Enter(float deltaTime)  ;
  virtual void Run(float deltaTime)    ;
  virtual void Leave(float deltaTime)  ;
  virtual void Finish(float deltaTime) ;
};


struct Menu_Options : public Menu
{
  Menu_Options(UI::ID i, StateMachine * parent, Font*fnt = NULL) :
    Menu("Options Menu", i, parent, fnt)
    {};

  virtual void run(float deltaTime);

protected:
  virtual void Begin(float deltaTime)  ;
  virtual void Enter(float deltaTime)  ;
  virtual void Run(float deltaTime)    ;
  virtual void Leave(float deltaTime)  ;
  virtual void Finish(float deltaTime) ;
};

struct Menu_ShowResults : public Menu
{
  Menu_ShowResults(UI::ID i, StateMachine * parent, Font*fnt = NULL) :
    Menu("Show Results Menu", i, parent, fnt)
    {};

  virtual void run(float deltaTime);

protected:
  virtual void Begin(float deltaTime)  ;
  //virtual void Enter(float deltaTime)  ;
  virtual void Run(float deltaTime)    ;
  //virtual void Leave(float deltaTime)  ;
  virtual void Finish(float deltaTime) ;
};

struct Menu_NotImplemented : public Menu
{
  Menu_NotImplemented(UI::ID i, StateMachine * parent, Font*fnt = NULL) :
    Menu("Not Implemented Menu", i, parent, fnt)
    {};

  //virtual void run(float deltaTime);

protected:
  virtual void Begin(float deltaTime)  ;
  //virtual void Enter(float deltaTime)  ;
  virtual void Run(float deltaTime)    ;
  //virtual void Leave(float deltaTime)  ;
  //virtual void Finish(float deltaTime) ;
};

//--------------------------------------------------------------------------------------------
// New menu code
//--------------------------------------------------------------------------------------------

//struct WidgetSlidy
//{
//  static int top;
//  static int left;
//  static float lerp;
//
//  static Widget ** button_list;
//
//  static void init(float lerp, Widget **buttons);
//  static void update(float deltaTime);
//  static void draw();
//};

//WidgetSlidy GSlidy;

//Widget ** WidgetSlidy::button_list;
//
//int   WidgetSlidy::top;
//int   WidgetSlidy::left;
//float WidgetSlidy::lerp;

//void //WidgetSlidy::init(float l, Widget **buttons)
//{
//  lerp = l;
//  button_list = (Widget**)buttons;
//}
//
//void WidgetSlidy::update(float deltaTime)
//{
//  lerp += (deltaTime * 1.5f);
//}
//
//void WidgetSlidy::draw()
//{
//  int i;
//
//  for (i = 0; NULL != button_list[i]; i++)
//  {
//    Widget * sw = button_list[i];
//
//    int x_save = sw->_region.left;
//    sw->_region.left -= (360 - i * 35)  * lerp;
//
//    sw->Draw(0);
//
//    sw->_region.left = x_save;
//  }
//}

void Menu::Enter(float deltaTime)
{
  // Draw the graphics
  Draw(-deltaTime);

  // Buttons
  Handle_Buttons(deltaTime);

  // Let lerp wind down relative to the time elapsed
  if (lerp <= 0.0f)
  {
    state = SM_Running;
  }
};

void Menu::Run(float deltaTime)
{
  // Draw the graphics
  Draw(0);

  // Buttons
  menuChoice = Handle_Buttons(deltaTime);

  if (menuChoice != 0)
  {
    state = SM_Leaving;
  }
};

void Menu::Leave(float deltaTime)
{
  // Draw the graphics
  Draw(deltaTime);

  // Buttons
  Handle_Buttons(deltaTime);

  if (lerp >= 1.0f)
  {
    state = SM_Finish;
  }
};

bool Menu::initialize(Font * fnt)
{
  if (NULL==fnt && NULL==default_font)
  {
    font_loaded = true;
    default_font = Font_Manager::loadFont("basicdat/Negatori.ttf", 18);
  }
  else if (NULL!=fnt)
    default_font = fnt;

  if (NULL==default_font)
  {
    log_error("Could not load the menu font!\n");
    return false;
  }

  initialized = true;
  atexit(Menu::shutdown);

  return true;
};

void Menu::shutdown()
{
  if (initialized && font_loaded)
  {
    Font_Manager::freeFont(default_font);
    font_loaded = false;
  }

  initialized = false;
};

/* and figures out where things should
 * be positioned.  If we ever allow changing resolution on the fly, this
 * function will have to be updated/called more than once.
 */
bool Menu::SetupAuto()
{
  Font *fnt = getFont();

  // Figure out where to draw the buttons
  buttonLeft = 40;
  buttonTop  = (displaySurface->h - 20) - buttonCount*35;

  // Figure out where to draw the copyright _text
  if (NULL!=fnt && NULL!=_text)
  {
    fnt->getTextBoxSize(_text, 20, textLeft, textTop);

    // Draw the copyright _text to the right of the buttons
    textLeft += 280;
    // And relative to the bottom of the screen
    textTop = displaySurface->h - textTop - 20;
  }
  else
  {
    textLeft = 0;
    textTop  = 0;
  }

  return true;
}


void Menu_Main::Begin(float deltaTime)
{
  // load the background
  GLTexture::Load(&_background, "basicdat/menu/menu_advent.bmp");
  SetImageRegion( REGION((displaySurface->w - _background.imgW), 0, 0, 0) );

  //count the buttons
  for (buttonCount=0; mainMenuButtons[buttonCount][0] != 0x00; buttonCount++);

  //load the window text
  text = "Egoboo 2.x\nhttp://www.mohiji.org/projects/egoboo2x";

  //calculate the regions
  SetupAuto();

  //load the buttons
  for (int i=0; mainMenuButtons[i][0] != 0x00; i++)
    AddButtonAuto(i, mainMenuButtons[i]);

  lerp = 1;

  menuChoice = 0;
  state = SM_Entering;
};

void Menu_Main::Finish(float deltaTime)
{
  // Free the _background texture; don't need to hold onto it
  GLTexture::Release(&_background);

  // Set the next menu to load
  result = menuChoice;

  state = SM_Unknown;
};

//--------------------------------------------------------------------------------------------


void Menu_SinglePlayer::Begin(float deltaTime)
{
  // Load resources for this menu
  GLTexture::Load(&_background, "basicdat/menu/menu_gnome.bmp");
  SetImageRegion( REGION((displaySurface->w - _background.imgW), 0, 0, 0) );

  //count the buttons
  for (buttonCount=0; singlePlayerButtons[buttonCount][0] != 0x00; buttonCount++);

  //load the window text
  text = "Egoboo 2.x\nhttp://www.mohiji.org/projects/egoboo2x";

  //calculate the regions
  SetupAuto();

  //load the buttons
  for (int i=0; singlePlayerButtons[i][0] != 0x00; i++)
    AddButtonAuto(i, singlePlayerButtons[i]);

  lerp = 1;

  menuChoice = 0;
  state = SM_Entering;
};



void Menu_SinglePlayer::Finish(float deltaTime)
{
  // Release the _background texture
  GLTexture::Release(&_background);

  // Set the next menu to load
  result = menuChoice;

  state = SM_Unknown;
};

//--------------------------------------------------------------------------------------------

void Menu_ChooseModule::Begin(float deltaTime)
{
  result = 0;

  startIndex = 0;
  selectedModule = -1;
  clickedModule  = -1; 

  // Find the module's that we want to allow loading for.  If startNewPlayer
  // is true, we want ones that don't allow imports (e.g. starter modules).
  // Otherwise, we want modules that allow imports
  memset(validModules, 0, sizeof(int) * MAXMODULE);
  numValidModules = 0;
  for (int i = 0; i<Module::globalnum; i++)
  {
    if (0==ModList[i].importamount)
    {
      if (startNewPlayer)
      {
        validModules[numValidModules] = i;
        numValidModules++;
      }
    }
    else
    {
      if (!startNewPlayer)
      {
        validModules[numValidModules] = i;
        numValidModules++;
      }
    }
  }


  // Load font & _background
  GLTexture::Load(&_background, "basicdat/menu/menu_sleepy.bmp");

  //count the buttons
  buttonCount = 0;

  //load the window text
  text = NULL;

  //calculate the regions
  SetupAuto();

  //create the buttons in a "non-traditional" format

  // Figure out at what offset we want to draw the module menu.
  moduleMenuOffsetX = (displaySurface->w - 640) / 2;
  moduleMenuOffsetY = (displaySurface->h - 480) / 2;


  // Draw the "selection buttons"
  AddButton(0.0f, REGION(moduleMenuOffsetX +  20, moduleMenuOffsetY + 74, 30, 30), "<-");
  AddButton(0.0f, REGION(moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30), "Select Module");
  AddButton(0.0f, REGION(moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30), "->");
  AddButton(0.0f, REGION(moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30), "Back");

  // Draw buttons for the modules that can be selected
  int x = 93;
  int y = 20;
  for (int i=0, j = startIndex; j < (startIndex + 3) && j < numValidModules; i++,j++)
  {
    Posters[i] = AddButton(0.0f, REGION(moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138));
    x += 138 + 20; // Width of the button, and the spacing between buttons
  }

  buttonCount = 7;

  // Draw a button as the backdrop for the text for now
  Textbox = AddButton(0.0f, REGION(moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230));

  x = (displaySurface->w / 2) - (_background.imgW / 2);
  y = displaySurface->h - _background.imgH;
  SetImageRegion( REGION(x,y,0,0) );

  state = SM_Entering;
};

void Menu_ChooseModule::Run(float deltaTime)
{
  int i,j;
  char tmpBuffer[0x80];
  static char txtBuffer[0x0400];
  static int  selected = 0;

  // Draw the graphics
  Draw(0);

  //do the buttons
  menuChoice = Handle_Buttons(deltaTime);

  switch (menuChoice)
  {
    case 1:
      // "<-"
      startIndex--;
      if (startIndex<0) startIndex = 0;
      break;

    case 2:
      // "Select Module"

      // constrain the selected index
      if (clickedModule>numValidModules || clickedModule<-1)
      {
        clickedModule  = -1;
        selectedModule = -1;
      }

      // map the selected index to an actual module
      if (clickedModule != -1)
      {
        selectedModule = validModules[clickedModule];
      }

      break;

    case 3:
      // "->"
      startIndex++;
      if (startIndex + 3 >= numValidModules)
      {
        startIndex = numValidModules - 3;
      }
      break;

    case 4:
      // "Back"
      clickedModule = -1;
      result = 4;
      break;

    case 5:
      // left pic
      clickedModule = startIndex;
      break;

    case 6:
      // middle pic
      clickedModule = startIndex + 1;
      break;

    case 7:
      // right pic
      clickedModule = startIndex + 2;
      break;

    case 8:
      // text box
      break;
  }



  // Set the _background for the image buttons
  for (i=0, j=startIndex; i<3; i++, j++)
  {
    if(NULL==Posters[i]) continue;

    if (j>=0 && j < numValidModules)
      Posters[i]->SetImage( TxTitleImage[validModules[j]] );
    else
      Posters[i]->ClearImage();
  };

  // Set the text in the Textbox widget
  if (-1 == clickedModule)
  {
    Textbox->ClearText();
  }
  else
  {
    txtBuffer[0] = 0;

    snprintf(tmpBuffer, 0x80, "%s\n", ModList[validModules[clickedModule]].longname);
    strcat(txtBuffer, tmpBuffer);

    snprintf(tmpBuffer, 0x80, "Difficulty: %s\n", ModList[validModules[clickedModule]].rank);
    strcat(txtBuffer, tmpBuffer);

    if (ModList[validModules[clickedModule]].maxplayers > 1)
    {
      if (ModList[validModules[clickedModule]].minplayers == ModList[validModules[clickedModule]].maxplayers)
        snprintf(tmpBuffer, 0x80, "%d Players\n", ModList[validModules[clickedModule]].minplayers);
      else
        snprintf(tmpBuffer, 0x80, "%d - %d Players\n", ModList[validModules[clickedModule]].minplayers, ModList[validModules[clickedModule]].maxplayers);
    }
    else
      snprintf(tmpBuffer, 0x80, "Starter Module\n");

    strcat(txtBuffer, tmpBuffer);

    snprintf(tmpBuffer, 0x80, "modules/%s/gamedat/menu.txt", ModList[validModules[clickedModule]].loadname);
    get_module_summary(tmpBuffer);
    for (int j = 0;j < SUMMARYLINES; j++)
    {
      snprintf(tmpBuffer, 0x80, "%s\n", modsummary[j]);
      strcat(txtBuffer, tmpBuffer);
    }
    Textbox->SetText( (char *)txtBuffer );
  }

  if (selectedModule != -1)
  {
    // Save the name of the module that we've picked
    strncpy(pickedmodule, ModList[selectedModule].loadname, 0x40);

    // If the module allows imports, return 1.  Else, return 2
    if (ModList[selectedModule].importamount > 0)
    {
      importvalid = true;
      importamount = ModList[selectedModule].importamount;
      result = 1;
    }
    else
    {
      importvalid = false;
      result = 2;
    }

    exportvalid  = ModList[selectedModule].allowexport;
    playeramount = ModList[selectedModule].maxplayers;

    respawn_mode = false;
    respawnanytime = false;
    if (ModList[selectedModule].respawn_mode) respawn_mode = true;
    if (ModList[selectedModule].respawn_mode == ANYTIME) respawnanytime = true;

    GRTS.on = false;
  }

};

void Menu_ChooseModule::Finish(float deltaTime)
{
  result = 0;

  GLTexture::Release(&_background);

  state = SM_Unknown;
};



//--------------------------------------------------------------------------------------------

void Menu_ChoosePlayer::Begin(float deltaTime)
{
  result = 0;

  selectedPlayer = 0;

  GLTexture::Load(&_background, "basicdat/menu/menu_sleepy.bmp");

  //count the buttons
  for (buttonCount=0; chooseplayerButtons[buttonCount][0] != 0x00; buttonCount++);

  //load the window text
  text = NULL;

  //calculate the regions
  SetupAuto();

  //load the buttons
  for (int i=0; chooseplayerButtons[i][0] != 0x00; i++)
    AddButtonAuto(i, chooseplayerButtons[i]);

  //place the buttons into the slidy
  //WidgetSlidy::init(1, &sub_widgets);

  // load information for all the players that could be imported
  check_player_import("players");

  state = SM_Entering;
};

void Menu_ChoosePlayer::Run(float deltaTime)
{
  int numVertical, numHorizontal;
  int x, y;

  result = 0;

  // Figure out how many players we can show without scrolling
  numVertical = 6;
  numHorizontal = 2;

  // Draw the graphics
  Draw(deltaTime);


  //do the buttons
  menuChoice = Handle_Buttons(deltaTime);


  // Draw the player selection buttons
  // I'm being tricky, and drawing two buttons right next to each other
  // for each player: one with the icon, and another with the name.  I'm
  // given those buttons the same ID, so they'll act like the same button.

  //player = 0;
  //x = 20;
  //for(j = 0;j < numHorizontal && player < numloadplayer;j++)
  //{
  // y = 20;
  // for(i = 0;i < numVertical && player < numloadplayer;i++)
  // {
  //  if(UI::doImageButtonWithText(player, &IconList[player], loadplayername[player],  x, y, 175, 42))
  //  {
  //   selectedPlayer = player;
  //  }

  //  player++;
  //  y += 47;
  // }
  // x += 180;
  //}

  // Draw the _background
  x = (displaySurface->w / 2) - (_background.imgW / 2);
  y = displaySurface->h - _background.imgH;
  UI::drawImage(&_background, REGION(x, y, 0, 0));

  //// Buttons for going ahead
  //if (UI::doButton(100, "Play!", 40, 350, 200, 30))
  //{
  // state = SM_Leaving;
  //}

  //if (UI::doButton(101, "Back", 40, 385, 200, 30))
  //{
  // selectedPlayer = -1;
  // state = SM_Leaving;
  //}
};

void Menu_ChoosePlayer::Finish(float deltaTime)
{
  int i;
  char srcDir[0x40], destDir[0x40];

  result = 0;

  GLTexture::Release(&_background);
  state = SM_Begin;

  if (selectedPlayer == -1)
    result = -1;
  else
  {
    // Build the import directory
    // I'm just allowing 1 player for now...
    empty_import_directory();
    fs_createDirectory("import");

    localcontrol[0] = INPUT_KEY | INPUT_MOUSE | INPUT_JOYA;
    localslot[0] = localmachine * 9;

    // Copy the character to the import directory
    sprintf(srcDir, "players/%s", loadplayerdir[selectedPlayer]);
    sprintf(destDir, "import/temp%04d.obj", localslot[0]);
    fs_copyDirectory(srcDir, destDir);

    // Copy all of the character's items to the import directory
    for (i = 0;i < 8;i++)
    {
      sprintf(srcDir, "players/%s/%d.obj", loadplayerdir[selectedPlayer], i);
      sprintf(destDir, "import/temp%04d.obj", localslot[0] + i + 1);

      fs_copyDirectory(srcDir, destDir);
    }

    numimport = 1;
    result = 1;
  }

  state = SM_Unknown;
};

//--------------------------------------------------------------------------------------------
void Menu_Options::Begin(float deltaTime)
{
  result = 0;

  // load the _background
  GLTexture::Load(&_background, "basicdat/menu/menu_gnome.bmp");

  //count the buttons
  for (buttonCount=0; optionsButtons[buttonCount][0] != 0x00; buttonCount++);

  //load the window text
  text = "Change your audio, input and video\nsettings here.";

  //calculate the regions
  SetupAuto();

  //load the buttons
  for (int i=0; optionsButtons[i][0] != 0x00; i++)
    AddButtonAuto(i, optionsButtons[i]);

  menuChoice = 0;
  state = SM_Entering;
};

void Menu_Options::Enter(float deltaTime)
{
  // do buttons sliding in animation, and _background fading in
  // _background
  glColor4f(1, 1, 1, 1-lerp);

  //Draw the _background
  UI::drawImage(&_background, REGION((displaySurface->w - _background.imgW), 0, 0, 0));

  // "Copyright" text
  default_font->drawTextBox(text, REGION(textLeft, textTop, 0, 0), 20);

  Draw(-deltaTime);

  // Let lerp wind down relative to the time elapsed
  if (lerp <= 0.0f)
  {
    state = SM_Running;
  }
};

void Menu_Options::Run(float deltaTime)
{
  // Draw the graphics
  Draw(deltaTime);

  //do the buttons
  menuChoice = Handle_Buttons(deltaTime);

  //// Buttons
  //if(UI::doButton(1, optionsButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
  //{
  // //audio options
  // menuChoice = 1;
  //}

  //if(UI::doButton(2, optionsButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
  //{
  // //input options
  // menuChoice = 2;
  //}

  //if(UI::doButton(3, optionsButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
  //{
  // //video options
  // menuChoice = 3;
  //}

  //if(UI::doButton(4, optionsButtons[3], buttonLeft, buttonTop + 35 * 3, 200, 30) == 1)
  //{
  // //back to options
  // menuChoice = 4;
  //}

  if (menuChoice != 0)
  {
    state = SM_Leaving;
  }
};

void Menu_Options::Leave(float deltaTime)
{
  // Do buttons sliding out and _background fading
  // Do the same stuff as in SM_Entering, but backwards
  glColor4f(1, 1, 1, 1 - lerp);
  UI::drawImage(&_background, REGION((displaySurface->w - _background.imgW), 0, 0, 0));

  // "Options" text
  default_font->drawTextBox(text, REGION(textLeft, textTop, 0, 0), 20);

  // Buttons
  Draw(deltaTime);

  if (lerp >= 1.0f)
  {
    state = SM_Finish;
  }

};

void Menu_Options::Finish(float deltaTime)
{
  result = 0;

  // Free the _background texture; don't need to hold onto it
  GLTexture::Release(&_background);
  state = SM_Begin; // Make sure this all resets next time doMainMenu is called

  // Set the next menu to load
  result = menuChoice;

  state = SM_Unknown;
};

//--------------------------------------------------------------------------------------------

//void Menu_ShowResults::run(float deltaTime)
//{
//  Menu::run(deltaTime);
//
//  switch(result)
//  {
//    case 1: spawn_subtask(new Menu_AudioOptions(this)); break;
//    case 2: spawn_subtask(new Menu_InputOptions(this)); break;
//    case 3: spawn_subtask(new Menu_VideoOptions(this)); break;
//    case 4: psm = get_parent();                    break;
//  }
//
//};

void Menu_ShowResults::Begin(float deltaTime)
{
  static char buffer[0x0100];

  snprintf(buffer, 0x80, "Module selected: %s", ModList[selectedModule].loadname);


  if (importvalid)
  {
    snprintf(buffer, 0x80, "%s\nPlayer selected: %s", buffer, loadplayername[selectedPlayer]);
  }
  else
  {
    snprintf(buffer, 0x80, "%s\nStarting a new player.", buffer);
  }

  SetText(buffer);

  int w, h;
  getFont()->getTextBoxSize(_text, 20, w, h);
  SetTextRegion( REGION(35,35,w,h) );

  state = SM_Entering;
};

//void Menu_ShowResults::Enter(float deltaTime)
//{
// result = 0;
//
//  return result;
//};

void Menu_ShowResults::Run(float deltaTime)
{
  Draw(0);
  result = 1;
};

//void Menu_ShowResults::Leave(float deltaTime)
//{
// result = 0;
//
//  return result;
//};

void Menu_ShowResults::Finish(float deltaTime)
{
  Draw(0);
  result = -1;
  state = SM_Unknown;
};

//--------------------------------------------------------------------------------------------

//void Menu_Options::run(float deltaTime)
//{
//  Menu::run(deltaTime);
//
//  switch(result)
//  {
//    case 1: spawn_subtask(new Menu_AudioOptions(this)); break;
//    case 2: spawn_subtask(new Menu_InputOptions(this)); break;
//    case 3: spawn_subtask(new Menu_VideoOptions(this)); break;
//    case 4: state = SM_Finish; this->kill();   break;
//  }
//
//};

void Menu_NotImplemented::Begin(float deltaTime)
{
  result = 0;

  //count the buttons
  buttonCount=0;

  //load the window text
  text = NULL;

  //calculate the regions
  SetupAuto();

  //load the buttons
  AddButtonAuto(0, "Not implemented yet!  Check back soon!");

  //place the buttons into the slidy
  //WidgetSlidy::init(1, &sub_widgets);

  state = SM_Entering;
};

//void Menu_NotImplemented::Enter(float deltaTime)
//{
// result = 0;
//
//  return result;
//};

void Menu_NotImplemented::Run(float deltaTime)
{
  int x, y;
  int w, h;
  char notImplementedMessage[] = "Not implemented yet!  Check back soon!";

  // Draw the graphics
  Draw(deltaTime);

  GUI.getFont()->getTextSize(notImplementedMessage, w, h);
  w += 50; // add some space on the sides

  x = displaySurface->w / 2 - w / 2;
  y = displaySurface->h / 2 - 17;

  //do the buttons
  menuChoice = Handle_Buttons(deltaTime);

};

//void Menu_NotImplemented::Leave(float deltaTime)
//{
// result = 0;
//
//  return result;
//};
//
//void Menu_NotImplemented::Finish(float deltaTime)
//{
// result = 0;
//
//  return result;
//};



void Menu_Main::run(float deltaTime)
{
  if(!isRunning()) return;

  Menu::run(deltaTime);

  switch (menuChoice)
  {
    case 1: spawn_subtask(new Menu_SinglePlayer(UI::new_ID(), this));    break;
    //case 2: spawn_subtask(new Menu_MultiPlayer(UI::new_ID(), this));  break;
    case 2: spawn_subtask(new Menu_NotImplemented(UI::new_ID(), this));  break;
    case 3: spawn_subtask(new Menu_Options(UI::new_ID(), this));         break;
    case 4: state = SM_Finish; this->kill();                    break;
  }
};

void Menu_SinglePlayer::run(float deltaTime)
{
  if(!isRunning()) return;

  Menu::run(deltaTime);

  switch (result)
  {
    case 1: spawn_subtask(new Menu_ChooseModule(UI::new_ID(), this)); startNewPlayer = true;  break;
    case 2: spawn_subtask(new Menu_ChooseModule(UI::new_ID(), this)); startNewPlayer = false; break;
    case 3: state = SM_Finish; this->kill();                                         break;
  }
};

void Menu_ChooseModule::run(float deltaTime)
{
  if(!isRunning()) return;

  Menu::run(deltaTime);

  switch (result)
  {
    case 1:
    case 2: 
      spawn_subtask(new Menu_ShowResults(UI::new_ID(), this));  break;
    case 4: 
      state = SM_Finish; this->kill();                 break;
    case 0: break;
    default:
      int i = 0;
      break;
  }

};

void Menu_Options::run(float deltaTime)
{
  if(!isRunning()) return;

  Menu::run(deltaTime);

  switch (result)
  {
    //case 1: spawn_subtask(new Menu_AudioOptions(this)); break;
    //case 2: spawn_subtask(new Menu_InputOptions(this)); break;
    //case 3: spawn_subtask(new Menu_VideoOptions(this)); break;
    case 1: spawn_subtask(new Menu_NotImplemented(UI::new_ID(), this)); break;
    case 2: spawn_subtask(new Menu_NotImplemented(UI::new_ID(), this)); break;
    case 3: spawn_subtask(new Menu_NotImplemented(UI::new_ID(), this)); break;
    case 4: state = SM_Finish; this->kill();                   break;
  }
};

void Menu_ChoosePlayer::run(float deltaTime)
{
  if(!isRunning()) return;

  Menu::run(deltaTime);

  switch (result)
  {
    case -1: state = SM_Finish; this->kill();                   break;
//  case  1: spawn_subtask(new Menu_TestResults(UI::new_ID(), this));    break;
    case  1: spawn_subtask(new Menu_NotImplemented(UI::new_ID(), this)); break;
  }

};

//int doMenu(float dt, Font * fnt)
//{
//  static IWidget * pmenu = NULL;
//  int result = -1;
//
//  if (NULL==pmenu)
//  {
//    pmenu = new Menu_Main(UI::new_ID(), NULL, fnt);
//  };
//
//  if (NULL!=pmenu)
//  {
//    pmenu  = pmenu->run(dt);
//    result = static_cast<Menu*>(pmenu)->result;
//  };
//
//  return result;
//};

WidgetButton * Menu::AddButtonAuto(int cnt, const char * t, GLTexture * bg)
{
  char buffer[0x0100];

  UI::ID id = UI::new_ID();

  // create a unique name for the button
  snprintf( buffer, 0xFF, "Widget Button 0x%08X", id);

  //create the button
  WidgetButton * butt = new WidgetButton(buffer, id, t, bg);

  // set the button _region
  butt->SetRegion     ( REGION(buttonLeft, buttonTop + cnt*35, 200, 30) );
  butt->SetImageRegion( REGION(buttonLeft, buttonTop + cnt*35, 200, 30) );
  butt->SetTextRegion ( REGION(buttonLeft, buttonTop + cnt*35, 200, 30) );

  //link the button into the menu's widget list
  AddSubWidget(butt);

  //set the button lerp
  butt->lerp = 1.0;
  butt->dx   = buttonLeft + 200;

  return butt;
};

WidgetButton * Menu::AddButton(float l, REGION & r, const char * t, GLTexture * bg)
{
  char buffer[0x0100];

  UI::ID id = UI::new_ID();

  // create a unique name for the button
  snprintf( buffer, 0xFF, "Widget Button 0x%08X", id);

  //create the button
  WidgetButton * butt = new WidgetButton(buffer, id, t, bg);

  // set the button _region
  butt->SetRegion     ( r );
  butt->SetImageRegion( r );
  butt->SetTextRegion ( r );

  //link the button into the menu's widget list
  AddSubWidget(butt);

  //set the button lerp
  butt->lerp = l;
  butt->dx   = r.left + r.width;

  return butt;
};

void Menu_ShowResults::run(float deltaTime)
{
  if(!isRunning()) return;

  Menu::run(deltaTime);

  switch (result)
  {
    case -1: state = SM_Finish; this->kill();                break;
//  case  1: spawn_subtask(new Menu_TestResults(UI::new_ID(), this)); break;
    case  1: spawn_subtask(new Machine_Game(this));                   break;
  }

};

//--------------------------------------------------------------------------------------------
void Menu::run(float deltaTime)
{
  BeginGraphics();

    StateMachine::run(deltaTime);

  EndGraphics();
};


//--------------------------------------------------------------------------------------------
void Menu::BeginGraphics()
{
  // do menus
  GClock.frameStep();
  double frameDuration = GClock.getFrameDuration();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //read_input();
  UI::beginFrame(frameDuration);
}


//--------------------------------------------------------------------------------------------
void Menu::EndGraphics()
{
  UI::endFrame();
  SDL_GL_SwapBuffers();
};

//--------------------------------------------------------------------------------------------
int Menu::Handle_Buttons(float deltaTime)
{
  int retval = 0;

  int i;
  widgetdeq_it j;
  for(i=0, j = subwidgets.begin(); j!=subwidgets.end(); i++, j++)
  {
    (*j)->run(deltaTime);
    if(0 != (*j)->result)
    {
      retval = i+1;
      (*j)->result = 0;
      break;
    }
  };

  return retval;
};

////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//struct _Menu : public Menu
//{
//  virtual int run(Widget * parent, float deltaTime);
//
//protected:
//  virtual void Begin(float deltaTime)  ;
// virtual void Enter(float deltaTime)  ;
// virtual void Run(float deltaTime)    ;
// virtual void Leave(float deltaTime)  ;
// virtual void Finish(float deltaTime) ;
//};
//
//void _Menu::Begin(float deltaTime)
//{
// result = 0;
//
//  return result;
//};
//
//void _Menu::Enter(float deltaTime)
//{
// result = 0;
//
//  return result;
//};
//
//void _Menu::Run(float deltaTime)
//{
// result = 0;
//
//  return result;
//};
//
//void _Menu::Leave(float deltaTime)
//{
// result = 0;
//
//  return result;
//};
//
//void _Menu::Finish(float deltaTime)
//{
// result = 0;
//
//  return result;
//};

//int doMainMenu(float deltaTime)
//{
// static int menuState = SM_Begin;
// static GLTexture _background;
// static float lerp;
// static int menuChoice = 0;
//
// result = 0;
//
// switch(menuState)
// {
// case SM_Begin:
//  // set up menu variables
//  GLTexture::Load(&_background, "basicdat/menu/menu_advent.bmp");
//  menuChoice = 0;
//  menuState = SM_Entering;
//
//  GSlidy.init(1.0f, mainMenuButtons);
//  // let this fall through into SM_Entering
//
// case SM_Entering:
//  // do buttons sliding in animation, and _background fading in
//  // _background
//  glColor4f(1, 1, 1, 1 -lerp);
//  UI::drawImage(0, &_background, (displaySurface->w - _background.imgW), 0, 0, 0);
//
//  // "Copyright" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  GSlidy.draw();
//  GSlidy.update(-deltaTime);
//
//  // Let lerp wind down relative to the time elapsed
//  if (lerp <= 0.0f)
//  {
//   menuState = SM_Running;
//  }
//
//  break;
//
// case SM_Running:
//  // Do normal run
//  // Background
//  glColor4f(1, 1, 1, 1);
//  UI::drawImage(0, &_background, (displaySurface->w - _background.imgW), 0, 0, 0);
//
//  // "Copyright" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  // Buttons
//  if(UI::doButton(1, mainMenuButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
//  {
//   // begin single player stuff
//   menuChoice = 1;
//  }
//
//  if(UI::doButton(2, mainMenuButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
//  {
//   // begin multi player stuff
//   menuChoice = 2;
//  }
//
//  if(UI::doButton(3, mainMenuButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
//  {
//   // go to options menu
//   menuChoice = 3;
//  }
//
//  if(UI::doButton(4, mainMenuButtons[3], buttonLeft, buttonTop + 35 * 3, 200, 30) == 1)
//  {
//   // quit game
//   menuChoice = 4;
//  }
//
//  if(menuChoice != 0)
//  {
//   menuState = SM_Leaving;
//   GSlidy.init(0.0f, mainMenuButtons);
//  }
//
//
//  break;
//
// case SM_Leaving:
//  // Do buttons sliding out and _background fading
//  // Do the same stuff as in SM_Entering, but backwards
//  glColor4f(1, 1, 1, 1 - lerp);
//  UI::drawImage(0, &_background, (displaySurface->w - _background.imgW), 0, 0, 0);
//
//  // "Copyright" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  // Buttons
//  GSlidy.draw();
//  GSlidy.update(deltaTime);
//  if(lerp >= 1.0f) {
//   menuState = SM_Finish;
//  }
//  break;
//
// case SM_Finish:
//  // Free the _background texture; don't need to hold onto it
//  GLTexture::Release(&_background);
//  menuState = SM_Begin; // Make sure this all resets next time doMainMenu is called
//
//  // Set the next menu to load
//  result = menuChoice;
//  break;
// };
//
// return result;
//}

//int doSinglePlayerMenu(float deltaTime)
//{
// static int menuState = SM_Begin;
// static GLTexture _background;
// static int menuChoice;
// result = 0;
//
// switch(menuState)
// {
// case SM_Begin:
//  // Load resources for this menu
//  GLTexture::Load(&_background, "basicdat/menu/menu_gnome.bmp");
//  menuChoice = 0;
//
//  menuState = SM_Entering;
//
//  GSlidy.init(1.0f, singlePlayerButtons);
//
//  // Let this fall through
//
// case SM_Entering:
//  glColor4f(1, 1, 1, 1 - lerp);
//
//  // Draw the _background image
//  UI::drawImage(0, &_background, displaySurface->w - _background.imgW, 0, 0, 0);
//
//  // "Copyright" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  GSlidy.draw();
//  GSlidy.update(-deltaTime);
//
//  if (lerp <= 0.0f)
//   menuState = SM_Running;
//
//  break;
//
// case SM_Running:
//
//  // Draw the _background image
//  UI::drawImage(0, &_background, displaySurface->w - _background.imgW, 0, 0, 0);
//
//  // "Copyright" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  // Buttons
//        if (UI::doButton(1, singlePlayerButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
//  {
//   menuChoice = 1;
//  }
//
//  if (UI::doButton(2, singlePlayerButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
//  {
//   menuChoice = 2;
//  }
//
//  if (UI::doButton(3, singlePlayerButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
//  {
//   menuChoice = 3;
//  }
//
//  if(menuChoice != 0)
//  {
//   menuState = SM_Leaving;
//   GSlidy.init(0.0f, singlePlayerButtons);
//  }
//
//  break;
//
// case SM_Leaving:
//  // Do buttons sliding out and _background fading
//  // Do the same stuff as in SM_Entering, but backwards
//  glColor4f(1, 1, 1, 1 - lerp);
//  UI::drawImage(0, &_background, displaySurface->w - _background.imgW, 0, 0, 0);
//
//  // "Copyright" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  GSlidy.draw();
//  GSlidy.update(deltaTime);
//
//  if(lerp >= 1.0f)
//  {
//   menuState = SM_Finish;
//  }
//  break;
//
// case SM_Finish:
//  // Release the _background texture
//  GLTexture::Release(&_background);
//
//  // Set the next menu to load
//  result = menuChoice;
//
//  // And make sure that if we come back to this menu, it resets
//  // properly
//  menuState = SM_Begin;
// }
//
// return result;
//}
//
// TODO: I totally fudged the layout of this menu by adding an offset for when
// the game isn't in 640x480.  Needs to be fixed.
//int doChooseModule(float deltaTime)
//{
// static int menuState = SM_Begin;
// static int startIndex;
// static GLTexture _background;
// static int validModules[MAXMODULE];
// static int numValidModules;
//
// static int moduleMenuOffsetX;
// static int moduleMenuOffsetY;
//
// result = 0;
// int i, x, y;
// char txtBuffer[0x80];
//
// switch(menuState)
// {
// case SM_Begin:
//  // Load font & _background
//  GLTexture::Load(&_background, "basicdat/menu/menu_sleepy.bmp");
//  startIndex = 0;
//  selectedModule = -1;
//
//  // Find the module's that we want to allow loading for.  If startNewPlayer
//  // is true, we want ones that don't allow imports (e.g. starter modules).
//  // Otherwise, we want modules that allow imports
//  memset(validModules, 0, sizeof(int) * MAXMODULE);
//  numValidModules = 0;
//  for (i = 0;i < Module::globalnum; i++)
//  {
//   if (ModList[i].importamount == 0)
//   {
//    if (startNewPlayer)
//    {
//     validModules[numValidModules] = i;
//     numValidModules++;
//    }
//   } else
//   {
//    if (!startNewPlayer)
//    {
//     validModules[numValidModules] = i;
//     numValidModules++;
//    }
//   }
//  }
//
//  // Figure out at what offset we want to draw the module menu.
//  moduleMenuOffsetX = (displaySurface->w - 640) / 2;
//  moduleMenuOffsetY = (displaySurface->h - 480) / 2;
//
//  menuState = SM_Entering;
//
//  // fall through...
//
// case SM_Entering:
//  menuState = SM_Running;
//
//  // fall through for now...
//
// case SM_Running:
//  // Draw the _background
//  glColor4f(1, 1, 1, 1);
//  x = (displaySurface->w / 2) - (_background.imgW / 2);
//  y = displaySurface->h - _background.imgH;
//  UI::drawImage(0, &_background, x, y, 0, 0);
//
//  // Fudged offset here.. DAMN!  Doesn't work, as the mouse tracking gets skewed
//  // I guess I'll do it the uglier way
//  //glTranslatef(moduleMenuOffsetX, moduleMenuOffsetY, 0);
//
//
//  // Draw the arrows to pick modules
//  if(UI::doButton(1051, "<-", moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30))
//  {
//   startIndex--;
//  }
//
//  if(UI::doButton(1052, "->", moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30))
//  {
//   startIndex++;
//
//   if(startIndex + 3 >= numValidModules)
//   {
//    startIndex = numValidModules - 3;
//   }
//  }
//
//  // Clamp startIndex to 0
//  startIndex = MAX(0, startIndex);
//
//  // Draw buttons for the modules that can be selected
//  x = 93;
//  y = 20;
//  for(i = startIndex; i < (startIndex + 3) && i < numValidModules; i++)
//  {
//   if(UI::doImageButton(i, &TxTitleImage[validModules[i]],
//    moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138))
//   {
//    selectedModule = i;
//   }
//
//   x += 138 + 20; // Width of the button, and the spacing between buttons
//  }
//
//  // Draw an unused button as the backdrop for the text for now
//  UI::drawButton(0xFFFFFFFF, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230);
//
//  // And draw the next & back buttons
//  if (UI::doButton(53, "Select Module",
//   moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30))
//  {
//   // go to the next menu with this module selected
//   selectedModule = validModules[selectedModule];
//   menuState = SM_Leaving;
//  }
//
//  if (UI::doButton(54, "Back", moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30))
//  {
//   // Signal doMenu to go back to the previous menu
//   selectedModule = -1;
//   menuState = SM_Leaving;
//  }
//
//  // Draw the text description of the selected module
//  if(selectedModule > -1)
//  {
//   y = 173 + 5;
//   x = 21 + 5;
//   glColor4f(1, 1, 1, 1);
//   Font::drawText(default_font, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
//    ModList[validModules[selectedModule]].longname);
//   y += 20;
//
//   snprintf(txtBuffer, 0x80, "Difficulty: %s", ModList[validModules[selectedModule]].rank);
//   Font::drawText(default_font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
//   y += 20;
//
//   if(ModList[validModules[selectedModule]].maxplayers > 1)
//   {
//    if(ModList[validModules[selectedModule]].minplayers == ModList[validModules[selectedModule]].maxplayers)
//    {
//     snprintf(txtBuffer, 0x80, "%d Players", ModList[validModules[selectedModule]].minplayers);
//    } else
//    {
//     snprintf(txtBuffer, 0x80, "%d - %d Players", ModList[validModules[selectedModule]].minplayers, ModList[validModules[selectedModule]].maxplayers);
//    }
//   } else
//   {
//    snprintf(txtBuffer, 0x80, "Starter Module");
//   }
//   Font::drawText(default_font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
//   y += 20;
//
//   // And finally, the summary
//   snprintf(txtBuffer, 0x80, "modules/%s/gamedat/menu.txt", ModList[validModules[selectedModule]].loadname);
//   get_module_summary(txtBuffer);
//   for(i = 0;i < SUMMARYLINES;i++)
//   {
//    Font::drawText(default_font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, ModList[i].summary);
//    y += 20;
//   }
//  }
//
//  break;
//
// case SM_Leaving:
//  menuState = SM_Finish;
//  // fall through for now
//
// case SM_Finish:
//  GLTexture::Release(&_background);
//
//  menuState = SM_Begin;
//
//  if(selectedModule == -1)
//  {
//   result = -1;
//  } else
//  {
//   // Save the name of the module that we've picked
//   strncpy(pickedmodule, ModList[selectedModule].loadname, 0x40);
//
//   // If the module allows imports, return 1.  Else, return 2
//   if(ModList[selectedModule].importamount > 0)
//   {
//    importvalid = true;
//    importamount = ModList[selectedModule].importamount;
//    result = 1;
//   }
//   else
//   {
//    importvalid = false;
//    result = 2;
//   }
//
//   exportvalid = ModList[selectedModule].allowexport;
//   playeramount = ModList[selectedModule].maxplayers;
//
//   respawn_mode = false;
//   respawnanytime = false;
//   if(ModList[selectedModule].respawn_mode) respawn_mode = true;
//   if(ModList[selectedModule].respawn_mode == ANYTIME) respawnanytime = true;
//
//   rts_mode = false;
//  }
//  break;
// }
//
// return result;
//}
//
//int doChoosePlayer(float deltaTime)
//{
// static int menuState = SM_Begin;
// static GLTexture _background;
// result = 0;
// int numVertical, numHorizontal;
// int i, j, x, y;
// int player;
// char srcDir[0x40], destDir[0x40];
//
// switch(menuState)
// {
// case SM_Begin:
//  selectedPlayer = 0;
//
//  GLTexture::Load(&_background, "basicdat/menu/menu_sleepy.bmp");
//
//  // load information for all the players that could be imported
//  check_player_import("players");
//
//  menuState = SM_Entering;
//  // fall through
//
// case SM_Entering:
//  menuState = SM_Running;
//  // fall through
//
// case SM_Running:
//  // Figure out how many players we can show without scrolling
//  numVertical = 6;
//  numHorizontal = 2;
//
//  // Draw the player selection buttons
//  // I'm being tricky, and drawing two buttons right next to each other
//  // for each player: one with the icon, and another with the name.  I'm
//  // given those buttons the same ID, so they'll act like the same button.
//  player = 0;
//  x = 20;
//  for(j = 0;j < numHorizontal && player < numloadplayer;j++)
//  {
//   y = 20;
//   for(i = 0;i < numVertical && player < numloadplayer;i++)
//   {
//    if(UI::doImageButtonWithText(player, &IconList[player], loadplayername[player],  x, y, 175, 42))
//    {
//     selectedPlayer = player;
//    }
//
//    player++;
//    y += 47;
//   }
//   x += 180;
//  }
//
//  // Draw the _background
//  x = (displaySurface->w / 2) - (_background.imgW / 2);
//  y = displaySurface->h - _background.imgH;
//  UI::drawImage(0, &_background, x, y, 0, 0);
//
//
//  // Buttons for going ahead
//  if (UI::doButton(100, "Play!", 40, 350, 200, 30))
//  {
//   menuState = SM_Leaving;
//  }
//
//  if (UI::doButton(101, "Back", 40, 385, 200, 30))
//  {
//   selectedPlayer = -1;
//   menuState = SM_Leaving;
//  }
//
//  break;
//
// case SM_Leaving:
//  menuState = SM_Finish;
//  // fall through
//
// case SM_Finish:
//  GLTexture::Release(&_background);
//  menuState = SM_Begin;
//
//  if(selectedPlayer == -1) result = -1;
//  else
//  {
//   // Build the import directory
//   // I'm just allowing 1 player for now...
//   empty_import_directory();
//   fs_createDirectory("import");
//
//   localcontrol[0] = INPUT_KEY | INPUT_MOUSE | INPUT_JOYA;
//   localslot[0] = localmachine * 9;
//
//   // Copy the character to the import directory
//   sprintf(srcDir, "players/%s", loadplayerdir[selectedPlayer]);
//   sprintf(destDir, "import/temp%04d.obj", localslot[0]);
//   fs_copyDirectory(srcDir, destDir);
//
//   // Copy all of the character's items to the import directory
//   for(i = 0;i < 8;i++)
//   {
//    sprintf(srcDir, "players/%s/%d.obj", loadplayerdir[selectedPlayer], i);
//    sprintf(destDir, "import/temp%04d.obj", localslot[0] + i + 1);
//
//    fs_copyDirectory(srcDir, destDir);
//   }
//
//   numimport = 1;
//   result = 1;
//  }
//
//  break;
// }
//
// return result;
//}
//
//TODO: This needs to be finished
//int doOptions(float deltaTime)
//{
// static int menuState = SM_Begin;
// static GLTexture _background;
// static float lerp;
// static int menuChoice = 0;
//
// result = 0;
//
// switch(menuState)
// {
// case SM_Begin:
//  // set up menu variables
//  GLTexture::Load(&_background, "basicdat/menu/menu_gnome.bmp");
//  menuChoice = 0;
//  menuState = SM_Entering;
//
//  GSlidy.init(1.0f, optionsButtons);
//  // let this fall through into SM_Entering
//
// case SM_Entering:
//  // do buttons sliding in animation, and _background fading in
//  // _background
//  glColor4f(1, 1, 1, 1 -lerp);
//
//  //Draw the _background
//  UI::drawImage(0, &_background, (displaySurface->w - _background.imgW), 0, 0, 0);
//
//  // "Copyright" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  GSlidy.draw();
//  GSlidy.update(-deltaTime);
//
//  // Let lerp wind down relative to the time elapsed
//  if (lerp <= 0.0f)
//  {
//   menuState = SM_Running;
//  }
//
//  break;
//
// case SM_Running:
//  // Do normal run
//  // Background
//  glColor4f(1, 1, 1, 1);
//  UI::drawImage(0, &_background, (displaySurface->w - _background.imgW), 0, 0, 0);
//
//  // "Options" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  // Buttons
//  if(UI::doButton(1, optionsButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
//  {
//   //audio options
//   menuChoice = 1;
//  }
//
//  if(UI::doButton(2, optionsButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
//  {
//   //input options
//   menuChoice = 2;
//  }
//
//  if(UI::doButton(3, optionsButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
//  {
//   //video options
//   menuChoice = 3;
//  }
//
//  if(UI::doButton(4, optionsButtons[3], buttonLeft, buttonTop + 35 * 3, 200, 30) == 1)
//  {
//   //back to options
//   menuChoice = 4;
//  }
//
//  if(menuChoice != 0)
//  {
//   menuState = SM_Leaving;
//   GSlidy.init(0.0f, optionsButtons);
//  }
//  break;
//
// case SM_Leaving:
//  // Do buttons sliding out and _background fading
//  // Do the same stuff as in SM_Entering, but backwards
//  glColor4f(1, 1, 1, 1 - lerp);
//  UI::drawImage(0, &_background, (displaySurface->w - _background.imgW), 0, 0, 0);
//
//  // "Options" text
//  Font::drawTextBox(default_font, text, textLeft, textTop, 0, 0, 20);
//
//  // Buttons
//  GSlidy.draw();
//  GSlidy.update(deltaTime);
//  if(lerp >= 1.0f) {
//   menuState = SM_Finish;
//  }
//  break;
//
// case SM_Finish:
//  // Free the _background texture; don't need to hold onto it
//  GLTexture::Release(&_background);
//  menuState = SM_Begin; // Make sure this all resets next time doMainMenu is called
//
//  // Set the next menu to load
//  result = menuChoice;
//  break;
// }
// return result;
//}

//int doShowMenuResults(float deltaTime)
//{
// int x, y;
// char text[0x80];
// Font *font;
//
// SDL_Surface *screen = SDL_GetVideoSurface();
// font = GUI.getFont();
//
// UI::drawButton(0xFFFFFFFF, 30, 30, screen->w - 60, screen->h - 65);
//
// x = 35;
// y = 35;
// glColor4f(1, 1, 1, 1);
// snprintf(text, 0x80, "Module selected: %s", ModList[selectedModule].loadname);
// Font::drawText(font, x, y, text);
// y += 35;
//
// if(importvalid)
// {
//  snprintf(text, 0x80, "Player selected: %s", loadplayername[selectedPlayer]);
// } else
// {
//  snprintf(text, 0x80, "Starting a new player.");
// }
// Font::drawText(font, x, y, text);
//
// return 1;
//}

//int doNotImplemented(float deltaTime)
//{
// int x, y;
// int w, h;
// char notImplementedMessage[] = "Not implemented yet!  Check back soon!";
//
//
// Font::getTextSize(GUI.getFont(), notImplementedMessage, &w, &h);
// w += 50; // add some space on the sides
//
// x = displaySurface->w / 2 - w / 2;
// y = displaySurface->h / 2 - 17;
//
// if(UI::doButton(1, notImplementedMessage, x, y, w, 30) == 1)
// {
//  return 1;
// }
//
// return 0;
//}

// All the different menus.  yay!
//enum
//{
// MainMenu,
// SinglePlayer,
// MultiPlayer,
// ChooseModule,
// ChoosePlayer,
// TestResults,
// Options,
// VideoOptions,
// AudioOptions,
// InputOptions,
// NewPlayer,
// LoadPlayer,
// HostGame,
// JoinGame,
//};

//int doMenu(float deltaTime)
//{
// static int whichMenu = Menu_Main;
// static int get_parent() = Menu_Main;
// result = 0;
//
// switch(whichMenu)
// {
// case Menu_Main:
//  result = doMainMenu(deltaTime);
//  if(result != 0)
//  {
//   get_parent() = Menu_Main;
//   if(result == 1) whichMenu = SinglePlayer;
//   else if(result == 2) whichMenu = MultiPlayer;
//   else if(result == 3) whichMenu = Options;
//   else if(result == 4) return -1; // need to request a quit somehow
//  }
//  break;
//
// case SinglePlayer:
//  result = doSinglePlayerMenu(deltaTime);
//  if(result != 0)
//  {
//   get_parent() = SinglePlayer;
//   if(result == 1)
//   {
//    whichMenu = ChooseModule;
//    startNewPlayer = true;
//   } else if(result == 2)
//   {
//    whichMenu = ChooseModule;
//    startNewPlayer = false;
//   }
//   else if(result == 3) whichMenu = Menu_Main;
//   else whichMenu = NewPlayer;
//  }
//  break;
//
// case ChooseModule:
//  result = doChooseModule(deltaTime);
//  if(result == -1) whichMenu = get_parent();
//  else if(result == 1) whichMenu = ChoosePlayer;
//  else if(result == 2) whichMenu = TestResults;
//  break;
//
// case ChoosePlayer:
//  result = doChoosePlayer(deltaTime);
//  if(result == -1)  whichMenu = ChooseModule;
//  else if(result == 1) whichMenu = TestResults;
//
//  break;
//
// case Options:
//  result = doOptions(deltaTime);
//  if(result != 0)
//  {
//   if(result == 1) whichMenu = AudioOptions;
//   else if(result == 2) whichMenu = InputOptions;
//   else if(result == 3) whichMenu = VideoOptions;
//   else if(result == 4) whichMenu = Menu_Main;
//  }
//  break;
//
// case TestResults:
//  result = doShowMenuResults(deltaTime);
//  if(result != 0)
//  {
//   whichMenu = Menu_Main;
//   return 1;
//  }
//  break;
//
// default:
//  result = doNotImplemented(deltaTime);
//  if(result != 0)
//  {
//   whichMenu = get_parent();
//  }
// }
//
// return 0;
//}
//

/* Old Menu Code */

#if 0
////--------------------------------------------------------------------------------------------
//void menu_service_select()
//{
//    // ZZ> This function lets the user choose a network service to use
//    char text[0x0100];
//    int x, y;
//    float open;
//    int cnt;
//    int stillchoosing;
//
//    GNet.workservice = NONETWORK;
//    if(GNet.numservice > 0)
//    {
//        // Open a big window
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, scrx, scry, open);
//            draw_trim_box_opening(0, 0, 320, GFont.spacing_y*(GNet.numservice+4), open);
//            flip_pages();
//            open += .030;
//        }
//        // Tell the user which ones we found ( in setup_GNet.work )
//        stillchoosing = true;
//        while(stillchoosing)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, scrx, scry);
//            draw_trim_box(0, 0, 320, GFont.spacing_y*(GNet.numservice+4));
//            y = 8;
//            sprintf(text, "Network options...");
//            draw_string(text, 14, y);
//            y += GFont.spacing_y;
//            cnt = 0;
//            while(cnt < GNet.numservice)
//            {
//                sprintf(text, "%s", GNet.servicename[cnt]);
//                draw_string(text, 50, y);
//                y += GFont.spacing_y;
//                cnt++;
//            }
//            sprintf(text, "No Network");
//            draw_string(text, 50, y);
//            do_cursor();
//            x = GUI.cursorx - 50;
//            y = (GUI.cursory - 8 - GFont.spacing_y);
//            if(x > 0 && x < 300 && y >= 0)
//            {
//                y = y/GFont.spacing_y;
//                if(y <= GNet.numservice)
//                {
//                    if(GMous.button[0] || GMous.button[1])
//                    {
//                        stillchoosing = false;
//                        GNet.workservice = y;
//                    }
//                }
//            }
//            flip_pages();
//        }
//    }
////    turn_on_service(GNet.workservice);
//}
//
////--------------------------------------------------------------------------------------------
//void menu_start_or_join()
//{
//    // ZZ> This function lets the user start or join a game for a network game
//    char text[0x0100];
//    int x, y;
//    float open;
//    int stillchoosing;
//
//    // Open another window
//    if(GNet.on)
//    {
//        open = 0;
//        while(open < 1.0)
//        {
//   //clear_surface(lpDDSBack);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box_opening(0, 0, scrx, scry, open);
//   draw_trim_box_opening(0, 0, 280, 102, open);
//   flip_pages();
//   open += .030;
//  }
//        // Give the user some options
//        stillchoosing = true;
//        while(stillchoosing)
//        {
//   //clear_surface(lpDDSBack);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box(0, 0, scrx, scry);
//   draw_trim_box(0, 0, 280, 102);
//
//   // Draw the menu text
//   y = 8;
//   sprintf(text, "Game options...");
//   draw_string(text, 14, y);
//   y += GFont.spacing_y;
//   sprintf(text, "New Game");
//   draw_string(text, 50, y);
//   y += GFont.spacing_y;
//   sprintf(text, "Join Game");
//   draw_string(text, 50, y);
//   y += GFont.spacing_y;
//   sprintf(text, "Quit Game");
//   draw_string(text, 50, y);
//
//   do_cursor();
//
////   sprintf(text, "Cursor position: %03d, %03d", GUI.cursorx, GUI.cursory);
////   draw_string(text, 14, 400);
//
//   x = GUI.cursorx - 50;
//   // The adjustments to y here were figured out empirically; I still
//   // don't understand the reasoning behind it.  I don't think the text
//   // draws where it says it's going to.
//   y = (GUI.cursory - 21 - GFont.spacing_y);
//
//
//
//            if(x > 0 && x < 280 && y >= 0)
//            {
//                y = y/GFont.spacing_y;
//                if(y < 3)
//                {
//                    if(GMous.button[0] || GMous.button[1])
//                    {
//                        if(y == 0)
//                        {
//                            if(sv_hostGame())
//                            {
//                                hostactive = true;
//                                nextmenu = MENUD;
//                                stillchoosing = false;
//                            }
//                        }
//                        if(y == 1 && GNet.workservice != NONETWORK)
//                        {
//                            nextmenu = MENUC;
//                            stillchoosing = false;
//                        }
//                        if(y == 2)
//                        {
//                            nextmenu = MENUB;
//                            menuactive = false;
//                            stillchoosing = false;
//                            gameactive = false;
//                        }
//                    }
//                }
//            }
//            flip_pages();
//        }
//    }
//    else
//    {
//        hostactive = true;
//        nextmenu = MENUD;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void draw_module_tag(int module, int y)
//{
//    // ZZ> This function draws a module tag
//    char text[0x0100];
//    draw_trim_box(0, y, 136, y+136);
//    draw_trim_box(132, y, scrx, y+136);
//    if(module < Module::globalnum)
//    {
//        draw_titleimage(module, 4, y+4);
//        y+=6;
//        sprintf(text, "%s", ModList[module].longname);  draw_string(text, 150, y);  y+=GFont.spacing_y;
//        sprintf(text, "%s", ModList[module].rank);  draw_string(text, 150, y);  y+=GFont.spacing_y;
//        if(ModList[module].maxplayers > 1)
//        {
//            if(ModList[module].minplayers==ModList[module].maxplayers)
//            {
//                sprintf(text, "%d players", ModList[module].minplayers);
//            }
//            else
//            {
//                sprintf(text, "%d-%d players", ModList[module].minplayers, ModList[module].maxplayers);
//            }
//        }
//        else
//        {
//            sprintf(text, "1 player");
//        }
//        draw_string(text, 150, y);  y+=GFont.spacing_y;
//        if(ModList[module].importamount == 0 && !ModList[module].allowexport)
//        {
//            sprintf(text, "No Import/Export");  draw_string(text, 150, y);  y+=GFont.spacing_y;
//        }
//        else
//        {
//            if(ModList[module].importamount == 0)
//            {
//                sprintf(text, "No Import");  draw_string(text, 150, y);  y+=GFont.spacing_y;
//            }
//            if(!ModList[module].allowexport)
//            {
//                sprintf(text, "No Export");  draw_string(text, 150, y);  y+=GFont.spacing_y;
//            }
//        }
//        if(!ModList[module].respawn_mode)
//        {
//            sprintf(text, "No Respawn");  draw_string(text, 150, y);  y+=GFont.spacing_y;
//        }
//        if(ModList[module].rts_mode)
//        {
//            sprintf(text, "RTS");  draw_string(text, 150, y);  y+=GFont.spacing_y;
//        }
//        if(ModList[module].rts_mode == ALLSELECT)
//        {
//            sprintf(text, "Diaboo RTS");  draw_string(text, 150, y);  y+=GFont.spacing_y;
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void menu_pick_player(int module)
//{
//    // ZZ> This function handles the display for picking players to import
//    int x, y;
//    float open;
//    int cnt, tnc, start, numshow;
//    int stillchoosing;
//    int import;
//    Uint8 control, sparkle;
//    char fromdir[0x80];
//    char todir[0x80];
// int clientFilesSent = 0;
// int hostFilesSent = 0;
// int pending;
//
//    // Set the important flags
//    respawn_mode = false;
//    respawnanytime = false;
//    if(ModList[module].respawn_mode)  respawn_mode = true;
//    if(ModList[module].respawn_mode==ANYTIME)  respawnanytime = true;
//    rts_mode = false;
//    if(ModList[module].rts_mode != false)
//    {
//        rts_mode = true;
//        allselect = false;
//        if(ModList[module].rts_mode == ALLSELECT)
//            allselect = true;
//    }
//    exportvalid = ModList[module].allowexport;
//    importvalid = (ModList[module].importamount > 0);
//    importamount = ModList[module].importamount;
//    playeramount = ModList[module].maxplayers;
//    fs_createDirectory("import");  // Just in case...
//
//    start = 0;
//    if(importvalid)
//    {
//        // Figure out which characters are available
//        check_player_import("players");
//        numshow = (scry-80-GFont.spacing_y-GFont.spacing_y)>>5;
//
//        // Open some windows
//        y = GFont.spacing_y + 8;
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, scrx, scry, open);
//            draw_trim_box_opening(0, 0, scrx, 40, open);
//            draw_trim_box_opening(0, scry-40, scrx, scry, open);
//            flip_pages();
//            open += .030;
//        }
//
//        wldframe = 0;  // For sparkle
//        stillchoosing = true;
//        while(stillchoosing)
//        {
//            // Draw the windows
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, scrx, scry);
//            draw_trim_box(0, 40, scrx, scry-40);
//
//            // Draw the Up/Down buttons
//            if(start == 0)
//            {
//                // Show the instructions
//                x = (scrx-270)>>1;
//                draw_string("Setup controls", x, 10);
//            }
//            else
//            {
//                x = (scrx-40)>>1;
//                draw_string("Up", x, 10);
//            }
//            x = (scrx-80)>>1;
//            draw_string("Down", x, scry-GFont.spacing_y-20);
//
//            // Draw each import character
//            y = 40+GFont.spacing_y;
//            cnt = 0;
//            while(cnt < numshow && cnt + start < numloadplayer)
//            {
//                sparkle = NOSPARKLE;
//                if(GKeyb.player == (cnt+start))
//                {
//                    draw_one_icon(IconList.keyb, 0x20, y, NOSPARKLE);
//                    sparkle = 0;  // White
//                }
//                else
//                    draw_one_icon(IconList.null, 0x20, y, NOSPARKLE);
//                if(GMous.player == (cnt+start))
//                {
//                    draw_one_icon(IconList.mous, 0x40, y, NOSPARKLE);
//                    sparkle = 0;  // White
//                }
//                else
//                    draw_one_icon(IconList.null, 0x40, y, NOSPARKLE);
//                if(GJoy[0].player == (cnt+start) && GJoy[0].on)
//                {
//                    draw_one_icon(IconList.joya, 0x80, y, NOSPARKLE);
//                    sparkle = 0;  // White
//                }
//                else
//                    draw_one_icon(IconList.null, 0x80, y, NOSPARKLE);
//                if(GJoy[1].player == (cnt+start) && GJoy[1].on)
//                {
//                    draw_one_icon(IconList.joyb, 160, y, NOSPARKLE);
//                    sparkle = 0;  // White
//                }
//                else
//                    draw_one_icon(IconList.null, 160, y, NOSPARKLE);
//                draw_one_icon((cnt+start), 96, y, sparkle);
//                draw_string(loadplayername[cnt+start], 200, y+6);
//                y+=0x20;
//                cnt++;
//            }
//            wldframe++;  // For sparkle
//
//            // Handle other stuff...
//            do_cursor();
//            if(GUI.pending_click)
//            {
//         GUI.pending_click=false;
//                if(GUI.cursory < 40 && start > 0)
//                {
//                    // Up button
//                    start--;
//                }
//                if(GUI.cursory >= (scry-40) && (start + numshow) < numloadplayer)
//                {
//                    // Down button
//                    start++;
//                }
//            }
//            if(GMous.button[0])
//            {
//                x = (GUI.cursorx - 0x20) >> 5;
//                y = (GUI.cursory - 44) >> 5;
//                if(y >= 0 && y < numshow)
//                {
//                    y += start;
//                    // Assign the controls
//                    if(y < numloadplayer)  // !!!BAD!!! do scroll
//                    {
//                        if(x == 0)  GKeyb.player = y;
//                        if(x == 1)  GMous.player = y;
//                        if(x == 3)  GJoy[0].player = y;
//                        if(x == 4)  GJoy[1].player = y;
//                    }
//                }
//            }
//            if(GMous.button[1])
//            {
//                // Done picking
//                stillchoosing = false;
//            }
//            flip_pages();
//        }
//        wldframe = 0;  // For sparkle
//
//  // Tell the user we're loading
//  y = GFont.spacing_y + 8;
//  open = 0;
//  while(open < 1.0)
//  {
//   //clear_surface(lpDDSBack);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box_opening(0, 0, scrx, scry, open);
//   flip_pages();
//   open += .030;
//  }
//
//  //clear_surface(lpDDSBack);
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//  glLoadIdentity();
//  draw_trim_box(0, 0, scrx, scry);
//  draw_string("Copying the imports...", y, y);
//  flip_pages();
//
//        // Now build the import directory...
//        empty_import_directory();
//        cnt = 0;
//        numimport = 0;
//        while(cnt < numloadplayer)
//        {
//            if((cnt == GKeyb.player && GKeyb.on)   || 
//               (cnt == GMous.player && GMous.on) || 
//               (cnt == GJoy[0].player && GJoy[0].on)  || 
//               (cnt == GJoy[1].player && GJoy[1].on))
//            {
//                // This character has been selected
//                control = INPUT_NONE;
//                if(cnt == GKeyb.player)  control = control | INPUT_KEY;
//                if(cnt == GMous.player)  control = control | INPUT_MOUSE;
//                if(cnt == GJoy[0].player)  control = control | INPUT_JOYA;
//                if(cnt == GJoy[1].player)  control = control | INPUT_JOYB;
//                localcontrol[numimport] = control;
////                localslot[numimport] = (numimport+(localmachine*4))*9;
//    localslot[numimport] = (numimport + localmachine) * 9;
//
//                // Copy the character to the import directory
//                sprintf(fromdir, "players/%s", loadplayerdir[cnt]);
//                sprintf(todir, "import/temp%04d.obj", localslot[numimport]);
//
//    // This will do a local copy if I'm already on the host machine, other
//    // wise the directory gets sent across the network to the host
//    net_copyDirectoryToHost(fromdir, todir);
//
//    // Copy all of the character's items to the import directory
//    tnc = 0;
//    while(tnc < 8)
//    {
//     sprintf(fromdir, "players/%s/%d.obj", loadplayerdir[cnt], tnc);
//     sprintf(todir, "import/temp%04d.obj", localslot[numimport]+tnc+1);
//
//     net_copyDirectoryToHost(fromdir, todir);
//     tnc++;
//    }
//
//                numimport++;
//            }
//            cnt++;
//        }
//
//  // Have clients wait until all files have been sent to the host
//  clientFilesSent = net_pendingFileTransfers();
//  if(GNet.on && !hostactive)
//  {
//   pending = net_pendingFileTransfers();
//
//   // Let the host know how many files you're sending it
//   net_startNewPacket();
//   packet_addUnsignedShort(NET_NUM_FILES_TO_SEND);
//   packet_addUnsignedShort((Uint16)pending);
//   net_sendPacketToHostGuaranteed();
//
//   while(pending)
//   {
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glLoadIdentity();
//
//    draw_trim_box(0, 0, scrx, scry);
//    y = GFont.spacing_y + 8;
//
//    sprintf(todir, "Sending file %d of %d...", clientFilesSent - pending, clientFilesSent);
//    draw_string(todir, GFont.spacing_y + 8, y);
//    flip_pages();
//
//    // do this to let SDL do it's window events stuff, so that windows doesn't think
//    // the game has hung while transferring files
//    do_cursor();
//
//    net_updateFileTransfers();
//
//    pending = net_pendingFileTransfers();
//   }
//
//   // Tell the host I'm done sending files
//   net_startNewPacket();
//   packet_addUnsignedShort(NET_DONE_SENDING_FILES);
//   net_sendPacketToHostGuaranteed();
//  }
//
//  if(GNet.on)
//  {
//   if(hostactive)
//   {
//    // Host waits for all files from all remotes
//    numfile = 0;
//    numfileexpected = 0;
//    numplayerrespond = 1;
//    while(numplayerrespond < GNet.numplayer)
//    {
//     //clear_surface(lpDDSBack);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glLoadIdentity();
//     draw_trim_box(0, 0, scrx, scry);
//     y = GFont.spacing_y + 8;
//     draw_string("Incoming files...", GFont.spacing_y+8, y);  y+=GFont.spacing_y;
//     sprintf(todir, "File %d/%d", numfile, numfileexpected);
//     draw_string(todir, GFont.spacing_y+20, y); y+=GFont.spacing_y;
//     sprintf(todir, "Play %d/%d", numplayerrespond, GNet.numplayer);
//     draw_string(todir, GFont.spacing_y+20, y);
//     flip_pages();
//
//     listen_for_packets();
//
//     do_cursor();
//
//     if(GKeyb.pressed(SDLK_ESCAPE))
//     {
//      gameactive = false;
//      menuactive = false;
//      close_session();
//      break;
//     }
//    }
//
//                // Say you're done
//                //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//                draw_trim_box(0, 0, scrx, scry);
//                y = GFont.spacing_y + 8;
//                draw_string("Sending files to remotes...", GFont.spacing_y+8, y);  y+=GFont.spacing_y;
//                flip_pages();
//
//                // Host sends import directory to all remotes, deletes extras
//                numfilesent = 0;
//                import = 0;
//                cnt = 0;
//    if(GNet.numplayer > 1)
//    {
//     while(cnt < MAXIMPORT)
//     {
//      sprintf(todir, "import/temp%04d.obj", cnt);
//      strncpy(fromdir, todir, 0x80);
//      if(fs_fileIsDirectory(fromdir))
//      {
//       // Only do directories that actually exist
//       if((cnt % 9)==0) import++;
//       if(import > importamount)
//       {
//        // Too many directories
//        fs_removeDirectoryAndContents(fromdir);
//       }
//       else
//       {
//        // Ship it out
//        net_copyDirectoryToAllPlayers(fromdir, todir);
//       }
//      }
//      cnt++;
//     }
//
//     hostFilesSent = net_pendingFileTransfers();
//     pending = hostFilesSent;
//
//     // Let the client know how many are coming
//     net_startNewPacket();
//     packet_addUnsignedShort(NET_NUM_FILES_TO_SEND);
//     packet_addUnsignedShort((Uint16)pending);
//     net_sendPacketToAllPlayersGuaranteed();
//
//     while(pending > 0)
//     {
//      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//      glLoadIdentity();
//
//      draw_trim_box(0, 0, scrx, scry);
//      y = GFont.spacing_y + 8;
//
//      sprintf(todir, "Sending file %d of %d...", hostFilesSent - pending, hostFilesSent);
//      draw_string(todir, GFont.spacing_y + 8, y);
//      flip_pages();
//
//      // do this to let SDL do it's window events stuff, so that windows doesn't think
//      // the game has hung while transferring files
//      do_cursor();
//
//      net_updateFileTransfers();
//
//      pending = net_pendingFileTransfers();
//     }
//
//     // Tell the players I'm done sending files
//     net_startNewPacket();
//     packet_addUnsignedShort(NET_DONE_SENDING_FILES);
//     net_sendPacketToAllPlayersGuaranteed();
//    }
//            }
//            else
//            {
//    // Remotes wait for all files in import directory
//    net_logf("menu_pick_player: Waiting for files to come from the host...\n");
//    numfile = 0;
//    numfileexpected = 0;
//    numplayerrespond = 0;
//    while(numplayerrespond < 1)
//    {
//     //clear_surface(lpDDSBack);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glLoadIdentity();
//     draw_trim_box(0, 0, scrx, scry);
//     y = GFont.spacing_y + 8;
//     draw_string("Incoming files from host...", GFont.spacing_y+8, y);  y+=GFont.spacing_y;
//     sprintf(todir, "File %d/%d", numfile, numfileexpected);
//     draw_string(todir, GFont.spacing_y+20, y);
//     flip_pages();
//
//     listen_for_packets();
//     do_cursor();
//
//     if(GKeyb.pressed(SDLK_ESCAPE))
//     {
//      gameactive = false;
//      menuactive = false;
//      break;
//      close_session();
//     }
//                }
//            }
//        }
//    }
//    nextmenu = MENUG;
//}
//
////--------------------------------------------------------------------------------------------
//void menu_module_loading(int module)
//{
//    // ZZ> This function handles the display for when a module is loading
//    char text[0x0100];
//    int y;
//    float open;
//    int cnt;
//
//    // Open some windows
//    y = GFont.spacing_y + 8;
//    open = 0;
//    while(open < 1.0)
//    {
//        //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//        draw_trim_box_opening(0, y, 136, y+136, open);
//        draw_trim_box_opening(132, y, scrx, y+136, open);
//        draw_trim_box_opening(0, y+132, scrx, scry, open);
//        flip_pages();
//        open += .030;
//    }
//
//    // Put the stuff in the windows
//    //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//    y = 0;
//    sprintf(text, "Loading...  Wait!!!");  draw_string(text, 0, y);  y+=GFont.spacing_y;
//    y+=8;
//    draw_module_tag(module, y);
//    draw_trim_box(0, y+132, scrx, scry);
//
//    // Show the summary
//    sprintf(text, "modules/%s/gamedat/menu.txt", ModList[module].loadname);
//    get_module_summary(text);
//    y = GFont.spacing_y+152;
//    cnt = 0;
//    while(cnt < SUMMARYLINES)
//    {
//        sprintf(text, "%s", ModList[cnt].summary);  draw_string(text, 14, y);  y+=GFont.spacing_y;
//        cnt++;
//    }
//    flip_pages();
//    nextmenu = MENUB;
//    menuactive = false;
//}
//
////--------------------------------------------------------------------------------------------
//void menu_join_multiplayer()
//{
// // JF> This function attempts to join the multiplayer game hosted
// //     by whatever server is named in the HOST_NAME part of SetupAuto.txt
// char text[0x0100];
// float open;
//
// if(GNet.on)
// {
//  // Do the little opening menu animation
//  open = 0;
//  while(open < 1.0)
//  {
//   //clear_surface(lpDDSBack);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box_opening(0, 0, scrx, scry, open);
//   flip_pages();
//   open += .030;
//  }
//
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//  glLoadIdentity();
//  draw_trim_box(0, 0, scrx, scry);
//
//  strncpy(text, "Attempting to join game at:", 0x0100);
//  draw_string(text, (scrx>>1)-240, (scry>>1)-GFont.spacing_y);
//
//  strncpy(text, GNet.hostname, 0x0100);
//  draw_string(text, (scrx>>1)-240, (scry>>1));
//  flip_pages();
//
//  if(cl_joinGame(GNet.hostname))
//  {
//   nextmenu = MENUE;
//  } else
//  {
//   nextmenu = MENUB;
//  }
// }
//}
//
////--------------------------------------------------------------------------------------------
//void menu_choose_host()
//{
//    // ZZ> This function lets the player choose a host
//    char text[0x0100];
//    int x, y;
//    float open;
//    int cnt;
//    int stillchoosing;
//
//    if(GNet.on)
//    {
//        // Bring up a helper window
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, scrx, scry, open);
//            flip_pages();
//            open += .030;
//        }
//        //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//        draw_trim_box(0, 0, scrx, scry);
//        sprintf(text, "Press Enter if");
//        draw_string(text, (scrx>>1)-120, (scry>>1)-GFont.spacing_y);
//        sprintf(text, "nothing happens");
//        draw_string(text, (scrx>>1)-120, (scry>>1));
//        flip_pages();
//
//        // Find available games
////        find_open_sessions();       // !!!BAD!!!  Do this every now and then
//
//        // Open a big window
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, scrx, scry, open);
//            draw_trim_box_opening(0, 0, 320, GFont.spacing_y*(GNet.numsession+4), open);
//            flip_pages();
//            open += .030;
//        }
//
//        // Tell the user which ones we found
//        stillchoosing = true;
//        while(stillchoosing)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, scrx, scry);
//            draw_trim_box(0, 0, 320, GFont.spacing_y*(GNet.numsession+4));
//            y = 8;
//            sprintf(text, "Open hosts...");
//            draw_string(text, 14, y);
//            y += GFont.spacing_y;
//            cnt = 0;
//            while(cnt < GNet.numsession)
//            {
//                sprintf(text, "%s", GNet.sessionname[cnt]);
//                draw_string(text, 50, y);
//                y += GFont.spacing_y;
//                cnt++;
//            }
//            sprintf(text, "Go Back...");
//            draw_string(text, 50, y);
//            do_cursor();
//            x = GUI.cursorx - 50;
//            y = (GUI.cursory - 8 - GFont.spacing_y);
//            if(x > 0 && x < 300 && y >= 0)
//            {
//                y = y/GFont.spacing_y;
//                if(y <= GNet.numsession)
//                {
//                    if(GMous.button[0] || GMous.button[1])
//                    {
//                        //if(y == GNet.numsession)
//                        //{
//                        //    nextmenu = MENUB;
//                        //    stillchoosing = false;
//                        //}
//                        //else
//                        {
//                            if(cl_joinGame("solace2.csusm.edu"))
//                            {
//                                nextmenu = MENUE;
//                                stillchoosing = false;
//                            }
//                        }
//                    }
//                }
//            }
//            flip_pages();
//        }
//    }
//    else
//    {
//        // This should never happen
//        nextmenu = MENUB;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void menu_choose_module()
//{
//    // ZZ> This function lets the host choose a module
//    int numtag;
//    char text[0x0100];
//    int x, y, stt.y;
//    float open;
//    int cnt;
//    int module;
//    int stillchoosing;
//    if(hostactive)
//    {
//        // Figure out how many tags to display
//        numtag = (scry-4-40)/132;
//        stt.y = (scry-(numtag*132)-4)>>1;
//
//        // Open the tag windows
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, scrx, scry, open);
//            y = stt.y;
//            cnt = 0;
//            while(cnt < numtag)
//            {
//                draw_trim_box_opening(0, y, 136, y+136, open);
//                draw_trim_box_opening(132, y, scrx, y+136, open);
//                y+=132;
//                cnt++;
//            }
//            flip_pages();
//            open += .030;
//        }
//
//        // Let the user pick a module
//        module = 0;
//        stillchoosing = true;
//        while(stillchoosing)
//        {
//            // Draw the tags
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, scrx, scry);
//            y = stt.y;
//            cnt = 0;
//            while(cnt < numtag)
//            {
//                draw_module_tag(module+cnt, y);
//                y+=132;
//                cnt++;
//            }
//
//            // Draw the Up/Down buttons
//            sprintf(text, "Up");
//            x = (scrx-40)>>1;
//            draw_string(text, x, 10);
//            sprintf(text, "Down");
//            x = (scrx-80)>>1;
//            draw_string(text, x, scry-GFont.spacing_y-20);
//
//            // Handle the mouse
//            do_cursor();
//            y = (GUI.cursory - stt.y)/132;
//            if(GUI.pending_click)
//       {
//         GUI.pending_click=false;
//                if(GUI.cursory < stt.y && module > 0)
//    {
//                    // Up button
//                    module--;
//    }
//                if(y >= numtag && module + numtag < Module::globalnum)
//    {
//                    // Down button
//                    module++;
//    }
//  if(GUI.cursory > stt.y && y > -1 && y < numtag)
//    {
//      y = module + y;
//      if((GMous.button[0] || GMous.button[1]) && y < Module::globalnum)
//        {
//   // Set start infow
//   playersready = 1;
//   seed = time(0);
//   pickedindex = y;
//   sprintf(pickedmodule, "%s", ModList[y].loadname);
//   readytostart = true;
//   stillchoosing = false;
//        }
//    }
//       }
//            // Check for quitters
//            if(GKeyb.pressed(SDLK_ESCAPE) && GNet.workservice == NONETWORK)
//            {
//                nextmenu = MENUB;
//                menuactive = false;
//                stillchoosing = false;
//                gameactive = false;
//            }
//            flip_pages();
//        }
//    }
//    nextmenu = MENUE;
//}
//
////--------------------------------------------------------------------------------------------
//void menu_boot_players()
//{
//    // ZZ> This function shows all the active players and lets the host kick 'em out
//    //     !!!BAD!!!  Let the host boot players
//    char text[0x0100];
//    int x, y, starttime, time;
//    float open;
//    int cnt, player;
//    int stillchoosing;
//
//    GNet.numplayer = 1;
//    if(GNet.on)
//    {
//        // Find players
//        sv_letPlayersJoin();
//
//        // Open a big window
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, scrx, scry, open);
//            draw_trim_box_opening(0, 0, 320, GFont.spacing_y*(GNet.numplayer+4), open);
//            flip_pages();
//            open += .030;
//        }
//
//        // Tell the user which ones we found
//        starttime = SDL_GetTicks();
//        stillchoosing = true;
//        while(stillchoosing)
//        {
//   time = SDL_GetTicks();
//   if((time-starttime) > NETREFRESH)
//   {
//    sv_letPlayersJoin();
//    starttime = time;
//   }
//
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box(0, 0, scrx, scry);
//   draw_trim_box(0, 0, 320, GFont.spacing_y*(GNet.numplayer+4));
//
//   if(hostactive)
//   {
//    y = 8;
//    sprintf(text, "Active machines...");
//    draw_string(text, 14, y);
//    y += GFont.spacing_y;
//
//    cnt = 0;
//    while(cnt < GNet.numplayer)
//    {
//     sprintf(text, "%s", GNet.playername[cnt]);
//     draw_string(text, 50, y);
//     y += GFont.spacing_y;
//     cnt++;
//    }
//
//    sprintf(text, "Start Game");
//                draw_string(text, 50, y);
//   } else
//   {
//    strncpy(text, "Connected to host:", 0x0100);
//    draw_string(text, 14, 8);
//    draw_string(GNet.hostname, 14, 8 + GFont.spacing_y);
//    listen_for_packets();  // This happens implicitly for the host in sv_letPlayersJoin
//   }
//
//            do_cursor();
//            x = GUI.cursorx - 50;
//   // Again, y adjustments were figured out empirically in menu_start_or_join
//            y = (GUI.cursory - 21 - GFont.spacing_y);
//
//            if(GKeyb.pressed(SDLK_ESCAPE)) // !!!BAD!!!
//            {
//                nextmenu = MENUB;
//                menuactive = false;
//                stillchoosing = false;
//                gameactive = false;
//            }
//            if(x > 0 && x < 300 && y >= 0 && (GMous.button[0] || GMous.button[1]) && hostactive)
//            {
//                // Let the host do things
//                y = y/GFont.spacing_y;
//                if(y < GNet.numplayer && hostactive)
//                {
//                    // Boot players
//                }
//                if(y == GNet.numplayer && readytostart)
//                {
//                    // Start the modules
//                    stillchoosing = false;
//                }
//            }
//            if(readytostart && !hostactive)
//            {
//                // Remotes automatically start
//                stillchoosing = false;
//            }
//            flip_pages();
//        }
//    }
//    if(GNet.on && hostactive)
//    {
//        // Let the host coordinate start
//        stop_players_from_joining();
//        sv_letPlayersJoin();
//        cnt = 0;
//        readytostart = false;
//        if(GNet.numplayer == 1)
//        {
//            // Don't need to bother, since the host is alone
//            readytostart = true;
//        }
//        while(!readytostart)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, scrx, scry);
//            y = 8;
//            sprintf(text, "Waiting for replies...");
//            draw_string(text, 14, y);
//            y += GFont.spacing_y;
//            do_cursor();
//            if(GKeyb.pressed(SDLK_ESCAPE)) // !!!BAD!!!
//            {
//                nextmenu = MENUB;
//                menuactive = false;
//                stillchoosing = false;
//                gameactive = false;
//                readytostart = true;
//            }
//            if((cnt&0x3F)==0)
//            {
//                sprintf(text, "  Lell...");
//                draw_string(text, 14, y);
//                player = 0;
//                while(player < GNet.numplayer-1)
//                {
//                    net_startNewPacket();
//                    packet_addUnsignedShort(TO_REMOTE_MODULE);
//                    packet_addUnsignedInt(seed);
//                    packet_addUnsignedByte(player+1);
//                    packet_addString(pickedmodule);
////                    send_packet_to_all_players();
//                    net_sendPacketToOnePlayerGuaranteed(player);
//                    player++;
//                }
//            }
//            listen_for_packets();
//            cnt++;
//            flip_pages();
//        }
//    }
//
//    nextmenu=MENUF;
//}
//
////--------------------------------------------------------------------------------------------
//void menu_end_text()
//{
//    // ZZ> This function gives the player the ending text
//    float open;
//    int stillchoosing;
////    SDL_Event ev;
//
//    // Open the text window
//    open = 0;
//    while(open < 1.0)
//    {
//        //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//        draw_trim_box_opening(0, 0, scrx, scry, open);
//        flip_pages();
//        open += .030;
//    }
//
//    // Wait for input
//    stillchoosing = true;
//    while(stillchoosing)
//    {
//        // Show the text
//        //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//        draw_trim_box(0, 0, scrx, scry);
//        draw_wrap_string(endtext, 14, 8, scrx-40);
//
//        // Handle the mouse
//        do_cursor();
//        if(GUI.pending_click || GKeyb.pressed(SDLK_ESCAPE))
//        {
//     GUI.pending_click = false;
//            stillchoosing = false;
//        }
//        flip_pages();
//    }
//    nextmenu = MENUB;
//}
//
////--------------------------------------------------------------------------------------------
//void menu_initial_text()
//{
//    // ZZ> This function gives the player the initial title screen
//    float open;
//    char text[0x0400];
//    int stillchoosing;
//
//    //fprintf(stderr,"DIAG: In menu_initial_text()\n");
//    //draw_trim_box(0, 0, scrx, scry);//draw_trim_box(60, 60, 320, 200); // JUST TEST BOX
//
//    // Open the text window
//    open = 0;
//    while(open < 1.0)
//    {
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glLoadIdentity();
//
//        // clear_surface(lpDDSBack); PORT!
//        draw_trim_box_opening(0, 0, scrx, scry, open);
//        flip_pages();
//        open += .030;
//    }
//
// /*fprintf(stderr,"waiting to read a scanf\n");
//    scanf("%s",text);
//    exit(0);*/
//
//    // Wait for input
//    stillchoosing = true;
//    while(stillchoosing)
//    {
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glLoadIdentity();
//
//        // Show the text
//        // clear_surface(lpDDSBack); PORT!
//        draw_trim_box(0, 0, scrx, scry);
//        sprintf(text, "Egoboo v2.22");
//        draw_string(text, (scrx>>1)-200, ((scry>>1)-30));
//        sprintf(text, "http://egoboo.sourceforge.net");
//        draw_string(text, (scrx>>1)-200, ((scry>>1)));
//        sprintf(text, "See controls.txt to configure input");
//        draw_string(text, (scrx>>1)-200, ((scry>>1)+30));
//
//  // get input
//  read_input(NULL);
//
//        // Handle the mouse
//        do_cursor();
//  if ( GUI.pending_click || GKeyb.pressed(SDLK_ESCAPE) )
//  {
//          GUI.pending_click = false;
//   stillchoosing = false;
//  }
//        flip_pages();
//    }
//    nextmenu = MENUA;
//}

//--------------------------------------------------------------------------------------------
//void fiddle_with_menu()
//{
//    // ZZ> This function gives a nice little menu to play around in.
//
//    menuactive = true;
//    readytostart = false;
//    playersready = 0;
//    localmachine = 0;
//    GRTS.team_local = 0;
//    numfile = 0;
//    numfilesent = 0;
//    numfileexpected = 0;
//    while(menuactive)
//    {
//        switch(nextmenu)
//        {
//            case MENUA:
//                // MENUA...  Let the user choose a network service
//                //printf("MENUA\n");
//                if(menuaneeded)
//                {
//                    menu_service_select();
//                    menuaneeded = false;
//                }
//                nextmenu = MENUB;
//                break;
//            case MENUB:
//                // MENUB...  Let the user start or join
//                //printf("MENUB\n");
//                menu_start_or_join();
//                break;
//            case MENUC:
//                // MENUC...  Choose an open game to join
//                //printf("MENUC\n");
//                //menu_choose_host();
//    menu_join_multiplayer();
//                break;
//            case MENUD:
//                // MENUD...  Choose a module to run
//                //printf("MENUD\n");
//                menu_choose_module();
//                break;
//            case MENUE:
//                // MENUE...  Wait for all the players
//                //printf("MENUE\n");
//                menu_boot_players();
//                break;
//            case MENUF:
//                // MENUF...  Let the players choose characters
//                //printf("MENUF\n");
//                menu_pick_player(pickedindex);
//                break;
//            case MENUG:
//                // MENUG...  Let the user read while it loads
//                //printf("MENUG\n");
//                menu_module_loading(pickedindex);
//                break;
//            case MENUH:
//                // MENUH...  Show the end text
//                //printf("MENUH\n");
//                menu_end_text();
//                break;
//            case MENUI:
//                // MENUI...  Show the initial text
//                //printf("MENUI\n");
//                menu_initial_text();
//                break;
//        }
//    }
//    //printf("Left menu system\n");
//}
//
//--------------------------------------------------------------------------------------------
//void release_menu_trim()
//{
// // ZZ> This function frees the menu trim memory
// //GLTexture::Release( &TxTrimX );  //RELEASE(lpDDSTrimX);
// //GLTexture::Release( &TxTrimY );  //RELEASE(lpDDSTrimY);
// GLTexture::Release( &TxBlip );  //RELEASE(lpDDSBlip);
// GLTexture::Release( &TxTrim );
//
//}
//
//--------------------------------------------------------------------------------------------
//void release_menu()
//{
// // ZZ> This function releases all the menu images
// GLTexture::Release( &GFont.texture );  //RELEASE(lpDDSFont);
//    release_all_titleimages();
//    IconList.release_all_icons();
//
//}
#endif