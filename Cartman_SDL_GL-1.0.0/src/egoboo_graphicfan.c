//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - graphicfan.c
 * World mesh drawing.
 */

#include "cartman_gfx.h"
#include "cartman_mpd.h"

#include "egoboo_graphic.h"

#include "ogl_texture.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static Uint32 faketoreal[MAXMESHVERTICES];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t generate_faketoreal( int fan )
{
    int cnt;
    Uint32 vert;

    if ( -1 == fan || FANOFF == fan ) return bfalse;

    for ( cnt = 0, vert = mesh.vrtstart[fan];
          cnt < mesh.command[fan].numvertices && CHAINEND != vert;
          cnt++, vert = mesh.vrtnext[vert] )
    {
        faketoreal[cnt] = vert;
    }

    return btrue;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_fan( int fan )
{
    // ZZ> This function draws a mesh fan
    simple_vertex_t v[MAXMESHVERTICES];
    Uint16 commands;
    Uint16 vertices;
    Uint16 basetile;
    Uint16 texture;
    Uint16 cnt, tnc, entry, vertex;
    Uint32 badvertex;
    Uint16 tx_bits;
    Uint8  fx;
    Uint16 fantype;

    if ( -1 == fan || FANOFF == fan ) return;

    // vertex is a value from 0-15, for the mesh.commandref/u/v variables
    // badvertex is a value that references the actual vertex number

    tx_bits = mesh.tx_bits[fan];               // Tile
    fx      = mesh.fx[fan];                   // Fx bits
    fantype = mesh.fantype[fan];               // Command fantype ( index to points in fan )

    if ( TILE_IS_FANOFF( tx_bits ) ) return;

    // Animate the tiles
    if ( fx & MPDFX_ANIM )
    {
        if ( fantype >= ( MAXMESHTYPE >> 1 ) )
        {
            // Big tiles
            basetile = tx_bits & biganimtilebaseand;// Animation set
            tx_bits += animtileframeadd << 1;         // Animated tx_bits
            tx_bits = ( tx_bits & biganimtileframeand ) + basetile;
        }
        else
        {
            // Small tiles
            basetile = tx_bits & animtilebaseand;// Animation set
            tx_bits += animtileframeadd;         // Animated tx_bits
            tx_bits = ( tx_bits & animtileframeand ) + basetile;
        }
    }

    // remove any of the upper bit information
    tx_bits &= 0xFF;

    texture = ( tx_bits >> 6 ) + TX_TILE_0;    // 64 tiles in each 256x256 texture
    vertices = mesh.command[fantype].numvertices;// Number of vertices
    commands = mesh.command[fantype].count;          // Number of commands

    // Generate the vertex "translation" from a linked list to an array
    generate_faketoreal( tx_bits );

    {
        for ( cnt = 0; cnt < vertices; cnt++ )
        {
            badvertex = faketoreal[cnt];

            v[cnt].x = mesh.vrtx[badvertex];
            v[cnt].y = mesh.vrty[badvertex];
            v[cnt].z = mesh.vrtz[badvertex];

            v[cnt].r =
                v[cnt].g =
                    v[cnt].b = mesh.vrta[badvertex] / 255.0;

            v[cnt].s = mesh.command[fantype].u[badvertex];
            v[cnt].t = mesh.command[fantype].v[badvertex];
            badvertex++;
        }
    }

    // bind the texture for this tx_bits
    glTexture_Bind( tile_at( tx_bits ) );

    // Render each command
    entry = 0;

    for ( cnt = 0; cnt < commands; cnt++ )
    {
        glBegin( GL_TRIANGLE_FAN );
        {
            for ( tnc = 0; tnc < mesh.command[fantype].size[cnt]; tnc++ )
            {
                vertex = mesh.command[fantype].vrt[entry];
                glColor3fv( &v[vertex].r );
                glTexCoord2f( mesh.command[fantype].u[vertex], mesh.command[fantype].v[vertex] );
                glVertex3fv( &v[vertex].x );
                entry++;
            }

        }
        glEnd();
    }

}

//--------------------------------------------------------------------------------------------
void render_water_fan( int tile, Uint8 layer )
{
    // ZZ> This function draws a water tile
    simple_vertex_t v[MAXMESHVERTICES];

    Uint16 type;
    Uint16 commands;
    Uint16 vertices;
    Uint16 texture, frame;
    Uint16 cnt, tnc, entry;
    Uint32  badvertex;
    float offu, offv;
    Uint32  ambi;
    Uint8 mode;
    int ix, iy, ix_off[4] = {1, 1, 0, 0}, iy_off[4] = {0, 1, 1, 0};

    if ( -1 == tile && FANOFF != tile ) return;

    // BB > the water info is for TILES, not fot vertices, so ignore all vertex info and just draw the water
    //      tile where it's supposed to go

    ix = tile % mesh.tiles_x;
    iy = tile / mesh.tiles_x;

    // just do the mode this way instead of requiring all mesh.es to be multiples of 4
    mode = ( iy & 1 ) | (( ix & 1 ) << 1 );

    // To make life easier
    type = 0;                           // Command type ( index to points in tile )
    offu = waterlayeru[layer];          // Texture offsets
    offv = waterlayerv[layer];          //
    frame = waterlayerframe[layer];     // Frame

    texture = layer + TX_WATER_TOP;         // Water starts at texture 5
    vertices = mesh.command[type].numvertices;// Number of vertices
    commands = mesh.command[type].count;          // Number of commands

    // Original points
    badvertex = mesh.vrtstart[tile];          // Get big reference value
    {
        for ( cnt = 0; cnt < 4; cnt++ )
        {
            v[cnt].x = ( ix + ix_off[cnt] ) * 128;
            v[cnt].y = ( iy + iy_off[cnt] ) * 128;
            v[cnt].s = ix_off[cnt] + offu;
            v[cnt].t = iy_off[cnt] + offv;
            v[cnt].z = waterlayerzadd[layer][frame][mode][cnt] + waterlayerz[layer];

            ambi = ( Uint32 ) mesh.vrta[badvertex] >> 1;
            ambi += waterlayercolor[layer][frame][mode][cnt];
            v[cnt].r = FP8_TO_FLOAT( ambi );
            v[cnt].g = FP8_TO_FLOAT( ambi );
            v[cnt].b = FP8_TO_FLOAT( ambi );
            v[cnt].a = FP8_TO_FLOAT( waterlayeralpha[layer] );

            badvertex++;
        }
    }

    // bind the texture for this tile
    glTexture_Bind( tile_at( tile ) );

    // Render each command
    entry = 0;

    for ( cnt = 0; cnt < commands; cnt++ )
    {
        glBegin( GL_TRIANGLE_FAN );
        {
            for ( tnc = 0; tnc < 4; tnc++ )
            {
                glColor4fv( &v[tnc].r );
                glTexCoord2fv( &v[tnc].s );
                glVertex3fv( &v[tnc].x );
                entry++;
            }
        }
        glEnd();
    }
}
