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

/* Egoboo - graphic.c
 * All sorts of stuff related to drawing the game, and all sorts of other stuff
 * (such as data loading) that really should not be in here.
 */

#include "cartman.h"
#include "egoboo_graphic.h"
#include "log.h"
#include "egoboo_fileutil.h"
//#include "script.h"
//#include "camera.h"

#include <math.h>
#include <assert.h>

#define MAXLIGHTROTATION                256         // Number of premade light maps
#define MD2LIGHTINDICES                 163//162    // MD2's store vertices as x,y,z,normal
#define MAXLIGHTLEVEL                   16          // Number of premade light intensities
#define MAXSPEKLEVEL                    16          // Number of premade specularities

renderlist_t renderlist;
dolist_t     dolist;

camera_t cam = {0, 0, 200, 200};

bool_t   twolayerwateron = btrue;      // Two layer water?
bool_t   usefaredge      = bfalse;

Uint8                   lightdirectionlookup[65536];// For lighting characters
Uint8                   lighttable[MAXLIGHTLEVEL][MAXLIGHTROTATION][MD2LIGHTINDICES];

float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
float           lighttoenviroy[256];                                // Environment map
Uint32           lighttospek[MAXSPEKLEVEL][256];                     //


float kMd2Normals[][3] =
{
#include "id_normals.inc"
    ,
    {0, 0, 0}  // Spiky Mace
};

// Defined in egoboo.h
SDL_Surface *displaySurface = NULL;
bool_t  gTextureOn = bfalse;

bool_t clearson     = btrue;             // Do we clear every time?
bool_t backgroundon = bfalse;    // Do we clear every time?
bool_t overlayon    = bfalse;
bool_t wateron      = bfalse;
GLenum shading      = GL_SMOOTH;

float sinlut[MAXLIGHTROTATION];
float coslut[MAXLIGHTROTATION];


bool_t                  fogallowed  = btrue;        //
bool_t                  fogon  = bfalse;            // Do ground fog?
float                   fogbottom  = 0.0f;          //
float                   fogtop  = 100;             //
float                   fogdistance  = 100;        //
Uint8                   fogred  = 255;             //  Fog collour
Uint8                   foggrn  = 255;             //
Uint8                   fogblu  = 255;             //
Uint8                   fogaffectswater;


float           hillslide      =  1.00f;           //
// Friction
float           slippyfriction =  1.00f;           // for Chevron
float           airfriction    =  0.95f;           //
float           waterfriction  =  0.85f;           //
float           noslipfriction =  0.95f;           //
float           platstick      =  0.040f;          //
float           gravity        = -1.0f;            // Gravitational accel

