#include "cartman_gfx.h"

#include "Log.h"

#include "egoboo_graphic.h"

#include "cartman.h"
#include "cartman_mpd.h"
#include "cartman_gui.h"
#include "cartman_functions.h"
#include "cartman_math.h"

#include "SDL_Pixel.h"

#include <SDL.h>
#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Sint16          damagetileparttype;
short           damagetilepartand;
short           damagetilesound;
short           damagetilesoundtime;
Uint16          damagetilemindistance;
int             damagetileamount = 256;                           // Amount of damage
Uint8           damagetiletype  = DAMAGE_FIRE;                      // Type of damage

int    animtileupdateand   = 7;
Uint16 animtileframeand    = 3;
Uint16 animtilebaseand     = ( Uint16 )( ~3 );
Uint16 biganimtileframeand = 7;
Uint16 biganimtilebaseand  = ( Uint16 )( ~7 );
Uint16 animtileframeadd    = 0;

SDLX_video_parameters_t sdl_vparam;
oglx_video_parameters_t ogl_vparam;

SDL_Surface * theSurface = NULL;
SDL_Surface * bmphitemap = NULL;        // Heightmap image

glTexture     tx_point;      // Vertex image
glTexture     tx_pointon;    // Vertex image ( select_vertsed )
glTexture     tx_ref;        // Meshfx images
glTexture     tx_drawref;    //
glTexture     tx_anim;       //
glTexture     tx_water;      //
glTexture     tx_wall;       //
glTexture     tx_impass;     //
glTexture     tx_damage;     //
glTexture     tx_slippy;     //

glTexture     tx_smalltile[MAXTILE]; // Tiles
glTexture     tx_bigtile[MAXTILE];   //
glTexture     tx_tinysmalltile[MAXTILE]; // Plan tiles
glTexture     tx_tinybigtile[MAXTILE];   //

int     numsmalltile = 0;   //
int     numbigtile = 0;     //

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void get_small_tiles( SDL_Surface* bmpload );
static void get_big_tiles( SDL_Surface* bmpload );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

SDL_Color MAKE_SDLCOLOR( Uint8 BB, Uint8 RR, Uint8 GG )
{
    SDL_Color tmp;

    tmp.r = RR << 3;
    tmp.g = GG << 3;
    tmp.b = BB << 3;

    return tmp;
}

//--------------------------------------------------------------------------------------------
glTexture * tiny_tile_at( int x, int y )
{
    Uint16 tx_bits, basetile;
    Uint8 fantype, fx;
    int fan;

    if ( x < 0 || x >= mesh.tiles_x || y < 0 || y >= mesh.tiles_y )
    {
        return NULL;
    }

    tx_bits = 0;
    fantype = 0;
    fx = 0;
    fan = mesh_get_fan( x, y );
    if ( -1 != fan )
    {
        if ( TILE_IS_FANOFF( mesh.tx_bits[fan] ) )
        {
            return NULL;
        }
        tx_bits = mesh.tx_bits[fan];
        fantype = mesh.fantype[fan];
        fx      = mesh.fx[fan];
    }

    if ( HAS_BITS( fx, MPDFX_ANIM ) )
    {
        animtileframeadd = ( timclock >> 3 ) & 3;
        if ( fantype >= ( MAXMESHTYPE >> 1 ) )
        {
            // Big tiles
            basetile = tx_bits & biganimtilebaseand;// Animation set
            tx_bits += ( animtileframeadd << 1 );   // Animated tx_bits
            tx_bits = ( tx_bits & biganimtileframeand ) + basetile;
        }
        else
        {
            // Small tiles
            basetile = tx_bits & animtilebaseand;// Animation set
            tx_bits += animtileframeadd;       // Animated tx_bits
            tx_bits = ( tx_bits & animtileframeand ) + basetile;
        }
    }

    // remove any of the upper bit information
    tx_bits &= 0xFF;

    if ( fantype >= ( MAXMESHTYPE >> 1 ) )
    {
        return tx_bigtile + tx_bits;
    }
    else
    {
        return tx_smalltile + tx_bits;
    }
}

