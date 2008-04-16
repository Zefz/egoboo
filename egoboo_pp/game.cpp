// game.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

//#define MENU_DEMO  // Uncomment this to build just the menu demo
#define DECLARE_GLOBALS

#include "Enchant.h"
#include "Character.h"
#include "Ui.h"
#include "Font.h"
#include "JF_Clock.h"
#include "Log.h"
#include "Menu.h"
#include "Input.h"
#include "Mad.h"
#include "network.h"
#include "Camera.h"
#include "Passage.h"
#include "Profile.h"
#include "egoboo.h"

#include "JF_Scheduler.h"
#include "JF_Task.h"

#include <SDL_endian.h>

#define INITGUID
#define NAME "Boo"
#define TITLE "Boo"

#define RELEASE(x) if (x) {x->Release(); x=NULL;}

char IDSZ::strval[5];
const IDSZ IDSZ::NONE("NONE");

Sint32 val[TMP_COUNT] = {0,0,0,0,0,0,0,0,0};

void bump_all(Physics_Info & loc_phys, float dframe);

static void move_water(float dframe = 1.0f);


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

struct Machine_Net : public StateMachine
{
  //virtual void run(float deltaTime);

  Machine_Net(const JF::Scheduler * s) :
StateMachine(s, "Network Process", NULL) {};

Machine_Net(const StateMachine * parent) :
StateMachine("Network Process", parent) {};


protected:
  virtual void Begin(float deltaTime);
  //virtual void Enter(float deltaTime);
  virtual void Run(float deltaTime);
  //virtual void Leave(float deltaTime);
  virtual void Finish(float deltaTime);
};



//--------------------------------------------------------------------------------------------
void general_error(int a, int b, char *szerrortext)
{
  // ZZ> This function displays an error message
  // Steinbach's Guideline for Systems Programming:
  //   Never test for an error condition you don't know how to handle.
  char                buf[0x0100];
  FILE*               filewrite;
  sprintf(buf, "%d, %d... %s\n", 0, 0, szerrortext);

  fprintf(stderr,"ERROR: %s\n",szerrortext);

  filewrite = fopen("errorlog.txt", "w");
  if (filewrite)
  {
    fprintf(filewrite, "I'M MELTING\n");
    fprintf(filewrite, "%d, %d... %s\n", a, b, szerrortext);
    fclose(filewrite);
  }
  release_module();
  close_session();

  release_grfx();
  fclose(globalnetworkerr);
  //DestroyWindow(hWnd);

  SDL_Quit();
  exit(0);
}

//------------------------------------------------------------------------------
//Random Things-----------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void make_newloadname(const char *modname, const char *appendname, char *newloadname)
{
  // ZZ> This function takes some names and puts 'em together

  strcpy(newloadname, modname);
  strcat(newloadname, appendname);
}

//--------------------------------------------------------------------------------------------
void load_global_waves(char *modname)
{
  // ZZ> This function loads the global waves
  char tmploadname[0x0100];
  char newloadname[0x0100];
  char wavename[0x0100];
  int cnt;

  make_newloadname(modname, ("gamedat/"), tmploadname);

  for (cnt = 0; VALID_WAVE_RANGE(cnt); cnt++ )
  {
    sprintf(wavename, "sound%d.wav", cnt);
    make_newloadname(tmploadname, wavename, newloadname);
    globalwave[cnt] = Mix_LoadWAV(newloadname);
  }
}

