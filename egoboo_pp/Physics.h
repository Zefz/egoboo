#pragma once

#include "egobootypedef.h"
#include "mathstuff.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define PLATASCEND          .10                     // Ascension rate
#define PLATKEEP            .90                     // Retention rate

//--------------------------------------------------------------------------------------------

struct Physics_Environment
{
  bool    mounted;
  bool    grounded;
  bool    platformed;

  float   lerp;
  float   level;

  vec3_t  normal;

  float   fric_surf;
  float   fric_fluid;
};

struct Physics_Info
{
  float    fric_hill;                   // on slope
  float    fric_slip;                   // icy
  float    fric_air;                    // flying
  float    fric_h2o;                    // water
  float    fric_stick;                  // static
  float    fric_plat;                   // platform
  float    gravity;                     // Gravitational accel

  Physics_Info()
  {
    fric_hill  =  1.00;
    fric_slip  =  1.00;
    fric_air   =  0.95;
    fric_h2o   =  0.85;
    fric_stick =  0.95;
    fric_plat  =  0.040;
    gravity    = -1.0;
  };
};

extern Physics_Info GPhys;


//--------------------------------------------------------------------------------------------
struct Orientation
{
  Orientation & getOrientation() { return *this; };

  vec3_t          pos;             // Object's position
  Uint16          turn_lr;         // Object's angular position (around z)
  vec3_t          up;

  vec3_t          vel;             // Object's velocity
  float           vel_lr;          // Object's angular velocity (around z)

  vec3_t          acc;             // Object's acceleration
  float           acc_lr;          //
};

//--------------------------------------------------------------------------------------------
struct Physics_Properties
{
  Physics_Properties & getProperties() { return *this; }

  Uint32        bump_size;                      // Bounding octagon
  Uint32        bump_size_big;                  // For octagonal bumpers
  Uint32        bump_height;                    //
  float         bump_dampen;                    // Mass
  Sint32        weight;                         // Weight
  float         dampen;                         // Bounciness

  Physics_Properties() { memset(this, 0, sizeof(Physics_Properties)); }
};

struct Physics_Accumulator : public Orientation, public Physics_Properties
{
  Orientation old;

  vec3_t apos, avel, aacc;
  float apos_lr, avel_lr, aacc_lr;
  bool  integration_allowed;

  bool  calc_is_platform;
  bool  calc_is_mount;     
  float calc_bump_size_x;
  float calc_bump_size_y;
  float calc_bump_size;
  float calc_bump_size_xy;
  float calc_bump_size_yx; 
  float calc_bump_size_big;
  float calc_bump_height;


  Physics_Accumulator & getAccumulator() { return *this; };

  Physics_Accumulator() { memset(this, 0, sizeof(Physics_Accumulator)); }

  bool hitawall(struct Mesh & msh, Uint32 stoppedby, vec3_t & normal);
  bool hitmesh(Mesh & msh, float level, vec3_t & normal);
  bool inawall(Mesh & msh, Uint32 stoppedby);

  void accumulate_acc(vec3_t & a, float alr = 0)
  {
    if(!integration_allowed) return;
    aacc += a;
    aacc_lr += alr;
    assert( dist_abs_horiz(a) < 5000);
  }

  void accumulate_acc_z(float a)
  {
    if(!integration_allowed) return;
    aacc.z += a;
    assert( a < 5000);
  }

  // express a requested change in velocity as a constant acceleration
  void accumulate_vel(vec3_t & v, float vlr = 0)
  {
    if(!integration_allowed) return;
    avel    += v;
    avel_lr += vlr;
    assert( dist_abs_horiz(v) < 5000);
  };

  void accumulate_vel_x(float v)
  {
    if(!integration_allowed) return;
    avel.x    += v;
    assert( v < 5000);
  };

  void accumulate_vel_y(float v)
  {
    if(!integration_allowed) return;
    avel.y    += v;
    assert( v < 5000);
  };

  void accumulate_vel_z(float v)
  {
    if(!integration_allowed) return;
    avel.z    += v;
  };

  // express a requested change in position as a constant acceleration
  void accumulate_pos(vec3_t & d, float dlr = 0)
  {
    if(!integration_allowed) return;

    apos    += d;
    apos_lr += dlr;

     assert( dist_abs_horiz(d) < 5000);
  };

  void accumulate_pos_x(float d)
  {
    if(!integration_allowed) return;
    apos.x  += d;
  };
  void accumulate_pos_y(float d)
  {
    if(!integration_allowed) return;
    apos.y  += d;
  };
  void accumulate_pos_z(float d)
  {
    if(!integration_allowed) return;
    apos.z  += d;
  };
  void accumulate_pos_lr(float dlr = 0)
  {
    if(!integration_allowed) return;
    apos_lr  += dlr;
  };

  void end_integration(float dframe)
  {
    if(!integration_allowed) return;

    aacc.x = aacc.x *(500/(500+fabs(aacc.x)));
    aacc.y = aacc.y *(500/(500+fabs(aacc.y)));
    aacc.z = aacc.z *(500/(500+fabs(aacc.z)));

    avel.x = avel.x *(500/(500+fabs(avel.x)));
    avel.y = avel.y *(500/(500+fabs(avel.y)));
    avel.z = avel.z *(500/(500+fabs(avel.z)));

    pos += apos + (vel + avel + 0.5*aacc*dframe)*dframe;
    vel += avel + aacc*dframe;
    acc  = aacc + avel/dframe + 2*(apos-vel*dframe)/dframe;

    turn_lr += (vel_lr + avel_lr + 0.5*aacc_lr*dframe)*dframe;
    vel_lr += aacc_lr * dframe + avel_lr;
    acc_lr = aacc_lr + avel_lr/dframe;

    integration_allowed = false;
  }

  void begin_integration()
  {
    clear();
    integration_allowed = true;
  }

  void clear()
  {
    apos = avel = aacc = vec3_t();
    apos_lr = avel_lr = aacc_lr = 0;
    integration_allowed = false;
  };
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool interact(Physics_Accumulator & ritema, Physics_Accumulator & ritemb, float tolerance_z = 0.0f);
bool interact_old(Physics_Accumulator & ritema, Physics_Accumulator & ritemb, float tolerance_z = 0.0f);

bool close_interact(Physics_Accumulator & ritema, Physics_Accumulator & ritemb, float tolerance_z = 0.0f);
bool close_interact_old(Physics_Accumulator & ritema, Physics_Accumulator & ritemb, float tolerance_z = 0.0f);

void collide_x(Physics_Accumulator & rchra, Physics_Accumulator & rchrb);
void collide_y(Physics_Accumulator & rchra, Physics_Accumulator & rchrb);
void collide_z(Physics_Accumulator & rchra, Physics_Accumulator & rchrb);
void pressure_x(Physics_Accumulator 
                & rchra, Physics_Accumulator & rchrb);
void pressure_y(Physics_Accumulator & rchra, Physics_Accumulator & rchrb);
void pressure_z(Physics_Accumulator & rchra, Physics_Accumulator & rchrb);

GLMatrix CreateOrientationMatrix(float tall, float wide, struct Orientation & ori, bool lean_into_turn=false);