// Weather and water gfx
int     weatheroverwater = bfalse;       // Only spawn over water?
int     weathertimereset = 10;          // Rate at which weather particles spawn
int     weathertime = 0;                // 0 is no weather
int     weatherplayer;
int     numwaterlayer = 0;              // Number of layers
float   watersurfacelevel = 0;          // Surface level for water striders
float   waterdouselevel = 0;            // Surface level for torches
Uint8   waterlight = 0;                 // Is it light ( default is alpha )
Uint8   waterspekstart = 128;           // Specular begins at which light value
Uint8   waterspeklevel = 128;           // General specular amount (0-255)
Uint8   wateriswater = btrue;          // Is it water?  ( Or lava... )
Uint8   waterlightlevel[MAXWATERLAYER]; // General light amount (0-63)
Uint8   waterlightadd[MAXWATERLAYER];   // Ambient light amount (0-63)
float   waterlayerz[MAXWATERLAYER];     // Base height of water
Uint8   waterlayeralpha[MAXWATERLAYER]; // Transparency
float   waterlayeramp[MAXWATERLAYER];   // Amplitude of waves
float   waterlayeru[MAXWATERLAYER];     // Coordinates of texture
float   waterlayerv[MAXWATERLAYER];     //
float   waterlayeruadd[MAXWATERLAYER];  // Texture movement
float   waterlayervadd[MAXWATERLAYER];  //
float   waterlayerzadd[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
Uint8   waterlayercolor[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
Uint16  waterlayerframe[MAXWATERLAYER]; // Frame
Uint16  waterlayerframeadd[MAXWATERLAYER];      // Speed
float   waterlayerdistx[MAXWATERLAYER];         // For distant backgrounds
float   waterlayerdisty[MAXWATERLAYER];         //
Uint32  waterspek[256];             // Specular highlights
float   foregroundrepeat  = 1;     //
float   backgroundrepeat  = 1;     //


void make_lighttable( float lx, float ly, float lz, float ambi );
void make_lighttospek( void );
void make_water();
float light_for_normal( int rotation, int normal, float lx, float ly, float lz, float ambi );

//void EnableTexturing()
//{
//    if ( !gTextureOn )
//    {
//        glEnable( GL_TEXTURE_2D );
//        gTextureOn = btrue;
//    }
//}
//
//void DisableTexturing()
//{
//    if ( gTextureOn )
//    {
//        glDisable( GL_TEXTURE_2D );
//        gTextureOn = bfalse;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void reset_character_alpha( Uint16 character )
//{
//    // ZZ> This function fixes an item's transparency
//    Uint16 enchant, mount;
//    if ( character != MAXCHR )
//    {
//        mount = chrattachedto[character];
//        if ( chron[character] && mount != MAXCHR && chrisitem[character] && chrtransferblend[mount] )
//        {
//            // Okay, reset transparency
//            enchant = chrfirstenchant[character];
//
//            while ( enchant < MAXENCHANT )
//            {
//                unset_enchant_value( enchant, SETALPHABLEND );
//                unset_enchant_value( enchant, SETLIGHTBLEND );
//                enchant = encnextenchant[enchant];
//            }
//
//            chralpha[character] = chrbasealpha[character];
//            chrlight[character] = caplight[chrmodel[character]];
//            enchant = chrfirstenchant[character];
//
//            while ( enchant < MAXENCHANT )
//            {
//                set_enchant_value( enchant, SETALPHABLEND, enceve[enchant] );
//                set_enchant_value( enchant, SETLIGHTBLEND, enceve[enchant] );
//                enchant = encnextenchant[enchant];
//            }
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void move_water( void )
//{
//    // ZZ> This function animates the water overlays
//    int layer;
//
//    for ( layer = 0; layer < MAXWATERLAYER; layer++ )
//    {
//        waterlayeru[layer] += waterlayeruadd[layer];
//        waterlayerv[layer] += waterlayervadd[layer];
//        if ( waterlayeru[layer] > 1.0f )  waterlayeru[layer] -= 1.0f;
//        if ( waterlayerv[layer] > 1.0f )  waterlayerv[layer] -= 1.0f;
//        if ( waterlayeru[layer] < -1.0f )  waterlayeru[layer] += 1.0f;
//        if ( waterlayerv[layer] < -1.0f )  waterlayerv[layer] += 1.0f;
//
//        waterlayerframe[layer] = ( waterlayerframe[layer] + waterlayerframeadd[layer] ) & WATERFRAMEAND;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void load_mesh_fans()
//{
//    // ZZ> This function loads fan types for the terrain
//    int cnt, entry;
//    int numfantype, fantype, bigfantype, vertices;
//    int numcommand, command, commandsize;
//    int itmp;
//    float ftmp;
//    FILE* fileread;
//    float offx, offy;
//
//    // Initialize all mesh types to 0
//    entry = 0;
//
//    while ( entry < MAXMESHTYPE )
//    {
//        mesh.command[entry].numvertices = 0;
//        mesh.command[entry].count = 0;
//        entry++;
//    }
//
//    // Open the file and go to it
//    fileread = fopen( "basicdat" SLASH_STR "fans.txt", "r" );
//    if ( fileread )
//    {
//        goto_colon( fileread );
//        fscanf( fileread, "%d", &numfantype );
//        fantype = 0;
//        bigfantype = MAXMESHTYPE / 2; // Duplicate for 64x64 tiles
//
//        while ( fantype < numfantype )
//        {
//            goto_colon( fileread );
//            fscanf( fileread, "%d", &vertices );
//            mesh.command[fantype].numvertices = vertices;
//            mesh.command[bigfantype].numvertices = vertices;  // Dupe
//            cnt = 0;
//
//            while ( cnt < vertices )
//            {
//                goto_colon( fileread );
//                fscanf( fileread, "%d", &itmp );
//                goto_colon( fileread );
//                fscanf( fileread, "%f", &ftmp );
//                mesh.command[fantype].x[cnt] = ftmp;
//                mesh.command[bigfantype].x[cnt] = ftmp;  // Dupe
//                goto_colon( fileread );
//                fscanf( fileread, "%f", &ftmp );
//                mesh.command[fantype].v[cnt] = ftmp;
//                mesh.command[bigfantype].v[cnt] = ftmp;  // Dupe
//                cnt++;
//            }
//
//            goto_colon( fileread );
//            fscanf( fileread, "%d", &numcommand );
//            mesh.command[fantype].count = numcommand;
//            mesh.command[bigfantype].count = numcommand;  // Dupe
//            entry = 0;
//            command = 0;
//
//            while ( command < numcommand )
//            {
//                goto_colon( fileread );
//                fscanf( fileread, "%d", &commandsize );
//                mesh.command[fantype].size[command] = commandsize;
//                mesh.command[bigfantype].size[command] = commandsize;  // Dupe
//                cnt = 0;
//
//                while ( cnt < commandsize )
//                {
//                    goto_colon( fileread );
//                    fscanf( fileread, "%d", &itmp );
//                    mesh.command[fantype].vrt[entry] = itmp;
//                    mesh.command[bigfantype].vrt[entry] = itmp;  // Dupe
//                    entry++;
//                    cnt++;
//                }
//
//                command++;
//            }
//
//            fantype++;
//            bigfantype++;  // Dupe
//        }
//
//        fclose( fileread );
//    }
//
//    // Correct all of them silly texture positions for seamless tiling
//    entry = 0;
//
//    while ( entry < MAXMESHTYPE / 2 )
//    {
//        cnt = 0;
//
//        while ( cnt < mesh.command[entry].numvertices )
//        {
////            mesh.command[entry].x[cnt] = ((0.5f/32)+(mesh.command[entry].x[cnt]*31/32))/8;
////            mesh.command[entry].v[cnt] = ((0.5f/32)+(mesh.command[entry].v[cnt]*31/32))/8;
//            mesh.command[entry].x[cnt] = ( ( 0.6f / 32 ) + ( mesh.command[entry].x[cnt] * 30.8f / 32 ) ) / 8;
//            mesh.command[entry].v[cnt] = ( ( 0.6f / 32 ) + ( mesh.command[entry].v[cnt] * 30.8f / 32 ) ) / 8;
//            cnt++;
//        }
//
//        entry++;
//    }
//
//    // Do for big tiles too
//    while ( entry < MAXMESHTYPE )
//    {
//        cnt = 0;
//
//        while ( cnt < mesh.command[entry].numvertices )
//        {
////            mesh.command[entry].x[cnt] = ((0.5f/64)+(mesh.command[entry].x[cnt]*63/64))/4;
////            mesh.command[entry].v[cnt] = ((0.5f/64)+(mesh.command[entry].v[cnt]*63/64))/4;
//            mesh.command[entry].x[cnt] = ( ( 0.6f / 64 ) + ( mesh.command[entry].x[cnt] * 62.8f / 64 ) ) / 4;
//            mesh.command[entry].v[cnt] = ( ( 0.6f / 64 ) + ( mesh.command[entry].v[cnt] * 62.8f / 64 ) ) / 4;
//            cnt++;
//        }
//
//        entry++;
//    }
//
//    // Make tile texture offsets
//    entry = 0;
//
//    while ( entry < MAXTILETYPE )
//    {
//        offx = ( entry & 7 ) / 8.0f;
//        offy = ( entry >> 3 ) / 8.0f;
//        mesh.tileoffu[entry] = offx;
//        mesh.tileoffv[entry] = offy;
//        entry++;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void make_fanstart()
//{
//    // ZZ> This function builds a look up table to ease calculating the
//    //     fan number given an x,y pair
//    int cnt;
//
//    // do the fanstart
//    for ( cnt = 0; cnt < mesh.tiles_y; cnt++ )
//    {
//        mesh.fanstart[cnt] = mesh.tiles_x * cnt;
//    }
//
//    // calculate some of the block info
//    mesh.blocksx = (mesh.tiles_x >> 2);
//    if( 0 != (mesh.tiles_x & 0x03) ) mesh.blocksx++;
//    if( mesh.blocksx >= MAXMESHBLOCKY )
//    {
//        log_warning( "Number of mesh blocks in the x direction too large (%d out of %d).\n", mesh.blocksx, MAXMESHBLOCKY );
//    }
//
//    mesh.blocksy = (mesh.tiles_y >> 2);
//    if( 0 != (mesh.tiles_y & 0x03) ) mesh.blocksy++;
//    if( mesh.blocksy >= MAXMESHBLOCKY )
//    {
//        log_warning( "Number of mesh blocks in the y direction too large (%d out of %d).\n", mesh.blocksy, MAXMESHBLOCKY );
//    }
//
//    mesh.blocks = mesh.blocksx * mesh.blocksy;
//
//    // do the blockstart
//    for ( cnt = 0; cnt < mesh.blocksy; cnt++ )
//    {
//        mesh.blockstart[cnt] = mesh.blocksx * cnt;
//    }
//
//}
//
//
////--------------------------------------------------------------------------------------------
//void make_vrtstart()
//{
//    int x, y, vert;
//    Uint32 fan;
//
//    vert = 0;
//    for ( y = 0; y < mesh.tiles_y; y++ )
//    {
//        for ( x = 0; x < mesh.tiles_x; x++ )
//        {
//            // allow raw access because we are careful
//            fan = mesh.fanstart[y] + x;
//            mesh.vrtstart[fan] = vert;
//            vert += mesh.commandnumvertices[mesh.type[fan]];
//        }
//    }
//}
//
////---------------------------------------------------------------------------------------------
//void make_lighttospek( void )
//{
//    // ZZ> This function makes a light table to fake directional lighting
//    int cnt, tnc;
//    Uint8 spek;
//    float fTmp, fPow;
//
//    // New routine
//    for ( cnt = 0; cnt < MAXSPEKLEVEL; cnt++ )
//    {
//        for ( tnc = 0; tnc < 256; tnc++ )
//        {
//            fTmp = tnc / 256.0f;
//            fPow = ( fTmp * 4.0f ) + 1;
//            fTmp = POW( fTmp, fPow );
//            fTmp = fTmp * cnt * 255.0f / MAXSPEKLEVEL;
//            if ( fTmp < 0 ) fTmp = 0;
//            if ( fTmp > 255 ) fTmp = 255;
//
//            spek = fTmp;
//            spek = spek >> 1;
//            lighttospek[cnt][tnc] = ( 0xff000000 ) | ( spek << 16 ) | ( spek << 8 ) | ( spek );
//        }
//    }
//}
//

////---------------------------------------------------------------------------------------------
//void make_lightdirectionlookup()
//{
//    // ZZ> This function builds the lighting direction table
//    //     The table is used to find which direction the light is coming
//    //     from, based on the four corner vertices of a mesh tile.
//    Uint32 cnt;
//    Uint16 tl, tr, br, bl;
//    int x, y;
//
//    for ( cnt = 0; cnt < 65536; cnt++ )
//    {
//        tl = ( cnt & 0xf000 ) >> 12;
//        tr = ( cnt & 0x0f00 ) >> 8;
//        br = ( cnt & 0x00f0 ) >> 4;
//        bl = ( cnt & 0x000f );
//        x = br + tr - bl - tl;
//        y = br + bl - tl - tr;
//        lightdirectionlookup[cnt] = ( ATAN2( -y, x ) + PI ) * 256 / ( TWO_PI );
//    }
//}
//


////---------------------------------------------------------------------------------------------
//float get_level( float x, float y, bool_t waterwalk )
//{
//    // ZZ> This function returns the height of a point within a mesh fan, precise
//    //     If waterwalk is nonzero and the fan is watery, then the level returned is the
//    //     level of the water.
//
//    Uint32 tile;
//    int ix, iy;
//
//    float z0, z1, z2, z3;         // Height of each fan corner
//    float zleft, zright, zdone;   // Weighted height of each side
//
//    tile = mesh_get_tile(x,y);
//    if( INVALID_TILE == tile ) return 0;
//
//    ix = x;
//    iy = y;
//
//    ix &= 127;
//    iy &= 127;
//
//    z0 = mesh.vrtz[ mesh.vrtstart[tile] + 0 ];
//    z1 = mesh.vrtz[ mesh.vrtstart[tile] + 1 ];
//    z2 = mesh.vrtz[ mesh.vrtstart[tile] + 2 ];
//    z3 = mesh.vrtz[ mesh.vrtstart[tile] + 3 ];
//
//    zleft = ( z0 * ( 128 - iy ) + z3 * iy ) / (float)(1 << 7);
//    zright = ( z1 * ( 128 - iy ) + z2 * iy ) / (float)(1 << 7);
//    zdone = ( zleft * ( 128 - ix ) + zright * ix ) / (float)(1 << 7);
//
//    if ( waterwalk )
//    {
//        if ( watersurfacelevel > zdone && 0 != ( mesh.fx[tile] & MPDFX_WATER ) && wateriswater )
//        {
//            return watersurfacelevel;
//        }
//    }
//
//    return zdone;
//}
//
//
//--------------------------------------------------------------------------------------------
//void make_renderlist()
//{
//    // ZZ> This function figures out which mesh fans to draw
//    int cnt, fan, fanx, fany;
//    int row, run, numrow;
//    int xlist[4], ylist[4];
//    int leftnum, leftlist[4];
//    int rightnum, rightlist[4];
//    int fanrowstart[128], fanrowrun[128];
//    int x, stepx, divx, basex;
//    int from, to;
//
//    // Clear old render lists
//    for ( cnt = 0; cnt < renderlist.all_count; cnt++ )
//    {
//        fan = renderlist.all[cnt];
//        mesh.inrenderlist[fan] = btrue;
//    }
//
//    renderlist.all_count = 0;
//    renderlist.ref_count = 0;
//    renderlist.sha_count = 0;
//
//    // Make sure it doesn't die ugly !!!BAD!!!
//
//    // It works better this way...
//    cornery[cornerlistlowtohighy[3]] += 256;
//
//    // Make life simpler
//    xlist[0] = cornerx[cornerlistlowtohighy[0]];
//    xlist[1] = cornerx[cornerlistlowtohighy[1]];
//    xlist[2] = cornerx[cornerlistlowtohighy[2]];
//    xlist[3] = cornerx[cornerlistlowtohighy[3]];
//    ylist[0] = cornery[cornerlistlowtohighy[0]];
//    ylist[1] = cornery[cornerlistlowtohighy[1]];
//    ylist[2] = cornery[cornerlistlowtohighy[2]];
//    ylist[3] = cornery[cornerlistlowtohighy[3]];
//
//    // Find the center line
//
//    divx = ylist[3] - ylist[0]; if ( divx < 1 ) return;
//
//    stepx = xlist[3] - xlist[0];
//    basex = xlist[0];
//
//    // Find the points in each edge
//    leftlist[0] = 0;  leftnum = 1;
//    rightlist[0] = 0;  rightnum = 1;
//    if ( xlist[1] < ( stepx*( ylist[1] - ylist[0] ) / divx ) + basex )
//    {
//        leftlist[leftnum] = 1;  leftnum++;
//        cornerx[1] -= 512;
//    }
//    else
//    {
//        rightlist[rightnum] = 1;  rightnum++;
//        cornerx[1] += 512;
//    }
//    if ( xlist[2] < ( stepx*( ylist[2] - ylist[0] ) / divx ) + basex )
//    {
//        leftlist[leftnum] = 2;  leftnum++;
//        cornerx[2] -= 512;
//    }
//    else
//    {
//        rightlist[rightnum] = 2;  rightnum++;
//        cornerx[2] += 512;
//    }
//
//    leftlist[leftnum] = 3;  leftnum++;
//    rightlist[rightnum] = 3;  rightnum++;
//
//    // Make the left edge ( rowstart )
//    fany = ylist[0] >> 7;
//    row = 0;
//    cnt = 1;
//
//    while ( cnt < leftnum )
//    {
//        from = leftlist[cnt-1];  to = leftlist[cnt];
//        x = xlist[from];
//        divx = ylist[to] - ylist[from];
//        stepx = 0;
//        if ( divx > 0 )
//        {
//            stepx = ( ( xlist[to] - xlist[from] ) << 7 ) / divx;
//        }
//
//        x -= 256;
//        run = ylist[to] >> 7;
//
//        while ( fany < run )
//        {
//            if ( fany >= 0 && fany < mesh.tiles_y )
//            {
//                fanx = x >> 7;
//                if ( fanx < 0 )  fanx = 0;
//                if ( fanx >= mesh.tiles_x )  fanx = mesh.tiles_x - 1;
//
//                fanrowstart[row] = fanx;
//                row++;
//            }
//
//            x += stepx;
//            fany++;
//        }
//
//        cnt++;
//    }
//
//    numrow = row;
//
//    // Make the right edge ( rowrun )
//    fany = ylist[0] >> 7;
//    row = 0;
//    cnt = 1;
//
//    while ( cnt < rightnum )
//    {
//        from = rightlist[cnt-1];  to = rightlist[cnt];
//        x = xlist[from];
//        // x+=128;
//        divx = ylist[to] - ylist[from];
//        stepx = 0;
//        if ( divx > 0 )
//        {
//            stepx = ( ( xlist[to] - xlist[from] ) << 7 ) / divx;
//        }
//
//        run = ylist[to] >> 7;
//
//        while ( fany < run )
//        {
//            if ( fany >= 0 && fany < mesh.tiles_y )
//            {
//                fanx = x >> 7;
//                if ( fanx < 0 )  fanx = 0;
//                if ( fanx >= mesh.tiles_x - 1 )  fanx = mesh.tiles_x - 1;//-2
//
//                fanrowrun[row] = ABS( fanx - fanrowstart[row] ) + 1;
//                row++;
//            }
//
//            x += stepx;
//            fany++;
//        }
//
//        cnt++;
//    }
//    if ( numrow != row )
//    {
//        log_error( "ROW error (%i, %i)\n", numrow, row );
//    }
//
//    // Fill 'em up again
//    fany = ylist[0] >> 7;
//    if ( fany < 0 ) fany = 0;
//    if ( fany >= mesh.tiles_y ) fany = mesh.tiles_y - 1;
//
//    row = 0;
//    while ( row < numrow )
//    {
//        // allow raw access because we have no choice
//        cnt = mesh.fanstart[fany] + fanrowstart[row];
//
//        run = fanrowrun[row];
//        fanx = 0;
//
//        while ( fanx < run )
//        {
//            if ( renderlist.all_count < MAXMESHRENDER )
//            {
//                // Put each tile in basic list
//                mesh.inrenderlist[cnt] = btrue;
//                renderlist.all[rlist_all_count] = cnt;
//                renderlist.all_count++;
//
//                // Put each tile in one other list, for shadows and relections
//                if ( 0 == mesh.fx[cnt] )
//                {
//                    renderlist.ref[renderlist.ref_count] = cnt;
//                    renderlist.ref_count++;
//                }
//
//                if ( 0 != ( mesh.fx[cnt] & MPDFX_SHA ) )
//                {
//                    renderlist.sha[renderlist.sha_count] = cnt;
//                    renderlist.sha_count++;
//                }
//
//                if ( 0 != ( mesh.fx[cnt] & MPDFX_DRAWREF ) )
//                {
//                    renderlist.dref[renderlist.dref_count] = cnt;
//                    renderlist.dref_count++;
//                }
//
//                if ( 0 != ( mesh.fx[cnt] & MPDFX_ANIM ) )
//                {
//                    renderlist.anim[renderlist.anim_count] = cnt;
//                    renderlist.anim_count++;
//                }
//
//                if ( 0 != ( mesh.fx[cnt] & MPDFX_WATER ) )
//                {
//                    renderlist.water[renderlist.water_count] = cnt;
//                    renderlist.water_count++;
//                }
//
//                if ( 0 != ( mesh.fx[cnt] & MPDFX_WALL ) )
//                {
//                    renderlist.wall[renderlist.wall_count] = cnt;
//                    renderlist.wall_count++;
//                }
//
//                if ( 0 != ( mesh.fx[cnt] & MPDFX_IMPASS ) )
//                {
//                    renderlist.impass[renderlist.impass_count] = cnt;
//                    renderlist.impass_count++;
//                }
//
//                if ( 0 != ( mesh.fx[cnt] & MPDFX_DAMAGE ) )
//                {
//                    renderlist.damage[renderlist.damage_count] = cnt;
//                    renderlist.damage_count++;
//                }
//
//                if ( 0 != ( mesh.fx[cnt] & MPDFX_SLIPPY ) )
//                {
//                    renderlist.slippy[renderlist.slippy_count] = cnt;
//                    renderlist.slippy_count++;
//                }
//            }
//
//            cnt++;
//            fanx++;
//        }
//
//        row++;
//        fany++;
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void figure_out_what_to_draw()
//{
//    // ZZ> This function determines the things that need to be drawn
//
//    // Find the render area corners
//    project_view();
//    // Make the render list for the mesh
//    make_renderlist();
//
//    camturnleftrightone = ( camturnleftright ) / ( TWO_PI );
//    camturnleftrightshort = camturnleftrightone * 65536;
//
//    // Request matrices needed for local machine
//    make_dolist();
//    order_dolist();
//}
//
////--------------------------------------------------------------------------------------------
//void animate_tiles()
//{
//    // This function changes the animated tile frame
//    if ( ( frame_wld & animtileupdateand ) == 0 )
//    {
//        animtileframeadd = ( animtileframeadd + 1 ) & animtileframeand;
//    }
//}

////--------------------------------------------------------------------------------------------
//Uint16 action_number()
//{
//    // ZZ> This function returns the number of the action in cFrameName, or
//    //     it returns NOACTION if it could not find a match
//    int cnt;
//    char first, second;
//
//    first = cFrameName[0];
//    second = cFrameName[1];
//
//    for ( cnt = 0; cnt < MAXACTION; cnt++ )
//    {
//        if ( first == cActionName[cnt][0] && second == cActionName[cnt][1] )
//        {
//            return cnt;
//        }
//    }
//
//    return NOACTION;
//}
//

////--------------------------------------------------------------------------------------------
//void make_water()
//{
//    // ZZ> This function sets up water movements
//    int layer, frame, point, mode, cnt;
//    float temp;
//    Uint8 spek;
//
//    layer = 0;
//
//    while ( layer < numwaterlayer )
//    {
//        if ( waterlight )  waterlayeralpha[layer] = 255;  // Some cards don't support alpha lights...
//
//        waterlayeru[layer] = 0;
//        waterlayerv[layer] = 0;
//        frame = 0;
//
//        while ( frame < MAXWATERFRAME )
//        {
//            // Do first mode
//            mode = 0;
//
//            for ( point = 0; point < WATERPOINTS; point++ )
//            {
//                temp = SIN( ( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( TWO_PI * layer / MAXWATERLAYER ) );
//                waterlayerzadd[layer][frame][mode][point] = temp * waterlayeramp[layer];
//                waterlayercolor[layer][frame][mode][point] = ( waterlightlevel[layer] * ( temp + 1.0f ) ) + waterlightadd[layer];
//            }
//
//            // Now mirror and copy data to other three modes
//            mode++;
//            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][1];
//            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][1];
//            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][0];
//            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][0];
//            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][3];
//            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][3];
//            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][2];
//            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][2];
//            mode++;
//            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][3];
//            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][3];
//            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][2];
//            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][2];
//            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][1];
//            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][1];
//            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][0];
//            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][0];
//            mode++;
//            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][2];
//            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][2];
//            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][3];
//            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][3];
//            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][0];
//            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][0];
//            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][1];
//            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][1];
//            frame++;
//        }
//
//        layer++;
//    }
//
//    // Calculate specular highlights
//    spek = 0;
//
//    for ( cnt = 0; cnt < 256; cnt++ )
//    {
//        spek = 0;
//        if ( cnt > waterspekstart )
//        {
//            temp = cnt - waterspekstart;
//            temp = temp / ( 256 - waterspekstart );
//            temp = temp * temp;
//            spek = temp * waterspeklevel;
//        }
//
//        // [claforte] Probably need to replace this with a
//        //            glColor4f(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
//        if ( shading == GL_FLAT )
//            waterspek[cnt] = 0;
//        else
//            waterspek[cnt] = spek;
//    }
//}

//--------------------------------------------------------------------------------------------
void read_wawalite( char *modname )
{
    // ZZ> This function sets up water and lighting for the module
    char newloadname[256];
    FILE* fileread;
    float lx, ly, lz, la;
    float fTmp;
    char cTmp;
    int iTmp;

    make_newloadname( modname, SLASH_STR "gamedat" SLASH_STR "wawalite.txt", newloadname );
    fileread = fopen( newloadname, "r" );
    if ( NULL == fileread )
    {
        log_error( "Could not read file! (wawalite.txt)\n" );
    }

    goto_colon( fileread );
    //  !!!BAD!!!
    //  Random map...
    //  If someone else wants to handle this, here are some thoughts for approaching
    //  it.  The .MPD file for the level should give the basic size of the map.  Use
    //  a standard tile set like the Palace modules.  Only use objects that are in
    //  the module's object directory, and only use some of them.  Imagine several Rock
    //  Moles eating through a stone filled level to make a path from the entrance to
    //  the exit.  Door placement will be difficult.
    //  !!!BAD!!!

    // Read water data first
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  numwaterlayer = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterspekstart = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterspeklevel = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterdouselevel = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  watersurfacelevel = iTmp;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    if ( cTmp == 'T' || cTmp == 't' )  waterlight = btrue;
    else waterlight = bfalse;

    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    wateriswater = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  wateriswater = btrue;

    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    if ( ( cTmp == 'T' || cTmp == 't' ) && cfg.overlayvalid ) overlayon = btrue;
    else overlayon = bfalse;

    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    if ( ( cTmp == 'T' || cTmp == 't' ) && cfg.backgroundvalid )  clearson = bfalse;
    else clearson = btrue;

    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdistx[0] = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdisty[0] = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdistx[1] = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayerdisty[1] = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  foregroundrepeat = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  backgroundrepeat = iTmp;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerz[0] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayeralpha[0] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerframeadd[0] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightlevel[0] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightadd[0] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeramp[0] = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeruadd[0] = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayervadd[0] = fTmp;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerz[1] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayeralpha[1] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlayerframeadd[1] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightlevel[1] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  waterlightadd[1] = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeramp[1] = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayeruadd[1] = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterlayervadd[1] = fTmp;

    waterlayeru[0] = 0;
    waterlayerv[0] = 0;
    waterlayeru[1] = 0;
    waterlayerv[1] = 0;
    waterlayerframe[0] = rand() & WATERFRAMEAND;
    waterlayerframe[1] = rand() & WATERFRAMEAND;
    // Read light data second
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  lx = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  ly = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  lz = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  la = fTmp;
    // Read tile data third
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  hillslide = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  slippyfriction = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  airfriction = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  waterfriction = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  noslipfriction = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  gravity = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  animtileupdateand = iTmp;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  animtileframeand = iTmp;
    biganimtileframeand = ( iTmp << 1 ) + 1;
    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  damagetileamount = iTmp;
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    if ( cTmp == 'S' || cTmp == 's' )  damagetiletype = DAMAGE_SLASH;
    if ( cTmp == 'C' || cTmp == 'c' )  damagetiletype = DAMAGE_CRUSH;
    if ( cTmp == 'P' || cTmp == 'p' )  damagetiletype = DAMAGE_POKE;
    if ( cTmp == 'H' || cTmp == 'h' )  damagetiletype = DAMAGE_HOLY;
    if ( cTmp == 'E' || cTmp == 'e' )  damagetiletype = DAMAGE_EVIL;
    if ( cTmp == 'F' || cTmp == 'f' )  damagetiletype = DAMAGE_FIRE;
    if ( cTmp == 'I' || cTmp == 'i' )  damagetiletype = DAMAGE_ICE;
    if ( cTmp == 'Z' || cTmp == 'z' )  damagetiletype = DAMAGE_ZAP;

    // Read weather data fourth
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    weatheroverwater = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  weatheroverwater = btrue;

    goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );  weathertimereset = iTmp;
    weathertime = weathertimereset;
    weatherplayer = 0;

    // Read extra data
    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    mesh.exploremode = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  mesh.exploremode = btrue;

    goto_colon( fileread );  cTmp = get_first_letter( fileread );
    usefaredge = bfalse;
    if ( cTmp == 'T' || cTmp == 't' )  usefaredge = btrue;

    cam.swing = 0;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  cam.swingrate = fTmp;
    goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  cam.swingamp = fTmp;

    // Read unnecessary data...  Only read if it exists...
    fogon = bfalse;
    fogaffectswater = btrue;
    fogtop = 100;
    fogbottom = 0;
    fogdistance = 100;
    fogred = 255;
    foggrn = 255;
    fogblu = 255;
    damagetileparttype = -1;
    damagetilepartand = 255;
    damagetilesound = -1;
    damagetilesoundtime = TILESOUNDTIME;
    damagetilemindistance = 9999;
    if ( goto_colon_yesno( fileread ) )
    {
        fogon = fogallowed;
        fscanf( fileread, "%f", &fTmp );  fogtop = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogbottom = fTmp;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogred = fTmp * 255;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  foggrn = fTmp * 255;
        goto_colon( fileread );  fscanf( fileread, "%f", &fTmp );  fogblu = fTmp * 255;
        goto_colon( fileread );  cTmp = get_first_letter( fileread );
        if ( cTmp == 'F' || cTmp == 'f' )  fogaffectswater = bfalse;

        fogdistance = ( fogtop - fogbottom );
        if ( fogdistance < 1.0f )  fogon = bfalse;

        // Read extra stuff for damage tile particles...
        if ( goto_colon_yesno( fileread ) )
        {
            fscanf( fileread, "%d", &iTmp );  damagetileparttype = iTmp;
            goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
            damagetilepartand = iTmp;
            goto_colon( fileread );  fscanf( fileread, "%d", &iTmp );
            damagetilesound = CLIP(iTmp, -1, MAXWAVE);
        }
    }

    // Allow slow machines to ignore the fancy stuff
    if ( !twolayerwateron && numwaterlayer > 1 )
    {
        numwaterlayer = 1;
        iTmp = waterlayeralpha[0];
        iTmp = FP8_MUL( waterlayeralpha[1], iTmp ) + iTmp;
        if ( iTmp > 255 ) iTmp = 255;

        waterlayeralpha[0] = iTmp;
    }

    fclose( fileread );
    // Do it
    make_lighttable( lx, ly, lz, la );
    make_lighttospek();
    make_water();


}

////--------------------------------------------------------------------------------------------
//void render_background( Uint16 texture )
//{
//    // ZZ> This function draws the large background
//    simple_vertex_t vtlist[4];
//    float size;
//    float sinsize, cossize;
//    float x, y, z, u, v;
//    float loc_backgroundrepeat;
//    Uint8 i;
//
//    // Figure out the coordinates of its corners
//    x = displaySurface->w << 6;
//    y = displaySurface->h << 6;
//    z = 0.99999f;
//    size = x + y + 1;
//    sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;   // why 3/8 of a turn???
//    cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;   // why 3/8 of a turn???
//    u = waterlayeru[1];
//    v = waterlayerv[1];
//    loc_backgroundrepeat = backgroundrepeat * MIN( x / displaySurface->w, y / displaySurface->h );
//
//    vtlist[0].x = x + cossize;
//    vtlist[0].y = y - sinsize;
//    vtlist[0].z = z;
//    vtlist[0].s = 0 + u;
//    vtlist[0].t = 0 + v;
//
//    vtlist[1].x = x + sinsize;
//    vtlist[1].y = y + cossize;
//    vtlist[1].z = z;
//    vtlist[1].s = loc_backgroundrepeat + u;
//    vtlist[1].t = 0 + v;
//
//    vtlist[2].x = x - cossize;
//    vtlist[2].y = y + sinsize;
//    vtlist[2].z = z;
//    vtlist[2].s = loc_backgroundrepeat + u;
//    vtlist[2].t = loc_backgroundrepeat + v;
//
//    vtlist[3].x = x - sinsize;
//    vtlist[3].y = y - cossize;
//    vtlist[3].z = z;
//    vtlist[3].s = 0 + u;
//    vtlist[3].t = loc_backgroundrepeat + v;
//
//    {
//        GLint shading_save, depthfunc_save;
//        GLboolean depthmask_save, cullface_save;
//
//        glGetIntegerv( GL_SHADE_MODEL, &shading_save );
//        glShadeModel( GL_FLAT );  // Flat shade this
//
//        depthmask_save = glIsEnabled( GL_DEPTH_WRITEMASK );
//        glDepthMask( GL_FALSE );
//
//        glGetIntegerv( GL_DEPTH_FUNC, &depthfunc_save );
//        glDepthFunc( GL_ALWAYS );
//
//        cullface_save = glIsEnabled( GL_CULL_FACE );
//        glDisable( GL_CULL_FACE );
//
//        glTexture_Bind( txTexture + texture );
//
//        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//        glBegin ( GL_TRIANGLE_FAN );
//        {
//            for ( i = 0; i < 4; i++ )
//            {
//                glTexCoord2f ( vtlist[i].s, vtlist[i].t );
//                glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
//            }
//        }
//        glEnd ();
//
//        glDepthFunc( depthfunc_save );
//        glDepthMask( depthmask_save );
//        glShadeModel(shading_save);
//        if (cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void render_foreground_overlay( Uint16 texture )
//{
//    // ZZ> This function draws the large foreground
//    simple_vertex_t vtlist[4];
//    int i;
//    float size;
//    float sinsize, cossize;
//    float x, y, z;
//    float u, v;
//    float loc_foregroundrepeat;
//
//    // Figure out the screen coordinates of its corners
//    x = displaySurface->w << 6;
//    y = displaySurface->h << 6;
//    z = 0;
//    u = waterlayeru[1];
//    v = waterlayerv[1];
//    size = x + y + 1;
//    sinsize = turntosin[( 3*2047 ) & TRIG_TABLE_MASK] * size;
//    cossize = turntocos[( 3*2047 ) & TRIG_TABLE_MASK] * size;
//    loc_foregroundrepeat = foregroundrepeat * MIN( x / displaySurface->w, y / displaySurface->h );
//
//    vtlist[0].x = x + cossize;
//    vtlist[0].y = y - sinsize;
//    vtlist[0].z = z;
//    vtlist[0].s = 0 + u;
//    vtlist[0].t = 0 + v;
//
//    vtlist[1].x = x + sinsize;
//    vtlist[1].y = y + cossize;
//    vtlist[1].z = z;
//    vtlist[1].s = loc_foregroundrepeat + u;
//    vtlist[1].t = v;
//
//    vtlist[2].x = x - cossize;
//    vtlist[2].y = y + sinsize;
//    vtlist[2].z = z;
//    vtlist[2].s = loc_foregroundrepeat + u;
//    vtlist[2].t = loc_foregroundrepeat + v;
//
//    vtlist[3].x = x - sinsize;
//    vtlist[3].y = y - cossize;
//    vtlist[3].z = z;
//    vtlist[3].s = 0 + u;
//    vtlist[3].t = loc_foregroundrepeat + v;
//
//    {
//        GLint shading_save, depthfunc_save, smoothhint_save;
//        GLboolean depthmask_save, cullface_save, alphatest_save;
//
//        GLint alphatestfunc_save, alphatestref_save, alphablendsrc_save, alphablenddst_save;
//        GLboolean alphablend_save;
//
//        glGetIntegerv(GL_POLYGON_SMOOTH_HINT, &smoothhint_save);
//        glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );             // make sure that the texture is as smooth as possible
//
//        glGetIntegerv( GL_SHADE_MODEL, &shading_save );
//        glShadeModel( GL_FLAT );  // Flat shade this
//
//        depthmask_save = glIsEnabled( GL_DEPTH_WRITEMASK );
//        glDepthMask( GL_FALSE );
//
//        glGetIntegerv( GL_DEPTH_FUNC, &depthfunc_save );
//        glDepthFunc( GL_ALWAYS );
//
//        cullface_save = glIsEnabled( GL_CULL_FACE );
//        glDisable( GL_CULL_FACE );
//
//        alphatest_save = glIsEnabled( GL_ALPHA_TEST );
//        glEnable( GL_ALPHA_TEST );
//
//        glGetIntegerv( GL_ALPHA_TEST_FUNC, &alphatestfunc_save );
//        glGetIntegerv( GL_ALPHA_TEST_REF, &alphatestref_save );
//        glAlphaFunc( GL_GREATER, 0 );
//
//        alphablend_save = glIsEnabled( GL_BLEND );
//        glEnable( GL_BLEND );
//
//        glGetIntegerv( GL_BLEND_SRC, &alphablendsrc_save );
//        glGetIntegerv( GL_BLEND_DST, &alphablenddst_save );
//        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // make the texture a filter
//
//        glTexture_Bind(txTexture + texture);
//
//        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//        glBegin ( GL_TRIANGLE_FAN );
//        {
//            for ( i = 0; i < 4; i++ )
//            {
//                glTexCoord2f ( vtlist[i].s, vtlist[i].t );
//                glVertex3f ( vtlist[i].x, vtlist[i].y, vtlist[i].z );
//            }
//        }
//        glEnd ();
//
//        glHint( GL_POLYGON_SMOOTH_HINT, smoothhint_save );
//        glShadeModel( shading_save );
//        glDepthMask( depthmask_save );
//        glDepthFunc( depthfunc_save );
//        if (cullface_save) glEnable( GL_CULL_FACE ); else glDisable( GL_CULL_FACE );
//        if (alphatest_save) glEnable( GL_ALPHA_TEST ); else glDisable( GL_ALPHA_TEST );
//
//        glAlphaFunc( alphatestfunc_save, alphatestref_save );
//        if (alphablend_save) glEnable( GL_BLEND ); else glDisable( GL_BLEND );
//
//        glBlendFunc( alphablendsrc_save, alphablenddst_save );
//    }
//}

////--------------------------------------------------------------------------------------------
//void render_shadow( int character )
//{
//    // ZZ> This function draws a NIFTY shadow
//    simple_vertex_t v[4];
//
//    float x, y;
//    float level;
//    float height, size_umbra, size_penumbra;
//    float alpha_umbra, alpha_penumbra;
//    Sint8 hide;
//    int i;
//
//    hide = caphidestate[chrmodel[character]];
//    if ( hide == NOHIDE || hide != chrai[character].state )
//    {
//        // Original points
//        level = chrlevel[character];
//        level += SHADOWRAISE;
//        height = chrmatrix[character].CNV( 3, 2 ) - level;
//        if ( height < 0 ) height = 0;
//
//        size_umbra    = 1.5f * ( chrbumpsize[character] - height / 30.0f );
//        size_penumbra = 1.5f * ( chrbumpsize[character] + height / 30.0f );
//        if ( height > 0 )
//        {
//            float factor_penumbra = ( 1.5f ) * ( ( chrbumpsize[character] ) / size_penumbra );
//            float factor_umbra = ( 1.5f ) * ( ( chrbumpsize[character] ) / size_umbra );
//            alpha_umbra = 0.3f / factor_umbra / factor_umbra / 1.5f;
//            alpha_penumbra = 0.3f / factor_penumbra / factor_penumbra / 1.5f;
//        }
//        else
//        {
//            alpha_umbra    = 0.3f;
//            alpha_penumbra = 0.3f;
//        };
//
//        x = chrmatrix[character].CNV( 3, 0 );
//
//        y = chrmatrix[character].CNV( 3, 1 );
//
//        // Choose texture.
//        glTexture_Bind( txTexture + particletexture );
//
//        // GOOD SHADOW
//        v[0].s = particleimageu[238][0];
//
//        v[0].t = particleimagev[238][0];
//
//        v[1].s = particleimageu[255][1];
//
//        v[1].t = particleimagev[238][0];
//
//        v[2].s = particleimageu[255][1];
//
//        v[2].t = particleimagev[255][1];
//
//        v[3].s = particleimageu[238][0];
//
//        v[3].t = particleimagev[255][1];
//
//        glEnable( GL_BLEND );
//
//        glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );
//
//        glDepthMask( GL_FALSE );
//        if ( size_penumbra > 0 )
//        {
//            v[0].x = x + size_penumbra;
//            v[0].y = y - size_penumbra;
//            v[0].z = level;
//
//            v[1].x = x + size_penumbra;
//            v[1].y = y + size_penumbra;
//            v[1].z = level;
//
//            v[2].x = x - size_penumbra;
//            v[2].y = y + size_penumbra;
//            v[2].z = level;
//
//            v[3].x = x - size_penumbra;
//            v[3].y = y - size_penumbra;
//            v[3].z = level;
//
//            glBegin( GL_TRIANGLE_FAN );
//            {
//                glColor4f( alpha_penumbra, alpha_penumbra, alpha_penumbra, 1.0f );
//
//                for ( i = 0; i < 4; i++ )
//                {
//                    glTexCoord2f ( v[i].s, v[i].t );
//                    glVertex3f ( v[i].x, v[i].y, v[i].z );
//                }
//            }
//            glEnd();
//        };
//        if ( size_umbra > 0 )
//        {
//            v[0].x = x + size_umbra;
//            v[0].y = y - size_umbra;
//            v[0].z = level + 0.1f;
//
//            v[1].x = x + size_umbra;
//            v[1].y = y + size_umbra;
//            v[1].z = level + 0.1f;
//
//            v[2].x = x - size_umbra;
//            v[2].y = y + size_umbra;
//            v[2].z = level + 0.1f;
//
//            v[3].x = x - size_umbra;
//            v[3].y = y - size_umbra;
//            v[3].z = level + 0.1f;
//
//            glBegin( GL_TRIANGLE_FAN );
//            glColor4f( alpha_umbra, alpha_umbra, alpha_umbra, 1.0f );
//
//            for ( i = 0; i < 4; i++ )
//            {
//                glTexCoord2f ( v[i].s, v[i].t );
//                glVertex3f ( v[i].x, v[i].y, v[i].z );
//            }
//
//            glEnd();
//        };
//
//        glDisable( GL_BLEND );
//
//        glDepthMask( GL_TRUE );
//    };
//}
//
////--------------------------------------------------------------------------------------------
//void render_bad_shadow( int character )
//{
//    // ZZ> This function draws a sprite shadow
//    simple_vertex_t v[4];
//    float size, x, y;
//    Uint8 ambi;
//    float level;
//    int height;
//    Sint8 hide;
//    Uint8 trans;
//    int i;
//
//    hide = caphidestate[chrmodel[character]];
//    if ( hide == NOHIDE || hide != chrai[character].state )
//    {
//        // Original points
//        level = chrlevel[character];
//        level += SHADOWRAISE;
//        height = chrmatrix[character].CNV( 3, 2 ) - level;
//        if ( height > 255 )  return;
//        if ( height < 0 ) height = 0;
//
//        size = chrshadowsize[character] - FP8_MUL( height, chrshadowsize[character] );
//        if ( size < 1 ) return;
//
//        ambi = chrlightlevel[character] >> 4;
//        trans = ( ( 255 - height ) >> 1 ) + 64;
//
//        glColor4f( ambi / 255.0f, ambi / 255.0f, ambi / 255.0f, trans / 255.0f );
//
//        x = chrmatrix[character].CNV( 3, 0 );
//        y = chrmatrix[character].CNV( 3, 1 );
//        v[0].x = ( float ) x + size;
//        v[0].y = ( float ) y - size;
//        v[0].z = ( float ) level;
//
//        v[1].x = ( float ) x + size;
//        v[1].y = ( float ) y + size;
//        v[1].z = ( float ) level;
//
//        v[2].x = ( float ) x - size;
//        v[2].y = ( float ) y + size;
//        v[2].z = ( float ) level;
//
//        v[3].x = ( float ) x - size;
//        v[3].y = ( float ) y - size;
//        v[3].z = ( float ) level;
//
//        // Choose texture and matrix
//        glTexture_Bind( txTexture + particletexture );
//
//        v[0].s = particleimageu[236][0];
//        v[0].t = particleimagev[236][0];
//
//        v[1].s = particleimageu[253][1];
//        v[1].t = particleimagev[236][0];
//
//        v[2].s = particleimageu[253][1];
//        v[2].t = particleimagev[253][1];
//
//        v[3].s = particleimageu[236][0];
//        v[3].t = particleimagev[253][1];
//
//        glBegin( GL_TRIANGLE_FAN );
//        {
//            for ( i = 0; i < 4; i++ )
//            {
//                glTexCoord2f ( v[i].s, v[i].t );
//                glVertex3f ( v[i].x, v[i].y, v[i].z );
//            }
//        }
//        glEnd();
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void light_characters()
//{
//    // ZZ> This function figures out character lighting
//    int cnt, tnc, x, y;
//    Uint16 tl, tr, bl, br;
//    Uint16 light;
//
//    for ( cnt = 0; cnt < dolist.count; cnt++ )
//    {
//        tnc = dolist.which[cnt];
//
//        x = chrxpos[tnc];
//        y = chrypos[tnc];
//        x = ( x & 127 ) >> 5;  // From 0 to 3
//        y = ( y & 127 ) >> 5;  // From 0 to 3
//
//        light = 0;
//
//        if( INVALID_TILE == chronwhichfan[tnc] )
//        {
//            tl = 0;
//            tr = 0;
//            br = 0;
//            bl = 0;
//        }
//        else
//        {
//            tl = mesh.vrtl[mesh.vrtstart[chronwhichfan[tnc]] + 0];
//            tr = mesh.vrtl[mesh.vrtstart[chronwhichfan[tnc]] + 1];
//            br = mesh.vrtl[mesh.vrtstart[chronwhichfan[tnc]] + 2];
//            bl = mesh.vrtl[mesh.vrtstart[chronwhichfan[tnc]] + 3];
//        }
//
//        // Interpolate lighting level using tile corners
//        switch ( x )
//        {
//            case 0:
//                light += tl << 1;
//                light += bl << 1;
//                break;
//            case 1:
//            case 2:
//                light += tl;
//                light += tr;
//                light += bl;
//                light += br;
//                break;
//            case 3:
//                light += tr << 1;
//                light += br << 1;
//                break;
//        }
//
//        switch ( y )
//        {
//            case 0:
//                light += tl << 1;
//                light += tr << 1;
//                break;
//            case 1:
//            case 2:
//                light += tl;
//                light += tr;
//                light += bl;
//                light += br;
//                break;
//            case 3:
//                light += bl << 1;
//                light += br << 1;
//                break;
//        }
//
//        light = light >> 3;
//        chrlightlevel[tnc] = light;
//        if ( !mesh.exploremode )
//        {
//            // Look up light direction using corners again
//            tl = ( tl << 8 ) & 0xf000;
//            tr = ( tr << 4 ) & 0x0f00;
//            br = ( br ) & 0x00f0;
//            bl = bl >> 4;
//            tl = tl | tr | br | bl;
//            chrlightturnleftright[tnc] = ( lightdirectionlookup[tl] << 8 );
//        }
//        else
//        {
//            chrlightturnleftright[tnc] = 0;
//        }
//    }
//}

////--------------------------------------------------------------------------------------------
//void light_particles()
//{
//    // ZZ> This function figures out particle lighting
//    int iprt;
//    int character;
//
//    for ( iprt = 0; iprt < maxparticles; iprt++ )
//    {
//        if ( !prton[iprt] ) continue;
//
//        character = prtattachedtocharacter[iprt];
//        if ( MAXCHR != character )
//        {
//            prtlight[iprt] = chrlightlevel[character];
//        }
//        else if ( INVALID_TILE == prtonwhichfan[iprt] )
//        {
//            prtlight[iprt] = 0;
//        }
//        else
//        {
//            int itmp = 0;
//            Uint32 itile = prtonwhichfan[iprt];
//
//            itmp += mesh.vrtl[mesh.vrtstart[itile] + 0];
//            itmp += mesh.vrtl[mesh.vrtstart[itile] + 1];
//            itmp += mesh.vrtl[mesh.vrtstart[itile] + 2];
//            itmp += mesh.vrtl[mesh.vrtstart[itile] + 3];
//
//            prtlight[iprt] = itmp / 4;
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void set_fan_light( int fanx, int fany, Uint16 particle )
//{
//    // ZZ> This function is a little helper, lighting the selected fan
//    //     with the chosen particle
//    float x, y;
//    int fan, vertex, lastvertex;
//    float level;
//    float light;
//
//    if ( fanx >= 0 && fanx < mesh.tiles_x && fany >= 0 && fany < mesh.tiles_y )
//    {
//        // allow raw access because we were careful
//        fan = fanx + mesh.fanstart[fany];
//
//        vertex = mesh.vrtstart[fan];
//        lastvertex = vertex + mesh.commandnumvertices[mesh.type[fan]];
//
//        while ( vertex < lastvertex )
//        {
//            light = mesh.vrta[vertex];
//            x = prtxpos[particle] - mesh.vrtx[vertex];
//            y = prtypos[particle] - mesh.vrty[vertex];
//            level = ( x * x + y * y ) / prtdynalightfalloff[particle];
//            level = 255 - level;
//            level = level * prtdynalightlevel[particle];
//            if ( level > light )
//            {
//                if ( level > 255 ) level = 255;
//
//                mesh.vrtl[vertex] = level;
//                mesh.vrta[vertex] = level;
//            }
//
//            vertex++;
//        }
//    }
//}
//
////--------------------------------------------------------------------------------------------
//void do_dynalight()
//{
//    // ZZ> This function does dynamic lighting of visible fans
//
//    int cnt, lastvertex, vertex, fan, entry, fanx, fany, addx, addy;
//    float x, y;
//    float level;
//    float light;
//
//    // Do each floor tile
//    if ( mesh.exploremode )
//    {
//        // Set base light level in explore mode...  Don't need to do every frame
//        if ( ( frame_all & 7 ) == 0 )
//        {
//            cnt = 0;
//
//            while ( cnt < maxparticles )
//            {
//                if ( prton[cnt] && prtdynalighton[cnt] )
//                {
//                    fanx = prtxpos[cnt];
//                    fany = prtypos[cnt];
//                    fanx = fanx >> 7;
//                    fany = fany >> 7;
//                    addy = -DYNAFANS;
//
//                    while ( addy <= DYNAFANS )
//                    {
//                        addx = -DYNAFANS;
//
//                        while ( addx <= DYNAFANS )
//                        {
//                            set_fan_light( fanx + addx, fany + addy, cnt );
//                            addx++;
//                        }
//
//                        addy++;
//                    }
//                }
//
//                cnt++;
//            }
//        }
//    }
//    else if ( shading != GL_FLAT )
//    {
//        // Add to base light level in normal mode
//        for ( entry = 0; entry < renderlist.all_count; entry++ )
//        {
//            fan = renderlist.all[entry];
//            if( INVALID_TILE == fan ) continue;
//
//            vertex = mesh.vrtstart[fan];
//            lastvertex = vertex + mesh.commandnumvertices[mesh.type[fan]];
//            while ( vertex < lastvertex )
//            {
//                // Do light particles
//                light = mesh.vrta[vertex];
//                cnt = 0;
//
//                while ( cnt < numdynalight )
//                {
//                    x = dynalightlistx[cnt] - mesh.vrtx[vertex];
//                    y = dynalightlisty[cnt] - mesh.vrty[vertex];
//                    level = ( x * x + y * y ) / dynalightfalloff[cnt];
//                    level = 255 - level;
//                    if ( level > 0 )
//                    {
//                        light += level * dynalightlevel[cnt];
//                    }
//
//                    cnt++;
//                }
//                if ( light > 255 ) light = 255;
//                if ( light < 0 ) light = 0;
//
//                mesh.vrtl[vertex] = light;
//                vertex++;
//            }
//        }
//    }
//}
//
//--------------------------------------------------------------------------------------------
void render_water()
{
    // ZZ> This function draws all of the water fans

    int cnt;

    // Bottom layer first
    if ( numwaterlayer > 1 && waterlayerz[1] > -waterlayeramp[1] )
    {
        cnt = 0;

        while ( cnt < renderlist.water_count )
        {
            render_water_fan( renderlist.water[cnt], 1 );
            cnt++;
        }
    }

    // Top layer second
    if ( numwaterlayer > 0 && waterlayerz[0] > -waterlayeramp[0] )
    {
        cnt = 0;

        while ( cnt < renderlist.water_count )
        {
            render_water_fan( renderlist.water[cnt], 0 );
            cnt++;
        }
    }
}


//--------------------------------------------------------------------------------------------
void draw_scene_sadreflection()
{
    // ZZ> This function draws 3D objects
    Uint16 cnt, tnc;
    Uint8 trans;


    // ZB> Clear the z-buffer
    glClear( GL_DEPTH_BUFFER_BIT );

    // Render the reflective floors
    glEnable( GL_CULL_FACE );
    glFrontFace( GL_CW );

    for ( cnt = 0; cnt < renderlist.ref_count; cnt++ )
    {
        render_fan( renderlist.ref[cnt] );
    }

    if ( cfg.refon )
    {
        // Render reflections of characters

        glEnable( GL_CULL_FACE );
        glFrontFace( GL_CCW );

        glDisable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );

        for ( cnt = 0; cnt < dolist.count; cnt++ )
        {
            tnc = dolist.which[cnt];
            //if( INVALID_TILE != chronwhichfan[tnc] && 0 != ( mesh.fx[chronwhichfan[tnc]] & MPDFX_DRAWREF ) )
            //{
            //    render_refmad( tnc, FP8_MUL( chralpha[tnc], chrlight[tnc] ) );
            //}
        }

        // Render the reflected sprites
        glFrontFace( GL_CW );
        //render_refprt();

        glDisable( GL_BLEND );
        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );
    }

    // Render the shadow floors
    for ( cnt = 0; cnt < renderlist.sha_count; cnt++ )
    {
        render_fan( renderlist.sha[cnt] );
    }

    // Render the shadows
    if ( cfg.shaon )
    {
        if ( cfg.shasprite )
        {
            // Bad shadows
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            for ( cnt = 0; cnt < dolist.count; cnt++ )
            {
                tnc = dolist.which[cnt];
                //if ( chrattachedto[tnc] == MAXCHR )
                //{
                //    if ( ( ( chrlight[tnc] == 255 && chralpha[tnc] == 255 ) || capforceshadow[chrmodel[tnc]] ) && chrshadowsize[tnc] != 0 )
                //    {
                //        render_bad_shadow( tnc );
                //    }
                //}
            }

            glDisable( GL_BLEND );
            glDepthMask( GL_TRUE );
        }
        else
        {
            // Good shadows for me
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_COLOR, GL_ZERO );

            for ( cnt = 0; cnt < dolist.count; cnt++ )
            {
                tnc = dolist.which[cnt];
                //if ( chrattachedto[tnc] == MAXCHR )
                //{
                //    if ( ( ( chrlight[tnc] == 255 && chralpha[tnc] == 255 ) || capforceshadow[chrmodel[tnc]] ) && chrshadowsize[tnc] != 0 )
                //    {
                //        render_shadow( tnc );
                //    }
                //}
            }

            glDisable( GL_BLEND );
            glDepthMask( GL_TRUE );
        }
    }

    glAlphaFunc( GL_GREATER, 0 );
    glEnable( GL_ALPHA_TEST );
    glDisable( GL_CULL_FACE );

    // Render the normal characters
    for ( cnt = 0; cnt < dolist.count; cnt++ )
    {
        tnc = dolist.which[cnt];
        //if ( chralpha[tnc] == 255 && chrlight[tnc] == 255 )
        //    render_mad( tnc, 255 );
    }

    // Render the sprites
    glDepthMask( GL_FALSE );
    glEnable( GL_BLEND );

    // Now render the transparent characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for ( cnt = 0; cnt < dolist.count; cnt++ )
    {
        tnc = dolist.which[cnt];
        //if ( chralpha[tnc] != 255 && chrlight[tnc] == 255 )
        //{
        //    trans = chralpha[tnc];
        //    if ( trans < SEEINVISIBLE && ( local_seeinvisible || chrislocalplayer[tnc] ) )  trans = SEEINVISIBLE;

        //    render_mad( tnc, trans );
        //}
    }

    // Alpha water
    //if ( !cfg.waterlight ) render_water();
    render_water();

    // Then do the light characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for ( cnt = 0; cnt < dolist.count; cnt++ )
    {
        tnc = dolist.which[cnt];
        //if ( chrlight[tnc] != 255 )
        //{
        //    trans = chrlight[tnc];
        //    if ( trans < SEEINVISIBLE && ( local_seeinvisible || chrislocalplayer[tnc] ) )  trans = SEEINVISIBLE;

        //    render_mad( tnc, trans );
        //}

        //// Do phong highlights
        //if ( phongon && chralpha[tnc] == 255 && chrlight[tnc] == 255 && !chrenviro[tnc] && chrsheen[tnc] > 0 )
        //{
        //    Uint16 texturesave;
        //    chrenviro[tnc] = btrue;
        //    texturesave = chrtexture[tnc];
        //    chrtexture[tnc] = TX_PHONG;  // The phong map texture...
        //    render_mad( tnc, chrsheen[tnc] << 4 );
        //    chrtexture[tnc] = texturesave;
        //    chrenviro[tnc] = bfalse;
        //}
    }

    // Light water
    //if ( cfg.waterlight )  render_water();

    // Turn Z buffer back on, alphablend off
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    //render_prt();
    glDisable( GL_ALPHA_TEST );

    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );

    // Done rendering
}