//--------------------------------------------------------------------------------------------
glTexture *tile_at( int fan )
{
    Uint16 tx_bits, basetile;
    Uint8 fantype, fx;

    if ( fan == -1 || TILE_IS_FANOFF( mesh.tx_bits[fan] ) )
    {
        return NULL;
    }

    tx_bits = mesh.tx_bits[fan];
    fantype = mesh.fantype[fan];
    fx = mesh.fx[fan];
    if ( HAS_BITS( fx, MPDFX_ANIM ) )
    {
        animtileframeadd = ( timclock >> 3 ) & 3;
        if ( fantype >= ( MAXMESHTYPE >> 1 ) )
        {
            // Big tiles
            basetile = tx_bits & biganimtilebaseand;// Animation set
            tx_bits += ( animtileframeadd << 1 );   // Animated tx_bits
            tx_bits = ( tx_bits & biganimtileframeand ) + basetile;
        }
        else
        {
            // Small tiles
            basetile = tx_bits & animtilebaseand;  // Animation set
            tx_bits += animtileframeadd;           // Animated tx_bits
            tx_bits = ( tx_bits & animtileframeand ) + basetile;
        }
    }

    // remove any of the upper bit information
    tx_bits &= 0xFF;

    if ( fantype >= ( MAXMESHTYPE >> 1 ) )
    {
        return tx_bigtile + tx_bits;
    }
    else
    {
        return tx_smalltile + tx_bits;
    }
}

//--------------------------------------------------------------------------------------------
void make_hitemap( void )
{
    int x, y, pixx, pixy, level, fan;

    if ( bmphitemap ) SDL_FreeSurface( bmphitemap );

    bmphitemap = cartman_CreateSurface( mesh.tiles_x << 2, mesh.tiles_y << 2 );
    if ( NULL == bmphitemap ) return;

    y = 16;
    pixy = 0;
    while ( pixy < ( mesh.tiles_y << 2 ) )
    {
        x = 16;
        pixx = 0;
        while ( pixx < ( mesh.tiles_x << 2 ) )
        {
            level = ( get_level( x, y ) * 255 / mesh.edgez );  // level is 0 to 255
            if ( level > 252 ) level = 252;
            fan = mesh_get_fan( pixx >> 2, pixy );
            level = 255;
            if ( -1 != fan )
            {
                if ( HAS_BITS( mesh.fx[fan], MPDFX_WALL ) ) level = 253;  // Wall
                if ( HAS_BITS( mesh.fx[fan], MPDFX_IMPASS ) ) level = 254;   // Impass
                if ( HAS_BITS( mesh.fx[fan], MPDFX_WALL ) &&
                     HAS_BITS( mesh.fx[fan], MPDFX_IMPASS ) ) level = 255;   // Both
            }
            SDL_PutPixel( bmphitemap, pixx, pixy, level );
            x += 32;
            pixx++;
        }
        y += 32;
        pixy++;
    }
}

