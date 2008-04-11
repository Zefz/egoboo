/* Egoboo - game.c
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

//#define MENU_DEMO   // Uncomment this to build just the menu demo
#define DECLARE_GLOBALS

#include "egoboo.h"
#include "Ui.h"
#include "Font.h"
#include "Clock.h"
#include "Log.h"
#include "Client.h"
#include "Server.h"
#include "System.h"
#include <SDL_endian.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#define INITGUID
#define NAME "Boo"
#define TITLE "Boo"

#define RELEASE(x) if (x) {x->Release(); x=NULL;}

typedef enum game_stages_t
{
  Stage_Beginning,
  Stage_Entering,
  Stage_Running,
  Stage_Leaving,
  Stage_Finishing
} GameStage;


GameStage doGame(GAME_STATE * gs, float dFrame);
float game_frameStep(GAME_STATE * gs, float dFrame);

TILE_DAMAGE GTile_Dam  = {256, DAMAGEFIRE};
TILE_ANIM GTile_Anim  = {7,3,0xfffc,7,0xfff8,0};
MESSAGE GMsg  = {0,0,0};



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
int what_action(char cTmp)
{
  // ZZ> This function changes a letter into an action code
  int action;
  action = ACTIONDA;
  if (cTmp == 'U' || cTmp == 'u')  action = ACTIONUA;
  if (cTmp == 'T' || cTmp == 't')  action = ACTIONTA;
  if (cTmp == 'S' || cTmp == 's')  action = ACTIONSA;
  if (cTmp == 'C' || cTmp == 'c')  action = ACTIONCA;
  if (cTmp == 'B' || cTmp == 'b')  action = ACTIONBA;
  if (cTmp == 'L' || cTmp == 'l')  action = ACTIONLA;
  if (cTmp == 'X' || cTmp == 'x')  action = ACTIONXA;
  if (cTmp == 'F' || cTmp == 'f')  action = ACTIONFA;
  if (cTmp == 'P' || cTmp == 'p')  action = ACTIONPA;
  if (cTmp == 'Z' || cTmp == 'z')  action = ACTIONZA;
  return action;
}

//------------------------------------------------------------------------------
bool_t memory_cleanUp(GAME_STATE * gs)
{
  //ZF> This function releases all loaded things in memory and cleans up everything properly
  bool_t result = bfalse;

  close_session(gs->ns);					  //Turn off networking
  if (NULL!=gs && gs->moduleActive) release_module(); //Remove memory loaded by a module
  if (mixeron) Mix_CloseAudio();      //Close audio systems
  ui_shutdown();					  //Shut down support systems
  net_shutDown(gs->ns);
  clock_shutdown(g_clk_state);
  sys_shutdown();
  SDL_Quit();						  //Quit SDL

  result = btrue;
  return result;  //Todo: Need to do a check here if the functions above worked
}

//------------------------------------------------------------------------------
//Random Things-----------------------------------------------------------------
//------------------------------------------------------------------------------
void make_newloadname(char *modname, char *appendname, char *newloadname)
{
  // ZZ> This function takes some names and puts 'em together
  int cnt, tnc;
  char ctmp;

  cnt = 0;
  ctmp = modname[cnt];
  while (ctmp != 0)
  {
    newloadname[cnt] = ctmp;
    cnt++;
    ctmp = modname[cnt];
  }
  tnc = 0;
  ctmp = appendname[tnc];
  while (ctmp != 0)
  {
    newloadname[cnt] = ctmp;
    cnt++;
    tnc++;
    ctmp = appendname[tnc];
  }
  newloadname[cnt] = 0;
}

//--------------------------------------------------------------------------------------------
void load_global_waves(char *modname)
{
  // ZZ> This function loads the global waves
  STRING tmploadname, newloadname;
  int cnt;

  if (CData.soundvalid)
  {
    // load in the sounds local to this module
    snprintf(tmploadname, sizeof(tmploadname), "%s%s/", modname, CData.gamedat_dir);
    for (cnt = 0; cnt < MAXWAVE; cnt++)
    {
      snprintf(newloadname, sizeof(newloadname), "%ssound%d.wav", tmploadname, cnt);
      globalwave[cnt] = Mix_LoadWAV(newloadname);
    };

    //These sounds are always standard, but DO NOT override sounds that were loaded local to this module
    if (NULL == globalwave[0])
    {
      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.coinget_sound);
      globalwave[0] = Mix_LoadWAV(CStringTmp1);
    };

    if (NULL == globalwave[1])
    {
      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.defend_sound);
      globalwave[1] = Mix_LoadWAV(CStringTmp1);
    }

    if (NULL == globalwave[4])
    {
      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.coinfall_sound);
      globalwave[4] = Mix_LoadWAV(CStringTmp1);
    };
  }

  /*  The Global Sounds
  * 0 - Pick up coin
  * 1 - Defend clank
  * 2 - Weather Effect
  * 3 - Hit Water tile (Splash)
  * 4 - Coin falls on ground

  //These new values todo should determine sound and particle effects (examples below)
  Weather Type: DROPS, RAIN, SNOW, LAVABUBBLE (Which weather effect to spawn)
  Water Type: LAVA, WATER, DARK (To determine sound and particle effects)

  //We shold also add standard particles that can be used everywhere (Located and
  //loaded in globalparticles folder) such as these below.
  Particle Effect: REDBLOOD, SMOKE, HEALCLOUD
  */
}


