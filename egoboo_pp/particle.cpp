// particle.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Particle.h"
#include "Character.h"
#include "Mesh.h"
#include "Mad.h"
#include "Camera.h"
#include "Profile.h"
#include "egoboo.h"

Dyna_List     DynaList;
Pip_List      PipList;
Particle_List PrtList;

//--------------------------------------------------------------------------------------------
void Dyna_List::make(Particle_List & plist)
{
  // ZZ> This function figures out which particles are visible, and it sets up dynamic
  //     lighting
  int cnt, tnc, slot;
  vec3_t dis;
  float distance;

  // Don't really make a list, just set to visible or not
  count = 0;
  distancetobeat = MAXDYNADIST*MAXDYNADIST;
  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    rprt_cnt.inview = true;
    if(!rprt_cnt.dyna_lighton) continue;

    // Set up the lights we need
    dis = rprt_cnt.pos - GCamera.track;

    distance = dis.x*dis.x + dis.y*dis.y + dis.z*dis.z;
    if(distance < distancetobeat)
    {
      if(count < DYNA_COUNT)
      {
        // Just add the light
        slot = count;
        _list[slot].distance = distance;
        count++;
      }
      else
      {
        // Overwrite the worst one
        slot = 0;
        tnc = 1;
        distancetobeat = _list[0].distance;
        while(tnc < DYNA_COUNT)
        {
          if(_list[tnc].distance > distancetobeat)
          {
            slot = tnc;
          }
          tnc++;
        }
        _list[slot].distance = distance;


        // Find the new distance to beat
        tnc = 1;
        distancetobeat = _list[0].distance;
        while(tnc < DYNA_COUNT)
        {
          if(_list[tnc].distance > distancetobeat)
          {
            distancetobeat = _list[tnc].distance;
          }
          tnc++;
        }
      }

      _list[slot].pos     = rprt_cnt.pos;
      _list[slot].level   = rprt_cnt.dyna_level;
      _list[slot].falloff = rprt_cnt.dyna_falloff;
    }

  } SCAN_PRT_END;
}

//--------------------------------------------------------------------------------------------
void play_particle_sound(int particle, Sint8 sound)
{
  // ZZ> This function plays a sound effect for a particle
  int frequency;

  frequency = FRQRANDOM;
  if ( VALID_WAVE_RANGE(sound) )
  {
    if ( VALID_MODEL(PrtList[particle].model) )
    {
      play_sound(PrtList[particle].pos, ProfileList[PrtList[particle].model].waveindex[sound]);
    }
    else
    {
      play_sound(PrtList[particle].pos, globalwave[sound]);
    };
  }
}

//--------------------------------------------------------------------------------------------
void Particle_List::free_one(int particle)
{
  // ZZ> This function sticks a particle back on the free particle stack and
  //     plays the sound associated with the particle

  if( INVALID_PRT(particle) ) return;

  if(SPRITE_ATTACK == PrtList[particle].type)
    int i=0;

  if (SPAWNNOCHARACTER != _list[particle].spawncharacterstate)
  {
    int child = spawn_one_character(_list[particle].pos, _list[particle].turn_lr, _list[particle].vel, _list[particle].vel_lr,
                                _list[particle].model, _list[particle].team, 0);
    if (VALID_CHR(child))
    {
      ChrList[child].ai.state = _list[particle].spawncharacterstate;
      ChrList[child].ai.owner = _list[particle].chr;
    }
  }

  play_particle_sound(particle, PipList[_list[particle].pip].soundend);

  return_one(particle);
}

//--------------------------------------------------------------------------------------------
Uint32 Particle_List::get_free(bool force)
{
  // ZZ> This function gets an unused particle.  If all particles are in use
  //     and force is set, it grabs the first unimportant one.  The particle
  //     index is the return value

  Uint32 prt_ref = my_alist_type::get_free();

  if(INVALID == prt_ref && force)
  {
    // search through for an invalid, but un-deleted particle
    bool found_one = false;
    for(int i=0; SIZE; i++)
    {
      // !!!!this should never happen!!!!
      if( !_list[i].allocated() ) 
        return i;

      if (0==_list[i].time || 0==_list[i].bump_size)
      {
        found_one = true;
        break;
      };
    }

    if(found_one)
    {
      // if found, de-allocate it
      return_one(i);

      // and re-allocate it, so that the particle data is refreshed
      prt_ref = get_free();
    }
  };

  if(INVALID!=prt_ref)
  {
    _list[prt_ref].initialize(); //initialize the particle data
  }

  return prt_ref;
}

