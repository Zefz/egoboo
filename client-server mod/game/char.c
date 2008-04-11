/* Egoboo - char.c
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

#include "mathstuff.h"
#include "network.h"
#include "Client.h"
#include "Server.h"
#include "Log.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void flash_character_height(int character, Uint8 valuelow, Sint16 low,
                            Uint8 valuehigh, Sint16 high)
{
  // ZZ> This function sets a character's lighting depending on vertex height...
  //     Can make feet dark and head light...
  int cnt;
  Uint16 frame;
  Sint16 z;


  frame = ChrList[character].frame;
  cnt = 0;
  while (cnt < MadList[ChrList[character].model].transvertices)
  {
    z = MadFrame[frame].vrtz[cnt];
    if (z < low)
    {
      ChrList[character].vrta[cnt] = valuelow;
    }
    else
    {
      if (z > high)
      {
        ChrList[character].vrta[cnt] = valuehigh;
      }
      else
      {
        ChrList[character].vrta[cnt] = (valuehigh * (z - low) / (high - low)) +
          (valuelow * (high - z) / (high - low));
      }
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void flash_character(int character, Uint8 value)
{
  // ZZ> This function sets a character's lighting
  int cnt;

  cnt = 0;
  while (cnt < MadList[ChrList[character].model].transvertices)
  {
    ChrList[character].vrta[cnt] = value;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void flash_select()
{
  /*   // ZZ> This function makes the selected characters blink
  int cnt;
  Uint8 value;

  if((wldframe&31)==0 && allselect==bfalse)
  {
  value = ((wldframe&32)<<3) - ((wldframe&32)>>5);
  cnt = 0;
  while(cnt < numrtsselect)
  {
  flash_character(GRTS.select[cnt], value);
  cnt++;
  }
  }*/
}

//--------------------------------------------------------------------------------------------
void add_to_dolist(GAME_STATE * gs, int cnt)
{
  // This function puts a character in the list
  int fan;


  if (!ChrList[cnt].indolist)
  {
    fan = ChrList[cnt].onwhichfan;
    if (Mesh.fanlist[fan].inrenderlist)
    {
      //ChrList[cnt].lightspek = Mesh[meshvrtstart[fan]].vrtl;
      dolist[numdolist] = cnt;
      ChrList[cnt].indolist = btrue;
      numdolist++;


      // Do flashing
      if ((allframe&ChrList[cnt].flashand) == 0 && ChrList[cnt].flashand != DONTFLASH)
      {
        flash_character(cnt, 255);
      }
      // Do blacking
      if ((allframe&SEEKURSEAND) == 0 && gs->cs->seekurse && ChrList[cnt].iskursed)
      {
        flash_character(cnt, 0);
      }
    }
    else
    {
      // Double check for large/special objects
      if (CapList[ChrList[cnt].model].alwaysdraw)
      {
        dolist[numdolist] = cnt;
        ChrList[cnt].indolist = btrue;
        numdolist++;
      }
    }
    // Add its weapons too
    if (ChrList[cnt].holdingwhich[0] != MAXCHR)
      add_to_dolist(gs, ChrList[cnt].holdingwhich[0]);
    if (ChrList[cnt].holdingwhich[1] != MAXCHR)
      add_to_dolist(gs, ChrList[cnt].holdingwhich[1]);
  }
}

