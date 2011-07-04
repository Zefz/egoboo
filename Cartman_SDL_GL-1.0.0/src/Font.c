/* Egoboo - Font.c
 * True-type font drawing functionality.  Uses Freetype 2 & OpenGL
 * to do it's business.
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
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "Font.h"

#include "SDL_GL_extensions.h"
#include "egoboo_config.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#define MAX_FNT_MESSAGE 1024

static SDL_Surface * fnt_printf( Font *font, SDL_Color color, const char *format, ... );

static void fnt_streamText_SDL( SDL_Surface * surface, TTF_Font * font, int x, int y, SDL_Color color, const char *text );
static void fnt_drawText_SDL( SDL_Surface * surface, TTF_Font * font, int x, int y, SDL_Color color, const char *text );

static void fnt_streamText_OGL( Font * font, int x, int y, const char *text );
static void fnt_drawText_OGL( Font *font, int x, int y, const char *text );

static char * writeFont( const char *format, va_list args )
{
    static char fntBuffer[MAX_FNT_MESSAGE];

    vsnprintf( fntBuffer, MAX_FNT_MESSAGE - 1, format, args );

    return fntBuffer;
}

struct Font
{
    TTF_Font *ttfFont;

    GLuint texture;
    GLfloat texCoords[4];
};

Font* fnt_loadFont( const char *fileName, int pointSize )
{
    Font *newFont;
    TTF_Font *ttfFont;

    // Make sure the TTF library was initialized
    if ( !TTF_WasInit() )
    {
        if ( TTF_Init() != -1 )
        {
            atexit( TTF_Quit );
        }
        else
        {
            printf( "fnt_loadFont: Could not initialize SDL_TTF!\n" );
            return NULL;
        }
    }

    // Try and open the font
    ttfFont = TTF_OpenFont( fileName, pointSize );
    if ( !ttfFont )
    {
        // couldn't open it, for one reason or another
        return NULL;
    }

    // Everything looks good
    newFont = ( Font* )malloc( sizeof( Font ) );
    newFont->ttfFont = ttfFont;
    newFont->texture = 0;

    return newFont;
}

void fnt_freeFont( Font *font )
{
    if ( font )
    {
        TTF_CloseFont( font->ttfFont );
        glDeleteTextures( 1, &font->texture );
        free( font );
    }
}
//
//void fnt_drawText( Font *font, int x, int y, const char *text )
//{
//  SDL_Surface *textSurf;
//  SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };
//
//  if ( font )
//  {
//    // Let TTF render the text
//    textSurf = TTF_RenderText_Blended( font->ttfFont, text, color );
//
//    // Does this font already have a texture?  If not, allocate it here
//    if ( font->texture == 0 )
//    {
//      glGenTextures( 1, &font->texture );
//    }
//
//    // Copy the surface to the texture
//    if ( copySurfaceToTexture( textSurf, font->texture, font->texCoords ) )
//    {
//      // And draw the darn thing
//      glBegin( GL_TRIANGLE_STRIP );
//      glTexCoord2f( font->texCoords[0], font->texCoords[1] );
//      glVertex2i( x, y );
//      glTexCoord2f( font->texCoords[2], font->texCoords[1] );
//      glVertex2i( x + textSurf->w, y );
//      glTexCoord2f( font->texCoords[0], font->texCoords[3] );
//      glVertex2i( x, y + textSurf->h );
//      glTexCoord2f( font->texCoords[2], font->texCoords[3] );
//      glVertex2i( x + textSurf->w, y + textSurf->h );
//      glEnd();
//    }
//
//    // Done with the surface
//    SDL_FreeSurface( textSurf );
//  }
//}
//
//void fnt_getTextSize( Font *font, const char *text, int *width, int *height )
//{
//  if ( font )
//  {
//    TTF_SizeText( font->ttfFont, text, width, height );
//  }
//}
//
/** font_drawTextBox
 * Draws a text string into a box, splitting it into lines according to newlines in the string.
 * NOTE: Doesn't pay attention to the width/height arguments yet.
 *
 * font    - The font to draw with
 * text    - The text to draw
 * x       - The x position to start drawing at
 * y       - The y position to start drawing at
 * width   - Maximum width of the box (not implemented)
 * height  - Maximum height of the box (not implemented)
 * spacing - Amount of space to move down between lines. (usually close to your font size)
 */
void fnt_drawTextBox( Font *font, const char *text, int x, int y, int width, int height, int spacing )
{
    size_t len;
    char *buffer, *line;

    if ( !font ) return;

    // If text is empty, there's nothing to draw
    if ( !text || !text[0] ) return;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = ( char * )calloc( 1, len + 1 );
    strncpy( buffer, text, len );

    line = strtok( buffer, "\n" );
    while ( line != NULL )
    {
        fnt_drawText_OGL( font, x, y, line );
        y += spacing;
        line = strtok( NULL, "\n" );
    }
    free( buffer );
}

