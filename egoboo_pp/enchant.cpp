// enchant.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Enchant.h"
#include "Character.h"
#include "Particle.h"
#include "Mad.h"
#include "Profile.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Enchant_List EncList;
Eve_List EveList;

//--------------------------------------------------------------------------------------------
void unset_enchant_value(Uint16 enchantindex, Uint8 valueindex)
{
  // ZZ> This function unsets a set value
  Uint16 character;

  if (EncList[enchantindex].setyesno[valueindex])
  {
    character = EncList[enchantindex].target;
    switch (valueindex)
    {
      case SETDAMAGETYPE:
        ChrList[character].damagetargettype = EncList[enchantindex].setsave[valueindex];
        break;

      case SETNUMBEROFJUMPS:
        ChrList[character].jumpnumberreset = EncList[enchantindex].setsave[valueindex];
        break;

      case SETLIFEBARCOLOR:
        ChrList[character].lifecolor = EncList[enchantindex].setsave[valueindex];
        break;

      case SETMANABARCOLOR:
        ChrList[character].manacolor = EncList[enchantindex].setsave[valueindex];
        break;

      case SETSLASHMODIFIER:
        ChrList[character].damagemodifier[DAMAGE_SLASH] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETCRUSHMODIFIER:
        ChrList[character].damagemodifier[DAMAGE_CRUSH] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETPOKEMODIFIER:
        ChrList[character].damagemodifier[DAMAGE_POKE] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETHOLYMODIFIER:
        ChrList[character].damagemodifier[DAMAGE_HOLY] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETEVILMODIFIER:
        ChrList[character].damagemodifier[DAMAGE_EVIL] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETFIREMODIFIER:
        ChrList[character].damagemodifier[DAMAGE_FIRE] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETICEMODIFIER:
        ChrList[character].damagemodifier[DAMAGE_ICE] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETZAPMODIFIER:
        ChrList[character].damagemodifier[DAMAGE_ZAP] = EncList[enchantindex].setsave[valueindex];
        break;

      case SETFLASHINGAND:
        ChrList[character].flashand = EncList[enchantindex].setsave[valueindex];
        break;

      case SETLIGHTBLEND:
        ChrList[character].light = EncList[enchantindex].setsave[valueindex];
        break;

      case SETALPHABLEND:
        ChrList[character].alpha = EncList[enchantindex].setsave[valueindex];
        break;

      case SETSHEEN:
        ChrList[character].sheen = EncList[enchantindex].setsave[valueindex];
        break;

      case SETFLYTOHEIGHT:
        ChrList[character].flyheight = EncList[enchantindex].setsave[valueindex];
        break;

      case SETWALKONWATER:
        ChrList[character].waterwalk = EncList[enchantindex].setsave[valueindex];
        break;

      case SETCANSEEINVISIBLE:
        ChrList[character].canseeinvisible = EncList[enchantindex].setsave[valueindex];
        break;

      case SETMISSILETREATMENT:
        ChrList[character].missiletreatment = (MISTREAT_TYPE)EncList[enchantindex].setsave[valueindex];
        break;

      case SETCOSTFOREACHMISSILE:
        ChrList[character].missilecost = EncList[enchantindex].setsave[valueindex];
        ChrList[character].missilehandler = character;
        break;

      case SETMORPH:
        // Need special handler for when this is removed
        change_character(character, ChrList[character].basemodel, EncList[enchantindex].setsave[valueindex], LEAVEALL);
        break;

      case SETCHANNEL:
        ChrList[character].canchannel = EncList[enchantindex].setsave[valueindex];
        break;

    }
    EncList[enchantindex].setyesno[valueindex] = false;
  }
}

//--------------------------------------------------------------------------------------------
void remove_enchant_value(Uint16 enchantindex, Uint8 valueindex)
{
  // ZZ> This function undoes cumulative modification to character stats
  float fvaluetoadd;
  int valuetoadd;

  Uint16 character = EncList[enchantindex].target;
  switch (valueindex)
  {
    case ADDJUMPPOWER:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex]/16.0;
      ChrList[character].jump-=fvaluetoadd;
      break;

    case ADDBUMPDAMPEN:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex]/float(0x80);
      ChrList[character].bump_dampen-=fvaluetoadd;
      break;

    case ADDBOUNCINESS:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex]/float(0x80);
      ChrList[character].dampen-=fvaluetoadd;
      break;

    case ADDDAMAGE:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].damageboost-=valuetoadd;
      break;

    case ADDSIZE:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex]/float(0x80);
      ChrList[character].scale_goto-=fvaluetoadd;
      ChrList[character].sizegototime = SIZETIME;
      break;

    case ADDACCEL:
      fvaluetoadd = EncList[enchantindex].addsave[valueindex]/1000.0;
      ChrList[character].maxaccel-=fvaluetoadd;
      break;

    case ADDRED:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].redshift-=valuetoadd;
      break;

    case ADDGRN:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].grnshift-=valuetoadd;
      break;

    case ADDBLU:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].blushift-=valuetoadd;
      break;

    case ADDDEFENSE:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].defense-=valuetoadd;
      break;

    case ADDMANA:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].manamax-=valuetoadd;
      ChrList[character].mana-=valuetoadd;
      if (ChrList[character].mana < 0) ChrList[character].mana = 0;
      break;

    case ADDLIFE:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].lifemax-=valuetoadd;
      ChrList[character].life-=valuetoadd;
      if (ChrList[character].life < 1) ChrList[character].life = 1;
      break;

    case ADDSTRENGTH:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].strength-=valuetoadd;
      break;

    case ADDWISDOM:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].wisdom-=valuetoadd;
      break;

    case ADDINTELLIGENCE:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].intelligence-=valuetoadd;
      break;

    case ADDDEXTERITY:
      valuetoadd = EncList[enchantindex].addsave[valueindex];
      ChrList[character].dexterity-=valuetoadd;
      break;

  }
}


