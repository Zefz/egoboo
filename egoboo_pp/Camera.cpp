// camera.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "egoboo.h"
#include "Camera.h"
#include "Character.h"
#include "mathstuff.h"
#include "input.h"
#include "Ui.h"
#include <assert.h>

Camera GCamera;

//--------------------------------------------------------------------------------------------

Camera        gCamera;

Camera::Camera()
{
  swing       = 0;               // Camera swingin'
  swing_rate  = 0;           //
  swing_amp   = 0;            //

  pos = center = vec3_t(150,750,750);
  zadd = zadd_goto = 750;
  zoom = 1000;


  //turn_lr = turn_old_lr = (float) -(1<<14) / ;       // Camera rotations
  //turnone_lr = (float) (-PI/4)*INV_TWO_PI;
  //turnshort_lr  = 0;
  turn_add    = 0;              // Turning rate
  sustain     = .80;                     // Turning rate falloff
  turn_ud     = (float) (PI/4);
  roll        = 0;                          //

  //grab the global defaults
  use_faredge = use_faredge;
  autoturn    = cam_autoturn;             // Type of camera control...
  turn_time   = cam_turn_time;            // Time for smooth turn
};

//--------------------------------------------------------------------------------------------
void Camera::make_matrix()
{
  // ZZ> This function sets mView to the camera's location and rotation

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
    glLoadIdentity();
    glScalef(-1,1,1);

    if(swing_amp > .001)
    {
      roll = sin_tab[swing]*swing_amp;
      glMultMatrixf( RotateY(roll).v );
    }

    //force the camera to look ahead
    gluLookAt( pos.x, pos.y, pos.z, track.x+10.0*track_vel.x, track.y+10.0*track_vel.y, track.z+track_vel.z, 0, 0, 1 );

    glGetFloatv( GL_MODELVIEW_MATRIX, mView.v );

  glPopMatrix();

  // ???? calculate the frustum every frame ????
  frustum.CalculateFrustum(mProjection.v, mView.v);
}

//--------------------------------------------------------------------------------------------
//void Camera::bound(Mesh & mlist)
//{
//  // ZZ> This function stops the camera from moving off the mesh
//
//  if(x < EDGE)  x = EDGE;
//  if(x > mlist.edge_x-EDGE)  x = mlist.edge_x-EDGE;
//  if(y < EDGE)  y = EDGE;
//  if(y > mlist.edge_y-EDGE)  y = mlist.edge_y-EDGE;
//}

//--------------------------------------------------------------------------------------------
//void Camera::bound_track(Mesh & mlist)
//{
//  // ZZ> This function stops the camera target from moving off the mesh
//  if(use_faredge)
//  {
//    if(track.x < TRACK_FAR)  track.x = TRACK_FAR;
//    if(track.x > mlist.edge_x-TRACK_FAR)  track.x = mlist.edge_x-TRACK_FAR;
//    if(track.y < TRACK_FAR)  track.y = TRACK_FAR;
//    if(track.y > mlist.edge_y-TRACK_FAR)  track.y = mlist.edge_y-TRACK_FAR;
//  }
//  else
//  {
//    if(track.x < TRACK_EDGE)  track.x = TRACK_EDGE;
//    if(track.x > mlist.edge_x-TRACK_EDGE)  track.x = mlist.edge_x-TRACK_EDGE;
//    if(track.y < TRACK_EDGE)  track.y = TRACK_EDGE;
//    if(track.y > mlist.edge_y-TRACK_EDGE)  track.y = mlist.edge_y-TRACK_EDGE;
//  }
//}

//--------------------------------------------------------------------------------------------
//void Camera::adjust_angle(int height)
//{
//  // ZZ> This function makes the camera look downwards as it is raised up
//  float percentmin, percentmax;
//
//
//  if(height < ZADD_MIN)  height = ZADD_MIN;
//  percentmax = (height-ZADD_MIN)/(float)(ZADD_MAX-ZADD_MIN);
//  percentmin = 1.0-percentmax;
//
//  turn_ud = ((UPDOWN_MIN*percentmin)+(UPDOWN_MAX*percentmax));
//  zoom    = (ZOOM_MIN*percentmin)+(ZOOM_MAX*percentmax);
//}