//--------------------------------------------------------------------------------------------
//void initi_particle()
//{
//  // ZZ> This function spawns a new particle, and returns the number of that particle
//  int cnt, velocity;
//  float vel.x, vel.y, vel.z, tvel;
//  int offsetfacing, newrand;
//
//
//
//    // Necessary data for any part
//    rprt.on = true;
//    rprt.pip = loc_pip;
//    rprt.model = model;
//    rprt.inview = false;
//    rprt.level = 0;
//    rprt.team = team;
//    rprt.chr = characterorigin;
//    rprt.damagetype = rpip.damagetype;
//    rprt.spawncharacterstate = SPAWNNOCHARACTER;
//
//    // Lighting and sound
//    rprt.dyna_lighton = false;
//    if (multispawn == 0)
//    {
//      rprt.dyna_lighton = rpip.dyna_lightmode;
//      if (rpip.dyna_lightmode==DYNALOCAL)
//      {
//        rprt.dyna_lighton = false;
//        if (ChrList[characterorigin].team == GRTS.team_local)
//        {
//          rprt.dyna_lighton = true;
//        }
//      }
//    }
//    rprt.dyna_level   = rpip.dyna_level;
//    rprt.dyna_falloff = rpip.dyna_falloff;
//
//    // Set character attachments ( INVALID_CHR(characterattach) means none )
//    rprt.attachedtocharacter = characterattach;
//    rprt.grip = grip;
//
//    // Correct facing
//    facing += rpip.facingbase;
//
//    // Targeting...
//    vel.z = 0;
//    newrand = RANDIE;
//    z = z+rpip.zspacingbase+(newrand&rpip.zspacingrand)-(rpip.zspacingrand>>1);
//    newrand = RANDIE;
//    velocity = (rpip.xvel_ybase+(newrand&rpip.xvel_yrand));
//    rprt.target = oldtarget;
//    if (rpip.newtargetonspawn)
//    {
//      if (rpip.targetcaster)
//      {
//        // Set the target to the caster
//        rprt.target = characterorigin;
//      }
//      else
//      {
//        // Find a target
//        rprt.target = find_target(GParams, x, y, facing, rpip.targetangle, rpip.onlydamagefriendly, false, team, characterorigin, oldtarget);
//        if (VALID_CHR(rprt.target) && !rpip.homing)
//        {
//          facing = facing-GParams.use_angle;
//        }
//        // Correct facing for dexterity...
//        offsetfacing = 0;
//        if (ChrList[characterorigin].dexterity<PERFECTSTAT)
//        {
//          // Correct facing for randomness
//          offsetfacing = RANDIE;
//          offsetfacing = offsetfacing & rpip.facingrand;
//          offsetfacing -= (rpip.facingrand>>1);
//          offsetfacing = (offsetfacing*(PERFECTSTAT-ChrList[characterorigin].dexterity))>>13;  // Divided by PERFECTSTAT
//        }
//        if (VALID_CHR(rprt.target) && rpip.zaimspd!=0)
//        {
//          // These aren't velocities...  This is to do aiming on the Z axis
//          if (velocity > 0)
//          {
//            vel.x = ChrList[rprt.target].pos.x-x;
//            vel.y = ChrList[rprt.target].pos.y-y;
//            tvel = sqrtf(vel.x*vel.x+vel.y*vel.y)/velocity;  // This is the number of steps...
//            if (tvel > 0)
//            {
//              vel.z = (ChrList[rprt.target].pos.z+(ChrList[rprt.target].bump_size>>1)-z)/tvel;  // This is the vel.z alteration
//              if (vel.z < -(rpip.zaimspd>>1)) vel.z = -(rpip.zaimspd>>1);
//              if (vel.z > rpip.zaimspd) vel.z = rpip.zaimspd;
//            }
//          }
//        }
//      }
//      // Does it go away?
//      if (INVALID_CHR(rprt.target) && rpip.needtarget)
//      {
//        PrtList.free_one(cnt);
//        return Particle_List::INVALID;
//      }
//      // Start on top of target
//      if (VALID_CHR(rprt.target) && rpip.startontarget)
//      {
//        x = ChrList[rprt.target].pos.x;
//        y = ChrList[rprt.target].pos.y;
//      }
//    }
//    else
//    {
//      // Correct facing for randomness
//      offsetfacing = RANDIE;
//      offsetfacing = offsetfacing & rpip.facingrand;
//      offsetfacing -= (rpip.facingrand>>1);
//    }
//    facing+=offsetfacing;
//    rprt.turn_lr = facing;
//    facing = facing>>2;
//
//    // Location data from arguments
//    newrand = RANDIE;
//    x += cos_tab[(facing+0x2000)&0x3FFF]*(rpip.xyspacingbase+(newrand&rpip.xyspacingrand));
//    y += sin_tab[(facing+0x2000)&0x3FFF]*(rpip.xyspacingbase+(newrand&rpip.xyspacingrand));
//    if (x < 0)  x = 0;
//    if (x > GMesh.width()-2)  x = GMesh.width() - 2;
//    if (y < 0)  y = 0;
//    if (y > GMesh.height()-2)  y = GMesh.height() - 2;
//    rprt.pos.x = x;
//    rprt.pos.y = y;
//    rprt.pos.z = z;
//
//    // Velocity data
//    vel.x = cos_tab[facing+0x2000]*velocity;
//    vel.y = sin_tab[facing+0x2000]*velocity;
//    newrand = RANDIE;
//    vel.z += rpip.vel_zbase+(newrand&rpip.vel_zrand)-(rpip.vel_zrand>>1);
//    rprt.vel.x = vel.x;
//    rprt.vel.y = vel.y;
//    rprt.vel.z = vel.z;
//
//    // Template values
//    rprt.bump_size = rpip.bump_size;
//    rprt.bump_size_big = rprt.bump_size+(rprt.bump_size>>1);
//    rprt.bump_height = rpip.bump_height;
//    rprt.type = rpip.type;
//
//    // Image data
//    newrand = RANDIE;
//    rprt.rotate = (newrand&rpip.rotate_rand)+rpip.rotate_base;
//    rprt.rotate_add = rpip.rotate_add;
//    rprt.size = rpip.size_base;
//    rprt.size_add = rpip.size_add;
//    rprt.image = 0;
//    newrand = RANDIE;
//    rprt.image_add = rpip.image_add+(newrand&rpip.image_add_rand);
//    rprt.image_stt = rpip.image_base<<8;
//    rprt.image_max = rpip.numframes<<8;
//    rprt.time = rpip.time;
//    if (rpip.endlastframe && rprt.image_add!=0)
//    {
//      if (rprt.time==0)
//      {
//        // Part time is set to 1 cycle
//        rprt.time = (rprt.image_max/rprt.image_add)-1;
//      }
//      else
//      {
//        // Part time is used to give number of cycles
//        rprt.time = rprt.time*((rprt.image_max/rprt.image_add)-1);
//      }
//    }
//
//    // Set onwhichfan...
//    rprt.onwhichfan = Mesh::INVALID_INDEX;
//    if (rprt.pos.x > 0 && rprt.pos.x < GMesh.width() && rprt.pos.y > 0 && rprt.pos.y < GMesh.height())
//    {
//      rprt.onwhichfan = GMesh.getIndexPos(rprt.pos.x,rprt.pos.y);
//    }
//
//    // Damage stuff
//    rprt.damagebase = rpip.damagebase;
//    rprt.damagerand = rpip.damagerand;
//
//    // Spawning data
//    rprt.spawntime = rpip.contspawntime;
//    if (rprt.spawntime!=0)
//    {
//      rprt.spawntime = 1;
//      if (VALID_CHR(rprt.attachedtocharacter))
//      {
//        rprt.spawntime++; // Because attachment takes an update before it happens
//      }
//    }
//
//    // Sound effect
//    play_particle_sound(cnt, rpip.soundspawn);
//  }
//  return cnt;
//}
//
//
//

void Particle::deconstruct()
{
  // BB> This function erases all of the character data

  // save the data in the base class !!!!YYYUUUCCCKKK!!!!
  my_base tmp = getBase();

  // set all fields to 0
  memset(this, 0, sizeof(Particle));

  // restore the base class
  getBase() = tmp;
};

void Particle::construct()
{
  // BB> This initialized erased character data

  spawncharacterstate = SPAWNNOCHARACTER;
  begin_integration(); 
};