//--------------------------------------------------------------------------------------------
void draw_scene_zreflection()
{
    // ZZ> This function draws 3D objects
    Uint16 cnt, tnc;
    Uint8 trans;

    // Clear the image if need be
    // PORT: I don't think this is needed if(clearson) { clear_surface(lpDDSBack); }
    // Zbuffer is cleared later

    // Render the reflective floors
    glDisable( GL_DEPTH_TEST );
    glDepthMask( GL_FALSE );

    for ( cnt = 0; cnt < renderlist.ref_count; cnt++ )
    {
        render_fan( renderlist.ref[cnt] );
    }

    // BAD: DRAW SHADOW STUFF TOO
    for ( cnt = 0; cnt < renderlist.sha_count; cnt++ )
    {
        render_fan( renderlist.sha[cnt] );
    }

    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );
    if ( cfg.refon )
    {
        // Render reflections of characters
        glFrontFace( GL_CCW );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glDepthFunc( GL_LEQUAL );

        for ( cnt = 0; cnt < dolist.count; cnt++ )
        {
            tnc = dolist.which[cnt];
            //if ( INVALID_TILE != chronwhichfan[tnc] && 0 != ( mesh.fx[chronwhichfan[tnc]]&MPDFX_DRAWREF ) )
            //{
            //    render_refmad( tnc, FP8_MUL( chralpha[tnc], chrlight[tnc] ) );
            //}
        }

        // [claforte] I think this is wrong... I think we should choose some other depth func.
        glDepthFunc( GL_ALWAYS );

        // Render the reflected sprites
        glDisable( GL_DEPTH_TEST );
        glDepthMask( GL_FALSE );
        glFrontFace( GL_CW );
        //render_refprt();

        glDisable( GL_BLEND );
        glDepthFunc( GL_LEQUAL );
        glEnable( GL_DEPTH_TEST );
        glDepthMask( GL_TRUE );
    }

    // Clear the Zbuffer at a bad time...  But hey, reflections work with Voodoo
    // lpD3DVViewport->Clear(1, &rect, D3DCLEAR_ZBUFFER);
    // Not sure if this is cool or not - DDOI
    // glClear ( GL_DEPTH_BUFFER_BIT );

    // Render the shadow floors
    for ( cnt = 0; cnt < renderlist.sha_count; cnt++ )
    {
        render_fan( renderlist.sha[cnt] );
    }

    // Render the shadows
    if ( cfg.shaon )
    {
        if ( cfg.shasprite )
        {
            // Bad shadows
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            for ( cnt = 0; cnt < dolist.count; cnt++ )
            {
                tnc = dolist.which[cnt];
                //if ( chrattachedto[tnc] == MAXCHR )
                //{
                //    if ( ( ( chrlight[tnc] == 255 && chralpha[tnc] == 255 ) || capforceshadow[chrmodel[tnc]] ) && chrshadowsize[tnc] != 0 )
                //        render_bad_shadow( tnc );
                //}
            }

            glDisable( GL_BLEND );
            glDepthMask( GL_TRUE );
        }
        else
        {
            // Good shadows for me
            glDepthMask( GL_FALSE );
            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_COLOR, GL_ZERO );

            for ( cnt = 0; cnt < dolist.count; cnt++ )
            {
                tnc = dolist.which[cnt];
                //if ( chrattachedto[tnc] == MAXCHR )
                //{
                //    if ( ( ( chrlight[tnc] == 255 && chralpha[tnc] == 255 ) || capforceshadow[chrmodel[tnc]] ) && chrshadowsize[tnc] != 0 )
                //        render_shadow( tnc );
                //}
            }

            glDisable( GL_BLEND );
            glDepthMask ( GL_TRUE );
        }
    }

    glAlphaFunc( GL_GREATER, 0 );
    glEnable( GL_ALPHA_TEST );

    // Render the normal characters
    for ( cnt = 0; cnt < dolist.count; cnt++ )
    {
        tnc = dolist.which[cnt];
        //if ( chralpha[tnc] == 255 && chrlight[tnc] == 255 )
        //    render_mad( tnc, 255 );
    }

    // Render the sprites
    glDepthMask ( GL_FALSE );
    glEnable( GL_BLEND );

    // Now render the transparent characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for ( cnt = 0; cnt < dolist.count; cnt++ )
    {
        tnc = dolist.which[cnt];
        //if ( chralpha[tnc] != 255 && chrlight[tnc] == 255 )
        //{
        //    trans = chralpha[tnc];
        //    if ( trans < SEEINVISIBLE && ( local_seeinvisible || chrislocalplayer[tnc] ) )  trans = SEEINVISIBLE;

        //    render_mad( tnc, trans );
        //}
    }

    // And alpha water floors
    //if ( !waterlight ) render_water();
    render_water();

    // Then do the light characters
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for ( cnt = 0; cnt < dolist.count; cnt++ )
    {
        tnc = dolist.which[cnt];
        //if ( chrlight[tnc] != 255 )
        //{
        //    trans = chrlight[tnc];
        //    if ( trans < SEEINVISIBLE && ( local_seeinvisible || chrislocalplayer[tnc] ) )  trans = SEEINVISIBLE;

        //    render_mad( tnc, trans );
        //}

        //// Do phong highlights
        //if ( phongon && chralpha[tnc] == 255 && chrlight[tnc] == 255 && !chrenviro[tnc] && chrsheen[tnc] > 0 )
        //{
        //    Uint16 texturesave;
        //    chrenviro[tnc] = btrue;
        //    texturesave = chrtexture[tnc];
        //    chrtexture[tnc] = TX_PHONG;  // The phong map texture...
        //    render_mad( tnc, chrsheen[tnc] << 4 );
        //    chrtexture[tnc] = texturesave;
        //    chrenviro[tnc] = bfalse;
        //}
    }

    // Do light water
    //if ( waterlight ) render_water();

    // Turn Z buffer back on, alphablend off
    glDepthMask( GL_TRUE );
    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    //render_prt();
    glDisable( GL_ALPHA_TEST );

    glDepthMask( GL_TRUE );

    glDisable( GL_BLEND );

    // Done rendering
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block( float pos_x, float pos_y )
{
    Uint32 block = INVALID_BLOCK;

    if ( pos_x >= 0.0f && pos_x < mesh.edgex && pos_y >= 0.0f && pos_y < mesh.edgey )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= 9;
        iy >>= 9;

        if ( iy < MAXMESHBLOCKY )
        {
            block = ix + mesh.blockstart[ iy ];
        }
    }

    return block;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile( float pos_x, float pos_y )
{
    Uint32 tile = INVALID_TILE;

    if ( pos_x >= 0.0f && pos_x < mesh.edgex && pos_y >= 0.0f && pos_y < mesh.edgey )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= 7;
        iy >>= 7;

        if ( iy < MAXMESHTILEY )
        {
            tile = ix + mesh.fanstart[ iy ];
        }
    }

    return tile;
}


