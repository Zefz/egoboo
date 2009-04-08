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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

///
/// @file
/// @brief Egoboo OpenGL texture interface
/// @details Implements OpenGL texture loading using SDL_image

#include "ogl_texture.h"
#include "SDL_GL_extensions.h"

#include "cartman.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define ErrorImage_width 2
#define ErrorImage_height 2

static GLboolean ErrorImage_defined = GL_FALSE;

typedef GLubyte SET_PACKING( image_row_t[ErrorImage_width][4], 1 );

static GLubyte ErrorImage[ErrorImage_height][ErrorImage_width][4];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void ErrorImage_create(void)
{
    // BB > define a default "error texture"

    int i, j;

    for (i = 0; i < ErrorImage_height; i++)
    {
        for (j = 0; j < ErrorImage_width; j++)
        {
            if ( 0 == ((i&0x1) ^ (j&0x1)) )
            {
                ErrorImage[i][j][0] = (GLubyte) 255;
                ErrorImage[i][j][1] = (GLubyte) 0;
                ErrorImage[i][j][2] = (GLubyte) 0;
            }
            else
            {
                ErrorImage[i][j][0] = (GLubyte) 0;
                ErrorImage[i][j][1] = (GLubyte) 255;
                ErrorImage[i][j][2] = (GLubyte) 255;
            }

            ErrorImage[i][j][3] = (GLubyte) 255;
        }
    }

    ErrorImage_defined = GL_TRUE;
}

void ErrorImage_bind(GLenum target, GLuint id)
{
    glPushClientAttrib( GL_CLIENT_PIXEL_STORE_BIT ) ;
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(target, id);

        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        if (target == GL_TEXTURE_1D)
        {
            glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, ErrorImage_width, 0, GL_RGBA, GL_UNSIGNED_BYTE, ErrorImage);
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ErrorImage_width, ErrorImage_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ErrorImage);
        }
    }
    glPopClientAttrib();

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
glTexture * glTexture_new(glTexture * ptex)
{
    if ( NULL == ptex ) return ptex;

    if ( INVALID_TX_ID != ptex->base.binding )
    {
        glDeleteTextures( 1, &(ptex->base.binding) );
        ptex->base.binding = INVALID_TX_ID;
    }

    memset( ptex, 0, sizeof(glTexture) );

    // only need one textureID per texture
    // do not need to ask for a new id, even if we change the texture data
    glGenTextures( 1, &(ptex->base.binding) );

    // set the image to be clamped in s and t
    ptex->base.wrap_s = GL_CLAMP;
    ptex->base.wrap_t = GL_CLAMP;

    return ptex;
}

//--------------------------------------------------------------------------------------------
void glTexture_delete(glTexture * ptex)
{
    if ( NULL == ptex ) return;

    // actually delete the OpenGL texture data
    glDeleteTextures( 1, &ptex->base.binding );
    ptex->base.binding = INVALID_TX_ID;

    // set the image to be clamped in s and t
    ptex->base.wrap_s = GL_CLAMP;
    ptex->base.wrap_t = GL_CLAMP;

    // Reset the other data
    ptex->imgH = ptex->imgW = ptex->base.width = ptex->base.height  = 0;
    ptex->name[0] = '\0';

    if ( NULL != ptex->surface )
    {
        SDL_FreeSurface( ptex->surface );
        ptex->surface = NULL;
    }
}

