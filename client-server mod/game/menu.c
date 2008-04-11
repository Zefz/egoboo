/* Egoboo - menu.c
 * Implements the main menu tree, using the code in Ui.*
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

#include "egobootypedef.h"
#include "egoboo.h"
#include "Ui.h"
#include "Menu.h"
#include "Log.h"
#include "Client.h"
#include "Server.h"
#include <assert.h>

static STRING filternamebuffer    = {0};
static STRING display_mode_buffer = {0};
static int  display_mode_index = 0;

//--------------------------------------------------------------------------------------------
// New menu code
//--------------------------------------------------------------------------------------------

// All the different menus.  yay!
enum menus_e
{
  MainMenu,
  SinglePlayer,
  MultiPlayer,
  NetworkMenu,
  HostGameMenu,
  UnhostGameMenu,
  JoinGameMenu,
  ChooseModule,
  ChoosePlayer,
  TestResults,
  Options,
  VideoOptions,
  AudioOptions,
  InputOptions,
  NewPlayer,
  LoadPlayer,
  HostGame,
  JoinGame,
  Inventory
};


enum MenuStates
{
  MM_Begin,
  MM_Entering,
  MM_Running,
  MM_Leaving,
  MM_Finish,
};

/* Variables for the model viewer in doChoosePlayer */
static float    modelAngle = 0;
static Uint32   modelIndex = 0;

/* Copyright text variables.  Change these to change how the copyright text appears */
const char copyrightText[] = "Welcome to Egoboo!\nhttp://home.no.net/egoboo\nVersion 2.7.5";
static int copyrightLeft = 0;
static int copyrightTop  = 0;

/* Options info text variables.  Change these to change how the options text appears */
const char optionsText[] = "Change your audio, input and video\nsettings here.";
static int optionsTextLeft = 0;
static int optionsTextTop  = 0;

