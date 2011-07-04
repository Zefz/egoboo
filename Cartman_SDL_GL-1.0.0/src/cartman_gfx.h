#include <SDL.h>

#include "SDL_extensions.h"
#include "ogl_extensions.h"

#include "ogl_texture.h"

#include "cartman_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_window;
struct s_camera;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_ogl_surface
{
    GLint viewport[4];
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXTILE 256             //

#define DAMAGENULL          255                        //

enum e_damage_type
{
    DAMAGE_SLASH = 0,                        //
    DAMAGE_CRUSH,                            //
    DAMAGE_POKE,                             //
    DAMAGE_HOLY,                             // (Most invert Holy damage )
    DAMAGE_EVIL,                             //
    DAMAGE_FIRE,                             //
    DAMAGE_ICE,                              //
    DAMAGE_ZAP,                              //
    DAMAGE_COUNT                             // Damage types
};

#define DAMAGECHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGEINVERT        4                       // 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       // 000000xx Resistance ( 1 is common )
#define DAMAGETILETIME      32                      // Invincibility time
#define DAMAGETIME          16                      // Invincibility time
#define DEFENDTIME          16                      // Invincibility time

#define OGL_MAKE_COLOR_3(COL, BB,GG,RR) { COL[0] = RR / 32.0f; COL[1] = GG / 32.0f; COL[2] = BB / 32.0f; }
#define OGL_MAKE_COLOR_4(COL, AA,BB,GG,RR) { COL[0] = RR / 32.0f; COL[1] = GG / 32.0f; COL[2] = BB / 32.0f; COL[3] = AA / 32.0f; }

#define MAKE_BGR(BMP,BB,GG,RR)     SDL_MapRGBA(BMP->format, (RR)<<3, (GG)<<3, (BB)<<3, 0xFF)
#define MAKE_ABGR(BMP,AA,BB,GG,RR) SDL_MapRGBA(BMP->format, (RR)<<3, (GG)<<3, (BB)<<3, AA)

#define POINT_SIZE(X) ( (X) * 0.5f + 4.0f )
#define MAXPOINTSIZE 16.0f

#define DEFAULT_Z_SIZE ( 180 << 4 )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern SDL_Surface * theSurface;
extern SDL_Surface * bmphitemap;        // Heightmap image

extern glTexture     tx_point;      // Vertex image
extern glTexture     tx_pointon;    // Vertex image ( select_vertsed )
extern glTexture     tx_ref;        // Meshfx images
extern glTexture     tx_drawref;    //
extern glTexture     tx_anim;       //
extern glTexture     tx_water;      //
extern glTexture     tx_wall;       //
extern glTexture     tx_impass;     //
extern glTexture     tx_damage;     //
extern glTexture     tx_slippy;     //

extern glTexture     tx_smalltile[MAXTILE]; // Tiles
extern glTexture     tx_bigtile[MAXTILE];   //
extern glTexture     tx_tinysmalltile[MAXTILE]; // Plan tiles
extern glTexture     tx_tinybigtile[MAXTILE];   //

extern int     numsmalltile;   //
extern int     numbigtile;     //

extern SDLX_video_parameters_t sdl_vparam;
extern oglx_video_parameters_t ogl_vparam;

extern int    animtileupdateand;                      // New tile every ( (1 << n) - 1 ) frames
extern Uint16 animtileframeand;                       // 1 << n frames
extern Uint16 animtilebaseand;
extern Uint16 biganimtileframeand;                    // 1 << n frames
extern Uint16 biganimtilebaseand;
extern Uint16 animtileframeadd;

extern Sint16 damagetileparttype;
extern short  damagetilepartand;
extern short  damagetilesound;
extern short  damagetilesoundtime;
extern Uint16 damagetilemindistance;
extern int    damagetileamount;                           // Amount of damage
extern Uint8  damagetiletype;                      // Type of damage

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// misc
SDL_Color MAKE_SDLCOLOR( Uint8 BB, Uint8 RR, Uint8 GG );

// make a bitmap of the mesh
void make_hitemap( void );
void make_planmap( void );

// tile rendering routines
void draw_top_fan( struct s_window * pwin, int fan, float zoom_hrz );
void draw_side_fan( struct s_window * pwin, int fan, float zoom_hrz, float zoom_vrt );
void draw_schematic( struct s_window * pwin, int fantype, int x, int y );
void draw_top_tile( float x0, float y0, int fan, glTexture * tx_tile, bool_t draw_tile );
void draw_tile_fx( float x, float y, Uint8 fx, float scale );

// ogl routines
void ogl_draw_sprite_2d( glTexture * img, float x, float y, float width, float height );
void ogl_draw_sprite_3d( glTexture * img, cart_vec_t pos, cart_vec_t vup, cart_vec_t vright, float width, float height );
void ogl_draw_box( float x, float y, float w, float h, float color[] );
void ogl_beginFrame();
void ogl_endFrame();

// SDL routines
void draw_sprite( SDL_Surface * dst, SDL_Surface * sprite, int x, int y );
int cartman_BlitScreen( SDL_Surface * bmp, SDL_Rect * prect );
SDL_Surface * cartman_CreateSurface( int w, int h );
int cartman_BlitSurface( SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect );
SDL_Surface * cartman_LoadIMG( const char * szName );

// camera stuff
void cartman_begin_ortho_camera_hrz( struct s_window * pwin, struct s_camera * pcam, float zoom_x, float zoom_y );
void cartman_begin_ortho_camera_vrt( struct s_window * pwin, struct s_camera * pcam, float zoom_x, float zoom_z );
void cartman_end_ortho_camera( );

// setup
void create_imgcursor( void );
void load_img( void );
void get_tiles( SDL_Surface* bmpload );

// misc
glTexture * tiny_tile_at( int x, int y );
glTexture * tile_at( int fan );