#pragma once

#include "egobootypedef.h"

//--------------------------------------------------------------------------------------------

#define EVE_SETVALUE_COUNT                  24          // Number of sets
#define EVE_ADDVALUE_COUNT                  16          // Number of adds
#define ENCHANT_COUNT                      0x80         // Number of enchantments
#define EVE_COUNT                          0x0100       // One enchant type per model


#define _VALID_ENC_RANGE(XX)     ((XX)>=0 && (XX) < Enchant_List::SIZE)

#define VALID_ENC(XX)   (_VALID_ENC_RANGE(XX) && EncList[XX].allocated() && EncList[XX]._on)
#define INVALID_ENC(XX) (!VALID_ENC(XX))

#define SCAN_ENCLIST_BEGIN(XX, YY) for (XX = 0; _VALID_ENC_RANGE(XX); XX++) { if (!VALID_ENC(XX)) continue; Enchant & YY = EncList[XX];
#define SCAN_ENCLIST_END           };


#define SCAN_ENC_BEGIN(XX, YY, ZZ) for (YY = XX.firstenchant; VALID_ENC(YY); YY = EncList[YY].nextenchant) { Enchant & ZZ = EncList[YY];
#define SCAN_ENC_END               };

#define LEAVEALL                0
#define LEAVEFIRST              1
#define LEAVENONE               2

#define SETDAMAGETYPE           0
#define SETNUMBEROFJUMPS        1
#define SETLIFEBARCOLOR         2
#define SETMANABARCOLOR         3
#define SETSLASHMODIFIER        4
#define SETCRUSHMODIFIER        5
#define SETPOKEMODIFIER         6
#define SETHOLYMODIFIER         7
#define SETEVILMODIFIER         8
#define SETFIREMODIFIER         9
#define SETICEMODIFIER          10
#define SETZAPMODIFIER          11
#define SETFLASHINGAND          12
#define SETLIGHTBLEND           13
#define SETALPHABLEND           14
#define SETSHEEN                15
#define SETFLYTOHEIGHT          16
#define SETWALKONWATER          17
#define SETCANSEEINVISIBLE      18
#define SETMISSILETREATMENT     19
#define SETCOSTFOREACHMISSILE   20
#define SETMORPH                21
#define SETCHANNEL              22

#define ADDJUMPPOWER            0
#define ADDBUMPDAMPEN           1
#define ADDBOUNCINESS           2
#define ADDDAMAGE               3
#define ADDSIZE                 4
#define ADDACCEL                5
#define ADDRED                  6
#define ADDGRN                  7
#define ADDBLU                  8
#define ADDDEFENSE              9
#define ADDMANA                 10
#define ADDLIFE                 11
#define ADDSTRENGTH             12
#define ADDWISDOM               13
#define ADDINTELLIGENCE         14
#define ADDDEXTERITY            15

//--------------------------------------------------------------------------------------------

struct Cap;
struct Mad;
struct Eve;
struct Script_Info;
struct Pip;


//--------------------------------------------------------------------------------------------
struct Eve : public TAllocClientStrict<Eve, EVE_COUNT>
{
  friend struct Eve_List;

  char              filename[0x0100];
  bool              loaded;

  bool              override;                    // Override other enchants?
  bool              removeoverridden;            // Remove other enchants?
  bool              setyesno[EVE_SETVALUE_COUNT];    // Set this value?
  Sint32            setvalue[EVE_SETVALUE_COUNT];    // Value to use
  Sint32            addvalue[EVE_ADDVALUE_COUNT];    // The values to add
  bool              retarget;                    // Pick a weapon?
  bool              killonend;                   // Kill the target on end?
  bool              poofonend;                   // Spawn a poof on end?
  bool              endifcantpay;                // End on out of mana
  bool              stayifnoowner;               // Stay if owner has died?
  Sint16            time;                        // Time in seconds
  Sint8             endmessage;                  // Message for end -1 for none
  Sint16            ownermana;                   // Boost values
  Sint16            ownerlife;                   //
  Sint16            targetmana;                  //
  Sint16            targetlife;                  //
  Uint8             dontdamagetype;              // Don't work if ...
  Uint8             onlydamagetype;              // Only work if ...
  IDSZ              removedbyidsz;               // By particle or [NONE]
  Uint16            contspawntime;               // Spawn timer
  Uint8             contspawnamount;             // Spawn amount
  Uint16            contspawnfacingadd;          // Spawn in circle
  Uint16            contspawnpip;                // Spawn type ( local )
  Mix_Chunk        *waveindex;                   // Sound on end
  Uint16            frequency;                   // Sound frequency
  Uint8             overlay;                     // Spawn an overlay?

