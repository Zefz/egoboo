/* Egoboo - egoboo.h
 * Disgusting, hairy, way too monolithic header file for the whole darn
 * project.  In severe need of cleaning up.  Venture here with extreme
 * caution, and bring one of those canaries with you to make sure you
 * don't run out of oxygen.
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
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _EGOBOO_H_
#define _EGOBOO_H_

/* Typedefs for various platforms */
#include "egobootypedef.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#include <SDL_opengl.h>

#include "proto.h"
#include "gltexture.h" /* OpenGL texture loader */
#include "mathstuff.h" /* vector and matrix math */
#include "configfile.h"
#include "Md2.h"

typedef struct glvertex_t
{
  GLfloat x, y, z, w;
  GLfloat nx, ny, nz;
  GLfloat r, g, b, a;
  Uint32 color; // should replace r,g,b,a and be called by glColor4ubv
  GLfloat s, t; // u and v in D3D I guess
} GLVERTEX;

// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#define EXTERN
#define EQ(x) =x;
#else
#define EXTERN extern
#define EQ(x)
#endif

EXTERN char					VERSION[] EQ("2.7.5");  // Version of the game

#define MAXMODULE           100                     // Number of modules
#define TITLESIZE           128                     // Size of a title image
#define MAXSEQUENCE         256                     // Number of tracks in sequence
#define MAXIMPORT           (36*8)                  // Number of subdirs in IMPORT directory

#define NOSPARKLE           255
#define ANYTIME             255                     // Code for gs->respawnvalid...

#define SIZETIME            50                      // Time it takes to resize a character
#define WATCHMIN            .01                     //

#define PRTLEVELFIX         20                      // Fix for shooting over cliffs
#define PACKDELAY           25                      // Time before inventory rotate again
#define GRABDELAY           25                      // Time before grab again
#define NOSKINOVERRIDE      -1                      // For import

#define PITDEPTH            -30                     // Depth to kill character
#define PITNOSOUND          -256                    // Stop sound at bottom of pits...

EXTERN Uint16  endtextindex[8];

/*
#define MAXSPEECH           6
#define SPEECHMOVE          0
#define SPEECHMOVEALT       1
#define SPEECHATTACK        2
#define SPEECHASSIST        3
#define SPEECHTERRAIN       4
#define SPEECHSELECT        5
*/

#define EXPKEEP 0.85                                // Experience to keep when respawning

#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4

#define EDGE                128                     // Camera bounds
#define FARTRACK            1200                    // For outside modules...
#define EDGETRACK           800                     // Camtrack bounds

#define MAXNUMINPACK        6                       // Max number of items to carry in pack

#define NOHIDE              127                     // Don't hide

#define THROWFIX            30.0                    // To correct thrown velocities
#define MINTHROWVELOCITY    15.0                    //
#define MAXTHROWVELOCITY    45.0                    //


#define LOWSTAT             256                     // Worst...
#define PERFECTSTAT         (75*256)                // Perfect...
#define PERFECTBIG          (127*256)               // Perfect life or mana...
#define HIGHSTAT            (99*256)                // Absolute MAX strength...
#define MINDAMAGE           256                     // Minimum damage for hurt animation
#define MSGDISTANCE         2000                    // Range for SendMessageNear

#define DAMFXNONE           0                       // Damage effects
#define DAMFXARMO           1                       // Armor piercing
#define DAMFXBLOC           2                       // Cannot be blocked by shield
#define DAMFXARRO           4                       // Only hurts the one it's attached to
#define DAMFXTURN           8                       // Turn to attached direction
#define DAMFXTIME           16                      //


#define ULTRABLOODY         2                       // This makes any damage draw blood
#define SPELLBOOK           127                     // The spellbook model TODO: change this badly thing

#define GENFEMALE           0                       // Gender
#define GENMALE             1                       //
#define GENOTHER            2                       //
#define GENRANDOM           3                       //

#define RETURNAND           63                      // Return mana every so often
#define MANARETURNSHIFT     4                       //
#define HURTDAMAGE          (1*256)                 //

#define MAXPASS             256                     // Maximum number of passages ( mul 32 )
#define MAXSTAT             16                      // Maximum status displays
#define MAXLOCO             3                       // Maximum number of local players

#define JOYBUTTON           8                       // Maximum number of joystick buttons
#define MAXMESSAGE          6                       // Number of messages
#define MAXTOTALMESSAGE     1024                    //
#define MESSAGESIZE         80                      //
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)
#define MESSAGETIME         200                     // Time to keep the message alive
#define TABAND              31                      // Tab size

#define MADFXINVICTUS       1                       // I Invincible
#define MADFXACTLEFT        2                       // AL Activate left item
#define MADFXACTRIGHT       4                       // AR Activate right item
#define MADFXGRABLEFT       8                       // GL GO Grab left/Grab only item
#define MADFXGRABRIGHT      16                      // GR Grab right item
#define MADFXDROPLEFT       32                      // DL DO Drop left/Drop only item
#define MADFXDROPRIGHT      64                      // DR Drop right item
#define MADFXSTOP           128                     // S Stop movement
#define MADFXFOOTFALL       256                     // F Footfall sound
#define MADFXCHARLEFT       512                     // CL Grab left/Grab only character
#define MADFXCHARRIGHT      1024                    // CR Grab right character
#define MADFXPOOF           2048                    // P Poof


#define GRABSIZE            90.0                    // Grab tolerance

#define LIPDA               0                       // For smooth transitions 'tween
#define LIPWA               1                       //   walking rates
#define LIPWB               2                       //
#define LIPWC               3                       //

#define NULLICON            0                       // Empty hand image

#define MAXPRTPIP           1024                    // Particle templates
#define MAXPRTPIPPEROBJECT  8                       //
#define COIN1               0                       // Coins are the first particles loaded
#define COIN5               1                       //
#define COIN25              2                       //
#define COIN100             3                       //
#define WEATHER4            4                       // Weather particles
#define WEATHER5            5                       // Weather particle finish
#define SPLASH              6                       // Water effects are next
#define RIPPLE              7                       //
#define DEFEND              8                       // Defend particle
#define RIPPLEAND           15                      // How often ripples spawn
#define RIPPLETOLERANCE     60                      // For deep water
#define SPLASHTOLERANCE     10                      //
#define CLOSETOLERANCE      2                       // For closing doors


#define MAXDAMAGETYPE       8                       // Damage types
#define DAMAGENULL          255                     //
#define DAMAGESLASH         0                       //
#define DAMAGECRUSH         1                       //
#define DAMAGEPOKE          2                       //
#define DAMAGEHOLY          3                       // (Most invert Holy damage )
#define DAMAGEEVIL          4                       //
#define DAMAGEFIRE          5                       //
#define DAMAGEICE           6                       //
#define DAMAGEZAP           7                       //
#define DAMAGECHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGEINVERT        4                       // 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       // 000000xx Resistance ( 1 is common )
#define DAMAGETILETIME      32                      // Invincibility time
#define DAMAGETIME          16                      // Invincibility time
#define DEFENDTIME          16                      // Invincibility time
#define DROPXYVEL           8                       //
#define DROPZVEL            7                       //
#define JUMPATTACKVEL       -2                      //
#define WATERJUMP           12                      //

#define MAXSTOR             8                       // Storage data
#define STORAND             7                       //

#define MAXWAY              8                       // Waypoints
#define WAYTHRESH           128                     // Threshold for reaching waypoint
#define AISMAXCOMPILESIZE   (128*4096/4)            // For parsing AI scripts
#define MAXLINESIZE         1024                    //
#define MAXAI               129                     //
#define MAXCODE             1024                    // Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     64                      //

#define MAXCAPNAMESIZE      32                      // Character class names
#define MAXLEVEL            6                       // Levels 0-5
#define MAXEXPERIENCETYPE   8                       // Number of ways to get experience
#define MAXIDSZ             6                       // ID strings per character
#define IDSZNONE            MAKE_IDSZ("NONE")       // [NONE]
#define IDSZPARENT          0                       // Parent index
#define IDSZTYPE            1                       // Self index
#define IDSZSKILL           2                       // Skill index
#define IDSZSPECIAL         3                       // Special index
#define IDSZHATE            4                       // Hate index
#define IDSZVULNERABILITY   5                       // Vulnerability index


#define XPFINDSECRET        0                       // Finding a secret
#define XPWINQUEST          1                       // Beating a module or a subquest
#define XPUSEDUNKOWN        2                       // Used an unknown item
#define XPKILLENEMY         3                       // Killed an enemy
#define XPKILLSLEEPY        4                       // Killed a sleeping enemy
#define XPKILLHATED         5                       // Killed a hated enemy
#define XPTEAMKILL          6                       // Team has killed an enemy
#define XPTALKGOOD          7                       // Talk good, er...  I mean well
#define XPDIRECT            255                     // No modification


#define MAXTEAM             27                      // Teams A-Z, +1 more for damage tiles
#define DAMAGETEAM          26                      // For damage tiles
#define EVILTEAM            4                       // E
#define GOODTEAM            6                       // G
#define NULLTEAM            13                      // N


#define NOACTION            0xffff                  // Action not valid for this character
#define MAXACTION           76                      // Number of action types
EXTERN char    cActionName[MAXACTION][2];                  // Two letter name code

#define TURNMODEVELOCITY    0                       // Character gets rotation from velocity
#define TURNMODEWATCH       1                       // For watch towers
#define TURNMODESPIN        2                       // For spinning objects
#define TURNMODEWATCHTARGET 3                       // For combat intensive AI
#define SPINRATE            200                     // How fast spinners spin
#define FLYDAMPEN           .001                    // Levelling rate for flyers

#define LATCHBUTTONLEFT      1                      // Character button presses
#define LATCHBUTTONRIGHT     2                      //
#define LATCHBUTTONJUMP      4                      //
#define LATCHBUTTONALTLEFT   8                      // ( Alts are for grab/drop )
#define LATCHBUTTONALTRIGHT  16                     //
#define LATCHBUTTONPACKLEFT  32                     // ( Packs are for inventory cycle )
#define LATCHBUTTONPACKRIGHT 64                     //
#define LATCHBUTTONRESPAWN   128                    //

#define JUMPINFINITE        255                     // Flying character
#define JUMPDELAY           20                      // Time between jumps
#define JUMPTOLERANCE       20                      // Distance above ground to be jumping
#define SLIDETOLERANCE      10                      // Stick to ground better
#define PLATTOLERANCE       50 //5 //10             // Platform tolerance...
#define PLATADD             -10                     // Height add...
#define PLATASCEND          .10                     // Ascension rate
#define PLATKEEP            .90                     // Retention rate

#define MAPID 0x4470614d                      // The string... MapD

#define RAISE 12 //25                               // Helps correct z level
#define SHADOWRAISE 5                               //
#define DAMAGERAISE 25                              //

#define MAXWATERLAYER 2                             // Maximum water layers
#define MAXWATERFRAME 512                           // Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)             //
#define WATERPOINTS 4                               // Points in a water fan
#define WATERMODE 4                                 // Ummm...  For making it work, yeah...

#define DONTFLASH 255                               //

#define FOV                             60          // Field of view
#define ROTMESHTOPSIDE                  55          // For figuring out what to draw
#define ROTMESHBOTTOMSIDE               65          //
#define ROTMESHUP                       40 //35          //
#define ROTMESHDOWN                     60          //
EXTERN int rotmeshtopside;                                 // The ones that get used
EXTERN int rotmeshbottomside;                              //
EXTERN int rotmeshup;                                      //
EXTERN int rotmeshdown;                                    //

#define NUMFONTX                        16          // Number of fonts in the bitmap
#define NUMFONTY                        6           //
#define NUMFONT                         (NUMFONTX*NUMFONTY)
#define FONTADD                         4           // Gap between letters
#define NUMBAR                          6           // Number of status bars
#define TABX                            32//16      // Size of little name tag on the bar
#define BARX                            112//216         // Size of bar
#define BARY                            16//8           //
#define NUMTICK                         10//50          // Number of ticks per row
#define TICKX                           8//4           // X size of each tick
#define MAXTICK                         (NUMTICK*5) // Max number of ticks to draw

#define TURNSPD                         .01         // Cutoff for turning or same direction
#define CAMKEYTURN                      10          // Keyboard camera rotation
#define CAMJOYTURN                      10         // Joystick camera rotation


// Multi cam
#define MINZOOM                         500         // Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         800         // Camera height
#define MAXZADD                         1500  //1000
#define MINUPDOWN                       (.18*PI)//(.24*PI)    // Camera updown angle
#define MAXUPDOWN                       (.18*PI)


#define MD2START                        0x32504449  // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (512*1024)  // Don't load any models bigger than 512k
#define MD2LIGHTINDICES                 163//162    // MD2's store vertices as x,y,z,normal
#define EQUALLIGHTINDEX                 162         // I added an extra index to do the
// spikey mace...

#define MAXTEXTURE                      512         // Max number of textures
#define MAXVERTICES                     512    //128     // Max number of points in a model
#define MAXCOMMAND                      256         // Max number of commands
#define MAXCOMMANDSIZE                  64          // Max number of points in a command
#define MAXCOMMANDENTRIES               512//256         // Max entries in a command list ( trigs )
#define MAXMODEL                        256         // Max number of models
#define MAXEVE                          MAXMODEL    // One enchant type per model
#define MAXEVESETVALUE                  24          // Number of sets
#define MAXEVEADDVALUE                  16          // Number of adds
#define MAXENCHANT                      128         // Number of enchantments
#define MAXFRAME                        (128*32)    // Max number of frames in all models
#define MAXCHR                          350         // Max number of characters
//#define MAXCHRBIT                       (256>>5)    // Bitwise compression
#define MAXLIGHTLEVEL                   16          // Number of premade light intensities
#define MAXSPEKLEVEL                    16          // Number of premade specularities
#define MAXLIGHTROTATION                256         // Number of premade light maps
#define MAXPRT                          512         // Max number of particles
#define MAXPARTICLEIMAGE                272         // Number of particle images ( frames )
#define MAXDYNA                         8           // Number of GDyna.mic lights
#define MAXDYNADIST                     2700        // Leeway for offscreen lights
#define GRIPVERTICES                    8           // Each model has 8 grip vertices
#define GRIPRIGHT                       8           // Right weapon grip starts 8 from last
#define GRIPLEFT                        4           // Left weapon grip starts 4 from last
#define GRIPONLY                        4           // Only weapon grip starts 4 from last
#define SPAWNORIGIN                     0           // Center for spawn attachments
#define SPAWNLAST                       1           // Position for spawn attachments
#define INVENTORY                       0           //

#define CHOPPERMODEL                    32          //
#define MAXSECTION                      4           // T-wi-n-k...  Most of 4 sections
#define MAXCHOP                         (MAXMODEL*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)

