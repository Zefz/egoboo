// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "egoboo.h"
#include "MD2_file.h"
#include "Mad.h"
#include "character.h"
#include "Input.h"
#include "Camera.h"
#include <SDL_opengl.h>

//---------------------------------------------------------------------------------------------
//// Labels for cached blend data
//static Uint16  md2_blendedmodel  = Mad_List::INVALID;
//static Uint32  md2_blendedframe0 = 0;
//static Uint32  md2_blendedframe1 = 0;
//static Uint32  md2_blendedvrtmin = 0;
//static Uint32  md2_blendedvrtmax = 0;
//static float   md2_blendedlerp   = 0.0f;
//
//// Storage for blended vertices
//int     md2_blendedCount;
//GLfloat md2_blendedVertices[MD2_MAX_VERTICES][3];
//GLfloat md2_blendedNormals[MD2_MAX_VERTICES][3];
//GLfloat md2_blendedColors[MD2_MAX_VERTICES][4];
//GLfloat md2_blendedTexture[MD2_MAX_VERTICES][2];

//---------------------------------------------------------------------------------------------
// blend_md2_vertices
// Blends the vertices and normals between 2 frames of a md2 model for animation.
//
// NOTE: Only meant to be called from draw_textured_md2, which does the necessary
// checks to make sure that the inputs are valid.  So this function itself assumes
// that they are valid.  User beware!
//
void blend_md2_vertices(Character & rchr, Uint32 vrtmin, Uint32 vrtmax)
{
  const JF::MD2_Frame *from, *to;
  Uint32 numVertices, i;

  // check to see if the blend is already cached
  if( (rchr.ani.last  == rchr.md2_blended.frame0) && 
      (rchr.ani.frame == rchr.md2_blended.frame1) && 
      (vrtmin         == rchr.md2_blended.vrtmin) &&
      (vrtmax         == rchr.md2_blended.vrtmax) &&
      (rchr.ani.lip/float(0x0100) + rchr.ani.flip == rchr.md2_blended.lerp))
      return;

  JF::MD2_Model * model = rchr.getMD2();
  from = rchr.getFrameLast(model);
  to   = rchr.getFrame(model);
  float lerp = rchr.ani.lip/float(0x0100) + rchr.ani.flip;
  CLIP(lerp,0,1);

  vec2_t off = rchr.off;

  // do some error trapping
  if(NULL==from && NULL==to) return;
  else if(NULL==from) lerp = 1.0f;
  else if(NULL==to  ) lerp = 0.0f;
  else if(from==to  ) lerp = 0.0f;

  // do the interpolating
  numVertices = model->numVertices();

  if (lerp <= 0)
  {
    // copy the vertices in frame 'from' over
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      rchr.md2_blended.Vertices[i].x = -from->vertices[i].x;
      rchr.md2_blended.Vertices[i].y =  from->vertices[i].y;
      rchr.md2_blended.Vertices[i].z =  from->vertices[i].z;

      if(shading != GL_FLAT)
      {
        rchr.md2_blended.Normals[i].x =  kMd2Normals[from->vertices[i].normal][0];
        rchr.md2_blended.Normals[i].y = -kMd2Normals[from->vertices[i].normal][1];
        rchr.md2_blended.Normals[i].z = -kMd2Normals[from->vertices[i].normal][2];
      }
      

      if(rchr.enviro)
      {
        rchr.md2_blended.Texture[i].s  = off.u + CLIP((1-rchr.md2_blended.Normals[i].z)/2.0f,0,1);
        rchr.md2_blended.Texture[i].t  = off.v + atan_tab.lookup(rchr.md2_blended.Normals[i].x, rchr.md2_blended.Normals[i].y)*INV_TWO_PI - rchr.turn_lr/float(1<<16);
        rchr.md2_blended.Texture[i].t -= atan_tab.lookup(rchr.pos.x-GCamera.pos.x, rchr.pos.y-GCamera.pos.y)*INV_TWO_PI;
      };
    }
  }
  else if (lerp >= 1.0f)
  {
    // copy the vertices in frame 'to'
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      rchr.md2_blended.Vertices[i].x = -to->vertices[i].x;
      rchr.md2_blended.Vertices[i].y = to->vertices[i].y;
      rchr.md2_blended.Vertices[i].z = to->vertices[i].z;

      if(shading != GL_FLAT)
      {
        rchr.md2_blended.Normals[i].x = kMd2Normals[to->vertices[i].normal][0];
        rchr.md2_blended.Normals[i].y = -kMd2Normals[to->vertices[i].normal][1];
        rchr.md2_blended.Normals[i].z = -kMd2Normals[to->vertices[i].normal][2];
      }

      if(rchr.enviro)
      {
        rchr.md2_blended.Texture[i].s  = off.u + CLIP((1-rchr.md2_blended.Normals[i].z)/2.0f,0,1);
        rchr.md2_blended.Texture[i].t  = off.v + atan_tab.lookup(rchr.md2_blended.Normals[i].x, rchr.md2_blended.Normals[i].y)*INV_TWO_PI - rchr.turn_lr/float(1<<16);
        rchr.md2_blended.Texture[i].t -= atan_tab.lookup(rchr.pos.x-GCamera.pos.x, rchr.pos.y-GCamera.pos.y)*INV_TWO_PI;
      };
    }
  }
  else
  {
    // mix the vertices
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      rchr.md2_blended.Vertices[i].x = from->vertices[i].x + (to->vertices[i].x - from->vertices[i].x) * lerp;
      rchr.md2_blended.Vertices[i].y = from->vertices[i].y + (to->vertices[i].y - from->vertices[i].y) * lerp;
      rchr.md2_blended.Vertices[i].z = from->vertices[i].z + (to->vertices[i].z - from->vertices[i].z) * lerp;

      rchr.md2_blended.Vertices[i].x *= -1;

      if(shading != GL_FLAT)
      {
        rchr.md2_blended.Normals[i].x = kMd2Normals[from->vertices[i].normal][0] + (kMd2Normals[to->vertices[i].normal][0] - kMd2Normals[from->vertices[i].normal][0]) * lerp;
        rchr.md2_blended.Normals[i].y = kMd2Normals[from->vertices[i].normal][1] + (kMd2Normals[to->vertices[i].normal][1] - kMd2Normals[from->vertices[i].normal][1]) * lerp;
        rchr.md2_blended.Normals[i].z = kMd2Normals[from->vertices[i].normal][2] + (kMd2Normals[to->vertices[i].normal][2] - kMd2Normals[from->vertices[i].normal][2]) * lerp;

        rchr.md2_blended.Normals[i].y  *= -1;
        rchr.md2_blended.Normals[i].z  *= -1;
      }

      if(rchr.enviro)
      {
        rchr.md2_blended.Texture[i].s  = off.u + CLIP((1-rchr.md2_blended.Normals[i].z)/2.0f,0,1);
        rchr.md2_blended.Texture[i].t  = off.v + atan_tab.lookup(rchr.md2_blended.Normals[i].x, rchr.md2_blended.Normals[i].y)*INV_TWO_PI - rchr.turn_lr/float(1<<16);
        rchr.md2_blended.Texture[i].t -= atan_tab.lookup(rchr.pos.x-GCamera.pos.x, rchr.pos.y-GCamera.pos.y)*INV_TWO_PI;
      };
    }
  }


  // cache the blend parameters
  rchr.md2_blended.frame0 = rchr.ani.last;
  rchr.md2_blended.frame1 = rchr.ani.frame;
  rchr.md2_blended.vrtmin = vrtmin;
  rchr.md2_blended.vrtmax = vrtmax;
  rchr.md2_blended.lerp   = rchr.ani.lip/float(0x0100) + rchr.ani.flip;

  assert(rchr.md2_blended.frame1>=0 && rchr.md2_blended.frame1<0xFFFF);
}