//--------------------------------------------------------------------------------------------
void order_dolist(void)
{
  // ZZ> This function GOrder.s the dolist based on distance from camera,
  //     which is needed for reflections to properly clip themselves.
  //     Order from closest to farthest
  int cnt, tnc, character, order;
  int dist[MAXCHR];
  Uint16 olddolist[MAXCHR];


  // Figure the distance of each
  cnt = 0;
  while (cnt < numdolist)
  {
    character = dolist[cnt];  olddolist[cnt] = character;
    if (ChrList[character].light != 255 || ChrList[character].alpha != 255)
    {
      // This makes stuff inside an invisible character visible...
      // A key inside a Jellcube, for example
      dist[cnt] = 0x7fffffff;
    }
    else
    {
      dist[cnt] = ABS(ChrList[character].xpos - GCamera.x) + ABS(ChrList[character].ypos - GCamera.y);
    }
    cnt++;
  }


  // Put em in the right order
  cnt = 0;
  while (cnt < numdolist)
  {
    character = olddolist[cnt];
    order = 0;  // Assume this character is closest
    tnc = 0;
    while (tnc < numdolist)
    {
      // For each one closer, increment the order
      order += (dist[cnt] > dist[tnc]);
      order += (dist[cnt] == dist[tnc]) && (cnt < tnc);
      tnc++;
    }
    dolist[order] = character;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_dolist(GAME_STATE * gs)
{
  // ZZ> This function finds the characters that need to be drawn and puts them in the list
  int cnt, character;


  // Remove everyone from the dolist
  cnt = 0;
  while (cnt < numdolist)
  {
    character = dolist[cnt];
    ChrList[character].indolist = bfalse;
    cnt++;
  }
  numdolist = 0;


  // Now fill it up again
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].on && (!ChrList[cnt].inpack))
    {
      // Add the character
      add_to_dolist(gs, cnt);
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void keep_weapons_with_holders()
{
  // ZZ> This function keeps weapons near their holders
  int cnt, character;


  // !!!BAD!!!  May need to do 3 levels of attachment...
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].on)
    {
      character = ChrList[cnt].attachedto;
      if (character == MAXCHR)
      {
        // Keep inventory with character
        if (ChrList[cnt].inpack == bfalse)
        {
          character = ChrList[cnt].nextinpack;
          while (character != MAXCHR)
          {
            ChrList[character].xpos = ChrList[cnt].xpos;
            ChrList[character].ypos = ChrList[cnt].ypos;
            ChrList[character].zpos = ChrList[cnt].zpos;
            // Copy olds to make SendMessageNear work
            ChrList[character].oldx = ChrList[cnt].xpos;
            ChrList[character].oldy = ChrList[cnt].ypos;
            ChrList[character].oldz = ChrList[cnt].zpos;
            character = ChrList[character].nextinpack;
          }
        }
      }
      else
      {
        // Keep in hand weapons with character
        if (ChrList[character].matrixvalid && ChrList[cnt].matrixvalid)
        {
          ChrList[cnt].xpos = (ChrList[cnt].matrix)_CNV(3, 0);
          ChrList[cnt].ypos = (ChrList[cnt].matrix)_CNV(3, 1);
          ChrList[cnt].zpos = (ChrList[cnt].matrix)_CNV(3, 2);
        }
        else
        {
          ChrList[cnt].xpos = ChrList[character].xpos;
          ChrList[cnt].ypos = ChrList[character].ypos;
          ChrList[cnt].zpos = ChrList[character].zpos;
        }
        ChrList[cnt].turnleftright = ChrList[character].turnleftright;
        // Copy this stuff ONLY if it's a weapon, not for mounts
        if (ChrList[character].transferblend && ChrList[cnt].isitem)
        {
          if (ChrList[character].alpha != 255)
          {
            ChrList[cnt].alpha = ChrList[character].alpha;
          }
          if (ChrList[character].light != 255)
          {
            ChrList[cnt].light = ChrList[character].light;
          }
        }
      }
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_turntosin(void)
{
  // ZZ> This function makes the lookup table for chrturn...
  int cnt;

  cnt = 0;
  while (cnt < 16384)
  {
    turntosin[cnt] = sin((TWO_PI * cnt) / (float)TRIGTABLE_SIZE);
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_one_character_matrix(Uint16 cnt)
{
  // ZZ> This function sets one character's matrix
  Uint16 tnc;
  ChrList[cnt].matrixvalid = btrue;
  if (ChrList[cnt].overlay)
  {
    // Overlays are kept with their target...
    tnc = ChrList[cnt].aitarget;
    ChrList[cnt].xpos = ChrList[tnc].xpos;
    ChrList[cnt].ypos = ChrList[tnc].ypos;
    ChrList[cnt].zpos = ChrList[tnc].zpos;
    (ChrList[cnt].matrix)_CNV(0, 0) = (ChrList[tnc].matrix)_CNV(0, 0);
    (ChrList[cnt].matrix)_CNV(0, 1) = (ChrList[tnc].matrix)_CNV(0, 1);
    (ChrList[cnt].matrix)_CNV(0, 2) = (ChrList[tnc].matrix)_CNV(0, 2);
    (ChrList[cnt].matrix)_CNV(0, 3) = (ChrList[tnc].matrix)_CNV(0, 3);
    (ChrList[cnt].matrix)_CNV(1, 0) = (ChrList[tnc].matrix)_CNV(1, 0);
    (ChrList[cnt].matrix)_CNV(1, 1) = (ChrList[tnc].matrix)_CNV(1, 1);
    (ChrList[cnt].matrix)_CNV(1, 2) = (ChrList[tnc].matrix)_CNV(1, 2);
    (ChrList[cnt].matrix)_CNV(1, 3) = (ChrList[tnc].matrix)_CNV(1, 3);
    (ChrList[cnt].matrix)_CNV(2, 0) = (ChrList[tnc].matrix)_CNV(2, 0);
    (ChrList[cnt].matrix)_CNV(2, 1) = (ChrList[tnc].matrix)_CNV(2, 1);
    (ChrList[cnt].matrix)_CNV(2, 2) = (ChrList[tnc].matrix)_CNV(2, 2);
    (ChrList[cnt].matrix)_CNV(2, 3) = (ChrList[tnc].matrix)_CNV(2, 3);
    (ChrList[cnt].matrix)_CNV(3, 0) = (ChrList[tnc].matrix)_CNV(3, 0);
    (ChrList[cnt].matrix)_CNV(3, 1) = (ChrList[tnc].matrix)_CNV(3, 1);
    (ChrList[cnt].matrix)_CNV(3, 2) = (ChrList[tnc].matrix)_CNV(3, 2);
    (ChrList[cnt].matrix)_CNV(3, 3) = (ChrList[tnc].matrix)_CNV(3, 3);
  }
  else
  {
    ChrList[cnt].matrix = ScaleXYZRotateXYZTranslate(ChrList[cnt].scale, ChrList[cnt].scale, ChrList[cnt].scale,
      ChrList[cnt].turnleftright >> 2,
      ((Uint16)(ChrList[cnt].turnmapud + 32768)) >> 2,
      ((Uint16)(ChrList[cnt].turnmaplr + 32768)) >> 2,
      ChrList[cnt].xpos, ChrList[cnt].ypos, ChrList[cnt].zpos);
  }
}

//--------------------------------------------------------------------------------------------
void free_one_character(int character)
{
  // ZZ> This function sticks a character back on the free character stack
  int cnt;

  freechrlist[numfreechr] = character;
  numfreechr++;
  // Remove from stat list
  if (ChrList[character].staton)
  {
    ChrList[character].staton = bfalse;
    cnt = 0;
    while (cnt < numstat)
    {
      if (statlist[cnt] == character)
      {
        cnt++;
        while (cnt < numstat)
        {
          statlist[cnt-1] = statlist[cnt];
          cnt++;
        }
        numstat--;
      }
      cnt++;
    }
  }
  // Make sure everyone knows it died
  if (ChrList[character].alive && CapList[ChrList[character].model].invictus == bfalse)
  {
    TeamList[ChrList[character].baseteam].morale--;
  }
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].on)
    {
      if (ChrList[cnt].aitarget == character)
      {
        ChrList[cnt].alert |= ALERTIFTARGETKILLED;
        ChrList[cnt].aitarget = cnt;
      }
      if (TeamList[ChrList[cnt].team].leader == character)
      {
        ChrList[cnt].alert |= ALERTIFLEADERKILLED;
      }
    }
    cnt++;
  }
  if (TeamList[ChrList[character].team].leader == character)
  {
    TeamList[ChrList[character].team].leader = NOLEADER;
  }
  ChrList[character].on = bfalse;
  ChrList[character].alive = bfalse;
  ChrList[character].inpack = bfalse;
}

//--------------------------------------------------------------------------------------------
void free_inventory(int character)
{
  // ZZ> This function frees every item in the character's inventory
  int cnt, next;

  cnt = ChrList[character].nextinpack;
  while (cnt < MAXCHR)
  {
    next = ChrList[cnt].nextinpack;
    free_one_character(cnt);
    cnt = next;
  }
}

//--------------------------------------------------------------------------------------------
void attach_particle_to_character(int particle, int character, int grip)
{
  // ZZ> This function sets one particle's position to be attached to a character.
  //     It will kill the particle if the character is no longer around
  Uint16 vertex, model, frame, lastframe;
  Uint8 lip;
  float pointx;
  float pointy;
  float pointz;
  int temp;


  // Check validity of attachment
  if (ChrList[character].on == bfalse || ChrList[character].inpack)
  {
    PrtList[particle].time = 1;
    return;
  }


  // Do we have a matrix???
  if (ChrList[character].matrixvalid)//Mesh.fanlist[ChrList[character].onwhichfan].inrenderlist)
  {
    // Transform the weapon grip from model to world space
    model = ChrList[character].model;
    frame = ChrList[character].frame;
    lastframe = ChrList[character].lastframe;
    lip = ChrList[character].lip >> 6;
    if (grip == SPAWNORIGIN)
    {
      PrtList[particle].xpos = (ChrList[character].matrix)_CNV(3, 0);
      PrtList[particle].ypos = (ChrList[character].matrix)_CNV(3, 1);
      PrtList[particle].zpos = (ChrList[character].matrix)_CNV(3, 2);
      return;
    }
    vertex = MadList[model].vertices - grip;


    // Calculate grip point locations with linear interpolation and other silly things
    switch (lip)
    {
    case 0:  // 25% this frame
      temp = MadFrame[lastframe].vrtx[vertex];
      temp = temp + temp + temp + MadFrame[frame].vrtx[vertex] >> 2;
      pointx = temp;///ChrList[cnt].scale;
      temp = MadFrame[lastframe].vrty[vertex];
      temp = temp + temp + temp + MadFrame[frame].vrty[vertex] >> 2;
      pointy = temp;///ChrList[cnt].scale;
      temp = MadFrame[lastframe].vrtz[vertex];
      temp = temp + temp + temp + MadFrame[frame].vrtz[vertex] >> 2;
      pointz = temp;///ChrList[cnt].scale;
      break;

    case 1:  // 50% this frame
      pointx = (MadFrame[frame].vrtx[vertex] + MadFrame[lastframe].vrtx[vertex] >> 1);///ChrList[cnt].scale;
      pointy = (MadFrame[frame].vrty[vertex] + MadFrame[lastframe].vrty[vertex] >> 1);///ChrList[cnt].scale;
      pointz = (MadFrame[frame].vrtz[vertex] + MadFrame[lastframe].vrtz[vertex] >> 1);///ChrList[cnt].scale;
      break;

    case 2:  // 75% this frame
      temp = MadFrame[frame].vrtx[vertex];
      temp = temp + temp + temp + MadFrame[lastframe].vrtx[vertex] >> 2;
      pointx = temp;///ChrList[cnt].scale;
      temp = MadFrame[frame].vrty[vertex];
      temp = temp + temp + temp + MadFrame[lastframe].vrty[vertex] >> 2;
      pointy = temp;///ChrList[cnt].scale;
      temp = MadFrame[frame].vrtz[vertex];
      temp = temp + temp + temp + MadFrame[lastframe].vrtz[vertex] >> 2;
      pointz = temp;///ChrList[cnt].scale;
      break;

    case 3:  // 100% this frame...  This is the legible one
      pointx = MadFrame[frame].vrtx[vertex];///ChrList[cnt].scale;
      pointy = MadFrame[frame].vrty[vertex];///ChrList[cnt].scale;
      pointz = MadFrame[frame].vrtz[vertex];///ChrList[cnt].scale;
      break;

    }





    // Do the transform
    PrtList[particle].xpos = (pointx * (ChrList[character].matrix)_CNV(0, 0) +
      pointy * (ChrList[character].matrix)_CNV(1, 0) +
      pointz * (ChrList[character].matrix)_CNV(2, 0));
    PrtList[particle].ypos = (pointx * (ChrList[character].matrix)_CNV(0, 1) +
      pointy * (ChrList[character].matrix)_CNV(1, 1) +
      pointz * (ChrList[character].matrix)_CNV(2, 1));
    PrtList[particle].zpos = (pointx * (ChrList[character].matrix)_CNV(0, 2) +
      pointy * (ChrList[character].matrix)_CNV(1, 2) +
      pointz * (ChrList[character].matrix)_CNV(2, 2));
    PrtList[particle].xpos += (ChrList[character].matrix)_CNV(3, 0);
    PrtList[particle].ypos += (ChrList[character].matrix)_CNV(3, 1);
    PrtList[particle].zpos += (ChrList[character].matrix)_CNV(3, 2);
  }
  else
  {
    // No matrix, so just wing it...
    PrtList[particle].xpos = ChrList[character].xpos;
    PrtList[particle].ypos = ChrList[character].ypos;
    PrtList[particle].zpos = ChrList[character].zpos;
  }
}

//--------------------------------------------------------------------------------------------
#define POINTS 4

void make_one_weapon_matrix(Uint16 cnt)
{
  // ZZ> This function sets one weapon's matrix, based on who it's attached to

  int tnc;
  Uint16 character, vertex, model, frame, lastframe;
  Uint8 lip;
  float pointx[POINTS];
  float pointy[POINTS];
  float pointz[POINTS];
  float nupointx[POINTS];
  float nupointy[POINTS];
  float nupointz[POINTS];
  int temp;


  // Transform the weapon grip from model to world space
  character = ChrList[cnt].attachedto;
  model = ChrList[character].model;
  frame = ChrList[character].frame;
  lastframe = ChrList[character].lastframe;
  lip = ChrList[character].lip >> 6;
  ChrList[cnt].matrixvalid = btrue;


  // Calculate grip point locations with linear interpolation and other silly things
  switch (lip)
  {
  case 0:  // 25% this frame
    vertex = ChrList[cnt].weapongrip[0];
    temp = MadFrame[lastframe].vrtx[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrtx[vertex] >> 2;
    pointx[0] = temp;
    temp = MadFrame[lastframe].vrty[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrty[vertex] >> 2;
    pointy[0] = temp;
    temp = MadFrame[lastframe].vrtz[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrtz[vertex] >> 2;
    pointz[0] = temp;

    vertex = ChrList[cnt].weapongrip[1];
    temp = MadFrame[lastframe].vrtx[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrtx[vertex] >> 2;
    pointx[1] = temp;
    temp = MadFrame[lastframe].vrty[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrty[vertex] >> 2;
    pointy[1] = temp;
    temp = MadFrame[lastframe].vrtz[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrtz[vertex] >> 2;
    pointz[1] = temp;

    vertex = ChrList[cnt].weapongrip[2];
    temp = MadFrame[lastframe].vrtx[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrtx[vertex] >> 2;
    pointx[2] = temp;
    temp = MadFrame[lastframe].vrty[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrty[vertex] >> 2;
    pointy[2] = temp;
    temp = MadFrame[lastframe].vrtz[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrtz[vertex] >> 2;
    pointz[2] = temp;

    vertex = ChrList[cnt].weapongrip[3];
    temp = MadFrame[lastframe].vrtx[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrtx[vertex] >> 2;
    pointx[3] = temp;
    temp = MadFrame[lastframe].vrty[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrty[vertex] >> 2;
    pointy[3] = temp;
    temp = MadFrame[lastframe].vrtz[vertex];
    temp = temp + temp + temp + MadFrame[frame].vrtz[vertex] >> 2;
    pointz[3] = temp;
    break;

  case 1:  // 50% this frame
    vertex = ChrList[cnt].weapongrip[0];
    pointx[0] = (MadFrame[frame].vrtx[vertex] + MadFrame[lastframe].vrtx[vertex] >> 1);
    pointy[0] = (MadFrame[frame].vrty[vertex] + MadFrame[lastframe].vrty[vertex] >> 1);
    pointz[0] = (MadFrame[frame].vrtz[vertex] + MadFrame[lastframe].vrtz[vertex] >> 1);
    vertex = ChrList[cnt].weapongrip[1];
    pointx[1] = (MadFrame[frame].vrtx[vertex] + MadFrame[lastframe].vrtx[vertex] >> 1);
    pointy[1] = (MadFrame[frame].vrty[vertex] + MadFrame[lastframe].vrty[vertex] >> 1);
    pointz[1] = (MadFrame[frame].vrtz[vertex] + MadFrame[lastframe].vrtz[vertex] >> 1);
    vertex = ChrList[cnt].weapongrip[2];
    pointx[2] = (MadFrame[frame].vrtx[vertex] + MadFrame[lastframe].vrtx[vertex] >> 1);
    pointy[2] = (MadFrame[frame].vrty[vertex] + MadFrame[lastframe].vrty[vertex] >> 1);
    pointz[2] = (MadFrame[frame].vrtz[vertex] + MadFrame[lastframe].vrtz[vertex] >> 1);
    vertex = ChrList[cnt].weapongrip[3];
    pointx[3] = (MadFrame[frame].vrtx[vertex] + MadFrame[lastframe].vrtx[vertex] >> 1);
    pointy[3] = (MadFrame[frame].vrty[vertex] + MadFrame[lastframe].vrty[vertex] >> 1);
    pointz[3] = (MadFrame[frame].vrtz[vertex] + MadFrame[lastframe].vrtz[vertex] >> 1);
    break;

  case 2:  // 75% this frame
    vertex = ChrList[cnt].weapongrip[0];
    temp = MadFrame[frame].vrtx[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrtx[vertex] >> 2;
    pointx[0] = temp;
    temp = MadFrame[frame].vrty[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrty[vertex] >> 2;
    pointy[0] = temp;
    temp = MadFrame[frame].vrtz[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrtz[vertex] >> 2;
    pointz[0] = temp;


    vertex = ChrList[cnt].weapongrip[1];
    temp = MadFrame[frame].vrtx[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrtx[vertex] >> 2;
    pointx[1] = temp;
    temp = MadFrame[frame].vrty[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrty[vertex] >> 2;
    pointy[1] = temp;
    temp = MadFrame[frame].vrtz[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrtz[vertex] >> 2;
    pointz[1] = temp;


    vertex = ChrList[cnt].weapongrip[2];
    temp = MadFrame[frame].vrtx[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrtx[vertex] >> 2;
    pointx[2] = temp;
    temp = MadFrame[frame].vrty[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrty[vertex] >> 2;
    pointy[2] = temp;
    temp = MadFrame[frame].vrtz[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrtz[vertex] >> 2;
    pointz[2] = temp;


    vertex = ChrList[cnt].weapongrip[3];
    temp = MadFrame[frame].vrtx[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrtx[vertex] >> 2;
    pointx[3] = temp;
    temp = MadFrame[frame].vrty[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrty[vertex] >> 2;
    pointy[3] = temp;
    temp = MadFrame[frame].vrtz[vertex];
    temp = temp + temp + temp + MadFrame[lastframe].vrtz[vertex] >> 2;
    pointz[3] = temp;

    break;

  case 3:  // 100% this frame...  This is the legible one
    vertex = ChrList[cnt].weapongrip[0];
    pointx[0] = MadFrame[frame].vrtx[vertex];
    pointy[0] = MadFrame[frame].vrty[vertex];
    pointz[0] = MadFrame[frame].vrtz[vertex];
    vertex = ChrList[cnt].weapongrip[1];
    pointx[1] = MadFrame[frame].vrtx[vertex];
    pointy[1] = MadFrame[frame].vrty[vertex];
    pointz[1] = MadFrame[frame].vrtz[vertex];
    vertex = ChrList[cnt].weapongrip[2];
    pointx[2] = MadFrame[frame].vrtx[vertex];
    pointy[2] = MadFrame[frame].vrty[vertex];
    pointz[2] = MadFrame[frame].vrtz[vertex];
    vertex = ChrList[cnt].weapongrip[3];
    pointx[3] = MadFrame[frame].vrtx[vertex];
    pointy[3] = MadFrame[frame].vrty[vertex];
    pointz[3] = MadFrame[frame].vrtz[vertex];
    break;

  }





  tnc = 0;
  while (tnc < POINTS)
  {
    // Do the transform
    nupointx[tnc] = (pointx[tnc] * (ChrList[character].matrix)_CNV(0, 0) +
      pointy[tnc] * (ChrList[character].matrix)_CNV(1, 0) +
      pointz[tnc] * (ChrList[character].matrix)_CNV(2, 0));
    nupointy[tnc] = (pointx[tnc] * (ChrList[character].matrix)_CNV(0, 1) +
      pointy[tnc] * (ChrList[character].matrix)_CNV(1, 1) +
      pointz[tnc] * (ChrList[character].matrix)_CNV(2, 1));
    nupointz[tnc] = (pointx[tnc] * (ChrList[character].matrix)_CNV(0, 2) +
      pointy[tnc] * (ChrList[character].matrix)_CNV(1, 2) +
      pointz[tnc] * (ChrList[character].matrix)_CNV(2, 2));

    nupointx[tnc] += (ChrList[character].matrix)_CNV(3, 0);
    nupointy[tnc] += (ChrList[character].matrix)_CNV(3, 1);
    nupointz[tnc] += (ChrList[character].matrix)_CNV(3, 2);

    tnc++;
  }




  // Calculate weapon's matrix based on positions of grip points
  // chrscale is recomputed at time of attachment
  ChrList[cnt].matrix = FourPoints(nupointx[0], nupointy[0], nupointz[0],
    nupointx[1], nupointy[1], nupointz[1],
    nupointx[2], nupointy[2], nupointz[2],
    nupointx[3], nupointy[3], nupointz[3],
    ChrList[cnt].scale);
}

//--------------------------------------------------------------------------------------------
void make_character_matrices()
{
  // ZZ> This function makes all of the character's matrices
  int cnt, tnc;

  // Forget about old matrices
  cnt = 0;
  while (cnt < MAXCHR)
  {
    ChrList[cnt].matrixvalid = bfalse;
    cnt++;
  }


  // Do base characters
  tnc = 0;
  while (tnc < MAXCHR)
  {
    if (ChrList[tnc].attachedto == MAXCHR && ChrList[tnc].on) // Skip weapons for now
    {
      make_one_character_matrix(tnc);
    }
    tnc++;
  }



  // Do first level of attachments
  tnc = 0;
  while (tnc < MAXCHR)
  {
    if (ChrList[tnc].attachedto != MAXCHR && ChrList[tnc].on)
    {
      if (ChrList[ChrList[tnc].attachedto].attachedto == MAXCHR)
      {
        make_one_weapon_matrix(tnc);
      }
    }
    tnc++;
  }


  // Do second level of attachments
  tnc = 0;
  while (tnc < MAXCHR)
  {
    if (ChrList[tnc].attachedto != MAXCHR && ChrList[tnc].on)
    {
      if (ChrList[ChrList[tnc].attachedto].attachedto != MAXCHR)
      {
        make_one_weapon_matrix(tnc);
      }
    }
    tnc++;
  }
}

//--------------------------------------------------------------------------------------------
int get_free_character()
{
  // ZZ> This function gets an unused character and returns its index
  int character;


  if (numfreechr == 0)
  {
    // Return MAXCHR if we can't find one
    return MAXCHR;
  }
  else
  {
    // Just grab the next one
    numfreechr--;
    character = freechrlist[numfreechr];
  }
  return character;
}

//--------------------------------------------------------------------------------------------
Uint8 find_target_in_block(int x, int y, float chrx, float chry, Uint16 facing,
                           Uint8 onlyfriends, Uint8 anyone, Uint8 team,
                           Uint16 donttarget, Uint16 oldtarget)
{
  // ZZ> This function helps find a target, returning btrue if it found a decent target
  int cnt;
  Uint16 angle;
  Uint16 charb;
  Uint8 enemies, returncode;
  Uint32 fanblock;
  int distance;



  returncode = bfalse;


  // Current fanblock
  if (x >= 0 && x < (Mesh.sizex >> 2) && y >= 0 && y < (Mesh.sizey >> 2))
  {
    fanblock = x + Mesh.blockstart[y];


    enemies = bfalse;
    if (onlyfriends == bfalse) enemies = btrue;


    charb = Mesh.bumplist[fanblock].chr;
    cnt = 0;
    while (cnt < Mesh.bumplist[fanblock].chrnum)
    {
      if (ChrList[charb].alive && ChrList[charb].invictus == bfalse && charb != donttarget && charb != oldtarget)
      {
        if (anyone || (ChrList[charb].team == team && onlyfriends) || (TeamList[team].hatesteam[ChrList[charb].team] && enemies))
        {
          distance = ABS(ChrList[charb].xpos - chrx) + ABS(ChrList[charb].ypos - chry);
          if (distance < globestdistance)
          {
            angle = RAD_TO_TURN(atan2(ChrList[charb].ypos - chry, ChrList[charb].xpos - chrx));
            angle = facing - angle;
            if (angle < globestangle || angle > (65535 - globestangle))
            {
              returncode = btrue;
              globesttarget = charb;
              globestdistance = distance;
              glouseangle = angle;
              if (angle  > 32767)
                globestangle = -angle;
              else
                globestangle = angle;
            }
          }
        }
      }
      charb = ChrList[charb].bumpnext;
      cnt++;
    }
  }
  return returncode;
}

//--------------------------------------------------------------------------------------------
Uint16 find_target(float chrx, float chry, Uint16 facing,
                   Uint16 targetangle, Uint8 onlyfriends, Uint8 anyone,
                   Uint8 team, Uint16 donttarget, Uint16 oldtarget)
{
  // This function finds the best target for the given parameters
  Uint8 done;
  int x, y;

  x = chrx;
  y = chry;
  x = x >> 9;
  y = y >> 9;
  globestdistance = 9999;
  globestangle = targetangle;
  done = find_target_in_block(x, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done |= find_target_in_block(x + 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done |= find_target_in_block(x - 1, y, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done |= find_target_in_block(x, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done |= find_target_in_block(x, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  if (done) return globesttarget;


  done = find_target_in_block(x + 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done |= find_target_in_block(x + 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done |= find_target_in_block(x - 1, y + 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done |= find_target_in_block(x - 1, y - 1, chrx, chry, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  if (done) return globesttarget;


  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void free_all_characters(GAME_STATE * gs)
{
  // ZZ> This function resets the character allocation list
  gs->modstate.nolocalplayers = btrue;
  numfreechr = 0;
  while (numfreechr < MAXCHR)
  {
    ChrList[numfreechr].on = bfalse;
    ChrList[numfreechr].alive = bfalse;
    ChrList[numfreechr].inpack = bfalse;
    ChrList[numfreechr].numinpack = 0;
    ChrList[numfreechr].nextinpack = MAXCHR;
    ChrList[numfreechr].staton = bfalse;
    ChrList[numfreechr].matrixvalid = bfalse;
    freechrlist[numfreechr] = numfreechr;
    numfreechr++;
  }
  numpla = 0;
  numlocalpla = 0;
  numstat = 0;
}

//--------------------------------------------------------------------------------------------
Uint8 __chrhitawall(int character)
{
  // ZZ> This function returns nonzero if the character hit a wall that the
  //     character is not allowed to cross
  Uint8 passtl, passtr, passbr, passbl;
  int x, y, bs;


  y = ChrList[character].ypos;  x = ChrList[character].xpos;  bs = ChrList[character].bumpsize >> 1;
  // !!!BAD!!! Should really do bound checking...
  passtl = Mesh.fanlist[Mesh.fanstart[y-bs>>7] + (x - bs >> 7)].fx;
  passtr = Mesh.fanlist[Mesh.fanstart[y-bs>>7] + (x + bs >> 7)].fx;
  passbr = Mesh.fanlist[Mesh.fanstart[y+bs>>7] + (x + bs >> 7)].fx;
  passbl = Mesh.fanlist[Mesh.fanstart[y+bs>>7] + (x - bs >> 7)].fx;
  passtl = (passtl | passtr | passbr | passbl) & ChrList[character].stoppedby;
  return passtl;
}

//--------------------------------------------------------------------------------------------
void reset_character_accel(Uint16 character)
{
  // ZZ> This function fixes a character's MAX acceleration
  Uint16 enchant;

  if (character != MAXCHR)
  {
    if (ChrList[character].on)
    {
      // Okay, remove all acceleration enchants
      enchant = ChrList[character].firstenchant;
      while (enchant < MAXENCHANT)
      {
        remove_enchant_value(enchant, ADDACCEL);
        enchant = EncList[enchant].nextenchant;
      }
      // Set the starting value
      ChrList[character].maxaccel = CapList[ChrList[character].model].maxaccel[(ChrList[character].texture - MadList[ChrList[character].model].skinstart) % MAXSKIN];
      // Put the acceleration enchants back on
      enchant = ChrList[character].firstenchant;
      while (enchant < MAXENCHANT)
      {
        add_enchant_value(enchant, ADDACCEL, EncList[enchant].eve);
        enchant = EncList[enchant].nextenchant;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void detach_character_from_mount(GAME_STATE * gs, Uint16 character, Uint8 ignorekurse,
                                 Uint8 doshop, Uint32 * rand_idx)
{
  // ZZ> This function drops an item
  Uint16 mount, hand, enchant, cnt, passage, owner=MAXCHR, price;
  Uint8 inshop;
  int loc;
  Uint32 chr_randie = *rand_idx;

  RANDIE(*rand_idx);


  // Make sure the character is valid
  if (character == MAXCHR)
    return;


  // Make sure the character is mounted
  mount = ChrList[character].attachedto;
  if (mount >= MAXCHR)
    return;


  // Make sure both are still around
  if (!ChrList[character].on || !ChrList[mount].on)
    return;


  // Don't allow living characters to drop kursed weapons
  if (ignorekurse == bfalse && ChrList[character].iskursed && ChrList[mount].alive && ChrList[character].isitem)
  {
    ChrList[character].alert = ChrList[character].alert | ALERTIFNOTDROPPED;
    return;
  }


  // Figure out which hand it's in
  hand = 0;
  if (ChrList[character].inwhichhand == GRIPRIGHT)
  {
    hand = 1;
  }


  // Rip 'em apart
  ChrList[character].attachedto = MAXCHR;
  if (ChrList[mount].holdingwhich[0] == character)
    ChrList[mount].holdingwhich[0] = MAXCHR;
  if (ChrList[mount].holdingwhich[1] == character)
    ChrList[mount].holdingwhich[1] = MAXCHR;
  ChrList[character].scale = ChrList[character].fat * MadList[ChrList[character].model].scale * 4;


  // Run the falling animation...
  play_action(character, ACTIONJB + hand, bfalse);



  // Set the positions
  if (ChrList[character].matrixvalid)
  {
    ChrList[character].xpos = (ChrList[character].matrix)_CNV(3, 0);
    ChrList[character].ypos = (ChrList[character].matrix)_CNV(3, 1);
    ChrList[character].zpos = (ChrList[character].matrix)_CNV(3, 2);
  }
  else
  {
    ChrList[character].xpos = ChrList[mount].xpos;
    ChrList[character].ypos = ChrList[mount].ypos;
    ChrList[character].zpos = ChrList[mount].zpos;
  }



  // Make sure it's not dropped in a wall...
  if (__chrhitawall(character))
  {
    ChrList[character].xpos = ChrList[mount].xpos;
    ChrList[character].ypos = ChrList[mount].ypos;
  }


  // Check for shop passages
  inshop = bfalse;
  if (ChrList[character].isitem && numshoppassage != 0 && doshop)
  {
    cnt = 0;
    while (cnt < numshoppassage)
    {
      passage = shoppassage[cnt];
      loc = ChrList[character].xpos;
      loc = loc >> 7;
      if (loc >= passtlx[passage] && loc <= passbrx[passage])
      {
        loc = ChrList[character].ypos;
        loc = loc >> 7;
        if (loc >= passtly[passage] && loc <= passbry[passage])
        {
          inshop = btrue;
          owner = shopowner[passage];
          cnt = numshoppassage;  // Finish loop
          if (owner == NOOWNER)
          {
            // The owner has died!!!
            inshop = bfalse;
          }
        }
      }
      cnt++;
    }
    if (inshop)
    {
      // Give the mount its money back, alert the shop owner
      price = CapList[ChrList[character].model].skincost[(ChrList[character].texture - MadList[ChrList[character].model].skinstart) % MAXSKIN];
      if (CapList[ChrList[character].model].isstackable)
      {
        price = price * ChrList[character].ammo;
      }
      ChrList[mount].money += price;
      ChrList[owner].money -= price;
      if (ChrList[owner].money < 0)  ChrList[owner].money = 0;
      if (ChrList[mount].money > MAXMONEY)  ChrList[mount].money = MAXMONEY;
      ChrList[owner].alert |= ALERTIFORDERED;
      ChrList[owner].order = price;  // Tell owner how much...
      ChrList[owner].counter = 0;  // 0 for buying an item
    }
  }



  // Make sure it works right
  ChrList[character].hitready = btrue;
  if (inshop)
  {
    // Drop straight down to avoid theft
    ChrList[character].xvel = 0;
    ChrList[character].yvel = 0;
  }
  else
  {
    ChrList[character].xvel = ChrList[mount].xvel;
    ChrList[character].yvel = ChrList[mount].yvel;
  }
  ChrList[character].zvel = DROPZVEL;


  // Turn looping off
  ChrList[character].loopaction = bfalse;


  // Reset the team if it is a mount
  if (ChrList[mount].ismount)
  {
    ChrList[mount].team = ChrList[mount].baseteam;
    ChrList[mount].alert |= ALERTIFDROPPED;
  }
  ChrList[character].team = ChrList[character].baseteam;
  ChrList[character].alert |= ALERTIFDROPPED;


  // Reset transparency
  if (ChrList[character].isitem && ChrList[mount].transferblend)
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


  // Set twist
  ChrList[character].turnmaplr = 32768;
  ChrList[character].turnmapud = 32768;
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount(Uint16 character, Uint16 mount,
                               Uint16 grip)
{
  // ZZ> This function attaches one character to another ( the mount )
  //     at either the left or right grip
  int tnc, hand;


  // Make sure both are still around
  if (!ChrList[character].on || !ChrList[mount].on || ChrList[character].inpack || ChrList[mount].inpack)
    return;

  // Figure out which hand this grip relates to
  hand = 1;
  if (grip == GRIPLEFT)
    hand = 0;


  // Make sure the the hand is valid
  if (CapList[ChrList[mount].model].gripvalid[hand] == bfalse)
    return;


  // Put 'em together
  ChrList[character].inwhichhand = grip;
  ChrList[character].attachedto = mount;
  ChrList[mount].holdingwhich[hand] = character;
  tnc = MadList[ChrList[mount].model].vertices - grip;
  ChrList[character].weapongrip[0] = tnc;
  ChrList[character].weapongrip[1] = tnc + 1;
  ChrList[character].weapongrip[2] = tnc + 2;
  ChrList[character].weapongrip[3] = tnc + 3;
  ChrList[character].scale = ChrList[character].fat / (ChrList[mount].fat * 1280);
  ChrList[character].inwater = bfalse;
  ChrList[character].jumptime = JUMPDELAY * 4;


  // Run the held animation
  if (ChrList[mount].ismount && grip == GRIPONLY)
  {
    // Riding mount
    play_action(character, ACTIONMI, btrue);
    ChrList[character].loopaction = btrue;
  }
  else
  {
    play_action(character, ACTIONMM + hand, bfalse);
    if (ChrList[character].isitem)
    {
      // Item grab
      ChrList[character].keepaction = btrue;
    }
  }




  // Set the team
  if (ChrList[character].isitem)
  {
    ChrList[character].team = ChrList[mount].team;
    // Set the alert
    ChrList[character].alert = ChrList[character].alert | ALERTIFGRABBED;
  }
  if (ChrList[mount].ismount)
  {
    ChrList[mount].team = ChrList[character].team;
    // Set the alert
    if (ChrList[mount].isitem == bfalse)
    {
      ChrList[mount].alert = ChrList[mount].alert | ALERTIFGRABBED;
    }
  }


  // It's not gonna hit the floor
  ChrList[character].hitready = bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 stack_in_pack(Uint16 item, Uint16 character)
{
  // ZZ> This function looks in the character's pack for an item similar
  //     to the one given.  If it finds one, it returns the similar item's
  //     index number, otherwise it returns MAXCHR.
  Uint16 inpack, id;
  Uint8 allok;


  if (CapList[ChrList[item].model].isstackable == btrue)
  {
    inpack = ChrList[character].nextinpack;
    allok = bfalse;
    while (inpack != MAXCHR && allok == bfalse)
    {
      allok = btrue;
      if (ChrList[inpack].model != ChrList[item].model)
      {
        if (CapList[ChrList[inpack].model].isstackable == bfalse)
        {
          allok = bfalse;
        }

        if (ChrList[inpack].ammomax != ChrList[item].ammomax)
        {
          allok = bfalse;
        }

        id = 0;
        while (id < MAXIDSZ && allok == btrue)
        {
          if (CapList[ChrList[inpack].model].idsz[id] != CapList[ChrList[item].model].idsz[id])
          {
            allok = bfalse;
          }
          id++;
        }
      }
      if (allok == bfalse)
      {
        inpack = ChrList[inpack].nextinpack;
      }
    }
    if (allok == btrue)
    {
      return inpack;
    }
  }
  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void add_item_to_character_pack(GAME_STATE * gs, Uint16 item, Uint16 character, Uint32 * rand_idx)
{
  // ZZ> This function puts one character inside the other's pack
  Uint16 oldfirstitem, newammo, stack;
  Uint32 chr_randie = *rand_idx;

  RANDIE(*rand_idx);


  // Make sure everything is hunkydori
  if ((!ChrList[item].on) || (!ChrList[character].on) || ChrList[item].inpack || ChrList[character].inpack ||
    ChrList[character].isitem)
    return;


  stack = stack_in_pack(item, character);
  if (stack != MAXCHR)
  {
    // We found a similar, stackable item in the pack
    if (ChrList[item].nameknown || ChrList[stack].nameknown)
    {
      ChrList[item].nameknown = btrue;
      ChrList[stack].nameknown = btrue;
    }
    if (CapList[ChrList[item].model].usageknown || CapList[ChrList[stack].model].usageknown)
    {
      CapList[ChrList[item].model].usageknown = btrue;
      CapList[ChrList[stack].model].usageknown = btrue;
    }
    newammo = ChrList[item].ammo + ChrList[stack].ammo;
    if (newammo <= ChrList[stack].ammomax)
    {
      // All transfered, so kill the in hand item
      ChrList[stack].ammo = newammo;
      if (ChrList[item].attachedto != MAXCHR)
      {
        detach_character_from_mount(gs, item, btrue, bfalse, &chr_randie);
      }
      free_one_character(item);
    }
    else
    {
      // Only some were transfered,
      ChrList[item].ammo = ChrList[item].ammo + ChrList[stack].ammo - ChrList[stack].ammomax;
      ChrList[stack].ammo = ChrList[stack].ammomax;
      ChrList[character].alert |= ALERTIFTOOMUCHBAGGAGE;
    }
  }
  else
  {
    // Make sure we have room for another item
    if (ChrList[character].numinpack >= MAXNUMINPACK)
    {
      ChrList[character].alert |= ALERTIFTOOMUCHBAGGAGE;
      return;
    }


    // Take the item out of hand
    if (ChrList[item].attachedto != MAXCHR)
    {
      detach_character_from_mount(gs, item, btrue, bfalse, &chr_randie);
      ChrList[item].alert &= (~ALERTIFDROPPED);
    }


    // Remove the item from play
    ChrList[item].hitready = bfalse;
    ChrList[item].inpack = btrue;


    // Insert the item into the pack as the first one
    oldfirstitem = ChrList[character].nextinpack;
    ChrList[character].nextinpack = item;
    ChrList[item].nextinpack = oldfirstitem;
    ChrList[character].numinpack++;
    if (CapList[ChrList[item].model].isequipment)
    {
      // AtLastWaypoint doubles as PutAway
      ChrList[item].alert |= ALERTIFATLASTWAYPOINT;
    }
  }
  return;
}

//--------------------------------------------------------------------------------------------
Uint16 get_item_from_character_pack(Uint16 character, Uint16 grip, Uint8 ignorekurse)
{
  // ZZ> This function takes the last item in the character's pack and puts
  //     it into the designated hand.  It returns the item number or MAXCHR.
  Uint16 item, nexttolastitem;


  // Make sure everything is hunkydori
  if ((!ChrList[character].on) || ChrList[character].inpack || ChrList[character].isitem || ChrList[character].nextinpack == MAXCHR)
    return MAXCHR;
  if (ChrList[character].numinpack == 0)
    return MAXCHR;


  // Find the last item in the pack
  nexttolastitem = character;
  item = ChrList[character].nextinpack;
  while (ChrList[item].nextinpack != MAXCHR)
  {
    nexttolastitem = item;
    item = ChrList[item].nextinpack;
  }


  // Figure out what to do with it
  if (ChrList[item].iskursed && ChrList[item].isequipped && ignorekurse == bfalse)
  {
    // Flag the last item as not removed
    ChrList[item].alert |= ALERTIFNOTPUTAWAY;  // Doubles as IfNotTakenOut
    // Cycle it to the front
    ChrList[item].nextinpack = ChrList[character].nextinpack;
    ChrList[nexttolastitem].nextinpack = MAXCHR;
    ChrList[character].nextinpack = item;
    if (character == nexttolastitem)
    {
      ChrList[item].nextinpack = MAXCHR;
    }
    return MAXCHR;
  }
  else
  {
    // Remove the last item from the pack
    ChrList[item].inpack = bfalse;
    ChrList[item].isequipped = bfalse;
    ChrList[nexttolastitem].nextinpack = MAXCHR;
    ChrList[character].numinpack--;
    ChrList[item].team = ChrList[character].team;


    // Attach the item to the character's hand
    attach_character_to_mount(item, character, grip);
    ChrList[item].alert &= (~ALERTIFGRABBED);
    ChrList[item].alert |= (ALERTIFTAKENOUT);
  }
  return item;
}

//--------------------------------------------------------------------------------------------
void drop_keys(Uint16 character)
{
  // ZZ> This function drops all GKeyb.s ( [KEYA] to [KEYZ] ) that are in a character's
  //     inventory ( Not hands ).
  Uint16 item, lastitem, nextitem, direction, cosdir;
  IDSZ testa, testz;

  Uint32 rand_save = randie_index;


  if (character < MAXCHR)
  {
    if (ChrList[character].on)
    {
      if (ChrList[character].zpos > -2) // Don't lose GKeyb.s in pits...
      {
        // The IDSZs to find
        testa = MAKE_IDSZ("KEYA");  // [KEYA]
        testz = MAKE_IDSZ("KEYZ");  // [KEYZ]


        lastitem = character;
        item = ChrList[character].nextinpack;
        while (item != MAXCHR)
        {
          nextitem = ChrList[item].nextinpack;
          if (item != character) // Should never happen...
          {
            if ((CapList[ChrList[item].model].idsz[IDSZPARENT] >= (Uint32) testa &&
              CapList[ChrList[item].model].idsz[IDSZPARENT] <= (Uint32) testz) ||
              (CapList[ChrList[item].model].idsz[IDSZTYPE] >= (Uint32) testa &&
              CapList[ChrList[item].model].idsz[IDSZTYPE] <= (Uint32) testz))
            {
              // We found a key...
              ChrList[item].inpack = bfalse;
              ChrList[item].isequipped = bfalse;
              ChrList[lastitem].nextinpack = nextitem;
              ChrList[item].nextinpack = MAXCHR;
              ChrList[character].numinpack--;
              ChrList[item].attachedto = MAXCHR;
              ChrList[item].alert |= ALERTIFDROPPED;
              ChrList[item].hitready = btrue;


              direction = RANDIE(randie_index);
              ChrList[item].turnleftright = direction + 32768;
              cosdir = direction + 16384;
              ChrList[item].level = ChrList[character].level;
              ChrList[item].xpos = ChrList[character].xpos;
              ChrList[item].ypos = ChrList[character].ypos;
              ChrList[item].zpos = ChrList[character].zpos;
              ChrList[item].xvel = turntosin[(cosdir>>2) & TRIGTABLE_MASK] * DROPXYVEL;
              ChrList[item].yvel = turntosin[(direction>>2) & TRIGTABLE_MASK] * DROPXYVEL;
              ChrList[item].zvel = DROPZVEL;
              ChrList[item].team = ChrList[item].baseteam;
            }
            else
            {
              lastitem = item;
            }
          }
          item = nextitem;
        }
      }
    }
  }

  randie_index = rand_save;
}

//--------------------------------------------------------------------------------------------
void drop_all_items(GAME_STATE * gs, Uint16 character, Uint32 * rand_idx)
{
  // ZZ> This function drops all of a character's items
  Uint16 item, direction, cosdir, diradd;
  Uint32 loc_randie = *rand_idx;


  if (character < MAXCHR)
  {
    if (ChrList[character].on)
    {
      detach_character_from_mount(gs, ChrList[character].holdingwhich[0], btrue, bfalse, &loc_randie);
      detach_character_from_mount(gs, ChrList[character].holdingwhich[1], btrue, bfalse, &loc_randie);
      if (ChrList[character].numinpack > 0)
      {
        direction = ChrList[character].turnleftright + 32768;
        diradd = 65535 / ChrList[character].numinpack;
        while (ChrList[character].numinpack > 0)
        {
          item = get_item_from_character_pack(character, GRIPLEFT, bfalse);
          if (item < MAXCHR)
          {
            detach_character_from_mount(gs, item, btrue, btrue, &loc_randie);
            ChrList[item].hitready = btrue;
            ChrList[item].alert |= ALERTIFDROPPED;
            ChrList[item].xpos = ChrList[character].xpos;
            ChrList[item].ypos = ChrList[character].ypos;
            ChrList[item].zpos = ChrList[character].zpos;
            ChrList[item].level = ChrList[character].level;
            ChrList[item].turnleftright = direction + 32768;
            cosdir = direction + 16384;
            ChrList[item].xvel = turntosin[(cosdir>>2) & TRIGTABLE_MASK] * DROPXYVEL;
            ChrList[item].yvel = turntosin[(direction>>2) & TRIGTABLE_MASK] * DROPXYVEL;
            ChrList[item].zvel = DROPZVEL;
            ChrList[item].team = ChrList[item].baseteam;
          }
          direction += diradd;
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void character_grab_stuff(int chara, int grip, Uint8 people)
{
  // ZZ> This function makes the character pick up an item if there's one around
  float xa, ya, za, xb, yb, zb, dist;
  int charb, hand;
  Uint16 vertex, model, frame, owner=MAXCHR, passage, cnt, price;
  float pointx, pointy, pointz;
  Uint8 inshop;
  int loc;


  // Make life easier
  model = ChrList[chara].model;
  hand = (grip - 4) >> 2;  // 0 is left, 1 is right


  // Make sure the character doesn't have something already, and that it has hands
  if (ChrList[chara].holdingwhich[hand] != MAXCHR || CapList[model].gripvalid[hand] == bfalse)
    return;


  // Do we have a matrix???
  if (ChrList[chara].matrixvalid)//Mesh.fanlist[ChrList[chara].onwhichfan].inrenderlist)
  {
    // Transform the weapon grip from model to world space
    frame = ChrList[chara].frame;
    vertex = MadList[model].vertices - grip;


    // Calculate grip point locations
    pointx = MadFrame[frame].vrtx[vertex];///ChrList[cnt].scale;
    pointy = MadFrame[frame].vrty[vertex];///ChrList[cnt].scale;
    pointz = MadFrame[frame].vrtz[vertex];///ChrList[cnt].scale;


    // Do the transform
    xa = (pointx * (ChrList[chara].matrix)_CNV(0, 0) +
      pointy * (ChrList[chara].matrix)_CNV(1, 0) +
      pointz * (ChrList[chara].matrix)_CNV(2, 0));
    ya = (pointx * (ChrList[chara].matrix)_CNV(0, 1) +
      pointy * (ChrList[chara].matrix)_CNV(1, 1) +
      pointz * (ChrList[chara].matrix)_CNV(2, 1));
    za = (pointx * (ChrList[chara].matrix)_CNV(0, 2) +
      pointy * (ChrList[chara].matrix)_CNV(1, 2) +
      pointz * (ChrList[chara].matrix)_CNV(2, 2));
    xa += (ChrList[chara].matrix)_CNV(3, 0);
    ya += (ChrList[chara].matrix)_CNV(3, 1);
    za += (ChrList[chara].matrix)_CNV(3, 2);
  }
  else
  {
    // Just wing it
    xa = ChrList[chara].xpos;
    ya = ChrList[chara].ypos;
    za = ChrList[chara].zpos;
  }



  // Go through all characters to find the best match
  charb = 0;
  while (charb < MAXCHR)
  {
    if (ChrList[charb].on && (!ChrList[charb].inpack) && ChrList[charb].weight < ChrList[chara].weight && ChrList[charb].alive && ChrList[charb].attachedto == MAXCHR && ((people == bfalse && ChrList[charb].isitem) || (people == btrue && ChrList[charb].isitem == bfalse)))
    {
      xb = ChrList[charb].xpos;
      yb = ChrList[charb].ypos;
      zb = ChrList[charb].zpos;
      // First check absolute value diamond
      xb = ABS(xa - xb);
      yb = ABS(ya - yb);
      zb = ABS(za - zb);
      dist = xb + yb;
      if (dist < GRABSIZE && zb < GRABSIZE)
      {
        // Don't grab your mount
        if (ChrList[charb].holdingwhich[0] != chara && ChrList[charb].holdingwhich[1] != chara)
        {
          // Check for shop
          inshop = bfalse;
          if (ChrList[charb].isitem && numshoppassage != 0)
          {
            cnt = 0;
            while (cnt < numshoppassage)
            {
              passage = shoppassage[cnt];
              loc = ChrList[charb].xpos;
              loc = loc >> 7;
              if (loc >= passtlx[passage] && loc <= passbrx[passage])
              {
                loc = ChrList[charb].ypos;
                loc = loc >> 7;
                if (loc >= passtly[passage] && loc <= passbry[passage])
                {
                  inshop = btrue;
                  owner = shopowner[passage];
                  cnt = numshoppassage;  // Finish loop
                  if (owner == NOOWNER)
                  {
                    // The owner has died!!!
                    inshop = bfalse;
                  }
                }
              }
              cnt++;
            }
            if (inshop)
            {
              // Pay the shop owner, or don't allow grab...
              if (ChrList[chara].isitem)
              {
                // Pets can shop for free =]
                inshop = bfalse;
              }
              else
              {
                ChrList[owner].alert |= ALERTIFORDERED;
                price = CapList[ChrList[charb].model].skincost[(ChrList[charb].texture - MadList[ChrList[charb].model].skinstart) % MAXSKIN];
                if (CapList[ChrList[charb].model].isstackable)
                {
                  price = price * ChrList[charb].ammo;
                }
                ChrList[owner].order = price;  // Tell owner how much...
                if (ChrList[chara].money >= price)
                {
                  // Okay to buy
                  ChrList[owner].counter = 1;  // 1 for selling an item
                  ChrList[chara].money -= price;  // Skin 0 cost is price
                  ChrList[owner].money += price;
                  if (ChrList[owner].money > MAXMONEY)  ChrList[owner].money = MAXMONEY;
                  inshop = bfalse;
                }
                else
                {
                  // Don't allow purchase
                  ChrList[owner].counter = 2;  // 2 for "you can't afford that"
                  inshop = btrue;
                }
              }
            }
          }


          if (inshop == bfalse)
          {
            // Stick 'em together and quit
            attach_character_to_mount(charb, chara, grip);
            charb = MAXCHR;
            if (people == btrue)
            {
              // Do a slam animation...  ( Be sure to drop!!! )
              play_action(chara, ACTIONMC + hand, bfalse);
            }
          }
          else
          {
            // Lift the item a little and quit...
            ChrList[charb].zvel = DROPZVEL;
            ChrList[charb].hitready = btrue;
            ChrList[charb].alert |= ALERTIFDROPPED;
            charb = MAXCHR;
          }
        }
      }
    }
    charb++;
  }
}

//--------------------------------------------------------------------------------------------
void character_swipe(GAME_STATE * gs, Uint16 cnt, Uint8 grip, Uint32 * rand_idx)
{
  // ZZ> This function spawns an attack particle
  int weapon, particle, spawngrip, thrown;
  Uint8 action;
  Uint16 tTmp;
  float dampen;
  float x, y, z, velocity;
  Uint32 chr_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);


  weapon = ChrList[cnt].holdingwhich[grip];
  spawngrip = SPAWNLAST;
  action = ChrList[cnt].action;
  // See if it's an unarmed attack...
  if (weapon == MAXCHR)
  {
    weapon = cnt;
    spawngrip = 4 + (grip << 2);  // 0 = GRIPLEFT, 1 = GRIPRIGHT
  }


  if (weapon != cnt && ((CapList[ChrList[weapon].model].isstackable && ChrList[weapon].ammo > 1) || (action >= ACTIONFA && action <= ACTIONFD)))
  {
    // Throw the weapon if it's stacked or a hurl animation
    x = ChrList[cnt].xpos;
    y = ChrList[cnt].ypos;
    z = ChrList[cnt].zpos;
    thrown = spawn_one_character(x, y, z, ChrList[weapon].model, ChrList[cnt].team, 0, ChrList[cnt].turnleftright, ChrList[weapon].name, MAXCHR, &chr_randie);
    if (thrown < MAXCHR)
    {
      ChrList[thrown].iskursed = bfalse;
      ChrList[thrown].ammo = 1;
      ChrList[thrown].alert |= ALERTIFTHROWN;
      velocity = ChrList[cnt].strength / (ChrList[thrown].weight * THROWFIX);
      velocity += MINTHROWVELOCITY;
      if (velocity > MAXTHROWVELOCITY)
      {
        velocity = MAXTHROWVELOCITY;
      }
      tTmp = (0x7FFF + ChrList[cnt].turnleftright) >> 2;
      ChrList[thrown].xvel += turntosin[(tTmp+12288) & TRIGTABLE_MASK] * velocity;
      ChrList[thrown].yvel += turntosin[(tTmp+8192) & TRIGTABLE_MASK] * velocity;
      ChrList[thrown].zvel = DROPZVEL;
      if (ChrList[weapon].ammo <= 1)
      {
        // Poof the item
        detach_character_from_mount(gs, weapon, btrue, bfalse, &chr_randie);
        free_one_character(weapon);
      }
      else
      {
        ChrList[weapon].ammo--;
      }
    }
  }
  else
  {
    // Spawn an attack particle
    if (ChrList[weapon].ammomax == 0 || ChrList[weapon].ammo != 0)
    {
      if (ChrList[weapon].ammo > 0 && CapList[ChrList[weapon].model].isstackable == bfalse)
      {
        ChrList[weapon].ammo--;  // Ammo usage
      }
      //HERE
      if (CapList[ChrList[weapon].model].attackprttype != -1)
      {
        particle = spawn_one_particle(ChrList[weapon].xpos, ChrList[weapon].ypos, ChrList[weapon].zpos, ChrList[cnt].turnleftright, ChrList[weapon].model, CapList[ChrList[weapon].model].attackprttype, weapon, spawngrip, ChrList[cnt].team, cnt, 0, MAXCHR, &chr_randie);
        if (particle != MAXPRT)
        {
          if (CapList[ChrList[weapon].model].attackattached == bfalse)
          {
            // Detach the particle
            if (PipList[PrtList[particle].pip].startontarget == bfalse || PrtList[particle].target == MAXCHR)
            {
              attach_particle_to_character(particle, weapon, spawngrip);
              // Correct Z spacing base, but nothing else...
              PrtList[particle].zpos += PipList[PrtList[particle].pip].zspacingbase;
            }
            PrtList[particle].attachedtocharacter = MAXCHR;
            // Don't spawn in walls
            if (__prthitawall(particle))
            {
              PrtList[particle].xpos = ChrList[weapon].xpos;
              PrtList[particle].ypos = ChrList[weapon].ypos;
              if (__prthitawall(particle))
              {
                PrtList[particle].xpos = ChrList[cnt].xpos;
                PrtList[particle].ypos = ChrList[cnt].ypos;
              }
            }
          }
          else
          {
            // Attached particles get a strength bonus for reeling...
            if(pipcauseknockback[prtpip[particle]]) dampen = (REELBASE + (chrstrength[cnt] / REEL))*3; //Extra knockback?
            else dampen = REELBASE + (chrstrength[cnt] / REEL);								//No, do normal
            PrtList[particle].xvel = PrtList[particle].xvel * dampen;
            PrtList[particle].yvel = PrtList[particle].yvel * dampen;
            PrtList[particle].zvel = PrtList[particle].zvel * dampen;
          }

          // Initial particles get a strength bonus, which may be 0.00
          PrtList[particle].damagebase += (ChrList[cnt].strength * CapList[ChrList[weapon].model].strengthdampen);
          // Initial particles get an enchantment bonus
          PrtList[particle].damagebase += ChrList[weapon].damageboost;
          // Initial particles inherit damage type of weapon
          PrtList[particle].damagetype = ChrList[weapon].damagetargettype;
        }
      }
    }
    else
    {
      ChrList[weapon].ammoknown = btrue;
    }
  }
}

//--------------------------------------------------------------------------------------------
void move_characters(GAME_STATE * gs, Uint32 * rand_idx)
{
  // ZZ> This function handles character physics
  int cnt;
  Uint32 mapud, maplr;
  Uint8 twist, actionready;
  Uint8 speed, framelip, allowedtoattack;
  float level, friction;
  float dvx, dvy, dvmax;
  Uint16 action, weapon, mount, item;
  int distance, volume;
  Uint8 watchtarget, grounded;
  Uint32 chr_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);


  // Move every character
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].on && (!ChrList[cnt].inpack))
    {
      grounded = bfalse;
      valuegopoof = bfalse;
      // Down that ol' damage timer
      ChrList[cnt].damagetime -= (ChrList[cnt].damagetime != 0);


      // Character's old location
      ChrList[cnt].oldx = ChrList[cnt].xpos;
      ChrList[cnt].oldy = ChrList[cnt].ypos;
      ChrList[cnt].oldz = ChrList[cnt].zpos;
      ChrList[cnt].oldturn = ChrList[cnt].turnleftright;
      //            if(ChrList[cnt].attachedto!=MAXCHR)
      //            {
      //                ChrList[cnt].turnleftright = ChrList[ChrList[cnt].attachedto].turnleftright;
      //                if(ChrList[cnt].indolist==bfalse)
      //                {
      //                    ChrList[cnt].xpos = ChrList[ChrList[cnt].attachedto].xpos;
      //                    ChrList[cnt].ypos = ChrList[ChrList[cnt].attachedto].ypos;
      //                    ChrList[cnt].zpos = ChrList[ChrList[cnt].attachedto].zpos;
      //                }
      //            }


      // Texture movement
      ChrList[cnt].uoffset += ChrList[cnt].uoffvel;
      ChrList[cnt].voffset += ChrList[cnt].voffvel;


      if (ChrList[cnt].alive)
      {
        if (ChrList[cnt].attachedto == MAXCHR)
        {
          // Character latches for generalized movement
          dvx = ChrList[cnt].latchx;
          dvy = ChrList[cnt].latchy;


          // Reverse movements for daze
          if (ChrList[cnt].dazetime > 0)
          {
            dvx = -dvx;
            dvy = -dvy;
          }
          // Switch x and y for daze
          if (ChrList[cnt].grogtime > 0)
          {
            dvmax = dvx;
            dvx = dvy;
            dvy = dvmax;
          }



          // Get direction from the DESIRED change in velocity
          if (ChrList[cnt].turnmode == TURNMODEWATCH)
          {
            if ((ABS(dvx) > WATCHMIN || ABS(dvy) > WATCHMIN))
            {
              ChrList[cnt].turnleftright = terp_dir(ChrList[cnt].turnleftright, RAD_TO_TURN(atan2(dvy, dvx)));
            }
          }
          // Face the target
          watchtarget = (ChrList[cnt].turnmode == TURNMODEWATCHTARGET);
          if (watchtarget)
          {
            if (cnt != ChrList[cnt].aitarget)
              ChrList[cnt].turnleftright = terp_dir(ChrList[cnt].turnleftright, RAD_TO_TURN(atan2(ChrList[ChrList[cnt].aitarget].ypos - ChrList[cnt].ypos, ChrList[ChrList[cnt].aitarget].xpos - ChrList[cnt].xpos)) );
          }



          if (MadFrame[ChrList[cnt].frame].framefx&MADFXSTOP)
          {
            dvx = 0;
            dvy = 0;
          }
          else
          {
            // Limit to MAX acceleration
            dvmax = ChrList[cnt].maxaccel;
            if (dvx < -dvmax) dvx = -dvmax;
            if (dvx > dvmax) dvx = dvmax;
            if (dvy < -dvmax) dvy = -dvmax;
            if (dvy > dvmax) dvy = dvmax;
            ChrList[cnt].xvel += dvx;
            ChrList[cnt].yvel += dvy;
          }


          // Get direction from ACTUAL change in velocity
          if (ChrList[cnt].turnmode == TURNMODEVELOCITY)
          {
            if (dvx < -TURNSPD || dvx > TURNSPD || dvy < -TURNSPD || dvy > TURNSPD)
            {
              if (ChrList[cnt].isplayer)
              {
                // Players turn quickly
                ChrList[cnt].turnleftright = terp_dir_fast(ChrList[cnt].turnleftright, RAD_TO_TURN(atan2(dvy, dvx)) );
              }
              else
              {
                // AI turn slowly
                ChrList[cnt].turnleftright = terp_dir(ChrList[cnt].turnleftright, RAD_TO_TURN(atan2(dvy, dvx)) );
              }
            }
          }
          // Otherwise make it spin
          else if (ChrList[cnt].turnmode == TURNMODESPIN)
          {
            ChrList[cnt].turnleftright += SPINRATE;
          }
        }


        // Character latches for generalized buttons
        if (ChrList[cnt].latchbutton != 0)
        {
          if (ChrList[cnt].latchbutton&LATCHBUTTONJUMP)
          {
            if (ChrList[cnt].attachedto != MAXCHR && ChrList[cnt].jumptime == 0)
            {
              detach_character_from_mount(gs, cnt, btrue, btrue, &chr_randie);
              ChrList[cnt].jumptime = JUMPDELAY;
              ChrList[cnt].zvel = DISMOUNTZVEL;
              if (ChrList[cnt].flyheight != 0)
                ChrList[cnt].zvel = DISMOUNTZVELFLY;
              ChrList[cnt].zpos += ChrList[cnt].zvel;
              if (ChrList[cnt].jumpnumberreset != JUMPINFINITE && ChrList[cnt].jumpnumber != 0)
                ChrList[cnt].jumpnumber--;
              // Play the jump sound
              play_sound(ChrList[cnt].xpos, ChrList[cnt].ypos, CapList[ChrList[cnt].model].waveindex[CapList[ChrList[cnt].model].wavejump]);

            }
            if (ChrList[cnt].jumptime == 0 && ChrList[cnt].jumpnumber != 0 && ChrList[cnt].flyheight == 0)
            {
              if (ChrList[cnt].jumpnumberreset != 1 || ChrList[cnt].jumpready)
              {
                // Make the character jump
                ChrList[cnt].hitready = btrue;
                if (ChrList[cnt].inwater)
                {
                  ChrList[cnt].zvel = WATERJUMP;
                }
                else
                {
                  ChrList[cnt].zvel = ChrList[cnt].jump;
                }
                ChrList[cnt].jumptime = JUMPDELAY;
                ChrList[cnt].jumpready = bfalse;
                if (ChrList[cnt].jumpnumberreset != JUMPINFINITE) ChrList[cnt].jumpnumber--;
                // Set to jump animation if not doing anything better
                if (ChrList[cnt].actionready)    play_action(cnt, ACTIONJA, btrue);
                // Play the jump sound (Boing!)
                distance = ABS(GCamera.trackx - ChrList[cnt].xpos) + ABS(GCamera.tracky - ChrList[cnt].ypos);
                volume = -distance;

                if (volume > VOLMIN)
                {
                  play_sound(ChrList[cnt].xpos, ChrList[cnt].ypos, CapList[ChrList[cnt].model].waveindex[CapList[ChrList[cnt].model].wavejump]);
                }

              }
            }
          }
          if ((ChrList[cnt].latchbutton&LATCHBUTTONALTLEFT) && ChrList[cnt].actionready && ChrList[cnt].reloadtime == 0)
          {
            ChrList[cnt].reloadtime = GRABDELAY;
            if (ChrList[cnt].holdingwhich[0] == MAXCHR)
            {
              // Grab left
              play_action(cnt, ACTIONME, bfalse);
            }
            else
            {
              // Drop left
              play_action(cnt, ACTIONMA, bfalse);
            }
          }
          if ((ChrList[cnt].latchbutton&LATCHBUTTONALTRIGHT) && ChrList[cnt].actionready && ChrList[cnt].reloadtime == 0)
          {
            ChrList[cnt].reloadtime = GRABDELAY;
            if (ChrList[cnt].holdingwhich[1] == MAXCHR)
            {
              // Grab right
              play_action(cnt, ACTIONMF, bfalse);
            }
            else
            {
              // Drop right
              play_action(cnt, ACTIONMB, bfalse);
            }
          }
          if ((ChrList[cnt].latchbutton&LATCHBUTTONPACKLEFT) && ChrList[cnt].actionready && ChrList[cnt].reloadtime == 0)
          {
            ChrList[cnt].reloadtime = PACKDELAY;
            item = ChrList[cnt].holdingwhich[0];
            if (item != MAXCHR)
            {
              if ((ChrList[item].iskursed || CapList[ChrList[item].model].istoobig) && CapList[ChrList[item].model].isequipment == bfalse)
              {
                // The item couldn't be put away
                ChrList[item].alert |= ALERTIFNOTPUTAWAY;
              }
              else
              {
                // Put the item into the pack
                add_item_to_character_pack(gs, item, cnt, &chr_randie);
              }
            }
            else
            {
              // Get a new one out and put it in hand
              get_item_from_character_pack(cnt, GRIPLEFT, bfalse);
            }
            // Make it take a little time
            play_action(cnt, ACTIONMG, bfalse);
          }
          if ((ChrList[cnt].latchbutton&LATCHBUTTONPACKRIGHT) && ChrList[cnt].actionready && ChrList[cnt].reloadtime == 0)
          {
            ChrList[cnt].reloadtime = PACKDELAY;
            item = ChrList[cnt].holdingwhich[1];
            if (item != MAXCHR)
            {
              if ((ChrList[item].iskursed || CapList[ChrList[item].model].istoobig) && CapList[ChrList[item].model].isequipment == bfalse)
              {
                // The item couldn't be put away
                ChrList[item].alert |= ALERTIFNOTPUTAWAY;
              }
              else
              {
                // Put the item into the pack
                add_item_to_character_pack(gs, item, cnt, &chr_randie);
              }
            }
            else
            {
              // Get a new one out and put it in hand
              get_item_from_character_pack(cnt, GRIPRIGHT, bfalse);
            }
            // Make it take a little time
            play_action(cnt, ACTIONMG, bfalse);
          }
          if (ChrList[cnt].latchbutton&LATCHBUTTONLEFT && ChrList[cnt].reloadtime == 0)
          {
            // Which weapon?
            weapon = ChrList[cnt].holdingwhich[0];
            if (weapon == MAXCHR)
            {
              // Unarmed means character itself is the weapon
              weapon = cnt;
            }
            action = CapList[ChrList[weapon].model].weaponaction;


            // Can it do it?
            allowedtoattack = btrue;
            if (MadList[ChrList[cnt].model].actionvalid[action] == bfalse || ChrList[weapon].reloadtime > 0 ||
              (CapList[ChrList[weapon].model].needskillidtouse && (check_skills(cnt, CapList[ChrList[weapon].model].idsz[IDSZSKILL]) == bfalse)))
            {
              allowedtoattack = bfalse;
              if (ChrList[weapon].reloadtime == 0)
              {
                // This character can't use this weapon
                ChrList[weapon].reloadtime = 50;
                if (ChrList[cnt].staton)
                {
                  // Tell the player that they can't use this weapon
                  debug_message("%s can't use this item...", ChrList[cnt].name);
                }
              }
            }
            if (action == ACTIONDA)
            {
              allowedtoattack = bfalse;
              if (ChrList[weapon].reloadtime == 0)
              {
                ChrList[weapon].alert = ChrList[weapon].alert | ALERTIFUSED;
              }
            }


            if (allowedtoattack)
            {
              // Rearing mount
              mount = ChrList[cnt].attachedto;
              if (mount != MAXCHR)
              {
                allowedtoattack = CapList[ChrList[mount].model].ridercanattack;
                if (ChrList[mount].ismount && ChrList[mount].alive && ChrList[mount].isplayer == bfalse && ChrList[mount].actionready)
                {
                  if ((action != ACTIONPA || allowedtoattack == bfalse) && ChrList[cnt].actionready)
                  {
                    play_action(ChrList[cnt].attachedto, ACTIONUA + (ego_rand(&ego_rand_seed)&1), bfalse);
                    ChrList[ChrList[cnt].attachedto].alert |= ALERTIFUSED;
                  }
                  else
                  {
                    allowedtoattack = bfalse;
                  }
                }
              }


              // Attack button
              if (allowedtoattack)
              {
                if (ChrList[cnt].actionready && MadList[ChrList[cnt].model].actionvalid[action])
                {
                  // Check mana cost
                  if (ChrList[cnt].mana >= ChrList[weapon].manacost || ChrList[cnt].canchannel)
                  {
                    cost_mana(gs, cnt, ChrList[weapon].manacost, weapon, &chr_randie);
                    // Check life healing
                    ChrList[cnt].life += ChrList[weapon].lifeheal;
                    if (ChrList[cnt].life > ChrList[cnt].lifemax)  ChrList[cnt].life = ChrList[cnt].lifemax;
                    actionready = bfalse;
                    if (action == ACTIONPA)
                      actionready = btrue;
                    action += ego_rand(&ego_rand_seed) & 1;
                    play_action(cnt, action, actionready);
                    if (weapon != cnt)
                    {
                      // Make the weapon attack too
                      play_action(weapon, ACTIONMJ, bfalse);
                      ChrList[weapon].alert = ChrList[weapon].alert | ALERTIFUSED;
                    }
                    else
                    {
                      // Flag for unarmed attack
                      ChrList[cnt].alert |= ALERTIFUSED;
                    }
                  }
                }
              }
            }
          }
          else if (ChrList[cnt].latchbutton&LATCHBUTTONRIGHT && ChrList[cnt].reloadtime == 0)
          {
            // Which weapon?
            weapon = ChrList[cnt].holdingwhich[1];
            if (weapon == MAXCHR)
            {
              // Unarmed means character itself is the weapon
              weapon = cnt;
            }
            action = CapList[ChrList[weapon].model].weaponaction + 2;


            // Can it do it?
            allowedtoattack = btrue;
            if (MadList[ChrList[cnt].model].actionvalid[action] == bfalse || ChrList[weapon].reloadtime > 0 ||
              (CapList[ChrList[weapon].model].needskillidtouse && (check_skills(cnt, CapList[ChrList[weapon].model].idsz[IDSZSKILL]) == bfalse)))
            {
              allowedtoattack = bfalse;
              if (ChrList[weapon].reloadtime == 0)
              {
                // This character can't use this weapon
                ChrList[weapon].reloadtime = 50;
                if (ChrList[cnt].staton)
                {
                  // Tell the player that they can't use this weapon
                  debug_message("%s can't use this item...", ChrList[cnt].name);
                }
              }
            }
            if (action == ACTIONDC)
            {
              allowedtoattack = bfalse;
              if (ChrList[weapon].reloadtime == 0)
              {
                ChrList[weapon].alert = ChrList[weapon].alert | ALERTIFUSED;
              }
            }


            if (allowedtoattack)
            {
              // Rearing mount
              mount = ChrList[cnt].attachedto;
              if (mount != MAXCHR)
              {
                allowedtoattack = CapList[ChrList[mount].model].ridercanattack;
                if (ChrList[mount].ismount && ChrList[mount].alive && ChrList[mount].isplayer == bfalse && ChrList[mount].actionready)
                {
                  if ((action != ACTIONPC || allowedtoattack == bfalse) && ChrList[cnt].actionready)
                  {
                    play_action(ChrList[cnt].attachedto, ACTIONUC + (ego_rand(&ego_rand_seed)&1), bfalse);
                    ChrList[ChrList[cnt].attachedto].alert |= ALERTIFUSED;
                  }
                  else
                  {
                    allowedtoattack = bfalse;
                  }
                }
              }


              // Attack button
              if (allowedtoattack)
              {
                if (ChrList[cnt].actionready && MadList[ChrList[cnt].model].actionvalid[action])
                {
                  // Check mana cost
                  if (ChrList[cnt].mana >= ChrList[weapon].manacost || ChrList[cnt].canchannel)
                  {
                    cost_mana(gs, cnt, ChrList[weapon].manacost, weapon, &chr_randie);
                    // Check life healing
                    ChrList[cnt].life += ChrList[weapon].lifeheal;
                    if (ChrList[cnt].life > ChrList[cnt].lifemax)  ChrList[cnt].life = ChrList[cnt].lifemax;
                    actionready = bfalse;
                    if (action == ACTIONPC)
                      actionready = btrue;
                    action += ego_rand(&ego_rand_seed) & 1;
                    play_action(cnt, action, actionready);
                    if (weapon != cnt)
                    {
                      // Make the weapon attack too
                      play_action(weapon, ACTIONMJ, bfalse);
                      ChrList[weapon].alert = ChrList[weapon].alert | ALERTIFUSED;
                    }
                    else
                    {
                      // Flag for unarmed attack
                      ChrList[cnt].alert |= ALERTIFUSED;
                    }
                  }
                }
              }
            }
          }
        }
      }




      // Is the character in the air?
      level = ChrList[cnt].level;
      if (ChrList[cnt].flyheight == 0)
      {
        ChrList[cnt].zpos += ChrList[cnt].zvel;
        if (ChrList[cnt].zpos > level || (ChrList[cnt].zvel > STOPBOUNCING && ChrList[cnt].zpos > level - STOPBOUNCING))
        {
          // Character is in the air
          ChrList[cnt].jumpready = bfalse;
          ChrList[cnt].zvel += gravity;


          // Down jump timers for flapping characters
          if (ChrList[cnt].jumptime != 0) ChrList[cnt].jumptime--;


          // Airborne characters still get friction to make control easier
          friction = airfriction;
        }
        else
        {
          // Character is on the ground
          ChrList[cnt].zpos = level;
          grounded = btrue;
          if (ChrList[cnt].hitready)
          {
            ChrList[cnt].alert |= ALERTIFHITGROUND;
            ChrList[cnt].hitready = bfalse;
          }


          // Make the characters slide
          twist = Mesh.fanlist[ChrList[cnt].onwhichfan].twist;
          friction = noslipfriction;
          if (Mesh.fanlist[ChrList[cnt].onwhichfan].fx&MESHFXSLIPPY)
          {
            if (wateriswater && (Mesh.fanlist[ChrList[cnt].onwhichfan].fx&MESHFXWATER) && ChrList[cnt].level < watersurfacelevel + RAISE + 1)
            {
              // It says it's slippy, but the water is covering it...
              // Treat exactly as normal
              ChrList[cnt].jumpready = btrue;
              ChrList[cnt].jumpnumber = ChrList[cnt].jumpnumberreset;
              if (ChrList[cnt].jumptime > 0) ChrList[cnt].jumptime--;
              ChrList[cnt].zvel = -ChrList[cnt].zvel * ChrList[cnt].dampen;
              ChrList[cnt].zvel += gravity;
            }
            else
            {
              // It's slippy all right...
              friction = slippyfriction;
              ChrList[cnt].jumpready = btrue;
              if (ChrList[cnt].jumptime > 0) ChrList[cnt].jumptime--;
              if (ChrList[cnt].weight != 65535)
              {
                // Slippy hills make characters slide
                ChrList[cnt].xvel += vellrtwist[twist];
                ChrList[cnt].yvel += veludtwist[twist];
                ChrList[cnt].zvel = -SLIDETOLERANCE;
              }
              if (flattwist[twist])
              {
                // Reset jumping on flat areas of slippiness
                ChrList[cnt].jumpnumber = ChrList[cnt].jumpnumberreset;
              }
            }
          }
          else
          {
            // Reset jumping
            ChrList[cnt].jumpready = btrue;
            ChrList[cnt].jumpnumber = ChrList[cnt].jumpnumberreset;
            if (ChrList[cnt].jumptime > 0) ChrList[cnt].jumptime--;
            ChrList[cnt].zvel = -ChrList[cnt].zvel * ChrList[cnt].dampen;
            ChrList[cnt].zvel += gravity;
          }




          // Characters with sticky butts lie on the surface of the mesh
          if (ChrList[cnt].stickybutt || ChrList[cnt].alive == bfalse)
          {
            maplr = ChrList[cnt].turnmaplr;
            maplr = (maplr << 6) - maplr + maplrtwist[twist];
            mapud = ChrList[cnt].turnmapud;
            mapud = (mapud << 6) - mapud + mapudtwist[twist];
            ChrList[cnt].turnmaplr = maplr >> 6;
            ChrList[cnt].turnmapud = mapud >> 6;
          }
        }
      }
      else
      {
        //  Flying
        ChrList[cnt].jumpready = bfalse;
        ChrList[cnt].zpos += ChrList[cnt].zvel;
        if (level < 0) level = 0; // Don't fall in pits...
        ChrList[cnt].zvel += (level + ChrList[cnt].flyheight - ChrList[cnt].zpos) * FLYDAMPEN;
        if (ChrList[cnt].zpos < level)
        {
          ChrList[cnt].zpos = level;
          ChrList[cnt].zvel = 0;
        }
        // Airborne characters still get friction to make control easier
        friction = airfriction;
      }
      // Move the character
      ChrList[cnt].xpos += ChrList[cnt].xvel;
      if (__chrhitawall(cnt)) { ChrList[cnt].xpos = ChrList[cnt].oldx; ChrList[cnt].xvel = -ChrList[cnt].xvel; }
      ChrList[cnt].ypos += ChrList[cnt].yvel;
      if (__chrhitawall(cnt)) { ChrList[cnt].ypos = ChrList[cnt].oldy; ChrList[cnt].yvel = -ChrList[cnt].yvel; }
      // Apply friction for next time
      ChrList[cnt].xvel = ChrList[cnt].xvel * friction;
      ChrList[cnt].yvel = ChrList[cnt].yvel * friction;


      // Animate the character
      ChrList[cnt].lip = (ChrList[cnt].lip + 64);
      if (ChrList[cnt].lip == 192)
      {
        // Check frame effects
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXACTLEFT)
          character_swipe(gs, cnt, 0, &chr_randie);
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXACTRIGHT)
          character_swipe(gs, cnt, 1, &chr_randie);
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXGRABLEFT)
          character_grab_stuff(cnt, GRIPLEFT, bfalse);
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXGRABRIGHT)
          character_grab_stuff(cnt, GRIPRIGHT, bfalse);
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXCHARLEFT)
          character_grab_stuff(cnt, GRIPLEFT, btrue);
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXCHARRIGHT)
          character_grab_stuff(cnt, GRIPRIGHT, btrue);
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXDROPLEFT)
          detach_character_from_mount(gs, ChrList[cnt].holdingwhich[0], bfalse, btrue, &chr_randie);
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXDROPRIGHT)
          detach_character_from_mount(gs, ChrList[cnt].holdingwhich[1], bfalse, btrue, &chr_randie);
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXPOOF && ChrList[cnt].isplayer == bfalse)
          valuegopoof = btrue;
        if (MadFrame[ChrList[cnt].frame].framefx&MADFXFOOTFALL)
        {
          if (CapList[ChrList[cnt].model].wavefootfall != -1)
          {
            play_sound(ChrList[cnt].xpos, ChrList[cnt].ypos, CapList[ChrList[cnt].model].waveindex[CapList[ChrList[cnt].model].wavefootfall]);
          }
        }
      }
      if (ChrList[cnt].lip == 0)
      {
        // Change frames
        ChrList[cnt].lastframe = ChrList[cnt].frame;
        ChrList[cnt].frame++;
        if (ChrList[cnt].frame == MadList[ChrList[cnt].model].actionend[ChrList[cnt].action])
        {
          // Action finished
          if (ChrList[cnt].keepaction)
          {
            // Keep the last frame going
            ChrList[cnt].frame = ChrList[cnt].lastframe;
          }
          else
          {
            if (ChrList[cnt].loopaction == bfalse)
            {
              // Go on to the next action
              ChrList[cnt].action = ChrList[cnt].nextaction;
              ChrList[cnt].nextaction = ACTIONDA;
            }
            else
            {
              // See if the character is mounted...
              if (ChrList[cnt].attachedto != MAXCHR)
              {
                ChrList[cnt].action = ACTIONMI;
              }
            }
            ChrList[cnt].frame = MadList[ChrList[cnt].model].actionstart[ChrList[cnt].action];
          }
          ChrList[cnt].actionready = btrue;
        }
      }



      // Do "Be careful!" delay
      if (ChrList[cnt].carefultime != 0)
      {
        ChrList[cnt].carefultime--;
      }


      // Get running, walking, sneaking, or dancing, from speed
      if ((ChrList[cnt].keepaction | ChrList[cnt].loopaction) == bfalse)
      {
        framelip = MadFrame[ChrList[cnt].frame].framelip;  // 0 - 15...  Way through animation
        if (ChrList[cnt].actionready && ChrList[cnt].lip == 0 && grounded && ChrList[cnt].flyheight == 0 && (framelip&7) < 2)
        {
          // Do the motion stuff
          speed = ABS((int) ChrList[cnt].xvel) + ABS((int) ChrList[cnt].yvel);
          if (speed < ChrList[cnt].sneakspd)
          {
            //                        ChrList[cnt].nextaction = ACTIONDA;
            // Do boredom
            ChrList[cnt].boretime--;
            if (ChrList[cnt].boretime < 0)
            {
              ChrList[cnt].alert |= ALERTIFBORED;
              ChrList[cnt].boretime = BORETIME;
            }
            else
            {
              // Do standstill
              if (ChrList[cnt].action > ACTIONDD)
              {
                ChrList[cnt].action = ACTIONDA;
                ChrList[cnt].frame = MadList[ChrList[cnt].model].actionstart[ChrList[cnt].action];
              }
            }
          }
          else
          {
            ChrList[cnt].boretime = BORETIME;
            if (speed < ChrList[cnt].walkspd)
            {
              ChrList[cnt].nextaction = ACTIONWA;
              if (ChrList[cnt].action != ACTIONWA)
              {
                ChrList[cnt].frame = MadList[ChrList[cnt].model].frameliptowalkframe[LIPWA][framelip];
                ChrList[cnt].action = ACTIONWA;
              }
            }
            else
            {
              if (speed < ChrList[cnt].runspd)
              {
                ChrList[cnt].nextaction = ACTIONWB;
                if (ChrList[cnt].action != ACTIONWB)
                {
                  ChrList[cnt].frame = MadList[ChrList[cnt].model].frameliptowalkframe[LIPWB][framelip];
                  ChrList[cnt].action = ACTIONWB;
                }
              }
              else
              {
                ChrList[cnt].nextaction = ACTIONWC;
                if (ChrList[cnt].action != ACTIONWC)
                {
                  ChrList[cnt].frame = MadList[ChrList[cnt].model].frameliptowalkframe[LIPWC][framelip];
                  ChrList[cnt].action = ACTIONWC;
                }
              }
            }
          }
        }
      }
      // Do poofing
      if (valuegopoof)
      {
        if (ChrList[cnt].attachedto != MAXCHR)
          detach_character_from_mount(gs, cnt, btrue, bfalse, &chr_randie);
        if (ChrList[cnt].holdingwhich[0] != MAXCHR)
          detach_character_from_mount(gs, ChrList[cnt].holdingwhich[0], btrue, bfalse, &chr_randie);
        if (ChrList[cnt].holdingwhich[1] != MAXCHR)
          detach_character_from_mount(gs, ChrList[cnt].holdingwhich[1], btrue, bfalse, &chr_randie);
        free_inventory(cnt);
        free_one_character(cnt);
      }
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void setup_characters(GAME_STATE * gs, char *modname, Uint32 * rand_idx)
{
  // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
  int currentcharacter = 0, lastcharacter, passage, content, money, level, skin, cnt, tnc, localnumber = 0;
  Uint8 ghost, team, stat, cTmp;
  char *name;
  char itislocal;
  STRING myname, newloadname;
  Uint16 facing, grip, attach;
  Uint32 slot;
  float x, y, z;
  FILE *fileread;
  Uint32 chr_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);


  // Turn all characters off
  free_all_characters(gs);


  // Turn some back on
  snprintf(newloadname, sizeof(newloadname), "%s%s/%s", modname, gs->cd->gamedat_dir, gs->cd->spawn_file);
  fileread = fopen(newloadname, "r");
  currentcharacter = MAXCHR;
  if (fileread)
  {
    while (goto_colon_yesno(fileread))
    {
      fscanf(fileread, "%s", myname);
      name = myname;
      if (myname[0] == 'N' && myname[1] == 'O' && myname[2] == 'N' &&
        myname[3] == 'E' && myname[4] == 0)
      {
        // Random name
        name = NULL;
      }
      cnt = 0;
      while (cnt < 256)
      {
        if (myname[cnt] == '_')  myname[cnt] = ' ';
        cnt++;
      }
      fscanf(fileread, "%d", &slot);
      fscanf(fileread, "%f%f%f", &x, &y, &z); x = x * 128;  y = y * 128;  z = z * 128;
      cTmp = get_first_letter(fileread);
      attach = MAXCHR;
      facing = NORTH;
      grip = GRIPONLY;
      if (cTmp == 'S' || cTmp == 's')  facing = SOUTH;
      if (cTmp == 'E' || cTmp == 'e')  facing = EAST;
      if (cTmp == 'W' || cTmp == 'w')  facing = WEST;
      if (cTmp == 'L' || cTmp == 'l')  { attach = currentcharacter; grip = GRIPLEFT;   }
      if (cTmp == 'R' || cTmp == 'r')  { attach = currentcharacter; grip = GRIPRIGHT;  }
      if (cTmp == 'I' || cTmp == 'i')  { attach = currentcharacter; grip = INVENTORY;  }
      fscanf(fileread, "%d%d%d%d%d", &money, &skin, &passage, &content, &level);
      cTmp = get_first_letter(fileread);
      stat = bfalse;
      if (cTmp == 'T' || cTmp == 't') stat = btrue;
      cTmp = get_first_letter(fileread);
      ghost = bfalse;
      if (cTmp == 'T' || cTmp == 't') ghost = btrue;
      team = (get_first_letter(fileread) - 'A') % MAXTEAM;


      // Spawn the character
      //if (team < gs->mod.minplayers || team >= MAXPLAYER) /* || ->modstate.rts_control == bfalse */
      {
        lastcharacter = spawn_one_character(x, y, z, slot, team, skin, facing, name, MAXCHR, &chr_randie);
        if (lastcharacter < MAXCHR)
        {
          ChrList[lastcharacter].money += money;
          if (ChrList[lastcharacter].money > MAXMONEY)  ChrList[lastcharacter].money = MAXMONEY;
          if (ChrList[lastcharacter].money < 0)  ChrList[lastcharacter].money = 0;
          ChrList[lastcharacter].aicontent = content;
          ChrList[lastcharacter].passage = passage;
          if (attach == MAXCHR)
          {
            // Free character
            currentcharacter = lastcharacter;
          }
          else
          {
            // Attached character
            if (grip != INVENTORY)
            {
              // Wielded character
              attach_character_to_mount(lastcharacter, currentcharacter, grip);
              let_character_think(gs, lastcharacter, &chr_randie);  // Empty the grabbed messages
            }
            else
            {
              // Inventory character
              add_item_to_character_pack(gs, lastcharacter, currentcharacter, &chr_randie);
              ChrList[lastcharacter].alert |= ALERTIFGRABBED;  // Make spellbooks change
              ChrList[lastcharacter].attachedto = currentcharacter;  // Make grab work
              let_character_think(gs, lastcharacter, &chr_randie);  // Empty the grabbed messages
              ChrList[lastcharacter].attachedto = MAXCHR;  // Fix grab
            }
          }
        }

        // Turn on player input devices
        if (stat)
        {
          if (gs->modstate.importamount == 0)
          {
            if (gs->mod.minplayers < 2)
            {
              if (numstat == 0)
              {
                // Single player module
                add_player(gs, lastcharacter, numstat, INPUTMOUSE | INPUTKEY | INPUTJOYA | INPUTJOYB);
              }
            }
            else
            {
              if (!gs->ns->networkon)
              {
                if (gs->mod.minplayers == 2)
                {
                  // Two player hack
                  if (numstat == 0)
                  {
                    // First player
                    add_player(gs, lastcharacter, numstat, INPUTMOUSE | INPUTKEY | INPUTJOYB);
                  }
                  if (numstat == 1)
                  {
                    // Second player
                    add_player(gs, lastcharacter, numstat, INPUTJOYA);
                  }
                }
                else
                {
                  // Three player hack
                  if (numstat == 0)
                  {
                    // First player
                    add_player(gs, lastcharacter, numstat, INPUTKEY);
                  }
                  if (numstat == 1)
                  {
                    // Second player
                    add_player(gs, lastcharacter, numstat, INPUTJOYA);
                  }
                  if (numstat == 2)
                  {
                    // Third player
                    add_player(gs, lastcharacter, numstat, INPUTJOYB | INPUTMOUSE);
                  }
                }
              }
              else
              {
                // One player per machine hack
                if (localmachine == numstat)
                {
                  add_player(gs, lastcharacter, numstat, INPUTMOUSE | INPUTKEY | INPUTJOYA | INPUTJOYB);
                }
              }
            }

            if (numstat < gs->modstate.importamount)
            {
              // Multiplayer import module
              itislocal = bfalse;
              tnc = 0;
              while (tnc < numimport)
              {
                if (CapList[ChrList[lastcharacter].model].importslot == localslot[tnc])
                {
                  itislocal = btrue;
                  localnumber = tnc;
                  tnc = numimport;
                }
                tnc++;
              }


              if (itislocal)
              {
                // It's a local player
                add_player(gs, lastcharacter, numstat, localcontrol[localnumber]);
              }
              else
              {
                // It's a remote player
                add_player(gs, lastcharacter, numstat, INPUTNONE);
              }
            }
            // Turn on the stat display
            add_stat(lastcharacter);
          }

          // Set the starting level
          if (ChrList[lastcharacter].isplayer == bfalse)
          {
            // Let the character gain levels
            level = level - 1;
            while (ChrList[lastcharacter].experiencelevel < level && ChrList[lastcharacter].experience < MAXXP)
            {
              give_experience(lastcharacter, MAXXP, XPDIRECT);
            }
          }
          if (ghost)
          {
            // Make the character a ghost !!!BAD!!!  Can do with enchants
            ChrList[lastcharacter].alpha = 128;
            ChrList[lastcharacter].light = 255;
          }
        }
      }
    }
    fclose(fileread);
  }
  else
  {
    log_error("Error loading module: %s\n", newloadname); //Something went wrong
  }
  clear_messages();


  // Make sure local players are displayed first
  sort_stat();


  // Fix tilting trees problem
  tilt_characters_to_terrain();

  // Assume RTS mode always has players...  So it doesn't quit
  if (gs->modstate.rts_control)  gs->modstate.nolocalplayers = bfalse;
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch(GAME_STATE * gs, Uint16 player)
{
  // ZZ> This function converts input readings to latch settings, so players can
  //     move around
  float newx, newy;
  Uint16 turnsin, turncos, character;
  Uint8 device;
  float dist, scale;
  float inputx, inputy;


  // Check to see if we need to bother
  if (PlaList[player].valid && PlaList[player].device != INPUTNONE)
  {
    // Make life easier
    character = PlaList[player].index;
    device = PlaList[player].device;

    // Clear the player's latch buffers
    PlaList[player].latchbutton = 0;
    PlaList[player].latchx = 0;
    PlaList[player].latchy = 0;

    // Mouse routines
    if ((device & INPUTMOUSE) && GMous.on)
    {
      // Movement
      newx = 0;
      newy = 0;
      if ((CData.autoturncamera == 255 && numlocalpla == 1) ||
        !control_mouse_is_pressed(gs, MOS_CAMERA))  // Don't allow movement in camera control mode
      {
        dist = sqrt(GMous.dx * GMous.dx + GMous.dy * GMous.dy);
        if (dist > 0)
        {
          scale = GMous.sense / dist;
          if (dist < GMous.sense)
          {
            scale = dist / GMous.sense;
          }
          scale = scale / GMous.sense;
          if (ChrList[character].attachedto != MAXCHR)
          {
            // Mounted
            inputx = GMous.dx * ChrList[ChrList[character].attachedto].maxaccel * scale;
            inputy = GMous.dy * ChrList[ChrList[character].attachedto].maxaccel * scale;
          }
          else
          {
            // Unmounted
            inputx = GMous.dx * ChrList[character].maxaccel * scale;
            inputy = GMous.dy * ChrList[character].maxaccel * scale;
          }
          turnsin = (GCamera.turnleftrightone * 16383);
          turnsin = turnsin & TRIGTABLE_MASK;
          turncos = (turnsin + 4096) & TRIGTABLE_MASK;
          if (CData.autoturncamera == 255 &&
            numlocalpla == 1 &&
            control_mouse_is_pressed(gs, MOS_CAMERA) == 0)  inputx = 0;
          newx = (inputx * turntosin[turncos] + inputy * turntosin[turnsin]);
          newy = (-inputx * turntosin[turnsin] + inputy * turntosin[turncos]);
          //                    PlaList[player].latchx+=newx;
          //                    PlaList[player].latchy+=newy;
        }
      }
      PlaList[player].latchx += newx * GMous.cover + GMous.latcholdx * GMous.sustain;
      PlaList[player].latchy += newy * GMous.cover + GMous.latcholdy * GMous.sustain;
      GMous.latcholdx = PlaList[player].latchx;
      GMous.latcholdy = PlaList[player].latchy;
      // Sustain old movements to ease mouse play
      //            PlaList[player].latchx+=GMous.latcholdx*mousesustain;
      //            PlaList[player].latchy+=GMous.latcholdy*mousesustain;
      //            GMous.latcholdx = PlaList[player].latchx;
      //            GMous.latcholdy = PlaList[player].latchy;
      // Read buttons
      if (control_mouse_is_pressed(gs, MOS_JUMP))
        PlaList[player].latchbutton |= LATCHBUTTONJUMP;
      if (control_mouse_is_pressed(gs, MOS_LEFT_USE))
        PlaList[player].latchbutton |= LATCHBUTTONLEFT;
      if (control_mouse_is_pressed(gs, MOS_LEFT_GET))
        PlaList[player].latchbutton |= LATCHBUTTONALTLEFT;
      if (control_mouse_is_pressed(gs, MOS_LEFT_PACK))
        PlaList[player].latchbutton |= LATCHBUTTONPACKLEFT;
      if (control_mouse_is_pressed(gs, MOS_RIGHT_USE))
        PlaList[player].latchbutton |= LATCHBUTTONRIGHT;
      if (control_mouse_is_pressed(gs, MOS_RIGHT_GET))
        PlaList[player].latchbutton |= LATCHBUTTONALTRIGHT;
      if (control_mouse_is_pressed(gs, MOS_RIGHT_PACK))
        PlaList[player].latchbutton |= LATCHBUTTONPACKRIGHT;
    }


    // Joystick A routines
    if ((device & INPUTJOYA) && GJoy[0].on)
    {
      // Movement
      if ((CData.autoturncamera == 255 && numlocalpla == 1) ||
        !control_joya_is_pressed(gs, JOA_CAMERA))
      {
        newx = 0;
        newy = 0;
        inputx = 0;
        inputy = 0;
        dist = sqrt(GJoy[0].x * GJoy[0].x + GJoy[0].y * GJoy[0].y);
        if (dist > 0)
        {
          scale = 1.0 / dist;
          if (ChrList[character].attachedto != MAXCHR)
          {
            // Mounted
            inputx = GJoy[0].x * ChrList[ChrList[character].attachedto].maxaccel * scale;
            inputy = GJoy[0].y * ChrList[ChrList[character].attachedto].maxaccel * scale;
          }
          else
          {
            // Unmounted
            inputx = GJoy[0].x * ChrList[character].maxaccel * scale;
            inputy = GJoy[0].y * ChrList[character].maxaccel * scale;
          }
        }
        turnsin = (GCamera.turnleftrightone * 16383);
        turnsin = turnsin & TRIGTABLE_MASK;
        turncos = (turnsin + 4096) & TRIGTABLE_MASK;
        if (CData.autoturncamera == 255 &&
          numlocalpla == 1 &&
          !control_joya_is_pressed(gs, JOA_CAMERA))  inputx = 0;
        newx = (inputx * turntosin[turncos] + inputy * turntosin[turnsin]);
        newy = (-inputx * turntosin[turnsin] + inputy * turntosin[turncos]);
        PlaList[player].latchx += newx;
        PlaList[player].latchy += newy;
      }
      // Read buttons
      if (control_joya_is_pressed(gs, JOA_JUMP))
        PlaList[player].latchbutton |= LATCHBUTTONJUMP;
      if (control_joya_is_pressed(gs, JOA_LEFT_USE))
        PlaList[player].latchbutton |= LATCHBUTTONLEFT;
      if (control_joya_is_pressed(gs, JOA_LEFT_GET))
        PlaList[player].latchbutton |= LATCHBUTTONALTLEFT;
      if (control_joya_is_pressed(gs, JOA_LEFT_PACK))
        PlaList[player].latchbutton |= LATCHBUTTONPACKLEFT;
      if (control_joya_is_pressed(gs, JOA_RIGHT_USE))
        PlaList[player].latchbutton |= LATCHBUTTONRIGHT;
      if (control_joya_is_pressed(gs, JOA_RIGHT_GET))
        PlaList[player].latchbutton |= LATCHBUTTONALTRIGHT;
      if (control_joya_is_pressed(gs, JOA_RIGHT_PACK))
        PlaList[player].latchbutton |= LATCHBUTTONPACKRIGHT;
    }


    // Joystick B routines
    if ((device & INPUTJOYB) && GJoy[1].on)
    {
      // Movement
      if ((CData.autoturncamera == 255 && numlocalpla == 1) ||
        !control_joyb_is_pressed(gs, JOB_CAMERA))
      {
        newx = 0;
        newy = 0;
        inputx = 0;
        inputy = 0;
        dist = sqrt(GJoy[1].x * GJoy[1].x + GJoy[1].y * GJoy[1].y);
        if (dist > 0)
        {
          scale = 1.0 / dist;
          if (ChrList[character].attachedto != MAXCHR)
          {
            // Mounted
            inputx = GJoy[1].x * ChrList[ChrList[character].attachedto].maxaccel * scale;
            inputy = GJoy[1].y * ChrList[ChrList[character].attachedto].maxaccel * scale;
          }
          else
          {
            // Unmounted
            inputx = GJoy[1].x * ChrList[character].maxaccel * scale;
            inputy = GJoy[1].y * ChrList[character].maxaccel * scale;
          }
        }
        turnsin = (GCamera.turnleftrightone * 16383);
        turnsin = turnsin & TRIGTABLE_MASK;
        turncos = (turnsin + 4096) & TRIGTABLE_MASK;
        if (CData.autoturncamera == 255 &&
          numlocalpla == 1 &&
          !control_joyb_is_pressed(gs, JOB_CAMERA))  inputx = 0;
        newx = (inputx * turntosin[turncos] + inputy * turntosin[turnsin]);
        newy = (-inputx * turntosin[turnsin] + inputy * turntosin[turncos]);
        PlaList[player].latchx += newx;
        PlaList[player].latchy += newy;
      }
      // Read buttons
      if (control_joyb_is_pressed(gs, JOB_JUMP))
        PlaList[player].latchbutton |= LATCHBUTTONJUMP;
      if (control_joyb_is_pressed(gs, JOB_LEFT_USE))
        PlaList[player].latchbutton |= LATCHBUTTONLEFT;
      if (control_joyb_is_pressed(gs, JOB_LEFT_GET))
        PlaList[player].latchbutton |= LATCHBUTTONALTLEFT;
      if (control_joyb_is_pressed(gs, JOB_LEFT_PACK))
        PlaList[player].latchbutton |= LATCHBUTTONPACKLEFT;
      if (control_joyb_is_pressed(gs, JOB_RIGHT_USE))
        PlaList[player].latchbutton |= LATCHBUTTONRIGHT;
      if (control_joyb_is_pressed(gs, JOB_RIGHT_GET))
        PlaList[player].latchbutton |= LATCHBUTTONALTRIGHT;
      if (control_joyb_is_pressed(gs, JOB_RIGHT_PACK))
        PlaList[player].latchbutton |= LATCHBUTTONPACKRIGHT;
    }

    // Keyboard routines
    if ((device & INPUTKEY) && GKeyb.on)
    {
      // Movement
      if (ChrList[character].attachedto != MAXCHR)
      {
        // Mounted
        inputx = (control_key_is_pressed(gs, KEY_RIGHT) - control_key_is_pressed(gs, KEY_LEFT)) * ChrList[ChrList[character].attachedto].maxaccel;
        inputy = (control_key_is_pressed(gs, KEY_DOWN) - control_key_is_pressed(gs, KEY_UP)) * ChrList[ChrList[character].attachedto].maxaccel;
      }
      else
      {
        // Unmounted
        inputx = (control_key_is_pressed(gs, KEY_RIGHT) - control_key_is_pressed(gs, KEY_LEFT)) * ChrList[character].maxaccel;
        inputy = (control_key_is_pressed(gs, KEY_DOWN) - control_key_is_pressed(gs, KEY_UP)) * ChrList[character].maxaccel;
      }
      turnsin = (GCamera.turnleftrightone * 16383);
      turnsin = turnsin & TRIGTABLE_MASK;
      turncos = (turnsin + 4096) & TRIGTABLE_MASK;
      if (CData.autoturncamera == 255 && numlocalpla == 1)  inputx = 0;
      newx = (inputx * turntosin[turncos] + inputy * turntosin[turnsin]);
      newy = (-inputx * turntosin[turnsin] + inputy * turntosin[turncos]);
      PlaList[player].latchx += newx;
      PlaList[player].latchy += newy;
      // Read buttons
      if (control_key_is_pressed(gs, KEY_JUMP))
        PlaList[player].latchbutton |= LATCHBUTTONJUMP;
      if (control_key_is_pressed(gs, KEY_LEFT_USE))
        PlaList[player].latchbutton |= LATCHBUTTONLEFT;
      if (control_key_is_pressed(gs, KEY_LEFT_GET))
        PlaList[player].latchbutton |= LATCHBUTTONALTLEFT;
      if (control_key_is_pressed(gs, KEY_LEFT_PACK))
        PlaList[player].latchbutton |= LATCHBUTTONPACKLEFT;
      if (control_key_is_pressed(gs, KEY_RIGHT_USE))
        PlaList[player].latchbutton |= LATCHBUTTONRIGHT;
      if (control_key_is_pressed(gs, KEY_RIGHT_GET))
        PlaList[player].latchbutton |= LATCHBUTTONALTRIGHT;
      if (control_key_is_pressed(gs, KEY_RIGHT_PACK))
        PlaList[player].latchbutton |= LATCHBUTTONPACKRIGHT;
    }
  }
}

//--------------------------------------------------------------------------------------------
void set_local_latches(GAME_STATE * gs)
{
  // ZZ> This function emulates AI thinkin' by setting latches from input devices
  int cnt;

  cnt = 0;
  while (cnt < MAXPLAYER)
  {
    set_one_player_latch(gs, cnt);
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_onwhichfan(GAME_STATE * gs, Uint32 * rand_idx)
{
  // ZZ> This function figures out which fan characters are on and sets their level
  Uint16 character;
  int x, y, ripand, distance;
  //int volume;
  float level;
  Uint32 game_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);


  // First figure out which fan each character is in
  character = 0;
  while (character < MAXCHR)
  {
    if (ChrList[character].on && (!ChrList[character].inpack))
    {
      x = ChrList[character].xpos;
      y = ChrList[character].ypos;
      x = x >> 7;
      y = y >> 7;
      ChrList[character].onwhichfan = Mesh.fanstart[y] + x;
    }
    character++;
  }

  // Get levels every update
  character = 0;
  while (character < MAXCHR)
  {
    if (ChrList[character].on && (!ChrList[character].inpack))
    {
      level = get_level(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].onwhichfan, ChrList[character].waterwalk) + RAISE;
      if (ChrList[character].alive)
      {
        if (Mesh.fanlist[ChrList[character].onwhichfan].fx&MESHFXDAMAGE && ChrList[character].zpos <= ChrList[character].level + DAMAGERAISE && ChrList[character].attachedto == MAXCHR)
        {
          if ((ChrList[character].damagemodifier[GTile_Dam.type]&DAMAGESHIFT) != 3 && !ChrList[character].invictus) // 3 means they're pretty well immune
          {
            distance = ABS(GCamera.trackx - ChrList[character].xpos) + ABS(GCamera.tracky - ChrList[character].ypos);
            if (distance < GTile_Dam.mindistance)
            {
              GTile_Dam.mindistance = distance;
            }
            if (distance < GTile_Dam.mindistance + 256)
            {
              GTile_Dam.soundtime = 0;
            }
            if (ChrList[character].damagetime == 0)
            {
              damage_character(gs, character, 32768, GTile_Dam.amount, 1, GTile_Dam.type, DAMAGETEAM, ChrList[character].bumplast, DAMFXBLOC | DAMFXARMO, &game_randie);
              ChrList[character].damagetime = DAMAGETILETIME;
            }
            if (GTile_Dam.parttype != -1 && (wldframe&GTile_Dam.partand) == 0)
            {
              spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos,
                0, MAXMODEL, GTile_Dam.parttype, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR, &game_randie);
            }
          }
          if (ChrList[character].reaffirmdamagetype == GTile_Dam.type)
          {
            if ((wldframe&TILEREAFFIRMAND) == 0)
              reaffirm_attached_particles(character, &game_randie);
          }
        }
      }
      if (ChrList[character].zpos < watersurfacelevel && (Mesh.fanlist[ChrList[character].onwhichfan].fx&MESHFXWATER))
      {
        if (ChrList[character].inwater == bfalse)
        {
          // Splash
          if (ChrList[character].attachedto == MAXCHR)
          {
            spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, watersurfacelevel + RAISE,
              0, MAXMODEL, SPLASH, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR, &game_randie);
          }
          ChrList[character].inwater = btrue;
          if (wateriswater)
          {
            ChrList[character].alert |= ALERTIFINWATER;
          }
        }
        else
        {
          if (ChrList[character].zpos > watersurfacelevel - RIPPLETOLERANCE && CapList[ChrList[character].model].ripple)
          {
            // Ripples
            ripand = ((int)ChrList[character].xvel != 0) | ((int)ChrList[character].yvel != 0);
            ripand = RIPPLEAND >> ripand;
            if ((wldframe&ripand) == 0 && ChrList[character].zpos < watersurfacelevel && ChrList[character].alive)
            {
              spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, watersurfacelevel,
                0, MAXMODEL, RIPPLE, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR, &game_randie);
            }
          }
          if (wateriswater && (wldframe&7) == 0)
          {
            ChrList[character].jumpready = btrue;
            ChrList[character].jumpnumber = 1; //ChrList[character].jumpnumberreset;
          }
        }
        ChrList[character].xvel = ChrList[character].xvel * waterfriction;
        ChrList[character].yvel = ChrList[character].yvel * waterfriction;
        ChrList[character].zvel = ChrList[character].zvel * waterfriction;
      }
      else
      {
        ChrList[character].inwater = bfalse;
      }
      ChrList[character].level = level;
    }
    character++;
  }


  // Play the damage tile sound
  if (GTile_Dam.sound >= 0)
  {
    if ((wldframe & 3) == 0)
    {
      // Change the volume...
      /*PORT
      volume = -(GTile_Dam.mindistance + (GTile_Dam.soundtime<<8));
      volume = volume<<VOLSHIFT;
      if(volume > VOLMIN)
      {
      lpDSBuffer[GTile_Dam.sound]->SetVolume(volume);
      }
      if(GTile_Dam.soundtime < TILESOUNDTIME)  GTile_Dam.soundtime++;
      else GTile_Dam.mindistance = 9999;
      */
    }
  }
}

//--------------------------------------------------------------------------------------------
void bump_characters(GAME_STATE * gs)
{
  // ZZ> This function sets handles characters hitting other characters or particles
  Uint16 character, particle, entry, pip, direction;
  Uint32 chara, charb, fanblock, prtidparent, prtidtype, chridvulnerability, eveidremove;
  Sint8 hide;
  int cnt, tnc, dist, chrinblock, prtinblock, enchant, temp;
  float xa, ya, za, xb, yb, zb;
  float ax, ay, nx, ny, scale;  // For deflection
  Uint16 facing;
  Uint32 chr_randie = gs->randie_index;


  RANDIE(gs->randie_index);


  // Clear the lists
  fanblock = 0;
  while (fanblock < numfanblock)
  {
    Mesh.bumplist[fanblock].chrnum = 0;
    Mesh.bumplist[fanblock].prtnum = 0;
    fanblock++;
  }



  // Fill 'em back up
  character = 0;
  while (character < MAXCHR)
  {
    if (ChrList[character].on && (!ChrList[character].inpack) && (ChrList[character].attachedto == MAXCHR || ChrList[character].reaffirmdamagetype != DAMAGENULL))
    {
      hide = CapList[ChrList[character].model].hidestate;
      if (hide == NOHIDE || hide != ChrList[character].aistate)
      {
        ChrList[character].holdingweight = 0;
        fanblock = (((int)ChrList[character].xpos) >> 9) + Mesh.blockstart[((int)ChrList[character].ypos) >> 9];
        // Insert before any other characters on the block
        entry = Mesh.bumplist[fanblock].chr;
        ChrList[character].bumpnext = entry;
        Mesh.bumplist[fanblock].chr = character;
        Mesh.bumplist[fanblock].chrnum++;
      }
    }
    character++;
  }
  particle = 0;
  while (particle < MAXPRT)
  {
    if (PrtList[particle].on && PrtList[particle].bumpsize)
    {
      fanblock = (((int)PrtList[particle].xpos) >> 9) + Mesh.blockstart[((int)PrtList[particle].ypos) >> 9];
      // Insert before any other particles on the block
      entry = Mesh.bumplist[fanblock].prt;
      PrtList[particle].bumpnext = entry;
      Mesh.bumplist[fanblock].prt = particle;
      Mesh.bumplist[fanblock].prtnum++;
    }
    particle++;
  }



  // Check collisions with other characters and bump particles
  // Only check each pair once
  fanblock = 0;
  while (fanblock < numfanblock)
  {
    chara = Mesh.bumplist[fanblock].chr;
    chrinblock = Mesh.bumplist[fanblock].chrnum;
    prtinblock = Mesh.bumplist[fanblock].prtnum;
    cnt = 0;
    while (cnt < chrinblock)
    {
      xa = ChrList[chara].xpos;
      ya = ChrList[chara].ypos;
      za = ChrList[chara].zpos;
      chridvulnerability = CapList[ChrList[chara].model].idsz[IDSZVULNERABILITY];
      // Don't let items bump
      if (ChrList[chara].bumpheight)//ChrList[chara].isitem==bfalse)
      {
        charb = ChrList[chara].bumpnext;  // Don't collide with self
        tnc = cnt + 1;
        while (tnc < chrinblock)
        {
          if (ChrList[charb].bumpheight)//ChrList[charb].isitem==bfalse)
          {
            xb = ChrList[charb].xpos;
            yb = ChrList[charb].ypos;
            zb = ChrList[charb].zpos;
            // First check absolute value diamond
            xb = ABS(xa - xb);
            yb = ABS(ya - yb);
            dist = xb + yb;
            if (dist < ChrList[chara].bumpsizebig || dist < ChrList[charb].bumpsizebig)
            {
              // Then check bounding box square...  Square+Diamond=Octagon
              if ((xb < ChrList[chara].bumpsize || xb < ChrList[charb].bumpsize) &&
                (yb < ChrList[chara].bumpsize || yb < ChrList[charb].bumpsize))
              {
                // Pretend that they collided
                ChrList[chara].bumplast = charb;
                ChrList[charb].bumplast = chara;
                // Now see if either is on top the other like a platform
                if (za > zb + ChrList[charb].bumpheight - PLATTOLERANCE + ChrList[chara].zvel - ChrList[charb].zvel && (CapList[ChrList[chara].model].canuseplatforms || za > zb + ChrList[charb].bumpheight))
                {
                  // Is A falling on B?
                  if (za < zb + ChrList[charb].bumpheight && ChrList[charb].platform)//&&ChrList[chara].flyheight==0)
                  {
                    // A is inside, coming from above
                    ChrList[chara].zpos = (ChrList[chara].zpos) * PLATKEEP + (ChrList[charb].zpos + ChrList[charb].bumpheight + PLATADD) * PLATASCEND;
                    ChrList[chara].xvel += (ChrList[charb].xvel) * platstick;
                    ChrList[chara].yvel += (ChrList[charb].yvel) * platstick;
                    if (ChrList[chara].zvel < ChrList[charb].zvel)
                      ChrList[chara].zvel = ChrList[charb].zvel;
                    ChrList[chara].turnleftright += (ChrList[charb].turnleftright - ChrList[charb].oldturn);
                    ChrList[chara].jumpready = btrue;
                    ChrList[chara].jumpnumber = ChrList[chara].jumpnumberreset;
                    ChrList[charb].holdingweight = ChrList[chara].weight;
                  }
                }
                else
                {
                  if (zb > za + ChrList[chara].bumpheight - PLATTOLERANCE + ChrList[charb].zvel - ChrList[chara].zvel && (CapList[ChrList[charb].model].canuseplatforms || zb > za + ChrList[chara].bumpheight))
                  {
                    // Is B falling on A?
                    if (zb < za + ChrList[chara].bumpheight && ChrList[chara].platform)//&&ChrList[charb].flyheight==0)
                    {
                      // B is inside, coming from above
                      ChrList[charb].zpos = (ChrList[charb].zpos) * PLATKEEP + (ChrList[chara].zpos + ChrList[chara].bumpheight + PLATADD) * PLATASCEND;
                      ChrList[charb].xvel += (ChrList[chara].xvel) * platstick;
                      ChrList[charb].yvel += (ChrList[chara].yvel) * platstick;
                      if (ChrList[charb].zvel < ChrList[chara].zvel)
                        ChrList[charb].zvel = ChrList[chara].zvel;
                      ChrList[charb].turnleftright += (ChrList[chara].turnleftright - ChrList[chara].oldturn);
                      ChrList[charb].jumpready = btrue;
                      ChrList[charb].jumpnumber = ChrList[charb].jumpnumberreset;
                      ChrList[chara].holdingweight = ChrList[charb].weight;
                    }
                  }
                  else
                  {
                    // They are inside each other, which ain't good
                    // Only collide if moving toward the other
                    if (ChrList[chara].xvel > 0)
                    {
                      if (ChrList[chara].xpos < ChrList[charb].xpos) { ChrList[charb].xvel += ChrList[chara].xvel * ChrList[charb].bumpdampen;  ChrList[chara].xvel = -ChrList[chara].xvel * ChrList[chara].bumpdampen;  ChrList[chara].xpos = ChrList[chara].oldx; }
                    }
                    else
                    {
                      if (ChrList[chara].xpos > ChrList[charb].xpos) { ChrList[charb].xvel += ChrList[chara].xvel * ChrList[charb].bumpdampen;  ChrList[chara].xvel = -ChrList[chara].xvel * ChrList[chara].bumpdampen;  ChrList[chara].xpos = ChrList[chara].oldx; }
                    }
                    if (ChrList[chara].yvel > 0)
                    {
                      if (ChrList[chara].ypos < ChrList[charb].ypos) { ChrList[charb].yvel += ChrList[chara].yvel * ChrList[charb].bumpdampen;  ChrList[chara].yvel = -ChrList[chara].yvel * ChrList[chara].bumpdampen;  ChrList[chara].ypos = ChrList[chara].oldy; }
                    }
                    else
                    {
                      if (ChrList[chara].ypos > ChrList[charb].ypos) { ChrList[charb].yvel += ChrList[chara].yvel * ChrList[charb].bumpdampen;  ChrList[chara].yvel = -ChrList[chara].yvel * ChrList[chara].bumpdampen;  ChrList[chara].ypos = ChrList[chara].oldy; }
                    }
                    if (ChrList[charb].xvel > 0)
                    {
                      if (ChrList[charb].xpos < ChrList[chara].xpos) { ChrList[chara].xvel += ChrList[charb].xvel * ChrList[chara].bumpdampen;  ChrList[charb].xvel = -ChrList[charb].xvel * ChrList[charb].bumpdampen;  ChrList[charb].xpos = ChrList[charb].oldx; }
                    }
                    else
                    {
                      if (ChrList[charb].xpos > ChrList[chara].xpos) { ChrList[chara].xvel += ChrList[charb].xvel * ChrList[chara].bumpdampen;  ChrList[charb].xvel = -ChrList[charb].xvel * ChrList[charb].bumpdampen;  ChrList[charb].xpos = ChrList[charb].oldx; }
                    }
                    if (ChrList[charb].yvel > 0)
                    {
                      if (ChrList[charb].ypos < ChrList[chara].ypos) { ChrList[chara].yvel += ChrList[charb].yvel * ChrList[chara].bumpdampen;  ChrList[charb].yvel = -ChrList[charb].yvel * ChrList[charb].bumpdampen;  ChrList[charb].ypos = ChrList[charb].oldy; }
                    }
                    else
                    {
                      if (ChrList[charb].ypos > ChrList[chara].ypos) { ChrList[chara].yvel += ChrList[charb].yvel * ChrList[chara].bumpdampen;  ChrList[charb].yvel = -ChrList[charb].yvel * ChrList[charb].bumpdampen;  ChrList[charb].ypos = ChrList[charb].oldy; }
                    }
                    xa = ChrList[chara].xpos;
                    ya = ChrList[chara].ypos;
                    ChrList[chara].alert = ChrList[chara].alert | ALERTIFBUMPED;
                    ChrList[charb].alert = ChrList[charb].alert | ALERTIFBUMPED;
                  }
                }
              }
            }
          }
          charb = ChrList[charb].bumpnext;
          tnc++;
        }
        // Now double check the last character we bumped into, in case it's a platform
        charb = ChrList[chara].bumplast;
        if (ChrList[charb].on && (!ChrList[charb].inpack) && charb != chara && ChrList[charb].attachedto == MAXCHR && ChrList[chara].bumpheight && ChrList[charb].bumpheight)
        {
          xb = ChrList[charb].xpos;
          yb = ChrList[charb].ypos;
          zb = ChrList[charb].zpos;
          // First check absolute value diamond
          xb = ABS(xa - xb);
          yb = ABS(ya - yb);
          dist = xb + yb;
          if (dist < ChrList[chara].bumpsizebig || dist < ChrList[charb].bumpsizebig)
          {
            // Then check bounding box square...  Square+Diamond=Octagon
            if ((xb < ChrList[chara].bumpsize || xb < ChrList[charb].bumpsize) &&
              (yb < ChrList[chara].bumpsize || yb < ChrList[charb].bumpsize))
            {
              // Now see if either is on top the other like a platform
              if (za > zb + ChrList[charb].bumpheight - PLATTOLERANCE + ChrList[chara].zvel - ChrList[charb].zvel && (CapList[ChrList[chara].model].canuseplatforms || za > zb + ChrList[charb].bumpheight))
              {
                // Is A falling on B?
                if (za < zb + ChrList[charb].bumpheight && ChrList[charb].platform && ChrList[chara].alive)//&&ChrList[chara].flyheight==0)
                {
                  // A is inside, coming from above
                  ChrList[chara].zpos = (ChrList[chara].zpos) * PLATKEEP + (ChrList[charb].zpos + ChrList[charb].bumpheight + PLATADD) * PLATASCEND;
                  ChrList[chara].xvel += (ChrList[charb].xvel) * platstick;
                  ChrList[chara].yvel += (ChrList[charb].yvel) * platstick;
                  if (ChrList[chara].zvel < ChrList[charb].zvel)
                    ChrList[chara].zvel = ChrList[charb].zvel;
                  ChrList[chara].turnleftright += (ChrList[charb].turnleftright - ChrList[charb].oldturn);
                  ChrList[chara].jumpready = btrue;
                  ChrList[chara].jumpnumber = ChrList[chara].jumpnumberreset;
                  if (MadList[ChrList[chara].model].actionvalid[ACTIONMI] && ChrList[chara].alive && ChrList[charb].alive && ChrList[charb].ismount && ChrList[chara].isitem == bfalse && ChrList[charb].holdingwhich[0] == MAXCHR && ChrList[chara].attachedto == MAXCHR && ChrList[chara].jumptime == 0 && ChrList[chara].flyheight == 0)
                  {
                    attach_character_to_mount(chara, charb, GRIPONLY);
                    ChrList[chara].bumplast = chara;
                    ChrList[charb].bumplast = charb;
                  }
                  ChrList[charb].holdingweight = ChrList[chara].weight;
                }
              }
              else
              {
                if (zb > za + ChrList[chara].bumpheight - PLATTOLERANCE + ChrList[charb].zvel - ChrList[chara].zvel && (CapList[ChrList[charb].model].canuseplatforms || zb > za + ChrList[chara].bumpheight))
                {
                  // Is B falling on A?
                  if (zb < za + ChrList[chara].bumpheight && ChrList[chara].platform && ChrList[charb].alive)//&&ChrList[charb].flyheight==0)
                  {
                    // B is inside, coming from above
                    ChrList[charb].zpos = (ChrList[charb].zpos) * PLATKEEP + (ChrList[chara].zpos + ChrList[chara].bumpheight + PLATADD) * PLATASCEND;
                    ChrList[charb].xvel += (ChrList[chara].xvel) * platstick;
                    ChrList[charb].yvel += (ChrList[chara].yvel) * platstick;
                    if (ChrList[charb].zvel < ChrList[chara].zvel)
                      ChrList[charb].zvel = ChrList[chara].zvel;
                    ChrList[charb].turnleftright += (ChrList[chara].turnleftright - ChrList[chara].oldturn);
                    ChrList[charb].jumpready = btrue;
                    ChrList[charb].jumpnumber = ChrList[charb].jumpnumberreset;
                    if (MadList[ChrList[charb].model].actionvalid[ACTIONMI] && ChrList[chara].alive && ChrList[charb].alive && ChrList[chara].ismount && ChrList[charb].isitem == bfalse && ChrList[chara].holdingwhich[0] == MAXCHR && ChrList[charb].attachedto == MAXCHR && ChrList[charb].jumptime == 0 && ChrList[charb].flyheight == 0)
                    {
                      attach_character_to_mount(charb, chara, GRIPONLY);
                      ChrList[chara].bumplast = chara;
                      ChrList[charb].bumplast = charb;
                    }
                    ChrList[chara].holdingweight = ChrList[charb].weight;
                  }
                }
                else
                {
                  // They are inside each other, which ain't good
                  // Only collide if moving toward the other
                  if (ChrList[chara].xvel > 0)
                  {
                    if (ChrList[chara].xpos < ChrList[charb].xpos) { ChrList[charb].xvel += ChrList[chara].xvel * ChrList[charb].bumpdampen;  ChrList[chara].xvel = -ChrList[chara].xvel * ChrList[chara].bumpdampen;  ChrList[chara].xpos = ChrList[chara].oldx; }
                  }
                  else
                  {
                    if (ChrList[chara].xpos > ChrList[charb].xpos) { ChrList[charb].xvel += ChrList[chara].xvel * ChrList[charb].bumpdampen;  ChrList[chara].xvel = -ChrList[chara].xvel * ChrList[chara].bumpdampen;  ChrList[chara].xpos = ChrList[chara].oldx; }
                  }
                  if (ChrList[chara].yvel > 0)
                  {
                    if (ChrList[chara].ypos < ChrList[charb].ypos) { ChrList[charb].yvel += ChrList[chara].yvel * ChrList[charb].bumpdampen;  ChrList[chara].yvel = -ChrList[chara].yvel * ChrList[chara].bumpdampen;  ChrList[chara].ypos = ChrList[chara].oldy; }
                  }
                  else
                  {
                    if (ChrList[chara].ypos > ChrList[charb].ypos) { ChrList[charb].yvel += ChrList[chara].yvel * ChrList[charb].bumpdampen;  ChrList[chara].yvel = -ChrList[chara].yvel * ChrList[chara].bumpdampen;  ChrList[chara].ypos = ChrList[chara].oldy; }
                  }
                  if (ChrList[charb].xvel > 0)
                  {
                    if (ChrList[charb].xpos < ChrList[chara].xpos) { ChrList[chara].xvel += ChrList[charb].xvel * ChrList[chara].bumpdampen;  ChrList[charb].xvel = -ChrList[charb].xvel * ChrList[charb].bumpdampen;  ChrList[charb].xpos = ChrList[charb].oldx; }
                  }
                  else
                  {
                    if (ChrList[charb].xpos > ChrList[chara].xpos) { ChrList[chara].xvel += ChrList[charb].xvel * ChrList[chara].bumpdampen;  ChrList[charb].xvel = -ChrList[charb].xvel * ChrList[charb].bumpdampen;  ChrList[charb].xpos = ChrList[charb].oldx; }
                  }
                  if (ChrList[charb].yvel > 0)
                  {
                    if (ChrList[charb].ypos < ChrList[chara].ypos) { ChrList[chara].yvel += ChrList[charb].yvel * ChrList[chara].bumpdampen;  ChrList[charb].yvel = -ChrList[charb].yvel * ChrList[charb].bumpdampen;  ChrList[charb].ypos = ChrList[charb].oldy; }
                  }
                  else
                  {
                    if (ChrList[charb].ypos > ChrList[chara].ypos) { ChrList[chara].yvel += ChrList[charb].yvel * ChrList[chara].bumpdampen;  ChrList[charb].yvel = -ChrList[charb].yvel * ChrList[charb].bumpdampen;  ChrList[charb].ypos = ChrList[charb].oldy; }
                  }
                  xa = ChrList[chara].xpos;
                  ya = ChrList[chara].ypos;
                  ChrList[chara].alert = ChrList[chara].alert | ALERTIFBUMPED;
                  ChrList[charb].alert = ChrList[charb].alert | ALERTIFBUMPED;
                }
              }
            }
          }
        }
      }
      // Now check collisions with every bump particle in same area
      if (ChrList[chara].alive)
      {
        particle = Mesh.bumplist[fanblock].prt;
        tnc = 0;
        while (tnc < prtinblock)
        {
          xb = PrtList[particle].xpos;
          yb = PrtList[particle].ypos;
          zb = PrtList[particle].zpos;
          // First check absolute value diamond
          xb = ABS(xa - xb);
          yb = ABS(ya - yb);
          dist = xb + yb;
          if (dist < ChrList[chara].bumpsizebig || dist < PrtList[particle].bumpsizebig)
          {
            // Then check bounding box square...  Square+Diamond=Octagon
            if ((xb < ChrList[chara].bumpsize  || xb < PrtList[particle].bumpsize) &&
              (yb < ChrList[chara].bumpsize  || yb < PrtList[particle].bumpsize) &&
              (zb > za - PrtList[particle].bumpheight && zb < za + ChrList[chara].bumpheight + PrtList[particle].bumpheight))
            {
              pip = PrtList[particle].pip;
              if (zb > za + ChrList[chara].bumpheight + PrtList[particle].zvel && PrtList[particle].zvel < 0 && ChrList[chara].platform && PrtList[particle].attachedtocharacter == MAXCHR)
              {
                // Particle is falling on A
                PrtList[particle].zpos = za + ChrList[chara].bumpheight;
                PrtList[particle].zvel = -PrtList[particle].zvel * PipList[pip].dampen;
                PrtList[particle].xvel += (ChrList[chara].xvel) * platstick;
                PrtList[particle].yvel += (ChrList[chara].yvel) * platstick;
              }
              // Check reaffirmation of particles
              if (PrtList[particle].attachedtocharacter != chara)
              {
                if (ChrList[chara].reloadtime == 0)
                {
                  if (ChrList[chara].reaffirmdamagetype == PrtList[particle].damagetype && ChrList[chara].damagetime == 0)
                  {
                    reaffirm_attached_particles(chara, &chr_randie);
                  }
                }
              }
              // Check for missile treatment
              if ((ChrList[chara].damagemodifier[PrtList[particle].damagetype]&3) < 2 ||
                ChrList[chara].missiletreatment == MISNORMAL ||
                PrtList[particle].attachedtocharacter != MAXCHR ||
                (PrtList[particle].chr == chara && PipList[pip].friendlyfire == bfalse) ||
                (ChrList[ChrList[chara].missilehandler].mana < (ChrList[chara].missilecost << 4) && ChrList[ChrList[chara].missilehandler].canchannel == bfalse))
              {
                if ((TeamList[PrtList[particle].team].hatesteam[ChrList[chara].team] || (PipList[pip].friendlyfire && ((chara != PrtList[particle].chr && chara != ChrList[PrtList[particle].chr].attachedto) || PipList[pip].onlydamagefriendly))) && ChrList[chara].invictus == bfalse)
                {
                  spawn_bump_particles(gs, chara, particle, &chr_randie); // Catch on fire
                  if ((PrtList[particle].damagebase | PrtList[particle].damagerand) > 1)
                  {
                    prtidparent = CapList[PrtList[particle].model].idsz[IDSZPARENT];
                    prtidtype = CapList[PrtList[particle].model].idsz[IDSZTYPE];
                    if (ChrList[chara].damagetime == 0 && PrtList[particle].attachedtocharacter != chara && (PipList[pip].damfx&DAMFXARRO) == 0)
                    {
                      // Normal particle damage
                      if (PipList[pip].allowpush)
                      {
                        ChrList[chara].xvel = PrtList[particle].xvel * ChrList[chara].bumpdampen;
                        ChrList[chara].yvel = PrtList[particle].yvel * ChrList[chara].bumpdampen;
                        ChrList[chara].zvel = PrtList[particle].zvel * ChrList[chara].bumpdampen;
                      }
                      direction = RAD_TO_TURN(atan2(PrtList[particle].yvel, PrtList[particle].xvel));
                      direction = ChrList[chara].turnleftright - direction + 32768;
                      // Check all enchants to see if they are removed
                      enchant = ChrList[chara].firstenchant;
                      while (enchant != MAXENCHANT)
                      {
                        eveidremove = EveList[EncList[enchant].eve].removedbyidsz;
                        temp = EncList[enchant].nextenchant;
                        if (eveidremove != IDSZNONE && (eveidremove == prtidtype || eveidremove == prtidparent))
                        {
                          remove_enchant(gs, enchant, &chr_randie);
                        }
                        enchant = temp;
                      }

                      //Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                      if (PipList[pip].intdamagebonus) PrtList[particle].damagebase += ((ChrList[PrtList[particle].chr].intelligence - 3584) * 0.25);  //+1 bonus for every 4 points of intelligence
                      if (PipList[pip].wisdamagebonus) PrtList[particle].damagebase += ((ChrList[PrtList[particle].chr].wisdom - 3584) * 0.25); //and/or wisdom above 14. Below 14 gives -1 instead!

					            //Force Pancake animation?
					            if(pipcauseknockback[pip])
					            {
							          //TODO
					            }

                      // Damage the character
                      if (chridvulnerability != IDSZNONE && (chridvulnerability == prtidtype || chridvulnerability == prtidparent))
                      {
                        damage_character(gs, chara, direction, PrtList[particle].damagebase << 1, PrtList[particle].damagerand << 1, PrtList[particle].damagetype, PrtList[particle].team, PrtList[particle].chr, PipList[pip].damfx, &chr_randie);
                        ChrList[chara].alert |= ALERTIFHITVULNERABLE;
                      }
                      else
                      {
                        damage_character(gs, chara, direction, PrtList[particle].damagebase, PrtList[particle].damagerand, PrtList[particle].damagetype, PrtList[particle].team, PrtList[particle].chr, PipList[pip].damfx, &chr_randie);
                      }
                      // Do confuse effects
                      if ((MadFrame[ChrList[chara].frame].framefx&MADFXINVICTUS) == bfalse || PipList[pip].damfx&DAMFXBLOC)
                      {
                        if (PipList[pip].grogtime != 0 && CapList[ChrList[chara].model].canbegrogged)
                        {
                          ChrList[chara].grogtime += PipList[pip].grogtime;
                          if (ChrList[chara].grogtime < 0)  ChrList[chara].grogtime = 32767;
                          ChrList[chara].alert = ChrList[chara].alert | ALERTIFGROGGED;
                        }
                        if (PipList[pip].dazetime != 0 && CapList[ChrList[chara].model].canbedazed)
                        {
                          ChrList[chara].dazetime += PipList[pip].dazetime;
                          if (ChrList[chara].dazetime < 0)  ChrList[chara].dazetime = 32767;
                          ChrList[chara].alert = ChrList[chara].alert | ALERTIFDAZED;
                        }
                      }
                      // Notify the attacker of a scored hit
                      if (PrtList[particle].chr != MAXCHR)
                      {
                        ChrList[PrtList[particle].chr].alert = ChrList[PrtList[particle].chr].alert | ALERTIFSCOREDAHIT;
                        ChrList[PrtList[particle].chr].hitlast = chara;
                      }
                    }
                    if ((wldframe&31) == 0 && PrtList[particle].attachedtocharacter == chara)
                    {
                      // Attached particle damage ( Burning )
                      if (PipList[pip].xyvelbase == 0)
                      {
                        // Make character limp
                        ChrList[chara].xvel = 0;
                        ChrList[chara].yvel = 0;
                      }
                      damage_character(gs, chara, 32768, PrtList[particle].damagebase, PrtList[particle].damagerand, PrtList[particle].damagetype, PrtList[particle].team, PrtList[particle].chr, PipList[pip].damfx, &chr_randie);
                    }
                  }
                  if (PipList[pip].endbump)
                  {
                    if (PipList[pip].bumpmoney)
                    {
                      if (ChrList[chara].cangrabmoney && ChrList[chara].alive && ChrList[chara].damagetime == 0 && ChrList[chara].money != MAXMONEY)
                      {
                        if (ChrList[chara].ismount)
                        {
                          // Let mounts collect money for their riders
                          if (ChrList[chara].holdingwhich[0] != MAXCHR)
                          {
                            ChrList[ChrList[chara].holdingwhich[0]].money += PipList[pip].bumpmoney;
                            if (ChrList[ChrList[chara].holdingwhich[0]].money > MAXMONEY) ChrList[ChrList[chara].holdingwhich[0]].money = MAXMONEY;
                            if (ChrList[ChrList[chara].holdingwhich[0]].money < 0) ChrList[ChrList[chara].holdingwhich[0]].money = 0;
                            PrtList[particle].time = 1;
                          }
                        }
                        else
                        {
                          // Normal money collection
                          ChrList[chara].money += PipList[pip].bumpmoney;
                          if (ChrList[chara].money > MAXMONEY) ChrList[chara].money = MAXMONEY;
                          if (ChrList[chara].money < 0) ChrList[chara].money = 0;
                          PrtList[particle].time = 1;
                        }
                      }
                    }
                    else
                    {
                      PrtList[particle].time = 1;
                      // Only hit one character, not several
                      PrtList[particle].damagebase = 0;
                      PrtList[particle].damagerand = 1;
                    }
                  }
                }
              }
              else
              {
                if (PrtList[particle].chr != chara)
                {
                  cost_mana(gs, ChrList[chara].missilehandler, (ChrList[chara].missilecost << 4), PrtList[particle].chr, &chr_randie);
                  // Treat the missile
                  if (ChrList[chara].missiletreatment == MISDEFLECT)
                  {
                    // Use old position to find normal
                    ax = PrtList[particle].xpos - PrtList[particle].xvel;
                    ay = PrtList[particle].ypos - PrtList[particle].yvel;
                    ax = ChrList[chara].xpos - ax;
                    ay = ChrList[chara].ypos - ay;
                    // Find size of normal
                    scale = ax * ax + ay * ay;
                    if (scale > 0)
                    {
                      // Make the normal a unit normal
                      scale = sqrt(scale);
                      nx = ax / scale;
                      ny = ay / scale;
                      // Deflect the incoming ray off the normal
                      scale = (PrtList[particle].xvel * nx + PrtList[particle].yvel * ny) * 2;
                      ax = scale * nx;
                      ay = scale * ny;
                      PrtList[particle].xvel = PrtList[particle].xvel - ax;
                      PrtList[particle].yvel = PrtList[particle].yvel - ay;
                    }
                  }
                  else
                  {
                    // Reflect it back in the direction it GCamera.e
                    PrtList[particle].xvel = -PrtList[particle].xvel;
                    PrtList[particle].yvel = -PrtList[particle].yvel;
                  }
                  // Change the owner of the missile
                  if (PipList[pip].homing == bfalse)
                  {
                    PrtList[particle].team = ChrList[chara].team;
                    PrtList[particle].chr = chara;
                  }
                  // Change the direction of the particle
                  if (PipList[pip].rotatetoface)
                  {
                    // Turn to face new direction
                    facing = atan2(PrtList[particle].yvel, PrtList[particle].xvel) * RAD_TO_SHORT;
                    facing += 32768;
                    PrtList[particle].facing = facing;
                  }
                }
              }
            }
          }
          particle = PrtList[particle].bumpnext;
          tnc++;
        }
      }
      chara = ChrList[chara].bumpnext;
      cnt++;
    }
    fanblock++;
  }
}


//--------------------------------------------------------------------------------------------
void stat_return(GAME_STATE * gs, Uint32 * rand_idx)
{
  // ZZ> This function brings mana and life back
  int cnt, owner, target, eve;
  Uint32 stat_randie = *rand_idx;

  RANDIE(*rand_idx);


  // Do reload time
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].reloadtime > 0)
    {
      ChrList[cnt].reloadtime--;
    }
    cnt++;
  }



  // Do stats
  if (gs->cs->statclock == ONESECOND)
  {
    // Reset the clock
    gs->cs->statclock = 0;


    // Do all the characters
    cnt = 0;
    while (cnt < MAXCHR)
    {
      if (ChrList[cnt].on && (!ChrList[cnt].inpack) && ChrList[cnt].alive)
      {
        ChrList[cnt].mana += ChrList[cnt].manareturn;
        if (ChrList[cnt].mana < 0)
          ChrList[cnt].mana = 0;
        if (ChrList[cnt].mana > ChrList[cnt].manamax)
          ChrList[cnt].mana = ChrList[cnt].manamax;
        ChrList[cnt].life += ChrList[cnt].lifereturn;
        if (ChrList[cnt].life < 1)
          ChrList[cnt].life = 1;
        if (ChrList[cnt].life > ChrList[cnt].lifemax)
          ChrList[cnt].life = ChrList[cnt].lifemax;
        if (ChrList[cnt].grogtime > 0)
        {
          ChrList[cnt].grogtime--;
          if (ChrList[cnt].grogtime < 0)
            ChrList[cnt].grogtime = 0;
        }
        if (ChrList[cnt].dazetime > 0)
        {
          ChrList[cnt].dazetime--;
          if (ChrList[cnt].dazetime < 0)
            ChrList[cnt].dazetime = 0;
        }
      }
      cnt++;
    }


    // Run through all the enchants as well
    cnt = 0;
    while (cnt < MAXENCHANT)
    {
      if (EncList[cnt].on)
      {
        if (EncList[cnt].time != 0)
        {
          if (EncList[cnt].time > 0)
          {
            EncList[cnt].time--;
          }
          owner = EncList[cnt].owner;
          target = EncList[cnt].target;
          eve = EncList[cnt].eve;


          // Do drains
          if (ChrList[owner].alive)
          {
            // Change life
            ChrList[owner].life += EncList[cnt].ownerlife;
            if (ChrList[owner].life < 1)
            {
              ChrList[owner].life = 1;
              kill_character(gs, owner, target, &stat_randie);
            }
            if (ChrList[owner].life > ChrList[owner].lifemax)
            {
              ChrList[owner].life = ChrList[owner].lifemax;
            }
            // Change mana
            if (cost_mana(gs, owner, -EncList[cnt].ownermana, target, &stat_randie) == bfalse && EveList[eve].endifcantpay)
            {
              remove_enchant(gs, cnt, &stat_randie);
            }
          }
          else
          {
            if (EveList[eve].stayifnoowner == bfalse)
            {
              remove_enchant(gs, cnt, &stat_randie);
            }
          }
          if (EncList[cnt].on)
          {
            if (ChrList[target].alive)
            {
              // Change life
              ChrList[target].life += EncList[cnt].targetlife;
              if (ChrList[target].life < 1)
              {
                ChrList[target].life = 1;
                kill_character(gs, target, owner, &stat_randie);
              }
              if (ChrList[target].life > ChrList[target].lifemax)
              {
                ChrList[target].life = ChrList[target].lifemax;
              }
              // Change mana
              if (cost_mana(gs, target, -EncList[cnt].targetmana, owner, &stat_randie) == bfalse && EveList[eve].endifcantpay )
              {
                remove_enchant(gs, cnt, &stat_randie);
              }
            }
            else
            {
              remove_enchant(gs, cnt, &stat_randie);
            }
          }
        }
        else
        {
          remove_enchant(gs, cnt, &stat_randie);
        }
      }
      cnt++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void pit_kill(GAME_STATE * gs, Uint32 * rand_idx)
{
  // ZZ> This function kills any character in a deep pit...
  int cnt;
  Uint32 pit_randie = *rand_idx;

  RANDIE(*rand_idx);

  if (pitskill)
  {
    if (pitclock > 19)
    {
      pitclock = 0;


      // Kill any particles that fell in a pit, if they die in water...
      cnt = 0;
      while (cnt < MAXPRT)
      {
        if (PrtList[cnt].on)
        {
          if (PrtList[cnt].zpos < PITDEPTH && PipList[PrtList[cnt].pip].endwater)
          {
            PrtList[cnt].time = 1;
          }
        }
        cnt++;
      }



      // Kill any characters that fell in a pit...
      cnt = 0;
      while (cnt < MAXCHR)
      {
        if (ChrList[cnt].on && ChrList[cnt].alive && ChrList[cnt].inpack == bfalse)
        {
          if (ChrList[cnt].invictus == bfalse && ChrList[cnt].zpos < PITDEPTH && ChrList[cnt].attachedto == MAXCHR)
          {
            // Got one!
            kill_character(gs, cnt, MAXCHR, &pit_randie);
            ChrList[cnt].xvel = 0;
            ChrList[cnt].yvel = 0;
          }
        }
        cnt++;
      }
    }
    else
    {
      pitclock++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void reset_players(GAME_STATE * gs)
{
  // ZZ> This function clears the player list data
  int cnt;

  // Reset the local data stuff
  gs->cs->seekurse = bfalse;
  gs->cs->seeinvisible = bfalse;
  gs->cs->allpladead = bfalse;

  // Reset the initial player data and latches
  cnt = 0;
  while (cnt < MAXPLAYER)
  {
    PlaList[cnt].valid = bfalse;
    PlaList[cnt].index = 0;
    PlaList[cnt].latchx = 0;
    PlaList[cnt].latchy = 0;
    PlaList[cnt].latchbutton = 0;
    PlaList[cnt].device = INPUTNONE;
    cnt++;
  }
  numpla = 0;

  cl_reset(gs->cs);
  sv_reset(gs->ss);
}

//--------------------------------------------------------------------------------------------
void resize_characters()
{
  // ZZ> This function makes the characters get bigger or smaller, depending
  //     on their sizegoto and sizegototime
  int cnt, item, mount;
  Uint8 willgetcaught;
  float newsize;


  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].on && ChrList[cnt].sizegototime)
    {
      // Make sure it won't get caught in a wall
      willgetcaught = bfalse;
      if (ChrList[cnt].sizegoto > ChrList[cnt].fat)
      {
        ChrList[cnt].bumpsize += 10;
        if (__chrhitawall(cnt))
        {
          willgetcaught = btrue;
        }
        ChrList[cnt].bumpsize -= 10;
      }


      // If it is getting caught, simply halt growth until later
      if (willgetcaught == bfalse)
      {
        // Figure out how big it is
        ChrList[cnt].sizegototime--;
        newsize = ChrList[cnt].sizegoto;
        if (ChrList[cnt].sizegototime != 0)
        {
          newsize = (ChrList[cnt].fat * .90) + (newsize * .10);
        }


        // Make it that big...
        ChrList[cnt].fat = newsize;
        ChrList[cnt].shadowsize = ChrList[cnt].shadowsizesave * newsize;
        ChrList[cnt].bumpsize = ChrList[cnt].bumpsizesave * newsize;
        ChrList[cnt].bumpsizebig = ChrList[cnt].bumpsizebigsave * newsize;
        ChrList[cnt].bumpheight = ChrList[cnt].bumpheightsave * newsize;
        ChrList[cnt].weight = CapList[ChrList[cnt].model].weight * newsize;
        if (CapList[ChrList[cnt].model].weight == 255) ChrList[cnt].weight = 65535;


        // Now come up with the magic number
        mount = ChrList[cnt].attachedto;
        if (mount == MAXCHR)
        {
          ChrList[cnt].scale = newsize * MadList[ChrList[cnt].model].scale * 4;
        }
        else
        {
          ChrList[cnt].scale = newsize / (ChrList[mount].fat * 1280);
        }


        // Make in hand items stay the same size...
        newsize = newsize * 1280;
        item = ChrList[cnt].holdingwhich[0];
        if (item != MAXCHR)
          ChrList[item].scale = ChrList[item].fat / newsize;
        item = ChrList[cnt].holdingwhich[1];
        if (item != MAXCHR)
          ChrList[item].scale = ChrList[item].fat / newsize;
      }
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
bool_t export_one_character_name(char *szSaveName, Uint16 character)
{
  // ZZ> This function makes the naming.txt file for the character
  FILE* filewrite;
  int profile;
  char cTmp;
  int cnt, tnc;


  // Can it export?
  profile = ChrList[character].model;
  filewrite = fopen(szSaveName, "w");

  if(NULL==filewrite)
  {
    log_error("Error writing file (%s)\n", szSaveName);
    return bfalse;
  };

  cnt = 0;
  cTmp = ChrList[character].name[0];
  cnt++;
  while (cnt < MAXCAPNAMESIZE && cTmp != 0)
  {
    fprintf(filewrite, ":");
    tnc = 0;
    while (tnc < 8 && cTmp != 0)
    {
      if (cTmp == ' ')
      {
        fprintf(filewrite, "_");
      }
      else
      {
        fprintf(filewrite, "%c", cTmp);
      }
      cTmp = ChrList[character].name[cnt];
      tnc++;
      cnt++;
    }
    fprintf(filewrite, "\n");
    fprintf(filewrite, ":STOP\n\n");
  }
  fclose(filewrite);

}

//--------------------------------------------------------------------------------------------
void export_one_character_profile(char *szSaveName, Uint16 character)
{
  // ZZ> This function creates a data.txt file for the given character.
  //     it is assumed that all enchantments have been done away with
  FILE* filewrite;
  int profile;
  int damagetype, skin;
  char types[10] = "SCPHEFIZ";
  char codes[4];


  // General stuff
  profile = ChrList[character].model;


  // Open the file
  filewrite = fopen(szSaveName, "w");
  if (filewrite)
  {
    // Real general data
    fprintf(filewrite, "Slot number    : -1\n");  // -1 signals a flexible load thing
    funderf(filewrite, "Class name     : ", CapList[profile].classname);
    ftruthf(filewrite, "Uniform light  : ", CapList[profile].uniformlit);
    fprintf(filewrite, "Maximum ammo   : %d\n", CapList[profile].ammomax);
    fprintf(filewrite, "Current ammo   : %d\n", ChrList[character].ammo);
    fgendef(filewrite, "Gender         : ", ChrList[character].gender);
    fprintf(filewrite, "\n");



    // Object stats
    fprintf(filewrite, "Life color     : %d\n", ChrList[character].lifecolor);
    fprintf(filewrite, "Mana color     : %d\n", ChrList[character].manacolor);
    fprintf(filewrite, "Life           : %4.2f\n", ChrList[character].lifemax / 256.0);
    fpairof(filewrite, "Life up        : ", CapList[profile].lifeperlevelbase, CapList[profile].lifeperlevelrand);
    fprintf(filewrite, "Mana           : %4.2f\n", ChrList[character].manamax / 256.0);
    fpairof(filewrite, "Mana up        : ", CapList[profile].manaperlevelbase, CapList[profile].manaperlevelrand);
    fprintf(filewrite, "Mana return    : %4.2f\n", ChrList[character].manareturn / 128.0);
    fpairof(filewrite, "Mana return up : ", CapList[profile].manareturnperlevelbase, CapList[profile].manareturnperlevelrand);
    fprintf(filewrite, "Mana flow      : %4.2f\n", ChrList[character].manaflow / 256.0);
    fpairof(filewrite, "Mana flow up   : ", CapList[profile].manaflowperlevelbase, CapList[profile].manaflowperlevelrand);
    fprintf(filewrite, "STR            : %4.2f\n", ChrList[character].strength / 256.0);
    fpairof(filewrite, "STR up         : ", CapList[profile].strengthperlevelbase, CapList[profile].strengthperlevelrand);
    fprintf(filewrite, "WIS            : %4.2f\n", ChrList[character].wisdom / 256.0);
    fpairof(filewrite, "WIS up         : ", CapList[profile].wisdomperlevelbase, CapList[profile].wisdomperlevelrand);
    fprintf(filewrite, "INT            : %4.2f\n", ChrList[character].intelligence / 256.0);
    fpairof(filewrite, "INT up         : ", CapList[profile].intelligenceperlevelbase, CapList[profile].intelligenceperlevelrand);
    fprintf(filewrite, "DEX            : %4.2f\n", ChrList[character].dexterity / 256.0);
    fpairof(filewrite, "DEX up         : ", CapList[profile].dexterityperlevelbase, CapList[profile].dexterityperlevelrand);
    fprintf(filewrite, "\n");



    // More physical attributes
    fprintf(filewrite, "Size           : %4.2f\n", ChrList[character].sizegoto);
    fprintf(filewrite, "Size up        : %4.2f\n", CapList[profile].sizeperlevel);
    fprintf(filewrite, "Shadow size    : %d\n", CapList[profile].shadowsize);
    fprintf(filewrite, "Bump size      : %d\n", CapList[profile].bumpsize);
    fprintf(filewrite, "Bump height    : %d\n", CapList[profile].bumpheight);
    fprintf(filewrite, "Bump dampen    : %4.2f\n", CapList[profile].bumpdampen);
    fprintf(filewrite, "Weight         : %d\n", CapList[profile].weight);
    fprintf(filewrite, "Jump power     : %4.2f\n", CapList[profile].jump);
    fprintf(filewrite, "Jump number    : %d\n", CapList[profile].jumpnumber);
    fprintf(filewrite, "Sneak speed    : %d\n", CapList[profile].sneakspd);
    fprintf(filewrite, "Walk speed     : %d\n", CapList[profile].walkspd);
    fprintf(filewrite, "Run speed      : %d\n", CapList[profile].runspd);
    fprintf(filewrite, "Fly to height  : %d\n", CapList[profile].flyheight);
    fprintf(filewrite, "Flashing AND   : %d\n", CapList[profile].flashand);
    fprintf(filewrite, "Alpha blending : %d\n", CapList[profile].alpha);
    fprintf(filewrite, "Light blending : %d\n", CapList[profile].light);
    ftruthf(filewrite, "Transfer blend : ", CapList[profile].transferblend);
    fprintf(filewrite, "Sheen          : %d\n", CapList[profile].sheen);
    ftruthf(filewrite, "Phong mapping  : ", CapList[profile].enviro);
    fprintf(filewrite, "Texture X add  : %4.2f\n", CapList[profile].uoffvel / 65535.0);
    fprintf(filewrite, "Texture Y add  : %4.2f\n", CapList[profile].voffvel / 65535.0);
    ftruthf(filewrite, "Sticky butt    : ", CapList[profile].stickybutt);
    fprintf(filewrite, "\n");



    // Invulnerability data
    ftruthf(filewrite, "Invictus       : ", CapList[profile].invictus);
    fprintf(filewrite, "NonI facing    : %d\n", CapList[profile].nframefacing);
    fprintf(filewrite, "NonI angle     : %d\n", CapList[profile].nframeangle);
    fprintf(filewrite, "I facing       : %d\n", CapList[profile].iframefacing);
    fprintf(filewrite, "I angle        : %d\n", CapList[profile].iframeangle);
    fprintf(filewrite, "\n");



    // Skin defenses
    fprintf(filewrite, "Base defense   : ");
    for (skin = 0; skin < MAXSKIN; skin++) { fprintf(filewrite, "%3d ", 255 - CapList[profile].defense[skin]); }
    fprintf(filewrite, "\n");

    for (damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++)
    {
      fprintf(filewrite, "%c damage shift :", types[damagetype]);
      for (skin = 0; skin < MAXSKIN; skin++) { fprintf(filewrite, "%3d ", CapList[profile].damagemodifier[damagetype][skin]&DAMAGESHIFT); };
      fprintf(filewrite, "\n");
    }

    for (damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++)
    {
      fprintf(filewrite, "%c damage code  : ", types[damagetype]);
      for (skin = 0; skin < MAXSKIN; skin++)
      {
        codes[skin] = 'F';
        if (CapList[profile].damagemodifier[damagetype][skin]&DAMAGECHARGE) codes[skin] = 'C';
        if (CapList[profile].damagemodifier[damagetype][skin]&DAMAGEINVERT) codes[skin] = 'T';
        fprintf(filewrite, "%3c ", codes[skin]);
      }
      fprintf(filewrite, "\n");
    }

    fprintf(filewrite, "Acceleration   : ");
    for (skin = 0; skin < MAXSKIN; skin++)
    {
      fprintf(filewrite, "%3.0f ", CapList[profile].maxaccel[skin]*80);
    }
    fprintf(filewrite, "\n");



    // Experience and level data
    fprintf(filewrite, "EXP for 2nd    : %d\n", CapList[profile].experienceforlevel[1]);
    fprintf(filewrite, "EXP for 3rd    : %d\n", CapList[profile].experienceforlevel[2]);
    fprintf(filewrite, "EXP for 4th    : %d\n", CapList[profile].experienceforlevel[3]);
    fprintf(filewrite, "EXP for 5th    : %d\n", CapList[profile].experienceforlevel[4]);
    fprintf(filewrite, "EXP for 6th    : %d\n", CapList[profile].experienceforlevel[5]);
    fprintf(filewrite, "Starting EXP   : %d\n", ChrList[character].experience);
    fprintf(filewrite, "EXP worth      : %d\n", CapList[profile].experienceworth);
    fprintf(filewrite, "EXP exchange   : %5.3f\n", CapList[profile].experienceexchange);
    fprintf(filewrite, "EXPSECRET      : %4.2f\n", CapList[profile].experiencerate[0]);
    fprintf(filewrite, "EXPQUEST       : %4.2f\n", CapList[profile].experiencerate[1]);
    fprintf(filewrite, "EXPDARE        : %4.2f\n", CapList[profile].experiencerate[2]);
    fprintf(filewrite, "EXPKILL        : %4.2f\n", CapList[profile].experiencerate[3]);
    fprintf(filewrite, "EXPMURDER      : %4.2f\n", CapList[profile].experiencerate[4]);
    fprintf(filewrite, "EXPREVENGE     : %4.2f\n", CapList[profile].experiencerate[5]);
    fprintf(filewrite, "EXPTEAMWORK    : %4.2f\n", CapList[profile].experiencerate[6]);
    fprintf(filewrite, "EXPROLEPLAY    : %4.2f\n", CapList[profile].experiencerate[7]);
    fprintf(filewrite, "\n");



    // IDSZ identification tags
    fprintf(filewrite, "IDSZ Parent    : [%s]\n", undo_idsz(CapList[profile].idsz[0]));
    fprintf(filewrite, "IDSZ Type      : [%s]\n", undo_idsz(CapList[profile].idsz[1]));
    fprintf(filewrite, "IDSZ Skill     : [%s]\n", undo_idsz(CapList[profile].idsz[2]));
    fprintf(filewrite, "IDSZ Special   : [%s]\n", undo_idsz(CapList[profile].idsz[3]));
    fprintf(filewrite, "IDSZ Hate      : [%s]\n", undo_idsz(CapList[profile].idsz[4]));
    fprintf(filewrite, "IDSZ Vulnie    : [%s]\n", undo_idsz(CapList[profile].idsz[5]));
    fprintf(filewrite, "\n");



    // Item and damage flags
    ftruthf(filewrite, "Is an item     : ", CapList[profile].isitem);
    ftruthf(filewrite, "Is a mount     : ", CapList[profile].ismount);
    ftruthf(filewrite, "Is stackable   : ", CapList[profile].isstackable);
    ftruthf(filewrite, "Name known     : ", ChrList[character].nameknown);
    ftruthf(filewrite, "Usage known    : ", CapList[profile].usageknown);
    ftruthf(filewrite, "Is exportable  : ", CapList[profile].cancarrytonextmodule);
    ftruthf(filewrite, "Requires skill : ", CapList[profile].needskillidtouse);
    ftruthf(filewrite, "Is platform    : ", CapList[profile].platform);
    ftruthf(filewrite, "Collects money : ", CapList[profile].cangrabmoney);
    ftruthf(filewrite, "Can open stuff : ", CapList[profile].canopenstuff);
    fprintf(filewrite, "\n");



    // Other item and damage stuff
    fdamagf(filewrite, "Damage type    : ", CapList[profile].damagetargettype);
    factiof(filewrite, "Attack type    : ", CapList[profile].weaponaction);
    fprintf(filewrite, "\n");



    // Particle attachments
    fprintf(filewrite, "Attached parts : %d\n", CapList[profile].attachedprtamount);
    fdamagf(filewrite, "Reaffirm type  : ", CapList[profile].attachedprtreaffirmdamagetype);
    fprintf(filewrite, "Particle type  : %d\n", CapList[profile].attachedprttype);
    fprintf(filewrite, "\n");



    // Character hands
    ftruthf(filewrite, "Left valid     : ", CapList[profile].gripvalid[0]);
    ftruthf(filewrite, "Right valid    : ", CapList[profile].gripvalid[1]);
    fprintf(filewrite, "\n");



    // Particle spawning on attack
    ftruthf(filewrite, "Part on weapon : ", CapList[profile].attackattached);
    fprintf(filewrite, "Part type      : %d\n", CapList[profile].attackprttype);
    fprintf(filewrite, "\n");



    // Particle spawning for GoPoof
    fprintf(filewrite, "Poof amount    : %d\n", CapList[profile].gopoofprtamount);
    fprintf(filewrite, "Facing add     : %d\n", CapList[profile].gopoofprtfacingadd);
    fprintf(filewrite, "Part type      : %d\n", CapList[profile].gopoofprttype);
    fprintf(filewrite, "\n");



    // Particle spawning for blood
    ftruthf(filewrite, "Blood valid    : ", CapList[profile].bloodvalid);
    fprintf(filewrite, "Part type      : %d\n", CapList[profile].bloodprttype);
    fprintf(filewrite, "\n");



    // Extra stuff
    ftruthf(filewrite, "Waterwalking   : ", CapList[profile].waterwalk);
    fprintf(filewrite, "Bounce dampen  : %5.3f\n", CapList[profile].dampen);
    fprintf(filewrite, "\n");



    // More stuff
    fprintf(filewrite, "Life healing   : %5.3f\n", CapList[profile].lifeheal / 256.0);
    fprintf(filewrite, "Mana cost      : %5.3f\n", CapList[profile].manacost / 256.0);
    fprintf(filewrite, "Life return    : %d\n", CapList[profile].lifereturn);
    fprintf(filewrite, "Stopped by     : %d\n", CapList[profile].stoppedby);

    for (skin = 0; skin < MAXSKIN; skin++)
    {
      STRING stmp;
      snprintf(stmp, sizeof(stmp), "Skin %d name    : ", skin);
      funderf(filewrite, stmp, CapList[profile].skinname[skin]);
    };

    for (skin = 0; skin < MAXSKIN; skin++)
    {
      fprintf(filewrite, "Skin %d cost    : %d\n", skin, CapList[profile].skincost[skin]);
    };

    fprintf(filewrite, "STR dampen     : %5.3f\n", CapList[profile].strengthdampen);
    fprintf(filewrite, "\n");



    // Another memory lapse
    ftruthf(filewrite, "No rider attak : ", btrue - CapList[profile].ridercanattack);
    ftruthf(filewrite, "Can be dazed   : ", CapList[profile].canbedazed);
    ftruthf(filewrite, "Can be grogged : ", CapList[profile].canbegrogged);
    fprintf(filewrite, "NOT USED       : 0\n");
    fprintf(filewrite, "NOT USED       : 0\n");
    ftruthf(filewrite, "Can see invisi : ", CapList[profile].canseeinvisible);
    fprintf(filewrite, "Kursed chance  : %d\n", ChrList[character].iskursed*100);
    fprintf(filewrite, "Footfall sound : %d\n", CapList[profile].wavefootfall);
    fprintf(filewrite, "Jump sound     : %d\n", CapList[profile].wavejump);
    fprintf(filewrite, "\n");


    // Expansions
    fprintf(filewrite, ":[GOLD] %d\n", ChrList[character].money);

    if (CapList[profile].skindressy&1) fprintf(filewrite, ":[DRES] 0\n");
    if (CapList[profile].skindressy&2) fprintf(filewrite, ":[DRES] 1\n");
    if (CapList[profile].skindressy&4) fprintf(filewrite, ":[DRES] 2\n");
    if (CapList[profile].skindressy&8) fprintf(filewrite, ":[DRES] 3\n");
    if (CapList[profile].resistbumpspawn) fprintf(filewrite, ":[STUK] 0\n");
    if (CapList[profile].istoobig) fprintf(filewrite, ":[PACK] 0\n");
    if (!CapList[profile].reflect) fprintf(filewrite, ":[VAMP] 1\n");
    if (CapList[profile].alwaysdraw) fprintf(filewrite, ":[DRAW] 1\n");
    if (CapList[profile].isranged) fprintf(filewrite, ":[RANG] 1\n");
    if (CapList[profile].hidestate!=NOHIDE) fprintf(filewrite, ":[HIDE] %d\n", CapList[profile].hidestate);
    if (CapList[profile].isequipment) fprintf(filewrite, ":[EQUI] 1\n");
    if (ChrList[character].bumpsizebig==ChrList[character].bumpsize*2) fprintf(filewrite, ":[SQUA] 1\n");
    if (ChrList[character].icon!=CapList[profile].usageknown) fprintf(filewrite, ":[ICON] %d\n", ChrList[character].icon);
    if (CapList[profile].forceshadow) fprintf(filewrite, ":[SHAD] 1\n");

    //Skill expansions
    if (ChrList[character].canseekurse)  fprintf(filewrite, ":[CKUR] 1\n");
    if (ChrList[character].canusearcane) fprintf(filewrite, ":[WMAG] 1\n");
    if (ChrList[character].canjoust)     fprintf(filewrite, ":[JOUS] 1\n");
    if (ChrList[character].canusedivine) fprintf(filewrite, ":[HMAG] 1\n");
    if (ChrList[character].candisarm)    fprintf(filewrite, ":[DISA] 1\n");
    if (ChrList[character].canusetech)   fprintf(filewrite, ":[TECH] 1\n");
    if (ChrList[character].canbackstab)  fprintf(filewrite, ":[STAB] 1\n");
    if (ChrList[character].canuseadvancedweapons) fprintf(filewrite, ":[AWEP] 1\n");
    if (ChrList[character].canusepoison) fprintf(filewrite, ":[POIS] 1\n");
    if (ChrList[character].canread)		fprintf(filewrite, ":[READ] 1\n");

    //General exported character information
    fprintf(filewrite, ":[PLAT] %d\n", CapList[profile].canuseplatforms);
    fprintf(filewrite, ":[SKIN] %d\n", (ChrList[character].texture - MadList[profile].skinstart) % MAXSKIN);
    fprintf(filewrite, ":[CONT] %d\n", ChrList[character].aicontent);
    fprintf(filewrite, ":[STAT] %d\n", ChrList[character].aistate);
    fprintf(filewrite, ":[LEVL] %d\n", ChrList[character].experiencelevel);
    fclose(filewrite);
  }
}

//--------------------------------------------------------------------------------------------
void export_one_character_skin(char *szSaveName, Uint16 character)
{
  // ZZ> This function creates a skin.txt file for the given character.
  FILE* filewrite;
  int profile;


  // General stuff
  profile = ChrList[character].model;


  // Open the file
  filewrite = fopen(szSaveName, "w");
  if (filewrite)
  {
    fprintf(filewrite, "This file is used only by the import menu\n");
    fprintf(filewrite, ": %d\n", (ChrList[character].texture - MadList[profile].skinstart) % MAXSKIN);
    fclose(filewrite);
  }
}

//--------------------------------------------------------------------------------------------
int load_one_character_profile(char *szLoadName)
{
  // ZZ> This function fills a character profile with data from CData.data_file, returning
  // the object slot that the profile was stuck into.  It may cause the program
  // to abort if bad things happen.
  FILE* fileread;
  int object=MAXMODEL, skin, cnt;
  int iTmp;
  float fTmp;
  char cTmp;
  int damagetype, level, xptype;
  IDSZ idsz;

  // Open the file
  fileread = fopen(szLoadName, "r");
  //printf(" DIAG: trying to read %s\n",szLoadName);
  if (fileread != NULL)
  {
    globalname = szLoadName;
    // Read in the object slot
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); object = iTmp;
    if (object < 0)
    {
      if (importobject < 0)
      {
        log_error("Object slot number %i is invalid. (%s) \n", object, szLoadName);
      }
      else
      {
        object = importobject;
      }
    }



    // Read in the real general data
    goto_colon(fileread);  get_name(fileread, CapList[object].classname);


    // Make sure we don't load over an existing model
    if (MadList[object].used)
    {
      log_error("Object slot %i is already used. (%s)\n", object, szLoadName);
    }
    MadList[object].used = btrue;


    // Light cheat
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].uniformlit = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].uniformlit = btrue;

    // Ammo
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].ammomax = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].ammo = iTmp;
    // Gender
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].gender = GENOTHER;
    if (cTmp == 'F' || cTmp == 'f')  CapList[object].gender = GENFEMALE;
    if (cTmp == 'M' || cTmp == 'm')  CapList[object].gender = GENMALE;
    if (cTmp == 'R' || cTmp == 'r')  CapList[object].gender = GENRANDOM;
    // Read in the object stats
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].lifecolor = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].manacolor = iTmp;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].lifebase = pairbase;  CapList[object].liferand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].lifeperlevelbase = pairbase;  CapList[object].lifeperlevelrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].manabase = pairbase;  CapList[object].manarand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].manaperlevelbase = pairbase;  CapList[object].manaperlevelrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].manareturnbase = pairbase;  CapList[object].manareturnrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].manareturnperlevelbase = pairbase;  CapList[object].manareturnperlevelrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].manaflowbase = pairbase;  CapList[object].manaflowrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].manaflowperlevelbase = pairbase;  CapList[object].manaflowperlevelrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].strengthbase = pairbase;  CapList[object].strengthrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].strengthperlevelbase = pairbase;  CapList[object].strengthperlevelrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].wisdombase = pairbase;  CapList[object].wisdomrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].wisdomperlevelbase = pairbase;  CapList[object].wisdomperlevelrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].intelligencebase = pairbase;  CapList[object].intelligencerand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].intelligenceperlevelbase = pairbase;  CapList[object].intelligenceperlevelrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].dexteritybase = pairbase;  CapList[object].dexterityrand = pairrand;
    goto_colon(fileread);  read_pair(fileread);
    CapList[object].dexterityperlevelbase = pairbase;  CapList[object].dexterityperlevelrand = pairrand;

    // More physical attributes
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].size = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].sizeperlevel = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].shadowsize = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].bumpsize = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].bumpheight = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].bumpdampen = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].weight = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].jump = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].jumpnumber = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].sneakspd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].walkspd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].runspd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].flyheight = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].flashand = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].alpha = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].light = iTmp;
    if (CapList[object].light < 0xff)
    {
      CapList[object].alpha = MIN(CapList[object].alpha, 0xff - CapList[object].light);
    };

    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].transferblend = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].transferblend = btrue;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].sheen = iTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].enviro = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].enviro = btrue;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].uoffvel = fTmp * 65535;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].voffvel = fTmp * 65535;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].stickybutt = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].stickybutt = btrue;


    // Invulnerability data
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].invictus = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].invictus = btrue;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].nframefacing = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].nframeangle = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].iframefacing = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].iframeangle = iTmp;
    // Resist burning and stuck arrows with nframe angle of 1 or more
    if (CapList[object].nframeangle > 0)
    {
      if (CapList[object].nframeangle == 1)
      {
        CapList[object].nframeangle = 0;
      }
    }


    // Skin defenses ( 4 skins )
    goto_colon(fileread);
    for (skin = 0; skin < MAXSKIN; skin++)
    { fscanf(fileread, "%d", &iTmp);  CapList[object].defense[skin] = 255 - iTmp; };

    for (damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++)
    {
      goto_colon(fileread);
      for (skin = 0;skin < MAXSKIN;skin++)
      { fscanf(fileread, "%d", &iTmp);  CapList[object].damagemodifier[damagetype][skin] = iTmp; };
    }

    for (damagetype = 0; damagetype < MAXDAMAGETYPE; damagetype++)
    {
      goto_colon(fileread);
      for (skin = 0; skin < MAXSKIN; skin++)
      {
        cTmp = get_first_letter(fileread);
        if (cTmp == 'T' || cTmp == 't')  CapList[object].damagemodifier[damagetype][skin] |= DAMAGEINVERT;
        if (cTmp == 'C' || cTmp == 'c')  CapList[object].damagemodifier[damagetype][skin] |= DAMAGECHARGE;
      }
    }

    goto_colon(fileread);
    for (skin = 0;skin < MAXSKIN;skin++)
    { fscanf(fileread, "%f", &fTmp);  CapList[object].maxaccel[skin] = fTmp / 80.0; };


    // Experience and level data
    CapList[object].experienceforlevel[0] = 0;
    for (level = 1; level < MAXLEVEL; level++)
    { goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].experienceforlevel[level] = iTmp; }
    CapList[object].experiencepower  = log(CapList[object].experienceforlevel[MAXLEVEL-1]) - log(CapList[object].experienceforlevel[1]);
    CapList[object].experiencepower /= log( (MAXLEVEL-1) ) - log(1);
    CapList[object].experiencecoeff = CapList[object].experienceforlevel[MAXLEVEL-1] / pow(MAXLEVEL-1,CapList[object].experiencepower);


    goto_colon(fileread);  read_pair(fileread);
    pairbase = pairbase >> 8;
    pairrand = pairrand >> 8;
    if (pairrand < 1)  pairrand = 1;
    CapList[object].experiencebase = pairbase;
    CapList[object].experiencerand = pairrand;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].experienceworth = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].experienceexchange = fTmp;

    for (xptype = 0; xptype < MAXEXPERIENCETYPE; xptype++)
    { goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].experiencerate[xptype] = fTmp + 0.001; }


    // IDSZ tags
    for (cnt = 0; cnt < MAXIDSZ; cnt++)
    { goto_colon(fileread);  iTmp = get_idsz(fileread);  CapList[object].idsz[cnt] = iTmp; }


    // Item and damage flags
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].isitem = bfalse;  CapList[object].ripple = btrue;
    if (cTmp == 'T' || cTmp == 't')  { CapList[object].isitem = btrue; CapList[object].ripple = bfalse; }
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].ismount = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].ismount = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].isstackable = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].isstackable = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].nameknown = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].nameknown = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].usageknown = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].usageknown = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].cancarrytonextmodule = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].cancarrytonextmodule = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].needskillidtouse = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].needskillidtouse = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].platform = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].platform = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].cangrabmoney = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].cangrabmoney = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].canopenstuff = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].canopenstuff = btrue;



    // More item and damage stuff
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if (cTmp == 'S' || cTmp == 's')  CapList[object].damagetargettype = DAMAGESLASH;
    if (cTmp == 'C' || cTmp == 'c')  CapList[object].damagetargettype = DAMAGECRUSH;
    if (cTmp == 'P' || cTmp == 'p')  CapList[object].damagetargettype = DAMAGEPOKE;
    if (cTmp == 'H' || cTmp == 'h')  CapList[object].damagetargettype = DAMAGEHOLY;
    if (cTmp == 'E' || cTmp == 'e')  CapList[object].damagetargettype = DAMAGEEVIL;
    if (cTmp == 'F' || cTmp == 'f')  CapList[object].damagetargettype = DAMAGEFIRE;
    if (cTmp == 'I' || cTmp == 'i')  CapList[object].damagetargettype = DAMAGEICE;
    if (cTmp == 'Z' || cTmp == 'z')  CapList[object].damagetargettype = DAMAGEZAP;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].weaponaction = what_action(cTmp);


    // Particle attachments
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].attachedprtamount = iTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if (cTmp == 'N' || cTmp == 'n')  CapList[object].attachedprtreaffirmdamagetype = DAMAGENULL;
    if (cTmp == 'S' || cTmp == 's')  CapList[object].attachedprtreaffirmdamagetype = DAMAGESLASH;
    if (cTmp == 'C' || cTmp == 'c')  CapList[object].attachedprtreaffirmdamagetype = DAMAGECRUSH;
    if (cTmp == 'P' || cTmp == 'p')  CapList[object].attachedprtreaffirmdamagetype = DAMAGEPOKE;
    if (cTmp == 'H' || cTmp == 'h')  CapList[object].attachedprtreaffirmdamagetype = DAMAGEHOLY;
    if (cTmp == 'E' || cTmp == 'e')  CapList[object].attachedprtreaffirmdamagetype = DAMAGEEVIL;
    if (cTmp == 'F' || cTmp == 'f')  CapList[object].attachedprtreaffirmdamagetype = DAMAGEFIRE;
    if (cTmp == 'I' || cTmp == 'i')  CapList[object].attachedprtreaffirmdamagetype = DAMAGEICE;
    if (cTmp == 'Z' || cTmp == 'z')  CapList[object].attachedprtreaffirmdamagetype = DAMAGEZAP;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].attachedprttype = iTmp;


    // Character hands
    CapList[object].gripvalid[0] = bfalse;
    CapList[object].gripvalid[1] = bfalse;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if (cTmp == 'T' || cTmp == 't')  CapList[object].gripvalid[0] = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if (cTmp == 'T' || cTmp == 't')  CapList[object].gripvalid[1] = btrue;




    // Attack order ( weapon )
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].attackattached = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].attackattached = btrue;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].attackprttype = iTmp;


    // GoPoof
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].gopoofprtamount = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].gopoofprtfacingadd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].gopoofprttype = iTmp;


    // Blood
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].bloodvalid = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].bloodvalid = btrue;
    if (cTmp == 'U' || cTmp == 'u')  CapList[object].bloodvalid = ULTRABLOODY;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].bloodprttype = iTmp;


    // Stuff I forgot
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].waterwalk = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].waterwalk = btrue;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].dampen = fTmp;


    // More stuff I forgot
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].lifeheal = fTmp * 256;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].manacost = fTmp * 256;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].lifereturn = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].stoppedby = iTmp | MESHFXIMPASS;

    for (skin = 0;skin < MAXSKIN;skin++)
    { goto_colon(fileread);  get_name(fileread, CapList[object].skinname[skin]); };

    for (skin = 0;skin < MAXSKIN;skin++)
    { goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  CapList[object].skincost[skin] = iTmp; };

    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  CapList[object].strengthdampen = fTmp;



    // Another memory lapse
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    CapList[object].ridercanattack = btrue;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].ridercanattack = bfalse;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);  // Can be dazed
    CapList[object].canbedazed = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].canbedazed = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);  // Can be grogged
    CapList[object].canbegrogged = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].canbegrogged = btrue;
    goto_colon(fileread);  // !!!BAD!!! Life add
    goto_colon(fileread);  // !!!BAD!!! Mana add
    goto_colon(fileread);  cTmp = get_first_letter(fileread);  // Can see invisible
    CapList[object].canseeinvisible = bfalse;
    if (cTmp == 'T' || cTmp == 't')  CapList[object].canseeinvisible = btrue;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);      // Chance of kursed
    CapList[object].kursechance = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);      // Footfall sound
    if (iTmp < -1) iTmp = -1;
    if (iTmp > MAXWAVE - 1) iTmp = MAXWAVE - 1;
    CapList[object].wavefootfall = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);      // Jump sound
    if (iTmp < -1) iTmp = -1;
    if (iTmp > MAXWAVE - 1) iTmp = MAXWAVE - 1;
    CapList[object].wavejump = iTmp;


    // Clear expansions...
    CapList[object].skindressy = bfalse;
    CapList[object].resistbumpspawn = bfalse;
    CapList[object].istoobig = bfalse;
    CapList[object].reflect = btrue;
    CapList[object].alwaysdraw = bfalse;
    CapList[object].isranged = bfalse;
    CapList[object].hidestate = NOHIDE;
    CapList[object].isequipment = bfalse;
    CapList[object].bumpsizebig = CapList[object].bumpsize + (CapList[object].bumpsize >> 1);
    CapList[object].money = 0;
    CapList[object].icon = CapList[object].usageknown;
    CapList[object].forceshadow = bfalse;
    CapList[object].skinoverride = NOSKINOVERRIDE;
    CapList[object].contentoverride = 0;
    CapList[object].stateoverride = 0;
    CapList[object].leveloverride = 0;
    CapList[object].canuseplatforms = !CapList[object].platform;

    //Reset Skill Expansions
    CapList[object].canseekurse = bfalse;
    CapList[object].canusearcane = bfalse;
    CapList[object].canjoust = bfalse;
    CapList[object].canusedivine = bfalse;
    CapList[object].candisarm = bfalse;
    CapList[object].canusetech = bfalse;
    CapList[object].canbackstab = bfalse;
    CapList[object].canusepoison = bfalse;
    CapList[object].canuseadvancedweapons = bfalse;

    // Read expansions
    while (goto_colon_yesno(fileread))
    {
      idsz = get_idsz(fileread);
      fscanf(fileread, "%c%d", &cTmp, &iTmp);
      if (MAKE_IDSZ("GOLD") == idsz)  CapList[object].money = iTmp;
      if (MAKE_IDSZ("STUK") == idsz)  CapList[object].resistbumpspawn = 1 - iTmp;
      if (MAKE_IDSZ("PACK") == idsz)  CapList[object].istoobig = 1 - iTmp;
      if (MAKE_IDSZ("VAMP") == idsz)  CapList[object].reflect = 1 - iTmp;
      if (MAKE_IDSZ("DRAW") == idsz)  CapList[object].alwaysdraw = iTmp;
      if (MAKE_IDSZ("RANG") == idsz)  CapList[object].isranged = iTmp;
      if (MAKE_IDSZ("HIDE") == idsz)  CapList[object].hidestate = iTmp;
      if (MAKE_IDSZ("EQUI") == idsz)  CapList[object].isequipment = iTmp;
      if (MAKE_IDSZ("SQUA") == idsz)  CapList[object].bumpsizebig = CapList[object].bumpsize << 1;
      if (MAKE_IDSZ("ICON") == idsz)  CapList[object].icon = iTmp;
      if (MAKE_IDSZ("SHAD") == idsz)  CapList[object].forceshadow = iTmp;
      if (MAKE_IDSZ("SKIN") == idsz)  CapList[object].skinoverride = iTmp % MAXSKIN;
      if (MAKE_IDSZ("CONT") == idsz)  CapList[object].contentoverride = iTmp;
      if (MAKE_IDSZ("STAT") == idsz)  CapList[object].stateoverride = iTmp;
      if (MAKE_IDSZ("LEVL") == idsz)  CapList[object].leveloverride = iTmp;
      if (MAKE_IDSZ("PLAT") == idsz)  CapList[object].canuseplatforms = iTmp;
      if (MAKE_IDSZ("RIPP") == idsz)  CapList[object].ripple = iTmp;

      //Skill Expansions
      // [CKUR] Can it see kurses?
      if (MAKE_IDSZ("CKUR") == idsz)  CapList[object].canseekurse  = iTmp;
      // [WMAG] Can the character use arcane spellbooks?
      if (MAKE_IDSZ("WMAG") == idsz)  CapList[object].canusearcane = iTmp;
      // [JOUS] Can the character joust with a lance?
      if (MAKE_IDSZ("JOUS") == idsz)  CapList[object].canusearcane = iTmp;
      // [HMAG] Can the character use divine spells?
      if (MAKE_IDSZ("HMAG") == idsz)  CapList[object].canusedivine = iTmp;
      // [TECH] Able to use items technological items?
      if (MAKE_IDSZ("TECH") == idsz)  CapList[object].canusetech   = iTmp;
      // [DISA] Find and disarm traps?
      if (MAKE_IDSZ("DISA") == idsz)  CapList[object].canusearcane = iTmp;
      // [STAB] Backstab and murder?
      if (idsz == MAKE_IDSZ("STAB"))  CapList[object].canbackstab = iTmp;
      // [AWEP] Profiency with advanced weapons?
      if (idsz == MAKE_IDSZ("AWEP"))  CapList[object].canuseadvancedweapons = iTmp;
      // [POIS] Use poison without err?
      if (idsz == MAKE_IDSZ("POIS"))  CapList[object].canusepoison = iTmp;
    }

    fclose(fileread);
  }
  else
  {
    // The data file wasn't found
    log_error("Data.txt could not be correctly read! (%s) \n", szLoadName);
  }

  return object;
}