//--------------------------------------------------------------------------------------------
void Camera::move(Player_List & plist, Character_List & clist, Mesh & mlist, float deltaTime)
{
  // ZZ> This function moves the camera
  int cnt, alive_count, band, movex, movey;
  vec3_t loc_pos, loc_vel;
  float level, newx, newy;
  unsigned short character, turnsin;

  float dframe = CLIP((1000.0f*deltaTime) / FRAMESKIP, 0.2, 2);

  float loc_sustain;
  if(dframe == 0.0f && dframe == 1.0f)
    loc_sustain = sustain;
  else
    loc_sustain = powf(sustain, dframe);

  //if(0!= autoturn)
  //  turn_time = 0xFF;
  //else if(turn_time > 0)
  //  turn_time--;

  track_old = track;

  //find the average values of all characters in the "party"
  loc_pos.x = loc_pos.y = loc_pos.z = 0;
  loc_vel.x = loc_vel.y = loc_vel.z = 0;
  level = 0;
  alive_count = 0;
  for (cnt = 0; cnt<Player_List::SIZE; cnt++)
  {
    if( VALID_PLAYER(cnt) )
    {
      character = plist[cnt].index;
      if( VALID_CHR(character) && clist[character].alive)
      {
        if( VALID_CHR(clist[character].held_by) )
        {
          // The character is mounted
          loc_pos.x += clist[character].pos.x;
          loc_pos.y += clist[character].pos.y;
          loc_pos.z += clist[character].pos.z + clist[character].calc_bump_height*1.5;
          loc_vel.x += clist[clist[character].held_by].vel.x;
          loc_vel.y += clist[clist[character].held_by].vel.y;
          loc_vel.z += clist[clist[character].held_by].vel.z;
          level     += clist[clist[character].held_by].level;
        }
        else
        {
          // The character is on foot
          loc_pos.x += clist[character].pos.x;
          loc_pos.y += clist[character].pos.y;
          loc_pos.z += clist[character].pos.z + clist[character].calc_bump_height*1.5;
          loc_vel.x += clist[character].vel.x;
          loc_vel.y += clist[character].vel.y;
          loc_vel.z += clist[character].vel.z;
          level     += clist[character].level;
        }

        alive_count++;
      }
    }
  }

  if(alive_count>1)
  {
    loc_pos.x /= alive_count;
    loc_pos.y /= alive_count;
    loc_pos.z /= alive_count;
    loc_vel.x /= alive_count;
    loc_vel.y /= alive_count;
    loc_vel.z /= alive_count;
    level     /= alive_count;
  }
  else if(alive_count==0)
  {
    loc_pos = track;
    loc_vel = track_vel;
    level   = track_level;
  }

  // do the RTS camera movement
  if(GRTS.on)
  {
    if(GMous.button[0])
    {
      loc_pos.x = track.x;
      loc_pos.y = track.y;
    }
    else
    {
      band = 50;
      movex = 0;
      movey = 0;
      if(GUI.mouseX < band+6)
        movex += -(band+6-GUI.mouseX);
      if(GUI.mouseX > scrx-band-16)
        movex += GUI.mouseX+16-scrx+band;
      if(GUI.mouseY < band+8)
        movey += -(band+8-GUI.mouseY);
      if(GUI.mouseY > scry-band-24)
        movey += GUI.mouseY+24-scry+band;

      loc_pos.x = (movex*cos_tab[turn_lr]+movey*sin_tab[turn_lr])*GRTS.scrollrate;
      loc_pos.y = (-movex*sin_tab[turn_lr]+movey*cos_tab[turn_lr])*GRTS.scrollrate;
      pos.x = (pos.x + pos.x + loc_pos.x)/2.0;
      pos.y = (pos.y + pos.y + loc_pos.y)/2.0;
      loc_pos.x = track.x+loc_pos.x;
      loc_pos.y = track.y+loc_pos.y;
    }

    if(GRTS.set_camera)
    {
      loc_pos.x = GRTS.set_camera_x;
      loc_pos.y = GRTS.set_camera_y;
    }
    loc_pos.z = track.z;
  }

  track_vel   = track_vel  *loc_sustain + loc_vel*(1.0-loc_sustain);
  track       = track      *loc_sustain + loc_pos*(1.0-loc_sustain);
  track_level = track_level*loc_sustain + level  *(1.0-loc_sustain);

  // Camera controls
  float loc_turn_add = 0;
  float loc_zoom = zoom;
  if(autoturn == 0xFF && plist.count_local==1)
  {
    //!! AUTOTURN !!
    if(GMous.on)
      if(!CtrlList.mouse_is_pressed(MOS_CAMERA))
        loc_turn_add = -(GMous.dx*.5)*dframe;

    if(GKeyb.on)
      loc_turn_add = -(CtrlList.key_is_pressed(KEY_LEFT)-CtrlList.key_is_pressed(KEY_RIGHT))*CAM_KEYTURN*dframe;

    if(GJoy[0].on)
      if(!CtrlList.joya_is_pressed(JOA_CAMERA))
        loc_turn_add = -GJoy[0].latch.x*CAM_JOYTURN*dframe;

    if(GJoy[1].on)
      if(!CtrlList.joyb_is_pressed(JOB_CAMERA))
        loc_turn_add = -GJoy[1].latch.x*CAM_JOYTURN*dframe;
  }
  else
  {
    if(GMous.on && CtrlList.mouse_is_pressed(MOS_CAMERA))
    {
      loc_turn_add = (GMous.dx/3.0)*dframe;
      zadd_goto += (float) GMous.dy/3.0*dframe;
    }

    if(GMous.on && GUI.wheel_event)
    {
      GMous.w = CLIP(GMous.w,-20,20);
      GUI.wheel_event = false;

      float tmp = GMous.w / 20.0f;
      loc_zoom = (ZOOM_MAX+ZOOM_MIN) - (ZOOM_MAX-ZOOM_MIN)*tmp;
    }

    // JoyA camera controls
    if(GJoy[0].on && CtrlList.joya_is_pressed(JOA_CAMERA))
    {
      loc_turn_add  = GJoy[0].latch.x*CAM_JOYTURN*dframe;
      zadd_goto    += GJoy[0].latch.y*CAM_JOYTURN*dframe;
    }

    // JoyB camera controls
    if(GJoy[1].on && CtrlList.joyb_is_pressed(JOB_CAMERA))
    {
      loc_turn_add = GJoy[1].latch.x*CAM_JOYTURN*dframe;
      zadd_goto   += GJoy[1].latch.y*CAM_JOYTURN*dframe;
    }
  }

  // Device_Keyboard camera controls
  if(GKeyb.on)
  {
    if(CtrlList.key_is_pressed(KEY_CAMERA_LEFT) || CtrlList.key_is_pressed(KEY_CAMERA_RIGHT))
    {
      loc_turn_add = -(CtrlList.key_is_pressed(KEY_CAMERA_LEFT)-CtrlList.key_is_pressed(KEY_CAMERA_RIGHT))*CAM_KEYTURN*dframe;
    }

    if(CtrlList.key_is_pressed(KEY_CAMERA_IN) || CtrlList.key_is_pressed(KEY_CAMERA_OUT))
    {
      zadd_goto += (CtrlList.key_is_pressed(KEY_CAMERA_OUT)-CtrlList.key_is_pressed(KEY_CAMERA_IN))*CAM_KEYTURN*dframe;
    }
  }

  zadd_goto = CLIP(zadd_goto, (float)ZADD_MIN, (float)ZADD_MAX);
  loc_zoom = float(zadd_goto-ZADD_MIN)/float(ZADD_MAX-ZADD_MIN) * (ZOOM_MAX - ZOOM_MIN) + ZOOM_MIN;

  loc_turn_add *= 100;
  turn_add  = turn_add*loc_sustain + loc_turn_add*(1.0f-loc_sustain);
  zadd      = zadd*loc_sustain + zadd_goto*(1.0f-loc_sustain);
  zoom      = zoom*loc_sustain + loc_zoom*(1.0f-loc_sustain);

  // Do distance effects for overlay and background
  if(overlay_on)
  {
    // Do fg distance effect
    WaterList[0].off.s += (WaterList[0].off_add.s - track_vel.x*WaterList[0].dist.x)*dframe;
    WaterList[0].off.t += (WaterList[0].off_add.t - track_vel.y*WaterList[0].dist.y)*dframe;
  }

  if(!clearson)
  {
    // Do bg distance effect
    WaterList[1].off.s += (WaterList[1].off_add.s - track_vel.x*WaterList[1].dist.x)*dframe;
    WaterList[1].off.t += (WaterList[1].off_add.t - track_vel.y*WaterList[1].dist.y)*dframe;
  }

  // Finish up the camera
  //set_look_at(center.x,center.y);
  float rad  = zoom*2.0*zadd/(zadd+ZADD_MIN);
  turn_lr   += turn_add / 30.0 * dframe;
  loc_pos.x  = track.x + rad*cos_tab[turn_lr];
  loc_pos.y  = track.y + rad*sin_tab[turn_lr];
  loc_pos.z  = track.z + zadd;


  pos = pos * loc_sustain + loc_pos*(1.0-loc_sustain);
  vec3_t vdif = pos-track;
  normalize_iterative(vdif);
  pos.x = vdif.x*rad + track.x;
  pos.y = vdif.y*rad + track.y;

  //bound(mlist);
  //adjust_angle(z-track_level);

  make_matrix();
}