//--------------------------------------------------------------------------------------------
Uint32 glTexture_Convert( GLenum tx_target, glTexture *ptex, SDL_Surface * image, Uint32 key )
{
    SDL_Surface     * screen;
    SDL_PixelFormat * pformat;
    SDL_PixelFormat   tmpformat;
    SDL_Surface     * tmp;
    Uint32            convert_flags;
    bool_t            use_alpha;

    SDLX_screen_info_t sdl_scr;

    if ( NULL == ptex ) return INVALID_TX_ID;

    SDLX_Get_Screen_Info(&sdl_scr, bfalse);

    // make sure the old texture has been freed
    glTexture_Release( ptex );

    if ( NULL == image ) return INVALID_TX_ID;

    /* set the color key, if valid */
    if ( NULL != image->format && NULL != image->format->palette && INVALID_KEY != key )
    {
        SDL_SetColorKey( image, SDL_SRCCOLORKEY, key );
    };

    /* Set the ptex's alpha */
    ptex->alpha = image->format->alpha / 255.0f;

    /* Set the original image's size (incase it's not an exact square of a power of two) */
    ptex->imgH = image->h;
    ptex->imgW = image->w;

    // adjust the texture target
    tx_target = ((1 == image->h) && (image->w > 1)) ? GL_TEXTURE_1D : GL_TEXTURE_2D;

    /* Determine the correct power of two greater than or equal to the original image's size */
    ptex->base.height = powerOfTwo( image->h );
    ptex->base.width  = powerOfTwo( image->w );

    screen  = SDL_GetVideoSurface();
    pformat = screen->format;
    memcpy( &tmpformat, screen->format, sizeof( SDL_PixelFormat ) );   // make a copy of the format

    if ( 0 != ( image->flags & ( SDL_SRCALPHA | SDL_SRCCOLORKEY ) ) )
    {
        int i;

        // the source image has an alpha channel
        /// @todo need to take into account the possible SDL_PixelFormat::Rloss, SDL_PixelFormat::Gloss, ...
        /// parameters

        tmpformat.Amask = sdl_a_mask;
        tmpformat.Bmask = sdl_b_mask;
        tmpformat.Gmask = sdl_g_mask;
        tmpformat.Rmask = sdl_r_mask;

        // make the pixel size match the screen format
        tmpformat.BitsPerPixel  = screen->format->BitsPerPixel;
        tmpformat.BytesPerPixel = screen->format->BytesPerPixel;

        // calculate the Amask from the SDL screen info
        for ( i = 0; i < sdl_scr.buf_d && 0 == ( tmpformat.Amask & ( 1 << i ) ); i++ );

        if ( 0 == (tmpformat.Amask & ( 1 << i )) )
        {
            // no alpha bits available
            tmpformat.Ashift = 0;
            tmpformat.Aloss  = 8;
        }
        else
        {
            // normal alpha channel
            tmpformat.Ashift = i;
            tmpformat.Aloss  = 0;
        }

        // replace pformat with the new format
        pformat = &tmpformat;
    }
    else
    {
        // the source image has no alpha channel
        // convert it to the screen format

        // correct the bits and bytes per pixel
        tmpformat.BitsPerPixel  = 32 - ( tmpformat.Rloss + tmpformat.Gloss + tmpformat.Bloss + tmpformat.Aloss );
        tmpformat.BytesPerPixel = tmpformat.BitsPerPixel / 8 + (( tmpformat.BitsPerPixel % 8 ) > 0 ? 1 : 0 );

        pformat = &tmpformat;
    }

    {
        SDL_Surface * tmp;
        Uint32 convert_flags = SDL_SWSURFACE;

        //convert the image format to the correct format
        convert_flags = SDL_SWSURFACE;
        tmp = SDL_ConvertSurface( image, pformat, convert_flags );
        SDL_FreeSurface( image );
        image = tmp;

        // fix the alpha channel on the new SDL_Surface.  For some reason, SDL wants to create
        // surface with surface->format->alpha==0x00 which causes a problem if we have to
        // use the SDL_BlitSurface() function below.  With the surface alpha set to zero, the
        // image will be converted to BLACK!
        //
        // The following statement tells SDL
        //   1) to set the image to opaque
        //   2) not to alpha blend the surface on blit
        SDL_SetAlpha( image, 0, SDL_ALPHA_OPAQUE );
        SDL_SetColorKey( image, 0, 0 );
    }

    // create a ptex that is acceptable to OpenGL (height and width are powers of 2)
    if ( ptex->imgH != ptex->base.height || ptex->imgW != ptex->base.width )
    {
        SDL_Surface * tmp = SDL_CreateRGBSurface( SDL_SWSURFACE, ptex->base.width, ptex->base.height, pformat->BitsPerPixel, pformat->Rmask, pformat->Gmask, pformat->Bmask, pformat->Amask );

        SDL_BlitSurface( image, &image->clip_rect, tmp, &image->clip_rect );
        SDL_FreeSurface( image );
        image = tmp;
    };

    /* Generate an OpenGL texture ID */
    if ( 0 == ptex->base.binding || INVALID_TX_ID == ptex->base.binding )
    {
        glGenTextures( 1, &ptex->base.binding );
    }

    ptex->base.target  = tx_target;
    ptex->surface      = image;

    /* Set up some parameters for the format of the glTexture */
    glTexture_Bind( ptex );

    /* actually create the OpenGL textures */
    use_alpha = !( 8 == image->format->Aloss );
    if ( tx_target == GL_TEXTURE_2D )
    {
        if ( cfg.texturefilter >= TX_MIPMAP )
        {
            oglx_upload_2d_mipmap(use_alpha, image->w, image->h, image->pixels);
        }
        else
        {
            oglx_upload_2d(use_alpha, image->w, image->h, image->pixels);
        }
    }
    else if (tx_target == GL_TEXTURE_1D)
    {
        oglx_upload_1d(use_alpha, image->w, image->pixels);
    }
    else
    {
        assert(0);
    }

    return ptex->base.binding;
}

