/* Egoboo - graphicmad.c
* Character model drawing code.
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
#include "Md2.h"
#include "id_md2.h"
#include "log.h"
#include <SDL_opengl.h>
#include "log.h"

/* Storage for blended vertices */
static GLfloat md2_blendedVertices[MD2_MAX_VERTICES][3];
static GLfloat md2_blendedNormals[MD2_MAX_VERTICES][3];

/* blend_md2_vertices
* Blends the vertices and normals between 2 frames of a md2 model for animation.
*
* NOTE: Only meant to be called from draw_textured_md2, which does the necessary
* checks to make sure that the inputs are valid.  So this function itself assumes
* that they are valid.  User beware!
*/
static void blend_md2_vertices(Uint8 character, const Md2Model *model, int from_, int to_, float lerp)
{
  struct Md2Frame *from, *to;
  int numVertices, i;
  Uint8 ambilevel     = ChrList[character].lightambi;
  Uint8 speklevel     = ChrList[character].lightspek;
  Uint8 sheen         = ChrList[character].sheen;
  Uint8 lightrotation = (ChrList[character].turnleftright + ChrList[character].lightturnleftright) >> 8;
  Uint16 lightold, lightnew;
  float spekularity = ((float)sheen / (float)MAXSPEKLEVEL);

  from = &model->frames[from_];
  to = &model->frames[to_];
  numVertices = model->numVertices;


  if (lerp <= 0)
  {
    // copy the vertices in frame 'from' over
    for (i = 0;i < numVertices;i++)
    {
      md2_blendedVertices[i][0] = from->vertices[i].x;
      md2_blendedVertices[i][1] = from->vertices[i].y;
      md2_blendedVertices[i][2] = from->vertices[i].z;

      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0];
      md2_blendedNormals[i][1] = kMd2Normals[from->vertices[i].normal][1];
      md2_blendedNormals[i][2] = kMd2Normals[from->vertices[i].normal][2];

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[from_].vrta[i]] + spekularity * 255 * spek_global[lightrotation][MadFrame[from_].vrta[i]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[i];
      ChrList[character].vrta[i] = 0.9 * lightold + 0.1 * lightnew;
    }
  }
  else if (lerp >= 1.0f)
  {
    // copy the vertices in frame 'to'
    for (i = 0;i < numVertices;i++)
    {
      md2_blendedVertices[i][0] = to->vertices[i].x;
      md2_blendedVertices[i][1] = to->vertices[i].y;
      md2_blendedVertices[i][2] = to->vertices[i].z;

      md2_blendedNormals[i][0] = kMd2Normals[to->vertices[i].normal][0];
      md2_blendedNormals[i][1] = kMd2Normals[to->vertices[i].normal][1];
      md2_blendedNormals[i][2] = kMd2Normals[to->vertices[i].normal][2];

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[to_].vrta[i]] + spekularity * 255 * spek_global[lightrotation][MadFrame[to_].vrta[i]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[i];
      ChrList[character].vrta[i] = 0.9 * lightold + 0.1 * lightnew;
    }
  }
  else
  {
    // mix the vertices
    for (i = 0;i < numVertices;i++)
    {
      md2_blendedVertices[i][0] = from->vertices[i].x +
                                  (to->vertices[i].x - from->vertices[i].x) * lerp;
      md2_blendedVertices[i][1] = from->vertices[i].y +
                                  (to->vertices[i].y - from->vertices[i].y) * lerp;
      md2_blendedVertices[i][2] = from->vertices[i].z +
                                  (to->vertices[i].z - from->vertices[i].z) * lerp;

      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][0] +
                                 (kMd2Normals[to->vertices[i].normal][0] - kMd2Normals[from->vertices[i].normal][0]) * lerp;
      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][1] +
                                 (kMd2Normals[to->vertices[i].normal][1] - kMd2Normals[from->vertices[i].normal][1]) * lerp;
      md2_blendedNormals[i][0] = kMd2Normals[from->vertices[i].normal][2] +
                                 (kMd2Normals[to->vertices[i].normal][2] - kMd2Normals[from->vertices[i].normal][2]) * lerp;


      lightold = MIN(255, speklevel * spek_local[lightrotation][MadFrame[from_].vrta[i]] + spekularity * 255 * spek_global[lightrotation][MadFrame[from_].vrta[i]]);
      lightold = lighttospek[sheen][lightold] + ambilevel + (lightambi * 255);

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[to_].vrta[i]] + spekularity * 255 * spek_global[lightrotation][MadFrame[to_].vrta[i]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);

      lightnew = (1 - lerp) * lightold + lerp * lightnew;
      lightold = ChrList[character].vrta[i];
      ChrList[character].vrta[i] = 0.9 * lightold + 0.1 * lightnew;
    }
  }
}