//---------------------------------------------------------------------------------------------
void export_one_character(int character, int owner, int number)
{
  // ZZ> This function exports a character
  int tnc, profile;
  char letter;
  char fromdir[0x80];
  char todir[0x80];
  char fromfile[0x80];
  char tofile[0x80];
  char todirname[16];
  char todirfullname[0x40];

  // Don't export enchants
  disenchant_character(character);

  profile = ChrList[character].model;
  if ((ChrList[character].getCap().cancarrytonextmodule || !ChrList[character].getCap().is_item) && exportvalid)
  {
    // TWINK_BO.OBJ
    sprintf(todirname, "badname.obj");//"BADNAME.OBJ");
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
      sprintf(todirfullname, "%s/%d.obj", todirname, number);
    }
    else
    {
      // Character directory
      sprintf(todirfullname, "%s", todirname);
    }

    // players/twink.obj or players/twink.obj/sword.obj
    sprintf(todir, "players/%s", todirfullname);

    // modules/advent.mod/objects/advent.obj
    sprintf(fromdir, "%s", ProfileList[profile].name);

    // Delete all the old items
    if (owner == character)
    {
      tnc = 0;
      while (tnc < 8)
      {
        sprintf(tofile, "%s/%d.obj", todir, tnc); /*.OBJ*/
        fs_removeDirectoryAndContents(tofile);
        tnc++;
      }
    }

    // Make the directory
    fs_createDirectory(todir);

    // Build the DATA.TXT file
    sprintf(tofile, "%s/data.txt", todir); /*DATA.TXT*/
    export_one_character_profile(tofile, character);

    // Build the SKIN.TXT file
    sprintf(tofile, "%s/skin.txt", todir); /*SKIN.TXT*/
    export_one_character_skin(tofile, character);

    // Build the NAMING.TXT file
    sprintf(tofile, "%s/naming.txt", todir); /*NAMING.TXT*/
    export_one_character_name(tofile, character);

    // Copy all of the misc. data files
    sprintf(fromfile, "%s/message.txt", fromdir); /*MESSAGE.TXT*/
    sprintf(tofile, "%s/message.txt", todir); /*MESSAGE.TXT*/
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/tris.md2", fromdir); /*TRIS.MD2*/
    sprintf(tofile,   "%s/tris.md2", todir); /*TRIS.MD2*/
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/copy.txt", fromdir); /*COPY.TXT*/
    sprintf(tofile,   "%s/copy.txt", todir); /*COPY.TXT*/
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/script.txt", fromdir);
    sprintf(tofile,   "%s/script.txt", todir);
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/enchant.txt", fromdir);
    sprintf(tofile,   "%s/enchant.txt", todir);
    fs_copyFile(fromfile, tofile);
    sprintf(fromfile, "%s/credits.txt", fromdir);
    sprintf(tofile,   "%s/credits.txt", todir);
    fs_copyFile(fromfile, tofile);

    // Copy all of the particle files
    tnc = 0;
    while (tnc < PRTPIP_COUNT)
    {
      sprintf(fromfile, "%s/part%d.txt", fromdir, tnc);
      sprintf(tofile,   "%s/part%d.txt", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      tnc++;
    }

    // Copy all of the sound files
    for (tnc = 0; VALID_WAVE_RANGE(tnc); tnc++ )
    {
      sprintf(fromfile, "%s/sound%d.wav", fromdir, tnc);
      sprintf(tofile,   "%s/sound%d.wav", todir,   tnc);
      fs_copyFile(fromfile, tofile);
    }

    // Copy all of the image files
    tnc = 0;
    while (tnc < 4)
    {
      sprintf(fromfile, "%s/tris%d.bmp", fromdir, tnc);
      sprintf(tofile,   "%s/tris%d.bmp", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      sprintf(fromfile, "%s/icon%d.bmp", fromdir, tnc);
      sprintf(tofile,   "%s/icon%d.bmp", todir,   tnc);
      fs_copyFile(fromfile, tofile);
      tnc++;
    }
  }
}

//---------------------------------------------------------------------------------------------
void export_all_local_players(void)
{
  // ZZ> This function saves all the local players in the
  //     PLAYERS directory
  int cnt, character, item, number;

  // Check each player
  if (exportvalid)
  {
    cnt = 0;
    while (cnt<Player_List::SIZE)
    {
      if ( VALID_PLAYER(cnt) )
      {
        // Is it alive?
        character = PlaList[cnt].index;
        if ( VALID_CHR(character) && ChrList[character].alive)
        {
          // Export the character
          export_one_character(character, character, 0);

          // Export the left hand item
          item = ChrList[character].holding_which[SLOT_LEFT];
          if (VALID_CHR(item)  && ChrList[item].is_item)  export_one_character(item, character, 0);

          // Export the right hand item
          item = ChrList[character].holding_which[SLOT_RIGHT];
          if (VALID_CHR(item)  && ChrList[item].is_item)  export_one_character(item, character, 1);

          // Export the inventory
          number = 2;
          SCAN_CHR_PACK_BEGIN(ChrList[character], item, rinv_item)
          {
            if (rinv_item.is_item) export_one_character(item, character, number);
            number++;
          } SCAN_CHR_PACK_END;
        }
      }
      cnt++;
    }
  }
}

//---------------------------------------------------------------------------------------------
void quit_module(void)
{
  // ZZ> This function forces a return to the menu
  moduleactive = false;
  hostactive   = false;
  export_all_local_players();

  //Play menu music again
  play_music(0, 0, -1);
}

//--------------------------------------------------------------------------------------------
void quit_game(void)
{
  log_info("Exiting Egoboo %s the good way...\n", VERSION);
  // ZZ> This function exits the game entirely
  if (gameactive)
  {
    /* PORT
    PostMessage(hGlobalWindow, WM_CLOSE, 0, 0);
    */
    gameactive = false;
  }
  if (moduleactive)
  {
    quit_module();
  }

  //if (GMesh.floatmemory != NULL)
  //{
  //  free(GMesh.floatmemory);
  //  GMesh.floatmemory = NULL;
  //}
}

/* ORIGINAL, UNOPTIMIZED VERSION
//--------------------------------------------------------------------------------------------
void goto_colon(FILE* fileread)
{
// ZZ> This function moves a file read pointer to the next colon
char cTmp;

fscanf(fileread, "%c", &cTmp);
while(cTmp != ':')
{
if(fscanf(fileread, "%c", &cTmp)==EOF)
{
if(globalname==NULL)
{
general_error(0, 0, "NOT ENOUGH COLONS IN FILE!!!");
}
else
{
general_error(0, 0, globalname);
}
}
}
}
*/

//--------------------------------------------------------------------------------------------
void goto_colon(FILE* fileread, char * key)
{
  // ZZ> This function moves a file read pointer to the next colon
  //    char cTmp;

  if( !goto_colon_yesno(fileread,key) )
    assert(false);

}

//--------------------------------------------------------------------------------------------
Uint32 get_damage_mods(FILE * fileread)
{
  Uint32 iTmp;
  Uint8  cTmp = get_first_letter(fileread);
  switch(toupper(cTmp))
  {
  case 'T': iTmp = DAMAGE_INVERT; break;
  case 'C': iTmp = DAMAGE_CHARGE; break;
  default:  iTmp = 0; break;
  }
  return iTmp;
};

//--------------------------------------------------------------------------------------------
Uint32 get_next_damage_mods(FILE * fileread)
{
  goto_colon(fileread);
  return get_damage_mods(fileread);
};

//--------------------------------------------------------------------------------------------
bool goto_colon_yesno(FILE* fileread, char * key)
{
  // ZZ> This function moves a file read pointer to the next colon, or it returns
  //     false if there are no more
  char c, *ptmp;

  if(NULL!=key)
  {
    ptmp = key;
    while( !feof(fileread) )
    {
      c = fgetc(fileread);
      switch(c)
      {
      case 0x00:
      case 0x0A:
      case 0x0D:
        ptmp  = key;
        *ptmp = 0x00;
        break;

      case ':': *ptmp++ = 0x00; return true; break;

      default: *ptmp++ = c;
      }
    }
  }
  else
  {
    while( !feof(fileread) )
    {
      c = fgetc(fileread);
      if( c == ':' ) return true;
    }
  }

  return false;
}

//--------------------------------------------------------------------------------------------
char get_first_letter(FILE* fileread)
{
  // ZZ> This function returns the next non-whitespace character
  char cTmp;
  do
  {
    cTmp = fgetc(fileread);
  } while(isspace(cTmp));

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
  // ZZ> This function finds the next tag, returning true if it found one
  if (goto_colon_yesno(fileread))
  {
    if (numscantag < MAXTAG)
    {
      fscanf(fileread, "%s%d", tagname[numscantag], &tagvalue[numscantag]);
      numscantag++;
      return true;
    }
  }
  return false;
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
  //     It will return 0xFF if there are no matches.
  int cnt;

  cnt = 0;
  while (cnt < numscantag)
  {
    if (strcmp(string, tagname[cnt])==0)
    {
      // They match
      return tagvalue[cnt];
    }
    cnt++;
  }
  // No matches
  return 0xFF;
}

//--------------------------------------------------------------------------------------------
void Control_List::read(char *szFilename)
{
  // ZZ> This function reads the controls.txt file
  FILE* fileread;
  char currenttag[TAGSIZE];
  int cnt;

  fileread = fopen(szFilename, "r");
  if (fileread)
  {
    cnt = 0;
    while (goto_colon_yesno(fileread) && cnt < CONTROL_COUNT)
    {
      fscanf(fileread, "%s", currenttag);
      CtrlList[cnt].value = tag_value(currenttag);
      //printf("CTRL: %i, %s\n", CtrlList[cnt].value, currenttag);
      CtrlList[cnt].key = (currenttag[0] == 'K');
      cnt++;
    }
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
bool Control_List::key_is_pressed(Uint32 control)
{
  // ZZ> This function returns true if the given control is GUI.pressed...
  if (GNetMsg.mode)  return false;

  return GKeyb.pressed( CtrlList[control].value );
}

//--------------------------------------------------------------------------------------------
bool Control_List::mouse_is_pressed(Uint32 control)
{
  // ZZ> This function returns true if the given control is GUI.pressed...
  if (CtrlList[control].key)
  {
    if (GNetMsg.mode)  return false;

    GKeyb.pressed( CtrlList[control].value );
  }
  else
  {
    return (GMous.latch.button==CtrlList[control].value);
  }
  return false;
}

//--------------------------------------------------------------------------------------------
bool Control_List::joya_is_pressed(Uint32 control)
{
  // ZZ> This function returns true if the given control is GUI.pressed...
  if (CtrlList[control].key)
  {
    if (GNetMsg.mode)  return false;

    return GKeyb.pressed( CtrlList[control].value );
  }
  else
  {
    return (GJoy[0].latch.button==CtrlList[control].value);
  }

  return false;
}

//--------------------------------------------------------------------------------------------
bool Control_List::joyb_is_pressed(Uint32 control)
{
  // ZZ> This function returns true if the given control is GUI.pressed...
  if (CtrlList[control].key)
  {
    if (GNetMsg.mode)  return false;

    return GKeyb.pressed( CtrlList[control].value );
  }
  else
  {
    return (GJoy[1].latch.button==CtrlList[control].value);
  }

  return false;
}

//--------------------------------------------------------------------------------------------
bool get_bool(FILE* fileread)
{
  char cTmp = get_first_letter(fileread);
  return toupper(cTmp) == 'T';
}

//--------------------------------------------------------------------------------------------
DAMAGE_TYPE get_damage_type(FILE* fileread)
{
  char cTmp = get_first_letter(fileread);

  DAMAGE_TYPE retval = DAMAGE_SLASH;
  switch( toupper(cTmp) )
  {
  case 'N':  retval = DAMAGE_NULL;  break;
  case 'S':  retval = DAMAGE_SLASH; break;
  case 'C':  retval = DAMAGE_CRUSH; break;
  case 'P':  retval = DAMAGE_POKE;  break;
  case 'H':  retval = DAMAGE_HOLY;  break;
  case 'E':  retval = DAMAGE_EVIL;  break;
  case 'F':  retval = DAMAGE_FIRE;  break;
  case 'I':  retval = DAMAGE_ICE;   break;
  case 'Z':  retval = DAMAGE_ZAP;   break;
  };

  return retval;

}



//--------------------------------------------------------------------------------------------
IDSZ get_idsz(FILE* fileread)
{
  // ZZ> This function reads and returns an IDSZ tag, or IDSZ::NONE if there wasn't one
  IDSZ idsz;
  char buffer[5];

  char cTmp = get_first_letter(fileread);
  if (cTmp == '[')
  {
    fscanf(fileread, "%4c", &buffer);
    buffer[4] = 0x00;
    idsz = IDSZ((char *)&buffer);
  }

  return idsz;
}

//--------------------------------------------------------------------------------------------
IDSZ get_next_idsz(FILE* fileread)
{
  goto_colon(fileread);
  return get_idsz(fileread);
};

//--------------------------------------------------------------------------------------------
bool get_next_bool(FILE* fileread)
{
  goto_colon(fileread);
  return get_bool(fileread);
}

//--------------------------------------------------------------------------------------------
Sint32 get_int(FILE* fileread)
{
  Sint32 iTmp;

  fscanf(fileread, "%d", &iTmp);

  return iTmp;
}

//--------------------------------------------------------------------------------------------
Uint8 get_team(FILE* fileread)
{
  char szTmp[0x0100];

  int read = fscanf(fileread, "%s", &szTmp);
  szTmp[0xFF] = 0x00;

  if(0 == read || 0 == strlen(szTmp))
    return 'N' - 'A';
  else
    return (toupper(szTmp[0])-'A') % TEAM_COUNT;
}

//--------------------------------------------------------------------------------------------
Uint8 get_next_team(FILE* fileread)
{
  goto_colon(fileread);
  return  get_team(fileread);
}

//--------------------------------------------------------------------------------------------
float get_float(FILE* fileread)
{
  float fTmp;

  fscanf(fileread, "%f", &fTmp);

  return fTmp;
}
//--------------------------------------------------------------------------------------------
Sint32 get_next_int(FILE* fileread)
{
  goto_colon(fileread);
  return  get_int(fileread);
};

//--------------------------------------------------------------------------------------------
float get_next_float(FILE* fileread)
{
  goto_colon(fileread);
  return get_float(fileread);
};

//--------------------------------------------------------------------------------------------
void expand_escape_sequence(char * szBuffer, char format, Uint32 character)
{
  if(NULL==szBuffer) return;

  //initialize the string
  szBuffer[0] = 0;

  Uint32 lTmp;

  switch(format)
  {
  case 'n': // Name
    {
      if (ChrList[character].nameknown)
        sprintf(szBuffer, "%s", ChrList[character].name);
      else
      {
        int lTmp = ChrList[character].getCap().classname[0];
        if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
          sprintf(szBuffer, "an %s", ChrList[character].getCap().classname);
        else
          sprintf(szBuffer, "a %s", ChrList[character].getCap().classname);
      }
    }
    break;

  case 'c': // Class name
    {
      strcpy(szBuffer, ChrList[character].getCap().classname);
    }
    break;

  case 't': // Target name
    {
      Uint32 target = ChrList[character].ai.target;
      if(VALID_CHR(target))
      {
        if (ChrList[target].nameknown)
          sprintf(szBuffer, "%s", ChrList[target].name);
        else
        {
          lTmp = ChrList[target].getCap().classname[0];
          if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
            sprintf(szBuffer, "an %s", ChrList[target].getCap().classname);
          else
            sprintf(szBuffer, "a %s", ChrList[target].getCap().classname);
        }
      }
      else
      {
        strcpy(szBuffer, "Nobody");
      }
    }
    break;

  case 'o': // Owner name
    {
      Uint32 owner = ChrList[character].ai.owner;

      if(VALID_CHR(owner))
      {
        if (ChrList[ChrList[character].ai.owner].nameknown)
          sprintf(szBuffer, "%s", ChrList[owner].name);
        else
        {
          lTmp = ChrList[ChrList[character].ai.owner].getCap().classname[0];
          if (lTmp == 'A' || lTmp == 'E' || lTmp == 'I' || lTmp == 'O' || lTmp == 'U')
            sprintf(szBuffer, "an %s", ChrList[owner].getCap().classname);
          else
            sprintf(szBuffer, "a %s", ChrList[owner].getCap().classname);
        }
      }
      else
      {
        strcpy(szBuffer, "Nobody");
      }
    }
    break;

  case 's': // Target class name
    {
      Uint32 target = ChrList[character].ai.target;
      if(VALID_CHR(target))
      {
        strcpy(szBuffer,ChrList[target].getCap().classname);
      }
      else
      {
        strcpy(szBuffer, "Nobody");
      }
    }
    break;

  case '0':  // Target's skin name
  case '1':
  case '2':
  case '3':
    {
      Uint32 target = ChrList[character].ai.target;
      if(VALID_CHR(target))
      {
        strcpy( szBuffer, ChrList[target].getCap().skinname[format-'0'] );
      }
      else
      {
        strcpy(szBuffer, "Nothing");
      }
    }
    break;

  case 'd': // tmpdistance value
    {
      sprintf(szBuffer, "%d", val[TMP_DISTANCE]);
    }
    break;

  case 'x': // tmpx value
    {
      sprintf(szBuffer, "%d", val[TMP_X]);
    }
    break;

  case 'y': // tmpy value
    {
      sprintf(szBuffer, "%d", val[TMP_Y]);
    }
    break;

  case 'D': // tmpdistance value
    {
      sprintf(szBuffer, "%2d", val[TMP_DISTANCE]);
    }
    break;

  case 'X': // tmpx value
    {
      sprintf(szBuffer, "%2d", val[TMP_X]);
    }
    break;

  case 'Y': // tmpy value
    {
      sprintf(szBuffer, "%2d", val[TMP_Y]);
    }
    break;

  case 'a': // Character's ammo
    {
      if (ChrList[character].ammoknown)
        sprintf(szBuffer, "%d", ChrList[character].ammo);
      else
        sprintf(szBuffer, "?");
    }
    break;

  case 'k': // Kurse state
    {
      if (ChrList[character].iskursed)
        sprintf(szBuffer, "kursed");
      else
        sprintf(szBuffer, "unkursed");
    }
    break;

  case 'p': // Character's possessive
    {
      if (ChrList[character].gender == GENFEMALE)
      {
        sprintf(szBuffer, "her");
      }
      else
      {
        if (ChrList[character].gender == GENMALE)
        {
          sprintf(szBuffer, "his");
        }
        else
        {
          sprintf(szBuffer, "its");
        }
      }
    }
    break;

  case 'm': // Character's gender
    {
      if (ChrList[character].gender == GENFEMALE)
      {
        sprintf(szBuffer, "female ");
      }
      else
      {
        if (ChrList[character].gender == GENMALE)
        {
          sprintf(szBuffer, "male ");
        }
        else
        {
          sprintf(szBuffer, " ");
        }
      }
    }
    break;

  case 'g': // Target's possessive
    {
      Uint32 target = ChrList[character].ai.target;
      if(VALID_CHR(target))
      {
        switch(ChrList[target].gender)
        {
        case GENFEMALE: sprintf(szBuffer, "her"); break;
        case GENMALE  : sprintf(szBuffer, "his"); break;
        case GENOTHER : sprintf(szBuffer, "its"); break;
        default:
        case GENRANDOM: sprintf(szBuffer, "???s"); break;
        };
      }
      else
      {
        strcpy(szBuffer, "nothing's");
      }
    }
  }
};

void expand_message(char * write, char * read, Uint32 character)
{
  char *eread, szTmp[0x0100];

  if(NULL==read || 0==*read || NULL==write || INVALID_CHR(character)) return;

  // Copy the message
  char * last_write = write + MESSAGESIZE-1;
  for ( /*nothing*/; *read!=0x00 && write<last_write; /* nothing */)
  {
    if (*read == '%')
    {
      // Escape sequence
      expand_escape_sequence(szTmp, *(++read), character);

      // copy the format into the buffer
      for (eread = szTmp; *eread!=0x00 && write<last_write; /* nothing */)
      {
        *(write++) = *(eread++);
      }
    }
    else
    {
      // Copy the letter
      *(write++) = *(read++);
    }
  }

  *write = 0;
};

//--------------------------------------------------------------------------------------------
Uint32 display_message(int message, Uint16 character)
{
  // ZZ> This function sticks a message in the display queue and sets its timer

  Uint32 slot = GMsg.get_free();
  if(Msg::INVALID == slot) return Msg::INVALID;

  expand_message( GMsg[slot].textdisplay, &GMsg.text[GMsg.index[message]], character );

  GMsg[slot].time = MESSAGETIME;

  return slot;
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
    while (cnt < ACTION_COUNT)
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
  char szTmp[0x0100];

  fscanf(fileread, "%s", szTmp);
  cnt = 0;
  while (cnt < MAXCAPNAMESIZE-1)
  {
    cTmp = szTmp[cnt];
    if (cTmp=='_')  cTmp=' ';
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
  char lCurSectionName[0x40];
  bool lTempBool;
  Sint32 lTmpInt;
  char lTmpStr[24];

  lConfigSetup = OpenConfigFile(filename);
  if (lConfigSetup == NULL)
  {
    //Major Error
    general_error(0,0,"Could not find Setup.txt\n");
  }
  else
  {
    globalname = filename; // heu!?

    /*********************************************

    GRAPHIC Section

    *********************************************/

    strcpy(lCurSectionName, "GRAPHIC");

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "Z_REFLECTION", &zreflect) == 0)
    {
      zreflect = false; // default
    }


    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "MAX_NUMBER_VERTICES", &lTmpInt) == 0)
    {
      lTmpInt = 25; // default
    }
    //GMesh.maxtotalvertices = lTmpInt * 0x0400;


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "FULLSCREEN", &fullscreen) == 0)
    {
      fullscreen = false; // default
    }


    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "SCREENSIZE_X", &lTmpInt) == 0)
    {
      lTmpInt = 640; // default
    }
    scrx = lTmpInt;

    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "SCREENSIZE_Y", &lTmpInt) == 0)
    {
      lTmpInt = 480; // default
    }
    scry = lTmpInt;


    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "COLOR_DEPTH", &lTmpInt) == 0)
    {
      lTmpInt = 16; // default
    }
    scrd = lTmpInt;


    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "Z_DEPTH", &lTmpInt) == 0)
    {
      lTmpInt = 16; // default
    }
    scrz = lTmpInt;

    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "MAX_TEXT_MESSAGE", &lTmpInt) == 0)
    {
      lTmpInt = 1; // default
    }
    messageon = true;
    maxmessage = lTmpInt;
    if (maxmessage < 1)  { maxmessage = 1;  messageon = false; }
    if (maxmessage > MAXMESSAGE)  { maxmessage = MAXMESSAGE; }


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "STATUS_BAR", &staton) == 0)
    {
      staton = false; // default
    }
    wraptolerance = 0x20;
    if (staton)
    {
      wraptolerance = 90;
    }

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "PERSPECTIVE_CORRECT", &lTempBool) == 0)
    {
      lTempBool = false; // default
    }
    perspective = lTempBool ? GL_NICEST : GL_FASTEST;

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "DITHERING", &dither) == 0)
    {
      dither = false; // default
    }

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "FLOOR_REFLECTION_FADEOUT", &lTempBool) == 0)
    {
      lTempBool = false; // default
    }
    reffadeor = lTempBool ? 0 : 0xFF;


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "REFLECTION", &refon) == 0)
    {
      refon = false; // default
    }


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "SHADOWS", &shaon) == 0)
    {
      shaon = false; // default
    }


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "SHADOW_AS_SPRITE", &shasprite) == 0)
    {
      shasprite = true; // default
    }


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "PHONG", &phongon) == 0)
    {
      phongon = true; // default
    }

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "MULTI_LAYER_WATER", &twolayerwateron) == 0)
    {
      twolayerwateron = false; // default
    }

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "OVERLAY", &overlayvalid) == 0)
    {
      overlayvalid = false; // default
    }

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "BACKGROUND", &backgroundvalid) == 0)
    {
      backgroundvalid = false; // default
    }


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "FOG", &GFog.allowed) == 0)
    {
      GFog.allowed = false; // default
    }

    if ( GetConfigBooleanValue( lConfigSetup, lCurSectionName, "TEXTURE_FILTERING", &lTempBool ) == 0 )
    {
      lTempBool = false; // default
    }
    GLTexture::filter_min = lTempBool ? GL_LINEAR : GL_NEAREST;
    GLTexture::filter_mag = lTempBool ? GL_LINEAR : GL_NEAREST;

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "GOURAUD_SHADING", &lTempBool) == 0)
    {
      lTempBool = false; // default
    }
    shading = lTempBool ? GL_SMOOTH : GL_FLAT;

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "ANTIALIASING", &lTempBool) == 0)
    {
      lTempBool = false; // default
    }
    antialias = lTempBool ? GL_NICEST : GL_FASTEST;

    fillmode_front = GL_FILL;  // GL_POINT, GL_LINE, or GL_FILL
    fillmode_back  = GL_FILL;  // GL_POINT, GL_LINE, or GL_FILL


    /*********************************************

    SOUND Section

    *********************************************/

    strcpy(lCurSectionName, "SOUND");


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "SOUND", &soundvalid) == 0)
    {
      soundvalid = false; // default
    }

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "MUSIC", &musicvalid) == 0)
    {
      musicvalid = false; // default
    }

    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "MUSIC_VOLUME", &musicvolume) == 0)
    {
      musicvolume = 75; // default
    }

    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "SOUND_VOLUME", &soundvolume) == 0)
    {
      soundvolume = 75; // default
    }

    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "MAX_SOUND_CHANNEL", &maxsoundchannel) == 0)
    {
      maxsoundchannel = 16; // default
    }
    if (maxsoundchannel < 8) maxsoundchannel = 8;
    if (maxsoundchannel > 0x20) maxsoundchannel = 0x20;

    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "OUPUT_BUFFER_SIZE", &buffersize) == 0)
    {
      buffersize = 0x0800; // default
    }
    if (buffersize < 0x0200) buffersize = 0x0200;
    if (buffersize > 8196) buffersize = 8196;

    /*********************************************

    CONTROL Section

    *********************************************/

    strcpy(lCurSectionName, "CONTROL");

    if (GetConfigValue(lConfigSetup, lCurSectionName, "AUTOTURN_CAMERA", lTmpStr, 24) == 0)
    {
      strcpy(lTmpStr, "GOOD");   // default
    }
    switch( toupper(lTmpStr[0]) )
    {
    case 'T': cam_autoturn = 1; break;
    case 'F': cam_autoturn = 0; break;

    default:
    case 'G': cam_autoturn = 0xFF; break;
    }


    //[claforte] Force cam_autoturn to false, or else it doesn't move right.
    //cam_autoturn = false;

    /*********************************************

    NETWORK Section

    *********************************************/

    strcpy(lCurSectionName, "NETWORK");


    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "NETWORK_ON", &GNet.on) == 0)
    {
      GNet.on = false; // default
    }

    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "LAG_TOLERANCE", &lTmpInt) == 0)
    {
      lTmpInt = 2; // default
    }
    GNet.lag_tolerance = lTmpInt;

    if (GetConfigIntValue(lConfigSetup, lCurSectionName, "RTS_LAG_TOLERANCE", &lTmpInt) == 0)
    {
      lTmpInt = 25; // default
    }
    Order::lag = lTmpInt;


    if (GetConfigValue(lConfigSetup, lCurSectionName, "HOST_NAME", GNet.hostname, 0x40) == 0)
    {
      strcpy(GNet.hostname, "no host");   // default
    }

    if (GetConfigValue(lConfigSetup, lCurSectionName, "MULTIPLAYER_NAME", GNetMsg.name, 0x40) == 0)
    {
      strcpy(GNetMsg.name, "little Raoul");   // default
    }

    /*********************************************

    DEBUG Section

    *********************************************/

    strcpy(lCurSectionName, "DEBUG");

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "DISPLAY_FPS", &lTempBool) == 0)
    {
      lTempBool = true; // default
    }
    fpson = lTempBool;

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "HIDE_MOUSE", &gHideMouse) == 0)
    {
      gHideMouse = true; // default
    }

    if (GetConfigBooleanValue(lConfigSetup, lCurSectionName, "GRAB_MOUSE", &gGrabMouse) == 0)
    {
      gGrabMouse = true; // default
    }

    CloseConfigFile(lConfigSetup);
  }

  reset_graphics();

}