/* Button labels.  Defined here for consistency's sake, rather than leaving them as constants */
const char *mainMenuButtons[] =
{
  "Single Player",
  "Multi-player",
  "Network Game",
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

const char *netMenuButtons[] =
{
  "Host Game",
  "Join Game",
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

const char *audioOptionsButtons[] =
{
  "N/A",    //Enable sound
  "N/A",    //Sound volume
  "N/A",    //Enable music
  "N/A",    //Music volume
  "N/A",    //Sound channels
  "N/A",    //Sound buffer
  "Save Settings",
  ""
};

const char *videoOptionsButtons[] =
{
  "N/A",  //Antialaising
  "N/A",  //Color depth
  "N/A",  //Fast & ugly
  "N/A",  //Fullscreen
  "N/A",  //Reflections
  "N/A",  //Texture filtering
  "N/A",  //Shadows
  "N/A",  //Z bit
  "N/A",  //Fog
  "N/A",  //3D effects
  "N/A",  //Multi water layer
  "N/A",  //Max messages
  "N/A",  //Screen resolution
  "Save Settings",
  ""
};


/* Button position for the "easy" menus, like the main one */
static int buttonLeft = 0;
static int buttonTop = 0;

static bool_t startNewPlayer = bfalse;

/* The font used for drawing text.  It's smaller than the button font */
Font *menuFont = NULL;

// "Slidy" buttons used in some of the menus.  They're shiny.
struct slidybuttonstate_t
{
  float lerp;
  int top;
  int left;
  char **buttons;
} SlidyButtonState;

int doHostGameMenu(ServerState_t * ss, float deltaTime);
int doUnhostGameMenu(ServerState_t * ss, float deltaTime);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void initSlidyButtons(float lerp, const char *buttons[])
{
  SlidyButtonState.lerp = lerp;
  SlidyButtonState.buttons = (char**)buttons;
}

//--------------------------------------------------------------------------------------------
static void updateSlidyButtons(float deltaTime)
{
  SlidyButtonState.lerp += (deltaTime * 1.5f);
}

//--------------------------------------------------------------------------------------------
static void drawSlidyButtons()
{
  int i;

  for (i = 0; SlidyButtonState.buttons[i][0] != 0; i++)
  {
    int x = buttonLeft - (360 - i * 35)  * SlidyButtonState.lerp;
    int y = buttonTop + (i * 35);

    ui_doButton(UI_Nothing, SlidyButtonState.buttons[i], x, y, 200, 30);
  }
}

//--------------------------------------------------------------------------------------------
/** initMenus
 * Loads resources for the menus, and figures out where things should
 * be positioned.  If we ever allow changing resolution on the fly, this
 * function will have to be updated/called more than once.
 */
bool_t initMenus()
{
  int i;

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.uifont_ttf);
  menuFont = fnt_loadFont(CStringTmp1, CData.uifont_points2);
  if (!menuFont)
  {
    log_error("Could not load the menu font!\n");
    return bfalse;
  }

  // Figure out where to draw the buttons
  buttonLeft = 40;
  buttonTop = displaySurface->h - 20;
  for (i = 0; mainMenuButtons[i][0] != 0; i++)
  {
    buttonTop -= 35;
  }

  // Figure out where to draw the copyright text
  copyrightLeft = 0;
  copyrightLeft = 0;
  fnt_getTextBoxSize(menuFont, copyrightText, 20, &copyrightLeft, &copyrightTop);
  // Draw the copyright text to the right of the buttons
  copyrightLeft = 280;
  // And relative to the bottom of the screen
  copyrightTop = displaySurface->h - copyrightTop - 20;

  // Figure out where to draw the options text
  optionsTextLeft = 0;
  optionsTextLeft = 0;
  fnt_getTextBoxSize(menuFont, optionsText, 20, &optionsTextLeft, &optionsTextTop);
  // Draw the copyright text to the right of the buttons
  optionsTextLeft = 280;
  // And relative to the bottom of the screen
  optionsTextTop = displaySurface->h - optionsTextTop - 20;

  return btrue;
}

//--------------------------------------------------------------------------------------------
int doMainMenu(float deltaTime)
{
  static int menuState = MM_Begin;
  static GLTexture background;
  static float lerp;
  static int menuChoice = 0;

  int result = 0;

  switch (menuState)
  {
  case MM_Begin:
    // set up menu variables
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_main_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_TEXTURE);
    menuChoice = 0;
    menuState = MM_Entering;

    initSlidyButtons(1.0f, mainMenuButtons);
    // let this fall through into MM_Entering

  case MM_Entering:
    // do buttons sliding in animation, and background fading in
    // background
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    drawSlidyButtons();
    updateSlidyButtons(-deltaTime);

    // Let lerp wind down relative to the time elapsed
    if (SlidyButtonState.lerp <= 0.0f)
    {
      menuState = MM_Running;
    }

    break;

  case MM_Running:
    // Do normal run
    // Background
    glColor4f(1, 1, 1, 1);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    // Buttons
    if (ui_doButton(1, mainMenuButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
    {
      // begin single player stuff
      menuChoice = 1;
    }

    if (ui_doButton(2, mainMenuButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
    {
      // begin multi player stuff
      menuChoice = 2;
    }

    if (ui_doButton(3, mainMenuButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
    {
      // begin networked stuff
      menuChoice = 3;
    }

    if (ui_doButton(4, mainMenuButtons[3], buttonLeft, buttonTop + 35 * 3, 200, 30) == 1)
    {
      // go to options menu
      menuChoice = 4;
    }

    if (ui_doButton(5, mainMenuButtons[4], buttonLeft, buttonTop + 35 * 4, 200, 30) == 1)
    {
      // quit game
      menuChoice = 5;
    }

    if (menuChoice != 0)
    {
      menuState = MM_Leaving;
      initSlidyButtons(0.0f, mainMenuButtons);
    }
    break;

  case MM_Leaving:
    // Do buttons sliding out and background fading
    // Do the same stuff as in MM_Entering, but backwards
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    // Buttons
    drawSlidyButtons();
    updateSlidyButtons(deltaTime);
    if (SlidyButtonState.lerp >= 1.0f)
    {
      menuState = MM_Finish;
    }
    break;

  case MM_Finish:
    // Free the background texture; don't need to hold onto it
    GLTexture_Release(&background);
    menuState = MM_Begin; // Make sure this all resets next time doMainMenu is called

    // Set the next menu to load
    result = menuChoice;
    break;

  };

  return result;
}

//--------------------------------------------------------------------------------------------
int doSinglePlayerMenu(float deltaTime)
{
  static int menuState = MM_Begin;
  static GLTexture background;
  static int menuChoice;
  int result = 0;

  switch (menuState)
  {
  case MM_Begin:
    // Load resources for this menu
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_advent_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    menuChoice = 0;

    menuState = MM_Entering;

    initSlidyButtons(1.0f, singlePlayerButtons);

    // Let this fall through

  case MM_Entering:
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);

    // Draw the background image
    ui_drawImage(0, &background, displaySurface->w - background.imgW, 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    drawSlidyButtons();
    updateSlidyButtons(-deltaTime);

    if (SlidyButtonState.lerp <= 0.0f)
      menuState = MM_Running;

    break;

  case MM_Running:

    // Draw the background image
    ui_drawImage(0, &background, displaySurface->w - background.imgW, 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    // Buttons
    if (ui_doButton(1, singlePlayerButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
    {
      menuChoice = 1;
    }

    if (ui_doButton(2, singlePlayerButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
    {
      menuChoice = 2;
    }

    if (ui_doButton(3, singlePlayerButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
    {
      menuChoice = 3;
    }

    if (menuChoice != 0)
    {
      menuState = MM_Leaving;
      initSlidyButtons(0.0f, singlePlayerButtons);
    }

    break;

  case MM_Leaving:
    // Do buttons sliding out and background fading
    // Do the same stuff as in MM_Entering, but backwards
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
    ui_drawImage(0, &background, displaySurface->w - background.imgW, 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    drawSlidyButtons();
    updateSlidyButtons(deltaTime);

    if (SlidyButtonState.lerp >= 1.0f)
    {
      menuState = MM_Finish;
    }
    break;

  case MM_Finish:
    // Release the background texture
    GLTexture_Release(&background);

    // Set the next menu to load
    result = menuChoice;

    // And make sure that if we come back to this menu, it resets
    // properly
    menuState = MM_Begin;
  }

  return result;
}

//--------------------------------------------------------------------------------------------
int doNetworkMenu(NET_STATE * ns, float deltaTime)
{
  static int menuState = MM_Begin;
  static GLTexture background;
  static float lerp;
  static int menuChoice = 0;
  int result = 0;

  if(!ns->networkon) return 3;

  switch (menuState)
  {
  case MM_Begin:
    // set up menu variables
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_main_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_TEXTURE);
    menuChoice = 0;
    menuState = MM_Entering;

    initSlidyButtons(1.0f, netMenuButtons);
    // let this fall through into MM_Entering

  case MM_Entering:
    // do buttons sliding in animation, and background fading in
    // background
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    drawSlidyButtons();
    updateSlidyButtons(-deltaTime);

    // Let lerp wind down relative to the time elapsed
    if (SlidyButtonState.lerp <= 0.0f)
    {
      menuState = MM_Running;
    }

    break;

  case MM_Running:
    // Do normal run
    // Background
    glColor4f(1, 1, 1, 1);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    // Buttons
    if (ui_doButton(1, netMenuButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
    {
      // begin single player stuff
      menuChoice = 1;
    }

    if (ui_doButton(2, netMenuButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
    {
      // begin multi player stuff
      menuChoice = 2;
    }

    if (ui_doButton(3, netMenuButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
    {
      // begin networked stuff
      menuChoice = 3;
    }

    if (menuChoice != 0)
    {
      menuState = MM_Leaving;
      initSlidyButtons(0.0f, netMenuButtons);
    }
    break;

  case MM_Leaving:
    // Do buttons sliding out and background fading
    // Do the same stuff as in MM_Entering, but backwards
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

    // Buttons
    drawSlidyButtons();
    updateSlidyButtons(deltaTime);
    if (SlidyButtonState.lerp >= 1.0f)
    {
      menuState = MM_Finish;
    }
    break;

  case MM_Finish:
    // Free the background texture; don't need to hold onto it
    GLTexture_Release(&background);
    menuState = MM_Begin; // Make sure this all resets next time doMainMenu is called

    // Set the next menu to load
    result = menuChoice;
    break;
  };

  return result;
}

//--------------------------------------------------------------------------------------------
// TODO: I totally fudged the layout of this menu by adding an offset for when
// the game isn't in 640x480.  Needs to be fixed.
int doChooseModule(GAME_STATE * gs, float deltaTime)
{
  static int menuState = MM_Begin;
  static int nummodules = 0;
  static int startIndex;
  static GLTexture background;
  static int validModules[MAXMODULE];
  static int numValidModules;

  static int moduleMenuOffsetX;
  static int moduleMenuOffsetY;

  ClientState_t * cs = gs->cs;
  ServerState_t * ss = gs->ss;

  int result = 0;
  int i, x, y;
  char txtBuffer[128];


  switch (menuState)
  {
  case MM_Begin:

    prime_titleimage(gs->modules, MAXMODULE);
    nummodules = load_all_module_data(gs->modules, MAXMODULE);

    // Load font & background
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_sleepy_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    startIndex = 0;
    gs->selectedModule = -1;

    // Find the module's that we want to allow loading for.  If startNewPlayer
    // is true, we want ones that don't allow imports (e.g. starter modules).
    // Otherwise, we want modules that allow imports
    memset(validModules, 0, sizeof(int) * MAXMODULE);
    numValidModules = 0;
    for (i = 0;i < nummodules; i++)
    {
      if (gs->modules[i].importamount == 0)
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

    // Figure out at what offset we want to draw the module menu.
    moduleMenuOffsetX = (displaySurface->w - 640) / 2;
    moduleMenuOffsetY = (displaySurface->h - 480) / 2;

    menuState = MM_Entering;

    // fall through...

  case MM_Entering:
    menuState = MM_Running;

    // fall through for now...

  case MM_Running:
    // Draw the background
    glColor4f(1, 1, 1, 1);
    x = (displaySurface->w / 2) - (background.imgW / 2);
    y = displaySurface->h - background.imgH;
    ui_drawImage(0, &background, x, y, 0, 0);

    // Fudged offset here.. DAMN!  Doesn't work, as the mouse tracking gets skewed
    // I guess I'll do it the uglier way
    //glTranslatef(moduleMenuOffsetX, moduleMenuOffsetY, 0);


    // Draw the arrows to pick modules
    if (ui_doButton(1051, "<-", moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30))
    {
      startIndex--;
    }

    if (ui_doButton(1052, "->", moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30))
    {
      startIndex++;

      if (startIndex + 3 >= numValidModules)
      {
        startIndex = numValidModules - 3;
      }
    }

    // Clamp startIndex to 0
    startIndex = MAX(0, startIndex);

    // Draw buttons for the modules that can be selected
    x = 93;
    y = 20;
    for (i = startIndex; i < (startIndex + 3) && i < numValidModules; i++)
    {
      if (ui_doImageButton(i, &TxTitleImage[validModules[i]],
                           moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138))
      {
        gs->selectedModule = i;
      }

      x += 138 + 20; // Width of the button, and the spacing between buttons
    }

    // Draw an unused button as the backdrop for the text for now
    ui_drawButton(0xFFFFFFFF, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230);

    // And draw the next & back buttons
    if (ui_doButton(53, "Select Module",
                    moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30))
    {
      // go to the next menu with this module selected
      menuState = MM_Leaving;
    }

    if (ui_doButton(54, "Back", moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30))
    {
      // Signal doMenu to go back to the previous menu
      gs->selectedModule = -1;
      menuState = MM_Leaving;
    }

    // Draw the text description of the selected module
    if (gs->selectedModule > -1)
    {
      y = 173 + 5;
      x = 21 + 5;
      glColor4f(1, 1, 1, 1);
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
                   gs->modules[validModules[gs->selectedModule]].longname);
      y += 20;

      snprintf(txtBuffer, sizeof(txtBuffer), "Difficulty: %s", gs->modules[validModules[gs->selectedModule]].rank);
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      if (gs->modules[validModules[gs->selectedModule]].maxplayers > 1)
      {
        if (gs->modules[validModules[gs->selectedModule]].minplayers == gs->modules[validModules[gs->selectedModule]].maxplayers)
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d Players", gs->modules[validModules[gs->selectedModule]].minplayers);
        }
        else
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d - %d Players", gs->modules[validModules[gs->selectedModule]].minplayers, gs->modules[validModules[gs->selectedModule]].maxplayers);
        }
      }
      else
      {
        snprintf(txtBuffer, sizeof(txtBuffer), "Starter Module");
      }
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      // And finally, the summary
      snprintf(txtBuffer, sizeof(txtBuffer), "%s/%s/%s/%s", CData.modules_dir, gs->modules[validModules[gs->selectedModule]].loadname, CData.gamedat_dir, CData.menu_file);
      get_module_summary(txtBuffer, &gs->modtxt);
      for (i = 0;i < SUMMARYLINES;i++)
      {
        fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, gs->modtxt.summary[i]);
        y += 20;
      }
    }

    break;

  case MM_Leaving:
    menuState = MM_Finish;
    // fall through for now

  case MM_Finish:
    GLTexture_Release(&background);

    menuState = MM_Begin;

    if (gs->selectedModule == -1)
    {
      result = -1;
    }
    else
    {
      // Copy all of the info over
      memcpy(&(gs->mod), &(gs->modules[validModules[gs->selectedModule]]), sizeof(MOD_INFO));

      // initialize the module state
      init_mod_state(&(gs->modstate), &(gs->mod), gs->randie_index);

      result = 1;
    }
    break;

  }

  return result;
}

//--------------------------------------------------------------------------------------------
int doUnhostGameMenu(ServerState_t * ss, float deltaTime)
{
  static int menuState = MM_Begin;
  static int nummodules = 0;
  static int startIndex;
  static GLTexture background;
  static int validModules[MAXMODULE];
  static int numValidModules;

  static int moduleMenuOffsetX;
  static int moduleMenuOffsetY;

  int result = 0;
  int i, x, y;
  char txtBuffer[128];

  if(!ss->ns->networkon || -1 == ss->selectedModule) return -1;

  switch (menuState)
  {
  case MM_Begin:
    // Load font & background
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_sleepy_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    startIndex = 0;

    // Find the module's that we want to allow hosting for.
    // Only modules taht allow importing AND have modmaxplayers > 1
    memset(validModules, 0, sizeof(int) * MAXMODULE);
    numValidModules = 0;
    for (i = 0;i < nummodules; i++)
    {
      if (ss->loc_mod[i].importamount > 0 && ss->loc_mod[i].maxplayers > 1)
      {
        validModules[numValidModules] = i;
        numValidModules++;
      }
    }

    // Figure out at what offset we want to draw the module menu.
    moduleMenuOffsetX = (displaySurface->w - 640) / 2;
    moduleMenuOffsetY = (displaySurface->h - 480) / 2;

    menuState = MM_Entering;

    // fall through...

  case MM_Entering:
    menuState = MM_Running;

    // fall through for now...

  case MM_Running:
    // Draw the background
    glColor4f(1, 1, 1, 1);
    x = (displaySurface->w / 2) - (background.imgW / 2);
    y = displaySurface->h - background.imgH;
    ui_drawImage(0, &background, x, y, 0, 0);

    // Clamp startIndex to 0
    startIndex = ss->selectedModule;

    // Draw buttons for the modules that can be selected
    x = 93;
    y = 20;
    ui_doImageButton(2, &TxTitleImage[ss->mod.texture_idx],
                          moduleMenuOffsetX + 138+20, moduleMenuOffsetY + y, 138, 138);

    // Draw an unused button as the backdrop for the text for now
    ui_drawButton(0xFFFFFFFF, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230);

    // And draw the next & back buttons
    if (ui_doButton(53, "Unhost Module", moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30))
    {
      // go to the next menu with this module selected
      result = 1;
      menuState = MM_Leaving;
    }

    if (ui_doButton(54, "Back", moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30))
    {
      // Signal doMenu to go back to the previous menu
      result = 2;
      menuState = MM_Leaving;
    }

    // Draw the text description of the selected module
    if (ss->selectedModule > -1)
    {
      y = 173 + 5;
      x = 21 + 5;
      glColor4f(1, 1, 1, 1);
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
                   ss->mod.longname);
      y += 20;

      snprintf(txtBuffer, sizeof(txtBuffer), "Difficulty: %s", ss->mod.rank);
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      if (ss->mod.maxplayers > 1)
      {
        if (ss->mod.minplayers == ss->mod.maxplayers)
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d Players", ss->mod.minplayers);
        }
        else
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d - %d Players", ss->mod.minplayers, ss->mod.maxplayers);
        }
      }
      else
      {
        snprintf(txtBuffer, sizeof(txtBuffer), "Starter Module");
      }
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      // And finally, the summary
      snprintf(txtBuffer, sizeof(txtBuffer), "%s/%s/%s/%s", CData.modules_dir, ss->mod.loadname, CData.gamedat_dir, CData.menu_file);
      get_module_summary(txtBuffer, &ss->loc_modtxt);
      for (i = 0;i < SUMMARYLINES;i++)
      {
        fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, ss->loc_modtxt.summary[i]);
        y += 20;
      }
    }

    break;

  case MM_Leaving:
    menuState = MM_Finish;
    // fall through for now

  case MM_Finish:
    GLTexture_Release(&background);

    ss->selectedModule = -1;

    result = 1;

    menuState = MM_Begin;
    break;

  }

  return result;
}




//--------------------------------------------------------------------------------------------
int doHostGameMenu(ServerState_t * ss, float deltaTime)
{
  static int menuState = MM_Begin;
  static int nummodules = 0;
  static int startIndex;
  static GLTexture background;
  static int validModules[MAXMODULE];
  static int numValidModules;

  static int moduleMenuOffsetX;
  static int moduleMenuOffsetY;

  int result = 0;
  int i, x, y;
  char txtBuffer[128];

  if(!ss->ns->networkon) return -1;

  switch (menuState)
  {
  case MM_Begin:
    // Load font & background
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_sleepy_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    startIndex = 0;
    ss->selectedModule = -1;

    prime_titleimage(ss->loc_mod, MAXMODULE);
    nummodules = load_all_module_data(ss->loc_mod, MAXMODULE);


    // Find the module's that we want to allow hosting for.
    // Only modules taht allow importing AND have modmaxplayers > 1
    memset(validModules, 0, sizeof(int) * MAXMODULE);
    numValidModules = 0;
    for (i = 0;i < nummodules; i++)
    {
      if (ss->loc_mod[i].importamount > 0 && ss->loc_mod[i].maxplayers > 1)
      {
        validModules[numValidModules] = i;
        numValidModules++;
      }
    }

    // Figure out at what offset we want to draw the module menu.
    moduleMenuOffsetX = (displaySurface->w - 640) / 2;
    moduleMenuOffsetY = (displaySurface->h - 480) / 2;

    menuState = MM_Entering;

    // fall through...

  case MM_Entering:
    menuState = MM_Running;

    // fall through for now...

  case MM_Running:
    // Draw the background
    glColor4f(1, 1, 1, 1);
    x = (displaySurface->w / 2) - (background.imgW / 2);
    y = displaySurface->h - background.imgH;
    ui_drawImage(0, &background, x, y, 0, 0);

    // Draw the arrows to pick modules
    if (ui_doButton(1051, "<-", moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30))
    {
      startIndex--;
    }

    if (ui_doButton(1052, "->", moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30))
    {
      startIndex++;

      if (startIndex + 3 >= numValidModules)
      {
        startIndex = numValidModules - 3;
      }
    }

    // Clamp startIndex to 0
    startIndex = MAX(0, startIndex);

    // Draw buttons for the modules that can be selected
    x = 93;
    y = 20;
    for (i = startIndex; i < (startIndex + 3) && i < numValidModules; i++)
    {
      if (ui_doImageButton(i, &TxTitleImage[ss->loc_mod[validModules[i]].texture_idx],
                           moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138))
      {
        ss->selectedModule = i;
      }

      x += 138 + 20; // Width of the button, and the spacing between buttons
    }

    // Draw an unused button as the backdrop for the text for now
    ui_drawButton(0xFFFFFFFF, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230);

    // And draw the next & back buttons
    if (ui_doButton(53, "Host Module", moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30))
    {
      // go to the next menu with this module selected
      menuState = MM_Leaving;
    }

    if (ui_doButton(54, "Back", moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30))
    {
      // Signal doMenu to go back to the previous menu
      ss->selectedModule = -1;
      result = 2;
      menuState = MM_Leaving;
    }

    // Draw the text description of the selected module
    if (ss->selectedModule > -1)
    {
      y = 173 + 5;
      x = 21 + 5;
      glColor4f(1, 1, 1, 1);
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
                   ss->loc_mod[validModules[ss->selectedModule]].longname);
      y += 20;

      snprintf(txtBuffer, sizeof(txtBuffer), "Difficulty: %s", ss->loc_mod[validModules[ss->selectedModule]].rank);
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      if (ss->loc_mod[validModules[ss->selectedModule]].maxplayers > 1)
      {
        if (ss->loc_mod[validModules[ss->selectedModule]].minplayers == ss->loc_mod[validModules[ss->selectedModule]].maxplayers)
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d Players", ss->loc_mod[validModules[ss->selectedModule]].minplayers);
        }
        else
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d - %d Players", ss->loc_mod[validModules[ss->selectedModule]].minplayers, ss->loc_mod[validModules[ss->selectedModule]].maxplayers);
        }
      }
      else
      {
        snprintf(txtBuffer, sizeof(txtBuffer), "Starter Module");
      }
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      // And finally, the summary
      snprintf(txtBuffer, sizeof(txtBuffer), "%s/%s/%s/%s", CData.modules_dir, ss->loc_mod[validModules[ss->selectedModule]].loadname, CData.gamedat_dir, CData.menu_file);
      get_module_summary(txtBuffer, &ss->loc_modtxt);
      for (i = 0;i < SUMMARYLINES;i++)
      {
        fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, ss->loc_modtxt.summary[i]);
        y += 20;
      }
    }

    break;

  case MM_Leaving:
    menuState = MM_Finish;
    // fall through for now

  case MM_Finish:
    GLTexture_Release(&background);

    menuState = MM_Begin;

    if (ss->selectedModule == -1)
    {
      result = -1;
    }
    else
    {
      // Save the module info of the picked module
      memcpy(&(ss->mod), &(ss->loc_mod[validModules[ss->selectedModule]]), sizeof(MOD_INFO)); 
      result = 1;
    }
    break;

  }

  return result;
}

//--------------------------------------------------------------------------------------------
int doJoinGameMenu(GAME_STATE * gs, float deltaTime)
{
  static int menuState = MM_Begin;
  static int startIndex;
  static GLTexture background;
  static int validModules[MAXMODULE];
  static int numValidModules;

  static int moduleMenuOffsetX;
  static int moduleMenuOffsetY;

  int cnt;
  PACKET_REQUEST * prequest;
  retval_t wait_return;
  Uint8 * buffer[1024];
  PACKET egopkt;
  ENetPeer * peer;
  size_t data_size;
  STREAM stream;
  bool_t all_module_images_loaded = bfalse;

  ClientState_t * cs = gs->cs;


  int result = 0;
  int i, x, y;
  char txtBuffer[128];

  if(!gs->ns->networkon) return -1;

  switch (menuState)
  {
  case MM_Begin:
    // Load font & background
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_sleepy_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    startIndex = 0;
    cs->selectedModule = -1;

    // Grab the module info from all the servers
    memset(validModules, 0, sizeof(int) * MAXMODULE);
    numValidModules = 0;
    for (i = 0;i < MAXNETPLAYER; i++)
    {
      if(0x00 == CData.net_hosts[i][0]) continue;

      if(cl_connectHost(cs, CData.net_hosts[i]))
      {
        net_startNewPacket(&egopkt);
        packet_addUnsignedShort(&egopkt, TO_HOST_MODULE);
        net_sendPacketToHost(cs, &egopkt);

        // Wait to get a response
        wait_return = net_waitForPacket(cs->request_buffer, 16, cs->gamePeer, 5000, buffer, sizeof(buffer), TO_REMOTE_MODULEINFO, &data_size);
        cl_disconnectHost(cs);
        if(rv_fail == wait_return || rv_error == wait_return) continue;

        stream_startRaw(&stream, buffer, data_size);

        cs->rem_mod[numValidModules].is_hosted   = btrue;
        cs->rem_mod[numValidModules].is_verified = bfalse;
        assert(TO_REMOTE_MODULEINFO == stream_readUnsignedShort(&stream));
        strncpy(cs->rem_mod[numValidModules].host, CData.net_hosts[i], MAXCAPNAMESIZE);
        stream_readString(&stream, cs->rem_mod[numValidModules].rank, RANKSIZE);               // Number of stars
        stream_readString(&stream, cs->rem_mod[numValidModules].longname, MAXCAPNAMESIZE);     // Module names
        stream_readString(&stream, cs->rem_mod[numValidModules].loadname, MAXCAPNAMESIZE);     // Module load names
        cs->rem_mod[numValidModules].importamount = stream_readUnsignedByte(&stream);                 // # of import characters
        cs->rem_mod[numValidModules].allowexport  = stream_readUnsignedByte(&stream);                  // Export characters?
        cs->rem_mod[numValidModules].minplayers   = stream_readUnsignedByte(&stream);                   // Number of players
        cs->rem_mod[numValidModules].maxplayers   = stream_readUnsignedByte(&stream);                   //
        cs->rem_mod[numValidModules].monstersonly = stream_readUnsignedByte(&stream);                 // Only allow monsters
        cs->rem_mod[numValidModules].rts_control   = stream_readUnsignedByte(&stream);                   // Real Time Stragedy?
        cs->rem_mod[numValidModules].respawnvalid = stream_readUnsignedByte(&stream);                 // Allow respawn
        stream_done(&stream);
        validModules[numValidModules] = i;
        numValidModules++;
      };
    }

    prime_titleimage(cs->rem_mod, MAXMODULE);
    cl_request_module_images(cs);

    // Figure out at what offset we want to draw the module menu.
    moduleMenuOffsetX = (displaySurface->w - 640) / 2;
    moduleMenuOffsetY = (displaySurface->h - 480) / 2;

    menuState = MM_Entering;

    // fall through...

  case MM_Entering:
    menuState = MM_Running;

    // fall through for now...

  case MM_Running:

    // keep trying to load the module images as long as the client is still deciding
    if(!all_module_images_loaded)
      all_module_images_loaded = cl_load_module_images(cs);

    // Draw the background
    glColor4f(1, 1, 1, 1);
    x = (displaySurface->w / 2) - (background.imgW / 2);
    y = displaySurface->h - background.imgH;
    ui_drawImage(0, &background, x, y, 0, 0);

    // Draw the arrows to pick modules
    if (ui_doButton(1051, "<-", moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30))
    {
      startIndex--;
    }

    if (ui_doButton(1052, "->", moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30))
    {
      startIndex++;

      if (startIndex + 3 >= numValidModules)
      {
        startIndex = numValidModules - 3;
      }
    }

    // Clamp startIndex to 0
    startIndex = MAX(0, startIndex);

    // Draw buttons for the modules that can be selected
    x = 93;
    y = 20;
    for (i = startIndex; i < (startIndex + 3) && i < numValidModules; i++)
    {
      if (ui_doImageButton(i, &TxTitleImage[validModules[i]],
                           moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138))
      {
        cs->selectedModule = i;
      }

      x += 138 + 20; // Width of the button, and the spacing between buttons
    }

    // Draw an unused button as the backdrop for the text for now
    ui_drawButton(0xFFFFFFFF, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230);

    // And draw the next & back buttons
    if (ui_doButton(53, "Join Module", moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30))
    {
      // go to the next menu with this module selected
      cs->selectedModule = validModules[cs->selectedModule];
      menuState = MM_Leaving;
    }

    if (ui_doButton(54, "Back", moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30))
    {
      // Signal doMenu to go back to the previous menu
      cs->selectedModule = -1;
      menuState = MM_Leaving;
    }

    // Draw the text description of the selected module
    if (cs->selectedModule > -1)
    {
      y = 173 + 5;
      x = 21 + 5;
      glColor4f(1, 1, 1, 1);
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
                   cs->rem_mod[validModules[cs->selectedModule]].longname);
      y += 20;

      
      fnt_drawTextFormatted(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
        "Host : %s", cs->rem_mod[validModules[cs->selectedModule]].host);
      y += 20;

      fnt_drawTextFormatted(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, 
        "Difficulty: %s", cs->rem_mod[validModules[cs->selectedModule]].rank);
      y += 20;

      if (cs->rem_mod[validModules[cs->selectedModule]].maxplayers > 1)
      {
        if (cs->rem_mod[validModules[cs->selectedModule]].minplayers == cs->rem_mod[validModules[cs->selectedModule]].maxplayers)
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d Players", cs->rem_mod[validModules[cs->selectedModule]].minplayers);
        }
        else
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d - %d Players", cs->rem_mod[validModules[cs->selectedModule]].minplayers, cs->rem_mod[validModules[cs->selectedModule]].maxplayers);
        }
      }
      else
      {
        snprintf(txtBuffer, sizeof(txtBuffer), "Starter Module");
      }
      fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      // And finally, the summary
      snprintf(txtBuffer, sizeof(txtBuffer), "%s/%s/%s/%s", CData.modules_dir, cs->rem_mod[validModules[cs->selectedModule]].loadname, CData.gamedat_dir, CData.menu_file);
      get_module_summary(txtBuffer, &cs->rem_modtxt);
      for (i = 0;i < SUMMARYLINES;i++)
      {
        fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, cs->rem_modtxt.summary[i]);
        y += 20;
      }
    }

    break;

  case MM_Leaving:
    menuState = MM_Finish;
    // fall through for now

  case MM_Finish:
    GLTexture_Release(&background);

    menuState = MM_Begin;

    if (cs->selectedModule == -1)
    {
      result = 2;  // Back
    }
    else
    {
      // Save the module info of the picked module
      memcpy(&cs->req_mod, &cs->rem_mod[cs->selectedModule], sizeof(MOD_INFO));
      init_mod_state(&(cs->req_modstate), &(cs->req_mod), gs->randie_index);

      if( cl_connectHost(cs, cs->rem_mod[cs->selectedModule].host) )
      {
        net_startNewPacket(&egopkt);
        packet_addUnsignedShort(&egopkt, TO_HOST_REQUEST_MODULE);
        packet_addString(&egopkt, cs->req_mod.loadname);
        net_sendPacketToHost(cs, &egopkt);

        // wait for up to 2 minutes to transfer needed files
        // this probably needs to be longer
        wait_return = net_waitForPacket(cs->ns->request_buffer, 16, cs->gamePeer, 2*60000, buffer, sizeof(buffer), NET_DONE_SENDING_FILES, NULL);
        cl_disconnectHost(cs);
        if(rv_fail == wait_return || rv_error == wait_return) assert(bfalse);

        result = 1;
      }
      else
      {
        result = 0;
      }



      
      
    }
    break;

  }

  return result;
}

//--------------------------------------------------------------------------------------------
int doChoosePlayer(GAME_STATE * gs, float deltaTime)
{
  static int menuState = MM_Begin;
  static GLTexture background;
  int result = 0;
  int numVertical, numHorizontal;
  int i, j, x, y;
  int player;
  char srcDir[64], destDir[64];
  ClientState_t * cs = gs->cs;
  ServerState_t * ss = gs->ss;


  switch (menuState)
  {
  case MM_Begin:
    cs->selectedPlayer = 0;

    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_sleepy_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);

    // load information for all the players that could be imported
    check_player_import("players");

    menuState = MM_Entering;
    // fall through

  case MM_Entering:
    menuState = MM_Running;
    // fall through

  case MM_Running:
    // Figure out how many players we can show without scrolling
    numVertical = 6;
    numHorizontal = 2;

    // Draw the player selection buttons
    // I'm being tricky, and drawing two buttons right next to each other
    // for each player: one with the icon, and another with the name.  I'm
    // given those buttons the same ID, so they'll act like the same button.
    player = 0;
    x = 20;
    for (j = 0;j < numHorizontal && player < numloadplayer;j++)
    {
      y = 20;
      for (i = 0;i < numVertical && player < numloadplayer;i++)
      {
        if (ui_doImageButtonWithText(player, &TxIcon[player], loadplayername[player],  x, y, 175, 42))
        {
          cs->selectedPlayer = player;
        }

        player++;
        y += 47;
      }
      x += 180;
    }

    fnt_drawTextFormatted(menuFont, 20, 20, "%d models available", 5);

    // Draw the background
    x = (displaySurface->w / 2) - (background.imgW / 2);
    y = displaySurface->h - background.imgH;
    ui_drawImage(0, &background, x, y, 0, 0);


    // Buttons for going ahead
    if (ui_doButton(100, "Select Module", 40, 350, 200, 30))
    {
      menuState = MM_Leaving;
    }

    if (ui_doButton(101, "Back", 40, 385, 200, 30))
    {
      cs->selectedPlayer = -1;
      menuState = MM_Leaving;
    }

    break;

  case MM_Leaving:
    menuState = MM_Finish;
    // fall through

  case MM_Finish:
    GLTexture_Release(&background);
    menuState = MM_Begin;

    if (cs->selectedPlayer == -1) result = -1;
    else
    {
      // Build the import directory
      // I'm just allowing 1 player for now...
      empty_import_directory();
      fs_createDirectory("import");

      localcontrol[0] = INPUTKEY | INPUTMOUSE | INPUTJOYA;
      localslot[0] = localmachine * 9;

      // Copy the character to the import directory
      snprintf(srcDir, sizeof(srcDir), "players/%s", loadplayerdir[cs->selectedPlayer]);
      snprintf(destDir, sizeof(destDir), "import/temp%04d.obj", localslot[0]);
      fs_copyDirectory(srcDir, destDir);

      // Copy all of the character's items to the import directory
      for (i = 0;i < 8;i++)
      {
        snprintf(srcDir, sizeof(srcDir), "players/%s/%d.obj", loadplayerdir[cs->selectedPlayer], i);
        snprintf(destDir, sizeof(destDir), "import/temp%04d.obj", localslot[0] + i + 1);

        fs_copyDirectory(srcDir, destDir);
      }

      numimport = 1;
      result = 1;
    }

    break;

  }

  return result;
}


//--------------------------------------------------------------------------------------------
//int doModel(float deltaTime)
//{
//  // Advance the model's animation
//  m_modelLerp += (float)deltaTime / kAnimateSpeed;
//  if (m_modelLerp >= 1.0f)
//  {
//   m_modelFrame = m_modelNextFrame;
//   m_modelNextFrame++;
//
//   // Roll back to the first frame if we run out of them
//   if (m_modelNextFrame >= m_model->numFrames())
//   {
//    m_modelNextFrame = 0;
//   }
//
//   m_modelLerp -= 1.0f;
//  }
//
//  float width, height;
//  width  = (float)CData.scrx;
//  height = (float)CData.scry;
//
//    glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT|GL_TRANSFORM_BIT|GL_VIEWPORT_BIT|GL_COLOR_BUFFER_BIT);
//  glMatrixMode(GL_PROJECTION);                              /* GL_TRANSFORM_BIT */
//  glLoadIdentity();                                         /* GL_TRANSFORM_BIT */
//  gluPerspective(60.0, width/height, 1, 2048);              /* GL_TRANSFORM_BIT */
//
//  glMatrixMode(GL_MODELVIEW);                               /* GL_TRANSFORM_BIT */
//  glLoadIdentity();                                         /* GL_TRANSFORM_BIT */
//  glTranslatef(20, -20, -75);                               /* GL_TRANSFORM_BIT */
//  glRotatef(-90, 1, 0, 0);                                  /* GL_TRANSFORM_BIT */
//  glRotatef(modelAngle, 0, 0, 1);                         /* GL_TRANSFORM_BIT */
//
//  glEnable(GL_TEXTURE_2D);
//  glEnable(GL_DEPTH_TEST);
//
//    GLTexture_Bind( &m_modelTxr, CData.texturefilter );
//  m_model->drawBlendedFrames(m_modelFrame, m_modelNextFrame, m_modelLerp);
// }
//};

//TODO: This needs to be finished
//--------------------------------------------------------------------------------------------
int doOptions(float deltaTime)
{
  static int menuState = MM_Begin;
  static GLTexture background;
  static float lerp;
  static int menuChoice = 0;

  int result = 0;

  switch (menuState)
  {
  case MM_Begin:
    // set up menu variables
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_gnome_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    menuChoice = 0;
    menuState = MM_Entering;

    initSlidyButtons(1.0f, optionsButtons);
    // let this fall through into MM_Entering

  case MM_Entering:
    // do buttons sliding in animation, and background fading in
    // background
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);

    //Draw the background
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Copyright" text
    fnt_drawTextBox(menuFont, optionsText, optionsTextLeft, optionsTextTop, 0, 0, 20);

    drawSlidyButtons();
    updateSlidyButtons(-deltaTime);

    // Let lerp wind down relative to the time elapsed
    if (SlidyButtonState.lerp <= 0.0f)
    {
      menuState = MM_Running;
    }

    break;

  case MM_Running:
    // Do normal run
    // Background
    glColor4f(1, 1, 1, 1);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Options" text
    fnt_drawTextBox(menuFont, optionsText, optionsTextLeft, optionsTextTop, 0, 0, 20);

    // Buttons
    if (ui_doButton(1, optionsButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
    {
      //audio options
      menuChoice = 1;
    }

    if (ui_doButton(2, optionsButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
    {
      //input options
      menuChoice = 2;
    }

    if (ui_doButton(3, optionsButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
    {
      //video options
      menuChoice = 3;
    }

    if (ui_doButton(4, optionsButtons[3], buttonLeft, buttonTop + 35 * 3, 200, 30) == 1)
    {
      //back to main menu
      menuChoice = 4;
    }

    if (menuChoice != 0)
    {
      menuState = MM_Leaving;
      initSlidyButtons(0.0f, optionsButtons);
    }
    break;

  case MM_Leaving:
    // Do buttons sliding out and background fading
    // Do the same stuff as in MM_Entering, but backwards
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Options" text
    fnt_drawTextBox(menuFont, optionsText, optionsTextLeft, optionsTextTop, 0, 0, 20);

    // Buttons
    drawSlidyButtons();
    updateSlidyButtons(deltaTime);
    if (SlidyButtonState.lerp >= 1.0f)
    {
      menuState = MM_Finish;
    }
    break;

  case MM_Finish:
    // Free the background texture; don't need to hold onto it
    GLTexture_Release(&background);
    menuState = MM_Begin; // Make sure this all resets next time doMainMenu is called

    // Set the next menu to load
    result = menuChoice;
    break;

  }
  return result;
}

//--------------------------------------------------------------------------------------------
int doAudioOptions(float deltaTime)
{
  static int menuState = MM_Begin;
  static GLTexture background;
  static float lerp;
  static int menuChoice = 0;
  static char Cmaxsoundchannel[128];
  static char Cbuffersize[128];
  static char Csoundvolume[128];
  static char Cmusicvolume[128];

  int result = 0;

  switch (menuState)
  {
  case MM_Begin:
    // set up menu variables
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_gnome_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    menuChoice = 0;
    menuState = MM_Entering;
    // let this fall through into MM_Entering

  case MM_Entering:
    // do buttons sliding in animation, and background fading in
    // background
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);

    //Draw the background
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    //Load the current settings
    if (CData.soundvalid) audioOptionsButtons[0] = "On";
    else audioOptionsButtons[0] = "Off";

    snprintf(Csoundvolume, sizeof(Csoundvolume), "%i", CData.soundvolume);
    audioOptionsButtons[1] = Csoundvolume;

    if (CData.musicvalid) audioOptionsButtons[2] = "On";
    else audioOptionsButtons[2] = "Off";

    snprintf(Cmusicvolume, sizeof(Cmusicvolume), "%i", CData.musicvolume);
    audioOptionsButtons[3] = Cmusicvolume;

    snprintf(Cmaxsoundchannel, sizeof(Cmaxsoundchannel), "%i", CData.maxsoundchannel);
    audioOptionsButtons[4] = Cmaxsoundchannel;

    snprintf(Cbuffersize, sizeof(Cbuffersize), "%i", CData.buffersize);
    audioOptionsButtons[5] = Cbuffersize;

    //Fall trough
    menuState = MM_Running;
    break;

  case MM_Running:
    // Do normal run
    // Background
    glColor4f(1, 1, 1, 1);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    fnt_drawTextBox(menuFont, "Sound:", buttonLeft, displaySurface->h - 270, 0, 0, 20);
    // Buttons
    if (ui_doButton(1, audioOptionsButtons[0], buttonLeft + 150, displaySurface->h - 270, 100, 30) == 1)
    {
      if (CData.soundvalid)
      {
        CData.soundvalid = bfalse;
        audioOptionsButtons[0] = "Off";
      }
      else
      {
        CData.soundvalid = btrue;
        audioOptionsButtons[0] = "On";
      }
    }

    fnt_drawTextBox(menuFont, "Sound Volume:", buttonLeft, displaySurface->h - 235, 0, 0, 20);
    if (ui_doButton(2, audioOptionsButtons[1], buttonLeft + 150, displaySurface->h - 235, 100, 30) == 1)
    {
      snprintf(Csoundvolume, sizeof(Csoundvolume), "%i", CData.soundvolume);
      audioOptionsButtons[1] = Csoundvolume;
    }

    fnt_drawTextBox(menuFont, "Music:", buttonLeft, displaySurface->h - 165, 0, 0, 20);
    if (ui_doButton(3, audioOptionsButtons[2], buttonLeft + 150, displaySurface->h - 165, 100, 30) == 1)
    {
      if (CData.musicvalid)
      {
        CData.musicvalid = bfalse;
        audioOptionsButtons[2] = "Off";
      }
      else
      {
        CData.musicvalid = btrue;
        audioOptionsButtons[2] = "On";
      }
    }

    fnt_drawTextBox(menuFont, "Music Volume:", buttonLeft, displaySurface->h - 130, 0, 0, 20);
    if (ui_doButton(4, audioOptionsButtons[3], buttonLeft + 150, displaySurface->h - 130, 100, 30) == 1)
    {
      snprintf(Cmusicvolume, sizeof(Cmusicvolume), "%i", CData.musicvolume);
      audioOptionsButtons[3] = Cmusicvolume;
    }

    fnt_drawTextBox(menuFont, "Sound Channels:", buttonLeft + 300, displaySurface->h - 200, 0, 0, 20);
    if (ui_doButton(5, audioOptionsButtons[4], buttonLeft + 450, displaySurface->h - 200, 100, 30) == 1)
    {
      switch (CData.maxsoundchannel)
      {
      case 32:
        CData.maxsoundchannel = 8;
        break;

      case 8:
        CData.maxsoundchannel = 16;
        break;

      case 16:
        CData.maxsoundchannel = 24;
        break;

      case 24:
        CData.maxsoundchannel = 32;
        break;

      default:
        CData.maxsoundchannel = 16;
        break;

      }

      snprintf(Cmaxsoundchannel, sizeof(Cmaxsoundchannel), "%i", CData.maxsoundchannel);
      audioOptionsButtons[4] = Cmaxsoundchannel;
    }

    fnt_drawTextBox(menuFont, "Buffer Size:", buttonLeft + 300, displaySurface->h - 165, 0, 0, 20);
    if (ui_doButton(6, audioOptionsButtons[5], buttonLeft + 450, displaySurface->h - 165, 100, 30) == 1)
    {
      switch (CData.buffersize)
      {
      case 8192:
        CData.buffersize = 512;
        break;

      case 512:
        CData.buffersize = 1024;
        break;

      case 1024:
        CData.buffersize = 2048;
        break;

      case 2048:
        CData.buffersize = 4096;
        break;

      case 4096:
        CData.buffersize = 8192;
        break;

      default:
        CData.buffersize = 2048;
        break;

      }
      snprintf(Cbuffersize, sizeof(Cbuffersize), "%i", CData.buffersize);
      audioOptionsButtons[5] = Cbuffersize;
    }

    if (ui_doButton(7, audioOptionsButtons[6], buttonLeft, displaySurface->h - 60, 200, 30) == 1)
    {
      //save settings and go back
      save_settings();
      if (CData.musicvalid) play_music(0, 0, -1);
      else if (mixeron) Mix_PauseMusic();
      if (!CData.musicvalid && !CData.soundvalid)
      {
        Mix_CloseAudio();
        mixeron = bfalse;
      }
      menuState = MM_Leaving;
    }

    break;

  case MM_Leaving:
    // Do buttons sliding out and background fading
    // Do the same stuff as in MM_Entering, but backwards
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    //Fall trough
    menuState = MM_Finish;
    break;

  case MM_Finish:
    // Free the background texture; don't need to hold onto it
    GLTexture_Release(&background);
    menuState = MM_Begin; // Make sure this all resets next time doMainMenu is called

    // Set the next menu to load
    result = 1;
    break;

  }
  return result;
}

//--------------------------------------------------------------------------------------------
int doVideoOptions(float deltaTime)
{
  static int menuState = MM_Begin;
  static GLTexture background;
  static float lerp;
  static int menuChoice = 0;
  int result = 0;
  static char Cmaxmessage[128];
  static char Cscrd[128];
  static char Cscrz[128];

  int dmode_min,  dmode, imode_min = 0, cnt;

  switch (menuState)
  {
  case MM_Begin:
    // set up menu variables
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.menu_dir, CData.menu_gnome_bitmap);
    GLTexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    menuChoice = 0;

    //scan the video_mode_list to find the mode closest to our own
    dmode_min = MAX(ABS(CData.scrx - video_mode_list[0]->w), ABS(CData.scry - video_mode_list[0]->h));
    imode_min = 0;
    for (cnt = 1; NULL != video_mode_list[cnt]; ++cnt)
    {
      dmode = MAX(ABS(CData.scrx - video_mode_list[cnt]->w), ABS(CData.scry - video_mode_list[cnt]->h));

      if (dmode < dmode_min)
      {
        dmode_min = dmode;
        imode_min = cnt;
      };
    };

    video_mode_chaged = bfalse;
    display_mode_index = imode_min;
    if (dmode_min != 0)
    {
      CData.scrx = video_mode_list[imode_min]->w;
      CData.scry = video_mode_list[imode_min]->h;
      video_mode_chaged = btrue;
    };

    menuState = MM_Entering;  // let this fall through into MM_Entering

  case MM_Entering:
    // do buttons sliding in animation, and background fading in
    // background
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);

    //Draw the background
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    //Load all the current video settings
    if (CData.antialiasing) videoOptionsButtons[0] = "On";
    else videoOptionsButtons[0] = "Off";

    switch (CData.scrd)
    {
    case 16:
      videoOptionsButtons[1] = "Low";
      break;

    case 24:
      videoOptionsButtons[1] = "Medium";
      break;

    case 32:
      videoOptionsButtons[1] = "High";
      break;

    default:         //Set to defaults
      videoOptionsButtons[1] = "Low";
      CData.scrd = 16;
      break;

    }


    if (CData.texturefilter == TX_UNFILTERED)
    {
      videoOptionsButtons[5] = "Unfiltered";
    }
    else if (CData.texturefilter == TX_LINEAR)
    {
      videoOptionsButtons[5] = "Linear";
    }
    else if (CData.texturefilter == TX_MIPMAP)
    {
      videoOptionsButtons[5] = "Mipmap";
    }
    else if (CData.texturefilter == TX_BILINEAR)
    {
      videoOptionsButtons[5] = "Bilinear";
    }
    else if (CData.texturefilter == TX_TRILINEAR_1)
    {
      videoOptionsButtons[5] = "Tlinear 1";
    }
    else if (CData.texturefilter == TX_TRILINEAR_2)
    {
      videoOptionsButtons[5] = "Tlinear 2";
    }
    else if (CData.texturefilter >= TX_ANISOTROPIC)
    {
      snprintf(filternamebuffer, sizeof(filternamebuffer), "%s%d", "AIso ", userAnisotropy),
      videoOptionsButtons[5] = filternamebuffer;
    }

    if (CData.dither && CData.shading == GL_FLAT)
    {
      videoOptionsButtons[2] = "Yes";
    }
    else          //Set to defaults
    {
      videoOptionsButtons[2] = "No";
      CData.dither = bfalse;
      CData.shading = GL_SMOOTH;
    }

    if (CData.fullscreen) videoOptionsButtons[3] = "True";
    else videoOptionsButtons[3] = "False";

    if (CData.refon)
    {
      videoOptionsButtons[4] = "Low";
      if (CData.reffadeor == 0)
      {
        videoOptionsButtons[4] = "Medium";
        if (CData.zreflect) videoOptionsButtons[4] = "High";
      }
    }
    else videoOptionsButtons[4] = "Off";


    snprintf(Cmaxmessage, sizeof(Cmaxmessage), "%i", CData.maxmessage);
    if (CData.maxmessage > MAXMESSAGE || CData.maxmessage < 0) CData.maxmessage = MAXMESSAGE - 1;
    if (CData.maxmessage == 0) snprintf(Cmaxmessage, sizeof(Cmaxmessage), "None");       //Set to default
    videoOptionsButtons[11] = Cmaxmessage;

    if (CData.shaon)
    {
      videoOptionsButtons[6] = "Normal";
      if (!CData.shasprite) videoOptionsButtons[6] = "Best";
    }
    else videoOptionsButtons[6] = "Off";

    if (CData.scrz != 32 && CData.scrz != 16 && CData.scrz != 24)
    {
      CData.scrz = 16;       //Set to default
    }
    snprintf(Cscrz, sizeof(Cscrz), "%i", CData.scrz);    //Convert the integer to a char we can use
    videoOptionsButtons[7] = Cscrz;

    if (CData.fogallowed) videoOptionsButtons[8] = "Enable";
    else videoOptionsButtons[8] = "Disable";

    if (CData.reffadeor)
    {
      videoOptionsButtons[9] = "Okay";
      if (CData.overlayvalid && CData.backgroundvalid)
      {
        videoOptionsButtons[9] = "Good";
        if (CData.perspective) videoOptionsButtons[9] = "Superb";
      }
      else              //Set to defaults
      {
        CData.perspective = bfalse;
        CData.backgroundvalid = bfalse;
        CData.overlayvalid = bfalse;
      }
    }
    else               //Set to defaults
    {
      CData.perspective = bfalse;
      CData.backgroundvalid = bfalse;
      CData.overlayvalid = bfalse;
      videoOptionsButtons[9] = "Off";
    }

    if (CData.twolayerwateron) videoOptionsButtons[10] = "On";
    else videoOptionsButtons[10] = "Off";


    if (NULL == video_mode_list[display_mode_index])
    {
      video_mode_chaged = btrue;
      display_mode_index = 0;
      CData.scrx = video_mode_list[display_mode_index]->w;
      CData.scry = video_mode_list[display_mode_index]->h;
    };


    snprintf(display_mode_buffer, sizeof(display_mode_buffer), "%dx%d", CData.scrx, CData.scry);
    videoOptionsButtons[12] = display_mode_buffer;

    menuState = MM_Running;
    break;

  case MM_Running:
    // Do normal run
    // Background
    glColor4f(1, 1, 1, 1);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    //Antialiasing Button
    fnt_drawTextBox(menuFont, "Antialiasing:", buttonLeft, displaySurface->h - 215, 0, 0, 20);
    if (ui_doButton(1, videoOptionsButtons[0], buttonLeft + 150, displaySurface->h - 215, 100, 30) == 1)
    {
      if (CData.antialiasing == btrue)
      {
        videoOptionsButtons[0] = "Off";
        CData.antialiasing = bfalse;
      }
      else
      {
        videoOptionsButtons[0] = "On";
        CData.antialiasing = btrue;
      }
    }

    //Color depth
    fnt_drawTextBox(menuFont, "Texture Quality:", buttonLeft, displaySurface->h - 180, 0, 0, 20);
    if (ui_doButton(2, videoOptionsButtons[1], buttonLeft + 150, displaySurface->h - 180, 100, 30) == 1)
    {
      switch (CData.scrd)
      {
      case 32:
        videoOptionsButtons[1] = "Low";
        CData.scrd = 16;
        break;

      case 16:
        videoOptionsButtons[1] = "Medium";
        CData.scrd = 24;
        break;

      case 24:
        videoOptionsButtons[1] = "High";
        CData.scrd = 32;
        break;

      default:
        videoOptionsButtons[1] = "Low";
        CData.scrd = 16;
        break;

      }
    }
	//DEBUG
	CData.scrd = 32;				//We need to force 32 bit, since SDL_image crashes without it
    videoOptionsButtons[1] = "High";
	//DEBUG END

    //Dithering and Gourad Shading
    fnt_drawTextBox(menuFont, "Fast and Ugly:", buttonLeft, displaySurface->h - 145, 0, 0, 20);
    if (ui_doButton(3, videoOptionsButtons[2], buttonLeft + 150, displaySurface->h - 145, 100, 30) == 1)
    {
      if (CData.dither && CData.shading == GL_FLAT)
      {
        videoOptionsButtons[2] = "No";
        CData.dither = bfalse;
        CData.shading = GL_SMOOTH;
      }
      else
      {
        videoOptionsButtons[2] = "Yes";
        CData.dither = btrue;
        CData.shading = GL_FLAT;
      }
    }

    //Fullscreen
    fnt_drawTextBox(menuFont, "Fullscreen:", buttonLeft, displaySurface->h - 110, 0, 0, 20);
    if (ui_doButton(4, videoOptionsButtons[3], buttonLeft + 150, displaySurface->h - 110, 100, 30) == 1)
    {
      if (CData.fullscreen == btrue)
      {
        videoOptionsButtons[3] = "False";
        CData.fullscreen = bfalse;
      }
      else
      {
        videoOptionsButtons[3] = "True";
        CData.fullscreen = btrue;
      }
    }

    //Reflection
    fnt_drawTextBox(menuFont, "Reflections:", buttonLeft, displaySurface->h - 250, 0, 0, 20);
    if (ui_doButton(5, videoOptionsButtons[4], buttonLeft + 150, displaySurface->h - 250, 100, 30) == 1)
    {
      if (CData.refon && CData.reffadeor == 0 && CData.zreflect)
      {
        CData.refon = bfalse;
        CData.reffadeor = bfalse;
        CData.zreflect = bfalse;
        videoOptionsButtons[4] = "Off";
      }
      else
      {
        if (CData.refon && CData.reffadeor == 255 && !CData.zreflect)
        {
          videoOptionsButtons[4] = "Medium";
          CData.reffadeor = 0;
          CData.zreflect = bfalse;
        }
        else
        {
          if (CData.refon && CData.reffadeor == 0 && !CData.zreflect)
          {
            videoOptionsButtons[4] = "High";
            CData.zreflect = btrue;
          }
          else
          {
            CData.refon = btrue;
            CData.reffadeor = 255;    //Just in case so we dont get stuck at "Low"
            videoOptionsButtons[4] = "Low";
            CData.zreflect = bfalse;
          }
        }
      }
    }

    //Texture Filtering
    fnt_drawTextBox(menuFont, "Texture Filtering:", buttonLeft, displaySurface->h - 285, 0, 0, 20);
    if (ui_doButton(6, videoOptionsButtons[5], buttonLeft + 150, displaySurface->h - 285, 130, 30) == 1)
    {
      if (CData.texturefilter == TX_UNFILTERED)
      {
        CData.texturefilter++;
        videoOptionsButtons[5] = "Linear";
      }
      else if (CData.texturefilter == TX_LINEAR)
      {
        CData.texturefilter++;
        videoOptionsButtons[5] = "Mipmap";
      }
      else if (CData.texturefilter == TX_MIPMAP)
      {
        CData.texturefilter++;
        videoOptionsButtons[5] = "Bilinear";
      }
      else if (CData.texturefilter == TX_BILINEAR)
      {
        CData.texturefilter++;
        videoOptionsButtons[5] = "Tlinear 1";
      }
      else if (CData.texturefilter == TX_TRILINEAR_1)
      {
        CData.texturefilter++;
        videoOptionsButtons[5] = "Tlinear 2";
      }
      else if (CData.texturefilter >= TX_TRILINEAR_2 && CData.texturefilter < TX_ANISOTROPIC + log2Anisotropy - 1)
      {
        CData.texturefilter++;
        userAnisotropy = 1 << (CData.texturefilter - TX_ANISOTROPIC + 1);
        snprintf(filternamebuffer, sizeof(filternamebuffer), "%s%d", "AIso ", userAnisotropy),
        videoOptionsButtons[5] = filternamebuffer;
      }
      else
      {
        CData.texturefilter = TX_UNFILTERED;
        videoOptionsButtons[5] = "Unfiltered";
      }
    }

    //Shadows
    fnt_drawTextBox(menuFont, "Shadows:", buttonLeft, displaySurface->h - 320, 0, 0, 20);
    if (ui_doButton(7, videoOptionsButtons[6], buttonLeft + 150, displaySurface->h - 320, 100, 30) == 1)
    {
      if (CData.shaon && !CData.shasprite)
      {
        CData.shaon = bfalse;
        CData.shasprite = bfalse;        //Just in case
        videoOptionsButtons[6] = "Off";
      }
      else
      {
        if (CData.shaon && CData.shasprite)
        {
          videoOptionsButtons[6] = "Best";
          CData.shasprite = bfalse;
        }
        else
        {
          CData.shaon = btrue;
          CData.shasprite = btrue;
          videoOptionsButtons[6] = "Normal";
        }
      }
    }

    //Z bit
    fnt_drawTextBox(menuFont, "Z Bit:", buttonLeft + 300, displaySurface->h - 320, 0, 0, 20);
    if (ui_doButton(8, videoOptionsButtons[7], buttonLeft + 450, displaySurface->h - 320, 100, 30) == 1)
    {
      switch (CData.scrz)
      {
      case 32:
        videoOptionsButtons[7] = "16";
        CData.scrz = 16;
        break;

      case 16:
        videoOptionsButtons[7] = "24";
        CData.scrz = 24;
        break;

      case 24:
        videoOptionsButtons[7] = "32";
        CData.scrz = 32;
        break;

      default:
        videoOptionsButtons[7] = "16";
        CData.scrz = 16;
        break;

      }
    }

    //Fog
    fnt_drawTextBox(menuFont, "Fog Effects:", buttonLeft + 300, displaySurface->h - 285, 0, 0, 20);
    if (ui_doButton(9, videoOptionsButtons[8], buttonLeft + 450, displaySurface->h - 285, 100, 30) == 1)
    {
      if (CData.fogallowed)
      {
        CData.fogallowed = bfalse;
        videoOptionsButtons[8] = "Disable";
      }
      else
      {
        CData.fogallowed = btrue;
        videoOptionsButtons[8] = "Enable";
      }
    }

    //Perspective correction and phong mapping
    fnt_drawTextBox(menuFont, "3D Effects:", buttonLeft + 300, displaySurface->h - 250, 0, 0, 20);
    if (ui_doButton(10, videoOptionsButtons[9], buttonLeft + 450, displaySurface->h - 250, 100, 30) == 1)
    {
      if (CData.phongon && CData.perspective && CData.overlayvalid && CData.backgroundvalid)
      {
        CData.phongon = bfalse;
        CData.perspective = bfalse;
        CData.overlayvalid = bfalse;
        CData.backgroundvalid = bfalse;
        videoOptionsButtons[9] = "Off";
      }
      else
      {
        if (!CData.phongon)
        {
          videoOptionsButtons[9] = "Okay";
          CData.phongon = btrue;
        }
        else
        {
          if (!CData.perspective && CData.overlayvalid && CData.backgroundvalid)
          {
            videoOptionsButtons[9] = "Superb";
            CData.perspective = btrue;
          }
          else
          {
            CData.overlayvalid = btrue;
            CData.backgroundvalid = btrue;
            videoOptionsButtons[9] = "Good";
          }
        }
      }
    }

    //Water Quality
    fnt_drawTextBox(menuFont, "Good Water:", buttonLeft + 300, displaySurface->h - 215, 0, 0, 20);
    if (ui_doButton(11, videoOptionsButtons[10], buttonLeft + 450, displaySurface->h - 215, 100, 30) == 1)
    {
      if (CData.twolayerwateron)
      {
        videoOptionsButtons[10] = "Off";
        CData.twolayerwateron = bfalse;
      }
      else
      {
        videoOptionsButtons[10] = "On";
        CData.twolayerwateron = btrue;
      }
    }

    //Text messages
    fnt_drawTextBox(menuFont, "Max  Messages:", buttonLeft + 300, displaySurface->h - 145, 0, 0, 20);
    if (ui_doButton(12, videoOptionsButtons[11], buttonLeft + 450, displaySurface->h - 145, 75, 30) == 1)
    {
      if (CData.maxmessage != MAXMESSAGE)
      {
        CData.maxmessage++;
        CData.messageon = btrue;
        snprintf(Cmaxmessage, sizeof(Cmaxmessage), "%i", CData.maxmessage);   //Convert integer to a char we can use
      }
      else
      {
        CData.maxmessage = 0;
        CData.messageon = bfalse;
        snprintf(Cmaxmessage, sizeof(Cmaxmessage), "None");
      }
      videoOptionsButtons[11] = Cmaxmessage;
    }

    //Screen Resolution
    fnt_drawTextBox(menuFont, "Resolution:", buttonLeft + 300, displaySurface->h - 110, 0, 0, 20);
    if (ui_doButton(13, videoOptionsButtons[12], buttonLeft + 450, displaySurface->h - 110, 125, 30) == 1)
    {

      if (NULL == video_mode_list[display_mode_index])
      {
        video_mode_chaged = btrue;
        display_mode_index = 0;
      }
      else
      {
        video_mode_chaged = btrue;
        display_mode_index++;
        if (NULL == video_mode_list[display_mode_index])
        {
          display_mode_index = 0;
        }
      };

      CData.scrx = video_mode_list[display_mode_index]->w;
      CData.scry = video_mode_list[display_mode_index]->h;
      snprintf(display_mode_buffer, sizeof(display_mode_buffer), "%dx%d", CData.scrx, CData.scry);
      videoOptionsButtons[12] = display_mode_buffer;
    };

    //Save settings button
    if (ui_doButton(14, videoOptionsButtons[13], buttonLeft, displaySurface->h - 60, 200, 30) == 1)
    {
      menuChoice = 1;
      save_settings();

      //Reload some of the graphics
      load_graphics();
    }

    if (menuChoice != 0)
    {
      menuState = MM_Leaving;
      initSlidyButtons(0.0f, videoOptionsButtons);
    }
    break;

  case MM_Leaving:
    // Do buttons sliding out and background fading
    // Do the same stuff as in MM_Entering, but backwards
    glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
    ui_drawImage(0, &background, (displaySurface->w - background.imgW), 0, 0, 0);

    // "Options" text
    fnt_drawTextBox(menuFont, optionsText, optionsTextLeft, optionsTextTop, 0, 0, 20);

    //Fall trough
    menuState = MM_Finish;
    break;

  case MM_Finish:
    // Free the background texture; don't need to hold onto it
    GLTexture_Release(&background);
    menuState = MM_Begin; // Make sure this all resets next time doMainMenu is called

    // Set the next menu to load
    result = menuChoice;
    break;

  }
  return result;
}

//--------------------------------------------------------------------------------------------
int doShowMenuResults(GAME_STATE * gs, float deltaTime)
{
  int x, y;
  char text[128];
  Font *font;
  ClientState_t * cs = gs->cs;
  ServerState_t * ss = gs->ss;


  SDL_Surface *screen = SDL_GetVideoSurface();
  font = ui_getFont();

  ui_drawButton(0xFFFFFFFF, 30, 30, screen->w - 60, screen->h - 65);

  x = 35;
  y = 35;
  glColor4f(1, 1, 1, 1);
  snprintf(text, sizeof(text), "Module selected: %s", gs->modules[ss->selectedModule].loadname);
  fnt_drawText(font, x, y, text);
  y += 35;

  if (gs->modstate.importvalid)
  {
    snprintf(text, sizeof(text), "Player selected: %s", loadplayername[cs->selectedPlayer]);
  }
  else
  {
    snprintf(text, sizeof(text), "Starting a new player.");
  }
  fnt_drawText(font, x, y, text);

  init_mod_state(&(gs->modstate), &(gs->mod), gs->randie_index);

  log_info("SDL_main: Loading module %s...\n", gs->mod.loadname);
  load_module(gs, gs->mod.loadname);

  return 1;
}

//--------------------------------------------------------------------------------------------
int doNotImplemented(float deltaTime)
{
  int x, y;
  int w, h;
  char notImplementedMessage[] = "Not implemented yet!  Check back soon!";


  fnt_getTextSize(ui_getFont(), notImplementedMessage, &w, &h);
  w += 50; // add some space on the sides

  x = displaySurface->w / 2 - w / 2;
  y = displaySurface->h / 2 - 17;

  if (ui_doButton(1, notImplementedMessage, x, y, w, 30) == 1)
  {
    return 1;
  }

  return 0;
}

//--------------------------------------------------------------------------------------------
int doIngameMenu(float deltaTime)
{
  static int whichMenu = Inventory;
  static int lastMenu  = Inventory;
  int result = 0;

  switch (whichMenu)
  {
  case Inventory:
    result = doNotImplemented(deltaTime);
    if (result != 0)
    {
      lastMenu = Inventory;
      if (result == 1) return -1;
    }
    break;

  default:
    result = doNotImplemented(deltaTime);
    if (result != 0)
    {
      whichMenu = lastMenu;
    }
  }

  return 0;
}


//--------------------------------------------------------------------------------------------
int doMenu(GAME_STATE * gs, float deltaTime)
{
  static int whichMenu = MainMenu;
  static int lastMenu = MainMenu;
  int result = 0;
  NET_STATE * ns = gs->ns;
  ServerState_t * ss = gs->ss;
  ClientState_t * cs = gs->cs;

  switch (whichMenu)
  {
  case MainMenu:
    result = doMainMenu(deltaTime);
    if (result != 0)
    {
      lastMenu = MainMenu;
      if (result == 1) whichMenu = SinglePlayer;
      else if (result == 2) whichMenu = MultiPlayer;
      else if (result == 3) whichMenu = NetworkMenu;
      else if (result == 4) whichMenu = Options;
      else if (result == 5) return -1; // need to request a quit somehow
    }
    break;

  case SinglePlayer:
    result = doSinglePlayerMenu(deltaTime);
    if (result != 0)
    {
      gs->ns->networkon = bfalse;
      lastMenu = SinglePlayer;
      if (result == 1)
      {
        whichMenu = ChooseModule;
        startNewPlayer = btrue;
      }
      else if (result == 2)
      {
        whichMenu = ChoosePlayer;
        startNewPlayer = bfalse;
      }
      else if (result == 3) whichMenu = MainMenu;
      else whichMenu = NewPlayer;
    }
    break;

  case NetworkMenu:
    result = doNetworkMenu(ns, deltaTime);
    gs->ns->networkon = btrue;
    if (result != 0)
    {
      lastMenu = MainMenu;
      if (result == 1) whichMenu = HostGameMenu;
      else if (result == 2) whichMenu = JoinGameMenu;
      else if (result == 3) return -1; // need to request a quit somehow
    }
    break;

  case HostGameMenu:
    lastMenu = NetworkMenu;
    if(ss->ready)
    {
      whichMenu = UnhostGameMenu;
    }
    else
    {
      result = doHostGameMenu(ss, deltaTime);

      if (result != 0)
      { 
        if (result == 1)
        {
          whichMenu = UnhostGameMenu;
          ss->amHost = sv_hostGame(ss);

          gs->modstate.respawnvalid   = ss->mod.respawnvalid != bfalse;     
          gs->modstate.respawnanytime = ss->mod.respawnvalid == ANYTIME;   
          gs->modstate.importvalid    = ss->mod.importamount>0;      
          gs->modstate.exportvalid    = ss->mod.allowexport;      
          gs->modstate.rts_control     = ss->mod.rts_control;       
          gs->modstate.net_messagemode = bfalse;   
          gs->modstate.nolocalplayers = btrue;   
          gs->modstate.beat           = bfalse;

          if(!gs->modstate.loaded)
          {
            log_info("SDL_main: Loading module %s...\n", ss->mod.loadname);
            load_module(gs, ss->mod.loadname);
            gs->modstate.loaded = btrue;
            ss->ready         = btrue;
          }
        }
        else if (result == 2)
        {
          whichMenu = lastMenu;
        }
      }
    }
    break;

  case UnhostGameMenu:
    lastMenu = NetworkMenu;
    if(!ss->ready)
    {
      whichMenu = HostGameMenu;
    }
    else
    {
       result = doUnhostGameMenu(ss, deltaTime);

      if (result != 0)
      {
        whichMenu = HostGameMenu;
        if (result == 1)
        {
          whichMenu = HostGameMenu;
          ss->amHost = sv_unhostGame(ss);

          if(!gs->modstate.loaded)
          {
            quit_module(gs);
            gs->modstate.loaded = bfalse;
          }

          ss->ready = bfalse;
        }
        else if (result == 2)
        {
          whichMenu = lastMenu;
        }
      };
    };
    break;

  case JoinGameMenu:
    result = doJoinGameMenu(gs, deltaTime);
    if (result != 0)
    {
      lastMenu = NetworkMenu;
      if (result == 1)
      {
        if(!gs->modstate.loaded)
        {
          memcpy(&(gs->modstate), &(cs->req_modstate), sizeof(MOD_STATE));

          log_info("SDL_main: Loading module %s...\n", cs->req_mod.loadname);
          load_module(gs, cs->req_mod.loadname);

          gs->modstate.loaded = btrue;
          cs->waiting       = btrue;
        };

        gs->modstate.net_messagemode = btrue;   
        gs->modstate.nolocalplayers = bfalse;   

        cs->amClient = (rv_succeed == cl_joinGame(cs, cs->req_host));

        whichMenu = ChoosePlayer;
      }       
      else if (result == 2) whichMenu = lastMenu;
    }
    break;

  case ChooseModule:
    result = doChooseModule(gs, deltaTime);
    if (result == -1) whichMenu = lastMenu;
    else if (result == 1) whichMenu = TestResults;
    break;

  case ChoosePlayer:
    result = doChoosePlayer(gs, deltaTime);
    if (result == -1)    whichMenu = lastMenu;
    else if (result == 1) whichMenu = ChooseModule;
    break;

  case Options:
    result = doOptions(deltaTime);
    if (result != 0)
    {
      if (result == -1)   whichMenu = lastMenu;
      else if (result == 1) whichMenu = AudioOptions;
      else if (result == 2) whichMenu = InputOptions;
      else if (result == 3) whichMenu = VideoOptions;
      else if (result == 4) whichMenu = MainMenu;
    }
    break;

  case AudioOptions:
    result = doAudioOptions(deltaTime);
    if (result != 0)
    {
      whichMenu = Options;
    }
    break;

  case VideoOptions:
    result = doVideoOptions(deltaTime);
    if (result != 0)
    {
      whichMenu = Options;
    }
    break;

  case TestResults:
    result = doShowMenuResults(gs, deltaTime);
    if (result != 0)
    {
      whichMenu = MainMenu;
      return 1;
    }
    break;

  default:
    result = doNotImplemented(deltaTime);
    if (result != 0)
    {
      whichMenu = lastMenu;
    }
  }

  return 0;
}

//--------------------------------------------------------------------------------------------
void menu_frameStep()
{

}

//--------------------------------------------------------------------------------------------
void save_settings()
{
  //This function saves all current game settings to setup.txt
  FILE* setupfile;
  STRING write;
  char *TxtTmp;

  setupfile = fopen(CData.setup_file, "w");
  if (setupfile)
  {
    /*GRAPHIC PART*/
    fputs("{GRAPHIC}\n", setupfile);
    snprintf(write, sizeof(write), "[MAX_NUMBER_VERTICES] : \"%i\"\n", CData.maxtotalmeshvertices / 1024);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[COLOR_DEPTH] : \"%i\"\n", CData.scrd);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[Z_DEPTH] : \"%i\"\n", CData.scrz);
    fputs(write, setupfile);
    if (CData.fullscreen) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[FULLSCREEN] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.zreflect) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[Z_REFLECTION] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[SCREENSIZE_X] : \"%i\"\n", CData.scrx);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[SCREENSIZE_Y] : \"%i\"\n", CData.scry);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[MAX_TEXT_MESSAGE] : \"%i\"\n", CData.maxmessage);
    fputs(write, setupfile);
    if (CData.staton) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[STATUS_BAR] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.perspective) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[PERSPECTIVE_CORRECT] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);

    if (CData.texturefilter >= TX_ANISOTROPIC && maxAnisotropy > 0.0f)
    {
      int anisotropy = MIN(maxAnisotropy, userAnisotropy);
      snprintf(filternamebuffer, sizeof(filternamebuffer), "%d ANISOTROPIC %d", CData.texturefilter, anisotropy);
    }
    else if (CData.texturefilter >= TX_ANISOTROPIC)
    {
      CData.texturefilter = TX_TRILINEAR_2;
      snprintf(filternamebuffer, sizeof(filternamebuffer), "%d TRILINEAR 2", CData.texturefilter);
    }
    else
    {
      switch (CData.texturefilter)
      {
      case TX_UNFILTERED:
        snprintf(filternamebuffer, sizeof(filternamebuffer), "%d UNFILTERED", CData.texturefilter);
        break;

      case TX_MIPMAP:
        snprintf(filternamebuffer, sizeof(filternamebuffer), "%d MIPMAP", CData.texturefilter);
        break;

      case TX_TRILINEAR_1:
        snprintf(filternamebuffer, sizeof(filternamebuffer), "%d TRILINEAR 1", CData.texturefilter);
        break;

      case TX_TRILINEAR_2:
        snprintf(filternamebuffer, sizeof(filternamebuffer), "%d TRILINEAR 2", CData.texturefilter);
        break;

      default:
      case TX_LINEAR:
        CData.texturefilter = TX_LINEAR;
        snprintf(filternamebuffer, sizeof(filternamebuffer), "%d LINEAR", CData.texturefilter);
        break;

      }
    }

    snprintf(write, sizeof(write), "[TEXTURE_FILTERING] : \"%s\"\n", filternamebuffer);
    fputs(write, setupfile);
    if (CData.shading == GL_SMOOTH) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[GOURAUD_SHADING] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.antialiasing) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[ANTIALIASING] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.dither) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[DITHERING] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.refon) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[REFLECTION] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.shaon) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[SHADOWS] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.shasprite) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[SHADOW_AS_SPRITE] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.phongon) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[PHONG] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.fogallowed) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[FOG] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.reffadeor == 0) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[FLOOR_REFLECTION_FADEOUT] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.twolayerwateron) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[MULTI_LAYER_WATER] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.overlayvalid) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[OVERLAY] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.backgroundvalid) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[BACKGROUND] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);

    /*SOUND PART*/
    snprintf(write, sizeof(write), "\n{SOUND}\n");
    fputs(write, setupfile);
    if (CData.musicvalid) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[MUSIC] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[MUSIC_VOLUME] : \"%i\"\n", CData.musicvolume);
    fputs(write, setupfile);
    if (CData.soundvalid) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[SOUND] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[SOUND_VOLUME] : \"%i\"\n", CData.soundvolume);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[OUTPUT_BUFFER_SIZE] : \"%i\"\n", CData.buffersize);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[MAX_SOUND_CHANNEL] : \"%i\"\n", CData.maxsoundchannel);
    fputs(write, setupfile);

    /*CAMERA PART*/
    snprintf(write, sizeof(write), "\n{CONTROL}\n");
    fputs(write, setupfile);
    switch (CData.autoturncamera)
    {
    case 255: TxtTmp = "GOOD";
      break;

    case btrue: TxtTmp = "TRUE";
      break;

    case bfalse: TxtTmp = "FALSE";
      break;

    }
    snprintf(write, sizeof(write), "[AUTOTURN_CAMERA] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);

    /*NETWORK PART*/
    snprintf(write, sizeof(write), "\n{NETWORK}\n");
    fputs(write, setupfile);
    if (CData.request_network) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[NETWORK_ON] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    TxtTmp = CData.net_hosts[0];
    snprintf(write, sizeof(write), "[HOST_NAME] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    TxtTmp = CData.net_messagename;
    snprintf(write, sizeof(write), "[MULTIPLAYER_NAME] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    snprintf(write, sizeof(write), "[LAG_TOLERANCE] : \"%i\"\n", CData.lag);
    fputs(write, setupfile);

    /*DEBUG PART*/
    snprintf(write, sizeof(write), "\n{DEBUG}\n");
    fputs(write, setupfile);
    if (CData.fpson) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[DISPLAY_FPS] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.HideMouse) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[HIDE_MOUSE] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (SDL_GRAB_ON==CData.GrabMouse) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[GRAB_MOUSE] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);
    if (CData.DevMode) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf(write, sizeof(write), "[DEVELOPER_MODE] : \"%s\"\n", TxtTmp);
    fputs(write, setupfile);

    //Close it up
    fclose(setupfile);
  }
  else
  {
    log_warning("Cannot open setup.txt to write new settings into.\n");
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/* Old Menu Code */

#if 0
//--------------------------------------------------------------------------------------------
//void draw_trimx(int x, int y, int length)
//{
//  // ZZ> This function draws a horizontal trim bar
//  GLfloat txWidth, txHeight, txLength;
//
//  if ( INVALID_TEXTURE != GLTexture_GetTextureID(&TxTrim) )
//  {
//    /*while( length > 0 )
//    {
//    trimrect.right = length;
//    if(length > TRIMX)  trimrect.right = TRIMX;
//    trimrect.bottom = 4;
//    lpDDSBack->BltFast(x, y, lpDDSTrimX, &trimrect, DDBLTFAST_NOCOLORKEY);
//    length-=TRIMX;
//    x+=TRIMX;
//    }*/
//
//    /* Calculate the texture width, height, and length */
//    txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTrim )/GLTexture_GetTextureWidth( &TxTrim ) );
//    txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTrim )/GLTexture_GetTextureHeight( &TxTrim ) );
//    txLength = ( GLfloat )( length/GLTexture_GetImageWidth( &TxTrim ) );
//
//
//    /* Bind our texture */
//    GLTexture_Bind( &TxTrim, CData.texturefilter );
//
//    /* Draw the trim */
//    glColor4f( 1, 1, 1, 1 );
//    glBegin( GL_QUADS );
//    glTexCoord2f( 0, 1 ); glVertex2f( x, CData.scry - y );
//    glTexCoord2f( 0, 1 - txHeight ); glVertex2f( x, CData.scry - y - GLTexture_GetImageHeight( &TxTrim ) );
//    glTexCoord2f( txWidth*txLength, 1 - txHeight ); glVertex2f( x + length, CData.scry - y - GLTexture_GetImageHeight( &TxTrim ) );
//    glTexCoord2f( txWidth*txLength, 1 ); glVertex2f( x + length, CData.scry - y );
//    glEnd();
//  }
//}
//
////--------------------------------------------------------------------------------------------
//void draw_trimy(int x, int y, int length)
//{
//  // ZZ> This function draws a vertical trim bar
//  GLfloat txWidth, txHeight, txLength;
//
//  if ( INVALID_TEXTURE != GLTexture_GetTextureID(&TxTrim) )
//  {
//    /*while(length > 0)
//    {
//    trimrect.bottom = length;
//    if(length > TRIMY)  trimrect.bottom = TRIMY;
//    trimrect.right = 4;
//    lpDDSBack->BltFast(x, y, lpDDSTrimY, &trimrect, DDBLTFAST_NOCOLORKEY);
//    length-=TRIMY;
//    y+=TRIMY;
//    }*/
//
//    /* Calculate the texture width, height, and length */
//    txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTrim )/GLTexture_GetTextureWidth( &TxTrim ) );
//    txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTrim )/GLTexture_GetTextureHeight( &TxTrim ) );
//    txLength = ( GLfloat )( length/GLTexture_GetImageHeight( &TxTrim ) );
//
//    /* Bind our texture */
//    GLTexture_Bind( &TxTrim, CData.texturefilter );
//
//    /* Draw the trim */
//    glColor4f( 1, 1, 1, 1 );
//    glBegin( GL_QUADS );
//    glTexCoord2f( 0, 1 ); glVertex2f( x, CData.scry - y );
//    glTexCoord2f( 0, 1 - txHeight*txLength ); glVertex2f( x, CData.scry - y - length );
//    glTexCoord2f( txWidth, 1 - txHeight*txLength ); glVertex2f( x + GLTexture_GetImageWidth( &TxTrim ), CData.scry - y - length );
//    glTexCoord2f( txWidth, 1 ); glVertex2f( x + GLTexture_GetImageWidth( &TxTrim ), CData.scry - y );
//    glEnd();
//  }
//}
//
////--------------------------------------------------------------------------------------------
//void draw_trim_box(int left, int top, int right, int bottom)
//{
//  // ZZ> This function draws a trim rectangle
//  float l,t,r,b;
//  l=((float)left)/CData.scrx;
//  r=((float)right)/CData.scrx;
//  t=((float)top)/CData.scry;
//  b=((float)bottom)/CData.scry;
//
//  Begin2DMode();
//
//  draw_trimx(left, top, right-left);
//  draw_trimx(left, bottom-4, right-left);
//  draw_trimy(left, top, bottom-top);
//  draw_trimy(right-4, top, bottom-top);
//
//  End2DMode();
//}
//
////--------------------------------------------------------------------------------------------
//void draw_trim_box_opening(int left, int top, int right, int bottom, float amount)
//{
//  // ZZ> This function draws a trim rectangle, scaled around its center
//  int x = (left + right)>>1;
//  int y = (top + bottom)>>1;
//  left   = (x * (1.0-amount)) + (left * amount);
//  right  = (x * (1.0-amount)) + (right * amount);
//  top    = (y * (1.0-amount)) + (top * amount);
//  bottom = (y * (1.0-amount)) + (bottom * amount);
//  draw_trim_box(left, top, right, bottom);
//}

////--------------------------------------------------------------------------------------------
//void menu_service_select()
//{
//    // ZZ> This function lets the user choose a network service to use
//    char text[256];
//    int x, y;
//    float open;
//    int cnt;
//    int stillchoosing;
//
//
//    NetState.service = NONETWORK;
//    if(numservice > 0)
//    {
//        // Open a big window
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//            draw_trim_box_opening(0, 0, 320, fontyspacing*(numservice+4), open);
//            flip_pages();
//            open += .030;
//        }
//        // Tell the user which ones we found ( in setup_network )
//        stillchoosing = btrue;
//        while(stillchoosing)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, CData.scrx, CData.scry);
//            draw_trim_box(0, 0, 320, fontyspacing*(numservice+4));
//            y = 8;
//            sprintf(text, "Network options...");
//            draw_string(text, 14, y);
//            y += fontyspacing;
//            cnt = 0;
//            while(cnt < numservice)
//            {
//                sprintf(text, "%s", NetState.servicename[cnt]);
//                draw_string(text, 50, y);
//                y += fontyspacing;
//                cnt++;
//            }
//            sprintf(text, "No Network");
//            draw_string(text, 50, y);
//            do_cursor();
//            x = cursorx - 50;
//            y = (cursory - 8 - fontyspacing);
//            if(x > 0 && x < 300 && y >= 0)
//            {
//                y = y/fontyspacing;
//                if(y <= numservice)
//                {
//                    if(GMous.button[0] || GMous.button[1])
//                    {
//                        stillchoosing = bfalse;
//                        NetState.service = y;
//                    }
//                }
//            }
//            flip_pages();
//        }
//    }
////    turn_on_service(NetState.service);
//}
//
////--------------------------------------------------------------------------------------------
//void menu_start_or_join()
//{
//    // ZZ> This function lets the user start or join a game for a network game
//    char text[256];
//    int x, y;
//    float open;
//    int stillchoosing;
//
//
//    // Open another window
//    if(CData.request_network)
//    {
//        open = 0;
//        while(open < 1.0)
//        {
//   //clear_surface(lpDDSBack);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//   draw_trim_box_opening(0, 0, 280, 102, open);
//   flip_pages();
//   open += .030;
//  }
//        // Give the user some options
//        stillchoosing = btrue;
//        while(stillchoosing)
//        {
//   //clear_surface(lpDDSBack);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box(0, 0, CData.scrx, CData.scry);
//   draw_trim_box(0, 0, 280, 102);
//
//   // Draw the menu text
//   y = 8;
//   sprintf(text, "Game options...");
//   draw_string(text, 14, y);
//   y += fontyspacing;
//   sprintf(text, "New Game");
//   draw_string(text, 50, y);
//   y += fontyspacing;
//   sprintf(text, "Join Game");
//   draw_string(text, 50, y);
//   y += fontyspacing;
//   sprintf(text, "Quit Game");
//   draw_string(text, 50, y);
//
//   do_cursor();
//
////   sprintf(text, "Cursor position: %03d, %03d", cursorx, cursory);
////   draw_string(text, 14, 400);
//
//   x = cursorx - 50;
//   // The adjustments to y here were figured out empirically; I still
//   // don't understand the reasoning behind it.  I don't think the text
//   // draws where it says it's going to.
//   y = (cursory - 21 - fontyspacing);
//
//
//
//            if(x > 0 && x < 280 && y >= 0)
//            {
//                y = y/fontyspacing;
//                if(y < 3)
//                {
//                    if(GMous.button[0] || GMous.button[1])
//                    {
//                        if(y == 0)
//                        {
//                            if(sv_hostGame())
//                            {
//                                NetState.ss->Active = btrue;
//                                nextmenu = MENUD;
//                                stillchoosing = bfalse;
//                            }
//                        }
//                        if(y == 1 && NetState.service != NONETWORK)
//                        {
//                            nextmenu = MENUC;
//                            stillchoosing = bfalse;
//                        }
//                        if(y == 2)
//                        {
//                            nextmenu = MENUB;
//                            menuactive = bfalse;
//                            stillchoosing = bfalse;
//                            GameState.Active = bfalse;
//                        }
//                    }
//                }
//            }
//            flip_pages();
//        }
//    }
//    else
//    {
//        NetState.ss->Active = btrue;
//        nextmenu = MENUD;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void draw_module_tag(int module, int y)
//{
//    // ZZ> This function draws a module tag
//    char text[256];
//    draw_trim_box(0, y, 136, y+136);
//    draw_trim_box(132, y, CData.scrx, y+136);
//    if(module < globalnummodule)
//    {
//        draw_titleimage(module, 4, y+4);
//        y+=6;
//        sprintf(text, "%s", ModList[module].longname);  draw_string(text, 150, y);  y+=fontyspacing;
//        sprintf(text, "%s", ModList[module].rank);  draw_string(text, 150, y);  y+=fontyspacing;
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
//        draw_string(text, 150, y);  y+=fontyspacing;
//        if(ModList[module].importamount == 0 && ModList[module].allowexport==bfalse)
//        {
//            sprintf(text, "No Import/Export");  draw_string(text, 150, y);  y+=fontyspacing;
//        }
//        else
//        {
//            if(ModList[module].importamount == 0)
//            {
//                sprintf(text, "No Import");  draw_string(text, 150, y);  y+=fontyspacing;
//            }
//            if(ModList[module].allowexport==bfalse)
//            {
//                sprintf(text, "No Export");  draw_string(text, 150, y);  y+=fontyspacing;
//            }
//        }
//        if(ModList[module].respawnvalid == bfalse)
//        {
//            sprintf(text, "No Respawn");  draw_string(text, 150, y);  y+=fontyspacing;
//        }
//        if(ModList[module].rts_control == btrue)
//        {
//            sprintf(text, "RTS");  draw_string(text, 150, y);  y+=fontyspacing;
//        }
//        if(ModList[module].rts_control == ALLSELECT)
//        {
//            sprintf(text, "Diaboo RTS");  draw_string(text, 150, y);  y+=fontyspacing;
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
//    char fromdir[128];
//    char todir[128];
// int clientFilesSent = 0;
// int hostFilesSent = 0;
// int pending;
//
//    // Set the important flags
//    gs->mod.respawnvalid = bfalse;
//    gs->modstate.respawnanytime = bfalse;
//    if(ModList[module].respawnvalid)  gs->mod.respawnvalid = btrue;
//    if(ModList[module].respawnvalid==ANYTIME)  gs->modstate.respawnanytime = btrue;
//    gs->mod.rts_control = bfalse;
//    if(ModList[module].rts_control != bfalse)
//    {
//        gs->mod.rts_control = btrue;
//        allselect = bfalse;
//        if(ModList[module].rts_control == ALLSELECT)
//            allselect = btrue;
//    }
//    gs->modstate.exportvalid = ModList[module].allowexport;
//    gs->modstate.importvalid = (ModList[module].importamount > 0);
//    gs->mod.importamount = ModList[module].importamount;
//    gs->mod.minplayers = ModList[module].maxplayers;
//    fs_createDirectory("import");  // Just in case...
//
//
//    start = 0;
//    if(gs->modstate.importvalid)
//    {
//        // Figure out which characters are available
//        check_player_import("players");
//        numshow = (CData.scry-80-fontyspacing-fontyspacing)>>5;
//
//
//        // Open some windows
//        y = fontyspacing + 8;
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//            draw_trim_box_opening(0, 0, CData.scrx, 40, open);
//            draw_trim_box_opening(0, CData.scry-40, CData.scrx, CData.scry, open);
//            flip_pages();
//            open += .030;
//        }
//
//
//        wldframe = 0;  // For sparkle
//        stillchoosing = btrue;
//        while(stillchoosing)
//        {
//            // Draw the windows
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, CData.scrx, CData.scry);
//            draw_trim_box(0, 40, CData.scrx, CData.scry-40);
//
//            // Draw the Up/Down buttons
//            if(start == 0)
//            {
//                // Show the instructions
//                x = (CData.scrx-270)>>1;
//                draw_string("Setup controls", x, 10);
//            }
//            else
//            {
//                x = (CData.scrx-40)>>1;
//                draw_string("Up", x, 10);
//            }
//            x = (CData.scrx-80)>>1;
//            draw_string("Down", x, CData.scry-fontyspacing-20);
//
//
//            // Draw each import character
//            y = 40+fontyspacing;
//            cnt = 0;
//            while(cnt < numshow && cnt + start < numloadplayer)
//            {
//                sparkle = NOSPARKLE;
//                if(GKeyb.player == (cnt+start))
//                {
//                    draw_one_icon(GKeyb.icon, 32, y, NOSPARKLE);
//                    sparkle = 0;  // White
//                }
//                else
//                    draw_one_icon(nullicon, 32, y, NOSPARKLE);
//                if(GMous.player == (cnt+start))
//                {
//                    draw_one_icon(GMous.icon, 64, y, NOSPARKLE);
//                    sparkle = 0;  // White
//                }
//                else
//                    draw_one_icon(nullicon, 64, y, NOSPARKLE);
//                if(GJoy[0].player == (cnt+start) && GJoy[0].on)
//                {
//                    draw_one_icon(GJoy[0].icon, 128, y, NOSPARKLE);
//                    sparkle = 0;  // White
//                }
//                else
//                    draw_one_icon(nullicon, 128, y, NOSPARKLE);
//                if(GJoy[1].player == (cnt+start) && GJoy[1].on)
//                {
//                    draw_one_icon(GJoy[1].icon, 160, y, NOSPARKLE);
//                    sparkle = 0;  // White
//                }
//                else
//                    draw_one_icon(nullicon, 160, y, NOSPARKLE);
//                draw_one_icon((cnt+start), 96, y, sparkle);
//                draw_string(loadplayername[cnt+start], 200, y+6);
//                y+=32;
//                cnt++;
//            }
//            wldframe++;  // For sparkle
//
//
//            // Handle other stuff...
//            do_cursor();
//            if(pending_click)
//            {
//         pending_click=bfalse;
//                if(cursory < 40 && start > 0)
//                {
//                    // Up button
//                    start--;
//                }
//                if(cursory >= (CData.scry-40) && (start + numshow) < numloadplayer)
//                {
//                    // Down button
//                    start++;
//                }
//            }
//            if(GMous.button[0])
//            {
//                x = (cursorx - 32) >> 5;
//                y = (cursory - 44) >> 5;
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
//                stillchoosing = bfalse;
//            }
//            flip_pages();
//        }
//        wldframe = 0;  // For sparkle
//
//
//  // Tell the user we're loading
//  y = fontyspacing + 8;
//  open = 0;
//  while(open < 1.0)
//  {
//   //clear_surface(lpDDSBack);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//   flip_pages();
//   open += .030;
//  }
//
//  //clear_surface(lpDDSBack);
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//  glLoadIdentity();
//  draw_trim_box(0, 0, CData.scrx, CData.scry);
//  draw_string("Copying the imports...", y, y);
//  flip_pages();
//
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
//                control = INPUTNONE;
//                if(cnt == GKeyb.player)  control = control | INPUTKEY;
//                if(cnt == GMous.player)  control = control | INPUTMOUSE;
//                if(cnt == GJoy[0].player)  control = control | INPUTJOYA;
//                if(cnt == GJoy[1].player)  control = control | INPUTJOYB;
//                localcontrol[numimport] = control;
////                localslot[numimport] = (numimport+(localmachine*4))*9;
//    localslot[numimport] = (numimport + localmachine) * 9;
//
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
//  if(CData.request_network && !NetState.ss->Active)
//  {
//   pending = net_pendingFileTransfers();
//
//   // Let the host know how many files you're sending it
//   net_startNewPacket();
//   packet_addUnsignedShort(&gPacket, NET_NUM_FILES_TO_SEND);
//   packet_addUnsignedShort(&gPacket, (Uint16)pending);
//   net_sendPacketToHostGuaranteed();
//
//   while(pending)
//   {
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//    glLoadIdentity();
//
//    draw_trim_box(0, 0, CData.scrx, CData.scry);
//    y = fontyspacing + 8;
//
//    sprintf(todir, "Sending file %d of %d...", clientFilesSent - pending, clientFilesSent);
//    draw_string(todir, fontyspacing + 8, y);
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
//   packet_addUnsignedShort(&gPacket, NET_DONE_SENDING_FILES);
//   net_sendPacketToHostGuaranteed();
//  }
//
//  if(CData.request_network)
//  {
//   if(NetState.ss->Active)
//   {
//    // Host waits for all files from all remotes
//    numfile = 0;
//    numfileexpected = 0;
//    numplayerrespond = 1;
//    while(numplayerrespond < numplayer)
//    {
//     //clear_surface(lpDDSBack);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glLoadIdentity();
//     draw_trim_box(0, 0, CData.scrx, CData.scry);
//     y = fontyspacing + 8;
//     draw_string("Incoming files...", fontyspacing+8, y);  y+=fontyspacing;
//     sprintf(todir, "File %d/%d", numfile, numfileexpected);
//     draw_string(todir, fontyspacing+20, y); y+=fontyspacing;
//     sprintf(todir, "Play %d/%d", numplayerrespond, numplayer);
//     draw_string(todir, fontyspacing+20, y);
//     flip_pages();
//
//     listen_for_packets();
//
//     do_cursor();
//
//     if(SDLKEYDOWN(SDLK_ESCAPE))
//     {
//      GameState.Active = bfalse;
//      menuactive = bfalse;
//      close_session(gs->ns);
//      break;

