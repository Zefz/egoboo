/* Egoboo - passage.c
* Passages and doors and whatnot.  Things that impede your progress!
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

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int open_passage(int passage)
{
  // ZZ> This function makes a passage passable
  int x, y;
  Uint32 fan;
  int useful;


  useful = bfalse;
  if (passage < numpassage)
  {
    useful = (passopen[passage] == bfalse);
    passopen[passage] = btrue;
    y = passtly[passage];
    while (y <= passbry[passage])
    {
      x = passtlx[passage];
      while (x <= passbrx[passage])
      {
        fan = Mesh.fanstart[y] + x;
        Mesh.fanlist[fan].fx = Mesh.fanlist[fan].fx & (255 - MESHFXWALL - MESHFXIMPASS - MESHFXSLIPPY);
        x++;
      }
      y++;
    }
  }
  return useful;
}

//--------------------------------------------------------------------------------------------
int break_passage(int passage, Uint16 starttile, Uint16 frames,
                  Uint16 become, Uint8 meshfxor)
{
  // ZZ> This function breaks the tiles of a passage if there is a character standing
  //     on 'em...  Turns the tiles into damage terrain if it reaches last frame.
  int x, y;
  Uint16 tile, endtile;
  Uint32 fan;
  int useful, character;


  endtile = starttile + frames - 1;
  useful = bfalse;
  if (passage < numpassage)
  {
    character = 0;
    while (character < MAXCHR)
    {
      if (ChrList[character].on && !ChrList[character].inpack)
      {
        if (ChrList[character].weight > 20 && ChrList[character].flyheight == 0 && ChrList[character].zpos < (ChrList[character].level + 20) && ChrList[character].attachedto == MAXCHR)
        {
          x = ChrList[character].xpos;  x = x >> 7;
          if (x >= passtlx[passage] && x <= passbrx[passage])
          {
            y = ChrList[character].ypos;  y = y >> 7;
            if (y >= passtly[passage] && y <= passbry[passage])
            {
              // The character is in the passage, so might need to break...
              fan = Mesh.fanstart[y] + x;
              tile = Mesh.fanlist[fan].tile;
              if (tile >= starttile && tile < endtile)
              {
                // Remember where the hit occured...
                valuetmpx = ChrList[character].xpos;
                valuetmpy = ChrList[character].ypos;
                useful = btrue;
                // Change the tile
                tile++;
                if (tile == endtile)
                {
                  Mesh.fanlist[fan].fx |= meshfxor;
                  if (become != 0)
                  {
                    tile = become;
                  }
                }
                Mesh.fanlist[fan].tile = tile;
              }
            }
          }
        }
      }
      character++;
    }
  }
  return useful;
}

//--------------------------------------------------------------------------------------------
void flash_passage(int passage, Uint8 color)
{
  // ZZ> This function makes a passage flash white
  int x, y, cnt, numvert;
  Uint32 fan, vert;


  if (passage < numpassage)
  {
    y = passtly[passage];
    while (y <= passbry[passage])
    {
      x = passtlx[passage];
      while (x <= passbrx[passage])
      {
        fan = Mesh.fanstart[y] + x;
        vert = Mesh.fanlist[fan].vrtstart;
        cnt = 0;
        numvert = Mesh.tilelist[Mesh.fanlist[fan].type].commandnumvertices;
        while (cnt < numvert)
        {
          Mesh.vrta[vert] = color;
          vert++;
          cnt++;
        }
        x++;
      }
      y++;
    }
  }
}

//--------------------------------------------------------------------------------------------
Uint8 find_tile_in_passage(int passage, int tiletype)
{
  // ZZ> This function finds the next tile in the passage, valuetmpx and valuetmpy
  //     must be set first, and are set on a find...  Returns btrue or bfalse
  //     depending on if it finds one or not
  int x, y;
  Uint32 fan;


  if (passage < numpassage)
  {
    // Do the first row
    x = valuetmpx >> 7;
    y = valuetmpy >> 7;
    if (x < passtlx[passage])  x = passtlx[passage];
    if (y < passtly[passage])  y = passtly[passage];
    if (y < passbry[passage])
    {
      while (x <= passbrx[passage])
      {
        fan = Mesh.fanstart[y] + x;
        if (Mesh.fanlist[fan].tile == tiletype)
        {
          valuetmpx = (x << 7) + 64;
          valuetmpy = (y << 7) + 64;
          return btrue;
        }
        x++;
      }
      y++;
    }


    // Do all remaining rows
    while (y <= passbry[passage])
    {
      x = passtlx[passage];
      while (x <= passbrx[passage])
      {
        fan = Mesh.fanstart[y] + x;
        if (Mesh.fanlist[fan].tile == tiletype)
        {
          valuetmpx = (x << 7) + 64;
          valuetmpy = (y << 7) + 64;
          return btrue;
        }
        x++;
      }
      y++;
    }
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage(int passage)
{
  // ZZ> This function returns MAXCHR if there is no character in the passage,
  //     otherwise the index of the first character found is returned...
  //     Finds living ones, then items and corpses
  float tlx, tly, brx, bry;
  Uint16 character, foundother;
  float bumpsize;


  // Passage area
  tlx = (passtlx[passage] << 7) - CLOSETOLERANCE;
  tly = (passtly[passage] << 7) - CLOSETOLERANCE;
  brx = ((passbrx[passage] + 1) << 7) + CLOSETOLERANCE;
  bry = ((passbry[passage] + 1) << 7) + CLOSETOLERANCE;


  // Look at each character
  foundother = MAXCHR;
  character = 0;
  while (character < MAXCHR)
  {
    if (ChrList[character].on)
    {
      bumpsize = ChrList[character].bumpsize;
      if ((!ChrList[character].inpack) && ChrList[character].attachedto == MAXCHR && bumpsize != 0)
      {
        if (ChrList[character].xpos > tlx - bumpsize && ChrList[character].xpos < brx + bumpsize)
        {
          if (ChrList[character].ypos > tly - bumpsize && ChrList[character].ypos < bry + bumpsize)
          {
            if (ChrList[character].alive && ChrList[character].isitem == bfalse)
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
    }
    character++;
  }



  // No characters found
  return foundother;
}

//--------------------------------------------------------------------------------------------
void check_passage_music()
{
  //This function checks all passages if there is a player in it, if it is, it plays a specified
  //song set in by the AI script functions
  float tlx, tly, brx, bry;
  Uint16 character, passage;
  float bumpsize;

  passage = 0;
  while (passage < numpassage)
  {
    if (passagemusic[passage] != -1)    //Only check passages that have music assigned to them
    {
      // Passage area
      tlx = (passtlx[passage] << 7) - CLOSETOLERANCE;
      tly = (passtly[passage] << 7) - CLOSETOLERANCE;
      brx = ((passbrx[passage] + 1) << 7) + CLOSETOLERANCE;
      bry = ((passbry[passage] + 1) << 7) + CLOSETOLERANCE;

      // Look at each character
      character = 0;

      while (character < MAXCHR)
      {
        if (ChrList[character].on)
        {
          bumpsize = ChrList[character].bumpsize;
          if ((!ChrList[character].inpack) && ChrList[character].attachedto == MAXCHR && bumpsize != 0)
          {
            if (ChrList[character].xpos > tlx - bumpsize && ChrList[character].xpos < brx + bumpsize)
            {
              if (ChrList[character].ypos > tly - bumpsize && ChrList[character].ypos < bry + bumpsize)
              {
                if (ChrList[character].alive && ChrList[character].isitem == bfalse && ChrList[character].isplayer)
                {
                  //Found a player, start music track
                  play_music(passagemusic[passage], 0, -1);
                }
              }
            }
          }
        }
        character++;
      }
    }
    passage++;
  }
}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage_ID(int passage, IDSZ idsz)
{
  // ZZ> This function returns MAXCHR if there is no character in the passage who
  //     have an item with the given ID.  Otherwise, the index of the first character
  //     found is returned...  Only finds living characters...
  float tlx, tly, brx, bry;
  Uint16 character, sTmp;
  float bumpsize;


  // Passage area
  tlx = (passtlx[passage] << 7) - CLOSETOLERANCE;
  tly = (passtly[passage] << 7) - CLOSETOLERANCE;
  brx = ((passbrx[passage] + 1) << 7) + CLOSETOLERANCE;
  bry = ((passbry[passage] + 1) << 7) + CLOSETOLERANCE;


  // Look at each character
  character = 0;
  while (character < MAXCHR)
  {
    if (ChrList[character].on)
    {
      bumpsize = ChrList[character].bumpsize;
      if ((!ChrList[character].isitem) && bumpsize != 0 && ChrList[character].inpack == 0)
      {
        if (ChrList[character].xpos > tlx - bumpsize && ChrList[character].xpos < brx + bumpsize)
        {
          if (ChrList[character].ypos > tly - bumpsize && ChrList[character].ypos < bry + bumpsize)
          {
            if (ChrList[character].alive)
            {
              // Found a live one...  Does it have a matching item?

              // Check the pack
              sTmp = ChrList[character].nextinpack;
              while (sTmp != MAXCHR)
              {
                if (CapList[ChrList[sTmp].model].idsz[IDSZPARENT] == idsz || CapList[ChrList[sTmp].model].idsz[IDSZTYPE] == idsz)
                {
                  // It has the item...
                  return character;
                }
                sTmp = ChrList[sTmp].nextinpack;
              }
              // Check left hand
              sTmp = ChrList[character].holdingwhich[0];
              if (sTmp != MAXCHR)
              {
                sTmp = ChrList[sTmp].model;
                if (CapList[sTmp].idsz[IDSZPARENT] == idsz || CapList[sTmp].idsz[IDSZTYPE] == idsz)
                {
                  // It has the item...
                  return character;
                }
              }
              // Check right hand
              sTmp = ChrList[character].holdingwhich[1];
              if (sTmp != MAXCHR)
              {
                sTmp = ChrList[sTmp].model;
                if (CapList[sTmp].idsz[IDSZPARENT] == idsz || CapList[sTmp].idsz[IDSZTYPE] == idsz)
                {
                  // It has the item...
                  return character;
                }
              }
            }
          }
        }
      }
    }
    character++;
  }



  // No characters found
  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
int close_passage(int passage)
{
  // ZZ> This function makes a passage impassable, and returns btrue if it isn't blocked
  int x, y, cnt;
  float tlx, tly, brx, bry;
  Uint32 fan;
  Uint16 character;
  float bumpsize;
  Uint16 numcrushed;
  Uint16 crushedcharacters[MAXCHR];


  if ((passmask[passage]&(MESHFXIMPASS | MESHFXWALL)))
  {
    // Make sure it isn't blocked
    tlx = (passtlx[passage] << 7) - CLOSETOLERANCE;
    tly = (passtly[passage] << 7) - CLOSETOLERANCE;
    brx = ((passbrx[passage] + 1) << 7) + CLOSETOLERANCE;
    bry = ((passbry[passage] + 1) << 7) + CLOSETOLERANCE;
    numcrushed = 0;
    character = 0;
    while (character < MAXCHR)
    {
      bumpsize = ChrList[character].bumpsize;
      if (ChrList[character].on && (!ChrList[character].inpack) && ChrList[character].attachedto == MAXCHR && ChrList[character].bumpsize != 0)
      {
        if (ChrList[character].xpos > tlx - bumpsize && ChrList[character].xpos < brx + bumpsize)
        {
          if (ChrList[character].ypos > tly - bumpsize && ChrList[character].ypos < bry + bumpsize)
          {
            if (ChrList[character].canbecrushed == bfalse)
            {
              return bfalse;
            }
            else
            {
              crushedcharacters[numcrushed] = character;
              numcrushed++;
            }
          }
        }
      }
      character++;
    }


    // Crush any unfortunate characters
    cnt = 0;
    while (cnt < numcrushed)
    {
      character = crushedcharacters[cnt];
      ChrList[character].alert |= ALERTIFCRUSHED;
      cnt++;
    }
  }

  // Close it off
  if (passage < numpassage)
  {
    passopen[passage] = bfalse;
    y = passtly[passage];
    while (y <= passbry[passage])
    {
      x = passtlx[passage];
      while (x <= passbrx[passage])
      {
        fan = Mesh.fanstart[y] + x;
        Mesh.fanlist[fan].fx = Mesh.fanlist[fan].fx | passmask[passage];
        x++;
      }
      y++;
    }
  }
  return btrue;
}

//--------------------------------------------------------------------------------------------
void clear_passages()
{
  // ZZ> This function clears the passage list ( for doors )
  numpassage = 0;
  numshoppassage = 0;
}

//--------------------------------------------------------------------------------------------
void add_shop_passage(int owner, int passage)
{
  // ZZ> This function creates a shop passage
  if (passage < numpassage && numshoppassage < MAXPASS)
  {
    // The passage exists...
    shoppassage[numshoppassage] = passage;
    shopowner[numshoppassage] = owner;  // Assume the owner is alive
    numshoppassage++;
  }
}

//--------------------------------------------------------------------------------------------
void add_passage(int tlx, int tly, int brx, int bry, Uint8 open, Uint8 mask)
{
  // ZZ> This function creates a passage area
  if (tlx < 0)  tlx = 0;
  if (tlx > Mesh.sizex - 1)  tlx = Mesh.sizex - 1;
  if (tly < 0)  tly = 0;
  if (tly > Mesh.sizey - 1)  tly = Mesh.sizey - 1;
  if (brx < 0)  brx = 0;
  if (brx > Mesh.sizex - 1)  brx = Mesh.sizex - 1;
  if (bry < 0)  bry = 0;
  if (bry > Mesh.sizey - 1)  bry = Mesh.sizey - 1;
  if (numpassage < MAXPASS)
  {
    passtlx[numpassage] = tlx;
    passtly[numpassage] = tly;
    passbrx[numpassage] = brx;
    passbry[numpassage] = bry;
    passmask[numpassage] = mask;
    passagemusic[numpassage] = -1;     //Set no song as default
    numpassage++;
    if (open)
      passopen[numpassage-1] = btrue;
    else
      passopen[numpassage-1] = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
void setup_passage(char *modname)
{
  // ZZ> This function reads the passage file
  STRING newloadname;
  Uint8 cTmp;
  int tlx, tly, brx, bry;
  Uint8 open, mask;
  FILE *fileread;


  // Reset all of the old passages
  clear_passages();


  // Load the file
  snprintf(newloadname, sizeof(newloadname), "%s%s/%s", modname, CData.gamedat_dir, CData.passage_file);
  fileread = fopen(newloadname, "r");
  if (fileread)
  {
    while (goto_colon_yesno(fileread))
    {
      fscanf(fileread, "%d%d%d%d", &tlx, &tly, &brx, &bry);
      cTmp = get_first_letter(fileread);
      open = bfalse;
      if (cTmp == 'T' || cTmp == 't') open = btrue;
      cTmp = get_first_letter(fileread);
      mask = MESHFXIMPASS | MESHFXWALL;
      if (cTmp == 'T' || cTmp == 't') mask = MESHFXIMPASS;
      cTmp = get_first_letter(fileread);
      if (cTmp == 'T' || cTmp == 't') mask = MESHFXSLIPPY;
      add_passage(tlx, tly, brx, bry, open, mask);
    }
    fclose(fileread);
  }
}