//---------------------------------------------------------------------------------------------
void export_one_character(GAME_STATE * gs, int character, int owner, int number, Uint32 * rand_idx)
{
  // ZZ> This function exports a character
  int tnc, profile;
  char letter;
  char fromdir[128];
  char todir[128];
  char fromfile[128];
  char tofile[128];
  char todirname[16];
  char todirfullname[64];
  Uint32 chr_randie = *rand_idx;

  RANDIE(*rand_idx);

  // Don't export enchants
  disenchant_character(gs, character, &chr_randie);

  profile = ChrList[character].model;
  if ((CapList[profile].cancarrytonextmodule || CapList[profile].isitem == bfalse) && gs->modstate.exportvalid)
  {
    // TWINK_BO.OBJ
    snprintf(todirname, sizeof(todirname), "badname.obj");//"BADNAME.OBJ");
    tnc = 0;
    letter = ChrList[owner].name[tnc];
    while (tnc < 8 && letter != 0)
    {
      letter = ChrList[owner].name[tnc];
      if (letter >= 'A' && letter <= 'Z')  letter -= 'A' - 'a';
      if (letter != 0)
      {
        if (letter < 'a' || letter > 'z')  letter = '_';
        todirname[tnc] = letter;
        tnc++;
      }
    }
    todirname[tnc] = '.'; tnc++;
    todirname[tnc] = 'o'; tnc++;
    todirname[tnc] = 'b'; tnc++;
    todirname[tnc] = 'j'; tnc++;
    todirname[tnc] = 0;



    // Is it a character or an item?
    if (owner != character)
    {
      // Item is a subdirectory of the owner directory...
      snprintf(todirfullname, sizeof(todirfullname), "%s/%d.obj", todirname, number);
    }
    else
    {
      // Character directory
      strncpy(todirfullname, todirname, sizeof(todirfullname));
    }


    // players/twink.obj or players/twink.obj/sword.obj
    snprintf(todir, sizeof(todir), "players/%s", todirfullname);
    // modules/advent.mod/objects/advent.obj
    strncpy(fromdir, MadList[profile].name, sizeof(fromdir));


    // Delete all the old items
    if (owner == character)
    {
      tnc = 0;
      while (tnc < 8)
      {
        snprintf(tofile, sizeof(tofile), "%s/%d.obj", todir, tnc); /*.OBJ*/
        fs_removeDirectoryAndContents(tofile);
        tnc++;
      }
    }


    // Make the directory
    fs_createDirectory(todir);


    // Build the DATA.TXT file
    snprintf(tofile, sizeof(tofile), "%s/%s", todir, CData.data_file);  /*DATA.TXT*/
    export_one_character_profile(tofile, character);


    // Build the SKIN.TXT file
    snprintf(tofile, sizeof(tofile), "%s/%s", todir, CData.skin_file);  /*SKIN.TXT*/
    export_one_character_skin(tofile, character);


    // Build the NAMING.TXT file
    snprintf(tofile, sizeof(tofile), "%s/%s", todir, CData.naming_file);  /*NAMING.TXT*/
    export_one_character_name(tofile, character);


    // Copy all of the misc. data files
    snprintf(fromfile, sizeof(fromfile), "%s/%s", fromdir, CData.message_file); /*MESSAGE.TXT*/
    snprintf(tofile, sizeof(tofile), "%s/%s", todir, CData.message_file); /*MESSAGE.TXT*/
    fs_copyFile(fromfile, tofile);

    snprintf(fromfile, sizeof(fromfile), "%s/tris.md2", fromdir);  /*TRIS.MD2*/
    snprintf(tofile,   sizeof(tofile), "%s/tris.md2", todir);  /*TRIS.MD2*/
    fs_copyFile(fromfile, tofile);

    snprintf(fromfile, sizeof(fromfile), "%s/%s", fromdir, CData.copy_file);  /*COPY.TXT*/
    snprintf(tofile,   sizeof(tofile), "%s/%s", todir, CData.copy_file);  /*COPY.TXT*/
    fs_copyFile(fromfile, tofile);

    snprintf(fromfile, sizeof(fromfile), "%s/%s", fromdir, CData.script_file);
    snprintf(tofile,   sizeof(tofile), "%s/%s", todir, CData.script_file);
    fs_copyFile(fromfile, tofile);

    snprintf(fromfile, sizeof(fromfile), "%s/%s", fromdir, CData.enchant_file);
    snprintf(tofile,   sizeof(tofile), "%s/%s", todir, CData.enchant_file);
    fs_copyFile(fromfile, tofile);

    snprintf(fromfile, sizeof(fromfile), "%s/%s", fromdir, CData.credits_file);
    snprintf(tofile,   sizeof(tofile), "%s/%s", todir, CData.credits_file);
    fs_copyFile(fromfile, tofile);


    // Copy all of the particle files
    tnc = 0;
    while (tnc < MAXPRTPIPPEROBJECT)
    {
      snprintf(fromfile, sizeof(fromfile), "%s/part%d.txt", fromdir, tnc);
      snprintf(tofile,   sizeof(tofile), "%s/part%d.txt", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      tnc++;
    }


    // Copy all of the sound files
    tnc = 0;
    while (tnc < MAXWAVE)
    {
      snprintf(fromfile, sizeof(fromfile), "%s/sound%d.wav", fromdir, tnc);
      snprintf(tofile,   sizeof(tofile), "%s/sound%d.wav", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      tnc++;
    }


    // Copy all of the image files
    tnc = 0;
    while (tnc < 4)
    {
      snprintf(fromfile, sizeof(fromfile), "%s/tris%d.bmp", fromdir, tnc);
      snprintf(tofile,   sizeof(tofile), "%s/tris%d.bmp", todir,   tnc);
      fs_copyFile(fromfile, tofile);

      snprintf(fromfile, sizeof(fromfile), "%s/icon%d.bmp", fromdir, tnc);
      snprintf(tofile,   sizeof(tofile), "%s/icon%d.bmp", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      tnc++;
    }
  }
}

//---------------------------------------------------------------------------------------------
void export_all_local_players(GAME_STATE * gs, Uint32 * rand_idx)
{
  // ZZ> This function saves all the local players in the
  //     PLAYERS directory
  int cnt, character, item, number;
  Uint32 loc_randie = *rand_idx;

  RANDIE(*rand_idx);

  // Check each player
  if (gs->modstate.exportvalid)
  {
    cnt = 0;
    while (cnt < MAXPLAYER)
    {
      if (PlaList[cnt].valid && PlaList[cnt].device)
      {
        // Is it alive?
        character = PlaList[cnt].index;
        if (ChrList[character].on && ChrList[character].alive)
        {
          // Export the character
          export_one_character(gs, character, character, 0, &loc_randie);


          // Export the left hand item
          item = ChrList[character].holdingwhich[0];
          if (item != MAXCHR && ChrList[item].isitem)  export_one_character(gs, item, character, 0, &loc_randie);

          // Export the right hand item
          item = ChrList[character].holdingwhich[1];
          if (item != MAXCHR && ChrList[item].isitem)  export_one_character(gs, item, character, 1, &loc_randie);

          // Export the inventory
          number = 2;
          item = ChrList[character].nextinpack;
          while (item != MAXCHR)
          {
            if (ChrList[item].isitem) export_one_character(gs, item, character, number, &loc_randie);
            item = ChrList[item].nextinpack;
            number++;
          }
        }
      }
      cnt++;
    }
  }
};

//---------------------------------------------------------------------------------------------
void quit_module(GAME_STATE * gs)
{
  // ZZ> This function forces a return to the menu

  if(NULL==gs) return;

  gs->moduleActive = bfalse;
  export_all_local_players(gs, &(gs->randie_index));
  gs->paused = bfalse;
  if (gs->cd->soundvalid) Mix_FadeOutChannel(-1, 500);        //Stop all sounds that are playing
}

//--------------------------------------------------------------------------------------------
void quit_game(GAME_STATE * gs)
{
  log_info("Exiting Egoboo %s the good way...\n", VERSION);
  // ZZ> This function exits the game entirely
  if (gs->Active)
  {
    gs->Active = bfalse;
  }
  if (gs->moduleActive)
  {
    quit_module(gs);
  }
  if (Mesh.floatmemory != NULL)
  {
    free(Mesh.floatmemory);
    Mesh.floatmemory = NULL;
  }
}

//--------------------------------------------------------------------------------------------
void goto_colon(FILE* fileread)
{
  // ZZ> This function moves a file read pointer to the next colon
  //    char cTmp;
  Uint32 ch = fgetc(fileread);

  //    fscanf(fileread, "%c", &cTmp);
  while (ch != ':')
  {
    if (ch == EOF)
    {
      // not enough colons in file!
      log_error("There are not enough colons in file! (%s)\n", globalname);
    }

    ch = fgetc(fileread);
  }
}

//--------------------------------------------------------------------------------------------
Uint8 goto_colon_yesno(FILE* fileread)
{
  // ZZ> This function moves a file read pointer to the next colon, or it returns
  //     bfalse if there are no more
  char cTmp;

  do
  {
    if (fscanf(fileread, "%c", &cTmp) == EOF)
    {
      return bfalse;
    }
  }
  while (cTmp != ':');
  return btrue;
}

//--------------------------------------------------------------------------------------------
char get_first_letter(FILE* fileread)
{
  // ZZ> This function returns the next non-whitespace character
  char cTmp;
  fscanf(fileread, "%c", &cTmp);
  while (isspace(cTmp))
  {
    fscanf(fileread, "%c", &cTmp);
  }
  return cTmp;
}

//--------------------------------------------------------------------------------------------
//Tag Reading---------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void reset_tags()
{
  // ZZ> This function resets the tags
  numscantag = 0;
}

//--------------------------------------------------------------------------------------------
int read_tag(FILE *fileread)
{
  // ZZ> This function finds the next tag, returning btrue if it found one
  if (goto_colon_yesno(fileread))
  {
    if (numscantag < MAXTAG)
    {
      fscanf(fileread, "%s%d", tagname[numscantag], &tagvalue[numscantag]);
      numscantag++;
      return btrue;
    }
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void read_all_tags(char *szFilename)
{
  // ZZ> This function reads the scancode.txt file
  FILE* fileread;


  reset_tags();
  fileread = fopen(szFilename, "r");
  if (fileread)
  {
    while (read_tag(fileread));
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
int tag_value(char *string)
{
  // ZZ> This function matches the string with its tag, and returns the value...
  //     It will return 255 if there are no matches.
  int cnt;

  cnt = 0;
  while (cnt < numscantag)
  {
    if (strcmp(string, tagname[cnt]) == 0)
    {
      // They match
      return tagvalue[cnt];
    }
    cnt++;
  }
  // No matches
  return 255;
}

//--------------------------------------------------------------------------------------------
void read_controls(char *szFilename)
{
  // ZZ> This function reads the controls.txt file
  FILE* fileread;
  char currenttag[TAGSIZE];
  int cnt;


  fileread = fopen(szFilename, "r");
  if (fileread)
  {
    cnt = 0;
    while (goto_colon_yesno(fileread) && cnt < MAXCONTROL)
    {
      fscanf(fileread, "%s", currenttag);
      controlvalue[cnt] = tag_value(currenttag);
      //printf("CTRL: %i, %s\n", controlvalue[cnt], currenttag);
      controliskey[cnt] = (currenttag[0] == 'K');
      cnt++;
    }
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
Uint8 control_key_is_pressed(GAME_STATE * gs, Uint8 control)
{
  // ZZ> This function returns btrue if the given control is pressed...
  if (gs->modstate.net_messagemode)  return bfalse;

  if (sdlkeybuffer)
    return (sdlkeybuffer[controlvalue[control]] != 0);
  else
    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint8 control_mouse_is_pressed(GAME_STATE * gs, Uint8 control)
{
  // ZZ> This function returns btrue if the given control is pressed...
  if (controliskey[control])
  {
    if (gs->modstate.net_messagemode)  return bfalse;

    if (sdlkeybuffer)
      return (sdlkeybuffer[controlvalue[control]] != 0);
    else
      return bfalse;
  }
  else
  {
    return (GMous.b == controlvalue[control]);
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint8 control_joya_is_pressed(GAME_STATE * gs, Uint8 control)
{
  // ZZ> This function returns btrue if the given control is pressed...
  if (controliskey[control])
  {
    if (gs->modstate.net_messagemode)  return bfalse;

    if (sdlkeybuffer)
      return (sdlkeybuffer[controlvalue[control]] != 0);
    else
      return bfalse;
  }
  else
  {
    return (GJoy[0].b == controlvalue[control]);
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint8 control_joyb_is_pressed(GAME_STATE * gs, Uint8 control)
{
  // ZZ> This function returns btrue if the given control is pressed...
  if (controliskey[control])
  {
    if (gs->modstate.net_messagemode)  return bfalse;

    if (sdlkeybuffer)
      return (sdlkeybuffer[controlvalue[control]] != 0);
    else
      return bfalse;
  }
  else
  {
    return (GJoy[1].b == controlvalue[control]);
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
char * undo_idsz(IDSZ idsz)
{
  // ZZ> This function takes an integer and makes an text IDSZ out of it.
  //     It will set valueidsz to "NONE" if the idsz is 0
  static char value_string[5] = {"NONE"};

  if (idsz == IDSZNONE)
  {
    snprintf(value_string, sizeof(value_string), "NONE");
  }
  else
  {
    value_string[0] = ((idsz >> 15) & 31) + 'A';
    value_string[1] = ((idsz >> 10) & 31) + 'A';
    value_string[2] = ((idsz >> 5) & 31) + 'A';
    value_string[3] = ((idsz) & 31) + 'A';
    value_string[4] = 0;
  }

  return value_string;
}

//--------------------------------------------------------------------------------------------
IDSZ get_idsz(FILE* fileread)
{
  // ZZ> This function reads and returns an IDSZ tag, or IDSZNONE if there wasn't one
  char sTemp[4];
  IDSZ idsz = IDSZNONE;

  char cTmp = get_first_letter(fileread);
  if (cTmp == '[')
  {
    fscanf(fileread, "%c", &sTemp[0]);
    fscanf(fileread, "%c", &sTemp[1]);
    fscanf(fileread, "%c", &sTemp[2]);
    fscanf(fileread, "%c", &sTemp[3]);
    idsz = MAKE_IDSZ(sTemp);
  }

  return idsz;
}

//--------------------------------------------------------------------------------------------
int get_free_message(void)
{
  // This function finds the best message to use
  // Pick the first one
  int tnc = GMsg.start;
  GMsg.start++;
  GMsg.start = GMsg.start % CData.maxmessage;
  return tnc;
}

//--------------------------------------------------------------------------------------------
void display_message(int message, Uint16 character)
{
  // ZZ> This function sticks a message in the display queue and sets its timer
  int slot, read, write, cnt;
  char *eread;
  STRING szTmp;
  char cTmp, lTmp;

  Uint16 target = ChrList[character].aitarget;
  Uint16 owner = ChrList[character].aiowner;
  if (message < GMsg.total)
  {
    slot = get_free_message();
    GMsg.list[slot].time = MESSAGETIME;
    // Copy the message
    read = GMsg.index[message];
    cnt = 0;
    write = 0;
    cTmp = GMsg.text[read];  read++;
    while (cTmp != 0)
    {
      if (cTmp == '%')
      {
        // Escape sequence
        eread = szTmp;
        szTmp[0] = 0;
        cTmp = GMsg.text[read];  read++;
        if (cTmp == 'n') // Name
        {
          if (ChrList[character].nameknown)
            strncpy(szTmp, ChrList[character].name, sizeof(STRING));
          else
          {
            lTmp = CapList[ChrList[character].model].classname[0];
            if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
              snprintf(szTmp, sizeof(szTmp), "an %s", CapList[ChrList[character].model].classname);
            else
              snprintf(szTmp, sizeof(szTmp), "a %s", CapList[ChrList[character].model].classname);
          }
          if (cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
        }
        if (cTmp == 'c') // Class name
        {
          eread = CapList[ChrList[character].model].classname;
        }
        if (cTmp == 't') // Target name
        {
          if (ChrList[target].nameknown)
            strncpy(szTmp, ChrList[target].name, sizeof(STRING));
          else
          {
            lTmp = CapList[ChrList[target].model].classname[0];
            if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
              snprintf(szTmp, sizeof(szTmp), "an %s", CapList[ChrList[target].model].classname);
            else
              snprintf(szTmp, sizeof(szTmp), "a %s", CapList[ChrList[target].model].classname);
          }
          if (cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
        }
        if (cTmp == 'o') // Owner name
        {
          if (ChrList[owner].nameknown)
            strncpy(szTmp, ChrList[owner].name, sizeof(STRING));
          else
          {
            lTmp = CapList[ChrList[owner].model].classname[0];
            if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
              snprintf(szTmp, sizeof(szTmp), "an %s", CapList[ChrList[owner].model].classname);
            else
              snprintf(szTmp, sizeof(szTmp), "a %s", CapList[ChrList[owner].model].classname);
          }
          if (cnt == 0 && szTmp[0] == 'a')  szTmp[0] = 'A';
        }
        if (cTmp == 's') // Target class name
        {
          eread = CapList[ChrList[target].model].classname;
        }
        if (cTmp >= '0' && cTmp <= '0' + (MAXSKIN - 1))  // Target's skin name
        {
          eread = CapList[ChrList[target].model].skinname[cTmp-'0'];
        }
        if (cTmp == 'd') // tmpdistance value
        {
          snprintf(szTmp, sizeof(szTmp), "%d", valuetmpdistance);
        }
        if (cTmp == 'x') // tmpx value
        {
          snprintf(szTmp, sizeof(szTmp), "%d", valuetmpx);
        }
        if (cTmp == 'y') // tmpy value
        {
          snprintf(szTmp, sizeof(szTmp), "%d", valuetmpy);
        }
        if (cTmp == 'D') // tmpdistance value
        {
          snprintf(szTmp, sizeof(szTmp), "%2d", valuetmpdistance);
        }
        if (cTmp == 'X') // tmpx value
        {
          snprintf(szTmp, sizeof(szTmp), "%2d", valuetmpx);
        }
        if (cTmp == 'Y') // tmpy value
        {
          snprintf(szTmp, sizeof(szTmp), "%2d", valuetmpy);
        }
        if (cTmp == 'a') // Character's ammo
        {
          if (ChrList[character].ammoknown)
            snprintf(szTmp, sizeof(szTmp), "%d", ChrList[character].ammo);
          else
            snprintf(szTmp, sizeof(szTmp), "?");
        }
        if (cTmp == 'k') // Kurse state
        {
          if (ChrList[character].iskursed)
            snprintf(szTmp, sizeof(szTmp), "kursed");
          else
            snprintf(szTmp, sizeof(szTmp), "unkursed");
        }
        if (cTmp == 'p') // Character's possessive
        {
          if (ChrList[character].gender == GENFEMALE)
          {
            snprintf(szTmp, sizeof(szTmp), "her");
          }
          else
          {
            if (ChrList[character].gender == GENMALE)
            {
              snprintf(szTmp, sizeof(szTmp), "his");
            }
            else
            {
              snprintf(szTmp, sizeof(szTmp), "its");
            }
          }
        }
        if (cTmp == 'm') // Character's gender
        {
          if (ChrList[character].gender == GENFEMALE)
          {
            snprintf(szTmp, sizeof(szTmp), "female ");
          }
          else
          {
            if (ChrList[character].gender == GENMALE)
            {
              snprintf(szTmp, sizeof(szTmp), "male ");
            }
            else
            {
              snprintf(szTmp, sizeof(szTmp), " ");
            }
          }
        }
        if (cTmp == 'g') // Target's possessive
        {
          if (ChrList[target].gender == GENFEMALE)
          {
            snprintf(szTmp, sizeof(szTmp), "her");
          }
          else
          {
            if (ChrList[target].gender == GENMALE)
            {
              snprintf(szTmp, sizeof(szTmp), "his");
            }
            else
            {
              snprintf(szTmp, sizeof(szTmp), "its");
            }
          }
        }
        cTmp = *eread;  eread++;
        while (cTmp != 0 && write < MESSAGESIZE - 1)
        {
          GMsg.list[slot].textdisplay[write] = cTmp;
          cTmp = *eread;  eread++;
          write++;
        }
      }
      else
      {
        // Copy the letter
        if (write < MESSAGESIZE - 1)
        {
          GMsg.list[slot].textdisplay[write] = cTmp;
          write++;
        }
      }
      cTmp = GMsg.text[read];  read++;
      cnt++;
    }
    GMsg.list[slot].textdisplay[write] = 0;
  }
}

//--------------------------------------------------------------------------------------------
void remove_enchant(GAME_STATE * gs, Uint16 enchantindex, Uint32 * rand_idx)
{
  // ZZ> This function removes a specific enchantment and adds it to the unused list
  Uint16 character, overlay;
  Uint16 lastenchant, currentenchant;
  int add, cnt;
  Uint32 enc_randie = *rand_idx;

  RANDIE(*rand_idx);


  if (enchantindex < MAXENCHANT)
  {
    if (EncList[enchantindex].on)
    {
      // Unsparkle the spellbook
      character = EncList[enchantindex].spawner;
      if (character < MAXCHR)
      {
        ChrList[character].sparkle = NOSPARKLE;
        // Make the spawner unable to undo the enchantment
        if (ChrList[character].undoenchant == enchantindex)
        {
          ChrList[character].undoenchant = MAXENCHANT;
        }
      }


      // Play the end sound
      character = EncList[enchantindex].target;
      if (EveList[EncList[enchantindex].eve].waveindex != NULL)
        play_sound(ChrList[character].oldx, ChrList[character].oldy, EveList[EncList[enchantindex].eve].waveindex);


      // Unset enchant values, doing morph last
      for(cnt=SETDAMAGETYPE; cnt<=SETMORPH; cnt++)
        unset_enchant_value(gs, enchantindex, cnt, &enc_randie);

      // Remove all of the cumulative values
      add = 0;
      while (add < MAXEVEADDVALUE)
      {
        remove_enchant_value(enchantindex, add);
        add++;
      }


      // Unlink it
      if (ChrList[character].firstenchant == enchantindex)
      {
        // It was the first in the list
        ChrList[character].firstenchant = EncList[enchantindex].nextenchant;
      }
      else
      {
        // Search until we find it
        lastenchant    = MAXENCHANT;
        currentenchant = ChrList[character].firstenchant;
        while (currentenchant != enchantindex)
        {
          lastenchant = currentenchant;
          currentenchant = EncList[currentenchant].nextenchant;
        }

        // Relink the last enchantment
        EncList[lastenchant].nextenchant = EncList[enchantindex].nextenchant;
      }



      // See if we spit out an end message
      if (EveList[EncList[enchantindex].eve].endmessage >= 0)
      {
        display_message(MadList[EncList[enchantindex].eve].msg_start + EveList[EncList[enchantindex].eve].endmessage, EncList[enchantindex].target);
      }
      // Check to see if we spawn a poof
      if (EveList[EncList[enchantindex].eve].poofonend)
      {
        spawn_poof(EncList[enchantindex].target, EncList[enchantindex].eve, &enc_randie);
      }
      // Check to see if the character dies
      if (EveList[EncList[enchantindex].eve].killonend)
      {
        if (ChrList[character].invictus)  TeamList[ChrList[character].baseteam].morale++;
        ChrList[character].invictus = bfalse;
        kill_character(gs, character, MAXCHR, &enc_randie);
      }
      // Kill overlay too...
      overlay = EncList[enchantindex].overlay;
      if (overlay < MAXCHR)
      {
        if (ChrList[overlay].invictus)  TeamList[ChrList[overlay].baseteam].morale++;
        ChrList[overlay].invictus = bfalse;
        kill_character(gs, overlay, MAXCHR, &enc_randie);
      }





      // Now get rid of it
      EncList[enchantindex].on = bfalse;
      freeenchant[numfreeenchant] = enchantindex;
      numfreeenchant++;


      // Now fix dem weapons
      reset_character_alpha(gs, ChrList[character].holdingwhich[0], &enc_randie);
      reset_character_alpha(gs, ChrList[character].holdingwhich[1], &enc_randie);
    }
  }
}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled(Uint16 enchantindex, Uint8 valueindex)
{
  // ZZ> This function returns MAXENCHANT if the enchantment's target has no conflicting
  //     set values in its other enchantments.  Otherwise it returns the enchantindex
  //     of the conflicting enchantment
  Uint16 character, currenchant;

  character = EncList[enchantindex].target;
  currenchant = ChrList[character].firstenchant;
  while (currenchant != MAXENCHANT)
  {
    if (EncList[currenchant].setyesno[valueindex] == btrue)
    {
      return currenchant;
    }
    currenchant = EncList[currenchant].nextenchant;
  }
  return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value(GAME_STATE * gs, Uint16 enchantindex, Uint8 valueindex,
                       Uint16 enchanttype, Uint32 * rand_idx)
{
  // ZZ> This function sets and saves one of the character's stats
  Uint16 conflict, character;
  Uint32 enc_randie = *rand_idx;

  RANDIE(*rand_idx);


  EncList[enchantindex].setyesno[valueindex] = bfalse;
  if (EveList[enchanttype].setyesno[valueindex])
  {
    conflict = enchant_value_filled(enchantindex, valueindex);
    if (conflict == MAXENCHANT || EveList[enchanttype].override)
    {
      // Check for multiple enchantments
      if (conflict < MAXENCHANT)
      {
        // Multiple enchantments aren't allowed for sets
        if (EveList[enchanttype].removeoverridden)
        {
          // Kill the old enchantment
          remove_enchant(gs, conflict, &enc_randie);
        }
        else
        {
          // Just unset the old enchantment's value
          unset_enchant_value(gs, conflict, valueindex, &enc_randie);
        }
      }
      // Set the value, and save the character's real stat
      character = EncList[enchantindex].target;
      EncList[enchantindex].setyesno[valueindex] = btrue;
      switch (valueindex)
      {
      case SETDAMAGETYPE:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagetargettype;
        ChrList[character].damagetargettype = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETNUMBEROFJUMPS:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].jumpnumberreset;
        ChrList[character].jumpnumberreset = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETLIFEBARCOLOR:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].lifecolor;
        ChrList[character].lifecolor = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETMANABARCOLOR:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].manacolor;
        ChrList[character].manacolor = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETSLASHMODIFIER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGESLASH];
        ChrList[character].damagemodifier[DAMAGESLASH] = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETCRUSHMODIFIER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGECRUSH];
        ChrList[character].damagemodifier[DAMAGECRUSH] = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETPOKEMODIFIER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGEPOKE];
        ChrList[character].damagemodifier[DAMAGEPOKE] = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETHOLYMODIFIER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGEHOLY];
        ChrList[character].damagemodifier[DAMAGEHOLY] = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETEVILMODIFIER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGEEVIL];
        ChrList[character].damagemodifier[DAMAGEEVIL] = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETFIREMODIFIER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGEFIRE];
        ChrList[character].damagemodifier[DAMAGEFIRE] = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETICEMODIFIER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGEICE];
        ChrList[character].damagemodifier[DAMAGEICE] = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETZAPMODIFIER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].damagemodifier[DAMAGEZAP];
        ChrList[character].damagemodifier[DAMAGEZAP] = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETFLASHINGAND:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].flashand;
        ChrList[character].flashand = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETLIGHTBLEND:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].light;
        ChrList[character].light = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETALPHABLEND:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].alpha;
        ChrList[character].alpha = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETSHEEN:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].sheen;
        ChrList[character].sheen = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETFLYTOHEIGHT:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].flyheight;
        if (ChrList[character].flyheight == 0 && ChrList[character].zpos > -2)
        {
          ChrList[character].flyheight = EveList[enchanttype].setvalue[valueindex];
        }
        break;

      case SETWALKONWATER:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].waterwalk;
        if (ChrList[character].waterwalk == bfalse)
        {
          ChrList[character].waterwalk = EveList[enchanttype].setvalue[valueindex];
        }
        break;

      case SETCANSEEINVISIBLE:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].canseeinvisible;
        ChrList[character].canseeinvisible = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETMISSILETREATMENT:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].missiletreatment;
        ChrList[character].missiletreatment = EveList[enchanttype].setvalue[valueindex];
        break;

      case SETCOSTFOREACHMISSILE:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].missilecost;
        ChrList[character].missilecost = EveList[enchanttype].setvalue[valueindex];
        ChrList[character].missilehandler = EncList[enchantindex].owner;
        break;

      case SETMORPH:
        EncList[enchantindex].setsave[valueindex] = (ChrList[character].texture - MadList[ChrList[character].model].skinstart) % MAXSKIN;
        // Special handler for morph
        change_character(gs, character, enchanttype, 0, LEAVEALL, &enc_randie); // LEAVEFIRST);
        ChrList[character].alert |= ALERTIFCHANGED;
        break;

      case SETCHANNEL:
        EncList[enchantindex].setsave[valueindex] = ChrList[character].canchannel;
        ChrList[character].canchannel = EveList[enchanttype].setvalue[valueindex];
        break;

      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void getadd(int MIN, int value, int MAX, int* valuetoadd)
{
  // ZZ> This function figures out what value to add should be in order
  //     to not overflow the MIN and MAX bounds
  int newvalue;

  newvalue = value + (*valuetoadd);
  if (newvalue < MIN)
  {
    // Increase valuetoadd to fit
    *valuetoadd = MIN - value;
    if (*valuetoadd > 0)  *valuetoadd = 0;
    return;
  }


  if (newvalue > MAX)
  {
    // Decrease valuetoadd to fit
    *valuetoadd = MAX - value;
    if (*valuetoadd < 0)  *valuetoadd = 0;
  }
}

//--------------------------------------------------------------------------------------------
void fgetadd(float MIN, float value, float MAX, float* valuetoadd)
{
  // ZZ> This function figures out what value to add should be in order
  //     to not overflow the MIN and MAX bounds
  float newvalue;


  newvalue = value + (*valuetoadd);
  if (newvalue < MIN)
  {
    // Increase valuetoadd to fit
    *valuetoadd = MIN - value;
    if (*valuetoadd > 0)  *valuetoadd = 0;
    return;
  }


  if (newvalue > MAX)
  {
    // Decrease valuetoadd to fit
    *valuetoadd = MAX - value;
    if (*valuetoadd < 0)  *valuetoadd = 0;
  }
}

//--------------------------------------------------------------------------------------------
void add_enchant_value(Uint16 enchantindex, Uint8 valueindex,
                       Uint16 enchanttype)
{
  // ZZ> This function does cumulative modification to character stats
  int valuetoadd, newvalue;
  float fvaluetoadd, fnewvalue;
  Uint16 character;


  character = EncList[enchantindex].target;
  valuetoadd = 0;
  switch (valueindex)
  {
  case ADDJUMPPOWER:
    fnewvalue = ChrList[character].jump;
    fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 16.0;
    fgetadd(0, fnewvalue, 30.0, &fvaluetoadd);
    valuetoadd = fvaluetoadd * 16.0; // Get save value
    fvaluetoadd = valuetoadd / 16.0;
    ChrList[character].jump += fvaluetoadd;
    break;

  case ADDBUMPDAMPEN:
    fnewvalue = ChrList[character].bumpdampen;
    fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 128.0;
    fgetadd(0, fnewvalue, 1.0, &fvaluetoadd);
    valuetoadd = fvaluetoadd * 128.0; // Get save value
    fvaluetoadd = valuetoadd / 128.0;
    ChrList[character].bumpdampen += fvaluetoadd;
    break;

  case ADDBOUNCINESS:
    fnewvalue = ChrList[character].dampen;
    fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 128.0;
    fgetadd(0, fnewvalue, 0.95, &fvaluetoadd);
    valuetoadd = fvaluetoadd * 128.0; // Get save value
    fvaluetoadd = valuetoadd / 128.0;
    ChrList[character].dampen += fvaluetoadd;
    break;

  case ADDDAMAGE:
    newvalue = ChrList[character].damageboost;
    valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
    getadd(0, newvalue, 4096, &valuetoadd);
    ChrList[character].damageboost += valuetoadd;
    break;

  case ADDSIZE:
    fnewvalue = ChrList[character].sizegoto;
    fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 128.0;
    fgetadd(0.5, fnewvalue, 2.0, &fvaluetoadd);
    valuetoadd = fvaluetoadd * 128.0; // Get save value
    fvaluetoadd = valuetoadd / 128.0;
    ChrList[character].sizegoto += fvaluetoadd;
    ChrList[character].sizegototime = SIZETIME;
    break;

  case ADDACCEL:
    fnewvalue = ChrList[character].maxaccel;
    fvaluetoadd = EveList[enchanttype].addvalue[valueindex] / 25.0;
    fgetadd(0, fnewvalue, 1.5, &fvaluetoadd);
    valuetoadd = fvaluetoadd * 1000.0; // Get save value
    fvaluetoadd = valuetoadd / 1000.0;
    ChrList[character].maxaccel += fvaluetoadd;
    break;

  case ADDRED:
    newvalue = ChrList[character].redshift;
    valuetoadd = EveList[enchanttype].addvalue[valueindex];
    getadd(0, newvalue, 6, &valuetoadd);
    ChrList[character].redshift += valuetoadd;
    break;

  case ADDGRN:
    newvalue = ChrList[character].grnshift;
    valuetoadd = EveList[enchanttype].addvalue[valueindex];
    getadd(0, newvalue, 6, &valuetoadd);
    ChrList[character].grnshift += valuetoadd;
    break;

  case ADDBLU:
    newvalue = ChrList[character].blushift;
    valuetoadd = EveList[enchanttype].addvalue[valueindex];
    getadd(0, newvalue, 6, &valuetoadd);
    ChrList[character].blushift += valuetoadd;
    break;

  case ADDDEFENSE:
    newvalue = ChrList[character].defense;
    valuetoadd = EveList[enchanttype].addvalue[valueindex];
    getadd(55, newvalue, 255, &valuetoadd);  // Don't fix again!
    ChrList[character].defense += valuetoadd;
    break;

  case ADDMANA:
    newvalue = ChrList[character].manamax;
    valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
    getadd(0, newvalue, HIGHSTAT, &valuetoadd);
    ChrList[character].manamax += valuetoadd;
    ChrList[character].mana += valuetoadd;
    if (ChrList[character].mana < 0)  ChrList[character].mana = 0;
    break;

  case ADDLIFE:
    newvalue = ChrList[character].lifemax;
    valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
    getadd(LOWSTAT, newvalue, HIGHSTAT, &valuetoadd);
    ChrList[character].lifemax += valuetoadd;
    ChrList[character].life += valuetoadd;
    if (ChrList[character].life < 1)  ChrList[character].life = 1;
    break;

  case ADDSTRENGTH:
    newvalue = ChrList[character].strength;
    valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
    getadd(0, newvalue, PERFECTSTAT, &valuetoadd);
    ChrList[character].strength += valuetoadd;
    break;

  case ADDWISDOM:
    newvalue = ChrList[character].wisdom;
    valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
    getadd(0, newvalue, PERFECTSTAT, &valuetoadd);
    ChrList[character].wisdom += valuetoadd;
    break;

  case ADDINTELLIGENCE:
    newvalue = ChrList[character].intelligence;
    valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
    getadd(0, newvalue, PERFECTSTAT, &valuetoadd);
    ChrList[character].intelligence += valuetoadd;
    break;

  case ADDDEXTERITY:
    newvalue = ChrList[character].dexterity;
    valuetoadd = EveList[enchanttype].addvalue[valueindex] << 6;
    getadd(0, newvalue, PERFECTSTAT, &valuetoadd);
    ChrList[character].dexterity += valuetoadd;
    break;
  }

  EncList[enchantindex].addsave[valueindex] = valuetoadd;  // Save the value for undo
}


//--------------------------------------------------------------------------------------------
Uint16 spawn_enchant(GAME_STATE * gs, Uint16 owner, Uint16 target,
                             Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional, Uint32 * rand_idx)
{
  // ZZ> This function enchants a target, returning the enchantment index or MAXENCHANT
  //     if failed
  Uint16 enchanttype, overlay;
  int add, cnt;
  Uint32 enc_randie = *rand_idx;

  // do exactly one iteration
  RANDIE(*rand_idx);


  if (modeloptional < MAXMODEL)
  {
    // The enchantment type is given explicitly
    enchanttype = modeloptional;
  }
  else
  {
    // The enchantment type is given by the spawner
    enchanttype = ChrList[spawner].model;
  }


  // Target and owner must both be alive and on and valid
  if (target < MAXCHR)
  {
    if (!ChrList[target].on || !ChrList[target].alive)
      return MAXENCHANT;
  }
  else
  {
    // Invalid target
    return MAXENCHANT;
  }
  if (owner < MAXCHR)
  {
    if (!ChrList[owner].on || !ChrList[owner].alive)
      return MAXENCHANT;
  }
  else
  {
    // Invalid target
    return MAXENCHANT;
  }


  if (EveList[enchanttype].valid)
  {
    if (enchantindex == MAXENCHANT)
    {
      // Should it choose an inhand item?
      if (EveList[enchanttype].retarget)
      {
        // Is at least one valid?
        if (ChrList[target].holdingwhich[0] == MAXCHR && ChrList[target].holdingwhich[1] == MAXCHR)
        {
          // No weapons to pick
          return MAXENCHANT;
        }
        // Left, right, or both are valid
        if (ChrList[target].holdingwhich[0] == MAXCHR)
        {
          // Only right hand is valid
          target = ChrList[target].holdingwhich[1];
        }
        else
        {
          // Pick left hand
          target = ChrList[target].holdingwhich[0];
        }
      }


      // Make sure it's valid
      if (EveList[enchanttype].dontdamagetype != DAMAGENULL)
      {
        if ((ChrList[target].damagemodifier[EveList[enchanttype].dontdamagetype]&7) >= 3)  // Invert | Shift = 7
        {
          return MAXENCHANT;
        }
      }
      if (EveList[enchanttype].onlydamagetype != DAMAGENULL)
      {
        if (ChrList[target].damagetargettype != EveList[enchanttype].onlydamagetype)
        {
          return MAXENCHANT;
        }
      }


      // Find one to use
      enchantindex = get_free_enchant();
    }
    else
    {
      numfreeenchant--;  // To keep it in order
    }
    if (enchantindex < MAXENCHANT)
    {
      // Make a new one
      EncList[enchantindex].on = btrue;
      EncList[enchantindex].target = target;
      EncList[enchantindex].owner = owner;
      EncList[enchantindex].spawner = spawner;
      if (spawner < MAXCHR)
      {
        ChrList[spawner].undoenchant = enchantindex;
      }
      EncList[enchantindex].eve = enchanttype;
      EncList[enchantindex].time = EveList[enchanttype].time;
      EncList[enchantindex].spawntime = 1;
      EncList[enchantindex].ownermana = EveList[enchanttype].ownermana;
      EncList[enchantindex].ownerlife = EveList[enchanttype].ownerlife;
      EncList[enchantindex].targetmana = EveList[enchanttype].targetmana;
      EncList[enchantindex].targetlife = EveList[enchanttype].targetlife;



      // Add it as first in the list
      EncList[enchantindex].nextenchant = ChrList[target].firstenchant;
      ChrList[target].firstenchant = enchantindex;


      // Now set all of the specific values, morph first
      for(cnt=SETMORPH; cnt<=SETCHANNEL; cnt++)
        set_enchant_value(gs, enchantindex, cnt, enchanttype, &enc_randie);

      // Now do all of the stat adds
      add = 0;
      while (add < MAXEVEADDVALUE)
      {
        add_enchant_value(enchantindex, add, enchanttype);
        add++;
      }


      // Create an overlay character?
      EncList[enchantindex].overlay = MAXCHR;
      if (EveList[enchanttype].overlay)
      {
        overlay = spawn_one_character(ChrList[target].xpos, ChrList[target].ypos, ChrList[target].zpos,
                                      enchanttype, ChrList[target].team, 0, ChrList[target].turnleftright,
                                      NULL, MAXCHR, &enc_randie);
        if (overlay < MAXCHR)
        {
          EncList[enchantindex].overlay = overlay;  // Kill this character on end...
          ChrList[overlay].aitarget = target;
          ChrList[overlay].aistate = EveList[enchanttype].overlay;
          ChrList[overlay].overlay = btrue;


          // Start out with ActionMJ...  Object activated
          if (MadList[ChrList[overlay].model].actionvalid[ACTIONMJ])
          {
            ChrList[overlay].action = ACTIONMJ;
            ChrList[overlay].lip = 0;
            ChrList[overlay].frame = MadList[ChrList[overlay].model].actionstart[ACTIONMJ];
            ChrList[overlay].lastframe = ChrList[overlay].frame;
            ChrList[overlay].actionready = bfalse;
          }
          ChrList[overlay].light = 254;  // Assume it's transparent...
        }
      }
    }
    return enchantindex;
  }
  return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void load_action_names(char* loadname)
{
  // ZZ> This function loads all of the 2 letter action names
  FILE* fileread;
  int cnt;
  char first, second;

  fileread = fopen(loadname, "r");
  if (fileread)
  {
    cnt = 0;
    while (cnt < MAXACTION)
    {
      goto_colon(fileread);
      fscanf(fileread, "%c%c", &first, &second);
      cActionName[cnt][0] = first;
      cActionName[cnt][1] = second;
      cnt++;
    }
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
void get_name(FILE* fileread, char *szName)
{
  // ZZ> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
  //     it for underscores.  The szName argument is rewritten with the null terminated
  //     string
  int cnt;
  char cTmp;
  STRING szTmp;


  fscanf(fileread, "%s", szTmp);
  cnt = 0;
  while (cnt < MAXCAPNAMESIZE - 1)
  {
    cTmp = szTmp[cnt];
    if (cTmp == '_')  cTmp = ' ';
    szName[cnt] = cTmp;
    cnt++;
  }
  szName[cnt] = 0;
}

//--------------------------------------------------------------------------------------------
void read_setup(char* filename)
{
  // ZZ> This function loads the setup file

  ConfigFilePtr lConfigSetup;
  char lCurSectionName[64];
  bool_t lTempBool;
  Sint32 lTmpInt;
  char lTmpStr[24];
  STRING tmpstring = {0};
  Uint32 cnt;


  lConfigSetup = OpenConfigFile(filename);
  if (lConfigSetup == NULL)
  {
    //Major Error
    log_error("Could not find setup file (%s).\n", filename);
  }
  else
  {
    globalname = filename; // heu!?

    /*********************************************

    GRAPHIC Section

    *********************************************/

    strcpy(lCurSectionName, "GRAPHIC");

    //Draw z reflection?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "Z_REFLECTION", &CData.zreflect) == 0)
    {
      CData.zreflect = bfalse; // default
    }

    //Max number of vertrices (Should always be 100!)
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "MAX_NUMBER_VERTICES", &lTmpInt) == 0)
    {
      lTmpInt = 100; // default
    }
    CData.maxtotalmeshvertices = lTmpInt * 1024;

    //Do CData.fullscreen?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "FULLSCREEN", &CData.fullscreen) == 0)
    {
      CData.fullscreen = bfalse; // default
    }

    //Screen Size
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "SCREENSIZE_X", &lTmpInt) == 0)
    {
      lTmpInt = 640; // default
    }
    CData.scrx = lTmpInt;
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "SCREENSIZE_Y", &lTmpInt) == 0)
    {
      lTmpInt = 480; // default
    }
    CData.scry = lTmpInt;

    //Color depth
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "COLOR_DEPTH", &lTmpInt) == 0)
    {
      lTmpInt = 16; // default
    }
    CData.scrd = lTmpInt;

    //The z depth
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "Z_DEPTH", &lTmpInt) == 0)
    {
      lTmpInt = 16; // default
    }
    CData.scrz = lTmpInt;

    //Max number of messages displayed
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "MAX_TEXT_MESSAGE", &lTmpInt) == 0)
    {
      lTmpInt = 1; // default
    }
    CData.messageon = btrue;
    CData.maxmessage = lTmpInt;
    if (CData.maxmessage < 1)  { CData.maxmessage = 1;  CData.messageon = bfalse; }
    if (CData.maxmessage > MAXMESSAGE)  { CData.maxmessage = MAXMESSAGE; }

    //Show status bars? (Life, mana, character icons, etc.)
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "STATUS_BAR", &CData.staton) == 0)
    {
      CData.staton = btrue; // default
    }
    CData.wraptolerance = 32;
    if (CData.staton == btrue)
    {
      CData.wraptolerance = 90;
    }

    //Perspective correction
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "PERSPECTIVE_CORRECT", &CData.perspective) == 0)
    {
      CData.perspective = bfalse; // default
    }

    //Enable dithering? (Reduces quality but increases preformance)
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "DITHERING", &CData.dither) == 0)
    {
      CData.dither = bfalse; // default
    }

    //Reflection fadeout
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "FLOOR_REFLECTION_FADEOUT", &lTempBool) == 0)
    {
      lTempBool = bfalse; // default
    }
    if (lTempBool)
    {
      CData.reffadeor = 0;
    }
    else
    {
      CData.reffadeor = 255;
    }

    //Draw Reflection?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "REFLECTION", &CData.refon) == 0)
    {
      CData.refon = bfalse; // default
    }

    //Draw shadows?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "SHADOWS", &CData.shaon) == 0)
    {
      CData.shaon = bfalse; // default
    }

    //Draw good shadows (BAD! Not working yet)
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "SHADOW_AS_SPRITE", &CData.shasprite) == 0)
    {
      CData.shasprite = btrue; // default
    }

    //Draw phong mapping?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "PHONG", &CData.phongon) == 0)
    {
      CData.phongon = btrue; // default
    }

    //Draw water with more layers?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "MULTI_LAYER_WATER", &CData.twolayerwateron) == 0)
    {
      CData.twolayerwateron = bfalse; // default
    }

    //TODO: This is not implemented
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "OVERLAY", &CData.overlayvalid) == 0)
    {
      CData.overlayvalid = bfalse; // default
    }

    //Allow backgrounds?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "BACKGROUND", &CData.backgroundvalid) == 0)
    {
      CData.backgroundvalid = bfalse; // default
    }

    //Enable fog?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "FOG", &CData.fogallowed) == 0)
    {
      CData.fogallowed = bfalse; // default
    }

    //Do gourad CData.shading?
    CData.shading = GL_FLAT; // default
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "GOURAUD_SHADING", &lTempBool) != 0)
    {
      CData.shading = lTempBool ? GL_SMOOTH : GL_FLAT;
    }

    //Enable CData.antialiasing?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "ANTIALIASING", &CData.antialiasing) == 0)
    {
      CData.antialiasing = bfalse; // default
    }

    //Do we do texture filtering?

    if (GetConfigValue(lConfigSetup, lCurSectionName, "TEXTURE_FILTERING", lTmpStr, 24) == 0)
    {
      CData.texturefilter = 1;
    }
    else if (isdigit(lTmpStr[0]))
    {
      sscanf(lTmpStr, "%d", &CData.texturefilter);
      if (CData.texturefilter >= TX_ANISOTROPIC)
      {
        int tmplevel = CData.texturefilter - TX_ANISOTROPIC + 1;
        userAnisotropy = 1 << tmplevel;
      }
    }
    else if (lTmpStr[0] == 'L' || lTmpStr[0] == 'l')  CData.texturefilter = TX_LINEAR;
    else if (lTmpStr[0] == 'B' || lTmpStr[0] == 'b')  CData.texturefilter = TX_BILINEAR;
    else if (lTmpStr[0] == 'T' || lTmpStr[0] == 't')  CData.texturefilter = TX_TRILINEAR_2;
    else if (lTmpStr[0] == 'A' || lTmpStr[0] == 'a')  CData.texturefilter = TX_ANISOTROPIC + log2Anisotropy;


    /*********************************************

    SOUND Section

    *********************************************/

    strcpy(lCurSectionName, "SOUND");

    //Enable sound
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "SOUND", &CData.soundvalid) == 0)
    {
      CData.soundvalid = bfalse; // default
    }

    //Enable music
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "MUSIC", &CData.musicvalid) == 0)
    {
      CData.musicvalid = bfalse; // default
    }

    //Music volume
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "MUSIC_VOLUME", &CData.musicvolume) == 0)
    {
      CData.musicvolume = 50; // default
    }

    //Sound volume
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "SOUND_VOLUME", &CData.soundvolume) == 0)
    {
      CData.soundvolume = 75; // default
    }

    //Max number of sound channels playing at the same time
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "MAX_SOUND_CHANNEL", &CData.maxsoundchannel) == 0)
    {
      CData.maxsoundchannel = 16; // default
    }
    if (CData.maxsoundchannel < 8) CData.maxsoundchannel = 8;
    if (CData.maxsoundchannel > 32) CData.maxsoundchannel = 32;

    //The output buffer size
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "OUPUT_BUFFER_SIZE", &CData.buffersize) == 0)
    {
      CData.buffersize = 2048; // default
    }
    if (CData.buffersize < 512) CData.buffersize = 512;
    if (CData.buffersize > 8196) CData.buffersize = 8196;


    /*********************************************

    CONTROL Section

    *********************************************/

    strcpy(lCurSectionName, "CONTROL");

    //Camera control mode
    if (GetConfigValue(lConfigSetup, lCurSectionName, "AUTOTURN_CAMERA", lTmpStr, 24) == 0)
    {
      strcpy(lTmpStr, "GOOD");   // default
    }

    if (lTmpStr[0] == 'G' || lTmpStr[0] == 'g')  CData.autoturncamera = 255;
    if (lTmpStr[0] == 'T' || lTmpStr[0] == 't')  CData.autoturncamera = btrue;
    if (lTmpStr[0] == 'F' || lTmpStr[0] == 'f')  CData.autoturncamera = bfalse;

    //[claforte] Force CData.autoturncamera to bfalse, or else it doesn't move right.
    //CData.autoturncamera = bfalse;



    /*********************************************

    NETWORK Section

    *********************************************/

    strcpy(lCurSectionName, "NETWORK");

    //Enable networking systems?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "NETWORK_ON", &CData.request_network) == 0)
    {
      CData.request_network = bfalse; // default
    }

    //Max CData.lag
    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "LAG_TOLERANCE", &lTmpInt) == 0)
    {
      lTmpInt = 2; // default
    }
    CData.lag = lTmpInt;

    /*
    goto_colon(fileread); fscanf(fileread, "%d", &CData.GOrder.lag);

    if ( GetConfigIntValue( lConfigSetup, lCurSectionName, "RTS_LAG_TOLERANCE", &lTmpInt ) == 0 )
    {
    lTmpInt = 25; // default
    }
    CData.GOrder.lag = lTmpInt;
    */

    // load the names of network hosts
    {
      if (GetConfigValue(lConfigSetup, lCurSectionName, "HOST_NAME", CData.net_hosts[0], sizeof(CData.net_hosts[0])) == 0)
      {
        strncpy(CData.net_hosts[0], "localhost", sizeof(CData.net_hosts[0]));   // default
      }

      for(cnt=1; cnt<MAXNETPLAYER; cnt++)
      {
        snprintf(tmpstring, sizeof(tmpstring), "HOST_NAME%d", cnt);
        if (GetConfigValue(lConfigSetup, lCurSectionName, tmpstring, CData.net_hosts[cnt], sizeof(CData.net_hosts[0])) == 0)
        {
          CData.net_hosts[cnt][0] = 0x00;
        }
      };
    }

    //Multiplayer name
    if (GetConfigValue(lConfigSetup, lCurSectionName, "MULTIPLAYER_NAME", CData.net_messagename, sizeof(CData.net_messagename)) == 0)
    {
      strncpy(CData.net_messagename, "little Raoul", sizeof(CData.net_messagename));   // default
    }


    /*********************************************

    DEBUG Section

    *********************************************/

    strcpy(lCurSectionName, "DEBUG");

    //Show the FPS counter?
    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "DISPLAY_FPS", &lTempBool) == 0)
    {
      lTempBool = btrue; // default
    }
    CData.fpson = lTempBool;

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "HIDE_MOUSE", &CData.HideMouse) == 0)
    {
      CData.HideMouse = btrue; // default
    }

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "GRAB_MOUSE", &lTempBool) == 0)
    {
      CData.GrabMouse = SDL_GRAB_ON; // default
    }
    CData.GrabMouse = lTempBool ? SDL_GRAB_ON : SDL_GRAB_OFF;

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "DEVELOPER_MODE", &CData.DevMode) == 0)
    {
      CData.DevMode = btrue; // default
    }
    CloseConfigFile(lConfigSetup);

  }
}
//--------------------------------------------------------------------------------------------
void log_madused(char *savename)
{
  // ZZ> This is a debug function for checking model loads
  FILE* hFileWrite;
  int cnt;

  hFileWrite = fopen(savename, "w");
  if (hFileWrite)
  {
    fprintf(hFileWrite, "Slot usage for objects in last module loaded...\n");
    fprintf(hFileWrite, "%d of %d frames used...\n", madloadframe, MAXFRAME);
    cnt = 0;
    while (cnt < MAXMODEL)
    {
      fprintf(hFileWrite, "%3d %32s %s\n", cnt, CapList[cnt].classname, MadList[cnt].name);
      cnt++;
    }
    fclose(hFileWrite);
  }
}

