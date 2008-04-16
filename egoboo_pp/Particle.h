#pragma once

#include "egobootypedef.h"
#include "SDL_opengl.h"
#include "Texture.h"
#include "Mesh.h"
#include "Physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MODEL_COUNT                        0x0100         // Max number of models
#define PRT_COUNT                          0x0200         // Max number of particles
#define DYNA_COUNT                         8           // Number of dynamic lights
#define MAXPARTICLEIMAGE                0x0100         // Number of particle images ( frames )

// Particle template
#define MAXFALLOFF                1400
#define MAXDYNADIST   (MAXFALLOFF*1.5)        // Leeway for offscreen lights
#define DYNAOFF                     0
#define DYNAON                      1
#define DYNALOCAL                   2
#define DYNAFANS     (MAXDYNADIST/0x80)

#define _VALID_PRT_RANGE(X)     ((X)>=0 && (X) < Particle_List::SIZE)
#define _INVALID_PRT_RANGE(X)   ((X)<0 || (X) >= Particle_List::SIZE)

#define VALID_PRT(XX)   (_VALID_PRT_RANGE(XX)   && PrtList[XX]._on)
#define INVALID_PRT(XX) (!VALID_PRT(XX))


#define SCAN_PRT_BEGIN(XX, YY) for (XX = 0; XX<PRT_COUNT; XX++) { if (INVALID_PRT(XX)) continue; Particle & YY = PrtList[XX];
#define SCAN_PRT_END           };


//--------------------------------------------------------------------------------------------
struct Particle_List;
struct Cap;
struct Mad;
struct Eve;
struct Script_Info;
struct Pip;
struct vec3_t;

//--------------------------------------------------------------------------------------------
struct Dyna_Info
{
  int   distance;     // The priority key for the list

  vec3_t pos;        // Light position
  float  level;      // Light level
  float  falloff;    // Light falloff
};

//--------------------------------------------------------------------------------------------
struct Dyna_List : public TList<Dyna_Info, DYNA_COUNT>
{
  int distancetobeat; // The number to beat
  int count;          // Number of dynamic lights

  void make(Particle_List & plist);
};


struct Particle_Data
{
  Particle_Data & getData() { return *this; };

  Uint8          damagetype;                   // Damage type
  Uint16         damagebase;                   // Damage
  Uint16         damagerand;                   // Damage

  float          dyna_level;                    // Intensity
  Uint16         dyna_falloff;                  // Falloff
  Uint8          dyna_lightmode;                // Dynamic light on?
  float          dyna_leveladd;                 // Dyna light changes
  float          dyna_falloffadd;               //

  Uint8          type;                         // Transparency mode
  Sint16         rotate_add;                    // Rotation rate
  Sint16         size_add;                      // Size rate
  Sint32         time;                         // Time until end

};

//--------------------------------------------------------------------------------------------
struct Pip : public Particle_Data, public TAllocClientStrict<Pip, MODEL_COUNT>, public Physics_Properties
{
  friend struct Pip_List;

  char              filename[0x0100];
  bool              loaded;

  char              comment[0x0100];

  bool           force;                        // Force spawn?
  Uint8          numframes;                    // Number of frames
  Uint8          image_base;                    // Starting image
  Uint16         image_add;                     // Frame rate
  Uint16         image_add_rand;                 // Frame rate randomness
  Uint16         rotate_base;                   // Rotation
  Uint16         rotate_rand;                   // Rotation
  Uint16         size_base;                     // Size
  float          spdlimit;                     // Speed limit
  float          dampen;                       // Bounciness
  Sint8          bumpmoney;                    // Value of particle
  bool           endwater;                     // End if underwater
  bool           endbump;                      // End if bumped
  bool           endground;                    // End if on ground
  bool           endwall;                      // End if hit a wall
  bool           endlastframe;                 // End on last frame
  Sint16         facingbase;                   // Facing
  Uint16         turn_add;                    // Facing
  Uint16         facingrand;                   // Facing
  Sint16         xyspacingbase;                // Spacing
  Uint16         xyspacingrand;                // Spacing
  Sint16         zspacingbase;                 // Altitude
  Uint16         zspacingrand;                 // Altitude
  Sint8          xvel_ybase;                    // Shot velocity
  Uint8          xvel_yrand;                    // Shot velocity
  Sint8          vel_zbase;                     // Up velocity
  Uint8          vel_zrand;                     // Up velocity
  Uint16         contspawntime;                // Spawn timer
  Uint8          contspawnamount;              // Spawn amount
  Uint16         contspawnfacingadd;           // Spawn in circle
  Uint16         contspawnpip;                 // Spawn type ( local )
  Uint8          endspawnamount;               // Spawn amount
  Uint16         endspawnfacingadd;            // Spawn in circle
  Uint8          endspawnpip;                  // Spawn type ( local )
  Uint8          bumpspawnamount;              // Spawn amount
  Uint8          bumpspawnpip;                 // Spawn type ( global )
  Uint16         dazetime;                     // Daze
  Uint16         grogtime;                     // Drunkeness
  Sint8          soundspawn;                   // Beginning sound
  Sint8          soundend;                     // Ending sound
  Sint8          soundfloor;                   // Floor sound
  Sint8          soundwall;                    // Ricochet sound
  bool           friendlyfire;                 // Friendly fire
  bool           rotatetoface;                 // Arrows/Missiles
  bool           newtargetonspawn;             // Get new target?
  bool           homing;                       // Homing?
  Uint16         targetangle;                  // To find target
  float          homingaccel;                  // Acceleration rate
  float          homingfriction;               // Deceleration rate
  bool           targetcaster;                 // Target caster?
  bool           spawnenchant;                 // Spawn enchant?
  bool           needtarget;                   // Need a target?
  bool           onlydamagefriendly;           // Only friends?
  bool           startontarget;                // Start on target?
  int            zaimspd;                      // [ZSPD] For Z aiming
  Uint16         damfx;                        // Damage effects
  bool           allowpush;                    //
  Uint16         bump_damage;
  Uint16         bump_mana;

