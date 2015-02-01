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

/// @file game/graphic.c
/// @brief Simple Egoboo renderer
/// @details All sorts of stuff related to drawing the game

#include "egolib/egolib.h"
#include "egolib/bsp.h"
#include "game/graphic.h"
#include "game/graphic_prt.h"
#include "game/graphic_mad.h"
#include "game/graphic_fan.h"
#include "game/graphic_billboard.h"
#include "game/graphic_texture.h"
#include "game/renderer_2d.h"
#include "game/renderer_3d.h"
#include "game/network_server.h"
#include "game/mad.h"
#include "game/bsp.h"
#include "game/player.h"
#include "game/collision.h"
#include "game/script.h"
#include "game/input.h"
#include "game/menu.h"
#include "game/script_compile.h"
#include "game/game.h"
#include "game/ui.h"
#include "game/lighting.h"
#include "game/egoboo.h"
#include "game/char.h"
#include "game/particle.h"
#include "game/enchant.h"
#include "game/mesh.h"

#include "game/profiles/Profile.hpp"
#include "game/graphics/CameraSystem.hpp"
#include "game/profiles/ProfileSystem.hpp"
#include "game/module/Module.hpp"

#include "game/module/ObjectHandler.hpp"
#include "game/EncList.h"
#include "game/PrtList.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

const size_t dolist_t::CAPACITY;

#define SPARKLE_SIZE ICON_SIZE
#define SPARKLE_AND  (SPARKLE_SIZE - 1)

#define BLIPSIZE 6

// camera frustum optimization
//#define ROTMESH_TOPSIDE                  50
//#define ROTMESH_BOTTOMSIDE               50
//#define ROTMESH_UP                       30
//#define ROTMESH_DOWN                     30

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Structure for keeping track of which dynalights are visible
struct dynalight_registry_t
{
    int         reference;
    ego_frect_t bound;
};

//--------------------------------------------------------------------------------------------
// the renderlist manager
//--------------------------------------------------------------------------------------------

#define RENDERLIST_ARY_INIT_VALS                                     \
    {                                                                \
        false,          /* bool         started                   */ \
        0,              /* size_t       free_count                */ \
        /*nothing */    /* int          free_lst[MAX_RENDER_LISTS]*/ \
        /*nothing */    /* renderlist_t lst[MAX_RENDER_LISTS]     */ \
    }

#define RENDERLIST_MGR_INIT_VALS                                \
    {                                                           \
        false,                   /* bool             started */ \
        RENDERLIST_ARY_INIT_VALS /* renderlist_ary_t ary     */ \
    }


#if 0
renderlist_mgr_t::renderlist_mgr_t()
	: started(false), ary()
{
}
#endif

//--------------------------------------------------------------------------------------------
// the dolist manager
//--------------------------------------------------------------------------------------------

#define DOLIST_ARY_INIT_VALS                                  \
    {                                                         \
        false,          /* bool     started                */ \
        0,              /* size_t   free_count             */ \
        /*nothing */    /* int      free_lst[MAX_DO_LISTS] */ \
        /*nothing */    /* dolist_t lst[MAX_DO_LISTS]      */ \
    }

#define DOLIST_MGR_INIT_VALS                            \
    {                                                   \
        false,               /* bool         started */ \
        DOLIST_ARY_INIT_VALS /* dolist_ary_t ary     */ \
    }

#if 0
dolist_mgr_t::dolist_mgr_t()
	: started(false), ary()
{
}
#endif


//--------------------------------------------------------------------------------------------
// dynalist
//--------------------------------------------------------------------------------------------

/// The active dynamic lights
struct dynalist_t
{
    int frame; ///< The last frame in shich the list was updated. @a -1 if there was no update yet.
    size_t size; ///< The size of the list.
    dynalight_data_t lst[TOTAL_MAX_DYNA];  ///< The list.
};

static gfx_rv dynalist_init(dynalist_t *self);
static gfx_rv do_grid_lighting(renderlist_t *self, dynalist_t *dynalist, std::shared_ptr<Camera> camera);

#define DYNALIST_INIT { -1 /* frame */, 0 /* count */ }



//--------------------------------------------------------------------------------------------
// special functions and data related to tiled texture "optimization"
//--------------------------------------------------------------------------------------------

/// has this system been initialized?
static bool _gfx_system_mesh_textures_initialized = false;

// the "small" textures
static oglx_texture_t mesh_tx_sml[MESH_IMG_COUNT];
static int mesh_tx_sml_cnt = 0;

// the "large" textures
static oglx_texture_t mesh_tx_big[MESH_IMG_COUNT];
static int mesh_tx_big_cnt = 0;

// actually break up a texture
static int  gfx_decimate_one_mesh_texture(oglx_texture_t *src_tx, oglx_texture_t *tx_lst, size_t tx_lst_cnt, int minification);
static void gfx_decimate_all_mesh_textures();

// control the state of the textures
static void gfx_system_begin_decimated_textures();
static void gfx_system_end_decimated_textures();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

PROFILE_DECLARE( render_scene_init );
PROFILE_DECLARE( render_scene_mesh );
PROFILE_DECLARE( render_scene_solid );
PROFILE_DECLARE( render_scene_water );
PROFILE_DECLARE( render_scene_trans );

PROFILE_DECLARE( gfx_make_renderlist );
PROFILE_DECLARE( gfx_make_dolist );
PROFILE_DECLARE( do_grid_lighting );
PROFILE_DECLARE( light_fans );
PROFILE_DECLARE( gfx_update_all_chr_instance );
PROFILE_DECLARE( update_all_prt_instance );

PROFILE_DECLARE( render_scene_mesh_dolist_sort );
PROFILE_DECLARE( render_scene_mesh_ndr );
PROFILE_DECLARE( render_scene_mesh_drf_back );
PROFILE_DECLARE( render_scene_mesh_ref );
PROFILE_DECLARE( render_scene_mesh_ref_chr );
PROFILE_DECLARE( render_scene_mesh_drf_solid );
PROFILE_DECLARE( render_scene_mesh_render_shadows );

float time_draw_scene       = 0.0f;
float time_render_scene_init  = 0.0f;
float time_render_scene_mesh  = 0.0f;
float time_render_scene_solid = 0.0f;
float time_render_scene_water = 0.0f;
float time_render_scene_trans = 0.0f;

float time_render_scene_init_renderlist_make         = 0.0f;
float time_render_scene_init_dolist_make             = 0.0f;
float time_render_scene_init_do_grid_dynalight       = 0.0f;
float time_render_scene_init_light_fans              = 0.0f;
float time_render_scene_init_update_all_chr_instance = 0.0f;
float time_render_scene_init_update_all_prt_instance = 0.0f;

float time_render_scene_mesh_dolist_sort    = 0.0f;
float time_render_scene_mesh_ndr            = 0.0f;
float time_render_scene_mesh_drf_back       = 0.0f;
float time_render_scene_mesh_ref            = 0.0f;
float time_render_scene_mesh_ref_chr        = 0.0f;
float time_render_scene_mesh_drf_solid      = 0.0f;
float time_render_scene_mesh_render_shadows = 0.0f;

Uint32          game_frame_all   = 0;             ///< The total number of frames drawn so far
Uint32          menu_frame_all   = 0;             ///< The total number of frames drawn so far

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int GFX_WIDTH  = 800;
int GFX_HEIGHT = 600;

gfx_config_t     gfx;

float            indextoenvirox[EGO_NORMAL_COUNT];
float            lighttoenviroy[256];

Uint8   mapon         = false;
Uint8   mapvalid      = false;
Uint8   youarehereon  = false;

size_t  blip_count    = 0;
float   blip_x[MAXBLIP];
float   blip_y[MAXBLIP];
Uint8   blip_c[MAXBLIP];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static gfx_error_stack_t gfx_error_stack = GFX_ERROR_STACK_INIT;

static SDLX_video_parameters_t sdl_vparam;
static oglx_video_parameters_t ogl_vparam;

static SDL_bool _sdl_initialized_graphics = SDL_FALSE;
static bool   _ogl_initialized          = false;

static float sinlut[MAXLIGHTROTATION];
static float coslut[MAXLIGHTROTATION];

// Interface stuff
static irect_t tabrect[NUMBAR];            // The tab rectangles
static irect_t barrect[NUMBAR];            // The bar rectangles
static irect_t bliprect[COLOR_MAX];        // The blip rectangles
static irect_t maprect;                    // The map rectangle

static bool  gfx_page_flip_requested  = false;
static bool  gfx_page_clear_requested = true;

static float dynalight_keep = 0.9f;

egolib_timer_t gfx_update_timer;

static egolib_throttle_t gfx_throttle = EGOLIB_THROTTLE_INIT;

static dynalist_t _dynalist = DYNALIST_INIT;

static renderlist_mgr_t _renderlist_mgr_data = RENDERLIST_MGR_INIT_VALS;
static dolist_mgr_t     _dolist_mgr_data = DOLIST_MGR_INIT_VALS;

static Ego::DynamicArray<BSP_leaf_t *> _renderlist_colst = DYNAMIC_ARY_INIT_VALS;
static Ego::DynamicArray<BSP_leaf_t *> _dolist_colst = DYNAMIC_ARY_INIT_VALS;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/**
 * @brief
 *	Initialize the SDL graphics system.
 */
static void gfx_system_init_SDL_graphics();

/**
 * @brief
 *	Uninitialize the SDL graphics system.
 */
static void gfx_system_uninit_SDL_graphics();

static void gfx_reset_timers();

static void _flip_pages();

static void gfx_update_fps_clock();
static void gfx_update_fps();

static gfx_rv light_fans( renderlist_t * prlist );

static gfx_rv render_scene_init( renderlist_t * prlist, dolist_t * pdolist, dynalist_t * pdylist, std::shared_ptr<Camera> pcam );
static gfx_rv render_scene_mesh_ndr( const renderlist_t * prlist );
static gfx_rv render_scene_mesh_drf_back( const renderlist_t * prlist );
static gfx_rv render_scene_mesh_ref( std::shared_ptr<Camera> pcam, const renderlist_t * prlist, const dolist_t * pdolist );
static gfx_rv render_scene_mesh_ref_chr( const renderlist_t * prlist );
static gfx_rv render_scene_mesh_drf_solid( const renderlist_t * prlist );
static gfx_rv render_scene_mesh_render_shadows( const dolist_t * pdolist );
static gfx_rv render_scene_mesh( std::shared_ptr<Camera> pcam, const renderlist_t * prlist, const dolist_t * pdolist );
static gfx_rv render_scene_solid( std::shared_ptr<Camera> pcam, dolist_t * pdolist );
static gfx_rv render_scene_trans( std::shared_ptr<Camera> pcam, dolist_t * pdolist );
static gfx_rv render_scene( std::shared_ptr<Camera> pcam, const int render_list_index, const int dolist_index );
static gfx_rv render_fans_by_list( const ego_mesh_t * pmesh, const renderlist_lst_t * rlst );
static void   render_shadow( const CHR_REF character );
static void   render_bad_shadow( const CHR_REF character );
static gfx_rv render_water( renderlist_t * prlist );
static void   render_shadow_sprite( float intensity, GLvertex v[] );
static gfx_rv render_world_background( std::shared_ptr<Camera> pcam, const TX_REF texture );
static gfx_rv render_world_overlay( std::shared_ptr<Camera> pcam, const TX_REF texture );


/**
 * @brief
 *	Find characters that need to be drawn and put them in the list.
 * @param dolist
 *	the list to add characters to
 * @param camera
 *	the camera
 */
static gfx_rv gfx_make_dolist(dolist_t *dolist, std::shared_ptr<Camera> camera);
static gfx_rv gfx_make_renderlist( renderlist_t * prlist, std::shared_ptr<Camera> pcam );
static gfx_rv gfx_make_dynalist( dynalist_t * pdylist, std::shared_ptr<Camera> pcam );

static float draw_one_xp_bar( float x, float y, Uint8 ticks );
static float draw_character_xp_bar( const CHR_REF character, float x, float y );
static void  draw_all_status();
static void  draw_map();
static float draw_fps( float y );
static float draw_help( float y );
static float draw_debug( float y );
static float draw_timer( float y );
static float draw_game_status( float y );
static void  draw_hud();
static void  draw_inventory();

static gfx_rv gfx_capture_mesh_tile( ego_tile_info_t * ptile );


static void gfx_reload_decimated_textures();

static gfx_rv update_one_chr_instance( GameObject * pchr );
static gfx_rv gfx_update_all_chr_instance();
static gfx_rv gfx_update_flashing( dolist_t * pdolist );

static gfx_rv light_fans_throttle_update( ego_mesh_t * pmesh, ego_tile_info_t * ptile, int fan, float threshold );
static gfx_rv light_fans_update_lcache( renderlist_t * prlist );
static gfx_rv light_fans_update_clst( renderlist_t * prlist );
static bool sum_global_lighting( lighting_vector_t lighting );
static float calc_light_rotation( int rotation, int normal );
static float calc_light_global( int rotation, int normal, float lx, float ly, float lz );

//static void gfx_init_icon_data();
static void   gfx_init_bar_data();
static void   gfx_init_blip_data();
static void   gfx_init_map_data();

static SDL_Surface *gfx_create_SDL_Surface(int w, int h);

//--------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------
SDL_Surface *gfx_create_SDL_Surface(int width, int height)
{
    SDL_PixelFormat pixelFormat;
	if (nullptr == sdl_scr.pscreen)
	{
		return nullptr;
	}
	// Same format as the screen extended by an alpha channel.
    memcpy(&pixelFormat, sdl_scr.pscreen->format, sizeof(SDL_PixelFormat));
    SDLX_ExpandFormat(&pixelFormat);

	return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, pixelFormat.BitsPerPixel, pixelFormat.Rmask, pixelFormat.Gmask, pixelFormat.Bmask, pixelFormat.Amask);
}

//--------------------------------------------------------------------------------------------
// renderlist_ary implementation
//--------------------------------------------------------------------------------------------