//---------------------------------------------------------------------------------------------
void make_lighttospek(void)
{
  // ZZ> This function makes a light table to fake directional lighting
  int cnt, tnc;
  Uint8 spek;
  float fTmp, fPow;

  // New routine
  for (cnt = 0; cnt < MAXSPEKLEVEL; cnt++)
  {
    fPow = cnt/float(MAXSPEKLEVEL);
    for (tnc = 0; tnc < 0x0100; tnc++)
    {
      fTmp = tnc/float(0x0100);
      fTmp = pow(fTmp, fPow);
      spek = CLIP(tnc*fTmp, 0, 0xFF);
      lighttospek[cnt][tnc] = spek;
    }
  }
}




//--------------------------------------------------------------------------------------------
void show_stat(Uint16 statindex)
{
  // ZZ> This function shows the more specific stats for a character
  int character, level;
  char text[0x40];
  char gender[8];

  if (statdelay == 0)
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
          sprintf(gender, "male ");
        }
        if (ChrList[character].gender == GENFEMALE)
        {
          sprintf(gender, "female ");
        }
        level = ChrList[character].experiencelevel;
        if (level == 0)
          sprintf(text, " 1st level %s%s", gender, ChrList[character].getCap().classname);
        if (level == 1)
          sprintf(text, " 2nd level %s%s", gender, ChrList[character].getCap().classname);
        if (level == 2)
          sprintf(text, " 3rd level %s%s", gender, ChrList[character].getCap().classname);
        if (level >  2)
          sprintf(text, " %dth level %s%s", level+1, gender, ChrList[character].getCap().classname);
      }
      else
      {
        sprintf(text, " Dead %s", ChrList[character].getCap().classname);
      }

      // Stats
      debug_message(text);
      debug_message(" STR:~%2d~WIS:~%2d~DEF:~%d", ChrList[character].strength>>FIXEDPOINT_BITS, ChrList[character].wisdom>>FIXEDPOINT_BITS, 0xFF-ChrList[character].defense);
      debug_message(" INT:~%2d~DEX:~%2d~EXP:~%d", ChrList[character].intelligence>>FIXEDPOINT_BITS, ChrList[character].dexterity>>FIXEDPOINT_BITS, ChrList[character].experience);
      statdelay = 10;
    }
  }
}

//--------------------------------------------------------------------------------------------
void check_stats()
{
  // ZZ> This function lets the players check character stats
  if (!GNetMsg.mode)
  {
    if (GKeyb.pressed(SDLK_1))  show_stat(0);
    if (GKeyb.pressed(SDLK_2))  show_stat(1);
    if (GKeyb.pressed(SDLK_3))  show_stat(2);
    if (GKeyb.pressed(SDLK_4))  show_stat(3);
    if (GKeyb.pressed(SDLK_5))  show_stat(4);
    if (GKeyb.pressed(SDLK_6))  show_stat(5);
    if (GKeyb.pressed(SDLK_7))  show_stat(6);
    if (GKeyb.pressed(SDLK_8))  show_stat(7);
    // !!!BAD!!!  CHEAT
    if (GKeyb.pressed(SDLK_x))
    {
      if (GKeyb.pressed(SDLK_1) && VALID_CHR(PlaList[0].index))  give_experience(PlaList[0].index, 25, XPDIRECT);
      if (GKeyb.pressed(SDLK_2) && VALID_CHR(PlaList[1].index))  give_experience(PlaList[1].index, 25, XPDIRECT);
      statdelay = 0;
    }
  }
}

void check_screenshot()
{
  if (GKeyb.pressed(SDLK_F11))
  {
    if (!dump_screenshot())
    {
      debug_message("Error writing screenshot");
    }
  }
}

bool dump_screenshot()
{
  // dumps the current screen (GL context) to a new bitmap file
  // right now it dumps it to whatever the current directory is

  // returns true if successful, false otherwise

  SDL_Surface *screen, *temp;
  Uint8 *pixels;
  char buff[100];
  int i;
  FILE *test;

  screen = SDL_GetVideoSurface();
  temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    0x0000FF, 0x00FF00, 0xFF0000, 0
#else
    0xFF0000, 0x00FF00, 0x0000FF, 0
#endif
    );

  if (temp == NULL)
    return false;

  pixels = (Uint8*)malloc(3*screen->w * screen->h);
  if (pixels == NULL)
  {
    SDL_FreeSurface(temp);
    return false;
  }

  glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

  for (i=0; i < screen->h; i++)
    memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3 * screen->w * (screen->h-i-1), screen->w * 3);
  free(pixels);

  // find the next EGO??.BMP file for writing
  i=0;
  test=NULL;

  do
  {
    if (test != NULL)
      fclose(test);

    sprintf(buff, "ego%02d.bmp",i);

    // lame way of checking if the file already exists...
    test = fopen(buff, "rb");
    i++;

  }
  while ((test != NULL) && (i < 100));

  SDL_SaveBMP(temp, buff);
  SDL_FreeSurface(temp);

  debug_message("Saved to %s", buff);

  return true;
}

