#pragma once

#include "egobootypedef.h"
#include "Script.h"
#include "Mad.h"
#include "mathstuff.h"
#include "Texture.h"
#include "Physics.h"
#include "Mesh.h"

struct Cap;
struct Mad;
struct Eve;
struct Script_Info;
struct Pip;

#define MODEL_COUNT                        0x0100         // Max number of models

#define _VALID_CHR_RANGE(XX) ((XX)>=0 && (XX) < Character_List::SIZE)
#define _INVALID_CHR_RANGE(XX) ((XX)<0 || (XX) >= Character_List::SIZE)

#define VALID_CHR(XX)   (_VALID_CHR_RANGE(XX) && ChrList[XX].valid())
#define INVALID_CHR(XX) (!VALID_CHR(XX))

#define SCAN_CHR_BEGIN(XX, YY) for (XX = 0; _VALID_CHR_RANGE(XX); XX++) { if (INVALID_CHR(XX)) continue; Character & YY = ChrList[XX];
#define SCAN_CHR_END           };

#define SCAN_CHR_PACK_BEGIN(RC, XX, YY) if(RC._on && !RC.is_inpack) { for(XX = RC.nextinpack; VALID_CHR(XX) && ChrList[XX].is_inpack; XX = ChrList[XX].nextinpack) { Character & YY = ChrList[XX];
#define SCAN_CHR_PACK_END               }};


enum MISTREAT_TYPE
{
  MISNONE,
  MISNORMAL,
  MISDEFLECT,
  MISREFLECT
};

#define MAXVERTICES                     0x80         // Max number of points in a model
#define MAXSECTION                      4           // T-wi-n-k...  Most of 4 sections
#define MAXCAPNAMESIZE      0x20                      // Character class names
#define MAXLEVEL            6                       // Levels 0-5
#define MAXWAVE             16                      // Up to 16 waves per model
#define VALID_WAVE_RANGE(XX) (((XX)>=0) && ((XX)<MAXWAVE))


#define RETURNAND           0x3F                      // Return mana every so often
#define MANARETURNSHIFT     4                       //

#define GENFEMALE           0                       // Gender
#define GENMALE             1                       //
#define GENOTHER            2                       //
#define GENRANDOM           3                       //

#define LOWSTAT             (0x00<<FIXEDPOINT_BITS)               // Worst...
#define PERFECTSTAT         (0x2A<<FIXEDPOINT_BITS)               // Perfect...
#define HIGHSTAT            (0x55<<FIXEDPOINT_BITS)               // Absolute max strength...
#define PERFECTBIG          (0x7F<<FIXEDPOINT_BITS)               // Perfect life or mana...

#define THROWFIX             30.0                    // To correct thrown velocities
#define MINTHROW_VELOCITY    15.0                    //
#define MAXTHROW_VELOCITY    45.0                    //

#define MAXNUMINPACK        6                       // Max number of items to carry in pack

enum IDSZ_T
{
  IDSZ_PARENT          = 0,                       // Parent index
  IDSZ_TYPE               ,                       // Self index
  IDSZ_SKILL              ,                       // Skill index
  IDSZ_SPECIAL            ,                       // Special index
  IDSZ_HATE               ,                       // Hate index
  IDSZ_VULNERABILITY      ,                       // Vulnerability index
  IDSZ_COUNT                                      // ID strings per character
};

#define MAXXP 9999                                  // Maximum experience
#define MAXMONEY 9999                               // Maximum money

#define REEL                            7600.0      // Dampen for melee knock back
#define REELBASE                        .35         //

#define TURNSPD                         .01         // Cutoff for turning or same direction

#define PLATADD             -10                     // Height add...

#define PLATTOLERANCE       20                      // Platform tolerance...
#define CLOSETOLERANCE      2                       // For closing doors
#define JUMPTOLERANCE       20                      // Distance above ground to be jumping
#define SLIDETOLERANCE      10                      // Stick to ground better

#define JUMPNUMBER_INFINITE 0xFF                     // Flying character
#define DELAY_JUMP          20                       // Time between jumps