//--------------------------------------------------------------------------------------------
void remove_enchant(Uint16 enchantindex)
{
  // ZZ> This function removes a specific enchantment and adds it to the unused list
  Uint16 lastenchant, currentenchant;
  int add;

  if (INVALID_ENC(enchantindex)) return;

  Enchant & renc = EncList[enchantindex];

  cout << "---- removing enchant " << enchantindex << endl;
  cout << "\teve == " << renc.getEve().filename << endl;

  // Unsparkle the spellbook
  Uint32 spawner = renc.spawner;
  if (VALID_CHR(spawner))
  {
    Character & rspawner = ChrList[spawner];

    cout << "\tspawner == " << rspawner.name << "(" << rspawner.getCap().classname << ")" << endl;

    rspawner.sparkle = NOSPARKLE;
    // Make the spawner unable to undo the enchantment
    if (rspawner.undoenchant == enchantindex)
    {
      rspawner.undoenchant = Enchant_List::INVALID;
    }
  }

  // Play the end sound   TODO: This is broken sounds do not get played at end of enchant
  Uint32 target = renc.target;
  if(VALID_CHR(target))
  {
    Character & rtarget = ChrList[target];

    cout << "\ttarget == " << rtarget.name << "(" << rtarget.getCap().classname << ")" << endl;

    play_sound(rtarget.pos, renc.getEve().waveindex );


    // Unset enchant values, doing morph last
    unset_enchant_value(enchantindex, SETDAMAGETYPE);
    unset_enchant_value(enchantindex, SETNUMBEROFJUMPS);
    unset_enchant_value(enchantindex, SETLIFEBARCOLOR);
    unset_enchant_value(enchantindex, SETMANABARCOLOR);
    unset_enchant_value(enchantindex, SETSLASHMODIFIER);
    unset_enchant_value(enchantindex, SETCRUSHMODIFIER);
    unset_enchant_value(enchantindex, SETPOKEMODIFIER);
    unset_enchant_value(enchantindex, SETHOLYMODIFIER);
    unset_enchant_value(enchantindex, SETEVILMODIFIER);
    unset_enchant_value(enchantindex, SETFIREMODIFIER);
    unset_enchant_value(enchantindex, SETICEMODIFIER);
    unset_enchant_value(enchantindex, SETZAPMODIFIER);
    unset_enchant_value(enchantindex, SETFLASHINGAND);
    unset_enchant_value(enchantindex, SETLIGHTBLEND);
    unset_enchant_value(enchantindex, SETALPHABLEND);
    unset_enchant_value(enchantindex, SETSHEEN);
    unset_enchant_value(enchantindex, SETFLYTOHEIGHT);
    unset_enchant_value(enchantindex, SETWALKONWATER);
    unset_enchant_value(enchantindex, SETCANSEEINVISIBLE);
    unset_enchant_value(enchantindex, SETMISSILETREATMENT);
    unset_enchant_value(enchantindex, SETCOSTFOREACHMISSILE);
    unset_enchant_value(enchantindex, SETCHANNEL);
    unset_enchant_value(enchantindex, SETMORPH);

    // Remove all of the cumulative values
    add = 0;
    while (add < EVE_ADDVALUE_COUNT)
    {
      remove_enchant_value(enchantindex, add);
      add++;
    }

    // Unlink it
    if (rtarget.firstenchant == enchantindex)
    {
      // It was the first in the list
      rtarget.firstenchant = renc.nextenchant;
    }
    else
    {
      // Search until we find it
      currentenchant = rtarget.firstenchant;
      lastenchant    = currentenchant;
      while (currentenchant != enchantindex)
      {
        lastenchant = currentenchant;
        currentenchant = EncList[currentenchant].nextenchant;
      }

      // Relink the last enchantment
      EncList[lastenchant].nextenchant = renc.nextenchant;
    }


    // See if we spit out an end message
    if (renc.getEve().endmessage >= 0)
    {
      display_message( ProfileList[renc.eve_prof].msg_start + renc.getEve().endmessage, renc.target);
    }

    // Check to see if we spawn a poof
    if ( renc.getEve().poofonend)
    {
      spawn_poof(renc.target, renc.eve_prof);
    }


    // Check to see if the target dies
    if (renc.getEve().killonend)
    {
      if (rtarget.invictus)  TeamList[rtarget.baseteam].morale++;
      rtarget.invictus = false;
      kill_character(target, Character_List::INVALID);
    }

    // Now fix dem weapons
    reset_character_alpha(rtarget.holding_which[SLOT_LEFT]);
    reset_character_alpha(rtarget.holding_which[SLOT_RIGHT]);
  }

  // Kill overlay too...
  Uint32 overlay = renc.overlay;
  if (VALID_CHR(overlay))
  {
    Character & roverlay = ChrList[overlay];

    if (roverlay.invictus)  TeamList[roverlay.baseteam].morale++;
    roverlay.invictus = false;
    kill_character(overlay, Character_List::INVALID);
  }

  // Now get rid of it
  EncList.return_one(enchantindex);
}

//--------------------------------------------------------------------------------------------
Uint16 enchant_value_filled(Uint16 enchantindex, Uint8 valueindex)
{
  // ZZ> This function returns Enchant_List::INVALID if the enchantment's target has no conflicting
  //     set values in its other enchantments.  Otherwise it returns the enchantindex
  //     of the conflicting enchantment
  Uint16 character, currenchant;

  character = EncList[enchantindex].target;

  SCAN_ENC_BEGIN(ChrList[character],currenchant, renc)
  {
    if (renc.setyesno[valueindex])
    {
      return currenchant;
    }
  } SCAN_ENC_END;

  return Enchant_List::INVALID;
}

