/* Egoboo - camera.c
 * Various functions related to how the game camera works.
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
#include "mathstuff.h"
#include <assert.h>

CAMERA GCamera = {0,0,0,0,1500,750,1000,800,800,800,-PI_OVER_FOUR,-1.0f/8.0f,0,0,.8,PI_OVER_FOUR,0};


//--------------------------------------------------------------------------------------------
void camera_look_at(float x, float y)
{
  // ZZ> This function makes the camera turn to face the character
  GCamera.zgoto = GCamera.zadd;
  if (doturntime != 0)
  {
    GCamera.turnleftright = (3.0f * PI_OVER_TWO) - atan2(y - GCamera.y, x - GCamera.x);  // xgg
  }
}

//--------------------------------------------------------------------------------------------
void screen_dump_matrix(GAME_STATE * gs, GLMATRIX a)
{
  int i, j;
  STRING buffer1 = {0};
  STRING buffer2 = {0};

  reset_messages(gs);
  for (j=0; j<4; j++)
  {
    snprintf(buffer1, sizeof(buffer1), "  ");
    for (i=0; i<4; i++)
    {
      snprintf(buffer2, sizeof(buffer2), "%2.4f ", (a)_CNV(i, j));
      strncat(buffer1, buffer2, sizeof(buffer1));
    };
    debug_message(buffer1);
    buffer1[0] = 0x00;
  }
}

//--------------------------------------------------------------------------------------------
void stdout_dump_matrix(GLMATRIX a)
{
  int i, j;

  for (j=0; j<4; j++)
  {
    printf("  ");
    for (i=0; i<4; i++)
    {
      printf("%2.4f ", (a)_CNV(i, j));
    };
    printf("\n");
  }
  printf("\n");
}

//--------------------------------------------------------------------------------------------
void project_view()
{
  // ZZ> This function figures out where the corners of the view area
  //     go when projected onto the plane of the mesh.  Used later for
  //     determining which mesh fans need to be rendered

  int cnt, tnc, extra[2];
  float ztemp;
  float numstep;
  float zproject;
  float xfin, yfin, zfin;
  GLMATRIX mTemp;

  // Range
  ztemp = (GCamera.z);

  // Topleft
  //printf("DIAG: In project_view\n");
  //printf("DIAG: dumping mView\n"); dump_matrix(mView);
  //printf("cam xyz,zoom = %f %f %f %f\n",GCamera.x,camy,GCamera.z,camzoom);

  mTemp = MatrixMult(RotateY(-rotmeshtopside * 0.5f * DEG_TO_RAD), mView);
  mTemp = MatrixMult(RotateX(rotmeshup * 0.5f * DEG_TO_RAD), mTemp);
  zproject = (mTemp)_CNV(2, 2);        //2,2
  // Camera must look down
  if (zproject < 0)
  {
    numstep = -ztemp / zproject;
    xfin = GCamera.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
    yfin = GCamera.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
    zfin = 0;
    cornerx[0] = xfin;
    cornery[0] = yfin;
    //printf("Camera TL: %f %f\n",xfin,yfin);
    //dump_matrix(mTemp);
  }

  // Topright
  mTemp = MatrixMult(RotateY(rotmeshtopside * 0.5f * DEG_TO_RAD), mView);
  mTemp = MatrixMult(RotateX(rotmeshup * 0.5f * DEG_TO_RAD), mTemp);
  zproject = (mTemp)_CNV(2, 2);        //2,2
  // Camera must look down
  if (zproject < 0)
  {
    numstep = -ztemp / zproject;
    xfin = GCamera.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
    yfin = GCamera.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
    zfin = 0;
    cornerx[1] = xfin;
    cornery[1] = yfin;
    //printf("Camera TR: %f %f\n",xfin,yfin);
    //dump_matrix(mTemp);
  }

  // Bottomright
  mTemp = MatrixMult(RotateY(rotmeshbottomside * 0.5f * DEG_TO_RAD), mView);
  mTemp = MatrixMult(RotateX(-rotmeshdown * 0.5f * DEG_TO_RAD), mTemp);
  zproject = (mTemp)_CNV(2, 2);        //2,2
  // Camera must look down
  if (zproject < 0)
  {
    numstep = -ztemp / zproject;
    xfin = GCamera.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
    yfin = GCamera.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
    zfin = 0;
    cornerx[2] = xfin;
    cornery[2] = yfin;
    //printf("Camera BR: %f %f\n",xfin,yfin);
    //dump_matrix(mTemp);
  }

  // Bottomleft
  mTemp = MatrixMult(RotateY(-rotmeshbottomside * 0.5f * DEG_TO_RAD), mView);
  mTemp = MatrixMult(RotateX(-rotmeshdown * 0.5f * DEG_TO_RAD), mTemp);
  zproject = (mTemp)_CNV(2, 2);        //2,2
  // Camera must look down
  if (zproject < 0)
  {
    numstep = -ztemp / zproject;
    xfin = GCamera.x + (numstep * (mTemp)_CNV(0, 2));  // xgg   //0,2
    yfin = GCamera.y + (numstep * (mTemp)_CNV(1, 2));     //1,2
    zfin = 0;
    cornerx[3] = xfin;
    cornery[3] = yfin;
    //printf("Camera BL: %f %f\n",xfin,yfin);
    //dump_matrix(mTemp);
  }

  // Get the extreme values
  cornerlowx = cornerx[0];
  cornerlowy = cornery[0];
  cornerhighx = cornerx[0];
  cornerhighy = cornery[0];
  cornerlistlowtohighy[0] = 0;
  cornerlistlowtohighy[3] = 0;

  for (cnt = 0; cnt < 4; cnt++)
  {
    if (cornerx[cnt] < cornerlowx)
      cornerlowx = cornerx[cnt];
    if (cornery[cnt] < cornerlowy)
    {
      cornerlowy = cornery[cnt];
      cornerlistlowtohighy[0] = cnt;
    }
    if (cornerx[cnt] > cornerhighx)
      cornerhighx = cornerx[cnt];
    if (cornery[cnt] > cornerhighy)
    {
      cornerhighy = cornery[cnt];
      cornerlistlowtohighy[3] = cnt;
    }
  }

  // Figure out the order of points
  tnc = 0;
  for (cnt = 0; cnt < 4; cnt++)
  {
    if (cnt != cornerlistlowtohighy[0] && cnt != cornerlistlowtohighy[3])
    {
      extra[tnc] = cnt;
      tnc++;
    }
  }
  cornerlistlowtohighy[1] = extra[1];
  cornerlistlowtohighy[2] = extra[0];
  if (cornery[extra[0]] < cornery[extra[1]])
  {
    cornerlistlowtohighy[1] = extra[0];
    cornerlistlowtohighy[2] = extra[1];
  }

  // BAD: exit here
  //printf("Corners:\n");
  //printf("x: %d %d\n",cornerlowx,cornerhighx);
  //printf("y: %d %d\n",cornerlowy,cornerhighy);
  /*printf("Exiting, camera code is broken\n");
  exit(0);*/
}

