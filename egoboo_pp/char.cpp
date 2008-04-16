// char.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Character.h"
#include "Particle.h"
#include "Input.h"
#include "Network.h"
#include "Mad.h"
#include "Camera.h"
#include "Passage.h"
#include "Enchant.h"
#include "Profile.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define IS_WALKING(XX) (((XX)>=ACTION_WA) && ((XX)<=ACTION_WD))
#define IS_DANCING(XX) (((XX)>=ACTION_DA) && ((XX)<=ACTION_DD))

//--------------------------------------------------------------------------------------------

Physics_Info   GPhys;
Cap_List       CapList;
Character_List ChrList;
Team_List      TeamList;

//--------------------------------------------------------------------------------------------
Uint32 Pack::stack_item(Uint32 item)
{
  // ZZ> This function looks in the character's pack for an item similar
  //     to the one given.  If it finds one, it returns the similar item's
  //     index number, otherwise it returns Character_List::INVALID.
  Uint16 is_inpack, id;
  bool allok;

  if (!ChrList[item].getCap().isstackable) return Character_List::INVALID;

  Character & ritem = ChrList[item];

  for(is_inpack=nextinpack; VALID_CHR(is_inpack); is_inpack=ChrList[is_inpack].nextinpack )
  {
    Character & rpack_item = ChrList[is_inpack];

    // require the same models
    if (rpack_item.model != ritem.model) continue;

    // make sure that it is stackable
    if (!rpack_item.getCap().isstackable) continue;

    // make sure they have the same max ammo
    if (rpack_item.ammomax!=ritem.ammomax) continue;

    // make sure all IDSZs match
    allok = true;
    for (id = 0; id < IDSZ_COUNT && allok; id++)
    {
      allok = (rpack_item.getCap().idsz[id] == ritem.getCap().idsz[id]);
    }
      
    if(allok) return is_inpack;
  };

  return Character_List::INVALID;
}

//--------------------------------------------------------------------------------------------
bool Pack::add_item(Uint32 item)
{
  // ZZ> This function puts one character inside the other's pack
  Uint16 oldfirstitem, newammo, stack;

  // Make sure everything is hunkydori
  if ( INVALID_CHR(item) || ChrList[item].is_inpack ) return false;

  stack = stack_item(item);
  if (VALID_CHR(stack))
  {
    // We found a similar, stackable item in the pack
    if (ChrList[item].nameknown || ChrList[stack].nameknown)
    {
      ChrList[item].nameknown = true;
      ChrList[stack].nameknown = true;
    }

    if (ChrList[item].getCap().usageknown || ChrList[stack].getCap().usageknown)
    {
      ChrList[item].getCap().usageknown = true;
      ChrList[stack].getCap().usageknown = true;
    }

    newammo = ChrList[item].ammo + ChrList[stack].ammo;
    if (newammo <= ChrList[stack].ammomax)
    {
      // All transfered, so kill the in hand item
      ChrList[stack].ammo = newammo;
      if (VALID_CHR(ChrList[item].held_by))
      {
        detach_character_from_mount(item, true, false);
      }
      free_one_character(item);

      return true;
    }

    // Only some were transfered. Fall through to the next section
    ChrList[item].ammo  = newammo - ChrList[stack].ammomax;
    ChrList[stack].ammo = ChrList[stack].ammomax;
  };

  // Make sure we have room for another item
  if (numinpack >= MAXNUMINPACK)
  {
    return false;
  }

  // Take the item out of hand
  if (VALID_CHR(ChrList[item].held_by))
  {
    detach_character_from_mount(item, true, false);
    ChrList[item].ai.alert &= (~ALERT_IF_DROPPED);
  }

  // Remove the item from play
  ChrList[item].hitready = false;
  ChrList[item].is_inpack   = true;

  // Insert the item into the pack as the first one
  oldfirstitem = nextinpack;
  nextinpack   = item;
  ChrList[item].nextinpack = oldfirstitem;
  numinpack++;

  if (ChrList[item].getCap().is_equipment)
  {
    // AtLastWaypoint doubles as PutAway
    ChrList[item].ai.alert |= ALERT_IF_ATLASTWAYPOINT;
  }

  return true;
}

//--------------------------------------------------------------------------------------------
Uint32 Pack::get_item(bool ignorekurse)
{
  // ZZ> This function takes the last item in the character's pack and puts
  //     it into the designated hand.  It returns the item number or CHR_COUNT.
  Uint16 item, nexttolastitem;

  // Make sure everything is hunkydori
  if ( INVALID_CHR(nextinpack) || numinpack == 0)
    return Character_List::INVALID;


  // Find the last item in the pack
  nexttolastitem = Character_List::INVALID;
  for(item = nextinpack; VALID_CHR(item) && VALID_CHR(ChrList[item].nextinpack); item=ChrList[item].nextinpack)
  {
    nexttolastitem = item;
  };

  // Figure out what to do with it
  if (ChrList[item].iskursed && ChrList[item].is_equipped && !ignorekurse)
  {
    //The item is Kursed and we can't remove it

    // Flag the last item as not removed
    ChrList[item].ai.alert |= ALERT_IF_NOTPUTAWAY;  // Doubles as IfNotTakenOut

    if(Character_List::INVALID != nexttolastitem)
    {
      // Cycle it to the front
      ChrList[item].nextinpack = nextinpack;
      ChrList[nexttolastitem].nextinpack = Character_List::INVALID;
      nextinpack = item;
    };

    return Character_List::INVALID;
  }
  else
  {
    // We can remove the item

    // Remove the last item from the pack
    if(Character_List::INVALID == nexttolastitem)
    {
      nextinpack = Character_List::INVALID;
    }
    else
    {
      ChrList[nexttolastitem].nextinpack = Character_List::INVALID;
    };

    numinpack--;

    // reset some item attributes
    ChrList[item].is_inpack   = false;
    ChrList[item].is_equipped = false;
    ChrList[item].nextinpack  = Character_List::INVALID;
  }
  return item;
};

//--------------------------------------------------------------------------------------------
Uint32 get_item_from_character_pack(Uint16 character, Uint16 grip, bool ignore_kurse )
{
  if(INVALID_CHR(character) || ChrList[character].is_inpack)
    return Character_List::INVALID;

  Character & rchr = ChrList[character];

  Uint32 item = rchr.get_item( ignore_kurse );

  if(Character_List::INVALID != item)
  {
    // Attach the item to the character's hand
    attach_character_to_mount(item, character, grip);
    ChrList[item].ai.alert &= (~ALERT_IF_GRABBED);
    ChrList[item].ai.alert |= (ALERT_IF_TAKENOUT);
    ChrList[item].team = rchr.team;
  };

  return item;
};


//--------------------------------------------------------------------------------------------
void add_item_to_character_pack(Uint32 item, Uint32 character)
{
  if(INVALID_CHR(character) || ChrList[character].is_inpack)
    return;

  Character & rchr = ChrList[character];

  //try to add the item
  if(!rchr.add_item(item) && VALID_CHR(item))
  {
    //if it fails with a valid item, too much bagage
    rchr.ai.alert |= ALERT_IF_TOOMUCHBAGGAGE;
  };

};