//---------------------------------------------------------------------------------------------
void blend_md2_lighting(Character & rchr)
{
  JF::MD2_Model * model = rchr.getMD2();
  if(NULL==model) return;

  //rotate the lx,ly vector in the opposite direction of the character
  float lit_x = rchr.light_x*cos_tab[rchr.turn_lr] - rchr.light_y*sin_tab[rchr.turn_lr];
  float lit_y = rchr.light_x*sin_tab[rchr.turn_lr] + rchr.light_y*cos_tab[rchr.turn_lr];

  Uint32 i, numVertices = model->numVertices();
  if(shading != GL_FLAT)
  {
    // mix the vertices
    for (i=0; i<numVertices; i++)
    {
      float lit = rchr.light_a + MAX(0, lit_x*rchr.md2_blended.Normals[i].x) + MAX(0, lit_y*rchr.md2_blended.Normals[i].y);
      lit /= float(0xFF);

      lit += WaterList.la * (1 + MAX(0, WaterList.lx*rchr.md2_blended.Normals[i].x) + MAX(0, WaterList.ly*rchr.md2_blended.Normals[i].y) +  MAX(0, WaterList.lz*rchr.md2_blended.Normals[i].z));
      lit = CLIP(lit,0,1);

      lit = lighttospek[rchr.sheen][int(lit*0xFF)]/float(0x0100);
      rchr.md2_blended.Colors[i].r  = rchr.md2_blended.Colors[i].r*0.9 + 0.1*lit;
      rchr.md2_blended.Colors[i].g  = rchr.md2_blended.Colors[i].g*0.9 + 0.1*lit;
      rchr.md2_blended.Colors[i].b  = rchr.md2_blended.Colors[i].b*0.9 + 0.1*lit;
      rchr.md2_blended.Colors[i].a  = rchr.md2_blended.Colors[i].a*0.9 + 1.0f;
    }
  }
  else
  {
    for (i=0; i<numVertices; i++)
    {
      rchr.md2_blended.Colors[i].r  =
      rchr.md2_blended.Colors[i].g  = 
      rchr.md2_blended.Colors[i].b  = rchr.light_a / float(0xFF);
      rchr.md2_blended.Colors[i].a  = 1.0;
    };

  };

}