//--------------------------------------------------------------------------------------------
void make_camera_matrix()
{
  // ZZ> This function sets mView to the camera's location and rotation
  mView = mViewSave;
  mView = MatrixMult(Translate(GCamera.x, -GCamera.y, GCamera.z), mView);  // xgg
  if (GCamera.swingamp > .001)
  {
    GCamera.roll = turntosin[GCamera.swing & TRIGTABLE_MASK] * GCamera.swingamp;
    mView = MatrixMult(RotateY(GCamera.roll), mView);
  }
  mView = MatrixMult(RotateZ(GCamera.turnleftright), mView);
  mView = MatrixMult(RotateX(GCamera.turnupdown), mView);
  //lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_VIEW, &mView);
//        glMatrixMode(GL_MODELVIEW);
///        glLoadMatrixf(mView.v);
}

//--------------------------------------------------------------------------------------------
void bound_camera()
{
  // ZZ> This function stops the camera from moving off the mesh
  if (GCamera.x < EDGE)  GCamera.x = EDGE;
  if (GCamera.x > Mesh.edgex - EDGE)  GCamera.x = Mesh.edgex - EDGE;
  if (GCamera.y < EDGE)  GCamera.y = EDGE;
  if (GCamera.y > Mesh.edgey - EDGE)  GCamera.y = Mesh.edgey - EDGE;
}