//--------------------------------------------------------------------------------------------
void set_enchant_value(Uint16 enchantindex, Uint8 valueindex, Uint32 profile)
{
  // ZZ> This function sets and saves one of the character's stats
  Uint16 conflict;

  Eve     & reve = EveList[ ProfileList[profile].eve_ref ];
  Enchant & renc = EncList[enchantindex];

  EncList[enchantindex].setyesno[valueindex] = false;
  if (reve.setyesno[valueindex])
  {
    conflict = enchant_value_filled(enchantindex, valueindex);
    if (INVALID_ENC(conflict) || reve.override)
    {
      // Check for multiple enchantments
      if (VALID_ENC(conflict))
      {
        // Multiple enchantments aren't allowed for sets
        if (reve.removeoverridden)
        {
          // Kill the old enchantment
          remove_enchant(conflict);
        }
        else
        {
          // Just unset the old enchantment's value
          unset_enchant_value(conflict, valueindex);
        }
      }

      // Set the value, and save the character's real stat
      Character & rchr = ChrList[ renc.target ];
      renc.setyesno[valueindex] = true;
      switch (valueindex)
      {
        case SETDAMAGETYPE:
          renc.setsave[valueindex] = rchr.damagetargettype;
          rchr.damagetargettype = reve.setvalue[valueindex];
          break;

        case SETNUMBEROFJUMPS:
          renc.setsave[valueindex] = rchr.jumpnumberreset;
          rchr.jumpnumberreset = reve.setvalue[valueindex];
          break;

        case SETLIFEBARCOLOR:
          renc.setsave[valueindex] = rchr.lifecolor;
          rchr.lifecolor = reve.setvalue[valueindex];
          break;

        case SETMANABARCOLOR:
          renc.setsave[valueindex] = rchr.manacolor;
          rchr.manacolor = reve.setvalue[valueindex];
          break;

        case SETSLASHMODIFIER:
          renc.setsave[valueindex] = rchr.damagemodifier[DAMAGE_SLASH];
          rchr.damagemodifier[DAMAGE_SLASH] = reve.setvalue[valueindex];
          break;

        case SETCRUSHMODIFIER:
          renc.setsave[valueindex] = rchr.damagemodifier[DAMAGE_CRUSH];
          rchr.damagemodifier[DAMAGE_CRUSH] = reve.setvalue[valueindex];
          break;

        case SETPOKEMODIFIER:
          renc.setsave[valueindex] = rchr.damagemodifier[DAMAGE_POKE];
          rchr.damagemodifier[DAMAGE_POKE] = reve.setvalue[valueindex];
          break;

        case SETHOLYMODIFIER:
          renc.setsave[valueindex] = rchr.damagemodifier[DAMAGE_HOLY];
          rchr.damagemodifier[DAMAGE_HOLY] = reve.setvalue[valueindex];
          break;

        case SETEVILMODIFIER:
          renc.setsave[valueindex] = rchr.damagemodifier[DAMAGE_EVIL];
          rchr.damagemodifier[DAMAGE_EVIL] = reve.setvalue[valueindex];
          break;

        case SETFIREMODIFIER:
          renc.setsave[valueindex] = rchr.damagemodifier[DAMAGE_FIRE];
          rchr.damagemodifier[DAMAGE_FIRE] = reve.setvalue[valueindex];
          break;

        case SETICEMODIFIER:
          renc.setsave[valueindex] = rchr.damagemodifier[DAMAGE_ICE];
          rchr.damagemodifier[DAMAGE_ICE] = reve.setvalue[valueindex];
          break;

        case SETZAPMODIFIER:
          renc.setsave[valueindex] = rchr.damagemodifier[DAMAGE_ZAP];
          rchr.damagemodifier[DAMAGE_ZAP] = reve.setvalue[valueindex];
          break;

        case SETFLASHINGAND:
          renc.setsave[valueindex] = rchr.flashand;
          rchr.flashand = reve.setvalue[valueindex];
          break;

        case SETLIGHTBLEND:
          renc.setsave[valueindex] = rchr.light;
          rchr.light = reve.setvalue[valueindex];
          break;

        case SETALPHABLEND:
          renc.setsave[valueindex] = rchr.alpha;
          rchr.alpha = reve.setvalue[valueindex];
          break;

        case SETSHEEN:
          renc.setsave[valueindex] = rchr.sheen;
          rchr.sheen = reve.setvalue[valueindex];
          break;

        case SETFLYTOHEIGHT:
          renc.setsave[valueindex] = rchr.flyheight;
          if (rchr.flyheight==0 && rchr.pos.z > -2)
          {
            rchr.flyheight = reve.setvalue[valueindex];
          }
          break;

        case SETWALKONWATER:
          renc.setsave[valueindex] = rchr.waterwalk;
          if (!rchr.waterwalk)
          {
            rchr.waterwalk = reve.setvalue[valueindex];
          }
          break;

        case SETCANSEEINVISIBLE:
          renc.setsave[valueindex] = rchr.canseeinvisible;
          rchr.canseeinvisible = reve.setvalue[valueindex];
          break;

        case SETMISSILETREATMENT:
          renc.setsave[valueindex] = rchr.missiletreatment;
          rchr.missiletreatment = (MISTREAT_TYPE)reve.setvalue[valueindex];
          break;

        case SETCOSTFOREACHMISSILE:
          renc.setsave[valueindex] = rchr.missilecost;
          rchr.missilecost = reve.setvalue[valueindex];
          rchr.missilehandler = renc.owner;
          break;

        case SETMORPH:
          renc.setsave[valueindex] = rchr.skin;
          // Special handler for morph
          change_character(renc.target, profile, 0, LEAVEALL); // LEAVEFIRST);
          rchr.ai.alert|=ALERT_IF_CHANGED;
          break;

        case SETCHANNEL:
          renc.setsave[valueindex] = rchr.canchannel;
          rchr.canchannel = reve.setvalue[valueindex];
          break;

      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void getadd(int minval, int value, int maxval, int & valuetoadd)
{
  // ZZ> This function figures out what value to add should be in order
  //     to not overflow the minval and maxval bounds
  int newvalue;

  newvalue = value+ valuetoadd;
  if (newvalue < minval)
  {
    // Increase valuetoadd to fit
    valuetoadd = minval-value;
    if (valuetoadd > 0)  valuetoadd=0;
    return;
  }

  if (newvalue > maxval)
  {
    // Decrease valuetoadd to fit
    valuetoadd = maxval-value;
    if (valuetoadd < 0) valuetoadd=0;
  }
}

//--------------------------------------------------------------------------------------------
void fgetadd(float minval, float value, float maxval, float & valuetoadd)
{
  // ZZ> This function figures out what value to add should be in order
  //     to not overflow the minval and maxval bounds
  float newvalue;

  newvalue = value+(valuetoadd);
  if (newvalue < minval)
  {
    // Increase valuetoadd to fit
    valuetoadd = minval-value;
    if (valuetoadd > 0)  valuetoadd=0;
    return;
  }

  if (newvalue > maxval)
  {
    // Decrease valuetoadd to fit
    valuetoadd = maxval-value;
    if (valuetoadd < 0)  valuetoadd=0;
  }
}

//--------------------------------------------------------------------------------------------
void add_enchant_value(Uint16 enchantindex, Uint8 valueindex, EVE_REF & eve_ref)
{
  // ZZ> This function does cumulative modification to character stats
  int valuetoadd, newvalue;
  float fvaluetoadd = 0, fnewvalue;

  Eve & reve = EveList[eve_ref];
  Enchant & renc = EncList[enchantindex];
  Character & rtarget = ChrList[renc.target];

  switch (valueindex)
  {
    case ADDJUMPPOWER:
      fnewvalue = rtarget.jump;
      fvaluetoadd = reve.addvalue[valueindex]/16.0;
      fgetadd(0, fnewvalue, 20.0, fvaluetoadd);
      valuetoadd = fvaluetoadd*16.0; // Get save value
      fvaluetoadd = valuetoadd/16.0;
      rtarget.jump+=fvaluetoadd;
      break;

    case ADDBUMPDAMPEN:
      fnewvalue = rtarget.bump_dampen;
      fvaluetoadd = reve.addvalue[valueindex]/float(0x80);
      fgetadd(0, fnewvalue, 1.0, fvaluetoadd);
      valuetoadd = fvaluetoadd*float(0x80); // Get save value
      fvaluetoadd = valuetoadd/float(0x80);
      rtarget.bump_dampen+=fvaluetoadd;
      break;

    case ADDBOUNCINESS:
      fnewvalue = rtarget.dampen;
      fvaluetoadd = reve.addvalue[valueindex]/float(0x80);
      fgetadd(0, fnewvalue, 0.95, fvaluetoadd);
      valuetoadd = fvaluetoadd*float(0x80); // Get save value
      fvaluetoadd = valuetoadd/float(0x80);
      rtarget.dampen+=fvaluetoadd;
      break;

    case ADDDAMAGE:
      newvalue = rtarget.damageboost;
      valuetoadd = reve.addvalue[valueindex] << 6;
      getadd(0, newvalue, 0x1000, valuetoadd);
      rtarget.damageboost+=valuetoadd;
      break;

    case ADDSIZE:
      fnewvalue = rtarget.scale_goto;
      fvaluetoadd = reve.addvalue[valueindex]/float(0x80);
      fgetadd(0.5, fnewvalue, 2.0, fvaluetoadd);
      valuetoadd = fvaluetoadd*float(0x80); // Get save value
      fvaluetoadd = valuetoadd/float(0x80);
      rtarget.scale_goto+=fvaluetoadd;
      rtarget.sizegototime = SIZETIME;
      break;

    case ADDACCEL:
      fnewvalue = rtarget.maxaccel;
      fvaluetoadd = reve.addvalue[valueindex]/25.0;
      fgetadd(0, fnewvalue, 1.5, fvaluetoadd);
      valuetoadd = fvaluetoadd*1000.0; // Get save value
      fvaluetoadd = valuetoadd/1000.0;
      rtarget.maxaccel+=fvaluetoadd;
      break;

    case ADDRED:
      newvalue = rtarget.redshift;
      valuetoadd = reve.addvalue[valueindex];
      getadd(0, newvalue, 6, valuetoadd);
      rtarget.redshift+=valuetoadd;
      break;

    case ADDGRN:
      newvalue = rtarget.grnshift;
      valuetoadd = reve.addvalue[valueindex];
      getadd(0, newvalue, 6, valuetoadd);
      rtarget.grnshift+=valuetoadd;
      break;

    case ADDBLU:
      newvalue = rtarget.blushift;
      valuetoadd = reve.addvalue[valueindex];
      getadd(0, newvalue, 6, valuetoadd);
      rtarget.blushift+=valuetoadd;
      break;

    case ADDDEFENSE:
      newvalue = rtarget.defense;
      valuetoadd = reve.addvalue[valueindex];
      getadd(55, newvalue, 0xFF, valuetoadd);  // Don't fix again!
      rtarget.defense+=valuetoadd;
      break;

    case ADDMANA:
      newvalue = rtarget.manamax;
      valuetoadd = reve.addvalue[valueindex] << 6;
      getadd(0, newvalue, HIGHSTAT, valuetoadd);
      rtarget.manamax+=valuetoadd;
      rtarget.mana+=valuetoadd;
      if (rtarget.mana < 0)  rtarget.mana = 0;
      break;

    case ADDLIFE:
      newvalue = rtarget.lifemax;
      valuetoadd = reve.addvalue[valueindex] << 6;
      getadd(LOWSTAT, newvalue, HIGHSTAT, valuetoadd);
      rtarget.lifemax+=valuetoadd;
      rtarget.life+=valuetoadd;
      if (rtarget.life < 1)  rtarget.life = 1;
      break;

    case ADDSTRENGTH:
      newvalue = rtarget.strength;
      valuetoadd = reve.addvalue[valueindex] << 6;
      getadd(0, newvalue, PERFECTSTAT, valuetoadd);
      rtarget.strength+=valuetoadd;
      break;

    case ADDWISDOM:
      newvalue = rtarget.wisdom;
      valuetoadd = reve.addvalue[valueindex] << 6;
      getadd(0, newvalue, PERFECTSTAT, valuetoadd);
      rtarget.wisdom+=valuetoadd;
      break;

    case ADDINTELLIGENCE:
      newvalue = rtarget.intelligence;
      valuetoadd = reve.addvalue[valueindex] << 6;
      getadd(0, newvalue, PERFECTSTAT, valuetoadd);
      rtarget.intelligence+=valuetoadd;
      break;

    case ADDDEXTERITY:
      newvalue = rtarget.dexterity;
      valuetoadd = reve.addvalue[valueindex] << 6;
      getadd(0, newvalue, PERFECTSTAT, valuetoadd);
      rtarget.dexterity+=valuetoadd;
      break;

  }

  renc.addsave[valueindex] = valuetoadd;  // Save the value for undo
}

//--------------------------------------------------------------------------------------------
void do_enchant_spawn()
{
  // ZZ> This function lets enchantments spawn particles
  int cnt, tnc;
  Uint16 facing, particle;
  EVE_REF eve;

  SCAN_ENCLIST_BEGIN(cnt, renc)
  {
    Eve & reve = renc.getEve();
    if (reve.contspawnamount>0)
    {
      renc.spawntime--;
      if (renc.spawntime == 0)
      {
        Character & rtarget = ChrList[renc.target];
        renc.spawntime = reve.contspawntime;
        facing = rtarget.turn_lr;
        tnc = 0;
        while (tnc < reve.contspawnamount)
        {
          particle = spawn_one_particle(rtarget.pos, facing, rtarget.vel, 0,
            renc.eve_prof, reve.contspawnpip,
            Character_List::INVALID, GRIP_LAST, ChrList[renc.owner].team, renc.owner, tnc, Character_List::INVALID);
          facing += reve.contspawnfacingadd;
          tnc++;
        }
      }
    }
  } SCAN_ENCLIST_END;
};

//--------------------------------------------------------------------------------------------
void disenchant_character(Uint16 cnt)
{
  // ZZ> This function removes all enchantments from a character
  while (VALID_ENC(ChrList[cnt].firstenchant))
  {
    remove_enchant(ChrList[cnt].firstenchant);
  }
}





//--------------------------------------------------------------------------------------------
Uint16 change_armor(Uint16 character, Uint16 skin)
{
  // ZZ> This function changes the armor of the character
  Uint16 enchant, sTmp;
  int iTmp;

  // Remove armor enchantments
  SCAN_ENC_BEGIN(ChrList[character], enchant, renc)
  {
    unset_enchant_value(enchant, SETSLASHMODIFIER);
    unset_enchant_value(enchant, SETCRUSHMODIFIER);
    unset_enchant_value(enchant, SETPOKEMODIFIER);
    unset_enchant_value(enchant, SETHOLYMODIFIER);
    unset_enchant_value(enchant, SETEVILMODIFIER);
    unset_enchant_value(enchant, SETFIREMODIFIER);
    unset_enchant_value(enchant, SETICEMODIFIER);
    unset_enchant_value(enchant, SETZAPMODIFIER);
  } SCAN_ENC_END;

  // Change the skin
  sTmp = ChrList[character].model;
  ChrList[character].skin    = skin;
  ChrList[character].texture = ProfileList[sTmp].skin_ref[skin&3];

  // Change stats associated with skin
  ChrList[character].defense = ChrList[character].getCap().defense[skin];
  iTmp = 0;
  while (iTmp < DAMAGE_COUNT)
  {
    ChrList[character].damagemodifier[iTmp] = ChrList[character].getCap().damagemodifier[iTmp][skin];
    iTmp++;
  }
  ChrList[character].maxaccel = ChrList[character].getCap().maxaccel[skin];

  // Reset armor enchantments
  // These should really be done in reverse order ( Start with last enchant ), but
  // I don't care at this point !!!BAD!!!
  SCAN_ENC_BEGIN(ChrList[character], enchant, renc)
  {
    set_enchant_value(enchant, SETSLASHMODIFIER, EncList[enchant].eve_prof);
    set_enchant_value(enchant, SETCRUSHMODIFIER, EncList[enchant].eve_prof);
    set_enchant_value(enchant, SETPOKEMODIFIER, EncList[enchant].eve_prof);
    set_enchant_value(enchant, SETHOLYMODIFIER, EncList[enchant].eve_prof);
    set_enchant_value(enchant, SETEVILMODIFIER, EncList[enchant].eve_prof);
    set_enchant_value(enchant, SETFIREMODIFIER, EncList[enchant].eve_prof);
    set_enchant_value(enchant, SETICEMODIFIER, EncList[enchant].eve_prof);
    set_enchant_value(enchant, SETZAPMODIFIER, EncList[enchant].eve_prof);

    add_enchant_value(enchant, ADDACCEL, ProfileList[EncList[enchant].eve_prof].eve_ref );
    add_enchant_value(enchant, ADDDEFENSE, ProfileList[EncList[enchant].eve_prof].eve_ref );
  } SCAN_ENC_END;

  return skin;
}

//--------------------------------------------------------------------------------------------
void change_character(Uint16 character, Uint16 profile, Uint8 skin,
                      Uint8 leavewhich)
{
  // ZZ> This function polymorphs a character, changing stats, dropping weapons
  int tnc, enchant;
  Uint16 sTmp, item;

  if( INVALID_MODEL(profile) ) return;

  {
    Character & rchr = ChrList[character];
    Cap & rcap = rchr.getCap();
    Mad & rmad = rchr.getMad();

    // Drop left weapon
    sTmp = rchr.holding_which[SLOT_LEFT];
    if (VALID_CHR(sTmp)  && (!rcap.slot_valid[SLOT_LEFT] || rcap.is_mount))
    {
      Character & rschr = ChrList[sTmp];
      detach_character_from_mount(sTmp, true, true);
      if (rchr.is_mount)
      {
        rschr.accumulate_vel_z(DISMOUNTZ_VELOCITY - DROPZ_VELOCITY);
        rschr.jumptime = DELAY_JUMP;
        rschr.jumpready = false;
      }
    }

    // Drop right weapon
    sTmp = rchr.holding_which[SLOT_RIGHT];
    if (VALID_CHR(sTmp)  && !rcap.slot_valid[SLOT_RIGHT])
    {
      Character & rschr = ChrList[sTmp];
      detach_character_from_mount(sTmp, true, true);
      if (rchr.is_mount)
      {
        rschr.accumulate_vel_z(DISMOUNTZ_VELOCITY - DROPZ_VELOCITY);
        rschr.jumptime = DELAY_JUMP;
        rschr.jumpready = false;
      }
    }

    // Remove particles
    disaffirm_attached_particles(character, Pip_List::INVALID);

    // Remove enchantments
    if (leavewhich == LEAVEFIRST)
    {
      // Remove all enchantments except top one
      enchant = rchr.firstenchant;
      if (VALID_ENC(enchant))
      {
        while (VALID_ENC(EncList[enchant].nextenchant))
        {
          remove_enchant(EncList[enchant].nextenchant);
        }
      }
    }

    if (leavewhich == LEAVENONE)
    {
      // Remove all enchantments
      disenchant_character(character);
    }

    // Stuff that must be set
    rchr.model     = profile;

    Uint8  tmp_gender = rchr.gender;
    Uint16 tmp_weight = rchr.weight;
    rchr.getData() = rcap.getData();


    // Gender
    if (rcap.gender == GENRANDOM) // GENRANDOM means keep old gender
    { rchr.gender = tmp_gender; }

    // AI stuff
    rchr.scr_ref = ProfileList[profile].scr_ref;
    rchr.ai.state = 0;
    rchr.ai.time = 0;
    rchr.ai.latch.clear();
    rchr.ai.turn_mode = TURNMODE_VELOCITY;

    // Flags
    rchr.jumptime   = DELAY_JUMP;
    rchr.jumpready  = false;
    rchr.jumpnumber = rchr.jumpnumberreset;

    // Character size and bumping
    rchr.scale_goto = rchr.scale_vert;

    rchr.shadow_size_save   = rchr.shadow_size;
    rchr.bump_size_save     = rchr.calc_bump_size;
    rchr.bump_size_big_save = rchr.calc_bump_size_big;
    rchr.bump_height_save   = rchr.calc_bump_height;

    make_one_character_matrix(rchr);
    rchr.calculate_bumpers();

    rchr.weight     *= rchr.scale_horiz * rchr.scale_horiz *rchr.scale_vert;

    // Image rendering
    rchr.off = vec2_t(0,0);

    // AI and action stuff
    rchr.act.ready = true;
    rchr.act.keep = false;
    rchr.act.loop = false;
    rchr.act.which = ACTION_DA;
    rchr.act.next = ACTION_DA;
    rchr.ani.lip = 0;
    rchr.ani.frame = rmad.framestart;
    rchr.ani.last = rchr.ani.frame;
    rchr.holding_weight = 0;

    // Set the skin
    change_armor(character, skin);

    // Reaffirm them particles...
    reaffirm_attached_particles(character);

    // Set up initial fade in lighting
    rchr.light_x = 0;
    rchr.light_y = 0;
    rchr.light_a = 0;
    tnc = 0;
    while (tnc < rchr.getMad().transvertices)
    {
      rchr.md2_blended.Ambient[tnc] = 0;
      tnc++;
    }
  }
}


//--------------------------------------------------------------------------------------------
bool cost_mana(Uint16 character, int amount, Uint16 killer)
{
  // ZZ> This function takes mana from a character ( or gives mana ),
  //     and returns true if the character had enough to pay, or false
  //     otherwise
  int iTmp;

  iTmp = ChrList[character].mana - amount;
  if (iTmp < 0)
  {
    ChrList[character].mana = 0;
    if (ChrList[character].canchannel)
    {
      ChrList[character].life += iTmp;
      if (ChrList[character].life <= 0)
      {
        kill_character(character, character);
      }
      return true;
    }
    return false;
  }
  else
  {
    ChrList[character].mana = iTmp;
    if (iTmp > ChrList[character].manamax)
    {
      ChrList[character].mana = ChrList[character].manamax;
    }
  }
  return true;
}


//--------------------------------------------------------------------------------------------
void switch_team(int character, Uint8 team)
{
  // ZZ> This function makes a character join another team...
  if (team < TEAM_COUNT)
  {
    if (!ChrList[character].invictus)
    {
      TeamList[ChrList[character].baseteam].morale--;
      TeamList[team].morale++;
    }
    if ((!ChrList[character].is_mount || INVALID_CHR(ChrList[character].holding_which[SLOT_LEFT])) && 
        (!ChrList[character].is_item || INVALID_CHR(ChrList[character].held_by)))
    {
      ChrList[character].team = team;
    }
    ChrList[character].baseteam = team;
    if (TeamList[team].leader==NOLEADER)
    {
      TeamList[team].leader = character;
    }
  }
}



//--------------------------------------------------------------------------------------------
void Enchant_List::release_all_enchants()
{
  // ZZ> This functions frees all of the enchantments

  for(int i=0; i<SIZE; i++)
  {
    _list[i]._on = false;
  };

  _setup();
}

//--------------------------------------------------------------------------------------------
Uint32 Eve_List::load_one_eve(const char* pathname, Uint32 cap_ref)
{
  char szLoadName[0x0100];

  Uint32 eve_ref = get_free();
  if(INVALID==eve_ref) return eve_ref;

  make_newloadname(pathname, "enchant.txt", szLoadName);
  if(!_list[eve_ref].load(szLoadName, cap_ref, _list[eve_ref]))
  {
    return_one(eve_ref);
    eve_ref = INVALID;
  };

  return eve_ref;
};

//--------------------------------------------------------------------------------------------
bool Eve::load(const char* szLoadName, Uint32 prof_ref, Eve & reve)
{
  // ZZ> This function loads the enchantment associated with an object
  FILE* fileread;
  char cTmp;
  int iTmp;
  IDSZ idsz, test;
  int num;

  globalname = szLoadName;
  fileread = fopen(szLoadName, "r");
  if (!fileread) return false;

  // true/false values
  reve.retarget = get_next_bool(fileread);
  reve.override = get_next_bool(fileread);
  reve.removeoverridden = get_next_bool(fileread);
  reve.killonend = get_next_bool(fileread);
  reve.poofonend = get_next_bool(fileread);

  // More stuff
  reve.time = get_next_int(fileread);
  reve.endmessage = get_next_int(fileread);

  // Drain stuff
  reve.ownermana = 0x0100*get_next_float(fileread);
  reve.targetmana = 0x0100*get_next_float(fileread);
  reve.endifcantpay = get_next_bool(fileread);
  reve.ownerlife = 0x0100*get_next_float(fileread);
  reve.targetlife = 0x0100*get_next_float(fileread);

  // Specifics
  goto_colon(fileread);  reve.dontdamagetype = get_damage_type(fileread);
  goto_colon(fileread);  reve.onlydamagetype = get_damage_type(fileread);
  goto_colon(fileread);  reve.removedbyidsz  = get_idsz(fileread);

  // Now the set values
  num = 0;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_type(fileread);

  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_int(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_int(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_int(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_mods(fileread) | Uint32(get_int(fileread));
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_mods(fileread) | Uint32(get_int(fileread));
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_mods(fileread) | Uint32(get_int(fileread));
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_mods(fileread) | Uint32(get_int(fileread));
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_mods(fileread) | Uint32(get_int(fileread));
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_mods(fileread) | Uint32(get_int(fileread));
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_mods(fileread) | Uint32(get_int(fileread));
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_damage_mods(fileread) | Uint32(get_int(fileread));
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_int(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_int(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_int(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_int(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_int(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_bool(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = get_bool(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  cTmp = get_first_letter(fileread);
  switch(toupper(cTmp))
  {
    case 'N': reve.setvalue[num] = MISNORMAL;  break;
    case 'R': reve.setvalue[num] = MISREFLECT; break;
    case 'D': reve.setvalue[num] = MISDEFLECT; break;
    default : reve.setvalue[num] = MISNONE;    break;
  };
  
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = 16 * get_float(fileread);
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = true;
  num++;
  goto_colon(fileread);
  reve.setyesno[num] = get_bool(fileread);
  reve.setvalue[num] = true;
  num++;

  // Now read in the add values
  num = 0;
  reve.addvalue[num] = 16 * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 0x7F * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 0x7F * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 4 * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 0x7F * get_next_float(fileread);
  num++;
  reve.addvalue[num] = get_next_int(fileread);
  num++;
  reve.addvalue[num] = get_next_int(fileread);
  num++;
  reve.addvalue[num] = get_next_int(fileread);
  num++;
  reve.addvalue[num] = get_next_int(fileread);
  num++;
  reve.addvalue[num] = - get_next_int(fileread); // Defense is backwards
  num++;
  reve.addvalue[num] = 4 * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 4 * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 4 * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 4 * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 4 * get_next_float(fileread);
  num++;
  reve.addvalue[num] = 4 * get_next_float(fileread);
  num++;

  // Clear expansions...
  reve.contspawntime = 0;
  reve.contspawnamount = 0;
  reve.contspawnfacingadd = 0;
  reve.contspawnpip = 0;
  reve.waveindex = NULL;
  reve.frequency = 11025;
  reve.stayifnoowner = 0;
  reve.overlay = 0;
  // Read expansions
  while (goto_colon_yesno(fileread))
  {
    idsz = get_idsz(fileread);
    fscanf(fileread, "%c%d", &cTmp, &iTmp);

    if (idsz == IDSZ("AMOU"))       reve.contspawnamount = iTmp;
    else if (idsz == IDSZ("TYPE"))  reve.contspawnpip = iTmp;
    else if (idsz == IDSZ("TIME"))  reve.contspawntime = iTmp;
    else if (idsz == IDSZ("FACE"))  reve.contspawnfacingadd = iTmp;
    else if (idsz == IDSZ("SEND"))
    {
      reve.waveindex = NULL;
      if (iTmp >= 0 && iTmp < MAXWAVE)
      {
        reve.waveindex = ProfileList[ prof_ref ].waveindex[iTmp];
      }
    }
    else if (idsz == IDSZ("SFRQ"))  reve.frequency = iTmp;
    else if (idsz == IDSZ("STAY"))  reve.stayifnoowner = (iTmp!=0);
    else if (idsz == IDSZ("OVER"))  reve.overlay = iTmp;
  }

  // All done ( finally )
  fclose(fileread);

    return true;
}


//--------------------------------------------------------------------------------------------
Uint16 spawn_one_enchant(Uint16 owner, Uint16 target, Uint16 spawner, Uint16 modeloptional)
{
  // ZZ> This function enchants a target, returning the enchantment index or Enchant_List::INVALID
  //     if failed
  Uint32 enc_ref, overlay;
  int add;

  cout << "---- spawning enchant " << endl;

  EVE_REF eve_ref;
  Uint32 eve_prof;
  if (VALID_MODEL(modeloptional))
  {
    // The enchantment type is given explicitly
    eve_ref  = ProfileList[modeloptional].eve_ref;
    eve_prof = modeloptional;
  }
  else if( VALID_MODEL(ChrList[spawner].model) )
  {
    // The enchantment type is given by the spawner
    eve_ref  = ProfileList[ChrList[spawner].model].eve_ref;
    eve_prof = ChrList[spawner].model;
  }
  else
  {
    // No valid template
    cout << "---- spawn_one_enchant failed" << endl;
    return Enchant_List::INVALID;
  }


  if ( !INVALID_EVE(eve_ref) )
  {
    // found template is not valid
    cout << "---- spawn_one_enchant failed" << endl;
    return Enchant_List::INVALID;
  }
  Eve & reve = EveList[eve_ref];
  cout << "\teve   == " << EveList[eve_ref].filename << endl;
  cout << "\tmodel == " << ProfileList[eve_prof].filename << ", " << ProfileList[eve_prof].name << endl;


  // Target and owner must both be alive and on and valid
  if (INVALID_CHR(target) || !ChrList[target].alive)
  {
    cout << "---- spawn_one_enchant failed" << endl;
    return Enchant_List::INVALID;
  }
  cout << "\ttarget == " << ChrList[target].name << "(" << ChrList[target].getCap().classname << ")" << endl;
 

  if (INVALID_CHR(owner) || !ChrList[owner].alive)
  {
    // Invalid target
    cout << "---- spawn_one_enchant failed" << endl;
    return Enchant_List::INVALID;
  }
  cout << "\towner == " << ChrList[owner].name << "(" << ChrList[owner].getCap().classname << ")" << endl;


  // Should it choose an inhand item?
  if (reve.retarget)
  {
    // Is at least one valid?
    if (INVALID_CHR(ChrList[target].holding_which[SLOT_LEFT]) && INVALID_CHR(ChrList[target].holding_which[SLOT_RIGHT]))
    {
      // No weapons to pick
      cout << "---- spawn_one_enchant failed" << endl;
      return Enchant_List::INVALID;
    }

    // Left, right, or both are valid
    if (INVALID_CHR(ChrList[target].holding_which[SLOT_LEFT]))
    {
      // Only right hand is valid
      target = ChrList[target].holding_which[SLOT_RIGHT];
    }
    else
    {
      // Pick left hand
      target = ChrList[target].holding_which[SLOT_LEFT];
    }
  }
  Character & rtarget = ChrList[target];
  cout << "\ttarget == " << rtarget.name << "(" << rtarget.getCap().classname << ")" << endl;


  // Make sure it's valid
  if (reve.dontdamagetype != DAMAGE_NULL)
  {
    if ((rtarget.damagemodifier[reve.dontdamagetype]&7)>=3) // Invert | Shift = 7
    {
      cout << "---- spawn_one_enchant failed" << endl;
      return Enchant_List::INVALID;
    }
  }


  if (reve.onlydamagetype != DAMAGE_NULL)
  {
    if (rtarget.damagetargettype != reve.onlydamagetype)
    {
      cout << "---- spawn_one_enchant failed" << endl;
      return Enchant_List::INVALID;
    }
  }


  // Find an enchant for us to use
  enc_ref = EncList.get_free();
  if (Enchant_List::INVALID == enc_ref)
  {
    cout << "---- spawn_one_enchant failed" << endl;
    return Enchant_List::INVALID;
  };

  Enchant & renc = EncList[enc_ref];

  // Make a new one
  renc._on = true;
  renc.target = target;
  renc.owner = owner;
  renc.spawner = spawner;
  if (VALID_CHR(spawner))
  {
    ChrList[spawner].undoenchant = enc_ref;
  }
  renc.eve_prof   = eve_prof;
  renc.time       = reve.time;
  renc.spawntime  = 1;
  renc.ownermana  = reve.ownermana;
  renc.ownerlife  = reve.ownerlife;
  renc.targetmana = reve.targetmana;
  renc.targetlife = reve.targetlife;

  // Add it as first in the list
  renc.nextenchant = rtarget.firstenchant;
  rtarget.firstenchant = enc_ref;

  // Now set all of the specific values, morph first
  set_enchant_value(enc_ref, SETMORPH, renc.eve_prof);
  set_enchant_value(enc_ref, SETDAMAGETYPE, renc.eve_prof);
  set_enchant_value(enc_ref, SETNUMBEROFJUMPS, renc.eve_prof);
  set_enchant_value(enc_ref, SETLIFEBARCOLOR, renc.eve_prof);
  set_enchant_value(enc_ref, SETMANABARCOLOR, renc.eve_prof);
  set_enchant_value(enc_ref, SETSLASHMODIFIER, renc.eve_prof);
  set_enchant_value(enc_ref, SETCRUSHMODIFIER, renc.eve_prof);
  set_enchant_value(enc_ref, SETPOKEMODIFIER, renc.eve_prof);
  set_enchant_value(enc_ref, SETHOLYMODIFIER, renc.eve_prof);
  set_enchant_value(enc_ref, SETEVILMODIFIER, renc.eve_prof);
  set_enchant_value(enc_ref, SETFIREMODIFIER, renc.eve_prof);
  set_enchant_value(enc_ref, SETICEMODIFIER, renc.eve_prof);
  set_enchant_value(enc_ref, SETZAPMODIFIER, renc.eve_prof);
  set_enchant_value(enc_ref, SETFLASHINGAND, renc.eve_prof);
  set_enchant_value(enc_ref, SETLIGHTBLEND, renc.eve_prof);
  set_enchant_value(enc_ref, SETALPHABLEND, renc.eve_prof);
  set_enchant_value(enc_ref, SETSHEEN, renc.eve_prof);
  set_enchant_value(enc_ref, SETFLYTOHEIGHT, renc.eve_prof);
  set_enchant_value(enc_ref, SETWALKONWATER, renc.eve_prof);
  set_enchant_value(enc_ref, SETCANSEEINVISIBLE, renc.eve_prof);
  set_enchant_value(enc_ref, SETMISSILETREATMENT, renc.eve_prof);
  set_enchant_value(enc_ref, SETCOSTFOREACHMISSILE, renc.eve_prof);
  set_enchant_value(enc_ref, SETCHANNEL, renc.eve_prof);

  // Now do all of the stat adds
  add = 0;
  while (add < EVE_ADDVALUE_COUNT)
  {
    add_enchant_value(enc_ref, add, eve_ref);
    add++;
  }

  // Create an overlay character?
  renc.overlay = Character_List::INVALID;
  if (reve.overlay)
  {
    overlay = spawn_one_character(rtarget.pos, rtarget.turn_lr,  rtarget.vel, rtarget.vel_lr,renc.eve_prof, rtarget.team, 0);

    if (VALID_CHR(overlay))
    {
      Character & roverlay = ChrList[overlay];

      renc.overlay       = overlay;  // Kill this character on end...
      roverlay.ai.target = target;
      roverlay.ai.state  = reve.overlay;
      roverlay.overlay   = true;

      // Start out with ActionMJ...  Object activated
      if ( roverlay.getMad().actinfo[ACTION_MJ].valid)
      {
        roverlay.act.which = ACTION_MJ;
        roverlay.ani.lip   = 0;
        roverlay.ani.frame = roverlay.getMad().actinfo[ACTION_MJ].start;
        roverlay.ani.last  = roverlay.ani.frame;
        roverlay.act.ready = false;
      }

      roverlay.light = 254;  // Assume it's transparent...
    }
  }


  return enc_ref;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Cap & Enchant::getCap()        { return CapList[ProfileList[eve_prof].cap_ref]; }
Mad & Enchant::getMad()        { return MadList[ProfileList[eve_prof].mad_ref]; }
Eve & Enchant::getEve()        { return EveList[ProfileList[eve_prof].eve_ref]; }
Script_Info & Enchant::getAI() { return ScrList[ProfileList[eve_prof].scr_ref]; }
Pip & Enchant::getPip(int i)   { return PipList[ProfileList[eve_prof].prtpip[i]]; }