//--------------------------------------------------------------------------------------------
Uint16 spawn_one_particle(vec3_t & pos, Uint16 facing, vec3_t & vel, Sint16 weight,
                          Uint16 model, Uint16 pip,
                          Uint16 characterattach, Uint16 grip, Uint8 team,
                          Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget)
{
  // ZZ> This function spawns a new particle, and returns the number of that particle
  int cnt, velocity;
  vec3_t loc_pos = pos, loc_vel = vel;
  float tvel;
  int offsetfacing, newrand;
  

  //---- try to resolve the pip ----

    PIP_REF loc_pip;

    if (pip<PRTPIP_COUNT && VALID_MODEL(model))
      loc_pip = ProfileList[model].prtpip[pip];

    // if the pip reference is still invalid, it must be a global pip
    if(Pip_List::INVALID == loc_pip.index)
      loc_pip = pip;

    if( INVALID_PIP(loc_pip) )
      return Particle_List::INVALID;

  //---- try to resolve the pip ----


  cnt = PrtList.get_free(PipList[loc_pip].force);
  if (Particle_List::INVALID == cnt) return Particle_List::INVALID;

  Pip      & rpip = PipList[loc_pip];
  Particle & rprt = PrtList[cnt];

  //do the direct copy from the pip
  rprt.getData() = rpip.getData();
  rprt.getProperties() = rpip.getProperties();

  //set the template info
  rprt.model = model;
  rprt.pip   = loc_pip;

  // Necessary data for any part
  rprt._on  = true;
  rprt.team = team;
  rprt.chr  = characterorigin;
  rprt.stt  = pos;


  //---- try to resolve the weight ----
    if( VALID_MODEL(model) && VALID_CAP(ProfileList[model].cap_ref) )
      rprt.weight = MAX(weight, CapList[ProfileList[model].cap_ref].weight);
    else 
      rprt.weight = MAX(weight,1);      
  //---- try to resolve the weight ----

  // Lighting and sound
  if (multispawn == 0)
  {
    rprt.dyna_lighton = rpip.dyna_lightmode;
    if (rpip.dyna_lightmode==DYNALOCAL)
    {
      rprt.dyna_lighton = false;
      if (ChrList[characterorigin].team == GRTS.team_local)
      {
        rprt.dyna_lighton = true;
      }
    }
  }

  // Set character attachments ( INVALID_CHR(characterattach) means none )
  rprt.attachedtocharacter = characterattach;
  rprt.grip = grip;

  if(VALID_CHR(characterattach))
  {
    attach_particle_to_character(cnt, characterattach, grip);
  };

  // Correct facing
  facing += rpip.facingbase;

  // Targeting...
  newrand = RANDIE;
  loc_pos.z = loc_pos.z + rpip.zspacingbase+(newrand&rpip.zspacingrand)-(rpip.zspacingrand>>1);
  newrand = RANDIE;
  velocity = (rpip.xvel_ybase+(newrand&rpip.xvel_yrand));
  rprt.target = oldtarget;
  offsetfacing = 0;
  if (rpip.newtargetonspawn)
  {
    if (rpip.targetcaster)
    {
      // Set the target to the caster
      rprt.target = characterorigin;
    }
    else
    {
      // Find a target
      rprt.target = find_target(GParams, loc_pos, facing, rpip.targetangle, rpip.onlydamagefriendly, false, team, characterorigin, oldtarget);
      if (VALID_CHR(rprt.target) && !rpip.homing)
      {
        facing = facing-GParams.use_angle;
      }

      // Correct facing for dexterity...
      if (ChrList[characterorigin].dexterity<PERFECTSTAT)
      {
        // Correct facing for randomness
        offsetfacing = RANDIE;
        offsetfacing = offsetfacing & rpip.facingrand;
        offsetfacing -= (rpip.facingrand>>1);
        offsetfacing = (offsetfacing*(PERFECTSTAT-ChrList[characterorigin].dexterity))>>13;  // Divided by PERFECTSTAT
      }

      if (VALID_CHR(rprt.target) && rpip.zaimspd!=0)
      {
        // These aren't velocities...  This is to do aiming on the Z axis
        if (velocity > 0)
        {
          vec3_t dpos = ChrList[rprt.target].pos - loc_pos;
          tvel = sqrtf(dpos.x*dpos.x + dpos.y*dpos.y)/velocity;  // This is the number of steps...
          if (tvel > 0)
          {
            dpos.z = (ChrList[rprt.target].pos.z+(ChrList[rprt.target].calc_bump_size)-loc_pos.z)/tvel;  // This is the dpos.z alteration
            if (dpos.z < -(rpip.zaimspd>>1)) dpos.z = -(rpip.zaimspd>>1);
            if (dpos.z > rpip.zaimspd)       dpos.z = rpip.zaimspd;
          }
        }
      }
    }

    // Does it go away?
    if (INVALID_CHR(rprt.target) && rpip.needtarget)
    {
      PrtList.free_one(cnt);
      return Particle_List::INVALID;
    }

    // Start on top of target
    if (VALID_CHR(rprt.target) && rpip.startontarget)
    {
      loc_pos.x = ChrList[rprt.target].pos.x;
      loc_pos.y = ChrList[rprt.target].pos.y;
    }
  }
  else
  {
    // Correct facing for randomness
    offsetfacing = RANDIE;
    offsetfacing = offsetfacing & rpip.facingrand;
    offsetfacing -= (rpip.facingrand>>1);
  }
  facing+=offsetfacing;
  rprt.turn_lr = facing;
  facing = facing>>2;

  // Location data from arguments
  newrand = RANDIE;
  loc_pos.x += cos_tab[facing]*(rpip.xyspacingbase+(newrand&rpip.xyspacingrand));
  loc_pos.y += sin_tab[facing]*(rpip.xyspacingbase+(newrand&rpip.xyspacingrand));
  if (loc_pos.x < 0)  loc_pos.x = 0;
  if (loc_pos.x > GMesh.width()-2)  loc_pos.x = GMesh.width() - 2;
  if (loc_pos.y < 0)  loc_pos.y = 0;
  if (loc_pos.y > GMesh.height()-2)  loc_pos.y = GMesh.height() - 2;
  rprt.pos = loc_pos;

  // Velocity data
  loc_vel.x += cos_tab[facing]*velocity;
  loc_vel.y += sin_tab[facing]*velocity;
  newrand = RANDIE;
  loc_vel.z += rpip.vel_zbase + (newrand&rpip.vel_zrand) - (rpip.vel_zrand>>1);
  rprt.vel = loc_vel;

  // Template values
  rprt.bump_size_big = rprt.bump_size+(rprt.bump_size>>1);


  // Image data
  newrand = RANDIE;
  rprt.rotate = (newrand&rpip.rotate_rand)+rpip.rotate_base;
  rprt.size =   rpip.size_base;
  rprt.image = 0;
  newrand = RANDIE;
  rprt.image_add = rpip.image_add+(newrand&rpip.image_add_rand);
  rprt.image_stt = rpip.image_base<<8;
  rprt.image_max = rpip.numframes<<8;

  if (rpip.endlastframe && rprt.image_add!=0)
  {
    if (rprt.time==0)
    {
      // Part time is set to 1 cycle
      rprt.time = (rprt.image_max/rprt.image_add)-1;
    }
    else
    {
      // Part time is used to give number of cycles
      rprt.time = rprt.time*((rprt.image_max/rprt.image_add)-1);
    }
  }

  if(rprt.time == 0)
  {
    // set the particle lifetime to be infinite
    rprt.time = -1;
  };

  // Set onwhichfan...
  rprt.onwhichfan = GMesh.getIndexPos(rprt.pos.x,rprt.pos.y);

  // Spawning data
  rprt.spawntime = rpip.contspawntime;
  if (rprt.spawntime!=0)
  {
    rprt.spawntime = 1;
    if (VALID_CHR(rprt.attachedtocharacter))
    {
      rprt.spawntime++; // Because attachment takes an update before it happens
    }
  }

  // Sound effect
  play_particle_sound(cnt, rpip.soundspawn);

  // bumpers
  rprt.calc_is_platform   = false;
  rprt.calc_is_mount      = false;
  rprt.calc_bump_size_x   = rprt.bump_size;
  rprt.calc_bump_size_y   = rprt.bump_size;
  rprt.calc_bump_size     = rprt.bump_size;
  rprt.calc_bump_size_xy  = rprt.bump_size_big;
  rprt.calc_bump_size_yx  = rprt.bump_size_big; 
  rprt.calc_bump_size_big = rprt.bump_size_big;
  rprt.calc_bump_height   = rprt.bump_height;

  return cnt;
}

