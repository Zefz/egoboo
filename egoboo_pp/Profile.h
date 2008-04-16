#pragma once

#include "Script.h"
#include "Mad.h"
#include "Character.h"
#include "Particle.h"
#include "Enchant.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CHOPPERMODEL                    0x20          //
#define MAXCHOP                         (MODEL_COUNT*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)

enum PRTPIP_TYPE
{
  PRTPIP_COIN_001            = 0,                       // Coins are the first particles loaded
  PRTPIP_COIN_005            ,                       //
  PRTPIP_COIN_025            ,                       //
  PRTPIP_COIN_100            ,                       //
  PRTPIP_WEATHER_1           ,                       // Weather particles
  PRTPIP_WEATHER_2           ,                       // Weather particle finish
  PRTPIP_SPLASH              ,                       // Water effects are next
  PRTPIP_RIPPLE              ,                       //
  PRTPIP_DEFEND              ,                       // Defend particle
  PRTPIP_COUNT                                       //
};

//--------------------------------------------------------------------------------------------

// This is for random naming
struct Chop_Data
{
  Uint16          count;                // The number of name parts
  Uint16          write;              // The data pointer
  char            data[CHOPDATACHUNK];    // The name parts
  Uint16          start[MAXCHOP];         // The first character of each part
  char            name[MAXCAPNAMESIZE];// The name returned by the function

  Chop_Data() { reset(); }
  void reset() {count = 0; write = 0;}
};

//--------------------------------------------------------------------------------------------

struct Profile : public TAllocClient<Profile, MODEL_COUNT>
{
  char   name[0x80];   // profile object name35VGRW2 RG
  char   filename[0x0100];
  bool   loaded;

  MAD_REF mad_ref;     // MD2 model data
  CAP_REF cap_ref;     // default character properties
  EVE_REF eve_ref;     // default enchantment properties
  AI_REF  scr_ref;

  Uint16  msg_start;                     // The first message

  // chop info
  Uint16 sectionsize[MAXSECTION];   // Number of choices, 0
  Uint16 sectionstart[MAXSECTION];  //
  void read_naming(char *szPathname);
  void naming_names();

  Mix_Chunk    *waveindex[MAXWAVE];            // Sounds, -1 for none

  TEX_REF       skin_ref[4];                         // Number of skins
  ICON_REF      icon_ref[4];                         // Number of skins

  Pip_List::index_t prtpip[PRTPIP_COUNT]; // Local particle profiles

  void load_pips(const char * pathname);
  void load_all_messages(char *loadname);

  void reset();

  void clear_icons()
  {
    for(int i=0; i<4; i++)
    {
      icon_ref[i] = ICON_REF(Icon_List::INVALID);
    }
  };

};


//--------------------------------------------------------------------------------------------

struct Profile_List : public TAllocList<Profile, MODEL_COUNT>
{
  Uint32 load_one_object(char* tmploadname, int & skin);
  //Uint32 load_one_particle(const char *szLoadName);

  void load_all_objects(char *modname);

  void release_all_models();
  void release_local_pips();

  void prime_names(void);

  Uint32 get_free(Uint32 force = INVALID)
  {
    Uint32 ref = my_alist_type::get_free(force);
    if(INVALID == ref) return INVALID;
    _list[ref].reset();
    return ref;
  };

  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;

    //remove this profile from any and all references in the slot_list
    for(int j=0;j<MODEL_COUNT;j++)
    {
      if(slot_list[j]==i) slot_list[j] = INVALID;
    }

    _list[i].loaded = false;
    _list[i].filename[0] = 0;
    my_alist_type::return_one(i);
  }

  Sint32 slot_list[MODEL_COUNT];

  void clear_icons()
  {
    for(int cnt =0; cnt<SIZE; cnt++)
      _list[cnt].clear_icons();
  }

  Profile_List() { _setup(); };

  void reset() { _setup(); my_alist_type::_setup(); }

protected:
  void _setup()
  {
    for(int i=0; i<MODEL_COUNT; i++)
      slot_list[i] = INVALID;
  }
};

//--------------------------------------------------------------------------------------------

extern Chop_Data GChops;
extern Profile_List ProfileList;