//--------------------------------------------------------------------------------------------
void add_stat(Uint16 character)
{
  // ZZ> This function adds a status display to the do list
  if (numstat < MAXSTAT)
  {
    statlist[numstat] = character;
    ChrList[character].staton = true;
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

  for (cnt = 0; cnt < PlaList.count_total; cnt++)
    if ( VALID_PLAYER(cnt) )
    {
      move_to_top(PlaList[cnt].index);
    }
}

//--------------------------------------------------------------------------------------------
void move_water(float dframe)
{
  // ZZ> This function animates the water overlays
  int layer;

  for (layer = 0; layer < MAXWATERLAYER; layer++)
  {
    WaterList[layer].off.s += WaterList[layer].off_add.s * dframe;
    if (WaterList[layer].off.s> 1.0)  WaterList[layer].off.s-=1.0;
    if (WaterList[layer].off.s<-1.0)  WaterList[layer].off.s+=1.0;


    WaterList[layer].off.t += WaterList[layer].off_add.t * dframe;
    if (WaterList[layer].off.t> 1.0)  WaterList[layer].off.t-=1.0;
    if (WaterList[layer].off.t<-1.0)  WaterList[layer].off.t+=1.0;

    WaterList[layer].frame = (WaterList[layer].frame + WaterList[layer].frame_add)&WATERFRAMEAND;
  }
}

//--------------------------------------------------------------------------------------------
bool play_action(Character & chr, ACTION_TYPE action, bool actready)
{
  // ZZ> This function starts a generic action for a character

  bool retval = false;
  if (action < ACTION_COUNT && chr.getMad().actinfo[action].valid)
  {
    chr.act.next  = ACTION_DA;
    chr.act.which = action;
    chr.ani.lip   = 0;
    chr.ani.last  = chr.ani.frame;
    chr.ani.frame = chr.getMad().actinfo[chr.act.which].start;
    chr.act.ready = actready;

    retval = true;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool play_action(Uint16 character, ACTION_TYPE action, bool actready)
{
  // ZZ> This function starts a generic action for a character

  bool retval = false;
  if (VALID_CHR(character))
  {
    retval = play_action(ChrList[character], action, actready);
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool set_frame(Character & chr, Uint16 frame, Uint8 lip)
{
  // ZZ> This function sets the frame for a character explicitly...  This is used to
  //     rotate Tank turrets

  if(frame > chr.getMad().numFrames()) return false;

  chr.act.next  = ACTION_DA;
  chr.act.which = ACTION_DA;
  chr.ani.lip   = (lip<<6);
  //chr.ani.last  = chr.getMad().framestart + frame;
  chr.ani.frame = chr.getMad().framestart + frame;
  chr.act.ready = true;

  return true;
}

//--------------------------------------------------------------------------------------------
bool set_frame(Uint16 character, Uint16 frame, Uint8 lip)
{
  // ZZ> This function sets the frame for a character explicitly...  This is used to
  //     rotate Tank turrets

  bool retval = false;
  if ( VALID_CHR(character) )
  {
    retval = set_frame(ChrList[character], frame, lip);
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
void reset_character_alpha(Uint16 character)
{
  // ZZ> This function fixes an item's transparency
  Uint16 enchant, mount;

  if (VALID_CHR(character))
  {
    mount = ChrList[character].held_by;
    if (VALID_CHR(mount) && ChrList[character].is_item && ChrList[mount].transferblend)
    {
      // Okay, reset transparency
      SCAN_ENC_BEGIN(ChrList[character], enchant, renc)
      {
        unset_enchant_value(enchant, SETALPHABLEND);
        unset_enchant_value(enchant, SETLIGHTBLEND);
      } SCAN_ENC_END;

      ChrList[character].alpha = ChrList[character].getCap().alpha;
      ChrList[character].light = ChrList[character].getCap().light;
      SCAN_ENC_BEGIN(ChrList[character], enchant, renc)
      {
        set_enchant_value(enchant, SETALPHABLEND, renc.eve_prof);
        set_enchant_value(enchant, SETLIGHTBLEND, renc.eve_prof);
      } SCAN_ENC_END;
    }
  }
}

//--------------------------------------------------------------------------------------------
int generate_number(int numbase, int numrand)
{
  int tmp;

  // ZZ> This function generates a random number
  if (numrand<=0)
  {
    general_error(numbase, numrand, "ONE OF THE DATA PAIRS IS WRONG");
  }
  tmp=(rand()%numrand)+numbase;
  return (tmp);
}

//--------------------------------------------------------------------------------------------
void drop_money(Uint16 character, Uint16 money)
{
  // ZZ> This function drops some of a character's money
  Uint16 huns, tfives, fives, ones, cnt;

  money = MIN(money, ChrList[character].money);

  if (money>0 && ChrList[character].pos.z > -2)
  {
    ChrList[character].money -= money;
    huns   = money/100;  money %= 100;
    tfives = money/25;   money %= 25;
    fives  = money/5;    money %= 5;
    ones   = money;

    for (cnt = 0; cnt < ones; cnt++)
      spawn_one_particle(ChrList[character].pos, 0, ChrList[character].vel, 0, Profile_List::INVALID, PRTPIP_COIN_001, Character_List::INVALID, GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, cnt, Character_List::INVALID);

    for (cnt = 0; cnt < fives; cnt++)
      spawn_one_particle(ChrList[character].pos, 0, ChrList[character].vel, 0, Profile_List::INVALID, PRTPIP_COIN_005, Character_List::INVALID, GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, cnt, Character_List::INVALID);

    for (cnt = 0; cnt < tfives; cnt++)
      spawn_one_particle(ChrList[character].pos, 0, ChrList[character].vel, 0, Profile_List::INVALID, PRTPIP_COIN_025, Character_List::INVALID, GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, cnt, Character_List::INVALID);

    for (cnt = 0; cnt < huns; cnt++)
      spawn_one_particle(ChrList[character].pos, 0, ChrList[character].vel, 0, Profile_List::INVALID, PRTPIP_COIN_100, Character_List::INVALID, GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, cnt, Character_List::INVALID);

    ChrList[character].damagetime = INVICTUS_DAMAGETIME;  // So it doesn't grab it again
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

  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if (cnt!=character && !TeamList[rchr_cnt.team].hatesteam[team])
    {
      rchr_cnt.ai.alert |= ALERT_IF_CALLEDFORHELP;
    }
  } SCAN_CHR_END;
}

//--------------------------------------------------------------------------------------------
void give_experience(int character, int amount, Uint8 xptype)
{
  // ZZ> This function gives a character experience, and pawns off level gains to
  //     another function
  int newamount;
  Uint8 curlevel;
  int number;
  int profile;

  Character & rchr = ChrList[character];
  Cap       & rcap = rchr.getCap();

  if (!rchr.invictus)
  {
    // Figure out how much experience to give
    profile = rchr.model;
    CAP_REF cap_ref = ProfileList[profile].cap_ref;
    newamount = amount;
    if (xptype < XP_COUNT)
    {
      newamount = amount*rcap.experiencerate[xptype];
    }
    newamount+=rchr.experience;
    if (newamount > MAXXP)  newamount = MAXXP;
    rchr.experience=newamount;

    // Do level ups and stat changes
    curlevel = rchr.experiencelevel;
    if (curlevel < MAXLEVEL-1)
    {
      if (rchr.experience >= rcap.experienceforlevel[curlevel+1])
      {
        // The character is ready to advance...
        if (rchr.isplayer)
        {
          debug_message("%s gained a level!!!", rchr.name);
          sound = Mix_LoadWAV("basicdat/lvlup.wav");
          Mix_PlayChannel(-1, sound, 0);
        }
        rchr.experiencelevel++;
        rchr.experience = rcap.experienceforlevel[curlevel+1];
        // Size
        rchr.scale_goto+=rcap.sizeperlevel;  // Limit this?
        rchr.sizegototime = SIZETIME;
        // Strength
        number = generate_number(rcap.strengthperlevelbase, rcap.strengthperlevelrand);
        number = number+rchr.strength;
        if (number > PERFECTSTAT) number = PERFECTSTAT;
        rchr.strength = number;
        // Wisdom
        number = generate_number(rcap.wisdomperlevelbase, rcap.wisdomperlevelrand);
        number = number+rchr.wisdom;
        if (number > PERFECTSTAT) number = PERFECTSTAT;
        rchr.wisdom = number;
        // Intelligence
        number = generate_number(rcap.intelligenceperlevelbase, rcap.intelligenceperlevelrand);
        number = number+rchr.intelligence;
        if (number > PERFECTSTAT) number = PERFECTSTAT;
        rchr.intelligence = number;
        // Dexterity
        number = generate_number(rcap.dexterityperlevelbase, rcap.dexterityperlevelrand);
        number = number+rchr.dexterity;
        if (number > PERFECTSTAT) number = PERFECTSTAT;
        rchr.dexterity = number;
        // Life
        number = generate_number(rcap.lifeperlevelbase, rcap.lifeperlevelrand);
        number = number+rchr.lifemax;
        if (number > PERFECTBIG) number = PERFECTBIG;
        rchr.life+=(number-rchr.lifemax);
        rchr.lifemax = number;
        // Mana
        number = generate_number(rcap.manaperlevelbase, rcap.manaperlevelrand);
        number = number+rchr.manamax;
        if (number > PERFECTBIG) number = PERFECTBIG;
        rchr.mana+=(number-rchr.manamax);
        rchr.manamax = number;
        // Mana Return
        number = generate_number(rcap.manareturnperlevelbase, rcap.manareturnperlevelrand);
        number = number+rchr.manareturn;
        if (number > PERFECTSTAT) number = PERFECTSTAT;
        rchr.manareturn = number;
        // Mana Flow
        number = generate_number(rcap.manaflowperlevelbase, rcap.manaflowperlevelrand);
        number = number+rchr.manaflow;
        if (number > PERFECTSTAT) number = PERFECTSTAT;
        rchr.manaflow = number;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void give_team_experience(Uint8 team, int amount, Uint8 xptype)
{
  // ZZ> This function gives a character experience, and pawns off level gains to
  //     another function
  int cnt;

  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if (rchr_cnt.team == team)
    {
      give_experience(cnt, amount, xptype);
    }
  } SCAN_CHR_END;
}

//--------------------------------------------------------------------------------------------
void Team_List::setup_alliances(char *modname)
{
  // ZZ> This function reads the alliance file
  char newloadname[0x0100];
  Uint8 teama, teamb;
  FILE *fileread;

  // Load the file
  make_newloadname(modname, "gamedat/alliance.txt", newloadname);
  fileread = fopen(newloadname, "r");
  if (fileread)
  {
    while (goto_colon_yesno(fileread))
    {
      teama = get_team(fileread);
      teamb = get_team(fileread);
      TeamList[teama].hatesteam[teamb] = false;
    }
    fclose(fileread);
  }
}


//--------------------------------------------------------------------------------------------
void clear_orders()
{
  // ZZ> This function clears the order list
  int cnt;

  cnt = 0;
  while (cnt < MAXORDER)
  {
    GOrder[cnt].valid = false;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
Uint16 get_empty_order()
{
  // ZZ> This function looks for an unused order
  int cnt;

  cnt = 0;
  while (cnt < MAXORDER)
  {
    // Find an empty slot
    if (!GOrder[cnt].valid)
    {
      GOrder[cnt].valid = true;
      return cnt;
    }
    cnt++;
  }
  return MAXORDER;
}

//--------------------------------------------------------------------------------------------
void begin_integration()
{
  int cnt;

  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    rchr_cnt.begin_integration();
    rchr_cnt.calculate_bumpers();
    Platform::do_attachment(rchr_cnt);
  } SCAN_CHR_END;

  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    rprt_cnt.begin_integration();
  } SCAN_PRT_END;
}

//--------------------------------------------------------------------------------------------
void end_integration(float dframe)
{
  int cnt;

  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    rchr_cnt.update_old(GMesh);
    rchr_cnt.end_integration(dframe);
  } SCAN_CHR_END;

  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    rprt_cnt.update_old(GMesh);
    rprt_cnt.end_integration(dframe);
  } SCAN_PRT_END;
}

//--------------------------------------------------------------------------------------------
void calculate_attached_accelerations()
{
  int cnt;

  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if(INVALID_CHR(rchr_cnt.held_by)) continue;

    // transfer non-trivial forces to the holders
    if( dist_abs(rchr_cnt.apos) + ABS(rchr_cnt.apos_lr) + 
      dist_abs(rchr_cnt.avel) + ABS(rchr_cnt.avel_lr) + 
      dist_abs(rchr_cnt.aacc) + ABS(rchr_cnt.aacc_lr) > 0.1)
    {

      propagate_forces(ChrList[rchr_cnt.held_by], rchr_cnt.getAccumulator());
    };

    rchr_cnt.getAccumulator().clear();

    // calculate the acceleration, etc. from the change from the last frame
    rchr_cnt.vel = rchr_cnt.pos - rchr_cnt.old.pos;
    rchr_cnt.acc = rchr_cnt.vel - rchr_cnt.old.vel;
  } SCAN_CHR_END;

  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    if(INVALID_CHR(rprt_cnt.attachedtocharacter)) continue;

    // transfer non-trivial forces to the holders
    if( dist_abs(rprt_cnt.apos) + ABS(rprt_cnt.apos_lr) + 
      dist_abs(rprt_cnt.avel) + ABS(rprt_cnt.avel_lr) + 
      dist_abs(rprt_cnt.aacc) + ABS(rprt_cnt.aacc_lr) > 0.1)
    {

      propagate_forces(ChrList[rprt_cnt.attachedtocharacter], rprt_cnt.getAccumulator());
    };

    rprt_cnt.getAccumulator().clear();

    // calculate the acceleration, etc. from the change from the last frame
    rprt_cnt.vel = rprt_cnt.pos - rprt_cnt.old.pos;
    rprt_cnt.acc = rprt_cnt.vel - rprt_cnt.old.vel;
  } SCAN_PRT_END;

};


//--------------------------------------------------------------------------------------------
void do_object_motion(float deltaTime)
{

  float dframe = CLIP((deltaTime*1000.0f) / FRAMESKIP, .2, 2);

  Physics_Info loc_phys;
  loc_phys.fric_air   = powf(GPhys.fric_air,   dframe);
  loc_phys.fric_h2o   = powf(GPhys.fric_h2o,   dframe);
  loc_phys.fric_plat  = powf(GPhys.fric_plat,  dframe);
  loc_phys.fric_slip  = powf(GPhys.fric_slip,  dframe);
  loc_phys.fric_stick = powf(GPhys.fric_stick, dframe);
  loc_phys.fric_hill  = powf(GPhys.fric_hill,  dframe);
  loc_phys.gravity    = GPhys.gravity;

  // do some mesh interaction
  make_onwhichfan();

  // start the integration
  begin_integration();
  {
    // set the AI latches, etc.
    let_ai_think();

    // update the character positions from last frame and accumulate the forces, etc.
    make_character_matrices();
    move_characters(loc_phys, dframe);

    // attach the particles from last frame and  and accumulate the forces, etc.
    attach_particles();
    move_particles(loc_phys, dframe);
    
    // propagate the forces from held items to their mounts and calculate the correct
    // velocities and accelerations for held items
    calculate_attached_accelerations();

    // do the object-object interaction
    bump_all(loc_phys, dframe);
  }

  // finish off the integration
  end_integration(dframe);
};

//--------------------------------------------------------------------------------------------
void update_game(float deltaTime)
{
  // ZZ> This function does several iterations of character movements and such
  //     to keep the game in sync.
  int cnt, numdead;

  bool tmpseeinvisible = false, tmpseekurse = false;

  // Check for all local players being dead
  alllocalpladead   = false;
  localseeinvisible = false;
  localseekurse     = false;

  numdead = 0;
  for (cnt = 0; cnt<Player_List::SIZE; cnt++)
  {
    if ( INVALID_PLAYER(cnt) ) continue;

    int character = PlaList[cnt].index;
    if( INVALID_CHR(character) ) continue;

    Character & rchr = ChrList[character];

    if (!rchr.alive)
    {
      numdead++;
      if (GKeyb.pressed(SDLK_SPACE) && respawn_mode)
      {
        respawn_character(character);
        rchr.experience *= EXPKEEP; //Apply xp Penality
      }
    }

    tmpseeinvisible = tmpseeinvisible || rchr.canseeinvisible;
    tmpseekurse     = tmpseekurse    || rchr.canseekurse;
  }

  if (numdead >= PlaList.count_local)
  {
    alllocalpladead   = true;
    localseeinvisible = tmpseeinvisible;
    localseekurse     = tmpseekurse;
  }

  // This is the main game loop
  GMsg.timechange = 0;

  //while (wldclock < allclock && (PlaList.count_times > 0 || rts_mode))
  //while (wldclock < allclock && (PlaList.count_times > 0 || GRTS.on))
  //{
  // Important stuff to keep in sync
  srand(randsave);
  resize_characters();
  //keep_weapons_with_holders();
  do_weather_spawn();
  do_enchant_spawn();

  do_object_motion(deltaTime);


  stat_return();
  pit_kill();
  chug_orders();

  // Generate the new seed
  randsave += *((Uint32*) &md2normals[wldframe&0x7F][0]);
  randsave += *((Uint32*) &md2normals[randsave&0x7F][1]);

  // Stuff for which sync doesn't matter
  flash_select();
  animate_tiles();
  move_water();

  // Timers
  wldclock+=FRAMESKIP;
  wldframe++;
  GMsg.timechange++;
  if (statdelay > 0)  statdelay--;
  statclock++;
  //}

  if (!GRTS.on)
  {
    if (PlaList.count_times == 0)
    {
      // The remote ran out of messages, and is now twiddling its thumbs...
      // Make it go slower so it doesn't happen again
      wldclock+=5;
    }
    if (PlaList.count_times > 3 && !hostactive)
    {
      // The host has too many messages, and is probably experiencing control
      // lag...  Speed it up so it gets closer to sync
      wldclock-=5;
    }
  }
}

//--------------------------------------------------------------------------------------------
void update_timers()
{
  // ZZ> This function updates the game timers
  lstclock = allclock;
  allclock = SDL_GetTicks()-sttclock;
  fpsclock+=allclock-lstclock;
  if (fpsclock >= CLOCKS_PER_SEC)
  {
    create_szfpstext(fpsframe);
    fpsclock = 0;
    fpsframe = 0;
  }
}

//--------------------------------------------------------------------------------------------
void read_pair(FILE* fileread, Uint16 & pbase, Uint16 & prand)
{
  // ZZ> This function reads a damage/stat pair ( eg. 5-9 )
  char cTmp;
  float  fBase, fRand;

  fscanf(fileread, "%f", &fBase);  // The first number
  pbase = fBase*0x0100;

  cTmp = get_first_letter(fileread);  // The hyphen
  if (cTmp!='-')
  {
    // Not in correct format, so fail
    prand = 1;
    return;
  }

  fscanf(fileread, "%f", &fRand);  // The second number
  prand = fRand*0x0100;
  prand = prand-pbase;
  if (prand<1)
    prand = 1;
}

//--------------------------------------------------------------------------------------------
void read_next_pair(FILE* fileread, Uint16 & pbase, Uint16 & prand)
{
  goto_colon(fileread);
  read_pair(fileread, pbase, prand);
};

//--------------------------------------------------------------------------------------------
void undo_pair(int base, int rand, float & from, float & to)
{
  // ZZ> This function generates a damage/stat pair ( eg. 3-6.5 )
  //     from the base and random values.  It set pairfrom and
  //     pairto
  from = base/float(0x0100);
  to   = rand/float(0x0100);
  if (from < 0.0) from = 0.0;
  if (to   < 0.0) to   = 0.0;
  to += from;
}

//--------------------------------------------------------------------------------------------
void ftruthf(FILE* filewrite, char* text, Uint8 truth)
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     true false statements

  fprintf(filewrite, text);
  if (truth)
  {
    fprintf(filewrite, "true\n");
  }
  else
  {
    fprintf(filewrite, "false\n");
  }
}

//--------------------------------------------------------------------------------------------
void fdamagf(FILE* filewrite, char* text, Uint8 damagetype)
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
  fprintf(filewrite, text);
  if (damagetype == DAMAGE_SLASH)
    fprintf(filewrite, "SLASH\n");
  if (damagetype == DAMAGE_CRUSH)
    fprintf(filewrite, "CRUSH\n");
  if (damagetype == DAMAGE_POKE)
    fprintf(filewrite, "POKE\n");
  if (damagetype == DAMAGE_HOLY)
    fprintf(filewrite, "HOLY\n");
  if (damagetype == DAMAGE_EVIL)
    fprintf(filewrite, "EVIL\n");
  if (damagetype == DAMAGE_FIRE)
    fprintf(filewrite, "FIRE\n");
  if (damagetype == DAMAGE_ICE)
    fprintf(filewrite, "ICE\n");
  if (damagetype == DAMAGE_ZAP)
    fprintf(filewrite, "ZAP\n");
  if (damagetype == DAMAGE_NULL)
    fprintf(filewrite, "NONE\n");
}

//--------------------------------------------------------------------------------------------
void factiof(FILE* filewrite, char* text, Uint8 action)
{
  // ZZ> This function kinda mimics fprintf for the output of
  //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements
  fprintf(filewrite, text);
  if (action == ACTION_DA)
    fprintf(filewrite, "WALK\n");
  if (action == ACTION_UA)
    fprintf(filewrite, "UNARMED\n");
  if (action == ACTION_TA)
    fprintf(filewrite, "THRUST\n");
  if (action == ACTION_SA)
    fprintf(filewrite, "SLASH\n");
  if (action == ACTION_CA)
    fprintf(filewrite, "CHOP\n");
  if (action == ACTION_BA)
    fprintf(filewrite, "BASH\n");
  if (action == ACTION_LA)
    fprintf(filewrite, "LONGBOW\n");
  if (action == ACTION_XA)
    fprintf(filewrite, "XBOW\n");
  if (action == ACTION_FA)
    fprintf(filewrite, "FLING\n");
  if (action == ACTION_PA)
    fprintf(filewrite, "PARRY\n");
  if (action == ACTION_ZA)
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
  float from, to;
  undo_pair(base, rand, from, to);
  fprintf(filewrite, text);
  fprintf(filewrite, "%4.2f-%4.2f\n", from, to);
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
  char szTmp[0x0100];

  if (GMsg.total<MAXTOTALMESSAGE)
  {
    if (GMsg.totalindex>=MESSAGEBUFFERSIZE)
    {
      GMsg.totalindex = MESSAGEBUFFERSIZE-1;
    }
    GMsg.index[GMsg.total]=GMsg.totalindex;
    fscanf(fileread, "%s", szTmp);
    szTmp[0xFF] = 0;
    cTmp = szTmp[0];
    cnt = 1;
    while (cTmp!=0 && GMsg.totalindex<MESSAGEBUFFERSIZE-1)
    {
      if (cTmp=='_')  cTmp=' ';
      GMsg.text[GMsg.totalindex] = cTmp;
      GMsg.totalindex++;
      cTmp = szTmp[cnt];
      cnt++;
    }
    GMsg.text[GMsg.totalindex]=0;  GMsg.totalindex++;
    GMsg.total++;
  }
}


//--------------------------------------------------------------------------------------------
void reset_teams()
{
  // ZZ> This function makes everyone hate everyone else
  int teama, teamb;

  teama = 0;
  while (teama < TEAM_COUNT)
  {
    // Make the team hate everyone
    teamb = 0;
    while (teamb < TEAM_COUNT)
    {
      TeamList[teama].hatesteam[teamb] = true;
      teamb++;
    }
    // Make the team like itself
    TeamList[teama].hatesteam[teama] = false;
    // Set defaults
    TeamList[teama].leader = NOLEADER;
    TeamList[teama].sissy = 0;
    TeamList[teama].morale = 0;
    teama++;
  }

  // Keep the null team neutral
  teama = 0;
  while (teama < TEAM_COUNT)
  {
    TeamList[teama].hatesteam[TEAM_NEUTRAL] = false;
    TeamList[TEAM_NEUTRAL].hatesteam[teama] = false;
    teama++;
  }
}

//--------------------------------------------------------------------------------------------
void reset_messages()
{
  // ZZ> This makes messages safe to use
  int cnt;

  GMsg.total=0;
  GMsg.totalindex=0;
  GMsg.timechange=0;
  GMsg.start=0;
  cnt = 0;
  while (cnt < MAXMESSAGE)
  {
    GMsg.time[cnt] = 0;
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
void make_randie()
{
  // ZZ> This function makes the random number table
  int tnc, cnt;

  // Fill in the basic values
  cnt = 0;
  while (cnt < MAXRAND)
  {
    randie[cnt] = rand()<<1;
    cnt++;
  }

  // Keep adjusting those values
  tnc = 0;
  while (tnc < 20)
  {
    cnt = 0;
    while (cnt < MAXRAND)
    {
      randie[cnt] += rand();
      cnt++;
    }
    tnc++;
  }

  // All done
  randindex = 0;
}

//--------------------------------------------------------------------------------------------
void reset_timers()
{
  // ZZ> This function resets the timers...
  sttclock = SDL_GetTicks();
  allclock = 0;
  lstclock = 0;
  wldclock = 0;
  statclock = 0;
  pitclock = 0;  pitskill = false;
  wldframe = 0;
  allframe = 0;
  fpsframe = 0;
  outofsync = false;
}

void menu_loadInitialMenu();


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Machine_Root * root_machine = NULL;
JF::Scheduler  root_scheduler;

int SDL_main(int argc, char **argv)
{
  double timeLast = 0, deltaTime = 0;

  sdlinit(argc,argv);
  glinit(argc,argv);

  sys_initialize();
  GClock.init();

  root_machine = new Machine_Root(&root_scheduler);
  root_scheduler.addTask(root_machine);

  // add the input handling Task
  root_scheduler.addTask(new Machine_Input(&root_scheduler));

  // add the network handling Task
  root_scheduler.addTask(new Machine_Net(&root_scheduler));

  // add the UI handling Task
  root_scheduler.addTask( &GUI );

  while( root_scheduler.run() );

  return 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void Machine_Root::Begin(float deltaTime)
{
  // ZZ> This is where the program starts and all the high level stuff happens

  // Initialize logging first, so that we can use it everywhere.
  log_init();
  log_setLoggingLevel(2);

  // start initializing the various subsystems
  log_message("Starting Egoboo %s...\n", VERSION);

  fs_init();

  read_setup("setup.txt");
  set_video_options();

  read_all_tags("basicdat/scancode.txt");
  CtrlList.read("controls.txt");
  load_ai_codes("basicdat/aicodes.txt");
  ScrList.reset_ai_script();
  load_action_names("basicdat/actions.txt");

  UI::initialize(&GUI, "basicdat/ge112__r.ttf", 24);

  //Initialize sound
  if (musicvalid || soundvalid)
  {
    log_info("Initializing SDL_mixer audio services...\n");
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize) != 0)
    {
      log_error("Unable to initialize audio: %s\n", Mix_GetError());
    }
    Mix_VolumeMusic(musicvolume); //*1.28
    Mix_AllocateChannels(maxsoundchannel);
  }

  //Load the menu music, BAD: Load manually here
  music = Mix_LoadMUS("basicdat/music/themesong.ogg");
  Mix_PlayMusic(music, -1);

  //if (!get_mesh_memory())
  //{
  //  fprintf(stderr,"Reduce the maximum number of vertices!!!  See SETUP.TXT");
  //  state = SM_Leaving;
  //  return;
  //}

  GCamera.set_up_matrices(FOV);
  //GCamera.set_up_matrices();

  IconList.prime_icons();
  prime_titleimage();
  load_blip_bitmap();

  Menu::initialize();

  // Let the normal OS mouse cursor work
  SDL_WM_GrabInput(SDL_GRAB_OFF);
  SDL_ShowCursor(1);

  // Network's temporarily disabled
  gameactive = true;
  menuactive = true;

  state = SM_Entering;
}

//--------------------------------------------------------------------------------------------
void Machine_Root::Run(float deltaTime)
{
  if (menuactive)
  {
    //starts the menu and suspends this machine
    // will only return when all menus are finished
    root_scheduler.addTask(new Menu_Main(UI::new_ID(),this) , this);
  }

  //When we're Completely done with the menu, we want to exit the game
  state = SM_Leaving;
}

//--------------------------------------------------------------------------------------------
void Machine_Root::Finish(float deltaTime)
{
  quit_game();
  Mix_CloseAudio();
  //release_menu_trim();
  release_grfx();
  UI::shutdown(&GUI);
  net_shutDown();
  GClock.shutdown();
  sys_shutdown();

  //we're finished with this machine, wake the parent
  state = SM_Unknown;
}

//--------------------------------------------------------------------------------------------
void Machine_Net::Begin(float deltaTime)
{
  GNetMsg.mode = false;
  GNetMsg.delay = 20;

  net_initialize();

  state = SM_Entering;
};

//--------------------------------------------------------------------------------------------
void Machine_Net::Run(float deltaTime)
{
  if(!GNet.on)
  {
    state = SM_Finish;
    return;
  };

  input_net_message();
  sv_talkToRemotes();

  if ( !GKeyb.pressed(SDLK_F8) )
  {
    listen_for_packets();
    if (!waitingforplayers) cl_talkToHost();
  }

};

//--------------------------------------------------------------------------------------------
void Machine_Net::Finish(float deltaTime)
{
  close_session();

  state = SM_Unknown;
};



//--------------------------------------------------------------------------------------------
void Machine_Game::Begin(float deltaTime)
{
  //printf("MENU: game is now active\n");
  // Start a new module
  seed = time(NULL);
  srand(seed);
  clear_select();

  net_logf("SDL_main: Loading module %s...\n", pickedmodule);
  load_module(GCamera, GMesh, pickedmodule);  // :TODO: Seems to be the next part to fix
  GUI.mousePressed = false;
  make_onwhichfan();
  GCamera.reset(PlaList, ChrList, GMesh, true);
  reset_timers();
  figure_out_what_to_draw();
  make_character_matrices();
  attach_particles();

  // Let the game go
  net_sayHello();
  moduleactive = true;
  randsave = 0;
  srand(0);
  //printf("moduleactive: %d\n", moduleactive);

  menuactive = false;
  state = SM_Entering;
};

//--------------------------------------------------------------------------------------------
void Machine_Game::Run(float deltaTime)
{
  // This is the control loop

  // Do important things
  if ( !GKeyb.pressed(SDLK_F8) )
  {
    check_screenshot();
    check_stats();

    //handle the local and remote game latches
    read_local_latches();
    //buffer_player_latches();
    unbuffer_player_latches();

    update_timers();

    if (!waitingforplayers)
    {
      update_game(deltaTime);
    }
    else
    {
      wldclock = allclock;
    }
  }
  else
  {
    update_timers();
    wldclock = allclock;
  }

  // Do the display stuff
  GCamera.move(PlaList, ChrList, GMesh, deltaTime);
  figure_out_what_to_draw();
  draw_main(GCamera);

  // Check for quitters
  // :TODO: nolocalplayers is not set correctly
  if (GKeyb.pressed(SDLK_ESCAPE) /* || nolocalplayers*/)
  {
    quit_module();
    gameactive = false;

    // Let the normal OS mouse cursor work
    SDL_WM_GrabInput(SDL_GRAB_OFF);
    SDL_ShowCursor(1);

    state = SM_Leaving;
    return;
  }

  if (!gameactive)
    state = SM_Leaving;
};

//--------------------------------------------------------------------------------------------
void Machine_Game::Finish(float deltaTime)
{
  release_module();

  state = SM_Unknown;
};



//--------------------------------------------------------------------------------------------
void bump_all(Physics_Info & loc_phys, float dframe)
{
  // ZZ> This function sets handles characters hitting other characters or particles
  Uint16 character, particle, entry, direction;
  Uint32 chara, charb, fanblock;
  IDSZ id_prt_parent, id_prt_type, id_chr_vulnerability, id_eve_remove;
  Sint8 hide;
  int cnt, tnc, chrinblock, prtinblock, enchant, temp;
  vec3_t apos, bpos;
  Uint16 facing;

  // Clear the lists
  BumpList.clear();

  // Fill 'em back up
  SCAN_CHR_BEGIN(character, rchr_chr)
  {
    if(rchr_chr.is_inpack) continue;

    rchr_chr.bump_next = Character_List::INVALID;

    // is item hidden ?
    hide = rchr_chr.getCap().hide_state;
    if (hide != NOHIDE && hide == rchr_chr.ai.state) continue;

    // can it bump anything?
    //if(rchr_chr.bump_height ==0) continue;
    {
      //rchr_chr.holding_weight = 0;
      fanblock = GMesh.getFanblockPos(rchr_chr.pos);

      if(Mesh::INVALID_INDEX!=fanblock)
      {
        // Insert before any other characters on the block
        entry = BumpList[fanblock].chr;
        rchr_chr.bump_next = entry;
        BumpList[fanblock].chr = character;
        BumpList[fanblock].chr_count++;
      };
    }
  } SCAN_CHR_END;

  SCAN_PRT_BEGIN(particle, rprt_prt)
  {
    if(0==rprt_prt.bump_size) continue;

    rprt_prt.bump_next = Particle_List::INVALID;

    fanblock = GMesh.getFanblockPos(rprt_prt.pos);
    if(Mesh::INVALID_INDEX!=fanblock)
    {
      // Insert before any other particles on the block
      entry = BumpList[fanblock].prt;
      rprt_prt.bump_next = entry;
      BumpList[fanblock].prt = particle;
      BumpList[fanblock].prt_count++;
    };
  } SCAN_PRT_END;

  // Check collisions with other characters and bump particles
  // Only check each pair once
  for (fanblock = 0; fanblock < BumpList.count; fanblock++)
  {
    chara = BumpList[fanblock].chr;

    if( INVALID_CHR(chara) ) continue;

    chrinblock = BumpList[fanblock].chr_count;
    prtinblock = BumpList[fanblock].prt_count;

    for (cnt = 0; cnt < chrinblock && VALID_CHR(chara); cnt++, chara = ChrList[chara].bump_next)
    {
      Character & rchra = ChrList[chara];

      id_chr_vulnerability = rchra.getCap().idsz[IDSZ_VULNERABILITY];

      // Don't let items bump
      //if (rchra.bump_height) // !rchra.is_item )
      {
        charb = rchra.bump_next;  // Don't collide with self
        if( INVALID_CHR(charb) ) continue;

        for (tnc = cnt+1; tnc < chrinblock && VALID_CHR(charb); tnc++, charb = ChrList[charb].bump_next)
        {
          Character & rchrb = ChrList[charb];

          // don't interact with the items that you are holding
          if(rchra.held_by == charb || rchrb.held_by == chara) continue;

          // calculate the displacement between the centers of the two objects
          vec3_t diff = rchra.pos - rchrb.pos;
          diff.z += rchra.calc_bump_height/2 - rchrb.calc_bump_height/2;

          // calculate the overlap of the two bounding boxes
          float maxbump = MAX(ABS(rchra.calc_bump_size), ABS(rchrb.calc_bump_size));
          float minbump = MIN(ABS(rchra.calc_bump_size), ABS(rchrb.calc_bump_size));
          float overlap_x  = (maxbump + minbump) - ABS(diff.x);
          float overlap_y  = (maxbump + minbump) - ABS(diff.y);

          float maxbump_big = MAX(ABS(rchra.calc_bump_size_big), ABS(rchrb.calc_bump_size_big));
          float minbump_big = MIN(ABS(rchra.calc_bump_size_big), ABS(rchrb.calc_bump_size_big));
          float overlap_xy  = (maxbump + minbump_big) - ABS(diff.x + diff.y);
          float overlap_yx  = (maxbump + minbump_big) - ABS(diff.x - diff.y);

          float maxheight = MAX(rchra.calc_bump_height, rchrb.calc_bump_height);
          float minheight = MIN(rchra.calc_bump_height, rchrb.calc_bump_height);
          float overlap_z = (maxheight + minheight) - ABS(diff.z);

          overlap_x  = CLIP(overlap_x,  0, minbump);
          overlap_y  = CLIP(overlap_y,  0, minbump);
          overlap_xy = CLIP(overlap_xy, 0, minbump_big);
          overlap_yx = CLIP(overlap_yx, 0, minbump_big);
          overlap_z  = CLIP(overlap_z,  0, minheight);

          float depth_x  = overlap_x  / minbump;
          float depth_y  = overlap_y  / minbump;
          float depth_xy = overlap_xy / minbump_big;
          float depth_yx = overlap_yx / minbump_big;
          float depth_z  = overlap_z  / minheight; 

          bool far_horiz   = (depth_x>-1 && depth_y>-1); //|| (depth_xy>-1 && depth_yx>-1);
          bool near_horiz  = (depth_x>0  && depth_y>0 ); //|| (depth_xy>0  && depth_yx>0 );
          bool close_horiz = (depth_x==1 && depth_y==1); //|| (depth_xy==1 && depth_yx==1);

          bool far_vert    = depth_z>-1;
          bool near_vert   = depth_z> 0;
          bool close_vert  = depth_z==1;

          // check to see if the characters stepped off a platform
          bool moving_away = diff.x*(rchra.vel.x-rchrb.vel.x) + diff.y*(rchra.vel.y-rchrb.vel.y) > 0;
          if( rchra.on_which_platform==charb && !(far_vert && close_horiz) && moving_away )
          {
            rchra.request_detachment();
          }
          else if (rchrb.on_which_platform==chara && !(far_vert && close_horiz) && moving_away )
          {
            rchrb.request_detachment();
          }

          if ( (far_vert && close_horiz) && (rchra.vel.z-rchrb.vel.z)<0 && rchra.pos.z > rchrb.pos.z + rchrb.calc_bump_height - PLATTOLERANCE)
          {
            //check to see if the characters are landing on top of a platform
            // A is landing on B
            if (rchrb.is_mount)
            {
              rchra.request_detachment();
              attach_character_to_mount(chara, charb, GRIP_SADDLE);
            } 
            else if ( INVALID_CHR(rchra.held_by) && rchra.flyheight==0)
            {
              //remove chra from the old platform and attach it to the new
              rchra.request_detachment();
              rchra.request_attachment(charb);
            }
          }
          else if ( (far_vert && close_horiz) && (rchrb.vel.z-rchra.vel.z)<0 && rchrb.pos.z > rchra.pos.z+rchra.calc_bump_height)
          {
            //check to see if the characters are landing on top of a platform
            if (rchra.is_mount)
            {
              rchrb.request_detachment();
              attach_character_to_mount(charb, chara, GRIP_SADDLE);
            }
            else if (INVALID_CHR(rchrb.held_by) && rchrb.flyheight==0)
            {
              //remove chra from the old platform and attach it to the new
              rchrb.request_detachment();
              rchrb.request_attachment(chara);
            }
          }
          else if( near_vert && near_horiz )
          {
            // check to see if the characters are inside each other and apply pressure to move
            // them appart
            vec3_t diff2;

            diff2.x = overlap_x*SGN(diff.x); // + overlap_xy*SGN(diff.x+diff.y) + overlap_yx*SGN(diff.x-diff.y);
            diff2.y = overlap_y*SGN(diff.y); // + overlap_xy*SGN(diff.x+diff.y) - overlap_yx*SGN(diff.x-diff.y)
            diff2.z = overlap_z*SGN(diff.z);  

            diff2.x *= (1-depth_x)*depth_y*depth_z;
            diff2.y *= depth_x*(1-depth_y)*depth_z;
            diff2.z *= depth_x*depth_y*(1-depth_z);

            if(rchra.weight>=0) rchra.accumulate_pos( diff2);
            if(rchrb.weight>=0) rchrb.accumulate_pos(-diff2);
          }
          else if(close_interact(rchra,rchrb))
          {
            // check for a true collision between characters
            //float dotprod = dot_product(diff, rchra.vel - rchrb.vel);
            //if(dotprod < 0)
            //{
            //  bool collision = false;

            //  //if (diff.x*(rchra.vel.x-rchrb.vel.x)<0)
            //  //{
            //  //  collide_x(rchra, rchrb);
            //  //  collision = true;
            //  //}
            //  //else if( ABS(diff2.x)>0 && (rchrb.on_which_platform != chara) )
            //  //{
            //  //   pressure_x(rchra, rchrb);
            //  //};

            //  //if (diff.y*(rchra.vel.y-rchrb.vel.y)<0)
            //  //{
            //  //  collide_y(rchra, rchrb);
            //  //  collision = true;
            //  //}
            //  //else if( ABS(diff2.y)>0 && (rchrb.on_which_platform != chara) )
            //  //{
            //  //  pressure_y(rchra, rchrb);
            //  //};

            //  //if (diff.z*(rchra.vel.z-rchrb.vel.z)<0)
            //  //{
            //  //  collide_z(rchra, rchrb);
            //  //  collision = true;
            //  //}
            //  //else if( ABS(diff2.z)>0 && (rchrb.on_which_platform != chara) )
            //  //{
            //  //  pressure_z(rchra, rchrb);
            //  //};
            //}

            if(!close_interact_old(rchra,rchrb))
            {
              rchra.ai.alert |= ALERT_IF_BUMPED;
              rchrb.ai.alert |= ALERT_IF_BUMPED;

              rchra.ai.bumplast = charb;
              rchrb.ai.bumplast = chara;
            };
          }
        }
      }

      // Now check collisions with every bump particle in same area
      //if (rchra.alive)
      {
        particle = BumpList[fanblock].prt;
        if(INVALID_PRT(particle)) continue;

        for (tnc = 0; tnc < prtinblock && VALID_PRT(particle); tnc++, particle = PrtList[particle].bump_next)
        {
          Particle & rprt = PrtList[particle];
          Pip      & rpip = PipList[rprt.pip];

          if(interact(rchra, rprt) && !interact_old(rchra, rprt))
          {
            vec3_t dv = rchra.vel - rprt.vel;
            vec3_t dr = rchra.pos - rprt.pos;
            dr.z     += rchra.calc_bump_height/2 - rprt.calc_bump_height/2;

            if( MISREFLECT!=rchra.missiletreatment && dot_product(dv, dr) < 0)
            {
              // find the appropriate "coefficient of restitution"
              float cr = MIN(rchra.bump_dampen, rprt.bump_dampen);

              if( (rchra.weight == rprt.weight) || 
                  (rchra.weight<0 && rprt.weight<0) )
              {
                // "equal masses"
                vec3_t vsum  = (rchra.vel + rprt.vel)*0.5;
                vec3_t vdiff = dv*0.5;

                vec3_t vdiff_perp = parallel(dr, vdiff);
                vec3_t vdiff_para = vdiff - vdiff_perp;

                vec3_t vdiff2 = vdiff_para*(1.0f-GPhys.fric_plat) - vdiff_perp*cr;

                // equal and opposite reactions
                if(rpip.allowpush) rchra.accumulate_vel( vsum + vdiff2 );
                rprt.accumulate_vel ( vsum - vdiff2 );
                int i=0;
              }
              else if (rchra.weight<0 || rprt.weight==0)
              {
                // immovable character or massless particle

                vec3_t vsum  = (rchra.vel + rprt.vel)*0.5;
                vec3_t vdiff = dv*0.5;

                vec3_t vdiff_perp = parallel(dr, vdiff);
                vec3_t vdiff_para = vdiff - vdiff_perp;

                vec3_t vdiff2 = vdiff_para*(1.0f-GPhys.fric_plat) - vdiff_perp*cr;

                rprt.accumulate_vel ( 2*(vsum - vdiff2) );
                int i=0;
              }
              else if (rchra.weight==0 || rprt.weight<0)
              {
                // massless character or immovable particle
                if(rpip.allowpush)
                {
                  vec3_t vsum  = (rchra.vel + rprt.vel)*0.5;
                  vec3_t vdiff = dv*0.5;

                  vec3_t vdiff_perp = parallel(dr, vdiff);
                  vec3_t vdiff_para = vdiff - vdiff_perp;

                  vec3_t vdiff2 = vdiff_para*(1.0f-GPhys.fric_plat) - vdiff_perp*cr;

                  rchra.accumulate_vel ( 2*(vsum + vdiff2) );
                  int i=0;
                };
              }
              else
              {
                // the "normal" case
                vec3_t vsum  = (rchra.vel*rchra.weight + rprt.vel*rprt.weight)*0.5;
                vec3_t vdiff = (rchra.vel*rchra.weight - rprt.vel*rprt.weight)*0.5;

                vec3_t vdiff_perp = parallel(dr, vdiff);
                vec3_t vdiff_para = vdiff - vdiff_perp;

                vec3_t vdiff2 = vdiff_para*(1.0f-GPhys.fric_plat) - vdiff_perp*cr;

                // equal and opposite reactions
                if(rpip.allowpush) rchra.accumulate_vel( (vsum + vdiff2)/rchra.weight );
                rprt.accumulate_vel ( (vsum - vdiff2)/rprt.weight );
                int i=0;
              }
            }
            else if (MISREFLECT==rchra.missiletreatment &&  dot_product(dv, dr) < 0)
            {
              // magically reflect the particle away
              rprt.vel *= -rchra.bump_dampen;
            };

            // Check reaffirmation of particles
            if (rprt.attachedtocharacter!=chara)
            {
              if (rchra.reloadtime==0)
              {
                if (rchra.attachedprtreaffirmdamagetype==rprt.damagetype && rchra.damagetime==0)
                {
                  reaffirm_attached_particles(chara);
                }
              }
            }

            // Check for missile treatment
            if ((rchra.damagemodifier[rprt.damagetype]&3) < 2 || 
                VALID_CHR(rprt.attachedtocharacter) || 
                (rprt.chr == chara && !rpip.friendlyfire) || 
                (ChrList[rchra.missilehandler].mana < (rchra.missilecost<<4) && !ChrList[rchra.missilehandler].canchannel))
            {
              if ((TeamList[rprt.team].hatesteam[rchra.team] || (rpip.friendlyfire && ((chara!=rprt.chr && chara!=ChrList[rprt.chr].held_by) || rpip.onlydamagefriendly))) && !rchra.invictus)
              {
                spawn_bump_particles(chara, particle); // Catch on fire

                if ( (rprt.damagebase|rprt.damagerand) > 1)
                {
                  id_prt_parent = rprt.getCap().idsz[IDSZ_PARENT];
                  id_prt_type = rprt.getCap().idsz[IDSZ_TYPE];
                  if (rchra.damagetime==0 && rprt.attachedtocharacter!=chara && (rpip.damfx&DAMFX_ARRO) == 0)
                  {
                    direction = rchra.turn_lr - vec_to_turn(rprt.vel);

                    // Check all enchants to see if they are removed
                    enchant = rchra.firstenchant;
                    while (enchant != Enchant_List::INVALID)
                    {
                      id_eve_remove = EncList[enchant].getEve().removedbyidsz;
                      temp = EncList[enchant].nextenchant;
                      if (id_eve_remove != IDSZ::NONE && (id_eve_remove == id_prt_type || id_eve_remove == id_prt_parent))
                      {
                        remove_enchant(enchant);
                      }
                      enchant = temp;
                    }

                    // Damage the character
                    if (id_chr_vulnerability != IDSZ::NONE && (id_chr_vulnerability == id_prt_type || id_chr_vulnerability == id_prt_parent))
                    {
                      damage_character(chara, direction, rprt.damagebase<<1, rprt.damagerand<<1, rprt.damagetype, rprt.team, rprt.chr, rpip.damfx);
                      rchra.ai.alert|=ALERT_IF_HITVULNERABLE;
                    }
                    else
                    {
                      damage_character(chara, direction, rprt.damagebase, rprt.damagerand, rprt.damagetype, rprt.team, rprt.chr, rpip.damfx);
                    }

                    // Do confuse effects
                    if (0==(rchra.getMad().getExtras(rchra.ani.frame).framefx&MADFX_INVICTUS) || (rpip.damfx&DAMFX_BLOC))
                    {
                      if (rpip.grogtime != 0 && rchra.getCap().canbegrogged)
                      {
                        rchra.grogtime+=rpip.grogtime;
                        if (rchra.grogtime<0)  rchra.grogtime = 0x7FFF;
                        rchra.ai.alert |= ALERT_IF_GROGGED;
                      }
                      if (rpip.dazetime != 0 && rchra.getCap().canbedazed)
                      {
                        rchra.dazetime+=rpip.dazetime;
                        if (rchra.dazetime<0)  rchra.dazetime = 0x7FFF;
                        rchra.ai.alert |= ALERT_IF_DAZED;
                      }
                    }

                    // Notify the attacker of a scored hit
                    if (VALID_CHR(rprt.chr))
                    {
                      ChrList[rprt.chr].ai.alert |= ALERT_IF_SCOREDAHIT;
                      ChrList[rprt.chr].ai.hitlast=chara;
                    }
                  }
                }

                if (rpip.endbump)
                {
                  if (rpip.bumpmoney)
                  {
                    if (rchra.cangrabmoney && rchra.alive && rchra.damagetime==0 && rchra.money!=MAXMONEY)
                    {
                      if (rchra.is_mount)
                      {
                        // Let mounts collect money for their riders
                        if (VALID_CHR(rchra.holding_which[SLOT_LEFT]))
                        {
                          ChrList[rchra.holding_which[SLOT_LEFT]].money+=rpip.bumpmoney;
                          if (ChrList[rchra.holding_which[SLOT_LEFT]].money > MAXMONEY) ChrList[rchra.holding_which[SLOT_LEFT]].money = MAXMONEY;
                          if (ChrList[rchra.holding_which[SLOT_LEFT]].money < 0) ChrList[rchra.holding_which[SLOT_LEFT]].money = 0;
                          rprt.requestDestroy();
                        }
                      }
                      else
                      {
                        // Normal money collection
                        rchra.money+=rpip.bumpmoney;
                        if (rchra.money > MAXMONEY) rchra.money = MAXMONEY;
                        if (rchra.money < 0) rchra.money = 0;
                        rprt.requestDestroy();
                      }
                    }
                  }
                  else
                  {
                    rprt.requestDestroy();
                    // Only hit one character, not several
                    rprt.damagebase = 0;
                    rprt.damagerand = 1;
                  }
                }
              }
            }
            else if (rprt.chr != chara)
            {
              cost_mana(rchra.missilehandler, (rchra.missilecost<<4), rprt.chr);

              //// Change the owner of the missile
              if (!rpip.homing)
              {
                rprt.team = rchra.team;
                rprt.chr  = chara;
              }

              // Change the direction of the particle
              if (rpip.rotatetoface)
              {
                // Turn to face new direction
                facing = vec_to_turn(rprt.vel);
                rprt.turn_lr = facing;
              }
            }
          }
        }
      }
    }
  }
}


//char.c
//--------------------------------------------------------------------------------------------
void damage_character(Uint16 character, Uint16 direction,
                      int damagebase, int damagerand, Uint8 damagetype, Uint8 team,
                      Uint16 attacker, Uint16 effects)
{
  // ZZ> This function calculates and applies damage to a character.  It also
  //     sets alerts and begins actions.  Blocking and frame invincibility
  //     are done here too.  Direction is 0 if the attack is coming head on,
  //     0x4000 if from the right, 0x8000 if from the back, 49152 if from the
  //     left.
  int tnc;
  ACTION_TYPE action;
  int damage, basedamage;
  Uint16 experience, model, left, right;

  if (ChrList[character].alive && damagebase>=0 && damagerand>=1)
  {
    Character & rchr = ChrList[character];

    // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    damage = damagebase+(rand()%damagerand);
    basedamage = damage;
    damage = damage>>(rchr.damagemodifier[damagetype]&DAMAGE_SHIFT);

    // Allow charging
    if (rchr.damagemodifier[damagetype]&DAMAGE_CHARGE)
    {
      rchr.mana += damage;
      if (rchr.mana > rchr.manamax)
      {
        rchr.mana = rchr.manamax;
      }
      return;
    }

    // Invert damage to heal
    if (rchr.damagemodifier[damagetype]&DAMAGE_INVERT)
      damage=-damage;

    // Remember the damage type
    rchr.ai.damagetypelast = damagetype;
    rchr.ai.directionlast = direction;

    // Do it already
    if (damage > 0)
    {
      // Only damage if not invincible
      if (rchr.damagetime==0 && !rchr.invictus)
      {
        model = rchr.model;
        if (0==(effects&DAMFX_BLOC))
        {
          // Only damage if hitting from proper direction
          if (rchr.getMad().getExtras(rchr.ani.frame).framefx&MADFX_INVICTUS)
          {
            // I Frame...
            direction -= rchr.getCap().iframefacing;
            left = (~rchr.getCap().iframeangle);
            right = rchr.getCap().iframeangle;
            // Check for shield
            if (rchr.act.which >= ACTION_PA && rchr.act.which <= ACTION_PD)
            {
              // Using a shield?
              if (rchr.act.which < ACTION_PC)
              {
                // Check left hand
                if (VALID_CHR(rchr.holding_which[SLOT_LEFT]))
                {
                  left = (~ChrList[rchr.holding_which[SLOT_LEFT]].getCap().iframeangle);
                  right = ChrList[rchr.holding_which[SLOT_LEFT]].getCap().iframeangle;
                }
              }
              else
              {
                // Check right hand
                if (VALID_CHR(rchr.holding_which[SLOT_RIGHT]))
                {
                  left = (~ChrList[rchr.holding_which[SLOT_RIGHT]].getCap().iframeangle);
                  right = ChrList[rchr.holding_which[SLOT_RIGHT]].getCap().iframeangle;
                }
              }
            }
          }
          else
          {
            // N Frame
            direction -= rchr.getCap().nframefacing;
            left = (~rchr.getCap().nframeangle);
            right = rchr.getCap().nframeangle;
          }
          // Check that direction
          if (direction > left || direction < right)
          {
            damage = 0;
          }
        }

        if (damage!=0)
        {
          if (effects&DAMFX_ARMO)
          {
            rchr.life-=damage;
          }
          else
          {
            rchr.life-=((damage*rchr.defense)>>FIXEDPOINT_BITS);
          }

          if (basedamage > MINDAMAGE)
          {
            // Call for help if below 1/2 life
            if (rchr.life < (rchr.lifemax>>1))
              call_for_help(character);

            // Spawn blood particles
            if (rchr.getCap().bloodvalid && (damagetype < DAMAGE_HOLY || rchr.getCap().bloodvalid==ULTRABLOODY))
            {
              int iwt = rchr.weight<0 ? 0 : rchr.weight/100+1;
              spawn_one_particle(rchr.pos, rchr.turn_lr+direction, rchr.vel, iwt,
                rchr.model, rchr.getCap().bloodprttype,
                Character_List::INVALID, GRIP_LAST, rchr.team, character, 0, Character_List::INVALID);
            }
            // Set attack ai.alert if it wasn't an accident
            if (team == TEAM_DAMAGE)
            {
              rchr.ai.attacklast = Character_List::INVALID;
            }
            else
            {
              // Don't ai.alert the character too much if under constant fire
              if (rchr.carefultime == 0)
              {
                // Don't let characters chase themselves...  That would be silly
                if (attacker != character)
                {
                  rchr.ai.alert |= ALERT_IF_ATTACKED;
                  rchr.ai.attacklast = attacker;
                  rchr.carefultime = CAREFULTIME;
                }
              }
            }
          }

          // Taking damage action
          action = ACTION_HA;
          if (rchr.life < 0)
          {
            // Character has died
            rchr.alive = false;
            disenchant_character(character);
            rchr.waskilled = true;
            rchr.act.keep = true;
            rchr.life = -1;
            rchr.is_platform = true;
            rchr.bump_dampen = rchr.bump_dampen/2.0;
            action = ACTION_KA;
            // Give kill experience
            experience = rchr.getCap().experienceworth+(rchr.experience*rchr.getCap().experienceexchange);
            if (VALID_CHR(attacker))
            {
              // Set target
              rchr.ai.target = attacker;
              if (team == TEAM_DAMAGE)  rchr.ai.target = character;
              if (team == TEAM_NEUTRAL)  rchr.ai.target = character;
              // Award direct kill experience
              if (TeamList[ChrList[attacker].team].hatesteam[rchr.team])
              {
                give_experience(attacker, experience, XP_KILLENEMY);
              }
              // Check for hated
              if (ChrList[attacker].getCap().idsz[IDSZ_HATE]==rchr.getCap().idsz[IDSZ_PARENT] || 
                ChrList[attacker].getCap().idsz[IDSZ_HATE]==rchr.getCap().idsz[IDSZ_TYPE])
              {
                give_experience(attacker, experience, XP_KILLHATED);
              }
            }

            // Clear all shop passages that it owned...
            ShopList.clear_passages(character);

            // Let the other characters know it died
            SCAN_CHR_BEGIN(tnc, rchr_tnc)
            {
              if (!rchr_tnc.alive) continue;

              if (rchr_tnc.ai.target == character)
              {
                rchr_tnc.ai.alert |= ALERT_IF_TARGETKILLED;
              }

              if ((!TeamList[rchr_tnc.team].hatesteam[team]) && (TeamList[rchr_tnc.team].hatesteam[rchr.team]))
              {
                // All allies get team experience, but only if they also hate the dead guy's team
                give_experience(tnc, experience, XP_TEAMKILL);
              }
            } SCAN_CHR_END;

            // Check if it was a leader
            if (TeamList[rchr.team].leader==character)
            {
              // It was a leader, so set more alerts
              SCAN_CHR_BEGIN(tnc, rchr_tnc)
              {
                if (rchr_tnc.team!=rchr.team) continue;

                // All folks on the leaders team get the ai.alert
                rchr_tnc.ai.alert |= ALERT_IF_LEADERKILLED;
              } SCAN_CHR_END;

              // The team now has no leader
              TeamList[rchr.team].leader = NOLEADER;
            }
            detach_character_from_mount(character, true, false);
            action = (ACTION_TYPE)(action + (rand()&3));
            play_action(character, action, false);

            // Turn off all sounds if it's a player
            if (rchr.isplayer)
            {

              for (tnc = 0; VALID_WAVE_RANGE(tnc); tnc++)
              {
                //Does this do anything?
                //stop_sound(rchr.getCap().waveindex[tnc]);
              }
            }

            // Afford it one last thought if it's an AI
            TeamList[rchr.baseteam].morale--;
            rchr.team = rchr.baseteam;
            rchr.ai.alert = ALERT_IF_KILLED;
            rchr.sparkle = NOSPARKLE;
            rchr.ai.time = 1;  // No timeout...
            let_character_think(character);
          }
          else
          {
            if (basedamage > MINDAMAGE)
            {
              action = (ACTION_TYPE)(action + (rand()&3));
              play_action(character, action, false);
              // Make the character invincible for a limited time only
              if (!(effects & DAMFX_TIME))
                rchr.damagetime = INVICTUS_DAMAGETIME;
            }
          }
        }
        else
        {
          // Spawn a defend particle
          spawn_one_particle(rchr.pos, rchr.turn_lr, rchr.vel, rchr.weight, Profile_List::INVALID, PRTPIP_DEFEND, Character_List::INVALID, GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, 0, Character_List::INVALID);
          rchr.damagetime = INVICTUS_DEFENDTIME;
          rchr.ai.alert |= ALERT_IF_BLOCKED;
        }
      }
    }
    else if (damage < 0)
    {
      rchr.life-=damage;
      if (rchr.life > rchr.lifemax)  rchr.life = rchr.lifemax;

      // Isssue an ai.alert
      rchr.ai.alert |= ALERT_IF_HEALED;
      rchr.ai.attacklast = attacker;
      if (team != TEAM_DAMAGE)
      {
        rchr.ai.attacklast = Character_List::INVALID;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void kill_character(Uint16 character, Uint16 killer)
{
  // ZZ> This function kills a character...  Character_List::INVALID killer for accidental death
  Uint8 modifier;

  if (INVALID_CHR(character) || !ChrList[character].alive) return;

  Character & rchr = ChrList[character];

  rchr.damagetime = 0;
  rchr.life = 1;
  modifier = rchr.damagemodifier[DAMAGE_CRUSH];
  rchr.damagemodifier[DAMAGE_CRUSH] = 1;
  if (VALID_CHR(killer))
  {
    damage_character(character, 0, 0x0200, 1, DAMAGE_CRUSH, ChrList[killer].team, killer, DAMFX_ARMO|DAMFX_BLOC);
  }
  else
  {
    damage_character(character, 0, 0x0200, 1, DAMAGE_CRUSH, TEAM_DAMAGE, rchr.ai.bumplast, DAMFX_ARMO|DAMFX_BLOC);
  }
  rchr.damagemodifier[DAMAGE_CRUSH] = modifier;

}

//--------------------------------------------------------------------------------------------
void spawn_poof(Uint16 character, Uint16 profile)
{
  // ZZ> This function spawns a character poof
  Uint16 sTmp;
  Uint16 origin;
  int iTmp;

  sTmp = ChrList[character].turn_lr;
  iTmp = 0;
  origin = ChrList[character].ai.owner;
  CAP_REF cap_ref = ProfileList[profile].cap_ref;
  int iwt;

  if(ChrList[character].weight<0)
  {
    iwt = -1;
  }
  else
  {
    float fwt = float(ChrList[character].weight) / float(CapList[cap_ref].gopoofprtamount);
    iwt = int(fwt) + 1;
  };

  while (iTmp < CapList[cap_ref].gopoofprtamount)
  {
    spawn_one_particle(ChrList[character].old.pos, sTmp, ChrList[character].old.vel, iwt, profile, CapList[cap_ref].gopoofprttype,
      Character_List::INVALID, GRIP_LAST, ChrList[character].team, origin, iTmp, Character_List::INVALID);

    sTmp += CapList[cap_ref].gopoofprtfacingadd;
    iTmp++;
  }
}


//--------------------------------------------------------------------------------------------
//void tilt_characters_to_terrain()
//{
//  // ZZ> This function sets all of the character's starting tilt values
//  int cnt;
//
//  const JF::MPD_Fan * pfan;
//
//  SCAN_CHR_BEGIN(cnt, rchr_cnt)
//  {
//    if (rchr_cnt.stickybutt)
//    {
//      pfan = GMesh.getFanPos(rchr_cnt.pos.x, rchr_cnt.pos.y);
//      if(NULL==pfan)
//      {
//        rchr_cnt.map_turn_lr = 0x8000;
//        rchr_cnt.map_turn_ud = 0x8000;
//      }
//      else
//      {
//        rchr_cnt.map_turn_lr = map_twist_lr[pfan->twist];
//        rchr_cnt.map_turn_ud = map_twist_ud[pfan->twist];
//      }
//    }
//  } SCAN_CHR_END;
//
//}

//--------------------------------------------------------------------------------------------
Uint16 get_target_in_block(int block_x, int block_y, Uint16 ichra, bool items,
                           bool friends, bool enemies, bool dead, bool seeinvisible, IDSZ idsz,
                           bool excludeid)
{
  // ZZ> This is a good little helper, that returns != Character_List::INVALID if a suitable target
  //     was found
  int cnt;
  Uint16 ichrb;
  Uint32 fanblock;

  fanblock = GMesh.getFanblockBlock(block_x, block_y);
  if(fanblock == Mesh::INVALID_INDEX) return Character_List::INVALID;
  if(0 == BumpList[fanblock].chr_count) return Character_List::INVALID;

  if(INVALID_CHR(ichra)) return Character_List::INVALID;
  Character & rchra = ChrList[ichra];


  ichrb = BumpList[fanblock].chr;
  for (cnt = 0; cnt < BumpList[fanblock].chr_count && VALID_CHR(ichrb); cnt++, ichrb = ChrList[ichrb].bump_next)
  {
    Character & rchrb = ChrList[ichrb];

    if(ichrb == ichra) continue;
    if(rchra.held_by == ichrb) continue;
    if(rchrb.is_item && !items) continue;
    if(dead == rchrb.alive) continue;
    if( VALID_CHR(rchrb.held_by) ) continue;

    if (seeinvisible || (rchrb.alpha>INVISIBLE && rchrb.light>INVISIBLE))
    {

      if ( (enemies && TeamList[rchra.team].hatesteam[rchrb.team] && !rchrb.invictus) || 
        (items && rchrb.is_item) || 
        (friends && rchrb.baseteam==rchra.team))
      {
        if (idsz == IDSZ::NONE)
        {
          return ichrb;
        }
        else if (rchrb.getCap().idsz[IDSZ_PARENT] == idsz || 
          rchrb.getCap().idsz[IDSZ_TYPE] == idsz)
        {
          if (!excludeid) return ichrb;
        }
        else
        {
          if (excludeid)  return ichrb;
        }
      }
    }
  }

  return Character_List::INVALID;
}

//--------------------------------------------------------------------------------------------
Uint16 get_nearby_target(Uint16 character, bool items,
                         bool friends, bool enemies, bool dead, IDSZ idsz)
{
  // ZZ> This function finds a nearby target, or it returns Character_List::INVALID if it can't find one
  int block_x, block_y;
  bool seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  block_x = int(ChrList[character].pos.x) >> Mesh::Block_bits;
  block_y = int(ChrList[character].pos.y) >> Mesh::Block_bits;
  return get_target_in_block(block_x, block_y, character, items, friends, enemies, dead, seeinvisible, idsz, 0);
}

//--------------------------------------------------------------------------------------------
Uint16 find_distant_target(Uint16 ichra, int maxdistance)
{
  // ZZ> This function finds a target, or it returns Character_List::INVALID if it can't find one...
  //     maxdistance should be the square of the actual distance you want to use
  //     as the cutoff...
  int ichrb;
  float distance;
  Uint8 team;

  if(INVALID_CHR(ichra)) return Character_List::INVALID;

  Character & rchra = ChrList[ichra];
  team = ChrList[ichra].team;
  distance = maxdistance;
  SCAN_CHR_BEGIN(ichrb, rchrb)
  {
    if( ichrb == ichra ) continue;
    if ( rchrb.is_inpack ) continue;
    if ( !rchrb.alive ) continue;
    if ( rchrb.invictus ) continue;
    if ( !TeamList[team].hatesteam[rchrb.team] ) continue;

    if (rchra.canseeinvisible || (rchrb.alpha>INVISIBLE && rchrb.light>INVISIBLE))
    {
      distance = diff_abs(rchra.pos, rchra.pos);
      if (distance < maxdistance) return ichrb;
    }

  } SCAN_CHR_END;

  return Character_List::INVALID;
}

//--------------------------------------------------------------------------------------------
void get_nearest_in_block(int block_x, int block_y, Uint32 ichra, bool items,
                          bool friends, bool enemies, bool dead, bool seeinvisible, IDSZ idsz)
{
  // ZZ> This is a good little helper
  float distance;
  vec3_t dis;
  int cnt;
  Uint32 ichrb, fanblock;

  fanblock = GMesh.getFanblockBlock(block_x, block_y);
  if(fanblock == Mesh::INVALID_INDEX) return;
  if(0 == BumpList[fanblock].chr_count) return;

  if(INVALID_CHR(ichra)) return;
  Character & rchra = ChrList[ichra];

  ichrb = BumpList[fanblock].chr;
  for (cnt = 0; cnt < BumpList[fanblock].chr_count && VALID_CHR(ichrb); cnt++, ichrb = ChrList[ichrb].bump_next)
  {
    Character & rchrb = ChrList[ichrb];

    if( ichrb == ichra ) continue;
    if( rchrb.is_inpack ) continue;
    if( rchrb.invictus && !items ) continue;
    if( dead && rchrb.alive ) continue;
    if( rchra.held_by == ichrb ) continue;
    if( VALID_CHR(rchrb.held_by) ) continue;

    if ( seeinvisible || (rchrb.alpha>INVISIBLE && rchrb.light>INVISIBLE))
    {
      if ((enemies && TeamList[rchra.team].hatesteam[rchrb.team]) || 
        (items && rchrb.is_item) || 
        (friends && (rchrb.baseteam==rchra.team)) )
      {

        if (idsz == IDSZ::NONE || rchrb.getCap().idsz[IDSZ_PARENT] == idsz ||  rchrb.getCap().idsz[IDSZ_TYPE] == idsz)
        {
          distance = diff_squared(rchra.pos, rchrb.pos);
          if (distance < GParams.distance)
          {
            GParams.nearest  = ichrb;
            GParams.distance = distance;
          }
        }
      }
    }
  }

}

//--------------------------------------------------------------------------------------------
Uint32 get_nearest_target(Uint32 character, bool items,
                          bool friends, bool enemies, bool dead, IDSZ idsz)
{
  // ZZ> This function finds an target, or it returns Character_List::INVALID if it can't find one
  int x, y;
  bool seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = int(ChrList[character].pos.x) >> Mesh::Block_bits;
  y = int(ChrList[character].pos.y) >> Mesh::Block_bits;

  GParams.nearest = Character_List::INVALID;
  GParams.distance = 999999;
  get_nearest_in_block(x, y, character, items, friends, enemies, dead, seeinvisible, idsz);

  get_nearest_in_block(x-1, y, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x+1, y, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x, y-1, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x, y+1, character, items, friends, enemies, dead, seeinvisible, idsz);

  get_nearest_in_block(x-1, y+1, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x+1, y-1, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x-1, y-1, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x+1, y+1, character, items, friends, enemies, dead, seeinvisible, idsz);

  return GParams.nearest;
}

//--------------------------------------------------------------------------------------------
Uint32 get_wide_target(Uint32 character, bool items,
                       bool friends, bool enemies, bool dead, IDSZ idsz, bool excludeid)
{
  // ZZ> This function finds an target, or it returns Character_List::INVALID if it can't find one
  int x, y;
  Uint16 enemy;
  bool seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = int(ChrList[character].pos.x) >> Mesh::Block_bits;
  y = int(ChrList[character].pos.y) >> Mesh::Block_bits;

  enemy = get_target_in_block(x, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (VALID_CHR(enemy))  return enemy;

  enemy = get_target_in_block(x-1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (VALID_CHR(enemy))  return enemy;
  enemy = get_target_in_block(x+1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (VALID_CHR(enemy))  return enemy;
  enemy = get_target_in_block(x, y-1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (VALID_CHR(enemy))  return enemy;
  enemy = get_target_in_block(x, y+1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (VALID_CHR(enemy))  return enemy;

  enemy = get_target_in_block(x-1, y+1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (VALID_CHR(enemy))  return enemy;
  enemy = get_target_in_block(x+1, y-1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (VALID_CHR(enemy))  return enemy;
  enemy = get_target_in_block(x-1, y-1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (VALID_CHR(enemy))  return enemy;
  enemy = get_target_in_block(x+1, y+1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  return enemy;
}

//--------------------------------------------------------------------------------------------
void issue_clean(Uint16 character)
{
  // ZZ> This function issues a clean up order to all teammates
  Uint8 team;
  Uint16 cnt;

  team = ChrList[character].team;
  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if (rchr_cnt.team==team)
    {
      rchr_cnt.ai.time  = 2;                   // Set a little delay
      rchr_cnt.ai.alert = ALERT_IF_CLEANEDUP;
    }
  } SCAN_CHR_END;
}

//--------------------------------------------------------------------------------------------
int restock_ammo(Uint16 character, IDSZ idsz)
{
  // ZZ> This function restocks the characters ammo, if it needs ammo and if
  //     either its parent or type idsz match the given idsz.  This
  //     function returns the amount of ammo given.
  int amount;

  amount = 0;
  if (VALID_CHR(character))
  {
    Character & rchr = ChrList[character];

    if (rchr.getCap().idsz[IDSZ_PARENT] == idsz || rchr.getCap().idsz[IDSZ_TYPE] == idsz)
    {
      if (rchr.ammo < rchr.ammomax)
      {
        amount = rchr.ammomax - rchr.ammo;
        rchr.ammo = rchr.ammomax;
      }
    }

  }
  return amount;
}

//--------------------------------------------------------------------------------------------
void issue_order(Uint16 character, Uint32 order)
{
  // ZZ> This function issues an order for help to all teammates
  Uint8 team;
  Uint8 counter;
  Uint16 cnt;

  team = ChrList[character].team;
  counter = 0;
  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if (!ChrList[cnt].team==team) continue;

    rchr_cnt.order     = order;
    rchr_cnt.counter   = counter;
    rchr_cnt.ai.alert |= ALERT_IF_ORDERED;
    counter++;

  } SCAN_CHR_END;
}

//--------------------------------------------------------------------------------------------
void issue_special_order(Uint32 order, IDSZ idsz)
{
  // ZZ> This function issues an order to all characters with the a matching special IDSZ
  Uint8 counter;
  Uint16 cnt;

  counter = 0;
  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {

    if (rchr_cnt.getCap().idsz[IDSZ_SPECIAL] == idsz)
    {
      rchr_cnt.order     = order;
      rchr_cnt.counter   = counter;
      rchr_cnt.ai.alert |= ALERT_IF_ORDERED;
      counter++;
    }

  } SCAN_CHR_END;
}

//--------------------------------------------------------------------------------------------
void set_alerts(int character)
{
  // ZZ> This function polls some ai.alert conditions
  if (ChrList[character].ai.time!=0)
  {
    ChrList[character].ai.time--;
  }
  if (ChrList[character].pos.x < ChrList[character].ai.goto_x[ChrList[character].ai.goto_cnt] + (WAYTHRESH + ChrList[character].calc_bump_size) && 
      ChrList[character].pos.x > ChrList[character].ai.goto_x[ChrList[character].ai.goto_cnt] - (WAYTHRESH + ChrList[character].calc_bump_size) && 
      ChrList[character].pos.y < ChrList[character].ai.goto_y[ChrList[character].ai.goto_cnt] + (WAYTHRESH + ChrList[character].calc_bump_size) && 
      ChrList[character].pos.y > ChrList[character].ai.goto_y[ChrList[character].ai.goto_cnt] - (WAYTHRESH + ChrList[character].calc_bump_size))
  {
    ChrList[character].ai.alert |= ALERT_IF_ATWAYPOINT;
    ChrList[character].ai.goto_cnt++;
    if (ChrList[character].ai.goto_cnt==ChrList[character].ai.goto_idx)
    {
      ChrList[character].ai.goto_cnt = 0;
      if (!ChrList[character].getCap().is_equipment)
      {
        ChrList[character].ai.alert |= ALERT_IF_ATLASTWAYPOINT;
      }
    }
  }
}