//---------------------------------------------------------------------------------------------
// Draws a JF::MD2_Model in the new format
//
void draw_textured_md2(Character & chr)
{
  int i, numTriangles;
  const struct JF::MD2_TexCoord *tc;
  const struct JF::MD2_Triangle *triangles;
  const struct JF::MD2_Triangle *tri;

  JF::MD2_Model * model = chr.getMD2();

  blend_md2_vertices(chr);

  numTriangles = model->numTriangles();
  tc           = model->getTexCoords();
  triangles    = model->getTriangles();

  if(shading != GL_FLAT)
  {
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(   GL_FLOAT, 0, chr.md2_blended.Normals[0].vals);
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, chr.md2_blended.Vertices[0].vals);

  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer (4, GL_FLOAT, 0, chr.md2_blended.Colors[0].vals);

  glBegin(GL_TRIANGLES);
  {
    for (i = 0;i < numTriangles;i++)
    {
      tri = &triangles[i];

      glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[0]]));
      glArrayElement(tri->vertexIndices[0]);

      glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[1]]));
      glArrayElement(tri->vertexIndices[1]);

      glTexCoord2fv((const GLfloat*)&(tc[tri->texCoordIndices[2]]));
      glArrayElement(tri->vertexIndices[2]);
    }
  }
  glEnd();

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);

  if(shading != GL_FLAT) glDisableClientState(GL_NORMAL_ARRAY);
}


//--------------------------------------------------------------------------------------------
// Draws a JF::MD2_Model in the new format
// using OpenGL commands in the MD2 for acceleration
void draw_textured_md2_opengl(Character & rchr)
{
  Uint32 cmd_count;
  const struct JF::MD2_GLCommand * cmd;

  JF::MD2_Model * model = rchr.getMD2();

  vec2_t off = rchr.off;

  blend_md2_vertices(rchr);

  if(shading != GL_FLAT)
  {
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(   GL_FLOAT, 0, rchr.md2_blended.Normals[0].vals);
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, rchr.md2_blended.Vertices[0].vals);

  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer (4, GL_FLOAT, 0, rchr.md2_blended.Colors[0].vals);

  cmd       = model->getCommands();
  for (/*nothing */; NULL!=cmd; cmd = cmd->next)
  {
    glBegin(cmd->gl_mode);

    cmd_count = cmd->command_count;
    for(Uint32 i=0; i<cmd_count; i++)
    {
      glTexCoord2f( cmd->data[i].s + off.u, cmd->data[i].t + off.v );
      glArrayElement( cmd->data[i].index );
    }
    glEnd();
  }

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);

  if(shading != GL_FLAT) glDisableClientState(GL_NORMAL_ARRAY);
}

