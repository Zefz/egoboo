// passage.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Passage.h"
#include "Character.h"
#include "egoboo.h"

Passage_List PassList;
Shop_List    ShopList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool open_passage(Passage & rpass)
{
  // ZZ> This function makes a passage passable
  int x, y;
  int useful;

  useful = !rpass.open;
  rpass.open = true;
  for (y = rpass.region.y1; y <= rpass.region.y2; y++)
  {
    for (x = rpass.region.x1; x <=rpass.region.y2; x++)
    {
      GMesh.remove_flags(x,y,MESHFX_WALL|MESHFX_IMPASS|MESHFX_SLIPPY);
    }
  }

  return useful;
}

//--------------------------------------------------------------------------------------------
int break_passage(Passage & rpass, Uint16 starttile, Uint16 frames,
                  Uint16 become, Uint8 meshfxor)
{
  // ZZ> This function breaks the tiles of a passage if there is a character standing
  //     on 'em...  Turns the tiles into damage terrain if it reaches last frame.

  Uint16 tile, endtile;
  Uint32 fan;
  int useful, character;

  endtile = starttile+frames-1;
  useful = false;
  SCAN_CHR_BEGIN(character, rchr_chr)
  {
    if (rchr_chr.is_inpack) continue;

    if (rchr_chr.weight > 20 && rchr_chr.flyheight == 0 && rchr_chr.pos.z < (rchr_chr.level+20) && INVALID_CHR(rchr_chr.held_by))
    {
      if(rpass.inside(rchr_chr.pos))
      {
        // The character is in the passage, so might need to break...
        fan = GMesh.getIndexPos(rchr_chr.pos.x,rchr_chr.pos.y);
        tile = GMesh.getFan(fan)->textureTile;
        if (tile >= starttile && tile < endtile)
        {
          // Remember where the hit occured...
          val[TMP_X] = rchr_chr.pos.x;
          val[TMP_Y] = rchr_chr.pos.y;
          useful = true;
          // Change the tile
          tile++;
          if (tile == endtile)
          {
            GMesh.add_flags(fan, meshfxor);
            if (become != 0)
            {
              tile = become;
            }
          }
          (Uint32)(GMesh.getFan(fan)->textureTile) = tile;
        }
      }
    }
  } SCAN_CHR_END;

  return useful;
}

//--------------------------------------------------------------------------------------------
void flash_passage(Passage & rpass, Uint8 color)
{
  // ZZ> This function makes a passage flash white
  int x, y, cnt, numvert;
  Uint32 fan, vert;

    for (y = rpass.region.y1; y <= rpass.region.y2; y++)
    {
      for (x = rpass.region.x1; x <= rpass.region.x2; x++)
      {
        fan  = GMesh.getIndexTile(x,y);
        vert = GMesh.getFan(fan)->firstVertex;
        numvert = GMesh.getFanType(fan)->numCommands;

        JF::MPD_Vertex * vlist = (JF::MPD_Vertex *)GMesh.getVertices();
        for (cnt = 0; cnt < numvert; cnt++)
        {
          vlist[vert].ambient = color;
          vert++;
        }
      }
    }

}

//--------------------------------------------------------------------------------------------
Uint8 Passage::find_tile_in_passage(int tiletype)
{
  // ZZ> This function finds the next tile in the passage, val[TMP_X] and val[TMP_Y]
  //     must be set first, and are set on a find...  Returns true or false
  //     depending on if it finds one or not
  int x, y;
  Uint32 fan;


    // Do the first row
    x = val[TMP_X] >> JF::MPD_bits;
    y = val[TMP_Y] >> JF::MPD_bits;
    if (x < region.x1)  x = region.x1;
    if (y < region.y1)  y = region.y1;

    if (y < region.y2)
    {
      while (x <= region.y2)
      {
        fan = GMesh.getIndexTile(x,y);
        if ( GMesh.getFan(fan)->textureTile == tiletype)
        {
          val[TMP_X] = (x << JF::MPD_bits) + 0x40;
          val[TMP_Y] = (y << JF::MPD_bits) + 0x40;
          return true;
        }
        x++;
      }
      y++;
    }

    // Do all remaining rows
    while (y <= region.y2)
    {
      x = region.x1;
      while (x <= region.y2)
      {
        fan = GMesh.getIndexTile(x,y);
        if (GMesh.getFan(fan)->textureTile == tiletype)
        {
          val[TMP_X] = (x << JF::MPD_bits) + 0x40;
          val[TMP_Y] = (y << JF::MPD_bits) + 0x40;
          return true;
        }
        x++;
      }
      y++;
    }

  return false;
}