//--------------------------------------------------------------------------------------------
int get_skin(char *filename)
{
  // ZZ> This function reads the skin.txt file...
  FILE*   fileread;
  int skin;


  skin = 0;
  fileread = fopen(filename, "r");
  if (fileread)
  {
    goto_colon_yesno(fileread);
    fscanf(fileread, "%d", &skin);
    skin %= MAXSKIN;
    fclose(fileread);
  }
  return skin;
}

//--------------------------------------------------------------------------------------------
void check_player_import(char *dirname)
{
  // ZZ> This function figures out which players may be imported, and loads basic
  //     data for each
  char searchname[128];
  char filename[128];
  int skin;
  bool_t keeplooking;
  const char *foundfile;


  // Set up...
  numloadplayer = 0;

  // Search for all objects
  snprintf(searchname, sizeof(searchname), "%s/*.obj", dirname);
  foundfile = fs_findFirstFile(dirname, "obj");
  keeplooking = 1;
  if (foundfile != NULL)
  {
    while (keeplooking && numloadplayer < MAXLOADPLAYER)
    {
      //fprintf(stderr,"foundfile=%s keeplooking=%d numload=%d/%d\n",foundfile,keeplooking,numloadplayer,MAXLOADPLAYER);
      prime_names();
      strncpy(loadplayerdir[numloadplayer], foundfile, sizeof(loadplayerdir[numloadplayer]));

      snprintf(filename, sizeof(filename), "%s/%s/%s", dirname, foundfile, CData.skin_file);
      skin = get_skin(filename);

      snprintf(filename, sizeof(filename), "%s/%s/tris.md2", dirname, foundfile);
      load_one_md2(filename, numloadplayer);

      snprintf(filename, sizeof(filename), "%s/%s/icon%d.bmp", dirname, foundfile, skin);
      load_one_icon(filename);

      snprintf(filename, sizeof(filename), "%s/%s/naming.txt", dirname, foundfile);
      read_naming(0, filename);
      naming_names(0);
      strncpy(loadplayername[numloadplayer], namingnames, sizeof(loadplayername[numloadplayer]));

      numloadplayer++;

      foundfile = fs_findNextFile();
      if (foundfile == NULL) keeplooking = 0; else keeplooking = 1;
    }
  }
  fs_findClose();

  nullicon = globalnumicon;
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.nullicon_bitmap);
  load_one_icon(CStringTmp1);

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.keybicon_bitmap);
  GKeyb.icon = globalnumicon;
  load_one_icon(CStringTmp1);

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.mousicon_bitmap);
  GMous.icon = globalnumicon;
  load_one_icon(CStringTmp1);

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.joyaicon_bitmap);
  GJoy[0].icon = globalnumicon;
  load_one_icon(CStringTmp1);

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s", CData.basicdat_dir, CData.joybicon_bitmap);
  GJoy[1].icon = globalnumicon;
  load_one_icon(CStringTmp1);


  GKeyb.player = 0;
  GMous.player = 0;
  GJoy[0].player = 0;
  GJoy[1].player = 0;
}