//--------------------------------------------------------------------------------------------
void disaffirm_attached_particles(Uint16 character, Uint32 pip_type)
{
  // ZZ> This function makes sure a character has no attached particles
  Uint16 particle;

  SCAN_PRT_BEGIN(particle, rprt_prt)
  {
    if (  rprt_prt.attachedtocharacter==character &&                              // prt is attached to this character
         (rprt_prt.pip.index==pip_type || Pip_List::INVALID==pip_type) )          // prt is of type pip_type
    {
      PrtList.free_one(particle);
    }
  } SCAN_PRT_END;

  // Set the ai.alert for disaffirmation ( wet torch )
  ChrList[character].ai.alert |= ALERT_IF_DISAFFIRMED;
}

//--------------------------------------------------------------------------------------------
Uint16 number_of_attached_particles(Uint16 character)
{
  // ZZ> This function returns the number of particles attached to the given character
  Uint16 cnt, particle;

  cnt = 0;
  SCAN_PRT_BEGIN(particle, rprt_prt)
  {
    if (rprt_prt.attachedtocharacter == character)
    {
      cnt++;
    }
  } SCAN_PRT_END;

  return cnt;
}

//--------------------------------------------------------------------------------------------
void reaffirm_attached_particles(Uint16 character)
{
  // ZZ> This function makes sure a character has all of it's particles
  Uint16 numberattached;
  Uint16 particle;

  if(INVALID_CHR(character)) return;
  Character & rchr = ChrList[character];

  numberattached = number_of_attached_particles(character);
  while (numberattached < rchr.getCap().attachedprtamount)
  {
    particle = spawn_one_particle(rchr.pos, 0, rchr.vel, 0, rchr.model, rchr.getCap().attachedprttype, character, GRIP_LAST+numberattached, rchr.team, character, numberattached, Character_List::INVALID);
    if (VALID_PRT(particle))
    {
      attach_particle_to_character(particle, character, PrtList[particle].grip);
    }
    numberattached++;
  }

  // Set the ai.alert for reaffirmation ( for exploding barrels with fire )
  rchr.ai.alert |= ALERT_IF_REAFFIRMED;
}

//--------------------------------------------------------------------------------------------
static int prt_counter = 0;
void calc_prt_environment(Physics_Info & loc_phys, Particle & rprt, Physics_Environment & enviro);

