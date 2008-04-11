/* Egoboo - graphicfan.c
* World mesh drawing.
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

#include "Log.h"
#include "egoboo.h"

MESH Mesh  = {bfalse, NULL,NULL,NULL,NULL,NULL,NULL, 0,0,0,0 };


//--------------------------------------------------------------------------------------------
void render_fan(Uint32 fan, char tex_loaded)
{
  // ZZ> This function draws a mesh fan

  GLVERTEX v[MAXMESHVERTICES];
  Uint16 commands;
  Uint16 vertices;
  Uint16 basetile;
  Uint16 texture;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  float offu, offv;
  int light_flat;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  Uint16 tile = Mesh.fanlist[fan].tile;               // Tile
  Uint8  fx   = Mesh.fanlist[fan].fx;                 // Fx bits
  Uint16 type = Mesh.fanlist[fan].type;               // Command type ( index to points in fan )

  if (tile == FANOFF)
    return;

  // Animate the tiles
  if (fx & MESHFXANIM)
  {
    if (type >= (MAXMESHTYPE >> 1))
    {
      // Big tiles
      basetile = tile & GTile_Anim.baseand_big;// Animation set
      tile += GTile_Anim.frameadd << 1;         // Animated tile
      tile = (tile & GTile_Anim.frameand_big) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & GTile_Anim.baseand;// Animation set
      tile += GTile_Anim.frameadd;         // Animated tile
      tile = (tile & GTile_Anim.frameand) + basetile;
    }
  }

  offu = Mesh.tilelist[tile].tileoffu;          // Texture offsets
  offv = Mesh.tilelist[tile].tileoffv;          //

  texture = (tile >> 6) + 1;                // 64 tiles in each 256x256 texture
  vertices = Mesh.tilelist[type].commandnumvertices;// Number of vertices
  commands = Mesh.tilelist[type].commands;          // Number of commands

  // Original points
  badvertex = Mesh.fanlist[fan].vrtstart;          // Get big reference value

  if (texture != tex_loaded) return;

  light_flat = 0;
  for (cnt = 0; cnt < vertices; cnt++)
  {
    light_flat += Mesh.vrtl[badvertex];

    v[cnt].x = (float) Mesh.vrtx[badvertex];
    v[cnt].y = (float) Mesh.vrty[badvertex];
    v[cnt].z = (float) Mesh.vrtz[badvertex];
    v[cnt].r = v[cnt].g = v[cnt].b = (float) Mesh.vrtl[badvertex] / 255.0f;
    v[cnt].s = Mesh.tilelist[type].commandu[badvertex] + offu;
    v[cnt].t = Mesh.tilelist[type].commandv[badvertex] + offv;

    badvertex++;
  }
  light_flat /= vertices;


  // Change texture if need be
  if (Mesh.lasttexture != texture)
  {
    GLTexture_Bind(&txTexture[texture], CData.texturefilter);
    Mesh.lasttexture = texture;
  }

  ATTRIB_PUSH("render_fan", GL_CURRENT_BIT);
  {
    // Render each command
    if (CData.shading == GL_FLAT)
    {
      // use the average lighting
      glColor4f(light_flat / 255.0f, light_flat / 255.0f, light_flat / 255.0f, 1);
      entry = 0;
      for (cnt = 0; cnt < commands; cnt++)
      {
        glBegin(GL_TRIANGLE_FAN);
        for (tnc = 0; tnc < Mesh.tilelist[type].commandsize[cnt]; tnc++)
        {
          vertex = Mesh.tilelist[type].commandvrt[entry];
          glTexCoord2f(Mesh.tilelist[type].commandu[vertex] + offu, Mesh.tilelist[type].commandv[vertex] + offv);
          glVertex3fv(&v[vertex].x);
          entry++;
        }
        glEnd();
      }
    }
    else
    {
      // use per-vertex lighting
      entry = 0;
      for (cnt = 0; cnt < commands; cnt++)
      {
        glBegin(GL_TRIANGLE_FAN);
        for (tnc = 0; tnc < Mesh.tilelist[type].commandsize[cnt]; tnc++)
        {
          vertex = Mesh.tilelist[type].commandvrt[entry];
          glColor3fv(&v[vertex].r);
          glTexCoord2f(Mesh.tilelist[type].commandu[vertex] + offu, Mesh.tilelist[type].commandv[vertex] + offv);
          glVertex3fv(&v[vertex].x);

          entry++;
        }
        glEnd();
      }
    }
  }
  ATTRIB_POP("render_fan");


}

//D3DLVERTEX v[MAXMESHVERTICES];
//D3DTLVERTEX vt[MAXMESHVERTICES];
//D3DTLVERTEX vtlist[MAXMESHCOMMANDSIZE];
// float z;
//DWORD ambi;
// DWORD GFog.spec;
// Uint8 red, grn, blu;
//TODO: Implement OpenGL fog effects
/*  if(GFog.on)
{
// The full fog value
GFog.spec = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);
for (cnt = 0; cnt < vertices; cnt++)
{
v[cnt].x = (float) Mesh.vrtx[badvertex];
v[cnt].y = (float) Mesh.vrty[badvertex];
v[cnt].z = (float) Mesh.vrtz[badvertex];
z = v[cnt].z;


// Figure out the fog coloring
if(z < GFog.top)
{
if(z < GFog.bottom)
{
v[cnt].dcSpecular = GFog.spec;  // Full fog
}
else
{
z = 1.0 - ((z - GFog.bottom)/GFog.distance);  // 0.0 to 1.0... Amount of fog to keep
red = (GFog.red * z);
grn = (GFog.grn * z);
blu = (GFog.blu * z);
ambi = 0xff000000 | (red<<16) | (grn<<8) | (blu);
v[cnt].dcSpecular = ambi;
}
}
else
{
v[cnt].dcSpecular = 0;  // No fog
}

ambi = (DWORD) Mesh.vrtl[badvertex];
ambi = (ambi<<8)|ambi;
ambi = (ambi<<8)|ambi;
//                v[cnt].dcColor = ambi;
//                v[cnt].dwReserved = 0;
badvertex++;
}
}
*/