//--------------------------------------------------------------------------------------------
bool_t check_skills(int who, Uint32 whichskill)
{
  // ZF> This checks if the specified character has the required skill. Returns btrue if true
  // and bfalse if not. Also checks Skill expansions.
  bool_t result = bfalse;

  // First check the character Skill ID matches
  // Then check for expansion skills too.
  if(CapList[ChrList[who].model].idsz[IDSZSKILL] == whichskill) result = btrue;
  else if(MAKE_IDSZ("CKUR") == whichskill) result = ChrList[who].canseekurse;
  else if(MAKE_IDSZ("WMAG") == whichskill) result = ChrList[who].canusearcane;
  else if(MAKE_IDSZ("JOUS") == whichskill) result = ChrList[who].canjoust;
  else if(MAKE_IDSZ("HMAG") == whichskill) result = ChrList[who].canusedivine;
  else if(MAKE_IDSZ("DISA") == whichskill) result = ChrList[who].candisarm;
  else if(MAKE_IDSZ("TECH") == whichskill) result = ChrList[who].canusetech;
  else if(MAKE_IDSZ("AWEP") == whichskill) result = ChrList[who].canuseadvancedweapons;
  else if(MAKE_IDSZ("STAB") == whichskill) result = ChrList[who].canbackstab;
  else if(MAKE_IDSZ("POIS") == whichskill) result = ChrList[who].canusepoison;
  else if(MAKE_IDSZ("READ") == whichskill) result = ChrList[who].canread;

  return result;
}