  void reset() { memset(this,0,sizeof(Pip)); time = 1; };

private:
  static bool load(const char *szLoadName, Pip & rpip);
};



//--------------------------------------------------------------------------------------------
struct Pip_List : public TAllocListStrict<Pip, MODEL_COUNT>
{
  void reset();

  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;
    _list[i].loaded = false;
    _list[i].filename[0] = 0;
    my_alist_type::return_one(i);
  }

  Uint32 load_one_pip(const char *szLoadName, Uint32 force = INVALID);

  void reset_particles(const char* modname);
};

typedef Pip_List::index_t    PIP_REF;

//--------------------------------------------------------------------------------------------
struct Particle : 
  public Particle_Data, public TAllocClient<Particle, PRT_COUNT>, 
  public Bump_List_Client, public Physics_Accumulator
{

  typedef my_aclient_type my_base;
  my_base & getBase() { return *(static_cast<my_base*>(this)); };

  bool hitawall(Mesh & msh, vec3_t & normal);
  bool hitmesh(Mesh & msh, vec3_t & normal);
  bool inawall(Mesh & msh);
  void calc_levels();

  bool calculate_bumpers()
  {
    // bumpers
    calc_bump_size_x   = bump_size;
    calc_bump_size_y   = bump_size;
    calc_bump_size     = bump_size;
    calc_bump_size_xy  = bump_size_big;
    calc_bump_size_yx  = bump_size_big; 
    calc_bump_size_big = bump_size_big;
    calc_bump_height   = bump_height;
  };

  void update_old(Mesh & msh)
  {
    // use "old" like breadcrumbs back to a good position
    if( pos.z < level ) return;
    if( inawall(GMesh) ) return;
    old = getOrientation();
  }

  void initialize() { deconstruct(); construct(); };

  void deconstruct();
  void construct();

  void requestDestroy() 
  { 
    destroy_me = true; image = -1; 
  };

  bool            destroy_me;
  bool           _on;                              // Does it exist?
  PIP_REF         pip;                             // The part template
  Uint16          model;                           // Pip spawn model

  bool            inwater;
  Uint16          attachedtocharacter;             // For torch flame
  Uint16          grip;                            // The vertex it's on
  Uint8           team;                            // Team

  vec3_t          stt;
  float           level;                           // Height of tile
  Uint8           spawncharacterstate;             //
  Uint16          rotate;                          // Rotation direction
  Uint32          onwhichfan;                      // Where the part is
  Sint32          size;                            // Size of particle>>8
  Uint8           inview;                          // Render this one?
  Uint16          image;                           // Which image ( >> 8 )
  Uint16          image_add;                        // Animation rate
  Uint16          image_max;                        // End of image loop
  Uint16          image_stt;                        // Start of image loop
  Uint8           light;                           // Light level
  Uint16          spawntime;                       // Time until spawn
  Uint16          chr;                             // The character that is attacking
  Uint8           dyna_lighton;                     // Dynamic light?
  Uint16          target;                          // Who it's chasing


  Cap & getCap();
  Mad & getMad();
  Eve & getEve();
  Script_Info & getAI();
  Pip & getPip(int i);
  Pip & getPip();

  void reset() { memset(this,0,sizeof(Particle)); };
};

//--------------------------------------------------------------------------------------------
struct Particle_List : public TAllocList<Particle, PRT_COUNT>
{
  TEX_REF         texture;                            // All in one bitmap
  vec2_t          txcoord[MAXPARTICLEIMAGE][2];       // Texture coordinates

  void free_one(int particle);
  void free_one_no_sound(int particle);
  void free_all();

  Uint32 get_free(bool force = false);

  void setup_particles();

  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;

    _list[i]._on     = false;
    my_alist_type::return_one(i);
  }
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Dyna_List DynaList;
extern Particle_List PrtList;

extern Pip_List PipList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Uint16 spawn_one_particle(vec3_t & pos, Uint16 facing, vec3_t & vel, Sint16 mass,
                          Uint16 model, Uint16 pip, Uint16 characterattach, Uint16 grip, Uint8 team,
                          Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget);
