#include "SDL_extensions.h"
//#include "ogl_extensions.h"
//#include "ogl_debug.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SDLX_screen_info_t sdl_scr;

// create the color masks
// this will work if both endian systems think they have "RGBA" graphics
// if you need a different pixel format (ARGB or BGRA or whatever) this section
// will have to be changed to reflect that

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
const Uint32 rmask = 0x000000ff;
const Uint32 gmask = 0x0000ff00;
const Uint32 bmask = 0x00ff0000;
const Uint32 amask = 0xff000000;
#else
const Uint32 rmask = 0xff000000;
const Uint32 gmask = 0x00ff0000;
const Uint32 bmask = 0x0000ff00;
const Uint32 amask = 0x000000ff;
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SDL_bool SDLX_Get_Screen_Info( SDLX_screen_info_t * psi, SDL_bool display )
{
    int cnt;
    Uint32 init_flags = 0;
    SDL_Surface * ps;
    const SDL_VideoInfo * pvi;

    memset( psi, 0, sizeof( SDLX_screen_info_t ) );

    init_flags = SDL_WasInit( SDL_INIT_EVERYTHING );
    if ( 0 == init_flags )
    {
        if ( display ) fprintf( stderr, "ERROR: SDLX_Get_Screen_Info() called before initializing SDL\n" );
        return SDL_FALSE;
    }
    else if ( HAS_NO_BITS( init_flags, SDL_INIT_VIDEO ) )
    {
        if ( display ) fprintf( stderr, "ERROR: SDLX_Get_Screen_Info() called before initializing SDL video driver\n" );
        return SDL_FALSE;
    }

    ps  = SDL_GetVideoSurface();
    pvi = SDL_GetVideoInfo();
    psi->video_mode_list = SDL_ListModes( ps->format, ps->flags | SDL_FULLSCREEN );

    // log the video driver info
    SDL_VideoDriverName( psi->szDriver, sizeof( psi->szDriver ) );
    if ( display ) fprintf( stdout, "INFO: Using Video Driver - %s\n", psi->szDriver );

    //Grab all the available video modes
    if ( NULL != psi->video_mode_list )
    {
        psi->video_mode_list = SDL_ListModes( ps->format, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN | SDL_OPENGL | SDL_HWACCEL | SDL_SRCALPHA );
        if ( display ) fprintf( stdout, "INFO: Detecting available full-screen video modes...\n" );
        for ( cnt = 0; NULL != psi->video_mode_list[cnt]; ++cnt )
        {
            if ( display ) fprintf( stdout, "INFO: \tVideo Mode - %d x %d\n", psi->video_mode_list[cnt]->w, psi->video_mode_list[cnt]->h );
        };
    }

    psi->pscreen         = ps;

    psi->is_sw           = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_SWSURFACE ) );
    psi->is_hw           = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_HWSURFACE ) );
    psi->use_asynch_blit = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_ASYNCBLIT ) );
    psi->use_anyformat   = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_ANYFORMAT ) );
    psi->use_hwpalette   = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_HWPALETTE ) );
    psi->is_doublebuf    = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_DOUBLEBUF ) );
    psi->is_fullscreen   = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_FULLSCREEN ) );
    psi->use_opengl      = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_OPENGL ) );
    psi->use_openglblit  = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_OPENGLBLIT ) );
    psi->sdl_resizable   = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_RESIZABLE ) );
    psi->use_hwaccel     = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_HWACCEL ) );
    psi->has_srccolorkey = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_SRCCOLORKEY ) );
    psi->use_rleaccel    = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_RLEACCEL ) );
    psi->use_srcalpha    = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_SRCALPHA ) );
    psi->is_prealloc     = BOOL_TO_BIT( HAS_SOME_BITS( ps->flags, SDL_PREALLOC ) );

    psi->hw_available = pvi->hw_available;
    psi->wm_available = pvi->wm_available;
    psi->blit_hw      = pvi->blit_hw;
    psi->blit_hw_CC   = pvi->blit_hw_CC;
    psi->blit_hw_A    = pvi->blit_hw_A;
    psi->blit_sw      = pvi->blit_sw;
    psi->blit_sw_CC   = pvi->blit_sw_CC;
    psi->blit_sw_A    = pvi->blit_sw_A;

    // get any SDL-OpenGL info
    if ( 1 == psi->use_opengl )
    {
        SDL_GL_GetAttribute( SDL_GL_RED_SIZE,         &psi->red_d );
        SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE,       &psi->grn_d );
        SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE,        &psi->blu_d );
        SDL_GL_GetAttribute( SDL_GL_ALPHA_SIZE,       &psi->alp_d );
        SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER,     &psi->dbuff );
        SDL_GL_GetAttribute( SDL_GL_BUFFER_SIZE,      &psi->buf_d );
        SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE,       &psi->zbf_d );
        SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE,     &psi->stn_d );
        SDL_GL_GetAttribute( SDL_GL_ACCUM_RED_SIZE,   &psi->acr_d );
        SDL_GL_GetAttribute( SDL_GL_ACCUM_GREEN_SIZE, &psi->acg_d );
        SDL_GL_GetAttribute( SDL_GL_ACCUM_BLUE_SIZE,  &psi->acb_d );
        SDL_GL_GetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, &psi->aca_d );
    }

    psi->d = ps->format->BitsPerPixel;
    psi->x = ps->w;
    psi->y = ps->h;
    psi->z = psi->zbf_d;

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
SDL_Surface * SDLX_RequestVideoMode( SDLX_video_parameters_t * v )
{

    Uint32 flags;
    SDL_Surface * ret = NULL;

    if ( NULL == v ) return ret;

    flags = v->flags;
    if ( 0 != v->doublebuffer ) flags |= SDL_DOUBLEBUF;
    if ( v->opengl ) flags |= SDL_OPENGL;
    if ( v->fullscreen ) flags |= SDL_FULLSCREEN;

    ret = SDL_SetVideoMode( v->width, v->height, v->depth, flags );

    // fix bad colordepth
    if (( 0 == v->colordepth[0] && 0 == v->colordepth[1] && 0 == v->colordepth[2] ) ||
        ( v->colordepth[0] + v->colordepth[1] + v->colordepth[2] > v->depth ) )
    {
        if ( v->depth > 24 )
        {
            v->colordepth[0] = v->colordepth[1] = v->colordepth[2] = v->depth / 4;
        }
        else
        {
            // do a kludge in case we have something silly like 16 bit "highcolor" mode
            v->colordepth[0] = v->colordepth[2] = v->depth / 3;
            v->colordepth[1] = v->depth - v->colordepth[0] - v->colordepth[2];
        }
    }

    if ( v->opengl )
    {
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, v->multibuffers );
        SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, v->multisamples );
        SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, v->glacceleration );
        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,       v->doublebuffer );