//--------------------------------------------------------------------------------------------
void Camera::reset(Player_List & plist, Character_List & clist, Mesh & mlist, bool reset_all)
{
  // ZZ> This function makes sure the camera starts in a suitable position
  swing = 0;

  pos.x = center.x = mlist.width()/2;        // start camera at center of mesh
  pos.y = center.y = mlist.height()/2;
  pos.z = center.z = 800;
  zadd = zadd_goto = 800;
  zoom = 1000;

  track_vel.x = 0;
  track_vel.y = 0;
  track_vel.z = 0;
  track.x = pos.x;
  track.y = pos.y;
  track.z = 0;

  turn_add = 0;
  track_level = 0;

  //turn_lr     = (float) (-PI/4);
  //turnone_lr = (float) (-PI/4)*INV_TWO_PI;
  //turnshort_lr  = 0;
  //turn_ud     = (float) (PI/4);
  roll        = 0;

  // Now move the camera towards the players
  mView = ZeroMatrix();

  Uint8 autoturn_save = autoturn;
  bool  faredge_save  = use_faredge;

  // force camera to relax from its intial position to a
  // position near the targeted players
  if(GRTS.on) GRTS.set_camera = true;
  autoturn = true;
  use_faredge = use_faredge;
  //for (cnt = 0; cnt < 0x20; cnt++)
  //{
  //  move(plist, clist, mlist, FRAMESKIP, true);
  //  center.x = track.x;
  //  center.y = track.y;
  //}

  if(reset_all)
  {
    //grab the global defaults
    autoturn    = ::cam_autoturn;             // Type of camera control...
    turn_time   = ::cam_turn_time;            // Time for smooth turn
    use_faredge = ::use_faredge;
  }
  else
  {
    autoturn    = autoturn_save;
    turn_time   = 0;
    use_faredge = faredge_save;
  };

  //GRTS.reset(*this, gMState, mlist, clist);
}

void Camera::set_up_matrices(float fov)
{
  GLint viewport[ 4 ];

  //rotmesh_topside    = ((float)scrx/scry)*ROTMESHTOPSIDE/(1.33333);
  //rotmesh_bottomside = ((float)scrx/scry)*ROTMESHBOTTOMSIDE/(1.33333);
  //rotmesh_up         = ((float)scrx/scry)*ROTMESHUP/(1.33333);
  //rotmesh_down       = ((float)scrx/scry)*ROTMESHDOWN/(1.33333);

  mWorld = IdentityMatrix();

  //use OpenGl to create our projection matrix
  glGetIntegerv ( GL_VIEWPORT, viewport );

  glMatrixMode ( GL_PROJECTION );
  glPushMatrix();
    glLoadIdentity ();
    gluPerspective( fov / 2.0, 480.0f / 640.0f * ( float ) ( viewport[ 2 ] - viewport[ 0 ] ) / ( float ) ( viewport[ 3 ] - viewport[ 1 ] ), 1.0f, 20000.0f );
    glGetFloatv( GL_PROJECTION_MATRIX, mProjection.v );
  glPopMatrix();

};