#pragma once

#define _GLTEXTURE_H_

#include <SDL.h>
#include <SDL_opengl.h>

/**> DATA STRUCTURE: GLTexture <**/
typedef struct GLTexture
{
  const static GLuint INVALID = (GLuint)(-1);
  static GLenum filter_mag;
  static GLenum filter_min;


  GLuint  textureID;           // The OpenGL texture number
  GLint   internalFormat;      // GL_RGB or GL_RGBA
  GLsizei imgH, imgW;          // the height & width of the original image
  GLsizei txH, txW;            // the height/width of the the OpenGL texture (must be a power of two)
  GLfloat alpha;               // the alpha for the texture
  GLenum  repeat_u, repeat_v;  // the repeat modes (GL_CLAMP, GL_REPEAT)

  GLTexture() { textureID = GLTexture::INVALID; imgH=imgW=txH=txW=0; }

  //static GLuint  Load (GLTexture *texture, const char *filename);
  static GLuint  Load(GLTexture *texture, const char *filename, Uint32 key = (Uint32)(-1));

  static const GLuint  GetTextureID(GLTexture *texture);
  static const GLsizei GetImageHeight(GLTexture *texture);
  static const GLsizei GetImageWidth(GLTexture *texture);
  static const GLsizei GetTextureHeight(GLTexture *texture);
  static const GLsizei GetTextureWidth(GLTexture *texture);

  static void          SetAlpha(GLTexture *texture, GLfloat alpha);
  static const GLfloat GetAlpha(GLTexture *texture);

  static void  Release(GLTexture *texture);

  bool Valid() { return (INVALID!=textureID) && (imgW>0) && (imgH>0); }

  void Bind(GLenum target)
  {
    if(!glIsEnabled(target)) glEnable(target);

    glGetError();
    glBindTexture(target, textureID);
    int errCode = glGetError();
    if (errCode != GL_NO_ERROR)
    {
      cout << "ERROR glBindTexture() : \"" << gluErrorString(errCode) << "\"" << endl;
    }

    //glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter_mag);
    //glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter_min);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat_u);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat_v);
  }

} GLTexture;