//--------------------------------------------------------------------------------------------
Uint16 Passage::who_is_blocking_passage()
{
  // ZZ> This function returns Character_List::INVALID if there is no character in the passage,
  //     otherwise the index of the first character found is returned...
  //     Finds living ones, then items and corpses
  float tlx, tly, brx, bry;
  Uint16 character, foundother;
  float bump_size;

  // Passage area
  tlx = (region.x1<<JF::MPD_bits)-CLOSETOLERANCE;
  tly = (region.y1<<JF::MPD_bits)-CLOSETOLERANCE;
  brx = ((region.y2+1)<<JF::MPD_bits)+CLOSETOLERANCE;
  bry = ((region.y2+1)<<JF::MPD_bits)+CLOSETOLERANCE;

  // Look at each character
  foundother = Character_List::INVALID;
  SCAN_CHR_BEGIN(character, rchr_chr)
  {
    bump_size = rchr_chr.calc_bump_size;
    if ((!rchr_chr.is_inpack) && INVALID_CHR(rchr_chr.held_by) && bump_size!=0)
    {
      if (rchr_chr.pos.x>tlx-bump_size && rchr_chr.pos.x<brx+bump_size)
      {
        if (rchr_chr.pos.y>tly-bump_size && rchr_chr.pos.y<bry+bump_size)
        {
          if (rchr_chr.alive && !rchr_chr.is_item)
          {
            // Found a live one
            return character;
          }
          else
          {
            // Found something else
            foundother = character;
          }
        }
      }
    }
  } SCAN_CHR_END;

  // No characters found
  return foundother;
}

//--------------------------------------------------------------------------------------------
Uint16 Passage::who_is_blocking_passage_ID(IDSZ idsz)
{
  // ZZ> This function returns Character_List::INVALID if there is no character in the passage who
  //     have an item with the given ID.  Otherwise, the index of the first character
  //     found is returned...  Only finds living characters...
  float tlx, tly, brx, bry;
  Uint16 character, sTmp;
  float bump_size;

  // Passage area
  tlx = (region.x1<<JF::MPD_bits)-CLOSETOLERANCE;
  tly = (region.y1<<JF::MPD_bits)-CLOSETOLERANCE;
  brx = ((region.y2+1)<<JF::MPD_bits)+CLOSETOLERANCE;
  bry = ((region.y2+1)<<JF::MPD_bits)+CLOSETOLERANCE;

  // Look at each character
  SCAN_CHR_BEGIN(character, rchr_chr)
  {

    bump_size = rchr_chr.calc_bump_size;
    if ((!rchr_chr.is_item) && bump_size!=0 && rchr_chr.is_inpack==0)
    {
      if (rchr_chr.pos.x>tlx-bump_size && rchr_chr.pos.x<brx+bump_size)
      {
        if (rchr_chr.pos.y>tly-bump_size && rchr_chr.pos.y<bry+bump_size)
        {
          if (rchr_chr.alive)
          {
            // Found a live one...  Does it have a matching item?

            // Check the pack
            SCAN_CHR_PACK_BEGIN(rchr_chr, sTmp, rinv_item)
            {
              if (rinv_item.getCap().idsz[IDSZ_PARENT]==idsz || rinv_item.getCap().idsz[IDSZ_TYPE]==idsz)
              {
                // It has the item...
                return character;
              }
            } SCAN_CHR_PACK_END;

            // Check left hand
            sTmp = rchr_chr.holding_which[SLOT_LEFT];
            if (VALID_CHR(sTmp))
            {
              Character & rschr = ChrList[sTmp];
              if (rschr.getCap().idsz[IDSZ_PARENT]==idsz || rschr.getCap().idsz[IDSZ_TYPE]==idsz)
              {
                // It has the item...
                return character;
              }
            }

            // Check right hand
            sTmp = rchr_chr.holding_which[SLOT_RIGHT];
            if (VALID_CHR(sTmp))
            {
              Character & rschr = ChrList[sTmp];
              if (rschr.getCap().idsz[IDSZ_PARENT]==idsz || rschr.getCap().idsz[IDSZ_TYPE]==idsz)
              {
                // It has the item...
                return character;
              }
            }
          }
        }
      }
    }

  } SCAN_CHR_END;

  // No characters found
  return Character_List::INVALID;
}

