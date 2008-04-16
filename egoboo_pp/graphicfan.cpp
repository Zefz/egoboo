// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "MPD_file.h"
#include "egoboo.h"

void render_fan(Mesh & msh, Uint32 fan, Uint32 texid)
{
  // ZZ> This function draws a mesh fan

  GLVertex v[MAXMESHVERTICES];
  Uint16 commands;
  Uint16 vertices;
  Uint16 basetile;
  TEX_REF texture;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  const JF::MPD_Fan * pfan = msh.getFan(fan);

  Uint16 tile = pfan->textureTile;               // Tile
  Uint8  fx   = pfan->flags;                     // Fx bits
  Uint16 type = pfan->type;                      // Command type ( index to points in fan )

  if (tile == Mesh::INVALID_INDEX)
    return;

  const JF::MPD_FanType * pfantype = msh.getFanType(type);

  // Animate the tiles
  if (fx & MESHFX_ANIM)
  {
    if (type >= (MAXMESHTYPE>>1))
    {
      // Big tiles
      basetile = tile & GTile_Anim.bigbaseand;// Animation set
      tile += GTile_Anim.frameadd << 1;         // Animated tile
      tile = (tile & GTile_Anim.bigframeand) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & GTile_Anim.baseand;// Animation set
      tile += GTile_Anim.frameadd;         // Animated tile
      tile = (tile & GTile_Anim.frameand) + basetile;
    }
  }

  texture = (tile>>6) + TX_TILE0;      // 0x40 tiles in each 256x256 texture
  vertices = pfantype->numVertices;    // Number of vertices
  commands = pfantype->numCommands;    // Number of commands

  vec3_t normal;
  float global_light = 0;
  if( GMesh.smoothed_normal(fan, normal) )
  {
    global_light = WaterList.la*(1 + MAX(0,normal.x*WaterList.lx) + MAX(0,normal.y*WaterList.ly) + MAX(0,normal.z*WaterList.lz));
  };

  // Original points
  const JF::MPD_Vertex * pverts = msh.getVertices();
  badvertex = pfan->firstVertex;          // Get big reference value
  {
    for (cnt = 0; cnt < vertices; cnt++)
    {

      v[cnt].pos.x = pverts[badvertex].x;
      v[cnt].pos.y = pverts[badvertex].y;
      v[cnt].pos.z = pverts[badvertex].z;


      v[cnt].fcolor.r  = pverts[badvertex].light + ((pverts[badvertex].color>> 0)&0xFF);
      v[cnt].fcolor.r /= float(0xFF);
      v[cnt].fcolor.r += global_light;

      v[cnt].fcolor.g  = pverts[badvertex].light + ((pverts[badvertex].color>> 8)&0xFF);
      v[cnt].fcolor.g /= float(0xFF);
      v[cnt].fcolor.g += global_light;

      v[cnt].fcolor.b  = pverts[badvertex].light + ((pverts[badvertex].color>>24)&0xFF);
      v[cnt].fcolor.b /= float(0xFF);
      v[cnt].fcolor.b += global_light;

      v[cnt].txcoord.s = pverts[badvertex].s;
      v[cnt].txcoord.t = pverts[badvertex].t;
      badvertex++;
    }
  }

  // Change texture if need be
  if(texid != (Uint32)(-1))
  {
    glBindTexture(GL_TEXTURE_2D, -1);
  }
  else if (GMesh.txref_last.index != texture.index)
  {
    TxList[texture].Bind(GL_TEXTURE_2D);
    GMesh.txref_last = texture;
  }

  // Render each command
  //dump_gl_state("before rendering", true, true);
  entry = 0;
  for (cnt = 0; cnt < commands; cnt++)
  {

    glColor4f(1,1,1,1);
    glBegin(GL_TRIANGLE_FAN);
      for (tnc = 0; tnc < pfantype->numCommandEntries[cnt]; tnc++)
      {
        vertex = pfantype->vertexIndices[entry];

        glColor3fv(v[vertex].fcolor.vals);
        glTexCoord2fv(v[vertex].txcoord.vals);
        glVertex3fv(v[vertex].pos.vals);

        entry++;
      }
    glEnd();
  }

  // GS - End3DMode ();
}