//---------------------------------------------------------------------------------------------
void make_lightdirectionlookup()
{
  // ZZ> This function builds the lighting direction table
  //     The table is used to find which direction the light is coming
  //     from, based on the four corner vertices of a mesh tile.
  Uint32 cnt;
  Uint16 tl, tr, br, bl;
  int x, y;

  for (cnt = 0; cnt < 65536; cnt++)
  {
    tl = (cnt & 0xf000) >> 12;
    tr = (cnt & 0x0f00) >> 8;
    br = (cnt & 0x00f0) >> 4;
    bl = (cnt & 0x000f);
    x = br + tr - bl - tl;
    y = br + bl - tl - tr;
    lightdirectionlookup[cnt] = (atan2(-y, x) + PI) * RAD_TO_BYTE;
  }
}

float sinlut[MAXLIGHTROTATION];
float coslut[MAXLIGHTROTATION];

//---------------------------------------------------------------------------------------------
float spek_global_lighting(int rotation, int normal, float lx, float ly, float lz)
{
  // ZZ> This function helps make_spektable
  float fTmp;
  float nx, ny, nz;
  float sinrot, cosrot;

  nx = md2normals[normal][0];
  ny = md2normals[normal][1];
  nz = md2normals[normal][2];
  sinrot = sinlut[rotation];
  cosrot = coslut[rotation];
  fTmp = cosrot * nx + sinrot * ny;
  ny = cosrot * ny - sinrot * nx;
  nx = fTmp;

  fTmp = 0;
  if (nx*lx > 0) fTmp += nx * lx;
  if (ny*ly > 0) fTmp += ny * ly;
  if (nz*lz > 0) fTmp += nz * lz;

  return fTmp;
}