//--------------------------------------------------------------------------------------------
void flash_character_height(int character, Uint8 valuelow, Sint16 low,
                            Uint8 valuehigh, Sint16 high)
{
  // ZZ> This function sets a character's lighting depending on vertex height...
  //     Can make feet dark and head light...
  int   cnt;
  float z;

  JF::MD2_Frame * pframe = (JF::MD2_Frame *)ChrList[character].getFrame();
  if(NULL==pframe) return;

  Character & rchr = ChrList[character];

  for (cnt=0; cnt<rchr.getMad().transvertices; cnt++)
  {
    z = pframe->vertices[cnt].z;
    if (z < low)
    {
      rchr.md2_blended.Ambient[cnt] = valuelow;
    }
    else
    {
      if (z > high)
      {
        rchr.md2_blended.Ambient[cnt] = valuehigh;
      }
      else
      {
        rchr.md2_blended.Ambient[cnt] = (valuehigh * (z - low) / (high - low)) +
                                        (valuelow * (high - z) / (high - low));
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void flash_character(int character, Uint8 value)
{
  // ZZ> This function sets a character's lighting
  int cnt;

  for(cnt = 0; cnt < ChrList[character].getMad().transvertices; cnt++)
  {
    ChrList[character].md2_blended.Ambient[cnt] = value;
  }
}

//--------------------------------------------------------------------------------------------
void flash_select()
{
  // ZZ> This function makes the selected characters blink
  int cnt;
  Uint8 value;

  if ((wldframe&0x1F)==0 && !allselect)
  {
    value = ((wldframe&0x20)<<3) - ((wldframe&0x20)>>5);
    cnt = 0;
    while (cnt < GRTS.select_count)
    {
      flash_character(GRTS.select[cnt], value);
      cnt++;
    }
  }
}


//--------------------------------------------------------------------------------------------
//void keep_weapons_with_holders()
//{
//  // ZZ> This function keeps weapons near their holders
//  int cnt, character;
//
//  // !!!BAD!!!  May need to do 3 levels of attachment...
//  SCAN_CHR_BEGIN(cnt, rchr_cnt)
//  {
//    character = rchr_cnt.held_by;
//    if (INVALID_CHR(character))
//    {
//      // Keep inventory with character
//      if (!rchr_cnt.is_inpack)
//      {
//        SCAN_CHR_PACK_BEGIN(rchr_cnt, character, ritem)
//        {
//          ritem.pos= rchr_cnt.pos;
//
//          // Copy olds to make SendMessageNear work
//          ritem.old.pos = rchr_cnt.pos;
//        } SCAN_CHR_PACK_END;
//
//      }
//    }
//    else
//    {
//      // Keep in hand weapons with character
//      if (ChrList[character].matrix_valid && rchr_cnt.matrix_valid)
//      {
//        rchr_cnt.pos.x = rchr_cnt.matrix.CNV(3,0);
//        rchr_cnt.pos.y = rchr_cnt.matrix.CNV(3,1);
//        rchr_cnt.pos.z = rchr_cnt.matrix.CNV(3,2);
//      }
//      else
//      {
//        rchr_cnt.pos = ChrList[character].pos;
//      }
//
//      rchr_cnt.turn_lr = ChrList[character].turn_lr;
//      // Copy this stuff ONLY if it's a weapon, not for mounts
//      if (ChrList[character].transferblend && rchr_cnt.is_item)
//      {
//        if (ChrList[character].alpha!=0xFF)
//        {
//          rchr_cnt.alpha = ChrList[character].alpha;
//        }
//        if (ChrList[character].light!=0xFF)
//        {
//          rchr_cnt.light = ChrList[character].light;
//        }
//      }
//    }
//  } SCAN_CHR_END;
//
//}

//--------------------------------------------------------------------------------------------
//void make_turntosin(void)
//{
//  // ZZ> This function makes the lookup table for chrturn...
//  int cnt;
//
//  cnt = 0;
//  while (cnt < 0x4000)
//  {
//    turntosin[cnt] = sin((float)(2*PI*cnt/float(0x4000)));
//    cnt++;
//  }
//}

//--------------------------------------------------------------------------------------------
bool make_one_character_matrix(Character & rchr)
{
  // ZZ> This function sets one character's matrix

  if (rchr.overlay && VALID_CHR(rchr.ai.target))
  {
    Character & rtarg = ChrList[rchr.ai.target];
    if(!rtarg.matrix_valid) return false;

    // Overlays are kept with their target...
    rchr.pos    = rtarg.pos;
    rchr.matrix = rtarg.matrix;
  }
  else
  {
    bool lean = INVALID_CHR(rchr.held_by) && !rchr.stickybutt;
    rchr.matrix = CreateOrientationMatrix(rchr.scale*rchr.scale_vert, rchr.scale*rchr.scale_horiz, rchr.getOrientation(), lean);
  }

  rchr.matrix_valid = true;
  return true;
}

//--------------------------------------------------------------------------------------------
void free_one_character(int character)
{
  // ZZ> This function sticks a character back on the free character stack
  int cnt;


  // Remove from stat list
  if (ChrList[character].staton)
  {
    ChrList[character].staton = false;
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
  if (ChrList[character].alive && !ChrList[character].getCap().invictus)
  {
    TeamList[ChrList[character].baseteam].morale--;
  }

  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if (rchr_cnt.ai.target == character)
    {
      rchr_cnt.ai.alert|=ALERT_IF_TARGETKILLED;
      rchr_cnt.ai.target = cnt;
    }
    if (TeamList[rchr_cnt.team].leader == character)
    {
      rchr_cnt.ai.alert|=ALERT_IF_LEADERKILLED;
    }
  } SCAN_CHR_END;

  if (TeamList[ChrList[character].team].leader==character)
  {
    TeamList[ChrList[character].team].leader = NOLEADER;
  }

  ChrList.return_one(character);
}

//--------------------------------------------------------------------------------------------
void free_inventory(int character)
{
  // ZZ> This function frees every item in the character's inventory
  int cnt, next;

  cnt = ChrList[character].nextinpack;
  while (_VALID_CHR_RANGE(cnt))
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
  Uint16 vertex;

  // Check validity of attachment
  if (INVALID_CHR(character) || ChrList[character].is_inpack)
  {
    PrtList[particle].requestDestroy();
    return;
  }

  Character & rchr = ChrList[character];
  Particle  & rprt = PrtList[particle];

  rprt.attachedtocharacter = character;

  // Do we have a matrix???
  if (rchr.matrix_valid)
  {

    if (grip == SPAWNORIGIN)
    {
      rprt.pos.x = rchr.matrix.CNV(3,0);
      rprt.pos.y = rchr.matrix.CNV(3,1);
      rprt.pos.z = rchr.matrix.CNV(3,2);
      return;
    }

    // Transform the weapon grip from model to world space
    vertex = rchr.getMad().numVertices() - grip;

    // Do the transform
    rprt.pos = matrix_mult(rchr.matrix, rchr.md2_blended.Vertices[vertex]);
  }
  else
  {
    // No matrix, so just wing it...
    rprt.pos = rchr.pos;
  }
}

//--------------------------------------------------------------------------------------------
bool make_one_matrix(Uint16 item_idx)
{
  // ZZ> This function sets one weapon's matrix, based on who it's attached to

  GLMatrix grip_mat;
  vec3_t   points[GRIP_POINTS];

  if(INVALID_CHR(item_idx)) return false;
  Character & ritem = ChrList[item_idx];
  ritem.matrix_valid = false;

  if(INVALID_CHR(ritem.held_by))
    return make_one_character_matrix(ritem);

  Character & rmount    = ChrList[ritem.held_by];
  if(!rmount.matrix_valid) return false;
  GLMatrix  & mount_mat = rmount.matrix;

  // Transform the weapon grip from model to world space
  Uint32 gripvert = rmount.getMad().numVertices() - ritem.gripoffset;
  blend_md2_vertices(rmount, gripvert, gripvert+GRIP_POINTS);

  for(int i=0; i<GRIP_POINTS; i++)
  {
    points[i] = rmount.md2_blended.Vertices[gripvert+i];
  }

  // Calculate weapon's matrix based on positions of grip points
  // chrscale is recomputed at time of attachment
  grip_mat = FourPoints(points[0], points[1], points[2], points[3], 1);

  ritem.matrix = MatrixMult(mount_mat, grip_mat);
  ritem.matrix_valid = true;

  return true;
}

//--------------------------------------------------------------------------------------------
void propagate_forces(Character & item, Physics_Accumulator & accum)
{
  if(VALID_CHR(item.held_by))
  {
    // keep passing the force on until we hit an un-mounted item
    Character & rmount = ChrList[item.held_by];
    propagate_forces(rmount, accum);
  }
  else
  {
    // add out forces to our mount
    if(accum.weight==item.weight)
    {
      // both have same mass, transfer directly
      item.accumulate_acc( accum.aacc, accum.aacc_lr );
      item.accumulate_vel( accum.avel, accum.avel_lr );
      item.accumulate_pos( accum.apos, accum.apos_lr );
    }
    else if( item.weight<0 || accum.weight==0)
    {
      // the mount is immovable or item is weightless
      // do not transfer velocities/accelerations
      item.accumulate_pos( accum.apos, accum.apos_lr );
    }
    else if (accum.weight<0)
    {
      // held item is immovable. overwrite changes?
      item.aacc    = accum.aacc;
      item.aacc_lr = accum.aacc_lr;

      item.avel    = accum.avel;
      item.avel_lr = accum.avel_lr;

      item.accumulate_pos( accum.apos, accum.apos_lr );
    }
    else
    {
      // transfer accelerations by mass ratio
      float ratio = float(accum.weight) / float(item.weight);
      item.accumulate_acc( accum.aacc*ratio, accum.aacc_lr*ratio );
      item.accumulate_vel( accum.avel*ratio, accum.avel_lr*ratio );
      item.accumulate_pos( accum.apos*ratio, accum.apos_lr*ratio );
    };
  }

};

//--------------------------------------------------------------------------------------------
void make_character_matrices()
{
  // ZZ> This function makes all of the character's matrices
  int cnt, tnc;

  // Forget about old matrices
  SCAN_CHR_BEGIN(cnt, rscan_chr)
  {
    rscan_chr.matrix_valid = false;
  } SCAN_CHR_END;

  bool finished = false;
  while(!finished)
  {
    bool found = false;

    // Do all characters, starting with base characters and moving up the chain
    SCAN_CHR_BEGIN(tnc, rchr_tnc)
    {
      if(rchr_tnc.matrix_valid) continue;
      if(rchr_tnc.held_by == tnc)
      {
        rchr_tnc.held_by = Character_List::INVALID;
        continue;
      }

      bool tmp = make_one_matrix(tnc);
      found = found || tmp;

    } SCAN_CHR_END;

    finished = !found;
  };

}

//--------------------------------------------------------------------------------------------
bool find_target_in_block(Search_Params &params, int x, int y, vec3_t &pos, Uint16 facing,
                          bool onlyfriends, bool anyone, Uint8 team,
                          Uint16 donttarget, Uint16 oldtarget)
{
  // ZZ> This function helps find a target, returning true if it found a decent target
  int cnt;
  Uint16 angle;
  Uint16 charb;
  Uint32 fanblock;
  int distance;

  fanblock = GMesh.getFanblockBlock(x,y);
  if(JF::MPD_IndexInvalid == fanblock) return false;

  bool returncode = false;
  bool enemies = !onlyfriends;

  Uint32 chars = BumpList[fanblock].chr_count;
  charb = BumpList[fanblock].chr;
  for ( cnt = 0; cnt<chars && VALID_CHR(charb); cnt++, charb = ChrList[charb].bump_next)
  {
    Character & rchr = ChrList[charb];

    if (rchr.alive && !rchr.invictus && charb != donttarget && charb != oldtarget)
    {
      if (anyone || (rchr.team==team && onlyfriends) || (TeamList[team].hatesteam[rchr.team] && enemies))
      {
        distance = diff_abs_horiz(rchr.pos, pos);
        if (distance < params.best_distance)
        {
          angle = 32786 + facing - diff_to_turn(rchr.pos, pos);
          if (angle < params.stt_angle || angle > (0xFFFF-params.stt_angle))
          {
            returncode = true;
            params.stt_target = charb;
            params.best_distance = distance;
            params.use_angle = angle;
            if (angle  > 0x7FFF)
              params.stt_angle = -angle;
            else
              params.stt_angle = angle;
          }
        }
      }
    }
  }

  return returncode;
}

//--------------------------------------------------------------------------------------------
Uint16 find_target(Search_Params &params, vec3_t &pos, Uint16 facing,
                   Uint16 targetangle, bool onlyfriends, bool anyone,
                   Uint8 team, Uint16 donttarget, Uint16 oldtarget)
{
  // This function finds the best target for the given parameters
  bool done;
  int x, y;

  x = int(pos.x) >> Mesh::Block_bits;
  y = int(pos.y) >> Mesh::Block_bits;

  params.best_distance = 9999;
  params.stt_angle = targetangle;

  done =         find_target_in_block(params,x, y, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  if (done) return params.stt_target;

  done =         find_target_in_block(params, x+1, y, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done = done || find_target_in_block(params, x-1, y, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done = done || find_target_in_block(params, x, y+1, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done = done || find_target_in_block(params, x, y-1, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  if (done) return params.stt_target;

  done =         find_target_in_block(params, x+1, y+1, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done = done || find_target_in_block(params, x+1, y-1, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done = done || find_target_in_block(params, x-1, y+1, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  done = done || find_target_in_block(params, x-1, y-1, pos, facing, onlyfriends, anyone, team, donttarget, oldtarget);
  if (done) return params.stt_target;

  return Character_List::INVALID;
}

//--------------------------------------------------------------------------------------------
void Character::reset()
{
  memset(this,0,sizeof(Character));
  nextinpack = Character_List::INVALID;
  begin_integration();
};


//--------------------------------------------------------------------------------------------
void Character_List::free_all()
{
  // ZZ> This function resets the character allocation list

  nolocalplayers = true;
  PlaList.count_total = 0;
  PlaList.count_local = 0;
  numstat = 0;

  //reset the ChrList
  for (int i=0; i<SIZE; i++)
  {
    _list[i]._on = false;
  }

  _setup();
}

//--------------------------------------------------------------------------------------------
void reset_character_accel(Uint16 character)
{
  // ZZ> This function fixes a character's max acceleration
  Uint16 enchant;

  if (VALID_CHR(character))
  {
    // Okay, remove all acceleration enchants
    SCAN_ENC_BEGIN(ChrList[character], enchant, renc)
    {
      remove_enchant_value(enchant, ADDACCEL);
    } SCAN_ENC_END;

    // Set the starting value
    ChrList[character].maxaccel = ChrList[character].getCap().maxaccel[ChrList[character].skin];

    // Put the acceleration enchants back on
    SCAN_ENC_BEGIN(ChrList[character], enchant, renc)
    {
      add_enchant_value(enchant, ADDACCEL, ProfileList[EncList[enchant].eve_prof].eve_ref);
    } SCAN_ENC_END;

  }
}

//--------------------------------------------------------------------------------------------
void detach_character_from_mount(Uint16 character, bool ignorekurse, bool doshop)
{
  // ZZ> This function drops an item
  Uint16 mount, slot, enchant, passage, owner, price;
  Uint8 inshop;


  // Make sure the character is valid
  if (INVALID_CHR(character)) return;
  Character & rchr = ChrList[character];

  // Make sure the character is mounted
  mount = rchr.held_by;
  if (INVALID_CHR(mount)) return;
  Character & rmount = ChrList[mount];

  // Don't allow living characters to drop kursed weapons
  if (!ignorekurse && rchr.iskursed && rmount.alive && rchr.is_item)
  {
    rchr.ai.alert |= ALERT_IF_NOTDROPPED;
    return;
  }

  // Figure out which slot it's in
  if(0 != rchr.gripoffset % GRIP_POINTS) return;
  slot = (rchr.gripoffset / GRIP_POINTS)-1;

  // Rip 'em apart
  rchr.held_by = Character_List::INVALID;

  if (rmount.holding_which[SLOT_LEFT] == character)
    rmount.holding_which[SLOT_LEFT] = Character_List::INVALID;

  if (rmount.holding_which[SLOT_RIGHT] == character)
    rmount.holding_which[SLOT_RIGHT] = Character_List::INVALID;

  // Run the falling animation...
  play_action(character, (ACTION_TYPE)(ACTION_JB+slot), false);

  // Set the positions
  if (rchr.matrix_valid)
  {
    rchr.pos.x = rchr.matrix.CNV(3,0);
    rchr.pos.y = rchr.matrix.CNV(3,1);
    rchr.pos.z = rchr.matrix.CNV(3,2);
  }
  else
  {
    rchr.pos = rmount.pos;
  }

  // Make sure it's not dropped in a wall...
  if (rchr.inawall(GMesh))
  {
    rchr.pos.x = rmount.pos.x;
    rchr.pos.y = rmount.pos.y;
  }

  // Check for shop passages
  inshop = false;
  if (rchr.is_item && doshop)
  {
    Uint32 which_shop = ShopList.inside(rchr.pos);

    if (Shop_List::INVALID != which_shop)
    {
      owner   = ShopList[which_shop].owner;
      passage = ShopList[which_shop].passage;

      // Give the mount its money back, ai.alert the shop owner
      price = rchr.getCap().skincost[0];
      if (rchr.getCap().isstackable)
      {
        price = price * rchr.ammo;
      }
      rmount.money += price;
      ChrList[owner].money -= price;
      if (ChrList[owner].money < 0)  ChrList[owner].money = 0;
      if (rmount.money > MAXMONEY)  rmount.money = MAXMONEY;
      ChrList[owner].ai.alert |= ALERT_IF_ORDERED;
      ChrList[owner].order = price;  // Tell owner how much...
      ChrList[owner].counter = 0;  // 0 for buying an item
    }
  }

  // Make sure it works right
  rchr.hitready=true;
  if (inshop)
  {
    // Drop straight down to avoid theft
    rchr.accumulate_vel( vec3_t(-rchr.vel.x, -rchr.vel.y, DROPZ_VELOCITY) );
  }
  else
  {
    rchr.accumulate_vel_z( DROPZ_VELOCITY );
  }

  // Turn looping off
  rchr.act.loop=false;

  // Reset the team if it is a mount
  if (rmount.is_mount)
  {
    rmount.team = rmount.baseteam;
    rmount.ai.alert |= ALERT_IF_DROPPED;
  }
  rchr.team = rchr.baseteam;
  rchr.ai.alert |= ALERT_IF_DROPPED;

  // Reset transparency
  if (rchr.is_item && rmount.transferblend)
  {
    // Okay, reset transparency
    SCAN_ENC_BEGIN(rchr, enchant, renc)
    {
      unset_enchant_value(enchant, SETALPHABLEND);
      unset_enchant_value(enchant, SETLIGHTBLEND);
    } SCAN_ENC_END;

    rchr.alpha = rchr.getCap().alpha;
    rchr.light = rchr.getCap().light;

    SCAN_ENC_BEGIN(rchr, enchant, renc)
    {
      set_enchant_value(enchant, SETALPHABLEND, renc.eve_prof);
      set_enchant_value(enchant, SETLIGHTBLEND, renc.eve_prof);
    } SCAN_ENC_END;
  }

  // Set twist
  rchr.up = rmount.up;
}

//--------------------------------------------------------------------------------------------
void attach_character_to_mount(Uint16 character, Uint16 mount, Uint16 grip)
{
  // ZZ> This function attaches one character to another ( the mount )
  //     at either the left or right grip
  int slot;

  // Make sure both are still around
  if ( INVALID_CHR(character) || INVALID_CHR(mount))
    return;

  Character & rchr   = ChrList[character];
  Character & rmount = ChrList[mount];

  if(rchr.is_inpack || rmount.is_inpack) return;

  // Figure out which slot this grip relates to
  if(0 != grip % GRIP_POINTS)  return;
  slot = (grip/GRIP_POINTS) - 1;

  // Make sure the the slot is valid
  if (!rmount.getCap().slot_valid[slot]) return;

  // Put 'em together
  rchr.gripoffset = grip;
  rchr.held_by    = mount;
  rmount.holding_which[slot] = character;
  rchr.inwater   = false;
  rchr.jumptime  = DELAY_JUMP*4;
  rchr.jumpready = false;

  // Run the held animation
  if (rmount.is_mount && grip == GRIP_SADDLE)
  {
    // Riding mount
    play_action(character, ACTION_MI, true);
    rchr.act.loop=true;
  }
  else
  {
    play_action(character, (ACTION_TYPE)(ACTION_MM+slot), false);
    rchr.act.keep = rchr.is_item; // Item grab ?
  }

  // Set the team
  if (rchr.is_item)
  {
    rchr.team=rmount.team;
    // Set the ai.alert
    rchr.ai.alert |= ALERT_IF_GRABBED;
  }

  if (rmount.is_mount)
  {
    rmount.team=rchr.team;
    // Set the ai.alert
    if (!rmount.is_item)
    {
      rmount.ai.alert |= ALERT_IF_GRABBED;
    }
  }

  // It's not gonna hit the floor
  rchr.hitready=false;
};

//--------------------------------------------------------------------------------------------
Uint32 Pack::get_idsz(IDSZ minval, IDSZ maxval)
{
  Uint16 item, lastitem;

  lastitem = Character_List::INVALID;
  for(item = nextinpack; VALID_CHR(item); item = ChrList[item].nextinpack)
  {
    Character & rinv_item = ChrList[item];

    if ((rinv_item.getCap().idsz[IDSZ_PARENT] >= minval && 
         rinv_item.getCap().idsz[IDSZ_PARENT] <= maxval) || 
        (rinv_item.getCap().idsz[IDSZ_TYPE]   >= minval && 
         rinv_item.getCap().idsz[IDSZ_TYPE]   <= maxval))
    {
      // We found a key...
      //remove the item from the list
      ChrList[lastitem].nextinpack = rinv_item.nextinpack;
      numinpack--;

      rinv_item.is_inpack     = false;
      rinv_item.nextinpack = Character_List::INVALID;
      rinv_item.held_by    = Character_List::INVALID;

      return lastitem;
    }

    lastitem = item;
  };

  return Character_List::INVALID;
};

//--------------------------------------------------------------------------------------------
void drop_keys(Uint32 character)
{
  // ZZ> This function drops all keys ( [KEYA] to [KEYZ] ) that are in a character's
  //     inventory ( Not hands ).

  if(INVALID_CHR(character)) return;
  if (ChrList[character].pos.z <= -2) return; // Don't lose keys in pits...

  Character & rchr = ChrList[character];

  // The IDSZs to find
  IDSZ testa = IDSZ("KEYA");  // [KEYA]
  IDSZ testz = IDSZ("KEYZ");  // [KEYZ]

  Uint32 item = rchr.get_idsz(testa, testz);
  for( /* nothing */; VALID_CHR(item); item = rchr.get_idsz(testa, testz))
  {
    Character & ritem = ChrList[item];

    ritem.is_equipped = false;
    ritem.ai.alert  |= ALERT_IF_DROPPED;
    ritem.hitready   = true;

    // shoot it out
    Uint16 direction = RANDIE;
    ritem.turn_lr = direction;
    ritem.level = ChrList[character].level;

    ritem.pos = ChrList[character].pos;

    ritem.accumulate_vel( vec3_t(cos_tab[direction>>2]*DROPXY_VELOCITY, sin_tab[direction>>2]*DROPXY_VELOCITY, DROPZ_VELOCITY) );
    ritem.team = ritem.baseteam;
  };
};



//--------------------------------------------------------------------------------------------
void drop_all_items(Uint16 character)
{
  // ZZ> This function drops all of a character's items
  Uint16 item, direction, diradd;

  if (INVALID_CHR(character)) return;
  Character & rchr = ChrList[character];


  detach_character_from_mount(rchr.holding_which[SLOT_LEFT], true, false);
  detach_character_from_mount(rchr.holding_which[SLOT_RIGHT], true, false);
  if (rchr.numinpack <= 0) return;


  direction = rchr.turn_lr;
  diradd = 0xFFFF / rchr.numinpack;
  while (rchr.numinpack > 0)
  {
    item = get_item_from_character_pack(character, GRIP_LEFT, false);
    if (VALID_CHR(item))
    {
      Character & ritem = ChrList[item];
      detach_character_from_mount(item, true, true);
      ritem.hitready=true;
      ritem.ai.alert |= ALERT_IF_DROPPED;

      ritem.getOrientation() = rchr.getOrientation();
      ritem.level   = rchr.level;
      ritem.turn_lr = direction;

      ritem.accumulate_vel( vec3_t(cos_tab[direction>>2]*DROPXY_VELOCITY, sin_tab[direction>>2]*DROPXY_VELOCITY, 0) );
      ritem.team = ritem.baseteam;
    }
    direction += diradd;
  }


}

//--------------------------------------------------------------------------------------------
void character_grab_stuff(int chara, int grip, Uint8 people)
{
  // ZZ> This function makes the character pick up an item if there's one around
  vec3_t posa, posb;
  float dist;
  int charb, slot;
  Uint16 vertex, model, frame, passage, cnt, price;
  Uint8 inshop;

  Uint32 owner = Character_List::INVALID;


  Character & rchra = ChrList[chara];

  // Make life easier
  model = rchra.model;
  if(0 != (grip % GRIP_POINTS)) return;
  slot = (grip/GRIP_POINTS) - 1;         // 0 is left, 1 is right

  // Make sure the character doesn't have something already, and that it has hands
  if (VALID_CHR(rchra.holding_which[slot]) || !rchra.getCap().slot_valid[slot])
    return;

  // Do we have a matrix???
  if (rchra.matrix_valid)//GMesh.fan_info[rchra.onwhichfan].inrenderlist)
  {
    // Transform the weapon grip from model to world space
    frame = rchra.ani.frame;
    vertex = rchra.getMad().numVertices() - grip;

    // Do the transform
    posa = matrix_mult(rchra.matrix, rchra.md2_blended.Vertices[vertex]);
  }
  else
  {
    // Just wing it
    posa = rchra.pos;
  }

  // Go through all characters to find the best match
  SCAN_CHR_BEGIN(charb, rchr_charb)
  {
    if ( (!rchr_charb.is_inpack) && rchr_charb.weight<rchra.weight && rchr_charb.alive && INVALID_CHR(rchr_charb.held_by) && ((!people && rchr_charb.is_item) || (people && !rchr_charb.is_item)) )
    {
      posb = rchr_charb.pos;

      // First check absolute value diamond
      posb.x = ABS(posa.x-posb.x);
      posb.y = ABS(posa.y-posb.y);
      posb.z = ABS(posa.z-posb.z);
      dist = posb.x + posb.y + posb.z;
      if (dist < GRABSIZE && posb.z < GRABSIZE)
      {
        // Don't grab your mount
        if (rchr_charb.holding_which[SLOT_LEFT]!=chara && rchr_charb.holding_which[SLOT_RIGHT]!=chara)
        {
          // Check for shop
          inshop = false;
          if (rchr_charb.is_item)
          {
            for (cnt = 0; cnt < Shop_List::SIZE; cnt++)
            {
              if(!ShopList[cnt].allocated()) continue;

              passage = ShopList[cnt].passage;
              if(PassList[passage].inside(rchr_charb.pos))
              {
                inshop = true;
                owner = ShopList[passage].owner;
                if (owner == Shop_List::INVALID)
                {
                  // The owner has died!!!
                  inshop = false;
                }
                break;
              }
            }

            if (inshop)
            {
              // Pay the shop owner, or don't allow grab...
              if (rchra.is_item)
              {
                // Pets can shop for free =]
                inshop = false;
              }
              else if(VALID_CHR(owner))
              {
                ChrList[owner].ai.alert |= ALERT_IF_ORDERED;
                price = rchr_charb.getCap().skincost[0];
                if (rchr_charb.getCap().isstackable)
                {
                  price = price * rchr_charb.ammo;
                }
                ChrList[owner].order = price;  // Tell owner how much...
                if (rchra.money >= price)
                {
                  // Okay to buy
                  ChrList[owner].counter = 1;  // 1 for selling an item
                  rchra.money -= price;  // Skin 0 cost is price
                  ChrList[owner].money += price;
                  if (ChrList[owner].money > MAXMONEY)  ChrList[owner].money = MAXMONEY;
                  inshop = false;
                }
                else
                {
                  // Don't allow purchase
                  ChrList[owner].counter = 2;  // 2 for "you can't afford that"
                  inshop = true;
                }
              }
            }
          }

          if (!inshop)
          {
            // Stick 'em together and quit
            attach_character_to_mount(charb, chara, grip);
            charb=Character_List::INVALID;
            if (people)
            {
              // Do a slam animation...  ( Be sure to drop!!! )
              play_action(chara, (ACTION_TYPE)(ACTION_MC+slot), false);
            }
          }
          else
          {
            // Lift the item a little and quit...
            rchr_charb.accumulate_vel_z(DROPZ_VELOCITY);
            rchr_charb.hitready=true;
            rchr_charb.ai.alert |= ALERT_IF_DROPPED;
            charb=Character_List::INVALID;
          }
        }
      }
    }
  } SCAN_CHR_END;
}

//--------------------------------------------------------------------------------------------
void character_swipe(Uint16 character, Uint8 slot)
{
  // ZZ> This function spawns an attack particle
  int weapon, particle, spawngrip, thrown;
  Uint8 action;
  Uint16 tTmp;
  float dampen;
  float velocity;

  if(INVALID_CHR(character)) return;
  Character & rchr = ChrList[character];

  spawngrip = GRIP_LAST;
  action    = rchr.act.which;

  // See if it's an unarmed attack...
  weapon = rchr.holding_which[slot];
  if (INVALID_CHR(weapon))
  {
    weapon    = character;
    spawngrip = 4+(slot<<2);  // 0 = GRIP_LEFT, 1 = GRIP_RIGHT
  }

  Character & rweapon = ChrList[weapon]; 

  if (weapon!=character && ((rweapon.getCap().isstackable && rweapon.ammo>1) || (action >= ACTION_FA && action <= ACTION_FD)))
  {
    // Throw the weapon if it's stacked or a hurl animation

    velocity = rchr.strength/( CapList[ProfileList[rweapon.model].cap_ref].weight*THROWFIX);
    velocity += MINTHROW_VELOCITY;
    if (velocity > MAXTHROW_VELOCITY)
    {
      velocity = MAXTHROW_VELOCITY;
    }
    tTmp = rchr.turn_lr>>2;

    vec3_t loc_vel( cos_tab[tTmp]*velocity, sin_tab[tTmp]*velocity, DROPZ_VELOCITY );

    thrown = spawn_one_character(rchr.pos, rchr.turn_lr,  rchr.vel + loc_vel, rchr.vel_lr, rweapon.model, rchr.team, 0, rweapon.name);
    if (VALID_CHR(thrown))
    {
      Character & rthrown = ChrList[thrown];

      rthrown.iskursed = false;
      rthrown.ammo = 1;
      rthrown.ai.alert |= ALERT_IF_THROWN;

      if (rweapon.ammo<=1)
      {
        // Poof the item
        detach_character_from_mount(weapon, true, false);
        free_one_character(weapon);
      }
      else
      {
        rweapon.ammo--;
      }
    }
  }
  else if (rweapon.ammomax==0 || rweapon.ammo!=0)
  {
    // Spawn an attack particle
    if (rweapon.ammo>0 && !rweapon.getCap().isstackable)
    {
      rweapon.ammo--;  // Ammo usage
    }

    if (rweapon.getCap().attackprttype!=-1)
    {
      particle = spawn_one_particle(rweapon.pos, rchr.turn_lr, rweapon.vel, rweapon.weight, rweapon.model, rweapon.getCap().attackprttype, weapon, spawngrip, rchr.team, character, 0, Character_List::INVALID);
      if (VALID_PRT(particle))
      {
        Particle & rprt = PrtList[particle];

        rprt.size  = MAX(19456,rprt.size);
        rprt.type  = SPRITE_ATTACK;

        rprt.begin_integration();

        if (rweapon.getCap().attackattached)
        {
          // Attached particles get a strength bonus for reeling...
          dampen = REELBASE+(rchr.strength/REEL);
        }
        else
        {
          // Detach the particle
          if (!PipList[rprt.pip].startontarget || INVALID_CHR(rprt.target))
          {
            // Correct Z spacing base, but nothing else...
            rprt.pos.z += PipList[rprt.pip].zspacingbase;
          }
          else
          {
            // Detach the particle
            rprt.attachedtocharacter = Character_List::INVALID;
          }

          // Don't spawn in walls
          if (Character_List::INVALID==rprt.attachedtocharacter && rprt.inawall(GMesh))
          {
            rprt.pos.x = rweapon.pos.x;
            rprt.pos.y = rweapon.pos.y;
            if ( rprt.inawall(GMesh) )
            {
              rprt.pos.x = rchr.pos.x;
              rprt.pos.y = rchr.pos.y;
            }
          }
        };

        // Initial particles get a strength bonus, which may be 0.00
        rprt.damagebase += (rchr.strength*rweapon.getCap().strengthdampen);

        // Initial particles get an enchantment bonus
        rprt.damagebase += rweapon.damageboost;

        // Initial particles inherit damage type of weapon
        rprt.damagetype = rweapon.damagetargettype;
      }
    }
    else
    {
      rweapon.ammoknown = true;
    }
  }
}

//--------------------------------------------------------------------------------------------
static void do_anim(Uint32 cnt, Physics_Environment & enviro, float dframe)
{
  if( INVALID_CHR(cnt) ) return;

  Character & rchr_cnt = ChrList[cnt];
  Mad       & rmad_cnt = rchr_cnt.getMad();

  rchr_cnt.ani.rate = 1.0;
  float speed = ABS(rchr_cnt.vel.x) + ABS(rchr_cnt.vel.y);

  // Calculate the correct frame rate and animation from the speed
  if( rchr_cnt.act.which >= ACTION_UA && rchr_cnt.act.which <= ACTION_BD)
  {
    // physical attacks are strength + dex dependent
    rchr_cnt.ani.rate = 0.25 + rchr_cnt.dexterity/float(PERFECTSTAT) + rchr_cnt.strength/float(PERFECTSTAT);
  }
  else if ( rchr_cnt.act.which >= ACTION_LA && rchr_cnt.act.which <= ACTION_XD)
  {
    // bow attacks are dex dependent
    rchr_cnt.ani.rate = 0.25 + 2.0*rchr_cnt.dexterity/float(PERFECTSTAT);
  }
  else if ( rchr_cnt.act.which >= ACTION_FA && rchr_cnt.act.which <= ACTION_FD)
  {
    // flinged attacks are strength + dex dependent
    rchr_cnt.ani.rate = 0.25 + rchr_cnt.dexterity/float(PERFECTSTAT) + rchr_cnt.strength/float(PERFECTSTAT);
  }
  else if(rchr_cnt.act.which >= ACTION_PA &&  rchr_cnt.act.which <= ACTION_PD)
  {
    // parry is dexterity
    rchr_cnt.ani.rate = 0.25 + 2*rchr_cnt.dexterity/float(PERFECTSTAT);
  }
  else if(rchr_cnt.act.which >= ACTION_EA &&  rchr_cnt.act.which <= ACTION_RA)
  {
    // evades are dexterity
    rchr_cnt.ani.rate = 0.25 + 2.0f*rchr_cnt.dexterity/float(PERFECTSTAT);
  }
  else if(rchr_cnt.act.which >= ACTION_ZA &&  rchr_cnt.act.which <= ACTION_ZD)
  {
    // zaps are int + wis
    rchr_cnt.ani.rate = 0.25 + rchr_cnt.intelligence/float(PERFECTSTAT) + rchr_cnt.wisdom/float(PERFECTSTAT);
  }
  else if (rchr_cnt.act.which == ACTION_JA)
  {
    // beginning jump is str + dex
    rchr_cnt.ani.rate = 0.25 + rchr_cnt.dexterity/float(PERFECTSTAT) + rchr_cnt.strength/float(PERFECTSTAT);
  }
  else if (rchr_cnt.act.which == ACTION_WA)
  {
    rchr_cnt.ani.rate = speed / rchr_cnt.spd_sneak;
  }
  else if (rchr_cnt.act.which == ACTION_WB)
  {
    rchr_cnt.ani.rate = speed / rchr_cnt.spd_walk;
  }
  else if (rchr_cnt.act.which == ACTION_WC)
  {
    rchr_cnt.ani.rate = speed / rchr_cnt.spd_run;
  }
  else if (rchr_cnt.act.which == ACTION_WD)
  {
    rchr_cnt.ani.rate = speed / rchr_cnt.spd_sneak;
  }

  // calculate the correct walking animation based on the speed
  ACTION_TYPE walk_action = rchr_cnt.act.which;
  if(speed == 0)
  {
    walk_action = ACTION_DA;
  }
  else if(speed < rchr_cnt.spd_sneak)
  {
    walk_action = ACTION_WA;
  }
  else if(speed < rchr_cnt.spd_walk)
  {
    walk_action = ACTION_WA;
  }
  else if(speed < rchr_cnt.spd_run)
  {
    walk_action = ACTION_WB;
  }
  else
  {
    walk_action = ACTION_WC;
  }


  // actually do the animation
  rchr_cnt.ani.flip += dframe*rchr_cnt.ani.rate*0.25;
  while(rchr_cnt.ani.flip > 0.25)
  {
    // Animate the character
    rchr_cnt.ani.lip  += 0x40;
    rchr_cnt.ani.flip -= 0.25;
    if (rchr_cnt.ani.lip == 192)
    {
      Frame_Extras & rframe_ex = rmad_cnt.getExtras(rchr_cnt.ani.frame);

      // Check frame effects
      if (rframe_ex.framefx&MADFX_ACTLEFT)
        character_swipe(cnt, 0);

      if (rframe_ex.framefx&MADFX_ACTRIGHT)
        character_swipe(cnt, 1);

      if (rframe_ex.framefx&MADFX_GRABLEFT)
        character_grab_stuff(cnt, GRIP_LEFT, false);

      if (rframe_ex.framefx&MADFX_GRABRIGHT)
        character_grab_stuff(cnt, GRIP_RIGHT, false);

      if (rframe_ex.framefx&MADFX_CHARLEFT)
        character_grab_stuff(cnt, GRIP_LEFT, true);

      if (rframe_ex.framefx&MADFX_CHARRIGHT)
        character_grab_stuff(cnt, GRIP_RIGHT, true);

      if (rframe_ex.framefx&MADFX_DROPLEFT)
        detach_character_from_mount(rchr_cnt.holding_which[SLOT_LEFT], false, true);

      if (rframe_ex.framefx&MADFX_DROPRIGHT)
        detach_character_from_mount(rchr_cnt.holding_which[SLOT_RIGHT], false, true);

      if (rframe_ex.framefx&MADFX_POOF && !rchr_cnt.isplayer)
        rchr_cnt.ai.gopoof = true;

      if (rframe_ex.framefx&MADFX_FOOTFALL)
      {
        if ( VALID_WAVE_RANGE(rchr_cnt.getCap().wavefootfall) )
        {
          play_sound(rchr_cnt.pos, ProfileList[rchr_cnt.model].waveindex[rchr_cnt.getCap().wavefootfall]);
        }
      }
    }

    if (rchr_cnt.ani.lip == 0)
    {
      // Change frames
      rchr_cnt.ani.last = rchr_cnt.ani.frame;
      rchr_cnt.ani.frame++;

      if (rchr_cnt.ani.frame == rmad_cnt.actinfo[rchr_cnt.act.which].end)
      {
        if (rchr_cnt.act.keep)
        {
          // Keep the last frame going
          rchr_cnt.ani.frame = rchr_cnt.ani.last;
        }
        else
        {
          if(!rchr_cnt.act.loop)
          {
            // Go on to the next action
            rchr_cnt.act.which = rchr_cnt.act.next;
            rchr_cnt.act.next  = ACTION_DA;
          }

          // mounted actions override all
          if ( VALID_CHR(rchr_cnt.held_by) )
          {
            rchr_cnt.act.which = ACTION_MI;
          }

          rchr_cnt.ani.frame = rmad_cnt.actinfo[rchr_cnt.act.which].start;
        }

        rchr_cnt.act.ready = true;
      }
    }
  }


  // switch to the correct walking mode given the speed

  if (enviro.grounded || enviro.platformed)
  {
    // handle existing animations
    if( IS_DANCING(walk_action) )
    {
      // Do boredom
      rchr_cnt.boretime--;
      if (rchr_cnt.boretime < 0)
      {
        rchr_cnt.ai.alert |= ALERT_IF_BORED;
        rchr_cnt.boretime  = BORETIME;

        // scale up the boredom animations
        walk_action = (ACTION_TYPE)(rchr_cnt.act.which + 1);
        if(walk_action>ACTION_DD) walk_action = ACTION_DA;
      }
    }
    else if( IS_WALKING(rchr_cnt.act.which) )
    {
      int framelip = rmad_cnt.getExtras(rchr_cnt.ani.frame).framelip;  // 0 - 15...  Way through animation

      if (walk_action==ACTION_WA && rchr_cnt.act.which!=ACTION_WA)
      {
        rchr_cnt.ani.frame = rmad_cnt.frameliptowalkframe[LIPWA][framelip];
        rchr_cnt.act.which = ACTION_WA;
      }
      else if (walk_action==ACTION_WB && rchr_cnt.act.which!=ACTION_WB)
      {
        rchr_cnt.ani.frame = rmad_cnt.frameliptowalkframe[LIPWB][framelip];
        rchr_cnt.act.which = ACTION_WB;
      }
      else if (walk_action==ACTION_WC && rchr_cnt.act.which!=ACTION_WC)
      {
        rchr_cnt.ani.frame = rmad_cnt.frameliptowalkframe[LIPWC][framelip];
        rchr_cnt.act.which = ACTION_WC;
      }
      else if (walk_action==ACTION_WD && rchr_cnt.act.which!=ACTION_WD)
      {
        rchr_cnt.ani.frame = rmad_cnt.frameliptowalkframe[LIPWD][framelip];
        rchr_cnt.act.which = ACTION_WD;
      }
    }
    
    // queue up a new animation based on speed
    if( IS_WALKING(walk_action) || IS_DANCING(walk_action) )
    {
      rchr_cnt.act.next = walk_action;
    }
  }


};


//-----------------------------------------------------------------------------------
void calc_chr_environment(Physics_Info & loc_phys, Character & rchr, Physics_Environment & enviro)
{
  //determine whether the character is mounted
  enviro.mounted = VALID_CHR(rchr.held_by);

  // determine whether the chr is on a platform
  enviro.platformed =  VALID_CHR(rchr.on_which_platform);

  // get the level for this character and set come properties from the mesh
  rchr.calc_levels();

  enviro.level      = rchr.level;
  enviro.lerp       = (rchr.pos.z - rchr.level)/PLATTOLERANCE;
  if(enviro.platformed && rchr.platform_level > rchr.level)
  {
    enviro.level  = rchr.platform_level;
    enviro.lerp   = (rchr.pos.z - rchr.platform_level)/PLATTOLERANCE;
  };

  enviro.grounded   = (enviro.lerp<1);

  // calculate the fluid type
  enviro.fric_fluid = loc_phys.fric_air;
  if(rchr.inwater)
  {
    // we're in the water, baby!
    enviro.fric_fluid = loc_phys.fric_h2o;
  }

  // calculate the surface type
  if(enviro.mounted || 0!=rchr.flyheight)
  {
    // Mounted of in the air, no surface
    enviro.fric_surf  = enviro.fric_fluid;
    enviro.grounded   = false;
    enviro.platformed = false;
  }
  else if ( !rchr.inwater && GMesh.has_flags(rchr.onwhichfan, MESHFX_SLIPPY) )
  {
    // On a slippery surface
    enviro.fric_surf = loc_phys.fric_slip;
  }
  else
  {
    // On a normal surface
    enviro.fric_surf = loc_phys.fric_stick;
  };

  // get the normal
  if(enviro.grounded)
  {
    GMesh.simple_normal(rchr.pos, enviro.normal);
    enviro.fric_surf = enviro.fric_surf;
  }
  else if(enviro.platformed)
  {
    // try to grab the normal from the "up vector" of the platform
    Character & rplat = ChrList[rchr.on_which_platform];
    enviro.normal = vec3_t(rplat.matrix.CNV(2,0), rplat.matrix.CNV(2,1), rplat.matrix.CNV(2,2));

    // the gravity and the normal are supposed to point in opposite directions
    if( SGN(enviro.normal.z * GPhys.gravity) > 0 ) enviro.normal *= -1;

    enviro.fric_surf = enviro.fric_surf;
  };

}

//-----------------------------------------------------------------------------------
void calc_prt_environment(Physics_Info & loc_phys, Particle & rprt, Physics_Environment & enviro)
{
  rprt.calc_levels();

  // calculate the fluid type
  enviro.fric_fluid = loc_phys.fric_air;
  if(rprt.inwater)
  {
    // we're in the water, baby!
    enviro.fric_fluid = loc_phys.fric_h2o;
  }

  // calculate the surface type
  enviro.level      = rprt.level;
  enviro.lerp       = (rprt.pos.z - rprt.level)/PLATTOLERANCE;
  enviro.platformed = false;
  enviro.grounded   = (enviro.lerp<1);

  if ( enviro.lerp > 1 )
  {
    // In the air, no surface
    enviro.fric_surf = enviro.fric_fluid;
  }
  else if ( !rprt.inwater && GMesh.has_flags(rprt.onwhichfan, MESHFX_SLIPPY) )
  {
    // On a slippery surface
    enviro.fric_surf = loc_phys.fric_slip;
  }
  else
  {
    // On a normal surface
    enviro.fric_surf = loc_phys.fric_stick;
  }
}



//--------------------------------------------------------------------------------------------
static void do_controls(Uint32 cnt, Physics_Environment & enviro, float dframe)
{
  // BB >> do the volountary actions of the player / AI

  bool actready;
  bool allowedtoattack;

  float dvx, dvy, dvmax;
  ACTION_TYPE action;
  Uint16 weapon, mount, item;

  if( INVALID_CHR(cnt) ) return;

  Character & rchr_cnt = ChrList[cnt];
  Mad       & rmad_cnt = rchr_cnt.getMad();

  float desired_dvx = 0;
  float desired_dvy = 0;

  // determine the best surface cusion value to use
  float loc_lerp = enviro.lerp;
  loc_lerp = CLIP(loc_lerp, 0, 1);

  float rot_speed = 2500 * rchr_cnt.dexterity / PERFECTSTAT * dframe;
  float side_step = (1 + rchr_cnt.dexterity / PERFECTSTAT)*.5;

  if (enviro.mounted)
  {
    dvx = ChrList[rchr_cnt.held_by].vel.x - ChrList[rchr_cnt.held_by].old.vel.x;
    dvy = ChrList[rchr_cnt.held_by].vel.y - ChrList[rchr_cnt.held_by].old.vel.y;

    if( fabsf(dvx) + fabsf(dvy) > 0 )
    {
      // scale the latches to lie between 0 and 1
      dvx = dvx/(WAYTHRESH + ABS(dvy) + ABS(dvx));
      dvy = dvy/(WAYTHRESH + ABS(dvy) + ABS(dvx));
    };
  }
  else
  {
    // Character latches for generalized movement
    dvx = rchr_cnt.ai.latch.x;
    dvy = rchr_cnt.ai.latch.y;

    // Reverse movements for daze
    if (rchr_cnt.dazetime > 0)
    {
      dvx=-dvx;
      dvy=-dvy;
    }

    // Switch x and y for daze
    if (rchr_cnt.grogtime > 0)
    {
      float tmp = dvx;
      dvx = dvy;
      dvy = tmp;
    }

    desired_dvx = dvx;
    desired_dvy = dvy;

    if (rmad_cnt.getExtras(rchr_cnt.ani.frame).framefx&MADFX_STOP)
    {
      // make the character come to a halt
      //rchr_cnt.accumulate_vel(-rchr_cnt.vel.x,-rchr_cnt.vel.y, 0);
      dvx = 0;
      dvy = 0;
    }
    else
    {
      // Limit to max acceleration
      dvmax = rchr_cnt.maxaccel;
      if (dvx < -dvmax) dvx =-dvmax;
      if (dvx >  dvmax) dvx = dvmax;
      if (dvy < -dvmax) dvy =-dvmax;
      if (dvy >  dvmax) dvy = dvmax;

      if(fabsf(dvx) + fabs(dvy) > 0.002)
      {
        Uint16 ang = rchr_cnt.turn_lr>>2;
        float dv2 = dvx*dvx + dvy*dvy;

        float fx = -cos_tab[ang];
        float fy = -sin_tab[ang];

        float dp = (dvx*fx + dvy*fy);
        float para_dvx = dp*fx;
        float para_dvy = dp*fy;

        float perp_dvx = dvx - para_dvx;
        float perp_dvy = dvy - para_dvy;

        // only allow the character to accelerate perpendicular to its motion
        // if it has high dexterity
        dvx = (para_dvx + side_step*perp_dvx) * (2.0+ABS(dp)*dp/dv2)/3.0;
        dvy = (para_dvy + side_step*perp_dvy) * (2.0+ABS(dp)*dp/dv2)/3.0;

        assert( fabs(dvx) + fabs(dvy) < 2 );
      }
    }
  }

  if(!rchr_cnt.alive)
  {
    dvx = dvy = 0;
  }
  else
  {
    switch(rchr_cnt.ai.turn_mode)
    {
    case TURNMODE_WATCH:
      // Get direction from the DESIRED change in velocity
      if ((ABS(desired_dvx) > WATCHMIN || ABS(desired_dvy) > WATCHMIN))
      {
        rchr_cnt.turn_lr += rot_speed*terp_dir(rchr_cnt.turn_lr, desired_dvx, desired_dvy);
      }
      break;

    case TURNMODE_WATCHTARGET:
      // Face the target, but do not face yourself
      if (cnt != rchr_cnt.ai.target)
        rchr_cnt.turn_lr += rot_speed*terp_dir(rchr_cnt.turn_lr, ChrList[rchr_cnt.ai.target].pos.x-rchr_cnt.pos.x, ChrList[rchr_cnt.ai.target].pos.y-rchr_cnt.pos.y);
      break;

    case TURNMODE_SPIN:
      // Get direction from ACTUAL velocity
      rchr_cnt.turn_lr += SPINRATE;
      break;

    default:
    case TURNMODE_VELOCITY:
      // Get direction from ACTUAL acceleration.
      // For low dex characters, this will be almost exactly along the actual velocity
      if(rchr_cnt.islocalplayer)
        rchr_cnt.turn_lr += rot_speed*terp_dir(rchr_cnt.turn_lr, desired_dvx, desired_dvy);
      else
        rchr_cnt.turn_lr += rot_speed*terp_dir(rchr_cnt.turn_lr, desired_dvx, desired_dvy);
      break;
    };
  }

  if(loc_lerp>1)
  {
    vec3_t tmpvec = vec3_t(dvx, dvy, 0);
    rchr_cnt.accumulate_acc( 10*tmpvec*(1-enviro.fric_fluid)*ABS(enviro.normal.z) );
  }
  else
  {
    // Make the accelerations parallel to the surface
    vec3_t tmpvec = vec3_t(dvx*ABS(enviro.normal.z), dvy*ABS(enviro.normal.z), -dvx*enviro.normal.x - dvy*enviro.normal.y);

    // Modify accelerations for reduced normal force and for reduced friction
    rchr_cnt.accumulate_acc( 20*tmpvec*(1-enviro.fric_surf)*(1-loc_lerp)*ABS(enviro.normal.z) );
  };

  // Character latches for generalized buttons
  if (rchr_cnt.alive && rchr_cnt.ai.latch.button!=0)
  {
    if (rchr_cnt.ai.latch.button&LATCHBIT_JUMP)
    {
      rchr_cnt.ai.latch.button &= ~LATCHBIT_JUMP;
      if (enviro.mounted && rchr_cnt.jumpready)
      {
        detach_character_from_mount(cnt, true, true);

        if (rchr_cnt.flyheight != 0) 
          rchr_cnt.accumulate_vel_z(DISMOUNTZFLY_VELOCITY-DROPZ_VELOCITY);
        else
          rchr_cnt.accumulate_vel_z(DISMOUNTZ_VELOCITY-DROPZ_VELOCITY);

        if (rchr_cnt.jumpnumberreset!=JUMPNUMBER_INFINITE && rchr_cnt.jumpnumber>0) rchr_cnt.jumpnumber--;
        rchr_cnt.jumptime  = DELAY_JUMP;
        rchr_cnt.jumpready = rchr_cnt.jumpnumber > 0;

        // Play the jump sound
        if( VALID_WAVE_RANGE(rchr_cnt.getCap().wavejump) )
          play_sound(rchr_cnt.pos, ProfileList[rchr_cnt.model].waveindex[rchr_cnt.getCap().wavejump]);
      }

      if (rchr_cnt.jumpready && rchr_cnt.jumpnumber!=0 && rchr_cnt.flyheight == 0)
      {
        if (rchr_cnt.jumpnumber > 0 || rchr_cnt.jumpready)
        {
          // Make the character jump
          rchr_cnt.hitready=true;
          float jumpspeed = rchr_cnt.jump;
          if (rchr_cnt.inwater)
          {
            jumpspeed = WATERJUMP;
          }

          rchr_cnt.accumulate_vel_z(jumpspeed*(1-loc_lerp)*2);
          rchr_cnt.request_detachment();
          
          if( VALID_CHR(rchr_cnt.on_which_platform) )
          {
            // !!!!equal and opposite reactions!!!!
            Character & rplat = ChrList[rchr_cnt.on_which_platform];
            if(rplat.weight > 0)
              rplat.accumulate_vel_z(-jumpspeed*rchr_cnt.weight/rplat.weight);
          };

          rchr_cnt.jumptime = DELAY_JUMP;
          if (rchr_cnt.jumpnumberreset!=JUMPNUMBER_INFINITE  && rchr_cnt.jumpnumber>0) rchr_cnt.jumpnumber--;
          rchr_cnt.jumpready = rchr_cnt.jumpnumber > 0;

          // Set to jump animation if not doing anything better
          if (rchr_cnt.act.ready) play_action(cnt, ACTION_JA, true);

          // Play the jump sound (Boing!)
          if( VALID_MODEL(rchr_cnt.model) )
          {
            Profile & rprof = ProfileList[rchr_cnt.model];
            Cap     & rcap  = rchr_cnt.getCap();

            if( VALID_WAVE_RANGE(rcap.wavejump) )
            {
              Mix_Chunk * mc = rprof.waveindex[rcap.wavejump];
              if(NULL!=mc) play_sound(rchr_cnt.pos, mc);
            };
          };
        }
      }
    }

    if ((rchr_cnt.ai.latch.button&LATCHBIT_GET_LEFT) && rchr_cnt.act.ready && rchr_cnt.reloadtime==0)
    {
      rchr_cnt.ai.latch.button &= ~LATCHBIT_GET_LEFT;
      rchr_cnt.reloadtime = DELAY_GRAB;
      if (INVALID_CHR(rchr_cnt.holding_which[SLOT_LEFT]))
      {
        // Grab left
        play_action(cnt, ACTION_ME, false);
      }
      else
      {
        // Drop left
        play_action(cnt, ACTION_MA, false);
      }
    }

    if ((rchr_cnt.ai.latch.button&LATCHBIT_GET_RIGHT) && rchr_cnt.act.ready && rchr_cnt.reloadtime==0)
    {
      rchr_cnt.ai.latch.button &= ~LATCHBIT_GET_RIGHT;
      rchr_cnt.reloadtime = DELAY_GRAB;
      if (INVALID_CHR(rchr_cnt.holding_which[SLOT_RIGHT]))
      {
        // Grab right
        play_action(cnt, ACTION_MF, false);
      }
      else
      {
        // Drop right
        play_action(cnt, ACTION_MB, false);
      }
    }

    if ((rchr_cnt.ai.latch.button&LATCHBIT_PACK_LEFT) && rchr_cnt.act.ready && rchr_cnt.reloadtime==0)
    {
      rchr_cnt.ai.latch.button &= ~LATCHBIT_PACK_LEFT;
      rchr_cnt.reloadtime = DELAY_PACK;
      item = rchr_cnt.holding_which[SLOT_LEFT];
      if (VALID_CHR(item))
      {
        if ((ChrList[item].iskursed || ChrList[item].getCap().istoobig) && !ChrList[item].getCap().is_equipment)
        {
          // The item couldn't be put away
          ChrList[item].ai.alert|=ALERT_IF_NOTPUTAWAY;
        }
        else
        {
          // Put the item into the pack
          add_item_to_character_pack(item, cnt);
        }
      }
      else
      {
        // Get a new one out and put it in hand
        get_item_from_character_pack(cnt, GRIP_LEFT, false);
      }
      // Make it take a little time
      play_action(cnt, ACTION_MG, false);
    }

    if ((rchr_cnt.ai.latch.button&LATCHBIT_PACK_RIGHT) && rchr_cnt.act.ready && rchr_cnt.reloadtime==0)
    {
      rchr_cnt.ai.latch.button &= ~LATCHBIT_PACK_RIGHT;
      rchr_cnt.reloadtime = DELAY_PACK;
      item = rchr_cnt.holding_which[SLOT_RIGHT];
      if (VALID_CHR(item))
      {
        if ((ChrList[item].iskursed || ChrList[item].getCap().istoobig) && !ChrList[item].getCap().is_equipment)
        {
          // The item couldn't be put away
          ChrList[item].ai.alert|=ALERT_IF_NOTPUTAWAY;
        }
        else
        {
          // Put the item into the pack
          add_item_to_character_pack(item, cnt);
        }
      }
      else
      {
        // Get a new one out and put it in hand
        get_item_from_character_pack(cnt, GRIP_RIGHT, false);
      }
      // Make it take a little time
      play_action(cnt, ACTION_MG, false);
    }

    if (rchr_cnt.ai.latch.button&LATCHBIT_USE_LEFT && rchr_cnt.reloadtime==0)
    {
      rchr_cnt.ai.latch.button &= ~LATCHBIT_USE_LEFT;
      // Which weapon?
      weapon = rchr_cnt.holding_which[SLOT_LEFT];
      if (INVALID_CHR(weapon))
      {
        // Unarmed means character itself is the weapon
        weapon = cnt;
      }
      action = ChrList[weapon].getCap().weaponaction;

      // Can it do it?
      allowedtoattack = true;
      if (!rmad_cnt.actinfo[action].valid || ChrList[weapon].reloadtime > 0 || 
        (ChrList[weapon].getCap().needskillidtouse && rchr_cnt.getCap().idsz[IDSZ_SKILL] != ChrList[weapon].getCap().idsz[IDSZ_SKILL]))
      {
        allowedtoattack = false;
        if (ChrList[weapon].reloadtime == 0)
        {
          // This character can't use this weapon
          ChrList[weapon].reloadtime = 50;
          if (rchr_cnt.staton)
          {
            // Tell the player that they can't use this weapon
            debug_message("%s can't use this item...", rchr_cnt.name);
          }
        }
      }
      if (action==ACTION_DA)
      {
        allowedtoattack = false;
        if (ChrList[weapon].reloadtime==0)
        {
          ChrList[weapon].ai.alert |= ALERT_IF_USED;
        }
      }

      if (allowedtoattack)
      {
        // Rearing mount
        mount = rchr_cnt.held_by;
        if (VALID_CHR(mount))
        {
          allowedtoattack = ChrList[mount].getCap().ridercanattack;
          if (ChrList[mount].is_mount && ChrList[mount].alive && !ChrList[mount].isplayer && ChrList[mount].act.ready)
          {
            if ((action != ACTION_PA || !allowedtoattack) && rchr_cnt.act.ready)
            {
              play_action(rchr_cnt.held_by, (ACTION_TYPE)(ACTION_UA+(rand()&1)), false);
              ChrList[rchr_cnt.held_by].ai.alert|=ALERT_IF_USED;
            }
            else
            {
              allowedtoattack = false;
            }
          }
        }

        // Attack button
        if (allowedtoattack)
        {
          if (rchr_cnt.act.ready && rmad_cnt.actinfo[action].valid)
          {
            // Check mana cost
            if (rchr_cnt.mana >= ChrList[weapon].manacost || rchr_cnt.canchannel)
            {
              cost_mana(cnt, ChrList[weapon].manacost, weapon);
              // Check life healing
              rchr_cnt.life+=ChrList[weapon].lifeheal;
              if (rchr_cnt.life > rchr_cnt.lifemax)  rchr_cnt.life = rchr_cnt.lifemax;
              actready = (action == ACTION_PA);
              action = (ACTION_TYPE)(action + (rand()&1));
              play_action(cnt, action, actready);
              if (weapon!=cnt)
              {
                // Make the weapon attack too
                play_action(weapon, ACTION_MJ, false);
                ChrList[weapon].ai.alert |= ALERT_IF_USED;
              }
              else
              {
                // Flag for unarmed attack
                rchr_cnt.ai.alert|=ALERT_IF_USED;
              }
            }
          }
        }
      }
    }
    else if (rchr_cnt.ai.latch.button&LATCHBIT_USE_RIGHT && rchr_cnt.reloadtime==0)
    {
      rchr_cnt.ai.latch.button &= ~LATCHBIT_USE_RIGHT;
      // Which weapon?
      weapon = rchr_cnt.holding_which[SLOT_RIGHT];
      if (INVALID_CHR(weapon))
      {
        // Unarmed means character itself is the weapon
        weapon = cnt;
      }
      action = (ACTION_TYPE)(ChrList[weapon].getCap().weaponaction+2);

      // Can it do it?
      allowedtoattack = true;
      if (!rmad_cnt.actinfo[action].valid || ChrList[weapon].reloadtime > 0 || 
        (ChrList[weapon].getCap().needskillidtouse && rchr_cnt.getCap().idsz[IDSZ_SKILL] != ChrList[weapon].getCap().idsz[IDSZ_SKILL]))
      {
        allowedtoattack = false;
        if (ChrList[weapon].reloadtime == 0)
        {
          // This character can't use this weapon
          ChrList[weapon].reloadtime = 50;
          if (rchr_cnt.staton)
          {
            // Tell the player that they can't use this weapon
            debug_message("%s can't use this item...", rchr_cnt.name);
          }
        }
      }
      if (action==ACTION_DC)
      {
        allowedtoattack = false;
        if (ChrList[weapon].reloadtime==0)
        {
          ChrList[weapon].ai.alert |= ALERT_IF_USED;
        }
      }

      if (allowedtoattack)
      {
        // Rearing mount
        mount = rchr_cnt.held_by;
        if (VALID_CHR(mount))
        {
          allowedtoattack = ChrList[mount].getCap().ridercanattack;
          if (ChrList[mount].is_mount && ChrList[mount].alive && !ChrList[mount].isplayer && ChrList[mount].act.ready)
          {
            if ((action != ACTION_PC || !allowedtoattack) && rchr_cnt.act.ready)
            {
              play_action(rchr_cnt.held_by,(ACTION_TYPE)(ACTION_UC+(rand()&1)), false);
              ChrList[rchr_cnt.held_by].ai.alert|=ALERT_IF_USED;
            }
            else
            {
              allowedtoattack = false;
            }
          }
        }

        // Attack button
        if (allowedtoattack)
        {
          if (rchr_cnt.act.ready && rmad_cnt.actinfo[action].valid)
          {
            // Check mana cost
            if (rchr_cnt.mana >= ChrList[weapon].manacost || rchr_cnt.canchannel)
            {
              cost_mana(cnt, ChrList[weapon].manacost, weapon);
              // Check life healing
              rchr_cnt.life+=ChrList[weapon].lifeheal;
              if (rchr_cnt.life > rchr_cnt.lifemax)  rchr_cnt.life = rchr_cnt.lifemax;
              actready = (action == ACTION_PC);
              action = (ACTION_TYPE)(action + (rand()&1));
              play_action(cnt, action, actready);
              if (weapon!=cnt)
              {
                // Make the weapon attack too
                play_action(weapon, ACTION_MJ, false);
                ChrList[weapon].ai.alert |= ALERT_IF_USED;
              }
              else
              {
                // Flag for unarmed attack
                rchr_cnt.ai.alert|=ALERT_IF_USED;
              }
            }
          }
        }
      }
    }
  }

};

//--------------------------------------------------------------------------------------------
static void do_movement(Uint32 cnt, Physics_Environment & enviro, float dframe)
{
  if( INVALID_CHR(cnt) ) return;

  Character & rchr_cnt = ChrList[cnt];
  Mad       & rmad_cnt = rchr_cnt.getMad();

  // Texture movement
  rchr_cnt.off += rchr_cnt.off_vel * dframe;


  // down the jump timer
  if (rchr_cnt.jumptime > 0 && (enviro.mounted || enviro.platformed || enviro.grounded)) 
    rchr_cnt.jumptime--;


  // keep the character with its holder
  if (enviro.mounted)
  {
    Character & holder = ChrList[rchr_cnt.held_by];
    rchr_cnt.vel    = holder.vel;
    rchr_cnt.vel_lr = holder.vel_lr;

    if(0 == rchr_cnt.jumptime)
    {
      rchr_cnt.jumpready  = true;
      rchr_cnt.jumpnumber = 1;
    };

    return;
  }

  // determine the influence of the surface
  float lerp  = enviro.lerp;
  float level = enviro.level;
  float lerp_last = (rchr_cnt.old.pos.z - enviro.level)/PLATTOLERANCE;

  //---- handle the jumping ----
  {
    if(rchr_cnt.flyheight!=0)
    {
      // Character is flying
      rchr_cnt.jumpready=false;  
    }
    else if( lerp > 1 )
    {
      // Character is in the air
      rchr_cnt.jumpready = (rchr_cnt.jumpnumber>0); // can still jump if it has multiple jumps
    }
    else if(0 == rchr_cnt.jumptime)
    {
      rchr_cnt.jumpready  = true;

      if (rchr_cnt.inwater || !GMesh.has_flags(rchr_cnt.onwhichfan, MESHFX_SLIPPY))
      {
        rchr_cnt.jumpnumber = rchr_cnt.jumpnumberreset;
      }
      else
      {
        // Reset jumping on flat areas, even if slippy
        rchr_cnt.jumpnumber = floor(rchr_cnt.jumpnumberreset*ABS(enviro.normal.z) + 0.5);
      }
    };

  }
  //---- handle jumping ----


  // Is the character in the air?
  if (rchr_cnt.flyheight!=0)
  {
    //  Flying
    if (rchr_cnt.level < 0) rchr_cnt.level = 0; // Don't fall in pits...

    rchr_cnt.accumulate_acc_z( (rchr_cnt.level+rchr_cnt.flyheight-rchr_cnt.pos.z)*FLYDAMPEN );
  }
  else if (lerp > 1)
  {
    rchr_cnt.accumulate_acc_z(GPhys.gravity);
  }
  else
  {
    if(lerp < 0)
    {
      rchr_cnt.accumulate_pos_z( (level - rchr_cnt.pos.z)*0.9 );
      int i = 0;
    }

    if(lerp < 0 && lerp_last > 0 && rchr_cnt.hitready)
    {
      // Just beginning to hit the surface
      rchr_cnt.ai.alert |= ALERT_IF_HITGROUND;
      rchr_cnt.hitready = false;
    }

    // Make the characters slide
    //rchr_cnt.onwhichfan = GMesh.getIndexPos(rchr_cnt.pos.x, rchr_cnt.pos.y);
    //const JF::MPD_Fan * pfan = GMesh.getFan(rchr_cnt.onwhichfan);

    // grab the pre-calculated twist from the mesh
    //twist = (NULL==pfan) ? 0x7F : pfan->twist;

    // Characters with sticky butts lie on the surface of the mesh
    if ( rchr_cnt.stickybutt || !rchr_cnt.alive )
    {
      GMesh.simple_normal(rchr_cnt.pos, rchr_cnt.up);
    }

    // estimate the ground friction
    if( dist_abs(rchr_cnt.vel) > 1e-6)
    {
      // velocity perp to the surface is parallel to the normal
      vec3_t vel_perp = parallel_normalized(enviro.normal, rchr_cnt.vel);
      vec3_t vel_para = rchr_cnt.vel - vel_perp;

      // determine the new perp velocity
      vec3_t d_vel = -(1+rchr_cnt.dampen)*vel_perp;

      // the repulsion from the surface increases as we approach
      if(lerp>0)
      {
        rchr_cnt.accumulate_acc(d_vel*(1-lerp));
      }
      else if ( lerp_last > 0)
      {
        rchr_cnt.accumulate_acc(d_vel);
      }

      // determine the new parallel velocity
      d_vel   = -(1-enviro.fric_surf)*vel_para;

      // the "frictional force" from the surface increases as we approach
      // will stop horiz motion if the rchr_cnt penetrates farther than PLATTOLERANCE
      if(lerp>0)
        rchr_cnt.accumulate_acc(d_vel*(1-lerp));
      else if(lerp>-1)
      {
        float loc_lerp = -lerp;
        rchr_cnt.accumulate_acc(d_vel*(1-loc_lerp) - vel_para*loc_lerp);
      }
      else
        rchr_cnt.accumulate_acc(-vel_para);

      int i = 0;
    }

    // determine the gravitational action
    if(rchr_cnt.weight>=0 && !enviro.platformed)
    {
      float grav_dotprod = GPhys.gravity*enviro.normal.z;

      vec3_t grav_perp = grav_dotprod * enviro.normal;

      // gravitational interaction perp to the surface weakens as we approach the surface.
      // this simulates the supporting action of the surface
      if(lerp>0) rchr_cnt.accumulate_acc(grav_perp*lerp);

      // gravitational interaction parallel to the surface strengthens as we approach
      vec3_t grav_para = vec3_t(0,0,GPhys.gravity) - grav_perp;
      if(lerp>0)
        rchr_cnt.accumulate_acc(grav_para*(1-lerp)*2);
      else
        rchr_cnt.accumulate_acc(grav_para*2);

      int i = 0;
    }

  }

  // calulate the fluid drag
  if(dist_abs(rchr_cnt.vel) > 1e-6 && rchr_cnt.weight>0)
  {
    float vel2_horiz = rchr_cnt.vel.x*rchr_cnt.vel.x + rchr_cnt.vel.y*rchr_cnt.vel.y;
    float vel2_vert  = rchr_cnt.vel.z*rchr_cnt.vel.z;
    float vel2       = vel2_horiz + vel2_vert;
    float sin2       = vel2_horiz / vel2;

    // the fluid friction depends on the surface area presented to the wind
    // assume that the "average" object is about an Adventurer
    // 60wide x 90tall units = (1/3 of a tile area) and has a weight of 110
    float area = (rchr_cnt.calc_bump_height+1)*(rchr_cnt.calc_bump_size+1) * sin2 + (rchr_cnt.calc_bump_size+1)*(rchr_cnt.calc_bump_size+1)*(1-sin2);
    float cd   = 0;

    cd = (area / float(60*90)) * ( float(110)/(rchr_cnt.weight+1)) * (1.0f-enviro.fric_fluid);
    cd = CLIP(cd,0,1);

    // do the friction
    rchr_cnt.accumulate_acc(-cd*rchr_cnt.vel);
    int i = 0;
  }
  else if(rchr_cnt.weight==0)
  {
    rchr_cnt.accumulate_acc(-rchr_cnt.vel);
  };

}



//--------------------------------------------------------------------------------------------
static void do_map_interaction(Uint32 cnt, Physics_Environment & enviro, float dframe)
{
  if( INVALID_CHR(cnt) ) return;

  Character & rchr_cnt = ChrList[cnt];

   if(rchr_cnt.vel.z > STOPBOUNCING && rchr_cnt.pos.z > rchr_cnt.level-STOPBOUNCING)
  {
    // make the object stop moving
    rchr_cnt.accumulate_vel_z(-rchr_cnt.vel.z);
  }

  //check for collisions with the mesh
  //if ( (fabs(rchr_cnt.vel.x)+fabs(rchr_cnt.vel.y)>0) && rchr_cnt.hitawall(GMesh, normal) && (fabs(normal.x) + fabs(normal.y) + fabs(normal.y) > 0.1) )
  //{
  //  float dprod = dot_product(normal, rchr_cnt.vel);

  //  vec3_t vperp = parallel_normalized(normal, rchr_cnt.vel);
  //  vec3_t vpara = rchr_cnt.vel - vperp;

  //  // calculate the reflection off the wall
  //  vec3_t vbounce = vpara - vperp*rchr_cnt.dampen;

  //  // make the chr bounce
  //  rchr_cnt.accumulate_vel(vbounce-rchr_cnt.vel);
  //}
  //else 
  if( rchr_cnt.hitmesh(GMesh, enviro.normal) )
  {
    vec3_t vperp = parallel_normalized(enviro.normal, rchr_cnt.vel);
    vec3_t vpara = rchr_cnt.vel - vperp;

    // calculate the reflection off the wall
    vec3_t vbounce = vpara - vperp*rchr_cnt.dampen;

    // make the chr bounce
    rchr_cnt.accumulate_vel(vbounce-rchr_cnt.vel);
  }
  else if (rchr_cnt.pos.z>WaterList.level_surface && rchr_cnt.pos.z + rchr_cnt.vel.z < WaterList.level_surface)
  {
    if(GMesh.has_flags(rchr_cnt.pos.x, rchr_cnt.pos.y, MESHFX_WATER))
    {
      // slow down on impact with water
      rchr_cnt.accumulate_vel_z(-rchr_cnt.pos.z*(1-GPhys.fric_h2o));
    }
  };

};

void do_poof(Uint32 cnt, Character & rchr_cnt)
{
  if (VALID_CHR(rchr_cnt.held_by))
    detach_character_from_mount(cnt, true, false);

  if (VALID_CHR(rchr_cnt.holding_which[SLOT_LEFT]))
    detach_character_from_mount(rchr_cnt.holding_which[SLOT_LEFT], true, false);

  if (VALID_CHR(rchr_cnt.holding_which[SLOT_RIGHT]))
    detach_character_from_mount(rchr_cnt.holding_which[SLOT_RIGHT], true, false);

  free_inventory(cnt);
  free_one_character(cnt);

  rchr_cnt.ai.gopoof = false;
};

//--------------------------------------------------------------------------------------------
void move_characters(Physics_Info & loc_phys, float dframe)
{
  // ZZ> This function handles character physics
  int cnt;

  Physics_Environment enviro;

  // Move every character
  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if (rchr_cnt.is_inpack) continue;

    //
    if(!rchr_cnt.integration_allowed) rchr_cnt.begin_integration();

    // Down that ol' damage timer
    rchr_cnt.damagetime -= (rchr_cnt.damagetime != 0);

   // Do "Be careful!" delay
    rchr_cnt.carefultime -= (rchr_cnt.carefultime != 0);

    calc_chr_environment(loc_phys, rchr_cnt, enviro);
    do_controls(cnt, enviro, dframe);
    do_movement(cnt, enviro, dframe);
    //do_map_interaction(cnt, enviro, dframe);
    do_anim(cnt, enviro, dframe);

    // Do poofing
    if (rchr_cnt.ai.gopoof) do_poof(cnt, rchr_cnt);

  } SCAN_CHR_END;

}

//--------------------------------------------------------------------------------------------
void Character_List::setup_characters(char *modname)
{
  // ZZ> This function sets up character data, loaded from "SPAWN.TXT"
  int current_character;
  int current_slot;
  int passage, content, money, level, skin, cnt, tnc, localnumber = 0;
  Uint8 ghost, team, stat, cTmp;
  char *name;
  char itislocal;
  char myname[0x0100], newloadname[0x0100], blahname[0x0100];
  Uint16 facing, grip, attach;
  Uint32 slot;
  vec3_t pos;
  FILE *fileread;

  // Turn all characters off
  ChrList.free_all();

  // Turn some back on
  make_newloadname(modname, "gamedat/spawn.txt", newloadname);
  fileread = fopen(newloadname, "r");
  if (!fileread) general_error(0, 0, newloadname);

  int last_character = Character_List::INVALID;
  int last_slot      = 0;
  while ( goto_colon_yesno(fileread, blahname) )
  {
    fscanf(fileread, "%s", myname);

    name = myname;
    if ( 0 == strcmp("NONE", myname) )
    {
      name = NULL;  // Random name
    }

    // replace "_"
    for (cnt = 0; cnt < 0x0100; cnt++)
    {
      if (myname[cnt]=='_')  myname[cnt] = ' ';
    }

    cout << endl << "reading " << myname << "(" << blahname << ")" << endl;

    fscanf(fileread, "%d", &slot);
    fscanf(fileread, "%f%f%f", &pos.x, &pos.y, &pos.z); pos.x *= 0x80;  pos.y *= 0x80;  pos.z *= 0x80;

    cTmp   = get_first_letter(fileread);
    attach = Character_List::INVALID;
    facing = NORTH;
    grip   = GRIP_SADDLE;
    switch( toupper(cTmp) )
    {
      case 'N': facing = NORTH; break;
      case 'S': facing = SOUTH; break;
      case 'E': facing = EAST;  break;
      case 'W': facing = WEST;  break;
      case 'L': { attach = last_character; grip = GRIP_LEFT;      }; break;
      case 'R': { attach = last_character; grip = GRIP_RIGHT;     }; break;
      case 'I': { attach = last_character; grip = GRIP_INVENTORY; }; break;
    }

    fscanf(fileread, "%d%d%d%d%d", &money, &skin, &passage, &content, &level);

    stat  = get_bool(fileread);
    ghost = get_bool(fileread);
    team  = get_team(fileread);

    // Spawn the character
    if (team < GNet.numplayer || !GRTS.on || team >= TEAM_COUNT)
    {
      Uint32 prof_idx = ProfileList.slot_list[slot];
      if(Profile_List::INVALID == prof_idx)
        continue;

      current_character = spawn_one_character(pos, facing, vec3_t(0,0,0), 0, prof_idx, team, skin, name);
      current_slot      = slot;

      if (VALID_CHR(current_character))
      {
        if (allselect && team == GRTS.team_local)
          add_select(current_character);

        Character & rchr = ChrList[current_character];

        rchr.money += money;
        if (rchr.money > MAXMONEY)  rchr.money = MAXMONEY;
        if (rchr.money < 0)  rchr.money = 0;
        rchr.ai.content = content;
        rchr.passage = passage;
        if (INVALID_CHR(attach))
        {
          // Free character
          last_character = current_character;
          last_slot      = current_slot;
        }
        else
        {
          // Attached character
          if (grip != GRIP_INVENTORY)
          {
            // Wielded character
            attach_character_to_mount(current_character, last_character, grip);
            let_character_think(current_character);  // Empty the grabbed messages
          }
          else
          {
            // Inventory character
            add_item_to_character_pack(current_character, last_character);
            rchr.ai.alert |= ALERT_IF_GRABBED;  // Make spellbooks change
            rchr.held_by = last_character;  // Make grab work
            rchr.held_by = Character_List::INVALID;  // Fix grab
          }
        }

        // Turn on player input devices
        if (stat)
        {
          if (importamount == 0)
          {
            if (playeramount < 2)
            {
              if (numstat == 0)
              {
                // Single player module
                PlaList.add_player(current_character, INPUT_MOUSE|INPUT_KEY|INPUT_JOYA|INPUT_JOYB);
              }
            }
            else
            {
              if (!GNet.on)
              {
                if (playeramount == 2)
                {
                  // Two player hack
                  if (numstat == 0)
                  {
                    // First player
                    PlaList.add_player(current_character, INPUT_MOUSE|INPUT_KEY|INPUT_JOYB);
                  }
                  if (numstat == 1)
                  {
                    // Second player
                    PlaList.add_player(current_character, INPUT_JOYA);
                  }
                }
                else
                {
                  // Three player hack
                  if (numstat == 0)
                  {
                    // First player
                    PlaList.add_player(current_character, INPUT_KEY);
                  }
                  if (numstat == 1)
                  {
                    // Second player
                    PlaList.add_player(current_character, INPUT_JOYA);
                  }
                  if (numstat == 2)
                  {
                    // Third player
                    PlaList.add_player(current_character, INPUT_JOYB|INPUT_MOUSE);
                  }
                }
              }
              else
              {
                // One player per machine hack
                if (localmachine == numstat)
                {
                  PlaList.add_player(current_character, INPUT_MOUSE|INPUT_KEY|INPUT_JOYA|INPUT_JOYB);
                }
              }
            }
          }

          if (numstat < importamount)
          {
            // Multiplayer import module
            itislocal = false;
            tnc = 0;
            while (tnc < numimport)
            {
              if ( CapList.m_slotlist[current_slot] == localslot[tnc])
              {
                itislocal = true;
                localnumber = tnc;
                tnc = numimport;
              }
              tnc++;
            }

            if (itislocal)
            {
              // It's a local player
              PlaList.add_player(current_character, localcontrol[localnumber]);
            }
            else
            {
              // It's a remote player
              PlaList.add_player(current_character, INPUT_NONE);
            }
          }
          // Turn on the stat display
          add_stat(current_character);
        }

        // Set the starting level
        if (!rchr.isplayer)
        {
          // Let the character gain levels
          level = level - 1;
          while (rchr.experiencelevel < level && rchr.experience < MAXXP)
          {
            give_experience(current_character, MAXXP, XPDIRECT);
          }
        }
        if (ghost)
        {
          // Make the character a ghost !!!BAD!!!  Can do with enchants
          rchr.alpha = 0x80;
          rchr.light = 0xFF;
        }
      }
    }
  }
  fclose(fileread);

  clear_messages();

  // Make sure local players are displayed first
  sort_stat();

  // Fix tilting trees problem
  //tilt_characters_to_terrain();

  // Assume RTS mode always has players...  So it doesn't quit
  if (GRTS.on)  nolocalplayers = false;
}

//--------------------------------------------------------------------------------------------
void set_one_player_latch(Uint16 player)
{
  // ZZ> This function converts input readings to latch settings, so players can
  //     move around

  Uint16 turnsin, character;
  Uint8 device;

  float inputx, inputy;

  // Check to see if we need to bother
  if (INVALID_PLAYER(player)) return;

  // Make life easier
  Player & rpla = PlaList[player];
  character = rpla.index;
  device    = rpla.device;

  //save the old latch
  rpla.latch_old = rpla.latch;

  // Clear the player's latch buffers
  rpla.latch.clear();

  inputx = 0;
  inputy = 0;

  // Device_Mouse routines
  if ((device & INPUT_MOUSE) && GMous.on)
  {
    // Movement
    if ( (cam_autoturn==0xFF) || 
         !CtrlList.mouse_is_pressed(MOS_CAMERA))  // Don't allow movement in camera control mode
    {
      float loc_inputx = 0, loc_inputy = 0;

      inputx += GMous.dx;
      inputy += GMous.dy;
    }

    // Read buttons
    if (CtrlList.mouse_is_pressed(MOS_JUMP))
      rpla.latch.button |= LATCHBIT_JUMP;
    if (CtrlList.mouse_is_pressed(MOS_USE_LEFT))
      rpla.latch.button |= LATCHBIT_USE_LEFT;
    if (CtrlList.mouse_is_pressed(MOS_GET_LEFT))
      rpla.latch.button |= LATCHBIT_GET_LEFT;
    if (CtrlList.mouse_is_pressed(MOS_PACK_LEFT))
      rpla.latch.button |= LATCHBIT_PACK_LEFT;
    if (CtrlList.mouse_is_pressed(MOS_USE_RIGHT))
      rpla.latch.button |= LATCHBIT_USE_RIGHT;
    if (CtrlList.mouse_is_pressed(MOS_GET_RIGHT))
      rpla.latch.button |= LATCHBIT_GET_RIGHT;
    if (CtrlList.mouse_is_pressed(MOS_PACK_RIGHT))
      rpla.latch.button |= LATCHBIT_PACK_RIGHT;
  }

  // Device_Joystick A routines
  if ((device & INPUT_JOYA) && GJoy[0].on)
  {
    // Movement
    if ((cam_autoturn==0xFF && PlaList.count_local == 1) || 
        !CtrlList.joya_is_pressed(JOA_CAMERA))
    {
      float loc_inputx = 0, loc_inputy = 0;

      loc_inputx = GJoy[0].latch.x;
      loc_inputy = GJoy[0].latch.y;

      if (cam_autoturn==0xFF && CtrlList.joya_is_pressed(JOA_CAMERA))  loc_inputx = 0;

      inputx += loc_inputx;
      inputy += loc_inputy;
    }

    // Read buttons
    if (CtrlList.joya_is_pressed(JOA_JUMP))
      rpla.latch.button |= LATCHBIT_JUMP;
    if (CtrlList.joya_is_pressed(JOA_USE_LEFT))
      rpla.latch.button |= LATCHBIT_USE_LEFT;
    if (CtrlList.joya_is_pressed(JOA_GET_LEFT))
      rpla.latch.button |= LATCHBIT_GET_LEFT;
    if (CtrlList.joya_is_pressed(JOA_PACK_LEFT))
      rpla.latch.button |= LATCHBIT_PACK_LEFT;
    if (CtrlList.joya_is_pressed(JOA_USE_RIGHT))
      rpla.latch.button |= LATCHBIT_USE_RIGHT;
    if (CtrlList.joya_is_pressed(JOA_GET_RIGHT))
      rpla.latch.button |= LATCHBIT_GET_RIGHT;
    if (CtrlList.joya_is_pressed(JOA_PACK_RIGHT))
      rpla.latch.button |= LATCHBIT_PACK_RIGHT;
  }

  // Device_Joystick B routines
  if ((device & INPUT_JOYB) && GJoy[1].on)
  {
    // Movement
    if ((cam_autoturn==0xFF && PlaList.count_local == 1) || 
        !CtrlList.joyb_is_pressed(JOB_CAMERA))
    {
      float loc_inputx = 0, loc_inputy = 0;

      loc_inputx = GJoy[1].latch.x;
      loc_inputy = GJoy[1].latch.y;

      if (cam_autoturn==0xFF && 
          CtrlList.joyb_is_pressed(JOB_CAMERA))  loc_inputx = 0;

      inputx += loc_inputx;
      inputy += loc_inputy;
    }

    // Read buttons
    if (CtrlList.joyb_is_pressed(JOB_JUMP))
      rpla.latch.button |= LATCHBIT_JUMP;
    if (CtrlList.joyb_is_pressed(JOB_USE_LEFT))
      rpla.latch.button |= LATCHBIT_USE_LEFT;
    if (CtrlList.joyb_is_pressed(JOB_GET_LEFT))
      rpla.latch.button |= LATCHBIT_GET_LEFT;
    if (CtrlList.joyb_is_pressed(JOB_PACK_LEFT))
      rpla.latch.button |= LATCHBIT_PACK_LEFT;
    if (CtrlList.joyb_is_pressed(JOB_USE_RIGHT))
      rpla.latch.button |= LATCHBIT_USE_RIGHT;
    if (CtrlList.joyb_is_pressed(JOB_GET_RIGHT))
      rpla.latch.button |= LATCHBIT_GET_RIGHT;
    if (CtrlList.joyb_is_pressed(JOB_PACK_RIGHT))
      rpla.latch.button |= LATCHBIT_PACK_RIGHT;
  }

  // Device_Keyboard routines
  if ((device & INPUT_KEY) && GKeyb.on)
  {
    float loc_inputx = 0, loc_inputy = 0;

    loc_inputx = GKeyb.latch.x;
    loc_inputy = GKeyb.latch.y;

    if (cam_autoturn==0xFF && 
        (CtrlList.key_is_pressed(KEY_CAMERA_LEFT ) || 
         CtrlList.key_is_pressed(KEY_CAMERA_RIGHT)))  loc_inputx = 0;

    inputx += loc_inputx;
    inputy += loc_inputy;

    // Read buttons
    if (CtrlList.key_is_pressed(KEY_JUMP))
      rpla.latch.button |= LATCHBIT_JUMP;
    if (CtrlList.key_is_pressed(KEY_USE_LEFT))
      rpla.latch.button |= LATCHBIT_USE_LEFT;
    if (CtrlList.key_is_pressed(KEY_GET_LEFT))
      rpla.latch.button |= LATCHBIT_GET_LEFT;
    if (CtrlList.key_is_pressed(KEY_PACK_LEFT))
      rpla.latch.button |= LATCHBIT_PACK_LEFT;
    if (CtrlList.key_is_pressed(KEY_USE_RIGHT))
      rpla.latch.button |= LATCHBIT_USE_RIGHT;
    if (CtrlList.key_is_pressed(KEY_GET_RIGHT))
      rpla.latch.button |= LATCHBIT_GET_RIGHT;
    if (CtrlList.key_is_pressed(KEY_PACK_RIGHT))
      rpla.latch.button |= LATCHBIT_PACK_RIGHT;
  }

  turnsin = GCamera.turn_lr;
  rpla.latch.x = ( inputx*sin_tab[turnsin]+inputy*cos_tab[turnsin]);
  rpla.latch.y = (-inputx*cos_tab[turnsin]+inputy*sin_tab[turnsin]);

  // scale the input response by the accelleration
  if (VALID_CHR(ChrList[character].held_by))
  {
    // Mounted
    rpla.latch.x *= ChrList[ChrList[character].held_by].maxaccel;
    rpla.latch.y *= ChrList[ChrList[character].held_by].maxaccel;
  }
  else
  {
    // Unmounted
    rpla.latch.x *= ChrList[character].maxaccel;
    rpla.latch.y *= ChrList[character].maxaccel;
  };

}

//--------------------------------------------------------------------------------------------
void read_local_latches(void)
{
  // ZZ> This function emulates AI thinkin' by setting latches from input devices

  for(int cnt=0; cnt<Player_List::SIZE; cnt++)
  {
    if( INVALID_PLAYER(cnt) ) continue;

    Uint16 chr = PlaList[cnt].index;
    if(INVALID_CHR(chr) || !ChrList[chr].islocalplayer) continue;

    set_one_player_latch(cnt);
  }
}

//--------------------------------------------------------------------------------------------
void make_onwhichfan(void)
{
  // ZZ> This function figures out which fan characters are on and sets their level
  Uint16 character;
  int rip_rate, distance;
  //int volume;

  // First figure out which fan each character is in
  SCAN_CHR_BEGIN(character, rchr_chr)
  {
    rchr_chr.onwhichfan = GMesh.getIndexPos(rchr_chr.pos.x, rchr_chr.pos.y);
  } SCAN_CHR_END;

  // Get levels every update
  SCAN_CHR_BEGIN(character, rchr_chr)
  {
    if ( rchr_chr.is_inpack ) continue;

    if (rchr_chr.alive)
    {
      if (GMesh.has_flags(rchr_chr.onwhichfan, MESHFX_DAMAGE) && rchr_chr.pos.z<=rchr_chr.level+DAMAGE_RAISE && INVALID_CHR(rchr_chr.held_by))
      {
        if ((rchr_chr.damagemodifier[GTile_Dam.type]&DAMAGE_SHIFT) != 3 && !rchr_chr.invictus) // 3 means they're pretty well immune
        {
          distance = diff_abs_horiz(GCamera.track,rchr_chr.pos);
          if (distance < GTile_Dam.mindistance)
          {
            GTile_Dam.mindistance = distance;
          }
          if (distance < GTile_Dam.mindistance + 0x0100)
          {
            GTile_Dam.soundtime = 0;
          }
          if (rchr_chr.damagetime == 0)
          {
            damage_character(character, BEHIND, GTile_Dam.amount, 1, GTile_Dam.type, TEAM_DAMAGE, rchr_chr.ai.bumplast, DAMFX_BLOC|DAMFX_ARMO);
            rchr_chr.damagetime = INVICTUS_TILETIME;
          }
          if (GTile_Dam.parttype != -1 && (wldframe&GTile_Dam.partand)==0)
          {
            spawn_one_particle(rchr_chr.pos, 0, vec3_t(0,0,0), 0,
               Profile_List::INVALID, GTile_Dam.parttype, Character_List::INVALID, GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, 0, Character_List::INVALID);
          }
        }
        if (rchr_chr.attachedprtreaffirmdamagetype==GTile_Dam.type)
        {
          if ((wldframe&TILEREAFFIRMAND)==0)
            reaffirm_attached_particles(character);
        }
      }
    }

    if (rchr_chr.pos.z < WaterList.level_surface && GMesh.has_flags(rchr_chr.onwhichfan, MESHFX_WATER))
    {
      if (!rchr_chr.inwater)
      {
        // Splash
        if (INVALID_CHR(rchr_chr.held_by))
        {
          vec3_t postmp = rchr_chr.pos;
          postmp.z = WaterList.level_surface+RAISE;
          spawn_one_particle( postmp,  0, vec3_t(0,0,0), 0, Profile_List::INVALID, PRTPIP_SPLASH, Character_List::INVALID, 
                              GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, 0, Character_List::INVALID);
        }
        rchr_chr.inwater = true;
        if (WaterList.is_water)
        {
          rchr_chr.ai.alert|=ALERT_IF_INWATER;
        }
      }
      else
      {
        if (rchr_chr.pos.z>WaterList.level_surface-RIPPLETOLERANCE && rchr_chr.getCap().ripple)
        {
          // Ripples
          rip_rate = dist_abs_horiz(rchr_chr.vel);
          if ( rip_rate>0 && (wldframe%rip_rate)==0 && rchr_chr.pos.z<WaterList.level_surface && rchr_chr.alive)
          {
            vec3_t postmp = rchr_chr.pos;
            postmp.z = WaterList.level_surface;
            spawn_one_particle(postmp, 0, vec3_t(0,0,0), 0, Profile_List::INVALID, PRTPIP_RIPPLE, Character_List::INVALID, 
                               GRIP_LAST, TEAM_NEUTRAL, Character_List::INVALID, 0, Character_List::INVALID);
          }
        }

        if (WaterList.is_water && (wldframe&7) == 0)
        {
          rchr_chr.jumpready = true;
          rchr_chr.jumpnumber=1; //rchr_chr.jumpnumberreset;
        }
      }
    }
    else
    {
      rchr_chr.inwater = false;
    }
  } SCAN_CHR_END;


  // Play the damage tile sound
  if (GTile_Dam.sound >= 0)
  {
    if ((wldframe & 3)==0)
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
void stat_return()
{
  // ZZ> This function brings mana and life back
  int cnt, owner, target;

  // Do reload time
  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if (rchr_cnt.reloadtime > 0)
    {
      rchr_cnt.reloadtime--;
    }
  } SCAN_CHR_END;

  // Do stats
  if (statclock==ONESECOND)
  {
    // Reset the clock
    statclock = 0;

    // Do all the characters
    SCAN_CHR_BEGIN(cnt, rchr_cnt)
    {
      if ( rchr_cnt.is_inpack || !rchr_cnt.alive ) continue;

      rchr_cnt.mana+=rchr_cnt.manareturn;
      if (rchr_cnt.mana < 0) rchr_cnt.mana = 0;
      if (rchr_cnt.mana > rchr_cnt.manamax) rchr_cnt.mana = rchr_cnt.manamax;

      rchr_cnt.life+=rchr_cnt.lifereturn;
      if (rchr_cnt.life < 1) rchr_cnt.life = 1;
      if (rchr_cnt.life > rchr_cnt.lifemax) rchr_cnt.life = rchr_cnt.lifemax;

      if (rchr_cnt.grogtime > 0)
      {
        rchr_cnt.grogtime--;
        if (rchr_cnt.grogtime < 0)
          rchr_cnt.grogtime = 0;
      }

      if (rchr_cnt.dazetime > 0)
      {
        rchr_cnt.dazetime--;
        if (rchr_cnt.dazetime < 0) rchr_cnt.dazetime = 0;
      }

    } SCAN_CHR_END;

    // Run through all the enchants as well
    SCAN_ENCLIST_BEGIN(cnt, renc)
    {
      if (renc.time == 0)
      {
        remove_enchant(cnt);
        continue;
      };


      if (renc.time > 0)
      {
        renc.time--;
      }

      owner  = renc.owner;
      target = renc.target;
      Eve & reve = renc.getEve();

      // Do drains
      if (VALID_CHR(owner) && ChrList[owner].alive)
      {
        Character & rowner = ChrList[owner];

        // Change life
        rowner.life+=renc.ownerlife;
        if (rowner.life < 1)
        {
          rowner.life = 1;
          kill_character(owner, target);
        }

        if (rowner.life > rowner.lifemax)
        {
          rowner.life = rowner.lifemax;
        }

        // Change mana
        if (!cost_mana(owner, -renc.ownermana, target) && reve.endifcantpay)
        {
          remove_enchant(cnt);
          continue;
        }
      }
      else if (!reve.stayifnoowner)
      {
        remove_enchant(cnt);
        continue;
      }

      if (VALID_CHR(target) && ChrList[target].alive)
      {
        Character & rtarget = ChrList[target];

        // Change life
        rtarget.life += renc.targetlife;
        if (rtarget.life < 1)
        {
          rtarget.life = 1;
          kill_character(target, owner);
        }

        if (rtarget.life > rtarget.lifemax)
        {
          rtarget.life = rtarget.lifemax;
        }

        // Change mana
        if (!cost_mana(target, -renc.targetmana, owner) && reve.endifcantpay)
        {
          remove_enchant(cnt);
        }
      }
      else
      {
        remove_enchant(cnt);
      }

    } SCAN_ENCLIST_END;
  }
}

//--------------------------------------------------------------------------------------------
void pit_kill()
{
  // ZZ> This function kills any character in a deep pit...
  int cnt;

  if (pitskill)
  {
    if (pitclock > 19)
    {
      pitclock = 0;

      // Kill any particles that fell in a pit, if they die in water...
      SCAN_PRT_BEGIN(cnt, rprt_cnt)
      {
        if (rprt_cnt.pos.z < PITDEPTH && PipList[rprt_cnt.pip].endwater)
        {
          rprt_cnt.requestDestroy();
        }
      } SCAN_PRT_END;

      // Kill any characters that fell in a pit...
      SCAN_CHR_BEGIN(cnt, rchr_cnt)
      {
        if (!rchr_cnt.alive || rchr_cnt.is_inpack) continue;

        if (!rchr_cnt.invictus && rchr_cnt.pos.z < PITDEPTH && INVALID_CHR(rchr_cnt.held_by))
        {
          // Got one!
          kill_character(cnt, Character_List::INVALID);
          rchr_cnt.vel.x = 0;
          rchr_cnt.vel.y = 0;
        }
      } SCAN_CHR_END;
    }
    else
    {
      pitclock++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void Player_List::reset_players()
{
  // ZZ> This function clears the player list data
  int cnt;

  // Reset the local data stuff
  localseekurse = false;
  localseeinvisible = false;
  alllocalpladead = false;

  // Reset the initial player data and latches
  cnt = 0;
  while (cnt<Player_List::SIZE)
  {
    PlaList[cnt]._on    = false;
    PlaList[cnt].index = 0;
    PlaList[cnt].latch.clear();
    PlaList[cnt].latch_old.clear();
    PlaList[cnt].timelatch.clear();
    PlaList[cnt].device = INPUT_NONE;
    cnt++;
  }
  PlaList.count_total = 0;
  nexttimestamp = -1;
  PlaList.count_times = GNet.lag_tolerance+1;
  if (hostactive) PlaList.count_times++;
}

//--------------------------------------------------------------------------------------------
void resize_characters()
{
  // ZZ> This function makes the characters get bigger or smaller, depending
  //     on their scale_goto and sizegototime
  int cnt, mount;
  bool willgetcaught;
  float newsize;

  SCAN_CHR_BEGIN(cnt, rchr_cnt)
  {
    if (!ChrList[cnt].sizegototime) continue;

    float lerp = rchr_cnt.sizegototime/float(SIZETIME);

    float aspectratio    = rchr_cnt.scale_horiz / rchr_cnt.scale_vert;
    float newscale_vert  = rchr_cnt.scale_goto * lerp + rchr_cnt.scale_vert * (1.0-lerp);
    float newscale_horiz = aspectratio*newscale_vert;

    // Make sure it won't get caught in a wall
    willgetcaught = false;
    if (newscale_horiz > rchr_cnt.scale_horiz)
    {
      float ftmp = rchr_cnt.calc_bump_size;
      rchr_cnt.calc_bump_size *= newscale_horiz/rchr_cnt.scale_horiz;
      if (rchr_cnt.inawall(GMesh))
      {
        willgetcaught = true;
      }
      rchr_cnt.calc_bump_size = ftmp;
    }

    // If it is getting caught, simply halt growth until later
    if (!willgetcaught)
    {
      // Figure out how big it is
      rchr_cnt.sizegototime--;

      rchr_cnt.scale_horiz = newscale_horiz;
      rchr_cnt.scale_vert  = newscale_vert;
      rchr_cnt.weight = rchr_cnt.getCap().weight*newscale_horiz*newscale_horiz*newscale_vert;

      make_one_character_matrix(rchr_cnt);
      rchr_cnt.calculate_bumpers();
    }

  } SCAN_CHR_END;
}

//--------------------------------------------------------------------------------------------
void export_one_character_name(char *szSaveName, Uint16 character)
{
  // ZZ> This function makes the naming.txt file for the character
  FILE* filewrite;
  int profile;
  char cTmp;
  int cnt, tnc;

  // Can it export?
  profile = ChrList[character].model;
  filewrite = fopen(szSaveName, "w");
  if (filewrite)
  {
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
}

//--------------------------------------------------------------------------------------------
void export_one_character_profile(char *szSaveName, Uint16 character)
{
  // ZZ> This function creates a data.txt file for the given character.
  //     it is assumed that all enchantments have been done away with
  FILE* filewrite;
  int damagetype, skin;
  char types[10] = "SCPHEFIZ";
  char codes[4];

  // General stuff
  Character & rchr = ChrList[character];
  Cap       & rcap = rchr.getCap();
  Mad       & rmad = rchr.getMad();

  // Open the file
  filewrite = fopen(szSaveName, "w");
  if (filewrite)
  {
    // Real general data
    fprintf(filewrite, "Slot number    : -1\n");  // -1 signals a flexible load thing
    funderf(filewrite, "Class name     : ", rcap.classname);
    ftruthf(filewrite, "Uniform light  : ", rcap.uniformlit);
    fprintf(filewrite, "Maximum ammo   : %d\n", rcap.ammomax);
    fprintf(filewrite, "Current ammo   : %d\n", rchr.ammo);
    fgendef(filewrite, "Gender         : ", rchr.gender);
    fprintf(filewrite, "\n");

    // Object stats
    fprintf(filewrite, "Life color     : %d\n", rchr.lifecolor);
    fprintf(filewrite, "Mana color     : %d\n", rchr.manacolor);
    fprintf(filewrite, "Life           : %4.2f\n", rchr.lifemax/float(0x0100));
    fpairof(filewrite, "Life up        : ", rcap.lifeperlevelbase, rcap.lifeperlevelrand);
    fprintf(filewrite, "Mana           : %4.2f\n", rchr.manamax/float(0x0100));
    fpairof(filewrite, "Mana up        : ", rcap.manaperlevelbase, rcap.manaperlevelrand);
    fprintf(filewrite, "Mana return    : %4.2f\n", rchr.manareturn/16.0);
    fpairof(filewrite, "Mana return up : ", rcap.manareturnperlevelbase, rcap.manareturnperlevelrand);
    fprintf(filewrite, "Mana flow      : %4.2f\n", rchr.manaflow/float(0x0100));
    fpairof(filewrite, "Mana flow up   : ", rcap.manaflowperlevelbase, rcap.manaflowperlevelrand);
    fprintf(filewrite, "STR            : %4.2f\n", rchr.strength/float(0x0100));
    fpairof(filewrite, "STR up         : ", rcap.strengthperlevelbase, rcap.strengthperlevelrand);
    fprintf(filewrite, "WIS            : %4.2f\n", rchr.wisdom/float(0x0100));
    fpairof(filewrite, "WIS up         : ", rcap.wisdomperlevelbase, rcap.wisdomperlevelrand);
    fprintf(filewrite, "INT            : %4.2f\n", rchr.intelligence/float(0x0100));
    fpairof(filewrite, "INT up         : ", rcap.intelligenceperlevelbase, rcap.intelligenceperlevelrand);
    fprintf(filewrite, "DEX            : %4.2f\n", rchr.dexterity/float(0x0100));
    fpairof(filewrite, "DEX up         : ", rcap.dexterityperlevelbase, rcap.dexterityperlevelrand);
    fprintf(filewrite, "\n");

    // More physical attributes
    fprintf(filewrite, "Size           : %4.2f\n", rchr.scale_goto);
    fprintf(filewrite, "Size up        : %4.2f\n", rcap.sizeperlevel);
    fprintf(filewrite, "Shadow size    : %d\n", rcap.shadow_size);
    fprintf(filewrite, "Bump size      : %d\n", rcap.bump_size);
    fprintf(filewrite, "Bump height    : %d\n", rcap.bump_height);
    fprintf(filewrite, "Bump dampen    : %4.2f\n", rcap.bump_dampen);
    fprintf(filewrite, "Weight         : %d\n", rcap.weight);
    fprintf(filewrite, "Jump power     : %4.2f\n", rcap.jump);
    fprintf(filewrite, "Jump number    : %d\n", rcap.jumpnumberreset);
    fprintf(filewrite, "Sneak speed    : %d\n", rcap.spd_sneak);
    fprintf(filewrite, "Walk speed     : %d\n", rcap.spd_walk);
    fprintf(filewrite, "Run speed      : %d\n", rcap.spd_run);
    fprintf(filewrite, "Fly to height  : %d\n", rcap.flyheight);
    fprintf(filewrite, "Flashing AND   : %d\n", rcap.flashand);
    fprintf(filewrite, "Alpha blending : %d\n", rcap.alpha);
    fprintf(filewrite, "Light blending : %d\n", rcap.light);
    ftruthf(filewrite, "Transfer blend : ", rcap.transferblend);
    fprintf(filewrite, "Sheen          : %d\n", rcap.sheen);
    ftruthf(filewrite, "Phong mapping  : ", rcap.enviro);
    fprintf(filewrite, "Texture X add  : %4.2f\n", rcap.off_vel.s);
    fprintf(filewrite, "Texture Y add  : %4.2f\n", rcap.off_vel.t);
    ftruthf(filewrite, "Sticky butt    : ", rcap.stickybutt);
    fprintf(filewrite, "\n");

    // Invulnerability data
    ftruthf(filewrite, "Invictus       : ", rcap.invictus);
    fprintf(filewrite, "NonI facing    : %d\n", rcap.nframefacing);
    fprintf(filewrite, "NonI angle     : %d\n", rcap.nframeangle);
    fprintf(filewrite, "I facing       : %d\n", rcap.iframefacing);
    fprintf(filewrite, "I angle        : %d\n", rcap.iframeangle);
    fprintf(filewrite, "\n");

    // Skin defenses
    fprintf(filewrite, "Base defense   : %3d %3d %3d %3d\n", 0xFF-rcap.defense[0], 0xFF-rcap.defense[1],
            0xFF-rcap.defense[2], 0xFF-rcap.defense[3]);
    damagetype = 0;
    while (damagetype < DAMAGE_COUNT)
    {
      fprintf(filewrite, "%c damage shift : %3d %3d %3d %3d\n", types[damagetype],
              rcap.damagemodifier[damagetype][0]&DAMAGE_SHIFT,
              rcap.damagemodifier[damagetype][1]&DAMAGE_SHIFT,
              rcap.damagemodifier[damagetype][2]&DAMAGE_SHIFT,
              rcap.damagemodifier[damagetype][3]&DAMAGE_SHIFT);
      damagetype++;
    }
    damagetype = 0;
    while (damagetype < DAMAGE_COUNT)
    {
      skin = 0;
      while (skin < 4)
      {
        codes[skin] = 'F';
        if (rcap.damagemodifier[damagetype][skin]&DAMAGE_INVERT)
          codes[skin] = 'T';
        if (rcap.damagemodifier[damagetype][skin]&DAMAGE_CHARGE)
          codes[skin] = 'C';
        skin++;
      }
      fprintf(filewrite, "%c damage code  : %3c %3c %3c %3c\n", types[damagetype], codes[0], codes[1], codes[2], codes[3]);
      damagetype++;
    }
    fprintf(filewrite, "Acceleration   : %3.0f %3.0f %3.0f %3.0f\n", rcap.maxaccel[0]*80,
            rcap.maxaccel[1]*80,
            rcap.maxaccel[2]*80,
            rcap.maxaccel[3]*80);
    fprintf(filewrite, "\n");

    // Experience and level data
    fprintf(filewrite, "EXP for 2nd    : %d\n", rcap.experienceforlevel[1]);
    fprintf(filewrite, "EXP for 3rd    : %d\n", rcap.experienceforlevel[2]);
    fprintf(filewrite, "EXP for 4th    : %d\n", rcap.experienceforlevel[3]);
    fprintf(filewrite, "EXP for 5th    : %d\n", rcap.experienceforlevel[4]);
    fprintf(filewrite, "EXP for 6th    : %d\n", rcap.experienceforlevel[5]);
    fprintf(filewrite, "Starting EXP   : %d\n", rchr.experience);
    fprintf(filewrite, "EXP worth      : %d\n", rcap.experienceworth);
    fprintf(filewrite, "EXP exchange   : %5.3f\n", rcap.experienceexchange);
    fprintf(filewrite, "EXPSECRET      : %4.2f\n", rcap.experiencerate[0]);
    fprintf(filewrite, "EXPQUEST       : %4.2f\n", rcap.experiencerate[1]);
    fprintf(filewrite, "EXPDARE        : %4.2f\n", rcap.experiencerate[2]);
    fprintf(filewrite, "EXPKILL        : %4.2f\n", rcap.experiencerate[3]);
    fprintf(filewrite, "EXPMURDER      : %4.2f\n", rcap.experiencerate[4]);
    fprintf(filewrite, "EXPREVENGE     : %4.2f\n", rcap.experiencerate[5]);
    fprintf(filewrite, "EXPTEAMWORK    : %4.2f\n", rcap.experiencerate[6]);
    fprintf(filewrite, "EXPROLEPLAY    : %4.2f\n", rcap.experiencerate[7]);
    fprintf(filewrite, "\n");

    // IDSZ identification tags
    IDSZ::convert(rcap.idsz[0]);
    fprintf(filewrite, "IDSZ Parent    : [%s]\n", valueidsz);
    IDSZ::convert(rcap.idsz[1]);
    fprintf(filewrite, "IDSZ Type      : [%s]\n", valueidsz);
    IDSZ::convert(rcap.idsz[2]);
    fprintf(filewrite, "IDSZ Skill     : [%s]\n", valueidsz);
    IDSZ::convert(rcap.idsz[3]);
    fprintf(filewrite, "IDSZ Special   : [%s]\n", valueidsz);
    IDSZ::convert(rcap.idsz[4]);
    fprintf(filewrite, "IDSZ Hate      : [%s]\n", valueidsz);
    IDSZ::convert(rcap.idsz[5]);
    fprintf(filewrite, "IDSZ Vulnie    : [%s]\n", valueidsz);
    fprintf(filewrite, "\n");

    // Item and damage flags
    ftruthf(filewrite, "Is an item     : ", rcap.is_item);
    ftruthf(filewrite, "Is a mount     : ", rcap.is_mount);
    ftruthf(filewrite, "Is stackable   : ", rcap.isstackable);
    ftruthf(filewrite, "Name known     : ", rchr.nameknown);
    ftruthf(filewrite, "Usage known    : ", rcap.usageknown);
    ftruthf(filewrite, "Is exportable  : ", rcap.cancarrytonextmodule);
    ftruthf(filewrite, "Requires skill : ", rcap.needskillidtouse);
    ftruthf(filewrite, "Is platform    : ", rcap.is_platform);
    ftruthf(filewrite, "Collects money : ", rcap.cangrabmoney);
    ftruthf(filewrite, "Can open stuff : ", rcap.canopenstuff);
    fprintf(filewrite, "\n");

    // Other item and damage stuff
    fdamagf(filewrite, "Damage type    : ", rcap.damagetargettype);
    factiof(filewrite, "Attack type    : ", rcap.weaponaction);
    fprintf(filewrite, "\n");

    // Particle attachments
    fprintf(filewrite, "Attached parts : %d\n", rcap.attachedprtamount);
    fdamagf(filewrite, "Reaffirm type  : ", rcap.attachedprtreaffirmdamagetype);
    fprintf(filewrite, "Particle type  : %d\n", rcap.attachedprttype);
    fprintf(filewrite, "\n");

    // Character hands
    ftruthf(filewrite, "Left valid     : ", rcap.slot_valid[SLOT_LEFT]);
    ftruthf(filewrite, "Right valid    : ", rcap.slot_valid[SLOT_RIGHT]);
    fprintf(filewrite, "\n");

    // Particle spawning on attack
    ftruthf(filewrite, "Part on weapon : ", rcap.attackattached);
    fprintf(filewrite, "Part type      : %d\n", rcap.attackprttype);
    fprintf(filewrite, "\n");

    // Particle spawning for GoPoof
    fprintf(filewrite, "Poof amount    : %d\n", rcap.gopoofprtamount);
    fprintf(filewrite, "Facing add     : %d\n", rcap.gopoofprtfacingadd);
    fprintf(filewrite, "Part type      : %d\n", rcap.gopoofprttype);
    fprintf(filewrite, "\n");

    // Particle spawning for blood
    ftruthf(filewrite, "Blood valid    : ", rcap.bloodvalid);
    fprintf(filewrite, "Part type      : %d\n", rcap.bloodprttype);
    fprintf(filewrite, "\n");

    // Extra stuff
    ftruthf(filewrite, "Waterwalking   : ", rcap.waterwalk);
    fprintf(filewrite, "Bounce dampen  : %5.3f\n", rcap.dampen);
    fprintf(filewrite, "\n");

    // More stuff
    fprintf(filewrite, "Life healing   : %5.3f\n", rcap.lifeheal/float(0x0100));
    fprintf(filewrite, "Mana cost      : %5.3f\n", rcap.manacost/float(0x0100));
    fprintf(filewrite, "Life return    : %d\n", rcap.lifereturn);
    fprintf(filewrite, "Stopped by     : %d\n", rcap.stoppedby);
    funderf(filewrite, "Skin 0 name    : ", rcap.skinname[0]);
    funderf(filewrite, "Skin 1 name    : ", rcap.skinname[1]);
    funderf(filewrite, "Skin 2 name    : ", rcap.skinname[2]);
    funderf(filewrite, "Skin 3 name    : ", rcap.skinname[3]);
    fprintf(filewrite, "Skin 0 cost    : %d\n", rcap.skincost[0]);
    fprintf(filewrite, "Skin 1 cost    : %d\n", rcap.skincost[1]);
    fprintf(filewrite, "Skin 2 cost    : %d\n", rcap.skincost[2]);
    fprintf(filewrite, "Skin 3 cost    : %d\n", rcap.skincost[3]);
    fprintf(filewrite, "STR dampen     : %5.3f\n", rcap.strengthdampen);
    fprintf(filewrite, "\n");

    // Another memory lapse
    ftruthf(filewrite, "No rider attak : ", true-rcap.ridercanattack);
    ftruthf(filewrite, "Can be dazed   : ", rcap.canbedazed);
    ftruthf(filewrite, "Can be grogged : ", rcap.canbegrogged);
    fprintf(filewrite, "NOT USED       : 0\n");
    fprintf(filewrite, "NOT USED       : 0\n");
    ftruthf(filewrite, "Can see invisi : ", rcap.canseeinvisible);
    fprintf(filewrite, "Kursed chance  : %d\n", rchr.iskursed*100);
    fprintf(filewrite, "Footfall sound : %d\n", rcap.wavefootfall);
    fprintf(filewrite, "Jump sound     : %d\n", rcap.wavejump);
    fprintf(filewrite, "\n");

    // Expansions
    fprintf(filewrite, ":[GOLD] %d\n", rchr.money);
    if (rcap.skindressy&1)
      fprintf(filewrite, ":[DRES] 0\n");
    if (rcap.skindressy&2)
      fprintf(filewrite, ":[DRES] 1\n");
    if (rcap.skindressy&4)
      fprintf(filewrite, ":[DRES] 2\n");
    if (rcap.skindressy&8)
      fprintf(filewrite, ":[DRES] 3\n");
    if (rcap.resistbumpspawn)
      fprintf(filewrite, ":[STUK] 0\n");
    if (rcap.istoobig)
      fprintf(filewrite, ":[PACK] 0\n");
    if (!rcap.reflect)
      fprintf(filewrite, ":[VAMP] 1\n");
    if (rcap.alwaysdraw)
      fprintf(filewrite, ":[DRAW] 1\n");
    if (rcap.isranged)
      fprintf(filewrite, ":[RANG] 1\n");
    if (rcap.hide_state!=NOHIDE)
      fprintf(filewrite, ":[HIDE] %d\n", rcap.hide_state);
    if (rcap.is_equipment)
      fprintf(filewrite, ":[EQUI] 1\n");
    if (rcap.bump_size_big==(rcap.bump_size<<1))
      fprintf(filewrite, ":[SQUA] 1\n");
    if (rcap.show_icon != rcap.usageknown)
      fprintf(filewrite, ":[ICON] %d\n", rcap.show_icon);
    if (rcap.forceshadow)
      fprintf(filewrite, ":[SHAD] 1\n");
    if (rcap.canseekurse)
      fprintf(filewrite, ":[CKUR] 1\n");
    if (rcap.ripple == rcap.is_item)
      fprintf(filewrite, ":[RIPP] %d\n", rcap.ripple);
    //fprintf(filewrite, ":[PLAT] %d\n", rcap.canuse_platforms);
    fprintf(filewrite, ":[SKIN] %d\n", rchr.skin);
    fprintf(filewrite, ":[CONT] %d\n", rchr.ai.content);
    fprintf(filewrite, ":[STAT] %d\n", rchr.ai.state);
    fprintf(filewrite, ":[LEVL] %d\n", rchr.experiencelevel);
    fclose(filewrite);
  }
}

//--------------------------------------------------------------------------------------------
void export_one_character_skin(char *szSaveName, Uint16 character)
{
  // ZZ> This function creates a skin.txt file for the given character.
  FILE* filewrite;

  // General stuff
  Character & rchr = ChrList[character];
  Mad       & rmad = rchr.getMad();

  // Open the file
  filewrite = fopen(szSaveName, "w");
  if (filewrite)
  {
    fprintf(filewrite, "This file is used only by the import menu\n");
    fprintf(filewrite, ": %d\n", (rchr.skin)&3);
    fclose(filewrite);
  }
}

//--------------------------------------------------------------------------------------------
Uint32 Cap_List::load_one_cap(const char *szLoadName)
{
  // ZZ> This function fills a character profile with data from data.txt, returning
  // the model slot that the profile was stuck into.  It may cause the program
  // to abort if bad things happen.
  FILE* fileread;
  int iTmp;
  char cTmp;
  int damagetype, level, xptype;
  int idsz_count;
  IDSZ idsz, test;
  Uint32 cap_ref;

  if (NULL==szLoadName) return Cap_List::INVALID;

  // Open the file
  fileread = fopen(szLoadName, "r");
  if(NULL==fileread) general_error(0, 0, "DATA.TXT NOT FOUND");

  // for debugging
  globalname = szLoadName;


  // do we have a valid value?
  cap_ref = get_free();
  if(Cap_List::INVALID == cap_ref) return cap_ref;

  // make life simple
  Cap & rcap = CapList[ CAP_REF(cap_ref) ];


  // Read in the cap_ref slot
  rcap.slot = get_next_int(fileread);
  if (rcap.slot < 0)
  {
    if (m_importobject < 0)
    {
      fprintf(stderr, "UNRESOLVABLE OBJECT SLOT \"%s\", (%d, %d)", szLoadName, rcap.slot, m_importobject);
    }
    else
    {
      rcap.slot = m_importobject;
    }
  }

  // Read in the real general data
  goto_colon(fileread);  get_name(fileread, rcap.classname);

  // Make sure we don't load over an existing cap_ref
  if (rcap.slot > 0 && Profile_List::INVALID != ProfileList.slot_list[rcap.slot])
  {
    fprintf(stderr, "<--MODEL SLOT USED TWICE \"%s\"(%s)\n", szLoadName, rcap.classname);
    fclose(fileread);
    return INVALID;
  }

  // Light cheat
  rcap.uniformlit = get_next_bool(fileread);

  // Ammo
  rcap.ammomax = get_next_int(fileread);
  rcap.ammo = get_next_int(fileread);

  // Gender
  goto_colon(fileread);  cTmp = get_first_letter(fileread);
  rcap.gender = GENOTHER;
  if (cTmp=='F' || cTmp=='f')  rcap.gender = GENFEMALE;
  if (cTmp=='M' || cTmp=='m')  rcap.gender = GENMALE;
  if (cTmp=='R' || cTmp=='r')  rcap.gender = GENRANDOM;

  // Read in the cap_ref stats
  rcap.lifecolor = get_next_int(fileread);
  rcap.manacolor = get_next_int(fileread);
  read_next_pair(fileread, rcap.lifebase, rcap.liferand);
  read_next_pair(fileread, rcap.lifeperlevelbase, rcap.lifeperlevelrand);
  read_next_pair(fileread, rcap.manabase, rcap.manarand);
  read_next_pair(fileread, rcap.manaperlevelbase, rcap.manaperlevelrand);
  read_next_pair(fileread, rcap.manareturnbase, rcap.manareturnrand);
  read_next_pair(fileread, rcap.manareturnperlevelbase, rcap.manareturnperlevelrand);
  read_next_pair(fileread, rcap.manaflowbase, rcap.manaflowrand);
  read_next_pair(fileread, rcap.manaflowperlevelbase, rcap.manaflowperlevelrand);
  read_next_pair(fileread, rcap.strengthbase, rcap.strengthrand);
  read_next_pair(fileread, rcap.strengthperlevelbase, rcap.strengthperlevelrand);
  read_next_pair(fileread, rcap.wisdombase, rcap.wisdomrand);
  read_next_pair(fileread, rcap.wisdomperlevelbase, rcap.wisdomperlevelrand);
  read_next_pair(fileread, rcap.intelligencebase, rcap.intelligencerand);
  read_next_pair(fileread, rcap.intelligenceperlevelbase, rcap.intelligenceperlevelrand);
  read_next_pair(fileread, rcap.dexteritybase, rcap.dexterityrand);
  read_next_pair(fileread, rcap.dexterityperlevelbase, rcap.dexterityperlevelrand);

  // More physical attributes
  rcap.scale_horiz = rcap.scale_vert = get_next_float(fileread);
  rcap.sizeperlevel = get_next_float(fileread);
  rcap.shadow_size = get_next_int(fileread);
  rcap.bump_size = get_next_int(fileread);
  rcap.bump_height = get_next_int(fileread);
  rcap.bump_dampen = get_next_float(fileread);
  rcap.weight = get_next_int(fileread);             if(rcap.weight==0xFF) rcap.weight = -1;
  rcap.jump = get_next_float(fileread);
  rcap.jumpnumberreset = get_next_int(fileread);
  rcap.spd_sneak = get_next_int(fileread)*2;
  rcap.spd_walk = get_next_int(fileread)*2;
  rcap.spd_run = get_next_int(fileread)*2;
  rcap.flyheight = get_next_int(fileread);
  rcap.flashand = get_next_int(fileread);
  rcap.alpha = get_next_int(fileread);
  rcap.light = get_next_int(fileread);
  rcap.transferblend = get_next_bool(fileread);
  rcap.sheen = get_next_int(fileread);
  rcap.enviro = get_next_bool(fileread);
  rcap.off_vel.s = get_next_float(fileread);
  rcap.off_vel.t = get_next_float(fileread);
  rcap.stickybutt = get_next_bool(fileread);

  // Invulnerability data
  rcap.invictus = get_next_bool(fileread);
  rcap.nframefacing = get_next_int(fileread);
  rcap.nframeangle = get_next_int(fileread);
  rcap.iframefacing = get_next_int(fileread);
  rcap.iframeangle = get_next_int(fileread);
  // Resist burning and stuck arrows with nframe angle of 1 or more
  if (rcap.nframeangle > 0)
  {
    if (rcap.nframeangle == 1)
    {
      rcap.nframeangle = 0;
    }
  }

  // Skin defenses ( 4 skins )
  goto_colon(fileread);
  rcap.defense[0] = 0xFF-get_int(fileread);
  rcap.defense[1] = 0xFF-get_int(fileread);
  rcap.defense[2] = 0xFF-get_int(fileread);
  rcap.defense[3] = 0xFF-get_int(fileread);
  damagetype = 0;
  while (damagetype < DAMAGE_COUNT)
  {
    goto_colon(fileread);
    rcap.damagemodifier[damagetype][0] = get_int(fileread);
    rcap.damagemodifier[damagetype][1] = get_int(fileread);
    rcap.damagemodifier[damagetype][2] = get_int(fileread);
    rcap.damagemodifier[damagetype][3] = get_int(fileread);
    damagetype++;
  }

  for (damagetype = 0; damagetype < DAMAGE_COUNT; damagetype++)
  {
    goto_colon(fileread);
    for(int i=0; i<4; i++)
    {
      rcap.damagemodifier[damagetype][i] |= get_damage_mods(fileread);
    }
  }
  goto_colon(fileread);
  for(int i=0; i<4; i++)
  {
    rcap.maxaccel[i] = get_float(fileread)/80.0;
  }


  // Experience and level data
  rcap.experienceforlevel[0] = 0;
  level = 1;
  while (level < MAXLEVEL)
  {
    rcap.experienceforlevel[level] = get_next_int(fileread);
    level++;
  }
  read_next_pair(fileread, rcap.experiencebase, rcap.experiencerand);
  rcap.experiencebase >>= 8;
  rcap.experiencerand >>= 8;
  if (rcap.experiencerand < 1)  rcap.experiencerand = 1;

  rcap.experienceworth    = get_next_int(fileread);
  rcap.experienceexchange = get_next_float(fileread);
  xptype = 0;
  while (xptype < XP_COUNT)
  {
    rcap.experiencerate[xptype] = 0.001 + get_next_float(fileread);
    xptype++;
  }

  // IDSZ tags
  idsz_count = 0;
  while (idsz_count < IDSZ_COUNT)
  {
    rcap.idsz[idsz_count] =  get_next_idsz(fileread);
    idsz_count++;
  }

  // Item and damage flags
  rcap.is_item = get_next_bool(fileread); rcap.ripple = !rcap.is_item;
  rcap.is_mount = get_next_bool(fileread);
  rcap.isstackable = get_next_bool(fileread);
  rcap.nameknown = get_next_bool(fileread);
  rcap.usageknown = get_next_bool(fileread);
  rcap.cancarrytonextmodule = get_next_bool(fileread);
  rcap.needskillidtouse = get_next_bool(fileread);
  rcap.is_platform = get_next_bool(fileread);
  rcap.cangrabmoney = get_next_bool(fileread);
  rcap.canopenstuff = get_next_bool(fileread);

  // More item and damage stuff
  goto_colon(fileread);  rcap.damagetargettype = get_damage_type(fileread);
  goto_colon(fileread);  rcap.weaponaction     = Mad::what_action(get_first_letter(fileread));

  // Particle attachments
  rcap.attachedprtamount = get_next_int(fileread);
  goto_colon(fileread);  rcap.attachedprtreaffirmdamagetype =  get_damage_type(fileread);
  rcap.attachedprttype = get_next_int(fileread);

  // Character hands
  rcap.slot_valid[SLOT_LEFT] = get_next_bool(fileread);
  rcap.slot_valid[SLOT_RIGHT] = get_next_bool(fileread);

  // Attack order ( weapon )
  rcap.attackattached = get_next_bool(fileread);
  rcap.attackprttype  = get_next_int(fileread);

  // GoPoof
  rcap.gopoofprtamount = get_next_int(fileread);
  rcap.gopoofprtfacingadd = get_next_int(fileread);
  rcap.gopoofprttype = get_next_int(fileread);

  // Blood
  goto_colon(fileread);  cTmp = get_first_letter(fileread);
  rcap.bloodvalid = false;
  if (cTmp=='T' || cTmp=='t')  rcap.bloodvalid = true;
  if (cTmp=='U' || cTmp=='u')  rcap.bloodvalid = ULTRABLOODY;
  rcap.bloodprttype = get_next_int(fileread);

  // Stuff I forgot
  rcap.waterwalk = get_next_bool(fileread);
  rcap.dampen = get_next_float(fileread);

  // More stuff I forgot
  rcap.lifeheal = 0x0100*get_next_float(fileread);
  rcap.manacost = 0x0100*get_next_float(fileread);
  rcap.lifereturn = get_next_int(fileread);
  rcap.stoppedby = MESHFX_IMPASS | Uint8(get_next_int(fileread));
  goto_colon(fileread);  get_name(fileread, rcap.skinname[0]);
  goto_colon(fileread);  get_name(fileread, rcap.skinname[1]);
  goto_colon(fileread);  get_name(fileread, rcap.skinname[2]);
  goto_colon(fileread);  get_name(fileread, rcap.skinname[3]);
  rcap.skincost[0] = get_next_int(fileread);
  rcap.skincost[1] = get_next_int(fileread);
  rcap.skincost[2] = get_next_int(fileread);
  rcap.skincost[3] = get_next_int(fileread);
  rcap.strengthdampen = get_next_float(fileread);

  // Another memory lapse
  rcap.ridercanattack = !get_next_bool(fileread);
  rcap.canbedazed = get_next_bool(fileread);
  rcap.canbegrogged = get_next_bool(fileread);
  goto_colon(fileread);  // !!!BAD!!! Life add
  goto_colon(fileread);  // !!!BAD!!! Mana add
  rcap.canseeinvisible = get_next_bool(fileread);
  rcap.kursechance = get_next_int(fileread);
  iTmp = get_next_int(fileread);
  if (iTmp < -1) iTmp = -1;
  if (iTmp > MAXWAVE-1) iTmp = MAXWAVE-1;
  rcap.wavefootfall = iTmp;
  iTmp = get_next_int(fileread);
  if (iTmp < -1) iTmp = -1;
  if (iTmp > MAXWAVE-1) iTmp = MAXWAVE-1;
  rcap.wavejump = iTmp;

  // Clear expansions...
  rcap.skindressy = false;
  rcap.resistbumpspawn = false;
  rcap.istoobig = false;
  rcap.reflect = true;
  rcap.alwaysdraw = false;
  rcap.isranged = false;
  rcap.hide_state = NOHIDE;
  rcap.is_equipment = false;
  rcap.bump_size_big = rcap.bump_size+(rcap.bump_size>>1);
  rcap.canseekurse = false;
  rcap.money = 0;
  rcap.show_icon = rcap.usageknown;
  rcap.forceshadow = false;
  rcap.skinoverride = NOSKINOVERRIDE;
  rcap.contentoverride = 0;
  rcap.stateoverride = 0;
  rcap.leveloverride = 0;

  // Read expansions
  while (goto_colon_yesno(fileread))
  {
    idsz = get_idsz(fileread);
    fscanf(fileread, "%c%d", &cTmp, &iTmp);

    if (idsz == IDSZ("DRES"))  rcap.skindressy |= 1<<iTmp;
    else if (idsz == IDSZ("GOLD"))  rcap.money = iTmp;
    else if (idsz == IDSZ("STUK"))  rcap.resistbumpspawn = (iTmp==0);
    else if (idsz == IDSZ("PACK"))  rcap.istoobig = (iTmp==0);
    else if (idsz == IDSZ("VAMP"))  rcap.reflect = (iTmp==0);
    else if (idsz == IDSZ("DRAW"))  rcap.alwaysdraw = (iTmp!=0);
    else if (idsz == IDSZ("RANG"))  rcap.isranged = (iTmp!=0);
    else if (idsz == IDSZ("HIDE"))  rcap.hide_state = iTmp;
    else if (idsz == IDSZ("EQUI"))  rcap.is_equipment = (iTmp!=0);
    else if (idsz == IDSZ("SQUA"))  rcap.bump_size_big = rcap.bump_size<<1;
    else if (idsz == IDSZ("ICON"))  rcap.show_icon = (iTmp!=0);
    else if (idsz == IDSZ("SHAD"))  rcap.forceshadow = (iTmp!=0);
    else if (idsz == IDSZ("CKUR"))  rcap.canseekurse = (iTmp!=0);
    else if (idsz == IDSZ("SKIN"))  rcap.skinoverride = iTmp&3;
    else if (idsz == IDSZ("CONT"))  rcap.contentoverride = iTmp;
    else if (idsz == IDSZ("STAT"))  rcap.stateoverride = iTmp;
    else if (idsz == IDSZ("LEVL"))  rcap.leveloverride = iTmp;
    //else if (idsz == IDSZ("PLAT"))  rcap.canuse_platforms = (iTmp!=0);
    else if (idsz == IDSZ("RIPP"))  rcap.ripple = (iTmp!=0);
  }
  fclose(fileread);

  return cap_ref;
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
    skin = get_int(fileread) & 3;
    fclose(fileread);
  }
  return skin;
}

//--------------------------------------------------------------------------------------------
void check_player_import(char *dirname)
{
  // ZZ> This function figures out which players may be imported, and loads basic
  //     data for each
  char searchname[0x80];
  char filename[0x80];
  int skin;
  bool keeplooking;
  const char *foundfile;

  // Set up...
  numloadplayer = 0;

  // Search for all objects
  sprintf(searchname, "%s/*.obj", dirname);
  foundfile = fs_findFirstFile(dirname, "obj");
  keeplooking = 1;
  if (foundfile != NULL)
  {
    while (keeplooking && numloadplayer < MAXLOADPLAYER)
    {
      //fprintf(stderr,"foundfile=%s keeplooking=%d numload=%d/%d\n",foundfile,keeplooking,numloadplayer,MAXLOADPLAYER);
      ProfileList.prime_names();
      sprintf(loadplayerdir[numloadplayer], "%s", foundfile);

      sprintf(filename, "%s/%s/skin.txt", dirname, foundfile);
      skin = get_skin(filename);

      //snprintf(filename, 0x80, "%s/%s/tris.md2", dirname, foundfile);
      //MadList[numloadplayer].load(filename);

      sprintf(filename, "%s/%s/icon%d.bmp", dirname, foundfile, skin);
      IconList.load_one_icon(filename);

      sprintf(filename, "%s/%s/naming.txt", dirname, foundfile);
      ProfileList[0].read_naming(filename);
      ProfileList[0].naming_names();
      sprintf(loadplayername[numloadplayer], "%s", GChops.name);

      numloadplayer++;

      foundfile=fs_findNextFile();
      if (foundfile==NULL) keeplooking=0; else keeplooking=1;
    }
  }
  fs_findClose();

  IconList.null = IconList.load_one_icon("basicdat/nullicon.bmp");
  IconList.keyb = IconList.load_one_icon("basicdat/Icon_keyb.bmp");
  IconList.mous = IconList.load_one_icon("basicdat/Icon_mous.bmp");
  IconList.joya = IconList.load_one_icon("basicdat/Icon_joya.bmp");
  IconList.joyb = IconList.load_one_icon("basicdat/Icon_joyb.bmp");

  GKeyb.player   = 0;
  GMous.player   = 0;
  GJoy[0].player = 0;
  GJoy[1].player = 0;
}


const JF::MD2_Frame * Character::getFrameLast(JF::MD2_Model * m)
{
  JF::MD2_Model * mdl = (NULL!=m) ? m : getMD2();
  if(NULL==mdl) return NULL;
  return mdl->getFrame(ani.last);
}

const JF::MD2_Frame * Character::getFrame(JF::MD2_Model * m)
{
  JF::MD2_Model * mdl = (NULL!=m) ? m : getMD2();
  if(NULL==mdl) return NULL;
  return mdl->getFrame(ani.frame);
}

JF::MD2_Model * Character::getMD2()
{
  if( INVALID_MODEL(model) ) return NULL;

  return &MadList[ProfileList[model].mad_ref];
};


Cap & Character::getCap()        { return CapList[ProfileList[model].cap_ref]; }
Mad & Character::getMad()        { return MadList[ProfileList[model].mad_ref]; }
Eve & Character::getEve()        { return EveList[ProfileList[model].eve_ref]; }
Script_Info & Character::getAI() { return ScrList[ProfileList[model].scr_ref]; }
Pip & Character::getPip(int i)   { return PipList[ProfileList[model].prtpip[i]]; }

//--------------------------------------------------------------------------------------------

float terp_dir(Uint16 majordir, float dx2, float dy2)
{
  // BB > This figures a rotation rate based on the cross proct between the current and
  //      desired vectors.  It will also work in 3D.

  if(ABS(dx2) + ABS(dy2) < 1e-6) return 0;

  float dx1 = cos_tab[majordir>>2];
  float dy1 = sin_tab[majordir>>2];

  float nrm = sqrtf(dx2*dx2 + dy2*dy2);
  float cp = (dx1*dy2 - dy1*dx2) / nrm;

  // TO DO : I want to increase the rotation rate if dot product between the vectors is negative
  //         but the way I was doing it makes the system unstable and introduces a latge amount
  //         of jittering in the angle

  //float dp = (dx1*dx2 + dy1*dy2) / nrm;

  float retval = -cp; //-(MAX(0,-dp) * SGN(cp) + cp);

  return retval;
}

//--------------------------------------------------------------------------------------------
//Uint16 terp_dir_fast(Uint16 majordir, float dx2, float dy2)
//{
//  // ZZ> This function returns a direction between the major and minor ones, closer
//  //     to the major, but not by much.  Makes turning faster.
//
//  const float rot0 = 5000;
//
//  float dx1 = cos_tab[majordir>>2];
//  float dy1 = sin_tab[majordir>>2];
//
//  float nrm = sqrtf(dx2*dx2 + dy2*dy2);
//  float cp = (dx1*dy2 - dy1*dx2) / nrm;
//  float dp = (dx1*dx2 + dy1*dy2) / nrm;
//
//  float rot1 = rot0/10 * MAX(0,-dp) * SGN(cp) + rot0 * cp;
//
//  return majordir - rot1;
//}


//void Platform::place_item(Uint32 platform, Uint32 item)
//{
//  if(INVALID_CHR(platform) || INVALID_CHR(item)) return;
//  if(platform == item) return;
//
//  Character & rplat = ChrList[platform];
//  Character & ritem = ChrList[item];
//
//  if(!rplat.is_platform || !ritem.canuse_platforms) return;
//
//  rplat.holding_weight    += ritem.weight;
//  ritem.on_which_platform = platform;
//};
//
//void Platform::remove_item(Uint32 platform, Uint32 item)
//{
//  if(INVALID_CHR(platform) || INVALID_CHR(item)) return;
//  if(platform == item) return;
//
//  Character & rplat = ChrList[platform];
//  Character & ritem = ChrList[item];
//
//  rplat.holding_weight    -= ritem.weight;
//  ritem.on_which_platform = Character_List::INVALID;
//  ritem.platform_level    = 0;
//};

bool Character::hitmesh(Mesh & msh, vec3_t & normal)
{
  return Physics_Accumulator::hitmesh(msh, level, normal);
}

bool Particle::hitmesh(Mesh & msh, vec3_t & normal)
{
  return Physics_Accumulator::hitmesh(msh, level, normal);
}

bool Character::hitawall(Mesh & msh, vec3_t & normal)
{
  return Physics_Accumulator::hitawall(msh, stoppedby, normal);
}

bool Particle::hitawall(Mesh & msh, vec3_t & normal)
{
  Uint32 stoppedby = MESHFX_IMPASS | (PipList[pip].bumpmoney ? MESHFX_WALL : 0);
 
  return Physics_Accumulator::hitawall(msh, stoppedby, normal);
}

bool Character::inawall(Mesh & msh)
{
  return Physics_Accumulator::inawall(msh, stoppedby);
}

bool Particle::inawall(Mesh & msh)
{
  Uint32 stoppedby = MESHFX_IMPASS | (PipList[pip].bumpmoney ? MESHFX_WALL : 0);
 
  return Physics_Accumulator::inawall(msh, stoppedby);
}

//--------------------------------------------------------------------------------------------
#define INV_SQR_2 0.70710678118654752440084436210485
bool Physics_Accumulator::inawall(Mesh & msh, Uint32 stoppedby)
{
  // ZZ> This function returns nonzero if the character hit a wall, the floor, or
  //     a boundary the character is not allowed to cross

  if(0!=stoppedby) return false;

  if( GMesh.has_flags(pos.x, pos.y, stoppedby) )
    return true;

  //if it is completely unstoppable...
  float br  = calc_bump_size;
  float brb = calc_bump_size_big;

  // every other means of interacting with the mesh is averaged
  if( GMesh.has_flags(pos.x,pos.y-brb, stoppedby) )
    return true;
  if( GMesh.has_flags(pos.x,pos.y+brb, stoppedby) )
    return true;
  if( GMesh.has_flags(pos.x+brb,pos.y, stoppedby) )
    return true;
  if( GMesh.has_flags(pos.x-brb,pos.y, stoppedby) )
    return true;

  if( GMesh.has_flags(pos.x-br,pos.y-br, stoppedby) )
    return true;
  if( GMesh.has_flags(pos.x-br,pos.y+br, stoppedby) )
    return true;
  if( GMesh.has_flags(pos.x+br,pos.y-br, stoppedby) )
    return true;
  if( GMesh.has_flags(pos.x+br,pos.y+br, stoppedby) )
    return true;

  return false;
}

//--------------------------------------------------------------------------------------------
#define INV_SQR_2 0.70710678118654752440084436210485
bool Physics_Accumulator::hitawall(Mesh & msh, Uint32 stoppedby, vec3_t & normal)
{
  // ZZ> This function returns nonzero if the character hit a wall, the floor, or
  //     a boundary the character is not allowed to cross

  //if it is completely unstoppable...
  if(0==stoppedby) return false;

  if( GMesh.has_flags(pos.x,pos.y, stoppedby) )
  { 
    // being inside in a forbidden region trumps all else
    // set the normal to be perpendicular to your velocity
    normal.x = -(pos.y-old.pos.y) - vel.y;
    normal.y =   pos.x-old.pos.x  + vel.x;
    normal.z = 0;

    return normalize_iterative(normal);
  }

  int sum = 0;
  float br  = bump_size     / 2.0f;
  float brb = bump_size_big / 2.0f;

  normal.x = normal.y = normal.z = 0;

  // every other means of interacting with the mesh is averaged
  if( GMesh.has_flags(pos.x,pos.y-brb, stoppedby) )
  { 
    normal.y += 1;
    sum++;
  }

  if( GMesh.has_flags(pos.x,pos.y+brb, stoppedby) )
  { 
    normal.y += -1;
    sum++;
  }

  if( GMesh.has_flags(pos.x+brb,pos.y, stoppedby) )
  { 
    normal.x +=-1;
    sum++;
  }

  if( GMesh.has_flags(pos.x-brb,pos.y, stoppedby) )
  { 
    normal.x += 1;
    sum++;
  }


  if( GMesh.has_flags(pos.x-br,pos.y-br, stoppedby) )
  { 
    normal.x += INV_SQR_2;
    normal.y += INV_SQR_2;
    sum++;
  }

  if( GMesh.has_flags(pos.x-br,pos.y+br, stoppedby) )
  { 
    normal.x += INV_SQR_2;
    normal.y +=-INV_SQR_2;
    sum++;
  }

  if( GMesh.has_flags(pos.x+br,pos.y-br, stoppedby) )
  { 
    normal.x +=-INV_SQR_2;
    normal.y += INV_SQR_2;
    sum++;
  }

  if( GMesh.has_flags(pos.x+br,pos.y+br, stoppedby) )
  { 
    normal.x +=-INV_SQR_2;
    normal.y +=-INV_SQR_2;
    sum++;
  }

  if(sum>1)
  {
    float factor = dist_squared(normal);
    if(factor==0) 
    {
      normal.z = -SGN(GPhys.gravity);
      return false;
    }

    normal /= factor;
  };


  return sum>0;
}

//--------------------------------------------------------------------------------------------
bool Physics_Accumulator::hitmesh(Mesh & msh, float level, vec3_t & normal)
{

  if(pos.z < level && old.pos.z > level)
  {
    return msh.simple_normal(pos, normal);
  }

  return false;
}

//--------------------------------------------------------------------------------------------
void Character::calc_levels()
{
  level = GMesh.getHeight(pos.x, pos.y);
  if(waterwalk && level<WaterList.level_surface) level = WaterList.level_surface;

  if(VALID_CHR(on_which_platform))
    platform_level = ChrList[on_which_platform].pos.z + ChrList[on_which_platform].calc_bump_height;

  onwhichfan = GMesh.getIndexPos(pos.x, pos.y);
  //inwater    = pos.z < WaterList.level_surface && GMesh.has_flags(onwhichfan, MESHFX_WATER);
};

//--------------------------------------------------------------------------------------------
void Particle::calc_levels()
{
  level = GMesh.getHeight(pos.x, pos.y);
  if(level<WaterList.level_surface) level = WaterList.level_surface;
  if (level <0) level = 0;

  onwhichfan = GMesh.getIndexPos(pos.x, pos.y);
  inwater    = pos.z < WaterList.level_surface && GMesh.has_flags(onwhichfan, MESHFX_SLIPPY);
};

//--------------------------------------------------------------------------------------------

Platform::Platform()
{
  memset(this,0,sizeof(Platform)); 

  on_which_platform = Character_List::INVALID;
  request_attach    = Character_List::INVALID;
}

void Platform::request_attachment(Uint32 platform) 
{ 
  if(VALID_CHR(platform) && canuse_platforms && ChrList[platform].is_platform) 
  {
    request_detach = true;
    request_attach = platform; 
  }
};
  
//--------------------------------------------------------------------------------------------
void Platform::request_detachment() 
{ request_detach = true; };

//--------------------------------------------------------------------------------------------
void Platform::do_attachment(Character & rchr)
{
  if(rchr.request_detach)
  {
    if( VALID_CHR(rchr.on_which_platform) )
    {
      ChrList[rchr.on_which_platform].holding_weight -= rchr.weight;
    }
    rchr.on_which_platform = Character_List::INVALID;

    rchr.request_detach = false;
  }

  if( VALID_CHR(rchr.request_attach) )
  {
    rchr.jumpready         = true;
    rchr.jumpnumber        = rchr.jumpnumberreset;
    rchr.on_which_platform = rchr.request_attach;
    rchr.platform_level    = ChrList[rchr.request_attach].pos.z + ChrList[rchr.request_attach].calc_bump_height;
    rchr.request_attach    = Character_List::INVALID;

    ChrList[rchr.request_attach].holding_weight += rchr.weight;
  }
};



//--------------------------------------------------------------------------------------------
bool Character::calculate_bumpers()
{
  calc_is_platform   = is_platform;
  calc_is_mount      = is_mount;
  calc_bump_size_x   = bump_size;
  calc_bump_size_y   = bump_size;
  calc_bump_size     = bump_size;
  calc_bump_size_xy  = bump_size_big;
  calc_bump_size_yx  = bump_size_big; 
  calc_bump_size_big = bump_size_big;
  calc_bump_height   = bump_height;

  if(INVALID_MODEL(model) || !matrix_valid )
  {
    return false;
  };

  Mad & rmad = getMad();
  const JF::MD2_Frame * fl = rmad.getFrame(ani.last);
  const JF::MD2_Frame * fc = rmad.getFrame(ani.frame);
  float lerp = (ani.lip>>6) / 4.0f + ani.flip;

  if(NULL==fl && NULL==fc)
  {
    return false;
  };

  vec3_t xdir(matrix.CNV(0,0), matrix.CNV(0,1), matrix.CNV(0,2));
  vec3_t ydir(matrix.CNV(1,0), matrix.CNV(1,1), matrix.CNV(1,2));
  vec3_t zdir(matrix.CNV(2,0), matrix.CNV(2,1), matrix.CNV(2,2));

  vec3_t diff;
  if(NULL==fl)
  {
    diff.x = (fc->max[0]-fc->min[0]);
    diff.y = (fc->max[1]-fc->min[1]);
    diff.z = (fc->max[2]-fc->min[2]);
  }
  else if(NULL==fc)
  {
    diff.x = (fl->max[0]-fl->min[0]);
    diff.y = (fl->max[1]-fl->min[1]);
    diff.z = (fl->max[2]-fl->min[2]);
  }
  else
  {
    diff.x = (fl->max[0]-fl->min[0])*(1.0f-lerp) + (fc->max[0]-fc->min[0])*lerp;
    diff.y = (fl->max[1]-fl->min[1])*(1.0f-lerp) + (fc->max[1]-fc->min[1])*lerp;
    diff.z = (fl->max[2]-fl->min[2])*(1.0f-lerp) + (fc->max[2]-fc->min[2])*lerp;
  };


  calc_is_platform  = is_platform && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  calc_is_mount     = is_mount    && (zdir.z > xdir.z) && (zdir.z > ydir.z);
  calc_bump_size_x  = 0.5*MAX( ABS(diff.x*xdir.x), MAX( ABS(diff.y*ydir.x), ABS(diff.z*zdir.x)) );
  calc_bump_size_y  = 0.5*MAX( ABS(diff.x*xdir.y), MAX( ABS(diff.y*ydir.y), ABS(diff.z*zdir.y)) );
  calc_bump_size_xy = 0.5*ABS((diff.x*xdir.x + diff.y*ydir.x + diff.z*zdir.x) + (diff.x*xdir.y + diff.y*ydir.y + diff.z*zdir.y));
  calc_bump_size_yx = 0.5*ABS((diff.x*xdir.x + diff.y*ydir.x + diff.z*zdir.x) - (diff.x*xdir.y + diff.y*ydir.y + diff.z*zdir.y));

  calc_bump_size     = MAX(calc_bump_size_x,  calc_bump_size_y);
  calc_bump_size_big = MAX(calc_bump_size_xy, calc_bump_size_yx);
  calc_bump_height   = MAX( ABS(diff.z*zdir.z), MAX( ABS(diff.y*ydir.z), ABS(diff.x*xdir.z)) );

  if(calc_bump_size_big < calc_bump_size*1.1) 
    calc_bump_size     *= -1;
  else if (calc_bump_size*2 < calc_bump_size_big*1.1)
    calc_bump_size_big *= -1;


  return true;
};


//--------------------------------------------------------------------------------------------
void Character::initialize()
{
  // BB> This function gives default values to all parameters

  deconstruct();
  construct();
};

//--------------------------------------------------------------------------------------------
void Character::deconstruct()
{
  // BB> This function erases all of the character data

  // delete all cached vertex data
  md2_blended.deconstruct();

  // save the data in the base class !!!!YYYUUUCCCKKK!!!!
  my_base tmp = getBase();

  // set all fields to 0
  memset(this, 0, sizeof(Character));

  // restore the base class
  getBase() = tmp;
};

//--------------------------------------------------------------------------------------------
void Character::construct()
{
  // BB> This function re-initializes erased character data

  // initialize all cached vertex data
  md2_blended.deconstruct();

  model     = Profile_List::INVALID;
  basemodel = Profile_List::INVALID;

  sparkle        = NOSPARKLE;
  missilehandler = Character_List::INVALID;

  // RTS Speech stuff...  Turn all off
  int tnc = 0;
  while (tnc < SPEECH_COUNT)
  {
    wavespeech[tnc] = -1;
    tnc++;
  }

  // Set up model stuff
  _on = false;
  gripoffset  = GRIP_LEFT;
  nextinpack  = Character_List::INVALID;
  hitready    = true;
  boretime    = BORETIME;
  carefultime = CAREFULTIME;

  // Enchant stuff
  firstenchant     = Enchant_List::INVALID;
  undoenchant      = Enchant_List::INVALID;
  missiletreatment = MISNONE;


  // Gender
  gender = GENRANDOM;

  // Team stuff
  team     = TEAM_NEUTRAL;
  baseteam = TEAM_NEUTRAL;

  // Skin
  texture = GLTexture::INVALID;

  // latch
  ai.latch.clear();

  // waypoints
  ai.goto_idx   = 1;
  ai.goto_x[0]  = 0;
  ai.goto_y[0]  = 0;

  ai.target     = Character_List::INVALID;
  ai.owner      = Character_List::INVALID;
  ai.child      = Character_List::INVALID;
  ai.turn_mode  = TURNMODE_VELOCITY;
  ai.bumplast   = Character_List::INVALID;
  ai.attacklast = Character_List::INVALID;
  ai.hitlast    = Character_List::INVALID;

  // Jumping
  jumptime   = DELAY_JUMP;
  jumpnumber = jumpnumberreset;
  jumpready  = false;

  // Other junk
  alpha = 0xFF;
  light = 0xFF;

  // Character size and bumping
  scale_horiz = 1.0f;
  scale_vert  = 1.0f;
  scale_goto  = 1.0f;

  // Grip info
  held_by      = Character_List::INVALID;
  holding_which[SLOT_LEFT] = Character_List::INVALID;
  holding_which[SLOT_RIGHT] = Character_List::INVALID;

  onwhichfan    = Mesh::INVALID_INDEX;

  up = vec3_t(0,0,-SGN(GPhys.gravity));  // on level surface
  scale       = 1;

  //action stuff
  act.ready = true;
  act.keep  = false;
  act.loop  = false;
  act.which = ACTION_DA;
  act.next  = ACTION_DA;
  ani.lip   = 0;
  ani.frame = 0;
  ani.last  = 0;

  // Name the character
  strcpy(name, "*INIT*");
}

//--------------------------------------------------------------------------------------------
Uint32 spawn_one_character(vec3_t & pos, Uint16 turn_lr, vec3_t & vel, float vel_lr, 
                           int profile, Uint8 team, Uint8 skin, char *name, Uint32 override)
{
  // ZZ> This function spawns a character and returns the character's index number
  //     if it worked, Character_List::INVALID otherwise

  Uint32 chr_ref = Character_List::INVALID;
  int tnc;

  // No profile, no character
  if(INVALID_MODEL(profile)) return Character_List::INVALID;

  // Make sure the team is valid
  if (team > TEAM_COUNT-1) team %= TEAM_COUNT;

  // Get a new character
  chr_ref = ChrList.get_free(override);
  if (_INVALID_CHR_RANGE(chr_ref)) return Character_List::INVALID;

  Character & rchr = ChrList[chr_ref];

  Profile & rprof = ProfileList[profile];
  Cap     & rcap  = CapList[rprof.cap_ref];
  Mad     & rmad  = MadList[rprof.mad_ref];


  // make some of the initialization EASY
  rchr.getData()       = rcap.getData();        // directly copy the shared data from cap to us
  rchr.getProperties() = rcap.getProperties();

  // allocate the memory for the frame buffer
  rchr.md2_blended.Allocate( rmad.numVertices() );

  // Now, the only things that are left to set are ones that need to be calculated
  // or that depend on the input parameters other than the Cap

  rchr._on       = true;
  rchr.model     = profile;
  rchr.basemodel = profile;
  rchr.scr_ref   = rprof.scr_ref;

  // Name the character
  if (name == NULL)
  {
    // Generate a random name
    ProfileList[profile].naming_names();
    sprintf(rchr.name, "%s", GChops.name);
  }
  else
  {
    // A name has been given
    strncpy(rchr.name, name, MAXCAPNAMESIZE);
    rchr.name[MAXCAPNAMESIZE-1] = 0;
  }

  cout << "spawning    " << rchr.name << "(" << rcap.classname << ")" << endl;
  cout << "\tplatform==" << (rchr.is_platform ? "TRUE" : "FALSE");
  cout << "  mount==" << (rchr.is_mount ? "TRUE" : "FALSE") << endl;
  cout << "\tbump hgt==" << rchr.bump_height;
  cout << "  bump damp==" << rchr.bump_dampen;
  cout << "  weight==" << rchr.weight<< endl;

  // Set up position
  rchr.pos     = pos;
  rchr.turn_lr = turn_lr;
  rchr.vel     = vel;
  rchr.vel_lr  = vel_lr;

  rchr.level = GMesh.getHeight(pos.x,pos.y) + RAISE;
  if(rchr.waterwalk && rchr.level<WaterList.level_surface + RAISE)
    rchr.level = WaterList.level_surface + RAISE;
  if(rchr.pos.z < rchr.level) rchr.pos.z = rchr.level;

  rchr.old = rchr.getOrientation();

  rchr.stt.x = rchr.pos.x;
  rchr.stt.y = rchr.pos.y;
  rchr.stt.z = rchr.pos.z;

  cout << "\tpos <" << rchr.pos.x << ", " << rchr.pos.y << ", " << rchr.pos.z << ">, " << rchr.turn_lr << endl;

  // Kurse state
  rchr.iskursed = ((rand()%100) < rcap.kursechance);
  if (!rchr.is_item)  rchr.iskursed = false;

  // Gender
  if (rchr.gender == GENRANDOM)  rchr.gender = GENFEMALE + (rand()&1);

  // Team stuff
  rchr.team     = team;
  rchr.baseteam = team;
  rchr.counter = TeamList[team].morale;
  if (!rchr.invictus)  TeamList[team].morale++;
  rchr.order = 0;
  // Firstborn becomes the leader
  if (TeamList[team].leader==NOLEADER)
  {
    TeamList[team].leader = chr_ref;
  }

  // Skin
  if (rcap.skinoverride!=NOSKINOVERRIDE)
  {
    skin = rcap.skinoverride&3;
  }

  rchr.skin    = skin;
  rchr.texture = rprof.skin_ref[skin&3];

  // Life and Mana
  rchr.lifemax = generate_number(rcap.lifebase, rcap.liferand);
  rchr.life = rchr.lifemax;
  rchr.manamax = generate_number(rcap.manabase, rcap.manarand);
  rchr.mana = rchr.manamax;
  rchr.manaflow = generate_number(rcap.manaflowbase, rcap.manaflowrand);
  rchr.manareturn = generate_number(rcap.manareturnbase, rcap.manareturnrand)>>MANARETURNSHIFT;
  rchr.alive = rchr.life>0;


  // SWID
  rchr.strength = generate_number(rcap.strengthbase, rcap.strengthrand);
  rchr.wisdom = generate_number(rcap.wisdombase, rcap.wisdomrand);
  rchr.intelligence = generate_number(rcap.intelligencebase, rcap.intelligencerand);
  rchr.dexterity = generate_number(rcap.dexteritybase, rcap.dexterityrand);

  // Damage
  rchr.defense = rcap.defense[skin];
  for (tnc = 0; tnc < DAMAGE_COUNT; tnc++)
  {
    rchr.damagemodifier[tnc] = rcap.damagemodifier[tnc][skin];
  }

  // AI stuff

  // waypoints
  rchr.ai.goto_cnt   = 0;
  rchr.ai.goto_idx   = 1;
  rchr.ai.goto_x[0]  = rchr.pos.x;
  rchr.ai.goto_y[0]  = rchr.pos.y;

  rchr.ai.alert   = ALERT_IF_SPAWNED;
  rchr.ai.state   = rcap.stateoverride;
  rchr.ai.content = rcap.contentoverride;
  rchr.ai.target  = chr_ref;
  rchr.ai.owner   = chr_ref;
  rchr.ai.child   = chr_ref;
  rchr.ai.time    = 0;
  rchr.ai.turn_mode  = TURNMODE_VELOCITY;
  rchr.ai.bumplast   = Character_List::INVALID;
  rchr.ai.attacklast = Character_List::INVALID;
  rchr.ai.hitlast    = Character_List::INVALID;
  rchr.ai.gopoof     = false;


  // Skin stuff

  rchr.maxaccel = rcap.maxaccel[skin];

  // Character size and bumping
  rchr.scale   = rchr.getMad().scale * 4 * 1.5;
  rchr.scale_goto = rchr.scale_vert;

  rchr.shadow_size_save   = rchr.shadow_size;
  rchr.bump_size_save     = rchr.calc_bump_size;
  rchr.bump_size_big_save = rchr.calc_bump_size_big;
  rchr.bump_height_save   = rchr.calc_bump_height;

  make_one_character_matrix(rchr);
  rchr.calculate_bumpers();

  rchr.onwhichfan = GMesh.getIndexPos(rchr.pos.x,rchr.pos.y);
  rchr.weight     *= rchr.scale_horiz * rchr.scale_horiz *rchr.scale_vert;

  //action stuff
  rchr.act.ready = true;
  rchr.act.keep  = false;
  rchr.act.loop  = false;
  rchr.act.which = ACTION_DA;
  rchr.act.next  = ACTION_DA;
  rchr.ani.lip   = 0;
  rchr.ani.frame = rchr.getMad().framestart;
  rchr.ani.last  = rchr.ani.frame;

  // Particle attachments
  for (tnc = 0; tnc < rcap.attachedprtamount; tnc++)
  {
    spawn_one_particle(rchr.pos, 0, rchr.vel, 0, rchr.model, rcap.attachedprttype,
      chr_ref, GRIP_LAST+tnc, rchr.team, chr_ref, tnc, Character_List::INVALID);
  }

  // Experience
  tnc = generate_number(rcap.experiencebase, rcap.experiencerand);
  if (tnc > MAXXP) tnc = MAXXP;
  rchr.experience = tnc;
  rchr.experiencelevel = rcap.leveloverride;

  // MISC
  rchr.ammoknown        = rcap.nameknown;
  rchr.is_platform      = rcap.is_platform;
  rchr.canuse_platforms = (rchr.weight>=0);

  // bumpers
  rchr.calc_is_platform   = rchr.is_platform;
  rchr.calc_is_mount      = rchr.is_mount;
  rchr.calc_bump_size_x   = rchr.bump_size;
  rchr.calc_bump_size_y   = rchr.bump_size;
  rchr.calc_bump_size     = rchr.bump_size;
  rchr.calc_bump_size_xy  = rchr.bump_size_big;
  rchr.calc_bump_size_yx  = rchr.bump_size_big; 
  rchr.calc_bump_size_big = rchr.bump_size_big;
  rchr.calc_bump_height   = rchr.bump_height;

  rchr.calculate_bumpers();

  cout << "\tcalc_bump_height==" <<  rchr.calc_bump_height;
  cout << "  calc_bump_size==" <<  rchr.calc_bump_size;
  cout << "  calc_bump_size_big==" <<  rchr.calc_bump_size_big<<endl;

  return chr_ref;
}



//--------------------------------------------------------------------------------------------
void respawn_character(Uint16 character)
{
  // ZZ> This function respawns a character
  Uint16 item;

//  if (!ChrList[character].alive)
//  {
//    spawn_poof(character, ChrList[character].model);
//    disaffirm_attached_particles(character, Pip_List::INVALID);
//    ChrList[character].alive = true;
//    ChrList[character].boretime = BORETIME;
//    ChrList[character].carefultime = CAREFULTIME;
//    ChrList[character].life = ChrList[character].lifemax;
//    ChrList[character].mana = ChrList[character].manamax;
//    ChrList[character].pos.x = ChrList[character].stt.x;
//    ChrList[character].pos.y = ChrList[character].stt.y;
//    ChrList[character].pos.z = ChrList[character].stt.z;
//    ChrList[character].vel.x = 0;
//    ChrList[character].vel.y = 0;
//    ChrList[character].vel.z = 0;
//    ChrList[character].team = ChrList[character].baseteam;
//    ChrList[character].canbecrushed = false;
//    ChrList[character].map_turn_lr = 0x8000;  // These two mean on level surface
//    ChrList[character].map_turn_ud = 0x8000;
//    if (TeamList[ChrList[character].team].leader==NOLEADER)  TeamList[ChrList[character].team].leader=character;
//    if (!ChrList[character].invictus)  TeamList[ChrList[character].baseteam].morale++;
//
//    ChrList[character].act.ready = true;
//    ChrList[character].act.keep = false;
//    ChrList[character].act.loop = false;
//    ChrList[character].act.which = ACTION_DA;
//    ChrList[character].act.next = ACTION_DA;
//
//    ChrList[character].ani.lip = 0;
//    ChrList[character].ani.frame = ChrList[character].getMad().framestart;
//    ChrList[character].ani.last  = ChrList[character].ani.frame;
//
//    ChrList[character].is_platform = ChrList[character].getCap().is_platform;
//    ChrList[character].flyheight = ChrList[character].getCap().flyheight;
//    ChrList[character].bump_dampen = ChrList[character].getCap().bump_dampen;
//    ChrList[character].bump_size = ChrList[character].getCap().bump_size*ChrList[character].fat;
//    ChrList[character].bump_size_big = ChrList[character].getCap().bump_size_big*ChrList[character].fat;
//    ChrList[character].bump_height = ChrList[character].getCap().bump_height*ChrList[character].fat;
//
//    ChrList[character].bump_size_save = ChrList[character].getCap().bump_size;
//    ChrList[character].bump_size_big_save = ChrList[character].getCap().bump_size_big;
//    ChrList[character].bump_height_save = ChrList[character].getCap().bump_height;
//
////        ChrList[character].ai.alert = ALERT_IF_SPAWNED;
//    ChrList[character].ai.alert = 0;
////        ChrList[character].ai.state = 0;
//    ChrList[character].ai.target = character;
//    ChrList[character].ai.time = 0;
//    ChrList[character].grogtime = 0;
//    ChrList[character].dazetime = 0;
//    reaffirm_attached_particles(character);
//
//    // Let worn items come back
//    SCAN_CHR_PACK_BEGIN(ChrList[character], item, rinv_item)
//    {
//      if (rinv_item.is_equipped)
//      {
//        rinv_item.is_equipped = false;
//        rinv_item.ai.alert |= ALERT_IF_ATLASTWAYPOINT;  // doubles as PutAway
//      }
//    } SCAN_CHR_PACK_END;
//  }
}