#define MAXMESHFAN                      (512*512)   // Terrain mesh size
#define MAXMESHSIZEY                    1024        // Max fans in y direction
#define BYTESFOREACHVERTEX              14          // 14 bytes each
#define MAXMESHVERTICES                 16          // Fansquare vertices
#define MAXMESHTYPE                     64          // Number of fansquare command types
#define MAXMESHCOMMAND                  4           // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32          // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32          // Max trigs in each command
#define MAXTILETYPE                     256         // Max number of tile images
#define MAXMESHRENDER                   1024        // Max number of tiles to draw
#define FANOFF                          0xffff      // Don't draw the fansquare if tile = this
#define OFFEDGE                         0           // Character not on a fan ( maybe )

#define MESHFXREF                       0           // 0 This tile is drawn 1st
#define MESHFXSHA                       1           // 0 This tile is drawn 2nd
#define MESHFXDRAWREF                   2           // 1 Draw reflection of characters
#define MESHFXANIM                      4           // 2 Animated tile ( 4 frame )
#define MESHFXWATER                     8           // 3 Render water above surface ( Water details are set per module )
#define MESHFXWALL                      16          // 4 Wall ( Passable by ghosts, particles )
#define MESHFXIMPASS                    32          // 5 Impassable
#define MESHFXDAMAGE                    64          // 6 Damage
#define MESHFXSLIPPY                    128         // 7 Ice or normal


#define SLOPE                           800         // Slope increments for terrain normals
#define SLIDE                           .04         // Acceleration for steep hills
#define SLIDEFIX                        .08         // To make almost flat surfaces flat

#define PRTLIGHTSPRITE                  0           // Magic effect particle
#define PRTSOLIDSPRITE                  1           // Sprite particle
#define PRTALPHASPRITE                  2           // Smoke particle

/* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000

#define ANCIENTFPS                      30.0f
#define TARGETFPS                       50.0f
#define FRAMESCALE                      (stabilizedfps/ANCIENTFPS)
#define FRAMESKIP                       (TICKS_PER_SEC/TARGETFPS)    // 1000 tics per sec / 50 fps = 20 ticks per frame
#define ONESECOND                       (TICKS_PER_SEC/FRAMESKIP)    // 1000 tics per sec / 20 ticks per frame = 50 fps

#define STOPBOUNCING                    0.1 //1.0         // To make objects stop bouncing
#define STOPBOUNCINGPART                5.0         // To make particles stop bouncing

#define TRANSCOLOR                      0           // Transparent color
#define BORETIME                        (ego_rand(&ego_rand_seed)&255)+120
#define CAREFULTIME                     50
#define REEL                            7600.0      // Dampen for melee knock back
#define REELBASE                        .35         //

/* PORT
GUID FAR* enum_id;                                  // Ben's Voodoo search
int enum_nonnull EQ(0);                                 //
char enum_desc[100];                                //
*/


typedef struct tile_anim_t
{
  int    updateand;          // New tile every 7 frames
  Uint16 frameand;           // Only 4 frames
  Uint16 baseand;            //
  Uint16 frameand_big;       // For big tiles
  Uint16 baseand_big;        //
  Uint16 frameadd;           // Current frame
} TILE_ANIM;

extern TILE_ANIM GTile_Anim;

EXTERN Uint16 bookicon  EQ(0);                        // The first book icon

#define NORTH 16384                                 // Character facings
#define SOUTH 49152                                 //
#define EAST 32768                                  //
#define WEST 0                                      //
#define FRONT 0                                     // Attack directions
#define BEHIND 32768                                //
#define LEFT 49152                                  //
#define RIGHT 16384                                 //


#define MAXXP    ((1<<30)-1)                               // Maximum experience (Sint32 32)
#define MAXMONEY 9999                                      // Maximum money
#define NOLEADER 65535                                     // If the team has no leader...

typedef struct team_t
{
  Uint8  hatesteam[MAXTEAM];     // Don't damage allies...
  Uint16 morale;                 // Number of characters on team
  Uint16 leader;                 // The leader of the team
  Uint16 sissy;                  // Whoever called for help last
} TEAM;

EXTERN TEAM TeamList[MAXTEAM];

typedef struct tile_damage_t
{
  int   amount;                             // Amount of damage
  Uint8 type;                        // Type of damage
  short parttype;
  short partand;
  short sound;
  short soundtime;
  int   mindistance;
} TILE_DAMAGE;

extern TILE_DAMAGE GTile_Dam;

#define TILESOUNDTIME 16
#define TILEREAFFIRMAND  3


//Minimap stuff
#define MAXBLIP 32
EXTERN Uint16          numblip  EQ(0);

typedef struct blip_t
{
  Uint16  x;
  Uint16  y;
  Uint8   c;
  rect_t  rect;           // The blip rectangles
} BLIP;

EXTERN BLIP BlipList[MAXBLIP];

EXTERN Uint8           mapon  EQ(bfalse);
EXTERN Uint8           youarehereon  EQ(bfalse);


EXTERN Uint8           timeron  EQ(bfalse);            // Game timer displayed?
EXTERN Uint32            timervalue  EQ(0);             // Timer time ( 50ths of a second )
EXTERN float           stabilizedfps EQ(TARGETFPS);
EXTERN char                    szfpstext[]  EQ("000 FPS");
EXTERN Sint32              sttclock;                   // GetTickCount at start
EXTERN Sint32              allclock  EQ(0);               // The total number of ticks so far
EXTERN Sint32              lstclock  EQ(0);               // The last total of ticks so far
EXTERN Sint32              wldclock  EQ(0);               // The sync clock
EXTERN Sint32              fpsclock  EQ(0);               // The number of ticks this second
EXTERN Uint32            wldframe  EQ(0);               // The number of frames that should have been drawn
EXTERN Uint32            allframe  EQ(0);               // The total number of frames drawn so far
EXTERN Uint32            fpsframe  EQ(0);               // The number of frames drawn this second
EXTERN Uint8           pitclock  EQ(0);               // For pit kills
EXTERN Uint8           pitskill  EQ(bfalse);            // Do they kill?
EXTERN Uint8 outofsync  EQ(0);   //Is this only for RTS? Can it be removed then?
EXTERN Uint8 parseerror  EQ(0);      //Do we have an script error?



//EXTERN bool_t                    menuaneeded  EQ(bfalse);         // Give them MENUA?
//EXTERN bool_t                    menuactive  EQ(bfalse);         // Menu running?

#define TURNTIME 16

EXTERN char            pickedmodule[64];           // The module load name

typedef struct net_message_t
{
  Uint8  delay;            // For slowing down input
  int    write;            // The cursor position
  int    writemin;         // The starting cursor position
  char   buffer[MESSAGESIZE];    // The input message
} NET_MESSAGE;

EXTERN NET_MESSAGE GNetMsg;

// JF - Added so that the video mode might be determined outside of the graphics code
extern SDL_Surface *displaySurface;

//Networking
EXTERN int                     localmachine  EQ(0);           // 0 is host, 1 is 1st remote, 2 is 2nd...
EXTERN int                     numimport;                  // Number of imports from this machine
EXTERN Uint8           localcontrol[16];           // For local imports
EXTERN short                   localslot[16];              // For local imports

//Setup values
EXTERN int                     cursorx  EQ(0);                // Cursor position
EXTERN int                     cursory  EQ(0);                //
EXTERN bool_t                    pressed EQ(0);                  //
EXTERN bool_t                    clicked EQ(0);                  //
EXTERN bool_t         pending_click EQ(0);
// EWWWW. GLOBALS ARE EVIL.

//Input Control
typedef struct mouse_t
{
  bool_t          on;                // Is the mouse alive?
  int             icon;
  int             player;
  float           sense;             // Sensitivity threshold
  float           sustain;           // Falloff rate for old movement
  float           cover;             // For falloff
  int             dx;                 // Mouse X movement counter
  int             dy;                 // Mouse Y movement counter
  int             x;                 // Mouse X movement counter
  int             y;                 // Mouse Y movement counter
  Sint32          z;                 // Mouse wheel movement counter
  Uint8           b;                 // Button mask
  float           latcholdx;         // For sustain
  float           latcholdy;         //
  Uint8           button[4];             // Mouse button states
} MOUSE;

extern MOUSE GMous;

typedef struct keyboard_t
{
  bool_t    on;                  // Is the GKeyb.board alive?
  int       icon;
  int       player;

  //PORT: Use sdlkeybuffer instead.
  //char    buffer[256];             // Keyboard key states
  //char    press[256];              // Keyboard new hits
} KEYBOARD;

extern KEYBOARD GKeyb;
#define KEYDOWN(k) (sdlkeybuffer[k])      // Helper for gettin' em

typedef struct joystick_t
{
  bool_t          on;                 // Is the holy joystick alive?
  int             icon;
  int             player;
  float           x;                  // Joystick A
  float           y;                  //
  Uint8           b;                  // Button mask
  Uint8           button[JOYBUTTON];  //
} JOYSTICK;

extern JOYSTICK GJoy[2];


//Weather and water gfx
typedef struct weather_t
{
  int overwater;         // Only spawn over water?
  int timereset;         // Rate at which weather particles spawn
  int time;              // 0 is no weather
  int player;
} WEATHER;

extern WEATHER GWeather;