//     }
//    }
//
//
//                // Say you're done
//                //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//                draw_trim_box(0, 0, CData.scrx, CData.scry);
//                y = fontyspacing + 8;
//                draw_string("Sending files to remotes...", fontyspacing+8, y);  y+=fontyspacing;
//                flip_pages();
//
//
//                // Host sends import directory to all remotes, deletes extras
//                numfilesent = 0;
//                import = 0;
//                cnt = 0;
//    if(numplayer > 1)
//    {
//     while(cnt < MAXIMPORT)
//     {
//      sprintf(todir, "import/temp%04d.obj", cnt);
//      strncpy(fromdir, todir, 128);
//      if(fs_fileIsDirectory(fromdir))
//      {
//       // Only do directories that actually exist
//       if((cnt % 9)==0) import++;
//       if(import > gs->mod.importamount)
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
//     packet_addUnsignedShort(&gPacket, NET_NUM_FILES_TO_SEND);
//     packet_addUnsignedShort(&gPacket, (Uint16)pending);
//     net_sendPacketToAllPlayersGuaranteed();
//
//     while(pending > 0)
//     {
//      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//      glLoadIdentity();
//
//      draw_trim_box(0, 0, CData.scrx, CData.scry);
//      y = fontyspacing + 8;
//
//      sprintf(todir, "Sending file %d of %d...", hostFilesSent - pending, hostFilesSent);
//      draw_string(todir, fontyspacing + 8, y);
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
//     packet_addUnsignedShort(&gPacket, NET_DONE_SENDING_FILES);
//     net_sendPacketToAllPlayersGuaranteed();
//    }
//            }
//            else
//            {
//    // Remotes wait for all files in import directory
//    log_info("menu_pick_player: Waiting for files to come from the host...\n");
//    numfile = 0;
//    numfileexpected = 0;
//    numplayerrespond = 0;
//    while(numplayerrespond < 1)
//    {
//     //clear_surface(lpDDSBack);
//     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glLoadIdentity();
//     draw_trim_box(0, 0, CData.scrx, CData.scry);
//     y = fontyspacing + 8;
//     draw_string("Incoming files from host...", fontyspacing+8, y);  y+=fontyspacing;
//     sprintf(todir, "File %d/%d", numfile, numfileexpected);
//     draw_string(todir, fontyspacing+20, y);
//     flip_pages();
//
//     listen_for_packets();
//     do_cursor();
//
//     if(SDLKEYDOWN(SDLK_ESCAPE))
//     {
//      GameState.Active = bfalse;
//      menuactive = bfalse;
//      break;

