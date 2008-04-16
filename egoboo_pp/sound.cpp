// sound.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Camera.h"
#include "egoboo.h"

//------------------------------------
//SOUND-------------------------------
//------------------------------------
int play_sound(vec3_t & pos, Mix_Chunk *loadedwave)
{
  //This function plays a specified sound
  if (soundvalid)
  {
    float intensity = 0x80*0x80 / (0x80*0x80 + pow(ABS(GCamera.track.x-pos.x), 2) + pow(ABS(GCamera.track.y-pos.y), 2)); //Ugly, but just the distance formula
    float volume = 0xFF*(1.0-intensity*soundvolume/100); //adjust volume with ratio and sound volume
    if (volume < 0) volume = 0;
    if (ceil(volume) >= 0xFF) return channel;

    int pan = 57.3 * (atan_tab.lookup(GCamera.pos.x-pos.x,GCamera.pos.y-pos.y) - GCamera.turn_lr/float(1<<16)*TWO_PI); //Convert the camera angle to the nearest integer degree
    pan %= 360;
    if (pan < 0) pan += 360;
    if (pan > 360) pan -= 360;


    if (loadedwave == NULL)
    {
      log_warning("Sound file not correctly loaded (Not found?).\n");
    }

    channel = Mix_PlayChannel(-1, loadedwave, 0);
    if (channel !=-1)
    {
      Mix_SetDistance(channel, volume);
      if (pan < 180)
      {
        if (pan < 90) Mix_SetPanning(channel, 0xFF - (pan * 2.83),0xFF);
        else Mix_SetPanning(channel, 0xFF - ((180 - pan) * 2.83),0xFF);
      }
      else
      {
        if (pan < 270) Mix_SetPanning(channel, 0xFF, 0xFF - ((pan - 180) * 2.83));
        else Mix_SetPanning(channel, 0xFF, 0xFF - ((360 - pan) * 2.83));
      }
    }
    else log_warning("All sound channels are currently in use. Sound is NOT playing.\n");

  }

  return channel;
}

//TODO:
void stop_sound(int whichchannel)
{
  if (soundvalid) Mix_HaltChannel(whichchannel);
}

//------------------------------------------------------------------------------
//Music Stuff-------------------------------------------------------------------
//------------------------------------------------------------------------------
void load_all_music_sounds()
{
  //This function loads all of the music sounds
  char loadpath[0x80];
  char songname[0x80];
  FILE *playlist;
  int cnt;

  //Open the playlist listing all music files
  playlist = fopen("basicdat/music/playlist.txt", "r");
  if (playlist == NULL) log_warning("Error opening playlist.txt");

  // Load the music data into memory
  musicinmemory = false;
  if (musicvalid)
  {
    cnt = 0;
    while (cnt < PLAYLISTLENGHT && !feof(playlist))
    {
      goto_colon_yesno(playlist);
      fscanf(playlist, "%s", songname);
      sprintf(loadpath,("basicdat/music/%s"), songname);
      instrumenttosound[cnt] = Mix_LoadMUS(loadpath);
      cnt++;
    }
    musicinmemory = true;
  }
  fclose(playlist);
}

void play_music(int songnumber, int fadetime, int loops)
{
  //This functions plays a specified track loaded into memory
  if (songplaying != songnumber && musicvalid)
  {
    Mix_FadeOutMusic(fadetime);
    Mix_PlayMusic(instrumenttosound[songnumber], loops);
    songplaying = songnumber;
  }
}

void stop_music()
{
  //This function stops playing music
  if (musicvalid) Mix_HaltMusic();
}