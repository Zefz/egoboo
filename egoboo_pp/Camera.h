#pragma once

#include "egoboo.h"
#include "Frustum.h"

struct Mesh;

#define CAM_KEYTURN                      100          // Device_Keyboard camera rotation
#define CAM_JOYTURN                      .01         // Device_Joystick camera rotation


// Multi cam
#define ZOOM_MIN                         100         // Camera distance
#define ZOOM_MAX                         900         //600
#define ZADD_MIN                         100         // Camera height
#define ZADD_MAX                         1000        //
#define UPDOWN_MIN                       (.24*PI)    // Camera updown angle
#define UPDOWN_MAX                       (.15*PI)    // (.18*PI)

#define FOV                             45          // Field of view
#define ROTMESHTOPSIDE                  55          // For figuring out what to draw
#define ROTMESHBOTTOMSIDE               65          //
#define ROTMESHUP                       40 //35          //
#define ROTMESHDOWN                     60          //

#define EDGE                0x80                     // Camera bounds
#define TRACK_FAR            1200                    // For outside modules...
#define TRACK_EDGE           800                     // Camtrack bounds

#define DELAY_TURN 16

#define TRACKXAREALOW     100
#define TRACKXAREAHIGH    180
#define TRACKYAREAMINLOW  320
#define TRACKYAREAMAXLOW  460
#define TRACKYAREAMINHIGH 460
#define TRACKYAREAMAXHIGH 600

struct Player_List;
struct Character_List;

struct Camera
{
  DigiBen::CFrustum       frustum;               // for frustum culling

  vec3_t                  pos;                   // Camera position

  vec3_t                  center;                // Move character to side before tracking

  vec3_t                  track;                 // Trackee position
  vec3_t                  track_vel;             // Change in trackee position
  float                   track_level;           //
  vec3_t                  track_old;

  float                   zoom;                  // Distance from the trackee
  float                   zadd;                  // Camera height above terrain
  float                   zadd_goto;             // Desired z position
  float                   z_goto;                //

  float                   turn_lr;               // Camera rotations
  float                   turn_ud;
  float                   roll;                  //
  float                   turn_old_lr;

  float                   turn_add;              // Turning rate

  int                     swing;                 // Camera swingin'
  int                     swing_rate;            //
  float                   swing_amp;             //

  float                   sustain;               // "memory" of past states

  GLMatrix mWorld;          // World Matrix
  GLMatrix mView;           // View Matrix
  GLMatrix mViewSave;       // View Matrix initial state
  GLMatrix mProjection;     // Projection Matrix

  //float    corner_x[4];                 // Render area corners
  //float    corner_y[4];                 //
  //int      corner_listlowtohigh_y[4];    // Ordered list
  //int      corner_low_x;                 // Render area extremes
  //int      corner_high_x;                //
  //int      corner_low_y;                 //
  //int      corner_high_y;                //
  //int      rotmesh_topside;
  //int      rotmesh_bottomside;
  //int      rotmesh_up;
  //int      rotmesh_down;

  //some local versions of global variables
  bool          use_faredge;                 //
  unsigned char autoturn;             // Type of camera control...
  unsigned char turn_time;            // Time for smooth turn

  Camera();

  //void set_look_at(vec2_t & pos);
  //void project_frustum();
  void make_matrix();
  void bound(Mesh & mesh);
  void bound_track(Mesh & mesh);
  void reset(Player_List & plist, Character_List & clist, Mesh & mesh, bool reset_all = false);
  void move(Player_List & plist, Character_List & clist, Mesh & mesh, float deltaTime = FRAMESKIP);

  void set_up_matrices(float fov);

protected:
  void adjust_angle(int height);
};

extern Camera GCamera;