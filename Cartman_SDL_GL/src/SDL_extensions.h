#pragma once

#include <SDL.h>

#ifdef __cplusplus
#    include <cassert>
#    include <cstdio>
extern "C"
{
#else
#    include <assert.h>
#    include <stdio.h>
#endif

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define BOOL_TO_BIT(X)       ((X) ? 1 : 0 )
#define BIT_TO_BOOL(X)       ((1 == X) ? SDL_TRUE : SDL_FALSE )

#define HAS_SOME_BITS(XX,YY) (0 != ((XX)&(YY)))
#define HAS_ALL_BITS(XX,YY)  ((YY) == ((XX)&(YY)))
#define HAS_NO_BITS(XX,YY)   (0 == ((XX)&(YY)))
#define MISSING_BITS(XX,YY)  (HAS_SOME_BITS(XX,YY) && !HAS_ALL_BITS(XX,YY))

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
    struct s_oglx_video_parameters;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// SDL graphics info
    struct s_SDLX_screen_info
    {
        // JF - Added so that the video mode might be determined outside of the graphics code
        SDL_Surface * pscreen;

        SDL_Rect **   video_mode_list;

        char          szDriver[256];    ///< graphics driver name;

        int    d;                  ///< Screen bit depth
        int    z;                  ///< Screen z-buffer depth ( 8 unsupported )
        int    x;                  ///< Screen X size
        int    y;                  ///< Screen Y size

        Uint8  alpha;              ///< Screen alpha

        // SDL OpenGL attributes
        int red_d;  // SDL_GL_RED_SIZE Size of the framebuffer red component, in bits
        int grn_d;  // SDL_GL_GREEN_SIZE Size of the framebuffer green component, in bits
        int blu_d;  // SDL_GL_BLUE_SIZE Size of the framebuffer blue component, in bits
        int alp_d;  // SDL_GL_ALPHA_SIZE Size of the framebuffer alpha component, in bits
        int dbuff;  // SDL_GL_DOUBLEBUFFER 0 or 1, enable or disable double buffering
        int buf_d;  // SDL_GL_BUFFER_SIZE Size of the framebuffer, in bits
        int zbf_d;  // SDL_GL_DEPTH_SIZE Size of the depth buffer, in bits
        int stn_d;  // SDL_GL_STENCIL_SIZE Size of the stencil buffer, in bits
        int acr_d;  // SDL_GL_ACCUM_RED_SIZE Size of the accumulation buffer red component, in bits
        int acg_d;  // SDL_GL_ACCUM_GREEN_SIZE Size of the accumulation buffer green component, in bits
        int acb_d;  // SDL_GL_ACCUM_BLUE_SIZE Size of the accumulation buffer blue component, in bits
        int aca_d;  // SDL_GL_ACCUM_ALPHA_SIZE Size of the accumulation buffer alpha component, in bits

        // selected SDL bitfields
        unsigned hw_available: 1;
        unsigned wm_available: 1;
        unsigned blit_hw: 1;
        unsigned blit_hw_CC: 1;
        unsigned blit_hw_A: 1;
        unsigned blit_sw: 1;
        unsigned blit_sw_CC: 1;
        unsigned blit_sw_A: 1;

        unsigned is_sw: 1;          // SDL_SWSURFACE Surface is stored in system memory
        unsigned is_hw: 1;          // SDL_HWSURFACE Surface is stored in video memory
        unsigned use_asynch_blit: 1; // SDL_ASYNCBLIT Surface uses asynchronous blits if possible
        unsigned use_anyformat: 1;  // SDL_ANYFORMAT Allows any pixel-format (Display surface)
        unsigned use_hwpalette: 1;    // SDL_HWPALETTE Surface has exclusive palette
        unsigned is_doublebuf: 1;    // SDL_DOUBLEBUF Surface is double buffered (Display surface)
        unsigned is_fullscreen: 1;   // SDL_FULLSCREEN Surface is full screen (Display Surface)
        unsigned use_opengl: 1;       // SDL_OPENGL Surface has an OpenGL context (Display Surface)
        unsigned use_openglblit: 1;   // SDL_OPENGLBLIT Surface supports OpenGL blitting (Display Surface)
        unsigned sdl_resizable: 1;    // SDL_RESIZABLE Surface is resizable (Display Surface)
        unsigned use_hwaccel: 1;      // SDL_HWACCEL Surface blit uses hardware acceleration
        unsigned has_srccolorkey: 1;  // SDL_SRCCOLORKEY Surface use colorkey blitting
        unsigned use_rleaccel: 1;     // SDL_RLEACCEL Colorkey blitting is accelerated with RLE
        unsigned use_srcalpha: 1;     // SDL_SRCALPHA Surface blit uses alpha blending
        unsigned is_prealloc: 1;     // SDL_PREALLOC Surface uses preallocated memory
    };
    typedef struct s_SDLX_screen_info SDLX_screen_info_t;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
    struct s_SDLX_video_parameters
    {
        SDL_Surface * surface;

        Uint32   flags;
        int      doublebuffer;
        SDL_bool opengl;
        SDL_bool fullscreen;
        SDL_bool vsync;

        int multibuffers;
        int multisamples;
        int glacceleration;
        int colordepth[3];

        int width;
        int height;
        int depth;
    };

    typedef struct s_SDLX_video_parameters SDLX_video_parameters_t;

    SDL_bool SDLX_video_parameters_default(SDLX_video_parameters_t * v);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

    extern SDLX_screen_info_t sdl_scr;

    extern const Uint32 rmask;
    extern const Uint32 gmask;
    extern const Uint32 bmask;
    extern const Uint32 amask;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

    SDL_bool      SDLX_Get_Screen_Info( SDLX_screen_info_t * psi, SDL_bool display );
    SDL_Surface * SDLX_RequestVideoMode ( SDLX_video_parameters_t * v );

    SDLX_video_parameters_t * SDLX_set_mode(SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new );

    SDL_bool SDLX_ExpandFormat(SDL_PixelFormat * pformat);

#ifdef __cplusplus
};
#endif