#ifndef __unix__
        // Under Unix we cannot specify these, we just get whatever format
        // the framebuffer has, specifying depths > the framebuffer one
        // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"

        SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   v->colordepth[0] );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, v->colordepth[1] );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  v->colordepth[2] );
        SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, v->depth );
#endif

        // Wait for vertical synchronization?
        if ( v->vsync )
        {
            // Fedora 7 doesn't suuport SDL_GL_SWAP_CONTROL, but we use this  nvidia extension instead.
#if defined(__unix__)
            SDL_putenv( "__GL_SYNC_TO_VBLANK=1" );
#else
            /* Turn on vsync, this works on Windows. */
            if ( SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, 1 ) < 0 )
            {
                //fprintf( stdout, "WARN: SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1) failed - %s\n\t\", SDL_GetError() );
            }
#endif
        }
    }

    if ( NULL == ret )
    {
        fprintf( stdout, "WARN: SDL unable to set video mode with current parameters - \n\t\"%s\"\n", SDL_GetError() );

        if ( v->opengl )
        {
            fprintf( stdout, "INFO: \tUsing OpenGL\n" );
            fprintf( stdout, "INFO: \tSDL_GL_MULTISAMPLEBUFFERS == %d\n", v->multibuffers );
            fprintf( stdout, "INFO: \tSDL_GL_MULTISAMPLESAMPLES == %d\n", v->multisamples );
            fprintf( stdout, "INFO: \tSDL_GL_ACCELERATED_VISUAL == %d\n", v->glacceleration );
            fprintf( stdout, "INFO: \tSDL_GL_DOUBLEBUFFER       == %d\n", v->doublebuffer );

#ifndef __unix__
            // Under Unix we cannot specify these, we just get whatever format
            // the framebuffer has, specifying depths > the framebuffer one
            // will cause SDL_SetVideoMode to fail with: "Unable to set video mode: Couldn't find matching GLX visual"

            fprintf( stdout, "INFO: \tSDL_GL_RED_SIZE   == %d\n", v->colordepth[0] );
            fprintf( stdout, "INFO: \tSDL_GL_GREEN_SIZE == %d\n", v->colordepth[1] );
            fprintf( stdout, "INFO: \tSDL_GL_BLUE_SIZE  == %d\n", v->colordepth[2] );
            fprintf( stdout, "INFO: \tSDL_GL_DEPTH_SIZE == %d\n", v->depth );
#endif
        }

        fprintf( stdout, "INFO: \t%s\n", (( 0 != v->doublebuffer ) ? "DOUBLE BUFFER" : "SINGLE BUFFER" ) );
        fprintf( stdout, "INFO: \t%s\n", ( v->fullscreen   ? "FULLSCREEN"    : "WINDOWED" ) );
        fprintf( stdout, "INFO: \twidth == %d, height == %d, depth == %d\n", v->width, v->height, v->depth );

    }
    else
    {
        // report current graphics values

        fprintf( stdout, "INFO: SDL set video mode to the current parameters\n", SDL_GetError() );

        SDLX_Get_Screen_Info( &sdl_scr, SDL_TRUE );

        v->opengl = ( SDL_bool )HAS_SOME_BITS( ret->flags, SDL_OPENGL );

        if ( v->opengl )
        {
            fprintf( stdout, "INFO: \tUsing OpenGL\n" );

            if ( 0 == SDL_GL_GetAttribute( SDL_GL_MULTISAMPLEBUFFERS, &( v->multibuffers ) ) )
            {
                fprintf( stdout, "INFO: \tGL_MULTISAMPLEBUFFERS == %d\n", v->multibuffers );
            };

            if ( 0 == SDL_GL_GetAttribute( SDL_GL_MULTISAMPLESAMPLES, &( v->multisamples ) ) )
            {
                fprintf( stdout, "INFO: \tSDL_GL_MULTISAMPLESAMPLES == %d\n", v->multisamples );
            };

            if ( 0 == SDL_GL_GetAttribute( SDL_GL_ACCELERATED_VISUAL, &( v->glacceleration ) ) )
            {
                fprintf( stdout, "INFO: \tSDL_GL_ACCELERATED_VISUAL == %d\n", v->glacceleration );
            };

            if ( 0 == SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &( v->doublebuffer ) ) )
            {
                fprintf( stdout, "INFO: \tSDL_GL_DOUBLEBUFFER == %d\n", v->doublebuffer );
            };

            SDL_GL_GetAttribute( SDL_GL_RED_SIZE, v->colordepth + 0 );
            {
                fprintf( stdout, "INFO: \tSDL_GL_RED_SIZE == %d\n", v->colordepth[0] );
            };

            SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, v->colordepth + 1 );
            {
                fprintf( stdout, "INFO: \tSDL_GL_GREEN_SIZE == %d\n", v->colordepth[1] );
            };

            SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE,  v->colordepth + 2 );
            {
                fprintf( stdout, "INFO: \tSDL_GL_BLUE_SIZE == %d\n", v->colordepth[2] );
            };

            SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &( v->depth ) );
            {
                fprintf( stdout, "INFO: \tSDL_GL_DEPTH_SIZE == %d\n", v->depth );
            };

        }

        fprintf( stdout, "INFO: \t%s\n", ( HAS_SOME_BITS( ret->flags, SDL_DOUBLEBUF )  ? "SDL DOUBLE BUFFER" : "SDL SINGLE BUFFER" ) );
        fprintf( stdout, "INFO: \t%s\n", ( HAS_SOME_BITS( ret->flags, SDL_FULLSCREEN )  ? "FULLSCREEN"    : "WINDOWED" ) );

        v->width  = ret->w;
        v->height = ret->h;
        v->depth  = ret->format->BitsPerPixel;
        fprintf( stdout, "INFO: \twidth == %d, height == %d, depth == %d\n", v->width, v->height, v->depth );
    }

    return ret;

}