//--------------------------------------------------------------------------------------------
void bound_camera_track()
{
  // ZZ> This function stops the camera target from moving off the mesh
  if (usefaredge)
  {
    if (GCamera.trackx < FARTRACK)  GCamera.trackx = FARTRACK;
    if (GCamera.trackx > Mesh.edgex - FARTRACK)  GCamera.trackx = Mesh.edgex - FARTRACK;
    if (GCamera.tracky < FARTRACK)  GCamera.tracky = FARTRACK;
    if (GCamera.tracky > Mesh.edgey - FARTRACK)  GCamera.tracky = Mesh.edgey - FARTRACK;
  }
  else
  {
    if (GCamera.trackx < EDGETRACK)  GCamera.trackx = EDGETRACK;
    if (GCamera.trackx > Mesh.edgex - EDGETRACK)  GCamera.trackx = Mesh.edgex - EDGETRACK;
    if (GCamera.tracky < EDGETRACK)  GCamera.tracky = EDGETRACK;
    if (GCamera.tracky > Mesh.edgey - EDGETRACK)  GCamera.tracky = Mesh.edgey - EDGETRACK;
  }
}

//--------------------------------------------------------------------------------------------
void adjust_camera_angle(int height)
{
  // ZZ> This function makes the camera look downwards as it is raised up
  float percentmin, percentmax;


  if (height < MINZADD)  height = MINZADD;
  percentmax = (height - MINZADD) / (float)(MAXZADD - MINZADD);
  percentmin = 1.0 - percentmax;

  GCamera.turnupdown = ((MINUPDOWN * percentmin) + (MAXUPDOWN * percentmax));
  GCamera.zoom = (MINZOOM * percentmin) + (MAXZOOM * percentmax);
}

