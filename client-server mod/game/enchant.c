/* Egoboo - enchant.c
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
void do_enchant_spawn(Uint32 * rand_idx)
{
  // ZZ> This function lets enchantments spawn particles
  int cnt, tnc;
  Uint16 facing, particle, eve, character;
  Uint32 enc_randie = *rand_idx;

  // do exactly one iteration
  RANDIE(*rand_idx);

  cnt = 0;
  while (cnt < MAXENCHANT)
  {
    if (EncList[cnt].on)
    {
      eve = EncList[cnt].eve;
      if (EveList[eve].contspawnamount > 0)
      {
        EncList[cnt].spawntime--;
        if (EncList[cnt].spawntime == 0)
        {
          character = EncList[cnt].target;
          EncList[cnt].spawntime = EveList[eve].contspawntime;
          facing = ChrList[character].turnleftright;
          tnc = 0;
          while (tnc < EveList[eve].contspawnamount)
          {
            particle = spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos,
                                          facing, eve, EveList[eve].contspawnpip,
                                          MAXCHR, SPAWNLAST, ChrList[EncList[cnt].owner].team, EncList[cnt].owner, tnc, MAXCHR, &enc_randie);
            facing += EveList[eve].contspawnfacingadd;
            tnc++;
          }
        }
      }
    }
    cnt++;
  }
}


//--------------------------------------------------------------------------------------------
void disenchant_character(GAME_STATE * gs, Uint16 cnt, Uint32 * rand_idx)
{
  // ZZ> This function removes all enchantments from a character

  Uint32 enc_randie = *rand_idx;

  RANDIE(*rand_idx);

  while (ChrList[cnt].firstenchant != MAXENCHANT)
  {
    remove_enchant(gs, ChrList[cnt].firstenchant, &enc_randie);
  }
}

//char.c
//--------------------------------------------------------------------------------------------
void damage_character(GAME_STATE * gs, Uint16 character, Uint16 direction,
                      int damagebase, int damagerand, Uint8 damagetype, Uint8 team,
                      Uint16 attacker, Uint16 effects, Uint32 * rand_idx)
{
  // ZZ> This function calculates and applies damage to a character.  It also
  //     sets alerts and begins actions.  Blocking and frame invincibility
  //     are done here too.  Direction is 0 if the attack is coming head on,
  //     16384 if from the right, 32768 if from the back, 49152 if from the
  //     left.
  int tnc;
  Uint16 action;
  int damage, basedamage;
  Uint16 experience, model, left, right;
  Uint32 chr_randie = *rand_idx;

  RANDIE(*rand_idx);


  if (ChrList[character].alive && damagebase >= 0 && damagerand >= 1)
  {
    // Lessen damage for resistance, 0 = Weakness, 1 = Normal, 2 = Resist, 3 = Big Resist
    // This can also be used to lessen effectiveness of healing
    damage = damagebase + (ego_rand(&ego_rand_seed) % damagerand);
    basedamage = damage;
    damage = damage >> (ChrList[character].damagemodifier[damagetype] & DAMAGESHIFT);


    // Allow charging (Invert damage to mana)
    if (ChrList[character].damagemodifier[damagetype]&DAMAGECHARGE)
    {
      ChrList[character].mana += damage;
      if (ChrList[character].mana > ChrList[character].manamax)
      {
        ChrList[character].mana = ChrList[character].manamax;
      }
      return;
    }


    // Invert damage to heal
    if (ChrList[character].damagemodifier[damagetype]&DAMAGEINVERT)
      damage = -damage;


    // Remember the damage type
    ChrList[character].damagetypelast = damagetype;
    ChrList[character].directionlast = direction;


    // Do it already
    if (damage > 0)
    {
      // Only damage if not invincible
      if (ChrList[character].damagetime == 0 && ChrList[character].invictus == bfalse)
      {
        model = ChrList[character].model;
        if ((effects&DAMFXBLOC) == bfalse)
        {
          // Only damage if hitting from proper direction
          if (MadFrame[ChrList[character].frame].framefx&MADFXINVICTUS)
          {
            // I Frame...
            direction -= CapList[model].iframefacing;
            left = (~CapList[model].iframeangle);
            right = CapList[model].iframeangle;
            // Check for shield
            if (ChrList[character].action >= ACTIONPA && ChrList[character].action <= ACTIONPD)
            {
              // Using a shield?
              if (ChrList[character].action < ACTIONPC)
              {
                // Check left hand
                if (ChrList[character].holdingwhich[0] != MAXCHR)
                {
                  left = (~CapList[ChrList[ChrList[character].holdingwhich[0]].model].iframeangle);
                  right = CapList[ChrList[ChrList[character].holdingwhich[0]].model].iframeangle;
                }
              }
              else
              {
                // Check right hand
                if (ChrList[character].holdingwhich[1] != MAXCHR)
                {
                  left = (~CapList[ChrList[ChrList[character].holdingwhich[1]].model].iframeangle);
                  right = CapList[ChrList[ChrList[character].holdingwhich[1]].model].iframeangle;
                }
              }
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
          if (direction > left || direction < right)
          {
            damage = 0;
          }
        }



        if (damage != 0)
        {
          if (effects&DAMFXARMO)
          {
            ChrList[character].life -= damage;
          }
          else
          {
            ChrList[character].life -= ((damage * ChrList[character].defense) >> 8);
          }


          if (basedamage > MINDAMAGE)
          {
            // Call for help if below 1/2 life
            if (ChrList[character].life < (ChrList[character].lifemax >> 1))
              call_for_help(character);
            // Spawn blood particles
            if (CapList[model].bloodvalid && (damagetype < DAMAGEHOLY || CapList[model].bloodvalid == ULTRABLOODY))
            {
              spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos,
                                 ChrList[character].turnleftright + direction, ChrList[character].model, CapList[model].bloodprttype,
                                 MAXCHR, SPAWNLAST, ChrList[character].team, character, 0, MAXCHR, &chr_randie);
            }
            // Set attack alert if it wasn't an accident
            if (team == DAMAGETEAM)
            {
              ChrList[character].attacklast = MAXCHR;
            }
            else
            {
              // Don't alert the character too much if under constant fire
              if (ChrList[character].carefultime == 0)
              {
                // Don't let characters chase themselves...  That would be silly
                if (attacker != character)
                {
                  ChrList[character].alert = ChrList[character].alert | ALERTIFATTACKED;
                  ChrList[character].attacklast = attacker;
                  ChrList[character].carefultime = CAREFULTIME;
                }
              }
            }
          }


          // Taking damage action
          action = ACTIONHA;
          if (ChrList[character].life < 0)
          {
            // Character has died
            ChrList[character].alive = bfalse;
            disenchant_character(gs, character, &chr_randie);
            ChrList[character].waskilled = btrue;
            ChrList[character].keepaction = btrue;
            ChrList[character].life = -1;
            ChrList[character].platform = btrue;
            ChrList[character].bumpdampen = ChrList[character].bumpdampen / 2.0;
            action = ACTIONKA;
            // Give kill experience
            experience = CapList[model].experienceworth + (ChrList[character].experience * CapList[model].experienceexchange);
            if (attacker < MAXCHR)
            {
              // Set target
              ChrList[character].aitarget = attacker;
              if (team == DAMAGETEAM)  ChrList[character].aitarget = character;
              if (team == NULLTEAM)  ChrList[character].aitarget = character;
              // Award direct kill experience
              if (TeamList[ChrList[attacker].team].hatesteam[ChrList[character].team])
              {
                give_experience(attacker, experience, XPKILLENEMY);
              }
              // Check for hated
              if (CapList[ChrList[attacker].model].idsz[IDSZHATE] == CapList[model].idsz[IDSZPARENT] ||
                  CapList[ChrList[attacker].model].idsz[IDSZHATE] == CapList[model].idsz[IDSZTYPE])
              {
                give_experience(attacker, experience, XPKILLHATED);
              }
            }
            // Clear all shop passages that it owned...
            tnc = 0;
            while (tnc < numshoppassage)
            {
              if (shopowner[tnc] == character)
              {
                shopowner[tnc] = NOOWNER;
              }
              tnc++;
            }
            // Let the other characters know it died
            tnc = 0;
            while (tnc < MAXCHR)
            {
              if (ChrList[tnc].on && ChrList[tnc].alive)
              {
                if (ChrList[tnc].aitarget == character)
                {
                  ChrList[tnc].alert = ChrList[tnc].alert | ALERTIFTARGETKILLED;
                }
                if ((TeamList[ChrList[tnc].team].hatesteam[team] == bfalse) && (TeamList[ChrList[tnc].team].hatesteam[ChrList[character].team] == btrue))
                {
                  // All allies get team experience, but only if they also hate the dead guy's team
                  give_experience(tnc, experience, XPTEAMKILL);
                }
              }
              tnc++;
            }
            // Check if it was a leader
            if (TeamList[ChrList[character].team].leader == character)
            {
              // It was a leader, so set more alerts
              tnc = 0;
              while (tnc < MAXCHR)
              {
                if (ChrList[tnc].on && ChrList[tnc].team == ChrList[character].team)
                {
                  // All folks on the leaders team get the alert
                  ChrList[tnc].alert = ChrList[tnc].alert | ALERTIFLEADERKILLED;
                }
                tnc++;
              }
              // The team now has no leader
              TeamList[ChrList[character].team].leader = NOLEADER;
            }
            detach_character_from_mount(gs, character, btrue, bfalse, &chr_randie);
            action += (ego_rand(&ego_rand_seed) & 3);
            play_action(character, action, bfalse);
            // Turn off all sounds if it's a player
            if (ChrList[character].isplayer)
            {
              tnc = 0;
              while (tnc < MAXWAVE)
              {
                //stop_sound(CapList[ChrList[character].model].waveindex[tnc]);
                //TODO Zefz: Do we need this? This makes all sounds a character makes stop when it dies...
                tnc++;
              }
            }
            // Afford it one last thought if it's an AI
            TeamList[ChrList[character].baseteam].morale--;
            ChrList[character].team = ChrList[character].baseteam;
            ChrList[character].alert = ALERTIFKILLED;
            ChrList[character].sparkle = NOSPARKLE;
            ChrList[character].aitime = 1;  // No timeout...
            let_character_think(gs, character, &chr_randie);
          }
          else
          {
            if (basedamage > MINDAMAGE)
            {
              action += (ego_rand(&ego_rand_seed) & 3);
              play_action(character, action, bfalse);
              // Make the character invincible for a limited time only
              if (!(effects & DAMFXTIME))
                ChrList[character].damagetime = DAMAGETIME;
            }
          }
        }
        else
        {
          // Spawn a defend particle
          spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, ChrList[character].turnleftright, MAXMODEL, DEFEND, MAXCHR, SPAWNLAST, NULLTEAM, MAXCHR, 0, MAXCHR, &chr_randie);
          ChrList[character].damagetime = DEFENDTIME;
          ChrList[character].alert = ChrList[character].alert | ALERTIFBLOCKED;
        }
      }
    }
    else if (damage < 0)
    {
      ChrList[character].life -= damage;
      if (ChrList[character].life > ChrList[character].lifemax)  ChrList[character].life = ChrList[character].lifemax;

      // Isssue an alert
      ChrList[character].alert = ChrList[character].alert | ALERTIFHEALED;
      ChrList[character].attacklast = attacker;
      if (team != DAMAGETEAM)
      {
        ChrList[character].attacklast = MAXCHR;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void kill_character(GAME_STATE * gs, Uint16 character, Uint16 killer, Uint32 * rand_idx)
{
  // ZZ> This function kills a character...  MAXCHR killer for accidental death
  Uint8 modifier;
  Uint32 chr_randie = *rand_idx;

  RANDIE(*rand_idx);

  if (ChrList[character].alive)
  {
    ChrList[character].damagetime = 0;
    ChrList[character].life = 1;
    modifier = ChrList[character].damagemodifier[DAMAGECRUSH];
    ChrList[character].damagemodifier[DAMAGECRUSH] = 1;
    if (killer != MAXCHR)
    {
      damage_character(gs, character, 0, 512, 1, DAMAGECRUSH, ChrList[killer].team, killer, DAMFXARMO | DAMFXBLOC, &chr_randie);
    }
    else
    {
      damage_character(gs, character, 0, 512, 1, DAMAGECRUSH, DAMAGETEAM, ChrList[character].bumplast, DAMFXARMO | DAMFXBLOC, &chr_randie);
    }
    ChrList[character].damagemodifier[DAMAGECRUSH] = modifier;
  }
}

//--------------------------------------------------------------------------------------------
void spawn_poof(Uint16 character, Uint16 profile, Uint32 * rand_idx)
{
  // ZZ> This function spawns a character poof
  Uint16 sTmp;
  Uint16 origin;
  int iTmp;
  Uint32 chr_randie = *rand_idx;

  RANDIE(*rand_idx);


  sTmp = ChrList[character].turnleftright;
  iTmp = 0;
  origin = ChrList[character].aiowner;
  while (iTmp < CapList[profile].gopoofprtamount)
  {
    spawn_one_particle(ChrList[character].oldx, ChrList[character].oldy, ChrList[character].oldz,
                       sTmp, profile, CapList[profile].gopoofprttype,
                       MAXCHR, SPAWNLAST, ChrList[character].team, origin, iTmp, MAXCHR, &chr_randie);
    sTmp += CapList[profile].gopoofprtfacingadd;
    iTmp++;
  }
}

//--------------------------------------------------------------------------------------------
void naming_names(int profile)
{
  // ZZ> This function generates a random name
  int read, write, section, mychop;
  char cTmp;

  if (CapList[profile].sectionsize[0] == 0)
  {
    namingnames[0] = 'B';
    namingnames[1] = 'l';
    namingnames[2] = 'a';
    namingnames[3] = 'h';
    namingnames[4] = 0;
  }
  else
  {
    write = 0;
    section = 0;
    while (section < MAXSECTION)
    {
      if (CapList[profile].sectionsize[section] != 0)
      {
        mychop = CapList[profile].sectionstart[section] + (ego_rand(&ego_rand_seed) % CapList[profile].sectionsize[section]);
        read = chopstart[mychop];
        cTmp = chopdata[read];
        while (cTmp != 0 && write < MAXCAPNAMESIZE - 1)
        {
          namingnames[write] = cTmp;
          write++;
          read++;
          cTmp = chopdata[read];
        }
      }
      section++;
    }
    if (write >= MAXCAPNAMESIZE) write = MAXCAPNAMESIZE - 1;
    namingnames[write] = 0;
  }
}

//--------------------------------------------------------------------------------------------
void read_naming(int profile, char *szLoadname)
{
  // ZZ> This function reads a naming file
  FILE *fileread;
  int section, chopinsection, cnt;
  char mychop[32], cTmp;

  fileread = fopen(szLoadname, "r");
  if (fileread)
  {
    section = 0;
    chopinsection = 0;
    while (goto_colon_yesno(fileread) && section < MAXSECTION)
    {
      fscanf(fileread, "%s", mychop);
      if (mychop[0] != 'S' || mychop[1] != 'T' || mychop[2] != 'O' || mychop[3] != 'P')
      {
        if (chopwrite >= CHOPDATACHUNK)  chopwrite = CHOPDATACHUNK - 1;
        chopstart[numchop] = chopwrite;
        cnt = 0;
        cTmp = mychop[0];
        while (cTmp != 0 && cnt < 31 && chopwrite < CHOPDATACHUNK)
        {
          if (cTmp == '_') cTmp = ' ';
          chopdata[chopwrite] = cTmp;
          cnt++;
          chopwrite++;
          cTmp = mychop[cnt];
        }
        if (chopwrite >= CHOPDATACHUNK)  chopwrite = CHOPDATACHUNK - 1;
        chopdata[chopwrite] = 0;  chopwrite++;
        chopinsection++;
        numchop++;
      }
      else
      {
        CapList[profile].sectionsize[section] = chopinsection;
        CapList[profile].sectionstart[section] = numchop - chopinsection;
        section++;
        chopinsection = 0;
      }
    }
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
void prime_names(void)
{
  // ZZ> This function prepares the name chopper for use
  int cnt, tnc;

  numchop = 0;
  chopwrite = 0;
  cnt = 0;
  while (cnt < MAXMODEL)
  {
    tnc = 0;
    while (tnc < MAXSECTION)
    {
      CapList[cnt].sectionstart[tnc] = MAXCHOP;
      CapList[cnt].sectionsize[tnc] = 0;
      tnc++;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void tilt_characters_to_terrain()
{
  // ZZ> This function sets all of the character's starting tilt values
  int cnt;
  Uint8 twist;

  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].stickybutt && ChrList[cnt].on)
    {
      twist = Mesh.fanlist[ChrList[cnt].onwhichfan].twist;
      ChrList[cnt].turnmaplr = maplrtwist[twist];
      ChrList[cnt].turnmapud = mapudtwist[twist];
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
int spawn_one_character(float x, float y, float z, int profile, Uint8 team,
                        Uint8 skin, Uint16 facing, char *name, int override, Uint32 * rand_idx)
{
  // ZZ> This function spawns a character and returns the character's index number
  //     if it worked, MAXCHR otherwise
  int cnt, tnc, ix, iy;
  Uint32 chr_randie = *rand_idx;

  // do exactly one iteration of the andom number generator
  RANDIE(*rand_idx);

  // Make sure the team is valid
  if (team > MAXTEAM - 1)
    team = MAXTEAM - 1;


  // Get a new character
  cnt = MAXCHR;
  if (MadList[profile].used)
  {
    if (override < MAXCHR)
    {
      cnt = get_free_character();
      if (cnt != override)
      {
        // Picked the wrong one, so put this one back and find the right one
        tnc = 0;
        while (tnc < MAXCHR)
        {
          if (freechrlist[tnc] == override)
          {
            freechrlist[tnc] = cnt;
            tnc = MAXCHR;
          }
          tnc++;
        }
        cnt = override;
      }
    }
    else
    {
      cnt = get_free_character();
    }
    if (cnt != MAXCHR)
    {
      // IMPORTANT!!!
      ChrList[cnt].indolist = bfalse;
      ChrList[cnt].isequipped = bfalse;
      ChrList[cnt].sparkle = NOSPARKLE;
      ChrList[cnt].overlay = bfalse;
      ChrList[cnt].missilehandler = cnt;

      // SetXY stuff...  Just in case
      tnc = 0;
      while (tnc < MAXSTOR)
      {
        ChrList[cnt].aix[tnc] = 0;
        ChrList[cnt].aiy[tnc] = 0;
        tnc++;
      }

      // Set up model stuff
      ChrList[cnt].on = btrue;
      ChrList[cnt].reloadtime = 0;
      ChrList[cnt].inwhichhand = GRIPLEFT;
      ChrList[cnt].waskilled = bfalse;
      ChrList[cnt].inpack = bfalse;
      ChrList[cnt].nextinpack = MAXCHR;
      ChrList[cnt].numinpack = 0;
      ChrList[cnt].model = profile;
      ChrList[cnt].basemodel = profile;
      ChrList[cnt].stoppedby = CapList[profile].stoppedby;
      ChrList[cnt].lifeheal = CapList[profile].lifeheal;
      ChrList[cnt].manacost = CapList[profile].manacost;
      ChrList[cnt].inwater = bfalse;
      ChrList[cnt].nameknown = CapList[profile].nameknown;
      ChrList[cnt].ammoknown = CapList[profile].nameknown;
      ChrList[cnt].hitready = btrue;
      ChrList[cnt].boretime = BORETIME;
      ChrList[cnt].carefultime = CAREFULTIME;
      ChrList[cnt].canbecrushed = bfalse;
      ChrList[cnt].damageboost = 0;
      ChrList[cnt].icon = CapList[profile].icon;


      // Enchant stuff
      ChrList[cnt].firstenchant = MAXENCHANT;
      ChrList[cnt].undoenchant = MAXENCHANT;
      ChrList[cnt].canseeinvisible = CapList[profile].canseeinvisible;
      ChrList[cnt].canchannel = bfalse;
      ChrList[cnt].missiletreatment = MISNORMAL;
      ChrList[cnt].missilecost = 0;

	  //Skill Expansions
      ChrList[cnt].canseekurse = CapList[profile].canseekurse;
      ChrList[cnt].canusedivine = CapList[profile].canusedivine;
      ChrList[cnt].canusearcane = CapList[profile].canusearcane;
      ChrList[cnt].candisarm = CapList[profile].candisarm;
	  ChrList[cnt].canjoust = CapList[profile].canjoust;
	  ChrList[cnt].canusetech = CapList[profile].canusetech;
	  ChrList[cnt].canusepoison = CapList[profile].canusepoison;
  	  ChrList[cnt].canuseadvancedweapons = CapList[profile].canuseadvancedweapons;
	  ChrList[cnt].canbackstab = CapList[profile].canbackstab;
	  ChrList[cnt].canread = CapList[profile].canread;


      // Kurse state
      ChrList[cnt].iskursed = ((RANDIE(chr_randie) % 100) < CapList[profile].kursechance);
      if (CapList[profile].isitem == bfalse)  ChrList[cnt].iskursed = bfalse;


      // Ammo
      ChrList[cnt].ammomax = CapList[profile].ammomax;
      ChrList[cnt].ammo = CapList[profile].ammo;


      // Gender
      ChrList[cnt].gender = CapList[profile].gender;
      if (ChrList[cnt].gender == GENRANDOM)  ChrList[cnt].gender = GENFEMALE + (RANDIE(chr_randie) & 1);



      // Team stuff
      ChrList[cnt].team = team;
      ChrList[cnt].baseteam = team;
      ChrList[cnt].counter = TeamList[team].morale;
      if (CapList[profile].invictus == bfalse)  TeamList[team].morale++;
      ChrList[cnt].order = 0;
      // Firstborn becomes the leader
      if (TeamList[team].leader == NOLEADER)
      {
        TeamList[team].leader = cnt;
      }
      // Skin
      if (CapList[profile].skinoverride != NOSKINOVERRIDE)
      {
        skin = CapList[profile].skinoverride % MAXSKIN;
      }
      if (skin >= MadList[profile].skins)
      {
        skin = 0;
        if (MadList[profile].skins > 1)
        {
          skin = RANDIE(chr_randie) % MadList[profile].skins;
        }
      }
      ChrList[cnt].texture = MadList[profile].skinstart + skin;
      // Life and Mana
      ChrList[cnt].alive = btrue;
      ChrList[cnt].lifecolor = CapList[profile].lifecolor;
      ChrList[cnt].manacolor = CapList[profile].manacolor;
      ChrList[cnt].lifemax = generate_number(CapList[profile].lifebase, CapList[profile].liferand);
      ChrList[cnt].life = ChrList[cnt].lifemax;
      ChrList[cnt].lifereturn = CapList[profile].lifereturn;
      ChrList[cnt].manamax = generate_number(CapList[profile].manabase, CapList[profile].manarand);
      ChrList[cnt].manaflow = generate_number(CapList[profile].manaflowbase, CapList[profile].manaflowrand);
      ChrList[cnt].manareturn = generate_number(CapList[profile].manareturnbase, CapList[profile].manareturnrand) >> MANARETURNSHIFT;
      ChrList[cnt].mana = ChrList[cnt].manamax;
      // SWID
      ChrList[cnt].strength = generate_number(CapList[profile].strengthbase, CapList[profile].strengthrand);
      ChrList[cnt].wisdom = generate_number(CapList[profile].wisdombase, CapList[profile].wisdomrand);
      ChrList[cnt].intelligence = generate_number(CapList[profile].intelligencebase, CapList[profile].intelligencerand);
      ChrList[cnt].dexterity = generate_number(CapList[profile].dexteritybase, CapList[profile].dexterityrand);
      // Damage
      ChrList[cnt].defense = CapList[profile].defense[skin];
      ChrList[cnt].reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;
      ChrList[cnt].damagetargettype = CapList[profile].damagetargettype;
      tnc = 0;
      while (tnc < MAXDAMAGETYPE)
      {
        ChrList[cnt].damagemodifier[tnc] = CapList[profile].damagemodifier[tnc][skin];
        tnc++;
      }
      // AI stuff
      ChrList[cnt].aitype = MadList[ChrList[cnt].model].ai;
      ChrList[cnt].isplayer = bfalse;
      ChrList[cnt].islocalplayer = bfalse;
      ChrList[cnt].alert = ALERTIFSPAWNED;
      ChrList[cnt].aistate = CapList[profile].stateoverride;
      ChrList[cnt].aicontent = CapList[profile].contentoverride;
      ChrList[cnt].aitarget = cnt;
      ChrList[cnt].aiowner = cnt;
      ChrList[cnt].aichild = cnt;
      ChrList[cnt].aitime = 0;
      ChrList[cnt].latchx = 0;
      ChrList[cnt].latchy = 0;
      ChrList[cnt].latchbutton = 0;
      ChrList[cnt].turnmode = TURNMODEVELOCITY;
      // Flags
      ChrList[cnt].stickybutt = CapList[profile].stickybutt;
      ChrList[cnt].openstuff = CapList[profile].canopenstuff;
      ChrList[cnt].transferblend = CapList[profile].transferblend;
      ChrList[cnt].enviro = CapList[profile].enviro;
      ChrList[cnt].waterwalk = CapList[profile].waterwalk;
      ChrList[cnt].platform = CapList[profile].platform;
      ChrList[cnt].isitem = CapList[profile].isitem;
      ChrList[cnt].invictus = CapList[profile].invictus;
      ChrList[cnt].ismount = CapList[profile].ismount;
      ChrList[cnt].cangrabmoney = CapList[profile].cangrabmoney;
      // Jumping
      ChrList[cnt].jump = CapList[profile].jump;
      ChrList[cnt].jumpnumber = 0;
      ChrList[cnt].jumpnumberreset = CapList[profile].jumpnumber;
      ChrList[cnt].jumptime = JUMPDELAY;
      // Other junk
      ChrList[cnt].flyheight = CapList[profile].flyheight;
      ChrList[cnt].maxaccel = CapList[profile].maxaccel[skin];
      ChrList[cnt].alpha = CapList[profile].alpha;
      ChrList[cnt].light = CapList[profile].light;
      ChrList[cnt].flashand = CapList[profile].flashand;
      ChrList[cnt].sheen = CapList[profile].sheen;
      ChrList[cnt].dampen = CapList[profile].dampen;
      // Character size and bumping
      ChrList[cnt].fat = CapList[profile].size;
      ChrList[cnt].sizegoto = ChrList[cnt].fat;
      ChrList[cnt].sizegototime = 0;
      ChrList[cnt].shadowsize = CapList[profile].shadowsize * ChrList[cnt].fat;
      ChrList[cnt].bumpsize = CapList[profile].bumpsize * ChrList[cnt].fat;
      ChrList[cnt].bumpsizebig = CapList[profile].bumpsizebig * ChrList[cnt].fat;
      ChrList[cnt].bumpheight = CapList[profile].bumpheight * ChrList[cnt].fat;

      ChrList[cnt].shadowsizesave = CapList[profile].shadowsize;
      ChrList[cnt].bumpsizesave = CapList[profile].bumpsize;
      ChrList[cnt].bumpsizebigsave = CapList[profile].bumpsizebig;
      ChrList[cnt].bumpheightsave = CapList[profile].bumpheight;

      ChrList[cnt].bumpdampen = CapList[profile].bumpdampen;
      ChrList[cnt].weight = CapList[profile].weight * ChrList[cnt].fat;
      if (CapList[profile].weight == 255) ChrList[cnt].weight = 65535;
      ChrList[cnt].bumplast = cnt;
      ChrList[cnt].attacklast = MAXCHR;
      ChrList[cnt].hitlast = cnt;
      // Grip info
      ChrList[cnt].attachedto = MAXCHR;
      ChrList[cnt].holdingwhich[0] = MAXCHR;
      ChrList[cnt].holdingwhich[1] = MAXCHR;
      // Image rendering
      ChrList[cnt].uoffset = 0;
      ChrList[cnt].voffset = 0;
      ChrList[cnt].uoffvel = CapList[profile].uoffvel;
      ChrList[cnt].voffvel = CapList[profile].voffvel;
      ChrList[cnt].redshift = 0;
      ChrList[cnt].grnshift = 0;
      ChrList[cnt].blushift = 0;
      // Movement
      ChrList[cnt].sneakspd = CapList[profile].sneakspd;
      ChrList[cnt].walkspd = CapList[profile].walkspd;
      ChrList[cnt].runspd = CapList[profile].runspd;


      // Set up position
      ChrList[cnt].xpos = x;
      ChrList[cnt].ypos = y;
      ChrList[cnt].oldx = x;
      ChrList[cnt].oldy = y;
      ChrList[cnt].turnleftright = facing;
      ChrList[cnt].lightturnleftright = 0;
      ix = x;
      iy = y;
      ChrList[cnt].onwhichfan = (ix >> 7) + Mesh.fanstart[iy>>7];
      ChrList[cnt].level = get_level(ChrList[cnt].xpos, ChrList[cnt].ypos, ChrList[cnt].onwhichfan, ChrList[cnt].waterwalk) + RAISE;
      if (z < ChrList[cnt].level)
        z = ChrList[cnt].level;
      ChrList[cnt].zpos = z;
      ChrList[cnt].oldz = z;
      ChrList[cnt].xstt = ChrList[cnt].xpos;
      ChrList[cnt].ystt = ChrList[cnt].ypos;
      ChrList[cnt].zstt = ChrList[cnt].zpos;
      ChrList[cnt].xvel = 0;
      ChrList[cnt].yvel = 0;
      ChrList[cnt].zvel = 0;
      ChrList[cnt].turnmaplr = 32768;  // These two mean on level surface
      ChrList[cnt].turnmapud = 32768;
      ChrList[cnt].scale = ChrList[cnt].fat * MadList[ChrList[cnt].model].scale * 4;


      // AI and action stuff
      ChrList[cnt].aigoto = 0;
      ChrList[cnt].aigotoadd = 1;
      ChrList[cnt].aigotox[0] = ChrList[cnt].xpos;
      ChrList[cnt].aigotoy[0] = ChrList[cnt].ypos;
      ChrList[cnt].actionready = btrue;
      ChrList[cnt].keepaction = bfalse;
      ChrList[cnt].loopaction = bfalse;
      ChrList[cnt].action = ACTIONDA;
      ChrList[cnt].nextaction = ACTIONDA;
      ChrList[cnt].lip = 0;
      ChrList[cnt].frame = MadList[ChrList[cnt].model].framestart;
      ChrList[cnt].lastframe = ChrList[cnt].frame;
      ChrList[cnt].passage = 0;
      ChrList[cnt].holdingweight = 0;


      // Timers set to 0
      ChrList[cnt].grogtime = 0;
      ChrList[cnt].dazetime = 0;


      // Money is added later
      ChrList[cnt].money = CapList[profile].money;


      // Name the character
      if (name == NULL)
      {
        // Generate a random name
        naming_names(profile);
        strncpy(ChrList[cnt].name, namingnames, sizeof(ChrList[cnt].name));
      }
      else
      {
        // A name has been given
        tnc = 0;
        while (tnc < MAXCAPNAMESIZE - 1)
        {
          ChrList[cnt].name[tnc] = name[tnc];
          tnc++;
        }
        ChrList[cnt].name[tnc] = 0;
      }

      // Set up initial fade in lighting
      tnc = 0;
      while (tnc < MadList[ChrList[cnt].model].transvertices)
      {
        ChrList[cnt].vrta[tnc] = 0;
        tnc++;
      }


      // Particle attachments
      tnc = 0;
      while (tnc < CapList[profile].attachedprtamount)
      {
        spawn_one_particle(ChrList[cnt].xpos, ChrList[cnt].ypos, ChrList[cnt].zpos,
                           0, ChrList[cnt].model, CapList[profile].attachedprttype,
                           cnt, SPAWNLAST + tnc, ChrList[cnt].team, cnt, tnc, MAXCHR, &chr_randie);
        tnc++;
      }
      ChrList[cnt].reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;


      // Experience
      tnc = generate_number(CapList[profile].experiencebase, CapList[profile].experiencerand);
      if (tnc > MAXXP) tnc = MAXXP;
      ChrList[cnt].experience = tnc;
      ChrList[cnt].experiencelevel = CapList[profile].leveloverride;
    }
  }
  return cnt;
}

//--------------------------------------------------------------------------------------------
void respawn_character(Uint16 character, Uint32 * rand_idx)
{
  // ZZ> This function respawns a character
  Uint16 item;
  Uint32 chr_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);

  if (ChrList[character].alive == bfalse)
  {
    spawn_poof(character, ChrList[character].model, &chr_randie);
    disaffirm_attached_particles(character, &chr_randie);
    ChrList[character].alive = btrue;
    ChrList[character].boretime = BORETIME;
    ChrList[character].carefultime = CAREFULTIME;
    ChrList[character].life = ChrList[character].lifemax;
    ChrList[character].mana = ChrList[character].manamax;
    ChrList[character].xpos = ChrList[character].xstt;
    ChrList[character].ypos = ChrList[character].ystt;
    ChrList[character].zpos = ChrList[character].zstt;
    ChrList[character].xvel = 0;
    ChrList[character].yvel = 0;
    ChrList[character].zvel = 0;
    ChrList[character].team = ChrList[character].baseteam;
    ChrList[character].canbecrushed = bfalse;
    ChrList[character].turnmaplr = 32768;  // These two mean on level surface
    ChrList[character].turnmapud = 32768;
    if (TeamList[ChrList[character].team].leader == NOLEADER)  TeamList[ChrList[character].team].leader = character;
    if (ChrList[character].invictus == bfalse)  TeamList[ChrList[character].baseteam].morale++;
    ChrList[character].actionready = btrue;
    ChrList[character].keepaction = bfalse;
    ChrList[character].loopaction = bfalse;
    ChrList[character].action = ACTIONDA;
    ChrList[character].nextaction = ACTIONDA;
    ChrList[character].lip = 0;
    ChrList[character].frame = MadList[ChrList[character].model].framestart;
    ChrList[character].lastframe = ChrList[character].frame;
    ChrList[character].platform = CapList[ChrList[character].model].platform;
    ChrList[character].flyheight = CapList[ChrList[character].model].flyheight;
    ChrList[character].bumpdampen = CapList[ChrList[character].model].bumpdampen;
    ChrList[character].bumpsize = CapList[ChrList[character].model].bumpsize * ChrList[character].fat;
    ChrList[character].bumpsizebig = CapList[ChrList[character].model].bumpsizebig * ChrList[character].fat;
    ChrList[character].bumpheight = CapList[ChrList[character].model].bumpheight * ChrList[character].fat;

    ChrList[character].bumpsizesave = CapList[ChrList[character].model].bumpsize;
    ChrList[character].bumpsizebigsave = CapList[ChrList[character].model].bumpsizebig;
    ChrList[character].bumpheightsave = CapList[ChrList[character].model].bumpheight;

    //        ChrList[character].alert = ALERTIFSPAWNED;
    ChrList[character].alert = 0;
    //        ChrList[character].aistate = 0;
    ChrList[character].aitarget = character;
    ChrList[character].aitime = 0;
    ChrList[character].grogtime = 0;
    ChrList[character].dazetime = 0;
    reaffirm_attached_particles(character, &chr_randie);


    // Let worn items come back
    item = ChrList[character].nextinpack;
    while (item != MAXCHR)
    {
      if (ChrList[item].isequipped)
      {
        ChrList[item].isequipped = bfalse;
        ChrList[item].alert |= ALERTIFATLASTWAYPOINT;  // doubles as PutAway
      }
      item = ChrList[item].nextinpack;
    }
  }
}

//--------------------------------------------------------------------------------------------
Uint16 change_armor(GAME_STATE * gs, Uint16 character, Uint16 skin, Uint32 * rand_idx)
{
  // ZZ> This function changes the armor of the character
  Uint16 enchant, sTmp;
  int iTmp, cnt;
  Uint32 chr_randie = *rand_idx;

  RANDIE(*rand_idx);

  // Remove armor enchantments
  enchant = ChrList[character].firstenchant;
  while (enchant < MAXENCHANT)
  {
    for(cnt=SETSLASHMODIFIER; cnt<=SETZAPMODIFIER; cnt++)
      unset_enchant_value(gs, enchant, cnt, &chr_randie);

    enchant = EncList[enchant].nextenchant;
  }


  // Change the skin
  sTmp = ChrList[character].model;
  if (skin > MadList[sTmp].skins)  skin = 0;
  ChrList[character].texture = MadList[sTmp].skinstart + skin;


  // Change stats associated with skin
  ChrList[character].defense = CapList[sTmp].defense[skin];
  iTmp = 0;
  while (iTmp < MAXDAMAGETYPE)
  {
    ChrList[character].damagemodifier[iTmp] = CapList[sTmp].damagemodifier[iTmp][skin];
    iTmp++;
  }
  ChrList[character].maxaccel = CapList[sTmp].maxaccel[skin];


  // Reset armor enchantments
  // These should really be done in reverse order ( Start with last enchant ), but
  // I don't care at this point !!!BAD!!!
  enchant = ChrList[character].firstenchant;
  while (enchant < MAXENCHANT)
  {
    for(cnt=SETSLASHMODIFIER; cnt<=SETZAPMODIFIER; cnt++)
      set_enchant_value(gs, enchant, cnt, EncList[enchant].eve, &chr_randie);

    add_enchant_value(enchant, ADDACCEL, EncList[enchant].eve);
    add_enchant_value(enchant, ADDDEFENSE, EncList[enchant].eve);
    enchant = EncList[enchant].nextenchant;
  }
  return skin;
}

//--------------------------------------------------------------------------------------------
void change_character(GAME_STATE * gs, Uint16 cnt, Uint16 profile, Uint8 skin,
                      Uint8 leavewhich, Uint32 * rand_idx)
{
  // ZZ> This function polymorphs a character, changing stats, dropping weapons
  int tnc, enchant;
  Uint16 sTmp, item;
  Uint32 chr_randie = *rand_idx;

  // exactly one iteration
  RANDIE(*rand_idx);


  profile = profile & (MAXMODEL - 1);
  if (MadList[profile].used)
  {
    // Drop left weapon
    sTmp = ChrList[cnt].holdingwhich[0];
    if (sTmp != MAXCHR && (CapList[profile].gripvalid[0] == bfalse || CapList[profile].ismount))
    {
      detach_character_from_mount(gs, sTmp, btrue, btrue, &chr_randie);
      if (ChrList[cnt].ismount)
      {
        ChrList[sTmp].zvel = DISMOUNTZVEL;
        ChrList[sTmp].zpos += DISMOUNTZVEL;
        ChrList[sTmp].jumptime = JUMPDELAY;
      }
    }


    // Drop right weapon
    sTmp = ChrList[cnt].holdingwhich[1];
    if (sTmp != MAXCHR && CapList[profile].gripvalid[1] == bfalse)
    {
      detach_character_from_mount(gs, sTmp, btrue, btrue, &chr_randie);
      if (ChrList[cnt].ismount)
      {
        ChrList[sTmp].zvel = DISMOUNTZVEL;
        ChrList[sTmp].zpos += DISMOUNTZVEL;
        ChrList[sTmp].jumptime = JUMPDELAY;
      }
    }


    // Remove particles
    disaffirm_attached_particles(cnt, &chr_randie);


    // Remove enchantments
    if (leavewhich == LEAVEFIRST)
    {
      // Remove all enchantments except top one
      enchant = ChrList[cnt].firstenchant;
      if (enchant != MAXENCHANT)
      {
        while (EncList[enchant].nextenchant != MAXENCHANT)
        {
          remove_enchant(gs, EncList[enchant].nextenchant, &chr_randie);
        }
      }
    }
    if (leavewhich == LEAVENONE)
    {
      // Remove all enchantments
      disenchant_character(gs, cnt, &chr_randie);
    }


    // Stuff that must be set
    ChrList[cnt].model = profile;
    ChrList[cnt].stoppedby = CapList[profile].stoppedby;
    ChrList[cnt].lifeheal = CapList[profile].lifeheal;
    ChrList[cnt].manacost = CapList[profile].manacost;
    // Ammo
    ChrList[cnt].ammomax = CapList[profile].ammomax;
    ChrList[cnt].ammo = CapList[profile].ammo;
    // Gender
    if (CapList[profile].gender != GENRANDOM) // GENRANDOM means keep old gender
    {
      ChrList[cnt].gender = CapList[profile].gender;
    }


    // AI stuff
    ChrList[cnt].aitype = MadList[profile].ai;
    ChrList[cnt].aistate = 0;
    ChrList[cnt].aitime = 0;
    ChrList[cnt].latchx = 0;
    ChrList[cnt].latchy = 0;
    ChrList[cnt].latchbutton = 0;
    ChrList[cnt].turnmode = TURNMODEVELOCITY;
    // Flags
    ChrList[cnt].stickybutt = CapList[profile].stickybutt;
    ChrList[cnt].openstuff = CapList[profile].canopenstuff;
    ChrList[cnt].transferblend = CapList[profile].transferblend;
    ChrList[cnt].enviro = CapList[profile].enviro;
    ChrList[cnt].platform = CapList[profile].platform;
    ChrList[cnt].isitem = CapList[profile].isitem;
    ChrList[cnt].invictus = CapList[profile].invictus;
    ChrList[cnt].ismount = CapList[profile].ismount;
    ChrList[cnt].cangrabmoney = CapList[profile].cangrabmoney;
    ChrList[cnt].jumptime = JUMPDELAY;
    // Character size and bumping
    ChrList[cnt].shadowsize = CapList[profile].shadowsize * ChrList[cnt].fat;
    ChrList[cnt].bumpsize = CapList[profile].bumpsize * ChrList[cnt].fat;
    ChrList[cnt].bumpsizebig = CapList[profile].bumpsizebig * ChrList[cnt].fat;
    ChrList[cnt].bumpheight = CapList[profile].bumpheight * ChrList[cnt].fat;

    ChrList[cnt].shadowsizesave = CapList[profile].shadowsize;
    ChrList[cnt].bumpsizesave = CapList[profile].bumpsize;
    ChrList[cnt].bumpsizebigsave = CapList[profile].bumpsizebig;
    ChrList[cnt].bumpheightsave = CapList[profile].bumpheight;

    ChrList[cnt].bumpdampen = CapList[profile].bumpdampen;
    ChrList[cnt].weight = CapList[profile].weight * ChrList[cnt].fat;
    if (CapList[profile].weight == 255) ChrList[cnt].weight = 65535;
    // Character scales...  Magic numbers
    if (ChrList[cnt].attachedto == MAXCHR)
    {
      ChrList[cnt].scale = ChrList[cnt].fat * MadList[profile].scale * 4;
    }
    else
    {
      ChrList[cnt].scale = ChrList[cnt].fat / (ChrList[ChrList[cnt].attachedto].fat * 1280);
      tnc = MadList[ChrList[ChrList[cnt].attachedto].model].vertices - ChrList[cnt].inwhichhand;
      ChrList[cnt].weapongrip[0] = tnc;
      ChrList[cnt].weapongrip[1] = tnc + 1;
      ChrList[cnt].weapongrip[2] = tnc + 2;
      ChrList[cnt].weapongrip[3] = tnc + 3;
    }
    item = ChrList[cnt].holdingwhich[0];
    if (item != MAXCHR)
    {
      ChrList[item].scale = ChrList[item].fat / (ChrList[cnt].fat * 1280);
      tnc = MadList[ChrList[cnt].model].vertices - GRIPLEFT;
      ChrList[item].weapongrip[0] = tnc;
      ChrList[item].weapongrip[1] = tnc + 1;
      ChrList[item].weapongrip[2] = tnc + 2;
      ChrList[item].weapongrip[3] = tnc + 3;
    }
    item = ChrList[cnt].holdingwhich[1];
    if (item != MAXCHR)
    {
      ChrList[item].scale = ChrList[item].fat / (ChrList[cnt].fat * 1280);
      tnc = MadList[ChrList[cnt].model].vertices - GRIPRIGHT;
      ChrList[item].weapongrip[0] = tnc;
      ChrList[item].weapongrip[1] = tnc + 1;
      ChrList[item].weapongrip[2] = tnc + 2;
      ChrList[item].weapongrip[3] = tnc + 3;
    }
    // Image rendering
    ChrList[cnt].uoffset = 0;
    ChrList[cnt].voffset = 0;
    ChrList[cnt].uoffvel = CapList[profile].uoffvel;
    ChrList[cnt].voffvel = CapList[profile].voffvel;
    // Movement
    ChrList[cnt].sneakspd = CapList[profile].sneakspd;
    ChrList[cnt].walkspd = CapList[profile].walkspd;
    ChrList[cnt].runspd = CapList[profile].runspd;


    // AI and action stuff
    ChrList[cnt].actionready = btrue;
    ChrList[cnt].keepaction = bfalse;
    ChrList[cnt].loopaction = bfalse;
    ChrList[cnt].action = ACTIONDA;
    ChrList[cnt].nextaction = ACTIONDA;
    ChrList[cnt].lip = 0;
    ChrList[cnt].frame = MadList[profile].framestart;
    ChrList[cnt].lastframe = ChrList[cnt].frame;
    ChrList[cnt].holdingweight = 0;


    // Set the skin
    change_armor(gs, cnt, skin, &chr_randie);


    // Reaffirm them particles...
    ChrList[cnt].reaffirmdamagetype = CapList[profile].attachedprtreaffirmdamagetype;
    reaffirm_attached_particles(cnt, &chr_randie);


    // Set up initial fade in lighting
    tnc = 0;
    while (tnc < MadList[ChrList[cnt].model].transvertices)
    {
      ChrList[cnt].vrta[tnc] = 0;
      tnc++;
    }
  }
}

//--------------------------------------------------------------------------------------------
Uint16 get_target_in_block(int x, int y, Uint16 character, char items,
                                   char friends, char enemies, char dead, char seeinvisible, IDSZ idsz,
                                   char excludeid)
{
  // ZZ> This is a good little helper, that returns != MAXCHR if a suitable target
  //     was found
  int cnt;
  Uint16 charb;
  Uint32 fanblock;
  Uint8 team;


  if (x >= 0 && x < (Mesh.sizex >> 2) && y >= 0 && y < (Mesh.sizey >> 2))
  {
    team = ChrList[character].team;
    fanblock = x + Mesh.blockstart[y];
    charb = Mesh.bumplist[fanblock].chr;
    cnt = 0;
    while (cnt < Mesh.bumplist[fanblock].chrnum)
    {
      if (dead != ChrList[charb].alive && (seeinvisible || (ChrList[charb].alpha + ChrList[charb].light > INVISIBLE)))
      {
        if ((enemies && TeamList[team].hatesteam[ChrList[charb].team] && ChrList[charb].invictus == bfalse) ||
            (items && ChrList[charb].isitem) ||
            (friends && ChrList[charb].baseteam == team))
        {
          if (charb != character && ChrList[character].attachedto != charb)
          {
            if (ChrList[charb].isitem == bfalse || items)
            {
              if (idsz != IDSZNONE)
              {
                if (CapList[ChrList[charb].model].idsz[IDSZPARENT] == idsz ||
                    CapList[ChrList[charb].model].idsz[IDSZTYPE] == idsz)
                {
                  if (!excludeid) return charb;
                }
                else
                {
                  if (excludeid)  return charb;
                }
              }
              else
              {
                return charb;
              }
            }
          }
        }
      }
      charb = ChrList[charb].bumpnext;
      cnt++;
    }
  }
  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
Uint16 get_nearby_target(Uint16 character, char items,
                                 char friends, char enemies, char dead, IDSZ idsz)
{
  // ZZ> This function finds a nearby target, or it returns MAXCHR if it can't find one
  int x, y;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;


  // Current fanblock
  x = ((int)ChrList[character].xpos) >> 9;
  y = ((int)ChrList[character].ypos) >> 9;
  return get_target_in_block(x, y, character, items, friends, enemies, dead, seeinvisible, idsz, 0);
}

//--------------------------------------------------------------------------------------------
Uint8 cost_mana(GAME_STATE * gs, Uint16 character, int amount, Uint16 killer, Uint32 * rand_idx)
{
  // ZZ> This function takes mana from a character ( or gives mana ),
  //     and returns btrue if the character had enough to pay, or bfalse
  //     otherwise
  int iTmp;
  Uint32 loc_randie = *rand_idx;

  RANDIE(*rand_idx);


  iTmp = ChrList[character].mana - amount;
  if (iTmp < 0)
  {
    ChrList[character].mana = 0;
    if (ChrList[character].canchannel)
    {
      ChrList[character].life += iTmp;
      if (ChrList[character].life <= 0)
      {
        kill_character(gs, character, character, &loc_randie);
      }
      return btrue;
    }
    return bfalse;
  }
  else
  {
    ChrList[character].mana = iTmp;
    if (iTmp > ChrList[character].manamax)
    {
      ChrList[character].mana = ChrList[character].manamax;
    }
  }
  return btrue;
}

//--------------------------------------------------------------------------------------------
Uint16 find_distant_target(Uint16 character, int maxdistance)
{
  // ZZ> This function finds a target, or it returns MAXCHR if it can't find one...
  //     maxdistance should be the square of the actual distance you want to use
  //     as the cutoff...
  int cnt, distance, xdistance, ydistance;
  Uint8 team;

  team = ChrList[character].team;
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].on)
    {
      if (ChrList[cnt].attachedto == MAXCHR && ChrList[cnt].inpack == bfalse)
      {
        if (TeamList[team].hatesteam[ChrList[cnt].team] && ChrList[cnt].alive && ChrList[cnt].invictus == bfalse)
        {
          if (ChrList[character].canseeinvisible || (ChrList[cnt].alpha + ChrList[cnt].light > INVISIBLE))
          {
            xdistance = ChrList[cnt].xpos - ChrList[character].xpos;
            ydistance = ChrList[cnt].ypos - ChrList[character].ypos;
            distance = xdistance * xdistance + ydistance * ydistance;
            if (distance < maxdistance)
            {
              return cnt;
            }
          }
        }
      }
    }
    cnt++;
  }
  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
void switch_team(int character, Uint8 team)
{
  // ZZ> This function makes a character join another team...
  if (team < MAXTEAM)
  {
    if (ChrList[character].invictus == bfalse)
    {
      TeamList[ChrList[character].baseteam].morale--;
      TeamList[team].morale++;
    }
    if ((ChrList[character].ismount == bfalse || ChrList[character].holdingwhich[0] == MAXCHR) &&
        (ChrList[character].isitem == bfalse || ChrList[character].attachedto == MAXCHR))
    {
      ChrList[character].team = team;
    }
    ChrList[character].baseteam = team;
    if (TeamList[team].leader == NOLEADER)
    {
      TeamList[team].leader = character;
    }
  }
}

//--------------------------------------------------------------------------------------------
void get_nearest_in_block(int x, int y, Uint16 character, char items,
                          char friends, char enemies, char dead, char seeinvisible, IDSZ idsz)
{
  // ZZ> This is a good little helper
  float distance, xdis, ydis;
  int cnt;
  Uint8 team;
  Uint16 charb;
  Uint32 fanblock;


  if (x >= 0 && x < (Mesh.sizex >> 2) && y >= 0 && y < (Mesh.sizey >> 2))
  {
    team = ChrList[character].team;
    fanblock = x + Mesh.blockstart[y];
    charb = Mesh.bumplist[fanblock].chr;
    cnt = 0;
    while (cnt < Mesh.bumplist[fanblock].chrnum)
    {
      if (dead != ChrList[charb].alive && (seeinvisible || (ChrList[charb].alpha + ChrList[charb].light > INVISIBLE)))
      {
        if ((enemies && TeamList[team].hatesteam[ChrList[charb].team]) ||
            (items && ChrList[charb].isitem) ||
            (friends && ChrList[charb].team == team) ||
            (friends && enemies))
        {
          if (charb != character && ChrList[character].attachedto != charb && ChrList[charb].attachedto == MAXCHR && ChrList[charb].inpack == bfalse)
          {
            if (ChrList[charb].invictus == bfalse || items)
            {
              if (idsz != IDSZNONE)
              {
                if (CapList[ChrList[charb].model].idsz[IDSZPARENT] == idsz ||
                    CapList[ChrList[charb].model].idsz[IDSZTYPE] == idsz)
                {
                  xdis = ChrList[character].xpos - ChrList[charb].xpos;
                  ydis = ChrList[character].ypos - ChrList[charb].ypos;
                  xdis = xdis * xdis;
                  ydis = ydis * ydis;
                  distance = xdis + ydis;
                  if (distance < globaldistance)
                  {
                    globalnearest = charb;
                    globaldistance = distance;
                  }
                }
              }
              else
              {
                xdis = ChrList[character].xpos - ChrList[charb].xpos;
                ydis = ChrList[character].ypos - ChrList[charb].ypos;
                xdis = xdis * xdis;
                ydis = ydis * ydis;
                distance = xdis + ydis;
                if (distance < globaldistance)
                {
                  globalnearest = charb;
                  globaldistance = distance;
                }
              }
            }
          }
        }
      }
      charb = ChrList[charb].bumpnext;
      cnt++;
    }
  }
  return;
}

//--------------------------------------------------------------------------------------------
Uint16 get_nearest_target(Uint16 character, char items,
                                  char friends, char enemies, char dead, IDSZ idsz)
{
  // ZZ> This function finds an target, or it returns MAXCHR if it can't find one
  int x, y;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;


  // Current fanblock
  x = ((int)ChrList[character].xpos) >> 9;
  y = ((int)ChrList[character].ypos) >> 9;


  globalnearest = MAXCHR;
  globaldistance = 999999;
  get_nearest_in_block(x, y, character, items, friends, enemies, dead, seeinvisible, idsz);

  get_nearest_in_block(x - 1, y, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x + 1, y, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz);

  get_nearest_in_block(x - 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x + 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x - 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz);
  get_nearest_in_block(x + 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz);
  return globalnearest;
}

//--------------------------------------------------------------------------------------------
Uint16 get_wide_target(Uint16 character, char items,
                               char friends, char enemies, char dead, IDSZ idsz, char excludeid)
{
  // ZZ> This function finds an target, or it returns MAXCHR if it can't find one
  int x, y;
  Uint16 enemy;
  char seeinvisible;
  seeinvisible = ChrList[character].canseeinvisible;

  // Current fanblock
  x = ((int)ChrList[character].xpos) >> 9;
  y = ((int)ChrList[character].ypos) >> 9;
  enemy = get_target_in_block(x, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (enemy != MAXCHR)  return enemy;

  enemy = get_target_in_block(x - 1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (enemy != MAXCHR)  return enemy;
  enemy = get_target_in_block(x + 1, y, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (enemy != MAXCHR)  return enemy;
  enemy = get_target_in_block(x, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (enemy != MAXCHR)  return enemy;
  enemy = get_target_in_block(x, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (enemy != MAXCHR)  return enemy;

  enemy = get_target_in_block(x - 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (enemy != MAXCHR)  return enemy;
  enemy = get_target_in_block(x + 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (enemy != MAXCHR)  return enemy;
  enemy = get_target_in_block(x - 1, y - 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  if (enemy != MAXCHR)  return enemy;
  enemy = get_target_in_block(x + 1, y + 1, character, items, friends, enemies, dead, seeinvisible, idsz, excludeid);
  return enemy;
}

//--------------------------------------------------------------------------------------------
void issue_clean(Uint16 character)
{
  // ZZ> This function issues a clean up order to all teammates
  Uint8 team;
  Uint16 cnt;


  team = ChrList[character].team;
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].team == team && ChrList[cnt].alive == bfalse)
    {
      ChrList[cnt].aitime = 2;  // Don't let it think too much...
      ChrList[cnt].alert = ALERTIFCLEANEDUP;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
int restock_ammo(Uint16 character, IDSZ idsz)
{
  // ZZ> This function restocks the characters ammo, if it needs ammo and if
  //     either its parent or type idsz match the given idsz.  This
  //     function returns the amount of ammo given.
  int amount, model;

  amount = 0;
  if (character < MAXCHR)
  {
    if (ChrList[character].on)
    {
      model = ChrList[character].model;
      if (CapList[model].idsz[IDSZPARENT] == idsz || CapList[model].idsz[IDSZTYPE] == idsz)
      {
        if (ChrList[character].ammo < ChrList[character].ammomax)
        {
          amount = ChrList[character].ammomax - ChrList[character].ammo;
          ChrList[character].ammo = ChrList[character].ammomax;
        }
      }
    }
  }
  return amount;
}

//--------------------------------------------------------------------------------------------
void issue_order(Uint16 character, Uint32 order)
{
  // ZZ> This function issues an order for help to all teammates
  Uint8 team;
  Uint8 counter;
  Uint16 cnt;


  team = ChrList[character].team;
  counter = 0;
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].team == team)
    {
      ChrList[cnt].order = order;
      ChrList[cnt].counter = counter;
      ChrList[cnt].alert = ChrList[cnt].alert | ALERTIFORDERED;
      counter++;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void issue_special_order(Uint32 order, IDSZ idsz)
{
  // ZZ> This function issues an order to all characters with the a matching special IDSZ
  Uint8 counter;
  Uint16 cnt;


  counter = 0;
  cnt = 0;
  while (cnt < MAXCHR)
  {
    if (ChrList[cnt].on)
    {
      if (CapList[ChrList[cnt].model].idsz[IDSZSPECIAL] == idsz)
      {
        ChrList[cnt].order = order;
        ChrList[cnt].counter = counter;
        ChrList[cnt].alert = ChrList[cnt].alert | ALERTIFORDERED;
        counter++;
      }
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void set_alerts(int character)
{
  // ZZ> This function polls some alert conditions
  if (ChrList[character].aitime != 0)
  {
    ChrList[character].aitime--;
  }
  if (ChrList[character].xpos < ChrList[character].aigotox[ChrList[character].aigoto] + WAYTHRESH &&
      ChrList[character].xpos > ChrList[character].aigotox[ChrList[character].aigoto] - WAYTHRESH &&
      ChrList[character].ypos < ChrList[character].aigotoy[ChrList[character].aigoto] + WAYTHRESH &&
      ChrList[character].ypos > ChrList[character].aigotoy[ChrList[character].aigoto] - WAYTHRESH)
  {
    ChrList[character].alert = ChrList[character].alert | ALERTIFATWAYPOINT;
    ChrList[character].aigoto++;
    if (ChrList[character].aigoto == ChrList[character].aigotoadd)
    {
      ChrList[character].aigoto = 0;
      if (CapList[ChrList[character].model].isequipment == bfalse)
      {
        ChrList[character].alert = ChrList[character].alert | ALERTIFATLASTWAYPOINT;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void free_all_enchants()
{
  // ZZ> This functions frees all of the enchantments
  numfreeenchant = 0;
  while (numfreeenchant < MAXENCHANT)
  {
    freeenchant[numfreeenchant] = numfreeenchant;
    EncList[numfreeenchant].on = bfalse;
    numfreeenchant++;
  }
}

//--------------------------------------------------------------------------------------------
void load_one_enchant_type(char* szLoadName, Uint16 profile)
{
  // ZZ> This function loads the enchantment associated with an object
  FILE* fileread;
  char cTmp;
  int iTmp, tTmp;
  float fTmp;
  int num;
  IDSZ idsz;

  globalname = szLoadName;
  EveList[profile].valid = bfalse;
  fileread = fopen(szLoadName, "r");
  if (fileread)
  {
    EveList[profile].valid = btrue;


    // btrue/bfalse values
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].retarget = bfalse;
    if (cTmp == 'T' || cTmp == 't')  EveList[profile].retarget = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].override = bfalse;
    if (cTmp == 'T' || cTmp == 't')  EveList[profile].override = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].removeoverridden = bfalse;
    if (cTmp == 'T' || cTmp == 't')  EveList[profile].removeoverridden = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].killonend = bfalse;
    if (cTmp == 'T' || cTmp == 't')  EveList[profile].killonend = btrue;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].poofonend = bfalse;
    if (cTmp == 'T' || cTmp == 't')  EveList[profile].poofonend = btrue;


    // More stuff
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  EveList[profile].time = iTmp;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  EveList[profile].endmessage = iTmp;


    // Drain stuff
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  EveList[profile].ownermana = fTmp * 256;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  EveList[profile].targetmana = fTmp * 256;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].endifcantpay = bfalse;
    if (cTmp == 'T' || cTmp == 't')  EveList[profile].endifcantpay = btrue;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  EveList[profile].ownerlife = fTmp * 256;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);  EveList[profile].targetlife = fTmp * 256;


    // Specifics
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].dontdamagetype = DAMAGENULL;
    if (cTmp == 'S' || cTmp == 's')  EveList[profile].dontdamagetype = DAMAGESLASH;
    if (cTmp == 'C' || cTmp == 'c')  EveList[profile].dontdamagetype = DAMAGECRUSH;
    if (cTmp == 'P' || cTmp == 'p')  EveList[profile].dontdamagetype = DAMAGEPOKE;
    if (cTmp == 'H' || cTmp == 'h')  EveList[profile].dontdamagetype = DAMAGEHOLY;
    if (cTmp == 'E' || cTmp == 'e')  EveList[profile].dontdamagetype = DAMAGEEVIL;
    if (cTmp == 'F' || cTmp == 'f')  EveList[profile].dontdamagetype = DAMAGEFIRE;
    if (cTmp == 'I' || cTmp == 'i')  EveList[profile].dontdamagetype = DAMAGEICE;
    if (cTmp == 'Z' || cTmp == 'z')  EveList[profile].dontdamagetype = DAMAGEZAP;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].onlydamagetype = DAMAGENULL;
    if (cTmp == 'S' || cTmp == 's')  EveList[profile].onlydamagetype = DAMAGESLASH;
    if (cTmp == 'C' || cTmp == 'c')  EveList[profile].onlydamagetype = DAMAGECRUSH;
    if (cTmp == 'P' || cTmp == 'p')  EveList[profile].onlydamagetype = DAMAGEPOKE;
    if (cTmp == 'H' || cTmp == 'h')  EveList[profile].onlydamagetype = DAMAGEHOLY;
    if (cTmp == 'E' || cTmp == 'e')  EveList[profile].onlydamagetype = DAMAGEEVIL;
    if (cTmp == 'F' || cTmp == 'f')  EveList[profile].onlydamagetype = DAMAGEFIRE;
    if (cTmp == 'I' || cTmp == 'i')  EveList[profile].onlydamagetype = DAMAGEICE;
    if (cTmp == 'Z' || cTmp == 'z')  EveList[profile].onlydamagetype = DAMAGEZAP;
    goto_colon(fileread);  EveList[profile].removedbyidsz = get_idsz(fileread);


    // Now the set values
    num = 0;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);
    EveList[profile].setvalue[num] = DAMAGESLASH;
    if (cTmp == 'C' || cTmp == 'c')  EveList[profile].setvalue[num] = DAMAGECRUSH;
    if (cTmp == 'P' || cTmp == 'p')  EveList[profile].setvalue[num] = DAMAGEPOKE;
    if (cTmp == 'H' || cTmp == 'h')  EveList[profile].setvalue[num] = DAMAGEHOLY;
    if (cTmp == 'E' || cTmp == 'e')  EveList[profile].setvalue[num] = DAMAGEEVIL;
    if (cTmp == 'F' || cTmp == 'f')  EveList[profile].setvalue[num] = DAMAGEFIRE;
    if (cTmp == 'I' || cTmp == 'i')  EveList[profile].setvalue[num] = DAMAGEICE;
    if (cTmp == 'Z' || cTmp == 'z')  EveList[profile].setvalue[num] = DAMAGEZAP;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%d", &iTmp);  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%d", &iTmp);  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%d", &iTmp);  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);  iTmp = 0;
    if (cTmp == 'T') iTmp = DAMAGEINVERT;
    if (cTmp == 'C') iTmp = DAMAGECHARGE;
    fscanf(fileread, "%d", &tTmp);  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);  iTmp = 0;
    if (cTmp == 'T') iTmp = DAMAGEINVERT;
    if (cTmp == 'C') iTmp = DAMAGECHARGE;
    fscanf(fileread, "%d", &tTmp);  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);  iTmp = 0;
    if (cTmp == 'T') iTmp = DAMAGEINVERT;
    if (cTmp == 'C') iTmp = DAMAGECHARGE;
    fscanf(fileread, "%d", &tTmp);  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);  iTmp = 0;
    if (cTmp == 'T') iTmp = DAMAGEINVERT;
    if (cTmp == 'C') iTmp = DAMAGECHARGE;
    fscanf(fileread, "%d", &tTmp);  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);  iTmp = 0;
    if (cTmp == 'T') iTmp = DAMAGEINVERT;
    if (cTmp == 'C') iTmp = DAMAGECHARGE;
    fscanf(fileread, "%d", &tTmp);  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);  iTmp = 0;
    if (cTmp == 'T') iTmp = DAMAGEINVERT;
    if (cTmp == 'C') iTmp = DAMAGECHARGE;
    fscanf(fileread, "%d", &tTmp);  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);  iTmp = 0;
    if (cTmp == 'T') iTmp = DAMAGEINVERT;
    if (cTmp == 'C') iTmp = DAMAGECHARGE;
    fscanf(fileread, "%d", &tTmp);  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);  iTmp = 0;
    if (cTmp == 'T') iTmp = DAMAGEINVERT;
    if (cTmp == 'C') iTmp = DAMAGECHARGE;
    fscanf(fileread, "%d", &tTmp);  EveList[profile].setvalue[num] = iTmp | tTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%d", &iTmp);  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%d", &iTmp);  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%d", &iTmp);  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%d", &iTmp);  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%d", &iTmp);  EveList[profile].setvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);
    EveList[profile].setvalue[num] = (cTmp == 'T' || cTmp == 't');
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);
    EveList[profile].setvalue[num] = (cTmp == 'T' || cTmp == 't');
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    cTmp = get_first_letter(fileread);
    EveList[profile].setvalue[num] = MISNORMAL;
    if (cTmp == 'R' || cTmp == 'r')  EveList[profile].setvalue[num] = MISREFLECT;
    if (cTmp == 'D' || cTmp == 'd')  EveList[profile].setvalue[num] = MISDEFLECT;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    fscanf(fileread, "%f", &fTmp);  fTmp = fTmp * 16;
    EveList[profile].setvalue[num] = fTmp;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    EveList[profile].setvalue[num] = btrue;
    num++;
    goto_colon(fileread);  cTmp = get_first_letter(fileread);
    EveList[profile].setyesno[num] = (cTmp == 'T' || cTmp == 't');
    EveList[profile].setvalue[num] = btrue;
    num++;


    // Now read in the add values
    num = 0;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 16;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 127;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 127;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 4;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 127;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
    EveList[profile].addvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
    EveList[profile].addvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
    EveList[profile].addvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);
    EveList[profile].addvalue[num] = iTmp;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%d", &iTmp);  // Defense is backwards
    EveList[profile].addvalue[num] = -iTmp;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 4;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 4;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 4;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 4;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 4;
    num++;
    goto_colon(fileread);  fscanf(fileread, "%f", &fTmp);
    EveList[profile].addvalue[num] = fTmp * 4;
    num++;

    // Clear expansions...
    EveList[profile].contspawntime = 0;
    EveList[profile].contspawnamount = 0;
    EveList[profile].contspawnfacingadd = 0;
    EveList[profile].contspawnpip = 0;
    EveList[profile].waveindex = NULL;
    EveList[profile].stayifnoowner = 0;
    EveList[profile].overlay = 0;
    // Read expansions
    while (goto_colon_yesno(fileread))
    {
      idsz = get_idsz(fileread);
      fscanf(fileread, "%c%d", &cTmp, &iTmp);

      if (MAKE_IDSZ("AMOU") == idsz)  EveList[profile].contspawnamount = iTmp;
      if (MAKE_IDSZ("TYPE") == idsz)  EveList[profile].contspawnpip = iTmp;
      if (MAKE_IDSZ("TIME") == idsz)  EveList[profile].contspawntime = iTmp;
      if (MAKE_IDSZ("FACE") == idsz)  EveList[profile].contspawnfacingadd = iTmp;
      if (MAKE_IDSZ("SEND") == idsz && iTmp > -1 && iTmp < MAXWAVE) EveList[profile].waveindex = CapList[profile].waveindex[iTmp];
      if (MAKE_IDSZ("STAY") == idsz)  EveList[profile].stayifnoowner = iTmp;
      if (MAKE_IDSZ("OVER") == idsz)  EveList[profile].overlay = iTmp;
    }


    // All done ( finally )
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
Uint16 get_free_enchant()
{
  // ZZ> This function returns the next free enchantment or MAXENCHANT if there are none
  if (numfreeenchant > 0)
  {
    numfreeenchant--;
    return freeenchant[numfreeenchant];
  }
  return MAXENCHANT;
}

//--------------------------------------------------------------------------------------------
void unset_enchant_value(GAME_STATE * gs, Uint16 enchantindex, Uint8 valueindex, Uint32 * rand_idx)
{
  // ZZ> This function unsets a set value
  Uint16 character;
  Uint32 enc_randie = *rand_idx;

  RANDIE(*rand_idx);

  if (EncList[enchantindex].setyesno[valueindex] == btrue)
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
      ChrList[character].damagemodifier[DAMAGESLASH] = EncList[enchantindex].setsave[valueindex];
      break;

    case SETCRUSHMODIFIER:
      ChrList[character].damagemodifier[DAMAGECRUSH] = EncList[enchantindex].setsave[valueindex];
      break;

    case SETPOKEMODIFIER:
      ChrList[character].damagemodifier[DAMAGEPOKE] = EncList[enchantindex].setsave[valueindex];
      break;

    case SETHOLYMODIFIER:
      ChrList[character].damagemodifier[DAMAGEHOLY] = EncList[enchantindex].setsave[valueindex];
      break;

    case SETEVILMODIFIER:
      ChrList[character].damagemodifier[DAMAGEEVIL] = EncList[enchantindex].setsave[valueindex];
      break;

    case SETFIREMODIFIER:
      ChrList[character].damagemodifier[DAMAGEFIRE] = EncList[enchantindex].setsave[valueindex];
      break;

    case SETICEMODIFIER:
      ChrList[character].damagemodifier[DAMAGEICE] = EncList[enchantindex].setsave[valueindex];
      break;

    case SETZAPMODIFIER:
      ChrList[character].damagemodifier[DAMAGEZAP] = EncList[enchantindex].setsave[valueindex];
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
      ChrList[character].missiletreatment = EncList[enchantindex].setsave[valueindex];
      break;

    case SETCOSTFOREACHMISSILE:
      ChrList[character].missilecost = EncList[enchantindex].setsave[valueindex];
      ChrList[character].missilehandler = character;
      break;

    case SETMORPH:
      // Need special handler for when this is removed
      change_character(gs, character, ChrList[character].basemodel, EncList[enchantindex].setsave[valueindex], LEAVEALL, &enc_randie);
      break;

    case SETCHANNEL:
      ChrList[character].canchannel = EncList[enchantindex].setsave[valueindex];
      break;

    }
    EncList[enchantindex].setyesno[valueindex] = bfalse;
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
    fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 16.0;
    ChrList[character].jump -= fvaluetoadd;
    break;

  case ADDBUMPDAMPEN:
    fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0;
    ChrList[character].bumpdampen -= fvaluetoadd;
    break;

  case ADDBOUNCINESS:
    fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0;
    ChrList[character].dampen -= fvaluetoadd;
    break;

  case ADDDAMAGE:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].damageboost -= valuetoadd;
    break;

  case ADDSIZE:
    fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 128.0;
    ChrList[character].sizegoto -= fvaluetoadd;
    ChrList[character].sizegototime = SIZETIME;
    break;

  case ADDACCEL:
    fvaluetoadd = EncList[enchantindex].addsave[valueindex] / 1000.0;
    ChrList[character].maxaccel -= fvaluetoadd;
    break;

  case ADDRED:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].redshift -= valuetoadd;
    break;

  case ADDGRN:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].grnshift -= valuetoadd;
    break;

  case ADDBLU:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].blushift -= valuetoadd;
    break;

  case ADDDEFENSE:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].defense -= valuetoadd;
    break;

  case ADDMANA:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].manamax -= valuetoadd;
    ChrList[character].mana -= valuetoadd;
    if (ChrList[character].mana < 0) ChrList[character].mana = 0;
    break;

  case ADDLIFE:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].lifemax -= valuetoadd;
    ChrList[character].life -= valuetoadd;
    if (ChrList[character].life < 1) ChrList[character].life = 1;
    break;

  case ADDSTRENGTH:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].strength -= valuetoadd;
    break;

  case ADDWISDOM:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].wisdom -= valuetoadd;
    break;

  case ADDINTELLIGENCE:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].intelligence -= valuetoadd;
    break;

  case ADDDEXTERITY:
    valuetoadd = EncList[enchantindex].addsave[valueindex];
    ChrList[character].dexterity -= valuetoadd;
    break;

  }
}