gfx_rv renderlist_lst_t::reset(renderlist_lst_t *self)
{
	if (nullptr == self)
	{
		return gfx_error;
	}
    self->count = 0;
    self->lst[0].index = INVALID_TILE; /// @todo This is redundant.

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_lst_t::push(renderlist_lst_t *self, Uint32 index, float distance)
{
    if (nullptr == self) return gfx_error;

    if (self->count >= renderlist_lst_t::CAPACITY) return gfx_fail;

	self->lst[self->count].index = index;
	self->lst[self->count].distance = distance;

	self->count++;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// renderlist implementation
//--------------------------------------------------------------------------------------------

renderlist_t *renderlist_t::init(renderlist_t *self, ego_mesh_t *mesh)
{
    if (nullptr == self)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return nullptr;
    }

    // Initialize the render list lists.
    renderlist_lst_t::reset(&(self->all));
    renderlist_lst_t::reset(&(self->ref));
    renderlist_lst_t::reset(&(self->sha));
    renderlist_lst_t::reset(&(self->drf));
    renderlist_lst_t::reset(&(self->ndr));
    renderlist_lst_t::reset(&(self->wat));

    self->mesh = mesh;

    return self;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_t::reset(renderlist_t *self)
{
    if (nullptr == self)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return gfx_error;
    }

    ego_mesh_t *mesh = renderlist_t::getMesh(self);
    if (nullptr == mesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "renderlist not attached to a mesh");
        return gfx_error;
    }

    // Clear out the "in render list" flag for the old mesh.
	ego_tile_info_t *tlist = mesh->tmem.tile_list;

    for (size_t i = 0; i < self->all.count; ++i)
    {
        Uint32 fan = self->all.lst[i].index;
        if (fan < mesh->info.tiles_count)
        {
            tlist[fan].inrenderlist = false;
            tlist[fan].inrenderlist_frame = 0;
        }
    }

    // Re-initialize the renderlist.
    renderlist_t::init(self, self->mesh);

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_t::insert(renderlist_t * plst, const Uint32 index, const std::shared_ptr<Camera> &camera)
{
    // aliases
    ego_mesh_t      * pmesh = NULL;
    ego_grid_info_t * pgrid = NULL;
    float distance;

    // Make sure it doesn't die ugly
    if ( NULL == plst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    if ( NULL == plst->mesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "renderlist not connected to a mesh" );
        return gfx_error;
    }
    pmesh = plst->mesh;

    // check for a valid tile
    if ( NULL == pmesh->gmem.grid_list || 0 == pmesh->gmem.grid_count || index >= pmesh->gmem.grid_count )
    {
        return gfx_fail;
    }
    pgrid = pmesh->gmem.grid_list + index;

    // we can only accept so many tiles
    if ( plst->all.count >= renderlist_lst_t::CAPACITY) return gfx_fail;

    {
        int ix, iy;
        float dx, dy;

        ix = index % pmesh->info.tiles_x;
        iy = index / pmesh->info.tiles_x;
        dx = ( ix + TILE_FSIZE * 0.5f ) - camera->getCenter().x;
        dy = ( iy + TILE_FSIZE * 0.5f ) - camera->getCenter().y;
        distance = dx * dx + dy * dy;
    }

    // Put each tile in basic list
    renderlist_lst_t::push( &( plst->all ), index, distance );

    // Put each tile in one other list, for shadows and relections
    if ( 0 != ego_grid_info_test_all_fx( pgrid, MAPFX_SHA ) )
    {
        renderlist_lst_t::push( &( plst->sha ), index, distance );
    }
    else
    {
        renderlist_lst_t::push( &( plst->ref ), index, distance );
    }

    if ( 0 != ego_grid_info_test_all_fx( pgrid, MAPFX_DRAWREF ) )
    {
        renderlist_lst_t::push( &( plst->drf ), index, distance );
    }
    else
    {
        renderlist_lst_t::push( &( plst->ndr ), index, distance );
    }

    if ( 0 != ego_grid_info_test_all_fx( pgrid, MAPFX_WATER ) )
    {
        renderlist_lst_t::push( &( plst->wat ), index, distance );
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
ego_mesh_t *renderlist_t::getMesh(const renderlist_t *self)
{
    if (nullptr == self)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return nullptr;
    }
    return self->mesh;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_t::setMesh(renderlist_t *self, ego_mesh_t *mesh)
{
    if (nullptr == self)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return gfx_error;
    }
	self->mesh = mesh;
    return gfx_success;
}


//--------------------------------------------------------------------------------------------
gfx_rv renderlist_t::add(renderlist_t *self, const Ego::DynamicArray<BSP_leaf_t *> *leaves, const std::shared_ptr<Camera> &camera)
{
    size_t colst_cp, colst_sz;
    ego_mesh_t *pmesh  = NULL;
    gfx_rv retval = gfx_error;

    if (NULL == self)
    {
        return gfx_error;
    }

    if (NULL == leaves)
    {
        return gfx_error;
    }

    colst_cp = leaves->capacity();
    if ( 0 == colst_cp )
    {
        return gfx_error;
    }

    colst_sz  = leaves->size();
    if ( 0 == colst_sz )
    {
        return gfx_fail;
    }

    pmesh = renderlist_t::getMesh(self);
    if (NULL == pmesh)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "render list is not attached to a mesh");
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    // transfer valid pcolst entries to the renderlist
    for (size_t j = 0; j < colst_sz; j++)
    {
		BSP_leaf_t *leaf = leaves->ary[j];

        if (!BSP_leaf_valid(leaf)) continue;

        if (BSP_LEAF_TILE == leaf->data_type)
        {
            Uint32 ifan;
            ego_tile_info_t * ptile;
            ego_grid_info_t * pgrid;

            // Get fan index.
            ifan = (Uint32)(leaf->index);

			// Get tile for fan index.
            ptile = ego_mesh_get_ptile( pmesh, ifan );
            if ( NULL == ptile ) continue;

			// Get grid for fan index.
            pgrid = ego_mesh_get_pgrid( pmesh, ifan );
            if ( NULL == pgrid ) continue;

            if (gfx_error == gfx_capture_mesh_tile(ptile))
            {
                retval = gfx_error;
                break;
            }

            if (gfx_error == renderlist_t::insert(self, ifan, camera))
            {
                retval = gfx_error;
                break;
            }
        }
        else
        {
            // how did we get here?
            log_warning( "%s-%s-%d- found non-tile in the mpd BSP\n", __FILE__, __FUNCTION__, __LINE__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// renderlist array implementation
//--------------------------------------------------------------------------------------------
renderlist_ary_t *renderlist_ary_t::begin(renderlist_ary_t * ptr)
{
    if (nullptr == ptr)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return nullptr;
    }

    if ( ptr->started ) return ptr;

    for ( size_t cnt = 0; cnt < MAX_RENDER_LISTS; cnt++ )
    {
        ptr->free_lst[cnt] = cnt;
        renderlist_t::init( ptr->lst + cnt, NULL );
    }
    ptr->free_count = MAX_RENDER_LISTS;

    ptr->started = true;

    return ptr;
}

//--------------------------------------------------------------------------------------------
renderlist_ary_t *renderlist_ary_t::end( renderlist_ary_t * ptr )
{
    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to renderlist array" );
        return NULL;
    }

    if ( !ptr->started ) return ptr;

    ptr->free_count = 0;
    for ( size_t cnt = 0; cnt < MAX_RENDER_LISTS; cnt++ )
    {
        ptr->free_lst[cnt] = -1;
        renderlist_t::init( ptr->lst + cnt, NULL );
    }

    ptr->started = false;

    return ptr;
}

//--------------------------------------------------------------------------------------------
// renderlist manager implementation
//--------------------------------------------------------------------------------------------
gfx_rv renderlist_mgr_t::begin( renderlist_mgr_t * pmgr )
{
    // valid pointer?
    if ( NULL == pmgr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to renderlist manager" );
        return gfx_error;
    }

    if ( pmgr->started ) return gfx_success;

    renderlist_ary_t::begin( &( pmgr->ary ) );

    pmgr->started = true;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_mgr_t::end( renderlist_mgr_t * pmgr )
{
    // valid pointer?
    if ( NULL == pmgr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to renderlist manager" );
        return gfx_error;
    }

    if ( !pmgr->started ) return gfx_success;

    renderlist_ary_t::end( &( pmgr->ary ) );

    // the manager is no longer running
    pmgr->started = false;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
int renderlist_mgr_t::get_free_idx( renderlist_mgr_t * pmgr )
{
    int retval = -1;

    // renderlist manager started?
    if ( gfx_success != renderlist_mgr_t::begin( pmgr ) )
    {
        return -1;
    }

    if ( pmgr->ary.free_count <= 0 ) return -1;

    // resduce the count
    pmgr->ary.free_count--;

    // grab the index
    retval = pmgr->ary.free_count;

    // blank the free list
    if ( retval >= 0 && retval < MAX_RENDER_LISTS )
    {
        pmgr->ary.free_lst[ retval ] = -1;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv renderlist_mgr_t::free_one( renderlist_mgr_t * pmgr, size_t index )
{
    gfx_rv retval = gfx_error;

    // renderlist manager started?
    if ( gfx_success != renderlist_mgr_t::begin( pmgr ) )
    {
        return gfx_error;
    }

    // valid index range?
    if ( index >= MAX_RENDER_LISTS ) return gfx_error;

    // is the index already freed?
    for ( size_t cnt = 0; cnt < pmgr->ary.free_count; cnt++ )
    {
        if ( index == pmgr->ary.free_lst[cnt] )
        {
            return gfx_fail;
        }
    }

    // push it on the list
    retval = gfx_fail;
    if ( pmgr->ary.free_count < MAX_RENDER_LISTS )
    {
        pmgr->ary.free_lst[pmgr->ary.free_count] = index;
        pmgr->ary.free_count++;
        retval = gfx_success;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
renderlist_t * renderlist_mgr_t::get_ptr( renderlist_mgr_t * pmgr, size_t index )
{
    // renderlist manager started?
    if ( gfx_success != renderlist_mgr_t::begin( pmgr ) )
    {
        return NULL;
    }

    // valid index range?
    if ( index >= MAX_RENDER_LISTS )
    {
        return NULL;
    }

    return pmgr->ary.lst + index;
}

//--------------------------------------------------------------------------------------------
// dolist implementation
//--------------------------------------------------------------------------------------------
dolist_t::dolist_t() : index(0), size(0), lst()  {

}
dolist_t * dolist_t::init(dolist_t *self, const size_t index)
{
	if (nullptr == self)
	{
		return nullptr;
	}
#if 0
	BLANK_STRUCT_PTR(self);
#endif
	for (size_t i = 0; i < dolist_t::CAPACITY; ++i)
	{
		dolist_t::element_t::init(&(self->lst[i]));
	}
	self->size = 0;

    // Give the dolist its "name".
    self->index = index;

    return self;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_t::reset(dolist_t *self, const size_t index)
{
    size_t entries;

    if (nullptr == self)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return gfx_error;
    }

    if (self->index >= MAX_DO_LISTS)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist index");
        return gfx_error;
    }

	self->index = index;

	// If there is nothing in the dolist, we are done.
	if (0 == self->size)
    {
        return gfx_success;
    }

	size_t size = std::min(self->size, dolist_t::CAPACITY); /// @todo This is redundant. Just use self->size.
    for (size_t i = 0; i < size; i++)
    {
        dolist_t::element_t *element = self->lst + i;

        // Tell all valid objects that they are removed from this dolist.
        if (INVALID_CHR_REF == element->ichr && _VALID_PRT_RANGE(element->iprt))
        {
            PrtList.lst[element->iprt].inst.indolist = false;
        }
        else if (INVALID_PRT_REF == element->iprt && VALID_CHR_RANGE(element->ichr))
        {
            GameObject *character = _gameObjects.get(element->ichr);
            if (nullptr != character) character->inst.indolist = false;
        }
    }
    self->size = 0;
#if 0
    // reset the dolist
    dolist_t::init(self, index);
#endif
    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_t::test_chr(dolist_t *self, const GameObject *pchr)
{
    if (nullptr == self)
    {
        return gfx_error;
    }

    if (self->size >= dolist_t::CAPACITY) /// @todo Proper encapsulation will allow me to show that '>' is not possible.
    {
        return gfx_fail;
    }

    if (!INGAME_PCHR(pchr))
    {
        return gfx_fail;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_t::add_chr_raw(dolist_t *self, GameObject *pchr)
{
    /// @author ZZ
    /// @details This function puts a character in the list

    if (nullptr == self)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return gfx_error;
    }

    if (nullptr == pchr)
    {
        return gfx_fail;
    }

	/*
    // Don't do anything if it's already in this(!) list.
    if (pchr->inst.indolist[self->index])
    {
		return gfx_success;
    }
	*/

    // Don't add if it is hidden.
    if ( pchr->is_hidden )
    {
        return gfx_fail;
    }

    // Don't add if it's in another character's inventory.
    if (_gameObjects.exists(pchr->inwhich_inventory))
    {
        return gfx_fail;
    }

    // Add!
    self->lst[self->size].ichr = GET_INDEX_PCHR(pchr);
    self->lst[self->size].iprt = INVALID_PRT_REF;
    self->size++;

    // Notify it that it is in a do list.
    pchr->inst.indolist = true;

    // Add any weapons it is holding.
    dolist_t::add_chr_raw(self, _gameObjects.get(pchr->holdingwhich[SLOT_LEFT]));
    dolist_t::add_chr_raw(self, _gameObjects.get(pchr->holdingwhich[SLOT_RIGHT]));

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_t::test_prt(dolist_t *self, const prt_t *pprt)
{
    if (nullptr == self)
    {
        return gfx_error;
    }

    if (self->size >= dolist_t::CAPACITY) /// @todo Proper encapsulation will allow me to show that '>' is not possible.
    {
        return gfx_fail;
    }

	/// @todo I can't explain why it's reject until someone explains to me what _DISPLAY_PPRT determines.
    if (!_DISPLAY_PPRT(pprt))
    {
        return gfx_fail;
    }

	// Don't add if it's explicitly or implicitly hidden.
    if (pprt->is_hidden || 0 == pprt->size)
    {
        return gfx_fail;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_t::add_prt_raw(dolist_t *self, prt_t * pprt)
{
    /// @author ZZ
    /// @details This function puts a character in the list

	/*
	// Don't do anything if it's already in this(!) list.
    if (pprt->inst.indolist[pdlist->index])
    {
		return gfx_success;
    }
	*/

    self->lst[self->size].ichr = INVALID_CHR_REF;
    self->lst[self->size].iprt = _GET_REF_PPRT(pprt);
    self->size++;

    pprt->inst.indolist = true;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_t::add_colst( dolist_t * pdlist, const Ego::DynamicArray<BSP_leaf_t *> *pcolst )
{
    BSP_leaf_t * pleaf;
    gfx_rv retval;

    if ( NULL == pdlist )
    {
        return gfx_error;
    }

    if ( NULL == pcolst )
    {
        return gfx_error;
    }

	size_t colst_cp = pcolst->capacity();
    if ( 0 == colst_cp )
    {
        return gfx_error;
    }

	size_t colst_sz  = pcolst->size();
    if ( 0 == colst_sz )
    {
        return gfx_fail;
    }

    // assume the best
    retval = gfx_success;

    for ( size_t j = 0; j < colst_sz; j++ )
    {
        pleaf = pcolst->ary[j];

        if ( !BSP_leaf_valid( pleaf ) ) continue;

        if ( BSP_LEAF_CHR == pleaf->data_type )
        {
            CHR_REF ichr;
            GameObject * pchr;

            // collided with a character
            ichr = ( CHR_REF )( pleaf->index );

            // is it in the array?
            if ( !VALID_CHR_RANGE( ichr ) ) continue;
            pchr = _gameObjects.get( ichr );

            // do some more obvious tests before testing the frustum
            if ( dolist_t::test_chr( pdlist, pchr ) )
            {
                // Add the character
                if ( gfx_error == dolist_t::add_chr_raw( pdlist, pchr ) )
                {
                    retval = gfx_error;
                    break;
                }
            }
        }
        else if ( BSP_LEAF_PRT == pleaf->data_type )
        {
            PRT_REF iprt;
            prt_t * pprt;

            // collided with a particle
            iprt = ( PRT_REF )( pleaf->index );

            // is it in the array?
            if ( !_VALID_PRT_RANGE( iprt ) ) continue;
            pprt = PrtList_get_ptr( iprt );

            // do some more obvious tests before testing the frustum
            if ( dolist_t::test_prt( pdlist, pprt ) )
            {
                // Add the particle
                if ( gfx_error == dolist_t::add_prt_raw( pdlist, pprt ) )
                {
                    retval = gfx_error;
                    break;
                }
            }
        }
        else
        {
            // how did we get here?
            log_warning( "%s-%s-%d- found unknown type in a dolist BSP\n", __FILE__, __FUNCTION__, __LINE__ );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_t::sort( dolist_t * pdlist, std::shared_ptr<Camera> pcam, const bool do_reflect )
{
    /// @author ZZ
    /// @details This function orders the dolist based on distance from camera,
    ///    which is needed for reflections to properly clip themselves.
    ///    Order from closest to farthest

    Uint32    cnt;
    fvec3_t   vcam;
    size_t    count;

    if ( NULL == pdlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

	if (pdlist->size >= dolist_t::CAPACITY)
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    mat_getCamForward(pcam->getView(), vcam);

    // Figure the distance of each
    count = 0;
    for ( cnt = 0; cnt < pdlist->size; cnt++ )
    {
        fvec3_t   vtmp;
        float dist;

        if ( INVALID_PRT_REF == pdlist->lst[cnt].iprt && VALID_CHR_RANGE( pdlist->lst[cnt].ichr ) )
        {
            CHR_REF ichr;
            fvec3_t pos_tmp;

            ichr = pdlist->lst[cnt].ichr;

            if ( do_reflect )
            {
                mat_getTranslate(_gameObjects.get(ichr)->inst.ref.matrix, pos_tmp);
            }
            else
            {
                mat_getTranslate(_gameObjects.get(ichr)->inst.matrix, pos_tmp);
            }

            vtmp = pos_tmp - pcam->getPosition();
        }
        else if ( INVALID_CHR_REF == pdlist->lst[cnt].ichr && _VALID_PRT_RANGE( pdlist->lst[cnt].iprt ) )
        {
            PRT_REF iprt = pdlist->lst[cnt].iprt;

            if ( do_reflect )
            {
                vtmp = PrtList.lst[iprt].inst.pos - pcam->getPosition();
            }
            else
            {
                vtmp = PrtList.lst[iprt].inst.ref_pos - pcam->getPosition();
            }
        }
        else
        {
            continue;
        }

        dist = vtmp.dot(vcam);
        if ( dist > 0 )
        {
            pdlist->lst[count].ichr = pdlist->lst[cnt].ichr;
            pdlist->lst[count].iprt = pdlist->lst[cnt].iprt;
            pdlist->lst[count].dist = dist;
            count++;
        }
    }
    pdlist->size = count;

    // use qsort to sort the list in-place
    if ( pdlist->size > 1 )
    {
		qsort(pdlist->lst, pdlist->size, sizeof(dolist_t::element_t), dolist_t::element_t::cmp);
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// dolist array implementation
//--------------------------------------------------------------------------------------------
dolist_ary_t * dolist_ary_t::begin( dolist_ary_t * ptr )
{
    int cnt;

    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to dolist array" );
        return NULL;
    }

    if ( ptr->started ) return ptr;

    for ( cnt = 0; cnt < MAX_DO_LISTS; cnt++ )
    {
        ptr->free_lst[cnt] = cnt;
        dolist_t::init( ptr->lst + cnt, cnt );
    }
    ptr->free_count = MAX_DO_LISTS;

    ptr->started = true;

    return ptr;
}

//--------------------------------------------------------------------------------------------
dolist_ary_t *dolist_ary_t::end( dolist_ary_t * ptr )
{
    int cnt;

    if ( NULL == ptr )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL pointer to dolist array" );
        return NULL;
    }

    if ( !ptr->started ) return ptr;

    ptr->free_count = 0;
    for ( cnt = 0; cnt < MAX_DO_LISTS; cnt++ )
    {
        ptr->free_lst[cnt] = -1;
        dolist_t::init( ptr->lst + cnt, cnt );
    }

    ptr->started = false;

    return ptr;
}

//--------------------------------------------------------------------------------------------
// dolist manager implementation
//--------------------------------------------------------------------------------------------
gfx_rv dolist_mgr_t::begin(dolist_mgr_t *self)
{
    if (nullptr == self)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return gfx_error;
    }

	// If it's already started, do nothing.
	if (self->started)
	{
		return gfx_success;
	}
    dolist_ary_t::begin(&(self->ary));

	// The manager is now running.
    self->started = true;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_mgr_t::end(dolist_mgr_t *self)
{
    if (nullptr == self)
    {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, 0, "nullptr == self");
        return gfx_error;
    }

	// If it's not started, do nothing.
    if (!self->started) return gfx_success;

    dolist_ary_t::end(&(self->ary));

    // The manager is no longer running.
    self->started = false;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
int dolist_mgr_t::get_free_idx(dolist_mgr_t *self)
{
#if 0
    int retval = -1;
#endif
    // Ensure dolist manager is started.
    if (gfx_success != dolist_mgr_t::begin(self))
    {
		// If starting the dolist manager fails, return -1.
        return -1;
    }

	// If no free do lists are available, return -1.
    if (self->ary.free_count <= 0 ) return -1;

    // Reduce the number of available dolists by 1.
	self->ary.free_count--;

    // grab the index
    int index = self->ary.free_count;

    // blank the free list
    if (index >= 0 && index < MAX_DO_LISTS)
    {
        self->ary.free_lst[index] = -1;
    }

    return index;
}

//--------------------------------------------------------------------------------------------
gfx_rv dolist_mgr_t::free_one(dolist_mgr_t *self, size_t index )
{
    gfx_rv retval = gfx_error;

    // dolist manager started?
    if (gfx_success != dolist_mgr_t::begin(self))
    {
        return gfx_error;
    }

    // valid index range?
    if ( index >= MAX_DO_LISTS ) return gfx_error;

    // is the index already freed?
    for ( size_t cnt = 0; cnt < self->ary.free_count; cnt++ )
    {
        if ( index == self->ary.free_lst[cnt] )
        {
            return gfx_fail;
        }
    }

    // push it on the list
    retval = gfx_fail;
    if ( self->ary.free_count < MAX_DO_LISTS )
    {
        self->ary.free_lst[self->ary.free_count] = index;
        self->ary.free_count++;
        retval = gfx_success;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
dolist_t *dolist_mgr_t::get_ptr(dolist_mgr_t *self, size_t index)
{
    // dolist manager started?
    if (gfx_success != dolist_mgr_t::begin(self))
    {
        return NULL;
    }

    // valid index range?
    if (index < 0 || index >= MAX_DO_LISTS)
    {
        return NULL;
    }

    return self->ary.lst + index;
}

//--------------------------------------------------------------------------------------------
// gfx_system INITIALIZATION
//--------------------------------------------------------------------------------------------
void gfx_system_begin()
{
    // set the graphics state
	gfx_system_init_SDL_graphics(); /* @todo Error handling. */
	gfx_system_init_OpenGL();       /* @todo Error handling. */

    // initialize the renderlist manager
    renderlist_mgr_t::begin( &_renderlist_mgr_data );

    // initialize the dolist manager
    dolist_mgr_t::begin( &_dolist_mgr_data );

    // initialize the dynalist frame
    // otherwise, it will not update until the frame count reaches whatever
    // left over or random value is in this counter
    _dynalist.frame = -1;
    _dynalist.size = 0;

    // begin the billboard system
    billboard_system_begin();

    // initialize the gfx data dtructures
    TxList_init_all();

    // initialize the decimated texture arrays
    gfx_system_begin_decimated_textures();

    // initialize the profiling variables
    PROFILE_INIT( render_scene_init );
    PROFILE_INIT( render_scene_mesh );
    PROFILE_INIT( render_scene_solid );
    PROFILE_INIT( render_scene_water );
    PROFILE_INIT( render_scene_trans );

    PROFILE_INIT( gfx_make_renderlist );
    PROFILE_INIT( gfx_make_dolist );
    PROFILE_INIT( do_grid_lighting );
    PROFILE_INIT( light_fans );
    PROFILE_INIT( gfx_update_all_chr_instance );
    PROFILE_INIT( update_all_prt_instance );

    PROFILE_INIT( render_scene_mesh_dolist_sort );
    PROFILE_INIT( render_scene_mesh_ndr );
    PROFILE_INIT( render_scene_mesh_drf_back );
    PROFILE_INIT( render_scene_mesh_ref );
    PROFILE_INIT( render_scene_mesh_ref_chr );
    PROFILE_INIT( render_scene_mesh_drf_solid );
    PROFILE_INIT( render_scene_mesh_render_shadows );

    // init some other variables
    stabilized_game_fps        = TARGET_FPS;
    stabilized_game_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_game_fps_weight = STABILIZED_COVER;

    gfx_reset_timers();

    // allocate the specailized "collision lists"
    if ( NULL == _dolist_colst.ctor(dolist_t::CAPACITY))
    {
        log_error( "%s-%s-%d - Could not allocate dolist collision list", __FILE__, __FUNCTION__, __LINE__ );
    }

    if ( NULL == _renderlist_colst.ctor(renderlist_lst_t::CAPACITY))
    {
        log_error( "%s-%s-%d - Could not allocate renderlist collision list", __FILE__, __FUNCTION__, __LINE__ );
    }

    egolib_timer__init( &gfx_update_timer );
}

//--------------------------------------------------------------------------------------------
void gfx_system_end()
{
    // initialize the profiling variables
    PROFILE_FREE( render_scene_init );
    PROFILE_FREE( render_scene_mesh );
    PROFILE_FREE( render_scene_solid );
    PROFILE_FREE( render_scene_water );
    PROFILE_FREE( render_scene_trans );

    PROFILE_FREE( gfx_make_renderlist );
    PROFILE_FREE( gfx_make_dolist );
    PROFILE_FREE( do_grid_lighting );
    PROFILE_FREE( light_fans );
    PROFILE_FREE( gfx_update_all_chr_instance );
    PROFILE_FREE( update_all_prt_instance );

    PROFILE_FREE( render_scene_mesh_dolist_sort );
    PROFILE_FREE( render_scene_mesh_ndr );
    PROFILE_FREE( render_scene_mesh_drf_back );
    PROFILE_FREE( render_scene_mesh_ref );
    PROFILE_FREE( render_scene_mesh_ref_chr );
    PROFILE_FREE( render_scene_mesh_drf_solid );
    PROFILE_FREE( render_scene_mesh_render_shadows );

    // end the billboard system
    billboard_system_end();

    // clear the decimated texture arrays
    gfx_system_end_decimated_textures();

    // release all textures
    TxList_release_all();

    // de-initialize the renderlist manager
    renderlist_mgr_t::end( &_renderlist_mgr_data );

    // de-initialize the dolist manager
    dolist_mgr_t::end( &_dolist_mgr_data );

    gfx_reset_timers();

    // deallocate the specailized "collistion lists"
	_dolist_colst.dtor();
	_renderlist_colst.dtor();
	
	gfx_system_uninit_OpenGL();
	gfx_system_uninit_SDL_graphics();

}

//--------------------------------------------------------------------------------------------
void gfx_system_uninit_OpenGL()
{
	Ego::Renderer::shutDown();
}

//--------------------------------------------------------------------------------------------
int gfx_system_init_OpenGL()
{
	Ego::Renderer::startUp(); /* @todo Add error handling. */

    // GL_DEBUG(glClear)) stuff
    GL_DEBUG( glClearColor )( 0.0f, 0.0f, 0.0f, 0.0f ); // Set the background black
    GL_DEBUG( glClearDepth )( 1.0f );

    // depth buffer stuff
    GL_DEBUG( glClearDepth )( 1.0f );
    GL_DEBUG( glDepthMask )( GL_TRUE );

    // do not draw hidden surfaces
    GL_DEBUG( glEnable )( GL_DEPTH_TEST );
    GL_DEBUG( glDepthFunc )( GL_LESS );

    // alpha stuff
    GL_DEBUG( glDisable )( GL_BLEND );

    // do not display the completely transparent portion
    GL_DEBUG( glEnable )( GL_ALPHA_TEST );
    GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );

    /// @todo Including backface culling here prevents the mesh from getting rendered
    /// backface culling

    // oglx_begin_culling( GL_BACK, GL_CW );            // GL_ENABLE_BIT | GL_POLYGON_BIT

    // disable OpenGL lighting
    GL_DEBUG( glDisable )( GL_LIGHTING );

    // fill mode
    GL_DEBUG( glPolygonMode )( GL_FRONT, GL_FILL );
    GL_DEBUG( glPolygonMode )( GL_BACK,  GL_FILL );

    // ?Need this for color + lighting?
    GL_DEBUG( glEnable )( GL_COLOR_MATERIAL );  // Need this for color + lighting

    // set up environment mapping
    GL_DEBUG( glTexGeni )( GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For S To Sphere Mapping (NEW)
    GL_DEBUG( glTexGeni )( GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP );  // Set The Texture Generation Mode For T To Sphere Mapping (NEW)

    //Initialize the motion blur buffer
    GL_DEBUG( glClearAccum )( 0.0f, 0.0f, 0.0f, 1.0f );
    GL_DEBUG( glClear )( GL_ACCUM_BUFFER_BIT );

    // Load the current graphical settings
    // gfx_system_load_assets();

    _ogl_initialized = true;

    return _ogl_initialized && _sdl_initialized_graphics;
}

//--------------------------------------------------------------------------------------------
void gfx_system_uninit_SDL_graphics()
{
	if (!_sdl_initialized_graphics)
	{
		return;
	}
}

void gfx_system_init_SDL_graphics()
{
	if (_sdl_initialized_graphics)
	{
		return;
	}

    ego_init_SDL_base();

    log_info( "Intializing SDL Video... " );
    if ( SDL_InitSubSystem( SDL_INIT_VIDEO ) < 0 )
    {
        log_message( "Failed!\n" );
        log_warning( "SDL error == \"%s\"\n", SDL_GetError() );
    }
    else
    {
        log_message( "Success!\n" );
    }

#if !defined(__APPLE__)
    {
        //Setup the cute windows manager icon, don't do this on Mac
        SDL_Surface *theSurface;
        const char * fname = "icon.bmp";
        STRING fileload;

        snprintf( fileload, SDL_arraysize( fileload ), "mp_data/%s", fname );

        theSurface = IMG_Load_RW(vfs_openRWopsRead(fileload), 1);
        if ( NULL == theSurface )
        {
            log_warning( "Unable to load icon (%s)\n", fname );
        }
        else
        {
            SDL_WM_SetIcon( theSurface, NULL );
        }
    }
#endif

    // Set the window name
    SDL_WM_SetCaption( "Egoboo " VERSION, "Egoboo" );

#if defined(__unix__)

    // GLX doesn't differentiate between 24 and 32 bpp, asking for 32 bpp
    // will cause SDL_SetVideoMode to fail with:
    // "Unable to set video mode: Couldn't find matching GLX visual"
    if ( 32 == cfg.scrd_req ) cfg.scrd_req = 24;
    if ( 32 == cfg.scrz_req ) cfg.scrz_req = 24;

#endif

    // the flags to pass to SDL_SetVideoMode
    sdl_vparam.width                     = cfg.scrx_req;
    sdl_vparam.height                    = cfg.scry_req;
    sdl_vparam.depth                     = cfg.scrd_req;

    sdl_vparam.flags.opengl              = SDL_TRUE;
    sdl_vparam.flags.double_buf          = SDL_TRUE;
    sdl_vparam.flags.full_screen         = cfg.fullscreen_req;

    sdl_vparam.gl_att.buffer_size        = cfg.scrd_req;
    sdl_vparam.gl_att.depth_size         = cfg.scrz_req;
    sdl_vparam.gl_att.multi_buffers      = ( cfg.multisamples > 1 ) ? 1 : 0;
    sdl_vparam.gl_att.multi_samples      = cfg.multisamples;
    sdl_vparam.gl_att.accelerated_visual = GL_TRUE;
    
    sdl_vparam.gl_att.accum[0]           = 8;
    sdl_vparam.gl_att.accum[1]           = 8;
    sdl_vparam.gl_att.accum[2]           = 8;
    sdl_vparam.gl_att.accum[3]           = 8;

    ogl_vparam.dither         = cfg.use_dither ? GL_TRUE : GL_FALSE;
    ogl_vparam.antialiasing   = GL_TRUE;
    ogl_vparam.perspective    = cfg.use_perspective ? GL_NICEST : GL_FASTEST;
    ogl_vparam.shading        = GL_SMOOTH;
    ogl_vparam.userAnisotropy = 16.0f * std::max( 0, cfg.texturefilter_req - TX_TRILINEAR_2 );

    log_info( "Opening SDL Video Mode...\n" );

    // actually set the video mode
    if ( NULL == SDL_GL_set_mode( NULL, &sdl_vparam, &ogl_vparam, _sdl_initialized_graphics ) )
    {
        log_message( "Failed!\n" );
        log_error( "I can't get SDL to set any video mode: %s\n", SDL_GetError() );
    }
    else
    {
        GFX_WIDTH = ( float )GFX_HEIGHT / ( float )sdl_vparam.height * ( float )sdl_vparam.width;
        log_message( "Success!\n" );
    }

    _sdl_initialized_graphics = SDL_TRUE;
}

//--------------------------------------------------------------------------------------------
void gfx_system_render_world( std::shared_ptr<Camera> pcam, const int render_list_index, const int dolist_index )
{
    gfx_error_state_t * err_tmp;

    gfx_error_clear();

    gfx_begin_3d( pcam );
    {
        if ( gfx.draw_background )
        {
            // Render the background
            // TX_WATER_LOW for waterlow.bmp
            render_world_background( pcam, ( TX_REF )TX_WATER_LOW );
        }

        render_scene( pcam, render_list_index, dolist_index );

        if ( gfx.draw_overlay )
        {
            // Foreground overlay
            // TX_WATER_TOP is watertop.bmp
            render_world_overlay( pcam, ( TX_REF )TX_WATER_TOP );
        }

        if ( pcam->getMotionBlur() > 0 )
        {
            if (pcam->getMotionBlurOld() < 0.001f)
                GL_DEBUG(glAccum)(GL_LOAD, 1);
            //Do motion blur
            if (!GProc->mod_paused)
            {
                GL_DEBUG( glAccum )( GL_MULT, pcam->getMotionBlur() );
                GL_DEBUG( glAccum )( GL_ACCUM, 1.0f - pcam->getMotionBlur() );
            }
            GL_DEBUG( glAccum )( GL_RETURN, 1.0f );
        }
    }
    gfx_end_3d();

    // Render the billboards
    billboard_system_render_all( pcam );

    err_tmp = gfx_error_pop();
    if ( NULL != err_tmp )
    {
        printf( "**** Encountered graphics errors in frame %d ****\n\n", game_frame_all );
        while ( NULL != err_tmp )
        {
            printf( "vvvv\n" );
            printf(
                "\tfile     == %s\n"
                "\tline     == %d\n"
                "\tfunction == %s\n"
                "\tcode     == %d\n"
                "\tstring   == %s\n",
                err_tmp->file, err_tmp->line, err_tmp->function, err_tmp->type, err_tmp->string );
            printf( "^^^^\n\n" );

            err_tmp = gfx_error_pop();
        }
        printf( "****\n\n" );
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_main()
{
    /// @author ZZ
    /// @details This function does all the drawing stuff

    _cameraSystem.renderAll(gfx_system_render_world);

    draw_hud();

    gfx_request_flip_pages();
}

//--------------------------------------------------------------------------------------------
bool gfx_system_set_virtual_screen( gfx_config_t * pgfx )
{
    float kx, ky;

    if ( NULL == pgfx ) return false;

    kx = ( float )GFX_WIDTH  / ( float )sdl_scr.x;
    ky = ( float )GFX_HEIGHT / ( float )sdl_scr.y;

    if ( kx == ky )
    {
        pgfx->vw = sdl_scr.x;
        pgfx->vh = sdl_scr.y;
    }
    else if ( kx > ky )
    {
        pgfx->vw = sdl_scr.x * kx / ky;
        pgfx->vh = sdl_scr.y;
    }
    else
    {
        pgfx->vw = sdl_scr.x;
        pgfx->vh = sdl_scr.y * ky / kx;
    }

    pgfx->vdw = ( GFX_WIDTH  - pgfx->vw ) * 0.5f;
    pgfx->vdh = ( GFX_HEIGHT - pgfx->vh ) * 0.5f;

    ui_set_virtual_screen( pgfx->vw, pgfx->vh, GFX_WIDTH, GFX_HEIGHT );

    //gfx_calc_rotmesh();

    return true;
}

//--------------------------------------------------------------------------------------------
renderlist_mgr_t * gfx_system_get_renderlist_mgr()
{
    if ( gfx_success != renderlist_mgr_t::begin( &_renderlist_mgr_data ) )
    {
        return NULL;
    }

    return &_renderlist_mgr_data;
}

//--------------------------------------------------------------------------------------------
dolist_mgr_t * gfx_system_get_dolist_mgr()
{
    if ( gfx_success != dolist_mgr_t::begin( &_dolist_mgr_data ) )
    {
        return NULL;
    }

    return &_dolist_mgr_data;
}

//--------------------------------------------------------------------------------------------
void gfx_system_load_assets()
{
    /// @author ZF
    /// @details This function loads all the graphics based on the game settings

    GLenum quality;

    // Check if the computer graphic driver supports anisotropic filtering

    if ( !ogl_caps.anisotropic_supported )
    {
        if ( tex_params.texturefilter >= TX_ANISOTROPIC )
        {
            tex_params.texturefilter = TX_TRILINEAR_2;
            log_warning( "Your graphics driver does not support anisotropic filtering.\n" );
        }
    }

    // Enable prespective correction?
    if ( gfx.perspective ) quality = GL_NICEST;
    else quality = GL_FASTEST;
    GL_DEBUG( glHint )( GL_PERSPECTIVE_CORRECTION_HINT, quality );

    // Enable dithering?
    if ( gfx.dither )
    {
        GL_DEBUG( glHint )( GL_GENERATE_MIPMAP_HINT, GL_NICEST );
        GL_DEBUG( glEnable )( GL_DITHER );
    }
    else
    {
        GL_DEBUG( glHint )( GL_GENERATE_MIPMAP_HINT, GL_FASTEST );
        GL_DEBUG( glDisable )( GL_DITHER );                          // ENABLE_BIT
    }

    // Enable Gouraud shading? (Important!)
    GL_DEBUG( glShadeModel )( gfx.shading );         // GL_LIGHTING_BIT

    // Enable antialiasing?
    if ( gfx.antialiasing )
    {
        GL_DEBUG( glEnable )( GL_MULTISAMPLE_ARB );

        GL_DEBUG( glEnable )( GL_LINE_SMOOTH );
        GL_DEBUG( glHint )( GL_LINE_SMOOTH_HINT,    GL_NICEST );

        GL_DEBUG( glEnable )( GL_POINT_SMOOTH );
        GL_DEBUG( glHint )( GL_POINT_SMOOTH_HINT,   GL_NICEST );

        GL_DEBUG( glDisable )( GL_POLYGON_SMOOTH );
        GL_DEBUG( glHint )( GL_POLYGON_SMOOTH_HINT,    GL_FASTEST );

        // PLEASE do not turn this on unless you use
        // GL_DEBUG(glEnable)(GL_BLEND);
        // GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // before every single draw command
        //
        // GL_DEBUG(glEnable)(GL_POLYGON_SMOOTH);
        // GL_DEBUG(glHint)(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    }
    else
    {
        GL_DEBUG( glDisable )( GL_MULTISAMPLE_ARB );
        GL_DEBUG( glDisable )( GL_POINT_SMOOTH );
        GL_DEBUG( glDisable )( GL_LINE_SMOOTH );
        GL_DEBUG( glDisable )( GL_POLYGON_SMOOTH );
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_init_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    gfx_init_map_data();
    font_bmp_init();

    billboard_system_init();
    TxList_init_all();

    PROFILE_RESET( render_scene_init );
    PROFILE_RESET( render_scene_mesh );
    PROFILE_RESET( render_scene_solid );
    PROFILE_RESET( render_scene_water );
    PROFILE_RESET( render_scene_trans );

    PROFILE_RESET( gfx_make_renderlist );
    PROFILE_RESET( gfx_make_dolist );
    PROFILE_RESET( do_grid_lighting );
    PROFILE_RESET( light_fans );
    PROFILE_RESET( gfx_update_all_chr_instance );
    PROFILE_RESET( update_all_prt_instance );

    PROFILE_RESET( render_scene_mesh_dolist_sort );
    PROFILE_RESET( render_scene_mesh_ndr );
    PROFILE_RESET( render_scene_mesh_drf_back );
    PROFILE_RESET( render_scene_mesh_ref );
    PROFILE_RESET( render_scene_mesh_ref_chr );
    PROFILE_RESET( render_scene_mesh_drf_solid );
    PROFILE_RESET( render_scene_mesh_render_shadows );

    stabilized_game_fps        = TARGET_FPS;
    stabilized_game_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_game_fps_weight = STABILIZED_COVER;
}

//--------------------------------------------------------------------------------------------
void gfx_system_release_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    gfx_init_map_data();

    BillboardList_free_all();
    TxList_release_all();
}

//--------------------------------------------------------------------------------------------
void gfx_system_delete_all_graphics()
{
    gfx_init_bar_data();
    gfx_init_blip_data();
    gfx_init_map_data();

    BillboardList_free_all();
    TxList_delete_all();
}

//--------------------------------------------------------------------------------------------
void gfx_system_load_basic_textures()
{
    /// @author ZZ
    /// @details This function loads the standard textures for a module

    // Particle sprites
    TxList_load_one_vfs( "mp_data/particle_trans", ( TX_REF )TX_PARTICLE_TRANS, TRANSCOLOR );
    prt_set_texture_params(( TX_REF )TX_PARTICLE_TRANS );

    TxList_load_one_vfs( "mp_data/particle_light", ( TX_REF )TX_PARTICLE_LIGHT, INVALID_KEY );
    prt_set_texture_params(( TX_REF )TX_PARTICLE_LIGHT );

    // Module background tiles
    TxList_load_one_vfs( "mp_data/tile0", ( TX_REF )TX_TILE_0, TRANSCOLOR );
    TxList_load_one_vfs( "mp_data/tile1", ( TX_REF )TX_TILE_1, TRANSCOLOR );
    TxList_load_one_vfs( "mp_data/tile2", ( TX_REF )TX_TILE_2, TRANSCOLOR );
    TxList_load_one_vfs( "mp_data/tile3", ( TX_REF )TX_TILE_3, TRANSCOLOR );

    // Water textures
    TxList_load_one_vfs( "mp_data/watertop", ( TX_REF )TX_WATER_TOP, TRANSCOLOR );
    TxList_load_one_vfs( "mp_data/waterlow", ( TX_REF )TX_WATER_LOW, TRANSCOLOR );

    // The phong map
    TxList_load_one_vfs( "mp_data/phong", ( TX_REF )TX_PHONG, TRANSCOLOR );

    gfx_decimate_all_mesh_textures();

    PROFILE_RESET( render_scene_init );
    PROFILE_RESET( render_scene_mesh );
    PROFILE_RESET( render_scene_solid );
    PROFILE_RESET( render_scene_water );
    PROFILE_RESET( render_scene_trans );

    PROFILE_RESET( gfx_make_renderlist );
    PROFILE_RESET( gfx_make_dolist );
    PROFILE_RESET( do_grid_lighting );
    PROFILE_RESET( light_fans );
    PROFILE_RESET( gfx_update_all_chr_instance );
    PROFILE_RESET( update_all_prt_instance );

    PROFILE_RESET( render_scene_mesh_dolist_sort );
    PROFILE_RESET( render_scene_mesh_ndr );
    PROFILE_RESET( render_scene_mesh_drf_back );
    PROFILE_RESET( render_scene_mesh_ref );
    PROFILE_RESET( render_scene_mesh_ref_chr );
    PROFILE_RESET( render_scene_mesh_drf_solid );
    PROFILE_RESET( render_scene_mesh_render_shadows );

    stabilized_game_fps        = TARGET_FPS;
    stabilized_game_fps_sum    = STABILIZED_COVER * TARGET_FPS;
    stabilized_game_fps_weight = STABILIZED_COVER;
}

//--------------------------------------------------------------------------------------------
void gfx_system_make_enviro()
{
    /// @author ZZ
    /// @details This function sets up the environment mapping table
    int cnt;
    float x, y, z;

    // Find the environment map positions
    for ( cnt = 0; cnt < EGO_NORMAL_COUNT; cnt++ )
    {
        x = MD2Model::getMD2Normal(cnt, 0);
        y = MD2Model::getMD2Normal(cnt, 1);
        indextoenvirox[cnt] = ATAN2( y, x ) * INV_TWO_PI;
    }

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        z = cnt / INV_FF;  // Z is between 0 and 1
        lighttoenviroy[cnt] = z;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_reload_all_textures()
{
    /// @author BB
    /// @details function is called when the graphics mode is changed or the program is
    /// restored from a minimized state. Otherwise, all OpenGL bitmaps return to a random state.

    TxList_reload_all();
    mnu_TxList_reload_all();
    gfx_reload_decimated_textures();
}

//--------------------------------------------------------------------------------------------
// 2D RENDERER FUNCTIONS
//--------------------------------------------------------------------------------------------
void draw_blip( float sizeFactor, Uint8 color, float x, float y, bool mini_map )
{
    /// @author ZZ
    /// @details This function draws a single blip
    ego_frect_t tx_rect, sc_rect;

    float width, height;
    float loc_x, loc_y;

    //Adjust the position values so that they fit inside the minimap
    if ( mini_map )
    {
        loc_x = x * MAPSIZE / PMesh->gmem.edge_x;
        loc_y = ( y * MAPSIZE / PMesh->gmem.edge_y ) + sdl_scr.y - MAPSIZE;
    }
    else
    {
        loc_x = x;
        loc_y = y;
    }

    //Now draw it
    if ( loc_x > 0.0f && loc_y > 0.0f )
    {
        oglx_texture_t * ptex = TxList_get_valid_ptr(( TX_REF )TX_BLIP );

        tx_rect.xmin = ( float )bliprect[color]._left   / ( float )oglx_texture_getTextureWidth( ptex );
        tx_rect.xmax = ( float )bliprect[color]._right  / ( float )oglx_texture_getTextureWidth( ptex );
        tx_rect.ymin = ( float )bliprect[color]._top    / ( float )oglx_texture_getTextureHeight( ptex );
        tx_rect.ymax = ( float )bliprect[color]._bottom / ( float )oglx_texture_getTextureHeight( ptex );

        width  = sizeFactor * ( bliprect[color]._right  - bliprect[color]._left );
        height = sizeFactor * ( bliprect[color]._bottom - bliprect[color]._top );

        sc_rect.xmin = loc_x - ( width / 2 );
        sc_rect.xmax = loc_x + ( width / 2 );
        sc_rect.ymin = loc_y - ( height / 2 );
        sc_rect.ymax = loc_y + ( height / 2 );

        draw_quad_2d( ptex, sc_rect, tx_rect, true, NULL );
    }
}

//--------------------------------------------------------------------------------------------
float draw_icon_texture( oglx_texture_t * ptex, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size )
{
    float       width, height;
    ego_frect_t tx_rect, sc_rect;

    if ( NULL == ptex )
    {
        // defaults
        tx_rect.xmin = 0.0f;
        tx_rect.xmax = 1.0f;
        tx_rect.ymin = 0.0f;
        tx_rect.ymax = 1.0f;
    }
    else
    {
        tx_rect.xmin = 0.0f;
        tx_rect.xmax = ( float )ptex->imgW / ( float )ptex->base.width;
        tx_rect.ymin = 0.0f;
        tx_rect.ymax = ( float )ptex->imgW / ( float )ptex->base.width;
    }

    width  = ICON_SIZE;
    height = ICON_SIZE;

    // handle non-default behavior
    if ( size >= 0.0f )
    {
        float factor_wid = ( float )size / width;
        float factor_hgt = ( float )size / height;
        float factor = std::min( factor_wid, factor_hgt );

        width *= factor;
        height *= factor;
    }

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d( ptex, sc_rect, tx_rect, false, NULL );

    if ( NOSPARKLE != sparkle_color )
    {
        int         position;
        float       loc_blip_x, loc_blip_y;

        position = sparkle_timer & SPARKLE_AND;

        loc_blip_x = x + position * ( width / SPARKLE_SIZE );
        loc_blip_y = y;
        draw_blip( 0.5f, sparkle_color, loc_blip_x, loc_blip_y, false );

        loc_blip_x = x + width;
        loc_blip_y = y + position * ( height / SPARKLE_SIZE );
        draw_blip( 0.5f, sparkle_color, loc_blip_x, loc_blip_y, false );

        loc_blip_x = loc_blip_x - position  * ( width / SPARKLE_SIZE );
        loc_blip_y = y + height;
        draw_blip( 0.5f, sparkle_color, loc_blip_x, loc_blip_y, false );

        loc_blip_x = x;
        loc_blip_y = loc_blip_y - position * ( height / SPARKLE_SIZE );
        draw_blip( 0.5f, sparkle_color, loc_blip_x, loc_blip_y, false );
    }

    return y + height;
}

//--------------------------------------------------------------------------------------------
float draw_game_icon( const TX_REF icontype, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size )
{
    /// @author ZZ
    /// @details This function draws an icon

    return draw_icon_texture( TxList_get_valid_ptr( icontype ), x, y, sparkle_color, sparkle_timer, size );
}

//--------------------------------------------------------------------------------------------
float draw_menu_icon( const TX_REF icontype, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size )
{
    return draw_icon_texture( mnu_TxList_get_valid_ptr( icontype ), x, y, sparkle_color, sparkle_timer, size );
}

//--------------------------------------------------------------------------------------------
void draw_map_texture( float x, float y )
{
    /// @author ZZ
    /// @details This function draws the map

    ego_frect_t sc_rect, tx_rect;

    oglx_texture_t * ptex = TxList_get_valid_ptr(( TX_REF )TX_MAP );
    if ( NULL == ptex ) return;

    sc_rect.xmin = x;
    sc_rect.xmax = x + MAPSIZE;
    sc_rect.ymin = y;
    sc_rect.ymax = y + MAPSIZE;

    tx_rect.xmin = 0;
    tx_rect.xmax = ( float )ptex->imgW / ( float )ptex->base.width;
    tx_rect.ymin = 0;
    tx_rect.ymax = ( float )ptex->imgH / ( float )ptex->base.height;

    draw_quad_2d( ptex, sc_rect, tx_rect, false, NULL );
}

//--------------------------------------------------------------------------------------------
float draw_one_xp_bar( float x, float y, Uint8 ticks )
{
    /// @author ZF
    /// @details This function draws a xp bar and returns the y position for the next one

    int width, height;
    Uint8 cnt;
    ego_frect_t tx_rect, sc_rect;

    ticks = std::min( ticks, (Uint8)NUMTICK );

    gfx_enable_texturing();               // Enable texture mapping
	Ego::Renderer::getSingleton()->setColour(Ego::Math::Colour4f::WHITE);

    //---- Draw the tab (always colored)

    width = 16;
    height = XPTICK;

    tx_rect.xmin = 0;
    tx_rect.xmax = 32.00f / 128;
    tx_rect.ymin = XPTICK / 16;
    tx_rect.ymax = XPTICK * 2 / 16;

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d( TxList_get_valid_ptr(( TX_REF )TX_XP_BAR ), sc_rect, tx_rect, true, NULL );

    x += width;

    //---- Draw the filled ones
    tx_rect.xmin = 0.0f;
    tx_rect.xmax = 32 / 128.0f;
    tx_rect.ymin = XPTICK / 16.0f;
    tx_rect.ymax = 2 * XPTICK / 16.0f;

    width  = XPTICK;
    height = XPTICK;

    for ( cnt = 0; cnt < ticks; cnt++ )
    {
        sc_rect.xmin = x + ( cnt * width );
        sc_rect.xmax = x + ( cnt * width ) + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( TxList_get_valid_ptr(( TX_REF )TX_XP_BAR ), sc_rect, tx_rect, true, NULL );
    }

    //---- Draw the remaining empty ones
    tx_rect.xmin = 0;
    tx_rect.xmax = 32 / 128.0f;
    tx_rect.ymin = 0;
    tx_rect.ymax = XPTICK / 16.0f;

    for ( /*nothing*/; cnt < NUMTICK; cnt++ )
    {
        sc_rect.xmin = x + ( cnt * width );
        sc_rect.xmax = x + ( cnt * width ) + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( TxList_get_valid_ptr(( TX_REF )TX_XP_BAR ), sc_rect, tx_rect, true, NULL );
    }

    return y + height;
}

//--------------------------------------------------------------------------------------------
float draw_one_bar( Uint8 bartype, float x_stt, float y_stt, int ticks, int maxticks )
{
    /// @author ZZ
    /// @details This function draws a bar and returns the y position for the next one

    const float scale = 1.0f;

    float       width, height;
    ego_frect_t tx_rect, sc_rect;
    oglx_texture_t * tx_ptr;

    float tx_width, tx_height, img_width;
    float tab_width, tick_width, tick_height;

    int total_ticks = maxticks;
    int tmp_bartype = bartype;

    float x_left = x_stt;
    float x = x_stt;
    float y = y_stt;

    if ( maxticks <= 0 || ticks < 0 || bartype > NUMBAR ) return y;

    // limit the values to reasonable ones
    if ( total_ticks > MAXTICK ) total_ticks = MAXTICK;
    if ( ticks       > total_ticks ) ticks = total_ticks;

    // grab a pointer to the bar texture
    tx_ptr = TxList_get_valid_ptr(( TX_REF )TX_BARS );

    // allow the bitmap to be scaled to arbitrary size
    tx_width   = 128.0f;
    tx_height  = 128.0f;
    img_width  = 112.0f;
    if ( NULL != tx_ptr )
    {
        tx_width  = tx_ptr->base.width;
        tx_height = tx_ptr->base.height;
        img_width = tx_ptr->imgW;
    }

    // calculate the bar parameters
    tick_width  = img_width / 14.0f;
    tick_height = img_width / 7.0f;
    tab_width   = img_width / 3.5f;

    //---- Draw the tab
    tmp_bartype = bartype;

    tx_rect.xmin  = 0.0f       / tx_width;
    tx_rect.xmax  = tab_width  / tx_width;
    tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
    tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

    width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
    height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

    sc_rect.xmin = x;
    sc_rect.xmax = x + width;
    sc_rect.ymin = y;
    sc_rect.ymax = y + height;

    draw_quad_2d( tx_ptr, sc_rect, tx_rect, true, NULL );

    // make the new left-hand margin after the tab
    x_left = x_stt + width;
    x      = x_left;

    //---- Draw the full rows of ticks
    while ( ticks >= NUMTICK )
    {
        tmp_bartype = bartype;

        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = img_width  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, true, NULL );

        y += height;
        ticks -= NUMTICK;
        total_ticks -= NUMTICK;
    }

    if ( ticks > 0 )
    {
        int full_ticks = NUMTICK - ticks;
        int empty_ticks = NUMTICK - ( std::min( NUMTICK, total_ticks ) - ticks );

        //---- draw a partial row of full ticks
        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = ( img_width - tick_width * full_ticks )  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, true, NULL );

        // move to the right after drawing the full ticks
        x += width;

        //---- draw a partial row of empty ticks
        tmp_bartype = 0;

        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = ( img_width - tick_width * empty_ticks )  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, true, NULL );

        y += height;
        ticks = 0;
        total_ticks -= NUMTICK;
    }

    // reset the x position
    x = x_left;

    // Draw full rows of empty ticks
    while ( total_ticks >= NUMTICK )
    {
        tmp_bartype = 0;

        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = img_width  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, true, NULL );

        y += height;
        total_ticks -= NUMTICK;
    }

    // Draw the last of the empty ones
    if ( total_ticks > 0 )
    {
        int remaining = NUMTICK - total_ticks;

        //---- draw a partial row of empty ticks
        tmp_bartype = 0;

        tx_rect.xmin  = tab_width  / tx_width;
        tx_rect.xmax  = ( img_width - tick_width * remaining )  / tx_width;
        tx_rect.ymin  = tick_height * ( tmp_bartype + 0 ) / tx_height;
        tx_rect.ymax  = tick_height * ( tmp_bartype + 1 ) / tx_height;

        width  = ( tx_rect.xmax - tx_rect.xmin ) * scale * tx_width;
        height = ( tx_rect.ymax - tx_rect.ymin ) * scale * tx_height;

        sc_rect.xmin = x;
        sc_rect.xmax = x + width;
        sc_rect.ymin = y;
        sc_rect.ymax = y + height;

        draw_quad_2d( tx_ptr, sc_rect, tx_rect, true, NULL );

        y += height;
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_one_character_icon( const CHR_REF item, float x, float y, bool draw_ammo, Uint8 draw_sparkle )
{
    /// @author BB
    /// @details Draw an icon for the given item at the position <x,y>.
    ///     If the object is invalid, draw the null icon instead of failing
    ///     If NOSPARKLE is specified the default item sparkle will be used (default behaviour)

    TX_REF icon_ref;

    GameObject * pitem = !_gameObjects.exists( item ) ? NULL : _gameObjects.get( item );

    // grab the icon reference
    icon_ref = chr_get_txtexture_icon_ref( item );

    // draw the icon
    if ( draw_sparkle == NOSPARKLE ) draw_sparkle = ( NULL == pitem ) ? NOSPARKLE : pitem->sparkle;
    draw_game_icon( icon_ref, x, y, draw_sparkle, update_wld, -1 );

    // draw the ammo, if requested
    if ( draw_ammo && ( NULL != pitem ) )
    {
        if ( 0 != pitem->ammomax && pitem->ammoknown )
        {
            if (( !chr_get_ppro(item)->isStackable() ) || pitem->ammo > 1 )
            {
                // Show amount of ammo left
                draw_string_raw( x, y - 8, "%2d", pitem->ammo );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
float draw_character_xp_bar( const CHR_REF character, float x, float y )
{
    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return y;
    pchr = _gameObjects.get( character );

    //Draw the small XP progress bar
    if ( pchr->experiencelevel < MAXLEVEL - 1 )
    {
        std::shared_ptr<ObjectProfile> profile = _profileSystem.getProfile(pchr->profile_ref);

        uint8_t  curlevel    = pchr->experiencelevel + 1;
        uint32_t xplastlevel = profile->getXPNeededForLevel(curlevel-1);
        uint32_t xpneed      = profile->getXPNeededForLevel(curlevel);
        
        while (pchr->experience < xplastlevel && curlevel > 1) {
            curlevel--;
            xplastlevel = profile->getXPNeededForLevel(curlevel-1);
        }

		float fraction = ((float)(pchr->experience - xplastlevel)) / (float)std::max<uint32_t>( xpneed - xplastlevel, 1 );
        int   numticks = fraction * NUMTICK;

        y = draw_one_xp_bar( x, y, CLIP( numticks, 0, NUMTICK ) );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_status( const CHR_REF character, float x, float y )
{
    /// @author ZZ
    /// @details This function shows a character's icon, status and inventory
    ///    The x,y coordinates are the top left point of the image to draw

    STRING generictext;
    int life_pips, life_pips_max;
    int mana_pips, mana_pips_max;

    GameObject * pchr;

    if ( !_gameObjects.exists( character ) ) return y;
    pchr = _gameObjects.get( character );

    life_pips      = SFP8_TO_SINT( pchr->life );
    life_pips_max  = SFP8_TO_SINT( pchr->life_max );
    mana_pips      = SFP8_TO_SINT( pchr->mana );
    mana_pips_max  = SFP8_TO_SINT( pchr->mana_max );

    // make a short name for the actual display
    chr_get_name( character, CHRNAME_CAPITAL, generictext, 7 );
    generictext[7] = CSTR_END;

    // draw the name
    y = draw_string_raw( x + 8, y, "%s", generictext );

    // draw the character's money
    y = draw_string_raw( x + 8, y, "$%4d", pchr->money ) + 8;

    // draw the character's main icon
    draw_one_character_icon( character, x + 40, y, false, NOSPARKLE );

    // draw the left hand item icon
    draw_one_character_icon( pchr->holdingwhich[SLOT_LEFT], x + 8, y, true, NOSPARKLE );

    // draw the right hand item icon
    draw_one_character_icon( pchr->holdingwhich[SLOT_RIGHT], x + 72, y, true, NOSPARKLE );

    // skip to the next row
    y += 32;

    //Draw the small XP progress bar
    y = draw_character_xp_bar( character, x + 16, y );

    // Draw the life_pips bar
    if ( pchr->alive )
    {
        y = draw_one_bar( pchr->life_color, x, y, life_pips, life_pips_max );
    }
    else
    {
        y = draw_one_bar( 0, x, y, 0, life_pips_max );  // Draw a black bar
    }

    // Draw the mana_pips bar
    if ( mana_pips_max > 0 )
    {
        y = draw_one_bar( pchr->mana_color, x, y, mana_pips, mana_pips_max );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_all_status()
{
    if ( !StatusList.on ) return;

    // connect each status object with its camera
    status_list_update_cameras( &StatusList );

    // get the camera list
    const std::vector<std::shared_ptr<Camera>> &cameraList = _cameraSystem.getCameraList();

    for(size_t i = 0; i < cameraList.size(); ++i)
    {
        const std::shared_ptr<Camera> &camera = cameraList[i];

        // draw all attached status
        int y = camera->getScreen().ymin;
        for ( size_t tnc = 0; tnc < StatusList.count; tnc++ )
        {
            status_list_element_t * pelem = StatusList.lst + tnc;

            if ( i == pelem->camera_index )
            {
                y = draw_status( pelem->who, camera->getScreen().xmax - BARX, y );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void draw_map()
{
    size_t cnt;

    // Map display
    if ( !mapvalid || !mapon ) return;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT );
    {

        GL_DEBUG( glEnable )( GL_BLEND );                               // GL_COLOR_BUFFER_BIT
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        GL_DEBUG( glColor4fv )( Ego::white_vec );
        draw_map_texture( 0, sdl_scr.y - MAPSIZE );

        GL_DEBUG( glBlendFunc )( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

        // If one of the players can sense enemies via ESP, draw them as blips on the map
        if ( TEAM_MAX != local_stats.sense_enemies_team )
        {
            CHR_REF ichr;

            for ( ichr = 0; ichr < MAX_CHR && blip_count < MAXBLIP; ichr++ )
            {
                GameObject * pchr;

                if ( !_gameObjects.exists( ichr ) ) continue;
                pchr = _gameObjects.get( ichr );

                const std::shared_ptr<ObjectProfile> &profile = _profileSystem.getProfile(pchr->profile_ref);

                // Show only teams that will attack the player
                if ( team_hates_team( pchr->team, local_stats.sense_enemies_team ) )
                {
                    // Only if they match the required IDSZ ([NONE] always works)
                    if ( local_stats.sense_enemies_idsz == IDSZ_NONE ||
                         local_stats.sense_enemies_idsz == profile->getIDSZ(IDSZ_PARENT) ||
                         local_stats.sense_enemies_idsz == profile->getIDSZ(IDSZ_TYPE  ) )
                    {
                        // Inside the map?
                        if ( pchr->getPosX() < PMesh->gmem.edge_x && pchr->getPosY() < PMesh->gmem.edge_y )
                        {
                            // Valid colors only
                            blip_x[blip_count] = pchr->getPosX();
                            blip_y[blip_count] = pchr->getPosY();
                            blip_c[blip_count] = COLOR_RED; // Red blips
                            blip_count++;
                        }
                    }
                }
            }
        }

        // draw all the blips
        for ( cnt = 0; cnt < blip_count; cnt++ )
        {
            draw_blip( 0.75f, blip_c[cnt], blip_x[cnt], blip_y[cnt], true );
        }
        blip_count = 0;

        // Show local player position(s)
        if ( youarehereon && ( update_wld & 8 ) )
        {
            PLA_REF iplayer;

            for ( iplayer = 0; iplayer < MAX_PLAYER; iplayer++ )
            {
                CHR_REF ichr;

                // Only valid players
                if ( !PlaStack.lst[iplayer].valid ) continue;

                // Dont do networked players
                if ( NULL == PlaStack.lst[iplayer].pdevice ) continue;

                ichr = PlaStack.lst[iplayer].index;
                if ( _gameObjects.exists( ichr ) && _gameObjects.get(ichr)->alive )
                {
                    draw_blip( 0.75f, COLOR_WHITE, _gameObjects.get(ichr)->getPosX(), _gameObjects.get(ichr)->getPosY(), true );
                }
            }
        }

        // draw the camera(s)
        //if ( update_wld & 2 )
        //{
        //    ext_camera_iterator_t * it;

        //    for( it = camera_list_iterator_begin(); NULL != it; it = camera_list_iterator_next( it ) )
        //    {
        //        fvec3_t tmp_diff;

        //        camera_t * pcam = camera_list_iterator_get_camera(it);
        //        if( NULL == pcam ) continue;

        //        draw_blip( 0.75f, COLOR_PURPLE, pcam->getPosition().x, pcam->getPosition().y, true );
        //    }
        //    it = camera_list_iterator_end(it);
        //}
    }
    ATTRIB_POP( __FUNCTION__ )
}

//--------------------------------------------------------------------------------------------
float draw_fps( float y )
{
    // FPS text

    parser_state_t * ps = script_compiler_get_state();

    if ( outofsync )
    {
        y = draw_string_raw( 0, y, "OUT OF SYNC" );
    }

    if ( script_compiler_error( ps ) )
    {
        y = draw_string_raw( 0, y, "SCRIPT ERROR ( see \"/debug/log.txt\" )" );
    }

    if ( fpson )
    {
        y = draw_string_raw( 0, y, "%2.3f FPS, %2.3f UPS, Update lag = %d", stabilized_game_fps, stabilized_game_ups, update_lag );

        //Extra debug info
        if ( cfg.dev_mode )
        {

#    if defined(DEBUG_BSP)
            y = draw_string_raw( 0, y, "BSP chr %d/%d - BSP prt %d/%lu", getChrBSP()->count, _characterList.size(), getPtrBSP()->count, maxparticles - PrtList_count_free() );
            y = draw_string_raw( 0, y, "BSP infinite %lu", getChrBSP()->tree.infinite.count + getPtrBSP()->tree.infinite.count );
            y = draw_string_raw( 0, y, "BSP collisions %d", CHashList_inserted );
            //y = draw_string_raw( 0, y, "chr-mesh tests %04d - prt-mesh tests %04d", chr_stoppedby_tests + chr_pressure_tests, prt_stoppedby_tests + prt_pressure_tests );
#    endif

#if defined(DEBUG_RENDERLIST)
            y = draw_string_raw( 0, y, "Renderlist tiles %d/%d", renderlist.all.count, PMesh->info.tiles_count );
#endif

#if defined(_DEBUG)

#    if defined(DEBUG_PROFILE_DISPLAY) && defined(_DEBUG)

#        if defined(DEBUG_PROFILE_RENDER) && defined(_DEBUG)
            y = draw_string_raw( 0, y, "estimated max FPS %2.3f UPS %4.2f GFX %4.2f", est_max_fps, est_max_ups, est_max_gfx );
            y = draw_string_raw( 0, y, "gfx:total %2.4f, render:total %2.4f", est_render_time, time_draw_scene );
            y = draw_string_raw( 0, y, "render:init %2.4f,  render:mesh %2.4f", time_render_scene_init, time_render_scene_mesh );
            y = draw_string_raw( 0, y, "render:solid %2.4f, render:water %2.4f", time_render_scene_solid, time_render_scene_water );
            y = draw_string_raw( 0, y, "render:trans %2.4f", time_render_scene_trans );
#        endif

#        if defined(DEBUG_PROFILE_MESH) && defined(_DEBUG)
            y = draw_string_raw( 0, y, "mesh:total %2.4f", time_render_scene_mesh );
            y = draw_string_raw( 0, y, "mesh:dolist_sort %2.4f, mesh:ndr %2.4f", time_render_scene_mesh_dolist_sort , time_render_scene_mesh_ndr );
            y = draw_string_raw( 0, y, "mesh:drf_back %2.4f, mesh:ref %2.4f", time_render_scene_mesh_drf_back, time_render_scene_mesh_ref );
            y = draw_string_raw( 0, y, "mesh:ref_chr %2.4f, mesh:drf_solid %2.4f", time_render_scene_mesh_ref_chr, time_render_scene_mesh_drf_solid );
            y = draw_string_raw( 0, y, "mesh:render_shadows %2.4f", time_render_scene_mesh_render_shadows );
#        endif

#        if defined(DEBUG_PROFILE_INIT) && defined(_DEBUG)
            y = draw_string_raw( 0, y, "init:total %2.4f", time_render_scene_init );
            y = draw_string_raw( 0, y, "init:gfx_make_renderlist %2.4f, init:gfx_make_dolist %2.4f", time_render_scene_init_renderlist_make, time_render_scene_init_dolist_make );
            y = draw_string_raw( 0, y, "init:do_grid_lighting %2.4f, init:light_fans %2.4f", time_render_scene_init_do_grid_dynalight, time_render_scene_init_light_fans );
            y = draw_string_raw( 0, y, "init:gfx_update_all_chr_instance %2.4f", time_render_scene_init_update_all_chr_instance );
            y = draw_string_raw( 0, y, "init:update_all_prt_instance %2.4f", time_render_scene_init_update_all_prt_instance );
#        endif

#    endif
#endif
        }
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_help( float y )
{
    if ( SDL_KEYDOWN( keyb, SDLK_F1 ) )
    {
        // In-Game help
        y = draw_string_raw( 0, y, "!!!MOUSE HELP!!!" );
        y = draw_string_raw( 0, y, "~~Go to input settings to change" );
        y = draw_string_raw( 0, y, "Default settings" );
        y = draw_string_raw( 0, y, "~~Left Click to use an item" );
        y = draw_string_raw( 0, y, "~~Left and Right Click to grab" );
        y = draw_string_raw( 0, y, "~~Middle Click to jump" );
        y = draw_string_raw( 0, y, "~~A and S keys do stuff" );
        y = draw_string_raw( 0, y, "~~Right Drag to move camera" );
    }
    if ( SDL_KEYDOWN( keyb, SDLK_F2 ) )
    {
        // In-Game help
        y = draw_string_raw( 0, y, "!!!JOYSTICK HELP!!!" );
        y = draw_string_raw( 0, y, "~~Go to input settings to change." );
        y = draw_string_raw( 0, y, "~~Hit the buttons" );
        y = draw_string_raw( 0, y, "~~You'll figure it out" );
    }
    if ( SDL_KEYDOWN( keyb, SDLK_F3 ) )
    {
        // In-Game help
        y = draw_string_raw( 0, y, "!!!KEYBOARD HELP!!!" );
        y = draw_string_raw( 0, y, "~~Go to input settings to change." );
        y = draw_string_raw( 0, y, "Default settings" );
        y = draw_string_raw( 0, y, "~~TGB control left hand" );
        y = draw_string_raw( 0, y, "~~YHN control right hand" );
        y = draw_string_raw( 0, y, "~~Keypad to move and jump" );
        y = draw_string_raw( 0, y, "~~Number keys for stats" );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_debug( float y )
{
    if ( !cfg.dev_mode ) return y;

    if ( SDL_KEYDOWN( keyb, SDLK_F5 ) )
    {
        CHR_REF ichr;
        PLA_REF ipla;

        // Debug information
        y = draw_string_raw( 0, y, "!!!DEBUG MODE-5!!!" );
        y = draw_string_raw( 0, y, "~~CAM %f %f %f", _cameraSystem.getMainCamera()->getPosition().x, _cameraSystem.getMainCamera()->getPosition().y, _cameraSystem.getMainCamera()->getPosition().z );
        ipla = ( PLA_REF )0;
        if(VALID_PLA(ipla))
        {
            ichr = PlaStack.lst[ipla].index;
            y = draw_string_raw( 0, y, "~~PLA0DEF %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f %4.2f",
                                 _gameObjects.get(ichr)->damage_resistance[DAMAGE_SLASH],
                                 _gameObjects.get(ichr)->damage_resistance[DAMAGE_CRUSH],
                                 _gameObjects.get(ichr)->damage_resistance[DAMAGE_POKE ],
                                 _gameObjects.get(ichr)->damage_resistance[DAMAGE_HOLY ],
                                 _gameObjects.get(ichr)->damage_resistance[DAMAGE_EVIL ],
                                 _gameObjects.get(ichr)->damage_resistance[DAMAGE_FIRE ],
                                 _gameObjects.get(ichr)->damage_resistance[DAMAGE_ICE  ],
                                 _gameObjects.get(ichr)->damage_resistance[DAMAGE_ZAP  ] );

            ichr = PlaStack.lst[ipla].index;
            y = draw_string_raw( 0, y, "~~PLA0 %5.1f %5.1f", _gameObjects.get(ichr)->getPosX() / GRID_FSIZE, _gameObjects.get(ichr)->getPosY() / GRID_FSIZE );            
        }

        ipla = ( PLA_REF )1;
        if(VALID_PLA(ipla))
        {
            ichr = PlaStack.lst[ipla].index;
            y = draw_string_raw( 0, y, "~~PLA1 %5.1f %5.1f", _gameObjects.get(ichr)->getPosY() / GRID_FSIZE, _gameObjects.get(ichr)->getPosY() / GRID_FSIZE );            
        }
    }

    if ( SDL_KEYDOWN( keyb, SDLK_F6 ) )
    {
        // More debug information
        y = draw_string_raw( 0, y, "!!!DEBUG MODE-6!!!" );
        y = draw_string_raw( 0, y, "~~FREEPRT %d", PrtList_count_free() );
        y = draw_string_raw( 0, y, "~~FREECHR %d", MAX_CHR - _gameObjects.getObjectCount() );
        y = draw_string_raw( 0, y, "~~MACHINE %d", egonet_get_local_machine() );
        y = draw_string_raw( 0, y, PMod->isExportValid() ? "~~EXPORT: TRUE" : "~~EXPORT: FALSE" );
        y = draw_string_raw( 0, y, "~~PASS %d", PMod->getPassageCount() );
        y = draw_string_raw( 0, y, "~~NETPLAYERS %d", egonet_get_client_count() );
        y = draw_string_raw( 0, y, "~~DAMAGEPART %d", damagetile.part_gpip );

        // y = draw_string_raw( 0, y, "~~FOGAFF %d", fog_data.affects_water );
    }

    if ( SDL_KEYDOWN( keyb, SDLK_F7 ) )
    {
        std::shared_ptr<Camera> camera = _cameraSystem.getMainCamera();

        // White debug mode
        y = draw_string_raw( 0, y, "!!!DEBUG MODE-7!!!" );
        y = draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", camera->getView().CNV( 0, 0 ), camera->getView().CNV( 1, 0 ), camera->getView().CNV( 2, 0 ), camera->getView().CNV( 3, 0 ) );
        y = draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", camera->getView().CNV( 0, 1 ), camera->getView().CNV( 1, 1 ), camera->getView().CNV( 2, 1 ), camera->getView().CNV( 3, 1 ) );
        y = draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", camera->getView().CNV( 0, 2 ), camera->getView().CNV( 1, 2 ), camera->getView().CNV( 2, 2 ), camera->getView().CNV( 3, 2 ) );
        y = draw_string_raw( 0, y, "CAM <%f, %f, %f, %f>", camera->getView().CNV( 0, 3 ), camera->getView().CNV( 1, 3 ), camera->getView().CNV( 2, 3 ), camera->getView().CNV( 3, 3 ) );
        y = draw_string_raw( 0, y, "CAM center <%f, %f>", camera->getCenter().x, camera->getCenter().y );
        y = draw_string_raw( 0, y, "CAM turn %d %d", camera->getTurnMode(), camera->getTurnTime() );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_timer( float y )
{
    int fifties, seconds, minutes;

    if ( timeron )
    {
        fifties = ( timervalue % 50 ) << 1;
        seconds = (( timervalue / 50 ) % 60 );
        minutes = ( timervalue / 3000 );
        y = draw_string_raw( 0, y, "=%d:%02d:%02d=", minutes, seconds, fifties );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
float draw_game_status( float y )
{

    if ( egonet_get_waitingforclients() )
    {
        y = draw_string_raw( 0, y, "Waiting for players... " );
    }
    else if ( ServerState.player_count > 0 )
    {
        if ( local_stats.allpladead || PMod->canRespawnAnyTime() )
        {
            if ( PMod->isRespawnValid() && cfg.difficulty < GAME_HARD )
            {
                y = draw_string_raw( 0, y, "PRESS SPACE TO RESPAWN" );
            }
            else
            {
                y = draw_string_raw( 0, y, "PRESS ESCAPE TO QUIT" );
            }
        }
        else if ( PMod->isBeaten() )
        {
            y = draw_string_raw( 0, y, "VICTORY!  PRESS ESCAPE" );
        }
    }
    else
    {
        y = draw_string_raw( 0, y, "ERROR: MISSING PLAYERS" );
    }

    return y;
}

//--------------------------------------------------------------------------------------------
void draw_hud()
{
    /// @author ZZ
    /// @details draw in-game heads up display

    int y;

    gfx_begin_2d();
    {
        draw_map();

        draw_inventory();

        draw_all_status();

        y = draw_fps( 0 );
        y = draw_help( y );
        y = draw_debug( y );
        y = draw_timer( y );
        y = draw_game_status( y );

        // Network message input
        if ( keyb.chat_mode )
        {
            char buffer[CHAT_BUFFER_SIZE + 128] = EMPTY_CSTR;

            snprintf( buffer, SDL_arraysize( buffer ), "%s > %s%s", cfg.network_messagename, net_chat.buffer, HAS_NO_BITS( update_wld, 8 ) ? "x" : "+" );

            y = draw_wrap_string( buffer, 0, y, sdl_scr.x - wraptolerance );
        }

        y = DisplayMsg_draw_all( y );
    }
    gfx_end_2d();
}

//--------------------------------------------------------------------------------------------
void draw_inventory()
{
    /// @author ZF
    /// @details This renders the open inventories of all local players

    PLA_REF ipla;
    player_t * ppla;
    GLXvector4f background_color = { 0.66f, 0.0f, 0.0f, 0.95f };

    CHR_REF ichr;
    GameObject *pchr;

    PLA_REF draw_list[MAX_LOCAL_PLAYERS];
    size_t cnt, draw_list_length = 0;

    float sttx, stty;
    int width, height;

    static int lerp_time[MAX_LOCAL_PLAYERS] = { 0 };

    //don't draw inventories while menu is open
    if ( GProc->mod_paused ) return;

    //figure out what we have to draw
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        //valid player?
        ppla = PlaStack.get_ptr( ipla );
        if ( !ppla->valid ) continue;

        //draw inventory?
        if ( !ppla->draw_inventory ) continue;
        ichr = ppla->index;

        //valid character?
        if ( !_gameObjects.exists( ichr ) ) continue;
        pchr = _gameObjects.get( ichr );

        //don't draw inventories of network players
        if ( !pchr->islocalplayer ) continue;

        draw_list[draw_list_length++] = ipla;
    }

    //figure out size and position of the inventory
    width = 180;
    height = 140;

    sttx = 0;
    stty = GFX_HEIGHT / 2 - height / 2;
    stty -= height * ( draw_list_length - 1 );
    stty = std::max( 0.0f, stty );

    //now draw each inventory
    for ( cnt = 0; cnt < draw_list_length; cnt++ )
    {
        size_t i;
        STRING buffer;
        int icon_count, item_count, weight_sum, max_weight;
        float x, y, edgex;

        //Figure out who this is
        ipla = draw_list[cnt];
        ppla = PlaStack.get_ptr( ipla );

        ichr = ppla->index;
        pchr = _gameObjects.get( ichr );

        //handle inventories sliding into view
        ppla->inventory_lerp = std::min( ppla->inventory_lerp, width );
        if ( ppla->inventory_lerp > 0 && lerp_time[cnt] < SDL_GetTicks() )
        {
            lerp_time[cnt] = SDL_GetTicks() + 1;
            ppla->inventory_lerp = std::max( 0, ppla->inventory_lerp - 16 );
        }

        //set initial positions
        x = sttx - ppla->inventory_lerp;
        y = stty;

        //threshold to start a new row
        edgex = sttx + width + 5 - 32;

        //calculate max carry weight
        max_weight = 200 + FP8_TO_FLOAT( pchr->strength ) * FP8_TO_FLOAT( pchr->strength );

        //draw the backdrop
        ui_drawButton( 0, x, y, width, height, background_color );
        x += 5;

        //draw title
        draw_wrap_string( chr_get_name( ichr, CHRNAME_CAPITAL, NULL, 0 ), x, y, x + width );
        y += 32;

        //draw each inventory icon
        weight_sum = 0;
        icon_count = 0;
        item_count = 0;
        for ( i = 0; i < MAXINVENTORY; i++ )
        {
            CHR_REF item = pchr->inventory[i];

            //calculate the sum of the weight of all items in inventory
            if ( _gameObjects.exists( item ) ) weight_sum += chr_get_ppro(item)->getWeight();

            //draw icon
            draw_one_character_icon( item, x, y, true, ( item_count == ppla->inventory_slot ) ? COLOR_WHITE : NOSPARKLE );
            icon_count++;
            item_count++;
            x += 32 + 5;

            //new row?
            if ( x >= edgex || icon_count >= MAXINVENTORY / 2 )
            {
                x = sttx + 5 - ppla->inventory_lerp;
                y += 32 + 5;
                icon_count = 0;
            }
        }

        //Draw weight
        x = sttx + 5 - ppla->inventory_lerp;
        y = stty + height - 42;
        snprintf( buffer, SDL_arraysize( buffer ), "Weight: %d/%d", weight_sum, max_weight );
        draw_wrap_string( buffer, x, y, sttx + width + 5 );

        //prepare drawing the next inventory
        stty += height + 10;
    }

}

//--------------------------------------------------------------------------------------------
void draw_mouse_cursor()
{
    int     x, y;
    oglx_texture_t * pcursor;

    if ( !mous.on )
    {
        SDL_ShowCursor( SDL_DISABLE );
        return;
    }

    pcursor = mnu_TxList_get_valid_ptr( MENU_TX_CURSOR );

    // Invalid texture?
    if ( NULL == pcursor )
    {
        SDL_ShowCursor( SDL_ENABLE );
    }
    else
    {
        oglx_frect_t tx_tmp, sc_tmp;

        // Hide the SDL mouse
        SDL_ShowCursor( SDL_DISABLE );

        x = ABS( mous.x );
        y = ABS( mous.y );

        if ( oglx_texture_getSize( pcursor, tx_tmp, sc_tmp ) )
        {
            ego_frect_t tx_rect, sc_rect;

            tx_rect.xmin = tx_tmp[0];
            tx_rect.ymin = tx_tmp[1];
            tx_rect.xmax = tx_tmp[2];
            tx_rect.ymax = tx_tmp[3];

            sc_rect.xmin = x + sc_tmp[0];
            sc_rect.ymin = y + sc_tmp[1];
            sc_rect.xmax = x + sc_tmp[2];
            sc_rect.ymax = y + sc_tmp[3];

            draw_quad_2d( pcursor, sc_rect, tx_rect, true, NULL );
        }
    }
}

//--------------------------------------------------------------------------------------------
// 3D RENDERER FUNCTIONS
//--------------------------------------------------------------------------------------------
void render_shadow_sprite( float intensity, GLvertex v[] )
{
    int i;

    if ( intensity*255.0f < 1.0f ) return;

    GL_DEBUG( glColor4f )( intensity, intensity, intensity, 1.0f );

    GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
    {
        for ( i = 0; i < 4; i++ )
        {
            GL_DEBUG( glTexCoord2fv )( v[i].tex );
            GL_DEBUG( glVertex3fv )( v[i].pos );
        }
    }
    GL_DEBUG_END();
}

//--------------------------------------------------------------------------------------------
void render_shadow( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function draws a NIFTY shadow

    GLvertex v[4];

    TX_REF  itex;
    int     itex_style;
    float   x, y;
    float   level;
    float   height, size_umbra, size_penumbra;
    float   alpha, alpha_umbra, alpha_penumbra;
    GameObject * pchr;
    ego_tile_info_t * ptile;

    if ( IS_ATTACHED_CHR( character ) ) return;
    pchr = _gameObjects.get( character );

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden || 0 == pchr->shadow_size ) return;

    // no shadow if off the mesh
    ptile = ego_mesh_get_ptile( PMesh, pchr->onwhichgrid );
    if ( NULL == ptile ) return;

    // no shadow if invalid tile image
    if ( TILE_IS_FANOFF( *ptile ) ) return;

    // no shadow if completely transparent
    alpha = ( 255 == pchr->inst.light ) ? pchr->inst.alpha  * INV_FF : ( pchr->inst.alpha - pchr->inst.light ) * INV_FF;

    /// @test ZF@> The previous test didn't work, but this one does
    //if ( alpha * 255 < 1 ) return;
    if ( pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE ) return;

    // much reduced shadow if on a reflective tile
    if ( 0 != ego_mesh_test_fx( PMesh, pchr->onwhichgrid, MAPFX_DRAWREF ) )
    {
        alpha *= 0.1f;
    }
    if ( alpha < INV_FF ) return;

    // Original points
    level = pchr->enviro.floor_level;
    level += SHADOWRAISE;
    height = pchr->inst.matrix.CNV( 3, 2 ) - level;
    if ( height < 0 ) height = 0;

    size_umbra    = 1.5f * ( pchr->bump.size - height / 30.0f );
    size_penumbra = 1.5f * ( pchr->bump.size + height / 30.0f );

    alpha *= 0.3f;
    alpha_umbra = alpha_penumbra = alpha;
    if ( height > 0 )
    {
        float factor_penumbra = ( 1.5f ) * (( pchr->bump.size ) / size_penumbra );
        float factor_umbra    = ( 1.5f ) * (( pchr->bump.size ) / size_umbra );

        factor_umbra    = std::max( 1.0f, factor_umbra );
        factor_penumbra = std::max( 1.0f, factor_penumbra );

        alpha_umbra    *= 1.0f / factor_umbra / factor_umbra / 1.5f;
        alpha_penumbra *= 1.0f / factor_penumbra / factor_penumbra / 1.5f;

        alpha_umbra    = CLIP( alpha_umbra,    0.0f, 1.0f );
        alpha_penumbra = CLIP( alpha_penumbra, 0.0f, 1.0f );
    }

    x = pchr->inst.matrix.CNV( 3, 0 );
    y = pchr->inst.matrix.CNV( 3, 1 );

    // Choose texture.
    itex = TX_PARTICLE_LIGHT;
    oglx_texture_Bind( TxList_get_valid_ptr( itex ) );

    itex_style = prt_get_texture_style( itex );
    if ( itex_style < 0 ) itex_style = 0;

    // GOOD SHADOW
    v[0].tex[SS] = CALCULATE_PRT_U0( itex_style, 238 );
    v[0].tex[TT] = CALCULATE_PRT_V0( itex_style, 238 );

    v[1].tex[SS] = CALCULATE_PRT_U1( itex_style, 255 );
    v[1].tex[TT] = CALCULATE_PRT_V0( itex_style, 238 );

    v[2].tex[SS] = CALCULATE_PRT_U1( itex_style, 255 );
    v[2].tex[TT] = CALCULATE_PRT_V1( itex_style, 255 );

    v[3].tex[SS] = CALCULATE_PRT_U0( itex_style, 238 );
    v[3].tex[TT] = CALCULATE_PRT_V1( itex_style, 255 );

    if ( size_penumbra > 0 )
    {
        v[0].pos[XX] = x + size_penumbra;
        v[0].pos[YY] = y - size_penumbra;
        v[0].pos[ZZ] = level;

        v[1].pos[XX] = x + size_penumbra;
        v[1].pos[YY] = y + size_penumbra;
        v[1].pos[ZZ] = level;

        v[2].pos[XX] = x - size_penumbra;
        v[2].pos[YY] = y + size_penumbra;
        v[2].pos[ZZ] = level;

        v[3].pos[XX] = x - size_penumbra;
        v[3].pos[YY] = y - size_penumbra;
        v[3].pos[ZZ] = level;

        render_shadow_sprite( alpha_penumbra, v );
    };

    if ( size_umbra > 0 )
    {
        v[0].pos[XX] = x + size_umbra;
        v[0].pos[YY] = y - size_umbra;
        v[0].pos[ZZ] = level + 0.1f;

        v[1].pos[XX] = x + size_umbra;
        v[1].pos[YY] = y + size_umbra;
        v[1].pos[ZZ] = level + 0.1f;

        v[2].pos[XX] = x - size_umbra;
        v[2].pos[YY] = y + size_umbra;
        v[2].pos[ZZ] = level + 0.1f;

        v[3].pos[XX] = x - size_umbra;
        v[3].pos[YY] = y - size_umbra;
        v[3].pos[ZZ] = level + 0.1f;

        render_shadow_sprite( alpha_umbra, v );
    };
}

//--------------------------------------------------------------------------------------------
void render_bad_shadow( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function draws a sprite shadow

    GLvertex v[4];

    TX_REF  itex;
    int     itex_style;
    float   size, x, y;
    float   level, height, height_factor, alpha;
    GameObject * pchr;
    ego_tile_info_t * ptile;

    if ( IS_ATTACHED_CHR( character ) ) return;
    pchr = _gameObjects.get( character );

    // if the character is hidden, not drawn at all, so no shadow
    if ( pchr->is_hidden || 0 == pchr->shadow_size ) return;

    // no shadow if off the mesh
    ptile = ego_mesh_get_ptile( PMesh, pchr->onwhichgrid );
    if ( NULL == ptile ) return;

    // no shadow if invalid tile image
    if ( TILE_IS_FANOFF( *ptile ) ) return;

    // no shadow if completely transparent or completely glowing
    alpha = ( 255 == pchr->inst.light ) ? pchr->inst.alpha  * INV_FF : ( pchr->inst.alpha - pchr->inst.light ) * INV_FF;

    /// @test ZF@> previous test didn't work, but this one does
    //if ( alpha * 255 < 1 ) return;
    if ( pchr->inst.light <= INVISIBLE || pchr->inst.alpha <= INVISIBLE ) return;

    // much reduced shadow if on a reflective tile
    if ( 0 != ego_mesh_test_fx( PMesh, pchr->onwhichgrid, MAPFX_DRAWREF ) )
    {
        alpha *= 0.1f;
    }
    if ( alpha < INV_FF ) return;

    // Original points
    level = pchr->enviro.floor_level;
    level += SHADOWRAISE;
    height = pchr->inst.matrix.CNV( 3, 2 ) - level;
    height_factor = 1.0f - height / ( pchr->shadow_size * 5.0f );
    if ( height_factor <= 0.0f ) return;

    // how much transparency from height
    alpha *= height_factor * 0.5f + 0.25f;
    if ( alpha < INV_FF ) return;

    x = pchr->inst.matrix.CNV( 3, 0 );
    y = pchr->inst.matrix.CNV( 3, 1 );

    size = pchr->shadow_size * height_factor;

    v[0].pos[XX] = ( float ) x + size;
    v[0].pos[YY] = ( float ) y - size;
    v[0].pos[ZZ] = ( float ) level;

    v[1].pos[XX] = ( float ) x + size;
    v[1].pos[YY] = ( float ) y + size;
    v[1].pos[ZZ] = ( float ) level;

    v[2].pos[XX] = ( float ) x - size;
    v[2].pos[YY] = ( float ) y + size;
    v[2].pos[ZZ] = ( float ) level;

    v[3].pos[XX] = ( float ) x - size;
    v[3].pos[YY] = ( float ) y - size;
    v[3].pos[ZZ] = ( float ) level;

    // Choose texture and matrix
    itex = TX_PARTICLE_LIGHT;
    oglx_texture_Bind( TxList_get_valid_ptr( itex ) );

    itex_style = prt_get_texture_style( itex );
    if ( itex_style < 0 ) itex_style = 0;

    v[0].tex[SS] = CALCULATE_PRT_U0( itex_style, 236 );
    v[0].tex[TT] = CALCULATE_PRT_V0( itex_style, 236 );

    v[1].tex[SS] = CALCULATE_PRT_U1( itex_style, 253 );
    v[1].tex[TT] = CALCULATE_PRT_V0( itex_style, 236 );

    v[2].tex[SS] = CALCULATE_PRT_U1( itex_style, 253 );
    v[2].tex[TT] = CALCULATE_PRT_V1( itex_style, 253 );

    v[3].tex[SS] = CALCULATE_PRT_U0( itex_style, 236 );
    v[3].tex[TT] = CALCULATE_PRT_V1( itex_style, 253 );

    render_shadow_sprite( alpha, v );
}

//--------------------------------------------------------------------------------------------
struct by_element_t
{
    float  dist;
    Uint32 tile;
    Uint32 texture;
};
#if 0
typedef struct s_by_element by_element_t;
#endif
int by_element_cmp( const void *lhs, const void *rhs )
{
    int retval = 0;
    by_element_t * loc_lhs = ( by_element_t * )lhs;
    by_element_t * loc_rhs = ( by_element_t * )rhs;

    if ( NULL == loc_lhs && NULL == loc_rhs )
    {
        retval = 0;
    }
    else if ( NULL == loc_lhs && NULL != loc_rhs )
    {
        retval = 1;
    }
    else if ( NULL != loc_lhs && NULL == loc_rhs )
    {
        retval = -1;
    }
    else
    {
        retval = loc_lhs->texture - loc_rhs->texture;
        if ( 0 == retval )
        {
            float ftmp = loc_lhs->dist - loc_rhs->dist;

            if ( ftmp < 0.0f )
            {
                retval = -1;
            }
            else if ( ftmp > 0.0f )
            {
                retval = 1;
            }
        }
    }

    return retval;
}

struct by_list_t
{
    size_t       count;
    by_element_t lst[renderlist_lst_t::CAPACITY];
};
#if 0
typedef struct s_by_list by_list_t;
#endif
by_list_t * by_list_qsort( by_list_t * lst )
{
    if ( NULL == lst ) return lst;

    qsort( lst->lst, lst->count, sizeof( lst->lst[0] ), by_element_cmp );

    return lst;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_fans_by_list( const ego_mesh_t * pmesh, const renderlist_lst_t * rlst )
{
    Uint32 cnt;
    gfx_rv render_rv;

    size_t                  tcnt;
    const ego_tile_info_t * tlst;

    by_list_t lst_vals = {0};

    if ( NULL == pmesh ) pmesh = PMesh;
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh" );
        return gfx_error;
    }
    tcnt = pmesh->tmem.tile_count;
    tlst = pmesh->tmem.tile_list;

    if ( NULL == rlst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL tile rlst" );
        return gfx_error;
    }

    if ( 0 == rlst->count ) return gfx_success;

    // insert the rlst values into lst_vals
    lst_vals.count = rlst->count;
    for ( cnt = 0; cnt < rlst->count; cnt++ )
    {
        lst_vals.lst[cnt].tile = rlst->lst[cnt].index;
        lst_vals.lst[cnt].dist = rlst->lst[cnt].distance;

        if ( rlst->lst[cnt].index >= tcnt )
        {
            lst_vals.lst[cnt].texture = ( Uint32 )( ~0 );
        }
        else
        {
            int img = ~0;
            const ego_tile_info_t * ptile = tlst + rlst->lst[cnt].index;

            img =  TILE_GET_LOWER_BITS( ptile->img );
            if ( ptile->type >= tile_dict.offset )
            {
                img += MESH_IMG_COUNT;
            }

            lst_vals.lst[cnt].texture = img;
        }
    }

    by_list_qsort( &lst_vals );

    // restart the mesh texture code
    mesh_texture_invalidate();

    for ( cnt = 0; cnt < rlst->count; cnt++ )
    {
        Uint32 tmp_itile = lst_vals.lst[cnt].tile;

        render_rv = render_fan( pmesh, tmp_itile );
        if ( cfg.dev_mode && gfx_error == render_rv )
        {
            log_warning( "%s - error rendering tile %d.\n", __FUNCTION__, tmp_itile );
        }
    }

    // let the mesh texture code know that someone else is in control now
    mesh_texture_invalidate();

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// render_scene FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv render_scene_init( renderlist_t * prlist, dolist_t * pdolist, dynalist_t * pdylist, std::shared_ptr<Camera> pcam )
{
    gfx_rv retval;
    ego_mesh_t * pmesh;

    // assume the best;
    retval = gfx_success;

    PROFILE_BEGIN( gfx_make_renderlist );
    {
        // Which tiles can be displayed
        if ( gfx_error == gfx_make_renderlist( prlist, pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( gfx_make_renderlist );

    pmesh = renderlist_t::getMesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

    PROFILE_BEGIN( gfx_make_dolist );
    {
        // determine which objects are visible
        if ( gfx_error == gfx_make_dolist( pdolist, pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( gfx_make_dolist );

    // put off sorting the dolist until later
    // because it has to be sorted differently for reflected and non-reflected objects
    // dolist_sort( pcam, false );

    PROFILE_BEGIN( do_grid_lighting );
    {
        // figure out the terrain lighting
        if ( gfx_error == do_grid_lighting( prlist, pdylist, pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( do_grid_lighting );

    PROFILE_BEGIN( light_fans );
    {
        // apply the lighting to the characters and particles
        if ( gfx_error == light_fans( prlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( light_fans );

    PROFILE_BEGIN( gfx_update_all_chr_instance );
    {
        // make sure the characters are ready to draw
        if ( gfx_error == gfx_update_all_chr_instance() )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( gfx_update_all_chr_instance );

    PROFILE_BEGIN( update_all_prt_instance );
    {
        // make sure the particles are ready to draw
        if ( gfx_error == update_all_prt_instance( pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( update_all_prt_instance );

    // do the flashing for kursed objects
    if ( gfx_error == gfx_update_flashing( pdolist ) )
    {
        retval = gfx_error;
    }

    time_render_scene_init_renderlist_make         = PROFILE_QUERY( gfx_make_renderlist ) * TARGET_FPS;
    time_render_scene_init_dolist_make             = PROFILE_QUERY( gfx_make_dolist ) * TARGET_FPS;
    time_render_scene_init_do_grid_dynalight       = PROFILE_QUERY( do_grid_lighting ) * TARGET_FPS;
    time_render_scene_init_light_fans              = PROFILE_QUERY( light_fans ) * TARGET_FPS;
    time_render_scene_init_update_all_chr_instance = PROFILE_QUERY( gfx_update_all_chr_instance ) * TARGET_FPS;
    time_render_scene_init_update_all_prt_instance = PROFILE_QUERY( update_all_prt_instance ) * TARGET_FPS;

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_ndr( const renderlist_t * prlist )
{
    /// @author BB
    /// @details draw all tiles that do not reflect characters

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    {
        // store the surface depth
        GL_DEBUG( glDepthMask )( GL_TRUE );         // GL_DEPTH_BUFFER_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );       // GL_DEPTH_BUFFER_BIT

        // no transparency
        GL_DEBUG( glDisable )( GL_BLEND );          // GL_ENABLE_BIT

        // draw draw front and back faces of polygons
        oglx_end_culling();      // GL_ENABLE_BIT

        // do not display the completely transparent portion
        // use alpha test to allow the thatched roof tiles to look like thatch
        GL_DEBUG( glEnable )( GL_ALPHA_TEST );      // GL_ENABLE_BIT
        // speed-up drawing of surfaces with alpha == 0.0f sections
        GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );   // GL_COLOR_BUFFER_BIT

        // reduce texture hashing by loading up each texture only once
        if ( gfx_error == render_fans_by_list( prlist->mesh, &( prlist->ndr ) ) )
        {
            retval = gfx_error;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_drf_back( const renderlist_t * prlist )
{
    /// @author BB
    /// @details draw the reflective tiles, but turn off the depth buffer
    ///               this blanks out any background that might've been drawn

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    {
        // DO NOT store the surface depth
        GL_DEBUG( glDepthMask )( GL_FALSE );        // GL_DEPTH_BUFFER_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );       // GL_DEPTH_BUFFER_BIT

        // black out any backgound, but allow the background to show through any holes in the floor
        GL_DEBUG( glEnable )( GL_BLEND );                              // GL_ENABLE_BIT
        // use the alpha channel to modulate the transparency
        GL_DEBUG( glBlendFunc )( GL_ZERO, GL_ONE_MINUS_SRC_ALPHA );    // GL_COLOR_BUFFER_BIT

        // do not display the completely transparent portion
        // use alpha test to allow the thatched roof tiles to look like thatch
        GL_DEBUG( glEnable )( GL_ALPHA_TEST );      // GL_ENABLE_BIT
        // speed-up drawing of surfaces with alpha == 0.0f sections
        GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );   // GL_COLOR_BUFFER_BIT

        // reduce texture hashing by loading up each texture only once
        if ( gfx_error == render_fans_by_list( prlist->mesh, &( prlist->drf ) ) )
        {
            retval = gfx_error;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_ref( std::shared_ptr<Camera> pcam, const renderlist_t * prlist, const dolist_t * pdolist )
{
    /// @author BB
    /// @details Render all reflected objects

    int cnt;
    gfx_rv retval;

    ego_mesh_t * pmesh;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->size >= dolist_t::CAPACITY )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    pmesh = renderlist_t::getMesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__,  GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_POLYGON_BIT | GL_CURRENT_BIT );
    {
        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        // turn off the depth mask by default. Can cause glitches if used improperly.
        GL_DEBUG( glDepthMask )( GL_FALSE );      // GL_DEPTH_BUFFER_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );    // GL_ENABLE_BIT
        // surfaces must be closer to the camera to be drawn
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );     // GL_DEPTH_BUFFER_BIT

        for ( cnt = (( int )pdolist->size ) - 1; cnt >= 0; cnt-- )
        {
            if ( INVALID_PRT_REF == pdolist->lst[cnt].iprt && INVALID_CHR_REF != pdolist->lst[cnt].ichr )
            {
                CHR_REF ichr;
                Uint32 itile;

                // cull backward facing polygons
                // use couter-clockwise orientation to determine backfaces
                oglx_begin_culling( GL_BACK, MAP_REF_CULL );            // GL_ENABLE_BIT | GL_POLYGON_BIT

                // allow transparent objects
                GL_DEBUG( glEnable )( GL_BLEND );                 // GL_ENABLE_BIT
                // use the alpha channel to modulate the transparency
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT

                ichr  = pdolist->lst[cnt].ichr;
                itile = _gameObjects.get(ichr)->onwhichgrid;

                if ( ego_mesh_grid_is_valid( pmesh, itile ) && ( 0 != ego_mesh_test_fx( pmesh, itile, MAPFX_DRAWREF ) ) )
                {
                    GL_DEBUG( glColor4fv )( Ego::white_vec );          // GL_CURRENT_BIT

                    if ( gfx_error == render_one_mad_ref( pcam, ichr ) )
                    {
                        retval = gfx_error;
                    }
                }
            }
            else if ( INVALID_CHR_REF == pdolist->lst[cnt].ichr && INVALID_PRT_REF != pdolist->lst[cnt].iprt )
            {
                Uint32 itile;
                PRT_REF iprt;

                // draw draw front and back faces of polygons
                oglx_end_culling();                     // GL_ENABLE_BIT

                // render_one_prt_ref() actually sets its own blend function, but just to be safe
                // allow transparent objects
                GL_DEBUG( glEnable )( GL_BLEND );                    // GL_ENABLE_BIT
                // set the default particle blending
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );     // GL_COLOR_BUFFER_BIT

                iprt = pdolist->lst[cnt].iprt;
                itile = PrtList.lst[iprt].onwhichgrid;

                if ( ego_mesh_grid_is_valid( pmesh, itile ) && ( 0 != ego_mesh_test_fx( pmesh, itile, MAPFX_DRAWREF ) ) )
                {
                    GL_DEBUG( glColor4fv )( Ego::white_vec );          // GL_CURRENT_BIT

                    if ( gfx_error == render_one_prt_ref( iprt ) )
                    {
                        retval = gfx_error;
                    }
                }
            }
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_ref_chr( const renderlist_t * prlist )
{
    /// @brief   BB@> Render the shadow floors ( let everything show through )
    /// @author BB
    /// @details turn on the depth mask, so that no objects under the floor will show through
    ///               this assumes that the floor is not partially transparent...

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    {
        // set the depth of these tiles
        GL_DEBUG( glDepthMask )( GL_TRUE );                   // GL_DEPTH_BUFFER_BIT

        GL_DEBUG( glEnable )( GL_BLEND );                     // GL_ENABLE_BIT
        GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE );      // GL_COLOR_BUFFER_BIT

        // draw draw front and back faces of polygons
        oglx_end_culling();                // GL_ENABLE_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );                // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );                 // GL_DEPTH_BUFFER_BIT

        // reduce texture hashing by loading up each texture only once
        if ( gfx_error == render_fans_by_list( prlist->mesh, &( prlist->drf ) ) )
        {
            retval = gfx_error;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_drf_solid( const renderlist_t * prlist )
{
    /// @brief BB@> Render the shadow floors as normal solid floors

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    {
        // no transparency
        GL_DEBUG( glDisable )( GL_BLEND );                    // GL_ENABLE_BIT

        // draw draw front and back faces of polygons
        oglx_end_culling();                // GL_ENABLE_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );                // GL_ENABLE_BIT

        // store the surface depth
        GL_DEBUG( glDepthMask )( GL_TRUE );                   // GL_DEPTH_BUFFER_BIT

        // do not display the completely transparent portion
        // use alpha test to allow the thatched roof tiles to look like thatch
        GL_DEBUG( glEnable )( GL_ALPHA_TEST );                // GL_ENABLE_BIT
        // speed-up drawing of surfaces with alpha = 0.0f sections
        GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );          // GL_COLOR_BUFFER_BIT

        // reduce texture hashing by loading up each texture only once
        if ( gfx_error == render_fans_by_list( prlist->mesh, &( prlist->drf ) ) )
        {
            retval = gfx_error;
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh_render_shadows( const dolist_t * pdolist )
{
    /// @author BB
    /// @details Render the shadows

    size_t cnt;
    int    tnc;

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->size >= dolist_t::CAPACITY )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    if ( !gfx.shaon ) return gfx_success;

    // don't write into the depth buffer (disable glDepthMask for transparent objects)
    GL_DEBUG( glDepthMask )( GL_FALSE );

    // do not draw hidden surfaces
    GL_DEBUG( glEnable )( GL_DEPTH_TEST );

    GL_DEBUG( glEnable )( GL_BLEND );
    GL_DEBUG( glBlendFunc )( GL_ZERO, GL_ONE_MINUS_SRC_COLOR );

    // keep track of the number of shadows actually rendered
    tnc = 0;

    if ( gfx.shasprite )
    {
        // Bad shadows
        for ( cnt = 0; cnt < pdolist->size; cnt++ )
        {
            CHR_REF ichr = pdolist->lst[cnt].ichr;
            if ( !VALID_CHR_RANGE( ichr ) ) continue;

            if ( 0 == _gameObjects.get(ichr)->shadow_size ) continue;

            render_bad_shadow( ichr );
            tnc++;
        }
    }
    else
    {
        // Good shadows for me
        for ( cnt = 0; cnt < pdolist->size; cnt++ )
        {
            CHR_REF ichr = pdolist->lst[cnt].ichr;
            if ( !VALID_CHR_RANGE( ichr ) ) continue;

            if ( 0 == _gameObjects.get(ichr)->shadow_size ) continue;

            render_shadow( ichr );
            tnc++;
        }
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_mesh( std::shared_ptr<Camera> pcam, const renderlist_t * prlist, const dolist_t * pdolist )
{
    /// @author BB
    /// @details draw the mesh and any reflected objects

    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    // advance the animation of all animated tiles
    animate_all_tiles( prlist->mesh );

    PROFILE_BEGIN( render_scene_mesh_ndr );
    {
        // draw all tiles that do not reflect characters
        if ( gfx_error == render_scene_mesh_ndr( prlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_mesh_ndr );

    //--------------------------------
    // draw the reflective tiles and any reflected objects
    if ( gfx.refon )
    {
        PROFILE_BEGIN( render_scene_mesh_drf_back );
        {
            // blank out the background behind reflective tiles

            if ( gfx_error == render_scene_mesh_drf_back( prlist ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_drf_back );

        PROFILE_BEGIN( render_scene_mesh_ref );
        {
            // Render all reflected objects
            if ( gfx_error == render_scene_mesh_ref( pcam, prlist, pdolist ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_ref );

        PROFILE_BEGIN( render_scene_mesh_ref_chr );
        {
            // Render the shadow floors
            if ( gfx_error == render_scene_mesh_ref_chr( prlist ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_ref_chr );
    }
    else
    {
        PROFILE_BEGIN( render_scene_mesh_drf_solid );
        {
            // Render the shadow floors as normal solid floors
            if ( gfx_error == render_scene_mesh_drf_solid( prlist ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_drf_solid );
    }

#if defined(RENDER_HMAP) && defined(_DEBUG)

    // restart the mesh texture code
    mesh_texture_invalidate();

    // render the heighmap
    for ( cnt = 0; cnt < prlist->all.count; cnt++ )
    {
        render_hmap_fan( pmesh, prlist->all[cnt] );
    }

    // let the mesh texture code know that someone else is in control now
    mesh_texture_invalidate();

#endif

    PROFILE_BEGIN( render_scene_mesh_render_shadows );
    {
        // Render the shadows
        if ( gfx_error == render_scene_mesh_render_shadows( pdolist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_mesh_render_shadows );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_solid( std::shared_ptr<Camera> pcam, dolist_t * pdolist )
{
    /// @detaile BB@> Render all solid objects

    Uint32 cnt;
    gfx_rv retval;

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->size >= dolist_t::CAPACITY )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT )
    {
        // scan for solid objects
        for ( cnt = 0; cnt < pdolist->size; cnt++ )
        {
            // solid objects draw into the depth buffer for hidden surface removal
            GL_DEBUG( glDepthMask )( GL_TRUE );                     // GL_ENABLE_BIT

            // do not draw hidden surfaces
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );                  // GL_ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_LESS );                   // GL_DEPTH_BUFFER_BIT

            GL_DEBUG( glEnable )( GL_ALPHA_TEST );                 // GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );             // GL_COLOR_BUFFER_BIT

            if ( INVALID_PRT_REF == pdolist->lst[cnt].iprt && VALID_CHR_RANGE( pdolist->lst[cnt].ichr ) )
            {
                if ( gfx_error == render_one_mad_solid( pcam, pdolist->lst[cnt].ichr ) )
                {
                    retval = gfx_error;
                }
            }
            else if ( INVALID_CHR_REF == pdolist->lst[cnt].ichr && _VALID_PRT_RANGE( pdolist->lst[cnt].iprt ) )
            {
                // draw draw front and back faces of polygons
                oglx_end_culling();              // GL_ENABLE_BIT

                if ( gfx_error == render_one_prt_solid( pdolist->lst[cnt].iprt ) )
                {
                    retval = gfx_error;
                }
            }
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene_trans( std::shared_ptr<Camera> pcam, dolist_t * pdolist )
{
    /// @author BB
    /// @details draw transparent objects

    int cnt;
    gfx_rv retval;

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->size >= dolist_t::CAPACITY )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT )
    {
        //---- set the the transparency parameters

        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        GL_DEBUG( glDepthMask )( GL_FALSE );                   // GL_DEPTH_BUFFER_BIT

        // do not draw hidden surfaces
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );                // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_LEQUAL );                 // GL_DEPTH_BUFFER_BIT

        // Now render all transparent and light objects
        for ( cnt = (( int )pdolist->size ) - 1; cnt >= 0; cnt-- )
        {
            if ( INVALID_PRT_REF == pdolist->lst[cnt].iprt && INVALID_CHR_REF != pdolist->lst[cnt].ichr )
            {
                if ( gfx_error == render_one_mad_trans( pcam, pdolist->lst[cnt].ichr ) )
                {
                    retval = gfx_error;
                }
            }
            else if ( INVALID_CHR_REF == pdolist->lst[cnt].ichr && INVALID_PRT_REF != pdolist->lst[cnt].iprt )
            {
                // this is a particle
                if ( gfx_error == render_one_prt_trans( pdolist->lst[cnt].iprt ) )
                {
                    retval = gfx_error;
                }
            }
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_scene( std::shared_ptr<Camera> pcam, const int render_list_index, const int dolist_index )
{
    /// @author ZZ
    /// @details This function draws 3D objects

    gfx_rv retval;
    renderlist_t * prlist = NULL;
    dolist_t     * pdlist = NULL;

    if ( NULL == pcam ) pcam = pcam;
    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    prlist = renderlist_mgr_t::get_ptr( &_renderlist_mgr_data, render_list_index );
    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "could lock a renderlist" );
        return gfx_error;
    }

    pdlist = dolist_mgr_t::get_ptr( &_dolist_mgr_data, dolist_index );
    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "could lock a dolist" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    PROFILE_BEGIN( render_scene_init );
    {
        if ( gfx_error == render_scene_init( prlist, pdlist, &_dynalist, pcam ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_init );

    PROFILE_BEGIN( render_scene_mesh );
    {
        PROFILE_BEGIN( render_scene_mesh_dolist_sort );
        {
            // sort the dolist for reflected objects
            // reflected characters and objects are drawn in this pass
            if ( gfx_error == dolist_t::sort( pdlist, pcam, true ) )
            {
                retval = gfx_error;
            }
        }
        PROFILE_END( render_scene_mesh_dolist_sort );

        // do the render pass for the mesh
        if ( gfx_error == render_scene_mesh( pcam, prlist, pdlist ) )
        {
            retval = gfx_error;
        }

        time_render_scene_mesh_dolist_sort    = PROFILE_QUERY( render_scene_mesh_dolist_sort ) * TARGET_FPS;
        time_render_scene_mesh_ndr            = PROFILE_QUERY( render_scene_mesh_ndr ) * TARGET_FPS;
        time_render_scene_mesh_drf_back       = PROFILE_QUERY( render_scene_mesh_drf_back ) * TARGET_FPS;
        time_render_scene_mesh_ref            = PROFILE_QUERY( render_scene_mesh_ref ) * TARGET_FPS;
        time_render_scene_mesh_ref_chr        = PROFILE_QUERY( render_scene_mesh_ref_chr ) * TARGET_FPS;
        time_render_scene_mesh_drf_solid      = PROFILE_QUERY( render_scene_mesh_drf_solid ) * TARGET_FPS;
        time_render_scene_mesh_render_shadows = PROFILE_QUERY( render_scene_mesh_render_shadows ) * TARGET_FPS;
    }
    PROFILE_END( render_scene_mesh );

    PROFILE_BEGIN( render_scene_solid );
    {
        // sort the dolist for non-reflected objects
        if ( gfx_error == dolist_t::sort( pdlist, pcam, false ) )
        {
            retval = gfx_error;
        }

        // do the render pass for solid objects
        if ( gfx_error == render_scene_solid( pcam, pdlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_solid );

    PROFILE_BEGIN( render_scene_water );
    {
        // draw the water
        if ( gfx_error == render_water( prlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_water );

    PROFILE_BEGIN( render_scene_trans );
    {
        // do the render pass for transparent objects
        if ( gfx_error == render_scene_trans( pcam, pdlist ) )
        {
            retval = gfx_error;
        }
    }
    PROFILE_END( render_scene_trans );

#if defined(_DEBUG)
    render_all_prt_attachment();
#endif

#if defined(DRAW_LISTS)
    // draw some debugging lines
    line_list_draw_all( pcam );
    point_list_draw_all( pcam );
#endif

#if defined(DRAW_PRT_BBOX)
    render_all_prt_bbox();
#endif

    time_render_scene_init  = PROFILE_QUERY( render_scene_init ) * TARGET_FPS;
    time_render_scene_mesh  = PROFILE_QUERY( render_scene_mesh ) * TARGET_FPS;
    time_render_scene_solid = PROFILE_QUERY( render_scene_solid ) * TARGET_FPS;
    time_render_scene_water = PROFILE_QUERY( render_scene_water ) * TARGET_FPS;
    time_render_scene_trans = PROFILE_QUERY( render_scene_trans ) * TARGET_FPS;

    time_draw_scene = time_render_scene_init + time_render_scene_mesh + time_render_scene_solid + time_render_scene_water + time_render_scene_trans;

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_world_background( std::shared_ptr<Camera> pcam, const TX_REF texture )
{
    /// @author ZZ
    /// @details This function draws the large background
    GLvertex vtlist[4];
    int i;
    float z0, Qx, Qy;
    float light = 1.0f, intens = 1.0f, alpha = 1.0f;

    float xmag, Cx_0, Cx_1;
    float ymag, Cy_0, Cy_1;

    ego_mesh_info_t * pinfo;
    grid_mem_t     * pgmem;
    oglx_texture_t   * ptex;
    water_instance_layer_t * ilayer;

    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    pinfo = &( PMesh->info );
    pgmem = &( PMesh->gmem );

    // which layer
    ilayer = water.layer + 0;

    // the "official" camera height
    z0 = 1500;

    // clip the waterlayer uv offset
    ilayer->tx.x = ilayer->tx.x - ( float )FLOOR( ilayer->tx.x );
    ilayer->tx.y = ilayer->tx.y - ( float )FLOOR( ilayer->tx.y );

    // determine the constants for the x-coordinate
    xmag = water.backgroundrepeat / 4 / ( 1.0f + z0 * ilayer->dist.x ) / GRID_FSIZE;
    Cx_0 = xmag * ( 1.0f +  pcam->getPosition().z       * ilayer->dist.x );
    Cx_1 = -xmag * ( 1.0f + ( pcam->getPosition().z - z0 ) * ilayer->dist.x );

    // determine the constants for the y-coordinate
    ymag = water.backgroundrepeat / 4 / ( 1.0f + z0 * ilayer->dist.y ) / GRID_FSIZE;
    Cy_0 = ymag * ( 1.0f +  pcam->getPosition().z       * ilayer->dist.y );
    Cy_1 = -ymag * ( 1.0f + ( pcam->getPosition().z - z0 ) * ilayer->dist.y );

    // Figure out the coordinates of its corners
    Qx = -pgmem->edge_x;
    Qy = -pgmem->edge_y;
    vtlist[0].pos[XX] = Qx;
    vtlist[0].pos[YY] = Qy;
    vtlist[0].pos[ZZ] = pcam->getPosition().z - z0;
    vtlist[0].tex[SS] = Cx_0 * Qx + Cx_1 * pcam->getPosition().x + ilayer->tx.x;
    vtlist[0].tex[TT] = Cy_0 * Qy + Cy_1 * pcam->getPosition().y + ilayer->tx.y;

    Qx = 2 * pgmem->edge_x;
    Qy = -pgmem->edge_y;
    vtlist[1].pos[XX] = Qx;
    vtlist[1].pos[YY] = Qy;
    vtlist[1].pos[ZZ] = pcam->getPosition().z - z0;
    vtlist[1].tex[SS] = Cx_0 * Qx + Cx_1 * pcam->getPosition().x + ilayer->tx.x;
    vtlist[1].tex[TT] = Cy_0 * Qy + Cy_1 * pcam->getPosition().y + ilayer->tx.y;

    Qx = 2 * pgmem->edge_x;
    Qy = 2 * pgmem->edge_y;
    vtlist[2].pos[XX] = Qx;
    vtlist[2].pos[YY] = Qy;
    vtlist[2].pos[ZZ] = pcam->getPosition().z - z0;
    vtlist[2].tex[SS] = Cx_0 * Qx + Cx_1 * pcam->getPosition().x + ilayer->tx.x;
    vtlist[2].tex[TT] = Cy_0 * Qy + Cy_1 * pcam->getPosition().y + ilayer->tx.y;

    Qx = -pgmem->edge_x;
    Qy = 2 * pgmem->edge_y;
    vtlist[3].pos[XX] = Qx;
    vtlist[3].pos[YY] = Qy;
    vtlist[3].pos[ZZ] = pcam->getPosition().z - z0;
    vtlist[3].tex[SS] = Cx_0 * Qx + Cx_1 * pcam->getPosition().x + ilayer->tx.x;
    vtlist[3].tex[TT] = Cy_0 * Qy + Cy_1 * pcam->getPosition().y + ilayer->tx.y;

    light = water.light ? 1.0f : 0.0f;
    alpha = ilayer->alpha * INV_FF;

    if ( gfx.usefaredge )
    {
        float fcos;

        intens = light_a * ilayer->light_add;

        fcos = light_nrm[kZ];
        if ( fcos > 0.0f )
        {
            intens += fcos * fcos * light_d * ilayer->light_dir;
        }

        intens = CLIP( intens, 0.0f, 1.0f );
    }

    ptex = TxList_get_valid_ptr( texture );

    oglx_texture_Bind( ptex );

    ATTRIB_PUSH( __FUNCTION__, GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT );
    {
        // flat shading
        GL_DEBUG( glShadeModel )( GL_FLAT );      // GL_LIGHTING_BIT

        // don't write into the depth buffer (disable glDepthMask for transparent objects)
        GL_DEBUG( glDepthMask )( GL_FALSE );      // GL_DEPTH_BUFFER_BIT

        // essentially disable the depth test without calling glDisable( GL_DEPTH_TEST )
        GL_DEBUG( glEnable )( GL_DEPTH_TEST );      // GL_ENABLE_BIT
        GL_DEBUG( glDepthFunc )( GL_ALWAYS );     // GL_DEPTH_BUFFER_BIT

        // draw draw front and back faces of polygons
        oglx_end_culling();    // GL_ENABLE_BIT

        if ( alpha > 0.0f )
        {
            ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT );
            {
                GL_DEBUG( glColor4f )( intens, intens, intens, alpha );             // GL_CURRENT_BIT

                if ( alpha >= 1.0f )
                {
                    GL_DEBUG( glDisable )( GL_BLEND );                               // GL_ENABLE_BIT
                }
                else
                {
                    GL_DEBUG( glEnable )( GL_BLEND );                               // GL_ENABLE_BIT
                    GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );  // GL_COLOR_BUFFER_BIT
                }

                GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
                {
                    for ( i = 0; i < 4; i++ )
                    {
                        GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                        GL_DEBUG( glVertex3fv )( vtlist[i].pos );
                    }
                }
                GL_DEBUG_END();
            }
            ATTRIB_POP( __FUNCTION__ );
        }

        if ( light > 0.0f )
        {
            ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT );
            {
                GL_DEBUG( glDisable )( GL_BLEND );                           // GL_COLOR_BUFFER_BIT
                GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE );            // GL_COLOR_BUFFER_BIT

                GL_DEBUG( glColor4f )( light, light, light, 1.0f );         // GL_CURRENT_BIT

                GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
                {
                    for ( i = 0; i < 4; i++ )
                    {
                        GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                        GL_DEBUG( glVertex3fv )( vtlist[i].pos );
                    }
                }
                GL_DEBUG_END();
            }
            ATTRIB_POP( __FUNCTION__ );
        }
    }
    ATTRIB_POP( __FUNCTION__ );

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_world_overlay( std::shared_ptr<Camera> pcam, const TX_REF texture )
{
    /// @author ZZ
    /// @details This function draws the large foreground

    float alpha, ftmp;
    fvec3_t   vforw_wind, vforw_cam;
    TURN_T default_turn;

    oglx_texture_t           * ptex;

    water_instance_layer_t * ilayer = water.layer + 1;

    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    vforw_wind.x = ilayer->tx_add.x;
    vforw_wind.y = ilayer->tx_add.y;
    vforw_wind.z = 0;
	vforw_wind.normalize();

    mat_getCamForward(pcam->getView(), vforw_cam);
	vforw_cam.normalize();

    // make the texture begin to disappear if you are not looking straight down
    ftmp = vforw_wind.dot(vforw_cam);

    alpha = ( 1.0f - ftmp * ftmp ) * ( ilayer->alpha * INV_FF );

    if ( alpha != 0.0f )
    {
        GLvertex vtlist[4];
        int i;
        float size;
        float sinsize, cossize;
        float x, y, z;
        float loc_foregroundrepeat;

        // Figure out the screen coordinates of its corners
        x = sdl_scr.x << 6;
        y = sdl_scr.y << 6;
        z = 0;
        size = x + y + 1;
        default_turn = ( 3 * 2047 ) & TRIG_TABLE_MASK;
        sinsize = turntosin[default_turn] * size;
        cossize = turntocos[default_turn] * size;
        loc_foregroundrepeat = water.foregroundrepeat * std::min( x / sdl_scr.x, y / sdl_scr.x );

        vtlist[0].pos[XX] = x + cossize;
        vtlist[0].pos[YY] = y - sinsize;
        vtlist[0].pos[ZZ] = z;
        vtlist[0].tex[SS] = ilayer->tx.x;
        vtlist[0].tex[TT] = ilayer->tx.y;

        vtlist[1].pos[XX] = x + sinsize;
        vtlist[1].pos[YY] = y + cossize;
        vtlist[1].pos[ZZ] = z;
        vtlist[1].tex[SS] = ilayer->tx.x + loc_foregroundrepeat;
        vtlist[1].tex[TT] = ilayer->tx.y;

        vtlist[2].pos[XX] = x - cossize;
        vtlist[2].pos[YY] = y + sinsize;
        vtlist[2].pos[ZZ] = z;
        vtlist[2].tex[SS] = ilayer->tx.x + loc_foregroundrepeat;
        vtlist[2].tex[TT] = ilayer->tx.y + loc_foregroundrepeat;

        vtlist[3].pos[XX] = x - sinsize;
        vtlist[3].pos[YY] = y - cossize;
        vtlist[3].pos[ZZ] = z;
        vtlist[3].tex[SS] = ilayer->tx.x;
        vtlist[3].tex[TT] = ilayer->tx.y + loc_foregroundrepeat;

        ptex = TxList_get_valid_ptr( texture );

        ATTRIB_PUSH( __FUNCTION__, GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_DEPTH_BUFFER_BIT | GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_HINT_BIT );
        {
            // make sure that the texture is as smooth as possible
            GL_DEBUG( glHint )( GL_POLYGON_SMOOTH_HINT, GL_NICEST );          // GL_HINT_BIT

            // flat shading
            GL_DEBUG( glShadeModel )( GL_FLAT );                             // GL_LIGHTING_BIT

            // don't write into the depth buffer (disable glDepthMask for transparent objects)
            GL_DEBUG( glDepthMask )( GL_FALSE );                             // GL_DEPTH_BUFFER_BIT

            // essentially disable the depth test without calling glDisable( GL_DEPTH_TEST )
            GL_DEBUG( glEnable )( GL_DEPTH_TEST );                           // GL_ENABLE_BIT
            GL_DEBUG( glDepthFunc )( GL_ALWAYS );                            // GL_DEPTH_BUFFER_BIT

            // draw draw front and back faces of polygons
            oglx_end_culling();                           // GL_ENABLE_BIT

            // do not display the completely transparent portion
            GL_DEBUG( glEnable )( GL_ALPHA_TEST );                            // GL_ENABLE_BIT
            GL_DEBUG( glAlphaFunc )( GL_GREATER, 0.0f );                      // GL_COLOR_BUFFER_BIT

            // make the texture a filter
            GL_DEBUG( glEnable )( GL_BLEND );                                 // GL_ENABLE_BIT
            GL_DEBUG( glBlendFunc )( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );  // GL_COLOR_BUFFER_BIT

            oglx_texture_Bind( ptex );

            GL_DEBUG( glColor4f )( 1.0f, 1.0f, 1.0f, 1.0f - ABS( alpha ) );
            GL_DEBUG( glBegin )( GL_TRIANGLE_FAN );
            for ( i = 0; i < 4; i++ )
            {
                GL_DEBUG( glTexCoord2fv )( vtlist[i].tex );
                GL_DEBUG( glVertex3fv )( vtlist[i].pos );
            }
            GL_DEBUG_END();
        }
        ATTRIB_POP( __FUNCTION__ );
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv render_water( renderlist_t * prlist )
{
    /// @author ZZ
    /// @details This function draws all of the water fans
    gfx_rv retval;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    // assume the best
    retval = gfx_success;

    // restart the mesh texture code
    mesh_texture_invalidate();

    // Bottom layer first
    if ( gfx.draw_water_1 )
    {
        for ( size_t cnt = 0; cnt < prlist->wat.count; cnt++ )
        {
            if ( gfx_error == render_water_fan( prlist->mesh, prlist->wat.lst[cnt].index, 1 ) )
            {
                retval = gfx_error;
            }
        }
    }

    // Top layer second
    if ( gfx.draw_water_0 )
    {
        for ( size_t cnt = 0; cnt < prlist->wat.count; cnt++ )
        {
            if ( gfx_error == render_water_fan( prlist->mesh, prlist->wat.lst[cnt].index, 0 ) )
            {
                retval = gfx_error;
            }
        }
    }

    // let the mesh texture code know that someone else is in control now
    mesh_texture_invalidate();

    return retval;
}

//--------------------------------------------------------------------------------------------
// gfx_config_t FUNCTIONS
//--------------------------------------------------------------------------------------------
bool gfx_config_download_from_egoboo_config( gfx_config_t * pgfx, egoboo_config_t * pcfg )
{
    // call gfx_config_init(), even if the config data is invalid
    if ( !gfx_config_init( pgfx ) ) return false;

    // if there is no config data, do not proceed
    if ( NULL == pcfg ) return false;

    pgfx->antialiasing = pcfg->multisamples > 0;

    pgfx->refon        = pcfg->reflect_allowed;
    pgfx->reffadeor    = pcfg->reflect_fade ? 0 : 255;

    pgfx->shaon        = pcfg->shadow_allowed;
    pgfx->shasprite    = pcfg->shadow_sprite;

    pgfx->shading      = pcfg->gouraud_req ? GL_SMOOTH : GL_FLAT;
    pgfx->dither       = pcfg->use_dither;
    pgfx->perspective  = pcfg->use_perspective;
    pgfx->phongon      = pcfg->use_phong;

    pgfx->draw_background = pcfg->background_allowed && water.background_req;
    pgfx->draw_overlay    = pcfg->overlay_allowed && water.overlay_req;

    pgfx->dynalist_max = CLIP( pcfg->dyna_count_req, 0, TOTAL_MAX_DYNA );

    pgfx->draw_water_0 = !pgfx->draw_overlay && ( water.layer_count > 0 );
    pgfx->clearson     = !pgfx->draw_background;
    pgfx->draw_water_1 = !pgfx->draw_background && ( water.layer_count > 1 );

    gfx_system_set_virtual_screen( pgfx );

    return true;
}

//--------------------------------------------------------------------------------------------
bool gfx_config_init( gfx_config_t * pgfx )
{
    if ( NULL == pgfx ) return false;

    pgfx->shading          = GL_SMOOTH;
    pgfx->refon            = true;
    pgfx->reffadeor        = 0;
    pgfx->antialiasing     = false;
    pgfx->dither           = false;
    pgfx->perspective      = false;
    pgfx->phongon          = true;
    pgfx->shaon            = true;
    pgfx->shasprite        = true;

    pgfx->clearson         = true;   // Do we clear every time?
    pgfx->draw_background  = false;   // Do we draw the background image?
    pgfx->draw_overlay     = false;   // Draw overlay?
    pgfx->draw_water_0     = true;   // Do we draw water layer 1 (TX_WATER_LOW)
    pgfx->draw_water_1     = true;   // Do we draw water layer 2 (TX_WATER_TOP)

    pgfx->dynalist_max    = 8;

    //gfx_calc_rotmesh();

    return true;
}

//--------------------------------------------------------------------------------------------
// oglx_texture_parameters_t FUNCTIONS
//--------------------------------------------------------------------------------------------
bool oglx_texture_parameters_download_gfx( oglx_texture_parameters_t * ptex, egoboo_config_t * pcfg )
{
    /// @author BB
    /// @details synch the texture parameters with the video mode

    if ( NULL == ptex || NULL == pcfg ) return false;

    if ( ogl_caps.maxAnisotropy <= 1.0f )
    {
        ptex->userAnisotropy = 0.0f;
		ptex->texturefilter = std::min<tx_filter_t>(pcfg->texturefilter_req, TX_TRILINEAR_2);
    }
    else
    {
		ptex->texturefilter = std::min<tx_filter_t>(pcfg->texturefilter_req, TX_FILTER_COUNT);
        ptex->userAnisotropy = ogl_caps.maxAnisotropy * std::max( 0, ( int )ptex->texturefilter - ( int )TX_TRILINEAR_2 );
    }

    return true;
}

//--------------------------------------------------------------------------------------------
// gfx_error FUNCTIONS
//--------------------------------------------------------------------------------------------
egolib_rv gfx_error_add( const char * file, const char * function, int line, int id, const char * sz )
{
    gfx_error_state_t * pstate;

    // too many errors?
    if ( gfx_error_stack.count >= GFX_ERROR_MAX ) return rv_fail;

    // grab an error state
    pstate = gfx_error_stack.lst + gfx_error_stack.count;
    gfx_error_stack.count++;

    // where is the error
    strncpy( pstate->file,     file,     SDL_arraysize( pstate->file ) );
    strncpy( pstate->function, function, SDL_arraysize( pstate->function ) );
    pstate->line = line;

    // what is the error
    pstate->type     = id;
    strncpy( pstate->string, sz, SDL_arraysize( pstate->string ) );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
gfx_error_state_t * gfx_error_pop()
{
    gfx_error_state_t * retval;

    if ( 0 == gfx_error_stack.count || gfx_error_stack.count >= GFX_ERROR_MAX ) return NULL;

    gfx_error_stack.count--;
    retval = gfx_error_stack.lst + gfx_error_stack.count;

    return retval;
}

//--------------------------------------------------------------------------------------------
void gfx_error_clear()
{
    gfx_error_stack.count = 0;
}

//--------------------------------------------------------------------------------------------
// grid_lighting FUNCTIONS
//--------------------------------------------------------------------------------------------
float grid_lighting_test( ego_mesh_t * pmesh, GLXvector3f pos, float * low_diff, float * hgh_diff )
{
    int ix, iy, cnt;
    Uint32 fan[4];
    float u, v;

    const lighting_cache_t * cache_list[4];
    ego_grid_info_t  * pgrid;

    if ( NULL == pmesh ) pmesh = PMesh;
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh" );
        return 0.0f;
    }

    ix = FLOOR( pos[XX] / GRID_FSIZE );
    iy = FLOOR( pos[YY] / GRID_FSIZE );

    fan[0] = ego_mesh_get_tile_int( pmesh, ix,     iy );
    fan[1] = ego_mesh_get_tile_int( pmesh, ix + 1, iy );
    fan[2] = ego_mesh_get_tile_int( pmesh, ix,     iy + 1 );
    fan[3] = ego_mesh_get_tile_int( pmesh, ix + 1, iy + 1 );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        cache_list[cnt] = NULL;

        pgrid = ego_mesh_get_pgrid( pmesh, fan[cnt] );
        if ( NULL == pgrid )
        {
            cache_list[cnt] = NULL;
        }
        else
        {
            cache_list[cnt] = &( pgrid->cache );
        }
    }

    u = pos[XX] / GRID_FSIZE - ix;
    v = pos[YY] / GRID_FSIZE - iy;

    return lighting_cache_test( cache_list, u, v, low_diff, hgh_diff );
}

//--------------------------------------------------------------------------------------------
bool grid_lighting_interpolate( const ego_mesh_t * pmesh, lighting_cache_t * dst, const fvec2_t& pos )
{
    int ix, iy, cnt;
    Uint32 fan[4];
    float u, v;
    fvec2_t tpos;

    ego_grid_info_t  * pgrid;
    const lighting_cache_t * cache_list[4];

    if ( NULL == pmesh ) pmesh = PMesh;
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid mesh" );
        return false;
    }

    if ( NULL == dst )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "no valid lighting cache" );
        return false;
    }

    // calculate the "tile position"
    fvec2_scale( tpos.v, pos.v, 1.0f / GRID_FSIZE );

    // grab this tile's coordinates
    ix = FLOOR( tpos.x );
    iy = FLOOR( tpos.y );

    // find the tile id for the surrounding tiles
    fan[0] = ego_mesh_get_tile_int( pmesh, ix,     iy );
    fan[1] = ego_mesh_get_tile_int( pmesh, ix + 1, iy );
    fan[2] = ego_mesh_get_tile_int( pmesh, ix,     iy + 1 );
    fan[3] = ego_mesh_get_tile_int( pmesh, ix + 1, iy + 1 );

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        pgrid = ego_mesh_get_pgrid( pmesh, fan[cnt] );

        if ( NULL == pgrid )
        {
            cache_list[cnt] = NULL;
        }
        else
        {
            cache_list[cnt] = &( pgrid->cache );
        }
    }

    // grab the coordinates relative to the parent tile
    u = tpos.x - ix;
    v = tpos.y - iy;

    return lighting_cache_interpolate( dst, cache_list, u, v );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void gfx_update_fps_clock()
{
    /// @author ZZ
    /// @details This function updates the graphics timers

    int gfx_clock_diff;

    // make sure at least one tick has passed
    if ( !egolib_timer__throttle( &gfx_update_timer, 1 ) ) return;

    // calculate the time since the from the last update
    if ( !single_frame_mode )
    {
        // update the graphics clock the normal way
        egolib_throttle_update( &gfx_throttle );
    }
    else
    {
        // do a single-frame update
        egolib_throttle_update_diff( &gfx_throttle, TICKS_PER_SEC / ( float )cfg.framelimit );
    }

    // measure the time change
    gfx_clock_diff = gfx_throttle.time_now - gfx_throttle.time_lst;

    // update the game fps
    if (process_t::running(PROC_PBASE(GProc)))
    {
        game_fps_clock += gfx_clock_diff;
    }

    // update the menu fps
    if (process_t::running(PROC_PBASE( MProc)))
    {
        menu_fps_clock += gfx_clock_diff;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_update_fps()
{
    // at fold = 0.60f, it will take approximately 9 updates for the
    // weight of the first value to be reduced to 1%
    const float fold = STABILIZED_KEEP;
    const float fnew = STABILIZED_COVER;

    // update the clock
    gfx_update_fps_clock();

    // update the game fps
    if (process_t::running(PROC_PBASE(GProc)))
    {
        if ( game_fps_loops > 0 && game_fps_clock > 0 )
        {
            stabilized_game_fps_sum    = fold * stabilized_game_fps_sum    + fnew * ( float ) game_fps_loops / (( float ) game_fps_clock / TICKS_PER_SEC );
            stabilized_game_fps_weight = fold * stabilized_game_fps_weight + fnew;

            // Don't allow the counters to overflow. 0x15555555 is 1/3 of the maximum Sint32 value
            if ( game_fps_loops > 0x15555555 || game_fps_clock > 0x15555555 )
            {
                game_fps_loops = 0;
                game_fps_clock = 0;
            }
        };

        if ( stabilized_game_fps_weight > 0.5f )
        {
            stabilized_game_fps = stabilized_game_fps_sum / stabilized_game_fps_weight;
        }
    }

    // update the menu fps
    if (process_t::running(PROC_PBASE(MProc)))
    {
        if ( menu_fps_loops > 0 && menu_fps_clock > 0 )
        {
            stabilized_menu_fps_sum    = fold * stabilized_menu_fps_sum    + fnew * ( float ) menu_fps_loops / (( float ) menu_fps_clock / TICKS_PER_SEC );
            stabilized_menu_fps_weight = fold * stabilized_menu_fps_weight + fnew;

            // Don't allow the counters to overflow. 0x15555555 is 1/3 of the maximum Sint32 value
            if ( menu_fps_loops > 0x15555555 || menu_fps_clock > 0x15555555 )
            {
                menu_fps_loops = 0;
                menu_fps_clock = 0;
            }
        };

        if ( stabilized_menu_fps_weight > 0.5f )
        {
            stabilized_menu_fps = stabilized_menu_fps_sum / stabilized_menu_fps_weight;
        }
    }

    // choose the correct fps to display
    if ( process_t::running( PROC_PBASE( GProc ) ) )
    {
        stabilized_fps = stabilized_game_fps;
    }
    else if ( process_t::running( PROC_PBASE( MProc ) ) )
    {
        stabilized_fps = stabilized_menu_fps;
    }
}

//--------------------------------------------------------------------------------------------
// obj_registry_entity_t IMPLEMENTATION
//--------------------------------------------------------------------------------------------

dolist_t::element_t *dolist_t::element_t::init( dolist_t::element_t * ptr )
{
    if ( NULL == ptr ) return NULL;

    BLANK_STRUCT_PTR( ptr )

    ptr->ichr = INVALID_CHR_REF;
    ptr->iprt = INVALID_PRT_REF;

    return ptr;
}

//--------------------------------------------------------------------------------------------
int dolist_t::element_t::cmp( const void * pleft, const void * pright )
{
	dolist_t::element_t * dleft = (dolist_t::element_t *)pleft;
	dolist_t::element_t * dright = (dolist_t::element_t *)pright;

    int   rv;
    float diff;

    diff = dleft->dist - dright->dist;

    if ( diff < 0.0f )
    {
        rv = -1;
    }
    else if ( diff > 0.0f )
    {
        rv = 1;
    }
    else
    {
        rv = 0;
    }

    return rv;
}

//--------------------------------------------------------------------------------------------
// ASSET INITIALIZATION
//--------------------------------------------------------------------------------------------

void gfx_init_bar_data()
{
    Uint8 cnt;

    // Initialize the life and mana bars
    for ( cnt = 0; cnt < NUMBAR; cnt++ )
    {
        tabrect[cnt]._left = 0;
        tabrect[cnt]._right = TABX;
        tabrect[cnt]._top = cnt * BARY;
        tabrect[cnt]._bottom = ( cnt + 1 ) * BARY;

        barrect[cnt]._left = TABX;
        barrect[cnt]._right = BARX;  // This is reset whenever a bar is drawn
        barrect[cnt]._top = tabrect[cnt]._top;
        barrect[cnt]._bottom = tabrect[cnt]._bottom;

    }
}

//--------------------------------------------------------------------------------------------
void gfx_init_blip_data()
{
    int cnt;

    // Set up the rectangles
    for ( cnt = 0; cnt < COLOR_MAX; cnt++ )
    {
        bliprect[cnt]._left   = cnt * BLIPSIZE;
        bliprect[cnt]._right  = cnt * BLIPSIZE + BLIPSIZE;
        bliprect[cnt]._top    = 0;
        bliprect[cnt]._bottom = BLIPSIZE;
    }

    youarehereon = false;
    blip_count   = 0;
}

//--------------------------------------------------------------------------------------------
void gfx_init_map_data()
{
    /// @author ZZ
    /// @details This function releases all the map images

    // Set up the rectangles
    maprect._left   = 0;
    maprect._right  = MAPSIZE;
    maprect._top    = 0;
    maprect._bottom = MAPSIZE;

    mapvalid = false;
    mapon    = false;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_bars()
{
    /// @author ZZ
    /// @details This function loads the status bar bitmap

    const char * pname = "";
    TX_REF load_rv = INVALID_TX_REF;
    gfx_rv retval  = gfx_success;

    pname = "mp_data/bars";
    load_rv = TxList_load_one_vfs( pname, ( TX_REF )TX_BARS, TRANSCOLOR );
    if ( !VALID_TX_RANGE( load_rv ) )
    {
        log_warning( "%s - Cannot load file! (\"%s\")\n", __FUNCTION__, pname );
        retval = gfx_fail;
    }

    pname = "mp_data/xpbar";
    load_rv = TxList_load_one_vfs( pname, ( TX_REF )TX_XP_BAR, TRANSCOLOR );
    if ( !VALID_TX_RANGE( load_rv ) )
    {
        log_warning( "%s - Cannot load file! (\"%s\")\n", __FUNCTION__, pname );
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_map()
{
    /// @author ZZ
    /// @details This function loads the map bitmap

    const char* szMap = "mp_data/plan";
    TX_REF load_rv = INVALID_TX_REF;
    gfx_rv retval  = gfx_success;

    // Turn it all off
    mapon        = false;
    youarehereon = false;
    blip_count   = 0;

    // Load the images
    load_rv = TxList_load_one_vfs( szMap, ( TX_REF )TX_MAP, INVALID_KEY );
    if ( !VALID_TX_RANGE( load_rv ) )
    {
        log_debug( "%s - Cannot load file! (\"%s\")\n", __FUNCTION__, szMap );
        retval   = gfx_fail;
        mapvalid = false;
    }
    else
    {
        retval   = gfx_success;
        mapvalid = true;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_blips()
{
    /// @author ZZ
    /// @details This function loads the blip bitmaps

    const char * pname = "mp_data/blip";
    TX_REF load_rv = INVALID_TX_REF;
    gfx_rv retval  = gfx_success;
    
    load_rv = TxList_load_one_vfs( pname, ( TX_REF )TX_BLIP, INVALID_KEY );
    if ( !VALID_TX_RANGE( load_rv ) )
    {
        log_warning( "%s - Blip bitmap not loaded! (\"%s\")\n", __FUNCTION__, pname );
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_load_icons()
{
    const char * pname = "mp_data/nullicon";
    TX_REF load_rv = INVALID_TX_REF;
    gfx_rv retval  = gfx_success;

    load_rv = TxList_load_one_vfs( pname, ( TX_REF )TX_ICON_NULL, INVALID_KEY );
    if ( !VALID_TX_RANGE( load_rv ) )
    {
        log_warning( "%s - cannot load \"empty hand\" icon! (\"%s\")\n", __FUNCTION__, pname );
        retval = gfx_fail;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// MODE CONTROL
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
void gfx_request_clear_screen()
{
    gfx_page_clear_requested = true;
}

//--------------------------------------------------------------------------------------------
void gfx_do_clear_screen()
{
    bool game_needs_clear, menu_needs_clear;

    if ( !gfx_page_clear_requested ) return;

    // clear the depth buffer if anything was drawn
    GL_DEBUG( glDepthMask )( GL_TRUE );
    GL_DEBUG( glClear )( GL_DEPTH_BUFFER_BIT );

    // does the game need a clear?
    game_needs_clear = false;
    if (process_t::running(PROC_PBASE(GProc)) && PROC_PBASE(GProc)->state > process_t::State::Begin)
    {
        game_needs_clear = gfx.clearson;
    }

    // does the menu need a clear?
    menu_needs_clear = false;
    if (process_t::running(PROC_PBASE(MProc)) && PROC_PBASE(MProc)->state > process_t::State::Begin)
    {
        menu_needs_clear = mnu_draw_background;
    }

    // if anything needs a clear, do it
    if ( game_needs_clear || menu_needs_clear )
    {
        GL_DEBUG( glClear )( GL_COLOR_BUFFER_BIT );

        gfx_page_clear_requested = false;

        gfx_clear_loops++;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_do_flip_pages()
{
    bool try_flip;

    try_flip = false;
    if (process_t::running(PROC_PBASE(GProc)) && PROC_PBASE(GProc)->state > process_t::State::Begin )
    {
        try_flip = gfx_page_flip_requested;
    }
    else if (process_t::running(PROC_PBASE(MProc)) && PROC_PBASE(MProc)->state > process_t::State::Begin )
    {
        try_flip = gfx_page_flip_requested;
    }

    if ( try_flip )
    {
        gfx_page_flip_requested = false;
        _flip_pages();

        gfx_page_clear_requested = true;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_request_flip_pages()
{
    gfx_page_flip_requested = true;
}

//--------------------------------------------------------------------------------------------
bool gfx_flip_pages_requested()
{
    return gfx_page_flip_requested;
}

//--------------------------------------------------------------------------------------------
void _flip_pages()
{
    //GL_DEBUG( glFlush )();

    // draw the console on top of everything
    egolib_console_draw_all();

    SDL_GL_SwapBuffers();

    if ( process_t::running( PROC_PBASE( MProc ) ) )
    {
        menu_fps_loops++;
        menu_frame_all++;
    }

    if ( process_t::running( PROC_PBASE( GProc ) ) )
    {
        game_fps_loops++;
        game_frame_all++;
    }

    // update the frames per second
    gfx_update_fps();

    if ( screenshot_requested )
    {
        screenshot_requested = false;

        // take the screenshot NOW, since we have just updated the screen buffer
        if ( !dump_screenshot() )
        {
            DisplayMsg_printf( "Error writing screenshot!" );    // send a failure message to the screen
            log_warning( "Error writing screenshot\n" );    // Log the error in log.txt
        }
    }
}

//--------------------------------------------------------------------------------------------
// LIGHTING FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv light_fans_throttle_update( ego_mesh_t * pmesh, ego_tile_info_t * ptile, int fan, float threshold )
{
    grid_mem_t * pgmem = NULL;
    bool       retval = false;

    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "no valid mesh" );
        return gfx_error;
    }
    pgmem = &( pmesh->gmem );

    if ( NULL == ptile )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "no valid tile" );
        return gfx_error;
    }

#if defined(CLIP_LIGHT_FANS) && !defined(CLIP_ALL_LIGHT_FANS)

    // visible fans based on the update "need"
    retval = ego_mesh_test_corners( pmesh, ptile, threshold );

    // update every 4 fans even if there is no need
    if ( !retval )
    {
        int ix, iy;

        // use a kind of checkerboard pattern
        ix = fan % pgmem->grids_x;
        iy = fan / pgmem->grids_x;
        if ( 0 != ((( ix ^ iy ) + game_frame_all ) & 0x03 ) )
        {
            retval = true;
        }
    }

#else
    retval = true;
#endif

    return retval ? gfx_success : gfx_fail;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans_update_lcache( renderlist_t * prlist )
{
    const int frame_skip = 1 << 2;
#if defined(CLIP_ALL_LIGHT_FANS)
    const int frame_mask = frame_skip - 1;
#endif

    int    entry;
    float  local_mesh_lighting_keep;

    /// @note we are measuring the change in the intensity at the corner of a tile (the "delta") as
    /// a fraction of the current intensity. This is because your eye is much more sensitive to
    /// intensity differences when the intensity is low.
    ///
    /// @note it is normally assumed that 64 colors of gray can make a smoothly colored black and white picture
    /// which means that the threshold could be set as low as 1/64 = 0.015625.
    const float delta_threshold = 0.05f;

    ego_mesh_t      * pmesh;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    pmesh = renderlist_t::getMesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

#if defined(CLIP_ALL_LIGHT_FANS)
    // update all visible fans once every 4 frames
    if ( 0 != ( game_frame_all & frame_mask ) ) return gfx_success;
#endif

#if !defined(CLIP_LIGHT_FANS)
    // update only every frame
    local_mesh_lighting_keep = 0.9f;
#else
    // update only every 4 frames
    local_mesh_lighting_keep = POW( 0.9f, frame_skip );
#endif

    // cache the grid lighting
    for ( entry = 0; entry < prlist->all.count; entry++ )
    {
        bool reflective;
        int fan;
        float delta;
        ego_tile_info_t * ptile;
        ego_grid_info_t * pgrid;

        // which tile?
        fan = prlist->all.lst[entry].index;

        // grab a pointer to the tile
        ptile = ego_mesh_get_ptile( pmesh, fan );
        if ( NULL == ptile ) continue;

        // Test to see whether the lcache was already updated
        // - ptile->lcache_frame < 0 means that the cache value is invalid.
        // - ptile->lcache_frame is updated inside ego_mesh_light_corners()
#if defined(CLIP_LIGHT_FANS)
        // clip the updated on each individual tile
        if ( ptile->lcache_frame >= 0 && ( Uint32 )ptile->lcache_frame + frame_skip >= game_frame_all )
#else
        // let the function clip all tile updates
        if ( ptile->lcache_frame >= 0 && ( Uint32 )ptile->lcache_frame >= game_frame_all )
#endif
        {
            continue;
        }

        // did someone else request an update?
        if ( !ptile->request_lcache_update )
        {
            // is someone else did not request an update, do we need an one?
            gfx_rv light_fans_rv = light_fans_throttle_update( pmesh, ptile, fan, delta_threshold );
            ptile->request_lcache_update = ( gfx_success == light_fans_rv );
        }

        // if there's no need for an update, go to the next tile
        if ( !ptile->request_lcache_update ) continue;

        // is the tile reflective?
        pgrid = ego_mesh_get_pgrid( pmesh, fan );
        reflective = ( 0 != ego_grid_info_test_all_fx( pgrid, MAPFX_DRAWREF ) );

        // light the corners of this tile
        delta = ego_mesh_light_corners( prlist->mesh, ptile, reflective, local_mesh_lighting_keep );

#if defined(CLIP_LIGHT_FANS)
        // use the actual maximum change in the intensity at a tile corner to
        // signal whether we need to calculate the next stage
        ptile->request_clst_update = ( delta > delta_threshold );
#else
        // make sure that ego_mesh_light_corners() did not return an "error value"
        ptile->request_clst_update = ( delta > 0.0f );
#endif
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans_update_clst( renderlist_t * prlist )
{
    /// @author BB
    /// @details update the tile's color list, if needed

    gfx_rv retval;
    int numvertices;
    int ivrt, vertex;
    float light;
    Uint32 fan;

    ego_tile_info_t   * ptile = NULL;
    ego_mesh_t         * pmesh = NULL;
    tile_mem_t        * ptmem = NULL;
    tile_definition_t * pdef  = NULL;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    pmesh = renderlist_t::getMesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

    // alias the tile memory
    ptmem = &( pmesh->tmem );

    // assume the best
    retval = gfx_success;

    // use the grid to light the tiles
    for ( size_t entry = 0; entry < prlist->all.count; entry++ )
    {
        fan = prlist->all.lst[entry].index;
        if ( INVALID_TILE == fan ) continue;

        // valid tile?
        ptile = ego_mesh_get_ptile( pmesh, fan );
        if ( NULL == ptile )
        {
            retval = gfx_fail;
            continue;
        }

        // do nothing if this tile has not been marked as needing an update
        if ( !ptile->request_clst_update )
        {
            continue;
        }

        // do nothing if the color list has already been computed this update
        if ( ptile->clst_frame  >= 0 && ( Uint32 )ptile->clst_frame >= game_frame_all )
        {
            continue;
        }

        pdef = TILE_DICT_PTR( tile_dict, ptile->type );
        if ( NULL != pdef )
        {
            numvertices = pdef->numvertices;
        }
        else
        {
            numvertices = 4;
        }

        // copy the 1st 4 vertices
        for ( ivrt = 0, vertex = ptile->vrtstart; ivrt < 4; ivrt++, vertex++ )
        {
            GLXvector3f * pcol = ptmem->clst + vertex;

            light = ptile->lcache[ivrt];

            ( *pcol )[RR] = ( *pcol )[GG] = ( *pcol )[BB] = INV_FF * CLIP( light, 0.0f, 255.0f );
        };

        for ( /* nothing */ ; ivrt < numvertices; ivrt++, vertex++ )
        {
            bool was_calculated;
            GLXvector3f * pcol, * ppos;

            pcol = ptmem->clst + vertex;
            ppos = ptmem->plst + vertex;

            light = 0;
            was_calculated = ego_mesh_interpolate_vertex( ptmem, ptile, *ppos, &light );
            if ( !was_calculated ) continue;

            ( *pcol )[RR] = ( *pcol )[GG] = ( *pcol )[BB] = INV_FF * CLIP( light, 0.0f, 255.0f );
        };

        // clear out the deltas
        BLANK_ARY( ptile->d1_cache );
        BLANK_ARY( ptile->d2_cache );

        // untag this tile
        ptile->request_clst_update = false;
        ptile->clst_frame          = game_frame_all;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv light_fans( renderlist_t * prlist )
{
    if ( gfx_error == light_fans_update_lcache( prlist ) )
    {
        return gfx_error;
    }

    if ( gfx_error == light_fans_update_clst( prlist ) )
    {
        return gfx_error;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
float get_ambient_level()
{
    /// @author BB
    /// @details get the actual global ambient level

    float glob_amb, min_amb;

    glob_amb = 0.0f;
    min_amb  = 0.0f;
    if ( gfx.usefaredge )
    {
        // for outside modules, max light_a means bright sunlight
        // this should be handled with directional lighting, so ambient light is 0
        //glob_amb = light_a * 255.0f;
        glob_amb = 0;
    }
    else
    {
        // for inside modules, max light_a means dingy dungeon lighting
        glob_amb = light_a * 32.0f;
    }

    // determine the minimum ambient, based on darkvision
    min_amb = INVISIBLE / 4;
    if ( local_stats.seedark_mag != 1.0f )
    {
        // start with the global light
        min_amb  = glob_amb;

        // give a iny boost in the case of no light_a
        if ( local_stats.seedark_mag > 0.0f ) min_amb += 1.0f;

        // light_a can be quite dark, so we need a large magnification
        min_amb *= std::pow(local_stats.seedark_mag, 5);
    }

    return std::max( glob_amb, min_amb );
}

//--------------------------------------------------------------------------------------------
bool sum_global_lighting( lighting_vector_t lighting )
{
    /// @author BB
    /// @details do ambient lighting. if the module is inside, the ambient lighting
    /// is reduced by up to a factor of 8. It is still kept just high enough
    /// so that ordnary objects will not be made invisible. This was breaking some of the AIs

    int cnt;
    float glob_amb;

    if ( NULL == lighting ) return false;

    glob_amb = get_ambient_level();

    for ( cnt = 0; cnt < LVEC_AMB; cnt++ )
    {
        lighting[cnt] = 0.0f;
    }
    lighting[LVEC_AMB] = glob_amb;

    if ( !gfx.usefaredge ) return true;

    // do "outside" directional lighting (i.e. sunlight)
    lighting_vector_sum( lighting, light_nrm, light_d * 255, 0.0f );

    return true;
}

//--------------------------------------------------------------------------------------------
// SEMI OBSOLETE FUNCTIONS
//--------------------------------------------------------------------------------------------
void draw_cursor()
{
    /// @author ZZ
    /// @details This function implements a mouse cursor

    oglx_texture_t * tx_ptr = mnu_TxList_get_valid_ptr(( TX_REF )MENU_TX_FONT_BMP );

    if ( input_cursor.x < 6 )  input_cursor.x = 6;
    if ( input_cursor.x > sdl_scr.x - 16 )  input_cursor.x = sdl_scr.x - 16;

    if ( input_cursor.y < 8 )  input_cursor.y = 8;
    if ( input_cursor.y > sdl_scr.y - 24 )  input_cursor.y = sdl_scr.y - 24;

    // Needed to setup text mode
    gfx_begin_text();
    {
        draw_one_font( tx_ptr, 95, input_cursor.x - 5, input_cursor.y - 7 );
    }
    // Needed when done with text mode
    gfx_end_text();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
gfx_rv dynalist_init( dynalist_t * pdylist )
{
    if ( NULL == pdylist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dynalist" );
        return gfx_error;
    }

    pdylist->size = 0;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_dynalist( dynalist_t * pdylist, std::shared_ptr<Camera> pcam )
{
    /// @author ZZ
    /// @details This function figures out which particles are visible, and it sets up dynamic
    ///    lighting

    size_t   tnc;
    fvec3_t  vdist;

    float         distance   = 0.0f;
    dynalight_data_t * plight     = NULL;

    float         distance_max = 0.0f;
    dynalight_data_t * plight_max   = NULL;

    // make sure we have a dynalist
    if ( NULL == pdylist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dynalist" );
        return gfx_error;
    }
    
    // HACK: if dynalist is ahead of the game by 30 frames or more, reset and force an update
    if ((Uint32) (pdylist->frame + 30) >= game_frame_all)
        pdylist->frame = -1;

    // do not update the dynalist more than once a frame
    if ( pdylist->frame >= 0 && ( Uint32 )pdylist->frame >= game_frame_all )
    {
        return gfx_success;
    }

    // Don't really make a list, just set to visible or not
    dynalist_init( pdylist );

    PRT_BEGIN_LOOP_DISPLAY( iprt, prt_bdl )
    {
        dynalight_info_t * pprt_dyna = &( prt_bdl.prt_ptr->dynalight );

        // is the light on?
        if ( !pprt_dyna->on || 0.0f == pprt_dyna->level ) continue;

        // reset the dynalight pointer
        plight = NULL;

        // find the distance to the camera
        vdist = prt_get_pos_v_const(prt_bdl.prt_ptr) - pcam->getTrackPosition();
        distance = vdist.length_2();

        // insert the dynalight
        if (pdylist->size < gfx.dynalist_max &&  pdylist->size < TOTAL_MAX_DYNA)
        {
            if (0 == pdylist->size)
            {
                distance_max = distance;
            }
            else
            {
                distance_max = std::max( distance_max, distance );
            }

            // grab a new light from the list
            plight = pdylist->lst + pdylist->size;
            pdylist->size++;

            if ( distance_max == distance )
            {
                plight_max = plight;
            }
        }
        else if ( distance < distance_max )
        {
            plight = plight_max;

            // find the new maximum distance
            distance_max = pdylist->lst[0].distance;
            plight_max   = pdylist->lst + 0;
            for ( tnc = 1; tnc < gfx.dynalist_max; tnc++ )
            {
                if ( pdylist->lst[tnc].distance > distance_max )
                {
                    plight_max   = pdylist->lst + tnc;
                    distance_max = plight->distance;
                }
            }
        }

        if ( NULL != plight )
        {
            plight->distance = distance;
            prt_get_pos(prt_bdl.prt_ptr, plight->pos);
            plight->level    = pprt_dyna->level;
            plight->falloff  = pprt_dyna->falloff;
        }
    }
    PRT_END_LOOP();

    // the list is updated, so update the frame count
    pdylist->frame = game_frame_all;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv do_grid_lighting( renderlist_t * prlist, dynalist_t * pdylist, std::shared_ptr<Camera> pcam )
{
    /// @author ZZ
    /// @details Do all tile lighting, dynamic and global

    size_t cnt;
    Uint32 fan;
    int    tnc;
    int ix, iy;
    float x0, y0, local_keep;
    bool needs_dynalight;
    ego_mesh_t * pmesh;

    lighting_vector_t global_lighting;

    size_t               reg_count = 0;
    dynalight_registry_t reg[TOTAL_MAX_DYNA];

    ego_frect_t mesh_bound, light_bound;

    ego_mesh_info_t  * pinfo;
    grid_mem_t      * pgmem;
    tile_mem_t      * ptmem;
    oct_bb_t        * poct;

    dynalight_data_t fake_dynalight;

    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    if ( NULL == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    pmesh = renderlist_t::getMesh( prlist );
    if ( NULL == pmesh )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist mesh" );
        return gfx_error;
    }

    pinfo = &( pmesh->info );
    pgmem = &( pmesh->gmem );
    ptmem = &( pmesh->tmem );

    // find a bounding box for the "frustum"
    mesh_bound.xmin = pgmem->edge_x;
    mesh_bound.xmax = 0;
    mesh_bound.ymin = pgmem->edge_y;
    mesh_bound.ymax = 0;
    for ( size_t entry = 0; entry < prlist->all.count; entry++ )
    {
        fan = prlist->all.lst[entry].index;
        if ( fan >= pinfo->tiles_count ) continue;

        poct = &( ptmem->tile_list[fan].oct );

        mesh_bound.xmin = std::min( mesh_bound.xmin, poct->mins[OCT_X] );
        mesh_bound.xmax = std::max( mesh_bound.xmax, poct->maxs[OCT_X] );
        mesh_bound.ymin = std::min( mesh_bound.ymin, poct->mins[OCT_Y] );
        mesh_bound.ymax = std::max( mesh_bound.ymax, poct->maxs[OCT_Y] );
    }

    // is the visible mesh list empty?
    if ( mesh_bound.xmin >= mesh_bound.xmax || mesh_bound.ymin >= mesh_bound.ymax )
        return gfx_success;

    // clear out the dynalight registry
    reg_count = 0;

    // refresh the dynamic light list
    gfx_make_dynalist( pdylist, pcam );

    // assume no dynamic lighting
    needs_dynalight = false;

    // assume no "extra help" for systems with only flat lighting
    dynalight_data__init( &fake_dynalight );

    // initialize the light_bound
    light_bound.xmin = pgmem->edge_x;
    light_bound.xmax = 0;
    light_bound.ymin = pgmem->edge_y;
    light_bound.ymax = 0;

    // make bounding boxes for each dynamic light
    if ( GL_FLAT != gfx.shading )
    {
        for ( cnt = 0; cnt < pdylist->size; cnt++ )
        {
            float radius;
            ego_frect_t ftmp;

            dynalight_data_t * pdyna = pdylist->lst + cnt;

            if ( pdyna->falloff <= 0.0f || 0.0f == pdyna->level ) continue;

            radius = std::sqrt( pdyna->falloff * 765.0f * 0.5f );

            // find the intersection with the frustum boundary
            ftmp.xmin = std::max( pdyna->pos.x - radius, mesh_bound.xmin );
            ftmp.xmax = std::min( pdyna->pos.x + radius, mesh_bound.xmax );
            ftmp.ymin = std::max( pdyna->pos.y - radius, mesh_bound.ymin );
            ftmp.ymax = std::min( pdyna->pos.y + radius, mesh_bound.ymax );

            // check to see if it intersects the "frustum"
            if ( ftmp.xmin >= ftmp.xmax || ftmp.ymin >= ftmp.ymax ) continue;

            reg[reg_count].bound     = ftmp;
            reg[reg_count].reference = cnt;
            reg_count++;

            // determine the maxumum bounding box that encloses all valid lights
            light_bound.xmin = std::min( light_bound.xmin, ftmp.xmin );
            light_bound.xmax = std::max( light_bound.xmax, ftmp.xmax );
            light_bound.ymin = std::min( light_bound.ymin, ftmp.ymin );
            light_bound.ymax = std::max( light_bound.ymax, ftmp.ymax );
        }

        // are there any dynalights visible?
        if ( reg_count > 0 && light_bound.xmax >= light_bound.xmin && light_bound.ymax >= light_bound.ymin )
        {
            needs_dynalight = true;
        }
    }
    else
    {
        float dyna_weight = 0.0f;
        float dyna_weight_sum = 0.0f;

        fvec3_t       diff;
        dynalight_data_t * pdyna;

        // evaluate all the lights at the camera position
        for ( cnt = 0; cnt < pdylist->size; cnt++ )
        {
            pdyna = pdylist->lst + cnt;

            // evaluate the intensity at the camera
            diff.x = pdyna->pos.x - pcam->getCenter().x;
            diff.y = pdyna->pos.y - pcam->getCenter().y;
            diff.z = pdyna->pos.z - pcam->getCenter().z - 90.0f;   // evaluated at the "head height" of a character

            dyna_weight = ABS( dyna_lighting_intensity( pdyna, diff ) );

            fake_dynalight.distance += dyna_weight * pdyna->distance;
            fake_dynalight.falloff  += dyna_weight * pdyna->falloff;
            fake_dynalight.level    += dyna_weight * pdyna->level;
            fake_dynalight.pos.x    += dyna_weight * ( pdyna->pos.x - pcam->getCenter().x );
            fake_dynalight.pos.y    += dyna_weight * ( pdyna->pos.y - pcam->getCenter().y );
            fake_dynalight.pos.z    += dyna_weight * ( pdyna->pos.z - pcam->getCenter().z );

            dyna_weight_sum         += dyna_weight;
        }

        // use a singel dynalight to represent the sum of all dynalights
        if ( dyna_weight_sum > 0.0f )
        {
            float radius;
            ego_frect_t ftmp;

            fake_dynalight.distance /= dyna_weight_sum;
            fake_dynalight.falloff  /= dyna_weight_sum;
            fake_dynalight.level    /= dyna_weight_sum;
            fake_dynalight.pos.x    = fake_dynalight.pos.x / dyna_weight_sum + pcam->getCenter().x;
            fake_dynalight.pos.y    = fake_dynalight.pos.y / dyna_weight_sum + pcam->getCenter().y;
            fake_dynalight.pos.z    = fake_dynalight.pos.z / dyna_weight_sum + pcam->getCenter().z;

            radius = std::sqrt( fake_dynalight.falloff * 765.0f * 0.5f );

            // find the intersection with the frustum boundary
            ftmp.xmin = std::max( fake_dynalight.pos.x - radius, mesh_bound.xmin );
            ftmp.xmax = std::min( fake_dynalight.pos.x + radius, mesh_bound.xmax );
            ftmp.ymin = std::max( fake_dynalight.pos.y - radius, mesh_bound.ymin );
            ftmp.ymax = std::min( fake_dynalight.pos.y + radius, mesh_bound.ymax );

            // make a fake light bound
            light_bound = ftmp;

            // register the fake dynalight
            reg[reg_count].bound     = ftmp;
            reg[reg_count].reference = -1;
            reg_count++;

            // let the downstream calc know we are coming
            needs_dynalight = true;
        }
    }

    // sum up the lighting from global sources
    sum_global_lighting( global_lighting );

    // make the grids update their lighting every 4 frames
    local_keep = POW( dynalight_keep, 4 );

    // Add to base light level in normal mode
    for ( size_t entry = 0; entry < prlist->all.count; entry++ )
    {
        bool resist_lighting_calculation = true;

        int                dynalight_count = 0;
        ego_grid_info_t  * pgrid = NULL;
        lighting_cache_t * pcache_old = NULL;
        lighting_cache_t   cache_new;

        // grab each grid box in the "frustum"
        fan = prlist->all.lst[entry].index;

        // a valid tile?
        pgrid = ego_mesh_get_pgrid( pmesh, fan );
        if ( NULL == pgrid ) continue;

        // do not update this more than once a frame
        if ( pgrid->cache_frame >= 0 && ( Uint32 )pgrid->cache_frame >= game_frame_all ) continue;

        ix = fan % pinfo->tiles_x;
        iy = fan / pinfo->tiles_x;

        // Resist the lighting calculation?
        // This is a speedup for lighting calculations so that
        // not every light-tile calculation is done every single frame
        resist_lighting_calculation = ( 0 != ((( ix + iy ) ^ game_frame_all ) & 0x03 ) );

        if ( resist_lighting_calculation ) continue;

        // this is not a "bad" grid box, so grab the lighting info
        pcache_old = &( pgrid->cache );

        lighting_cache_init( &cache_new );

        // copy the global lighting
        for ( tnc = 0; tnc < LIGHTING_VEC_SIZE; tnc++ )
        {
            cache_new.low.lighting[tnc] = global_lighting[tnc];
            cache_new.hgh.lighting[tnc] = global_lighting[tnc];
        };

        // do we need any dynamic lighting at all?
        if ( needs_dynalight )
        {
            // calculate the local lighting

            ego_frect_t fgrid_rect;

            x0 = ix * GRID_FSIZE;
            y0 = iy * GRID_FSIZE;

            // check this grid vertex relative to the measured light_bound
            fgrid_rect.xmin = x0 - GRID_FSIZE * 0.5f;
            fgrid_rect.xmax = x0 + GRID_FSIZE * 0.5f;
            fgrid_rect.ymin = y0 - GRID_FSIZE * 0.5f;
            fgrid_rect.ymax = y0 + GRID_FSIZE * 0.5f;

            // check the bounding box of this grid vs. the bounding box of the lighting
            if ( fgrid_rect.xmin <= light_bound.xmax && fgrid_rect.xmax >= light_bound.xmin )
            {
                if ( fgrid_rect.ymin <= light_bound.ymax && fgrid_rect.ymax >= light_bound.ymin )
                {
                    // this grid has dynamic lighting. add it.
                    for ( cnt = 0; cnt < reg_count; cnt++ )
                    {
                        fvec3_t       nrm;
                        dynalight_data_t * pdyna;

                        // does this dynamic light intersects this grid?
                        if ( fgrid_rect.xmin > reg[cnt].bound.xmax || fgrid_rect.xmax < reg[cnt].bound.xmin ) continue;
                        if ( fgrid_rect.ymin > reg[cnt].bound.ymax || fgrid_rect.ymax < reg[cnt].bound.ymin ) continue;

                        dynalight_count++;

                        // this should be a valid intersection, so proceed
                        tnc = reg[cnt].reference;
                        if ( tnc < 0 )
                        {
                            pdyna = &fake_dynalight;
                        }
                        else
                        {
                            pdyna = pdylist->lst + tnc;
                        }

                        nrm.x = pdyna->pos.x - x0;
                        nrm.y = pdyna->pos.y - y0;
                        nrm.z = pdyna->pos.z - ptmem->bbox.mins[ZZ];
                        sum_dyna_lighting( pdyna, cache_new.low.lighting, nrm );

                        nrm.z = pdyna->pos.z - ptmem->bbox.maxs[ZZ];
                        sum_dyna_lighting( pdyna, cache_new.hgh.lighting, nrm );
                    }
                }
            }
        }
        else if ( GL_FLAT == gfx.shading )
        {
            // evaluate the intensity at the camera
        }

        // blend in the global lighting every single time
        // average this in with the existing lighting
        lighting_cache_blend( pcache_old, &cache_new, local_keep );

        // find the max intensity
        lighting_cache_max_light( pcache_old );

        pgrid->cache_frame = game_frame_all;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_capture_mesh_tile( ego_tile_info_t * ptile )
{
    if ( NULL == ptile )
    {
        return gfx_fail;
    }

    // Flag the tile as in the renderlist
    ptile->inrenderlist = true;

    // if the tile was not in the renderlist last frame, then we need to force a lighting update of this tile
    if ( ptile->inrenderlist_frame < 0 )
    {
        ptile->request_lcache_update = true;
        ptile->lcache_frame        = -1;
    }
    else
    {
        Uint32 last_frame = ( game_frame_all > 0 ) ? game_frame_all - 1 : 0;

        if (( Uint32 ) ptile->inrenderlist_frame < last_frame )
        {
            ptile->request_lcache_update = true;
        }
    }

    // make sure to cache the frame number of this update
    ptile->inrenderlist_frame = game_frame_all;

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_renderlist( renderlist_t * prlist, std::shared_ptr<Camera> pcam )
{
    gfx_rv      retval;
    bool      local_allocation;

    // Make sure there is a renderlist
    if ( NULL == prlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
        return gfx_error;
    }

    // make sure that we have a valid camera
    if ( nullptr == pcam )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
        return gfx_error;
    }

    // because the main loop of the program will always flip the
    // page before rendering the 1st frame of the actual game,
    // game_frame_all will always start at 1
    if ( 1 != ( game_frame_all & 3 ) )
    {
        return gfx_success;
    }

    // reset the renderlist
    if ( gfx_error == renderlist_t::reset( prlist ) )
    {
        return gfx_error;
    }

    // has the colst been allocated?
    local_allocation = false;
    if ( 0 == _renderlist_colst.capacity())
    {
        // allocate a BSP leaf pointer array to return the detected nodes
        local_allocation = true;
        if ( NULL == _renderlist_colst.ctor(renderlist_lst_t::CAPACITY))
        {
            gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "Could not allocate collision list" );
            return gfx_error;
        }
    }

    // assume the best
    retval = gfx_success;

    // get the tiles in the center of the view
    _renderlist_colst.clear();
	//getMeshBSP()->collide(&(pcam->getFrustum()), nullptr, &_renderlist_colst);
    //TODO

    // transfer valid _renderlist_colst entries to the dolist
    if ( gfx_error == renderlist_t::add( prlist, &_renderlist_colst, pcam ) )
    {
        retval = gfx_error;
        goto gfx_make_renderlist_exit;
    }

gfx_make_renderlist_exit:

    // if there was a local allocation, make sure to deallocate
    if ( local_allocation )
    {
        _renderlist_colst.dtor();
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_make_dolist( dolist_t * pdlist, std::shared_ptr<Camera> pcam )
{
    gfx_rv retval;
    bool local_allocation;

    // assume the best
    retval = gfx_success;

    if ( NULL == pdlist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

	if (pdlist->size >= dolist_t::CAPACITY)
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    // Remove everyone from the dolist
    dolist_t::reset( pdlist, pdlist->index );

    // has the colst been allocated?
    local_allocation = false;
    if ( 0 == _dolist_colst.capacity() )
    {
        // allocate a BSP leaf pointer array to return the detected nodes
        local_allocation = true;
        if ( NULL == _dolist_colst.ctor(dolist_t::CAPACITY))
        {
            gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "Could not allocate collision list" );
            return gfx_error;
        }
    }

    // collide the characters with the frustum
    _dolist_colst.clear();
	//getChrBSP()->collide(&(pcam->getFrustum()), chr_BSP_is_visible, &_dolist_colst);
    //TODO

    // transfer valid _dolist_colst entries to the dolist
    if ( gfx_error == dolist_t::add_colst( pdlist, &_dolist_colst ) )
    {
        retval = gfx_error;
        goto gfx_make_dolist_exit;
    }

    // collide the particles with the frustum
    _dolist_colst.clear();
	//getPrtBSP()->collide(&(pcam->getFrustum()), prt_BSP_is_visible, &_dolist_colst);
    //TODO

    // transfer valid _dolist_colst entries to the dolist
    if (gfx_error == dolist_t::add_colst(pdlist, &_dolist_colst))
    {
        retval = gfx_error;
        goto gfx_make_dolist_exit;
    }

gfx_make_dolist_exit:

    // if there was a local allocation, make sure to deallocate
    if ( local_allocation )
    {
        _dolist_colst.dtor();
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS

float calc_light_rotation( int rotation, int normal )
{
    /// @author ZZ
    /// @details This function helps make_lighttable
    fvec3_t   nrm, nrm2;
    float sinrot, cosrot;

    nrm.x = MD2Model::getMD2Normal(normal, 0);
    nrm.y = MD2Model::getMD2Normal(normal, 1);
    nrm.z = MD2Model::getMD2Normal(normal, 2);

    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];

    nrm2.x = cosrot * nrm.x + sinrot * nrm.y;
    nrm2.y = cosrot * nrm.y - sinrot * nrm.x;
    nrm2.z = nrm.z;

    return ( nrm2.x < 0 ) ? 0 : ( nrm2.x * nrm2.x );
}

//--------------------------------------------------------------------------------------------
float calc_light_global( int rotation, int normal, float lx, float ly, float lz )
{
    /// @author ZZ
    /// @details This function helps make_lighttable
    float fTmp;
    fvec3_t   nrm, nrm2;
    float sinrot, cosrot;

    nrm.x = MD2Model::getMD2Normal(normal, 0);
    nrm.y = MD2Model::getMD2Normal(normal, 1);
    nrm.z = MD2Model::getMD2Normal(normal, 2);

    sinrot = sinlut[rotation];
    cosrot = coslut[rotation];

    nrm2.x = cosrot * nrm.x + sinrot * nrm.y;
    nrm2.y = cosrot * nrm.y - sinrot * nrm.x;
    nrm2.z = nrm.z;

    fTmp = nrm2.x * lx + nrm2.y * ly + nrm2.z * lz;
    if ( fTmp < 0 ) fTmp = 0;

    return fTmp * fTmp;
}

//--------------------------------------------------------------------------------------------
void gfx_reset_timers()
{
    egolib_throttle_reset( &gfx_throttle );
    gfx_clear_loops = 0;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_update_flashing( dolist_t * pdolist )
{
    gfx_rv retval;
    Uint32 i;

    if ( NULL == pdolist )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL dolist" );
        return gfx_error;
    }

    if ( pdolist->size >= dolist_t::CAPACITY )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "invalid dolist size" );
        return gfx_error;
    }

    retval = gfx_success;
    for ( i = 0; i < pdolist->size; i++ )
    {
        float tmp_seekurse_level;

        GameObject          * pchr;
        chr_instance_t * pinst;

        CHR_REF ichr = pdolist->lst[i].ichr;

        pchr = _gameObjects.get( ichr );
        if ( nullptr == ( pchr ) ) continue;

        pinst = &( pchr->inst );

        // Do flashing
        if ( DONTFLASH != pchr->flashand )
        {
            if ( HAS_NO_BITS( game_frame_all, pchr->flashand ) )
            {
                if ( gfx_error == chr_instance_flash( pinst, 255 ) )
                {
                    retval = gfx_error;
                }
            }
        }

        // Do blacking
        // having one holy player in your party will cause the effect, BUT
        // having some non-holy players will dilute it
        tmp_seekurse_level = std::min( local_stats.seekurse_level, 1.0f );
        if (( local_stats.seekurse_level > 0.0f ) && pchr->iskursed && 1.0f != tmp_seekurse_level )
        {
            if ( HAS_NO_BITS( game_frame_all, SEEKURSEAND ) )
            {
                if ( gfx_error == chr_instance_flash( pinst, 255.0f *( 1.0f - tmp_seekurse_level ) ) )
                {
                    retval = gfx_error;
                }
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv gfx_update_all_chr_instance()
{
    CHR_REF cnt;
    gfx_rv retval;
    gfx_rv tmp_rv;

    // assume the best
    retval = gfx_success;

    for(const std::shared_ptr<GameObject> &pchr : _gameObjects.iterator())
    {
        //Dont do terminated characters
        if(pchr->terminateRequested) {
            continue;
        }

        if ( !ego_mesh_grid_is_valid( PMesh, pchr->onwhichgrid ) ) continue;

        tmp_rv = update_one_chr_instance( pchr.get() );

        // deal with return values
        if ( gfx_error == tmp_rv )
        {
            retval = gfx_error;
        }
        else if ( gfx_success == tmp_rv )
        {
            // the instance has changed, refresh the collision bound
            chr_update_collision_size( pchr.get(), true );
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// Tiled texture "optimizations"
//--------------------------------------------------------------------------------------------
void gfx_system_begin_decimated_textures()
{
    if ( !_gfx_system_mesh_textures_initialized )
    {
        int cnt;

        for ( cnt = 0; cnt < MESH_IMG_COUNT; cnt++ )
        {
            oglx_texture_ctor( mesh_tx_sml + cnt );
            oglx_texture_ctor( mesh_tx_big + cnt );
        }
        mesh_tx_sml_cnt = 0;
        mesh_tx_big_cnt = 0;

        _gfx_system_mesh_textures_initialized = true;
    }
}

//--------------------------------------------------------------------------------------------
void gfx_system_end_decimated_textures()
{
    if ( _gfx_system_mesh_textures_initialized )
    {
        int cnt;

        for ( cnt = 0; cnt < MESH_IMG_COUNT; cnt++ )
        {
            oglx_texture_dtor( mesh_tx_sml + cnt );
            oglx_texture_dtor( mesh_tx_big + cnt );
        }
        mesh_tx_sml_cnt = 0;
        mesh_tx_big_cnt = 0;

        _gfx_system_mesh_textures_initialized = false;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
oglx_texture_t * gfx_get_mesh_tx_sml( int which )
{
    if ( !_gfx_system_mesh_textures_initialized ) return NULL;

    if ( which < 0 || which >= mesh_tx_sml_cnt || which >= MESH_IMG_COUNT ) return NULL;

    return mesh_tx_sml + which;
}

//--------------------------------------------------------------------------------------------
oglx_texture_t * gfx_get_mesh_tx_big( int which )
{
    if ( !_gfx_system_mesh_textures_initialized ) return NULL;

    if ( which < 0 || which >= mesh_tx_big_cnt || which >= MESH_IMG_COUNT ) return NULL;

    return mesh_tx_big + which;
}

//--------------------------------------------------------------------------------------------
int gfx_decimate_one_mesh_texture( oglx_texture_t * src_tx, oglx_texture_t * tx_lst, size_t tx_lst_cnt, int minification )
{
    static const int sub_textures = 8;

    float step_fx, step_fy;
    float fx, fy;
    int   blit_rv;
	size_t ix, iy, cnt;

    SDL_Surface * src_img = NULL;
    int src_img_w, src_img_h;
    SDL_Rect src_img_rect;

    oglx_texture_t * dst_tx = NULL;
    SDL_Surface    * dst_img = NULL;

    if ( NULL == src_tx )
    {
        // the source image doesn't exist, so punt
        goto gfx_decimate_one_mesh_texture_error;
    }

    // make an alias for the texture's SDL_Surface
    src_img = src_tx->surface;
    if ( NULL == src_img )
    {
        goto gfx_decimate_one_mesh_texture_error;
    }

    // grab parameters from the mesh
    src_img_w = src_img->w;
    src_img_h = src_img->h;

    // how large a step every time through the mesh?
    step_fx = ( float )src_img_w / ( float )sub_textures;
    step_fy = ( float )src_img_h / ( float )sub_textures;

    src_img_rect.w = CEIL( step_fx * minification );
    src_img_rect.w = std::max<Uint16>( 1, src_img_rect.w );
    src_img_rect.h = CEIL( step_fy * minification );
    src_img_rect.h = std::max<Uint16>( 1, src_img_rect.h );

    // scan across the src_img
    for ( iy = 0, fy = 0.0f; iy < sub_textures; iy++, fy += step_fy )
    {
        src_img_rect.y = FLOOR( fy );

        for ( ix = 0, fx = 0.0f; ix < sub_textures; ix++, fx += step_fx )
        {
            src_img_rect.x = FLOOR( fx );

            // grab the destination texture
            dst_tx =  tx_lst + tx_lst_cnt;

            // prepare the destination texture
            oglx_texture_ctor( dst_tx );

            // create a blank destination SDL_Surface
            dst_img = gfx_create_SDL_Surface( src_img_rect.w, src_img_rect.h );
            SDL_FillRect( dst_img, NULL, SDL_MapRGBA( dst_img->format, 0xFF, 0, 0, 0xFF ) );

            // blit the source region into the destination bitmap
            blit_rv = SDL_BlitSurface( src_img, &src_img_rect, dst_img, NULL );

            // upload the SDL_Surface into OpenGL
            oglx_texture_Convert( dst_tx, dst_img, INVALID_KEY );

            // count the number of textures we're using
            tx_lst_cnt++;
        }
    }

    return tx_lst_cnt;

gfx_decimate_one_mesh_texture_error:

    // the source texture doesn't exist. Just blank out the destination textures
    for ( cnt = 0; cnt < sub_textures*sub_textures; cnt++ )
    {
        oglx_texture_ctor( tx_lst + tx_lst_cnt );
        tx_lst_cnt++;
    }

    return tx_lst_cnt;
}

//--------------------------------------------------------------------------------------------
void gfx_decimate_all_mesh_textures()
{
    // decimate the tile textures and store them in
    oglx_texture_t * ptx;

    // release any existing textures
    gfx_system_end_decimated_textures();
    gfx_system_begin_decimated_textures();

    // do the "small" textures
    mesh_tx_sml_cnt = 0;
    for ( size_t cnt = 0; cnt < 4; cnt++ )
    {
        ptx = TxList_get_valid_ptr( TX_TILE_0 + cnt );

        mesh_tx_sml_cnt = gfx_decimate_one_mesh_texture( ptx, mesh_tx_sml, mesh_tx_sml_cnt, 1 );
    }

    // do the "big" textures
    mesh_tx_big_cnt = 0;
    for ( size_t cnt = 0; cnt < 4; cnt++ )
    {
        ptx = TxList_get_valid_ptr( TX_TILE_0 + cnt );

        mesh_tx_big_cnt = gfx_decimate_one_mesh_texture( ptx, mesh_tx_big, mesh_tx_big_cnt, 2 );
    }
}

//--------------------------------------------------------------------------------------------
void gfx_reload_decimated_textures()
{
    /// @author BB
    /// @details This function re-loads all the current textures back into
    ///               OpenGL texture memory using the cached SDL surfaces

    TX_REF cnt;
    size_t count;

    if ( !_gfx_system_mesh_textures_initialized ) return;

    count = std::min( MESH_IMG_COUNT, mesh_tx_sml_cnt );
    for ( cnt = 0; cnt < count; cnt++ )
    {
        if ( oglx_texture_Valid( mesh_tx_sml + cnt ) )
        {
            oglx_texture_Convert( mesh_tx_sml + cnt, mesh_tx_sml[cnt].surface, INVALID_KEY );
        }
    }

    count = std::min( MESH_IMG_COUNT, mesh_tx_big_cnt );
    for ( cnt = 0; cnt < count; cnt++ )
    {
        if ( oglx_texture_Valid( mesh_tx_big + cnt ) )
        {
            oglx_texture_Convert( mesh_tx_big + cnt, mesh_tx_big[cnt].surface, INVALID_KEY );
        }
    }
}

//--------------------------------------------------------------------------------------------
// chr_instance_t FUNCTIONS
//--------------------------------------------------------------------------------------------
gfx_rv update_one_chr_instance( GameObject * pchr )
{
    chr_instance_t * pinst;
    gfx_rv retval;

    if ( !ACTIVE_PCHR( pchr ) )
    {
        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, GET_INDEX_PCHR( pchr ), "invalid character" );
        return gfx_error;
    }
    pinst = &( pchr->inst );

    // only update once per frame
    if ( pinst->update_frame >= 0 && ( Uint32 )pinst->update_frame >= game_frame_all )
    {
        return gfx_success;
    }

    // make sure that the vertices are interpolated
    retval = chr_instance_update_vertices( pinst, -1, -1, true );
    if ( gfx_error == retval )
    {
        return gfx_error;
    }

    // do the basic lighting
    chr_instance_update_lighting_base( pinst, pchr, false );

    // set the update_frame to the current frame
    pinst->update_frame = game_frame_all;

    return retval;
}

//--------------------------------------------------------------------------------------------
gfx_rv chr_instance_flash( chr_instance_t * pinst, Uint8 value )
{
    /// @author ZZ
    /// @details This function sets a character's lighting

    size_t     cnt;
    float      flash_val = value * INV_FF;
    GLvertex * pv;

    if ( NULL == pinst ) return gfx_error;

    // flash the ambient color
    pinst->color_amb = flash_val;

    // flash the directional lighting
    pinst->color_amb = flash_val;
    for ( cnt = 0; cnt < pinst->vrt_count; cnt++ )
    {
        pv = pinst->vrt_lst + cnt;

        pv->color_dir = flash_val;
    }

    return gfx_success;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE
//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
// projection_bitmap_t
//--------------------------------------------------------------------------------------------

//struct s_projection_bitmap;
//typedef struct s_projection_bitmap projection_bitmap_t;

//#define PROJECTION_BITMAP_ROWS 128

//struct s_projection_bitmap
//{
//    int y_stt;
//    int row_cnt;
//
//    int row_stt[PROJECTION_BITMAP_ROWS];
//    int row_end[PROJECTION_BITMAP_ROWS];
//};

//static projection_bitmap_t * projection_bitmap_ctor( projection_bitmap_t * ptr );

//projection_bitmap_t * projection_bitmap_ctor( projection_bitmap_t * ptr )
//{
//    if ( NULL == ptr ) return ptr;
//
//    ptr->y_stt   = 0;
//    ptr->row_cnt = 0;
//
//    return ptr;
//}

//--------------------------------------------------------------------------------------------
// cam_corner_info_t
//--------------------------------------------------------------------------------------------

//struct s_cam_corner_info;
//typedef struct s_cam_corner_info cam_corner_info_t;

//struct s_cam_corner_info
//{
//    float                   lowx;
//    float                   highx;
//    float                   lowy;
//    float                   highy;
//
//    float                   x[4];
//    float                   y[4];
//    int                     listlowtohighy[4];
//};

//void gfx_calc_rotmesh()
//{
//    static const float assumed_aspect_ratio = 4.0f / 3.0f;
//
//    float aspect_ratio = ( float )sdl_scr.x / ( float )sdl_scr.y;
//
//    // Matrix init stuff (from remove.c)
//    rotmesh_topside    = aspect_ratio / assumed_aspect_ratio * ROTMESH_TOPSIDE ;
//    rotmesh_bottomside = aspect_ratio / assumed_aspect_ratio * ROTMESH_BOTTOMSIDE;
//    rotmesh_up         = aspect_ratio / assumed_aspect_ratio * ROTMESH_UP;
//    rotmesh_down       = aspect_ratio / assumed_aspect_ratio * ROTMESH_DOWN;
//}

//--------------------------------------------------------------------------------------------
//void get_clip_planes( const camera_t * pcam, GLfloat planes[] )
//{
//    // clip plane normals point IN
//
//    GLfloat clip[16];
//    GLfloat _proj[16];
//    GLfloat _modl[16];
//    GLfloat t;
//
//    const GLfloat * proj = NULL;
//    const GLfloat * modl = NULL;
//
//    // get the matrices...
//    if ( NULL != pcam )
//    {
//        // ...from the camera
//        proj = pcam->mProjection_big.v;
//        modl = pcam->getView().v;
//    }
//    else
//    {
//        // ... from the current OpenGL state
//
//        // The Current PROJECTION Matrix
//        proj = _proj;
//        glGetFloatv( GL_PROJECTION_MATRIX, _proj );
//
//        // The Current MODELVIEW Matrix
//        modl = _modl;
//        glGetFloatv( GL_MODELVIEW_MATRIX, _modl );
//    }
//
//    // Multiply Projection By Modelview
//    clip[ MAT_IDX( 0, 0 )] = modl[ MAT_IDX( 0, 0 )] * proj[ MAT_IDX( 0, 0 )] + modl[ MAT_IDX( 0, 1 )] * proj[ MAT_IDX( 1, 0 )] + modl[ MAT_IDX( 0, 2 )] * proj[ MAT_IDX( 2, 0 )] + modl[ MAT_IDX( 0, 3 )] * proj[ MAT_IDX( 3, 0 )];
//    clip[ MAT_IDX( 0, 1 )] = modl[ MAT_IDX( 0, 0 )] * proj[ MAT_IDX( 0, 1 )] + modl[ MAT_IDX( 0, 1 )] * proj[ MAT_IDX( 1, 1 )] + modl[ MAT_IDX( 0, 2 )] * proj[ MAT_IDX( 2, 1 )] + modl[ MAT_IDX( 0, 3 )] * proj[ MAT_IDX( 3, 1 )];
//    clip[ MAT_IDX( 0, 2 )] = modl[ MAT_IDX( 0, 0 )] * proj[ MAT_IDX( 0, 2 )] + modl[ MAT_IDX( 0, 1 )] * proj[ MAT_IDX( 1, 2 )] + modl[ MAT_IDX( 0, 2 )] * proj[ MAT_IDX( 2, 2 )] + modl[ MAT_IDX( 0, 3 )] * proj[ MAT_IDX( 3, 2 )];
//    clip[ MAT_IDX( 0, 3 )] = modl[ MAT_IDX( 0, 0 )] * proj[ MAT_IDX( 0, 3 )] + modl[ MAT_IDX( 0, 1 )] * proj[ MAT_IDX( 1, 3 )] + modl[ MAT_IDX( 0, 2 )] * proj[ MAT_IDX( 2, 3 )] + modl[ MAT_IDX( 0, 3 )] * proj[ MAT_IDX( 3, 3 )];
//
//    clip[ MAT_IDX( 1, 0 )] = modl[ MAT_IDX( 1, 0 )] * proj[ MAT_IDX( 0, 0 )] + modl[ MAT_IDX( 1, 1 )] * proj[ MAT_IDX( 1, 0 )] + modl[ MAT_IDX( 1, 2 )] * proj[ MAT_IDX( 2, 0 )] + modl[ MAT_IDX( 1, 3 )] * proj[ MAT_IDX( 3, 0 )];
//    clip[ MAT_IDX( 1, 1 )] = modl[ MAT_IDX( 1, 0 )] * proj[ MAT_IDX( 0, 1 )] + modl[ MAT_IDX( 1, 1 )] * proj[ MAT_IDX( 1, 1 )] + modl[ MAT_IDX( 1, 2 )] * proj[ MAT_IDX( 2, 1 )] + modl[ MAT_IDX( 1, 3 )] * proj[ MAT_IDX( 3, 1 )];
//    clip[ MAT_IDX( 1, 2 )] = modl[ MAT_IDX( 1, 0 )] * proj[ MAT_IDX( 0, 2 )] + modl[ MAT_IDX( 1, 1 )] * proj[ MAT_IDX( 1, 2 )] + modl[ MAT_IDX( 1, 2 )] * proj[ MAT_IDX( 2, 2 )] + modl[ MAT_IDX( 1, 3 )] * proj[ MAT_IDX( 3, 2 )];
//    clip[ MAT_IDX( 1, 3 )] = modl[ MAT_IDX( 1, 0 )] * proj[ MAT_IDX( 0, 3 )] + modl[ MAT_IDX( 1, 1 )] * proj[ MAT_IDX( 1, 3 )] + modl[ MAT_IDX( 1, 2 )] * proj[ MAT_IDX( 2, 3 )] + modl[ MAT_IDX( 1, 3 )] * proj[ MAT_IDX( 3, 3 )];
//
//    clip[ MAT_IDX( 2, 0 )] = modl[ MAT_IDX( 2, 0 )] * proj[ MAT_IDX( 0, 0 )] + modl[ MAT_IDX( 2, 1 )] * proj[ MAT_IDX( 1, 0 )] + modl[ MAT_IDX( 2, 2 )] * proj[ MAT_IDX( 2, 0 )] + modl[ MAT_IDX( 2, 3 )] * proj[ MAT_IDX( 3, 0 )];
//    clip[ MAT_IDX( 2, 1 )] = modl[ MAT_IDX( 2, 0 )] * proj[ MAT_IDX( 0, 1 )] + modl[ MAT_IDX( 2, 1 )] * proj[ MAT_IDX( 1, 1 )] + modl[ MAT_IDX( 2, 2 )] * proj[ MAT_IDX( 2, 1 )] + modl[ MAT_IDX( 2, 3 )] * proj[ MAT_IDX( 3, 1 )];
//    clip[ MAT_IDX( 2, 2 )] = modl[ MAT_IDX( 2, 0 )] * proj[ MAT_IDX( 0, 2 )] + modl[ MAT_IDX( 2, 1 )] * proj[ MAT_IDX( 1, 2 )] + modl[ MAT_IDX( 2, 2 )] * proj[ MAT_IDX( 2, 2 )] + modl[ MAT_IDX( 2, 3 )] * proj[ MAT_IDX( 3, 2 )];
//    clip[ MAT_IDX( 2, 3 )] = modl[ MAT_IDX( 2, 0 )] * proj[ MAT_IDX( 0, 3 )] + modl[ MAT_IDX( 2, 1 )] * proj[ MAT_IDX( 1, 3 )] + modl[ MAT_IDX( 2, 2 )] * proj[ MAT_IDX( 2, 3 )] + modl[ MAT_IDX( 2, 3 )] * proj[ MAT_IDX( 3, 3 )];
//
//    clip[ MAT_IDX( 3, 0 )] = modl[ MAT_IDX( 3, 0 )] * proj[ MAT_IDX( 0, 0 )] + modl[ MAT_IDX( 3, 1 )] * proj[ MAT_IDX( 1, 0 )] + modl[ MAT_IDX( 3, 2 )] * proj[ MAT_IDX( 2, 0 )] + modl[ MAT_IDX( 3, 3 )] * proj[ MAT_IDX( 3, 0 )];
//    clip[ MAT_IDX( 3, 1 )] = modl[ MAT_IDX( 3, 0 )] * proj[ MAT_IDX( 0, 1 )] + modl[ MAT_IDX( 3, 1 )] * proj[ MAT_IDX( 1, 1 )] + modl[ MAT_IDX( 3, 2 )] * proj[ MAT_IDX( 2, 1 )] + modl[ MAT_IDX( 3, 3 )] * proj[ MAT_IDX( 3, 1 )];
//    clip[ MAT_IDX( 3, 2 )] = modl[ MAT_IDX( 3, 0 )] * proj[ MAT_IDX( 0, 2 )] + modl[ MAT_IDX( 3, 1 )] * proj[ MAT_IDX( 1, 2 )] + modl[ MAT_IDX( 3, 2 )] * proj[ MAT_IDX( 2, 2 )] + modl[ MAT_IDX( 3, 3 )] * proj[ MAT_IDX( 3, 2 )];
//    clip[ MAT_IDX( 3, 3 )] = modl[ MAT_IDX( 3, 0 )] * proj[ MAT_IDX( 0, 3 )] + modl[ MAT_IDX( 3, 1 )] * proj[ MAT_IDX( 1, 3 )] + modl[ MAT_IDX( 3, 2 )] * proj[ MAT_IDX( 2, 3 )] + modl[ MAT_IDX( 3, 3 )] * proj[ MAT_IDX( 3, 3 )];
//
//    // Extract The Numbers For The RIGHT Plane
//    planes[MAT_IDX( 0, 0 )] = clip[ MAT_IDX( 0, 3 )] - clip[ MAT_IDX( 0, 0 )];
//    planes[MAT_IDX( 0, 1 )] = clip[ MAT_IDX( 1, 3 )] - clip[ MAT_IDX( 1, 0 )];
//    planes[MAT_IDX( 0, 2 )] = clip[ MAT_IDX( 2, 3 )] - clip[ MAT_IDX( 2, 0 )];
//    planes[MAT_IDX( 0, 3 )] = clip[ MAT_IDX( 3, 3 )] - clip[ MAT_IDX( 3, 0 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 0, 0 )] * planes[MAT_IDX( 0, 0 )] + planes[MAT_IDX( 0, 1 )] * planes[MAT_IDX( 0, 1 )] + planes[MAT_IDX( 0, 2 )] * planes[MAT_IDX( 0, 2 )] );
//    planes[MAT_IDX( 0, 0 )] /= t;
//    planes[MAT_IDX( 0, 1 )] /= t;
//    planes[MAT_IDX( 0, 2 )] /= t;
//    planes[MAT_IDX( 0, 3 )] /= t;
//
//    // Extract The Numbers For The LEFT Plane
//    planes[MAT_IDX( 1, 0 )] = clip[ MAT_IDX( 0, 3 )] + clip[ MAT_IDX( 0, 0 )];
//    planes[MAT_IDX( 1, 1 )] = clip[ MAT_IDX( 1, 3 )] + clip[ MAT_IDX( 1, 0 )];
//    planes[MAT_IDX( 1, 2 )] = clip[ MAT_IDX( 2, 3 )] + clip[ MAT_IDX( 2, 0 )];
//    planes[MAT_IDX( 1, 3 )] = clip[ MAT_IDX( 3, 3 )] + clip[ MAT_IDX( 3, 0 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 1, 0 )] * planes[MAT_IDX( 1, 0 )] + planes[MAT_IDX( 1, 1 )] * planes[MAT_IDX( 1, 1 )] + planes[MAT_IDX( 1, 2 )] * planes[MAT_IDX( 1, 2 )] );
//    planes[MAT_IDX( 1, 0 )] /= t;
//    planes[MAT_IDX( 1, 1 )] /= t;
//    planes[MAT_IDX( 1, 2 )] /= t;
//    planes[MAT_IDX( 1, 3 )] /= t;
//
//    // Extract The BOTTOM Plane
//    planes[MAT_IDX( 2, 0 )] = clip[ MAT_IDX( 0, 3 )] + clip[ MAT_IDX( 0, 1 )];
//    planes[MAT_IDX( 2, 1 )] = clip[ MAT_IDX( 1, 3 )] + clip[ MAT_IDX( 1, 1 )];
//    planes[MAT_IDX( 2, 2 )] = clip[ MAT_IDX( 2, 3 )] + clip[ MAT_IDX( 2, 1 )];
//    planes[MAT_IDX( 2, 3 )] = clip[ MAT_IDX( 3, 3 )] + clip[ MAT_IDX( 3, 1 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 2, 0 )] * planes[MAT_IDX( 2, 0 )] + planes[MAT_IDX( 2, 1 )] * planes[MAT_IDX( 2, 1 )] + planes[MAT_IDX( 2, 2 )] * planes[MAT_IDX( 2, 2 )] );
//    planes[MAT_IDX( 2, 0 )] /= t;
//    planes[MAT_IDX( 2, 1 )] /= t;
//    planes[MAT_IDX( 2, 2 )] /= t;
//    planes[MAT_IDX( 2, 3 )] /= t;
//
//    // Extract The TOP Plane
//    planes[MAT_IDX( 3, 0 )] = clip[ MAT_IDX( 0, 3 )] - clip[ MAT_IDX( 0, 1 )];
//    planes[MAT_IDX( 3, 1 )] = clip[ MAT_IDX( 1, 3 )] - clip[ MAT_IDX( 1, 1 )];
//    planes[MAT_IDX( 3, 2 )] = clip[ MAT_IDX( 2, 3 )] - clip[ MAT_IDX( 2, 1 )];
//    planes[MAT_IDX( 3, 3 )] = clip[ MAT_IDX( 3, 3 )] - clip[ MAT_IDX( 3, 1 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 3, 0 )] * planes[MAT_IDX( 3, 0 )] + planes[MAT_IDX( 3, 1 )] * planes[MAT_IDX( 3, 1 )] + planes[MAT_IDX( 3, 2 )] * planes[MAT_IDX( 3, 2 )] );
//    planes[MAT_IDX( 3, 0 )] /= t;
//    planes[MAT_IDX( 3, 1 )] /= t;
//    planes[MAT_IDX( 3, 2 )] /= t;
//    planes[MAT_IDX( 3, 3 )] /= t;
//
//    // Extract The FAR Plane
//    planes[MAT_IDX( 4, 0 )] = clip[ MAT_IDX( 0, 3 )] - clip[ MAT_IDX( 0, 2 )];
//    planes[MAT_IDX( 4, 1 )] = clip[ MAT_IDX( 1, 3 )] - clip[ MAT_IDX( 1, 2 )];
//    planes[MAT_IDX( 4, 2 )] = clip[ MAT_IDX( 2, 3 )] - clip[ MAT_IDX( 2, 2 )];
//    planes[MAT_IDX( 4, 3 )] = clip[ MAT_IDX( 3, 3 )] - clip[ MAT_IDX( 3, 2 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 4, 0 )] * planes[MAT_IDX( 4, 0 )] + planes[MAT_IDX( 4, 1 )] * planes[MAT_IDX( 4, 1 )] + planes[MAT_IDX( 4, 2 )] * planes[MAT_IDX( 4, 2 )] );
//    planes[MAT_IDX( 4, 0 )] /= t;
//    planes[MAT_IDX( 4, 1 )] /= t;
//    planes[MAT_IDX( 4, 2 )] /= t;
//    planes[MAT_IDX( 4, 3 )] /= t;
//
//    // Extract The NEAR Plane
//    planes[MAT_IDX( 5, 0 )] = clip[ MAT_IDX( 0, 3 )] + clip[ MAT_IDX( 0, 2 )];
//    planes[MAT_IDX( 5, 1 )] = clip[ MAT_IDX( 1, 3 )] + clip[ MAT_IDX( 1, 2 )];
//    planes[MAT_IDX( 5, 2 )] = clip[ MAT_IDX( 2, 3 )] + clip[ MAT_IDX( 2, 2 )];
//    planes[MAT_IDX( 5, 3 )] = clip[ MAT_IDX( 3, 3 )] + clip[ MAT_IDX( 3, 2 )];
//
//    // Normalize The Result
//    t = SQRT( planes[MAT_IDX( 5, 0 )] * planes[MAT_IDX( 5, 0 )] + planes[MAT_IDX( 5, 1 )] * planes[MAT_IDX( 5, 1 )] + planes[MAT_IDX( 5, 2 )] * planes[MAT_IDX( 5, 2 )] );
//    planes[MAT_IDX( 5, 0 )] /= t;
//    planes[MAT_IDX( 5, 1 )] /= t;
//    planes[MAT_IDX( 5, 2 )] /= t;
//    planes[MAT_IDX( 5, 3 )] /= t;
//}
//
//--------------------------------------------------------------------------------------------
//gfx_rv gfx_project_cam_view( const camera_t * pcam, cam_corner_info_t * pinfo )
//{
//    /// @author ZZ
/// @details This function figures out where the corners of the view area
//    ///    go when projected onto the plane of the PMesh.  Used later for
//    ///    determining which mesh fans need to be rendered
//
//    enum
//    {
//        plane_RIGHT  = 0,
//        plane_LEFT   = 1,
//        plane_BOTTOM = 2,
//        plane_TOP    = 3,
//        plane_FAR    = 4,
//        plane_NEAR   = 5,
//
//        // other values
//        plane_SIZE = 4
//    };
//
//    enum
//    {
//        plane_offset_RIGHT  = plane_RIGHT  * plane_SIZE,
//        plane_offset_LEFT   = plane_LEFT   * plane_SIZE,
//        plane_offset_BOTTOM = plane_BOTTOM * plane_SIZE,
//        plane_offset_TOP    = plane_TOP    * plane_SIZE,
//        plane_offset_FAR    = plane_FAR    * plane_SIZE,
//        plane_offset_NEAR   = plane_NEAR   * plane_SIZE
//    };
//
//    enum
//    {
//        corner_TOP_LEFT = 0,
//        corner_TOP_RIGHT,
//        corner_BOTTOM_RIGHT,
//        corner_BOTTOM_LEFT,
//        corner_COUNT
//    };
//
//    int cnt, tnc, extra[2];
//    fvec3_t point;
//    int corner_count;
//    fvec3_t corner_pos[corner_COUNT], corner_dir[corner_COUNT];
//
//    // six planes with 4 components.
//    // the plane i starts at planes + 4*i
//    // plane_offset_RIGHT, plane_offset_LEFT, plane_offset_BOTTOM, plane_offset_TOP, plane_offset_FAR, plane_offset_NEAR
//    GLfloat planes[ 6 * 4 ];
//
//    GLfloat ground_plane[4] = {0.0f, 0.0f, 1.0f, 0.0f};
//
//    if ( NULL == pinfo )
//    {
//        return gfx_fail;
//    }
//
//    if ( NULL == pcam )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
//        return gfx_error;
//    }
//
//    get_clip_planes( pcam, planes );
//
//    // the number of valid points
//    corner_count = 0;
//
//    // grab the coefficients of the rays at the 4 "folds" in the frustum
//    two_plane_intersection( corner_pos[corner_TOP_LEFT].v, corner_dir[corner_TOP_LEFT].v, planes + plane_offset_LEFT, planes + plane_offset_TOP );
//    if ( corner_dir[corner_TOP_LEFT].z < 0.0f )
//    {
//        float t = corner_pos[corner_TOP_LEFT].z / corner_dir[corner_TOP_LEFT].z;
//        pinfo->x[corner_count] = corner_pos[corner_TOP_LEFT].x + corner_dir[corner_TOP_LEFT].x * t;
//        pinfo->y[corner_count] = corner_pos[corner_TOP_LEFT].y + corner_dir[corner_TOP_LEFT].y * t;
//        corner_count++;
//    }
//
//    two_plane_intersection( corner_pos[corner_TOP_RIGHT].v, corner_dir[corner_TOP_RIGHT].v, planes + plane_offset_TOP, planes + plane_offset_RIGHT );
//    if ( corner_dir[corner_TOP_RIGHT].z < 0.0f )
//    {
//        float t = corner_pos[corner_TOP_RIGHT].z / corner_dir[corner_TOP_RIGHT].z;
//        pinfo->x[corner_count] = corner_pos[corner_TOP_RIGHT].x + corner_dir[corner_TOP_RIGHT].x * t;
//        pinfo->y[corner_count] = corner_pos[corner_TOP_RIGHT].y + corner_dir[corner_TOP_RIGHT].y * t;
//        corner_count++;
//    }
//
//    two_plane_intersection( corner_pos[corner_BOTTOM_RIGHT].v, corner_dir[corner_BOTTOM_RIGHT].v, planes + plane_offset_RIGHT, planes + plane_offset_BOTTOM );
//    if ( corner_dir[corner_BOTTOM_RIGHT].z < 0.0f )
//    {
//        float t = corner_pos[corner_BOTTOM_RIGHT].z / corner_dir[corner_BOTTOM_RIGHT].z;
//        pinfo->x[corner_count] = corner_pos[corner_BOTTOM_RIGHT].x + corner_dir[corner_BOTTOM_RIGHT].x * t;
//        pinfo->y[corner_count] = corner_pos[corner_BOTTOM_RIGHT].y + corner_dir[corner_BOTTOM_RIGHT].y * t;
//        corner_count++;
//    }
//
//    two_plane_intersection( corner_pos[corner_BOTTOM_LEFT].v, corner_dir[corner_BOTTOM_LEFT].v, planes + plane_offset_BOTTOM, planes + plane_offset_LEFT );
//    if ( corner_dir[corner_BOTTOM_LEFT].z < 0.0f )
//    {
//        float t = corner_pos[corner_BOTTOM_LEFT].z / corner_dir[corner_BOTTOM_LEFT].z;
//        pinfo->x[corner_count] = corner_pos[corner_BOTTOM_LEFT].x + corner_dir[corner_BOTTOM_LEFT].x * t;
//        pinfo->y[corner_count] = corner_pos[corner_BOTTOM_LEFT].y + corner_dir[corner_BOTTOM_LEFT].y * t;
//        corner_count++;
//    }
//
//    if ( corner_count < 4 )
//    {
//        // try plane_offset_RIGHT, plane_offset_FAR, and GROUND
//        if ( three_plane_intersection( point.v, planes + plane_offset_RIGHT, planes + plane_offset_FAR, ground_plane ) )
//        {
//            pinfo->x[corner_count] = point.x;
//            pinfo->y[corner_count] = point.y;
//            corner_count++;
//        }
//    }
//
//    if ( corner_count < 4 )
//    {
//        // try plane_offset_LEFT, plane_offset_FAR, and GROUND
//        if ( three_plane_intersection( point.v, planes + plane_offset_LEFT, planes + plane_offset_FAR, ground_plane ) )
//        {
//            pinfo->x[corner_count] = point.x;
//            pinfo->y[corner_count] = point.y;
//            corner_count++;
//        }
//    }
//
//    if ( corner_count < 4 )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "not enough corner points intersecting with the ground" );
//        return gfx_error;
//    }
//
//    // Get the extreme values
//    pinfo->lowx = pinfo->x[0];
//    pinfo->lowy = pinfo->y[0];
//    pinfo->highx = pinfo->x[0];
//    pinfo->highy = pinfo->y[0];
//    pinfo->listlowtohighy[0] = 0;
//    pinfo->listlowtohighy[3] = 0;
//
//    // sort the corners
//    for ( cnt = 0; cnt < 4; cnt++ )
//    {
//        if ( pinfo->x[cnt] < pinfo->lowx )
//        {
//            pinfo->lowx = pinfo->x[cnt];
//        }
//
//        if ( pinfo->x[cnt] > pinfo->highx )
//        {
//            pinfo->highx = pinfo->x[cnt];
//        }
//
//        if ( pinfo->y[cnt] < pinfo->lowy )
//        {
//            pinfo->lowy = pinfo->y[cnt];
//            pinfo->listlowtohighy[0] = cnt;
//        }
//
//        if ( pinfo->y[cnt] > pinfo->highy )
//        {
//            pinfo->highy = pinfo->y[cnt];
//            pinfo->listlowtohighy[3] = cnt;
//        }
//    }
//
//    // Figure out the order of points
//    for ( cnt = 0, tnc = 0; cnt < 4; cnt++ )
//    {
//        if ( cnt != pinfo->listlowtohighy[0] && cnt != pinfo->listlowtohighy[3] )
//        {
//            extra[tnc] = cnt;
//            tnc++;
//        }
//    }
//
//    pinfo->listlowtohighy[1] = extra[1];
//    pinfo->listlowtohighy[2] = extra[0];
//    if ( pinfo->y[extra[0]] < pinfo->y[extra[1]] )
//    {
//        pinfo->listlowtohighy[1] = extra[0];
//        pinfo->listlowtohighy[2] = extra[1];
//    }
//
//    return gfx_success;
//}

//--------------------------------------------------------------------------------------------
//gfx_rv gfx_make_projection_bitmap( renderlist_t * prlist, cam_corner_info_t * pinfo, projection_bitmap_t * pbmp )
//{
//    int cnt, grid_x, grid_y;
//    int row, run;
//    int leftedge[4], leftedge_count;
//    int rightedge[4], rightedge_count;
//    int x, center_dx, center_dy, center_x0, center_y0;
//    int corner_x[4], corner_y[4];
//    int corner_stt, corner_end;
//
//    ego_mesh_t       * pmesh;
//
//    if ( NULL == prlist )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
//
//        return gfx_error;
//    }
//
//    if ( NULL == pinfo )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL camera frustum data" );
//
//        return gfx_error;
//    }
//
//    if ( NULL == pbmp )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL projection data" );
//
//        return gfx_error;
//    }
//
//    pmesh = renderlist_get_pmesh( prlist );
//    if ( NULL == pmesh )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL mesh" );
//        return gfx_error;
//    }
//
//    // Make life simpler
//    for ( cnt = 0; cnt < 4; cnt++ )
//    {
//        corner_x[cnt] = pinfo->x[pinfo->listlowtohighy[cnt]];
//        corner_y[cnt] = pinfo->y[pinfo->listlowtohighy[cnt]];
//    }
//
//    // It works better this way...
//    corner_y[0] -= 128;
//    corner_y[3] += 128;
//
//    // Find the center line
//    center_dx = corner_x[3] - corner_x[0];
//    center_dy = corner_y[3] - corner_y[0];
//    center_x0 = corner_x[0];
//    center_y0 = corner_y[0];
//
//    if ( center_dy < 1 )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "the corner's y-coordinates are not sorted properly" );
//        return gfx_error;
//    }
//
//    // Find the points in each edge
//
//    // bottom
//    // corner 0 is on both the left and right edges
//    leftedge[0] = 0;  leftedge_count = 1;
//    rightedge[0] = 0;  rightedge_count = 1;
//
//    // middle
//    if ( corner_x[1] < ( ( center_dx * ( corner_y[1] - corner_y[0] ) ) / center_dy ) + center_x0 )
//    {
//        // corner 1 is on the left edge
//
//        leftedge[leftedge_count] = 1;
//        leftedge_count++;
//
//        corner_x[1] -= 512;
//    }
//    else
//    {
//        // corner 1 is on the right edge
//
//        rightedge[rightedge_count] = 1;
//        rightedge_count++;
//
//        corner_x[1] += 512;
//    }
//
//    // middle
//    if ( corner_x[2] < (( center_dx * ( corner_y[2] - corner_y[0] )) / center_dy ) + center_x0 )
//    {
//        // corner 2 is on the left edge
//
//        leftedge[leftedge_count] = 2;
//        leftedge_count++;
//
//        corner_x[2] -= 512;
//    }
//    else
//    {
//        // corner 2 is on the right edge
//
//        rightedge[rightedge_count] = 2;
//        rightedge_count++;
//
//        corner_x[2] += 512;
//    }
//
//    // top
//    // corner 3 is on the left and the right edge
//    leftedge[leftedge_count] = 3;  leftedge_count++;
//    rightedge[rightedge_count] = 3;  rightedge_count++;
//
//    // Make the left edge
//    row = 0;
//    for ( cnt = 1; cnt < leftedge_count && row < PROJECTION_BITMAP_ROWS; cnt++ )
//    {
//        int grid_x, x_stt, x_end;
//        int grid_y, y_stt, y_end;
//        int grid_y_stt, grid_y_end;
//
//        corner_stt = leftedge[cnt-1];
//        corner_end = leftedge[cnt];
//
//        y_stt = corner_y[corner_stt];
//        y_end = corner_y[corner_end];
//
//        x_stt = corner_x[corner_stt];
//        x_end = corner_x[corner_end];
//
//        edge_dy   = y_end - y_stt;
//        edge_dx   = x_end - x_stt;
//        edge_dx_step = edge_dx / edge_dy;
//
//        grid_y_stt = y_stt / GRID_ISIZE;
//        grid_y_end = y_end / GRID_ISIZE;
//
//        grid_x_stt = x_stt / GRID_ISIZE;
//        grid_x_end = x_end / GRID_ISIZE;
//
//        if( grid_y_stt < 0 )
//        {
//            grid_y_stt = 0;
//
//            // move the x starting point
//            grid_x_stt -= grid_y_stt * edge_step;
//        }
//
//        if( grid_y_end > pmesh->info.tiles_y )
//        {
//            grid_y_end = pmesh->info.tiles_y;
//        }
//
//        for ( grid_y = grid_y_stt; grid_y <= grid_y_end && row < PROJECTION_BITMAP_ROWS; grid_y++  )
//        {
//            if ( grid_y >= 0 && grid_y < pmesh->info.tiles_y )
//            {
//                grid_x = x / GRID_ISIZE;
//                pbmp->corner_stt[row] = CLIP( grid_x, 0, pmesh->info.tiles_x - 1 );
//                row++;
//            }
//        }
//    }
//    pbmp->row_cnt = row;
//
//    // Make the right edge ( rowrun )
//    grid_y = corner_y[0] / GRID_ISIZE;
//    row = 0;
//
//    for( cnt = 1; cnt < rightedge_count; cnt++ )
//    {
//        corner_stt = rightedge[cnt-1];
//        corner_end = rightedge[cnt];
//
//        x = corner_x[corner_stt] + 128;
//
//        edge_dy = corner_y[corner_end] - corner_y[corner_stt];
//        edge_dx = 0;
//        if ( edge_dy > 0 )
//        {
//            edge_dx = (( corner_x[corner_end] - corner_x[corner_stt] ) * GRID_ISIZE ) / edge_dy;
//        }
//
//        run = corner_y[corner_end] / GRID_ISIZE;
//
//        for ( /* nothing */; grid_y < run; grid_y++ )
//        {
//            if ( grid_y >= 0 && grid_y < pmesh->info.tiles_y )
//            {
//                grid_x = x / GRID_ISIZE;
//                pbmp->corner_end[row] = CLIP( grid_x, 0, pmesh->info.tiles_x - 1 );
//                row++;
//            }
//
//            x += edge_dx;
//        }
//    }
//
//    if ( pbmp->row_cnt != row )
//    {
//        log_warning( "ROW error (%i, %i)\n", pbmp->row_cnt, row );
//        return gfx_fail;
//    }
//
//    // set the starting y value
//    pbmp->y_stt = corner_y[0] / GRID_ISIZE;
//
//    return gfx_success;
//}

//--------------------------------------------------------------------------------------------
//gfx_rv gfx_make_renderlist( renderlist_t * prlist, const camera_t * pcam )
//{
//    /// @author ZZ
/// @details This function figures out which mesh fans to draw
//
//    int cnt, grid_x, grid_y;
//    int row;
//
//    gfx_rv              retval;
//    cam_corner_info_t   corner;
//    projection_bitmap_t proj;
//
//    ego_mesh_t * pmesh = NULL;
//
//
//    // Make sure there is a renderlist
//    if ( NULL == prlist )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "NULL renderlist pointer" );
//        return gfx_error;
//    }
//
//    // make sure the renderlist is attached to a mesh
//    pmesh = renderlist_get_pmesh( prlist );
//    if ( NULL == pmesh )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "renderlist is not attached to a mesh" );
//        return gfx_error;
//    }
//
//    // make sure that we have a valid camera
//    if ( NULL == pcam )
//    {
//        gfx_error_add( __FILE__, __FUNCTION__, __LINE__, 0, "cannot find a valid camera" );
//        return gfx_error;
//    }
//
//    // because the main loop of the program will always flip the
//    // page before rendering the 1st frame of the actual game,
//    // game_frame_all will always start at 1
//    if ( 1 != ( game_frame_all & 3 ) )
//    {
//        return gfx_success;
//    }
//
//    // Find the render area corners.
//    // if this fails, we cannot have a valid list of corners, so this function fails
//    if ( gfx_error == gfx_project_cam_view( pcam, &corner ) )
//    {
//        return gfx_error;
//    }
//
//    // assume the best
//    retval = gfx_success;
//
//    // reset the renderlist
//    if ( gfx_error == renderlist_reset( prlist ) )
//    {
//        retval = gfx_error;
//    }
//
//    // scan through each fan... not as fact as colliding with the BSP, but maybe fast enough
//    tile_count = pmesh->info.tiles_count;
//    for ( fan = 0; fan < tile_count; fan++ )
//    {
//        inview = egolib_frustum_intersects_ego_aabb( &(pcam->frustum), mm->tilelst[fan].bbox.mins.v, mm->tilelst[fan].bbox.maxs.v );
//
//    }
//
//    // get the actual projection "bitmap"
//    gfx_make_projection_bitmap( prlist, &corner, &proj );
//
//    // fill the renderlist from the projected view
//    grid_y = proj.y_stt;
//    grid_y = CLIP( grid_y, 0, pmesh->info.tiles_y - 1 );
//    for ( row = 0; row < proj.row_cnt; row++, grid_y++ )
//    {
//        for ( grid_x = proj.row_stt[row]; grid_x <= proj.row_end[row] && prlist->all.count < MAXMESHRENDER; grid_x++ )
//        {
//            cnt = pmesh->gmem.tilestart[grid_y] + grid_x;
//
//            if ( gfx_error == gfx_capture_mesh_tile( ego_mesh_get_ptile( pmesh, cnt ) ) )
//            {
//                retval = gfx_error;
//                goto gfx_make_renderlist_exit;
//            }
//
//            if ( gfx_error == renderlist_insert( prlist, ego_mesh_get_pgrid( pmesh, cnt ), cnt ) )
//            {
//                retval = gfx_error;
//                goto gfx_make_renderlist_exit;
//            }
//        }
//    }
//
//gfx_make_renderlist_exit:
//    return retval;
//}