//---------------------------------------------------------------------------------------------
float spek_local_lighting(int rotation, int normal)
{
  // ZZ> This function helps make_spektable
  float fTmp;
  float nx, ny, nz;
  float sinrot, cosrot;

  nx = md2normals[normal][0];
  ny = md2normals[normal][1];
  nz = md2normals[normal][2];
  sinrot = sinlut[rotation];
  cosrot = coslut[rotation];
  fTmp = cosrot * nx + sinrot * ny;

  fTmp = 0;
  if (cosrot*nx + sinrot*ny > 0) fTmp += cosrot * nx + sinrot * ny;

  return fTmp;
}


//---------------------------------------------------------------------------------------------
void make_spektable(float lx, float ly, float lz)
{
  // ZZ> This function makes a light table to fake directional lighting
  int cnt, tnc;
  float flight;

  // Build a lookup table for sin/cos
  for (cnt = 0; cnt < MAXLIGHTROTATION; cnt++)
  {
    sinlut[cnt] = sin(TWO_PI * cnt / MAXLIGHTROTATION);
    coslut[cnt] = cos(TWO_PI * cnt / MAXLIGHTROTATION);
  }

  flight = sqrt(lx * lx + ly * ly + lz * lz);
  for (cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++)  // Spikey mace
  {
    for (tnc = 0; tnc < MAXLIGHTROTATION; tnc++)
    {
      spek_global[tnc][cnt] = lightambi * flight * spek_global_lighting(tnc, cnt, lx / flight, ly / flight, lz / flight);
    }
  }

  for (cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++)  // Spikey mace
  {
    for (tnc = 0; tnc < MAXLIGHTROTATION; tnc++)
    {
      spek_local[tnc][cnt] = spek_local_lighting(tnc, cnt);
    }
  }

  // Fill in index number 162 for the spike mace
  for (tnc = 0; tnc < MAXLIGHTROTATION; tnc++)
  {
    spek_global[tnc][MD2LIGHTINDICES-1] = 0;
    spek_local[tnc][cnt]                = 0;
  }
}

//---------------------------------------------------------------------------------------------
void make_lighttospek(void)
{
  // ZZ> This function makes a light table to fake directional lighting
  int cnt, tnc;
//  Uint8 spek;
//  float fTmp, fPow;


  // New routine
  for (cnt = 0; cnt < MAXSPEKLEVEL; cnt++)
  {
    for (tnc = 0; tnc < 256; tnc++)
    {
      lighttospek[cnt][tnc] = 255 * pow(tnc / 255.0f, 1.0 + cnt / 2.0f);
      lighttospek[cnt][tnc] = MIN(255, lighttospek[cnt][tnc]);

      //fTmp = tnc/256.0;
      //fPow = (fTmp*4.0)+1;
      //fTmp = pow(fTmp, fPow);
      //fTmp = fTmp*cnt*255.0/MAXSPEKLEVEL;
      //if(fTmp<0) fTmp=0;
      //if(fTmp>255) fTmp=255;
      //spek = fTmp;
      //spek = spek>>1;
      //lighttospek[cnt][tnc] = spek;
    }
  }
}

//---------------------------------------------------------------------------------------------
int vertexconnected(int modelindex, int vertex)
{
  // ZZ> This function returns 1 if the model vertex is connected, 0 otherwise
  int cnt, tnc, entry;

  entry = 0;
  for (cnt = 0; cnt < MadList[modelindex].commands; cnt++)
  {
    for (tnc = 0; tnc < MadList[modelindex].commandsize[cnt]; tnc++)
    {
      if (MadList[modelindex].commandvrt[entry] == vertex)
      {
        // The vertex is used
        return 1;
      }
      entry++;
    }
  }

  // The vertex is not used
  return 0;
}

//---------------------------------------------------------------------------------------------
void get_madtransvertices(int modelindex)
{
  // ZZ> This function gets the number of vertices to transform for a model...
  //     That means every one except the grip ( unconnected ) vertices
  int cnt, trans = 0;

  for (cnt = 0; cnt < MadList[modelindex].vertices; cnt++)
    trans += vertexconnected(modelindex, cnt);

  MadList[modelindex].transvertices = trans;
}

//---------------------------------------------------------------------------------------------
int rip_md2_header(void)
{
  // ZZ> This function makes sure an md2 is really an md2
  int iTmp;
  int* ipIntPointer;

  // Check the file type
  ipIntPointer = (int*) cLoadBuffer;
  iTmp = ipIntPointer[0];

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  iTmp = SDL_Swap32(iTmp);
#endif

  if (iTmp != MD2START) return bfalse;

  return btrue;
}

//---------------------------------------------------------------------------------------------
void fix_md2_normals(Uint16 modelindex)
{
  // ZZ> This function helps light not flicker so much
  int cnt, tnc;
  Uint8 indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
  Uint8 indexofnextnextnextnext;
  Uint32 frame;

  frame = MadList[modelindex].framestart;
  cnt = 0;
  while (cnt < MadList[modelindex].vertices)
  {
    tnc = 0;
    while (tnc < MadList[modelindex].frames)
    {
      indexofcurrent = MadFrame[frame].vrta[cnt];
      indexofnext = MadFrame[frame+1].vrta[cnt];
      indexofnextnext = MadFrame[frame+2].vrta[cnt];
      indexofnextnextnext = MadFrame[frame+3].vrta[cnt];
      indexofnextnextnextnext = MadFrame[frame+4].vrta[cnt];
      if (indexofcurrent == indexofnextnext && indexofnext != indexofcurrent)
      {
        MadFrame[frame+1].vrta[cnt] = indexofcurrent;
      }
      if (indexofcurrent == indexofnextnextnext)
      {
        if (indexofnext != indexofcurrent)
        {
          MadFrame[frame+1].vrta[cnt] = indexofcurrent;
        }
        if (indexofnextnext != indexofcurrent)
        {
          MadFrame[frame+2].vrta[cnt] = indexofcurrent;
        }
      }
      if (indexofcurrent == indexofnextnextnextnext)
      {
        if (indexofnext != indexofcurrent)
        {
          MadFrame[frame+1].vrta[cnt] = indexofcurrent;
        }
        if (indexofnextnext != indexofcurrent)
        {
          MadFrame[frame+2].vrta[cnt] = indexofcurrent;
        }
        if (indexofnextnextnext != indexofcurrent)
        {
          MadFrame[frame+3].vrta[cnt] = indexofcurrent;
        }
      }
      tnc++;
    }
    cnt++;
  }
}

//---------------------------------------------------------------------------------------------
void rip_md2_commands(Uint16 modelindex)
{
  // ZZ> This function converts an md2's GL commands into our little command list thing
  int iTmp;
  float fTmpu, fTmpv;
  int iNumVertices;
  int tnc;

  char* cpCharPointer = (char*) cLoadBuffer;
  int* ipIntPointer = (int*) cLoadBuffer;
  float* fpFloatPointer = (float*) cLoadBuffer;

  // Number of GL commands in the MD2
  int iNumCommands = ipIntPointer[9];

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  iNumCommands = SDL_Swap32(iNumCommands);
#endif

  // Offset (in DWORDS) from the start of the file to the gl command list.
  int iCommandOffset = ipIntPointer[15] >> 2;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  iCommandOffset = SDL_Swap32(iCommandOffset);
#endif

  // Read in each command
  // iNumCommands isn't the number of commands, rather the number of dwords in
  // the command list...  Use iCommandCount to figure out how many we use
  int iCommandCount = 0;
  int entry = 0;

  int cnt = 0;
  while (cnt < iNumCommands)
  {
    iNumVertices = ipIntPointer[iCommandOffset];

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    iNumVertices = SDL_Swap32(iNumVertices);
#endif

    iCommandOffset++;
    cnt++;

    if (iNumVertices != 0)
    {
      if (iNumVertices < 0)
      {
        // Fans start with a negative
        iNumVertices = -iNumVertices;
        // PORT: MadList[modelindex].commandtype[iCommandCount] = (Uint8) D3DPT_TRIANGLEFAN;
        MadList[modelindex].commandtype[iCommandCount] = GL_TRIANGLE_FAN;
        MadList[modelindex].commandsize[iCommandCount] = (Uint8) iNumVertices;
      }
      else
      {
        // Strips start with a positive
        // PORT: MadList[modelindex].commandtype[iCommandCount] = (Uint8) D3DPT_TRIANGLESTRIP;
        MadList[modelindex].commandtype[iCommandCount] = GL_TRIANGLE_STRIP;
        MadList[modelindex].commandsize[iCommandCount] = (Uint8) iNumVertices;
      }

      // Read in vertices for each command
      tnc = 0;
      while (tnc < iNumVertices)
      {
        fTmpu = fpFloatPointer[iCommandOffset];  iCommandOffset++;  cnt++;
        fTmpv = fpFloatPointer[iCommandOffset];  iCommandOffset++;  cnt++;
        iTmp = ipIntPointer[iCommandOffset];  iCommandOffset++;  cnt++;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
        fTmpu = LoadFloatByteswapped(&fTmpu);
        fTmpv = LoadFloatByteswapped(&fTmpv);
        iTmp = SDL_Swap32(iTmp);
#endif
        MadList[modelindex].commandu[entry] = fTmpu - (.5 / 64); // GL doesn't align correctly
        MadList[modelindex].commandv[entry] = fTmpv - (.5 / 64); // with D3D
        MadList[modelindex].commandvrt[entry] = (Uint16) iTmp;
        entry++;
        tnc++;
      }
      iCommandCount++;
    }
  }
  MadList[modelindex].commands = iCommandCount;
}

//---------------------------------------------------------------------------------------------
int rip_md2_frame_name(int frame)
{
  // ZZ> This function gets frame names from the load buffer, it returns
  //     btrue if the name in cFrameName[] is valid
  int iFrameOffset;
  int iNumVertices;
  int iNumFrames;
  int cnt;
  int* ipNamePointer;
  int* ipIntPointer;
  int foundname;

  // Jump to the Frames section of the md2 data
  ipNamePointer = (int*) cFrameName;
  ipIntPointer = (int*) cLoadBuffer;


  iNumVertices = ipIntPointer[6];
  iNumFrames = ipIntPointer[10];
  iFrameOffset = ipIntPointer[14] >> 2;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  iNumVertices = SDL_Swap32(iNumVertices);
  iNumFrames = SDL_Swap32(iNumFrames);
  iFrameOffset = SDL_Swap32(iFrameOffset);
#endif


  // Chug through each frame
  foundname = bfalse;
  cnt = 0;
  while (cnt < iNumFrames && !foundname)
  {
    iFrameOffset += 6;
    if (cnt == frame)
    {
      ipNamePointer[0] = ipIntPointer[iFrameOffset]; iFrameOffset++;
      ipNamePointer[1] = ipIntPointer[iFrameOffset]; iFrameOffset++;
      ipNamePointer[2] = ipIntPointer[iFrameOffset]; iFrameOffset++;
      ipNamePointer[3] = ipIntPointer[iFrameOffset]; iFrameOffset++;
      foundname = btrue;
    }
    else
    {
      iFrameOffset += 4;
    }
    iFrameOffset += iNumVertices;
    cnt++;
  }
  cFrameName[15] = 0;  // Make sure it's null terminated
  return foundname;
}

//---------------------------------------------------------------------------------------------
void rip_md2_frames(Uint16 modelindex)
{
  // ZZ> This function gets frames from the load buffer and adds them to
  //     the indexed model
  Uint8 cTmpx, cTmpy, cTmpz;
  Uint8 cTmpNormalIndex;
  float fRealx, fRealy, fRealz;
  float fScalex, fScaley, fScalez;
  float fTranslatex, fTranslatey, fTranslatez;
  int iFrameOffset;
  int iNumVertices;
  int iNumFrames;
  int cnt, tnc;
  char* cpCharPointer;
  int* ipIntPointer;
  float* fpFloatPointer;


  // Jump to the Frames section of the md2 data
  cpCharPointer = (char*) cLoadBuffer;
  ipIntPointer = (int*) cLoadBuffer;
  fpFloatPointer = (float*) cLoadBuffer;


  iNumVertices = ipIntPointer[6];
  iNumFrames = ipIntPointer[10];
  iFrameOffset = ipIntPointer[14] >> 2;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
  iNumVertices = SDL_Swap32(iNumVertices);
  iNumFrames = SDL_Swap32(iNumFrames);
  iFrameOffset = SDL_Swap32(iFrameOffset);
#endif


  // Read in each frame
  MadList[modelindex].framestart = madloadframe;
  MadList[modelindex].frames = iNumFrames;
  MadList[modelindex].vertices = iNumVertices;
  MadList[modelindex].scale = (float)(1.0 / 320.0); // Scale each vertex float to fit it in a short
  cnt = 0;
  while (cnt < iNumFrames && madloadframe < MAXFRAME)
  {
    fScalex = fpFloatPointer[iFrameOffset]; iFrameOffset++;
    fScaley = fpFloatPointer[iFrameOffset]; iFrameOffset++;
    fScalez = fpFloatPointer[iFrameOffset]; iFrameOffset++;
    fTranslatex = fpFloatPointer[iFrameOffset]; iFrameOffset++;
    fTranslatey = fpFloatPointer[iFrameOffset]; iFrameOffset++;
    fTranslatez = fpFloatPointer[iFrameOffset]; iFrameOffset++;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    fScalex = LoadFloatByteswapped(&fScalex);
    fScaley = LoadFloatByteswapped(&fScaley);
    fScalez = LoadFloatByteswapped(&fScalez);

    fTranslatex = LoadFloatByteswapped(&fTranslatex);
    fTranslatey = LoadFloatByteswapped(&fTranslatey);
    fTranslatez = LoadFloatByteswapped(&fTranslatez);
#endif

    iFrameOffset += 4;
    tnc = 0;
    while (tnc < iNumVertices)
    {
      // This should work because it's reading a single character
      cTmpx = cpCharPointer[(iFrameOffset<<2)];
      cTmpy = cpCharPointer[(iFrameOffset<<2)+1];
      cTmpz = cpCharPointer[(iFrameOffset<<2)+2];
      cTmpNormalIndex = cpCharPointer[(iFrameOffset<<2)+3];
      fRealx = (cTmpx * fScalex) + fTranslatex;
      fRealy = (cTmpy * fScaley) + fTranslatey;
      fRealz = (cTmpz * fScalez) + fTranslatez;
      //            fRealx = (cTmpx*fScalex);
      //            fRealy = (cTmpy*fScaley);
      //            fRealz = (cTmpz*fScalez);
      //            MadFrame[madloadframe].vrtx[tnc] = (Sint16) (fRealx*256); // HUK
      MadFrame[madloadframe].vrtx[tnc] = (Sint16)(-fRealx * 256);
      MadFrame[madloadframe].vrty[tnc] = (Sint16)(fRealy * 256);
      MadFrame[madloadframe].vrtz[tnc] = (Sint16)(fRealz * 256);
      MadFrame[madloadframe].vrta[tnc] = cTmpNormalIndex;
      iFrameOffset++;
      tnc++;
    }
    madloadframe++;
    cnt++;
  }
}

//---------------------------------------------------------------------------------------------
int load_one_md2(char* szLoadname, Uint16 modelindex)
{
  // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
  //    int iFileHandleRead;
  size_t iBytesRead = 0;
  int iReturnValue;

  // Read the input file
  FILE *file = fopen(szLoadname, "rb");
  if (!file)
    return bfalse;

  // Read up to MD2MAXLOADSIZE bytes from the file into the cLoadBuffer array.
  iBytesRead = fread(cLoadBuffer, 1, MD2MAXLOADSIZE, file);
  if (iBytesRead == 0)
    return bfalse;

  // Check the header
  // TODO: Verify that the header's filesize correspond to iBytesRead.
  iReturnValue = rip_md2_header();
  if (iReturnValue == bfalse)
    return bfalse;

  // Get the frame vertices
  rip_md2_frames(modelindex);
  // Get the commands
  rip_md2_commands(modelindex);
  // Fix them normals
  //fix_md2_normals(modelindex);
  // Figure out how many vertices to transform
  get_madtransvertices(modelindex);

  fclose(file);

  return btrue;
}

//--------------------------------------------------------------------------------------------
void make_enviro(void)
{
  // ZZ> This function sets up the environment mapping table
  int cnt;
  float z;
  float x, y;

  // Find the environment map positions
  for (cnt = 0; cnt < MD2LIGHTINDICES; cnt++)
  {
    x = md2normals[cnt][0];
    y = md2normals[cnt][1];
    x = 1.0f + atan2(y, x)/PI;
    x--;

    if (x < 0)
      x--;

    indextoenvirox[cnt] = x;
  }

  for (cnt = 0; cnt < 256; cnt++)
  {
    z = cnt / 256.0;  // Z is between 0 and 1
    lighttoenviroy[cnt] = z;
  }
}

//--------------------------------------------------------------------------------------------
void show_stat(GAME_STATE * gs, Uint16 statindex)
{
  // ZZ> This function shows the more specific stats for a character
  int character, level;
  char gender[8];

  if (gs->cs->statdelay == 0)
  {
    if (statindex < numstat)
    {
      character = statlist[statindex];


      // Name
      debug_message("=%s=", ChrList[character].name);

      // Level and gender and class
      gender[0] = 0;
      if (ChrList[character].alive)
      {
        if (ChrList[character].gender == GENMALE)
        {
          snprintf(gender, sizeof(gender), "male ");
        }
        if (ChrList[character].gender == GENFEMALE)
        {
          snprintf(gender, sizeof(gender), "female ");
        }

        level = ChrList[character].experiencelevel;
        if (level == 0)
          debug_message(" 1st level %s%s", gender, CapList[ChrList[character].model].classname);
        if (level == 1)
          debug_message(" 2nd level %s%s", gender, CapList[ChrList[character].model].classname);
        if (level == 2)
          debug_message(" 3rd level %s%s", gender, CapList[ChrList[character].model].classname);
        if (level >  2)
          debug_message(" %dth level %s%s", level + 1, gender, CapList[ChrList[character].model].classname);
      }
      else
      {
        debug_message(" Dead %s", CapList[ChrList[character].model].classname);
      }

      // Stats
      debug_message(" STR:%2d ~WIS:%2d ~DEF:%d", ChrList[character].strength >> 8, ChrList[character].wisdom >> 8, 255 - ChrList[character].defense);
      debug_message(" INT:%2d ~DEX:%2d ~EXP:%d", ChrList[character].intelligence >> 8, ChrList[character].dexterity >> 8, ChrList[character].experience);

      gs->cs->statdelay = 10;
    }
  }
}