EXTERN int                     numwaterlayer EQ(0);                // Number of layers
EXTERN Uint8     watershift  EQ(3);
EXTERN float                   watersurfacelevel EQ(0);            // Surface level for water striders
EXTERN float                   waterdouselevel EQ(0);              // Surface level for torches
EXTERN Uint8           waterlight EQ(0);                   // Is it light ( default is alpha )
EXTERN Uint8           waterspekstart EQ(128);             // Specular begins at which light value
EXTERN Uint8           waterspeklevel EQ(128);             // General specular amount (0-255)
EXTERN Uint8           wateriswater  EQ(btrue);            // Is it water?  ( Or lava... )
EXTERN Uint8           waterlightlevel[MAXWATERLAYER]; // General light amount (0-63)
EXTERN Uint8           waterlightadd[MAXWATERLAYER];   // Ambient light amount (0-63)
EXTERN float                   waterlayerz[MAXWATERLAYER];     // Base height of water
EXTERN Uint8           waterlayeralpha[MAXWATERLAYER]; // Transparency
EXTERN float                   waterlayeramp[MAXWATERLAYER];   // Amplitude of waves
EXTERN float                   waterlayeru[MAXWATERLAYER];     // Coordinates of texture
EXTERN float                   waterlayerv[MAXWATERLAYER];     //
EXTERN float                   waterlayeruadd[MAXWATERLAYER];  // Texture movement
EXTERN float                   waterlayervadd[MAXWATERLAYER];  //
EXTERN float                   waterlayerzadd[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN Uint8           waterlayercolor[MAXWATERLAYER][MAXWATERFRAME][WATERMODE][WATERPOINTS];
EXTERN Uint16          waterlayerframe[MAXWATERLAYER]; // Frame
EXTERN Uint16          waterlayerframeadd[MAXWATERLAYER];      // Speed
EXTERN float                   waterlayerdistx[MAXWATERLAYER];         // For distant backgrounds
EXTERN float                   waterlayerdisty[MAXWATERLAYER];         //
EXTERN Uint32                  waterspek[256];             // Specular highlights
EXTERN float                   foregroundrepeat  EQ(1);       //
EXTERN float                   backgroundrepeat  EQ(1);       //

// Global lighting stuff
EXTERN float                   lightspekx EQ(0);
EXTERN float                   lightspeky EQ(0);
EXTERN float                   lightspekz EQ(0);
EXTERN float                   lightspek  EQ(0);
EXTERN float                   lightambi  EQ(0);


//Fog stuff
typedef struct fog_info_t
{
  Uint8           on;              // Do ground fog?
  float           bottom;          //
  float           top;             //
  float           distance;        //
  Uint8           red;             //  Fog collour
  Uint8           grn;             //
  Uint8           blu;             //
  Uint8           affectswater;
} FOG_INFO;

extern FOG_INFO GFog;


//Camera control stuff
typedef struct camera_t
{
  int             swing;               // Camera swingin'
  int             swingrate;           //
  float           swingamp;            //
  float           x;                   // Camera position
  float           y;                //
  float           z;                 // 500-1000
  float           zoom;             // Distance from the trackee
  float           zadd;              // Camera height above terrain
  float           zaddgoto;          // Desired z position
  float           zgoto;             //
  float           turnleftright;      // Camera rotations
  float           turnleftrightone;
  Uint16          turnleftrightshort;
  float           turnadd;             // Turning rate
  float           sustain;           // Turning rate falloff
  float           turnupdown;
  float           roll;                //
  float           trackxvel;               // Change in trackee position
  float           trackyvel;               //
  float           trackzvel;               //
  float           centerx;                 // Move character to side before tracking
  float           centery;                 //
  float           trackx;                  // Trackee position
  float           tracky;                  //
  float           trackz;                  //
  float           tracklevel;              //
} CAMERA;

extern CAMERA GCamera;


EXTERN float                   cornerx[4];                 // Render area corners
EXTERN float                   cornery[4];                 //
EXTERN int                     cornerlistlowtohighy[4];    // Ordered list
EXTERN int                     cornerlowx;                 // Render area extremes
EXTERN int                     cornerhighx;                //
EXTERN int                     cornerlowy;                 //
EXTERN int                     cornerhighy;                //
EXTERN int                     fontoffset;                 // Line up fonts from top of screen


/*OpenGL Textures*/
EXTERN  GLTexture       TxIcon[MAXTEXTURE+1];        /* icons */
EXTERN  GLTexture       TxTitleImage[MAXMODULE];   /* title images */
EXTERN  GLTexture       TxTrimX;                     /* trim horiz */
EXTERN  GLTexture       TxTrimY;                     /* trim vert  */
EXTERN  GLTexture       TxTrim;                      /* trim block */
EXTERN  GLTexture       TxFont;                      /* font */
EXTERN  GLTexture       TxBars;                      /* status bars */
EXTERN  GLTexture       TxBlip;                      /* you are here texture */
EXTERN  GLTexture       TxMap;                       /* the map texture */
EXTERN  GLTexture       txTexture[MAXTEXTURE];       /* All textures */
EXTERN  Sint32          tx_index;                    /* index to top of ferr list */
EXTERN  Uint32          txFree[MAXTEXTURE];          /* list of free textures */

//Texture filtering
typedef enum tx_filters_e
{
  TX_UNFILTERED,
  TX_LINEAR,
  TX_MIPMAP,
  TX_BILINEAR,
  TX_TRILINEAR_1,
  TX_TRILINEAR_2,
  TX_ANISOTROPIC
} TX_FILTERS;

//Anisotropic filtering - yay! :P
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
EXTERN float maxAnisotropy;          // Max anisotropic filterings (Between 1.00 and 16.00)
EXTERN int   userAnisotropy;           // Requested anisotropic level
EXTERN int   log2Anisotropy;                  // Max levels of anisotropy

EXTERN bool_t      video_mode_chaged EQ(bfalse);
EXTERN SDL_Rect ** video_mode_list EQ(NULL);

EXTERN GLMATRIX mWorld;           // World Matrix
EXTERN GLMATRIX mView;           // View Matrix
EXTERN GLMATRIX mViewSave;          //View Matrix initial state
EXTERN GLMATRIX mProjection;         //Projection Matrix


//Input player control
EXTERN int                     numloadplayer  EQ(0);
#define MAXLOADPLAYER   100
EXTERN char                    loadplayername[MAXLOADPLAYER][MAXCAPNAMESIZE];
EXTERN char                    loadplayerdir[MAXLOADPLAYER][16];
EXTERN int                     nullicon  EQ(0);


//Interface stuff
#define TRIMX 640
#define TRIMY 480
EXTERN rect_t                    iconrect;                   // The 32x32 icon rectangle
EXTERN rect_t                    trimrect;                   // The menu trim rectangle
EXTERN SDL_Rect      fontrect[NUMFONT];          // The font rectangles
EXTERN Uint8    fontxspacing[NUMFONT];      // The spacing stuff
EXTERN Uint8    fontyspacing;               //
EXTERN rect_t                    tabrect[NUMBAR];            // The tab rectangles
EXTERN rect_t                    barrect[NUMBAR];            // The bar rectangles
EXTERN rect_t                    maprect;                    // The map rectangle
#define SPARKLESIZE 28
#define SPARKLEADD 2
#define MAPSIZE 64
#define BLIPSIZE 3

//Lightning effects
EXTERN int                     numdynalight;               // Number of GDyna.mic lights
typedef struct dyna_lightlist_t
{
  int                     distancetobeat;         // The number to beat
  int                     distance[MAXDYNA];      // The distances
  float                   lightlistx[MAXDYNA];    // Light position
  float                   lightlisty[MAXDYNA];    //
  float                   lightlistz[MAXDYNA];    //
  float                   lightlevel[MAXDYNA];    // Light level
  float                   lightfalloff[MAXDYNA];  // Light falloff
} DYNA_LIGHTLIST;

EXTERN DYNA_LIGHTLIST GDyna;

EXTERN Uint8           lightdirectionlookup[65536];// For lighting characters
EXTERN float                   spek_global[MAXLIGHTROTATION][MD2LIGHTINDICES];
EXTERN float                   spek_local[MAXLIGHTROTATION][MD2LIGHTINDICES];
EXTERN Uint8           cLoadBuffer[MD2MAXLOADSIZE];// Where to put an MD2

EXTERN Uint32            maplrtwist[256];            // For surface normal of mesh
EXTERN Uint32            mapudtwist[256];            //
EXTERN float                   vellrtwist[256];            // For sliding down steep hills
EXTERN float                   veludtwist[256];            //
EXTERN Uint8           flattwist[256];             //

//Camera stuff
#define TRACKXAREALOW     100
#define TRACKXAREAHIGH    180
#define TRACKYAREAMINLOW  320
#define TRACKYAREAMAXLOW  460
#define TRACKYAREAMINHIGH 460
#define TRACKYAREAMAXHIGH 600


//Character stuff
EXTERN int                     numfreechr EQ(0);               // For allocation
EXTERN Uint16          freechrlist[MAXCHR];        //

typedef struct character_t
{
  GLMATRIX        matrix;			// Character's matrix
  char            matrixvalid;     // Did we make one yet?
  char            name[MAXCAPNAMESIZE];  // Character name
  Uint8           on;              // Does it exist?
  Uint8           onold;           // Network fix
  Uint8           alive;           // Is it alive?
  Uint8           waskilled;       // Fix for network
  Uint8           inpack;          // Is it in the inventory?
  Uint8           wasinpack;       // Temporary thing...
  Uint16          nextinpack;      // Link to the next item
  Uint8           numinpack;       // How many
  Uint8           openstuff;       // Can it open chests/doors?
  Uint8           lifecolor;       // Bar color
  Uint8           sparkle;         // Sparkle color or 0 for off
  Sint16          life;            // Basic character stats
  Sint16          lifemax;         //   All 8.8 fixed point
  Uint16          lifeheal;        //
  Uint8           manacolor;       // Bar color
  Uint8           ammomax;         // Ammo stuff
  Uint8           ammo;            //
  Uint8           gender;          // Gender
  Sint16          mana;            // Mana stuff
  Sint16          manamax;         //
  Sint16          manaflow;        //
  Sint16          manareturn;      //
  Sint16          strength;        // Strength
  Sint16          wisdom;          // Wisdom
  Sint16          intelligence;    // Intelligence
  Sint16          dexterity;       // Dexterity
  Uint8           aitype;          // The AI script to run
  bool_t          icon;            // Show the icon?
  bool_t          cangrabmoney;    // Picks up coins?
  bool_t          isplayer;        // btrue = player
  bool_t          islocalplayer;   // btrue = local player
  Uint16          aitarget;        // Who the AI is after
  Uint16          aiowner;         // The character's owner
  Uint16          aichild;         // The character's child
  int             aistate;         // Short term memory for AI
  int             aicontent;       // More short term memory
  Uint16          aitime;          // AI Timer
  Uint8           aigoto;          // Which waypoint
  Uint8           aigotoadd;       // Where to stick next
  float           aigotox[MAXWAY]; // Waypoint
  float           aigotoy[MAXWAY]; // Waypoint
  int             aix[MAXSTOR];    // Temporary values...  SetXY
  int             aiy[MAXSTOR];    //
  Uint8           stickybutt;      // Rests on floor
  Uint8           enviro;          // Environment map?
  float           oldx;            // Character's last position
  float           oldy;            //
  float           oldz;            //
  Uint8           inwater;         //
  Uint16          oldturn;         //
  Uint32          alert;           // Alerts for AI script
  Uint8           flyheight;       // Height to stabilize at
  Uint8           team;            // Character's team
  Uint8           baseteam;        // Character's starting team
  Uint8           staton;          // Display stats?
  float           xstt;            // Starting position
  float           ystt;            //
  float           zstt;            //
  float           xpos;            // Character's position
  float           ypos;            //
  float           zpos;            //
  float           xvel;            // Character's velocity
  float           yvel;            //
  float           zvel;            //
  float           latchx;          // Character latches
  float           latchy;          //
  Uint8           latchbutton;     // Button latches
  Uint8           reloadtime;      // Time before another shot
  float           maxaccel;        // Maximum acceleration
  float           scale;           // Character's size (useful)
  float           fat;             // Character's size (legible)
  float           sizegoto;        // Character's size goto ( legible )
  Uint8           sizegototime;    // Time left in siez change
  float           dampen;          // Bounciness
  float           level;           // Height of tile
  float           jump;            // Jump power
  Uint8           jumptime;        // Delay until next jump
  Uint8           jumpnumber;      // Number of jumps remaining
  Uint8           jumpnumberreset; // Number of jumps total, 255=Flying
  Uint8           jumpready;       // For standing on a platform character
  Uint32          onwhichfan;      // Where the char is
  Uint8           indolist;        // Has it been added yet?
  Uint16          uoffset;         // For moving textures
  Uint16          voffset;         //
  Uint16          uoffvel;         // Moving texture speed
  Uint16          voffvel;         //
  Uint16          turnleftright;   // Character's rotation 0 to 65535
  Uint16          turnmaplr;       //
  Uint16          turnmapud;       //
  Uint16          texture;         // Character's skin
  Uint8           model;           // Character's model
  Uint8           basemodel;       // The true form
  Uint8           actionready;     // Ready to play a new one
  Uint8           action;          // Character's action
  Uint8           keepaction;      // Keep the action playing
  Uint8           loopaction;      // Loop it too
  Uint8           nextaction;      // Character's action to play next
  Uint16          frame;           // Character's frame
  Uint16          lastframe;       // Character's last frame
  Uint8           lip;             // Character's frame in betweening
  Uint8           vrta[MAXVERTICES];// Lighting hack ( Ooze )
  Uint16          holdingwhich[2]; // !=MAXCHR if character is holding something
  Uint16          attachedto;      // !=MAXCHR if character is a held weapon
  Uint16          weapongrip[4];   // Vertices which describe the weapon grip
  Uint8           alpha;           // 255 = Solid, 0 = Invisible
  Uint8           light;           // 1 = Light, 0 = Normal
  Uint8           flashand;        // 1,3,7,15,31 = Flash, 255 = Don't
  Uint8           lightambi;      // 0-255, terrain light
  Uint8           lightspek;      // 0-255, terrain light
  Uint16          lightturnleftright;// Character's light rotation 0 to 65535
  Uint8           sheen;           // 0-15, how shiny it is
  Uint8           transferblend;   // Give transparency to weapons?
  Uint8           isitem;          // Is it grabbable?
  Uint8           ismount;         // Can you ride it?
  Uint8           redshift;        // Color channel shifting
  Uint8           grnshift;        //
  Uint8           blushift;        //
  Uint8           shadowsize;      // Size of shadow
  Uint8           bumpsize;        // Size of bumpers
  Uint8           bumpsizebig;     // For octagonal bumpers
  Uint8           bumpheight;      // Distance from head to toe
  Uint8           shadowsizesave;  // Without size modifiers
  Uint8           bumpsizesave;    //
  Uint8           bumpsizebigsave; //
  Uint8           bumpheightsave;  //
  Uint16          bumpnext;        // Next character on fanblock
  Uint16          bumplast;        // Last character it was bumped by
  float           bumpdampen;      // Character bump mass
  Uint16          attacklast;      // Last character it was attacked by
  Uint16          hitlast;         // Last character it hit
  Uint16          directionlast;   // Direction of last attack/healing
  Uint8           damagetypelast;  // Last damage type
  Uint8           platform;        // Can it be stood on
  Uint8           turnmode;        // Turning mode
  Uint8           sneakspd;        // Sneaking if above this speed
  Uint8           walkspd;         // Walking if above this speed
  Uint8           runspd;          // Running if above this speed
  Uint8           damagetargettype;// Type of damage for AI DamageTarget
  Uint8           reaffirmdamagetype; // For relighting torches
  Uint8           damagemodifier[MAXDAMAGETYPE];  // Resistances and inversion
  Uint8           damagetime;      // Invincibility timer
  Uint8           defense;         // Base defense rating
  Uint16          weight;          // Weight ( for pressure plates )
  Uint8           passage;         // The passage associated with this character
  Uint32          order;           // The last order given the character
  Uint8           counter;         // The rank of the character on the order chain
  Uint16          holdingweight;   // For weighted buttons
  Sint32          money;           // Money
  Sint16          lifereturn;      // Regeneration/poison
  Sint16          manacost;        // Mana cost to use
  Uint8           stoppedby;       // Collision mask
  Sint32          experience;      // Experience
  Sint32          experiencelevel; // Experience Level
  Sint16          grogtime;        // Grog timer
  Sint16          dazetime;        // Daze timer
  Uint8           nameknown;       // Is the name known?
  Uint8           ammoknown;       // Is the ammo known?
  Uint8           hitready;        // Was it just dropped?
  Sint16          boretime;        // Boredom timer
  Uint8           carefultime;     // "You hurt me!" timer
  Uint8           canbecrushed;    // Crush in a door?
  Uint8           inwhichhand;     // GRIPLEFT or GRIPRIGHT
  Uint8           isequipped;      // For boots and rings and stuff
  Uint8           firstenchant;    // Linked list for enchants
  Uint8           undoenchant;     // Last enchantment spawned
  Uint8           missiletreatment;// For deflection, etc.
  Uint8           missilecost;     // Mana cost for each one
  Uint16          missilehandler;  // Who pays the bill for each one...
  Uint16          damageboost;     // Add to swipe damage
  bool_t          overlay;         // Is this an overlay?  Track aitarget...

  // [BEGIN] Character states that are like skill expansions
  bool_t     invictus;          // Totally invincible?
  bool_t     waterwalk;         // Always above watersurfacelevel?
  bool_t     iskursed;          // Can't be dropped?  Could this also mean damage debuff? Spell fizzle rate? etc.
  bool_t     canseeinvisible;   //
  bool_t     canchannel;        //
  // [END] Character states that are like skill expansions

  // [BEGIN] Skill Expansions
  bool_t     canseekurse;             // Can it see kurses?
  bool_t     canusearcane;            // Can use [WMAG] spells?
  bool_t     canjoust;                // Can it use a lance to joust?
  bool_t     canusetech;              // Can it use [TECH]items?
  bool_t     canusedivine;            // Can it use [HMAG] runes?
  bool_t     candisarm;               // Disarm and find traps [DISA]
  bool_t     canbackstab;             // Backstab and murder [STAB]
  bool_t     canuseadvancedweapons;   // Advanced weapons usage [AWEP]
  bool_t     canusepoison;            // Use poison without err [POIS]
  bool_t     canread;				 // Can read books and scrolls
  // [END]  Skill Expansions
} CHARACTER;

EXTERN CHARACTER ChrList[MAXCHR];

#define SEEKURSEAND         31                      // Blacking flash
#define SEEINVISIBLE        128                     // Cutoff for invisible characters
#define INVISIBLE           20                      // The character can't be detected



//------------------------------------
//Enchantment variables
//------------------------------------
EXTERN Uint16    numfreeenchant;             // For allocating new ones
EXTERN Uint16    freeenchant[MAXENCHANT];    //

typedef struct eve_t
{
  bool_t      valid;                       // Enchant.txt loaded?
  bool_t      override;                    // Override other enchants?
  bool_t      removeoverridden;            // Remove other enchants?
  bool_t      setyesno[MAXEVESETVALUE];    // Set this value?
  Uint8       setvalue[MAXEVESETVALUE];    // Value to use
  Sint8       addvalue[MAXEVEADDVALUE];    // The values to add
  bool_t      retarget;                    // Pick a weapon?
  bool_t      killonend;                   // Kill the target on end?
  bool_t      poofonend;                   // Spawn a poof on end?
  bool_t      endifcantpay;                // End on out of mana
  bool_t      stayifnoowner;               // Stay if owner has died?
  Sint16      time;                        // Time in seconds
  Sint8       endmessage;                  // Message for end -1 for none
  Sint16      ownermana;                   // Boost values
  Sint16      ownerlife;                   //
  Sint16      targetmana;                  //
  Sint16      targetlife;                  //
  Uint8       dontdamagetype;              // Don't work if ...
  Uint8       onlydamagetype;              // Only work if ...
  IDSZ        removedbyidsz;               // By particle or [NONE]
  Uint16      contspawntime;               // Spawn timer
  Uint8       contspawnamount;             // Spawn amount
  Uint16      contspawnfacingadd;          // Spawn in circle
  Uint16      contspawnpip;                // Spawn type ( local )
  Mix_Chunk * waveindex;                   // Sound on end (-1 for none)
  Uint16      frequency;                   // Sound frequency
  Uint8       overlay;                     // Spawn an overlay?
} EVE;

EXTERN EVE EveList[MAXEVE];

typedef struct enchant_t
{
  Uint8           on;                      // Enchantment on
  Uint8           eve;                     // The type
  Uint16          target;                  // Who it enchants
  Uint16          nextenchant;             // Next in the list
  Uint16          owner;                   // Who cast the enchant
  Uint16          spawner;                 // The spellbook character
  Uint16          overlay;                 // The overlay character
  Sint16          ownermana;               // Boost values
  Sint16          ownerlife;               //
  Sint16          targetmana;              //
  Sint16          targetlife;              //
  bool_t          setyesno[MAXEVESETVALUE];// Was it set?
  Uint8           setsave[MAXEVESETVALUE]; // The value to restore
  Sint16          addsave[MAXEVEADDVALUE]; // The value to take away
  Sint16          time;                    // Time before end
  Uint16          spawntime;               // Time before spawn
} ENCHANT;

EXTERN ENCHANT EncList[MAXENCHANT];

#define MISNORMAL               0         //Treat missiles normally
#define MISDEFLECT              1         //Deflect incoming missiles
#define MISREFLECT              2         //Reflect them back!

#define LEAVEALL                0
#define LEAVEFIRST              1
#define LEAVENONE               2

#define SETDAMAGETYPE           0
#define SETNUMBEROFJUMPS        1
#define SETLIFEBARCOLOR         2
#define SETMANABARCOLOR         3
#define SETSLASHMODIFIER        4   //Damage modifiers
#define SETCRUSHMODIFIER        5
#define SETPOKEMODIFIER         6
#define SETHOLYMODIFIER         7
#define SETEVILMODIFIER         8
#define SETFIREMODIFIER         9
#define SETICEMODIFIER          10
#define SETZAPMODIFIER          11
#define SETFLASHINGAND          12
#define SETLIGHTBLEND           13
#define SETALPHABLEND           14
#define SETSHEEN                15   //Shinyness
#define SETFLYTOHEIGHT          16
#define SETWALKONWATER          17
#define SETCANSEEINVISIBLE      18
#define SETMISSILETREATMENT     19
#define SETCOSTFOREACHMISSILE   20
#define SETMORPH                21   //Morph character?
#define SETCHANNEL              22   //Can channel life as mana?

#define ADDJUMPPOWER            0
#define ADDBUMPDAMPEN           1
#define ADDBOUNCINESS           2
#define ADDDAMAGE               3
#define ADDSIZE                 4
#define ADDACCEL                5
#define ADDRED                  6   //Red shift
#define ADDGRN                  7   //Green shift
#define ADDBLU                  8   //Blue shift
#define ADDDEFENSE              9   //Defence adjustments
#define ADDMANA                 10
#define ADDLIFE                 11
#define ADDSTRENGTH             12
#define ADDWISDOM               13
#define ADDINTELLIGENCE         14
#define ADDDEXTERITY            15


//------------------------------------
//Particle variables
//------------------------------------
#define SPAWNNOCHARACTER        255                 // For particles that spawn characters...

EXTERN float                   textureoffset[256];         // For moving textures
EXTERN Uint16          dolist[MAXCHR];             // List of which characters to draw
EXTERN Uint16          numdolist;                  // How many in the list

EXTERN int                     numfreeprt EQ(0);                           // For allocation
EXTERN Uint16          freeprtlist[MAXPRT];                        //

typedef struct particle_t
{
  Uint8           on;                              // Does it exist?
  Uint16          pip;                             // The part template
  Uint16          model;                           // Pip spawn model
  Uint16          attachedtocharacter;             // For torch flame
  Uint16          grip;                            // The vertex it's on
  Uint8           type;                            // Transparency mode, 0-2
  Uint16          facing;                          // Direction of the part
  Uint8           team;                            // Team
  float           xpos;                            // Position
  float           ypos;                            //
  float           zpos;                            //
  float           xvel;                            // Velocity
  float           yvel;                            //
  float           zvel;                            //
  float           level;                           // Height of tile
  Uint8           spawncharacterstate;             //
  Uint16          rotate;                          // Rotation direction
  Sint16          rotateadd;                       // Rotation rate
  Uint32          onwhichfan;                      // Where the part is
  Uint16          size;                            // Size of particle>>8
  Sint16          sizeadd;                         // Change in size
  Uint8           inview;                          // Render this one?
  Uint32          image;                           // Which image ( >> 8 )
  Uint32          imageadd;                        // Animation rate
  Uint32          imagemax;                        // End of image loop
  Uint32          imagestt;                        // Start of image loop
  Uint8           light;                           // Light level
  Uint16          time;                            // Duration of particle
  Uint16          spawntime;                       // Time until spawn
  Uint8           bumpsize;                        // Size of bumpers
  Uint8           bumpsizebig;                     //
  Uint8           bumpheight;                      // Bounding box height
  Uint16          bumpnext;                        // Next particle on fanblock
  Uint16          damagebase;                      // For strength
  Uint16          damagerand;                      // For fixes...
  Uint8           damagetype;                      // Damage type
  Uint16          chr;                             // The character that is attacking
  float           dyna_lightfalloff;                // Dyna light...
  float           dyna_lightlevel;                  //
  Uint8           dyna_lighton;                     // Dynamic light?
  Uint16          target;                          // Who it's chasing
} PARTICLE;

EXTERN PARTICLE PrtList[MAXPRT];

EXTERN Uint16          particletexture;                            // All in one bitmap
EXTERN Uint16          prttexw;
EXTERN Uint16          prttexh;
EXTERN float           prttexwscale EQ(1.0f);
EXTERN float           prttexhscale EQ(1.0f);
//EXTERN float                   particleimageu[MAXPARTICLEIMAGE][2];        // Texture coordinates
//EXTERN float                   particleimagev[MAXPARTICLEIMAGE][2];        //

#define CALCULATE_PRT_U0(CNT)  (((.05f+(CNT&15))/16.0f)*prttexwscale)
#define CALCULATE_PRT_U1(CNT)  (((.95f+(CNT&15))/16.0f)*prttexwscale)
#define CALCULATE_PRT_V0(CNT)  (((.05f+(CNT/16))/16.0f) * ((float)prttexw/(float)prttexh)*prttexhscale)
#define CALCULATE_PRT_V1(CNT)  (((.95f+(CNT/16))/16.0f) * ((float)prttexw/(float)prttexh)*prttexhscale)

//------------------------------------
//Module variables
//------------------------------------
#define RANKSIZE 8
#define SUMMARYLINES 8
#define SUMMARYSIZE  80
//EXTERN int             globalnummodule;                            // Number of modules
//EXTERN char            ModList[MAXMODULE].rank[RANKSIZE];               // Number of stars
//EXTERN char            ModList[MAXMODULE].longname[MAXCAPNAMESIZE];     // Module names
//EXTERN char            ModList[MAXMODULE].loadname[MAXCAPNAMESIZE];     // Module load names
//EXTERN Uint8           ModList[MAXMODULE].importamount;                 // # of import characters
//EXTERN Uint8           ModList[MAXMODULE].allowexport;                  // Export characters?
//EXTERN Uint8           ModList[MAXMODULE].minplayers;                   // Number of players
//EXTERN Uint8           ModList[MAXMODULE].maxplayers;                   //
//EXTERN Uint8           ModList[MAXMODULE].monstersonly;                 // Only allow monsters
//EXTERN Uint8           ModList[MAXMODULE].rts_control;                   // Real Time Stragedy?
//EXTERN Uint8           ModList[MAXMODULE].respawnvalid;                 // Allow respawn
//EXTERN int                     numlines;                                   // Lines in summary
//EXTERN char                    ModList[SUMMARYLINES].summary[SUMMARYSIZE];      // Quest description


//------------------------------------
//Model stuff
//------------------------------------
// TEMPORARY: Needs to be moved out of egoboo.h eventually
extern struct Md2Model *md2_Models[MAXMODEL];          // Md2 models

EXTERN int                     globalnumicon;                              // Number of icons
EXTERN Uint16          madloadframe;                               // Where to load next

typedef struct mad_frame_t
{
  Sint16 vrtx[MAXVERTICES];             // Vertex position
  Sint16 vrty[MAXVERTICES];             //
  Sint16 vrtz[MAXVERTICES];             //
  Uint8  vrta[MAXVERTICES];             // Light index of vertex
  Uint8  framelip;                      // 0-15, How far into action is each frame
  Uint16 framefx;                       // Invincibility, Spawning
} MAD_FRAME;

EXTERN MAD_FRAME MadFrame[MAXFRAME];

typedef struct mad_t
{
  Uint8        used;                          // Model slot
  char                 name[128];                     // Model name
  Uint16       skins;                         // Number of skins
  Uint16       skinstart;                     // Starting skin of model
  Uint16       frames;                        // Number of frames
  Uint16       framestart;                    // Starting frame of model
  Uint16       msg_start;                      // The first message
  Uint16       vertices;                      // Number of vertices
  Uint16       transvertices;                 // Number to transform
  Uint16       commands;                      // Number of commands
  float        scale;                         // Multiply by value
  GLenum       commandtype[MAXCOMMAND];       // Fan or strip
  Uint8        commandsize[MAXCOMMAND];       // Entries used by command
  Uint16       commandvrt[MAXCOMMANDENTRIES]; // Which vertex
  float        commandu[MAXCOMMANDENTRIES];   // Texture position
  float        commandv[MAXCOMMANDENTRIES];   //

  Uint16       frameliptowalkframe[4][16];    // For walk animations
  Uint16       ai;                            // AI for each model
  Uint8        actionvalid[MAXACTION];        // bfalse if not valid
  Uint16       actionstart[MAXACTION];        // First frame of anim
  Uint16       actionend[MAXACTION];          // One past last frame
  Uint16       prtpip[MAXPRTPIPPEROBJECT];    // Local particles
} MAD;

EXTERN MAD MadList[MAXMODEL];

Uint16  skintoicon[MAXTEXTURE];                  // Skin to icon

//Sound using SDL_Mixer
EXTERN bool_t         mixeron EQ(bfalse);   //Is the SDL_Mixer loaded?
#define MAXWAVE         16        // Up to 16 waves per model
#define VOLMIN          -4000      // Minumum Volume level
#define VOLUMERATIO     7        // Volume ratio
EXTERN Mix_Chunk  *globalwave[10];   //All sounds loaded into memory
EXTERN Mix_Chunk  *sound;      //Used for playing one selected sound file
EXTERN int              channel;     //Which channel the current sound is using

// Character profiles
#define MAXSKIN 4

EXTERN   int importobject;

typedef struct cap_t
{
  short         importslot;
  char          classname[MAXCAPNAMESIZE];     // Class name
  Sint8         skinoverride;                  // -1 or 0-3.. For import
  Uint8         leveloverride;                 // 0 for normal
  int           stateoverride;                 // 0 for normal
  int           contentoverride;               // 0 for normal
  Uint8         skindressy;                    // Dressy
  float         strengthdampen;                // Strength damage factor
  Uint8         stoppedby;                     // Collision Mask
  Uint8         uniformlit;                    // Bad lighting?
  Uint8         lifecolor;                     // Bar colors
  Uint8         manacolor;                     //
  Uint8         ammomax;                       // Ammo stuff
  Uint8         ammo;                          //
  Uint8         gender;                        // Gender
  Uint16        lifebase;                      // Life
  Uint16        liferand;                      //
  Uint16        lifeperlevelbase;              //
  Uint16        lifeperlevelrand;              //
  Sint16        lifereturn;                    //
  Sint16        money;                         // Money
  Uint16        lifeheal;                      //
  Uint16        manabase;                      // Mana
  Uint16        manarand;                      //
  Sint16        manacost;                      //
  Uint16        manaperlevelbase;              //
  Uint16        manaperlevelrand;              //
  Uint16        manareturnbase;                //
  Uint16        manareturnrand;                //
  Uint16        manareturnperlevelbase;        //
  Uint16        manareturnperlevelrand;        //
  Uint16        manaflowbase;                  //
  Uint16        manaflowrand;                  //
  Uint16        manaflowperlevelbase;          //
  Uint16        manaflowperlevelrand;          //
  Uint16        strengthbase;                  // Strength
  Uint16        strengthrand;                  //
  Uint16        strengthperlevelbase;          //
  Uint16        strengthperlevelrand;          //
  Uint16        wisdombase;                    // Wisdom
  Uint16        wisdomrand;                    //
  Uint16        wisdomperlevelbase;            //
  Uint16        wisdomperlevelrand;            //
  Uint16        intelligencebase;              // Intlligence
  Uint16        intelligencerand;              //
  Uint16        intelligenceperlevelbase;      //
  Uint16        intelligenceperlevelrand;      //
  Uint16        dexteritybase;                 // Dexterity
  Uint16        dexterityrand;                 //
  Uint16        dexterityperlevelbase;         //
  Uint16        dexterityperlevelrand;         //
  float         size;                          // Scale of model
  float         sizeperlevel;                  // Scale increases
  float         dampen;                        // Bounciness
  Uint8         shadowsize;                    // Shadow size
  Uint8         bumpsize;                      // Bounding octagon
  Uint8         bumpsizebig;                   // For octagonal bumpers
  Uint8         bumpheight;                    //
  float         bumpdampen;                    // Mass
  Uint8         weight;                        // Weight
  float         jump;                          // Jump power
  Uint8         jumpnumber;                    // Number of jumps ( Ninja )
  Uint8         sneakspd;                      // Sneak threshold
  Uint8         walkspd;                       // Walk threshold
  Uint8         runspd;                        // Run threshold
  Uint8         flyheight;                     // Fly height
  Uint8         flashand;                      // Flashing rate
  Uint8         alpha;                         // Transparency
  Uint8         light;                         // Light blending
  Uint8         transferblend;                 // Transfer blending to rider/weapons
  Uint8         sheen;                         // How shiny it is ( 0-15 )
  Uint8         enviro;                        // Phong map this baby?
  Uint16        uoffvel;                       // Texture movement rates
  Uint16        voffvel;                       //
  Uint8         stickybutt;                    // Stick to the ground?
  Uint16        iframefacing;                  // Invincibility frame
  Uint16        iframeangle;                   //
  Uint16        nframefacing;                  // Normal frame
  Uint16        nframeangle;                   //
  Uint8         resistbumpspawn;               // Don't catch fire
  int           experienceforlevel[MAXLEVEL];  // Experience needed for next level
  float         experiencepower;
  float         experiencecoeff;
  int           experiencebase;                // Starting experience
  int           experiencerand;                //
  int           experienceworth;               // Amount given to killer/user
  float         experienceexchange;            // Adds to worth
  float         experiencerate[MAXEXPERIENCETYPE];
  IDSZ          idsz[MAXIDSZ];                 // ID strings
  bool_t        isitem;                        // Is it an item?
  bool_t        ismount;                       // Can you ride it?
  bool_t        isstackable;                   // Is it arrowlike?
  bool_t        nameknown;                     // Is the class name known?
  bool_t        usageknown;                    // Is its usage known
  bool_t        cancarrytonextmodule;          // Take it with you?
  bool_t        needskillidtouse;              // Check IDSZ first?
  bool_t        platform;                      // Can be stood on?
  bool_t        canuseplatforms;               // Can use platforms?
  bool_t        cangrabmoney;                  // Collect money?
  bool_t        canopenstuff;                  // Open chests/doors?
  bool_t        icon;                          // Draw icon
  bool_t        forceshadow;                   // Draw a shadow?
  bool_t        ripple;                        // Spawn ripples?
  Uint8         damagetargettype;              // For AI DamageTarget
  Uint8         weaponaction;                  // Animation needed to swing
  Uint8         gripvalid[2];                  // Left/Right hands valid
  Uint8         attackattached;                //
  Sint8         attackprttype;                 //
  Uint8         attachedprtamount;             // Sticky particles
  Uint8         attachedprtreaffirmdamagetype; // Relight that torch...
  Uint8         attachedprttype;               //
  Uint8         gopoofprtamount;               // Poof effect
  Sint16        gopoofprtfacingadd;            //
  Uint8         gopoofprttype;                 //
  Uint8         bloodvalid;                    // Blood ( yuck )
  Uint8         bloodprttype;                  //
  Sint8         wavefootfall;                  // Footfall sound, -1
  Sint8         wavejump;                      // Jump sound, -1
  Uint8         ridercanattack;                // Rider attack?
  bool_t        canbedazed;                    // Can it be dazed?
  bool_t        canbegrogged;                  // Can it be grogged?
  Uint8         kursechance;                   // Chance of being kursed
  Uint8         istoobig;                      // Can't be put in pack
  Uint8         reflect;                       // Draw the reflection
  Uint8         alwaysdraw;                    // Always render
  Uint8         isranged;                      // Flag for ranged weapon
  Sint8         hidestate;                       // Don't draw when...
  Uint8         isequipment;                     // Behave in silly ways

  Uint8         defense[MAXSKIN];                    // Defense for each skin
  char          skinname[MAXSKIN][MAXCAPNAMESIZE];   // Skin name
  Uint16        skincost[MAXSKIN];                   // Store prices
  Uint8         damagemodifier[MAXDAMAGETYPE][MAXSKIN];
  float         maxaccel[MAXSKIN];                   // Acceleration for each skin

  Mix_Chunk  *  waveindex[MAXWAVE];    //sounds in a object
  Uint16        sectionsize[MAXSECTION];   // Number of choices, 0
  Uint16        sectionstart[MAXSECTION];  //


// [BEGIN] Character template parameters that are like Skill Expansions
  bool_t        waterwalk;                     // Walk on water?
  bool_t        invictus;                      // Is it invincible?
  bool_t        canseeinvisible;               // Can it see invisible?
// [END] Character template parameters that are like Skill Expansions

// [BEGIN] Skill Expansions
  bool_t     canseekurse;                   // Can it see kurses?
  bool_t     canusearcane;      // Can use [WMAG] spells?
  bool_t     canjoust;       // Can it use a lance to joust?
  bool_t     canusetech;      // Can it use [TECH]items?
  bool_t     canusedivine;      // Can it use [HMAG] runes?
  bool_t     candisarm;         // Disarm and find traps [DISA]
  bool_t     canbackstab;         // Backstab and murder [STAB]
  bool_t     canuseadvancedweapons;   // Advanced weapons usage [AWEP]
  bool_t     canusepoison;         // Use poison without err [POIS]
  bool_t     canread;			// Can read books and scrolls [READ]
// [END] Skill Expansions
} CAP;

EXTERN CAP CapList[MAXCHR];

// Particle template
#define DYNAOFF   0
#define DYNAON    1
#define DYNALOCAL 2
#define DYNAFANS  12
#define MAXFALLOFF 1400

EXTERN int                     numpip  EQ(0);
typedef struct pip_t
{
  Uint8           force;                        // Force spawn?
  Uint8           type;                         // Transparency mode
  Uint8           numframes;                    // Number of frames
  Uint16          imagebase;                    // Starting image
  Uint32          imageadd;                     // Frame rate
  Uint32          imageaddrand;                 // Frame rate randomness
  Uint16          time;                         // Time until end
  Uint16          rotatebase;                   // Rotation
  Uint16          rotaterand;                   // Rotation
  Sint16          rotateadd;                    // Rotation rate
  Uint16          sizebase;                     // Size
  Sint16          sizeadd;                      // Size rate
  float           spdlimit;                     // Speed limit
  float           dampen;                       // Bounciness
  Sint8           bumpmoney;                    // Value of particle
  Uint8           bumpsize;                     // Bounding box size
  Uint8           bumpheight;                   // Bounding box height
  Uint8           endwater;                     // End if underwater
  Uint8           endbump;                      // End if bumped
  Uint8           endground;                    // End if on ground
  Uint8           endwall;                      // End if hit a wall
  Uint8           endlastframe;                 // End on last frame
  Uint16          damagebase;                   // Damage
  Uint16          damagerand;                   // Damage
  Uint8           damagetype;                   // Damage type
  Sint16          facingbase;                   // Facing
  Uint16          facingadd;                    // Facing
  Uint16          facingrand;                   // Facing
  Sint16          xyspacingbase;                // Spacing
  Uint16          xyspacingrand;                // Spacing
  Sint16          zspacingbase;                 // Altitude
  Uint16          zspacingrand;                 // Altitude
  Sint8           xyvelbase;                    // Shot velocity
  Uint8           xyvelrand;                    // Shot velocity
  Sint8           zvelbase;                     // Up velocity
  Uint8           zvelrand;                     // Up velocity
  Uint16          contspawntime;                // Spawn timer
  Uint8           contspawnamount;              // Spawn amount
  Uint16          contspawnfacingadd;           // Spawn in circle
  Uint16          contspawnpip;                 // Spawn type ( local )
  Uint8           endspawnamount;               // Spawn amount
  Uint16          endspawnfacingadd;            // Spawn in circle
  Uint8           endspawnpip;                  // Spawn type ( local )
  Uint8           bumpspawnamount;              // Spawn amount
  Uint8           bumpspawnpip;                 // Spawn type ( global )
  Uint8           dyna_lightmode;                // Dynamic light on?
  float           dyna_level;                    // Intensity
  Uint16          dyna_falloff;                  // Falloff
  float           dyna_lightleveladd;            // Dyna light changes
  float           dyna_lightfalloffadd;          //
  Uint16          dazetime;                     // Daze
  Uint16          grogtime;                     // Drunkeness
  Sint8           soundspawn;                   // Beginning sound
  Sint8           soundend;                     // Ending sound
  Sint8           soundfloor;                   // Floor sound
  Sint8           soundwall;                    // Ricochet sound
  Uint8           friendlyfire;                 // Friendly fire
  Uint8           rotatetoface;                 // Arrows/Missiles
  Uint8           causepancake;                 // Cause pancake?
  Uint8           causeknockback;               // Cause knockback?

  Uint8           newtargetonspawn;             // Get new target?
  Uint8           homing;                       // Homing?
  Uint16          targetangle;                  // To find target
  float           homingaccel;                  // Acceleration rate
  float           homingfriction;               // Deceleration rate

  Uint8           targetcaster;                 // Target caster?
  Uint8           spawnenchant;                 // Spawn enchant?
  Uint8           needtarget;                   // Need a target?
  Uint8           onlydamagefriendly;           // Only friends?
  Uint8           startontarget;                // Start on target?
  int             zaimspd;                      // [ZSPD] For Z aiming
  Uint16          damfx;                        // Damage effects
  bool_t          allowpush;                    //Allow particle to push characters around
  bool_t          intdamagebonus;               //Add intelligence as damage bonus
  bool_t          wisdamagebonus;               //Add wisdom as damage bonus
} PIP;

EXTERN PIP PipList[MAXPRTPIP];

// The ID number for host searches
// {A0F72DE8-2C17-11d3-B7FE-444553540000}
/* PORT
DEFINE_GUID(NETWORKID, 0xa0f72de8, 0x2c17, 0x11d3, 0xb7, 0xfe, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
*/
#define MAXSERVICE 16
#define NETNAMESIZE 16
#define MAXSESSION 16
#define MAXNETPLAYER 8
EXTERN Uint32      randsave;         //Used in network timer

EXTERN char *globalname  EQ(NULL);  // For debuggin' goto_colon
EXTERN char *globalparsename  EQ(NULL); // The SCRIPT.TXT filename
EXTERN FILE *globalparseerr  EQ(NULL); // For debuggin' scripted AI
EXTERN FILE *globalnetworkerr  EQ(NULL); // For debuggin' network


EXTERN float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
EXTERN float           lighttoenviroy[256];                                // Environment map
EXTERN Uint32          lighttospek[MAXSPEKLEVEL][256];                     //


EXTERN float           hillslide  EQ(1.00);                                   //
EXTERN float           slippyfriction  EQ(1.00);  //1.05 for Chevron          // Friction
EXTERN float           airfriction  EQ(.95);                                  //
EXTERN float           waterfriction  EQ(.85);                                //
EXTERN float           noslipfriction  EQ(0.95);                              //
EXTERN float           platstick  EQ(.040);                                   //
EXTERN float           gravity  EQ((float) - 1.0);                            // Gravitational accel


EXTERN char            cFrameName[16];                                     // MD2 Frame Name


EXTERN Uint16  globesttarget;                                      // For find_target
EXTERN Uint16  globestangle;                                       //
EXTERN Uint16  glouseangle;                                        //
EXTERN int     globestdistance;


EXTERN Uint32    numfanblock  EQ(0);                                    // Number of collision areas

typedef struct bumplist_element_t
{
  Uint16  chr;                     // For character collisions
  Uint16  chrnum;                  // Number on the block
  Uint16  prt;                     // For particle collisions
  Uint16  prtnum;                  // Number on the block
} BUMPLIST_ELEMENT;

typedef struct fan_element_t
{
  Uint8   type;                               // Command type
  Uint8   fx;                                 // Special effects flags
  Uint8   twist;                              //
  Uint8   inrenderlist;                       //
  Uint16  tile;                               // Get texture from this
  Uint32  vrtstart;                           // Which vertex to start at
} FAN_ELEMENT;

typedef struct tile_element_t
{
  Uint8   commands;                          // Number of commands
  Uint8   commandsize[MAXMESHCOMMAND];       // Entries in each command
  Uint16  commandvrt[MAXMESHCOMMANDENTRIES]; // Fansquare vertex list
  Uint8   commandnumvertices;                // Number of vertices

  float   commandu[MAXMESHVERTICES];         // Vertex texture posi
  float   commandv[MAXMESHVERTICES];         //

  float   tileoffu;                          // Tile texture offset
  float   tileoffv;                          //
} TILE_ELEMENT;

typedef struct renderlist_element_t
{
  Uint32    all;                   // List of which to render, total
  Uint32    ref;                   // ..., reflective
  Uint32    sha;                   // ..., shadow
} RENDERLIST_ELEMENT;


typedef struct mesh_t
{
  Uint8   exploremode;                            // Explore mode?

  float* floatmemory;                                    // For malloc
  float* vrtx;                                           // Vertex position
  float* vrty;                                           //
  float* vrtz;                                           // Vertex elevation
  Uint8* vrta;                                           // Vertex base light
  Uint8* vrtl;                                           // Vertex light

  int     sizex;                                          // Size in fansquares
  int     sizey;                                          //
  float   edgex;                                          // Limits !!!BAD!!!
  float   edgey;                                          //
  Uint16  lasttexture;                                    // Last texture used

  Uint32    blockstart[(MAXMESHSIZEY/4)+1];
  Uint32    fanstart[MAXMESHSIZEY];                         // Which fan to start a row with


  BUMPLIST_ELEMENT bumplist[MAXMESHFAN/16];
  FAN_ELEMENT      fanlist[MAXMESHFAN];
  TILE_ELEMENT     tilelist[MAXTILETYPE];

  int                numrenderlist_all;                                  // Number to render, total
  int                numrenderlist_ref;                               // ..., reflective
  int                numrenderlist_sha;                               // ..., shadow
  RENDERLIST_ELEMENT renderlist[MAXMESHRENDER];
} MESH;

extern MESH Mesh;



EXTERN Uint8   asciitofont[256];                                   // Conversion table


// Display messages
typedef struct message_element_t
{
  Sint16    time;                                //
  char      textdisplay[MESSAGESIZE];            // The displayed text

} MESSAGE_ELEMENT;

typedef struct message_t
{
  Uint16  start;                                         // The message queue

  // Message files
  Uint16  total;                                         // The number of messages
  Uint32  totalindex;                                    // Where to put letter

  Uint32  index[MAXTOTALMESSAGE];                        // Where it is
  char    text[MESSAGEBUFFERSIZE];                       // The text buffer

  MESSAGE_ELEMENT list[MAXMESSAGE];

} MESSAGE;


extern MESSAGE GMsg;


#define MAXENDTEXT 1024
EXTERN char generictext[80];         // Use for whatever purpose
EXTERN char endtext[MAXENDTEXT];     // The end-module text
EXTERN int endtextwrite;


// This is id's normal table for computing light values
#ifdef DECLARE_GLOBALS
float md2normals[MD2LIGHTINDICES][3] =
{{ -0.5257, 0.0000, 0.8507}, { -0.4429, 0.2389, 0.8642}, { -0.2952, 0.0000, 0.9554},
  { -0.3090, 0.5000, 0.8090}, { -0.1625, 0.2629, 0.9511}, {0.0000, 0.0000, 1.0000},
  {0.0000, 0.8507, 0.5257}, { -0.1476, 0.7166, 0.6817}, {0.1476, 0.7166, 0.6817},
  {0.0000, 0.5257, 0.8507}, {0.3090, 0.5000, 0.8090}, {0.5257, 0.0000, 0.8507},
  {0.2952, 0.0000, 0.9554}, {0.4429, 0.2389, 0.8642}, {0.1625, 0.2629, 0.9511},
  { -0.6817, 0.1476, 0.7166}, { -0.8090, 0.3090, 0.5000}, { -0.5878, 0.4253, 0.6882},
  { -0.8507, 0.5257, 0.0000}, { -0.8642, 0.4429, 0.2389}, { -0.7166, 0.6817, 0.1476},
  { -0.6882, 0.5878, 0.4253}, { -0.5000, 0.8090, 0.3090}, { -0.2389, 0.8642, 0.4429},
  { -0.4253, 0.6882, 0.5878}, { -0.7166, 0.6817, -0.1476}, { -0.5000, 0.8090, -0.3090},
  { -0.5257, 0.8507, 0.0000}, {0.0000, 0.8507, -0.5257}, { -0.2389, 0.8642, -0.4429},
  {0.0000, 0.9554, -0.2952}, { -0.2629, 0.9511, -0.1625}, {0.0000, 1.0000, 0.0000},
  {0.0000, 0.9554, 0.2952}, { -0.2629, 0.9511, 0.1625}, {0.2389, 0.8642, 0.4429},
  {0.2629, 0.9511, 0.1625}, {0.5000, 0.8090, 0.3090}, {0.2389, 0.8642, -0.4429},
  {0.2629, 0.9511, -0.1625}, {0.5000, 0.8090, -0.3090}, {0.8507, 0.5257, 0.0000},
  {0.7166, 0.6817, 0.1476}, {0.7166, 0.6817, -0.1476}, {0.5257, 0.8507, 0.0000},
  {0.4253, 0.6882, 0.5878}, {0.8642, 0.4429, 0.2389}, {0.6882, 0.5878, 0.4253},
  {0.8090, 0.3090, 0.5000}, {0.6817, 0.1476, 0.7166}, {0.5878, 0.4253, 0.6882},
  {0.9554, 0.2952, 0.0000}, {1.0000, 0.0000, 0.0000}, {0.9511, 0.1625, 0.2629},
  {0.8507, -0.5257, 0.0000}, {0.9554, -0.2952, 0.0000}, {0.8642, -0.4429, 0.2389},
  {0.9511, -0.1625, 0.2629}, {0.8090, -0.3090, 0.5000}, {0.6817, -0.1476, 0.7166},
  {0.8507, 0.0000, 0.5257}, {0.8642, 0.4429, -0.2389}, {0.8090, 0.3090, -0.5000},
  {0.9511, 0.1625, -0.2629}, {0.5257, 0.0000, -0.8507}, {0.6817, 0.1476, -0.7166},
  {0.6817, -0.1476, -0.7166}, {0.8507, 0.0000, -0.5257}, {0.8090, -0.3090, -0.5000},
  {0.8642, -0.4429, -0.2389}, {0.9511, -0.1625, -0.2629}, {0.1476, 0.7166, -0.6817},
  {0.3090, 0.5000, -0.8090}, {0.4253, 0.6882, -0.5878}, {0.4429, 0.2389, -0.8642},
  {0.5878, 0.4253, -0.6882}, {0.6882, 0.5878, -0.4253}, { -0.1476, 0.7166, -0.6817},
  { -0.3090, 0.5000, -0.8090}, {0.0000, 0.5257, -0.8507}, { -0.5257, 0.0000, -0.8507},
  { -0.4429, 0.2389, -0.8642}, { -0.2952, 0.0000, -0.9554}, { -0.1625, 0.2629, -0.9511},
  {0.0000, 0.0000, -1.0000}, {0.2952, 0.0000, -0.9554}, {0.1625, 0.2629, -0.9511},
  { -0.4429, -0.2389, -0.8642}, { -0.3090, -0.5000, -0.8090}, { -0.1625, -0.2629, -0.9511},
  {0.0000, -0.8507, -0.5257}, { -0.1476, -0.7166, -0.6817}, {0.1476, -0.7166, -0.6817},
  {0.0000, -0.5257, -0.8507}, {0.3090, -0.5000, -0.8090}, {0.4429, -0.2389, -0.8642},
  {0.1625, -0.2629, -0.9511}, {0.2389, -0.8642, -0.4429}, {0.5000, -0.8090, -0.3090},
  {0.4253, -0.6882, -0.5878}, {0.7166, -0.6817, -0.1476}, {0.6882, -0.5878, -0.4253},
  {0.5878, -0.4253, -0.6882}, {0.0000, -0.9554, -0.2952}, {0.0000, -1.0000, 0.0000},
  {0.2629, -0.9511, -0.1625}, {0.0000, -0.8507, 0.5257}, {0.0000, -0.9554, 0.2952},
  {0.2389, -0.8642, 0.4429}, {0.2629, -0.9511, 0.1625}, {0.5000, -0.8090, 0.3090},
  {0.7166, -0.6817, 0.1476}, {0.5257, -0.8507, 0.0000}, { -0.2389, -0.8642, -0.4429},
  { -0.5000, -0.8090, -0.3090}, { -0.2629, -0.9511, -0.1625}, { -0.8507, -0.5257, 0.0000},
  { -0.7166, -0.6817, -0.1476}, { -0.7166, -0.6817, 0.1476}, { -0.5257, -0.8507, 0.0000},
  { -0.5000, -0.8090, 0.3090}, { -0.2389, -0.8642, 0.4429}, { -0.2629, -0.9511, 0.1625},
  { -0.8642, -0.4429, 0.2389}, { -0.8090, -0.3090, 0.5000}, { -0.6882, -0.5878, 0.4253},
  { -0.6817, -0.1476, 0.7166}, { -0.4429, -0.2389, 0.8642}, { -0.5878, -0.4253, 0.6882},
  { -0.3090, -0.5000, 0.8090}, { -0.1476, -0.7166, 0.6817}, { -0.4253, -0.6882, 0.5878},
  { -0.1625, -0.2629, 0.9511}, {0.4429, -0.2389, 0.8642}, {0.1625, -0.2629, 0.9511},
  {0.3090, -0.5000, 0.8090}, {0.1476, -0.7166, 0.6817}, {0.0000, -0.5257, 0.8507},
  {0.4253, -0.6882, 0.5878}, {0.5878, -0.4253, 0.6882}, {0.6882, -0.5878, 0.4253},
  { -0.9554, 0.2952, 0.0000}, { -0.9511, 0.1625, 0.2629}, { -1.0000, 0.0000, 0.0000},
  { -0.8507, 0.0000, 0.5257}, { -0.9554, -0.2952, 0.0000}, { -0.9511, -0.1625, 0.2629},
  { -0.8642, 0.4429, -0.2389}, { -0.9511, 0.1625, -0.2629}, { -0.8090, 0.3090, -0.5000},
  { -0.8642, -0.4429, -0.2389}, { -0.9511, -0.1625, -0.2629}, { -0.8090, -0.3090, -0.5000},
  { -0.6817, 0.1476, -0.7166}, { -0.6817, -0.1476, -0.7166}, { -0.8507, 0.0000, -0.5257},
  { -0.6882, 0.5878, -0.4253}, { -0.5878, 0.4253, -0.6882}, { -0.4253, 0.6882, -0.5878},
  { -0.4253, -0.6882, -0.5878}, { -0.5878, -0.4253, -0.6882}, { -0.6882, -0.5878, -0.4253},
  {0, 0, 0}  // Spikey mace
};
#else
EXTERN float md2normals[MD2LIGHTINDICES][3];
#endif

// This is for random naming
EXTERN Uint16          numchop  EQ(0);                // The number of name parts
EXTERN Uint32            chopwrite  EQ(0);              // The data pointer
EXTERN char                    chopdata[CHOPDATACHUNK];    // The name parts
EXTERN Uint16          chopstart[MAXCHOP];         // The first character of each part
EXTERN char                    namingnames[MAXCAPNAMESIZE];// The name returned by the function



// These are for the AI script loading/parsing routines
extern int                     iNumAis;

#define ALERTIFSPAWNED                      1           // 0
#define ALERTIFHITVULNERABLE                2           // 1
#define ALERTIFATWAYPOINT                   4           // 2
#define ALERTIFATLASTWAYPOINT               8           // 3
#define ALERTIFATTACKED                     16          // 4
#define ALERTIFBUMPED                       32          // 5
#define ALERTIFORDERED                      64          // 6
#define ALERTIFCALLEDFORHELP                128         // 7
#define ALERTIFKILLED                       256         // 8
#define ALERTIFTARGETKILLED                 512         // 9
#define ALERTIFDROPPED                      1024        // 10
#define ALERTIFGRABBED                      2048        // 11
#define ALERTIFREAFFIRMED                   4096        // 12
#define ALERTIFLEADERKILLED                 8192        // 13
#define ALERTIFUSED                         16384       // 14
#define ALERTIFCLEANEDUP                    32768       // 15
#define ALERTIFSCOREDAHIT                   65536       // 16
#define ALERTIFHEALED                       131072      // 17
#define ALERTIFDISAFFIRMED                  262144      // 18
#define ALERTIFCHANGED                      524288      // 19
#define ALERTIFINWATER                      1048576     // 20
#define ALERTIFBORED                        2097152     // 21
#define ALERTIFTOOMUCHBAGGAGE               4194304     // 22
#define ALERTIFGROGGED                      8388608     // 23
#define ALERTIFDAZED                        16777216    // 24
#define ALERTIFHITGROUND                    33554432    // 25
#define ALERTIFNOTDROPPED                   67108864    // 26
#define ALERTIFBLOCKED                      134217728   // 27
#define ALERTIFTHROWN                       268435456   // 28
#define ALERTIFCRUSHED                      536870912   // 29
#define ALERTIFNOTPUTAWAY                   1073741824  // 30
#define ALERTIFTAKENOUT                     2147483648u // 31

#define FIFSPAWNED                          0   // Scripted AI functions (v0.10)
#define FIFTIMEOUT                          1   //
#define FIFATWAYPOINT                       2   //
#define FIFATLASTWAYPOINT                   3   //
#define FIFATTACKED                         4   //
#define FIFBUMPED                           5   //
#define FIFORDERED                          6   //
#define FIFCALLEDFORHELP                    7   //
#define FSETCONTENT                         8   //
#define FIFKILLED                           9   //
#define FIFTARGETKILLED                     10  //
#define FCLEARWAYPOINTS                     11  //
#define FADDWAYPOINT                        12  //
#define FFINDPATH                           13  //
#define FCOMPASS                            14  //
#define FGETTARGETARMORPRICE                15  //
#define FSETTIME                            16  //
#define FGETCONTENT                         17  //
#define FJOINTARGETTEAM                     18  //
#define FSETTARGETTONEARBYENEMY             19  //
#define FSETTARGETTOTARGETLEFTHAND          20  //
#define FSETTARGETTOTARGETRIGHTHAND         21  //
#define FSETTARGETTOWHOEVERATTACKED         22  //
#define FSETTARGETTOWHOEVERBUMPED           23  //
#define FSETTARGETTOWHOEVERCALLEDFORHELP    24  //
#define FSETTARGETTOOLDTARGET               25  //
#define FSETTURNMODETOVELOCITY              26  //
#define FSETTURNMODETOWATCH                 27  //
#define FSETTURNMODETOSPIN                  28  //
#define FSETBUMPHEIGHT                      29  //
#define FIFTARGETHASID                      30  //
#define FIFTARGETHASITEMID                  31  //
#define FIFTARGETHOLDINGITEMID              32  //
#define FIFTARGETHASSKILLID                 33  //
#define FELSE                               34  //
#define FRUN                                35  //
#define FWALK                               36  //
#define FSNEAK                              37  //
#define FDOACTION                           38  //
#define FKEEPACTION                         39  //
#define FISSUEORDER                         40  //
#define FDROPWEAPONS                        41  //
#define FTARGETDOACTION                     42  //
#define FOPENPASSAGE                        43  //
#define FCLOSEPASSAGE                       44  //
#define FIFPASSAGEOPEN                      45  //
#define FGOPOOF                             46  //
#define FCOSTTARGETITEMID                   47  //
#define FDOACTIONOVERRIDE                   48  //
#define FIFHEALED                           49  //
#define FSENDMESSAGE                        50  //
#define FCALLFORHELP                        51  //
#define FADDIDSZ                            52  //
#define FEND                                53  //
#define FSETSTATE                           54  // Scripted AI functions (v0.20)
#define FGETSTATE                           55  //
#define FIFSTATEIS                          56  //
#define FIFTARGETCANOPENSTUFF               57  // Scripted AI functions (v0.30)
#define FIFGRABBED                          58  //
#define FIFDROPPED                          59  //
#define FSETTARGETTOWHOEVERISHOLDING        60  //
#define FDAMAGETARGET                       61  //
#define FIFXISLESSTHANY                     62  //
#define FSETWEATHERTIME                     63  // Scripted AI functions (v0.40)
#define FGETBUMPHEIGHT                      64  //
#define FIFREAFFIRMED                       65  //
#define FUNKEEPACTION                       66  //
#define FIFTARGETISONOTHERTEAM              67  //
#define FIFTARGETISONHATEDTEAM              68  // Scripted AI functions (v0.50)
#define FPRESSLATCHBUTTON                   69  //
#define FSETTARGETTOTARGETOFLEADER          70  //
#define FIFLEADERKILLED                     71  //
#define FBECOMELEADER                       72  //
#define FCHANGETARGETARMOR                  73  // Scripted AI functions (v0.60)
#define FGIVEMONEYTOTARGET                  74  //
#define FDROPKEYS                           75  //
#define FIFLEADERISALIVE                    76  //
#define FIFTARGETISOLDTARGET                77  //
#define FSETTARGETTOLEADER                  78  //
#define FSPAWNCHARACTER                     79  //
#define FRESPAWNCHARACTER                   80  //
#define FCHANGETILE                         81  //
#define FIFUSED                             82  //
#define FDROPMONEY                          83  //
#define FSETOLDTARGET                       84  //
#define FDETACHFROMHOLDER                   85  //
#define FIFTARGETHASVULNERABILITYID         86  //
#define FCLEANUP                            87  //
#define FIFCLEANEDUP                        88  //
#define FIFSITTING                          89  //
#define FIFTARGETISHURT                     90  //
#define FIFTARGETISAPLAYER                  91  //
#define FPLAYSOUND                          92  //
#define FSPAWNPARTICLE                      93  //
#define FIFTARGETISALIVE                    94  //
#define FSTOP                               95  //
#define FDISAFFIRMCHARACTER                 96  //
#define FREAFFIRMCHARACTER                  97  //
#define FIFTARGETISSELF                     98  //
#define FIFTARGETISMALE                     99  //
#define FIFTARGETISFEMALE                   100 //
#define FSETTARGETTOSELF                    101 // Scripted AI functions (v0.70)
#define FSETTARGETTORIDER                   102 //
#define FGETATTACKTURN                      103 //
#define FGETDAMAGETYPE                      104 //
#define FBECOMESPELL                        105 //
#define FBECOMESPELLBOOK                    106 //
#define FIFSCOREDAHIT                       107 //
#define FIFDISAFFIRMED                      108 //
#define FTRANSLATEORDER                     109 //
#define FSETTARGETTOWHOEVERWASHIT           110 //
#define FSETTARGETTOWIDEENEMY               111 //
#define FIFCHANGED                          112 //
#define FIFINWATER                          113 //
#define FIFBORED                            114 //
#define FIFTOOMUCHBAGGAGE                   115 //
#define FIFGROGGED                          116 //
#define FIFDAZED                            117 //
#define FIFTARGETHASSPECIALID               118 //
#define FPRESSTARGETLATCHBUTTON             119 //
#define FIFINVISIBLE                        120 //
#define FIFARMORIS                          121 //
#define FGETTARGETGROGTIME                  122 //
#define FGETTARGETDAZETIME                  123 //
#define FSETDAMAGETYPE                      124 //
#define FSETWATERLEVEL                      125 //
#define FENCHANTTARGET                      126 //
#define FENCHANTCHILD                       127 //
#define FTELEPORTTARGET                     128 //
#define FGIVEEXPERIENCETOTARGET             129 //
#define FINCREASEAMMO                       130 //
#define FUNKURSETARGET                      131 //
#define FGIVEEXPERIENCETOTARGETTEAM         132 //
#define FIFUNARMED                          133 //
#define FRESTOCKTARGETAMMOIDALL             134 //
#define FRESTOCKTARGETAMMOIDFIRST           135 //
#define FFLASHTARGET                        136 //
#define FSETREDSHIFT                        137 //
#define FSETGREENSHIFT                      138 //
#define FSETBLUESHIFT                       139 //
#define FSETLIGHT                           140 //
#define FSETALPHA                           141 //
#define FIFHITFROMBEHIND                    142 //
#define FIFHITFROMFRONT                     143 //
#define FIFHITFROMLEFT                      144 //
#define FIFHITFROMRIGHT                     145 //
#define FIFTARGETISONSAMETEAM               146 //
#define FKILLTARGET                         147 //
#define FUNDOENCHANT                        148 //
#define FGETWATERLEVEL                      149 //
#define FCOSTTARGETMANA                     150 //
#define FIFTARGETHASANYID                   151 //
#define FSETBUMPSIZE                        152 //
#define FIFNOTDROPPED                       153 //
#define FIFYISLESSTHANX                     154 //
#define FSETFLYHEIGHT                       155 //
#define FIFBLOCKED                          156 //
#define FIFTARGETISDEFENDING                157 //
#define FIFTARGETISATTACKING                158 //
#define FIFSTATEIS0                         159 //
#define FIFSTATEIS1                         160 //
#define FIFSTATEIS2                         161 //
#define FIFSTATEIS3                         162 //
#define FIFSTATEIS4                         163 //
#define FIFSTATEIS5                         164 //
#define FIFSTATEIS6                         165 //
#define FIFSTATEIS7                         166 //
#define FIFCONTENTIS                        167 //
#define FSETTURNMODETOWATCHTARGET           168 //
#define FIFSTATEISNOT                       169 //
#define FIFXISEQUALTOY                      170 //
#define FDEBUGMESSAGE                       171 //
#define FBLACKTARGET                        172 // Scripted AI functions (v0.80)
#define FSENDMESSAGENEAR                    173 //
#define FIFHITGROUND                        174 //
#define FIFNAMEISKNOWN                      175 //
#define FIFUSAGEISKNOWN                     176 //
#define FIFHOLDINGITEMID                    177 //
#define FIFHOLDINGRANGEDWEAPON              178 //
#define FIFHOLDINGMELEEWEAPON               179 //
#define FIFHOLDINGSHIELD                    180 //
#define FIFKURSED                           181 //
#define FIFTARGETISKURSED                   182 //
#define FIFTARGETISDRESSEDUP                183 //
#define FIFOVERWATER                        184 //
#define FIFTHROWN                           185 //
#define FMAKENAMEKNOWN                      186 //
#define FMAKEUSAGEKNOWN                     187 //
#define FSTOPTARGETMOVEMENT                 188 //
#define FSETXY                              189 //
#define FGETXY                              190 //
#define FADDXY                              191 //
#define FMAKEAMMOKNOWN                      192 //
#define FSPAWNATTACHEDPARTICLE              193 //
#define FSPAWNEXACTPARTICLE                 194 //
#define FACCELERATETARGET                   195 //
#define FIFDISTANCEISMORETHANTURN           196 //
#define FIFCRUSHED                          197 //
#define FMAKECRUSHVALID                     198 //
#define FSETTARGETTOLOWESTTARGET            199 //
#define FIFNOTPUTAWAY                       200 //
#define FIFTAKENOUT                         201 //
#define FIFAMMOOUT                          202 //
#define FPLAYSOUNDLOOPED                    203 //
#define FSTOPSOUND                          204 //
#define FHEALSELF                           205 //
#define FEQUIP                              206 //
#define FIFTARGETHASITEMIDEQUIPPED          207 //
#define FSETOWNERTOTARGET                   208 //
#define FSETTARGETTOOWNER                   209 //
#define FSETFRAME                           210 //
#define FBREAKPASSAGE                       211 //
#define FSETRELOADTIME                      212 //
#define FSETTARGETTOWIDEBLAHID              213 //
#define FPOOFTARGET                         214 //
#define FCHILDDOACTIONOVERRIDE              215 //
#define FSPAWNPOOF                          216 //
#define FSETSPEEDPERCENT                    217 //
#define FSETCHILDSTATE                      218 //
#define FSPAWNATTACHEDSIZEDPARTICLE         219 //
#define FCHANGEARMOR                        220 //
#define FSHOWTIMER                          221 //
#define FIFFACINGTARGET                     222 //
#define FPLAYSOUNDVOLUME                    223 //
#define FSPAWNATTACHEDFACEDPARTICLE         224 //
#define FIFSTATEISODD                       225 //
#define FSETTARGETTODISTANTENEMY            226 //
#define FTELEPORT                           227 //
#define FGIVESTRENGTHTOTARGET               228 //
#define FGIVEWISDOMTOTARGET                 229 //
#define FGIVEINTELLIGENCETOTARGET           230 //
#define FGIVEDEXTERITYTOTARGET              231 //
#define FGIVELIFETOTARGET                   232 //
#define FGIVEMANATOTARGET                   233 //
#define FSHOWMAP                            234 //
#define FSHOWYOUAREHERE                     235 //
#define FSHOWBLIPXY                         236 //
#define FHEALTARGET                         237 //
#define FPUMPTARGET                         238 //
#define FCOSTAMMO                           239 //
#define FMAKESIMILARNAMESKNOWN              240 //
#define FSPAWNATTACHEDHOLDERPARTICLE        241 //
#define FSETTARGETRELOADTIME                242 //
#define FSETFOGLEVEL                        243 //
#define FGETFOGLEVEL                        244 //
#define FSETFOGTAD                          245 //
#define FSETFOGBOTTOMLEVEL                  246 //
#define FGETFOGBOTTOMLEVEL                  247 //
#define FCORRECTACTIONFORHAND               248 //
#define FIFTARGETISMOUNTED                  249 //
#define FSPARKLEICON                        250 //
#define FUNSPARKLEICON                      251 //
#define FGETTILEXY                          252 //
#define FSETTILEXY                          253 //
#define FSETSHADOWSIZE                      254 //
#define FORDERTARGET                        255 //
#define FSETTARGETTOWHOEVERISINPASSAGE      256 //
#define FIFCHARACTERWASABOOK                257 //
#define FSETENCHANTBOOSTVALUES              258 // Scripted AI functions (v0.90)
#define FSPAWNCHARACTERXYZ                  259 //
#define FSPAWNEXACTCHARACTERXYZ             260 //
#define FCHANGETARGETCLASS                  261 //
#define FPLAYFULLSOUND                      262 //
#define FSPAWNEXACTCHASEPARTICLE            263 //
#define FCREATEORDER                        264 //
#define FORDERSPECIALID                     265 //
#define FUNKURSETARGETINVENTORY             266 //
#define FIFTARGETISSNEAKING                 267 //
#define FDROPITEMS                          268 //
#define FRESPAWNTARGET                      269 //
#define FTARGETDOACTIONSETFRAME             270 //
#define FIFTARGETCANSEEINVISIBLE            271 //
#define FSETTARGETTONEARESTBLAHID           272 //
#define FSETTARGETTONEARESTENEMY            273 //
#define FSETTARGETTONEARESTFRIEND           274 //
#define FSETTARGETTONEARESTLIFEFORM         275 //
#define FFLASHPASSAGE                       276 //
#define FFINDTILEINPASSAGE                  277 //
#define FIFHELDINLEFTHAND                   278 //
#define FNOTANITEM                          279 //
#define FSETCHILDAMMO                       280 //
#define FIFHITVULNERABLE                    281 //
#define FIFTARGETISFLYING                   282 //
#define FIDENTIFYTARGET                     283 //
#define FBEATMODULE                         284 //
#define FENDMODULE                          285 //
#define FDISABLEEXPORT                      286 //
#define FENABLEEXPORT                       287 //
#define FGETTARGETSTATE                     288 //
#define FIFEQUIPPED                         289 //Redone in v 0.95
#define FDROPTARGETMONEY                    290 //
#define FGETTARGETCONTENT					291 //
#define FDROPTARGETKEYS                     292 //
#define FJOINTEAM							293 //
#define FTARGETJOINTEAM                     294 //
#define FCLEARMUSICPASSAGE                  295 //Below is original code again
#define FCLEARENDMESSAGE                    296 //
#define FADDENDMESSAGE                      297 //
#define FPLAYMUSIC                          298 //
#define FSETMUSICPASSAGE                    299 //
#define FMAKECRUSHINVALID                   300 //
#define FSTOPMUSIC                          301 //
#define FFLASHVARIABLE                      302 //
#define FACCELERATEUP                       303 //
#define FFLASHVARIABLEHEIGHT                304 //
#define FSETDAMAGETIME                      305 //
#define FIFSTATEIS8                         306 //
#define FIFSTATEIS9                         307 //
#define FIFSTATEIS10                        308 //
#define FIFSTATEIS11                        309 //
#define FIFSTATEIS12                        310 //
#define FIFSTATEIS13                        311 //
#define FIFSTATEIS14                        312 //
#define FIFSTATEIS15                        313 //
#define FIFTARGETISAMOUNT                   314 //
#define FIFTARGETISAPLATFORM                315 //
#define FADDSTAT                            316 //
#define FDISENCHANTTARGET                   317 //
#define FDISENCHANTALL                      318 //
#define FSETVOLUMENEARESTTEAMMATE           319 //
#define FADDSHOPPASSAGE                     320 //
#define FTARGETPAYFORARMOR                  321 //
#define FJOINEVILTEAM                       322 //
#define FJOINNULLTEAM                       323 //
#define FJOINGOODTEAM                       324 //
#define FPITSKILL                           325 //
#define FSETTARGETTOPASSAGEID               326 //
#define FMAKENAMEUNKNOWN                    327 //
#define FSPAWNEXACTPARTICLEENDSPAWN         328 //
#define FSPAWNPOOFSPEEDSPACINGDAMAGE        329 //
#define FGIVEEXPERIENCETOGOODTEAM           330 //
#define FDONOTHING                          331 // Scripted AI functions (v0.95)
#define FGROGTARGET                         332 //
#define FDAZETARGET                         333 //
#define FADDQUEST                           334 // Scripted AI functions (v1.00)
#define FBEATQUEST                          335 //
#define FIFTARGETHASQUEST                   336 //

#define OPADD 0        // +
#define OPSUB 1        // -
#define OPAND 2        // &
#define OPSHR 3        // >
#define OPSHL 4        // <
#define OPMUL 5        // *
#define OPDIV 6        // /
#define OPMOD 7        // %

#define VARTMPX             0
#define VARTMPY             1
#define VARTMPDISTANCE      2
#define VARTMPTURN          3
#define VARTMPARGUMENT      4
#define VARRAND             5
#define VARSELFX            6
#define VARSELFY            7
#define VARSELFTURN         8
#define VARSELFCOUNTER      9
#define VARSELFORDER        10
#define VARSELFMORALE       11
#define VARSELFLIFE         12
#define VARTARGETX          13
#define VARTARGETY          14
#define VARTARGETDISTANCE   15
#define VARTARGETTURN       16
#define VARLEADERX          17
#define VARLEADERY          18
#define VARLEADERDISTANCE   19
#define VARLEADERTURN       20
#define VARGOTOX            21
#define VARGOTOY            22
#define VARGOTODISTANCE     23
#define VARTARGETTURNTO     24
#define VARPASSAGE          25
#define VARWEIGHT           26
#define VARSELFALTITUDE     27
#define VARSELFID           28
#define VARSELFHATEID       29
#define VARSELFMANA         30
#define VARTARGETSTR        31
#define VARTARGETWIS        32
#define VARTARGETINT        33
#define VARTARGETDEX        34
#define VARTARGETLIFE       35
#define VARTARGETMANA       36
#define VARTARGETLEVEL      37
#define VARTARGETSPEEDX     38
#define VARTARGETSPEEDY     39
#define VARTARGETSPEEDZ     40
#define VARSELFSPAWNX       41
#define VARSELFSPAWNY       42
#define VARSELFSTATE        43
#define VARSELFSTR          44
#define VARSELFWIS          45
#define VARSELFINT          46
#define VARSELFDEX          47
#define VARSELFMANAFLOW     48
#define VARTARGETMANAFLOW   49
#define VARSELFATTACHED     50
#define VARSWINGTURN        51
#define VARXYDISTANCE       52
#define VARSELFZ            53
#define VARTARGETALTITUDE   54
#define VARTARGETZ          55
#define VARSELFINDEX        56
#define VAROWNERX           57
#define VAROWNERY           58
#define VAROWNERTURN        59
#define VAROWNERDISTANCE    60
#define VAROWNERTURNTO      61
#define VARXYTURNTO         62
#define VARSELFMONEY        63
#define VARSELFACCEL        64
#define VARTARGETEXP        65
#define VARSELFAMMO         66
#define VARTARGETAMMO       67
#define VARTARGETMONEY      68
#define VARTARGETTURNAWAY   69
#define VARSELFLEVEL     70

EXTERN Uint16 valueoldtarget EQ(0);
EXTERN int valuetmpx EQ(0);
EXTERN int valuetmpy EQ(0);
EXTERN Uint16 valuetmpturn EQ(0);
EXTERN int valuetmpdistance EQ(0);
EXTERN int valuetmpargument EQ(0);
EXTERN Uint32 valuelastindent EQ(0);
EXTERN int valueoperationsum EQ(0);
EXTERN Uint8 valuegopoof;

// This stuff is for actions
#define ACTIONDA            0
#define ACTIONDB            1
#define ACTIONDC            2
#define ACTIONDD            3
#define ACTIONUA            4
#define ACTIONUB            5
#define ACTIONUC            6
#define ACTIONUD            7
#define ACTIONTA            8
#define ACTIONTB            9
#define ACTIONTC            10
#define ACTIONTD            11
#define ACTIONCA            12
#define ACTIONCB            13
#define ACTIONCC            14
#define ACTIONCD            15
#define ACTIONSA            16
#define ACTIONSB            17
#define ACTIONSC            18
#define ACTIONSD            19
#define ACTIONBA            20
#define ACTIONBB            21
#define ACTIONBC            22
#define ACTIONBD            23
#define ACTIONLA            24
#define ACTIONLB            25
#define ACTIONLC            26
#define ACTIONLD            27
#define ACTIONXA            28
#define ACTIONXB            29
#define ACTIONXC            30
#define ACTIONXD            31
#define ACTIONFA            32
#define ACTIONFB            33
#define ACTIONFC            34
#define ACTIONFD            35
#define ACTIONPA            36
#define ACTIONPB            37
#define ACTIONPC            38
#define ACTIONPD            39
#define ACTIONEA            40
#define ACTIONEB            41
#define ACTIONRA            42
#define ACTIONZA            43
#define ACTIONZB            44
#define ACTIONZC            45
#define ACTIONZD            46
#define ACTIONWA            47
#define ACTIONWB            48
#define ACTIONWC            49
#define ACTIONWD            50
#define ACTIONJA            51
#define ACTIONJB            52
#define ACTIONJC            53
#define ACTIONHA            54
#define ACTIONHB            55
#define ACTIONHC            56
#define ACTIONHD            57
#define ACTIONKA            58
#define ACTIONKB            59
#define ACTIONKC            60
#define ACTIONKD            61
#define ACTIONMA            62
#define ACTIONMB            63
#define ACTIONMC            64
#define ACTIONMD            65
#define ACTIONME            66
#define ACTIONMF            67
#define ACTIONMG            68
#define ACTIONMH            69
#define ACTIONMI            70
#define ACTIONMJ            71
#define ACTIONMK            72
#define ACTIONML            73
#define ACTIONMM            74
#define ACTIONMN            75


// For damage/stat pair reads/writes
EXTERN int pairbase, pairrand;
EXTERN float pairfrom, pairto;


//Passages
EXTERN int numpassage;       //Number of passages in the module
EXTERN int passtlx[MAXPASS];     //Passage positions
EXTERN int passtly[MAXPASS];
EXTERN int passbrx[MAXPASS];
EXTERN int passbry[MAXPASS];
EXTERN int passagemusic[MAXPASS];    //Music track appointed to the specific passage
EXTERN Uint8 passmask[MAXPASS];
EXTERN Uint8 passopen[MAXPASS];   //Is the passage open?

// For shops
EXTERN int numshoppassage;
EXTERN Uint16 shoppassage[MAXPASS];  // The passage number
EXTERN Uint16 shopowner[MAXPASS];    // Who gets the gold?
#define NOOWNER 65535


// Status displays
EXTERN int numstat  EQ(0);
EXTERN Uint16 statlist[MAXSTAT];
EXTERN Uint32 particletrans  EQ(0x80);
EXTERN Uint32 antialiastrans  EQ(0xC0);


//Network Stuff
#define CHARVEL 5.0
#define MAXPLAYER   8                               // 2 to a power...  2^3
#define INPUTNONE   0                               //
#define INPUTMOUSE  1                               // Input devices
#define INPUTKEY    2                               //
#define INPUTJOYA   4                               //
#define INPUTJOYB   8                               //
typedef struct player_t
{
  Uint8   valid;                    // Player used?
  Uint16  index;                    // Which character?
  float   latchx;                   // Local latches
  float   latchy;                   //
  Uint8   latchbutton;              //
  Uint8   device;                   // Input device
} PLAYER;

EXTERN PLAYER PlaList[MAXPLAYER];

EXTERN int               numpla;                                 // Number of players
EXTERN int               numlocalpla;                            //
EXTERN int               numfile;                                // For network copy
EXTERN int               numfilesent;                            // For network copy
EXTERN int               numfileexpected;                        // For network copy
EXTERN int               numplayerrespond;                       //





//Music using SDL_Mixer
#define MAXPLAYLISTLENGHT 25      //Max number of different tracks loaded into memory
EXTERN bool_t   musicinmemory EQ(bfalse); //Is the music loaded in memory?
EXTERN Mix_Music        *instrumenttosound[MAXPLAYLISTLENGHT]; //This is a specific music file loaded into memory
EXTERN int              songplaying EQ(-1);    //Current song that is playing



//Some various other stuff
EXTERN Uint8 changed;

//Key/Control input defenitions
#define MAXTAG              128                     // Number of tags in scancode.txt
#define TAGSIZE             32                      // Size of each tag
EXTERN int numscantag;
EXTERN char tagname[MAXTAG][TAGSIZE];                      // Scancode names
EXTERN Uint32 tagvalue[MAXTAG];                     // Scancode values
#define MAXCONTROL          64
EXTERN Uint32 controlvalue[MAXCONTROL];             // The scancode or mask
EXTERN Uint32 controliskey[MAXCONTROL];             // Is it a key?
#define KEY_JUMP            0
#define KEY_LEFT_USE        1
#define KEY_LEFT_GET        2
#define KEY_LEFT_PACK       3
#define KEY_RIGHT_USE       4
#define KEY_RIGHT_GET       5
#define KEY_RIGHT_PACK      6
#define KEY_MESSAGE         7
#define KEY_CAMERA_LEFT     8
#define KEY_CAMERA_RIGHT    9
#define KEY_CAMERA_IN       10
#define KEY_CAMERA_OUT      11
#define KEY_UP              12
#define KEY_DOWN            13
#define KEY_LEFT            14
#define KEY_RIGHT           15
#define MOS_JUMP            16
#define MOS_LEFT_USE        17
#define MOS_LEFT_GET        18
#define MOS_LEFT_PACK       19
#define MOS_RIGHT_USE       20
#define MOS_RIGHT_GET       21
#define MOS_RIGHT_PACK      22
#define MOS_CAMERA          23
#define JOA_JUMP            24
#define JOA_LEFT_USE        25
#define JOA_LEFT_GET        26
#define JOA_LEFT_PACK       27
#define JOA_RIGHT_USE       28
#define JOA_RIGHT_GET       29
#define JOA_RIGHT_PACK      30
#define JOA_CAMERA          31
#define JOB_JUMP            32
#define JOB_LEFT_USE        33
#define JOB_LEFT_GET        34
#define JOB_LEFT_PACK       35
#define JOB_RIGHT_USE       36
#define JOB_RIGHT_GET       37
#define JOB_RIGHT_PACK      38
#define JOB_CAMERA          39

//AI Targeting
EXTERN Uint16 globalnearest;
EXTERN float globaldistance;

// SDL specific declarations
EXTERN SDL_Joystick *sdljoya EQ(NULL);
EXTERN SDL_Joystick *sdljoyb EQ(NULL);
EXTERN Uint8 *sdlkeybuffer;
#define SDLKEYDOWN(k) sdlkeybuffer[k]

#undef DEBUG_ATTRIB

#if defined(DEBUG_ATTRIB) && defined(_DEBUG)
#    define ATTRIB_PUSH(TXT, BITS)    { GLint xx=0; glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); glPushAttrib(BITS); log_info("PUSH  ATTRIB: %s before attrib stack push. level == %d\n", TXT, xx); }
#    define ATTRIB_POP(TXT)           { GLint xx=0; glPopAttrib(); glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&xx); log_info("POP   ATTRIB: %s after attrib stack pop. level == %d\n", TXT, xx); }
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX); log_info("OPEN ATTRIB_GUARD: before attrib stack push. level == %d\n", XX); }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); if(XX!=YY) log_error("CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY); else log_info("CLOSE ATTRIB_GUARD: after attrib stack pop. level == %d\n", XX); }
#elif defined(_DEBUG)
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)     { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&XX);  }
#    define ATTRIB_GUARD_CLOSE(XX,YY) { glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&YY); assert(XX==YY); if(XX!=YY) log_error("CLOSE ATTRIB_GUARD: after attrib stack pop. level conflict %d != %d\n", XX, YY);  }
#else
#    define ATTRIB_PUSH(TXT, BITS)    glPushAttrib(BITS);
#    define ATTRIB_POP(TXT)           glPopAttrib();
#    define ATTRIB_GUARD_OPEN(XX)
#    define ATTRIB_GUARD_CLOSE(XX,YY)
#endif