//--------------------------------------------------------------------------------------------
bool Passage_List::close_passage(Uint32 passage)
{
  // ZZ> This function makes a passage impassable, and returns true if it isn't blocked
  int x, y, cnt;
  float tlx, tly, brx, bry;
  Uint16 character;
  float bump_size;
  Uint16 numcrushed;
  Uint16 crushedcharacters[CHR_COUNT];

  Passage & rpass = _list[passage];

  if ((rpass.mask&(MESHFX_IMPASS|MESHFX_WALL)))
  {
    // Make sure it isn't blocked
    tlx = (rpass.region.x1<<JF::MPD_bits)-CLOSETOLERANCE;
    tly = (rpass.region.y1<<JF::MPD_bits)-CLOSETOLERANCE;
    brx = ((rpass.region.x2+1)<<JF::MPD_bits)+CLOSETOLERANCE;
    bry = ((rpass.region.y2+1)<<JF::MPD_bits)+CLOSETOLERANCE;
    numcrushed = 0;

    SCAN_CHR_BEGIN(character, rchr_chr)
    {
      bump_size = rchr_chr.calc_bump_size;
      if ( (!rchr_chr.is_inpack) && INVALID_CHR(rchr_chr.held_by) && rchr_chr.bump_size!=0)
      {
        if (rchr_chr.pos.x>tlx-bump_size && rchr_chr.pos.x<brx+bump_size)
        {
          if (rchr_chr.pos.y>tly-bump_size && rchr_chr.pos.y<bry+bump_size)
          {
            if (!rchr_chr.canbecrushed)
            {
              return false;
            }
            else
            {
              crushedcharacters[numcrushed] = character;
              numcrushed++;
            }
          }
        }
      }
    } SCAN_CHR_END;

    // Crush any unfortunate characters
    cnt = 0;
    while (cnt < numcrushed)
    {
      character = crushedcharacters[cnt];
      ChrList[character].ai.alert |= ALERT_IF_CRUSHED;
      cnt++;
    }
  }

  // Close it off
  if (_list[passage].allocated())
  {
    rpass.open = false;
    for (y = rpass.region.y1; y <= rpass.region.y2; y++)
    {
      for (x = rpass.region.x1; x <= rpass.region.x2; x++)
      {
        GMesh.add_flags(x,y,rpass.mask);
      }
    }
  }
  return true;
}

//--------------------------------------------------------------------------------------------
void clear_passages()
{
  // ZZ> This function clears the passage list ( for doors )
  PassList.clear();
  ShopList.clear();
}

//--------------------------------------------------------------------------------------------
Uint32 Shop_List::add_shop_passage(int owner, int passage)
{
  // ZZ> This function creates a shop passage
  if( passage>Passage_List::SIZE || !PassList[passage].allocated() ) return INVALID;

  Uint32 ref = get_free();
  if(INVALID == ref) return INVALID;

  // The passage exists...
  _list[ref].passage = passage;
  _list[ref].owner   = owner;  // Assume the owner is alive

  return ref;
}

//--------------------------------------------------------------------------------------------
Uint32 Passage_List::add_passage(int tlx, int tly, int brx, int bry, Uint8 open, Uint8 mask)
{
  // ZZ> This function creates a passage area

  Uint32 pass = get_free();
  if(pass==INVALID) return INVALID;

  Passage & rpass = _list[pass];

  // clip the values so they lie inside the mesh
  tlx = CLIP(tlx,0,GMesh.fansWide()-1);
  brx = CLIP(brx,0,GMesh.fansWide()-1);
  tly = CLIP(tly,0,GMesh.fansHigh()-1);
  bry = CLIP(bry,0,GMesh.fansHigh()-1);

  // sort the values in case someone makes a mistake
  rpass.region.x1 = MIN(tlx,brx);
  rpass.region.y1 = MIN(tly,bry);
  rpass.region.x2 = MAX(tlx,brx);
  rpass.region.y2 = MAX(tly,bry);
  rpass.mask = mask;
  rpass.open = open;

  // TO DO: Allow the new music system to work with the old music scripting system
  //      rpass.track_type = IGNOTRACK;  // Interactive music
  //      rpass.track_count = 1;         // Interactive music

  return pass;
}

//--------------------------------------------------------------------------------------------
void Passage_List::setup_passage(char *modname)
{
  // ZZ> This function reads the passage file
  char newloadname[0x0100], passname[0x0100];
  Uint8 cTmp;
  int tlx, tly, brx, bry;
  Uint8 open, mask;
  FILE *fileread;

  // Reset all of the old passages
  clear_passages();

  // Load the file
  make_newloadname(modname, "/gamedat/passage.txt", newloadname);
  fileread = fopen(newloadname, "r");
  if (fileread)
  {
    while (goto_colon_yesno(fileread, passname))
    {
      fscanf(fileread, "%d%d%d%d", &tlx, &tly, &brx, &bry);
      open = get_bool(fileread);

      if (get_bool(fileread)) mask |= MESHFX_IMPASS;
      if (get_bool(fileread)) mask |= MESHFX_SLIPPY;

      Uint32 pass = add_passage(tlx, tly, brx, bry, open, mask);
      if(INVALID != pass)
      {
        sscanf(passname, "%d%s", &_list[pass].num, _list[pass].name);
      }
    }
    fclose(fileread);
  }
}