#define TURNMODE_VELOCITY    0                       // Character gets rotation from velocity
#define TURNMODE_WATCH       1                       // For watch towers
#define TURNMODE_SPIN        2                       // For spinning objects
#define TURNMODE_WATCHTARGET 3                       // For combat intensive AI

#define SPINRATE            200                     // How fast spinners spin
#define FLYDAMPEN           .001                    // Levelling rate for flyers

enum XP_TYPE
{
  XP_FINDSECRET        = 0,                       // Finding a secret
  XP_WINQUEST             ,                       // Beating a module or a subquest
  XP_USEDUNKOWN           ,                       // Used an unknown item
  XP_KILLENEMY            ,                       // Killed an enemy
  XP_KILLSLEEPY           ,                       // Killed a sleeping enemy
  XP_KILLHATED            ,                       // Killed a hated enemy
  XP_TEAMKILL             ,                       // Team has killed an enemy
  XP_TALKGOOD             ,                       // Talk good, er...  I mean well
  XP_COUNT                                        // Number of ways to get experience
};

#define XPDIRECT            0xFF                     // No modification

#define MAXWAY              8                       // Waypoints
#define WAYTHRESH           0x80                     // Threshold for reaching waypoint

#define CHR_COUNT                          350         // Max number of characters

#define MAXSTOR             8                       // Storage data
#define STORAND             7                       //

enum DAMAGE_TYPE
{
  DAMAGE_SLASH      = 0,                       //
  DAMAGE_CRUSH         ,                       //
  DAMAGE_POKE          ,                       //
  DAMAGE_HOLY          ,                       // (Most invert Holy damage )
  DAMAGE_EVIL          ,                       //
  DAMAGE_FIRE          ,                       //
  DAMAGE_ICE           ,                       //
  DAMAGE_ZAP           ,                       //
  DAMAGE_COUNT         ,                       // Damage types
  DAMAGE_NULL     = 0xFF
};

#define DAMAGE_CHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGE_INVERT        4                       // 00000x00 Makes damage heal
#define DAMAGE_SHIFT         3                       // 000000xx Resistance ( 1 is common )

enum SPEECH_TYPE
{
  SPEECH_MOVE       = 0,
  SPEECH_MOVEALT       ,
  SPEECH_ATTACK        ,
  SPEECH_ASSIST        ,
  SPEECH_TERRAIN       ,
  SPEECH_SELECT        ,
  SPEECH_COUNT
};

#define DELAY_PACK           25                      // Time before inventory rotate again
#define DELAY_GRAB           25                      // Time before grab again

#define DROPXY_VELOCITY      8                       //
#define DROPZ_VELOCITY       7                       //
#define JUMPATTACK_VELOCITY       -2                      //
#define WATERJUMP           12                      //



struct Character_Data
{
  Character_Data & getData() { return *this; };

  Uint8         stoppedby;                     // Collision Mask
  Uint16        lifeheal;                      //
  Sint16        manacost;                      //
  bool          nameknown;                     // Is the class name known?
  bool          show_icon;                     // show icon?
  bool          canseeinvisible;               // Can it see invisible?
  bool          canseekurse;                   // Can it see kurses?
  Uint8         ammomax;                       // Ammo stuff
  Uint8         ammo;                          //
  Uint8         gender;                        // Gender
  Uint8         lifecolor;                     // Bar colors
  Uint8         manacolor;                     //
  Sint16        lifereturn;                    //
  Uint8         attachedprtreaffirmdamagetype; // Relight that torch...
  Uint8         damagetargettype;              // For AI DamageTarget
  bool          stickybutt;                    // Stick to the ground?
  bool          canopenstuff;                  // Open chests/doors?
  bool          transferblend;                 // Transfer blending to rider/weapons
  bool          enviro;                        // Phong map this baby?
  bool          waterwalk;                     // Walk on water?
  bool          is_item;                        // Is it an item?
  bool          invictus;                      // Is it invincible?
  bool          is_mount;                       // Can you ride it?
  bool          cangrabmoney;                  // Collect money?
  float         jump;                          // Jump power
  Uint8         jumpnumberreset;               // Number of jumps ( Ninja )
  Uint8         flyheight;                     // Fly height
  Uint8         alpha;                         // Transparency
  Uint8         light;                         // Light blending
  Uint8         flashand;                      // Flashing rate
  Uint8         sheen;                         // How shiny it is ( 0-15 )
  float         scale_horiz;                   // Horizontal scale of model
  float         scale_vert;                    // Vertical scale of model
  Uint8         shadow_size;                   // Shadow size