//--------------------------------------------------------------------------------------------
void make_planmap( void )
{
    int x, y, putx, puty;
    //SDL_Surface* bmptemp;

    //bmptemp = cartman_CreateSurface(64, 64);
    //if(NULL != bmptemp)  return;

    if ( NULL == bmphitemap ) SDL_FreeSurface( bmphitemap );
    bmphitemap = cartman_CreateSurface( mesh.tiles_x * TINYXY, mesh.tiles_y * TINYXY );
    if ( NULL == bmphitemap ) return;

    SDL_FillRect( bmphitemap, NULL, MAKE_BGR( bmphitemap, 0, 0, 0 ) );

    puty = 0;
    for ( y = 0; y < mesh.tiles_y; y++ )
    {
        putx = 0;
        for ( x = 0; x < mesh.tiles_x; x++ )
        {
            glTexture * tx_tile;
            tx_tile = tiny_tile_at( x, y );

            if ( NULL != tx_tile )
            {
                SDL_Rect dst = {putx, puty, TINYXY, TINYXY};
                cartman_BlitSurface( tx_tile->surface, NULL, bmphitemap, &dst );
            }
            putx += TINYXY;
        }
        puty += TINYXY;
    }

    //SDL_SoftStretch(bmphitemap, NULL, bmptemp, NULL);
    //SDL_FreeSurface(bmphitemap);

    //bmphitemap = bmptemp;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void draw_top_fan( window_t * pwin, int fan, float zoom_hrz )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     A wireframe tile from a vertex connection window
    Uint32 faketoreal[MAXMESHVERTICES];

    int fantype;
    int cnt, stt, end, vert;
    float color[4];
    float size;
    float x, y;

    cart_vec_t vup    = { 0, 1, 0};
    cart_vec_t vright = { -1, 0, 0};
    cart_vec_t vpos;

    if ( -1 == fan ) return;

    fantype = mesh.fantype[fan];

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( fantype >= MAXMESHTYPE / 2 )
    {
        OGL_MAKE_COLOR_4( color, 32, 31, 16, 16 );
    }

    for ( cnt = 0, vert = mesh.vrtstart[fan];
          cnt < mesh.command[fantype].numvertices && CHAINEND != vert;
          cnt++, vert = mesh.vrtnext[vert] )
    {
        faketoreal[cnt] = vert;
    }

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {
            for ( cnt = 0; cnt < mesh.numline[fantype]; cnt++ )
            {
                stt = faketoreal[mesh.line[fantype].start[cnt]];
                end = faketoreal[mesh.line[fantype].end[cnt]];

                glVertex3f( mesh.vrtx[stt], mesh.vrty[stt], mesh.vrtz[stt] );
                glVertex3f( mesh.vrtx[end], mesh.vrty[end], mesh.vrtz[end] );
            }
        }
        glEnd();
    }
    glPopAttrib();

    glColor4f( 1, 1, 1, 1 );
    for ( cnt = 0; cnt < mesh.command[fantype].numvertices; cnt++ )
    {
        int point_size;

        vert = faketoreal[cnt];

        size = MAXPOINTSIZE * mesh.vrtz[vert] / ( float ) mesh.edgez;
        if ( size < 0.0f ) size = 0.0f;
        if ( size > MAXPOINTSIZE ) size = MAXPOINTSIZE;

        point_size = 4.0f * POINT_SIZE( size ) / zoom_hrz;

        if ( point_size > 0 )
        {
            glTexture * tx_tmp;

            if ( select_has_vert( vert ) )
            {
                tx_tmp = &tx_pointon;
            }
            else
            {
                tx_tmp = &tx_point;
            }

            vpos[kX] = mesh.vrtx[vert];
            vpos[kY] = mesh.vrty[vert];
            vpos[kZ] = mesh.vrtz[vert];

            ogl_draw_sprite_3d( tx_tmp, vpos, vup, vright, point_size, point_size );
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_side_fan( window_t * pwin, int fan, float zoom_hrz, float zoom_vrt )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     A wireframe tile from a vertex connection window ( Side view )
    Uint32 faketoreal[MAXMESHVERTICES];
    int fantype;
    int cnt, stt, end, vert;
    float color[4];
    float size;
    float point_size;
    float x, z;

    cart_vec_t vup    = { 0, 0, 1};
    cart_vec_t vright = { 1, 0, 0};
    cart_vec_t vpos;

    fantype = mesh.fantype[fan];
    if ( fantype > MAXMESHTYPE ) return;

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( fantype >= MAXMESHTYPE / 2 )
    {
        OGL_MAKE_COLOR_4( color, 32, 31, 16, 16 );
    }

    for ( cnt = 0, vert = mesh.vrtstart[fan];
          cnt < mesh.command[fantype].numvertices && CHAINEND != vert;
          cnt++, vert = mesh.vrtnext[vert] )
    {
        faketoreal[cnt] = vert;
    }

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {
            for ( cnt = 0; cnt < mesh.numline[fantype]; cnt++ )
            {
                stt = faketoreal[mesh.line[fantype].start[cnt]];
                end = faketoreal[mesh.line[fantype].end[cnt]];

                glVertex3f( mesh.vrtx[stt], mesh.vrty[stt], mesh.vrtz[stt] );
                glVertex3f( mesh.vrtx[end], mesh.vrty[end], mesh.vrtz[end] );
            }
        }
        glEnd();
    }
    glPopAttrib();

    size = 7;
    point_size = 4.0f * POINT_SIZE( size ) / zoom_hrz;

    glColor4f( 1, 1, 1, 1 );

    for ( cnt = 0; cnt < mesh.command[fantype].numvertices; cnt++ )
    {
        glTexture * tx_tmp = NULL;

        vert = faketoreal[cnt];

        if ( select_has_vert( vert ) )
        {
            tx_tmp = &tx_pointon;
        }
        else
        {
            tx_tmp = &tx_point;
        }

        vpos[kX] = mesh.vrtx[vert];
        vpos[kY] = mesh.vrty[vert];
        vpos[kZ] = mesh.vrtz[vert];

        ogl_draw_sprite_3d( tx_tmp, vpos, vup, vright, point_size, point_size );
    }
}