//--------------------------------------------------------------------------------------------
void draw_scene()
{

    //make_prtlist();
    //do_dynalight();
    //light_characters();
    //light_particles();

    // clear the depth buffer
    glClear( GL_DEPTH_BUFFER_BIT );

    // Clear the image if need be
    //if ( cfg.clearson )
    //{
    glClear( GL_COLOR_BUFFER_BIT );
    //}
    //else
    //{
    //    // Render the background
    //    render_background( TX_WATER_LOW );  // 6 is the texture for waterlow.bmp
    //}

    if ( cfg.zreflect ) // DO REFLECTIONS
        draw_scene_zreflection();
    else
        draw_scene_sadreflection();

    // clear the depth buffer
    glClear( GL_DEPTH_BUFFER_BIT );

    // Foreground overlay
    //if ( cfg.overlayon )
    //{
    //    render_foreground_overlay( TX_WATER_TOP );  // Texture 5 is watertop.bmp
    //}
}

//---------------------------------------------------------------------------------------------
bool_t dump_screenshot()
{
    // dumps the current screen (GL context) to a new bitmap file
    // right now it dumps it to whatever the current directory is

    // returns btrue if successful, bfalse otherwise

    int i;
    FILE *test;
    bool_t savefound = bfalse;
    bool_t saved     = bfalse;
    char szFilename[100];

    SDL_Surface * displaySurface;

    // find a valid file name
    savefound = bfalse;
    i = 0;
    while ( !savefound && ( i < 100 ) )
    {
        sprintf( szFilename, "ego%02d.bmp", i );

        // lame way of checking if the file already exists...
        test = fopen( szFilename, "rb" );
        if ( test != NULL )
        {
            fclose( test );
            i++;
        }
        else
        {
            savefound = btrue;
        }
    }
    if ( !savefound ) return bfalse;

    // grab the current screen
    displaySurface = SDL_GetVideoSurface();

    // if we are not using OpenGl, jsut dump the screen
    if ( 0 == (displaySurface->flags & SDL_OPENGL) )
    {
        SDL_SaveBMP(displaySurface, szFilename);
        return bfalse;
    }

    // we ARE using OpenGL
    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        SDL_Surface *temp;

        // create a SDL surface
        temp = SDL_CreateRGBSurface( SDL_SWSURFACE, displaySurface->w, displaySurface->h, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                     0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
                                     0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
                                   );
        if ( temp == NULL ) return bfalse;
        if ( -1 != SDL_LockSurface( temp ) )
        {
            SDL_Rect rect;

            memcpy( &rect, &(displaySurface->clip_rect), sizeof(SDL_Rect) );
            if ( 0 == rect.w && 0 == rect.h )
            {
                rect.w = displaySurface->w;
                rect.h = displaySurface->h;
            }
            if ( rect.w > 0 && rect.h > 0 )
            {
                int y;
                Uint8 * pixels;

                glGetError();

                //// use the allocated screen to tell OpenGL about the row length (including the lapse) in pixels
                //// stolen from SDL ;)
                //glPixelStorei( GL_UNPACK_ROW_LENGTH, temp->pitch / temp->format->BytesPerPixel );
                //assert( GL_NO_ERROR == glGetError() );

                //// since we have specified the row actual length and will give a pointer to the actual pixel buffer,
                //// it is not necesssaty to mess with the alignment
                //glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
                //assert( GL_NO_ERROR == glGetError() );

                // ARGH! Must copy the pixels row-by-row, since the OpenGL video memory is flipped vertically
                // relative to the SDL Screen memory

                // this is supposed to be a DirectX thing, so it needs to be tested out on glx
                // there should probably be [SCREENSHOT_INVERT] and [SCREENSHOT_VALID] keys in setup.txt
                pixels = (Uint8 *)temp->pixels;
                for (y = rect.y; y < rect.y + rect.h; y++)
                {
                    glReadPixels(rect.x, (rect.h - y) - 1, rect.w, 1, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                    pixels += temp->pitch;
                }
                assert( GL_NO_ERROR == glGetError() );
            }

            SDL_UnlockSurface( temp );

            // Save the file as a .bmp
            saved = ( -1 != SDL_SaveBMP( temp, szFilename ) );
        }

        // free the SDL surface
        SDL_FreeSurface( temp );
    }
    glPopClientAttrib();

    return savefound;
}