  vec2_t        off_vel;                       // Texture movement rates
  Uint16        spd_sneak;                      // Sneak threshold
  Uint16        spd_walk;                       // Walk threshold
  Uint16        spd_run;                        // Run threshold
  Sint16        money;                          // Money
};

struct Cap : public Character_Data, public TAllocClientStrict<Cap, MODEL_COUNT>, public Physics_Properties
{
  char              filename[0x0100];
  bool              loaded;
  Sint32            slot;


  Sint8         hide_state;                    // Don't draw when...
  bool          is_equipment;                  // Behave in silly ways

  char          classname[MAXCAPNAMESIZE];     // Class name
  char          skinname[4][MAXCAPNAMESIZE];   // Skin name
  Sint8         skinoverride;                  // -1 or 0-3.. For import
  Uint8         leveloverride;                 // 0 for normal
  int           stateoverride;                 // 0 for normal
  int           contentoverride;               // 0 for normal
  Uint16        skincost[4];                   // Store prices
  Uint8         skindressy;                    // Dressy
  float         strengthdampen;                // Strength damage factor
  Uint8         uniformlit;                    // Bad lighting?

  Uint16        lifebase;                      // Life
  Uint16        liferand;                      //

  Uint16        lifeperlevelbase;              //
  Uint16        lifeperlevelrand;              //

  Uint16        manabase;                      // Mana
  Uint16        manarand;                      //

  Uint16        manaperlevelbase;              //
  Uint16        manaperlevelrand;              //

  Uint16        manareturnbase;                //
  Uint16        manareturnrand;                //

  Uint16        manareturnperlevelbase;        //
  Uint16        manareturnperlevelrand;        //

  Uint16        manaflowbase;                  //
  Uint16        manaflowrand;                  //

  Uint16        manaflowperlevelbase;          //
  Uint16        manaflowperlevelrand;          //

  Uint16        strengthbase;                  // Strength
  Uint16        strengthrand;                  //

  Uint16        strengthperlevelbase;          //
  Uint16        strengthperlevelrand;          //

  Uint16        wisdombase;                    // Wisdom
  Uint16        wisdomrand;                    //

  Uint16        wisdomperlevelbase;            //
  Uint16        wisdomperlevelrand;            //

  Uint16        intelligencebase;              // Intlligence
  Uint16        intelligencerand;              //

  Uint16        intelligenceperlevelbase;      //
  Uint16        intelligenceperlevelrand;      //

  Uint16        dexteritybase;                 // Dexterity
  Uint16        dexterityrand;                 //

  Uint16        dexterityperlevelbase;         //
  Uint16        dexterityperlevelrand;         //

  float         sizeperlevel;                  // Scale increases

  Uint16        iframefacing;                  // Invincibility frame
  Uint16        iframeangle;                   //

  Uint16        nframefacing;                  // Normal frame
  Uint16        nframeangle;                   //

  bool          resistbumpspawn;               // Don't catch fire

  Uint8         defense[4];                    // Defense for each skin
  Uint8         damagemodifier[DAMAGE_COUNT][4];
  float         maxaccel[4];                   // Acceleration for each skin

  Uint16        experienceforlevel[MAXLEVEL];  // Experience needed for next level

  Uint16        experiencebase;                // Starting experience
  Uint16        experiencerand;                //

  Uint16        experienceworth;               // Amount given to killer/user
  float         experienceexchange;            // Adds to worth
  float         experiencerate[XP_COUNT];
  IDSZ          idsz[IDSZ_COUNT];                 // ID strings