typedef struct configurable_data_t
{
  STRING basicdat_dir;
  STRING gamedat_dir;
  STRING menu_dir;
  STRING globalparticles_dir;
  STRING modules_dir;
  STRING music_dir;
  STRING objects_dir;
  STRING import_dir;
  STRING players_dir;

  STRING nullicon_bitmap;
  STRING keybicon_bitmap;
  STRING mousicon_bitmap;
  STRING joyaicon_bitmap;
  STRING joybicon_bitmap;

  STRING tile0_bitmap;
  STRING tile1_bitmap;
  STRING tile2_bitmap;
  STRING tile3_bitmap;
  STRING watertop_bitmap;
  STRING waterlow_bitmap;
  STRING phong_bitmap;
  STRING plan_bitmap;
  STRING blip_bitmap;
  STRING font_bitmap;
  STRING icon_bitmap;
  STRING bars_bitmap;
  STRING particle_bitmap;
  STRING title_bitmap;

  STRING menu_main_bitmap;
  STRING menu_advent_bitmap;
  STRING menu_sleepy_bitmap;
  STRING menu_gnome_bitmap;


  STRING slotused_file;
  STRING passage_file;
  STRING aicodes_file;
  STRING actions_file;
  STRING alliance_file;
  STRING fans_file;
  STRING fontdef_file;
  STRING menu_file;
  STRING money1_file;
  STRING money5_file;
  STRING money25_file;
  STRING money100_file;
  STRING weather4_file;
  STRING weather5_file;
  STRING script_file;
  STRING ripple_file;
  STRING scancode_file;
  STRING playlist_file;
  STRING spawn_file;
  STRING wawalite_file;
  STRING defend_file;
  STRING splash_file;
  STRING mesh_file;
  STRING setup_file;
  STRING log_file;
  STRING controls_file;
  STRING data_file;
  STRING copy_file;
  STRING enchant_file;
  STRING message_file;
  STRING naming_file;
  STRING modules_file;
  STRING skin_file;
  STRING credits_file;
  STRING quest_file;

  int    uifont_points;
  int    uifont_points2;
  STRING uifont_ttf;

  STRING coinget_sound;
  STRING defend_sound;
  STRING coinfall_sound;
  STRING lvlup_sound;

  bool_t zreflect;              // Reflection z buffering?
  int    maxtotalmeshvertices;  // of vertices
  bool_t fullscreen;            // Start in CData.fullscreen?
  int    scrd;                   // Screen bit depth
  int    scrx;                 // Screen X size
  int    scry;                 // Screen Y size
  int    scrz;                  // Screen z-buffer depth ( 8 unsupported )
  int    maxmessage;    //
  bool_t messageon;           // Messages?
  int    wraptolerance;     // Status bar

  bool_t  staton;                 // Draw the status bars?

  bool_t  overlayon;          //Draw overlay?
  bool_t  perspective;        // Perspective correct textures?
  bool_t  dither;             // Dithering?
  Uint8 reffadeor;              // 255 = Don't fade reflections

  GLenum  shading;             //Gourad CData.shading?
  bool_t  antialiasing;       //Antialiasing?
  bool_t  refon;              // Reflections?
  bool_t  shaon;              // Shadows?
  int     texturefilter;       //Texture filtering?
  bool_t  wateron;             // Water overlays?
  bool_t  shasprite;          // Shadow sprites?
  bool_t  phongon;                // Do phong overlay?
  bool_t  twolayerwateron;        // Two layer water?
  bool_t  overlayvalid;               // Allow large overlay?
  bool_t  backgroundvalid;            // Allow large background?
  bool_t  fogallowed;          //

  bool_t  soundvalid;     //Allow playing of sound?
  bool_t  musicvalid;     // Allow music and loops?

  int     musicvolume;    //The sound volume of music
  int     soundvolume;    //Volume of sounds played
  int     maxsoundchannel;   //Max number of sounds playing at the same time
  int     buffersize;     //Buffer size set in setup.txt

  Uint8 autoturncamera;             // Type of camera control...

  bool_t   request_network;              // Try to connect?
  int      lag;                                // Lag tolerance
  //int      GOrder.lag;                                // RTS Lag tolerance
  char     net_hosts[MAXNETPLAYER][64];                            // Name for hosting session
  STRING   net_messagename;                         // Name for messages

  Uint8 fpson;               // FPS displayed?

  // Debug option
  SDL_GrabMode GrabMouse;
  bool_t HideMouse;
  bool_t DevMode;
  // Debug option

} CONFIG_DATA;