//--------------------------------------------------------------------------------------------
void draw_schematic( window_t * pwin, int fantype, int x, int y )
{
    // ZZ> This function draws the line drawing preview of the tile type...
    //     The wireframe on the left side of the theSurface.
    int cnt, stt, end;
    float color[4];

    OGL_MAKE_COLOR_4( color, 32, 16, 16, 31 );
    if ( fantype >= MAXMESHTYPE / 2 )
    {
        OGL_MAKE_COLOR_4( color, 32, 31, 16, 16 );
    };

    glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT );
    {
        glColor4fv( color );
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_LINES );
        {
            for ( cnt = 0; cnt < mesh.numline[fantype]; cnt++ )
            {
                stt = mesh.line[fantype].start[cnt];
                end = mesh.line[fantype].end[cnt];

                glVertex2i( mesh.command[fantype].x[stt] + x, mesh.command[fantype].y[stt] + y );
                glVertex2i( mesh.command[fantype].x[end] + x, mesh.command[fantype].y[end] + y );
            }
        }
        glEnd();
    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void draw_top_tile( float x0, float y0, int fan, glTexture * tx_tile, bool_t draw_tile )
{
    int cnt;
    Uint32 vert;
    float min_s, min_t, max_s, max_t;

    const float dst = 1.0f / 64.0f;

    simple_vertex_t vrt[4];

    if ( -1 == fan || TILE_IS_FANOFF( fan ) ) return;

    if ( NULL == tx_tile ) return;

    glTexture_Bind( tx_tile );

    min_s = dst;
    min_t = dst;

    max_s = -dst + ( float ) glTexture_GetImageWidth( tx_tile )  / ( float ) glTexture_GetTextureWidth( tx_tile );
    max_t = -dst + ( float ) glTexture_GetImageHeight( tx_tile )  / ( float ) glTexture_GetTextureHeight( tx_tile );

    // set the texture coordinates
    vrt[0].s = min_s;
    vrt[0].t = min_t;

    vrt[1].s = max_s;
    vrt[1].t = min_t;

    vrt[2].s = max_s;
    vrt[2].t = max_t;

    vrt[3].s = min_s;
    vrt[3].t = max_t;

    // set the tile corners
    if ( draw_tile )
    {
        // draw the tile on a 31x31 grix, using the values of x0,y0

        vert = mesh.vrtstart[fan];

        // Top Left
        vrt[0].x = x0;
        vrt[0].y = y0;
        vrt[0].z = 0;
        vrt[0].l = mesh.vrta[vert] / 255.0f;
        vert = mesh.vrtnext[vert];

        // Top Right
        vrt[1].x = x0 + TILE_SIZE;
        vrt[1].y = y0;
        vrt[1].z = 0;
        vrt[1].l = mesh.vrta[vert] / 255.0f;
        vert = mesh.vrtnext[vert];

        // Bottom Right
        vrt[2].x = x0 + TILE_SIZE;
        vrt[2].y = y0 + TILE_SIZE;
        vrt[2].z = 0;
        vrt[2].l = mesh.vrta[vert] / 255.0f;
        vert = mesh.vrtnext[vert];

        // Bottom Left
        vrt[3].x = x0;
        vrt[3].y = y0 + TILE_SIZE;
        vrt[3].z = 0;
        vrt[3].l = mesh.vrta[vert] / 255.0f;
    }
    else
    {
        // draw the tile using the actual values of the coordinates

        int cnt;

        vert = mesh.vrtstart[fan];
        for ( cnt = 0;
              cnt < 4 && CHAINEND != vert;
              cnt++, vert = mesh.vrtnext[vert] )
        {
            vrt[cnt].x = mesh.vrtx[vert];
            vrt[cnt].y = mesh.vrty[vert];
            vrt[cnt].z = mesh.vrtz[vert];
            vrt[cnt].l = mesh.vrta[vert] / 255.0f;
        }
    }

    // Draw A Quad
    glBegin( GL_QUADS );
    {
        // initialize the color. remove any transparency!
        glColor4f( 1.0f,  1.0f,  1.0f, 1.0f );

        for ( cnt = 0; cnt < 4; cnt++ )
        {
            glColor3f( vrt[cnt].l,  vrt[cnt].l,  vrt[cnt].l );
            glTexCoord2f( vrt[cnt].s, vrt[cnt].t );
            glVertex3f( vrt[cnt].x, vrt[cnt].y, vrt[cnt].z );
        };
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
void draw_tile_fx( float x, float y, Uint8 fx, float scale )
{
    const int ioff_0 = TILE_SIZE >> 3;
    const int ioff_1 = TILE_SIZE >> 4;

    const float foff_0 = ioff_0 * scale;
    const float foff_1 = ioff_1 * scale;

    float x1, y1;
    float w1, h1;

    // water is whole tile
    if ( HAS_BITS( fx, MPDFX_WATER ) )
    {
        x1 = x;
        y1 = y;
        w1 = tx_water.imgW * scale;
        h1 = tx_water.imgH * scale;

        ogl_draw_sprite_2d( &tx_water, x1, y1, w1, h1 );
    }

    // "reflectable tile" is upper left
    if ( !HAS_BITS( fx, MPDFX_SHA ) )
    {
        x1 = x;
        y1 = y;
        w1 = tx_ref.imgW * scale;
        h1 = tx_ref.imgH * scale;

        ogl_draw_sprite_2d( &tx_ref, x1, y1, w1, h1 );
    }

    // "reflects characters" is upper right
    if ( HAS_BITS( fx, MPDFX_DRAWREF ) )
    {
        x1 = x + foff_0;
        y1 = y;

        w1 = tx_drawref.imgW * scale;
        h1 = tx_drawref.imgH * scale;

        ogl_draw_sprite_2d( &tx_drawref, x1, y1, w1, h1 );
    }

    // "animated tile" is lower left
    if ( HAS_BITS( fx, MPDFX_ANIM ) )
    {
        x1 = x;
        y1 = y + foff_0;

        w1 = tx_anim.imgW * scale;
        h1 = tx_anim.imgH * scale;

        ogl_draw_sprite_2d( &tx_anim, x1, y1, w1, h1 );
    }

    // the following are all in the lower left quad
    x1 = x + foff_0;
    y1 = y + foff_0;

    if ( HAS_BITS( fx, MPDFX_WALL ) )
    {
        float x2 = x1;
        float y2 = y1;

        w1 = tx_wall.imgW * scale;
        h1 = tx_wall.imgH * scale;

        ogl_draw_sprite_2d( &tx_wall, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MPDFX_IMPASS ) )
    {
        float x2 = x1 + foff_1;
        float y2 = y1;

        w1 = tx_impass.imgW * scale;
        h1 = tx_impass.imgH * scale;

        ogl_draw_sprite_2d( &tx_impass, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MPDFX_DAMAGE ) )
    {
        float x2 = x1;
        float y2 = y1 + foff_1;

        w1 = tx_damage.imgW * scale;
        h1 = tx_damage.imgH * scale;

        ogl_draw_sprite_2d( &tx_damage, x2, y2, w1, h1 );
    }

    if ( HAS_BITS( fx, MPDFX_SLIPPY ) )
    {
        float x2 = x1 + foff_1;
        float y2 = y1 + foff_1;

        w1 = tx_slippy.imgW * scale;
        h1 = tx_slippy.imgH * scale;

        ogl_draw_sprite_2d( &tx_slippy, x2, y2, w1, h1 );
    }

}

//--------------------------------------------------------------------------------------------
void ogl_draw_sprite_2d( glTexture * img, float x, float y, float width, float height )
{
    float w, h;
    float min_s, max_s, min_t, max_t;

    const float dst = 1.0f / 64.0f;

    w = width;
    h = height;

    if ( NULL != img )
    {
        if ( width == 0 || height == 0 )
        {
            w = glTexture_GetTextureWidth( img );
            h = glTexture_GetTextureHeight( img );
        }

        min_s = dst;
        min_t = dst;

        max_s = -dst + ( float ) glTexture_GetImageWidth( img )  / ( float ) glTexture_GetTextureWidth( img );
        max_t = -dst + ( float ) glTexture_GetImageHeight( img )  / ( float ) glTexture_GetTextureHeight( img );
    }
    else
    {
        min_s = dst;
        min_t = dst;

        max_s = 1.0f - dst;
        max_t = 1.0f - dst;
    }

    // Draw the image
    glTexture_Bind( img );

    glColor4f( 1, 1, 1, 1 );

    glBegin( GL_TRIANGLE_STRIP );
    {
        glTexCoord2f( min_s, min_t );  glVertex2f( x,     y );
        glTexCoord2f( max_s, min_t );  glVertex2f( x + w, y );
        glTexCoord2f( min_s, max_t );  glVertex2f( x,     y + h );
        glTexCoord2f( max_s, max_t );  glVertex2f( x + w, y + h );
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
void ogl_draw_sprite_3d( glTexture * img, cart_vec_t pos, cart_vec_t vup, cart_vec_t vright, float width, float height )
{
    float w, h;
    float min_s, max_s, min_t, max_t;
    cart_vec_t bboard[4];

    const float dst = 1.0f / 64.0f;

    w = width;
    h = height;

    if ( NULL != img )
    {
        if ( width == 0 || height == 0 )
        {
            w = glTexture_GetTextureWidth( img );
            h = glTexture_GetTextureHeight( img );
        }

        min_s = dst;
        min_t = dst;

        max_s = -dst + ( float ) glTexture_GetImageWidth( img )  / ( float ) glTexture_GetTextureWidth( img );
        max_t = -dst + ( float ) glTexture_GetImageHeight( img )  / ( float ) glTexture_GetTextureHeight( img );
    }
    else
    {
        min_s = dst;
        min_t = dst;

        max_s = 1.0f - dst;
        max_t = 1.0f - dst;
    }

    // Draw the image
    glTexture_Bind( img );

    glColor4f( 1, 1, 1, 1 );

    bboard[0][kX] = pos[kX] - w / 2 * vright[kX] + h / 2 * vup[kX];
    bboard[0][kY] = pos[kY] - w / 2 * vright[kY] + h / 2 * vup[kY];
    bboard[0][kZ] = pos[kZ] - w / 2 * vright[kZ] + h / 2 * vup[kZ];

    bboard[1][kX] = pos[kX] + w / 2 * vright[kX] + h / 2 * vup[kX];
    bboard[1][kY] = pos[kY] + w / 2 * vright[kY] + h / 2 * vup[kY];
    bboard[1][kZ] = pos[kZ] + w / 2 * vright[kZ] + h / 2 * vup[kZ];

    bboard[2][kX] = pos[kX] - w / 2 * vright[kX] - h / 2 * vup[kX];
    bboard[2][kY] = pos[kY] - w / 2 * vright[kY] - h / 2 * vup[kY];
    bboard[2][kZ] = pos[kZ] - w / 2 * vright[kZ] - h / 2 * vup[kZ];

    bboard[3][kX] = pos[kX] + w / 2 * vright[kX] - h / 2 * vup[kX];
    bboard[3][kY] = pos[kY] + w / 2 * vright[kY] - h / 2 * vup[kY];
    bboard[3][kZ] = pos[kZ] + w / 2 * vright[kZ] - h / 2 * vup[kZ];

    glBegin( GL_TRIANGLE_STRIP );
    {
        glTexCoord2f( min_s, min_t );  glVertex3fv( bboard[0] );
        glTexCoord2f( max_s, min_t );  glVertex3fv( bboard[1] );
        glTexCoord2f( min_s, max_t );  glVertex3fv( bboard[2] );
        glTexCoord2f( max_s, max_t );  glVertex3fv( bboard[3] );
    }
    glEnd();
}

//--------------------------------------------------------------------------------------------
void ogl_draw_box( float x, float y, float w, float h, float color[] )
{
    glPushAttrib( GL_ENABLE_BIT );
    {
        glDisable( GL_TEXTURE_2D );

        glBegin( GL_QUADS );
        {
            glColor4fv( color );

            glVertex2f( x,     y );
            glVertex2f( x,     y + h );
            glVertex2f( x + w, y + h );
            glVertex2f( x + w, y );
        }
        glEnd();
    }
    glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void ogl_beginFrame()
{
    glPushAttrib( GL_ENABLE_BIT );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glEnable( GL_TEXTURE_2D );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glViewport( 0, 0, theSurface->w, theSurface->h );

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( 0, theSurface->w, theSurface->h, 0, -1, 1 );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
}

//--------------------------------------------------------------------------------------------
void ogl_endFrame()
{
    // Restore the OpenGL matrices to what they were
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    // Re-enable any states disabled by gui_beginFrame
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------
void draw_sprite( SDL_Surface * dst, SDL_Surface * sprite, int x, int y )
{
    SDL_Rect rdst;

    if ( NULL == dst || NULL == sprite ) return;

    rdst.x = x;
    rdst.y = y;
    rdst.w = sprite->w;
    rdst.h = sprite->h;

    cartman_BlitSurface( sprite, NULL, dst, &rdst );
}

//--------------------------------------------------------------------------------------------
int cartman_BlitScreen( SDL_Surface * bmp, SDL_Rect * prect )
{
    return cartman_BlitSurface( bmp, NULL, theSurface, prect );
}

//--------------------------------------------------------------------------------------------
int cartman_BlitSurface( SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect )
{
    // clip the source and destination rectangles

    int retval = -1;
    SDL_Rect rsrc, rdst;

    if ( NULL == src || HAS_BITS(( size_t )src->map, 0x80000000 ) ) return 0;

    if ( NULL == srcrect && NULL == dstrect )
    {
        retval = SDL_BlitSurface( src, NULL, dst, NULL );
        if ( retval >= 0 )
        {
            SDL_UpdateRect( dst, 0, 0, 0, 0 );
        }
    }
    else if ( NULL == srcrect )
    {
        SDL_RectIntersect( &( dst->clip_rect ), dstrect, &rdst );
        retval = SDL_BlitSurface( src, NULL, dst, &rdst );
        if ( retval >= 0 )
        {
            SDL_UpdateRect( dst, rdst.x, rdst.y, rdst.w, rdst.h );
        }
    }
    else if ( NULL == dstrect )
    {
        SDL_RectIntersect( &( src->clip_rect ), srcrect, &rsrc );

        retval = SDL_BlitSurface( src, &rsrc, dst, NULL );
        if ( retval >= 0 )
        {
            SDL_UpdateRect( dst, 0, 0, 0, 0 );
        }
    }
    else
    {
        SDL_RectIntersect( &( src->clip_rect ), srcrect, &rsrc );
        SDL_RectIntersect( &( dst->clip_rect ), dstrect, &rdst );

        retval = SDL_BlitSurface( src, &rsrc, dst, &rdst );
        if ( retval >= 0 )
        {
            SDL_UpdateRect( dst, rdst.x, rdst.y, rdst.w, rdst.h );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
SDL_Surface * cartman_LoadIMG( const char * szName )
{
    SDL_PixelFormat tmpformat;
    SDL_Surface * bmptemp, * bmpconvert;

    // load the bitmap
    bmptemp = IMG_Load( szName );

    // expand the screen format to support alpha
    memcpy( &tmpformat, theSurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
    SDLX_ExpandFormat( &tmpformat );

    // convert it to the same pixel format as the screen surface
    bmpconvert = SDL_ConvertSurface( bmptemp, &tmpformat, SDL_SWSURFACE );
    SDL_FreeSurface( bmptemp );

    return bmpconvert;
}

//--------------------------------------------------------------------------------------------
void cartman_begin_ortho_camera_hrz( window_t * pwin, camera_t * pcam, float zoom_x, float zoom_y )
{
    float w, h, d;
    float aspect;
    float left, right, bottom, top, front, back;

    w = ( float )DEFAULT_RESOLUTION * ( float )TILE_SIZE * (( float )pwin->surfacex / ( float )DEFAULT_WINDOW_W ) / zoom_x;
    h = ( float )DEFAULT_RESOLUTION * ( float )TILE_SIZE * (( float )pwin->surfacey / ( float )DEFAULT_WINDOW_H ) / zoom_y;
    d = DEFAULT_Z_SIZE;

    pcam->w = w;
    pcam->h = h;
    pcam->d = d;

    left   = - w / 2;
    right  =   w / 2;
    bottom = - h / 2;
    top    =   h / 2;
    front  = -d;
    back   =  d;

    aspect = ( float ) w / h;
    if ( aspect < 1.0f )
    {
        // window taller than wide
        bottom /= aspect;
        top /= aspect;
    }
    else
    {
        left *= aspect;
        right *= aspect;
    }

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( left, right, bottom, top, front, back );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glScalef( -1.0f, 1.0f, 1.0f );
    gluLookAt( pcam->x, pcam->y, back, pcam->x, pcam->y, front, 0.0f, -1.0f, 0.0f );
}

//--------------------------------------------------------------------------------------------
void cartman_begin_ortho_camera_vrt( window_t * pwin, camera_t * pcam, float zoom_x, float zoom_z )
{
    float w, h, d;
    float aspect;
    float left, right, bottom, top, back, front;

    w = pwin->surfacex * ( float )DEFAULT_RESOLUTION * ( float )TILE_SIZE / ( float )DEFAULT_WINDOW_W / zoom_x;
    h = w;
    d = pwin->surfacey * ( float )DEFAULT_RESOLUTION * ( float )TILE_SIZE / ( float )DEFAULT_WINDOW_H / zoom_z;

    pcam->w = w;
    pcam->h = h;
    pcam->d = d;

    left   = - w / 2;
    right  =   w / 2;
    bottom = - d / 2;
    top    =   d / 2;
    front  =   0;
    back   =   h;

    aspect = ( float ) w / ( float ) d;
    if ( aspect < 1.0f )
    {
        // window taller than wide
        bottom /= aspect;
        top /= aspect;
    }
    else
    {
        left *= aspect;
        right *= aspect;
    }

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glOrtho( left, right, bottom, top, front, back );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glScalef( 1.0f, 1.0f, 1.0f );
    gluLookAt( pcam->x, pcam->y, pcam->z, pcam->x, pcam->y + back, pcam->z, 0.0f, 0.0f, 1.0f );
}

//--------------------------------------------------------------------------------------------
void cartman_end_ortho_camera( )
{
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
}

//--------------------------------------------------------------------------------------------
void create_imgcursor( void )
{
    int x, y;
    Uint32 col, loc, clr;
    SDL_Rect rtmp;

    bmpcursor = cartman_CreateSurface( 8, 8 );
    col = MAKE_BGR( bmpcursor, 31, 31, 31 );    // White color
    loc = MAKE_BGR( bmpcursor, 3, 3, 3 );           // Gray color
    clr = MAKE_ABGR( bmpcursor, 0, 0, 0, 8 );

    // Simple triangle
    rtmp.x = 0;
    rtmp.y = 0;
    rtmp.w = 8;
    rtmp.h = 1;
    SDL_FillRect( bmpcursor, &rtmp, loc );

    for ( y = 0; y < 8; y++ )
    {
        for ( x = 0; x < 8; x++ )
        {
            if ( x + y < 8 ) SDL_PutPixel( bmpcursor, x, y, col );
            else SDL_PutPixel( bmpcursor, x, y, clr );
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_img( void )
{
    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_point, "data" SLASH_STR "point.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "point.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_pointon, "data" SLASH_STR "pointon.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "pointon.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_ref, "data" SLASH_STR "ref.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "ref.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_drawref, "data" SLASH_STR "drawref.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "drawref.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_anim, "data" SLASH_STR "anim.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "anim.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_water, "data" SLASH_STR "water.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "water.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_wall, "data" SLASH_STR "slit.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "slit.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_impass, "data" SLASH_STR "impass.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "impass.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_damage, "data" SLASH_STR "damage.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "damage.png" );
    }

    if ( INVALID_TX_ID == glTexture_Load( GL_TEXTURE_2D, &tx_slippy, "data" SLASH_STR "slippy.png", INVALID_KEY ) )
    {
        log_warning( "Cannot load image \"%s\".\n", "slippy.png" );
    }
}

//--------------------------------------------------------------------------------------------
void get_small_tiles( SDL_Surface* bmpload )
{
    SDL_Surface * image;

    int x, y, x1, y1;
    int sz_x = bmpload->w;
    int sz_y = bmpload->h;
    int step_x = sz_x >> 3;
    int step_y = sz_y >> 3;

    if ( step_x == 0 ) step_x = 1;
    if ( step_y == 0 ) step_y = 1;

    y1 = 0;
    y = 0;
    while ( y < sz_y && y1 < 256 )
    {
        x1 = 0;
        x = 0;
        while ( x < sz_x && x1 < 256 )
        {
            SDL_Rect src1 = { x, y, ( step_x - 1 ), ( step_y - 1 ) };

            glTexture_new( tx_smalltile + numsmalltile );

            image = cartman_CreateSurface( SMALLXY, SMALLXY );
            SDL_FillRect( image, NULL, MAKE_BGR( image, 0, 0, 0 ) );
            SDL_SoftStretch( bmpload, &src1, image, NULL );

            glTexture_Convert( GL_TEXTURE_2D, tx_smalltile + numsmalltile, image, INVALID_KEY );

            numsmalltile++;
            x += step_x;
            x1 += 32;
        }
        y += step_y;
        y1 += 32;
    }
}

//--------------------------------------------------------------------------------------------
void get_big_tiles( SDL_Surface* bmpload )
{
    SDL_Surface * image;

    int x, y, x1, y1;
    int sz_x = bmpload->w;
    int sz_y = bmpload->h;
    int step_x = sz_x >> 3;
    int step_y = sz_y >> 3;

    if ( step_x == 0 ) step_x = 1;
    if ( step_y == 0 ) step_y = 1;

    y1 = 0;
    y = 0;
    while ( y < sz_y )
    {
        x1 = 0;
        x = 0;
        while ( x < sz_x )
        {
            int wid, hgt;

            SDL_Rect src1;

            wid = ( 2 * step_x - 1 );
            if ( x + wid > bmpload->w ) wid = bmpload->w - x;

            hgt = ( 2 * step_y - 1 );
            if ( y + hgt > bmpload->h ) hgt = bmpload->h - y;

            src1.x = x;
            src1.y = y;
            src1.w = wid;
            src1.h = hgt;

            glTexture_new( tx_bigtile + numbigtile );

            image = cartman_CreateSurface( SMALLXY, SMALLXY );
            SDL_FillRect( image, NULL, MAKE_BGR( image, 0, 0, 0 ) );

            SDL_SoftStretch( bmpload, &src1, image, NULL );

            glTexture_Convert( GL_TEXTURE_2D, tx_bigtile + numbigtile, image, INVALID_KEY );

            numbigtile++;
            x += step_x;
            x1 += 32;
        }
        y += step_y;
        y1 += 32;
    }
}

//--------------------------------------------------------------------------------------------
void get_tiles( SDL_Surface* bmpload )
{
    get_small_tiles( bmpload );
    get_big_tiles( bmpload );
}

//--------------------------------------------------------------------------------------------
SDL_Surface * cartman_CreateSurface( int w, int h )
{
    SDL_PixelFormat   tmpformat;

    if ( NULL == theSurface ) return NULL;

    // expand the screen format to support alpha
    memcpy( &tmpformat, theSurface->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format
    SDLX_ExpandFormat( &tmpformat );

    return SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, tmpformat.BitsPerPixel, tmpformat.Rmask, tmpformat.Gmask, tmpformat.Bmask, tmpformat.Amask );
}

