// GLTexture.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "egoboo.h" // GAC - Needed for Win32 stuff
#include "gltexture.h"
#include "graphic.h"
#include <SDL_image.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


GLenum GLTexture::filter_mag = GL_LINEAR;
GLenum GLTexture::filter_min = GL_LINEAR;

//--------------------------------------------------------------------------------------------

const GLuint  GLTexture::GetTextureID(GLTexture *texture) { return texture->textureID; }

const GLsizei  GLTexture::GetImageHeight(GLTexture *texture) { return texture->imgH;}
const GLsizei  GLTexture::GetImageWidth(GLTexture *texture) { return texture->imgW; }

const GLsizei  GLTexture::GetTextureHeight(GLTexture *texture) { return texture->txH;}
const GLsizei  GLTexture::GetTextureWidth(GLTexture *texture) { return texture->txW; }

void           GLTexture::SetAlpha(GLTexture *texture, GLfloat alpha) { texture->alpha = alpha; }
const GLfloat  GLTexture::GetAlpha(GLTexture *texture) { return texture->alpha; }

/********************> GLTexture::Load() <*****/
//GLuint  GLTexture::Load(GLTexture *texture, const char *filename)
//{
//  SDL_Surface * image = IMG_Load(filename);
//  if(NULL == image) return GLTexture::INVALID;
//
//  /* Set the texture's alpha */
//  texture->alpha = 1;
//
//  /* Generate an OpenGL texture */
//  glGenTextures(1, &texture->textureID);
//
//  /* Set up some parameters for the format of the OpenGL texture */
//  glBindTexture(GL_TEXTURE_2D, texture->textureID);       /* Bind Our Texture */
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   /* Linear Filtered */
//  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   /* Linear Filtered */
//
//  /* Set the original image's size (incase it's not an exact square of a power of two) */
//  texture->imgH = image->h;
//  texture->imgW = image->w;
//
//  /* Determine the correct power of two greater than or equal to the original image's size */
//  texture->txH = powerOfTwo(image->h);
//  texture->txW = powerOfTwo(image->w);
//
//  SDL_Surface * screen = SDL_GetVideoSurface();
//  SDL_PixelFormat * pformat    = screen->format;  
//  SDL_PixelFormat   tmpformat  = *(screen->format); // make a copy of the format
//
//  Uint32 convert_flags = SDL_SWSURFACE;
//  if(image->flags & SDL_SRCALPHA)    convert_flags |= SDL_SRCALPHA;
//  if(image->flags & SDL_SRCCOLORKEY) convert_flags |= SDL_SRCCOLORKEY;
//
//  if(image->flags & (SDL_SRCALPHA|SDL_SRCCOLORKEY))
//  {
//    // the source image has an alpha channel
//    // TO DO : need to take into account the possible SDL_PixelFormat::Rloss, SDL_PixelFormat::Gloss, ... 
//    //         parameters
//
//    // create the trial mask
//    tmpformat.Amask = ~(tmpformat.Rmask | tmpformat.Gmask | tmpformat.Bmask);
//
//    // fix the Amask so that it sits inside the BytesPerPixel
//    tmpformat.BitsPerPixel  = tmpformat.BytesPerPixel * 8;
//    tmpformat.Amask &= (1<<tmpformat.BitsPerPixel)-1;
//
//    for(int i = 0; i<32 && (tmpformat.Amask&(1<<i)) == 0; i++);
//    tmpformat.Ashift = i;
//    tmpformat.Aloss  = 0;
//
//    pformat = &tmpformat;
//  }
//
//  //convert the image format to the correct format
//  SDL_Surface * tmp = SDL_ConvertSurface(image, pformat, convert_flags);
//  SDL_FreeSurface(image);
//  image = tmp;
//
//  // create a texture that is acceptable to OpenGL (height and width are powers of 2)
//  if(texture->imgH!= texture->txH || texture->imgW!= texture->txW)
//  {
//    SDL_Surface * tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, pformat->BitsPerPixel, pformat->Rmask, pformat->Gmask, pformat->Bmask, pformat->Amask);
//    SDL_BlitSurface(image, &image->clip_rect, tmp, &tmp->clip_rect);
//    SDL_FreeSurface(image);
//    image = tmp;
//  };
//
//  GLenum src_format = image->format->BitsPerPixel<32 ? GL_RGB : GL_RGBA;
//  GLenum dst_format = pformat->5BitsPerPixel<32       ? GL_RGB : GL_RGBA;
//  glTexImage2D(GL_TEXTURE_2D, 0, dst_format, image->w, image->h, 0, src_format, GL_UNSIGNED_BYTE, image->pixels);
//
//  SDL_FreeSurface(image);
//
//
////  SDL_Surface *tempSurface, *imageSurface;
////
////  if(NULL==texture) return INVALID;
////
////  /* Load the bitmap into an SDL_Surface */
////  imageSurface = SDL_LoadBMP(filename);
////
////  /* Make sure a valid SDL_Surface was returned */
////  if (imageSurface != NULL)
////  {
////    /* Generate an OpenGL texture */
////    glGenTextures(1, &texture->textureID);
////
////    /* Set up some parameters for the format of the OpenGL texture */
////    glBindTexture(GL_TEXTURE_2D, texture->textureID);       /* Bind Our Texture */
////    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   /* Linear Filtered */
////    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   /* Linear Filtered */
////
////    /* Set the original image's size (incase it's not an exact square of a power of two) */
////    texture->imgH = imageSurface->h;
////    texture->imgW = imageSurface->w;
////
////    /* Determine the correct power of two greater than or equal to the original image's size */
////    texture->txH = powerOfTwo(imageSurface->h);
////    texture->txW = powerOfTwo(imageSurface->w);
////
////    /* Set the texture's alpha */
////    texture->alpha = 1;
////
////    /* Create a blank SDL_Surface (of the smallest size to fit the image) & copy imageSurface into it*/
////    if (imageSurface->format->Gmask)
////    {
////      tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, 24, imageSurface->format->Rmask, imageSurface->format->Gmask, imageSurface->format->Bmask, imageSurface->format->Amask);
////    }
////    else
////    {
////#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
////      tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, 24, 0x00FF, 0xFF00, 0xFF0000, 0x0000);
////#else
////      tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, 24, 0xFF0000, 0xFF00, 0x00FF, 0x0000);
////#endif
////    }
////    SDL_BlitSurface(imageSurface, &imageSurface->clip_rect, tempSurface, &imageSurface->clip_rect);
////
////    /* actually create the OpenGL textures */
////    if (imageSurface->format->BytesPerPixel > 1)
////    {
////      // Bitmaps come in BGR format by default...
////      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tempSurface->w, tempSurface->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, tempSurface->pixels);
////    }
////    else
////    {
////      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tempSurface->w, tempSurface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, tempSurface->pixels);
////    }
////
////    /* get rid of our SDL_Surfaces now that we're done with them */
////    SDL_FreeSurface(tempSurface);
////    SDL_FreeSurface(imageSurface);
////  }
//
//  return texture->textureID;
//}
//
/********************> GLTexture::LoadA() <*****/
GLuint  GLTexture::Load(GLTexture *texture, const char *filename, Uint32 key)
{

  SDL_Surface * image = IMG_Load(filename);
  if(NULL == image) return GLTexture::INVALID;

  /* set the color key, if valid */
  if((Uint32)(-1)!=key)
  {
    SDL_SetColorKey(image, SDL_SRCCOLORKEY, key);
  };

  /* Set the texture's alpha */
  texture->alpha = 1;

  /* Generate an OpenGL texture */
  glGenTextures(1, &texture->textureID);

  /* Set up some parameters for the format of the OpenGL texture */
  glBindTexture(GL_TEXTURE_2D, texture->textureID);       /* Bind Our Texture */
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   /* Linear Filtered */
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   /* Linear Filtered */

  /* Set the original image's size (incase it's not an exact square of a power of two) */
  texture->imgH = image->h;
  texture->imgW = image->w;

  /* Determine the correct power of two greater than or equal to the original image's size */
  texture->txH = powerOfTwo(image->h);
  texture->txW = powerOfTwo(image->w);

  SDL_Surface * screen = SDL_GetVideoSurface();
  SDL_PixelFormat * pformat    = screen->format;  
  SDL_PixelFormat   tmpformat  = *(screen->format); // make a copy of the format

  if(0 != (image->flags & (SDL_SRCALPHA|SDL_SRCCOLORKEY)) )
  {
    // the source image has an alpha channel
    // TO DO : need to take into account the possible SDL_PixelFormat::Rloss, SDL_PixelFormat::Gloss, ... 
    //         parameters

    // create the mask
#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
      tmpformat.Amask = 0xFF << 24; 
      tmpformat.Bmask = 0xFF << 16; 
      tmpformat.Gmask = 0xFF <<  8; 
      tmpformat.Rmask = 0xFF <<  0; 
#else
      tmpformat.Amask = 0xFF <<  0; 
      tmpformat.Bmask = 0xFF <<  8; 
      tmpformat.Gmask = 0xFF << 16; 
      tmpformat.Rmask = 0xFF << 24; 
#endif

    tmpformat.BitsPerPixel  = 32;
    tmpformat.BytesPerPixel =  4;

    for(int i = 0; i<32 && (tmpformat.Amask&(1<<i)) == 0; i++);
    tmpformat.Ashift = i;
    tmpformat.Aloss  = 0;

    pformat = &tmpformat;
  }
  else
  {
    // the source image has no alpha channel
    // convert it to the screen format

    // correct the bits and bytes per pixel
    tmpformat.BitsPerPixel  = 32-(tmpformat.Rloss + tmpformat.Gloss + tmpformat.Bloss + tmpformat.Aloss);
    tmpformat.BytesPerPixel = tmpformat.BitsPerPixel/8 + ((tmpformat.BitsPerPixel%8)>0 ? 1 : 0);

    pformat = &tmpformat;
  }

  {
    //convert the image format to the correct format
    Uint32 convert_flags = SDL_SWSURFACE;
    if(image->flags & SDL_SRCALPHA) convert_flags |= SDL_SRCALPHA;
    SDL_Surface * tmp = SDL_ConvertSurface(image, pformat, convert_flags);
    SDL_FreeSurface(image);
    image = tmp;
  }

  // create a texture that is acceptable to OpenGL (height and width are powers of 2)
  if(texture->imgH!= texture->txH || texture->imgW!= texture->txW)
  {
    SDL_Surface * tmp = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, pformat->BitsPerPixel, pformat->Rmask, pformat->Gmask, pformat->Bmask, pformat->Amask);
    SDL_BlitSurface(image, &image->clip_rect, tmp, &image->clip_rect);
    SDL_FreeSurface(image);
    image = tmp;
  };

  GLenum src_format, dst_format;
  if(image->format->Aloss==8)
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image->w, image->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, image->pixels);
  }
  else
  {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
  }

  SDL_FreeSurface(image);

  return texture->textureID;