//--------------------------------------------------------------------------------------------
void render_water_fan(Uint32 fan, Uint8 layer, Uint8 mode)
{
  // ZZ> This function draws a water fan
  GLVERTEX v[MAXMESHVERTICES];
  Uint16 type;
  Uint16 commands;
  Uint16 vertices;
  Uint16 texture, frame;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  float offu, offv;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  // To make life easier
  type = 0;                           // Command type ( index to points in fan )
  offu = waterlayeru[layer];          // Texture offsets
  offv = waterlayerv[layer];          //
  frame = waterlayerframe[layer];     // Frame

  texture = layer + 5;                    // Water starts at texture 5
  vertices = Mesh.tilelist[type].commandnumvertices;// Number of vertices
  commands = Mesh.tilelist[type].commands;          // Number of commands


  // figure the ambient light
  badvertex = Mesh.fanlist[fan].vrtstart;          // Get big reference value
  for (cnt = 0; cnt < vertices; cnt++)
  {
    v[cnt].x = Mesh.vrtx[badvertex];
    v[cnt].y = Mesh.vrty[badvertex];
    v[cnt].z = waterlayerzadd[layer][frame][mode][cnt] + waterlayerz[layer];

    if (!waterlight)
    {
      v[cnt].r = v[cnt].g = v[cnt].b = (float)Mesh.vrtl[badvertex] / 255.0f;
      v[cnt].a = (float)waterlayeralpha[layer] / 255.0f;
    }
    else
    {
      v[cnt].r = v[cnt].g = v[cnt].b = (float)Mesh.vrtl[badvertex] / 255.0f * (float)waterlayeralpha[layer] / 255.0f;
      v[cnt].a = 1.0f;
    }


    // !!!BAD!!!  Debug code for show what mode means...
    //red = 50;
    //grn = 50;
    //blu = 50;
    //switch(mode)
    //{
    //    case 0:
    //      red = 255;
    //      break;

    //    case 1:
    //      grn = 255;
    //      break;

    //    case 2:
    //      blu = 255;
    //      break;

    //    case 3:
    //      red = 255;
    //      grn = 255;
    //      blu = 255;
    //      break;

    //}
    //ambi = 0xbf000000 | (red<<16) | (grn<<8) | (blu);
    // !!!BAD!!!

    badvertex++;
  };

  // Render each command
  v[0].s = 1 + offu;
  v[0].t = 0 + offv;
  v[1].s = 1 + offu;
  v[1].t = 1 + offv;
  v[2].s = 0 + offu;
  v[2].t = 1 + offv;
  v[3].s = 0 + offu;
  v[3].t = 0 + offv;

  ATTRIB_PUSH("render_water_fan", GL_TEXTURE_BIT | GL_CURRENT_BIT);
  {
    GLTexture_Bind(&txTexture[texture], CData.texturefilter);

    entry = 0;
    for (cnt = 0; cnt < commands; cnt++)
    {
      glBegin(GL_TRIANGLE_FAN);
      for (tnc = 0; tnc < Mesh.tilelist[type].commandsize[cnt]; tnc++)
      {
        vertex = Mesh.tilelist[type].commandvrt[entry];
        glColor4fv(&v[vertex].r);
        glTexCoord2fv(&v[vertex].s);
        glVertex3fv(&v[vertex].x);

        entry++;
      }
      glEnd();
    }
  }
  ATTRIB_POP("render_water_fan");
}
// Uint8 red, grn, blu;
// float z;
// DWORD GFog.spec;