/* draw_textured_md2
* Draws a Md2Model in the new format
*/
void draw_textured_md2(Uint16 character, const Md2Model *model, int from_, int to_, float lerp)
{
  int i, numTriangles;
  const struct Md2TexCoord *tc;
  const struct Md2Triangle *triangles;
  const struct Md2Triangle *tri;

  if (model == NULL) return;
  if (from_ < 0 || from_ >= model->numFrames) return;
  if (to_ < 0 || to_ >= model->numFrames) return;

  blend_md2_vertices(character, model, from_, to_, lerp);

  numTriangles = model->numTriangles;
  tc = model->texCoords;
  triangles = model->triangles;

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);

  glVertexPointer(3, GL_FLOAT, 0, md2_blendedVertices);
  glNormalPointer(GL_FLOAT, 0, md2_blendedNormals);

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
  glDisableClientState(GL_NORMAL_ARRAY);
}

//--------------------------------------------------------------------------------------------
void render_mad_lit(Uint16 character)
{
  // ZZ> This function draws an environment mapped model

  GLVERTEX v[MAXVERTICES];
  Uint16 cnt, tnc, entry;
  Uint16 vertex;
  Sint32 temp;

  Uint8  sheen = ChrList[character].sheen;
  float          spekularity = ((float)sheen / (float)MAXSPEKLEVEL);
  Uint16 model = ChrList[character].model;
  Uint16 texture = ChrList[character].texture;
  Uint16 frame = ChrList[character].frame;
  Uint16 lastframe = ChrList[character].lastframe;
  Uint8 lip = ChrList[character].lip >> 6;
  Uint8 lightrotation = (ChrList[character].turnleftright + ChrList[character].lightturnleftright) >> 8;
  Uint8 ambilevel = ChrList[character].lightambi;
  Uint8 speklevel = ChrList[character].lightspek;


  float uoffset = textureoffset[ChrList[character].uoffset >> 8];
  float voffset = textureoffset[ChrList[character].voffset >> 8];
  Uint8 rs = ChrList[character].redshift;
  Uint8 gs = ChrList[character].grnshift;
  Uint8 bs = ChrList[character].blushift;

  float mat_none[4]     = {0, 0, 0, 0};
  float mat_emission[4] = {0, 0, 0, 0};
  float mat_diffuse[4]  = {0, 0, 0, 0};
  float mat_specular[4] = {0, 0, 0, 0};
  float shininess[1] = {2};

  float ftmp;

  // Original points with linear interpolation ( lip )
  switch (lip)
  {
  case 0:  // 25% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      temp = MadFrame[lastframe].vrtx[cnt];
      temp = temp + temp + temp;
      v[cnt].x = (float)(MadFrame[frame].vrtx[cnt] + temp >> 2);

      temp = MadFrame[lastframe].vrty[cnt];
      temp = temp + temp + temp;
      v[cnt].y = (float)(MadFrame[frame].vrty[cnt] + temp >> 2);

      temp = MadFrame[lastframe].vrtz[cnt];
      temp = temp + temp + temp;
      v[cnt].z = (float)(MadFrame[frame].vrtz[cnt] + temp >> 2);
    }
    break;

  case 1:  // 50% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      v[cnt].x = (float)(MadFrame[frame].vrtx[cnt] +
                         MadFrame[lastframe].vrtx[cnt] >> 1);
      v[cnt].y = (float)(MadFrame[frame].vrty[cnt] +
                         MadFrame[lastframe].vrty[cnt] >> 1);
      v[cnt].z = (float)(MadFrame[frame].vrtz[cnt] +
                         MadFrame[lastframe].vrtz[cnt] >> 1);
    }
    break;

  case 2:  // 75% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      temp = MadFrame[frame].vrtx[cnt];
      temp = temp + temp + temp;
      v[cnt].x = (float)(MadFrame[lastframe].vrtx[cnt] + temp >> 2);
      temp = MadFrame[frame].vrty[cnt];
      temp = temp + temp + temp;
      v[cnt].y = (float)(MadFrame[lastframe].vrty[cnt] + temp >> 2);
      temp = MadFrame[frame].vrtz[cnt];
      temp = temp + temp + temp;
      v[cnt].z = (float)(MadFrame[lastframe].vrtz[cnt] + temp >> 2);
    }
    break;

  case 3:  // 100% this frame...  This is the legible one
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      v[cnt].x = (float) MadFrame[frame].vrtx[cnt];
      v[cnt].y = (float) MadFrame[frame].vrty[cnt];
      v[cnt].z = (float) MadFrame[frame].vrtz[cnt];
    }
    break;

  }

  ftmp = ((float)(MAXSPEKLEVEL - sheen) / (float)MAXSPEKLEVEL) * ((float)ChrList[character].alpha / 255.0f);
  mat_diffuse[0] = ftmp / (float)(1 << rs) * (float)ChrList[character].alpha / 255.0f;
  mat_diffuse[1] = ftmp / (float)(1 << gs) * (float)ChrList[character].alpha / 255.0f;
  mat_diffuse[2] = ftmp / (float)(1 << bs) * (float)ChrList[character].alpha / 255.0f;


  ftmp = ((float)sheen / (float)MAXSPEKLEVEL) * ((float)ChrList[character].alpha / 255.0f);
  mat_specular[0] = ftmp / (float)(1 << rs);
  mat_specular[1] = ftmp / (float)(1 << gs);
  mat_specular[2] = ftmp / (float)(1 << bs);

  shininess[0] = sheen + 2;

  if (255 != ChrList[character].light)
  {
    ftmp = (ChrList[character].light / 255.0f);
    mat_emission[0] = ftmp / (float)(1 << rs);
    mat_emission[1] = ftmp / (float)(1 << gs);
    mat_emission[2] = ftmp / (float)(1 << bs);
  }

  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   mat_none);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_none);
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  mat_emission);

  ATTRIB_PUSH("render_mad_lit", GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
  {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(ChrList[character].matrix.v);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Choose texture and matrix
    GLTexture_Bind(&txTexture[texture], CData.texturefilter);

    // Render each command
    glColor4f(1, 1, 1, 1);
    entry = 0;
    for (cnt = 0; cnt < MadList[model].commands; cnt++)
    {
      glBegin(MadList[model].commandtype[cnt]);

      for (tnc = 0; tnc < MadList[model].commandsize[cnt]; tnc++)
      {
        vertex = MadList[model].commandvrt[entry];


        glTexCoord2f(MadList[model].commandu[entry] + uoffset, MadList[model].commandv[entry] + voffset);
        glNormal3f(kMd2Normals[MadFrame[lastframe].vrta[vertex]][0], -kMd2Normals[MadFrame[lastframe].vrta[vertex]][1], -kMd2Normals[MadFrame[lastframe].vrta[vertex]][2]);
        glVertex3fv(&v[vertex].x);

        entry++;
      }
      glEnd();
    }

    glPopMatrix();
  }
  ATTRIB_POP("render_mad_lit");

}