void move_particles(Physics_Info & loc_phys, float dframe)
{
  // ZZ> This is the particle physics function
  int cnt, tnc;
  Uint16 facing, particle;
  float level;

  // move all new particles
  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    // To make it easier
    Pip & rpip = rprt_cnt.getPip();

    if(!rprt_cnt.integration_allowed) rprt_cnt.begin_integration();

    rprt_cnt.onwhichfan = GMesh.getFanblockPos(rprt_cnt.pos);
    rprt_cnt.level      = 0;
    if (Mesh::INVALID_INDEX != rprt_cnt.onwhichfan)
    {
      rprt_cnt.level = GMesh.getHeight(rprt_cnt.pos.x, rprt_cnt.pos.y);

      if( PipList[rprt_cnt.pip].homing && rprt_cnt.level<WaterList.level_surface)
        rprt_cnt.level = WaterList.level_surface = RAISE;
    }

    // Animate particle
    if( rpip.endlastframe && rprt_cnt.image >= rprt_cnt.image_max && SPRITE_ATTACK!=rprt_cnt.type)
    {
      rprt_cnt.requestDestroy();
      rprt_cnt.image = 0;
    }
    else
    {
      rprt_cnt.image += rprt_cnt.image_add;
      if(rprt_cnt.image > rprt_cnt.image_max)
      {
        rprt_cnt.image = 0;
      };
    }
    rprt_cnt.rotate += rprt_cnt.rotate_add;
    rprt_cnt.size   += rprt_cnt.size_add;
    if(rprt_cnt.size < 0) rprt_cnt.size = 0;

    // Change dyna light values
    rprt_cnt.dyna_level   += rpip.dyna_leveladd;
    rprt_cnt.dyna_falloff += rpip.dyna_falloffadd;

    // Make it sit on the floor...  Shift is there to correct for sprite size (8 bit fixed point)
    level = rprt_cnt.level + (rprt_cnt.size >> (FIXEDPOINT_BITS+1));

    // do the particle physics only if it it not directly attached to something
    if (INVALID_CHR(rprt_cnt.attachedtocharacter))
    {
      Physics_Environment enviro;

      calc_prt_environment(loc_phys, rprt_cnt, enviro);

      // apply gravity
      rprt_cnt.accumulate_acc_z(loc_phys.gravity);

      // apply "air friction" from last frame
      rprt_cnt.accumulate_vel( -(1-enviro.fric_fluid)*rprt_cnt.vel, -(1-enviro.fric_fluid)*rprt_cnt.vel_lr);

      // Check wall/floor collisions
      vec3_t normal;
      if ( rprt_cnt.hitawall(GMesh, normal) )
      {
        if(rpip.endwall || rpip.endground)
        {
          rprt_cnt.requestDestroy();
          continue;
        };

        // perp velocity is parallel to the normal...
        vec3_t vperp = parallel_normalized(normal, rprt_cnt.vel);
        vec3_t vpara = rprt_cnt.vel - vperp;

        // calculate the reflection off the wall
        vec3_t vbounce = vpara - vperp*rprt_cnt.dampen;

        // make the chr bounce
        rprt_cnt.accumulate_vel(vbounce-rprt_cnt.vel, 0);

        // TO DO : scale the volume by the reflection impulse?

        // Play the sound for hitting a wall [WSND]
        play_particle_sound(cnt, rpip.soundwall);
      }

      if( fabsf(normal.x) > 1e-4 )
      {
        // Change facing
        facing = rprt_cnt.turn_lr;
        if (facing < 0x8000)
        {
          facing-=NORTH;
          facing = ~facing;
          facing+=NORTH;
        }
        else
        {
          facing-=SOUTH;
          facing = ~facing;
          facing+=SOUTH;
        }
        rprt_cnt.turn_lr = facing;
      }

      // Check wall collisions in Y
      if( fabsf(normal.y) > 1e-4 )
      {
        // Change facing
        facing = rprt_cnt.turn_lr;
        if (facing < 0x4000 || facing > 49152)
        {
          facing = ~facing;
        }
        else
        {
          facing-=EAST;
          facing = ~facing;
          facing+=EAST;
        }
        rprt_cnt.turn_lr = facing;
      }

      // Check floor collisions
      if (rprt_cnt.pos.z <= level && rprt_cnt.old.pos.z > level)
      {
        if (rpip.endground)
        {
          rprt_cnt.requestDestroy();
          continue;
        };

        if(fabs(rprt_cnt.vel.z) < STOPBOUNCINGPART)
        {
          rprt_cnt.accumulate_vel_z(-rprt_cnt.vel.z);
        }
        else
        {
          rprt_cnt.accumulate_vel_z(-(1+rprt_cnt.dampen)*rprt_cnt.vel.z);
        }

        rprt_cnt.accumulate_pos_z( (level-rprt_cnt.pos.z) );

        //else if (rprt_cnt.vel.z < 0)
        //{
        //  // Make it bounce
        //  fx += -rprt_cnt.vel.x*(1-rpip.dampen);
        //  fy += -rprt_cnt.vel.y*(1-rpip.dampen);
        //  fz += -rprt_cnt.vel.z*(1+rpip.dampen);

        //  // Play the sound for hitting the floor [FSND]
        //  play_particle_sound(cnt, rpip.soundfloor);
        //}
      }

      // Do homing
      if (rpip.homing && VALID_CHR(rprt_cnt.target))
      {
        if (!ChrList[rprt_cnt.target].alive)
        {
          rprt_cnt.requestDestroy();
          continue;
        }


        if (INVALID_CHR(rprt_cnt.attachedtocharacter))
        {
          vec3_t fhoming = (ChrList[rprt_cnt.target].pos - rprt_cnt.pos);
          fhoming.z += ChrList[rprt_cnt.target].calc_bump_height/2 - rprt_cnt.calc_bump_height/2;
          float fhoming2  = dist_squared( fhoming );
          if(fhoming2 > 0)
          {
            float nrmf = rpip.homingaccel * (128*128 / ( 128*128 + fhoming2 ));
            normalize_iterative(fhoming);
            rprt_cnt.accumulate_acc(fhoming*nrmf, 0);
          };
        };

        if (rpip.rotatetoface)
        {
          // Turn to face target
          facing = diff_to_turn(ChrList[rprt_cnt.target].pos, rprt_cnt.pos);
          rprt_cnt.accumulate_pos_lr(facing - rprt_cnt.turn_lr);
        }

      }

      if(rpip.spdlimit<0)
      {
        //this thing is buoyant
        float height_multiply = 100;
        float buoyant_rise    = height_multiply*(-rpip.spdlimit);
        float buoyant_height  = buoyant_rise + rprt_cnt.stt.z;
        float height_lerp     = (rprt_cnt.pos.z-rprt_cnt.stt.z)/buoyant_rise;
 
        float drag   = 0.3;
        float spring = (-rpip.spdlimit)/buoyant_height/(1.0-drag);

        rprt_cnt.accumulate_vel( -rprt_cnt.vel * drag );
        rprt_cnt.accumulate_acc_z( spring*(buoyant_height-rprt_cnt.pos.z) );
      } 
      else if(rpip.spdlimit>0)
      {
        //estimate the drag
        float drag = 1 - ABS(GPhys.gravity)/rpip.spdlimit;

        rprt_cnt.accumulate_vel( -rprt_cnt.vel*(1-drag) );
        rprt_cnt.accumulate_acc_z(GPhys.gravity);
      }

      //float buoy_speed = -rpip.spdlimit;
      //// Do buoyancy
      //if(buoy_speed > 0)
      //{
      //  // Estimate some constants
      //  float bdrag = 1 - loc_phys.fric_air;
      //  float bmax  = fabs(buoy_speed)*loc_phys.fric_air;

      //  float buoyant_height = bmax*100.0f;
      //  float bouyant_force  = (buoyant_height-rprt_cnt.pos.z)/100.0f;
      //  float bouyant_drag   = -rprt_cnt.vel.z*bdrag;
      //        fz += bouyant_force + bouyant_drag;
      //}
      //else if(buoy_speed<0 && buoy_speed>loc_phys.gravity)
      //{
      //  float bdrag = (1-loc_phys.gravity/buoy_speed)/loc_phys.fric_air;
      //  fz += rprt_cnt.vel.z*bdrag;
      //}
      //else if(buoy_speed < loc_phys.gravity)
      //{
      //  float bdrag = (1-loc_phys.gravity/buoy_speed)/loc_phys.fric_air;
      //  fz += -rprt_cnt.vel.z*bdrag;
      //};

    }
    else
    {

      if (0==(wldframe&0x1F))
      {
        Uint32 chara = rprt_cnt.attachedtocharacter;
        Character & rchr = ChrList[chara];

        // Attached particle damage ( Burning )
        if (PipList[rprt_cnt.pip].xvel_ybase==0)
        {
          // Make character limp
          rchr.accumulate_vel( vec3_t(-rchr.vel.x, -rchr.vel.y, 0), 0);
        }

        damage_character(chara, 0x8000, rprt_cnt.damagebase, rprt_cnt.damagerand, rprt_cnt.damagetype, rprt_cnt.team, rprt_cnt.chr, PipList[rprt_cnt.pip].damfx);
      };

      if (rpip.endground && rprt_cnt.vel.z<0 && rprt_cnt.pos.z < level + 0.1)
      {
        rprt_cnt.requestDestroy();
        continue;
      }

      if (rpip.endwall)
      {
        vec3_t normal;
        if ( rprt_cnt.hitawall(GMesh, normal) )
        {
          rprt_cnt.requestDestroy();
          continue;
        }
      }
    }




    // Spawn new particles if continually spawning
    if (rpip.contspawnamount>0)
    {
      rprt_cnt.spawntime--;
      if (rprt_cnt.spawntime == 0)
      {
        rprt_cnt.spawntime = rpip.contspawntime;
        facing = rprt_cnt.turn_lr;
        tnc = 0;
        while (tnc < rpip.contspawnamount)
        {
          particle = spawn_one_particle(rprt_cnt.pos, facing, rprt_cnt.vel, 0, rprt_cnt.model, rpip.contspawnpip,
                                        Character_List::INVALID, GRIP_LAST, rprt_cnt.team, rprt_cnt.chr, tnc, rprt_cnt.target);

          if (rpip.turn_add!=0 &&  VALID_PRT(particle))
          {
            // Hack to fix velocity
            PrtList[particle].accumulate_vel(rprt_cnt.vel, rprt_cnt.vel_lr);
          }
          facing+=rpip.contspawnfacingadd;
          tnc++;
        }
      }
    }


    // Check underwater
    if (rpip.endwater && rprt_cnt.pos.z < WaterList.level_douse && GMesh.has_flags(rprt_cnt.onwhichfan, MESHFX_WATER))
    {
      // Splash for particles is just a ripple
      vec3_t postmp = rprt_cnt.pos;
      postmp.z = WaterList.level_surface;

      spawn_one_particle(postmp, 0, rprt_cnt.vel, 0, Profile_List::INVALID, PRTPIP_RIPPLE, Character_List::INVALID, 
                         GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, 0, Character_List::INVALID);

      // Check for disaffirming character
      if ( VALID_CHR(rprt_cnt.attachedtocharacter) )
      {
        // Disaffirm the whole character
        disaffirm_attached_particles(rprt_cnt.attachedtocharacter, rprt_cnt.pip.index);
      }
      else
      {
        //deferred destroy. set timeout for next frame
        rprt_cnt.requestDestroy();
        continue;
      }
    }

    // handle the lifetime
    //prt_counter++;
    //if(0 == (prt_counter & ((1<<0)-1)))
    //{
      if(rprt_cnt.time>0) rprt_cnt.time--;
    //};

    // destroy the particle if it dies from old age
    if(rprt_cnt.time==0) 
      rprt_cnt.requestDestroy();

  } SCAN_PRT_END;

  // deal with all timed-out particles
  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    if (!rprt_cnt.destroy_me) continue;

    facing = rprt_cnt.turn_lr;
    for (tnc=0; tnc < PipList[rprt_cnt.pip].endspawnamount; tnc++)
    {

      spawn_one_particle(rprt_cnt.old.pos, facing, rprt_cnt.old.vel, 0, rprt_cnt.model, PipList[rprt_cnt.pip].endspawnpip,
        Character_List::INVALID, GRIP_LAST, rprt_cnt.team, rprt_cnt.chr, tnc, rprt_cnt.target);

      facing += PipList[rprt_cnt.pip].endspawnfacingadd;
    }

    PrtList.free_one(cnt);
  } SCAN_PRT_END;


}