//--------------------------------------------------------------------------------------------
void move_camera(GAME_STATE * gs, float dFrame)
{
  // ZZ> This function moves the camera
  int cnt, locoalive, movex, movey;  //Used in rts remove? -> int band,
  float x, y, z, level, newx, newy;
  Uint16 character, turnsin, turncos;

  //screen_dump_matrix(mView);

  if (CData.autoturncamera)
    doturntime = 255;
  else if (doturntime != 0)
    doturntime--;

  x = 0;
  y = 0;
  z = 0;
  level = 0;
  locoalive = 0;

  for (cnt = 0; cnt < MAXPLAYER; cnt++)
  {
    if (PlaList[cnt].valid && PlaList[cnt].device != INPUTNONE)
    {
      character = PlaList[cnt].index;
      if (ChrList[character].alive)
      {
        if (ChrList[character].attachedto == MAXCHR)
        {
          // The character is on foot
          x += ChrList[character].xpos;
          y += ChrList[character].ypos;
          z += ChrList[character].zpos;
          level += ChrList[character].level;
        }
        else
        {
          // The character is mounted
          x += ChrList[ChrList[character].attachedto].xpos;
          y += ChrList[ChrList[character].attachedto].ypos;
          z += ChrList[ChrList[character].attachedto].zpos;
          level += ChrList[ChrList[character].attachedto].level;
        }
        locoalive++;
      }
    }
  }

  if (locoalive > 0)
  {
    x = x / locoalive;
    y = y / locoalive;
    z = z / locoalive;
    level = level / locoalive;
  }
  else
  {
    x = GCamera.trackx;
    y = GCamera.tracky;
    z = GCamera.trackz;
  }

//Remove old RTS stuff?
  /*    if(->modstate.rts_control)
      {
          if(GMous.button[0])
          {
              x = GCamera.trackx;
              y = GCamera.tracky;
          }
          else
          {
              band = 50;
              movex = 0;
              movey = 0;
              if(cursorx < band+6)
                  movex += -(band+6-cursorx);
              if(cursorx > CData.scrx-band-16)
                  movex += cursorx+16-CData.scrx+band;
              if(cursory < band+8)
                  movey += -(band+8-cursory);
              if(cursory > CData.scry-band-24)
                  movey += cursory+24-CData.scry+band;
              turnsin = (GCamera.turnleftrightone*16383);
              turnsin = turnsin & TRIGTABLE_MASK;
              turncos = (turnsin+4096) & TRIGTABLE_MASK;
              x = (movex*turntosin[turncos]+movey*turntosin[turnsin])*GRTS.scrollrate;
              y = (-movex*turntosin[turnsin]+movey*turntosin[turncos])*GRTS.scrollrate;
              GCamera.x = (GCamera.x + GCamera.x + x)/2.0;
              GCamera.y = (GCamera.y + GCamera.y + y)/2.0;
              x = GCamera.trackx+x;
              y = GCamera.tracky+y;
          }
          if(GRTS.setcamera)
          {
              x = GRTS.setcamerax;
              y = GRTS.setcameray;
          }
          z = GCamera.trackz;
      }*/
  GCamera.trackxvel = -GCamera.trackx;
  GCamera.trackyvel = -GCamera.tracky;
  GCamera.trackzvel = -GCamera.trackz;
  GCamera.trackx = (GCamera.trackx + x) / 2.0;
  GCamera.tracky = (GCamera.tracky + y) / 2.0;
  GCamera.trackz = (GCamera.trackz + z) / 2.0;
  GCamera.tracklevel = (GCamera.tracklevel + level) / 2.0;


  GCamera.turnadd = GCamera.turnadd * GCamera.sustain;
  GCamera.zadd = (GCamera.zadd * 3.0 + GCamera.zaddgoto) / 4.0;
  GCamera.z = (GCamera.z * 3.0 + GCamera.zgoto) / 4.0;
  // Camera controls
  if (CData.autoturncamera == 255 && numlocalpla == 1)
  {
    if (GMous.on && !control_mouse_is_pressed(gs, MOS_CAMERA))
        GCamera.turnadd -= (GMous.dx * .5) * dFrame;
    if (GKeyb.on)
      GCamera.turnadd += (control_key_is_pressed(gs, KEY_LEFT) - control_key_is_pressed(gs, KEY_RIGHT)) * CAMKEYTURN * dFrame;
    if (GJoy[0].on && !control_joya_is_pressed(gs, JOA_CAMERA))
        GCamera.turnadd -= GJoy[0].x * CAMJOYTURN * dFrame;
    if (GJoy[1].on && !control_joyb_is_pressed(gs, JOB_CAMERA))
        GCamera.turnadd -= GJoy[1].x * CAMJOYTURN * dFrame;
  }
  else
  {
    if (GMous.on)
    {
      if (control_mouse_is_pressed(gs, MOS_CAMERA))
      {
        GCamera.turnadd += (GMous.dx / 3.0) * dFrame;
        GCamera.zaddgoto += (float) GMous.dy / 3.0 * dFrame;
        if (GCamera.zaddgoto < MINZADD)  GCamera.zaddgoto = MINZADD;
        if (GCamera.zaddgoto > MAXZADD)  GCamera.zaddgoto = MAXZADD;
        doturntime = TURNTIME;  // Sticky turn...
      }
    }
    // JoyA camera controls
    if (GJoy[0].on)
    {
      if (control_joya_is_pressed(gs, JOA_CAMERA))
      {
        GCamera.turnadd  += GJoy[0].x * CAMJOYTURN * dFrame;
        GCamera.zaddgoto += GJoy[0].y * CAMJOYTURN * dFrame;
        if (GCamera.zaddgoto < MINZADD)  GCamera.zaddgoto = MINZADD;
        if (GCamera.zaddgoto > MAXZADD)  GCamera.zaddgoto = MAXZADD;
        doturntime = TURNTIME;  // Sticky turn...
      }
    }
    // JoyB camera controls
    if (GJoy[1].on)
    {
      if (control_joyb_is_pressed(gs, JOB_CAMERA))
      {
        GCamera.turnadd  += GJoy[1].x * CAMJOYTURN * dFrame;
        GCamera.zaddgoto += GJoy[1].y * CAMJOYTURN * dFrame;
        if (GCamera.zaddgoto < MINZADD)  GCamera.zaddgoto = MINZADD;
        if (GCamera.zaddgoto > MAXZADD)  GCamera.zaddgoto = MAXZADD;
        doturntime = TURNTIME;  // Sticky turn...
      }
    }
  }
  // Keyboard camera controls
  if (GKeyb.on)
  {
    if (control_key_is_pressed(gs, KEY_CAMERA_LEFT) || control_key_is_pressed(gs, KEY_CAMERA_RIGHT))
    {
      GCamera.turnadd += (control_key_is_pressed(gs, KEY_CAMERA_LEFT) - control_key_is_pressed(gs, KEY_CAMERA_RIGHT)) * CAMKEYTURN * dFrame;
      doturntime = TURNTIME;  // Sticky turn...
    }
    if (control_key_is_pressed(gs, KEY_CAMERA_IN) || control_key_is_pressed(gs, KEY_CAMERA_OUT))
    {
      GCamera.zaddgoto += (control_key_is_pressed(gs, KEY_CAMERA_OUT) - control_key_is_pressed(gs, KEY_CAMERA_IN)) * CAMKEYTURN * dFrame;
      if (GCamera.zaddgoto < MINZADD)  GCamera.zaddgoto = MINZADD;
      if (GCamera.zaddgoto > MAXZADD)  GCamera.zaddgoto = MAXZADD;
    }
  }
  GCamera.x -= (float)((mView)_CNV(0, 0)) * GCamera.turnadd * dFrame;  // xgg
  GCamera.y += (float)((mView)_CNV(1, 0)) * -GCamera.turnadd * dFrame;

  // Make it not break...
  //bound_camera_track();
  //bound_camera();

  // Do distance effects for overlay and background
  GCamera.trackxvel += GCamera.trackx;
  GCamera.trackyvel += GCamera.tracky;
  GCamera.trackzvel += GCamera.trackz;
  if (CData.overlayon)
  {
    // Do fg distance effect
    waterlayeru[0] += GCamera.trackxvel * waterlayerdistx[0] * dFrame;
    waterlayerv[0] += GCamera.trackyvel * waterlayerdisty[0] * dFrame;
  }
  if (clearson == bfalse)
  {
    // Do bg distance effect
    waterlayeru[1] += GCamera.trackxvel * waterlayerdistx[1] * dFrame;
    waterlayerv[1] += GCamera.trackyvel * waterlayerdisty[1] * dFrame;
  }

  // Center on target for doing rotation...
  if (doturntime != 0)
  {
    GCamera.centerx = GCamera.centerx * .9 + GCamera.trackx * .1;
    GCamera.centery = GCamera.centery * .9 + GCamera.tracky * .1;
  }

  // Create a tolerance area for walking without camera movement
  x = GCamera.trackx - GCamera.x;
  y = GCamera.tracky - GCamera.y;
  newx = -((mView)_CNV(0, 0) * x + (mView)_CNV(1, 0) * y); //newx = -(mView(0,0) * x + mView(1,0) * y);
  newy = -((mView)_CNV(0, 1) * x + (mView)_CNV(1, 1) * y); //newy = -(mView(0,1) * x + mView(1,1) * y);


  // Debug information
  //debug_message("%f %f", newx, newy);

  // Get ready to scroll...
  movex = 0;
  movey = 0;

  // Adjust for camera height...
  z = (TRACKXAREALOW  * (MAXZADD - GCamera.zadd)) +
      (TRACKXAREAHIGH * (GCamera.zadd - MINZADD));
  z = z / (MAXZADD - MINZADD);
  if (newx < -z)
  {
    // Scroll left
    movex += (newx + z) * dFrame;
  }
  if (newx > z)
  {
    // Scroll right
    movex += (newx - z) * dFrame;
  }

  // Adjust for camera height...
  z = (TRACKYAREAMINLOW  * (MAXZADD - GCamera.zadd)) +
      (TRACKYAREAMINHIGH * (GCamera.zadd - MINZADD));
  z = z / (MAXZADD - MINZADD);

  if (newy < z)
  {
    // Scroll down
    movey -= (newy - z) * dFrame;
  }
  else
  {
    // Adjust for camera height...
    z = (TRACKYAREAMAXLOW  * (MAXZADD - GCamera.zadd)) +
        (TRACKYAREAMAXHIGH * (GCamera.zadd - MINZADD));
    z = z / (MAXZADD - MINZADD);
    if (newy > z)
    {
      // Scroll up
      movey -= (newy - z) * dFrame;
    }
  }

  turnsin = (GCamera.turnleftrightone * 16383);
  turnsin = turnsin & TRIGTABLE_MASK;
  turncos = (turnsin + 4096) & TRIGTABLE_MASK;
  GCamera.centerx += (movex * turntosin[turncos] + movey * turntosin[turnsin]) * dFrame;
  GCamera.centery += (-movex * turntosin[turnsin] + movey * turntosin[turncos]) * dFrame;

  // Finish up the camera
  camera_look_at(GCamera.centerx, GCamera.centery);
  GCamera.x = (float) GCamera.centerx + (GCamera.zoom * sin(GCamera.turnleftright));
  GCamera.y = (float) GCamera.centery + (GCamera.zoom * cos(GCamera.turnleftright));
  //bound_camera();
//        adjust_camera_angle(GCamera.z-camtracklevel);
  adjust_camera_angle(GCamera.z);

  make_camera_matrix();
}