/*
  //    if(GFog.on)
  //    {
  //      // Uint8 red, grn, blu;
  //      // float z;
  //      //DWORD ambi;
  //      // DWORD GFog.spec;

  //        // The full fog value
  //        GFog.spec = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);
  //        for (cnt = 0; cnt < vertices; cnt++)
  //        {
  //            v[cnt].x = (D3DVALUE) GMesh.vrt_x[badvertex];
  //            v[cnt].y = (D3DVALUE) GMesh.vrt_y[badvertex];
  //            v[cnt].z = (D3DVALUE) GMesh.vrt_z[badvertex];
  //            z = v[cnt].z;

  //            // Figure out the fog coloring
  //            if(z < GFog.top)
  //            {
  //                if(z < GFog.bottom)
  //                {
  //                    v[cnt].dcSpecular = GFog.spec;  // Full fog
  //                }
  //                else
  //                {
  //                    z = 1.0 - ((z - GFog.bottom)/GFog.distance);  // 0.0 to 1.0... Amount of fog to keep
  //                    red = (GFog.red * z);
  //                    grn = (GFog.grn * z);
  //                    blu = (GFog.blu * z);
  //                    ambi = 0xff000000 | (red<<16) | (grn<<8) | (blu);
  //                    v[cnt].dcSpecular = ambi;
  //                }
  //            }
  //            else
  //            {
  //                v[cnt].dcSpecular = 0;  // No fog
  //            }

  //            ambi = (DWORD) GMesh.vrt_l[badvertex];
  //            ambi = (ambi<<8)|ambi;
  //            ambi = (ambi<<8)|ambi;
  ////                v[cnt].dcColor = ambi;
  ////                v[cnt].dwReserved = 0;
  //            badvertex++;
  //        }
  //    }
  //    else
  //*/