//--------------------------------------------------------------------------------------------
bool_t check_player_quest(char *whichplayer, IDSZ idsz)
{
  // ZF> This function checks if the specified player has the IDSZ in his or her quest.txt
  // and returns the quest level of that specific quest (Or -2 if it is not found, -1 if it is finished)

  //TODO: should also check if the IDSZ isnt beaten
  FILE *fileread;
  STRING newloadname;
  IDSZ newidsz;
  bool_t foundidsz = bfalse;
  int result = -2;
  int iTmp;

  //Always return "true" for [NONE] IDSZ checks
  if (idsz == IDSZNONE) result = 0;

  snprintf(newloadname, sizeof(newloadname), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file);
  fileread = fopen(newloadname, "r");
  if (NULL == fileread)
  {
    log_warning("File could not be read. (%s)\n", newloadname); 
    return result;
  };

  // Check each expansion
  while (goto_colon_yesno(fileread) && foundidsz == bfalse)
  {
    newidsz = get_idsz(fileread);
    if (newidsz == idsz)
    {
      foundidsz = btrue;
      //fscanf(fileread, "%o", &iTmp);		//Read value behind colon (TODO)
      iTmp = 0; //BAD should be read value
      result = iTmp;	
    }
  }
  fclose(fileread);

  return result;
}