//--------------------------------------------------------------------------------------------
void check_stats(GAME_STATE * gs)
{
  // ZZ> This function lets the players check character stats
  if (gs->modstate.net_messagemode == bfalse)
  {
    if (SDLKEYDOWN(SDLK_1))  show_stat(gs, 0);
    if (SDLKEYDOWN(SDLK_2))  show_stat(gs, 1);
    if (SDLKEYDOWN(SDLK_3))  show_stat(gs, 2);
    if (SDLKEYDOWN(SDLK_4))  show_stat(gs, 3);
    if (SDLKEYDOWN(SDLK_5))  show_stat(gs, 4);
    if (SDLKEYDOWN(SDLK_6))  show_stat(gs, 5);
    if (SDLKEYDOWN(SDLK_7))  show_stat(gs, 6);
    if (SDLKEYDOWN(SDLK_8))  show_stat(gs, 7);

    // !!!BAD!!!  CHEAT
    if (SDLKEYDOWN(SDLK_x) && gs->cd->DevMode)
    {
      if (SDLKEYDOWN(SDLK_1) && PlaList[0].index < MAXCHR)  give_experience(PlaList[0].index, 25, XPDIRECT);
      if (SDLKEYDOWN(SDLK_2) && PlaList[1].index < MAXCHR)  give_experience(PlaList[1].index, 25, XPDIRECT);
      gs->cs->statdelay = 0;
    }
  }
}

//--------------------------------------------------------------------------------------------
void check_screenshot()
{
  //This function checks if we want to take a screenshot
  if (SDLKEYDOWN(SDLK_F11))
  {
    if (!dump_screenshot())                 //Take the shot, returns bfalse if failed
    {
      debug_message("Error writing screenshot");
      log_warning("Error writing screenshot\n");      //Log the error in log.txt
    }
  }
}

bool_t dump_screenshot()
{
  // dumps the current screen (GL context) to a new bitmap file
  // right now it dumps it to whatever the current directory is

  // returns btrue if successful, bfalse otherwise

  SDL_Surface *screen, *temp;
  Uint8 *pixels;
  STRING buff;
  int i;
  FILE *test;

  screen = SDL_GetVideoSurface();
  temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                              0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
                              0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
                             );


  if (temp == NULL)
    return bfalse;

  pixels = malloc(3 * screen->w * screen->h);
  if (pixels == NULL)
  {
    SDL_FreeSurface(temp);
    return bfalse;
  }

  glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

  for (i = 0; i < screen->h; i++)
    memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3 * screen->w * (screen->h - i - 1), screen->w * 3);
  free(pixels);

  // find the next EGO??.BMP file for writing
  i = 0;
  test = NULL;

  do
  {
    if (test != NULL)
      fclose(test);

    snprintf(buff, sizeof(buff), "ego%02d.bmp", i);

    // lame way of checking if the file already exists...
    test = fopen(buff, "rb");
    i++;

  }
  while ((test != NULL) && (i < 100));

  SDL_SaveBMP(temp, buff);
  SDL_FreeSurface(temp);

  debug_message("Saved to %s", buff);

  return btrue;
}

//--------------------------------------------------------------------------------------------
void add_stat(Uint16 character)
{
  // ZZ> This function adds a status display to the do list
  if (numstat < MAXSTAT)
  {
    statlist[numstat] = character;
    ChrList[character].staton = btrue;
    numstat++;
  }
}

//--------------------------------------------------------------------------------------------
void move_to_top(Uint16 character)
{
  // ZZ> This function puts the character on top of the statlist
  int cnt, oldloc;


  // Find where it is
  oldloc = numstat;

  for (cnt = 0; cnt < numstat; cnt++)
    if (statlist[cnt] == character)
    {
      oldloc = cnt;
      cnt = numstat;
    }

  // Change position
  if (oldloc < numstat)
  {
    // Move all the lower ones up
    while (oldloc > 0)
    {
      oldloc--;
      statlist[oldloc+1] = statlist[oldloc];
    }
    // Put the character in the top slot
    statlist[0] = character;
  }
}

//--------------------------------------------------------------------------------------------
void sort_stat()
{
  // ZZ> This function puts all of the local players on top of the statlist
  int cnt;

  for (cnt = 0; cnt < numpla; cnt++)
    if (PlaList[cnt].valid && PlaList[cnt].device != INPUTNONE)
    {
      move_to_top(PlaList[cnt].index);
    }
}

//--------------------------------------------------------------------------------------------
void move_water(void)
{
  // ZZ> This function animates the water overlays
  int layer;

  for (layer = 0; layer < MAXWATERLAYER; layer++)
  {
    waterlayeru[layer] += waterlayeruadd[layer];
    waterlayerv[layer] += waterlayervadd[layer];
    if (waterlayeru[layer] > 1.0)  waterlayeru[layer] -= 1.0;
    if (waterlayerv[layer] > 1.0)  waterlayerv[layer] -= 1.0;
    if (waterlayeru[layer] < -1.0)  waterlayeru[layer] += 1.0;
    if (waterlayerv[layer] < -1.0)  waterlayerv[layer] += 1.0;
    waterlayerframe[layer] = (waterlayerframe[layer] + waterlayerframeadd[layer]) & WATERFRAMEAND;
  }
}

//--------------------------------------------------------------------------------------------
void play_action(Uint16 character, Uint16 action, Uint8 actionready)
{
  // ZZ> This function starts a generic action for a character
  if (MadList[ChrList[character].model].actionvalid[action])
  {
    ChrList[character].nextaction = ACTIONDA;
    ChrList[character].action = action;
    ChrList[character].lip = 0;
    ChrList[character].lastframe = ChrList[character].frame;
    ChrList[character].frame = MadList[ChrList[character].model].actionstart[ChrList[character].action];
    ChrList[character].actionready = actionready;
  }
}

//--------------------------------------------------------------------------------------------
void set_frame(Uint16 character, Uint16 frame, Uint8 lip)
{
  // ZZ> This function sets the frame for a character explicitly...  This is used to
  //     rotate Tank turrets
  ChrList[character].nextaction = ACTIONDA;
  ChrList[character].action = ACTIONDA;
  ChrList[character].lip = (lip << 6);
  ChrList[character].lastframe = MadList[ChrList[character].model].actionstart[ACTIONDA] + frame;
  ChrList[character].frame = MadList[ChrList[character].model].actionstart[ACTIONDA] + frame + 1;
  ChrList[character].actionready = btrue;
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha(GAME_STATE * gs, Uint16 character, Uint32 * rand_idx)
{
  // ZZ> This function fixes an item's transparency
  Uint16 enchant, mount;
  Uint32 chr_randie = *rand_idx;

  RANDIE(*rand_idx);

  if (character != MAXCHR)
  {
    mount = ChrList[character].attachedto;
    if (ChrList[character].on && mount != MAXCHR && ChrList[character].isitem && ChrList[mount].transferblend)
    {
      // Okay, reset transparency
      enchant = ChrList[character].firstenchant;
      while (enchant < MAXENCHANT)
      {
        unset_enchant_value(gs, enchant, SETALPHABLEND, &chr_randie);
        unset_enchant_value(gs, enchant, SETLIGHTBLEND, &chr_randie);
        enchant = EncList[enchant].nextenchant;
      }
      ChrList[character].alpha = CapList[ChrList[character].model].alpha;
      ChrList[character].light = CapList[ChrList[character].model].light;
      enchant = ChrList[character].firstenchant;
      while (enchant < MAXENCHANT)
      {
        set_enchant_value(gs, enchant, SETALPHABLEND, EncList[enchant].eve, &chr_randie);
        set_enchant_value(gs, enchant, SETLIGHTBLEND, EncList[enchant].eve, &chr_randie);
        enchant = EncList[enchant].nextenchant;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
int generate_number(int numbase, int numrand)
{
  int tmp;

  // ZZ> This function generates a random number
  if (numrand <= 0)
  {
    log_error("One of the data pairs in randomization is wrong! (%i, %i)\n", numbase, numrand);
  }
  tmp = (ego_rand(&ego_rand_seed) % numrand) + numbase;
  return (tmp);
}

//--------------------------------------------------------------------------------------------
void drop_money(Uint16 character, Uint16 money, Uint32 * rand_idx)
{
  // ZZ> This function drops some of a character's money
  Uint16 huns, tfives, fives, ones, cnt;
  Uint32 tmp_randie = *rand_idx;
  

  if (money > ChrList[character].money)  money = ChrList[character].money;
  if (money > 0 && ChrList[character].zpos > -2)
  {
    ChrList[character].money = ChrList[character].money - money;
    huns = money / 100;  money -= (huns << 7) - (huns << 5) + (huns << 2);
    tfives = money / 25;  money -= (tfives << 5) - (tfives << 3) + tfives;
    fives = money / 5;  money -= (fives << 2) + fives;
    ones = money;

    for (cnt = 0; cnt < ones; cnt++)
      spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos,  ChrList[character].zpos, 0, MAXMODEL, COIN1, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR, &tmp_randie);

    for (cnt = 0; cnt < fives; cnt++)
      spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos,  ChrList[character].zpos, 0, MAXMODEL, COIN5, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR, &tmp_randie);

    for (cnt = 0; cnt < tfives; cnt++)
      spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos,  ChrList[character].zpos, 0, MAXMODEL, COIN25, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR, &tmp_randie);

    for (cnt = 0; cnt < huns; cnt++)
      spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos,  ChrList[character].zpos, 0, MAXMODEL, COIN100, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, cnt, MAXCHR, &tmp_randie);

    ChrList[character].damagetime = DAMAGETIME;  // So it doesn't grab it again
  }
}

//--------------------------------------------------------------------------------------------
void call_for_help(Uint16 character)
{
  // ZZ> This function issues a call for help to all allies
  Uint8 team;
  Uint16 cnt;

  team = ChrList[character].team;
  TeamList[team].sissy = character;

  for (cnt = 0; cnt < MAXCHR; cnt++)
    if (ChrList[cnt].on && cnt != character && TeamList[ChrList[cnt].team].hatesteam[team] == bfalse)
      ChrList[cnt].alert = ChrList[cnt].alert | ALERTIFCALLEDFORHELP;
}

//--------------------------------------------------------------------------------------------
void give_experience(int character, int amount, Uint8 xptype)
{
  // ZZ> This function gives a character experience, and pawns off level gains to
  //     another function
  int newamount;
  int curlevel, nextexperience;
  int number;
  int profile;


  if (ChrList[character].invictus) return;


  // Figure out how much experience to give
  profile = ChrList[character].model;
  newamount = amount;
  if (xptype < MAXEXPERIENCETYPE)
  {
    newamount = amount * CapList[profile].experiencerate[xptype];
  }
  newamount += ChrList[character].experience;
  if (newamount > MAXXP)  newamount = MAXXP;
  ChrList[character].experience = newamount;


  // Do level ups and stat changes
  curlevel       = ChrList[character].experiencelevel;
  nextexperience = (curlevel<MAXLEVEL-1) ? CapList[profile].experienceforlevel[curlevel+1] : CapList[profile].experiencecoeff*pow(curlevel+1,CapList[profile].experiencepower);
  if (ChrList[character].experience >= nextexperience)
  {
    // The character is ready to advance...
    if (ChrList[character].isplayer)
    {
      debug_message("%s gained a level!!!", ChrList[character].name);

      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.lvlup_sound);
      sound = Mix_LoadWAV(CStringTmp1);

      Mix_PlayChannel(-1, sound, 0);
    }
    ChrList[character].experiencelevel++;

    // Size
    if((ChrList[character].sizegoto + CapList[profile].sizeperlevel) < 1+(CapList[profile].sizeperlevel*10)) ChrList[character].sizegoto += CapList[profile].sizeperlevel;
	ChrList[character].sizegototime = SIZETIME*100;

    // Strength
    number = generate_number(CapList[profile].strengthperlevelbase, CapList[profile].strengthperlevelrand);
    number = number + ChrList[character].strength;
    if (number > PERFECTSTAT) number = PERFECTSTAT;
    ChrList[character].strength = number;

    // Wisdom
    number = generate_number(CapList[profile].wisdomperlevelbase, CapList[profile].wisdomperlevelrand);
    number = number + ChrList[character].wisdom;
    if (number > PERFECTSTAT) number = PERFECTSTAT;
    ChrList[character].wisdom = number;

    // Intelligence
    number = generate_number(CapList[profile].intelligenceperlevelbase, CapList[profile].intelligenceperlevelrand);
    number = number + ChrList[character].intelligence;
    if (number > PERFECTSTAT) number = PERFECTSTAT;
    ChrList[character].intelligence = number;

    // Dexterity
    number = generate_number(CapList[profile].dexterityperlevelbase, CapList[profile].dexterityperlevelrand);
    number = number + ChrList[character].dexterity;
    if (number > PERFECTSTAT) number = PERFECTSTAT;
    ChrList[character].dexterity = number;

    // Life
    number = generate_number(CapList[profile].lifeperlevelbase, CapList[profile].lifeperlevelrand);
    number = number + ChrList[character].lifemax;
    if (number > PERFECTBIG) number = PERFECTBIG;
    ChrList[character].life += (number - ChrList[character].lifemax);
    ChrList[character].lifemax = number;

    // Mana
    number = generate_number(CapList[profile].manaperlevelbase, CapList[profile].manaperlevelrand);
    number = number + ChrList[character].manamax;
    if (number > PERFECTBIG) number = PERFECTBIG;
    ChrList[character].mana += (number - ChrList[character].manamax);
    ChrList[character].manamax = number;

    // Mana Return
    number = generate_number(CapList[profile].manareturnperlevelbase, CapList[profile].manareturnperlevelrand);
    number = number + ChrList[character].manareturn;
    if (number > PERFECTSTAT) number = PERFECTSTAT;
    ChrList[character].manareturn = number;

    // Mana Flow
    number = generate_number(CapList[profile].manaflowperlevelbase, CapList[profile].manaflowperlevelrand);
    number = number + ChrList[character].manaflow;
    if (number > PERFECTSTAT) number = PERFECTSTAT;
    ChrList[character].manaflow = number;
  }
}


//--------------------------------------------------------------------------------------------
void give_team_experience(Uint8 team, int amount, Uint8 xptype)
{
  // ZZ> This function gives a character experience, and pawns off level gains to
  //     another function
  int cnt;

  for (cnt = 0; cnt < MAXCHR; cnt++)
    if (ChrList[cnt].team == team && ChrList[cnt].on)
      give_experience(cnt, amount, xptype);
}


//--------------------------------------------------------------------------------------------
void setup_alliances(char *modname)
{
  // ZZ> This function reads the alliance file
  STRING newloadname, szTemp;
  Uint8 teama, teamb;
  FILE *fileread;


  // Load the file
  snprintf(newloadname, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.alliance_file);
  fileread = fopen(newloadname, "r");
  if (fileread)
  {
    while (goto_colon_yesno(fileread))
    {
      fscanf(fileread, "%s", szTemp);
      teama = (szTemp[0] - 'A') % MAXTEAM;
      fscanf(fileread, "%s", szTemp);
      teamb = (szTemp[0] - 'A') % MAXTEAM;
      TeamList[teama].hatesteam[teamb] = bfalse;
    }
    fclose(fileread);
  }
}

//grfx.c
//--------------------------------------------------------------------------------------------
void load_mesh_fans()
{
  // ZZ> This function loads fan types for the terrain
  int cnt, entry;
  int numfantype, fantype, bigfantype, vertices;
  int numcommand, command, commandsize;
  int itmp;
  float ftmp;
  FILE* fileread;
  float offx, offy;


  // Initialize all mesh types to 0
  entry = 0;
  while (entry < MAXMESHTYPE)
  {
    Mesh.tilelist[entry].commandnumvertices = 0;
    Mesh.tilelist[entry].commands = 0;
    entry++;
  }


  // Open the file and go to it
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.fans_file);
  fileread = fopen(CStringTmp1, "r");
  if (fileread)
  {
    goto_colon(fileread);
    fscanf(fileread, "%d", &numfantype);
    fantype = 0;
    bigfantype = MAXMESHTYPE / 2; // Duplicate for 64x64 tiles
    while (fantype < numfantype)
    {
      goto_colon(fileread);
      fscanf(fileread, "%d", &vertices);
      Mesh.tilelist[fantype].commandnumvertices = vertices;
      Mesh.tilelist[bigfantype].commandnumvertices = vertices;  // Dupe
      cnt = 0;
      while (cnt < vertices)
      {
        goto_colon(fileread);
        fscanf(fileread, "%d", &itmp);
        goto_colon(fileread);
        fscanf(fileread, "%f", &ftmp);
        Mesh.tilelist[fantype].commandu[cnt] = ftmp;
        Mesh.tilelist[bigfantype].commandu[cnt] = ftmp;  // Dupe
        goto_colon(fileread);
        fscanf(fileread, "%f", &ftmp);
        Mesh.tilelist[fantype].commandv[cnt] = ftmp;
        Mesh.tilelist[bigfantype].commandv[cnt] = ftmp;  // Dupe
        cnt++;
      }


      goto_colon(fileread);
      fscanf(fileread, "%d", &numcommand);
      Mesh.tilelist[fantype].commands = numcommand;
      Mesh.tilelist[bigfantype].commands = numcommand;  // Dupe
      entry = 0;
      command = 0;
      while (command < numcommand)
      {
        goto_colon(fileread);
        fscanf(fileread, "%d", &commandsize);
        Mesh.tilelist[fantype].commandsize[command] = commandsize;
        Mesh.tilelist[bigfantype].commandsize[command] = commandsize;  // Dupe
        cnt = 0;
        while (cnt < commandsize)
        {
          goto_colon(fileread);
          fscanf(fileread, "%d", &itmp);
          Mesh.tilelist[fantype].commandvrt[entry] = itmp;
          Mesh.tilelist[bigfantype].commandvrt[entry] = itmp;  // Dupe
          entry++;
          cnt++;
        }
        command++;
      }
      fantype++;
      bigfantype++;  // Dupe
    }
    fclose(fileread);
  }


  // Correct all of them silly texture positions for seamless tiling
  entry = 0;
  while (entry < MAXMESHTYPE / 2)
  {
    cnt = 0;
    while (cnt < Mesh.tilelist[entry].commandnumvertices)
    {
      //            Mesh.tilelist[entry].commandu[cnt] = ((.5/32)+(Mesh.tilelist[entry].commandu[cnt]*31/32))/8;
      //            Mesh.tilelist[entry].commandv[cnt] = ((.5/32)+(Mesh.tilelist[entry].commandv[cnt]*31/32))/8;
      Mesh.tilelist[entry].commandu[cnt] = ((.6 / 32) + (Mesh.tilelist[entry].commandu[cnt] * 30.8 / 32)) / 8;
      Mesh.tilelist[entry].commandv[cnt] = ((.6 / 32) + (Mesh.tilelist[entry].commandv[cnt] * 30.8 / 32)) / 8;
      cnt++;
    }
    entry++;
  }
  // Do for big tiles too
  while (entry < MAXMESHTYPE)
  {
    cnt = 0;
    while (cnt < Mesh.tilelist[entry].commandnumvertices)
    {
      //            Mesh.tilelist[entry].commandu[cnt] = ((.5/64)+(Mesh.tilelist[entry].commandu[cnt]*63/64))/4;
      //            Mesh.tilelist[entry].commandv[cnt] = ((.5/64)+(Mesh.tilelist[entry].commandv[cnt]*63/64))/4;
      Mesh.tilelist[entry].commandu[cnt] = ((.6 / 64) + (Mesh.tilelist[entry].commandu[cnt] * 62.8 / 64)) / 4;
      Mesh.tilelist[entry].commandv[cnt] = ((.6 / 64) + (Mesh.tilelist[entry].commandv[cnt] * 62.8 / 64)) / 4;
      cnt++;
    }
    entry++;
  }


  // Make tile texture offsets
  entry = 0;
  while (entry < MAXTILETYPE)
  {
    offx = (entry & 7) / 8.0;
    offy = (entry >> 3) / 8.0;
    Mesh.tilelist[entry].tileoffu = offx;
    Mesh.tilelist[entry].tileoffv = offy;
    entry++;
  }
}