  bool          isstackable;                   // Is it arrowlike?
  bool          usageknown;                    // Is its usage known
  bool          cancarrytonextmodule;          // Take it with you?
  bool          needskillidtouse;              // Check IDSZ first?
  bool          forceshadow;                   // Draw a shadow?
  bool          ripple;                        // Spawn ripples?
  ACTION_TYPE   weaponaction;                  // Animation needed to swing
  bool          slot_valid[2];                  // Left/Right hands valid
  Uint8         attackattached;                //
  Sint8         attackprttype;                 //
  Uint8         attachedprtamount;             // Sticky particles
  Uint8         attachedprttype;               //
  Uint8         gopoofprtamount;               // Poof effect
  Sint16        gopoofprtfacingadd;            //
  Uint8         gopoofprttype;                 //
  Uint8         bloodvalid;                    // Blood ( yuck )
  Uint8         bloodprttype;                  //
  Sint8         wavefootfall;                  // Footfall sound, -1
  Sint8         wavejump;                      // Jump sound, -1
  bool          ridercanattack;                // Rider attack?
  bool          canbedazed;                    // Can it be dazed?
  bool          canbegrogged;                  // Can it be grogged?
  Uint8         kursechance;                   // Chance of being kursed
  bool          istoobig;                      // Can't be put in pack
  bool          reflect;                       // Draw the reflection
  bool          alwaysdraw;                    // Always render
  bool          isranged;                      // Flag for ranged weapon
  bool          is_platform;                   // Can be stood on?

  void reset() { memset(this, 0, sizeof(Cap)); wavejump = -1; };
};

struct Cap_List : public TAllocListStrict<Cap, MODEL_COUNT>
{
  void   free_import_slots();
  Uint32 load_one_cap(const char *szLoadName);

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

  void set_import(Uint32 object, Uint32 slot)
  {
    m_importobject     = object;
    m_slotlist[object] = slot;
  };

  Cap_List() { _setup(); }

  void reset() { _setup(); my_alist_type::_setup(); }

  int m_importobject;
  int m_slotlist[MODEL_COUNT];

protected:
  void _setup()
  {
    free_import_slots();
  };
};

extern Cap_List CapList;

typedef Cap_List::index_t    CAP_REF;


//--------------------------------------------------------------------------------------------

struct AI_State
{
  Uint32          alert;           // Alerts for AI script
  Uint16          target;        // Who the AI is after
  Uint16          owner;         // The character's owner
  Uint16          child;         // The character's child
  Uint16          time;          // AI Timer

  int             state;         // Short term memory for AI
  int             content;       // More short term memory
  int             x[MAXSTOR];    // Temporary values...  SetXY
  int             y[MAXSTOR];    //

  Uint8           goto_cnt;          // Which waypoint
  Uint8           goto_idx;       // Where to stick next
  float           goto_x[MAXWAY]; // Waypoint
  float           goto_y[MAXWAY]; // Waypoint

  Latch           latch;          // Character latches

  Uint8           turn_mode;        // Turning mode

  Uint16          bumplast;        // Last character it was bumped   by
  Uint16          attacklast;      // Last character it was attacked by
  Uint16          hitlast;         // Last character it hit
  Uint16          directionlast;   // Direction of last attack/healing
  Uint8           damagetypelast;  // Last damage type

  bool            gopoof;
};

//--------------------------------------------------------------------------------------------

struct Action
{
  bool            ready;     // Ready to play a new one
  bool            keep;      // Keep the action playing
  bool            loop;      // Loop it too

  ACTION_TYPE     which;     // Character's action
  ACTION_TYPE     next;      // Character's action to play next
};

//--------------------------------------------------------------------------------------------

struct Animation
{
  Uint16          frame;           // Character's frame
  Uint16          last;       // Character's last frame
  Uint8           lip;             // Character's frame in betweening
  float           flip;
  float           rate;

  Animation() { lip = 0; flip = 0.0f; rate = 1.0; };
};

bool play_action(struct Character & chr, ACTION_TYPE action, bool actready);
bool play_action(Uint16 character, ACTION_TYPE action, bool actready);


//--------------------------------------------------------------------------------------------
#define SLOT_COUNT 2
#define SLOT_LEFT  0
#define SLOT_RIGHT 1

struct Pack
{
  bool            is_inpack;          // Is it in the inventory?
  bool            wasinpack;       // Temporary thing...
  Uint16          nextinpack;      // Link to the next item
  Uint8           numinpack;       // How many