//--------------------------------------------------------------------------------------------
// Draws a JF::MD2_Model in the new format
// using OpenGL commands in the MD2 for acceleration
void draw_enviromapped_md2_opengl(Character & rchr)
{
  Uint32 cmd_count;
  const struct JF::MD2_GLCommand * cmd;

  JF::MD2_Model * model = rchr.getMD2();

  blend_md2_vertices(rchr);

  if(shading != GL_FLAT)
  {
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(   GL_FLOAT, 0, rchr.md2_blended.Normals[0].vals);
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, rchr.md2_blended.Vertices[0].vals);

  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer (4, GL_FLOAT, 0, rchr.md2_blended.Colors[0].vals);

  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, 0, rchr.md2_blended.Texture[0].vals);

  cmd = model->getCommands();
  for (/*nothing */; NULL!=cmd; cmd = cmd->next)
  {
    glBegin(cmd->gl_mode);

    cmd_count = cmd->command_count;
    for(Uint32 i=0; i<cmd_count; i++)
    {
      glArrayElement( cmd->data[i].index );
    }
    glEnd();
  }

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  if(shading != GL_FLAT) glDisableClientState(GL_NORMAL_ARRAY);
}

//--------------------------------------------------------------------------------------------
void render_texmad(Uint16 character, Uint8 trans)
{
  Locker_3DMode loc_locker_3d;

  if(INVALID_CHR(character)) return;

  Character & rchr = ChrList[character];

  // Grab some basic information about the model

  loc_locker_3d.begin(GCamera);

  // Choose texture and matrix
  if (GKeyb.pressed(SDLK_F7))
  {
    glBindTexture(GL_TEXTURE_2D, -1);
  }
  else
  {
    TxList[rchr.texture].Bind(GL_TEXTURE_2D);
  }

  glPushAttrib(GL_TRANSFORM_BIT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(rchr.matrix.v);

    glColor4f(1<<rchr.redshift, 1<<rchr.grnshift, 1<<rchr.blushift, rchr.alpha/float(0xFF));

    draw_textured_md2_opengl(rchr);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  glPopAttrib();
}


//--------------------------------------------------------------------------------------------
void render_enviromad(Uint16 character, Uint8 trans)
{
  Locker_3DMode loc_locker_3d;

  if(INVALID_CHR(character)) return;

  Character & rchr = ChrList[character];

  // Grab some basic information about the model

  loc_locker_3d.begin(GCamera);

  // Choose texture and matrix
  if (GKeyb.pressed(SDLK_F7))
  {
    glBindTexture(GL_TEXTURE_2D, -1);
  }
  else
  {
    TxList[rchr.texture].Bind(GL_TEXTURE_2D);
  }

  glPushAttrib(GL_TRANSFORM_BIT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(rchr.matrix.v);

    glColor4f(1<<rchr.redshift, 1<<rchr.grnshift, 1<<rchr.blushift, rchr.alpha/float(0xFF));

    draw_enviromapped_md2_opengl(rchr);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  glPopAttrib();
}

#if 0
////--------------------------------------------------------------------------------------------
//void render_enviromad(Uint16 character, Uint8 trans)
//{
//  // ZZ> This function draws an environment mapped model
//  Uint16 tnc, entry;
//  Uint16 vertex;
//  Uint8 ambi;
//  GLMatrix tempWorld = GCamera.mWorld;
//
//  TEX_REF texture = ChrList[character].texture;
//
//  float off_u = textureoffset[ChrList[character].off_u>>FIXEDPOINT_BITS] - (GCamera.turn_lr/sin_tab.size);
//  float off_v = textureoffset[ChrList[character].off_v>>FIXEDPOINT_BITS];
//
//  Uint8 rs = ChrList[character].redshift;
//  Uint8 gs = ChrList[character].grnshift;
//  Uint8 bs = ChrList[character].blushift;
//
//  GCamera.mWorld = ChrList[character].matrix;
//  // GS - Begin3DMode ();
//
//  glLoadMatrixf(GCamera.mView.v);
//  glMultMatrixf(GCamera.mWorld.v);
//
//  // Choose texture and matrix
//  TxList[texture].Bind(GL_TEXTURE_2D);
//
//  // Render each command
//  entry = 0;
//  JF::MD2_Model     * mdl = ChrList[character].getMD2();
//  JF::MD2_GLCommand * cmd = (JF::MD2_GLCommand *)mdl->getCommands();
//  for ( /* nothing */ ; NULL!=cmd; cmd =cmd->next)
//  {
//    glBegin(cmd->gl_mode);
//
//    for (tnc = 0; tnc < cmd->command_count; tnc++)
//    {
//      vertex = cmd->data[tnc].index;
//
//      glTexCoord2f( md2_blendedNormals[vertex][0] + off_u,
//                    md2_blendedNormals[vertex][1] + off_v);
//
//
//      glVertex3fv(md2_blendedVertices[vertex]);
//    }
//    glEnd();
//  }
//
//  GCamera.mWorld = tempWorld;
//  glLoadMatrixf(GCamera.mView.v);
//  glMultMatrixf(GCamera.mWorld.v);
//  // GS - End3DMode ();
//}

// !!!!!! DO THE FOG AS A SEPARATE PASS !!!!!!
  // Do fog...
  /*
  if(GFog.on && ChrList[character].light==0xFF)
  {
      // The full fog value
      alpha = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);

      for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
      {
          // Figure out the z position of the vertex...  Not totally accurate
          z = (v[cnt].z * ChrList[character].scale) + ChrList[character].matrix(3,2);

          // Figure out the fog coloring
          if(z < GFog.top)
          {
              if(z < GFog.bottom)
              {
                  v[cnt].specular = alpha;
              }
              else
              {
                  z = 1.0 - ((z - GFog.bottom)/GFog.distance);  // 0.0 to 1.0...  Amount of fog to keep
                  red = GFog.red * z;
                  grn = GFog.grn * z;
                  blu = GFog.blu * z;
                  GFog.spec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
                  v[cnt].specular = GFog.spec;
              }
          }
          else
          {
              v[cnt].specular = 0;
          }
      }
  }
  else
  {
      for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
          v[cnt].specular = 0;
  }
  */
//
//
//--------------------------------------------------------------------------------------------
//void render_texmad(Uint16 character, Uint8 trans)
//{
//  // ZZ> This function draws a model
////    D3DLVERTEX v[MAXVERTICES];
////    D3DTLVERTEX vt[MAXVERTICES];
////    D3DTLVERTEX vtlist[MAXCOMMANDSIZE];
//  GLVertex v[MAXVERTICES];
//  Uint16 cnt, tnc, entry;
//  Uint16 vertex;
//  Sint32 temp;
//// float z, GFog.tokeep;
//// Uint8 red, grn, blu;
//  Uint8 ambi;
//// DWORD GFog.spec;
//
//  // To make life easier
//  Uint16 model = ChrList[character].model;
//  Uint16 texture = ChrList[character].texture;
//  Uint16 frame = ChrList[character].ani.frame;
//  Uint16 frame_last = ChrList[character].ani.last;
//  Uint8 lip = ChrList[character].ani.lip>>6;
//  Uint8 lightrotation =
//    (ChrList[character].turn_lr+ChrList[character].light_turn_lr)>>8;
//  Uint8 light_level = ChrList[character].light_level>>4;
//  Uint32 alpha = trans<<24;
//  Uint8 spek = ChrList[character].sheen;
//
//  float off_u = textureoffset[ChrList[character].off_u>>8];
//  float off_v = textureoffset[ChrList[character].off_v>>8];
//  Uint8 rs = ChrList[character].redshift;
//  Uint8 gs = ChrList[character].grnshift;
//  Uint8 bs = ChrList[character].blushift;
//  GLMatrix mTempWorld = GCamera.mWorld;
//
//  if (phongon && trans == 0xFF)
//    spek = 0;
//
//  // Original points with linear interpolation ( lip )
//  switch (lip)
//  {
//    case 0:  // 25% this frame
//      for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
//      {
//        temp = FrameList[frame_last].vrt_x[cnt];
//        temp = temp+temp+temp;
//        //v[cnt].x = (D3DVALUE) (FrameList[frame].vrt_x[cnt] + temp>>2);
//        v[cnt].x = (float)(FrameList[frame].vrt_x[cnt] + temp>>2);
//        temp = FrameList[frame_last].vrt_y[cnt];
//        temp = temp+temp+temp;
//        //v[cnt].y = (D3DVALUE) (FrameList[frame].vrt_y[cnt] + temp>>2);
//        v[cnt].y = (float)(FrameList[frame].vrt_y[cnt] + temp>>2);
//        temp = FrameList[frame_last].vrt_z[cnt];
//        temp = temp+temp+temp;
//        //v[cnt].z = (D3DVALUE) (FrameList[frame].vrt_z[cnt] + temp>>2);
//        v[cnt].z = (float)(FrameList[frame].vrt_z[cnt] + temp>>2);
//
//        ambi = ChrList[character].vrt_a[cnt];
//        ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[light_level][lightrotation][FrameList[frame].vrt_a[cnt]]>>3;
//        ChrList[character].vrt_a[cnt] = ambi;
//        //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
//        v[cnt].r = (float)(ambi>>rs) / float(0xFF);
//        v[cnt].g = (float)(ambi>>gs) / float(0xFF);
//        v[cnt].b = (float)(ambi>>bs) / float(0xFF);
//        v[cnt].a = (float) alpha / float(0xFF);
//
//        //v[cnt].specular = lighttospek[spek][ambi];
//
//        //v[cnt].dwReserved = 0;
//      }
//      break;
//    case 1:  // 50% this frame
//      for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
//      {
//        /*
//                      v[cnt].x = (D3DVALUE) (FrameList[frame].vrt_x[cnt] +
//                                             FrameList[frame_last].vrt_x[cnt]>>1);
//                      v[cnt].y = (D3DVALUE) (FrameList[frame].vrt_y[cnt] +
//                                             FrameList[frame_last].vrt_y[cnt]>>1);
//                      v[cnt].z = (D3DVALUE) (FrameList[frame].vrt_z[cnt] +
//                                             FrameList[frame_last].vrt_z[cnt]>>1);
//        */
//        v[cnt].x = (float)(FrameList[frame].vrt_x[cnt] +
//                           FrameList[frame_last].vrt_x[cnt]>>1);
//        v[cnt].y = (float)(FrameList[frame].vrt_y[cnt] +
//                           FrameList[frame_last].vrt_y[cnt]>>1);
//        v[cnt].z = (float)(FrameList[frame].vrt_z[cnt] +
//                           FrameList[frame_last].vrt_z[cnt]>>1);
//
//        ambi = ChrList[character].vrt_a[cnt];
//        ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[light_level][lightrotation][FrameList[frame].vrt_a[cnt]]>>3;
//        ChrList[character].vrt_a[cnt] = ambi;
//        //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
//        v[cnt].r = (float)(ambi>>rs) / float(0xFF);
//        v[cnt].g = (float)(ambi>>gs) / float(0xFF);
//        v[cnt].b = (float)(ambi>>bs) / float(0xFF);
//        v[cnt].a = (float) alpha  / float(0xFF);
//
//        //v[cnt].specular = lighttospek[spek][ambi];
//
//        //v[cnt].dwReserved = 0;
//      }
//      break;
//    case 2:  // 75% this frame
//      for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
//      {
//        temp = FrameList[frame].vrt_x[cnt];
//        temp = temp+temp+temp;
//        //v[cnt].x = (D3DVALUE) (FrameList[frame_last].vrt_x[cnt] + temp>>2);
//        v[cnt].x = (float)(FrameList[frame_last].vrt_x[cnt] + temp>>2);
//        temp = FrameList[frame].vrt_y[cnt];
//        temp = temp+temp+temp;
//        //v[cnt].y = (D3DVALUE) (FrameList[frame_last].vrt_y[cnt] + temp>>2);
//        v[cnt].y = (float)(FrameList[frame_last].vrt_y[cnt] + temp>>2);
//        temp = FrameList[frame].vrt_z[cnt];
//        temp = temp+temp+temp;
//        //v[cnt].z = (D3DVALUE) (FrameList[frame_last].vrt_z[cnt] + temp>>2);
//        v[cnt].z = (float)(FrameList[frame_last].vrt_z[cnt] + temp>>2);
//
//        ambi = ChrList[character].vrt_a[cnt];
//        ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[light_level][lightrotation][FrameList[frame].vrt_a[cnt]]>>3;
//        ChrList[character].vrt_a[cnt] = ambi;
//        //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
//        v[cnt].r = (float)(ambi>>rs) / float(0xFF);
//        v[cnt].g = (float)(ambi>>gs) / float(0xFF);
//        v[cnt].b = (float)(ambi>>bs) / float(0xFF);
//        v[cnt].a = (float) alpha / float(0xFF);
//
//        //v[cnt].specular = lighttospek[spek][ambi];
//
//        //v[cnt].dwReserved = 0;
//      }
//      break;
//    case 3:  // 100% this frame...  This is the legible one
//      for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
//      {
//        /*
//                      v[cnt].x = (D3DVALUE) FrameList[frame].vrt_x[cnt];
//                      v[cnt].y = (D3DVALUE) FrameList[frame].vrt_y[cnt];
//                      v[cnt].z = (D3DVALUE) FrameList[frame].vrt_z[cnt];
//        */
//        v[cnt].x = (float) FrameList[frame].vrt_x[cnt];
//        v[cnt].y = (float) FrameList[frame].vrt_y[cnt];
//        v[cnt].z = (float) FrameList[frame].vrt_z[cnt];
//
//        ambi = ChrList[character].vrt_a[cnt];
//        ambi = (ambi+ambi+ambi<<1)+ambi+lighttable[light_level][lightrotation][FrameList[frame].vrt_a[cnt]]>>3;
//        ChrList[character].vrt_a[cnt] = ambi;
//        //v[cnt].color = alpha | ((ambi>>rs)<<16) | ((ambi>>gs)<<8) | ((ambi>>bs));
//        v[cnt].r = (float)(ambi>>rs) / float(0xFF);
//        v[cnt].g = (float)(ambi>>gs) / float(0xFF);
//        v[cnt].b = (float)(ambi>>bs) / float(0xFF);
//        v[cnt].a = (float) alpha / float(0xFF);
//
//        //v[cnt].specular = lighttospek[spek][ambi];
//
//        //v[cnt].dwReserved = 0;
//      }
//      break;
//  }
//
//  /*
//      // Do fog...
//      if(GFog.on && ChrList[character].light==0xFF)
//      {
//          // The full fog value
//          alpha = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);
//
//          for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
//          {
//              // Figure out the z position of the vertex...  Not totally accurate
//              z = (v[cnt].z * ChrList[character].scale) + ChrList[character].matrix(3,2);
//
//              // Figure out the fog coloring
//              if(z < GFog.top)
//              {
//                  if(z < GFog.bottom)
//                  {
//                      v[cnt].specular = alpha;
//                  }
//                  else
//                  {
//                      spek = v[cnt].specular & 0xFF;
//                      z = (z - GFog.bottom)/GFog.distance;  // 0.0 to 1.0...  Amount of old to keep
//                      GFog.tokeep = 1.0-z;  // 0.0 to 1.0...  Amount of fog to keep
//                      spek = spek * z;
//                      red = (GFog.red * GFog.tokeep) + spek;
//                      grn = (GFog.grn * GFog.tokeep) + spek;
//                      blu = (GFog.blu * GFog.tokeep) + spek;
//                      GFog.spec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
//                      v[cnt].specular = GFog.spec;
//                  }
//              }
//          }
//      }
//  */
//  // Choose texture and matrix
//  if (GKeyb.pressed(SDLK_F7))
//  {
//    //lpD3DDDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, NULL);
//    TxList[texture].Bind(GL_TEXTURE_2D);
//  }
//  else
//  {
//    //lpD3DDDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, TxList[texture].GetHandle());
//    TxList[texture].Bind(GL_TEXTURE_2D);
//  }
//
//  //lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &ChrList[character].matrix);
//  GCamera.mWorld = ChrList[character].matrix;
//
//  //Begin3DMode();
//  glLoadMatrixf(GCamera.mView.v);
//  glMultMatrixf(GCamera.mWorld.v);
//
//  // Make new ones so we can index them and not transform 'em each time
//// if(transform_vertices(MadList[model].transvertices, v, vt))
//  //      return;
//
//  // Render each command
//  entry = 0;
//
//  for (cnt = 0; cnt < MadList[model].commands; cnt++)
//  {
//    glBegin(MadList[model].commandtype[cnt]);
//    for (tnc = 0; tnc < MadList[model].commandsize[cnt]; tnc++)
//    {
//      vertex = MadList[model].commandvrt[entry];
//      glColor4fv(&v[vertex].r);
//      glTexCoord2f(MadList[model].commandu[entry]+off_u, MadList[model].commandv[entry]+off_v);
//      glVertex3fv(&v[vertex].x);
//      /*
//             vtlist[tnc].dvSX = vt[vertex].dvSX;
//             vtlist[tnc].dvSY = vt[vertex].dvSY;
//             vtlist[tnc].dvSZ = (vt[vertex].dvSZ);
//             vtlist[tnc].dvRHW = vt[vertex].dvRHW;
//             vtlist[tnc].dcColor = vt[vertex].dcColor;
//             vtlist[tnc].dcSpecular = vt[vertex].dcSpecular;
//             vtlist[tnc].dvTU = MadList[model].commandu[entry]+off_u;
//             vtlist[tnc].dvTV = MadList[model].commandv[entry]+off_v;
//      */
//      entry++;
//    }
//    glEnd();
//    //lpD3DDDevice->DrawPrimitive((D3DPRIMITIVETYPE) MadList[model].commandtype[cnt],
//    //                            D3DVT_TLVERTEX, (LPVOID)vtlist, tnc, NULL);
//  }
//  //End3DMode ();
//
//  GCamera.mWorld = mTempWorld;
//  glLoadMatrixf(GCamera.mView.v);
//  glMultMatrixf(GCamera.mWorld.v);
//}
//
//
#endif

//--------------------------------------------------------------------------------------------
void render_mad(Uint16 character, Uint8 trans)
{
  // ZZ> This function picks the actual function to use
  Sint8 hide = ChrList[character].getCap().hide_state;

  if (hide == NOHIDE || hide != ChrList[character].ai.state)
  {
    // blend the vertices
    blend_md2_vertices(ChrList[character]);
    light_character(ChrList[character], GMesh);
    blend_md2_lighting(ChrList[character]);

    // do separate render passes on the same blended data
    
    if(ChrList[character].enviro)
      render_enviromad(character, trans);
    else
      render_texmad(character, trans);

//  if(GFog.on)
//    render_fogmad(character, trans);
  }
}

//--------------------------------------------------------------------------------------------
void render_refmad(int tnc, Uint8 trans)
{
  // ZZ> This function draws characters reflected in the floor
  if (ChrList[tnc].getCap().reflect)
  {
    int level = ChrList[tnc].level;
    int alphatemp = trans;
    int pos_z = ChrList[tnc].matrix.CNV(3,2)-level;
    alphatemp -= pos_z>>1;

    if (alphatemp < 0) alphatemp = 0;

    alphatemp = alphatemp|reffadeor;  // Fix for Riva owners

    if (alphatemp > 0xFF) alphatemp = 0xFF;

    if (alphatemp > 0)
    {
      Uint8 sheensave = ChrList[tnc].sheen;
      ChrList[tnc].redshift+=1;
      ChrList[tnc].grnshift+=1;
      ChrList[tnc].blushift+=1;
      ChrList[tnc].sheen = ChrList[tnc].sheen>>1;
      ChrList[tnc].matrix.CNV(0,2) = -ChrList[tnc].matrix.CNV(0,2);
      ChrList[tnc].matrix.CNV(1,2) = -ChrList[tnc].matrix.CNV(1,2);
      ChrList[tnc].matrix.CNV(2,2) = -ChrList[tnc].matrix.CNV(2,2);
      ChrList[tnc].matrix.CNV(3,2) = -ChrList[tnc].matrix.CNV(3,2)+level+level;
      pos_z = GFog.on;
      GFog.on = false;
      render_mad(tnc, alphatemp);
      GFog.on = pos_z;
      ChrList[tnc].matrix.CNV(0,2) = -ChrList[tnc].matrix.CNV(0,2);
      ChrList[tnc].matrix.CNV(1,2) = -ChrList[tnc].matrix.CNV(1,2);
      ChrList[tnc].matrix.CNV(2,2) = -ChrList[tnc].matrix.CNV(2,2);
      ChrList[tnc].matrix.CNV(3,2) = -ChrList[tnc].matrix.CNV(3,2)+level+level;
      ChrList[tnc].sheen = sheensave;
      ChrList[tnc].redshift-=1;
      ChrList[tnc].grnshift-=1;
      ChrList[tnc].blushift-=1;
    }
  }
}