char * get_config_string(CONFIG_DATA * cd, char * szin, char ** szout);
char * get_config_string_name(CONFIG_DATA * cd, STRING * pconfig_string);


EXTERN bool_t          requested_pageflip EQ(bfalse);
EXTERN bool_t          clearson   EQ(btrue);               // Do we clear every time?
EXTERN bool_t          soundon    EQ(btrue);                // Is the sound alive?
EXTERN bool_t          usefaredge EQ(bfalse);                 // Far edge maps? (Outdoor)
EXTERN Uint8					 doturntime EQ(0);                 // Time for smooth turn

EXTERN STRING      CStringTmp1, CStringTmp2;
EXTERN CONFIG_DATA CData;

struct NetState_t;
struct ClientState_t;
struct ServerState_t;

typedef struct ModuleState_t
{
  bool_t loaded;
  Uint32 seed;                       // the seed for the module
  bool_t respawnvalid;               // Can players respawn with Spacebar?
  bool_t respawnanytime;             // True if it's a small level...
  bool_t importvalid;                // Can it import?
  bool_t exportvalid;                // Can it export?
  bool_t rts_control;                 // Play as a real-time stragedy? BAD REMOVE
  bool_t net_messagemode;             // Input text from GKeyb.board?
  bool_t nolocalplayers;             // Are there any local players?
  bool_t beat;                       // Show Module Ended text?
  int    importamount;       // Number of imports for the active module

} MOD_STATE;