//--------------------------------------------------------------------------------------------
bool_t add_quest_idsz(char *whichplayer, IDSZ idsz)
{
  // ZF> This function writes a IDSZ into a player quest.txt file, returns btrue if succeeded
  FILE *filewrite;
  STRING newloadname;
  bool_t result = bfalse;

  // Only add quest IDSZ if it doesnt have it already
  if (check_player_quest(whichplayer, idsz) > -2)
  {
    return result;
  };

  // Try to open the file in read and append mode
  snprintf(newloadname, sizeof(newloadname), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file);
  filewrite = fopen(newloadname, "a+");
  if (NULL == filewrite)
  {
    log_warning("Could not write into %s\n", newloadname);
    return result;
  };

  fprintf(filewrite, "\n:[%4s]: 0", undo_idsz(idsz));
  fclose(filewrite);

  return result;
}

//--------------------------------------------------------------------------------------------
int modify_quest_level(char *whichplayer, IDSZ idsz, int adjustment)
// ZF> This function increases or decreases a Quest IDSZ quest level by the amount determined in
// adjustment. It then returns the current quest level it now has (Or -1 if not found).
{
  //TODO
  return -1;
}

//--------------------------------------------------------------------------------------------
bool_t beat_quest_idsz(char *whichplayer, IDSZ idsz)
{
  // ZF> This function marks a IDSZ in the quest.txt file as beaten (Followed by a !)
  //     and returns btrue if it succeeded.
  FILE *filewrite;
  STRING newloadname;
  bool_t result = bfalse;
  bool_t foundidsz = bfalse;
  IDSZ newidsz;
  int QuestLevel;

  //TODO: This also needs to be done

  // Try to open the file in read/write mode
  snprintf(newloadname, sizeof(newloadname), "%s/%s/%s", CData.players_dir, whichplayer, CData.quest_file);
  filewrite = fopen(newloadname, "w+");
  if (NULL == filewrite)
  {
    log_warning("Could not write into %s\n", newloadname);
    return result;
  };

  //Now check each expansion until we find correct IDSZ
  while (goto_colon_yesno(filewrite) && foundidsz == bfalse)
  {
    newidsz = get_idsz(filewrite);
    if (newidsz == idsz)
    {
      foundidsz = btrue;
      fscanf(filewrite, "%d", &QuestLevel);
      if(QuestLevel == -1) result = bfalse;		//Is quest is already finished?
      break;
    }
  }
  fclose(filewrite);

  return result;
}