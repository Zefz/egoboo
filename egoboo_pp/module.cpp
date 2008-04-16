// module.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Input.h"
#include "Mad.h"
#include "Character.h"
#include "MPD_file.h"
#include "Particle.h"
#include "Font.h"
#include "UI.h"
#include "Passage.h"
#include "Profile.h"
#include "egoboo.h"

int      Module::globalnum = 0;                            // Number of modules


//--------------------------------------------------------------------------------------------
void release_module(void)
{
  // ZZ> This function frees up memory used by the module
  TxList.release_all_textures();
  IconList.release_all_icons();
  release_map();
  Mix_CloseAudio(); // Close and then reopen SDL_mixer; it's easier than manually unloading each sound
  songplaying = -1;
  Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, buffersize);
  Mix_AllocateChannels(maxsoundchannel);
}

//--------------------------------------------------------------------------------------------
int module_reference_matches(char *szLoadName, IDSZ idsz)
{
  // ZZ> This function returns true if the named module has the required IDSZ
  FILE *fileread;
  char newloadname[0x0100];
  IDSZ newidsz;
  int foundidsz;
  int cnt;

  if (szLoadName[0] == 'N' && szLoadName[1] == 'O' && szLoadName[2] == 'N' && szLoadName[3] == 'E' && szLoadName[4] == 0)
    return true;
  if (idsz == IDSZ::NONE)
    return true;

  foundidsz = false;
  sprintf(newloadname, "modules/%s/gamedat/menu.txt", szLoadName);
  fileread = fopen(newloadname, "r");
  if (fileread)
  {
    // Read basic data
    globalname = szLoadName;
    goto_colon(fileread);  // Name of module...  Doesn't matter
    goto_colon(fileread);  // Reference directory...
    goto_colon(fileread);  // Reference IDSZ...
    goto_colon(fileread);  // Import...
    goto_colon(fileread);  // Export...
    goto_colon(fileread);  // Min players...
    goto_colon(fileread);  // Max players...
    goto_colon(fileread);  // Respawn...
    goto_colon(fileread);  // RTS...
    goto_colon(fileread);  // Rank...

    // Summary...
    cnt = 0;
    while (cnt < SUMMARYLINES)
    {
      goto_colon(fileread);
      cnt++;
    }

    // Now check expansions
    while (goto_colon_yesno(fileread) && !foundidsz)
    {
      newidsz = get_idsz(fileread);
      if (newidsz == idsz)
      {
        foundidsz = true;
      }
    }

    fclose(fileread);
  }
  return foundidsz;
}

//--------------------------------------------------------------------------------------------
void add_module_idsz(char *szLoadName, IDSZ idsz)
{
  // ZZ> This function appends an IDSZ to the module's menu.txt file
  FILE *filewrite;
  char newloadname[0x0100];

  // Only add if there isn't one already
  if (!module_reference_matches(szLoadName, idsz))
  {
    // Try to open the file in append mode
    sprintf(newloadname, "modules/%s/gamedat/menu.txt", szLoadName);
    filewrite = fopen(newloadname, "a");
    if (filewrite)
    {
      fprintf(filewrite, "\n:[%4s]\n", IDSZ::convert(idsz));
      fclose(filewrite);
    }
  }
}

//--------------------------------------------------------------------------------------------
int find_module(char *smallname)
{
  // ZZ> This function returns -1 if the module does not exist locally, the module
  //     index otherwise

  int cnt, index;
  cnt = 0;
  index = -1;
  while (cnt < Module::globalnum)
  {
    if (strcmp(smallname, ModList[cnt].loadname) == 0)
    {
      index = cnt;
      cnt = Module::globalnum;
    }
    cnt++;
  }
  return index;
}

//--------------------------------------------------------------------------------------------