//---------------------------------------------------------------------------------------------
void make_lighttable( float lx, float ly, float lz, float ambi )
{
    // ZZ> This function makes a light table to fake directional lighting
    int lev, cnt, tnc;
    int itmp, itmptwo;

    // Build a lookup table for sin/cos
    for ( cnt = 0; cnt < MAXLIGHTROTATION; cnt++ )
    {
        sinlut[cnt] = sin( TWO_PI * cnt / MAXLIGHTROTATION );
        coslut[cnt] = cos( TWO_PI * cnt / MAXLIGHTROTATION );
    }

    for ( cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++ )  // Spikey mace
    {
        for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
        {
            lev = MAXLIGHTLEVEL - 1;
            itmp = ( 255 * light_for_normal( tnc,
                                             cnt,
                                             lx * lev / MAXLIGHTLEVEL,
                                             ly * lev / MAXLIGHTLEVEL,
                                             lz * lev / MAXLIGHTLEVEL,
                                             ambi ) );

            // This creates the light value for each level entry
            while ( lev >= 0 )
            {
                itmptwo = ( ( ( lev * itmp / ( MAXLIGHTLEVEL - 1 ) ) ) );
                if ( itmptwo > 255 )  itmptwo = 255;

                lighttable[lev][tnc][cnt] = ( Uint8 ) itmptwo;
                lev--;
            }
        }
    }

    // Fill in index number 162 for the spike mace
    for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
    {
        lev = MAXLIGHTLEVEL - 1;
        itmp = 255;

        // This creates the light value for each level entry
        while ( lev >= 0 )
        {
            itmptwo = ( ( ( lev * itmp / ( MAXLIGHTLEVEL - 1 ) ) ) );
            if ( itmptwo > 255 )  itmptwo = 255;

            lighttable[lev][tnc][cnt] = ( Uint8 ) itmptwo;
            lev--;
        }
    }
}