//--------------------------------------------------------------------------------------------
void render_water_fan_lit(Uint32 fan, Uint8 layer, Uint8 mode)
{
  // ZZ> This function draws a water fan
  GLVERTEX v[MAXMESHVERTICES];
  Uint16 type;
  Uint16 commands;
  Uint16 vertices;
  Uint16 texture, frame;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  // Uint8 red, grn, blu;
  float offu, offv;
  // float z;
  //Uint32 ambi, spek;
  // DWORD GFog.spec;



  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  // To make life easier
  type = 0;                           // Command type ( index to points in fan )
  offu = waterlayeru[layer];          // Texture offsets
  offv = waterlayerv[layer];          //
  frame = waterlayerframe[layer];     // Frame

  texture = layer + 5;                    // Water starts at texture 5
  vertices = Mesh.tilelist[type].commandnumvertices;// Number of vertices
  commands = Mesh.tilelist[type].commands;          // Number of commands


  badvertex = Mesh.fanlist[fan].vrtstart;          // Get big reference value
  for (cnt = 0; cnt < vertices; cnt++)
  {
    v[cnt].x = Mesh.vrtx[badvertex];
    v[cnt].y = Mesh.vrty[badvertex];
    v[cnt].z = waterlayerzadd[layer][frame][mode][cnt] + waterlayerz[layer];

    v[cnt].r = v[cnt].g = v[cnt].b = 1.0f;
    v[cnt].a = (float)waterlayeralpha[layer] / 255.0f;

    badvertex++;
  };

  // Render each command
  v[0].s = 1 + offu;
  v[0].t = 0 + offv;
  v[1].s = 1 + offu;
  v[1].t = 1 + offv;
  v[2].s = 0 + offu;
  v[2].t = 1 + offv;
  v[3].s = 0 + offu;
  v[3].t = 0 + offv;

  ATTRIB_PUSH("render_water_fan_lit", GL_TEXTURE_BIT | GL_CURRENT_BIT);
  {
    // Change texture if need be
    if (Mesh.lasttexture != texture)
    {
      GLTexture_Bind(&txTexture[texture], CData.texturefilter);
      Mesh.lasttexture = texture;
    }

    entry = 0;
    for (cnt = 0; cnt < commands; cnt++)
    {
      glBegin(GL_TRIANGLE_FAN);
      for (tnc = 0; tnc < Mesh.tilelist[type].commandsize[cnt]; tnc++)
      {
        vertex = Mesh.tilelist[type].commandvrt[entry];
        glColor4fv(&v[vertex].r);
        glTexCoord2fv(&v[vertex].s);
        glVertex3fv(&v[vertex].x);

        entry++;
      }
      glEnd();
    }
  }
  ATTRIB_POP("render_water_fan_lit");
}