//--------------------------------------------------------------------------------------------
void make_fanstart()
{
  // ZZ> This function builds a look up table to ease calculating the
  //     fan number given an x,y pair
  int cnt;


  cnt = 0;
  while (cnt < Mesh.sizey)
  {
    Mesh.fanstart[cnt] = Mesh.sizex * cnt;
    cnt++;
  }
  cnt = 0;
  while (cnt < (Mesh.sizey >> 2))
  {
    Mesh.blockstart[cnt] = (Mesh.sizex >> 2) * cnt;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_twist()
{
  // ZZ> This function precomputes surface normals and steep hill acceleration for
  //     the mesh
  int cnt;
  int x, y;
  float xslide, yslide;

  cnt = 0;
  while (cnt < 256)
  {
    y = cnt >> 4;
    x = cnt & 15;
    y = y - 7;  // -7 to 8
    x = x - 7;  // -7 to 8
    mapudtwist[cnt] = 32768 + y * SLOPE;
    maplrtwist[cnt] = 32768 + x * SLOPE;
    if (ABS(y) >= 7) y = y << 1;
    if (ABS(x) >= 7) x = x << 1;
    xslide = x * SLIDE;
    yslide = y * SLIDE;
    if (xslide < 0)
    {
      xslide += SLIDEFIX;
      if (xslide > 0)
        xslide = 0;
    }
    else
    {
      xslide -= SLIDEFIX;
      if (xslide < 0)
        xslide = 0;
    }
    if (yslide < 0)
    {
      yslide += SLIDEFIX;
      if (yslide > 0)
        yslide = 0;
    }
    else
    {
      yslide -= SLIDEFIX;
      if (yslide < 0)
        yslide = 0;
    }
    veludtwist[cnt] = -yslide * hillslide;
    vellrtwist[cnt] = xslide * hillslide;
    flattwist[cnt] = bfalse;
    if (ABS(veludtwist[cnt]) + ABS(vellrtwist[cnt]) < SLIDEFIX*4)
    {
      flattwist[cnt] = btrue;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
int load_mesh(char *modname)
{
  // ZZ> This function loads the level.mpd file
  FILE* fileread;
  STRING newloadname;
  int itmp, cnt;
  float ftmp;
  int fan;
  int numvert, numfan;
  int x, y, vert;

  snprintf(newloadname, sizeof(newloadname), "%s%s/%s", modname, CData.gamedat_dir, CData.mesh_file);
  fileread = fopen(newloadname, "rb");
  if (!fileread)
  {
    log_error("Could not read mesh file (%s)\n", newloadname);
  }
  else
  {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    fread(&itmp, 4, 1, fileread);  if (itmp != MAPID) return bfalse;
    fread(&itmp, 4, 1, fileread);  numvert = itmp;
    fread(&itmp, 4, 1, fileread);  Mesh.sizex = itmp;
    fread(&itmp, 4, 1, fileread);  Mesh.sizey = itmp;
#else
    fread(&itmp, 4, 1, fileread);  if ((int)SDL_Swap32(itmp) != MAPID) return bfalse;
    fread(&itmp, 4, 1, fileread);  numvert = (int)SDL_Swap32(itmp);
    fread(&itmp, 4, 1, fileread);  Mesh.sizex = (int)SDL_Swap32(itmp);
    fread(&itmp, 4, 1, fileread);  Mesh.sizey = (int)SDL_Swap32(itmp);
#endif

    numfan = Mesh.sizex * Mesh.sizey;
    Mesh.edgex = Mesh.sizex * 128;
    Mesh.edgey = Mesh.sizey * 128;
    numfanblock = ((Mesh.sizex >> 2)) * ((Mesh.sizey >> 2));  // MESHSIZEX MUST BE MULTIPLE OF 4
    watershift = 3;
    if (Mesh.sizex > 16)  watershift++;
    if (Mesh.sizex > 32)  watershift++;
    if (Mesh.sizex > 64)  watershift++;
    if (Mesh.sizex > 128)  watershift++;
    if (Mesh.sizex > 256)  watershift++;


    // Load fan data
    fan = 0;
    while (fan < numfan)
    {
      fread(&itmp, 4, 1, fileread);

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
      itmp = SDL_Swap32(itmp);
#endif

      Mesh.fanlist[fan].type = itmp >> 24;
      Mesh.fanlist[fan].fx = itmp >> 16;
      Mesh.fanlist[fan].tile = itmp;

      fan++;
    }
    // Load fan data
    fan = 0;
    while (fan < numfan)
    {
      fread(&itmp, 1, 1, fileread);
      Mesh.fanlist[fan].twist = itmp;
      fan++;
    }


    // Load vertex x data
    cnt = 0;
    while (cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread);

      Mesh.vrtx[cnt] = ftmp;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
      Mesh.vrtx[cnt] = LoadFloatByteswapped(&Mesh.vrtx[cnt]);
#endif
      cnt++;
    }
    // Load vertex y data
    cnt = 0;
    while (cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread);

      Mesh.vrty[cnt] = ftmp;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
      Mesh.vrty[cnt] = LoadFloatByteswapped(&Mesh.vrty[cnt]);
#endif

      cnt++;
    }
    // Load vertex z data
    cnt = 0;
    while (cnt < numvert)
    {
      fread(&ftmp, 4, 1, fileread);

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
      Mesh.vrtz[cnt] = ftmp / 16.0;  // Cartman uses 4 bit fixed point for Z
#else
      Mesh.vrtz[cnt] = (LoadFloatByteswapped(&ftmp)) / 16.0;  // Cartman uses 4 bit fixed point for Z
#endif

      cnt++;
    }

    //if(CData.shading==GL_FLAT && ->modstate.rts_control==bfalse)
    //{
    //  // Assume fullbright
    //  cnt = 0;
    //  while(cnt < numvert)
    //  {
    //    Mesh.vrta[cnt] = 255;
    //    Mesh.vrtl[cnt] = 255;
    //    cnt++;
    //  }
    //}
    //else
    {
      // Load vertex a data
      cnt = 0;
      while (cnt < numvert)
      {
        fread(&itmp, 1, 1, fileread);

        Mesh.vrta[cnt] = itmp;
        Mesh.vrtl[cnt] = 0;

        cnt++;
      }
    }
    fclose(fileread);

    make_fanstart();


    vert = 0;
    y = 0;
    while (y < Mesh.sizey)
    {
      x = 0;
      while (x < Mesh.sizex)
      {
        fan = Mesh.fanstart[y] + x;
        Mesh.fanlist[fan].vrtstart = vert;
        vert += Mesh.tilelist[Mesh.fanlist[fan].type].commandnumvertices;
        x++;
      }
      y++;
    }

    return btrue;
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void check_respawn(GAME_STATE * gs, Uint32 * rand_idx)
{
  int cnt;
  Uint32 loc_randie = *rand_idx;

  RANDIE(*rand_idx);

  for(cnt=0; cnt<MAXCHR; cnt++)
  {
    // Let players respawn
    if (gs->modstate.respawnvalid && !ChrList[cnt].alive && (0 != ChrList[cnt].latchbutton & LATCHBUTTONRESPAWN))
    {
      TeamList[ChrList[cnt].team].leader = cnt;
      ChrList[cnt].alert |= ALERTIFCLEANEDUP;

      // Cost some experience for doing this...
      ChrList[cnt].experience = ChrList[cnt].experience * EXPKEEP;

      respawn_character(cnt, &loc_randie);
    }
    ChrList[cnt].latchbutton &= ~LATCHBUTTONRESPAWN;
  }

};

//--------------------------------------------------------------------------------------------
void cl_update_game(GAME_STATE * gs, float dFrame, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.
  int    cnt, numdead;
  Uint32 cl_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);

  // client stuff
  {
    // Check for all local players being dead
    gs->cs->allpladead = bfalse;
    gs->cs->seeinvisible = bfalse;
    gs->cs->seekurse = bfalse;
    numdead = 0;
    for (cnt = 0; cnt < MAXPLAYER; cnt++)
    {
      if(!PlaList[cnt].valid || INPUTNONE==PlaList[cnt].device) continue;

      if (!ChrList[PlaList[cnt].index].alive && ChrList[PlaList[cnt].index].islocalplayer)
      {
        numdead++;
      };

      if (ChrList[PlaList[cnt].index].canseeinvisible)
      {
        gs->cs->seeinvisible = btrue;
      }

      if (ChrList[PlaList[cnt].index].canseekurse)
      {
        gs->cs->seekurse = btrue;
      }
    }

    if (numdead >= numlocalpla)
    {
      gs->cs->allpladead = btrue;
    }
  }

  // This is the main game loop
  gs->cs->msg_timechange = 0;

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync
  while ( (wldclock<allclock) && (gs->cs->numplatimes > 0))
  {
    srand(randsave);                          // client/server function
    resize_characters();                      // client function
    keep_weapons_with_holders();              // client-ish function

    // unbuffer the updated latches after let_ai_think() and before move_characters()
    cl_unbufferLatches(gs->cs);              // client function

    check_respawn(gs, &cl_randie);                         // client function

    move_characters(gs, &cl_randie);                       // client/server function
    move_particles(&cl_randie);                        // client/server function
    make_character_matrices();               // client function
    attach_particles();                      // client/server function
    make_onwhichfan(gs, &cl_randie);                       // client/server function
    bump_characters(gs);                       // client/server function
    stat_return(gs, &cl_randie);                           // client/server function
    pit_kill(gs, &cl_randie);                              // client/server function

    {
      // Generate the new seed
      randsave += *((Uint32*) & md2normals[wldframe&127][0]);
      randsave += *((Uint32*) & md2normals[randsave&127][1]);
    }

    // Stuff for which sync doesn't matter
    flash_select();                          // client function
    animate_tiles();                         // client function
    move_water();                            // client function

    // Timers
    wldclock += FRAMESKIP;
    wldframe++;

    gs->cs->msg_timechange++;
    if (gs->cs->statdelay > 0)  gs->cs->statdelay--;
    gs->cs->statclock++;
  }

  if (gs->ns->networkon)
  {
    if (gs->cs->numplatimes == 0)
    {
      // The remote ran out of messages, and is now twiddling its thumbs...
      // Make it go slower so it doesn't happen again
      wldclock += FRAMESKIP/4.0f;
    }
    else if (gs->cs->amClient && gs->cs->numplatimes > 3)
    {
      // The host has too many messages, and is probably experiencing control
      // gs->cd->lag...  Speed it up so it gets closer to sync
      wldclock -= FRAMESKIP/4.0f;
    }
  }
}


//--------------------------------------------------------------------------------------------
void sv_update_game(GAME_STATE * gs, float dFrame, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.
  int    cnt, numdead;
  Uint32 sv_randie = *rand_idx;

  RANDIE(*rand_idx);

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync
  while ( (wldclock<allclock) && (gs->cs->numplatimes > 0))
  {
    srand(randsave);                          // client/server function
    keep_weapons_with_holders();              // client-ish function
    let_ai_think(gs, &sv_randie);                           // server function
    do_weather_spawn();                       // server function
    do_enchant_spawn(rand_idx);                       // server function

    // unbuffer the updated latches after let_ai_think() and before move_characters()
    sv_unbufferLatches(gs->ss);              // server function

    move_characters(gs, &sv_randie);                       // client/server function
    move_particles(&sv_randie);                        // client/server function
    attach_particles();                      // client/server function
    make_onwhichfan(gs, &sv_randie);                       // client/server function
    bump_characters(gs);                       // client/server function
    stat_return(gs, &sv_randie);                           // client/server function
    pit_kill(gs, &sv_randie);                              // client/server function

    {
      // Generate the new seed
      randsave += *((Uint32*) & md2normals[wldframe&127][0]);
      randsave += *((Uint32*) & md2normals[randsave&127][1]);
    }

    // Timers
    wldclock += FRAMESKIP;
    wldframe++;
  }
}

//--------------------------------------------------------------------------------------------
void update_game(GAME_STATE * gs, float dFrame, Uint32 * rand_idx)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.
  int    cnt, numdead;
  Uint32 game_randie = *rand_idx;

  RANDIE(*rand_idx);

  // client stuff
  {
    // Check for all local players being dead
    gs->cs->allpladead = bfalse;
    gs->cs->seeinvisible = bfalse;
    gs->cs->seekurse = bfalse;
    numdead = 0;
    for (cnt = 0; cnt < MAXPLAYER; cnt++)
    {
      if(!PlaList[cnt].valid || INPUTNONE==PlaList[cnt].device) continue;

      if (!ChrList[PlaList[cnt].index].alive && ChrList[PlaList[cnt].index].islocalplayer)
      {
        numdead++;
      };

      if (ChrList[PlaList[cnt].index].canseeinvisible)
      {
        gs->cs->seeinvisible = btrue;
      }

      if (ChrList[PlaList[cnt].index].canseekurse)
      {
        gs->cs->seekurse = btrue;
      }
    }

    if (numdead >= numlocalpla)
    {
      gs->cs->allpladead = btrue;
    }
  }

  // This is the main game loop
  gs->cs->msg_timechange = 0;

  // [claforte Jan 6th 2001]
  // TODO: Put that back in place once networking is functional.
  // Important stuff to keep in sync
  while ( (wldclock<allclock) )
  {
    srand(randsave);                          // client/server function
    resize_characters();                      // client function
    keep_weapons_with_holders();              // client-ish function
    let_ai_think(gs, &game_randie);                           // server function
    do_weather_spawn(&game_randie);                       // server function
    do_enchant_spawn(&game_randie);                       // server function

    // unbuffer the updated latches after let_ai_think() and before move_characters()
    //cl_unbufferLatches(gs->cs);        // client function
    //sv_unbufferLatches(gs->ss);        // server function

    check_respawn(gs, &game_randie);                         // client function

    move_characters(gs, &game_randie);                       // client/server function
    move_particles(&game_randie);                        // client/server function
    make_character_matrices();               // client function
    attach_particles();                      // client/server function
    make_onwhichfan(gs, &game_randie);                       // client/server function
    bump_characters(gs);                       // client/server function
    stat_return(gs, &game_randie);                           // client/server function
    pit_kill(gs, &game_randie);                              // client/server function

    {
      // Generate the new seed
      randsave += *((Uint32*) & md2normals[wldframe&127][0]);
      randsave += *((Uint32*) & md2normals[randsave&127][1]);
    }

    // Stuff for which sync doesn't matter
    flash_select();                          // client function
    animate_tiles();                         // client function
    move_water();                            // client function

    // Timers
    wldclock += FRAMESKIP;
    wldframe++;
    gs->cs->msg_timechange++;
    if (gs->cs->statdelay > 0)  gs->cs->statdelay--;
    gs->cs->statclock++;
  }

  if (gs->ns->networkon)
  {
    if (gs->cs->numplatimes == 0)
    {
      // The remote ran out of messages, and is now twiddling its thumbs...
      // Make it go slower so it doesn't happen again
      wldclock += FRAMESKIP/4.0f;
    }
    else if (gs->cs->amClient && gs->cs->numplatimes > 3)
    {
      // The host has too many messages, and is probably experiencing control
      // gs->cd->lag...  Speed it up so it gets closer to sync
      wldclock -= FRAMESKIP/4.0f;
    }
  }
}

//--------------------------------------------------------------------------------------------
void update_timers()
{
  // ZZ> This function updates the game timers
  lstclock = allclock;
  allclock = SDL_GetTicks() - sttclock;
  fpsclock += allclock - lstclock;
  if (fpsclock >= TICKS_PER_SEC)
  {
    create_szfpstext(fpsframe);
    fpsclock = 0;
    fpsframe = 0;
  }

  if(fpsclock > 0)
  {
    stabilizedfps = stabilizedfps*0.9 + 0.1*(float)fpsframe / ((float)fpsclock / (float)FRAMESKIP);
  };
}

//--------------------------------------------------------------------------------------------
void read_pair(FILE* fileread)
{
  // ZZ> This function reads a damage/stat pair ( eg. 5-9 )
  char cTmp;
  float  fBase, fRand;

  fscanf(fileread, "%f", &fBase);  // The first number
  pairbase = fBase * 256;
  cTmp = get_first_letter(fileread);  // The hyphen
  if (cTmp != '-')
  {
    // Not in correct format, so fail
    pairrand = 1;
    return;
  }
  fscanf(fileread, "%f", &fRand);  // The second number
  pairrand = fRand * 256;
  pairrand = pairrand - pairbase;
  if (pairrand < 1)
    pairrand = 1;
}

//--------------------------------------------------------------------------------------------
void undo_pair(int base, int rand)
{
  // ZZ> This function generates a damage/stat pair ( eg. 3-6.5 )
  //     from the base and random values.  It set pairfrom and
  //     pairto
  pairfrom = base / 256.0;
  pairto = rand / 256.0;
  if (pairfrom < 0.0)  pairfrom = 0.0;
  if (pairto < 0.0)  pairto = 0.0;
  pairto += pairfrom;
}

//--------------------------------------------------------------------------------------------
void ftruthf(FILE* filewrite, char* text, Uint8 truth)
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     btrue bfalse statements

  fprintf(filewrite, text);
  if (truth)
  {
    fprintf(filewrite, "TRUE\n");
  }
  else
  {
    fprintf(filewrite, "FALSE\n");
  }
}

//--------------------------------------------------------------------------------------------
void fdamagf(FILE* filewrite, char* text, Uint8 damagetype)
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
  fprintf(filewrite, text);
  if (damagetype == DAMAGESLASH)
    fprintf(filewrite, "SLASH\n");
  if (damagetype == DAMAGECRUSH)
    fprintf(filewrite, "CRUSH\n");
  if (damagetype == DAMAGEPOKE)
    fprintf(filewrite, "POKE\n");
  if (damagetype == DAMAGEHOLY)
    fprintf(filewrite, "HOLY\n");
  if (damagetype == DAMAGEEVIL)
    fprintf(filewrite, "EVIL\n");
  if (damagetype == DAMAGEFIRE)
    fprintf(filewrite, "FIRE\n");
  if (damagetype == DAMAGEICE)
    fprintf(filewrite, "ICE\n");
  if (damagetype == DAMAGEZAP)
    fprintf(filewrite, "ZAP\n");
  if (damagetype == DAMAGENULL)
    fprintf(filewrite, "NONE\n");
}

//--------------------------------------------------------------------------------------------
void factiof(FILE* filewrite, char* text, Uint8 action)
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
  fprintf(filewrite, text);
  if (action == ACTIONDA)
    fprintf(filewrite, "WALK\n");
  if (action == ACTIONUA)
    fprintf(filewrite, "UNARMED\n");
  if (action == ACTIONTA)
    fprintf(filewrite, "THRUST\n");
  if (action == ACTIONSA)
    fprintf(filewrite, "SLASH\n");
  if (action == ACTIONCA)
    fprintf(filewrite, "CHOP\n");
  if (action == ACTIONBA)
    fprintf(filewrite, "BASH\n");
  if (action == ACTIONLA)
    fprintf(filewrite, "LONGBOW\n");
  if (action == ACTIONXA)
    fprintf(filewrite, "XBOW\n");
  if (action == ACTIONFA)
    fprintf(filewrite, "FLING\n");
  if (action == ACTIONPA)
    fprintf(filewrite, "PARRY\n");
  if (action == ACTIONZA)
    fprintf(filewrite, "ZAP\n");
}