//---------------------------------------------------------------------------------------------
void make_lighttospek( void )
{
    // ZZ> This function makes a light table to fake directional lighting
    int cnt, tnc;
    Uint8 spek;
    float fTmp, fPow;

    // New routine
    for ( cnt = 0; cnt < MAXSPEKLEVEL; cnt++ )
    {
        for ( tnc = 0; tnc < 256; tnc++ )
        {
            fTmp = tnc / 256.0f;
            fPow = ( fTmp * 4.0f ) + 1;
            fTmp = pow( fTmp, fPow );
            fTmp = fTmp * cnt * 255.0f / MAXSPEKLEVEL;
            if ( fTmp < 0 ) fTmp = 0;
            if ( fTmp > 255 ) fTmp = 255;

            spek = fTmp;
            spek = spek >> 1;
            lighttospek[cnt][tnc] = ( 0xff000000 ) | ( spek << 16 ) | ( spek << 8 ) | ( spek );
        }
    }
}


//--------------------------------------------------------------------------------------------
void make_water()
{
    // ZZ> This function sets up water movements
    int layer, frame, point, mode, cnt;
    float temp;
    Uint8 spek;

    layer = 0;

    while ( layer < numwaterlayer )
    {
        if ( waterlight )  waterlayeralpha[layer] = 255;  // Some cards don't support alpha lights...

        waterlayeru[layer] = 0;
        waterlayerv[layer] = 0;
        frame = 0;

        while ( frame < MAXWATERFRAME )
        {
            // Do first mode
            mode = 0;

            for ( point = 0; point < WATERPOINTS; point++ )
            {
                temp = sin( ( frame * TWO_PI / MAXWATERFRAME ) + ( TWO_PI * point / WATERPOINTS ) + ( TWO_PI * layer / MAXWATERLAYER ) );
                waterlayerzadd[layer][frame][mode][point] = temp * waterlayeramp[layer];
                waterlayercolor[layer][frame][mode][point] = ( waterlightlevel[layer] * ( temp + 1.0f ) ) + waterlightadd[layer];
            }

            // Now mirror and copy data to other three modes
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][1];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][0];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][2];
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][2];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][1];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][0];
            mode++;
            waterlayerzadd[layer][frame][mode][0] = waterlayerzadd[layer][frame][0][2];
            waterlayercolor[layer][frame][mode][0] = waterlayercolor[layer][frame][0][2];
            waterlayerzadd[layer][frame][mode][1] = waterlayerzadd[layer][frame][0][3];
            waterlayercolor[layer][frame][mode][1] = waterlayercolor[layer][frame][0][3];
            waterlayerzadd[layer][frame][mode][2] = waterlayerzadd[layer][frame][0][0];
            waterlayercolor[layer][frame][mode][2] = waterlayercolor[layer][frame][0][0];
            waterlayerzadd[layer][frame][mode][3] = waterlayerzadd[layer][frame][0][1];
            waterlayercolor[layer][frame][mode][3] = waterlayercolor[layer][frame][0][1];
            frame++;
        }

        layer++;
    }

    // Calculate specular highlights
    spek = 0;

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        spek = 0;
        if ( cnt > waterspekstart )
        {
            temp = cnt - waterspekstart;
            temp = temp / ( 256 - waterspekstart );
            temp = temp * temp;
            spek = temp * waterspeklevel;
        }

        // [claforte] Probably need to replace this with a
        //            glColor4f(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
        if ( shading == GL_FLAT )
            waterspek[cnt] = 0;
        else
            waterspek[cnt] = spek;
    }
}

//---------------------------------------------------------------------------------------------
float light_for_normal( int rotation, int normal, float lx, float ly, float lz, float ambi )
{
    // ZZ> This function helps make_lighttable
    float fTmp;
    float nx, ny, nz;
    float sinrot, cosrot;

    nx = kMd2Normals[normal][0];
    ny = kMd2Normals[normal][1];
    nz = kMd2Normals[normal][2];
    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];
    fTmp = cosrot * nx + sinrot * ny;
    ny = cosrot * ny - sinrot * nx;
    nx = fTmp;
    fTmp = nx * lx + ny * ly + nz * lz + ambi;
    if ( fTmp < ambi ) fTmp = ambi;

    return fTmp;
}