//--------------------------------------------------------------------------------------------
Uint32 glTexture_Load( GLenum tx_target, glTexture *ptex, const char *filename, Uint32 key )
{
    Uint32 retval;
    SDL_Surface * image;

    // initialize the ptex
    glTexture_delete( ptex );
    if ( NULL == glTexture_new(ptex) ) return INVALID_TX_ID;

    image = IMG_Load( filename );
    if ( NULL == image ) return INVALID_TX_ID;

    retval = glTexture_Convert( tx_target, ptex, image, key );

    if ( INVALID_TX_ID == retval )
    {
        glTexture_delete(ptex);
    }
    else
    {
        strncpy( ptex->name, filename, sizeof(STRING) );

        ptex->base.wrap_s = GL_REPEAT;
        ptex->base.wrap_t = GL_REPEAT;
    }

    return retval;
}

/********************> glTexture_GetTextureID() <*****/
GLuint  glTexture_GetTextureID( glTexture *texture )
{
    return texture->base.binding;
}

/********************> glTexture_GetImageHeight() <*****/
GLsizei  glTexture_GetImageHeight( glTexture *texture )
{
    return texture->imgH;
}

/********************> glTexture_GetImageWidth() <*****/
GLsizei  glTexture_GetImageWidth( glTexture *texture )
{
    return texture->imgW;
}

/********************> glTexture_GetTextureWidth() <*****/
GLsizei  glTexture_GetTextureWidth( glTexture *texture )
{
    return texture->base.width;
}

/********************> glTexture_GetTextureHeight() <*****/
GLsizei  glTexture_GetTextureHeight( glTexture *texture )
{
    return texture->base.height;
}

/********************> glTexture_SetAlpha() <*****/
void  glTexture_SetAlpha( glTexture *texture, GLfloat alpha )
{
    texture->alpha = alpha;
}

/********************> glTexture_GetAlpha() <*****/
GLfloat  glTexture_GetAlpha( glTexture *texture )
{
    return texture->alpha;
}

/********************> glTexture_Release() <*****/
void  glTexture_Release( glTexture *texture )
{
    if (!ErrorImage_defined) ErrorImage_create();

    if ( NULL == texture ) return;

    // Bind an "error texture" to this texture
    if (INVALID_TX_ID == texture->base.binding)
    {
        glGenTextures( 1, &texture->base.binding );
    }

    ErrorImage_bind(GL_TEXTURE_2D, texture->base.binding);

    // Reset the other data
    texture->imgW = texture->base.width = ErrorImage_width;
    texture->imgH = texture->base.height = ErrorImage_height;
    strncpy( texture->name, "ErrorImage", sizeof(texture->name) );

    // set the image to be repeat in s and t
    texture->base.wrap_s = GL_REPEAT;
    texture->base.wrap_t = GL_REPEAT;

}

/********************> glTexture_Release() <*****/
void glTexture_Bind( glTexture *texture )
{
    int    filt_type, anisotropy;
    GLenum target;
    GLuint id;

    target = GL_TEXTURE_2D;
    id     = INVALID_TX_ID;
    if ( NULL != texture && 0 != texture->base.target )
    {
        target = texture->base.target;
        id     = texture->base.binding;
    }

    filt_type  = cfg.texturefilter;
    anisotropy = ogl_vparam.userAnisotropy;

    if ( !glIsEnabled( target ) )
    {
        glEnable( target );
    };

    if ( filt_type >= TX_ANISOTROPIC )
    {
        //Anisotropic filtered!
        oglx_bind(target, id, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR, anisotropy);
    }
    else
    {
        switch ( filt_type )
        {
                // Unfiltered
            case TX_UNFILTERED:
                oglx_bind(target, id, GL_REPEAT, GL_NEAREST, GL_NEAREST, GL_LINEAR, 0);
                break;

                // Linear filtered
            case TX_LINEAR:
                oglx_bind(target, id, GL_REPEAT, GL_LINEAR, GL_LINEAR, GL_LINEAR, 0);
                break;

                // Bilinear interpolation
            case TX_MIPMAP:
                oglx_bind(target, id, GL_REPEAT, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR, GL_LINEAR, 0);
                break;

                // Bilinear interpolation
            case TX_BILINEAR:
                oglx_bind(target, id, GL_REPEAT, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR, GL_LINEAR, 0);
                break;

                // Trilinear filtered (quality 1)
            case TX_TRILINEAR_1:
                oglx_bind(target, id, GL_REPEAT, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR, GL_LINEAR, 0);
                break;

                // Trilinear filtered (quality 2)
            case TX_TRILINEAR_2:
                oglx_bind(target, id, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_LINEAR, 0);
                break;
        };
    }

};