//--------------------------------------------------------------------------------------------
void fgendef(FILE* filewrite, char* text, Uint8 gender)
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     MALE FEMALE OTHER statements

  fprintf(filewrite, text);
  if (gender == GENMALE)
    fprintf(filewrite, "MALE\n");
  if (gender == GENFEMALE)
    fprintf(filewrite, "FEMALE\n");
  if (gender == GENOTHER)
    fprintf(filewrite, "OTHER\n");
}

//--------------------------------------------------------------------------------------------
void fpairof(FILE* filewrite, char* text, int base, int rand)
{
  // ZZ> This function mimics fprintf in spitting out
  //     damage/stat pairs
  undo_pair(base, rand);
  fprintf(filewrite, text);
  fprintf(filewrite, "%4.2f-%4.2f\n", pairfrom, pairto);
}

//--------------------------------------------------------------------------------------------
void funderf(FILE* filewrite, char* text, char* usename)
{
  // ZZ> This function mimics fprintf in spitting out
  //     a name with underscore spaces
  char cTmp;
  int cnt;


  fprintf(filewrite, text);
  cnt = 0;
  cTmp = usename[0];
  cnt++;
  while (cTmp != 0)
  {
    if (cTmp == ' ')
    {
      fprintf(filewrite, "_");
    }
    else
    {
      fprintf(filewrite, "%c", cTmp);
    }
    cTmp = usename[cnt];
    cnt++;
  }
  fprintf(filewrite, "\n");
}

//--------------------------------------------------------------------------------------------
void get_message(FILE* fileread)
{
  // ZZ> This function loads a string into the message buffer, making sure it
  //     is null terminated.
  int cnt;
  char cTmp;
  STRING szTmp;


  if (GMsg.total < MAXTOTALMESSAGE)
  {
    if (GMsg.totalindex >= MESSAGEBUFFERSIZE)
    {
      GMsg.totalindex = MESSAGEBUFFERSIZE - 1;
    }
    GMsg.index[GMsg.total] = GMsg.totalindex;
    fscanf(fileread, "%s", szTmp);
    szTmp[255] = 0;
    cTmp = szTmp[0];
    cnt = 1;
    while (cTmp != 0 && GMsg.totalindex < MESSAGEBUFFERSIZE - 1)
    {
      if (cTmp == '_')  cTmp = ' ';
      GMsg.text[GMsg.totalindex] = cTmp;
      GMsg.totalindex++;
      cTmp = szTmp[cnt];
      cnt++;
    }
    GMsg.text[GMsg.totalindex] = 0;  GMsg.totalindex++;
    GMsg.total++;
  }
}

//--------------------------------------------------------------------------------------------
void load_all_messages(char *loadname, int object)
{
  // ZZ> This function loads all of an objects messages
  FILE *fileread;


  MadList[object].msg_start = 0;
  fileread = fopen(loadname, "r");
  if (fileread)
  {
    MadList[object].msg_start = GMsg.total;
    while (goto_colon_yesno(fileread))
    {
      get_message(fileread);
    }
    fclose(fileread);
  }
}


//--------------------------------------------------------------------------------------------
void reset_teams()
{
  // ZZ> This function makes everyone hate everyone else
  int teama, teamb;


  teama = 0;
  while (teama < MAXTEAM)
  {
    // Make the team hate everyone
    teamb = 0;
    while (teamb < MAXTEAM)
    {
      TeamList[teama].hatesteam[teamb] = btrue;
      teamb++;
    }
    // Make the team like itself
    TeamList[teama].hatesteam[teama] = bfalse;
    // Set defaults
    TeamList[teama].leader = NOLEADER;
    TeamList[teama].sissy = 0;
    TeamList[teama].morale = 0;
    teama++;
  }


  // Keep the null team neutral
  teama = 0;
  while (teama < MAXTEAM)
  {
    TeamList[teama].hatesteam[NULLTEAM] = bfalse;
    TeamList[NULLTEAM].hatesteam[teama] = bfalse;
    teama++;
  }
}

//--------------------------------------------------------------------------------------------
void reset_messages(GAME_STATE * gs)
{
  // ZZ> This makes messages safe to use
  int cnt;

  GMsg.total = 0;
  GMsg.totalindex = 0;
  gs->cs->msg_timechange = 0;
  GMsg.start = 0;
  cnt = 0;
  while (cnt < MAXMESSAGE)
  {
    GMsg.list[cnt].time = 0;
    cnt++;
  }

  cnt = 0;
  while (cnt < MAXTOTALMESSAGE)
  {
    GMsg.index[cnt] = 0;
    cnt++;
  }

  GMsg.text[0] = 0;
}



//--------------------------------------------------------------------------------------------
void reset_timers(GAME_STATE * gs)
{
  // ZZ> This function resets the timers...
  sttclock = SDL_GetTicks();
  allclock = 0;
  lstclock = 0;
  wldclock = 0;
  gs->cs->statclock = 0;
  pitclock = 0;  pitskill = bfalse;
  wldframe = 0;
  allframe = 0;
  fpsframe = 0;
  outofsync = bfalse;
}

extern bool_t initMenus();