  Uint32 stack_item(Uint32 item);
  bool   add_item(Uint32 item);
  Uint32 get_item(bool ignorekurse);
  Uint32 get_idsz(IDSZ minval, IDSZ maxval);
};

struct Platform
{
  // client info
  bool            canuse_platforms;               // Can use platforms?
  Uint32          on_which_platform;   // for items sitting on platforms
  float           platform_level;

  //server info
  bool            is_platform;
  Sint32          holding_weight;       // For weighted buttons

  void request_attachment(Uint32 platform);
  void request_detachment();  

  static void do_attachment(struct Character & rchr);

  Platform();

protected:
  Uint32          request_attach;      // for deferred attachment
  bool            request_detach;      // for deferred detachment

};

struct Character : 
  public Character_Data, 
  public TAllocClient<Character, CHR_COUNT> , 
  public Pack, public Platform, public Bump_List_Client, public Physics_Accumulator
{
  friend struct Character_List;

  bool hitawall(Mesh & msh, vec3_t & normal);
  bool hitmesh(Mesh & msh, vec3_t & normal);
  bool inawall(Mesh & msh);

  void update_old(Mesh & msh)
  {
    // use "old" like breadcrumbs back to a good position
    if( pos.z < level  ) return;
    if( inawall(GMesh) ) return;
    old = getOrientation();
  }

  bool calculate_bumpers();

  void calc_levels();

  bool valid() { return allocated() && _on; };

  typedef my_aclient_type my_base;
  my_base & getBase() { return *(static_cast<my_base*>(this)); };

  void initialize();
  void deconstruct();
  void construct();

  BlendedVertexData md2_blended;

  bool            _on;              // Does it exist?
  bool            _onold;           // Network fix
  char            name[MAXCAPNAMESIZE];  // Character name

  Uint32          model;           // Character's model
  Uint32          basemodel;       // The true form

  TEX_REF         texture;         // Character's skin texture
  Uint16          skin;            // Character's skin index
  Uint8           team;            // Character's team
  Uint8           baseteam;        // Character's starting team

  GLMatrix        matrix;          // Character's matrix
  bool            matrix_valid;    // Did we make one yet?

  bool            alive;           // Is it alive?
  bool            waskilled;       // Fix for network

  Uint8           sparkle;         // Sparkle color or 0 for off
  Sint16          life;            // Basic character stats
  Sint16          lifemax;         //   All 8.8 fixed point
  Sint16          mana;            // Mana stuff
  Sint16          manamax;         //
  Sint16          manaflow;        //
  Sint16          manareturn;      //
  Sint16          strength;        // Strength
  Sint16          wisdom;          // Wisdom
  Sint16          intelligence;    // Intelligence
  Sint16          dexterity;       // Dexterity

  bool            isplayer;        // true = player
  bool            islocalplayer;   // true = local player

  AI_REF          scr_ref;          // The AI script to run
  AI_State        ai;

  Uint32          order;           // The last order given the character
  Uint8           counter;         // The rank of the character on the order chain

  Uint8           inwater;         //


  Uint8           staton;          // Display stats?
  vec3_t          stt;            // Starting position
  float           level;            // Height of tile


  Uint8           reloadtime;      // Time before another shot
  float           maxaccel;        // Maximum acceleration
  float           scale;           // Character's size (useful)
  float           scale_goto;        // Character's size goto ( legible )
  Uint8           sizegototime;    // Time left in siez change
  Uint8           jumptime;        // Delay until next jump
  Uint8           jumpnumber;      // Number of jumps remaining
  Uint8           jumpready;       // For standing on a platform character
  Uint32          onwhichfan;      // Where the char is
  Uint8           indolist;        // Has it been added yet?
  vec2_t          off;           // For moving textures

  Sint16          light_x;
  Sint16          light_y;
  Uint16          light_a;

  Action          act;
  Animation       ani;

  Uint16          holding_which[SLOT_COUNT]; // !=Character_List::INVALID if character is holding something
  Uint16          held_by;                   // !=Character_List::INVALID if character is a held weapon

  Uint8           light_level;     // 0-0xFF, terrain light
  Uint8           redshift;        // Color channel shifting
  Uint8           grnshift;        //
  Uint8           blushift;        //

  Uint8           shadow_size_save;  // Without size modifiers
  Uint8           bump_size_save;    //
  Uint8           bump_size_big_save; //
  Uint8           bump_height_save;  //

  Uint8           damagemodifier[DAMAGE_COUNT];  // Resistances and inversion
  Uint8           damagetime;      // Invincibility timer
  Uint8           defense;         // Base defense rating
  Uint8           passage;         // The passage associated with this character

  Uint16          experience;      // Experience
  Uint8           experiencelevel; // Experience Level
  Sint16          grogtime;        // Grog timer
  Sint16          dazetime;        // Daze timer
  Uint8           iskursed;        // Can't be dropped?
  Uint8           ammoknown;       // Is the ammo known?
  Uint8           hitready;        // Was it just dropped?
  Sint16          boretime;        // Boredom timer
  Uint8           carefultime;     // "You hurt me!" timer
  bool            canbecrushed;    // Crush in a door?
  Uint8           gripoffset;     // GRIP_LEFT or GRIP_RIGHT
  Uint8           is_equipped;      // For boots and rings and stuff

  Uint8           firstenchant;    // Linked list for enchants
  Uint8           undoenchant;     // Last enchantment spawned

  bool            canchannel;      //
  bool            overlay;         // Is this an overlay?  Track ai.target...

  MISTREAT_TYPE     missiletreatment;// For deflection, etc.
  Uint8           missilecost;     // Mana cost for each one
  Uint16          missilehandler;  // Who pays the bill for each one...

  Uint16          damageboost;     // Add to swipe damage
  Sint8           wavespeech[SPEECH_COUNT];    // For RTS speech

  const JF::MD2_Frame * getFrameLast(JF::MD2_Model * m = NULL);
  const JF::MD2_Frame * getFrame(JF::MD2_Model * m = NULL);
  JF::MD2_Model * getMD2();

  Cap & getCap();
  Mad & getMad();
  Eve & getEve();
  Script_Info & getAI();
  Pip & getPip(int i);

private:
  void reset();
};