//--------------------------------------------------------------------------------------------
void render_enviromad(Uint16 character, Uint8 trans)
{
  // ZZ> This function draws an environment mapped model
  //D3DLVERTEX v[MAXVERTICES];
  //D3DTLVERTEX vt[MAXVERTICES];
  //D3DTLVERTEX vtlist[MAXCOMMANDSIZE];
  GLVERTEX v[MAXVERTICES];
  Uint16 cnt, tnc, entry;
  Uint16 vertex;
  Sint32 temp;
  //float z;
  //Uint8 red, grn, blu;
  //DWORD GFog.spec;

  Uint8  sheen = ChrList[character].sheen;
  float          spekularity = ((float)sheen / (float)MAXSPEKLEVEL);
  Uint16 model = ChrList[character].model;
  Uint16 texture = ChrList[character].texture;
  Uint16 frame = ChrList[character].frame;
  Uint16 lastframe = ChrList[character].lastframe;
  Uint16 framestt = MadList[ChrList[character].model].framestart;
  Uint8 lip = ChrList[character].lip >> 6;
  Uint8 lightrotation = (ChrList[character].turnleftright + ChrList[character].lightturnleftright) >> 8;
  Uint8 ambilevel = ChrList[character].lightambi;
  Uint8 speklevel = ChrList[character].lightspek;
  float uoffset = textureoffset[ChrList[character].uoffset >> 8] + GCamera.turnleftrightone;
  float voffset = textureoffset[ChrList[character].voffset >> 8];
  Uint8 rs = ChrList[character].redshift;
  Uint8 gs = ChrList[character].grnshift;
  Uint8 bs = ChrList[character].blushift;
  Uint16 lightnew, lightold;


  // Original points with linear interpolation ( lip )
  switch (lip)
  {
  case 0:  // 25% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      temp = MadFrame[lastframe].vrtx[cnt];
      temp = temp + temp + temp;
      v[cnt].x = (float)(MadFrame[frame].vrtx[cnt] + temp >> 2);
      temp = MadFrame[lastframe].vrty[cnt];
      temp = temp + temp + temp;
      v[cnt].y = (float)(MadFrame[frame].vrty[cnt] + temp >> 2);
      temp = MadFrame[lastframe].vrtz[cnt];
      temp = temp + temp + temp;
      v[cnt].z = (float)(MadFrame[frame].vrtz[cnt] + temp >> 2);


      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[frame].vrta[cnt]] + spekularity * 255 * spek_global[lightrotation][MadFrame[frame].vrta[cnt]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[cnt];
      ChrList[character].vrta[cnt] = MIN(255, 0.9 * lightold + 0.1 * lightnew);

      v[cnt].r = (float)(ChrList[character].vrta[cnt] >> rs) / 255.0f;
      v[cnt].g = (float)(ChrList[character].vrta[cnt] >> gs) / 255.0f;
      v[cnt].b = (float)(ChrList[character].vrta[cnt] >> bs) / 255.0f;
      v[cnt].a = (float)(trans) / 255.0f;
    }
    break;

  case 1:  // 50% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      v[cnt].x = (float)(MadFrame[frame].vrtx[cnt] +
                         MadFrame[lastframe].vrtx[cnt] >> 1);
      v[cnt].y = (float)(MadFrame[frame].vrty[cnt] +
                         MadFrame[lastframe].vrty[cnt] >> 1);
      v[cnt].z = (float)(MadFrame[frame].vrtz[cnt] +
                         MadFrame[lastframe].vrtz[cnt] >> 1);

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[frame].vrta[cnt]] + spekularity * 255 * spek_global[lightrotation][MadFrame[frame].vrta[cnt]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[cnt];
      ChrList[character].vrta[cnt] = MIN(255, 0.9 * lightold + 0.1 * lightnew);
      v[cnt].r = (float)(ChrList[character].vrta[cnt] >> rs) / 255.0f;
      v[cnt].g = (float)(ChrList[character].vrta[cnt] >> gs) / 255.0f;
      v[cnt].b = (float)(ChrList[character].vrta[cnt] >> bs) / 255.0f;
      v[cnt].a = (float)(trans) / 255.0f;
    }
    break;

  case 2:  // 75% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      temp = MadFrame[frame].vrtx[cnt];
      temp = temp + temp + temp;
      v[cnt].x = (float)(MadFrame[lastframe].vrtx[cnt] + temp >> 2);
      temp = MadFrame[frame].vrty[cnt];
      temp = temp + temp + temp;
      v[cnt].y = (float)(MadFrame[lastframe].vrty[cnt] + temp >> 2);
      temp = MadFrame[frame].vrtz[cnt];
      temp = temp + temp + temp;
      v[cnt].z = (float)(MadFrame[lastframe].vrtz[cnt] + temp >> 2);

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[frame].vrta[cnt]] + spekularity * 255 * spek_global[lightrotation][MadFrame[frame].vrta[cnt]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[cnt];
      ChrList[character].vrta[cnt] = MIN(255, 0.9 * lightold + 0.1 * lightnew);
      v[cnt].r = (float)(ChrList[character].vrta[cnt] >> rs) / 255.0f;
      v[cnt].g = (float)(ChrList[character].vrta[cnt] >> gs) / 255.0f;
      v[cnt].b = (float)(ChrList[character].vrta[cnt] >> bs) / 255.0f;
      v[cnt].a = (float)(trans) / 255.0f;
    }
    break;

  case 3:  // 100% this frame...  This is the legible one
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      v[cnt].x = (float) MadFrame[frame].vrtx[cnt];
      v[cnt].y = (float) MadFrame[frame].vrty[cnt];
      v[cnt].z = (float) MadFrame[frame].vrtz[cnt];

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[frame].vrta[cnt]] + spekularity * 255 * spek_global[lightrotation][MadFrame[frame].vrta[cnt]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[cnt];
      ChrList[character].vrta[cnt] = MIN(255, 0.9 * lightold + 0.1 * lightnew);
      v[cnt].a = (float)(trans) / 255.0f;
      v[cnt].r = (float)(ChrList[character].vrta[cnt] >> rs) / 255.0f;
      v[cnt].g = (float)(ChrList[character].vrta[cnt] >> gs) / 255.0f;
      v[cnt].b = (float)(ChrList[character].vrta[cnt] >> bs) / 255.0f;
    }
    break;

  }

  // Do fog...
  /*
  if(GFog.on && ChrList[character].light==255)
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

  ATTRIB_PUSH("render_enviromad", GL_TRANSFORM_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);
  {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(ChrList[character].matrix.v);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Choose texture and matrix
    GLTexture_Bind(&txTexture[texture], CData.texturefilter);

    glEnable(GL_TEXTURE_GEN_S);       // Enable Texture Coord Generation For S (NEW)
    glEnable(GL_TEXTURE_GEN_T);       // Enable Texture Coord Generation For T (NEW)

    // Render each command
    entry = 0;
    for (cnt = 0; cnt < MadList[model].commands; cnt++)
    {
      glBegin(MadList[model].commandtype[cnt]);

      for (tnc = 0; tnc < MadList[model].commandsize[cnt]; tnc++)
      {
        vertex = MadList[model].commandvrt[entry];

        glColor4fv(&v[vertex].r);
        glNormal3f(kMd2Normals[MadFrame[lastframe].vrta[vertex]][0], -kMd2Normals[MadFrame[lastframe].vrta[vertex]][1], -kMd2Normals[MadFrame[lastframe].vrta[vertex]][2]);
        glVertex3fv(&v[vertex].x);

        entry++;
      }
      glEnd();
    }

    glPopMatrix();
  }
  ATTRIB_POP("render_enviromad");
}




//--------------------------------------------------------------------------------------------
void render_texmad(Uint16 character, Uint8 trans)
{
  // ZZ> This function draws a model

  GLVERTEX v[MAXVERTICES];
  Uint16 cnt, tnc, entry;
  Uint16 vertex;
  Sint32 temp;


  Uint8  sheen = ChrList[character].sheen;
  float          spekularity = ((float)sheen / (float)MAXSPEKLEVEL);
  Uint16 model = ChrList[character].model;
  Uint16 texture = ChrList[character].texture;
  Uint16 frame = ChrList[character].frame;
  Uint16 lastframe = ChrList[character].lastframe;
  Uint8 lip = ChrList[character].lip >> 6;
  Uint8 lightrotation = (ChrList[character].turnleftright + ChrList[character].lightturnleftright) >> 8;
  Uint8 ambilevel = ChrList[character].lightambi;
  Uint8 speklevel = ChrList[character].lightspek;



  float uoffset = textureoffset[ChrList[character].uoffset >> 8];
  float voffset = textureoffset[ChrList[character].voffset >> 8];
  Uint8 rs = ChrList[character].redshift;
  Uint8 gs = ChrList[character].grnshift;
  Uint8 bs = ChrList[character].blushift;
  Uint16 lightold, lightnew;

  // Original points with linear interpolation ( lip )
  switch (lip)
  {
  case 0:  // 25% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      temp = MadFrame[lastframe].vrtx[cnt];
      temp = temp + temp + temp;
      v[cnt].x = (float)(MadFrame[frame].vrtx[cnt] + temp >> 2);

      temp = MadFrame[lastframe].vrty[cnt];
      temp = temp + temp + temp;
      v[cnt].y = (float)(MadFrame[frame].vrty[cnt] + temp >> 2);

      temp = MadFrame[lastframe].vrtz[cnt];
      temp = temp + temp + temp;
      v[cnt].z = (float)(MadFrame[frame].vrtz[cnt] + temp >> 2);

      v[cnt].nx = kMd2Normals[MadFrame[frame].vrta[cnt]][0]*0.25 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][0]*0.75;
      v[cnt].ny = -(kMd2Normals[MadFrame[frame].vrta[cnt]][1]*0.25 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][1]*0.75);
      v[cnt].nz = -(kMd2Normals[MadFrame[frame].vrta[cnt]][2]*0.25 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][2]*0.75);


      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[frame].vrta[cnt]] + spekularity * 255 * spek_global[lightrotation][MadFrame[frame].vrta[cnt]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[cnt];
      ChrList[character].vrta[cnt] = MIN(255, 0.9 * lightold + 0.1 * lightnew);

      v[cnt].r = (float)(ChrList[character].vrta[cnt] >> rs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].g = (float)(ChrList[character].vrta[cnt] >> gs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].b = (float)(ChrList[character].vrta[cnt] >> bs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].a = 1.0f;
    }
    break;

  case 1:  // 50% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      v[cnt].x = (float)(MadFrame[frame].vrtx[cnt] +
                         MadFrame[lastframe].vrtx[cnt] >> 1);
      v[cnt].y = (float)(MadFrame[frame].vrty[cnt] +
                         MadFrame[lastframe].vrty[cnt] >> 1);
      v[cnt].z = (float)(MadFrame[frame].vrtz[cnt] +
                         MadFrame[lastframe].vrtz[cnt] >> 1);

      v[cnt].nx = kMd2Normals[MadFrame[frame].vrta[cnt]][0]*0.5 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][0]*0.5;
      v[cnt].ny = -(kMd2Normals[MadFrame[frame].vrta[cnt]][1]*0.5 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][1]*0.5);
      v[cnt].nz = -(kMd2Normals[MadFrame[frame].vrta[cnt]][2]*0.5 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][2]*0.5);

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[frame].vrta[cnt]] + spekularity * 255 * spek_global[lightrotation][MadFrame[frame].vrta[cnt]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[cnt];
      ChrList[character].vrta[cnt] = MIN(255, 0.9 * lightold + 0.1 * lightnew);

      v[cnt].r = (float)(ChrList[character].vrta[cnt] >> rs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].g = (float)(ChrList[character].vrta[cnt] >> gs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].b = (float)(ChrList[character].vrta[cnt] >> bs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].a = 1.0f;
    }
    break;

  case 2:  // 75% this frame
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      temp = MadFrame[frame].vrtx[cnt];
      temp = temp + temp + temp;
      v[cnt].x = (float)(MadFrame[lastframe].vrtx[cnt] + temp >> 2);
      temp = MadFrame[frame].vrty[cnt];
      temp = temp + temp + temp;
      v[cnt].y = (float)(MadFrame[lastframe].vrty[cnt] + temp >> 2);
      temp = MadFrame[frame].vrtz[cnt];
      temp = temp + temp + temp;
      v[cnt].z = (float)(MadFrame[lastframe].vrtz[cnt] + temp >> 2);

      v[cnt].nx = kMd2Normals[MadFrame[frame].vrta[cnt]][0]*0.75 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][0]*0.25;
      v[cnt].ny = -(kMd2Normals[MadFrame[frame].vrta[cnt]][1]*0.75 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][1]*0.25);
      v[cnt].nz = -(kMd2Normals[MadFrame[frame].vrta[cnt]][2]*0.75 + kMd2Normals[MadFrame[lastframe].vrta[cnt]][2]*0.25);

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[frame].vrta[cnt]] + spekularity * 255 * spek_global[lightrotation][MadFrame[frame].vrta[cnt]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[cnt];
      ChrList[character].vrta[cnt] = MIN(255, 0.9 * lightold + 0.1 * lightnew);

      v[cnt].r = (float)(ChrList[character].vrta[cnt] >> rs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].g = (float)(ChrList[character].vrta[cnt] >> gs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].b = (float)(ChrList[character].vrta[cnt] >> bs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].a = 1.0f;
    }
    break;

  case 3:  // 100% this frame...  This is the legible one
    for (cnt = 0; cnt < MadList[model].transvertices; cnt++)
    {
      v[cnt].x = (float) MadFrame[frame].vrtx[cnt];
      v[cnt].y = (float) MadFrame[frame].vrty[cnt];
      v[cnt].z = (float) MadFrame[frame].vrtz[cnt];

      v[cnt].nx = kMd2Normals[MadFrame[frame].vrta[cnt]][0];
      v[cnt].ny = -kMd2Normals[MadFrame[frame].vrta[cnt]][1];
      v[cnt].nz = -kMd2Normals[MadFrame[frame].vrta[cnt]][2];

      lightnew = MIN(255, speklevel * spek_local[lightrotation][MadFrame[frame].vrta[cnt]] + spekularity * 255 * spek_global[lightrotation][MadFrame[frame].vrta[cnt]]);
      lightnew = lighttospek[sheen][lightnew] + ambilevel + (lightambi*255);
      lightold = ChrList[character].vrta[cnt];
      ChrList[character].vrta[cnt] = MIN(255, 0.9 * lightold + 0.1 * lightnew);

      v[cnt].r = (float)(ChrList[character].vrta[cnt] >> rs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].g = (float)(ChrList[character].vrta[cnt] >> gs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].b = (float)(ChrList[character].vrta[cnt] >> bs) / 255.0f * (float)(trans) / 255.0f;
      v[cnt].a = 1.0f;
    }
    break;

  }

  /*
  // Do fog...
  if(GFog.on && ChrList[character].light==255)
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
  spek = v[cnt].specular & 255;
  z = (z - GFog.bottom)/GFog.distance;  // 0.0 to 1.0...  Amount of old to keep
  GFog.tokeep = 1.0-z;  // 0.0 to 1.0...  Amount of fog to keep
  spek = spek * z;
  red = (GFog.red * GFog.tokeep) + spek;
  grn = (GFog.grn * GFog.tokeep) + spek;
  blu = (GFog.blu * GFog.tokeep) + spek;
  GFog.spec = 0xff000000 | (red<<16) | (grn<<8) | (blu);
  v[cnt].specular = GFog.spec;
  }
  }
  }
  }
  */

  ATTRIB_PUSH("render_texmad", GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT);
  {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixf(ChrList[character].matrix.v);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Choose texture and matrix
    GLTexture_Bind(&txTexture[texture], CData.texturefilter);

    // Render each command
    entry = 0;
    for (cnt = 0; cnt < MadList[model].commands; cnt++)
    {
      glBegin(MadList[model].commandtype[cnt]);

      for (tnc = 0; tnc < MadList[model].commandsize[cnt]; tnc++)
      {
        vertex = MadList[model].commandvrt[entry];

        glColor4fv(&v[vertex].r);
        glTexCoord2f(MadList[model].commandu[entry] + uoffset, MadList[model].commandv[entry] + voffset);
        glVertex3fv(&v[vertex].x);
        glNormal3fv(&v[vertex].nx);

        entry++;
      }
      glEnd();
    }

    glPopMatrix();
  }
  ATTRIB_POP("render_texmad");

}