//--------------------------------------------------------------------------------------------
void reset_camera(GAME_STATE * gs)
{
  // ZZ> This function makes sure the camera starts in a suitable position
  int cnt, save;
//    int mi;

  GCamera.swing = 0;
  GCamera.x = Mesh.edgex / 2;
  GCamera.y = Mesh.edgey / 2;
  GCamera.z = 800;
  GCamera.zoom = 1000;
//    GRTS.x = 0;
//    GRTS.y = 0;
  GCamera.trackxvel = 0;
  GCamera.trackyvel = 0;
  GCamera.trackzvel = 0;
  GCamera.centerx = GCamera.x;
  GCamera.centery = GCamera.y;
  GCamera.trackx = GCamera.x;
  GCamera.tracky = GCamera.y;
  GCamera.trackz = 0;
  GCamera.turnadd = 0;
  GCamera.tracklevel = 0;
  GCamera.zadd = 800;
  GCamera.zaddgoto = 800;
  GCamera.zgoto = 800;
  GCamera.turnleftright = (float)(-PI_OVER_FOUR);
  GCamera.turnleftrightone = (float)(-PI_OVER_FOUR) / (TWO_PI);
  GCamera.turnleftrightshort = 0;
  GCamera.turnupdown = (float)(PI_OVER_FOUR);
  GCamera.roll = 0;

  // Now move the camera towards the players
  mView = ZeroMatrix();

  save = CData.autoturncamera;
  CData.autoturncamera = btrue;

  for (cnt = 0; cnt < 32; cnt++)
  {
    move_camera(gs, 1.0);
    GCamera.centerx = GCamera.trackx;
    GCamera.centery = GCamera.tracky;
  }

  CData.autoturncamera = save;
  doturntime = 0;
}