#define DO_CONFIGSTRING_COMPARE(XX) if(strncmp(#XX, szin, strlen(#XX))) { if(NULL!=szout) *szout = szin + strlen(#XX); return cd->XX; }
//--------------------------------------------------------------------------------------------
char * get_config_string(CONFIG_DATA * cd, char * szin, char ** szout)
{
  // BB > localize a string by converting the name of the string to the string itself

  DO_CONFIGSTRING_COMPARE(basicdat_dir)
  else DO_CONFIGSTRING_COMPARE(gamedat_dir)
  else DO_CONFIGSTRING_COMPARE(menu_dir)
  else DO_CONFIGSTRING_COMPARE(globalparticles_dir)
  else DO_CONFIGSTRING_COMPARE(modules_dir)
  else DO_CONFIGSTRING_COMPARE(music_dir)
  else DO_CONFIGSTRING_COMPARE(objects_dir)
  else DO_CONFIGSTRING_COMPARE(import_dir)
  else DO_CONFIGSTRING_COMPARE(players_dir)
  else DO_CONFIGSTRING_COMPARE(nullicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(keybicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(mousicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(joyaicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(joybicon_bitmap)
  else DO_CONFIGSTRING_COMPARE(tile0_bitmap)
  else DO_CONFIGSTRING_COMPARE(tile1_bitmap)
  else DO_CONFIGSTRING_COMPARE(tile2_bitmap)
  else DO_CONFIGSTRING_COMPARE(tile3_bitmap)
  else DO_CONFIGSTRING_COMPARE(watertop_bitmap)
  else DO_CONFIGSTRING_COMPARE(waterlow_bitmap)
  else DO_CONFIGSTRING_COMPARE(phong_bitmap)
  else DO_CONFIGSTRING_COMPARE(plan_bitmap)
  else DO_CONFIGSTRING_COMPARE(blip_bitmap)
  else DO_CONFIGSTRING_COMPARE(font_bitmap)
  else DO_CONFIGSTRING_COMPARE(icon_bitmap)
  else DO_CONFIGSTRING_COMPARE(bars_bitmap)
  else DO_CONFIGSTRING_COMPARE(particle_bitmap)
  else DO_CONFIGSTRING_COMPARE(title_bitmap)
  else DO_CONFIGSTRING_COMPARE(menu_main_bitmap)
  else DO_CONFIGSTRING_COMPARE(menu_advent_bitmap)
  else DO_CONFIGSTRING_COMPARE(menu_sleepy_bitmap)
  else DO_CONFIGSTRING_COMPARE(menu_gnome_bitmap)
  else DO_CONFIGSTRING_COMPARE(slotused_file)
  else DO_CONFIGSTRING_COMPARE(passage_file)
  else DO_CONFIGSTRING_COMPARE(aicodes_file)
  else DO_CONFIGSTRING_COMPARE(actions_file)
  else DO_CONFIGSTRING_COMPARE(alliance_file)
  else DO_CONFIGSTRING_COMPARE(fans_file)
  else DO_CONFIGSTRING_COMPARE(fontdef_file)
  else DO_CONFIGSTRING_COMPARE(menu_file)
  else DO_CONFIGSTRING_COMPARE(money1_file)
  else DO_CONFIGSTRING_COMPARE(money5_file)
  else DO_CONFIGSTRING_COMPARE(money25_file)
  else DO_CONFIGSTRING_COMPARE(money100_file)
  else DO_CONFIGSTRING_COMPARE(weather4_file)
  else DO_CONFIGSTRING_COMPARE(weather5_file)
  else DO_CONFIGSTRING_COMPARE(script_file)
  else DO_CONFIGSTRING_COMPARE(ripple_file)
  else DO_CONFIGSTRING_COMPARE(scancode_file)
  else DO_CONFIGSTRING_COMPARE(playlist_file)
  else DO_CONFIGSTRING_COMPARE(spawn_file)
  else DO_CONFIGSTRING_COMPARE(wawalite_file)
  else DO_CONFIGSTRING_COMPARE(defend_file)
  else DO_CONFIGSTRING_COMPARE(splash_file)
  else DO_CONFIGSTRING_COMPARE(mesh_file)
  else DO_CONFIGSTRING_COMPARE(setup_file)
  else DO_CONFIGSTRING_COMPARE(log_file)
  else DO_CONFIGSTRING_COMPARE(controls_file)
  else DO_CONFIGSTRING_COMPARE(data_file)
  else DO_CONFIGSTRING_COMPARE(copy_file)
  else DO_CONFIGSTRING_COMPARE(enchant_file)
  else DO_CONFIGSTRING_COMPARE(message_file)
  else DO_CONFIGSTRING_COMPARE(naming_file)
  else DO_CONFIGSTRING_COMPARE(modules_file)
  else DO_CONFIGSTRING_COMPARE(setup_file)
  else DO_CONFIGSTRING_COMPARE(skin_file)
  else DO_CONFIGSTRING_COMPARE(credits_file)
  else DO_CONFIGSTRING_COMPARE(quest_file)
  else DO_CONFIGSTRING_COMPARE(uifont_ttf)
  else DO_CONFIGSTRING_COMPARE(coinget_sound)
  else DO_CONFIGSTRING_COMPARE(defend_sound)
  else DO_CONFIGSTRING_COMPARE(coinfall_sound)
  else DO_CONFIGSTRING_COMPARE(lvlup_sound);

  return NULL;
}


//--------------------------------------------------------------------------------------------
char * get_config_string_name(CONFIG_DATA * cd, STRING * pconfig_string)
{
  // BB > localize a string by converting the name of the string to the string itself

  if(pconfig_string == &(cd->basicdat_dir)) return "basicdat_dir";
  else if(pconfig_string == &(cd->gamedat_dir)) return "gamedat_dir";
  else if(pconfig_string == &(cd->menu_dir)) return "menu_dir";
  else if(pconfig_string == &(cd->globalparticles_dir)) return "globalparticles_dir";
  else if(pconfig_string == &(cd->modules_dir)) return "modules_dir";
  else if(pconfig_string == &(cd->music_dir)) return "music_dir";
  else if(pconfig_string == &(cd->objects_dir)) return "objects_dir";
  else if(pconfig_string == &(cd->import_dir)) return "import_dir";
  else if(pconfig_string == &(cd->players_dir)) return "players_dir";
  else if(pconfig_string == &(cd->nullicon_bitmap)) return "nullicon_bitmap";
  else if(pconfig_string == &(cd->keybicon_bitmap)) return "keybicon_bitmap";
  else if(pconfig_string == &(cd->mousicon_bitmap)) return "mousicon_bitmap";
  else if(pconfig_string == &(cd->joyaicon_bitmap)) return "joyaicon_bitmap";
  else if(pconfig_string == &(cd->joybicon_bitmap)) return "joybicon_bitmap";
  else if(pconfig_string == &(cd->tile0_bitmap)) return "tile0_bitmap";
  else if(pconfig_string == &(cd->tile1_bitmap)) return "tile1_bitmap";
  else if(pconfig_string == &(cd->tile2_bitmap)) return "tile2_bitmap";
  else if(pconfig_string == &(cd->tile3_bitmap)) return "tile3_bitmap";
  else if(pconfig_string == &(cd->watertop_bitmap)) return "watertop_bitmap";
  else if(pconfig_string == &(cd->waterlow_bitmap)) return "waterlow_bitmap";
  else if(pconfig_string == &(cd->phong_bitmap)) return "phong_bitmap";
  else if(pconfig_string == &(cd->plan_bitmap)) return "plan_bitmap";
  else if(pconfig_string == &(cd->blip_bitmap)) return "blip_bitmap";
  else if(pconfig_string == &(cd->font_bitmap)) return "font_bitmap";
  else if(pconfig_string == &(cd->icon_bitmap)) return "icon_bitmap";
  else if(pconfig_string == &(cd->bars_bitmap)) return "bars_bitmap";
  else if(pconfig_string == &(cd->particle_bitmap)) return "particle_bitmap";
  else if(pconfig_string == &(cd->title_bitmap)) return "title_bitmap";
  else if(pconfig_string == &(cd->menu_main_bitmap)) return "menu_main_bitmap";
  else if(pconfig_string == &(cd->menu_advent_bitmap)) return "menu_advent_bitmap";
  else if(pconfig_string == &(cd->menu_sleepy_bitmap)) return "menu_sleepy_bitmap";
  else if(pconfig_string == &(cd->menu_gnome_bitmap)) return "menu_gnome_bitmap";
  else if(pconfig_string == &(cd->slotused_file)) return "slotused_file";
  else if(pconfig_string == &(cd->passage_file)) return "passage_file";
  else if(pconfig_string == &(cd->aicodes_file)) return "aicodes_file";
  else if(pconfig_string == &(cd->actions_file)) return "actions_file";
  else if(pconfig_string == &(cd->alliance_file)) return "alliance_file";
  else if(pconfig_string == &(cd->fans_file)) return "fans_file";
  else if(pconfig_string == &(cd->fontdef_file)) return "fontdef_file";
  else if(pconfig_string == &(cd->menu_file)) return "menu_file";
  else if(pconfig_string == &(cd->money1_file)) return "money1_file";
  else if(pconfig_string == &(cd->money5_file)) return "money5_file";
  else if(pconfig_string == &(cd->money25_file)) return "money25_file";
  else if(pconfig_string == &(cd->money100_file)) return "money100_file";
  else if(pconfig_string == &(cd->weather4_file)) return "weather4_file";
  else if(pconfig_string == &(cd->weather5_file)) return "weather5_file";
  else if(pconfig_string == &(cd->script_file)) return "script_file";
  else if(pconfig_string == &(cd->ripple_file)) return "ripple_file";
  else if(pconfig_string == &(cd->scancode_file)) return "scancode_file";
  else if(pconfig_string == &(cd->playlist_file)) return "playlist_file";
  else if(pconfig_string == &(cd->spawn_file)) return "spawn_file";
  else if(pconfig_string == &(cd->wawalite_file)) return "wawalite_file";
  else if(pconfig_string == &(cd->defend_file)) return "defend_file";
  else if(pconfig_string == &(cd->splash_file)) return "splash_file";
  else if(pconfig_string == &(cd->mesh_file)) return "mesh_file";
  else if(pconfig_string == &(cd->setup_file)) return "setup_file";
  else if(pconfig_string == &(cd->log_file)) return "log_file";
  else if(pconfig_string == &(cd->controls_file)) return "controls_file";
  else if(pconfig_string == &(cd->data_file)) return "data_file";
  else if(pconfig_string == &(cd->copy_file)) return "copy_file";
  else if(pconfig_string == &(cd->enchant_file)) return "enchant_file";
  else if(pconfig_string == &(cd->message_file)) return "message_file";
  else if(pconfig_string == &(cd->naming_file)) return "naming_file";
  else if(pconfig_string == &(cd->modules_file)) return "modules_file";
  else if(pconfig_string == &(cd->setup_file)) return "setup_file";
  else if(pconfig_string == &(cd->skin_file)) return "skin_file";
  else if(pconfig_string == &(cd->credits_file)) return "credits_file";
  else if(pconfig_string == &(cd->quest_file)) return "quest_file";
  else if(pconfig_string == &(cd->uifont_ttf)) return "uifont_ttf";
  else if(pconfig_string == &(cd->coinget_sound)) return "coinget_sound";
  else if(pconfig_string == &(cd->defend_sound)) return "defend_sound";
  else if(pconfig_string == &(cd->coinfall_sound)) return "coinfall_sound";
  else if(pconfig_string == &(cd->lvlup_sound)) return "lvlup_sound";

  return NULL;
}

//--------------------------------------------------------------------------------------------
void set_default_config_data(CONFIG_DATA * cd)
{
  Uint32 cnt;


  strncpy(cd->basicdat_dir, "basicdat", sizeof(STRING));
  strncpy(cd->gamedat_dir, "gamedat" , sizeof(STRING));
  strncpy(cd->menu_dir, "menu" , sizeof(STRING));
  strncpy(cd->globalparticles_dir, "globalparticles" , sizeof(STRING));
  strncpy(cd->modules_dir, "modules" , sizeof(STRING));
  strncpy(cd->music_dir, "music" , sizeof(STRING));
  strncpy(cd->objects_dir, "objects" , sizeof(STRING));
  strncpy(cd->import_dir, "import" , sizeof(STRING));
  strncpy(cd->players_dir, "players", sizeof(STRING));

  strncpy(cd->nullicon_bitmap, "nullicon.bmp" , sizeof(STRING));
  strncpy(cd->keybicon_bitmap, "GKeyb.icon.bmp" , sizeof(STRING));
  strncpy(cd->mousicon_bitmap, "GMous.icon.bmp" , sizeof(STRING));
  strncpy(cd->joyaicon_bitmap, "GJoy[0].icon.bmp" , sizeof(STRING));
  strncpy(cd->joybicon_bitmap, "GJoy[1].icon.bmp" , sizeof(STRING));

  strncpy(cd->tile0_bitmap, "tile0.bmp" , sizeof(STRING));
  strncpy(cd->tile1_bitmap, "tile1.bmp" , sizeof(STRING));
  strncpy(cd->tile2_bitmap, "tile2.bmp" , sizeof(STRING));
  strncpy(cd->tile3_bitmap, "tile3.bmp" , sizeof(STRING));
  strncpy(cd->watertop_bitmap, "watertop.bmp" , sizeof(STRING));
  strncpy(cd->waterlow_bitmap, "waterlow.bmp" , sizeof(STRING));
  strncpy(cd->phong_bitmap, "phong.bmp" , sizeof(STRING));
  strncpy(cd->plan_bitmap, "plan.bmp" , sizeof(STRING));
  strncpy(cd->blip_bitmap, "blip.bmp" , sizeof(STRING));
  strncpy(cd->font_bitmap, "font.bmp" , sizeof(STRING));
  strncpy(cd->icon_bitmap, "icon.bmp" , sizeof(STRING));
  strncpy(cd->bars_bitmap, "bars.bmp" , sizeof(STRING));
  strncpy(cd->particle_bitmap, "particle.bmp" , sizeof(STRING));
  strncpy(cd->title_bitmap, "title.bmp" , sizeof(STRING));

  strncpy(cd->menu_main_bitmap, "menu_main.bmp" , sizeof(STRING));
  strncpy(cd->menu_advent_bitmap, "menu_advent.bmp" , sizeof(STRING));
  strncpy(cd->menu_sleepy_bitmap, "menu_sleepy.bmp" , sizeof(STRING));
  strncpy(cd->menu_gnome_bitmap, "menu_gnome.bmp" , sizeof(STRING));


  strncpy(cd->slotused_file, "slotused.txt" , sizeof(STRING));
  strncpy(cd->passage_file, "passage.txt" , sizeof(STRING));
  strncpy(cd->aicodes_file, "aicodes.txt" , sizeof(STRING));
  strncpy(cd->actions_file, "actions.txt" , sizeof(STRING));
  strncpy(cd->alliance_file, "alliance.txt" , sizeof(STRING));
  strncpy(cd->fans_file, "fans.txt" , sizeof(STRING));
  strncpy(cd->fontdef_file, "font.txt" , sizeof(STRING));
  strncpy(cd->menu_file, "menu.txt" , sizeof(STRING));
  strncpy(cd->money1_file, "1money.txt" , sizeof(STRING));
  strncpy(cd->money5_file, "5money.txt" , sizeof(STRING));
  strncpy(cd->money25_file, "25money.txt" , sizeof(STRING));
  strncpy(cd->money100_file, "100money.txt" , sizeof(STRING));
  strncpy(cd->weather4_file, "weather4.txt" , sizeof(STRING));
  strncpy(cd->weather5_file, "weather5.txt" , sizeof(STRING));
  strncpy(cd->script_file, "script.txt" , sizeof(STRING));
  strncpy(cd->ripple_file, "ripple.txt" , sizeof(STRING));
  strncpy(cd->scancode_file, "scancode.txt" , sizeof(STRING));
  strncpy(cd->playlist_file, "playlist.txt" , sizeof(STRING));
  strncpy(cd->spawn_file, "spawn.txt" , sizeof(STRING));
  strncpy(cd->wawalite_file, "wawalite.txt" , sizeof(STRING));
  strncpy(cd->defend_file, "defend.txt" , sizeof(STRING));
  strncpy(cd->splash_file, "splash.txt" , sizeof(STRING));
  strncpy(cd->mesh_file, "level.mpd" , sizeof(STRING));
  strncpy(cd->setup_file, "setup.txt" , sizeof(STRING));
  strncpy(cd->log_file, "log.txt", sizeof(STRING));
  strncpy(cd->controls_file, "controls.txt", sizeof(STRING));
  strncpy(cd->data_file, "data.txt", sizeof(STRING));
  strncpy(cd->copy_file, "copy.txt", sizeof(STRING));
  strncpy(cd->enchant_file, "enchant.txt", sizeof(STRING));
  strncpy(cd->message_file, "message.txt", sizeof(STRING));
  strncpy(cd->naming_file, "naming.txt", sizeof(STRING));
  strncpy(cd->modules_file, "modules.txt", sizeof(STRING));
  strncpy(cd->setup_file, "setup.txt", sizeof(STRING));
  strncpy(cd->skin_file, "skin.txt", sizeof(STRING));
  strncpy(cd->credits_file, "credits.txt", sizeof(STRING));
  strncpy(cd->quest_file, "quest.txt", sizeof(STRING));


  cd->uifont_points  = 20;
  cd->uifont_points2 = 18;
  strncpy(cd->uifont_ttf, "Negatori.ttf" , sizeof(STRING));

  strncpy(cd->coinget_sound, "coinget.wav" , sizeof(STRING));
  strncpy(cd->defend_sound, "defend.wav" , sizeof(STRING));
  strncpy(cd->coinfall_sound, "coinfall.wav" , sizeof(STRING));
  strncpy(cd->lvlup_sound, "lvlup.wav" , sizeof(STRING));

  cd->fullscreen = bfalse;
  cd->zreflect = bfalse;
  cd->maxtotalmeshvertices = 256*256*6;
  cd->scrd = 8;                   // Screen bit depth
  cd->scrx = 320;                 // Screen X size
  cd->scry = 200;                 // Screen Y size
  cd->scrz = 16;                  // Screen z-buffer depth ( 8 unsupported )
  cd->maxmessage = MAXMESSAGE;    //
  cd->messageon  = btrue;           // Messages?
  cd->wraptolerance = 80;     // Status bar
  cd->staton = btrue;                 // Draw the status bars?
  cd->overlayon = bfalse;      
  cd->perspective = bfalse;    
  cd->dither = bfalse;         
  cd->shading = GL_FLAT;       
  cd->antialiasing = bfalse;   
  cd->refon = bfalse;          
  cd->shaon = bfalse;          
  cd->texturefilter = TX_LINEAR;       
  cd->wateron = btrue;         
  cd->shasprite = bfalse;  
  cd->phongon = btrue;                // Do phong overlay?
  cd->zreflect = bfalse; 
  cd->reffadeor = 0;              // 255 = Don't fade reflections
  cd->twolayerwateron = bfalse;        // Two layer water?
  cd->overlayvalid = bfalse;               // Allow large overlay?
  cd->backgroundvalid = bfalse;            // Allow large background?
  cd->fogallowed = btrue;          //
  cd->soundvalid = bfalse;     //Allow playing of sound?
  cd->musicvalid = bfalse;     // Allow music and loops?
  cd->request_network  = btrue;              // Try to connect?
  cd->lag        = 3;                                // Lag tolerance
  //cd->GOrder.lag   = 25;                                // RTS Lag tolerance

  strncpy(cd->net_hosts[0], "localhost", sizeof(cd->net_hosts[0]));                            // Name for hosting session
  for(cnt=0; cnt<MAXNETPLAYER; cnt++)
  {
    cd->net_hosts[cnt][0] = 0x00;
  };

  strncpy(cd->net_messagename, "little Raoul", sizeof(cd->net_messagename));                 // Name for messages
  cd->fpson = btrue;               // FPS displayed?
  // Debug option
  cd->GrabMouse = SDL_GRAB_ON;
  cd->HideMouse = bfalse;
  cd->DevMode  = btrue;
  // Debug option
};

//--------------------------------------------------------------------------------------------
void game_handleKeyboard(GAME_STATE * gs, float dFrame)
{
  check_screenshot();

  //Todo zefz: where to put this?
  //Check for pause key   //TODO: What to do in network games?
  if (!SDLKEYDOWN(SDLK_F8)) gs->can_pause = btrue;
  if (SDLKEYDOWN(SDLK_F8) && GKeyb.on && gs->can_pause)
  {
    if (gs->paused) gs->paused = bfalse;
    else gs->paused = btrue;
    gs->can_pause = bfalse;
  }

  if(SDLKEYDOWN(SDLK_F9) && gs->cd->DevMode)
  {
    gs->ingameMenuActive = btrue;
    SDL_WM_GrabInput(SDL_GRAB_OFF);
    SDL_ShowCursor(SDL_DISABLE);
    GMous.on = bfalse;
  }

  // Check for quitters
  // :TODO: gs->nolocalplayers is not set correctly
  if (SDLKEYDOWN(SDLK_ESCAPE) /*|| gs->nolocalplayers*/)
  {
    quit_module(gs);
    gs->Active = bfalse;
    gs->menuActive = btrue;
  }
};

//--------------------------------------------------------------------------------------------
void game_handleIO(GAME_STATE * gs, float dFrame)
{
  set_local_latches(gs);                   // client function

  // NETWORK PORT
  if(gs->ns->networkon)
  {
    // buffer the existing latches
    input_net_message(gs);        // client function
    cl_bufferLatches(gs->cs);     // client function
    sv_bufferLatches(gs->ss);     // server function

    // upload the information
    cl_talkToHost(gs->cs);        // client function
    sv_talkToRemotes(gs->ss);     // server function

    // download/handle any queued packets
    //listen_for_packets(gs->ns);               // client/server function
  } 
};

//--------------------------------------------------------------------------------------------
void game_handleGraphics(GAME_STATE * gs, float dFrame)
{
  move_camera(gs, 1.0);                     // client function 
  figure_out_what_to_draw(gs);              // client function
  draw_main(gs, 1.0);                       // client function
};

//--------------------------------------------------------------------------------------------
float game_frameStep(GAME_STATE * gs, float dFrame)
{
  bool_t client_running = bfalse, server_running = bfalse, local_running = bfalse;

  if (NULL==gs || dFrame < 0.1) 
    return dFrame;


  local_running  = !gs->ns->networkon;
  client_running = gs->ns->networkon && gs->cs->amClient && !gs->cs->waiting;
  server_running = gs->ns->networkon && gs->ss->amHost   &&  gs->ss->ready;


  // Do important things
  if (!local_running && !client_running && !server_running)
  {
    wldclock = allclock;
  }
  else if (gs->paused && !server_running && (local_running || client_running))
  {
    wldclock = allclock;
  }
  else
  {
    // This is the control loop
    read_input();

    game_handleKeyboard(gs, dFrame);

    update_timers();                       // client/server function
    check_stats(gs);                       // client function

    check_passage_music();                 // client function

    // handle the game and network IO
    game_handleIO(gs, dFrame);

    // make sure we do enough game updates
    while(dFrame>=1.0)
    {
      if(local_running)
      {
        update_game(gs, 1.0, &(gs->randie_index));                     // non-networked function
      }
      else 
      {
        if(server_running)
          sv_update_game(gs, 1.0, &(gs->randie_index));                  // server function

        if(client_running)
          cl_update_game(gs, 1.0, &(gs->randie_index));                  // client function
      };

      dFrame -= 1.0f;
    }

    if(client_running || local_running)
    {
      game_handleGraphics(gs, dFrame);
      request_pageflip();
    };
  };

  return dFrame;
};

//--------------------------------------------------------------------------------------------
GameStage doGame(GAME_STATE * gs, float dFrame)
{
  static int gameStage = Stage_Beginning;

  switch (gameStage)
  {
    case Stage_Beginning:

      SDL_WM_GrabInput(gs->cd->GrabMouse);

      // Start a new module
      srand((Uint32)(-1));
  	
      pressed = bfalse;
      make_onwhichfan(gs, &(gs->randie_index));
      reset_camera(gs);
      reset_timers(gs);
      figure_out_what_to_draw(gs);
      make_character_matrices();
      attach_particles();

      if (gs->ns->networkon)
      {
        gs->modstate.net_messagemode = bfalse;
        GNetMsg.delay = 20;

        log_info("SDL_main: Setting up network game connections\n");

        if(net_beginGame(gs))
        {
          net_sayHello(gs->ns);
        }
        else
        {
          log_warning("SDL_main: Could not set up network game. Defaulting to non-network play.\n");

          gs->ns->networkon = bfalse;

          gs->cs->amClient  = bfalse;
          gs->cs->waiting   = bfalse;

          gs->ss->amHost    = bfalse;
          gs->ss->ready     = btrue;
        };
      }

      // Let the game go
      gs->moduleActive = btrue;
      randsave = 0;
      srand(0);
      gameStage = Stage_Entering;
    break;

  case Stage_Entering:
    gameStage = Stage_Running;
    break;

  case Stage_Running:
    if(!gs->Active) 
    {
      gameStage = Stage_Leaving;
    }
    else if(!gs->ns->networkon || (gs->ss->amHost && gs->ss->ready) || (gs->cs->amClient && !gs->cs->waiting) )
    {
      game_frameStep(gs, dFrame);
    };
    break;

  case Stage_Leaving:
    gameStage = Stage_Finishing;
    break;

  case Stage_Finishing:
      release_module();
      close_session(gs->ns);

      // Let the normal OS mouse cursor work
      SDL_WM_GrabInput(gs->cd->GrabMouse);
      //SDL_ShowCursor(SDL_ENABLE);
      gameStage = Stage_Beginning;
    break;

  };

  return gameStage;
}

//--------------------------------------------------------------------------------------------
void loopMenu(GAME_STATE * gs, float frameDuration)
{
  int    menuResult;

  //Play the menu music
  play_music(0, 500, -1);

  // do menus
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  read_input();

  //Pressed panic button
  if (SDLKEYDOWN(SDLK_q) && SDLKEYDOWN(SDLK_LCTRL))
  {
    log_info("User pressed escape button (LCTRL+Q)... Quitting game.\n");
    gs->menuActive = bfalse;
    gs->Active = bfalse;
  }

  ui_beginFrame(frameDuration);

  menuResult = doMenu(gs, (float)frameDuration);
  switch (menuResult)
  {
  case 1:
    // Go ahead and start the game
    gs->menuActive = bfalse;
    gs->Active = btrue;
    break;

  case - 1:
    // The user selected "Quit"
    gs->menuActive = bfalse;
    gs->Active = bfalse;
    break;

  }

  ui_endFrame();
  request_pageflip();
};

//--------------------------------------------------------------------------------------------
bool_t request_pageflip()
{
  bool_t retval = !requested_pageflip;

  requested_pageflip = btrue;

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t do_pageflip()
{
  bool_t retval = bfalse;

  if( requested_pageflip )
  {
    requested_pageflip = bfalse;
    allframe++;
    fpsframe++;
    SDL_GL_SwapBuffers();
    retval = btrue;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
GameStage loopMain(int argc, char **argv, GAME_STATE * gs )
{
  double frameDuration;
  static double dDuration = 0.0f;
  static float  dFrame = 0.0f;

  struct glvector t1 = {0, 0, 0};
  struct glvector t2 = {0, 0, -1};
  struct glvector t3 = {0, 1, 0};

  static int gameStage = Stage_Beginning;

  switch (gameStage)
  {
    case Stage_Beginning:

      set_default_config_data(&CData);

      // Initialize logging first, so that we can use it everywhere.
      log_init(gs);
      log_setLoggingLevel(2);

      // start initializing the various subsystems
      log_message("Starting Egoboo %s...\n", VERSION);

      sys_initialize();
      clock_init();
      fs_init();

      make_randie();

      read_setup(gs->cd->setup_file);

      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->scancode_file);
      read_all_tags(CStringTmp1);

      read_controls(gs->cd->controls_file);
      reset_ai_script();

      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->aicodes_file);
      load_ai_codes(CStringTmp1);

      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->actions_file);
      load_action_names(CStringTmp1);

      sdlinit(argc, argv);
      glinit(argc, argv);
      net_initialize(gs->ns, gs->cs, gs->ss, gs);

      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->uifont_ttf);
      ui_initialize(CStringTmp1, gs->cd->uifont_points);

      sdlmixer_initialize();

      if (!get_mesh_memory())
      {
        log_error("Unable to initialize Mesh Memory - Reduce the maximum number of vertices (See SETUP.TXT)\n");
        return bfalse;
      }

      // Matrix init stuff (from remove.c)
      rotmeshtopside = ((float)gs->cd->scrx / gs->cd->scry) * ROTMESHTOPSIDE / (1.33333);
      rotmeshbottomside = ((float)gs->cd->scrx / gs->cd->scry) * ROTMESHBOTTOMSIDE / (1.33333);
      rotmeshup = ((float)gs->cd->scrx / gs->cd->scry) * ROTMESHUP / (1.33333);
      rotmeshdown = ((float)gs->cd->scrx / gs->cd->scry) * ROTMESHDOWN / (1.33333);
      mWorld = IdentityMatrix();
      mViewSave = mView = ViewMatrix(t1, t2, t3, 0);
      mProjection = ProjectionMatrix(.001f, 2000.0f, (float)(FOV * DEG_TO_RAD)); // 60 degree FOV
      mProjection = MatrixMult(Translate(0, 0, -.999996), mProjection); // Fix Z value...
      mProjection = MatrixMult(ScaleXYZ(-1, -1, 100000), mProjection);  // HUK // ...'cause it needs it

      //[claforte] Fudge the values.
      mProjection.v[10] /= 2.0;
      mProjection.v[11] /= 2.0;

      //Load stuff into memory
      prime_icons();
      make_textureoffset();  // THIS SHOULD WORK
      make_lightdirectionlookup(); // THIS SHOULD WORK
      make_turntosin();  // THIS SHOULD WORK
      make_enviro(); // THIS SHOULD WORK
      load_mesh_fans(); // THIS SHOULD WORK
      load_all_music_sounds();

      initMenus();        //Start the game menu

      // initialize the bitmap font so we can use the cursor
      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->font_bitmap);
      snprintf(CStringTmp2, sizeof(CStringTmp2), "%s/%s", gs->cd->basicdat_dir, gs->cd->fontdef_file);
      if (!load_font(CStringTmp1, CStringTmp2))
      {
        log_warning("UI unable to use load bitmap font for cursor. Files missing from %s directory\n", gs->cd->basicdat_dir);
      };


      // Let the normal OS mouse cursor work
      SDL_WM_GrabInput(SDL_GRAB_OFF);
      SDL_ShowCursor(SDL_DISABLE);
      GMous.on = bfalse;

      gameStage = Stage_Entering;
    break;

  case Stage_Entering:
    clock_frameStep();
    frameDuration = clock_getFrameDuration();
    gs->Active = bfalse;
    gs->menuActive = btrue;
    dFrame = 1.0f;
    dDuration = 0.0;

    gameStage = Stage_Running;
    break;

  case Stage_Running:
    // Clock updates each frame
    clock_frameStep();
    frameDuration = clock_getFrameDuration();
    update_timers();
    dDuration += frameDuration;
    dFrame += (float)(frameDuration*1000.0f)/(float)FRAMESKIP;

    // Do the game (if active)
    if (gs->Active && dFrame >= 1.0)
    {
      doGame(gs, dFrame);
    }

    // run through the menus (if active)
    if (gs->menuActive && dFrame >= 1.0)
    {
      loopMenu(gs, dDuration);
      dDuration = 0.0f;
    }

    // handle any requested pageflips
    if(dFrame >= 1.0)
    {
      do_pageflip();
      dFrame = 0.0;
    }
    else
    {
      float tmp = MAX(0, (1.0f-dFrame)*0.5f*FRAMESKIP);
      SDL_Delay( (int)ceil(tmp) );
    };
    if(!gs->Active && !gs->menuActive) gameStage = Stage_Leaving;
    break;

  case Stage_Leaving:
    gameStage = Stage_Finishing;
    break;

  case Stage_Finishing:
    quit_game(gs);
    if (memory_cleanUp(gs) == bfalse) log_message("WARNING! COULD NOT CLEAN MEMORY AND SHUTDOWN SUPPORT SYSTEMS!!");
    gameStage = Stage_Beginning;
    break;
  };

  return gameStage;
};


//--------------------------------------------------------------------------------------------
int SDL_main(int argc, char **argv)
{
  // ZZ> This is where the program starts and all the high level stuff happens

  GameStage stage = Stage_Beginning;
  GAME_STATE AGameState;
  MOD_STATE  AModState;

  AGameState.modstate.loaded  = bfalse;
  AGameState.paused           = bfalse;    //Is the game paused?
  AGameState.can_pause        = btrue;  //Pause button avalible?
  AGameState.Active           = bfalse;         // Stay in game or quit to windows?
  AGameState.menuActive       = btrue;
  AGameState.ingameMenuActive = bfalse;   // Is the in-game menu active?
  AGameState.moduleActive     = bfalse;       // Is the control loop still going?
  AGameState.ms               = &AModState;
  AGameState.ns               = &ANetState;
  AGameState.ss               = &AServerState;
  AGameState.cs               = &AClientState;
  AGameState.cd               = &CData;

  do
  {
    stage = loopMain(argc, argv, &AGameState);
  } 
  while (Stage_Beginning != stage);


  return btrue;
}