//--------------------------------------------------------------------------------------------
void render_mad(Uint16 character, Uint8 trans)
{
  // ZZ> This function picks the actual function to use
  Sint8 hide = CapList[ChrList[character].model].hidestate;

  if (hide == NOHIDE || hide != ChrList[character].aistate)
  {
    if (ChrList[character].enviro)
      render_enviromad(character, trans);
    else
      render_texmad(character, trans);
  }
}

//--------------------------------------------------------------------------------------------
void render_refmad(int tnc, Uint8 trans)
{
  // ZZ> This function draws characters reflected in the floor
  if (CapList[ChrList[tnc].model].reflect)
  {
    int alphatemp;
    int level = ChrList[tnc].level;
    int zpos = (ChrList[tnc].matrix)_CNV(3, 2) - level;
    int model = ChrList[tnc].model;
    Uint16 lastframe = ChrList[tnc].lastframe;

    alphatemp = trans - (zpos >> 1);
    if (alphatemp < 0) alphatemp = 0;
    if (alphatemp > 255) alphatemp = 255;

    if (alphatemp > 0)
    {
      Uint8 sheensave = ChrList[tnc].sheen;
      ChrList[tnc].redshift += 1;
      ChrList[tnc].grnshift += 1;
      ChrList[tnc].blushift += 1;
      ChrList[tnc].sheen = ChrList[tnc].sheen >> 1;
      (ChrList[tnc].matrix)_CNV(0, 2) = -(ChrList[tnc].matrix)_CNV(0, 2);
      (ChrList[tnc].matrix)_CNV(1, 2) = -(ChrList[tnc].matrix)_CNV(1, 2);
      (ChrList[tnc].matrix)_CNV(2, 2) = -(ChrList[tnc].matrix)_CNV(2, 2);
      (ChrList[tnc].matrix)_CNV(3, 2) = -(ChrList[tnc].matrix)_CNV(3, 2) + level + level;
      zpos = GFog.on;
      GFog.on = bfalse;

      render_mad(tnc, alphatemp);

      GFog.on = zpos;
      (ChrList[tnc].matrix)_CNV(0, 2) = -(ChrList[tnc].matrix)_CNV(0, 2);
      (ChrList[tnc].matrix)_CNV(1, 2) = -(ChrList[tnc].matrix)_CNV(1, 2);
      (ChrList[tnc].matrix)_CNV(2, 2) = -(ChrList[tnc].matrix)_CNV(2, 2);
      (ChrList[tnc].matrix)_CNV(3, 2) = -(ChrList[tnc].matrix)_CNV(3, 2) + level + level;
      ChrList[tnc].sheen = sheensave;
      ChrList[tnc].redshift -= 1;
      ChrList[tnc].grnshift -= 1;
      ChrList[tnc].blushift -= 1;

    }


  }
}