//
//
//  /* The key param indicates which color to set alpha to 0.  All other values are 0xFF. */
//  SDL_Surface *tempSurface, *imageSurface;
//  Sint16  x,y;
//  Uint32  *p;
//
//  if(NULL==texture) return INVALID;
//
//  /* Load the bitmap into an SDL_Surface */
//  imageSurface = SDL_LoadBMP(filename);
//
//  /* Make sure a valid SDL_Surface was returned */
//  if (imageSurface != NULL)
//  {
//    /* Generate an OpenGL texture */
//    glGetError();
//    glGenTextures(1, &texture->textureID);
//    GLenum errCode = glGetError();
//    if (errCode != GL_NO_ERROR)
//    {
//      cout << "GL Error : " << gluErrorString(errCode) << endl;
//    }
//
//    /* Set up some parameters for the format of the OpenGL texture */
//    glBindTexture(GL_TEXTURE_2D, texture->textureID);       /* Bind Our Texture */
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   /* Linear Filtered */
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   /* Linear Filtered */
//
//    /* Set the original image's size (incase it's not an exact square of a power of two) */
//    texture->imgH = imageSurface->h;
//    texture->imgW = imageSurface->w;
//
//    /* Determine the correct power of two greater than or equal to the original image's size */
//    texture->txH = powerOfTwo(imageSurface->h);
//    texture->txW = powerOfTwo(imageSurface->w);
//
//    /* Set the texture's alpha */
//    texture->alpha = 1;
//
//    /* Create a blank SDL_Surface (of the smallest size to fit the image) & copy imageSurface into it*/
//    //SDL_SetColorKey(imageSurface, SDL_SRCCOLORKEY,0);
//    //cvtSurface = SDL_ConvertSurface(imageSurface, &fmt, SDL_SWSURFACE);
//
//    if (imageSurface->format->Gmask)
//    {
//      tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, 0x20, imageSurface->format->Rmask, imageSurface->format->Gmask, imageSurface->format->Bmask, imageSurface->format->Amask);
//    }
//    else
//    {
//#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
//      tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, 0x20, 0x00FF, 0xFF00, 0xFF0000, 0x0000);
//#else
//      tempSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, texture->txW, texture->txH, 0x20, 0xFF000000, 0xFF0000, 0x00FF00, 0x000000);
//#endif
//    }
//    //SDL_BlitSurface( cvtSurface, &cvtSurface->clip_rect, tempSurface, &cvtSurface->clip_rect );
//    SDL_BlitSurface(imageSurface, &imageSurface->clip_rect, tempSurface, &imageSurface->clip_rect);
//
//    /* Fix the alpha values */
//    SDL_LockSurface(tempSurface);
//    p = (Uint32*)tempSurface->pixels;
//    for (y = (texture->txH - 1) ;y >= 0; y--)
//    {
//      for (x = (texture->txW - 1); x >= 0; x--)
//      {
//        if (p[x+y*texture->txW] != key)
//        {
//#if (SDL_BYTEORDER == SDL_LIL_ENDIAN)
//          p[x+y*texture->txW] = p[x+y*texture->txW] | 0xFF000000;
//#else
//          p[x+y*texture->txW] = p[x+y*texture->txW] | 0x0000FF;
//#endif
//        }
//      }
//    }
//    SDL_UnlockSurface(tempSurface);
//
//    /* actually create the OpenGL textures */
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tempSurface->w, tempSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tempSurface->pixels);
//
//    /* get rid of our SDL_Surfaces now that we're done with them */
//    SDL_FreeSurface(tempSurface);
//    SDL_FreeSurface(imageSurface);
//  }
//
//  return texture->textureID;
}

/********************> GLTexture::Release() <*****/
void  GLTexture::Release(GLTexture *texture)
{
  /* Delete the OpenGL texture */
  glDeleteTextures(1, &texture->textureID);
  texture->textureID = INVALID;

  /* Reset the other data */
  texture->imgH = texture->imgW = texture->txH = texture->txW = 0;
}
