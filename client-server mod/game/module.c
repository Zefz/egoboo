/* Egoboo - module.c
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

#include "egoboo.h"
#include "Log.h"

//--------------------------------------------------------------------------------------------
bool_t init_mod_state(MOD_STATE * ms, MOD_INFO * mi, Uint32 seed)
{
  ms->beat           = bfalse;
  ms->nolocalplayers = btrue;

  ms->exportvalid  = mi->allowexport;
  ms->importamount = mi->importamount;
  ms->importvalid  = mi->importamount > 0;

  ms->respawnvalid = bfalse;
  ms->respawnanytime = bfalse;
  if (mi->respawnvalid) ms->respawnvalid = btrue;
  if (mi->respawnvalid == ANYTIME) ms->respawnanytime = btrue;
  ms->rts_control     = mi->rts_control;
  ms->net_messagemode = bfalse;
  ms->seed            = seed;

  return btrue;
};

//--------------------------------------------------------------------------------------------
void release_module(void)
{
  // ZZ> This function frees up memory used by the module
  release_all_textures();
  release_all_icons();
  release_map();

  // Close and then reopen SDL_mixer; it's easier than manually unloading each sound
  if (CData.musicvalid || CData.soundvalid)
  {
    Mix_CloseAudio();
    songplaying = -1;
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, CData.buffersize);
    Mix_AllocateChannels(CData.maxsoundchannel);
  }
}

//--------------------------------------------------------------------------------------------
bool_t module_reference_matches(char *szLoadName, IDSZ idsz)
{
  // ZZ> This function returns btrue if the named module has the required IDSZ
  FILE *fileread;
  STRING newloadname;
  IDSZ newidsz;
  bool_t foundidsz = bfalse;
  int cnt;


  if (szLoadName[0] == 'N' && szLoadName[1] == 'O' && szLoadName[2] == 'N' && szLoadName[3] == 'E' && szLoadName[4] == 0)
    return btrue;

  if (idsz == IDSZNONE)
    return btrue;


  foundidsz = bfalse;
  snprintf(newloadname, sizeof(newloadname), "%s/%s/%s/%s", CData.modules_dir, szLoadName, CData.gamedat_dir, CData.menu_file);
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
    while (goto_colon_yesno(fileread) && foundidsz == bfalse)
    {
      newidsz = get_idsz(fileread);
      if (newidsz == idsz)
      {
        foundidsz = btrue;
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
  STRING newloadname;

  // Only add if there isn't one already
  if (!module_reference_matches(szLoadName, idsz) == bfalse)
  {
    // Try to open the file in append mode
    snprintf(newloadname, sizeof(newloadname), "%s/%s/%s/%s", CData.modules_dir, szLoadName, CData.gamedat_dir, CData.menu_file);
    filewrite = fopen(newloadname, "a");
    if (filewrite)
    {
      fprintf(filewrite, "\n:[%s]\n", undo_idsz(idsz));
      fclose(filewrite);
    }
  }
}

//--------------------------------------------------------------------------------------------
int find_module(char *smallname, MOD_INFO * mi_ary, size_t mi_size)
{
  // ZZ> This function returns -1 if the module does not exist locally, the module
  //     index otherwise

  int cnt, index;
  cnt = 0;
  index = -1;
  while (cnt < mi_size)
  {
    if (0 == strcmp(smallname, mi_ary[cnt].loadname))
    {
      index = cnt;
      break;
    }
    cnt++;
  }

  return index;
}

//--------------------------------------------------------------------------------------------
void load_module(GAME_STATE * gs, char *smallname)
{
  // ZZ> This function loads a module
  STRING modname;
  Uint32 mod_randie = gs->ms->seed;

  gs->modstate.beat = bfalse;
  timeron = bfalse;
  snprintf(modname, sizeof(modname), "%s/%s/", gs->cd->modules_dir, smallname);

  reset_teams();

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->nullicon_bitmap);
  load_one_icon(CStringTmp1);

  load_global_waves(modname);

  reset_particles(modname);

  read_wawalite(modname, &mod_randie);

  make_twist();

  reset_messages(gs);

  prime_names();

  load_basic_textures(modname);

  reset_ai_script();


  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, gs->cd->gamedat_dir, gs->cd->script_file);
  if (!load_ai_script(CStringTmp1));
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->script_file);
    load_ai_script(CStringTmp1);
  };

  release_all_models();

  free_all_enchants();

  //printf("Got to load_all_objects\n");
  load_all_objects(gs, modname); // This is broken and needs to be fixed (is it really?)

  //snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", gs->cd->basicdat_dir, gs->cd->objects_dir, gs->cd->script_file);
  //load_one_object(0, CStringTmp1);

  if (!load_mesh(modname))
  {
    log_error("Load problems with the mesh.\n");
  }

  setup_particles();

  setup_passage(modname);

  reset_players(gs);

  setup_characters(gs, modname, &mod_randie);

  reset_end_text();

  setup_alliances(modname);

  // Load fonts and bars after other images, as not to hog videomem
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, gs->cd->gamedat_dir, gs->cd->font_bitmap);
  snprintf(CStringTmp2, sizeof(CStringTmp2), "%s%s/%s", modname, gs->cd->gamedat_dir, gs->cd->fontdef_file);
  if (!load_font(CStringTmp1, CStringTmp2))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->font_bitmap);
    snprintf(CStringTmp2, sizeof(CStringTmp2), "%s/%s", gs->cd->basicdat_dir, gs->cd->fontdef_file);
    if (!load_font(CStringTmp1, CStringTmp2))
    {
      log_warning("Fonts not loaded.  Files missing from %s directory", gs->cd->basicdat_dir);
    }
  };

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, gs->cd->gamedat_dir, gs->cd->bars_bitmap);
  if (!load_bars(CStringTmp1))
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->bars_bitmap);
    if (!load_bars(CStringTmp1))
    {
      log_warning("Could not load status bars. File missing = \"%s\"\n", CStringTmp1);
    }
  };

  load_map(modname);
  load_blip_bitmap(modname);

  if (gs->cd->DevMode)
  {
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", gs->cd->basicdat_dir, gs->cd->slotused_file);
    log_madused(CStringTmp1);
  };

}

//--------------------------------------------------------------------------------------------
retval_t get_module_data(int modnumber, char *szLoadName, MOD_INFO * mi_ary, size_t mi_size)
{
  // ZZ> This function loads the module data file
  FILE *fileread;
  char reference[128];
  IDSZ idsz;
  char cTmp;
  int iTmp;
  retval_t retval = rv_fail;

  fileread = fopen(szLoadName, "r");
  if (!fileread) return rv_error;

  // Read basic data
  globalname = szLoadName;

  mi_ary[modnumber].is_hosted   = bfalse;
  mi_ary[modnumber].is_verified = btrue;

  goto_colon(fileread);  get_name(fileread, mi_ary[modnumber].longname);
  goto_colon(fileread);  fscanf(fileread, "%s", reference);
  goto_colon(fileread);  idsz = get_idsz(fileread);
  if (module_reference_matches(reference, idsz))
  {
    globalname = szLoadName;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
    mi_ary[modnumber].importamount = iTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    mi_ary[modnumber].allowexport = bfalse;
    if (cTmp == 'T' || cTmp == 't')  mi_ary[modnumber].allowexport = btrue;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  mi_ary[modnumber].minplayers = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  mi_ary[modnumber].maxplayers = iTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    mi_ary[modnumber].respawnvalid = bfalse;
    if (cTmp == 'T' || cTmp == 't')  mi_ary[modnumber].respawnvalid = btrue;
    if (cTmp == 'A' || cTmp == 'a')  mi_ary[modnumber].respawnvalid = ANYTIME;

    goto_colon(fileread);  cTmp = get_first_letter(fileread); // Todo: this line can be removed
    mi_ary[modnumber].rts_control = bfalse;                    // as it is outdated.

    goto_colon(fileread);  fscanf(fileread, "%s", generictext);
    strncpy(mi_ary[modnumber].rank, generictext, sizeof(mi_ary[modnumber].rank));

    retval = rv_succeed;

    // Read any expansions
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
int get_module_summary(char *szLoadName, MOD_SUMMARY * ms)
{
  // ZZ> This function gets the quest description out of the module's menu file
  FILE *fileread;
  char cTmp;
  char szLine[160];
  int cnt;
  int tnc;
  bool_t result = bfalse;

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
    goto_colon(fileread);  // RTS control... (Outdated?)
    goto_colon(fileread);  // Rank...


    // Read the summary
    cnt = 0;
    while (cnt < SUMMARYLINES)
    {
      goto_colon(fileread);  fscanf(fileread, "%s", szLine);
      tnc = 0;
      cTmp = szLine[tnc];  if (cTmp == '_')  cTmp = ' ';
      while (tnc < SUMMARYSIZE - 1 && cTmp != 0)
      {
        ms->summary[cnt][tnc] = cTmp;
        tnc++;
        cTmp = szLine[tnc];  if (cTmp == '_')  cTmp = ' ';
      }
      ms->summary[cnt][tnc] = 0;
      cnt++;
    }
    result = btrue;
  }
  fclose(fileread);
  return result;
}