typedef struct module_info_t
{
  char   rank[RANKSIZE];               // Number of stars
  char   longname[MAXCAPNAMESIZE];     // Module names
  char   loadname[MAXCAPNAMESIZE];     // Module load names
  Uint8  importamount;                 // # of import characters
  Uint8  allowexport;                  // Export characters?
  Uint8  minplayers;                   // Number of players
  Uint8  maxplayers;                   //
  Uint8  monstersonly;                 // Only allow monsters
  Uint8  rts_control;                   // Real Time Stragedy?
  Uint8  respawnvalid;                 // Allow respawn

  char   host[MAXCAPNAMESIZE];         // what is the host of this module? blank if network not being used.
  Uint32 texture_idx;                  // which texture do we use?
  bool_t is_hosted;                   // is this module here, or on the server?
  bool_t is_verified;
} MOD_INFO;

bool_t init_mod_state(MOD_STATE * ms, MOD_INFO * mi, Uint32 seed);

typedef struct module_summary_t
{
  int    numlines;                                   // Lines in summary
  char   summary[SUMMARYLINES][SUMMARYSIZE];      // Quest description
} MOD_SUMMARY;

typedef struct GameState_t
{
  bool_t paused;             //Is the game paused?
  bool_t can_pause;          //Pause button avalible?
  bool_t Active;             // Stay in game or quit to windows?
  bool_t menuActive;
  bool_t ingameMenuActive;   // Is the in-game menu active?
  bool_t moduleActive;       // Is the control loop still going?
  Uint32 randie_index;       // The current index of the random number table

  // module parameters
  MOD_INFO    mod;
  MOD_STATE   modstate;

  // local module parameters
  int         selectedModule;
  MOD_INFO    modules[MAXMODULE];
  MOD_SUMMARY modtxt;


  struct ModuleState_t  * ms;
  struct NetState_t     * ns;
  struct ClientState_t  * cs;
  struct ServerState_t  * ss;
  CONFIG_DATA           * cd;
} GAME_STATE;

#endif


