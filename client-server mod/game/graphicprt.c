/* Egoboo - graphicprc.c
* Particle system drawing and management code.
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
#include "Log.h"
#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern void Begin3DMode();

GLint prt_attrib_open;
GLint prt_attrib_close;

//--------------------------------------------------------------------------------------------
void render_antialias_prt(Uint32 vrtcount, GLVERTEX * vrtlist)
{
  GLVERTEX vtlist[4];
  GLVECTOR vector_right, vector_up;
  GLVECTOR loc_vector_right, loc_vector_up;
  Uint16 cnt, prt;
  Uint16 image;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  int i;

  ATTRIB_PUSH("render_antialias_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    glDepthMask(GL_FALSE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render each particle that was on
    cnt = 0;
    while (cnt < vrtcount)
    {
      // Get the index from the color slot
      prt = (Uint16) vrtlist[cnt].color;

      // Draw transparent sprites this round
      if (PrtList[prt].type == PRTSOLIDSPRITE) // Render solid ones twice...  For Antialias
      {

        float color_component = PrtList[prt].light / 255.0;
        float alpha_component = antialiastrans / 255.0;

        // Figure out the sprite's size based on distance
        size = (float)PrtList[prt].size / (float)(1 << 10) * 1.1; // [claforte] Fudge the value.


        if (!PipList[PrtList[prt].pip].rotatetoface)
        {
          GLVECTOR vec1, vec2;

          vec1.x = 0;
          vec1.y = 0;
          vec1.z = 1;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vector_right = Normalize(CrossProduct(vec1, vec2));
          vector_up    = Normalize(CrossProduct(vector_right, vec2));

          //vector_right.x = mView.v[0];
          //vector_right.y = mView.v[4];
          //vector_right.z = mView.v[8];

          //vector_up.x = mView.v[1];
          //vector_up.y = mView.v[5];
          //vector_up.z = mView.v[9];

          rotate = (PrtList[prt].rotate - 24576) >> 2;
        }
        else
        {
          GLVECTOR vec_vel, vec1, vec2, vec3;

          vec_vel.x = PrtList[prt].xvel;
          vec_vel.y = PrtList[prt].yvel;
          vec_vel.z = PrtList[prt].zvel;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vec1 = Normalize(vec_vel);
          vec3 = CrossProduct(vec1, Normalize(vec2));

          vector_right = vec3;
          vector_up    = vec1;

          rotate = (PrtList[prt].rotate - 8192) >> 2;
        };

        sinsize = turntosin[rotate & TRIGTABLE_MASK];
        cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK];

        loc_vector_right.x = cossize * vector_right.x - sinsize * vector_up.x;
        loc_vector_right.y = cossize * vector_right.y - sinsize * vector_up.y;
        loc_vector_right.z = cossize * vector_right.z - sinsize * vector_up.z;

        loc_vector_up.x    = sinsize * vector_right.x + cossize * vector_up.x;
        loc_vector_up.y    = sinsize * vector_right.y + cossize * vector_up.y;
        loc_vector_up.z    = sinsize * vector_right.z + cossize * vector_up.z;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].x = vrtlist[cnt].x + ((-loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[0].y = vrtlist[cnt].y + ((-loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[0].z = vrtlist[cnt].z + ((-loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[1].x = vrtlist[cnt].x + ((loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[1].y = vrtlist[cnt].y + ((loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[1].z = vrtlist[cnt].z + ((loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[2].x = vrtlist[cnt].x + ((loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[2].y = vrtlist[cnt].y + ((loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[2].z = vrtlist[cnt].z + ((loc_vector_right.z + loc_vector_up.z) * size);

        vtlist[3].x = vrtlist[cnt].x + ((-loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[3].y = vrtlist[cnt].y + ((-loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[3].z = vrtlist[cnt].z + ((-loc_vector_right.z + loc_vector_up.z) * size);

        // Fill in the rest of the data
        image = (Uint16)((PrtList[prt].image + PrtList[prt].imagestt) >> 8);

        vtlist[0].s = CALCULATE_PRT_U0(image);
        vtlist[0].t = CALCULATE_PRT_V0(image);

        vtlist[1].s = CALCULATE_PRT_U1(image);
        vtlist[1].t = CALCULATE_PRT_V0(image);

        vtlist[2].s = CALCULATE_PRT_U1(image);
        vtlist[2].t = CALCULATE_PRT_V1(image);

        vtlist[3].s = CALCULATE_PRT_U0(image);
        vtlist[3].t = CALCULATE_PRT_V1(image);

        //[claforte] should use alpha_component instead of 0.5?
        glColor4f(color_component, color_component, color_component, alpha_component);

        // Go on and draw it
        glBegin(GL_TRIANGLE_FAN);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2f(vtlist[i].s, vtlist[i].t);
          glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
        }
        glEnd();

      }
      cnt++;
    }
  }
  ATTRIB_POP("render_antialias_prt");
};

//--------------------------------------------------------------------------------------------
void render_solid_prt(Uint32 vrtcount, GLVERTEX * vrtlist)
{
  GLVERTEX vtlist[4];
  GLVECTOR vector_right, vector_up;
  GLVECTOR loc_vector_right, loc_vector_up;
  Uint16 cnt, prt;
  Uint16 image;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  int i;

  ATTRIB_PUSH("render_solid_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    // Render each particle that was on
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    cnt = 0;
    while (cnt < vrtcount)
    {
      // Get the index from the color slot
      prt = (Uint16) vrtlist[cnt].color;

      // Draw sprites this round
      if (PrtList[prt].type == PRTSOLIDSPRITE)
      {
        float color_component = PrtList[prt].light / 255.0;
        glColor4f(color_component, color_component, color_component, 1.0);

        // [claforte] Fudge the value.
        size = (float)(PrtList[prt].size) / (float)(1 << 10);

        if (!PipList[PrtList[prt].pip].rotatetoface)
        {
          GLVECTOR vec1, vec2;

          vec1.x = 0;
          vec1.y = 0;
          vec1.z = 1;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vector_right = Normalize(CrossProduct(vec1, vec2));
          vector_up    = Normalize(CrossProduct(vector_right, vec2));

          //vector_right.x = mView.v[0];
          //vector_right.y = mView.v[4];
          //vector_right.z = mView.v[8];

          //vector_up.x = mView.v[1];
          //vector_up.y = mView.v[5];
          //vector_up.z = mView.v[9];

          rotate = (PrtList[prt].rotate - 24576) >> 2;
        }
        else
        {
          GLVECTOR vec_vel, vec1, vec2, vec3;

          vec_vel.x = PrtList[prt].xvel;
          vec_vel.y = PrtList[prt].yvel;
          vec_vel.z = PrtList[prt].zvel;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vec1 = Normalize(vec_vel);
          vec3 = CrossProduct(vec1, Normalize(vec2));

          vector_right = vec3;
          vector_up    = vec1;

          rotate = (PrtList[prt].rotate - 8192) >> 2;
        };

        sinsize = turntosin[rotate & TRIGTABLE_MASK];
        cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK];

        loc_vector_right.x = cossize * vector_right.x - sinsize * vector_up.x;
        loc_vector_right.y = cossize * vector_right.y - sinsize * vector_up.y;
        loc_vector_right.z = cossize * vector_right.z - sinsize * vector_up.z;

        loc_vector_up.x    = sinsize * vector_right.x + cossize * vector_up.x;
        loc_vector_up.y    = sinsize * vector_right.y + cossize * vector_up.y;
        loc_vector_up.z    = sinsize * vector_right.z + cossize * vector_up.z;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].x = vrtlist[cnt].x + ((-loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[0].y = vrtlist[cnt].y + ((-loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[0].z = vrtlist[cnt].z + ((-loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[1].x = vrtlist[cnt].x + ((loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[1].y = vrtlist[cnt].y + ((loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[1].z = vrtlist[cnt].z + ((loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[2].x = vrtlist[cnt].x + ((loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[2].y = vrtlist[cnt].y + ((loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[2].z = vrtlist[cnt].z + ((loc_vector_right.z + loc_vector_up.z) * size);

        vtlist[3].x = vrtlist[cnt].x + ((-loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[3].y = vrtlist[cnt].y + ((-loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[3].z = vrtlist[cnt].z + ((-loc_vector_right.z + loc_vector_up.z) * size);

        // Fill in the rest of the data
        image = (Uint16)((PrtList[prt].image + PrtList[prt].imagestt) >> 8);

        vtlist[0].s = CALCULATE_PRT_U0(image);
        vtlist[0].t = CALCULATE_PRT_V0(image);

        vtlist[1].s = CALCULATE_PRT_U1(image);
        vtlist[1].t = CALCULATE_PRT_V0(image);

        vtlist[2].s = CALCULATE_PRT_U1(image);
        vtlist[2].t = CALCULATE_PRT_V1(image);

        vtlist[3].s = CALCULATE_PRT_U0(image);
        vtlist[3].t = CALCULATE_PRT_V1(image);

        glBegin(GL_TRIANGLE_FAN);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2f(vtlist[i].s, vtlist[i].t);
          glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
        }
        glEnd();
      }
      cnt++;
    }
  }
  glPopAttrib();
};
//--------------------------------------------------------------------------------------------
void render_transparent_prt(Uint32 vrtcount, GLVERTEX * vrtlist)
{
  GLVERTEX vtlist[4];
  GLVECTOR vector_right, vector_up;
  GLVECTOR loc_vector_right, loc_vector_up;
  Uint16 cnt, prt;
  Uint16 image;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  int i;

  ATTRIB_PUSH("render_transparent_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    cnt = 0;
    while (cnt < vrtcount)
    {
      // Get the index from the color slot
      prt = (Uint16) vrtlist[cnt].color;

      // Draw transparent sprites this round
      if (PrtList[prt].type == PRTALPHASPRITE)
      {
        float color_component = PrtList[prt].light / 255.0;
        float alpha_component = particletrans / 255.0;

        // Figure out the sprite's size based on distance
        size = (float)PrtList[prt].size / (float)(1 << 10); // [claforte] Fudge the value.


        if (!PipList[PrtList[prt].pip].rotatetoface)
        {
          GLVECTOR vec1, vec2;

          vec1.x = 0;
          vec1.y = 0;
          vec1.z = 1;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vector_right = Normalize(CrossProduct(vec1, vec2));
          vector_up    = Normalize(CrossProduct(vector_right, vec2));

          //vector_right.x = mView.v[0];
          //vector_right.y = mView.v[4];
          //vector_right.z = mView.v[8];

          //vector_up.x = mView.v[1];
          //vector_up.y = mView.v[5];
          //vector_up.z = mView.v[9];

          rotate = (PrtList[prt].rotate - 24576) >> 2;
        }
        else
        {
          GLVECTOR vec_vel, vec1, vec2, vec3;

          vec_vel.x = PrtList[prt].xvel;
          vec_vel.y = PrtList[prt].yvel;
          vec_vel.z = PrtList[prt].zvel;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vec1 = Normalize(vec_vel);
          vec3 = CrossProduct(vec1, Normalize(vec2));

          vector_right = vec3;
          vector_up    = vec1;

          rotate = (PrtList[prt].rotate - 8192) >> 2;
        };

        sinsize = turntosin[rotate & TRIGTABLE_MASK];
        cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK];

        loc_vector_right.x = cossize * vector_right.x - sinsize * vector_up.x;
        loc_vector_right.y = cossize * vector_right.y - sinsize * vector_up.y;
        loc_vector_right.z = cossize * vector_right.z - sinsize * vector_up.z;

        loc_vector_up.x    = sinsize * vector_right.x + cossize * vector_up.x;
        loc_vector_up.y    = sinsize * vector_right.y + cossize * vector_up.y;
        loc_vector_up.z    = sinsize * vector_right.z + cossize * vector_up.z;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].x = vrtlist[cnt].x + ((-loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[0].y = vrtlist[cnt].y + ((-loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[0].z = vrtlist[cnt].z + ((-loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[1].x = vrtlist[cnt].x + ((loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[1].y = vrtlist[cnt].y + ((loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[1].z = vrtlist[cnt].z + ((loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[2].x = vrtlist[cnt].x + ((loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[2].y = vrtlist[cnt].y + ((loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[2].z = vrtlist[cnt].z + ((loc_vector_right.z + loc_vector_up.z) * size);

        vtlist[3].x = vrtlist[cnt].x + ((-loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[3].y = vrtlist[cnt].y + ((-loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[3].z = vrtlist[cnt].z + ((-loc_vector_right.z + loc_vector_up.z) * size);

        // Fill in the rest of the data
        image = (Uint16)((PrtList[prt].image + PrtList[prt].imagestt) >> 8);

        vtlist[0].s = CALCULATE_PRT_U0(image);
        vtlist[0].t = CALCULATE_PRT_V0(image);

        vtlist[1].s = CALCULATE_PRT_U1(image);
        vtlist[1].t = CALCULATE_PRT_V0(image);

        vtlist[2].s = CALCULATE_PRT_U1(image);
        vtlist[2].t = CALCULATE_PRT_V1(image);

        vtlist[3].s = CALCULATE_PRT_U0(image);
        vtlist[3].t = CALCULATE_PRT_V1(image);

        // Go on and draw it
        glBegin(GL_TRIANGLE_FAN);
        //[claforte] should use alpha_component instead of 0.5?
        glColor4f(color_component, color_component, color_component, alpha_component);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2f(vtlist[i].s, vtlist[i].t);
          glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
        }
        glEnd();
      }
      cnt++;
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_light_prt(Uint32 vrtcount, GLVERTEX * vrtlist)
{
  GLVERTEX vtlist[4];
  GLVECTOR vector_right, vector_up;
  GLVECTOR loc_vector_right, loc_vector_up;
  Uint16 cnt, prt;
  Uint16 image;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  int i;

  ATTRIB_PUSH("render_light_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    glDepthMask(GL_FALSE);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);



    cnt = 0;
    while (cnt < vrtcount)
    {
      // Get the index from the color slot
      prt = (Uint16) vrtlist[cnt].color;

      // Draw lights this round
      if (PrtList[prt].type == PRTLIGHTSPRITE)
      {
        // [claforte] Fudge the value.
        size = (float)PrtList[prt].size / (float)(1 << 9);

        if (!PipList[PrtList[prt].pip].rotatetoface)
        {
          GLVECTOR vec1, vec2;

          vec1.x = 0;
          vec1.y = 0;
          vec1.z = 1;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vector_right = Normalize(CrossProduct(vec1, vec2));
          vector_up    = Normalize(CrossProduct(vector_right, vec2));

          //vector_right.x = vmatrix.v[0];
          //vector_right.y = vmatrix.v[4];
          //vector_right.z = vmatrix.v[8];

          //vector_up.x = vmatrix.v[1];
          //vector_up.y = vmatrix.v[5];
          //vector_up.z = vmatrix.v[9];

          rotate = (PrtList[prt].rotate - 24576) >> 2;
        }
        else
        {
          GLVECTOR vec_vel, vec1, vec2, vec3;

          vec_vel.x = PrtList[prt].xvel;
          vec_vel.y = PrtList[prt].yvel;
          vec_vel.z = PrtList[prt].zvel;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vec1 = Normalize(vec_vel);
          vec3 = CrossProduct(vec1, Normalize(vec2));

          vector_right = vec3;
          vector_up    = vec1;

          rotate = (PrtList[prt].rotate - 8192) >> 2;
        };

        sinsize = turntosin[rotate & TRIGTABLE_MASK];
        cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK];

        loc_vector_right.x = cossize * vector_right.x - sinsize * vector_up.x;
        loc_vector_right.y = cossize * vector_right.y - sinsize * vector_up.y;
        loc_vector_right.z = cossize * vector_right.z - sinsize * vector_up.z;

        loc_vector_up.x    = sinsize * vector_right.x + cossize * vector_up.x;
        loc_vector_up.y    = sinsize * vector_right.y + cossize * vector_up.y;
        loc_vector_up.z    = sinsize * vector_right.z + cossize * vector_up.z;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].x = vrtlist[cnt].x + ((-loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[0].y = vrtlist[cnt].y + ((-loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[0].z = vrtlist[cnt].z + ((-loc_vector_right.z - loc_vector_up.z) * size);
        vtlist[1].x = vrtlist[cnt].x + ((loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[1].y = vrtlist[cnt].y + ((loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[1].z = vrtlist[cnt].z + ((loc_vector_right.z - loc_vector_up.z) * size);
        vtlist[2].x = vrtlist[cnt].x + ((loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[2].y = vrtlist[cnt].y + ((loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[2].z = vrtlist[cnt].z + ((loc_vector_right.z + loc_vector_up.z) * size);
        vtlist[3].x = vrtlist[cnt].x + ((-loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[3].y = vrtlist[cnt].y + ((-loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[3].z = vrtlist[cnt].z + ((-loc_vector_right.z + loc_vector_up.z) * size);


        // Fill in the rest of the data
        image = (Uint16)((PrtList[prt].image + PrtList[prt].imagestt) >> 8);

        vtlist[0].s = CALCULATE_PRT_U0(image);
        vtlist[0].t = CALCULATE_PRT_V0(image);

        vtlist[1].s = CALCULATE_PRT_U1(image);
        vtlist[1].t = CALCULATE_PRT_V0(image);

        vtlist[2].s = CALCULATE_PRT_U1(image);
        vtlist[2].t = CALCULATE_PRT_V1(image);

        vtlist[3].s = CALCULATE_PRT_U0(image);
        vtlist[3].t = CALCULATE_PRT_V1(image);

        // Go on and draw it
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0, 1.0, 1.0, 1.0);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2f(vtlist[i].s, vtlist[i].t);
          glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
        }
        glEnd();
      }
      cnt++;
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_particles()
{
  // ZZ> This function draws the sprites for particle systems

  GLVERTEX v[MAXPRT];
  Uint16 cnt, numparticle;

  if (INVALID_TEXTURE == GLTexture_GetTextureID(&txTexture[particletexture]))
    return;
  prttexw = txTexture[particletexture].imgW;
  prttexh = txTexture[particletexture].imgH;
  prttexwscale = (float)txTexture[particletexture].imgW / (float)txTexture[particletexture].txW;
  prttexhscale = (float)txTexture[particletexture].imgH / (float)txTexture[particletexture].txH;

  // Original points
  numparticle = 0;
  for (cnt = 0; cnt < MAXPRT; cnt++)
  {
    if (PrtList[cnt].inview && PrtList[cnt].size != 0)
    {
      v[numparticle].x = (float) PrtList[cnt].xpos;
      v[numparticle].y = (float) PrtList[cnt].ypos;
      v[numparticle].z = (float) PrtList[cnt].zpos;

      // [claforte] Aaron did a horrible hack here. Fix that ASAP.
      v[numparticle].color = cnt;  // Store an index in the color slot...
      numparticle++;
    }
  }

  ATTRIB_PUSH("render_particles", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
  {
    // Flat shade these babies
    glShadeModel(CData.shading);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DITHER);

    // Choose texture
    GLTexture_Bind(&txTexture[particletexture], CData.texturefilter);

    // DO ANTIALIAS SOLID SPRITES FIRST
    render_antialias_prt(numparticle, v);

    // DO SOLID SPRITES FIRST
    render_solid_prt(numparticle, v);

    // LIGHTS DONE LAST
    render_light_prt(numparticle, v);

    // DO TRANSPARENT SPRITES NEXT
    render_transparent_prt(numparticle, v);
  }
  glPopAttrib();

};

//--------------------------------------------------------------------------------------------
void render_antialias_prt_ref(Uint32 vrtcount, GLVERTEX * vrtlist)
{
  GLVERTEX vtlist[4];
  GLVECTOR vector_right, vector_up;
  GLVECTOR loc_vector_right, loc_vector_up;
  Uint16 cnt, prt;
  Uint16 image;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  int i;

  ATTRIB_PUSH("render_antialias_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    glDepthMask(GL_FALSE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render each particle that was on
    cnt = 0;
    while (cnt < vrtcount)
    {
      // Get the index from the color slot
      prt = (Uint16) vrtlist[cnt].color;

      // Draw transparent sprites this round
      if (PrtList[prt].type == PRTSOLIDSPRITE) // Render solid ones twice...  For Antialias
      {

        float color_component = PrtList[prt].light / 255.0;
        float alpha_component = antialiastrans / 255.0;

        // Figure out the sprite's size based on distance
        size = (float)PrtList[prt].size / (float)(1 << 10) * 1.1; // [claforte] Fudge the value.


        if (!PipList[PrtList[prt].pip].rotatetoface)
        {
          GLVECTOR vec1, vec2;

          vec1.x = 0;
          vec1.y = 0;
          vec1.z = 1;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vector_right = Normalize(CrossProduct(vec1, vec2));
          vector_up    = Normalize(CrossProduct(vector_right, vec2));

          //vector_right.x = mView.v[0];
          //vector_right.y = mView.v[4];
          //vector_right.z = mView.v[8];

          //vector_up.x = mView.v[1];
          //vector_up.y = mView.v[5];
          //vector_up.z = mView.v[9];

          rotate = (PrtList[prt].rotate - 24576) >> 2;
        }
        else
        {
          GLVECTOR vec_vel, vec1, vec2, vec3;

          vec_vel.x = PrtList[prt].xvel;
          vec_vel.y = PrtList[prt].yvel;
          vec_vel.z = PrtList[prt].zvel;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vec1 = Normalize(vec_vel);
          vec3 = CrossProduct(vec1, Normalize(vec2));

          vector_right = vec3;
          vector_up    = vec1;

          rotate = (PrtList[prt].rotate - 8192) >> 2;
        };

        sinsize = turntosin[rotate & TRIGTABLE_MASK];
        cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK];

        loc_vector_right.x = cossize * vector_right.x - sinsize * vector_up.x;
        loc_vector_right.y = cossize * vector_right.y - sinsize * vector_up.y;
        loc_vector_right.z = cossize * vector_right.z - sinsize * vector_up.z;

        loc_vector_up.x    = sinsize * vector_right.x + cossize * vector_up.x;
        loc_vector_up.y    = sinsize * vector_right.y + cossize * vector_up.y;
        loc_vector_up.z    = sinsize * vector_right.z + cossize * vector_up.z;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].x = vrtlist[cnt].x + ((-loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[0].y = vrtlist[cnt].y + ((-loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[0].z = vrtlist[cnt].z + ((-loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[1].x = vrtlist[cnt].x + ((loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[1].y = vrtlist[cnt].y + ((loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[1].z = vrtlist[cnt].z + ((loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[2].x = vrtlist[cnt].x + ((loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[2].y = vrtlist[cnt].y + ((loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[2].z = vrtlist[cnt].z + ((loc_vector_right.z + loc_vector_up.z) * size);

        vtlist[3].x = vrtlist[cnt].x + ((-loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[3].y = vrtlist[cnt].y + ((-loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[3].z = vrtlist[cnt].z + ((-loc_vector_right.z + loc_vector_up.z) * size);

        // Fill in the rest of the data
        image = (Uint16)((PrtList[prt].image + PrtList[prt].imagestt) >> 8);

        vtlist[0].s = CALCULATE_PRT_U0(image);
        vtlist[0].t = CALCULATE_PRT_V0(image);

        vtlist[1].s = CALCULATE_PRT_U1(image);
        vtlist[1].t = CALCULATE_PRT_V0(image);

        vtlist[2].s = CALCULATE_PRT_U1(image);
        vtlist[2].t = CALCULATE_PRT_V1(image);

        vtlist[3].s = CALCULATE_PRT_U0(image);
        vtlist[3].t = CALCULATE_PRT_V1(image);

        // Go on and draw it
        glBegin(GL_TRIANGLE_FAN);
        //[claforte] should use alpha_component instead of 0.5?
        glColor4f(color_component, color_component, color_component, alpha_component);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2f(vtlist[i].s, vtlist[i].t);
          glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
        }
        glEnd();
      }
      cnt++;
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_solid_prt_ref(Uint32 vrtcount, GLVERTEX * vrtlist)
{
  GLVERTEX vtlist[4];
  GLVECTOR vector_right, vector_up;
  GLVECTOR loc_vector_right, loc_vector_up;
  Uint16 cnt, prt;
  Uint16 image;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  int i;

  ATTRIB_PUSH("render_solid_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    // Render each particle that was on
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    cnt = 0;
    while (cnt < vrtcount)
    {
      // Get the index from the color slot
      prt = (Uint16) vrtlist[cnt].color;

      // Draw sprites this round
      if (PrtList[prt].type == PRTSOLIDSPRITE)
      {
        float color_component = PrtList[prt].light / 255.0;
        glColor4f(color_component, color_component, color_component, 1.0);

        // [claforte] Fudge the value.
        size = (float)(PrtList[prt].size) / (float)(1 << 10);

        if (!PipList[PrtList[prt].pip].rotatetoface)
        {
          GLVECTOR vec1, vec2;

          vec1.x = 0;
          vec1.y = 0;
          vec1.z = 1;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vector_right = Normalize(CrossProduct(vec1, vec2));
          vector_up    = Normalize(CrossProduct(vector_right, vec2));

          //vector_right.x = mView.v[0];
          //vector_right.y = mView.v[4];
          //vector_right.z = mView.v[8];

          //vector_up.x = mView.v[1];
          //vector_up.y = mView.v[5];
          //vector_up.z = mView.v[9];

          rotate = (PrtList[prt].rotate - 24576) >> 2;
        }
        else
        {
          GLVECTOR vec_vel, vec1, vec2, vec3;

          vec_vel.x = PrtList[prt].xvel;
          vec_vel.y = PrtList[prt].yvel;
          vec_vel.z = PrtList[prt].zvel;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vec1 = Normalize(vec_vel);
          vec3 = CrossProduct(vec1, Normalize(vec2));

          vector_right = vec3;
          vector_up    = vec1;

          rotate = (PrtList[prt].rotate - 8192) >> 2;
        };

        sinsize = turntosin[rotate & TRIGTABLE_MASK];
        cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK];

        loc_vector_right.x = cossize * vector_right.x - sinsize * vector_up.x;
        loc_vector_right.y = cossize * vector_right.y - sinsize * vector_up.y;
        loc_vector_right.z = cossize * vector_right.z - sinsize * vector_up.z;

        loc_vector_up.x    = sinsize * vector_right.x + cossize * vector_up.x;
        loc_vector_up.y    = sinsize * vector_right.y + cossize * vector_up.y;
        loc_vector_up.z    = sinsize * vector_right.z + cossize * vector_up.z;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].x = vrtlist[cnt].x + ((-loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[0].y = vrtlist[cnt].y + ((-loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[0].z = vrtlist[cnt].z + ((-loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[1].x = vrtlist[cnt].x + ((loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[1].y = vrtlist[cnt].y + ((loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[1].z = vrtlist[cnt].z + ((loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[2].x = vrtlist[cnt].x + ((loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[2].y = vrtlist[cnt].y + ((loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[2].z = vrtlist[cnt].z + ((loc_vector_right.z + loc_vector_up.z) * size);

        vtlist[3].x = vrtlist[cnt].x + ((-loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[3].y = vrtlist[cnt].y + ((-loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[3].z = vrtlist[cnt].z + ((-loc_vector_right.z + loc_vector_up.z) * size);

        // Fill in the rest of the data
        image = (Uint16)((PrtList[prt].image + PrtList[prt].imagestt) >> 8);

        vtlist[0].s = CALCULATE_PRT_U0(image);
        vtlist[0].t = CALCULATE_PRT_V0(image);

        vtlist[1].s = CALCULATE_PRT_U1(image);
        vtlist[1].t = CALCULATE_PRT_V0(image);

        vtlist[2].s = CALCULATE_PRT_U1(image);
        vtlist[2].t = CALCULATE_PRT_V1(image);

        vtlist[3].s = CALCULATE_PRT_U0(image);
        vtlist[3].t = CALCULATE_PRT_V1(image);

        glBegin(GL_TRIANGLE_FAN);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2f(vtlist[i].s, vtlist[i].t);
          glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
        }
        glEnd();
      }
      cnt++;
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_transparent_prt_ref(Uint32 vrtcount, GLVERTEX * vrtlist)
{
  GLVERTEX vtlist[4];
  GLVECTOR vector_right, vector_up;
  GLVECTOR loc_vector_right, loc_vector_up;
  Uint16 cnt, prt;
  Uint16 image;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  int i;

  ATTRIB_PUSH("render_transparent_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    glDepthMask(GL_FALSE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    cnt = 0;
    while (cnt < vrtcount)
    {
      // Get the index from the color slot
      prt = (Uint16) vrtlist[cnt].color;

      // Draw transparent sprites this round
      if (PrtList[prt].type == PRTALPHASPRITE)
      {
        float color_component = PrtList[prt].light / 255.0;
        float alpha_component = particletrans / 255.0;

        // Figure out the sprite's size based on distance
        size = (float)PrtList[prt].size / (float)(1 << 10); // [claforte] Fudge the value.


        if (!PipList[PrtList[prt].pip].rotatetoface)
        {
          GLVECTOR vec1, vec2;

          vec1.x = 0;
          vec1.y = 0;
          vec1.z = 1;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vector_right = Normalize(CrossProduct(vec1, vec2));
          vector_up    = Normalize(CrossProduct(vector_right, vec2));

          //vector_right.x = mView.v[0];
          //vector_right.y = mView.v[4];
          //vector_right.z = mView.v[8];

          //vector_up.x = mView.v[1];
          //vector_up.y = mView.v[5];
          //vector_up.z = mView.v[9];

          rotate = (PrtList[prt].rotate - 24576) >> 2;
        }
        else
        {
          GLVECTOR vec_vel, vec1, vec2, vec3;

          vec_vel.x = PrtList[prt].xvel;
          vec_vel.y = PrtList[prt].yvel;
          vec_vel.z = PrtList[prt].zvel;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vec1 = Normalize(vec_vel);
          vec3 = CrossProduct(vec1, Normalize(vec2));

          vector_right = vec3;
          vector_up    = vec1;

          rotate = (PrtList[prt].rotate - 8192) >> 2;
        };

        sinsize = turntosin[rotate & TRIGTABLE_MASK];
        cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK];

        loc_vector_right.x = cossize * vector_right.x - sinsize * vector_up.x;
        loc_vector_right.y = cossize * vector_right.y - sinsize * vector_up.y;
        loc_vector_right.z = cossize * vector_right.z - sinsize * vector_up.z;

        loc_vector_up.x    = sinsize * vector_right.x + cossize * vector_up.x;
        loc_vector_up.y    = sinsize * vector_right.y + cossize * vector_up.y;
        loc_vector_up.z    = sinsize * vector_right.z + cossize * vector_up.z;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].x = vrtlist[cnt].x + ((-loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[0].y = vrtlist[cnt].y + ((-loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[0].z = vrtlist[cnt].z + ((-loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[1].x = vrtlist[cnt].x + ((loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[1].y = vrtlist[cnt].y + ((loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[1].z = vrtlist[cnt].z + ((loc_vector_right.z - loc_vector_up.z) * size);

        vtlist[2].x = vrtlist[cnt].x + ((loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[2].y = vrtlist[cnt].y + ((loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[2].z = vrtlist[cnt].z + ((loc_vector_right.z + loc_vector_up.z) * size);

        vtlist[3].x = vrtlist[cnt].x + ((-loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[3].y = vrtlist[cnt].y + ((-loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[3].z = vrtlist[cnt].z + ((-loc_vector_right.z + loc_vector_up.z) * size);

        // Fill in the rest of the data
        image = (Uint16)((PrtList[prt].image + PrtList[prt].imagestt) >> 8);

        vtlist[0].s = CALCULATE_PRT_U0(image);
        vtlist[0].t = CALCULATE_PRT_V0(image);

        vtlist[1].s = CALCULATE_PRT_U1(image);
        vtlist[1].t = CALCULATE_PRT_V0(image);

        vtlist[2].s = CALCULATE_PRT_U1(image);
        vtlist[2].t = CALCULATE_PRT_V1(image);

        vtlist[3].s = CALCULATE_PRT_U0(image);
        vtlist[3].t = CALCULATE_PRT_V1(image);

        // Go on and draw it
        glBegin(GL_TRIANGLE_FAN);
        //[claforte] should use alpha_component instead of 0.5?
        glColor4f(color_component, color_component, color_component, alpha_component);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2f(vtlist[i].s, vtlist[i].t);
          glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
        }
        glEnd();
      }
      cnt++;
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_light_prt_ref(Uint32 vrtcount, GLVERTEX * vrtlist)
{
  GLVERTEX vtlist[4];
  GLVECTOR vector_right, vector_up;
  GLVECTOR loc_vector_right, loc_vector_up;
  Uint16 cnt, prt;
  Uint16 image;
  float size;
  Uint16 rotate;
  float sinsize, cossize;
  int i;

  ATTRIB_PUSH("render_light_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
  {
    glDepthMask(GL_FALSE);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    cnt = 0;
    while (cnt < vrtcount)
    {
      // Get the index from the color slot
      prt = (Uint16) vrtlist[cnt].color;

      // Draw lights this round
      if (PrtList[prt].type == PRTLIGHTSPRITE)
      {
        // [claforte] Fudge the value.
        size = (float)PrtList[prt].size / (float)(1 << 9);

        if (!PipList[PrtList[prt].pip].rotatetoface)
        {
          GLVECTOR vec1, vec2;

          vec1.x = 0;
          vec1.y = 0;
          vec1.z = 1;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vector_right = Normalize(CrossProduct(vec1, vec2));
          vector_up    = Normalize(CrossProduct(vector_right, vec2));

          //vector_right.x = mView.v[0];
          //vector_right.y = mView.v[4];
          //vector_right.z = mView.v[8];

          //vector_up.x = mView.v[1];
          //vector_up.y = mView.v[5];
          //vector_up.z = mView.v[9];

          rotate = (PrtList[prt].rotate - 24576) >> 2;
        }
        else
        {
          GLVECTOR vec_vel, vec1, vec2, vec3;

          vec_vel.x = PrtList[prt].xvel;
          vec_vel.y = PrtList[prt].yvel;
          vec_vel.z = PrtList[prt].zvel;

          vec2.x = GCamera.x - PrtList[prt].xpos;
          vec2.y = GCamera.y - PrtList[prt].ypos;
          vec2.z = GCamera.z - PrtList[prt].zpos;

          vec1 = Normalize(vec_vel);
          vec3 = CrossProduct(vec1, Normalize(vec2));

          vector_right = vec3;
          vector_up    = vec1;

          rotate = (PrtList[prt].rotate - 8192) >> 2;
        };

        sinsize = turntosin[rotate & TRIGTABLE_MASK];
        cossize = turntosin[(rotate+4096) & TRIGTABLE_MASK];

        loc_vector_right.x = cossize * vector_right.x - sinsize * vector_up.x;
        loc_vector_right.y = cossize * vector_right.y - sinsize * vector_up.y;
        loc_vector_right.z = cossize * vector_right.z - sinsize * vector_up.z;

        loc_vector_up.x    = sinsize * vector_right.x + cossize * vector_up.x;
        loc_vector_up.y    = sinsize * vector_right.y + cossize * vector_up.y;
        loc_vector_up.z    = sinsize * vector_right.z + cossize * vector_up.z;

        // Calculate the position of the four corners of the billboard
        // used to display the particle.
        vtlist[0].x = vrtlist[cnt].x + ((-loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[0].y = vrtlist[cnt].y + ((-loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[0].z = vrtlist[cnt].z + ((-loc_vector_right.z - loc_vector_up.z) * size);
        vtlist[1].x = vrtlist[cnt].x + ((loc_vector_right.x - loc_vector_up.x) * size);
        vtlist[1].y = vrtlist[cnt].y + ((loc_vector_right.y - loc_vector_up.y) * size);
        vtlist[1].z = vrtlist[cnt].z + ((loc_vector_right.z - loc_vector_up.z) * size);
        vtlist[2].x = vrtlist[cnt].x + ((loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[2].y = vrtlist[cnt].y + ((loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[2].z = vrtlist[cnt].z + ((loc_vector_right.z + loc_vector_up.z) * size);
        vtlist[3].x = vrtlist[cnt].x + ((-loc_vector_right.x + loc_vector_up.x) * size);
        vtlist[3].y = vrtlist[cnt].y + ((-loc_vector_right.y + loc_vector_up.y) * size);
        vtlist[3].z = vrtlist[cnt].z + ((-loc_vector_right.z + loc_vector_up.z) * size);


        // Fill in the rest of the data
        image = (Uint16)((PrtList[prt].image + PrtList[prt].imagestt) >> 8);

        vtlist[0].s = CALCULATE_PRT_U0(image);
        vtlist[0].t = CALCULATE_PRT_V0(image);

        vtlist[1].s = CALCULATE_PRT_U1(image);
        vtlist[1].t = CALCULATE_PRT_V0(image);

        vtlist[2].s = CALCULATE_PRT_U1(image);
        vtlist[2].t = CALCULATE_PRT_V1(image);

        vtlist[3].s = CALCULATE_PRT_U0(image);
        vtlist[3].t = CALCULATE_PRT_V1(image);

        // Go on and draw it
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0, 1.0, 1.0, 1.0);
        for (i = 0; i < 4; i++)
        {
          glTexCoord2f(vtlist[i].s, vtlist[i].t);
          glVertex3f(vtlist[i].x, vtlist[i].y, vtlist[i].z);
        }
        glEnd();
      }
      cnt++;
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_particle_reflections()
{
  // ZZ> This function draws the sprites for particle systems

  GLVERTEX v[MAXPRT];
  Uint16 cnt, numparticle;
  float level;

  if (INVALID_TEXTURE == GLTexture_GetTextureID(&txTexture[particletexture]))
    return;

  // Original points
  numparticle = 0;
  cnt = 0;
  {
    while (cnt < MAXPRT)
    {
      if (PrtList[cnt].inview && PrtList[cnt].size != 0)
      {
        if (Mesh.fanlist[PrtList[cnt].onwhichfan].fx & MESHFXDRAWREF)
        {
          level = PrtList[cnt].level;
          v[numparticle].x = (float) PrtList[cnt].xpos;
          v[numparticle].y = (float) PrtList[cnt].ypos;
          v[numparticle].z = (float) - PrtList[cnt].zpos + level + level;
          v[numparticle].color = cnt;  // Store an index in the color slot...
          numparticle++;
        }

      }
      cnt++;
    }
  }

  ATTRIB_PUSH("render_particle_reflections", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
  {
    // Flat shade these babies
    glShadeModel(CData.shading);

    // Choose texture and matrix
    GLTexture_Bind(&txTexture[particletexture], CData.texturefilter);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DITHER);

    // DO ANTIALIAS SOLID SPRITES FIRST
    ATTRIB_GUARD_OPEN(prt_attrib_open);
    render_antialias_prt_ref(numparticle, v);
    ATTRIB_GUARD_CLOSE(prt_attrib_open, prt_attrib_close);

    // DO SOLID SPRITES FIRST
    ATTRIB_GUARD_OPEN(prt_attrib_open);
    render_solid_prt_ref(numparticle, v);
    ATTRIB_GUARD_CLOSE(prt_attrib_open, prt_attrib_close);

    // DO TRANSPARENT SPRITES NEXT
    ATTRIB_GUARD_OPEN(prt_attrib_open);
    render_transparent_prt_ref(numparticle, v);
    ATTRIB_GUARD_CLOSE(prt_attrib_open, prt_attrib_close);

    // LIGHTS DONE LAST
    ATTRIB_GUARD_OPEN(prt_attrib_open);
    render_light_prt_ref(numparticle, v);
    ATTRIB_GUARD_CLOSE(prt_attrib_open, prt_attrib_close);
  }
  glPopAttrib();

}