//      close_session(gs->ns);
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
//    char text[256];
//    int y;
//    float open;
//    int cnt;
//
//
//    // Open some windows
//    y = fontyspacing + 8;
//    open = 0;
//    while(open < 1.0)
//    {
//        //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//        draw_trim_box_opening(0, y, 136, y+136, open);
//        draw_trim_box_opening(132, y, CData.scrx, y+136, open);
//        draw_trim_box_opening(0, y+132, CData.scrx, CData.scry, open);
//        flip_pages();
//        open += .030;
//    }
//
//
//    // Put the stuff in the windows
//    //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//    y = 0;
//    sprintf(text, "Loading...  Wait!!!");  draw_string(text, 0, y);  y+=fontyspacing;
//    y+=8;
//    draw_module_tag(module, y);
//    draw_trim_box(0, y+132, CData.scrx, CData.scry);
//
//
//    // Show the summary
//    sprintf(text, "%s/%s/%s/%s", CData.modules_dir, ModList[module].loadname, CData.gamedat_dir, CData.menu_file);
//    get_module_summary(text);
//    y = fontyspacing+152;
//    cnt = 0;
//    while(cnt < SUMMARYLINES)
//    {
//        sprintf(text, "%s", ModList[cnt].summary);  draw_string(text, 14, y);  y+=fontyspacing;
//        cnt++;
//    }
//    flip_pages();
//    nextmenu = MENUB;
//    menuactive = bfalse;
//}
//
////--------------------------------------------------------------------------------------------
//void menu_join_multiplayer()
//{
// // JF> This function attempts to join the multiplayer game hosted
// //     by whatever server is named in the HOST_NAME part of setup.txt
// char text[256];
// float open;
//
// if(CData.request_network)
// {
//  // Do the little opening menu animation
//  open = 0;
//  while(open < 1.0)
//  {
//   //clear_surface(lpDDSBack);
//   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//   glLoadIdentity();
//   draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//   flip_pages();
//   open += .030;
//  }
//
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//  glLoadIdentity();
//  draw_trim_box(0, 0, CData.scrx, CData.scry);
//
//  strncpy(text, "Attempting to join game at:", 256);
//  draw_string(text, (CData.scrx>>1)-240, (CData.scry>>1)-fontyspacing);
//
//  strncpy(text, CData.net_hosts[0], 256);
//  draw_string(text, (CData.scrx>>1)-240, (CData.scry>>1));
//  flip_pages();
//
//  if(cl_joinGame(CData.net_hosts[0]))
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
//    char text[256];
//    int x, y;
//    float open;
//    int cnt;
//    int stillchoosing;
//
//
//    if(CData.request_network)
//    {
//        // Bring up a helper window
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//            flip_pages();
//            open += .030;
//        }
//        //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//        draw_trim_box(0, 0, CData.scrx, CData.scry);
//        sprintf(text, "Press Enter if");
//        draw_string(text, (CData.scrx>>1)-120, (CData.scry>>1)-fontyspacing);
//        sprintf(text, "nothing happens");
//        draw_string(text, (CData.scrx>>1)-120, (CData.scry>>1));
//        flip_pages();
//
//
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
//            draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//            draw_trim_box_opening(0, 0, 320, fontyspacing*(numsession+4), open);
//            flip_pages();
//            open += .030;
//        }
//
//        // Tell the user which ones we found
//        stillchoosing = btrue;
//        while(stillchoosing)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, CData.scrx, CData.scry);
//            draw_trim_box(0, 0, 320, fontyspacing*(numsession+4));
//            y = 8;
//            sprintf(text, "Open hosts...");
//            draw_string(text, 14, y);
//            y += fontyspacing;
//            cnt = 0;
//            while(cnt < numsession)
//            {
//                sprintf(text, "%s", NetState.sessionname[cnt]);
//                draw_string(text, 50, y);
//                y += fontyspacing;
//                cnt++;
//            }
//            sprintf(text, "Go Back...");
//            draw_string(text, 50, y);
//            do_cursor();
//            x = cursorx - 50;
//            y = (cursory - 8 - fontyspacing);
//            if(x > 0 && x < 300 && y >= 0)
//            {
//                y = y/fontyspacing;
//                if(y <= numsession)
//                {
//                    if(GMous.button[0] || GMous.button[1])
//                    {
//                        //if(y == numsession)
//                        //{
//                        //    nextmenu = MENUB;
//                        //    stillchoosing = bfalse;
//                        //}
//                        //else
//                        {
//                            if(cl_joinGame("solace2.csusm.edu"))
//                            {
//                                nextmenu = MENUE;
//                                stillchoosing = bfalse;
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
//    char text[256];
//    int x, y, ystt;
//    float open;
//    int cnt;
//    int module;
//    int stillchoosing;
//    if(NetState.ss->Active)
//    {
//        // Figure out how many tags to display
//        numtag = (CData.scry-4-40)/132;
//        ystt = (CData.scry-(numtag*132)-4)>>1;
//
//
//        // Open the tag windows
//        open = 0;
//        while(open < 1.0)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//            y = ystt;
//            cnt = 0;
//            while(cnt < numtag)
//            {
//                draw_trim_box_opening(0, y, 136, y+136, open);
//                draw_trim_box_opening(132, y, CData.scrx, y+136, open);
//                y+=132;
//                cnt++;
//            }
//            flip_pages();
//            open += .030;
//        }
//
//
//
//
//        // Let the user pick a module
//        module = 0;
//        stillchoosing = btrue;
//        while(stillchoosing)
//        {
//            // Draw the tags
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, CData.scrx, CData.scry);
//            y = ystt;
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
//            x = (CData.scrx-40)>>1;
//            draw_string(text, x, 10);
//            sprintf(text, "Down");
//            x = (CData.scrx-80)>>1;
//            draw_string(text, x, CData.scry-fontyspacing-20);
//
//
//            // Handle the mouse
//            do_cursor();
//            y = (cursory - ystt)/132;
//            if(pending_click)
//       {
//         pending_click=bfalse;
//                if(cursory < ystt && module > 0)
//    {
//                    // Up button
//                    module--;
//    }
//                if(y >= numtag && module + numtag < globalnummodule)
//    {
//                    // Down button
//                    module++;
//    }
//  if(cursory > ystt && y > -1 && y < numtag)
//    {
//      y = module + y;
//      if((GMous.button[0] || GMous.button[1]) && y < globalnummodule)
//        {
//   // Set start infow
//   ss->num_players = 1;
//   seed = time(0);
//   gs->modules.uleIndex = y;
//   sprintf(pickedmodule, "%s", ModList[y].loadname);
//   ServerState.ready = btrue;
//   stillchoosing = bfalse;
//        }
//    }
//       }
//            // Check for quitters
//            if(SDLKEYDOWN(SDLK_ESCAPE) && NetState.service == NONETWORK)
//            {
//                nextmenu = MENUB;
//                menuactive = bfalse;
//                stillchoosing = bfalse;
//                GameState.Active = bfalse;
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
//    char text[256];
//    int x, y, starttime, time;
//    float open;
//    int cnt, player;
//    int stillchoosing;
//
//
//    numplayer = 1;
//    if(CData.request_network)
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
//            draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//            draw_trim_box_opening(0, 0, 320, fontyspacing*(numplayer+4), open);
//            flip_pages();
//            open += .030;
//        }
//
//        // Tell the user which ones we found
//        starttime = SDL_GetTicks();
//        stillchoosing = btrue;
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
//   draw_trim_box(0, 0, CData.scrx, CData.scry);
//   draw_trim_box(0, 0, 320, fontyspacing*(numplayer+4));
//
//   if(NetState.ss->Active)
//   {
//    y = 8;
//    sprintf(text, "Active machines...");
//    draw_string(text, 14, y);
//    y += fontyspacing;
//
//    cnt = 0;
//    while(cnt < numplayer)
//    {
//     sprintf(text, "%s", NetState.PlaList[cnt].yername);
//     draw_string(text, 50, y);
//     y += fontyspacing;
//     cnt++;
//    }
//
//    sprintf(text, "Start Game");
//                draw_string(text, 50, y);
//   } else
//   {
//    strncpy(text, "Connected to host:", 256);
//    draw_string(text, 14, 8);
//    draw_string(CData.net_hosts[0], 14, 8 + fontyspacing);
//    listen_for_packets();  // This happens implicitly for the host in sv_letPlayersJoin
//   }
//
//            do_cursor();
//            x = cursorx - 50;
//   // Again, y adjustments were figured out empirically in menu_start_or_join
//            y = (cursory - 21 - fontyspacing);
//
//            if(SDLKEYDOWN(SDLK_ESCAPE)) // !!!BAD!!!
//            {
//                nextmenu = MENUB;
//                menuactive = bfalse;
//                stillchoosing = bfalse;
//                GameState.Active = bfalse;
//            }
//            if(x > 0 && x < 300 && y >= 0 && (GMous.button[0] || GMous.button[1]) && NetState.ss->Active)
//            {
//                // Let the host do things
//                y = y/fontyspacing;
//                if(y < numplayer && NetState.ss->Active)
//                {
//                    // Boot players
//                }
//                if(y == numplayer && ServerState.ready)
//                {
//                    // Start the modules
//                    stillchoosing = bfalse;
//                }
//            }
//            if(ServerState.ready && NetState.ss->Active == bfalse)
//            {
//                // Remotes automatically start
//                stillchoosing = bfalse;
//            }
//            flip_pages();
//        }
//    }
//    if(CData.request_network && NetState.ss->Active)
//    {
//        // Let the host coordinate start
//        stop_players_from_joining();
//        sv_letPlayersJoin();
//        cnt = 0;
//        ServerState.ready = bfalse;
//        if(numplayer == 1)
//        {
//            // Don't need to bother, since the host is alone
//            ServerState.ready = btrue;
//        }
//        while(ServerState.ready==bfalse)
//        {
//            //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//            draw_trim_box(0, 0, CData.scrx, CData.scry);
//            y = 8;
//            sprintf(text, "Waiting for replies...");
//            draw_string(text, 14, y);
//            y += fontyspacing;
//            do_cursor();
//            if(SDLKEYDOWN(SDLK_ESCAPE)) // !!!BAD!!!
//            {
//                nextmenu = MENUB;
//                menuactive = bfalse;
//                stillchoosing = bfalse;
//                GameState.Active = bfalse;
//                ServerState.ready = btrue;
//            }
//            if((cnt&63)==0)
//            {
//                sprintf(text, "  Lell...");
//                draw_string(text, 14, y);
//                player = 0;
//                while(player < numplayer-1)
//                {
//                    net_startNewPacket();
//                    packet_addUnsignedShort(&gPacket, TO_REMOTE_MODULE);
//                    packet_addUnsignedInt(&gPacket, seed);
//                    packet_addUnsignedByte(&gPacket, player+1);
//                    packet_addString(&gPacket, pickedmodule);
////                    send_packet_to_all_players(&gPacket);
//                    net_sendPacketToOnePlayerGuaranteed(player, &gPacket);
//                    player++;
//                }
//            }
//            listen_for_packets();
//            cnt++;
//            flip_pages();
//        }
//    }
//
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
//
//    // Open the text window
//    open = 0;
//    while(open < 1.0)
//    {
//        //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//        draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//        flip_pages();
//        open += .030;
//    }
//
//
//
//    // Wait for input
//    stillchoosing = btrue;
//    while(stillchoosing)
//    {
//        // Show the text
//        //clear_surface(lpDDSBack);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//       glLoadIdentity();
//        draw_trim_box(0, 0, CData.scrx, CData.scry);
//        draw_wrap_string(endtext, 14, 8, CData.scrx-40);
//
//
//
//        // Handle the mouse
//        do_cursor();
//        if(pending_click || SDLKEYDOWN(SDLK_ESCAPE))
//        {
//     pending_click = bfalse;
//            stillchoosing = bfalse;
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
//    char text[1024];
//    int stillchoosing;
//
//
//    //fprintf(stderr,"DIAG: In menu_initial_text()\n");
//    //draw_trim_box(0, 0, CData.scrx, CData.scry);//draw_trim_box(60, 60, 320, 200); // JUST TEST BOX
//
//    // Open the text window
//    open = 0;
//    while(open < 1.0)
//    {
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glLoadIdentity();
//
//        // clear_surface(lpDDSBack); PORT!
//        draw_trim_box_opening(0, 0, CData.scrx, CData.scry, open);
//        flip_pages();
//        open += .030;
//    }
//
// /*fprintf(stderr,"waiting to read a scanf\n");
//    scanf("%s",text);
//    exit(0);*/
//
//    // Wait for input
//    stillchoosing = btrue;
//    while(stillchoosing)
//    {
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//     glLoadIdentity();
//
//        // Show the text
//        // clear_surface(lpDDSBack); PORT!
//        draw_trim_box(0, 0, CData.scrx, CData.scry);
//        sprintf(text, "Egoboo v2.22");
//        draw_string(text, (CData.scrx>>1)-200, ((CData.scry>>1)-30));
//        sprintf(text, "http://egoboo.sourceforge.net");
//        draw_string(text, (CData.scrx>>1)-200, ((CData.scry>>1)));
//        sprintf(text, "See controls.txt to configure input");
//        draw_string(text, (CData.scrx>>1)-200, ((CData.scry>>1)+30));
//
//  // get input
//  read_input();
//
//        // Handle the mouse
//        do_cursor();
//  if ( pending_click || SDLKEYDOWN(SDLK_ESCAPE) )
//  {
//          pending_click = bfalse;
//   stillchoosing = bfalse;
//  }
//        flip_pages();
//    }
//    nextmenu = MENUA;
//}
//
////--------------------------------------------------------------------------------------------
//void fiddle_with_menu()
//{
//    // ZZ> This function gives a nice little menu to play around in.
//
//    menuactive = btrue;
//    ServerState.ready = bfalse;
//    ss->num_players = 0;
//    localmachine = 0;
//    GRTS.localteam = 0;
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
//                    menuaneeded = bfalse;
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
//                menu_pick_player(gs->modules.uleIndex);
//                break;

//            case MENUG:
//                // MENUG...  Let the user read while it loads
//                //printf("MENUG\n");
//                menu_module_loading(gs->modules.uleIndex);
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

////--------------------------------------------------------------------------------------------
//void release_menu_trim()
//{
// // ZZ> This function frees the menu trim memory
// //GLTexture_Release( &TxTrimX );  //RELEASE(lpDDSTrimX);
// //GLTexture_Release( &TxTrimY );  //RELEASE(lpDDSTrimY);
// GLTexture_Release( &TxBlip );  //RELEASE(lpDDSBlip);
// GLTexture_Release( &TxTrim );
//
//}

////--------------------------------------------------------------------------------------------
//void release_menu()
//{
// // ZZ> This function releases all the menu images
// GLTexture_Release( &TxFont );  //RELEASE(lpDDSFont);
//    release_all_titleimages();
//    release_all_icons();
//
//}
#endif
