/* Egoboo - particle.c
 * Manages particle systems.
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
#include "mathstuff.h"
#include "Log.h"

WEATHER GWeather  = {bfalse, 10, 0, 0};


//--------------------------------------------------------------------------------------------
void make_prtlist(void)
{
  // ZZ> This function figures out which particles are visible, and it sets up GDyna.mic
  //     lighting
  int cnt, tnc, disx, disy, distance, slot;


  // Don't really make a list, just set to visible or not
  numdynalight = 0;
  GDyna.distancetobeat = MAXDYNADIST;
  cnt = 0;
  while (cnt < MAXPRT)
  {
    PrtList[cnt].inview = bfalse;
    if (PrtList[cnt].on)
    {
      PrtList[cnt].inview = Mesh.fanlist[PrtList[cnt].onwhichfan].inrenderlist;
      // Set up the lights we need
      if (PrtList[cnt].dyna_lighton)
      {
        disx = PrtList[cnt].xpos - GCamera.trackx;
        disx = ABS(disx);
        disy = PrtList[cnt].ypos - GCamera.tracky;
        disy = ABS(disy);
        distance = disx + disy;
        if (distance < GDyna.distancetobeat)
        {
          if (numdynalight < MAXDYNA)
          {
            // Just add the light
            slot = numdynalight;
            GDyna.distance[slot] = distance;
            numdynalight++;
          }
          else
          {
            // Overwrite the worst one
            slot = 0;
            tnc = 1;
            GDyna.distancetobeat = GDyna.distance[0];
            while (tnc < MAXDYNA)
            {
              if (GDyna.distance[tnc] > GDyna.distancetobeat)
              {
                slot = tnc;
              }
              tnc++;
            }
            GDyna.distance[slot] = distance;


            // Find the new distance to beat
            tnc = 1;
            GDyna.distancetobeat = GDyna.distance[0];
            while (tnc < MAXDYNA)
            {
              if (GDyna.distance[tnc] > GDyna.distancetobeat)
              {
                GDyna.distancetobeat = GDyna.distance[tnc];
              }
              tnc++;
            }
          }
          GDyna.lightlistx[slot] = PrtList[cnt].xpos;
          GDyna.lightlisty[slot] = PrtList[cnt].ypos;
          GDyna.lightlistz[slot] = PrtList[cnt].zpos;
          GDyna.lightlevel[slot] = PrtList[cnt].dyna_lightlevel;
          GDyna.lightfalloff[slot] = PrtList[cnt].dyna_lightfalloff;
        }
      }
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void free_one_particle_no_sound(int particle)
{
  // ZZ> This function sticks a particle back on the free particle stack
  freeprtlist[numfreeprt] = particle;
  numfreeprt++;
  PrtList[particle].on = bfalse;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound(int particle, Sint8 sound)
{
  //This function plays a sound effect for a particle
  if (sound != -1)
  {
    if (PrtList[particle].model != MAXMODEL) play_sound(PrtList[particle].xpos, PrtList[particle].ypos, CapList[PrtList[particle].model].waveindex[sound]);
    else  play_sound(PrtList[particle].xpos, PrtList[particle].ypos, globalwave[sound]);
  }
}

//--------------------------------------------------------------------------------------------
void free_one_particle(int particle, Uint32 * rand_idx)
{
  // ZZ> This function sticks a particle back on the free particle stack and
  //     plays the sound associated with the particle
  int child;
  Uint32 rpt_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);

  if (PrtList[particle].spawncharacterstate != SPAWNNOCHARACTER)
  {
    child = spawn_one_character(PrtList[particle].xpos, PrtList[particle].ypos, PrtList[particle].zpos,
                                PrtList[particle].model, PrtList[particle].team, 0, PrtList[particle].facing,
                                NULL, MAXCHR, &rpt_randie);
    if (child != MAXCHR)
    {
      ChrList[child].aistate = PrtList[particle].spawncharacterstate;
      ChrList[child].aiowner = PrtList[particle].chr;
    }
  }
  play_particle_sound(particle, PipList[PrtList[particle].pip].soundend);
  freeprtlist[numfreeprt] = particle;
  numfreeprt++;
  PrtList[particle].on = bfalse;
}

//--------------------------------------------------------------------------------------------
int get_free_particle(int force)
{
  // ZZ> This function gets an unused particle.  If all particles are in use
  //     and force is set, it grabs the first unimportant one.  The particle
  //     index is the return value
  int particle;


  // Return MAXPRT if we can't find one
  particle = MAXPRT;
  if (numfreeprt == 0)
  {
    if (force)
    {
      // Gotta find one, so go through the list
      particle = 0;
      while (particle < MAXPRT)
      {
        if (PrtList[particle].bumpsize == 0)
        {
          // Found one
          return particle;
        }
        particle++;
      }
    }
  }
  else
  {
    if (force || numfreeprt > (MAXPRT / 4))
    {
      // Just grab the next one
      numfreeprt--;
      particle = freeprtlist[numfreeprt];
    }
  }
  return particle;
}

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_particle(float x, float y, float z,
                                  Uint16 facing, Uint16 model, Uint16 pip,
                                  Uint16 characterattach, Uint16 grip, Uint8 team,
                                  Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget, Uint32 * rand_idx)
{
  // ZZ> This function spawns a new particle, and returns the number of that particle
  int cnt, velocity;
  float xvel, yvel, zvel, tvel;
  int offsetfacing, newrand;
  Uint32 prt_randie = *rand_idx;

  // do exactly one iteration of the andom number generator
  RANDIE(*rand_idx);

  // Convert from local pip to global pip
  if (model < MAXMODEL)
    pip = MadList[model].prtpip[pip];


  cnt = get_free_particle(PipList[pip].force);
  if (cnt == MAXPRT) return MAXPRT;

  // Necessary data for any part
  PrtList[cnt].on = btrue;
  PrtList[cnt].pip = pip;
  PrtList[cnt].model = model;
  PrtList[cnt].inview = bfalse;
  PrtList[cnt].level = 0;
  PrtList[cnt].team = team;
  PrtList[cnt].chr = characterorigin;
  PrtList[cnt].damagetype = PipList[pip].damagetype;
  PrtList[cnt].spawncharacterstate = SPAWNNOCHARACTER;


  // Lighting and sound
  PrtList[cnt].dyna_lighton = bfalse;
  if (multispawn == 0)
  {
    PrtList[cnt].dyna_lighton = PipList[pip].dyna_lightmode;
    if (PipList[pip].dyna_lightmode == DYNALOCAL)
    {
      PrtList[cnt].dyna_lighton = bfalse;
    }
  }
  PrtList[cnt].dyna_lightlevel = PipList[pip].dyna_level;
  PrtList[cnt].dyna_lightfalloff = PipList[pip].dyna_falloff;



  // Set character attachments ( characterattach==MAXCHR means none )
  PrtList[cnt].attachedtocharacter = characterattach;
  PrtList[cnt].grip = grip;



  // Correct facing
  facing += PipList[pip].facingbase;


  // Targeting...
  offsetfacing = 0;
  zvel = 0;
  newrand = RANDIE(prt_randie);
  z = z + PipList[pip].zspacingbase + (newrand & PipList[pip].zspacingrand) - (PipList[pip].zspacingrand >> 1);
  newrand = RANDIE(prt_randie);
  velocity = (PipList[pip].xyvelbase + (newrand & PipList[pip].xyvelrand));
  PrtList[cnt].target = oldtarget;
  if (PipList[pip].newtargetonspawn)
  {
    if (PipList[pip].targetcaster)
    {
      // Set the target to the caster
      PrtList[cnt].target = characterorigin;
    }
    else
    {
      // Find a target
      PrtList[cnt].target = find_target(x, y, facing, PipList[pip].targetangle, PipList[pip].onlydamagefriendly, bfalse, team, characterorigin, oldtarget);
      if (PrtList[cnt].target != MAXCHR && PipList[pip].homing == bfalse)
      {
        facing = facing - glouseangle;
      }
      // Correct facing for dexterity...
      offsetfacing = 0;
      if (ChrList[characterorigin].dexterity < PERFECTSTAT)
      {
        // Correct facing for randomness
        offsetfacing = RANDIE(prt_randie);
        offsetfacing = offsetfacing & PipList[pip].facingrand;
        offsetfacing -= (PipList[pip].facingrand >> 1);
        offsetfacing = (offsetfacing * (PERFECTSTAT - ChrList[characterorigin].dexterity)) >> 13;  // Divided by PERFECTSTAT
      }
      if (PrtList[cnt].target != MAXCHR && PipList[pip].zaimspd != 0)
      {
        // These aren't velocities...  This is to do aiming on the Z axis
        if (velocity > 0)
        {
          xvel = ChrList[PrtList[cnt].target].xpos - x;
          yvel = ChrList[PrtList[cnt].target].ypos - y;
          tvel = sqrt(xvel * xvel + yvel * yvel) / velocity;  // This is the number of steps...
          if (tvel > 0)
          {
            zvel = (ChrList[PrtList[cnt].target].zpos + (ChrList[PrtList[cnt].target].bumpsize >> 1) - z) / tvel;  // This is the zvel alteration
            if (zvel < -(PipList[pip].zaimspd >> 1)) zvel = -(PipList[pip].zaimspd >> 1);
            if (zvel > PipList[pip].zaimspd) zvel = PipList[pip].zaimspd;
          }
        }
      }
    }
    // Does it go away?
    if (PrtList[cnt].target == MAXCHR && PipList[pip].needtarget)
    {
      free_one_particle(cnt, &prt_randie);
      return MAXPRT;
    }
    // Start on top of target
    if (PrtList[cnt].target != MAXCHR && PipList[pip].startontarget)
    {
      x = ChrList[PrtList[cnt].target].xpos;
      y = ChrList[PrtList[cnt].target].ypos;
    }
  }
  else
  {
    // Correct facing for randomness
    offsetfacing = RANDIE(prt_randie);
    offsetfacing = offsetfacing & PipList[pip].facingrand;
    offsetfacing -= (PipList[pip].facingrand >> 1);
  }
  facing += offsetfacing;
  PrtList[cnt].facing = facing;
  facing = facing >> 2;


  // Location data from arguments
  newrand = RANDIE(prt_randie);
  x = x + turntosin[(facing+12288) & TRIGTABLE_MASK] * (PipList[pip].xyspacingbase + (newrand & PipList[pip].xyspacingrand));
  y = y + turntosin[(facing+8192) & TRIGTABLE_MASK] * (PipList[pip].xyspacingbase + (newrand & PipList[pip].xyspacingrand));
  if (x < 0)  x = 0;
  if (x > Mesh.edgex - 2)  x = Mesh.edgex - 2;
  if (y < 0)  y = 0;
  if (y > Mesh.edgey - 2)  y = Mesh.edgey - 2;
  PrtList[cnt].xpos = x;
  PrtList[cnt].ypos = y;
  PrtList[cnt].zpos = z;


  // Velocity data
  xvel = turntosin[(facing+12288) & TRIGTABLE_MASK] * velocity;
  yvel = turntosin[(facing+8192) & TRIGTABLE_MASK] * velocity;
  newrand = RANDIE(prt_randie);
  zvel += PipList[pip].zvelbase + (newrand & PipList[pip].zvelrand) - (PipList[pip].zvelrand >> 1);
  PrtList[cnt].xvel = xvel;
  PrtList[cnt].yvel = yvel;
  PrtList[cnt].zvel = zvel;


  // Template values
  PrtList[cnt].bumpsize = PipList[pip].bumpsize;
  PrtList[cnt].bumpsizebig = PrtList[cnt].bumpsize + (PrtList[cnt].bumpsize >> 1);
  PrtList[cnt].bumpheight = PipList[pip].bumpheight;
  PrtList[cnt].type = PipList[pip].type;


  // Image data
  newrand = RANDIE(prt_randie);
  PrtList[cnt].rotate = (newrand & PipList[pip].rotaterand) + PipList[pip].rotatebase;
  PrtList[cnt].rotateadd = PipList[pip].rotateadd;
  PrtList[cnt].size = PipList[pip].sizebase;
  PrtList[cnt].sizeadd = PipList[pip].sizeadd;
  PrtList[cnt].image = 0;
  newrand = RANDIE(prt_randie);
  PrtList[cnt].imageadd = PipList[pip].imageadd + (newrand & PipList[pip].imageaddrand);
  PrtList[cnt].imagestt = PipList[pip].imagebase << 8;
  PrtList[cnt].imagemax = PipList[pip].numframes << 8;
  PrtList[cnt].time = PipList[pip].time;
  if (PipList[pip].endlastframe && PrtList[cnt].imageadd != 0)
  {
    if (PrtList[cnt].time == 0)
    {
      // Part time is set to 1 cycle
      PrtList[cnt].time = (PrtList[cnt].imagemax / PrtList[cnt].imageadd) - 1;
    }
    else
    {
      // Part time is used to give number of cycles
      PrtList[cnt].time = PrtList[cnt].time * ((PrtList[cnt].imagemax / PrtList[cnt].imageadd) - 1);
    }
  }


  // Set onwhichfan...
  PrtList[cnt].onwhichfan = OFFEDGE;
  if (PrtList[cnt].xpos > 0 && PrtList[cnt].xpos < Mesh.edgex && PrtList[cnt].ypos > 0 && PrtList[cnt].ypos < Mesh.edgey)
  {
    PrtList[cnt].onwhichfan = Mesh.fanstart[((int)PrtList[cnt].ypos) >> 7] + (((int)PrtList[cnt].xpos) >> 7);
  }


  // Damage stuff
  PrtList[cnt].damagebase = PipList[pip].damagebase;
  PrtList[cnt].damagerand = PipList[pip].damagerand;



  // Spawning data
  PrtList[cnt].spawntime = PipList[pip].contspawntime;
  if (PrtList[cnt].spawntime != 0)
  {
    PrtList[cnt].spawntime = 1;
    if (PrtList[cnt].attachedtocharacter != MAXCHR)
    {
      PrtList[cnt].spawntime++; // Because attachment takes an update before it happens
    }
  }

  // Sound effect
  play_particle_sound(cnt, PipList[pip].soundspawn);

  return cnt;
}

//--------------------------------------------------------------------------------------------
Uint8 __prthitawall(int particle)
{
  // ZZ> This function returns nonzero if the particle hit a wall
  int x, y;

  y = PrtList[particle].ypos;  x = PrtList[particle].xpos;
  // !!!BAD!!! Should really do bound checking...
  if (PipList[PrtList[particle].pip].bumpmoney)
  {
    return ((Mesh.fanlist[Mesh.fanstart[y>>7] + (x >> 7)].fx)&(MESHFXIMPASS | MESHFXWALL));
  }
  else
  {
    return ((Mesh.fanlist[Mesh.fanstart[y>>7] + (x >> 7)].fx)&(MESHFXIMPASS));
  }
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles(Uint16 character, Uint32 * rand_idx)
{
  // ZZ> This function makes sure a character has no attached particles
  Uint16 particle;
  Uint32 prt_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);

  particle = 0;
  while (particle < MAXPRT)
  {
    if (PrtList[particle].on && PrtList[particle].attachedtocharacter == character)
    {
      free_one_particle(particle, &prt_randie);
    }
    particle++;
  }

  // Set the alert for disaffirmation ( wet torch )
  ChrList[character].alert |= ALERTIFDISAFFIRMED;
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles(Uint16 character)
{
  // ZZ> This function returns the number of particles attached to the given character
  Uint16 cnt, particle;

  cnt = 0;
  particle = 0;
  while (particle < MAXPRT)
  {
    if (PrtList[particle].on && PrtList[particle].attachedtocharacter == character)
    {
      cnt++;
    }
    particle++;
  }

  return cnt;
}

//--------------------------------------------------------------------------------------------
void reaffirm_attached_particles(Uint16 character, Uint32 * rand_idx)
{
  // ZZ> This function makes sure a character has all of it's particles
  Uint16 numberattached;
  Uint16 particle;
  Uint32 prt_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);

  numberattached = number_of_attached_particles(character);
  while (numberattached < CapList[ChrList[character].model].attachedprtamount)
  {
    particle = spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, 0, ChrList[character].model, CapList[ChrList[character].model].attachedprttype, character, SPAWNLAST + numberattached, ChrList[character].team, character, numberattached, MAXCHR, &prt_randie);
    if (particle != MAXPRT)
    {
      attach_particle_to_character(particle, character, PrtList[particle].grip);
    }
    numberattached++;
  }

  // Set the alert for reaffirmation ( for exploding barrels with fire )
  ChrList[character].alert = ChrList[character].alert | ALERTIFREAFFIRMED;
}


//--------------------------------------------------------------------------------------------
void move_particles(Uint32 * rand_idx)
{
  // ZZ> This is the particle physics function
  int cnt, tnc, x, y, fan;
  Uint16 facing, pip, particle;
  float level;
  Uint32 prt_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);

  cnt = 0;
  while (cnt < MAXPRT)
  {
    if (PrtList[cnt].on)
    {
      PrtList[cnt].onwhichfan = OFFEDGE;
      PrtList[cnt].level = 0;
      if (PrtList[cnt].xpos > 0 && PrtList[cnt].xpos < Mesh.edgex && PrtList[cnt].ypos > 0 && PrtList[cnt].ypos < Mesh.edgey)
      {
        x = PrtList[cnt].xpos;
        y = PrtList[cnt].ypos;
        x = x >> 7;
        y = y >> 7;
        fan = Mesh.fanstart[y] + x;
        PrtList[cnt].onwhichfan = fan;
        PrtList[cnt].level = get_level(PrtList[cnt].xpos, PrtList[cnt].ypos, fan, bfalse);
      }


      // To make it easier
      pip = PrtList[cnt].pip;

      // Animate particle
      PrtList[cnt].image = (PrtList[cnt].image + PrtList[cnt].imageadd);
      if (PrtList[cnt].image >= PrtList[cnt].imagemax)
        PrtList[cnt].image = 0;
      PrtList[cnt].rotate += PrtList[cnt].rotateadd;
      PrtList[cnt].size += PrtList[cnt].sizeadd;


      // Change dyna light values
      PrtList[cnt].dyna_lightlevel += PipList[pip].dyna_lightleveladd;
      PrtList[cnt].dyna_lightfalloff += PipList[pip].dyna_lightfalloffadd;


      // Make it sit on the floor...  Shift is there to correct for sprite size
      level = PrtList[cnt].level + (PrtList[cnt].size >> 9);


      // Check floor collision and do iterative physics
      if ((PrtList[cnt].zpos < level && PrtList[cnt].zvel < 0.1) || (PrtList[cnt].zpos < level - PRTLEVELFIX))
      {
        PrtList[cnt].zpos = level;
        PrtList[cnt].xvel = PrtList[cnt].xvel * noslipfriction;
        PrtList[cnt].yvel = PrtList[cnt].yvel * noslipfriction;
        if (PipList[pip].endground)  PrtList[cnt].time = 1;
        if (PrtList[cnt].zvel < 0)
        {
          if (PrtList[cnt].zvel > -STOPBOUNCINGPART)
          {
            // Make it not bounce
            PrtList[cnt].zpos -= .0001;
          }
          else
          {
            // Make it bounce
            PrtList[cnt].zvel = -PrtList[cnt].zvel * PipList[pip].dampen;

            // Play the sound for hitting the floor [FSND]
            play_particle_sound(cnt, PipList[pip].soundfloor);
          }
        }
      }
      else
      {
        if (PrtList[cnt].attachedtocharacter == MAXCHR)
        {
          PrtList[cnt].xpos += PrtList[cnt].xvel;
          if (__prthitawall(cnt))
          {
            // Play the sound for hitting a wall [WSND]
            play_particle_sound(cnt, PipList[pip].soundwall);
            PrtList[cnt].xpos -= PrtList[cnt].xvel;
            PrtList[cnt].xvel = (-PrtList[cnt].xvel * PipList[pip].dampen);
            if (PipList[pip].endwall)
            {
              PrtList[cnt].time = 1;
            }
            else
            {
              // Change facing
              facing = PrtList[cnt].facing;
              if (facing < 32768)
              {
                facing -= NORTH;
                facing = ~facing;
                facing += NORTH;
              }
              else
              {
                facing -= SOUTH;
                facing = ~facing;
                facing += SOUTH;
              }
              PrtList[cnt].facing = facing;
            }
          }
          PrtList[cnt].ypos += PrtList[cnt].yvel;
          if (__prthitawall(cnt))
          {
            PrtList[cnt].ypos -= PrtList[cnt].yvel;
            PrtList[cnt].yvel = (-PrtList[cnt].yvel * PipList[pip].dampen);
            if (PipList[pip].endwall)
            {
              PrtList[cnt].time = 1;
            }
            else
            {
              // Change facing
              facing = PrtList[cnt].facing;
              if (facing < 16384 || facing > 49152)
              {
                facing = ~facing;
              }
              else
              {
                facing -= EAST;
                facing = ~facing;
                facing += EAST;
              }
              PrtList[cnt].facing = facing;
            }
          }
          PrtList[cnt].zpos += PrtList[cnt].zvel;
          PrtList[cnt].zvel += gravity;
        }
      }
      // Do homing
      if (PipList[pip].homing && PrtList[cnt].target != MAXCHR)
      {
        if (ChrList[PrtList[cnt].target].alive == bfalse)
        {
          PrtList[cnt].time = 1;
        }
        else
        {
          if (PrtList[cnt].attachedtocharacter == MAXCHR)
          {
            PrtList[cnt].xvel = (PrtList[cnt].xvel + ((ChrList[PrtList[cnt].target].xpos - PrtList[cnt].xpos) * PipList[pip].homingaccel)) * PipList[pip].homingfriction;
            PrtList[cnt].yvel = (PrtList[cnt].yvel + ((ChrList[PrtList[cnt].target].ypos - PrtList[cnt].ypos) * PipList[pip].homingaccel)) * PipList[pip].homingfriction;
            PrtList[cnt].zvel = (PrtList[cnt].zvel + ((ChrList[PrtList[cnt].target].zpos + (ChrList[PrtList[cnt].target].bumpheight >> 1) - PrtList[cnt].zpos) * PipList[pip].homingaccel));

          }
          if (PipList[pip].rotatetoface)
          {
            // Turn to face target
            facing = atan2(ChrList[PrtList[cnt].target].ypos - PrtList[cnt].ypos, ChrList[PrtList[cnt].target].xpos - PrtList[cnt].xpos) * RAD_TO_SHORT;
            facing += 32768;
            PrtList[cnt].facing = facing;
          }
        }
      }
      // Do speed limit on Z
      if (PrtList[cnt].zvel < -PipList[pip].spdlimit)  PrtList[cnt].zvel = -PipList[pip].spdlimit;



      // Spawn new particles if continually spawning
      if (PipList[pip].contspawnamount > 0)
      {
        PrtList[cnt].spawntime--;
        if (PrtList[cnt].spawntime == 0)
        {
          PrtList[cnt].spawntime = PipList[pip].contspawntime;
          facing = PrtList[cnt].facing;
          tnc = 0;
          while (tnc < PipList[pip].contspawnamount)
          {
            particle = spawn_one_particle(PrtList[cnt].xpos, PrtList[cnt].ypos, PrtList[cnt].zpos,
                                          facing, PrtList[cnt].model, PipList[pip].contspawnpip,
                                          MAXCHR, SPAWNLAST, PrtList[cnt].team, PrtList[cnt].chr, tnc, PrtList[cnt].target, &prt_randie);
            if (PipList[PrtList[cnt].pip].facingadd != 0 && particle < MAXPRT)
            {
              // Hack to fix velocity
              PrtList[particle].xvel += PrtList[cnt].xvel;
              PrtList[particle].yvel += PrtList[cnt].yvel;
            }
            facing += PipList[pip].contspawnfacingadd;
            tnc++;
          }
        }
      }


      // Check underwater
      if (PrtList[cnt].zpos < waterdouselevel && (Mesh.fanlist[PrtList[cnt].onwhichfan].fx&MESHFXWATER) && PipList[pip].endwater)
      {
        // Splash for particles is just a ripple
        spawn_one_particle(PrtList[cnt].xpos, PrtList[cnt].ypos, watersurfacelevel,
                           0, MAXMODEL, RIPPLE, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR, &prt_randie);


        // Check for disaffirming character
        if (PrtList[cnt].attachedtocharacter != MAXCHR && PrtList[cnt].chr == PrtList[cnt].attachedtocharacter)
        {
          // Disaffirm the whole character
          disaffirm_attached_particles(PrtList[cnt].attachedtocharacter, &prt_randie);
        }
        else
        {
          // Just destroy the particle
//                    free_one_particle(cnt);
          PrtList[cnt].time = 1;
        }
      }
//            else
//            {
      // Spawn new particles if time for old one is up
      if (PrtList[cnt].time != 0)
      {
        PrtList[cnt].time--;
        if (PrtList[cnt].time == 0)
        {
          facing = PrtList[cnt].facing;
          tnc = 0;
          while (tnc < PipList[pip].endspawnamount)
          {
            spawn_one_particle(PrtList[cnt].xpos - PrtList[cnt].xvel, PrtList[cnt].ypos - PrtList[cnt].yvel, PrtList[cnt].zpos,
                               facing, PrtList[cnt].model, PipList[pip].endspawnpip,
                               MAXCHR, SPAWNLAST, PrtList[cnt].team, PrtList[cnt].chr, tnc, PrtList[cnt].target, &prt_randie);
            facing += PipList[pip].endspawnfacingadd;
            tnc++;
          }
          free_one_particle(cnt, &prt_randie);
        }
      }
//            }
      PrtList[cnt].facing += PipList[pip].facingadd;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
  // ZZ> This function attaches particles to their characters so everything gets
  //     drawn right
  int cnt;

  cnt = 0;
  while (cnt < MAXPRT)
  {
    if (PrtList[cnt].on && PrtList[cnt].attachedtocharacter != MAXCHR)
    {
      attach_particle_to_character(cnt, PrtList[cnt].attachedtocharacter, PrtList[cnt].grip);
      // Correct facing so swords knock characters in the right direction...
      if (PipList[PrtList[cnt].pip].damfx&DAMFXTURN)
        PrtList[cnt].facing = ChrList[PrtList[cnt].attachedtocharacter].turnleftright;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void free_all_particles()
{
  // ZZ> This function resets the particle allocation lists
  numfreeprt = 0;
  while (numfreeprt < MAXPRT)
  {
    PrtList[numfreeprt].on = 0;
    freeprtlist[numfreeprt] = numfreeprt;
    numfreeprt++;
  }
}

//--------------------------------------------------------------------------------------------
void setup_particles()
{
  // ZZ> This function sets up particle data

  particletexture = 0;

  // Reset the allocation table
  free_all_particles();
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir(Uint16 majordir, Uint16 minordir)
{
  // ZZ> This function returns a direction between the major and minor ones, closer
  //     to the major.
  Uint16 temp;

  // Align major direction with 0
  minordir -= majordir;
  if (minordir > 32768)
  {
    temp = 65535;
    minordir = (minordir + (temp << 3) - temp) >> 3;
    minordir += majordir;
    return minordir;
  }
  temp = 0;
  minordir = (minordir + (temp << 3) - temp) >> 3;
  minordir += majordir;
  return minordir;
}

//--------------------------------------------------------------------------------------------
Uint16 terp_dir_fast(Uint16 majordir, Uint16 minordir)
{
  // ZZ> This function returns a direction between the major and minor ones, closer
  //     to the major, but not by much.  Makes turning faster.
  Uint16 temp;

  // Align major direction with 0
  minordir -= majordir;
  if (minordir > 32768)
  {
    temp = 65535;
    minordir = (minordir + (temp << 1) - temp) >> 1;
    minordir += majordir;
    return minordir;
  }
  temp = 0;
  minordir = (minordir + (temp << 1) - temp) >> 1;
  minordir += majordir;
  return minordir;
}

//--------------------------------------------------------------------------------------------
void spawn_bump_particles(GAME_STATE * gs, Uint16 character, Uint16 particle, Uint32 * rand_idx)
{
  // ZZ> This function is for catching characters on fire and such
  int cnt;
  Sint16 x, y, z;
  int distance, bestdistance;
  Uint16 frame;
  Uint16 facing, bestvertex;
  Uint16 amount;
  Uint16 pip;
  Uint16 vertices;
  Uint16 direction, left, right, model;
  float fsin, fcos;
  Uint32 prt_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);


  pip = PrtList[particle].pip;
  amount = PipList[pip].bumpspawnamount;


  if (amount != 0 || PipList[pip].spawnenchant)
  {
    // Only damage if hitting from proper direction
    model = ChrList[character].model;
    vertices = MadList[model].vertices;
    direction = RAD_TO_TURN(atan2(PrtList[particle].yvel, PrtList[particle].xvel));
    direction = ChrList[character].turnleftright - direction + 32768;
    if (MadFrame[ChrList[character].frame].framefx&MADFXINVICTUS)
    {
      // I Frame
      if (PipList[pip].damfx&DAMFXBLOC)
      {
        left = 65535;
        right = 0;
      }
      else
      {
        direction -= CapList[model].iframefacing;
        left = (~CapList[model].iframeangle);
        right = CapList[model].iframeangle;
      }
    }
    else
    {
      // N Frame
      direction -= CapList[model].nframefacing;
      left = (~CapList[model].nframeangle);
      right = CapList[model].nframeangle;
    }
    // Check that direction
    if (direction <= left && direction >= right)
    {
      // Spawn new enchantments
      if (PipList[pip].spawnenchant)
      {
        spawn_enchant(gs, PrtList[particle].chr, character, MAXCHR, MAXENCHANT, PrtList[particle].model, &prt_randie);
      }
      // Spawn particles
      if (amount != 0 && CapList[ChrList[character].model].resistbumpspawn == bfalse && ChrList[character].invictus == bfalse && vertices != 0 && (ChrList[character].damagemodifier[PrtList[particle].damagetype]&DAMAGESHIFT) < 3)
      {
        if (amount == 1)
        {
          // A single particle ( arrow? ) has been stuck in the character...
          // Find best vertex to attach to
          bestvertex = 0;
          bestdistance = 9999999;
          z = -ChrList[character].zpos + PrtList[particle].zpos + RAISE;
          facing = PrtList[particle].facing - ChrList[character].turnleftright - 16384;
          facing = facing >> 2;
          fsin = turntosin[facing & TRIGTABLE_MASK];
          fcos = turntosin[(facing+4096) & TRIGTABLE_MASK];
          y = 8192;
          x = -y * fsin;
          y = y * fcos;
          z = z << 10;///ChrList[character].scale;
          frame = MadList[ChrList[character].model].framestart;
          cnt = 0;
          while (cnt < vertices)
          {
            distance = ABS(x - MadFrame[frame].vrtx[vertices-cnt-1]) + ABS(y - MadFrame[frame].vrty[vertices-cnt-1]) + (ABS(z - MadFrame[frame].vrtz[vertices-cnt-1]));
            if (distance < bestdistance)
            {
              bestdistance = distance;
              bestvertex = cnt;
            }
            cnt++;
          }
          spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, 0, PrtList[particle].model, PipList[pip].bumpspawnpip,
                             character, bestvertex + 1, PrtList[particle].team, PrtList[particle].chr, cnt, character, &prt_randie);
        }
        else
        {
          amount = (amount * vertices) >> 5;  // Correct amount for size of character
          cnt = 0;
          while (cnt < amount)
          {
            spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, 0, PrtList[particle].model, PipList[pip].bumpspawnpip,
                               character, ego_rand(&ego_rand_seed) % vertices, PrtList[particle].team, PrtList[particle].chr, cnt, character, &prt_randie);
            cnt++;
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
int prt_is_over_water(int cnt)
{
  // This function returns btrue if the particle is over a water tile
  int x, y, fan;


  if (cnt < MAXPRT)
  {
    if (PrtList[cnt].xpos > 0 && PrtList[cnt].xpos < Mesh.edgex && PrtList[cnt].ypos > 0 && PrtList[cnt].ypos < Mesh.edgey)
    {
      x = PrtList[cnt].xpos;
      y = PrtList[cnt].ypos;
      x = x >> 7;
      y = y >> 7;
      fan = Mesh.fanstart[y] + x;
      if (Mesh.fanlist[fan].fx&MESHFXWATER)  return btrue;
    }
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn(Uint32 * rand_idx)
{
  // ZZ> This function drops snowflakes or rain or whatever, also swings the camera
  int particle, cnt;
  float x, y, z;
  Uint8 foundone;
  Uint32 weather_randie = *rand_idx;

  RANDIE(*rand_idx);

  if (GWeather.time > 0)
  {
    GWeather.time--;
    if (GWeather.time == 0)
    {
      GWeather.time = GWeather.timereset;


      // Find a valid player
      foundone = bfalse;
      cnt = 0;
      while (cnt < MAXPLAYER)
      {
        GWeather.player = (GWeather.player + 1) & (MAXPLAYER - 1);
        if (PlaList[GWeather.player].valid)
        {
          foundone = btrue;
          cnt = MAXPLAYER;
        }
        cnt++;
      }


      // Did we find one?
      if (foundone)
      {
        // Yes, but is the character valid?
        cnt = PlaList[GWeather.player].index;
        if (ChrList[cnt].on && ChrList[cnt].inpack == bfalse)
        {
          // Yes, so spawn over that character
          x = ChrList[cnt].xpos;
          y = ChrList[cnt].ypos;
          z = ChrList[cnt].zpos;
          particle = spawn_one_particle(x, y, z, 0, MAXMODEL, WEATHER4, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR, &weather_randie);
          if (GWeather.overwater && particle != MAXPRT)
          {
            if (!prt_is_over_water(particle))
            {
              free_one_particle_no_sound(particle);
            }
          }

        }
      }
    }
  }
  GCamera.swing = (GCamera.swing + GCamera.swingrate) & TRIGTABLE_MASK;
}

//--------------------------------------------------------------------------------------------
int load_one_particle(char *szLoadName, int object, int pip)
{
  // ZZ> This function loads a particle template, returning bfalse if the file wasn't
  //     found
  FILE* fileread;
  IDSZ idsz;
  int iTmp;
  float fTmp;
  char cTmp;


  fileread = fopen(szLoadName, "r");
  if (fileread)
  {
    // General data
    globalname = szLoadName;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].force = bfalse;
    if (cTmp == 'T' || cTmp == 't')  PipList[numpip].force = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if (cTmp == 'L' || cTmp == 'l')  PipList[numpip].type = PRTLIGHTSPRITE;
    if (cTmp == 'S' || cTmp == 's')  PipList[numpip].type = PRTSOLIDSPRITE;
    if (cTmp == 'T' || cTmp == 't')  PipList[numpip].type = PRTALPHASPRITE;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].imagebase = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].numframes = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].imageadd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].imageaddrand = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].rotatebase = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].rotaterand = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].rotateadd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].sizebase = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].sizeadd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp); PipList[numpip].spdlimit = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].facingadd = iTmp;


    // Ending conditions
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].endwater = btrue;
    if (cTmp == 'F' || cTmp == 'f')  PipList[numpip].endwater = bfalse;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].endbump = btrue;
    if (cTmp == 'F' || cTmp == 'f')  PipList[numpip].endbump = bfalse;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].endground = btrue;
    if (cTmp == 'F' || cTmp == 'f')  PipList[numpip].endground = bfalse;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].endlastframe = btrue;
    if (cTmp == 'F' || cTmp == 'f')  PipList[numpip].endlastframe = bfalse;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].time = iTmp;


    // Collision data
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp); PipList[numpip].dampen = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].bumpmoney = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].bumpsize = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].bumpheight = iTmp;
    goto_colon(fileread);  read_pair(fileread);
    PipList[numpip].damagebase = pairbase;
    PipList[numpip].damagerand = pairrand;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    if (cTmp == 'S' || cTmp == 's') PipList[numpip].damagetype = DAMAGESLASH;
    if (cTmp == 'C' || cTmp == 'c') PipList[numpip].damagetype = DAMAGECRUSH;
    if (cTmp == 'P' || cTmp == 'p') PipList[numpip].damagetype = DAMAGEPOKE;
    if (cTmp == 'H' || cTmp == 'h') PipList[numpip].damagetype = DAMAGEHOLY;
    if (cTmp == 'E' || cTmp == 'e') PipList[numpip].damagetype = DAMAGEEVIL;
    if (cTmp == 'F' || cTmp == 'f') PipList[numpip].damagetype = DAMAGEFIRE;
    if (cTmp == 'I' || cTmp == 'i') PipList[numpip].damagetype = DAMAGEICE;
    if (cTmp == 'Z' || cTmp == 'z') PipList[numpip].damagetype = DAMAGEZAP;


    // Lighting data
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].dyna_lightmode = DYNAOFF;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].dyna_lightmode = DYNAON;
    if (cTmp == 'L' || cTmp == 'l') PipList[numpip].dyna_lightmode = DYNALOCAL;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp); PipList[numpip].dyna_level = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].dyna_falloff = iTmp;
    if (PipList[numpip].dyna_falloff > MAXFALLOFF)  PipList[numpip].dyna_falloff = MAXFALLOFF;



    // Initial spawning of this particle
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].facingbase = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].facingrand = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].xyspacingbase = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].xyspacingrand = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].zspacingbase = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].zspacingrand = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].xyvelbase = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].xyvelrand = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].zvelbase = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].zvelrand = iTmp;


    // Continuous spawning of other particles
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].contspawntime = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].contspawnamount = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].contspawnfacingadd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].contspawnpip = iTmp;


    // End spawning of other particles
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].endspawnamount = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].endspawnfacingadd = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].endspawnpip = iTmp;


    // Bump spawning of attached particles
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].bumpspawnamount = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].bumpspawnpip = iTmp;


    // Random stuff  !!!BAD!!! Not complete
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].dazetime = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].grogtime = iTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].spawnenchant = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].spawnenchant = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    pipcauseknockback[numpip] = bfalse;
    if (cTmp == 'T' || cTmp == 't') pipcauseknockback[numpip] = btrue;
    goto_colon(fileread);
    PipList[numpip].causepancake = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].causepancake = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].needtarget = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].needtarget = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].targetcaster = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].targetcaster = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].startontarget = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].startontarget = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].onlydamagefriendly = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].onlydamagefriendly = btrue;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
    if (iTmp < -1) iTmp = -1;
    if (iTmp > MAXWAVE - 1) iTmp = MAXWAVE - 1;
    PipList[numpip].soundspawn = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
    if (iTmp < -1) iTmp = -1;
    if (iTmp > MAXWAVE - 1) iTmp = MAXWAVE - 1;
    PipList[numpip].soundend = iTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].friendlyfire = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].friendlyfire = btrue;
    goto_colon(fileread);  // !!Hate group only
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].newtargetonspawn = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].newtargetonspawn = btrue;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp); PipList[numpip].targetangle = iTmp >> 1;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].homing = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].homing = btrue;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp); PipList[numpip].homingfriction = fTmp;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp); PipList[numpip].homingaccel = fTmp;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    PipList[numpip].rotatetoface = bfalse;
    if (cTmp == 'T' || cTmp == 't') PipList[numpip].rotatetoface = btrue;

    // Clear expansions...
    PipList[numpip].zaimspd = 0;
    PipList[numpip].soundfloor = -1;
    PipList[numpip].soundwall = -1;
    PipList[numpip].endwall = PipList[numpip].endground;
    PipList[numpip].damfx = DAMFXTURN;
    if (PipList[numpip].homing)  PipList[numpip].damfx = DAMFXNONE;
    PipList[numpip].allowpush = btrue;
    PipList[numpip].dyna_lightfalloffadd = 0;
    PipList[numpip].dyna_lightleveladd = 0;
    PipList[numpip].intdamagebonus = bfalse;
    PipList[numpip].wisdamagebonus = bfalse;
    // Read expansions
    while (goto_colon_yesno(fileread))
    {
      idsz = get_idsz(fileread);
      fscanf(fileread, "%c%d", &cTmp, &iTmp);

      if (MAKE_IDSZ("TURN") == idsz)  PipList[numpip].damfx = DAMFXNONE;
      if (MAKE_IDSZ("ZSPD") == idsz)  PipList[numpip].zaimspd = iTmp;
      if (MAKE_IDSZ("FSND") == idsz)  PipList[numpip].soundfloor = iTmp;
      if (MAKE_IDSZ("WSND") == idsz)  PipList[numpip].soundwall = iTmp;
      if (MAKE_IDSZ("WEND") == idsz)  PipList[numpip].endwall = iTmp;
      if (MAKE_IDSZ("ARMO") == idsz)  PipList[numpip].damfx |= DAMFXARMO;
      if (MAKE_IDSZ("BLOC") == idsz)  PipList[numpip].damfx |= DAMFXBLOC;
      if (MAKE_IDSZ("ARRO") == idsz)  PipList[numpip].damfx |= DAMFXARRO;
      if (MAKE_IDSZ("TIME") == idsz)  PipList[numpip].damfx |= DAMFXTIME;
      if (MAKE_IDSZ("PUSH") == idsz)  PipList[numpip].allowpush = iTmp;
      if (MAKE_IDSZ("DLEV") == idsz)  PipList[numpip].dyna_lightleveladd = iTmp / 1000.0;
      if (MAKE_IDSZ("DRAD") == idsz)  PipList[numpip].dyna_lightfalloffadd = iTmp / 1000.0;
      if (MAKE_IDSZ("IDAM") == idsz)  PipList[numpip].intdamagebonus = iTmp;
      if (MAKE_IDSZ("WDAM") == idsz)  PipList[numpip].wisdamagebonus = iTmp;
    }


    // Make sure it's referenced properly
    MadList[object].prtpip[pip] = numpip;
    numpip++;


    fclose(fileread);
    return btrue;
  }
  return bfalse;
}

//--------------------------------------------------------------------------------------------
void reset_particles(char* modname)
{
  // ZZ> This resets all particle data and reads in the coin and water particles
  int cnt, object;

  // Load in the standard global particles ( the coins for example )
  //BAD! This should only be needed once at the start of the game
  numpip = 0;

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money1_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money5_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money25_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.money100_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }

  //Load module specific information
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.weather4_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.weather5_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.splash_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }

  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s%s/%s", modname, CData.gamedat_dir, CData.ripple_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }

  //This is also global...
  snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s", CData.basicdat_dir, CData.globalparticles_dir, CData.defend_file);
  if (load_one_particle(CStringTmp1, 0, 0) == bfalse)
  {
    log_error("Data file was not found! (%s)\n", CStringTmp1);
  }


  // Now clear out the local pips
  object = 0;
  while (object < MAXMODEL)
  {
    cnt = 0;
    while (cnt < MAXPRTPIPPEROBJECT)
    {
      MadList[object].prtpip[cnt] = 0;
      cnt++;
    }
    object++;
  }
}