//--------------------------------------------------------------------------------------------
void attach_particles()
{
  // ZZ> This function attaches particles to their characters so everything gets
  //     drawn right
  int cnt;

  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    if (INVALID_CHR(rprt_cnt.attachedtocharacter)) continue;

    attach_particle_to_character(cnt, rprt_cnt.attachedtocharacter, rprt_cnt.grip);

    // Correct facing so swords knock characters in the right direction...
    if (PipList[rprt_cnt.pip].damfx&DAMFX_TURN)
      rprt_cnt.turn_lr = ChrList[rprt_cnt.attachedtocharacter].turn_lr;

  } SCAN_PRT_END;
}

//--------------------------------------------------------------------------------------------
void Particle_List::free_all()
{
  // ZZ> This function resets the particle allocation lists

  for (int i=0; i<SIZE; i++)
  {
    _list[i]._on = false;
  }

  _setup();
}

//--------------------------------------------------------------------------------------------
void Particle_List::setup_particles()
{
  // ZZ> This function sets up particle data
  int cnt;
  double x, y;

  PrtList.texture = 0;

  // Image coordinates on the big particle bitmap
  for (cnt = 0; cnt < MAXPARTICLEIMAGE; cnt++)
  {
    x = cnt&15;
    y = cnt>>4;
    PrtList.txcoord[cnt][0].s = (float)((.05+x)/16.0);
    PrtList.txcoord[cnt][1].s = (float)((.95+x)/16.0);
    PrtList.txcoord[cnt][0].t = (float)((.05+y)/16.0);
    PrtList.txcoord[cnt][1].t = (float)((.95+y)/16.0);
  }

  // Reset the allocation table
  PrtList.free_all();
}