  void reset() { memset(this, 0, sizeof(Eve)); }

private:
  bool load(const char* szLoadName, Uint32 cap_ref, Eve & reve);
};

//--------------------------------------------------------------------------------------------
struct Eve_List : public TAllocListStrict<Eve, EVE_COUNT>
{

  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;
    _list[i].loaded = false;
    _list[i].filename[0] = 0;
    my_alist_type::return_one(i);
  }

  Uint32 get_free(Uint32 force = INVALID)
  {
    Uint32 ref = my_alist_type::get_free(force);
    if(INVALID == ref) return INVALID;
    _list[ref].reset();
    return ref;
  };

  Uint32 load_one_eve(const char * pathname, Uint32 cap_ref);
};

//--------------------------------------------------------------------------------------------

struct Enchant : public TAllocClient<Enchant, ENCHANT_COUNT>
{
  Uint8           _on;                      // Enchantment on
  Uint32          eve_prof;                // The profile ref with an eve
  Uint16          target;                  // Who it enchants
  Uint16          nextenchant;             // Next in the list
  Uint16          owner;                   // Who cast the enchant
  Uint16          spawner;                 // The spellbook character
  Uint16          overlay;                 // The overlay character
  Sint16          ownermana;               // Boost values
  Sint16          ownerlife;               //
  Sint16          targetmana;              //
  Sint16          targetlife;              //
  bool            setyesno[EVE_SETVALUE_COUNT];// Was it set?
  Sint32          setsave[EVE_SETVALUE_COUNT]; // The value to restore
  Sint16          addsave[EVE_ADDVALUE_COUNT]; // The value to take away
  Sint16          time;                    // Time before end
  Uint16          spawntime;               // Time before spawn

  Cap & getCap();
  Mad & getMad();
  Eve & getEve();
  Script_Info & getAI();
  Pip & getPip(int i);

  void reset() { memset(this,0,sizeof(Enchant)); }

};

//--------------------------------------------------------------------------------------------

struct Enchant_List : public TAllocList<Enchant, ENCHANT_COUNT>
{
  void release_all_enchants();

  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;
    _list[i]._on     = false;
    my_alist_type::return_one(i);
  }

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Enchant_List EncList;

extern Eve_List EveList;
typedef Eve_List::index_t EVE_REF;


//--------------------------------------------------------------------------------------------
void getadd(int min, int value, int max, int & valuetoadd);
void fgetadd(float min, float value, float max, float & valuetoadd);

void set_enchant_value(Uint16 enchantindex, Uint8 valueindex, Uint32 profile);
void unset_enchant_value(Uint16 enchantindex, Uint8 valueindex);

void add_enchant_value(Uint16 enchantindex, Uint8 valueindex, EVE_REF & eve_ref);
void remove_enchant_value(Uint16 enchantindex, Uint8 valueindex);

void remove_enchant(Uint16 enchantindex);
Uint16 spawn_one_enchant(Uint16 owner, Uint16 target, Uint16 spawner, Uint16 modeloptional);

Uint16 enchant_value_filled(Uint16 enchantindex, Uint8 valueindex);
void do_enchant_spawn();
void disenchant_character(Uint16 cnt);
Uint16 change_armor(Uint16 character, Uint16 skin);
void change_character(Uint16 character, Uint16 profile, Uint8 skin, Uint8 leavewhich);
bool cost_mana(Uint16 character, int amount, Uint16 killer);
void switch_team(int character, Uint8 team);