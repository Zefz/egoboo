
// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Particle.h"
#include "Character.h"
#include "Camera.h"
#include "egoboo.h"

void render_prt()
{
  // ZZ> This function draws the sprites for particle systems
  GLVertex v[PRT_COUNT];
  GLVertex vtlist[4];
  Uint16 cnt, prt, numparticle;
  Uint16 image;
  float size;
  Uint32 light;
  int i;

  // Calculate the up and right vectors for billboarding.
  vec3_t cam_up, cam_right, cam_in;

  cam_right.x = GCamera.mView.v[0];
  cam_right.y = GCamera.mView.v[4];
  cam_right.z = GCamera.mView.v[8];

  cam_up.x    = GCamera.mView.v[1];
  cam_up.y    = GCamera.mView.v[5];
  cam_up.z    = GCamera.mView.v[9];

  cam_in.x    = GCamera.mView.v[2];
  cam_in.y    = GCamera.mView.v[6];
  cam_in.z    = GCamera.mView.v[10];


  // Flat shade these babies
  glShadeModel(GL_FLAT);

  // Original points
  numparticle = 0;
  cnt = 0;
  {
    SCAN_PRT_BEGIN(cnt, rprt_cnt)
    {
      if (!rprt_cnt.inview || 0==rprt_cnt.size) continue;

      v[numparticle].pos.x = (float) rprt_cnt.pos.x;
      v[numparticle].pos.y = (float) rprt_cnt.pos.y;
      v[numparticle].pos.z = (float) rprt_cnt.pos.z;

      // [claforte] Aaron did a horrible hack here. Fix that ASAP.
      v[numparticle].icolor = cnt;  // Store an index in the color slot...
      numparticle++;

    }SCAN_PRT_END;
  }

  // Choose texture and matrix
  TxList[PrtList.texture].Bind(GL_TEXTURE_2D);

  // Make new ones so we can index them and not transform 'em each time
  //transform_vertices(numparticle, v, vt);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DITHER);

  //// DO ANTIALIAS OF SOLID SPRITES FIRST
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Render each particle that was on
  for (cnt = 0; cnt < numparticle; cnt++)
  {
    // Get the index from the color slot
    prt = (Uint16) v[cnt].icolor;

    // Draw transparent sprites this round
    if (PrtList[prt].type == SPRITE_SOLID)
    {
      float color_component = PrtList[prt].light / float(0xFF);
      float alpha_component;

      // Figure out the sprite's size based on distance
      alpha_component = antialiastrans / float(0xFF);

      glColor4f(color_component, color_component, color_component, alpha_component);//[claforte] should use alpha_component instead of 0.5?

      // [claforte] Fudge the value.
      size = (PrtList[prt].size) / float(1<<10) * 1.1f;

    // rotation
    Uint32 turn = PrtList[prt].rotate;
    float cosval = cos_tab[turn>>2];
    float sinval = sin_tab[turn>>2];

    vec3_t loc_vector_right = cam_right*cosval - cam_up*sinval;
    vec3_t loc_vector_up    = cam_right*sinval + cam_up*cosval;

    // Calculate the position of the four corners of the billboard
    // used to display the particle.
    vtlist[0].pos = v[cnt].pos + ((-loc_vector_right - loc_vector_up) * size);
    vtlist[1].pos = v[cnt].pos + (( loc_vector_right - loc_vector_up) * size);
    vtlist[2].pos = v[cnt].pos + (( loc_vector_right + loc_vector_up) * size);
    vtlist[3].pos = v[cnt].pos + ((-loc_vector_right + loc_vector_up) * size);

      // Fill in the rest of the data
      image = ((Uint16)(PrtList[prt].image+PrtList[prt].image_stt))>>8;

      vtlist[0].txcoord.s = PrtList.txcoord[image][0].s;
      vtlist[0].txcoord.t = PrtList.txcoord[image][0].t;

      vtlist[1].txcoord.s = PrtList.txcoord[image][1].s;
      vtlist[1].txcoord.t = PrtList.txcoord[image][0].t;

      vtlist[2].txcoord.s = PrtList.txcoord[image][1].s;
      vtlist[2].txcoord.t = PrtList.txcoord[image][1].t;

      vtlist[3].txcoord.s = PrtList.txcoord[image][0].s;
      vtlist[3].txcoord.t = PrtList.txcoord[image][1].t;

      // Go on and draw it
      glBegin(GL_TRIANGLE_FAN);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2fv(vtlist[i].txcoord.vals);
        glVertex3fv(vtlist[i].pos.vals);
      }
      glEnd();
    }
  };


  //======== DO SOLID SPRITES FIRST ========
  //Render each particle that was on
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for (cnt = 0; cnt < numparticle; cnt++)
  {
    // Get the index from the color slot
    prt = (Uint16) v[cnt].icolor;

    // Draw sprites this round
    if ( !(PrtList[prt].type == SPRITE_ATTACK || PrtList[prt].type == SPRITE_SOLID) ) continue;

    float color_component = PrtList[prt].light / float(0xFF);

    // [claforte] Fudge the value.
    size = float(PrtList[prt].size) / float(1<<10);

    // rotation
    Uint32 turn = PrtList[prt].rotate;
    float cosval = cos_tab[turn>>2];
    float sinval = sin_tab[turn>>2];

    vec3_t loc_vector_right = cam_right*cosval - cam_up*sinval;
    vec3_t loc_vector_up    = cam_right*sinval + cam_up*cosval;

    // Calculate the position of the four corners of the billboard
    // used to display the particle.
    vtlist[0].pos = v[cnt].pos + ((-loc_vector_right - loc_vector_up) * size);
    vtlist[1].pos = v[cnt].pos + (( loc_vector_right - loc_vector_up) * size);
    vtlist[2].pos = v[cnt].pos + (( loc_vector_right + loc_vector_up) * size);
    vtlist[3].pos = v[cnt].pos + ((-loc_vector_right + loc_vector_up) * size);

    // Fill in the rest of the data
    image = ((Uint16)(PrtList[prt].image + PrtList[prt].image_stt))>>8;

    vtlist[0].txcoord.s = PrtList.txcoord[image][0].s;
    vtlist[0].txcoord.t = PrtList.txcoord[image][0].t;

    vtlist[1].txcoord.s = PrtList.txcoord[image][1].s;
    vtlist[1].txcoord.t = PrtList.txcoord[image][0].t;

    vtlist[2].txcoord.s = PrtList.txcoord[image][1].s;
    vtlist[2].txcoord.t = PrtList.txcoord[image][1].t;

    vtlist[3].txcoord.s = PrtList.txcoord[image][0].s;
    vtlist[3].txcoord.t = PrtList.txcoord[image][1].t;

    glColor4f(color_component, color_component, color_component, 1.0);

    glBegin(GL_TRIANGLE_FAN);
    for (i = 0; i < 4; i++)
    {
      glTexCoord2fv(vtlist[i].txcoord.vals);
      glVertex3fv(vtlist[i].pos.vals);
    }
    glEnd();

  }

  //======== DO TRANSPARENT SPRITES NEXT ========
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Render each particle that was on
  for (cnt = 0; cnt < numparticle; cnt++)
  {
    // Get the index from the color slot
    prt = (Uint16) v[cnt].icolor;

    // Draw transparent sprites this round
    if (PrtList[prt].type != SPRITE_ALPHA) continue;

    float color_component = PrtList[prt].light / float(0xFF);
    float alpha_component = particletrans / float(0xFF);

    // [claforte] use alpha_component
    glColor4f(color_component, color_component, color_component, alpha_component);

    size = (PrtList[prt].size) / float(1<<10);

    // rotation
    Uint32 turn = PrtList[prt].rotate;
    float cosval = cos_tab[turn>>2];
    float sinval = sin_tab[turn>>2];

    vec3_t loc_vector_right = cam_right*cosval - cam_up*sinval;
    vec3_t loc_vector_up    = cam_right*sinval + cam_up*cosval;

    // Calculate the position of the four corners of the billboard
    // used to display the particle.
    vtlist[0].pos = v[cnt].pos + ((-loc_vector_right - loc_vector_up) * size);
    vtlist[1].pos = v[cnt].pos + (( loc_vector_right - loc_vector_up) * size);
    vtlist[2].pos = v[cnt].pos + (( loc_vector_right + loc_vector_up) * size);
    vtlist[3].pos = v[cnt].pos + ((-loc_vector_right + loc_vector_up) * size);

    // Fill in the rest of the data
    image = ((Uint16)(PrtList[prt].image+PrtList[prt].image_stt))>>8;

    vtlist[0].txcoord.s = PrtList.txcoord[image][0].s;
    vtlist[0].txcoord.t = PrtList.txcoord[image][0].t;

    vtlist[1].txcoord.s = PrtList.txcoord[image][1].s;
    vtlist[1].txcoord.t = PrtList.txcoord[image][0].t;

    vtlist[2].txcoord.s = PrtList.txcoord[image][1].s;
    vtlist[2].txcoord.t = PrtList.txcoord[image][1].t;

    vtlist[3].txcoord.s = PrtList.txcoord[image][0].s;
    vtlist[3].txcoord.t = PrtList.txcoord[image][1].t;

    // Go on and draw it
    glBegin(GL_TRIANGLE_FAN);
    for (i = 0; i < 4; i++)
    {
      glTexCoord2fv(vtlist[i].txcoord.vals);
      glVertex3fv(vtlist[i].pos.vals);
    }
    glEnd();

  }

  //======== LIGHTS DONE LAST======== 
  glDepthMask(GL_FALSE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  // Render each particle that was on
  for (cnt = 0; cnt < numparticle; cnt++)
  {
    // Get the index from the color slot
    prt = v[cnt].icolor;

    Particle & rprt = PrtList[prt];

    // Draw lights this round
    if (rprt.type != SPRITE_LIGHT) continue;

    // [claforte] Fudge the value.
    size = float(rprt.size) / float(1<<9);

    float color_component = rprt.dyna_level;

    vec3_t world_up, world_right;
    //if( VALID_CHR(rprt.attachedtocharacter) )
    //{
    //  Character & rchr = ChrList[rprt.attachedtocharacter];
    //  vec3_t eye_vec;

    //  world_up.x = rchr.matrix.CNV(2,0);   
    //  world_up.y = rchr.matrix.CNV(2,1);   
    //  world_up.z = rchr.matrix.CNV(2,2); 

    //  world_right = cross_product(cam_in, world_up);

    //  normalize_iterative(world_up);
    //  normalize_iterative(world_right);
    //}
    //else
    {
      world_up    = cam_up;
      world_right = cam_right;
    };

    // rotation
    Uint32 turn = rprt.rotate+8210;
    float cosval = cos_tab[turn>>2];
    float sinval = sin_tab[turn>>2];

    vec3_t loc_vector_right = world_right*cosval - world_up*sinval;
    vec3_t loc_vector_up    = world_right*sinval + world_up*cosval;

    // Calculate the position of the four corners of the billboard
    // used to display the particle.
    vtlist[0].pos = v[cnt].pos + ((-loc_vector_right - loc_vector_up) * size);
    vtlist[1].pos = v[cnt].pos + (( loc_vector_right - loc_vector_up) * size);
    vtlist[2].pos = v[cnt].pos + (( loc_vector_right + loc_vector_up) * size);
    vtlist[3].pos = v[cnt].pos + ((-loc_vector_right + loc_vector_up) * size);

    // Fill in the rest of the data
    image = ((Uint16)(rprt.image+rprt.image_stt))>>8;

    vtlist[0].txcoord.s = PrtList.txcoord[image][0].s;
    vtlist[0].txcoord.t = PrtList.txcoord[image][0].t;

    vtlist[1].txcoord.s = PrtList.txcoord[image][1].s;
    vtlist[1].txcoord.t = PrtList.txcoord[image][0].t;

    vtlist[2].txcoord.s = PrtList.txcoord[image][1].s;
    vtlist[2].txcoord.t = PrtList.txcoord[image][1].t;

    vtlist[3].txcoord.s = PrtList.txcoord[image][0].s;
    vtlist[3].txcoord.t = PrtList.txcoord[image][1].t;

    // Go on and draw it
    glColor4f(color_component, color_component, color_component, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    for (i = 0; i < 4; i++)
    {
      glTexCoord2fv(vtlist[i].txcoord.vals);
      glVertex3fv(vtlist[i].pos.vals);
    }
    glEnd();
  }

  glShadeModel(shading);
}

  /*
  if(GFog.on)
  {
      // The full fog value
      GFog.spec = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);
      while(_VALID_PRT_RANGE(cnt) )
      {
          if(PrtList[cnt].inview && PrtList[cnt].size!=0)
          {
              v[numparticle].x = (D3DVALUE) PrtList[cnt].pos.x;
              v[numparticle].y = (D3DVALUE) PrtList[cnt].pos.y;
              v[numparticle].z = (D3DVALUE) PrtList[cnt].pos.z;
              v[numparticle].color = cnt;  // Store an index in the color slot...

              // Figure out the fog coloring
              z = v[numparticle].z;
              if(z < GFog.top)
              {
                  if(z < GFog.bottom)
                  {
                      v[numparticle].dcSpecular = GFog.spec;  // Full fog
                  }
                  else
                  {
                      z = 1.0 - ((z - GFog.bottom)/GFog.distance);  // 0.0 to 1.0... Amount of fog to keep
                      red = (GFog.red * z);
                      grn = (GFog.grn * z);
                      blu = (GFog.blu * z);
                      light = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                      v[numparticle].dcSpecular = light;
                  }
              }
              else
              {
                  v[numparticle].dcSpecular = 0;  // No fog
              }

              v[numparticle].dwReserved = 0;
              numparticle++;
          }
          cnt++;
      }
  }
  else
  */

//--------------------------------------------------------------------------------------------
void render_refprt()
{
  // ZZ> This function draws sprites reflected in the floor
  GLVertex v[PRT_COUNT];
  GLVertex vtlist[4];
  Uint16 cnt, prt, numparticle;
  Uint16 image;
// float scale;
  float size;
// DWORD light;
  int startalpha;
  Uint32 usealpha = 0xffffff;
  int level;
// Uint16 rotate;
// float sinsize, cossize;
  int i;

  // Calculate the up and right vectors for billboarding.
  vec3_t vector_up, vector_right;
  vector_right.x =  GCamera.mView.v[0];
  vector_right.y =  GCamera.mView.v[4];
  vector_right.z =  GCamera.mView.v[8];

  vector_up.x    = -GCamera.mView.v[1];
  vector_up.y    = -GCamera.mView.v[5];
  vector_up.z    = -GCamera.mView.v[9];

  // Original points
  numparticle = 0;
  SCAN_PRT_BEGIN(cnt, rprt_cnt)
  {
    if (!rprt_cnt.inview || 0==rprt_cnt.size) continue;

    level = rprt_cnt.level;
    if (GMesh.has_flags(rprt_cnt.onwhichfan, MESHFX_DRAWREF))
    {
      level = rprt_cnt.level;
      v[numparticle].pos.x = (float) rprt_cnt.pos.x;
      v[numparticle].pos.y = (float) rprt_cnt.pos.y;
      v[numparticle].pos.z = (float)-rprt_cnt.pos.z+level+level;
      if (VALID_CHR(rprt_cnt.attachedtocharacter))
      {
        v[numparticle].pos.z += RAISE+RAISE;
      }
      v[numparticle].icolor = cnt;  // Store an index in the color slot...
      numparticle++;
    }

  } SCAN_PRT_END;

  // Choose texture.
  TxList[PrtList.texture].Bind(GL_TEXTURE_2D);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DITHER);

  // Render each particle that was on
  
  for (cnt = 0; cnt < numparticle; cnt++)
  {
    // Get the index from the color slot
    prt = (Uint16) v[cnt].icolor;

    // Draw lights this round
    if (PrtList[prt].type != SPRITE_LIGHT) continue;

    size = float(PrtList[prt].size) / float(1<<9);

    // Calculate the position of the four corners of the billboard
    // used to display the particle.
    vtlist[0].pos = v[cnt].pos + ((-vector_right - vector_up) * size);
    vtlist[1].pos = v[cnt].pos + (( vector_right - vector_up) * size);
    vtlist[2].pos = v[cnt].pos + (( vector_right + vector_up) * size);
    vtlist[3].pos = v[cnt].pos + ((-vector_right + vector_up) * size);

    // Fill in the rest of the data
    startalpha = (int)(0xFF+v[cnt].pos.z-level);
    if (startalpha < 0) startalpha = 0;
    startalpha = (startalpha|reffadeor)>>1;  // Fix for Riva owners
    if (startalpha > 0xFF) startalpha = 0xFF;
    if (startalpha > 0)
    {
      image = ((Uint16)(PrtList[prt].image+PrtList[prt].image_stt))>>8;
      //light = (startalpha<<24)|usealpha;
      glColor4f(1.0, 1.0, 1.0, startalpha / float(0xFF));

      //vtlist[0].dcSpecular = vt[cnt].dcSpecular;
      vtlist[0].txcoord.s = PrtList.txcoord[image][1].s;
      vtlist[0].txcoord.t = PrtList.txcoord[image][1].t;

      //vtlist[1].dcSpecular = vt[cnt].dcSpecular;
      vtlist[1].txcoord.s = PrtList.txcoord[image][0].s;
      vtlist[1].txcoord.t = PrtList.txcoord[image][1].t;

      //vtlist[2].dcSpecular = vt[cnt].dcSpecular;
      vtlist[2].txcoord.s = PrtList.txcoord[image][0].s;
      vtlist[2].txcoord.t = PrtList.txcoord[image][0].t;

      //vtlist[3].dcSpecular = vt[cnt].dcSpecular;
      vtlist[3].txcoord.s = PrtList.txcoord[image][1].s;
      vtlist[3].txcoord.t = PrtList.txcoord[image][0].t;

      glBegin(GL_TRIANGLE_FAN);
      for (i = 0; i < 4; i++)
      {
        glTexCoord2fv(vtlist[i].txcoord.vals);
        glVertex3fv(vtlist[i].pos.vals);
      }
      glEnd();

    }
  }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Render each particle that was on
  for (cnt = 0; cnt < numparticle; cnt++)
  {
    // Get the index from the color slot
    prt = (Uint16) v[cnt].icolor;

    // Draw solid and transparent sprites this round
    if (PrtList[prt].type != SPRITE_LIGHT)
    {
      // Figure out the sprite's size based on distance
      size = float(PrtList[prt].size) / float(1<<9);

      // Calculate the position of the four corners of the billboard
      // used to display the particle.
      vtlist[0].pos = v[cnt].pos + ((-vector_right - vector_up) * size);
      vtlist[1].pos = v[cnt].pos + (( vector_right - vector_up) * size);
      vtlist[2].pos = v[cnt].pos + (( vector_right + vector_up) * size);
      vtlist[3].pos = v[cnt].pos + ((-vector_right + vector_up) * size);

      // Fill in the rest of the data
      startalpha = (int)(0xFF+v[cnt].pos.z-level);
      if (startalpha < 0) startalpha = 0;
      startalpha = (startalpha|reffadeor)>>(1+PrtList[prt].type);  // Fix for Riva owners
      if (startalpha > 0xFF) startalpha = 0xFF;
      if (startalpha > 0)
      {
        float color_component = PrtList[prt].light / 16.0;
        image = ((Uint16)(PrtList[prt].image+PrtList[prt].image_stt))>>8;
        glColor4f(color_component, color_component, color_component, startalpha / float(0xFF));

        //vtlist[0].dcSpecular = vt[cnt].dcSpecular;
        vtlist[0].txcoord.s = PrtList.txcoord[image][1].s;
        vtlist[0].txcoord.t = PrtList.txcoord[image][1].t;

        //vtlist[1].dcSpecular = vt[cnt].dcSpecular;
        vtlist[1].txcoord.s = PrtList.txcoord[image][0].s;
        vtlist[1].txcoord.t = PrtList.txcoord[image][1].t;

        //vtlist[2].dcSpecular = vt[cnt].dcSpecular;
        vtlist[2].txcoord.s = PrtList.txcoord[image][0].s;
        vtlist[2].txcoord.t = PrtList.txcoord[image][0].t;

        //vtlist[3].dcSpecular = vt[cnt].dcSpecular;
        vtlist[3].txcoord.s = PrtList.txcoord[image][1].s;
        vtlist[3].txcoord.t = PrtList.txcoord[image][0].t;

        // Go on and draw it
        glBegin(GL_TRIANGLE_FAN);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2fv(vtlist[i].txcoord.vals);
          glVertex3fv(vtlist[i].pos.vals);
        }
        glEnd();
      }
    }
  }
}

