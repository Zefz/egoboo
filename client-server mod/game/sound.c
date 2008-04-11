/* Egoboo - sound.c
* Sound code in Egoboo is implemented using SDL_mixer.
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

//This function enables the use of SDL_Mixer functions, returns btrue if success
bool_t sdlmixer_initialize()
{
  if ((CData.musicvalid || CData.soundvalid) && !mixeron)
  {
    log_info("Initializing SDL_mixer audio services... ");
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, CData.buffersize);
    Mix_VolumeMusic(CData.musicvolume);
    Mix_AllocateChannels(CData.maxsoundchannel);
    if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, CData.buffersize) != 0)
    {
	  log_message("Failed!\n");
      log_error("Unable to initialize audio: %s\n", Mix_GetError());
    }
	else log_message("Succeeded!!\n");
    mixeron = btrue;
    return btrue;
  }
  else return bfalse;
}

//------------------------------------
//SOUND-------------------------------
//------------------------------------
int play_sound(float xpos, float ypos, Mix_Chunk *loadedwave)
{
  //This function plays a specified sound
  if (CData.soundvalid)
  {
    int distance, volume, pan;
    distance = sqrt(pow(ABS(GCamera.trackx - xpos), 2) + pow(ABS(GCamera.tracky - ypos), 2)); //Ugly, but just the distance formula
    volume = ((distance / VOLUMERATIO) * (1 + (CData.soundvolume / 100))); //adjust volume with ratio and sound volume
    pan = RAD_TO_DEG * ((1.5 * PI) - atan2(GCamera.y - ypos, GCamera.x - xpos) - GCamera.turnleftright); //Convert the camera angle to the nearest integer degree
    if (pan < 0) pan += 360;
    if (pan > 360) pan -= 360;
    if (volume < 255)
    {
      if (loadedwave == NULL)
      {
        log_warning("Sound file not correctly loaded (Not found?).\n");
      }

      channel = Mix_PlayChannel(-1, loadedwave, 0);
      if (channel != -1)
      {
        Mix_SetDistance(channel, volume);
        if (pan < 180)
        {
          if (pan < 90) Mix_SetPanning(channel, 255 - (pan * 2.83), 255);
          else Mix_SetPanning(channel, 255 - ((180 - pan) * 2.83), 255);
        }
        else
        {
          if (pan < 270) Mix_SetPanning(channel, 255, 255 - ((pan - 180) * 2.83));
          else Mix_SetPanning(channel, 255, 255 - ((360 - pan) * 2.83));
        }
      }
      else log_warning("All sound channels are currently in use. Sound is NOT playing.\n");
    }
  }
  return channel;
}

//TODO:
void stop_sound(int whichchannel)
{
  if (CData.soundvalid) Mix_HaltChannel(whichchannel);
}

//------------------------------------------------------------------------------
//Music Stuff-------------------------------------------------------------------
//------------------------------------------------------------------------------
void load_all_music_sounds()
{
  //This function loads all of the music sounds
  STRING songname;
  FILE *playlist;
  int cnt;

  //Open the playlist listing all music files
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.music_dir, CData.playlist_file);
  playlist = fopen(CStringTmp1, "r");
  if (playlist == NULL) log_warning("Error opening playlist.txt\n");

  // Load the music data into memory
  if (CData.musicvalid && !musicinmemory)
  {
    cnt = 0;
    while (cnt < MAXPLAYLISTLENGHT && !feof(playlist))
    {
      goto_colon_yesno(playlist);
      fscanf(playlist, "%s", songname);
      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.music_dir, songname);
      instrumenttosound[cnt] = Mix_LoadMUS(CStringTmp1);
      cnt++;
    }
    musicinmemory = btrue;
  }
  fclose(playlist);
}

void play_music(int songnumber, int fadetime, int loops)
{
  //This functions plays a specified track loaded into memory
  if (songplaying != songnumber && CData.musicvalid)
  {
    Mix_FadeOutMusic(fadetime);
    Mix_PlayMusic(instrumenttosound[songnumber], loops);
    songplaying = songnumber;
  }
}

void stop_music()
{
  //This function sets music track to pause
  if (CData.musicvalid)
  {
    Mix_HaltMusic();
  }
}