//--------------------------------------------------------------------------------------------
void spawn_bump_particles(Uint16 character, Uint16 particle)
{
  // ZZ> This function is for catching characters on fire and such
  int cnt;
  Sint16 x, y, z;
  int distance, bestdistance;
  Uint16 facing, bestvertex;
  Uint16 amount;
  PIP_REF pip;
  Uint16 vertices;
  Uint16 direction, left, right, model;

  pip = PrtList[particle].pip;
  amount = PipList[pip].bumpspawnamount;

  if (amount != 0 || PipList[pip].spawnenchant)
  {
    // Only damage if hitting from proper direction
    model = ChrList[character].model;
    CAP_REF cap_ref = ProfileList[model].cap_ref;
    MAD_REF mad_ref = ProfileList[model].mad_ref;

    vertices = MadList[mad_ref].numVertices();
    direction = ChrList[character].turn_lr - vec_to_turn(PrtList[particle].vel);
    if ( ChrList[character].getMad().getExtras(ChrList[character].ani.frame).framefx & MADFX_INVICTUS)
    {
      // I Frame
      if (PipList[pip].damfx&DAMFX_BLOC)
      {
        left = 0xFFFF;
        right = 0;
      }
      else
      {
        direction -= CapList[cap_ref].iframefacing;
        left = (~CapList[cap_ref].iframeangle);
        right = CapList[cap_ref].iframeangle;
      }
    }
    else
    {
      // N Frame
      direction -= CapList[cap_ref].nframefacing;
      left = (~CapList[cap_ref].nframeangle);
      right = CapList[cap_ref].nframeangle;
    }

    // Check that direction
    if (direction <= left && direction >= right)
    {
      // Spawn new enchantments
      if (PipList[pip].spawnenchant)
      {
        spawn_one_enchant(PrtList[particle].chr, character, Character_List::INVALID, PrtList[particle].model);
      }

      // Spawn particles
      if (amount != 0 && !ChrList[character].getCap().resistbumpspawn && !ChrList[character].invictus && vertices != 0 && (ChrList[character].damagemodifier[PrtList[particle].damagetype]&DAMAGE_SHIFT)<3)
      {
        if (amount == 1)
        {
          // A single particle ( arrow? ) has been stuck in the character...
          // Find best vertex to attach to
          bestvertex = 0;
          bestdistance = 9999999;
          cnt = 0;
          z = -ChrList[character].pos.z + PrtList[particle].pos.z + RAISE;
          facing = PrtList[particle].turn_lr-ChrList[character].turn_lr-0x4000;
          facing = facing>>2;
          x = -0x2000*sin_tab[facing];
          y =  0x2000*cos_tab[facing];
          z =  z<<10;

          JF::MD2_Model * mdl = ChrList[character].getMD2();
          if(NULL!=mdl)
          {
            const JF::MD2_Frame * pframe = ChrList[character].getFrame(mdl);

            if(NULL!=pframe)
            {
              Uint32 vrt_cnt = mdl->numVertices();

              for (cnt = 0; cnt < vrt_cnt; cnt++)
              {
                distance = ABS(x-pframe->vertices[cnt].x) + ABS(y-pframe->vertices[cnt].y) + ABS(z-pframe->vertices[cnt].z);
                if (distance < bestdistance)
                {
                  bestdistance = distance;
                  break;
                }
              }
            }
          }

          spawn_one_particle(ChrList[character].pos, 0, ChrList[character].vel, 0, PrtList[particle].model, PipList[pip].bumpspawnpip,
                             character, bestvertex+1, PrtList[particle].team, PrtList[particle].chr, cnt, character);
        }
        else
        {
          amount = (amount*vertices)>>5;  // Correct amount for size of character

          for (cnt = 0; cnt < amount; cnt++)
          {
            spawn_one_particle(ChrList[character].pos,  0, ChrList[character].vel, 0, PrtList[particle].model, PipList[pip].bumpspawnpip,
                               character, rand()%vertices, PrtList[particle].team, PrtList[particle].chr, cnt, character);
          }
        }
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void do_weather_spawn()
{
  // ZZ> This function drops snowflakes or rain or whatever, also swings the camera
  int particle, cnt;
  Uint8 foundone;


  GWeather.time -= GWeather.time>0 ? 1 : 0;
  if (GWeather.time==0)
  {
    GWeather.time = GWeather.time_reset;

    // Find a valid character
    foundone = false;
    for (cnt = 0; cnt < Character_List::SIZE; cnt++)
    {
      GWeather.player++;
      GWeather.player %= Character_List::SIZE;
      if ( VALID_CHR(GWeather.player) && !ChrList[GWeather.player].is_inpack )
      {
        foundone = true;
        cnt      = GWeather.player;
        break;
      }
    }

    // Did we find one?
    if (foundone)
    {
      // Yes, so spawn over that character
      vec3_t postmp = ChrList[cnt].pos;
      postmp.z += 5*0x80;

      if (!GWeather.overwater || mesh_is_over_water(ChrList[cnt].pos))
      {
        spawn_one_particle(postmp, 0, ChrList[cnt].vel, 0, Profile_List::INVALID, PRTPIP_WEATHER_1, Character_List::INVALID, GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, 0, Character_List::INVALID);
      }
    }
  }

  GCamera.swing = (GCamera.swing + GCamera.swing_rate) & 0x3FFF;
}

//--------------------------------------------------------------------------------------------
bool Pip::load(const char *szLoadName, Pip & rpip)
{
  // ZZ> This function loads a particle template, returning false if the file wasn't
  //     found
  FILE* fileread;
  IDSZ test, idsz;
  int iTmp;
  char cTmp;


  fileread = fopen(szLoadName, "r");
  if (!fileread) return false;

  //read up to the 1st ':'
  fgets(rpip.comment, 0xFF, fileread);

  //go back to the beginning, just in case
  rewind(fileread);

  // General data
  globalname = szLoadName;
  rpip.force = get_next_bool(fileread);
  goto_colon(fileread);  cTmp = get_first_letter(fileread);
  switch(toupper(cTmp))
  {
    case 'L': rpip.type = SPRITE_LIGHT; break;
    case 'T': rpip.type = SPRITE_ALPHA; break;
    default:
    case 'S': rpip.type = SPRITE_SOLID; break;
  };

  rpip.image_base = get_next_int(fileread);
  rpip.numframes = get_next_int(fileread);
  rpip.image_add = get_next_int(fileread);
  rpip.image_add_rand = get_next_int(fileread);
  rpip.rotate_base = get_next_int(fileread);
  rpip.rotate_rand = get_next_int(fileread);
  rpip.rotate_add = get_next_int(fileread);
  rpip.size_base = get_next_int(fileread);
  rpip.size_add = get_next_int(fileread);
  rpip.spdlimit = get_next_float(fileread);
  rpip.turn_add = get_next_int(fileread);

  // Ending conditions
  rpip.endwater = get_next_bool(fileread);
  rpip.endbump = get_next_bool(fileread);
  rpip.endground = get_next_bool(fileread);
  rpip.endlastframe = get_next_bool(fileread);
  rpip.time = get_next_int(fileread);

  // Collision data
  rpip.dampen = get_next_float(fileread);
  rpip.bumpmoney = get_next_int(fileread);
  rpip.bump_size = get_next_int(fileread);
  rpip.bump_height = get_next_int(fileread);
  read_next_pair(fileread, rpip.damagebase, rpip.damagerand);
  goto_colon(fileread);  rpip.damagetype =  get_damage_type(fileread);

  // Lighting data
  goto_colon(fileread);  cTmp = get_first_letter(fileread);
  rpip.dyna_lightmode = DYNAOFF;
  if (cTmp=='T' || cTmp=='t') rpip.dyna_lightmode = DYNAON;
  if (cTmp=='L' || cTmp=='l') rpip.dyna_lightmode = DYNALOCAL;
  rpip.dyna_level = get_next_float(fileread);
  rpip.dyna_falloff = get_next_int(fileread);
  if(rpip.dyna_level>0 && rpip.dyna_lightmode==DYNAOFF)
  {
     rpip.dyna_lightmode = DYNAON;
  };
  if (rpip.dyna_falloff > MAXFALLOFF)  rpip.dyna_falloff = MAXFALLOFF;

  // Initial spawning of this particle
  rpip.facingbase = get_next_int(fileread);
  rpip.facingrand = get_next_int(fileread);
  rpip.xyspacingbase = get_next_int(fileread);
  rpip.xyspacingrand = get_next_int(fileread);
  rpip.zspacingbase = get_next_int(fileread);
  rpip.zspacingrand = get_next_int(fileread);
  rpip.xvel_ybase = get_next_int(fileread);
  rpip.xvel_yrand = get_next_int(fileread);
  rpip.vel_zbase = get_next_int(fileread);
  rpip.vel_zrand = get_next_int(fileread);

  // Continuous spawning of other particles
  rpip.contspawntime = get_next_int(fileread);
  rpip.contspawnamount = get_next_int(fileread);
  rpip.contspawnfacingadd = get_next_int(fileread);
  rpip.contspawnpip = get_next_int(fileread);

  // End spawning of other particles
  rpip.endspawnamount = get_next_int(fileread);
  rpip.endspawnfacingadd = get_next_int(fileread);
  rpip.endspawnpip = get_next_int(fileread);

  // Bump spawning of attached particles
  rpip.bumpspawnamount = get_next_int(fileread);
  rpip.bumpspawnpip = get_next_int(fileread);

  // Random stuff  !!!BAD!!! Not complete
  rpip.dazetime = get_next_int(fileread);
  rpip.grogtime = get_next_int(fileread);
  rpip.spawnenchant = get_next_bool(fileread);
  goto_colon(fileread);  // !!Cause roll
  goto_colon(fileread);  // !!Cause pancake
  rpip.needtarget = get_next_bool(fileread);
  rpip.targetcaster = get_next_bool(fileread);
  rpip.startontarget = get_next_bool(fileread);
  rpip.onlydamagefriendly = get_next_bool(fileread);
  iTmp = get_next_int(fileread);
  rpip.soundspawn = CLIP(iTmp, -1, MAXWAVE-1);
  iTmp = get_next_int(fileread);
  rpip.soundend = CLIP(iTmp, -1, MAXWAVE-1);
  rpip.friendlyfire = get_next_bool(fileread);
  goto_colon(fileread);  // !!Hate group only
  rpip.newtargetonspawn = get_next_bool(fileread);
  rpip.targetangle = get_next_int(fileread)>>1;
  rpip.homing = get_next_bool(fileread);
  rpip.homingfriction = get_next_float(fileread);
  rpip.homingaccel = get_next_float(fileread);
  rpip.rotatetoface = get_next_bool(fileread);

  // Clear expansions...
  rpip.zaimspd = 0;
  rpip.soundfloor = -1;
  rpip.soundwall = -1;
  rpip.endwall = rpip.endground;
  rpip.damfx = DAMFX_TURN;
  if (rpip.homing)  rpip.damfx = DAMFX_NONE;
  rpip.allowpush = true;
  rpip.dyna_falloffadd = 0;
  rpip.dyna_leveladd = 0;
  // Read expansions
  while (goto_colon_yesno(fileread))
  {
    idsz = get_idsz(fileread);
    fscanf(fileread, "%c%d", &cTmp, &iTmp);

    if (idsz == IDSZ("TURN"))  rpip.damfx = DAMFX_NONE;
    else if (idsz == IDSZ("ZSPD"))  rpip.zaimspd = iTmp;
    else if (idsz == IDSZ("FSND"))  rpip.soundfloor = iTmp;
    else if (idsz == IDSZ("WSND"))  rpip.soundwall = iTmp;
    else if (idsz == IDSZ("WEND"))  rpip.endwall = iTmp;
    else if (idsz == IDSZ("ARMO"))  rpip.damfx |= DAMFX_ARMO;
    else if (idsz == IDSZ("BLOC"))  rpip.damfx |= DAMFX_BLOC;
    else if (idsz == IDSZ("ARRO"))  rpip.damfx |= DAMFX_ARRO;
    else if (idsz == IDSZ("TIME"))  rpip.damfx |= DAMFX_TIME;
    else if (idsz == IDSZ("PUSH"))  rpip.allowpush = (iTmp!=0);
    else if (idsz == IDSZ("DLEV"))  rpip.dyna_leveladd = iTmp/1000.0;
    else if (idsz == IDSZ("DRAD"))  rpip.dyna_falloffadd = iTmp/1000.0;
  }

  fclose(fileread);

  rpip.loaded = true;

  return true;
}

//--------------------------------------------------------------------------------------------
void Pip_List::reset_particles(const char * modname)
{
  // ZZ> This resets all particle data and reads in the coin and water particles
  char newloadname[0x0100];

  //get rid of the old particle info
  PipList.reset();
  ProfileList.release_local_pips();

  // Load in the standard particles ( the coins for example )
  make_newloadname(modname, "gamedat/1money.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_COIN_001))
  {
    general_error(0, 0, "\"gamedat/1money.txt\" NOT FOUND");
  }

  make_newloadname(modname, "gamedat/5money.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_COIN_005))
  {
    general_error(0, 0, "\"gamedat/5money.txt\" NOT FOUND");
  }

  make_newloadname(modname, "gamedat/25money.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_COIN_025))
  {
    general_error(0, 0, "\"gamedat/25money.txt\" NOT FOUND");
  }

  make_newloadname(modname, "gamedat/100money.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_COIN_100))
  {
    general_error(0, 0, "\"gamedat/25money.txt\" NOT FOUND");
  }

  make_newloadname(modname, "gamedat/weather4.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_WEATHER_1))
  {
    general_error(0, 0, "\"gamedat/weather4.txt\" NOT FOUND");
  }

  make_newloadname(modname, "gamedat/weather5.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_WEATHER_2))
  {
    general_error(0, 0, "\"gamedat/weather5.txt\" NOT FOUND");
  }

  make_newloadname(modname, "gamedat/splash.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_SPLASH))
  {
    general_error(0, 0, "\"gamedat/splash.txt\" NOT FOUND");
  }

  make_newloadname(modname, "gamedat/ripple.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_RIPPLE))
  {
    general_error(0, 0, "\"gamedat/ripple.txt\" NOT FOUND");
  }

  make_newloadname(modname, "gamedat/defend.txt", newloadname);
  if (Pip_List::INVALID==PipList.load_one_pip(newloadname,PRTPIP_DEFEND))
  {
    general_error(0, 0, "\"gamedat/defend.txt\" NOT FOUND");
  }
}

Uint32 Pip_List::load_one_pip(const char *szLoadName, Uint32 force)
{
  Uint32 ref = get_free(force);

  if( !Pip::load(szLoadName,_list[ref]) )
  {
    return_one(ref);
    ref = Pip_List::INVALID;
  };

  return ref;
};


Cap & Particle::getCap()        { return CapList[ProfileList[model].cap_ref]; }
Mad & Particle::getMad()        { return MadList[ProfileList[model].mad_ref]; }
Eve & Particle::getEve()        { return EveList[ProfileList[model].eve_ref]; }
Script_Info & Particle::getAI() { return ScrList[ProfileList[model].scr_ref];  }
Pip & Particle::getPip(int i)   { return PipList[ProfileList[model].prtpip[i]]; }
Pip & Particle::getPip()        { return PipList[pip];                        };


void Pip_List::reset() { ProfileList.release_local_pips();  _setup(); }