//--------------------------------------------------------------------------------------------
void render_water_fan(Mesh & msh, Uint32 fan, Uint8 layer)
{
  // ZZ> This function draws a water fan
  GLVertex v[MAXMESHVERTICES];
  Uint16 commands;
  Uint16 vertices;
  TEX_REF texture;
  Uint16 frame;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  vec2_t off;
  Uint32 ambi;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  const JF::MPD_Fan * pfan = msh.getFan(fan);

  Uint16 tile = pfan->textureTile;               // Tile
  Uint8  fx   = pfan->flags;                     // Fx bits
  Uint16 type = 0;                               // Command type ( index to points in fan )


  // To make life easier
  off   = WaterList[layer].off;          // Texture offsets
  frame = WaterList[layer].frame;     // Frame

  const JF::MPD_FanType * pfantype = msh.getFanType(type);

  texture  = layer+TX_WATERTOP;        // Water starts at texture 5
  vertices = pfantype->numVertices;    // Number of vertices
  commands = pfantype->numCommands;    // Number of commands

  // Original points
  const JF::MPD_Vertex * pverts = msh.getVertices();
  badvertex = pfan->firstVertex;          // Get big reference value
  {
    for (cnt = 0; cnt < vertices; cnt++)
    {

      v[cnt].pos.x = pverts[badvertex].x;
      v[cnt].pos.y = pverts[badvertex].y;

      int ix = int(pverts[badvertex].x);
      int iy = int(pverts[badvertex].y);

      int jx = ((ix>>JF::MPD_bits)&0x01) ^ ((ix>>(JF::MPD_bits-1))&0x01);
      int jy = ((iy>>JF::MPD_bits)&0x01) ^ ((iy>>(JF::MPD_bits-1))&0x01);

      v[cnt].pos.z = WaterList[layer].z + WaterList[layer].z_add[frame][jx][jy];

      ambi  = pverts[badvertex].ambient;
      ambi += WaterList[layer].color[frame][jx][jy];
      v[cnt].fcolor.r = v[cnt].fcolor.g = v[cnt].fcolor.b = ambi/float(0xFF);
      v[cnt].fcolor.a = WaterList[layer].alpha / float(0xFF);


      // !!!BAD!!!  Debug code for show what mode means...
      //red = 50;
      //grn = 50;
      //blu = 50;
      //switch(mode)
      //{
      //    case 0:
      //      red = 0xFF;
      //      break;
      //    case 1:
      //      grn = 0xFF;
      //      break;
      //    case 2:
      //      blu = 0xFF;
      //      break;
      //    case 3:
      //      red = 0xFF;
      //      grn = 0xFF;
      //      blu = 0xFF;
      //      break;
      //}
      //ambi = 0xbf000000 | (red<<16) | (grn<<8) | (blu);
      // !!!BAD!!!

      badvertex++;
    }
  }

  // Change texture if need be
  if (GMesh.txref_last.index != texture.index)
  {
    TxList[texture].Bind(GL_TEXTURE_2D);
    GMesh.txref_last = texture;
  }

  // Render each command
  entry = 0;
  v[0].txcoord = off + vec2_t(1,0);
  v[1].txcoord = off + vec2_t(1,1);
  v[2].txcoord = off + vec2_t(0,1);
  v[3].txcoord = off + vec2_t(0,0);

  entry = 0;
  for (cnt = 0; cnt < commands; cnt++)
  {

    glColor4f(1,1,1,1);
    glBegin(GL_TRIANGLE_FAN);
      for (tnc = 0; tnc < pfantype->numCommandEntries[cnt]; tnc++)
      {
        vertex = pfantype->vertexIndices[entry];

        glColor4fv(v[vertex].fcolor.vals);
        glTexCoord2fv(v[vertex].txcoord.vals);
        glVertex3fv(v[vertex].pos.vals);

        entry++;
      }
    glEnd();
  }
}

  ///*
  //   if(GFog.on)
  //   {
  //       // The full fog value
  //       GFog.spec = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);
  //       for (cnt = 0; cnt < vertices; cnt++)
  //       {
  //           v[cnt].x = (D3DVALUE) GMesh.vrt_x[badvertex];
  //           v[cnt].y = (D3DVALUE) GMesh.vrt_y[badvertex];
  //           v[cnt].z = WaterList[layer].z_add[frame][mode][cnt]+WaterList[layer].z;
  //           z = v[cnt].z;
  //           ambi = (DWORD) GMesh.vrt_l[badvertex]>>1;
  //           ambi+= WaterList[layer].color[frame][mode][cnt];
  //           ambi = (ambi<<8)|ambi;
  //           ambi = (ambi<<8)|ambi;
  //           ambi = (WaterList[layer].alpha<<24)|ambi;
  //           v[cnt].dcColor = ambi;

  //           // Figure out the fog coloring
  //           if(z < GFog.top && GFog.affectswater)
  //           {
  //               if(z < GFog.bottom)
  //               {
  //                   v[cnt].dcSpecular = GFog.spec;  // Full fog
  //               }
  //               else
  //               {
  //                   spectokeep = ((z - GFog.bottom)/GFog.distance);  // 0.0 to 1.0... Amount of old to keep
  //                   z = 1.0 - spectokeep;  // 0.0 to 1.0... Amount of fog to keep
  //                   spek = WaterList.spek[ambi&0xFF]&0xFF;
  //                   spek = spek * spectokeep;
  //                   red = (GFog.red * z) + spek;
  //                   grn = (GFog.grn * z) + spek;
  //                   blu = (GFog.blu * z) + spek;
  //                   ambi = 0xff000000 | (red<<16) | (grn<<8) | (blu);
  //                   v[cnt].dcSpecular = ambi;
  //               }
  //           }
  //           else
  //           {
  //               v[cnt].dcSpecular = WaterList.spek[ambi&0xFF];  // Old spec
  //           }

  //           v[cnt].dwReserved = 0;
  //           badvertex++;
  //       }
  //   }
  //   else
  //   */

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if 0
//void render_fan(Uint32 fan)
//{
//  // ZZ> This function draws a mesh fan
//
//  GLVertex v[MAXMESHVERTICES];
//  Uint16 commands;
//  Uint16 vertices;
//  Uint16 basetile;
//  GLuint texture;
//  Uint16 cnt, tnc, entry, vertex;
//  Uint32 badvertex;
//  float off_u, off_v;
//
//  // vertex is a value from 0-15, for the meshcommandref/u/v variables
//  // badvertex is a value that references the actual vertex number
//
//  Uint16 tile = GMesh.fan_info[fan].tile;               // Tile
//  Uint8  fx   = GMesh.fan_info[fan].fx;                 // Fx bits
//  Uint16 type = GMesh.fan_info[fan].type;               // Command type ( index to points in fan )
//
//  if (tile == Mesh::INVALID_INDEX)
//    return;
//
//  // Animate the tiles
//  if (fx & MESHFX_ANIM)
//  {
//    if (type >= (MAXMESHTYPE>>1))
//    {
//      // Big tiles
//      basetile = tile & GTile_Anim.bigbaseand;// Animation set
//      tile += GTile_Anim.frameadd << 1;         // Animated tile
//      tile = (tile & GTile_Anim.bigframeand) + basetile;
//    }
//    else
//    {
//      // Small tiles
//      basetile = tile & GTile_Anim.baseand;// Animation set
//      tile += GTile_Anim.frameadd;         // Animated tile
//      tile = (tile & GTile_Anim.frameand) + basetile;
//    }
//  }
//  off_u = GMesh.tile_off_u[tile];          // Texture offsets
//  off_v = GMesh.tile_off_v[tile];          //
//
//  texture = (tile>>6) + TX_TILE0;            // 0x40 tiles in each 256x256 texture
//  vertices = GMesh.commandnumvertices[type]; // Number of vertices
//  commands = GMesh.commands[type];           // Number of commands
//
//  // Original points
//  badvertex = GMesh.fan_info[fan].vrtstart;          // Get big reference value
//  {
//    for (cnt = 0; cnt < vertices; cnt++)
//    {
//      v[cnt].x = (float) GMesh.vrt_x[badvertex];
//      v[cnt].y = (float) GMesh.vrt_y[badvertex];
//      v[cnt].z = (float) GMesh.vrt_z[badvertex];
//      v[cnt].r = v[cnt].g = v[cnt].b = (float)GMesh.vrt_l[badvertex] / float(0xFF);
//      v[cnt].s = GMesh.commandu[type][badvertex] + off_u;
//      v[cnt].t = GMesh.commandv[type][badvertex] + off_v;
//      badvertex++;
//    }
//  }
//
//  // Change texture if need be
//  if (GMesh.txref_last != texture)
//  {
//    TxList[texture].Bind(GL_TEXTURE_2D);
//    GMesh.txref_last = texture;
//  }
//
//  // Make new ones so we can index them and not transform 'em each time
//  //if(transform_vertices(vertices, v, vt))
//  //  return;
//
//  //dump_gl_state();
//
//  // Render each command
//  entry = 0;
//  for (cnt = 0; cnt < commands; cnt++)
//  {
//    glBegin(GL_TRIANGLE_FAN);
//    for (tnc = 0; tnc < GMesh.commandsize[type][cnt]; tnc++)
//    {
//      vertex = GMesh.commandvrt[type][entry];
//      glColor3fv(&v[vertex].r);
//      glTexCoord2f(GMesh.commandu[type][vertex]+off_u, GMesh.commandv[type][vertex]+off_v);
//      glVertex3fv(&v[vertex].x);
//
//      entry++;
//    }
//    glEnd();
//  }
//
//  // GS - End3DMode ();
//}
//
///*
//      if(GFog.on)
//      {
//        // Uint8 red, grn, blu;
//        // float z;
//        //DWORD ambi;
//        // DWORD GFog.spec;
//
//          // The full fog value
//          GFog.spec = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);
//          for (cnt = 0; cnt < vertices; cnt++)
//          {
//              v[cnt].x = (D3DVALUE) GMesh.vrt_x[badvertex];
//              v[cnt].y = (D3DVALUE) GMesh.vrt_y[badvertex];
//              v[cnt].z = (D3DVALUE) GMesh.vrt_z[badvertex];
//              z = v[cnt].z;
//
//              // Figure out the fog coloring
//              if(z < GFog.top)
//              {
//                  if(z < GFog.bottom)
//                  {
//                      v[cnt].dcSpecular = GFog.spec;  // Full fog
//                  }
//                  else
//                  {
//                      z = 1.0 - ((z - GFog.bottom)/GFog.distance);  // 0.0 to 1.0... Amount of fog to keep
//                      red = (GFog.red * z);
//                      grn = (GFog.grn * z);
//                      blu = (GFog.blu * z);
//                      ambi = 0xff000000 | (red<<16) | (grn<<8) | (blu);
//                      v[cnt].dcSpecular = ambi;
//                  }
//              }
//              else
//              {
//                  v[cnt].dcSpecular = 0;  // No fog
//              }
//
//              ambi = (DWORD) GMesh.vrt_l[badvertex];
//              ambi = (ambi<<8)|ambi;
//              ambi = (ambi<<8)|ambi;
//  //                v[cnt].dcColor = ambi;
//  //                v[cnt].dwReserved = 0;
//              badvertex++;
//          }
//      }
//      else
//  */
//
////--------------------------------------------------------------------------------------------
//void render_water_fan(Uint32 fan, Uint8 layer)
//{
//  // ZZ> This function draws a water fan
//  GLVertex v[MAXMESHVERTICES];
//  Uint16 type;
//  Uint16 commands;
//  Uint16 vertices;
//  Uint16 texture, frame;
//  Uint16 cnt, tnc, entry, vertex;
//  Uint32 badvertex;
//// Uint8 red, grn, blu;
//  float off_u, off_v;
//// float z;
//  Uint32 ambi;
//// DWORD GFog.spec;
//
//  // vertex is a value from 0-15, for the meshcommandref/u/v variables
//  // badvertex is a value that references the actual vertex number
//
//  // To make life easier
//  type = 0;                           // Command type ( index to points in fan )
//  off_u = WaterList[layer].u;          // Texture offsets
//  off_v = WaterList[layer].v;          //
//  frame = WaterList[layer].frame;     // Frame
//
//  texture = layer+TX_WATERTOP;              // Water starts at texture 5
//  vertices = GMesh.commandnumvertices[type];// Number of vertices
//  commands = GMesh.commands[type];          // Number of commands
//
//  // Original points
//  badvertex = GMesh.fan_info[fan].vrtstart;          // Get big reference value
//  // Corners
//  /*
//     if(GFog.on)
//     {
//         // The full fog value
//         GFog.spec = 0xff000000 | (GFog.red<<16) | (GFog.grn<<8) | (GFog.blu);
//         for (cnt = 0; cnt < vertices; cnt++)
//         {
//             v[cnt].x = (D3DVALUE) GMesh.vrt_x[badvertex];
//             v[cnt].y = (D3DVALUE) GMesh.vrt_y[badvertex];
//             v[cnt].z = WaterList[layer].z_add[frame][mode][cnt]+WaterList[layer].z;
//             z = v[cnt].z;
//             ambi = (DWORD) GMesh.vrt_l[badvertex]>>1;
//             ambi+= WaterList[layer].color[frame][mode][cnt];
//             ambi = (ambi<<8)|ambi;
//             ambi = (ambi<<8)|ambi;
//             ambi = (WaterList[layer].alpha<<24)|ambi;
//             v[cnt].dcColor = ambi;
//
//             // Figure out the fog coloring
//             if(z < GFog.top && GFog.affectswater)
//             {
//                 if(z < GFog.bottom)
//                 {
//                     v[cnt].dcSpecular = GFog.spec;  // Full fog
//                 }
//                 else
//                 {
//                     spectokeep = ((z - GFog.bottom)/GFog.distance);  // 0.0 to 1.0... Amount of old to keep
//                     z = 1.0 - spectokeep;  // 0.0 to 1.0... Amount of fog to keep
//                     spek = WaterList.spek[ambi&0xFF]&0xFF;
//                     spek = spek * spectokeep;
//                     red = (GFog.red * z) + spek;
//                     grn = (GFog.grn * z) + spek;
//                     blu = (GFog.blu * z) + spek;
//                     ambi = 0xff000000 | (red<<16) | (grn<<8) | (blu);
//                     v[cnt].dcSpecular = ambi;
//                 }
//             }
//             else
//             {
//                 v[cnt].dcSpecular = WaterList.spek[ambi&0xFF];  // Old spec
//             }
//
//             v[cnt].dwReserved = 0;
//             badvertex++;
//         }
//     }
//     else
//     */
//  {
//    for (cnt = 0; cnt < vertices; cnt++)
//    {
//
//      //v[cnt].x = (D3DVALUE) GMesh.vrt_x[badvertex];
//      //v[cnt].y = (D3DVALUE) GMesh.vrt_y[badvertex];
//      //v[cnt].z = WaterList[layer].z_add[frame][mode][cnt]+WaterList[layer].z;
//      v[cnt].x = GMesh.vrt_x[badvertex];
//      v[cnt].y = GMesh.vrt_y[badvertex];
//      v[cnt].z = WaterList[layer].z_add[frame][mode][cnt]+WaterList[layer].z;
//
//      ambi = (Uint32) GMesh.vrt_l[badvertex]>>1;
//      ambi+= WaterList[layer].color[frame][mode][cnt];
//      v[cnt].r = v[cnt].g = v[cnt].b = (float)ambi / float(0xFF);
//      /*
//      ambi = (ambi<<8)|ambi;
//               ambi = (ambi<<8)|ambi;
//               */
//      v[cnt].a = (float)WaterList[layer].alpha / float(0xFF);
//
//// !!!BAD!!!  Debug code for show what mode means...
////red = 50;
////grn = 50;
////blu = 50;
////switch(mode)
////{
////    case 0:
////      red = 0xFF;
////      break;
////    case 1:
////      grn = 0xFF;
////      break;
////    case 2:
////      blu = 0xFF;
////      break;
////    case 3:
////      red = 0xFF;
////      grn = 0xFF;
////      blu = 0xFF;
////      break;
////}
////ambi = 0xbf000000 | (red<<16) | (grn<<8) | (blu);
//// !!!BAD!!!
//
//      //v[cnt].dcColor = ambi;
//      //v[cnt].dcSpecular = WaterList.spek[ambi&0xFF];
//      //v[cnt].dwReserved = 0;
//      badvertex++;
//    }
//  }
//
//  // Change texture if need be
//  if (GMesh.txref_last != texture)
//  {
//    //lpD3DDDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
//    //          TxList[texture].GetHandle());
//    TxList[texture].Bind(GL_TEXTURE_2D);
//    GMesh.txref_last = texture;
//  }
//
//  // Make new ones so we can index them and not transform 'em each time
//  //if(transform_vertices(vertices, v, vt))
//  //    return;
//
//  // Render each command
//  entry = 0;
//  v[0].s = 1+off_u;
//  v[0].t = 0+off_v;
//  v[1].s = 1+off_u;
//  v[1].t = 1+off_v;
//  v[2].s = 0+off_u;
//  v[2].t = 1+off_v;
//  v[3].s = 0+off_u;
//  v[3].t = 0+off_v;
//  for (cnt = 0; cnt < commands; cnt++)
//  {
//    glBegin(GL_TRIANGLE_FAN);
//    for (tnc = 0; tnc < GMesh.commandsize[type][cnt]; tnc++)
//    {
//      vertex = GMesh.commandvrt[type][entry];
//      glColor4fv(&v[vertex].r);
//      glTexCoord2fv(&v[vertex].s);
//      glVertex3fv(&v[vertex].x);
//      /*
//                    vtlist[tnc].dvSX = vt[vertex].dvSX;
//                    vtlist[tnc].dvSY = vt[vertex].dvSY;
//                    vtlist[tnc].dvSZ = vt[vertex].dvSZ;
//                    vtlist[tnc].dvRHW = vt[vertex].dvRHW;
//                    vtlist[tnc].dcColor = vt[vertex].dcColor;
//                    vtlist[tnc].dcSpecular = vt[vertex].dcSpecular;
//                    vtlist[tnc].dvTU = GMesh.commandu[type][vertex]+off_u;
//                    vtlist[tnc].dvTV = GMesh.commandv[type][vertex]+off_v;
//      */
//      entry++;
//    }
//    glEnd();
////            lpD3DDDevice->DrawPrimitive((D3DPRIMITIVETYPE) D3DPT_TRIANGLEFAN,
////                                    D3DVT_TLVERTEX, (LPVOID)vtlist, tnc, NULL);
//  }
//}

#endif