//--------------------------------------------------------------------------------------------
SDL_bool SDLX_video_parameters_default( SDLX_video_parameters_t * v )
{
    if ( NULL == v ) return SDL_FALSE;

    v->surface      = NULL;

    v->flags        = 0;
    v->doublebuffer = SDL_TRUE;
    v->opengl       = SDL_TRUE;
    v->fullscreen   = SDL_FALSE;

    v->multibuffers   = 1;
    v->multisamples   = 16;
    v->glacceleration = 1;

    v->colordepth[0] = 8;
    v->colordepth[1] = 8;
    v->colordepth[2] = 8;

    v->width  = 640;
    v->height = 480;
    v->depth  =  32;

    return SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
SDLX_video_parameters_t * SDLX_set_mode( SDLX_video_parameters_t * v_old, SDLX_video_parameters_t * v_new )
{
    /// @details BB> let SDL try to set a new video mode.

    SDLX_video_parameters_t param_old, param_new;
    SDLX_video_parameters_t * retval = NULL;

    // initialize v_old and param_old
    if ( NULL == v_old )
    {
        SDLX_video_parameters_default( &param_old );
        v_old = &param_old;
    }
    else
    {
        memcpy( &param_old, v_old, sizeof( SDLX_video_parameters_t ) );
    }

    // initialize v_new and param_new
    if ( NULL == v_new )
    {
        SDLX_video_parameters_default( &param_new );
        v_new = &param_new;
    }
    else
    {
        memcpy( &param_new, v_new, sizeof( SDLX_video_parameters_t ) );
    }

    // assume any problem with setting the graphics mode is with the multisampling
    while ( param_new.multisamples > 0 )
    {
        v_new->surface = SDLX_RequestVideoMode( &param_new );
        if ( NULL != v_new->surface ) break;
        param_new.multisamples >>= 1;
    };

    if ( NULL == v_new->surface )
    {
        // we can't have any multi...

        param_new.multibuffers = 0;
        param_new.multisamples = 0;

        v_new->surface = SDLX_RequestVideoMode( &param_new );
    }

    if ( NULL != v_new->surface )
    {
        retval = v_new;
    }
    else
    {
        v_old->surface = SDLX_RequestVideoMode( &param_old );

        if ( NULL == v_old->surface )
        {
            //log_error("Could not restore the old video mode. Terminating.\n");
            exit( -1 );
        }

        retval = v_old;
    }

    return retval;
};

//--------------------------------------------------------------------------------------------
SDL_bool SDLX_ExpandFormat( SDL_PixelFormat * pformat )
{
    // use the basic screen format to create a surface with proper alpha support

    int i;

    if ( NULL == pformat ) return SDL_FALSE;

    if ( pformat->BitsPerPixel > 24 )
    {
        pformat->Amask = amask;
        pformat->Bmask = bmask;
        pformat->Gmask = gmask;
        pformat->Rmask = rmask;

        for ( i = 0; i < sdl_scr.d && ( pformat->Amask & ( 1 << i ) ) == 0; i++ );

        if ( 0 == ( pformat->Amask & ( 1 << i ) ) )
        {
            // no alpha bits available
            pformat->Ashift = 0;
            pformat->Aloss  = 8;
        }
        else
        {
            // normal alpha channel
            pformat->Ashift = i;
            pformat->Aloss  = 0;
        }
    }

    return SDL_TRUE;
}