//--------------------------------------------------------------------------------------------

struct Character_List : public TAllocList<Character, CHR_COUNT>
{
  void free_all();

  void setup_characters(char *modname);

  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;
    _list[i]._on       = false;
    _list[i].alive     = false;
    _list[i].is_inpack = false;
    my_alist_type::return_one(i);
  }

  Uint32 get_free(Uint32 force = INVALID)
  {
    Uint32 ref = my_alist_type::get_free(force);

    if(INVALID!=ref)
    {
      _list[ref].initialize(); //initialize the character data
    };

    return ref;
  };
};

extern Character_List ChrList;

//--------------------------------------------------------------------------------------------

#define NOLEADER 0xFFFF                              // If the team has no leader...

enum TERM_TYPE
{
  TEAM_EVIL            = 'E'-'A',              // E
  TEAM_GOOD            = 'G'-'A',              // G
  TEAM_NEUTRAL         = 'N'-'A',              // N
  TEAM_ZIPPY           = 'Z'-'A',
  TEAM_DAMAGE                   ,              // For damage tiles
  TEAM_COUNT                                   // Teams A-Z, +1 more for damage tiles
};

struct Team
{
  bool   hatesteam[TEAM_COUNT];     // Don't damage allies...
  Uint16 morale;                 // Number of characters on team
  Uint16 leader;                 // The leader of the team
  Uint16 sissy;                  // Whoever called for help last
};

struct Team_List : public TList<Team, TEAM_COUNT>
{
  void setup_alliances(char *modname);
};

extern Team_List TeamList;

Uint32 spawn_one_character(vec3_t & pos, Uint16 turn_lr, vec3_t & vel, float vel_lr, 
                           int profile, Uint8 team, Uint8 skin, char *name = NULL, Uint32 override = Character_List::INVALID);

void respawn_character(Uint16 character);
void propagate_forces(Character & item, struct Physics_Accumulator & accum);

bool make_one_character_matrix(Character & rchr);