//#if 0
////--------------------------------------------------------------------------------------------
//void render_texmad(Uint16 character, Uint8 trans)
//{
//  Md2Model *model;
//  Uint16 texture;
//  int frame0, frame1;
//  float lerp;
//  GLMATRIX mTempWorld;
//
//  // Grab some basic information about the model
//  model = md2_Models[ChrList[character].model];
//  texture = ChrList[character].texture;
//  frame0 = ChrList[character].frame;
//  frame1 = ChrList[character].frame;
//  lerp = (float)ChrList[character].lip / 256.0f;
//
//  mTempWorld = mWorld;
//  /*
//  // Lighting information
//  Uint8 lightrotation =
//  (ChrList[character].turnleftright+ChrList[character].lightturnleftright)>>8;
//  Uint8 speklevel = ChrList[character].lightspek;
//  Uint32 alpha = trans<<24;
//  Uint8 spek = ChrList[character].sheen;
//
//  float uoffset = textureoffset[ChrList[character].uoffset>>8];
//  float voffset = textureoffset[ChrList[character].voffset>>8];
//  Uint8 rs = ChrList[character].redshift;
//  Uint8 gs = ChrList[character].grnshift;
//  Uint8 bs = ChrList[character].blushift;
//
//
//  if(CData.phongon && trans == 255)
//  spek = 0;
//  */
//
//  // Choose texture and matrix
//  if(KEYDOWN(SDLK_F7))
//  {
//    glBindTexture( GL_TEXTURE_2D, -1 );
//  }
//  else
//  {
//    GLTexture_Bind( &txTexture[texture], CData.texturefilter );
//  }
//
//  mWorld = ChrList[character].matrix;
//
//  glLoadMatrixf(mView.v);
//  glMultMatrixf(mWorld.v);
//
//  draw_textured_md2(character, model, frame0, frame1, lerp);
//
//
//  mWorld = mTempWorld;
//  glLoadMatrixf(mView.v);
//  glMultMatrixf(mWorld.v);
//}
//#endif