void load_module(Camera & cam, Mesh & msh, char *smallname)
{
  // ZZ> This function loads a module
  char modname[0x80];

// char songlist[0x80];  TODO: see below
//    FILE* playlist;


  beatmodule = false;
  timeron = false;
  sprintf(modname, "modules/%s/", smallname);

  make_randie();
  reset_teams();
  IconList.null = IconList.load_one_icon("basicdat/nullicon.bmp");
  PipList.reset_particles(modname);
  read_wawalite(modname);
  //make_twist();
  reset_messages();
  ProfileList.prime_names();
  load_basic_textures(modname);

  ScrList.reset_ai_script();
  ProfileList.release_all_models();
  EncList.release_all_enchants();

  // Load sound files
  if (soundvalid) load_global_waves(modname);

  //First unload menu music then load ingame music
  if (musicvalid)
  {
    songplaying = -1;
    Mix_FadeOutMusic(1000);
    Mix_FreeMusic(music);
    /*TODO: load music without scripts?       //Now load the new music
            sprintf(playlist,("%sgamedat/music.txt"), modname);  // The file to load
            playlist = fopen(songlist, "r");
            if(musicfile)
            {
                fscanf(playlist, "%s", songlist);  // Read in a new directory
                fclose(playlist);
            }*/
    load_all_music_sounds();
  }

  ProfileList.load_all_objects(modname);

  if ( !GMesh.load(modname) )
    general_error(0, 0, "LOAD PROBLEMS");

  PrtList.setup_particles();
  PassList.setup_passage(modname);
  PlaList.reset_players();
  ChrList.setup_characters(modname);
  TeamList.setup_alliances(modname);

  reset_end_text();

  // Load fonts and bars after other images, as not to hog videomem
  UI::bmp_fnt = Font_Manager::loadFont("basicdat/font");
  load_bars("basicdat/bars.bmp");
  load_map(modname, false);

  // GS - log_madused("basicdat/slotused.txt");

  // RTS stuff
  clear_orders();
}

//--------------------------------------------------------------------------------------------
int get_module_data(int modnumber, char *szLoadName)
{
  // ZZ> This function loads the module data file
  FILE *fileread;
  char reference[0x80];
  IDSZ idsz;
  char cTmp;
  int iTmp;

  fileread = fopen(szLoadName, "r");
  if (fileread)
  {
    // Read basic data
    globalname = szLoadName;
    goto_colon(fileread);  get_name(fileread, ModList[modnumber].longname);
    goto_colon(fileread);  fscanf(fileread, "%s", reference);
    idsz = get_next_idsz(fileread);
    if (module_reference_matches(reference, idsz))
    {
      globalname = szLoadName;
      ModList[modnumber].importamount = get_next_int(fileread);
      ModList[modnumber].allowexport = get_next_bool(fileread);
      ModList[modnumber].minplayers = get_next_int(fileread);
      ModList[modnumber].maxplayers = get_next_int(fileread);

      goto_colon(fileread);  cTmp = get_first_letter(fileread);
      switch(toupper(cTmp))
      {
        case 'T': ModList[modnumber].respawn_mode = true;    break;
        case 'A': ModList[modnumber].respawn_mode = ANYTIME; break;
        default:
        case 'F': ModList[modnumber].respawn_mode = false;   break;
      };

      goto_colon(fileread);  cTmp = get_first_letter(fileread);
      switch(toupper(cTmp))
      {
        case 'T': ModList[modnumber].rts_mode = true;    break;
        case 'A': ModList[modnumber].rts_mode = ALLSELECT; break;
        default:
        case 'F': ModList[modnumber].rts_mode = false;   break;
      };

      goto_colon(fileread);  fscanf(fileread, "%s", generictext);
      iTmp = 0;
      while (iTmp < RANKSIZE-1)
      {
        ModList[modnumber].rank[iTmp] = generictext[iTmp];
        iTmp++;
      }
      ModList[modnumber].rank[iTmp] = 0;

      // Read the expansions
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------
int get_module_summary(char *szLoadName)
{
  // ZZ> This function gets the quest description out of the module's menu file
  FILE *fileread;
  char cTmp;
  char szLine[160];
  int cnt;
  int tnc;
  bool result = false;

  fileread = fopen(szLoadName, "r");
  if (fileread)
  {
    // Skip over basic data
    globalname = szLoadName;
    goto_colon(fileread);  // Name...
    goto_colon(fileread);  // Reference...
    goto_colon(fileread);  // IDSZ...
    goto_colon(fileread);  // Import...
    goto_colon(fileread);  // Export...
    goto_colon(fileread);  // Min players...
    goto_colon(fileread);  // Max players...
    goto_colon(fileread);  // Respawn...
    goto_colon(fileread);  // RTS control...
    goto_colon(fileread);  // Rank...

    // Read the summary
    cnt = 0;
    while (cnt < SUMMARYLINES)
    {
      goto_colon(fileread);  fscanf(fileread, "%s", szLine);
      tnc = 0;
      cTmp = szLine[tnc];  if (cTmp == '_')  cTmp = ' ';
      while (tnc < SUMMARYSIZE-1 && cTmp != 0)
      {
        modsummary[cnt][tnc] = cTmp;
        tnc++;
        cTmp = szLine[tnc];  if (cTmp == '_')  cTmp = ' ';
      }
      modsummary[cnt][tnc] = 0;
      cnt++;
    }
    result = true;
  }
  fclose(fileread);
  return result;
}