void fnt_getTextBoxSize( Font *font, const char *text, int spacing, int *width, int *height )
{
    char *buffer, *line;
    size_t len;
    int tmp_w, tmp_h;

    if ( !font ) return;
    if ( !text || !text[0] ) return;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = ( char * )calloc( 1, len + 1 );
    strncpy( buffer, text, len );

    line = strtok( buffer, "\n" );
    *width = *height = 0;
    while ( line != NULL )
    {
        TTF_SizeText( font->ttfFont, line, &tmp_w, &tmp_h );
        *width = ( *width > tmp_w ) ? *width : tmp_w;
        *height += spacing;

        line = strtok( NULL, "\n" );
    }
    free( buffer );
}

SDL_Surface * fnt_printf( Font *font, SDL_Color color, const char *format, ... )
{
    char * msg;
    va_list args;

    va_start( args, format );
    msg = writeFont( format, args );
    va_end( args );

    return TTF_RenderText_Blended( font->ttfFont, msg, color );
}
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fnt_printf_SDL( SDL_Surface * surface, TTF_Font * font, int x, int y, SDL_Color color, const char * format, ... )
{
    char * text;
    va_list args;

    // If format text is empty, there's nothing to draw
    if ( !format || !format[0] ) return;

    va_start( args, format );
    text = writeFont( format, args );
    va_end( args );

    fnt_streamText_SDL( surface, font, x, y, color, text );
};

//--------------------------------------------------------------------------------------------
void fnt_streamText_SDL( SDL_Surface * surface, TTF_Font * font, int x, int y, SDL_Color color, const char *text )
{
    size_t len;
    char *buffer, *line;
    Uint16 spacing;

    if ( NULL == surface ) return;

    if ( NULL == font ) return;
    spacing = TTF_FontHeight( font );

    // If text is empty, there's nothing to draw
    if ( NULL == text || '\0' == text[0] ) return;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = ( char * )calloc( 1, len + 1 );
    strncpy( buffer, text, len );

    line = strtok( buffer, "\n" );
    while ( line != NULL )
    {
        fnt_drawText_SDL( surface, font, x, y, color, line );

        y += spacing;
        line = strtok( NULL, "\n" );
    }
    free( buffer );
};

//--------------------------------------------------------------------------------------------
void fnt_drawText_SDL( SDL_Surface * surface, TTF_Font * font, int x, int y, SDL_Color color, const char *text )
{
    SDL_Surface * ptmp;
    Uint16 spacing;
    SDL_Rect rtmp;

    if ( !font ) return;
    spacing = TTF_FontHeight( font );

    // If text is empty, there's nothing to draw
    if ( !text || !text[0] ) return;

    ptmp = TTF_RenderText_Blended( font, text, color );
    if ( NULL != ptmp )
    {
        rtmp.x = x;
        rtmp.y = y;
        rtmp.w = ptmp->w;
        rtmp.h = ptmp->h;

        SDL_BlitSurface( ptmp, NULL, surface, &rtmp );
        SDL_FreeSurface( ptmp );
    };

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fnt_printf_OGL( Font * font, int x, int y, const char * format, ... )
{
    char * text;
    va_list args;

    // If format text is empty, there's nothing to draw
    if ( !format || !format[0] ) return;

    va_start( args, format );
    text = writeFont( format, args );
    va_end( args );

    fnt_streamText_OGL( font, x, y, text );
};

//--------------------------------------------------------------------------------------------
void fnt_streamText_OGL( Font * font, int x, int y, const char *text )
{
    size_t len;
    char *buffer, *line;
    Uint16 spacing;

    if ( NULL == font ) return;
    spacing = TTF_FontHeight( font->ttfFont );

    // If text is empty, there's nothing to draw
    if ( NULL == text || '\0' == text[0] ) return;

    // Split the passed in text into separate lines
    len = strlen( text );
    buffer = ( char * )calloc( 1, len + 1 );
    strncpy( buffer, text, len );

    line = strtok( buffer, "\n" );
    while ( line != NULL )
    {
        fnt_drawText_OGL( font, x, y, line );

        y += spacing;
        line = strtok( NULL, "\n" );
    }
    free( buffer );
};

//--------------------------------------------------------------------------------------------
void fnt_drawText_OGL( Font *font, int x, int y, const char *text )
{
    SDL_Surface *textSurf;
    SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };
    if ( NULL == font ) return;

    // Let TTF render the text
    textSurf = TTF_RenderText_Blended( font->ttfFont, text, color );
    if ( NULL == textSurf ) return;

    // Does this font already have a texture?  If not, allocate it here
    if (( GLuint )( ~0 ) == font->texture )
    {
        glGenTextures( 1, &font->texture );
    }

    // Copy the surface to the texture
    if ( SDL_GL_uploadSurface( textSurf, font->texture, font->texCoords ) )
    {
        // And draw the darn thing
        glBegin( GL_TRIANGLE_STRIP );
        {
            glTexCoord2f( font->texCoords[0], font->texCoords[1] );
            glVertex2i( x, y );
            glTexCoord2f( font->texCoords[2], font->texCoords[1] );
            glVertex2i( x + textSurf->w, y );
            glTexCoord2f( font->texCoords[0], font->texCoords[3] );
            glVertex2i( x, y + textSurf->h );
            glTexCoord2f( font->texCoords[2], font->texCoords[3] );
            glVertex2i( x + textSurf->w, y + textSurf->h );
        }
        glEnd();
    }

    // Done with the surface
    SDL_FreeSurface( textSurf );
}
