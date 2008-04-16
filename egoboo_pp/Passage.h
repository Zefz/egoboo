#pragma once

#include "egobootypedef.h"
#include "mathstuff.h"
#include "MPD_file.h"

#define MAXPASS             0x0100                     // Maximum number of passages ( mul 0x20 )


// For opening doors
struct Passage_Region
{
  Sint32 x1,y1,x2,y2;
};

struct Passage : public TAllocClient<Passage, MAXPASS>
{
  Uint8  num;
  char   name[0x0100];

  Uint8  mask;
  bool   open;
  Passage_Region region;

  // TO DO: Allow the new music system to work with the old music scripting system
  //Uint8 track_type;
  //Uint8 track_count;

  Uint8  find_tile_in_passage(int tiletype);
  Uint16 who_is_blocking_passage();
  Uint16 who_is_blocking_passage_ID(IDSZ idsz);

  bool inside(vec3_t & pos)
  {
    int loc = int(pos.x) >> JF::MPD_bits;
    if (loc >= region.x1 && loc <= region.x2)
    {
      loc = int(pos.y) >> JF::MPD_bits;
      if (loc >= region.y1 && loc <= region.y2)
      {
        return true;
      }
    }
    return false;
  }

};

struct Passage_List : public TAllocList<Passage, MAXPASS>
{
  Uint32 add_passage(int tlx, int tly, int brx, int bry, Uint8 open, Uint8 mask);
  bool   close_passage(Uint32 passage);

  void clear() { _setup(); }
  void setup_passage(char *modname);
};

extern Passage_List PassList;

#define _VALID_PASS_RANGE(XX) ( (XX)>=0 && (XX)<Passage_List::SIZE )
#define VALID_PASS(XX) ( _VALID_PASS_RANGE(XX) && PassList[XX].allocated() )
#define INVALID_PASS(XX) ( !VALID_PASS(XX) )


// For shops
struct Shop : public TAllocClient<Shop, MAXPASS>
{
  Uint16 passage;   // The passage reference
  Uint32 owner;     // Who gets the gold?
};

struct Shop_List : public TAllocList<Shop, MAXPASS>
{
  Uint32 add_shop_passage(int owner, int passage);

  void clear_passages(Uint32 character)
  {
    // Clear all shop passages that it owned...
    for (int tnc=0; tnc<SIZE; tnc++)
    {
      if(!_list[tnc].allocated()) continue;

      if (_list[tnc].owner == character)
      {
        _list[tnc].owner = Shop_List::INVALID;
      }
    }
  };

  Uint32 inside(vec3_t & pos)
  {
    for (int cnt=0; cnt < SIZE; cnt++)
    {
      if(!_list[cnt].allocated()) continue;

      Uint32 passage = _list[cnt].passage;
      if(PassList[passage].inside(pos))
        return cnt;
    }

    return INVALID;
  };

  void clear() { _setup(); }
};

extern Shop_List ShopList;