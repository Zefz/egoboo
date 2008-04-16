#ifndef _EGOBOO_H_
#define _EGOBOO_H_

#pragma warning(disable : 4305)                     // Turn off silly warnings
#pragma warning(disable : 4244)                     //

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
#include "Log.h"  //Used to log errors, debug info etc.

// The following magic allows this include to work in multiple files
#ifdef DECLARE_GLOBALS
#    define EXTERN
#    define EQ(x) =x;
#else
#    define EXTERN extern
#    define EQ(x)
#endif

EXTERN char     VERSION[] EQ("2.4.1");     // Version of the game

#ifndef MAX
#define MAX(a,b) ( (a)>(b) ? (a) : (b))
#endif

#define NETREFRESH          1000                    // Every second
#define NONETWORK           GNet.numservice              //
#define MAXMODULE           100                     // Number of modules
#define TITLESIZE           0x80                     // Size of a title image
#define MAXSEQUENCE         0x0100                     // Number of tracks in sequence
#define MAXIMPORT           (36*8)                  // Number of subdirs in IMPORT directory

#define ALLSELECT           2                       // An rts mode

#define NOSPARKLE           0xFF
#define ANYTIME             0xFF                     // Code for respawn_mode...

#define SIZETIME            50                      // Time it takes to resize a character
#define WATCHMIN            .01                     //

#define PRTLEVELFIX         20                      // Fix for shooting over cliffs
#define NOSKINOVERRIDE      -1                      // For import

#define PITDEPTH            -30                     // Depth to kill character
#define PITNOSOUND          -0x0100                    // Stop sound at bottom of pits...

//#define MENUA               0                       // Network type
//#define MENUB               1                       // Start or Join
//#define MENUC               2                       // Open games
//#define MENUD               3                       // Module select
//#define MENUE               4                       // Wait for all machines
//#define MENUF               5                       // Character select
//#define MENUG               6                       // Loading screen
//#define MENUH               7                       // End screen
//#define MENUI               8                       // Initial screen

#define ENDY 8
EXTERN Uint16 endtextindex[ENDY];



#define EXPKEEP 0.75                                // Experience to keep when respawning

#define DISMOUNTZ_VELOCITY        16
#define DISMOUNTZFLY_VELOCITY     4

#define EDGE                0x80                     // Camera bounds
#define FARTRACK            1200                    // For outside modules...
#define EDGETRACK           800                     // Camtrack bounds


#define NOHIDE              0x7F                     // Don't hide


#define MINDAMAGE           0x0100                     // Minimum damage for hurt animation
#define MSGDISTANCE         2000                    // Range for SendMessageNear
EXTERN int wraptolerance  EQ(80);

#define DAMFX_NONE           0                       // Damage effects
#define DAMFX_ARMO           1                       // Armor piercing
#define DAMFX_BLOC           2                       // Cannot be blocked by shield
#define DAMFX_ARRO           4                       // Only hurts the one it's attached to
#define DAMFX_TURN           8                       // Turn to attached direction
#define DAMFX_TIME           16                      //

#define MAXSOUND            0x0200                     // Maximum number of sounds
#define PANLEFT             -10000             // Stereo panning
#define PANMID              0           //
#define PANRIGHT            10000            //
#define VOLMIN              -4000 //DSBVOLUME_MIN   // Volume
#define VOLMAX              0           //
#define VOLSHIFT            1                       // For making sounds fade
#define FRQDEFAULT          22050                   // Frequency
#define FRQRANDOM           (20000+(rand()&0x0FFF))   //
#define VOLUMERATIO         7                       // Volume ratio

#define ULTRABLOODY         2                       // This makes any damage draw blood
#define SPELLBOOK           0x7F                     // The spellbook model


#define HURTDAMAGE          (1*0x0100)                 //

#define MAXSTAT             16                      // Maximum status displays
#define MAXLOCO             3                       // Maximum number of local players

#define MAXMESSAGE          4                       // Number of messages
#define MAXTOTALMESSAGE     0x0400                    //
#define MESSAGEBUFFERSIZE   (MAXTOTALMESSAGE*40)
#define MESSAGETIME         200                     // Time to keep the message alive

enum MADFX_BITS
{
  MADFX_NONE          = 0,
  MADFX_INVICTUS      = 1 <<  0,                // I Invincible
  MADFX_ACTLEFT       = 1 <<  1,                // AL Activate left item
  MADFX_ACTRIGHT      = 1 <<  2,                // AR Activate right item
  MADFX_GRABLEFT      = 1 <<  3,                // GL GO Grab left/Grab only item
  MADFX_GRABRIGHT     = 1 <<  4,                // GR Grab right item
  MADFX_DROPLEFT      = 1 <<  5,                // DL DO Drop left/Drop only item
  MADFX_DROPRIGHT     = 1 <<  6,                // DR Drop right item
  MADFX_STOP          = 1 <<  7,                // S Stop movement
  MADFX_FOOTFALL      = 1 <<  8,                // F Footfall sound
  MADFX_CHARLEFT      = 1 <<  9,                // CL Grab left/Grab only character
  MADFX_CHARRIGHT     = 1 << 10,                // CR Grab right character
  MADFX_POOF          = 1 << 11,                // P Poof
};

#define GRABSIZE            90.0                    // Grab tolerance

enum WALK_LIP_TYPE
{
  LIPDA = 0,                                   // For smooth transitions 'tween
  LIPWA,                                       //   walking rates
  LIPWB,
  LIPWC,
  LIPWD,
  LIPCOUNT
};

#define NULLICON            0                       // Empty hand image


#define RIPPLEAND           15                      // How often ripples spawn
#define RIPPLETOLERANCE     60                      // For deep water
#define SPLASHTOLERANCE     10                      //



#define INVICTUS_TILETIME   0x20                      // Invincibility time (frames)
#define INVICTUS_DAMAGETIME 16                      // Invincibility time (frames)
#define INVICTUS_DEFENDTIME 16                      // Invincibility time (frames)





#define MAXCAPNAMESIZE      0x20                      // Character class names





enum LATCH_BITS
{
  LATCHBIT_NONE         = 0,
  LATCHBIT_USE_LEFT     = 1 <<  0,                   // LEFT_USE  Character button presses
  LATCHBIT_USE_RIGHT    = 1 <<  1,                   // RIGHT_USE
  LATCHBIT_JUMP         = 1 <<  2,                   // JUMP
  LATCHBIT_GET_LEFT     = 1 <<  3,                  // GET_LEFT ( Alts are for grab/drop )
  LATCHBIT_GET_RIGHT    = 1 <<  4,                   // RIGHT_GET
  LATCHBIT_PACK_LEFT    = 1 <<  5,                   // LEFT_PACK ( Packs are for inventory cycle )
  LATCHBIT_PACK_RIGHT   = 1 <<  6,                   // RIGHT_PACK
  LATCHBIT_RESPAWN      = 1 <<  7,                   //
  LATCHBIT_CAMERA_LEFT  = 1 <<  8,                   // CAMERA_LEFT
  LATCHBIT_CAMERA_RIGHT = 1 <<  9,                   // CAMERA_RIGHT
  LATCHBIT_CAMERA_IN    = 1 << 10,                   // CAMERA_IN
  LATCHBIT_CAMERA_OUT   = 1 << 11                    // CAMERA_OUT
};

#define MAPID 0x4470614d                      // The string... MapD

#define RAISE 12 //25                               // Helps correct z level
#define SHADOWRAISE 5                               //
#define DAMAGE_RAISE 25                              //

#define DONTFLASH 0xFF                               //


#define NUMBAR                          6           // Number of status bars
#define TABX                            0x20//16      // Size of little name tag on the bar
#define BARX                            112//216         // Size of bar
#define BARY                            16//8           //
#define NUMTICK                         10//50          // Number of ticks per row
#define TICKX                           8//4           // X size of each tick
#define MAXTICK                         (NUMTICK*5) // Max number of ticks to draw

#define CAMKEYTURN                      10          // Device_Keyboard camera rotation
#define CAMJOYTURN                      .01         // Device_Joystick camera rotation

// Multi cam
#define MINZOOM                         500         // Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         800         // Camera height
#define MAXZADD                         1500  //1000        //
#define MINUPDOWN                       (.24*PI)    // Camera updown angle
#define MAXUPDOWN                       (.18*PI)//(.15*PI) // (.18*PI)    //

#define MD2START                        0x32504449  // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (0x0200*0x0400)  // Don't load any models bigger than 512k
#define MD2LIGHTINDICES                 163//162    // MD2's store vertices as x,y,z,normal
#define EQUALLIGHTINDEX                 162         // I added an extra index to do the
// spikey mace...

#define MAXCOMMAND                      0x0100         // Max number of commands
#define MAXCOMMANDSIZE                  0x40          // Max number of points in a command
#define MAXCOMMANDENTRIES               0x0200//0x0100         // Max entries in a command list ( trigs )

#define MODEL_COUNT                        0x0100         // Max number of models

#define MAXCHRBIT                       (0x0100>>5)    // Bitwise compression
#define MAXLIGHTLEVEL                   16          // Number of premade light intensities
#define MAXSPEKLEVEL                    16          // Number of premade specularities
#define MAXLIGHTROTATION                0x0100         // Number of premade light maps

#define GID_HAL                         0           // For deviceguid
#define GID_RAMP                        1           //
#define GID_MMX                         2           //
#define GID_RGB                         3           //

#define SPAWNORIGIN                     0                      // Center for spawn attachments



#define SLOPE                           800         // Slope increments for terrain normals
#define SLIDE                           .04         // Acceleration for steep hills
#define SLIDEFIX                        .08         // To make almost flat surfaces flat

enum SPRITE_TYPE
{
  SPRITE_LIGHT                  = 0,           // Magic effect particle
  SPRITE_SOLID,                                // Sprite particle
  SPRITE_ALPHA,                                // Smoke particle
  SPRITE_ATTACK                                // Attack particle
};

#ifndef CLOCK_PER_SEC
#define CLOCKS_PER_SEC          1000        // Windows ticks 1000 times per second
#endif

#define FRAMESKIP                       int(CLOCKS_PER_SEC/50) //20          // 1000/20 = 50 game updates per second
#define ONESECOND                       int(CLOCKS_PER_SEC/FRAMESKIP)
#define STOPBOUNCING                    0.1 //1.0         // To make objects stop bouncing
#define STOPBOUNCINGPART                5.0         // To make particles stop bouncing

#define COLORKEY_TRANS                   0           // Transparent color
#define COLORKEY_INVALID                ((Uint32)(-1))           // Transparent color
#define BORETIME                        (rand()&0xFF)+120
#define CAREFULTIME                     50

// Debug option
EXTERN bool gGrabMouse EQ(true);
EXTERN bool gHideMouse EQ(false);
// Debug option

#define NORTH 0x4000                                 // Character facings
#define SOUTH 49152                                 //
#define EAST 0x8000                                  //
#define WEST 0                                      //
#define FRONT 0                                     // Attack directions
#define BEHIND 0x8000                                //
#define LEFT 49152                                  //
#define RIGHT 0x4000                                 //

#define TILESOUNDTIME 16
#define TILEREAFFIRMAND  3

//Minimap stuff
#define MAXBLIP 0x20

struct Blip
{
  static rect_t rect;           // The blip rectangles

  Uint16 x;
  Uint16 y;
  Uint8  c;

  static Uint16   Blip::numblip;
  static Uint8    Blip::mapon;
  static Uint8    Blip::youarehereon;
};

EXTERN Blip BlipList[MAXBLIP];

EXTERN Uint8           timeron  EQ(false);            // Game timer displayed?
EXTERN Uint32            timervalue  EQ(0);             // Timer time ( 50ths of a second )
EXTERN char                    szfpstext[]  EQ("000 FPS");
EXTERN Uint8           fpson  EQ(true);               // FPS displayed?
EXTERN Sint32              sttclock;                   // GetTickCount at start
EXTERN Sint32              allclock  EQ(0);               // The total number of ticks so far
EXTERN Sint32              lstclock  EQ(0);               // The last total of ticks so far
EXTERN Sint32              wldclock  EQ(0);               // The sync clock
EXTERN Sint32              fpsclock  EQ(0);               // The number of ticks this second
EXTERN Uint32            wldframe  EQ(0);               // The number of frames that should have been drawn
EXTERN Uint32            allframe  EQ(0);               // The total number of frames drawn so far
EXTERN Uint32          fpsframe  EQ(0);               // The number of frames drawn this second
EXTERN Uint8           statclock  EQ(0);              // For stat regeneration
EXTERN Uint8           pitclock  EQ(0);               // For pit kills
EXTERN bool            pitskill  EQ(false);            // Do they kill?
EXTERN bool            outofsync  EQ(false);
EXTERN bool            parseerror  EQ(false);

EXTERN bool                    fullscreen EQ(false);          // Set to 1 if deviceguid == 0
EXTERN bool                    videoacceleratoron;            // Set to 1 if deviceguid == 0
EXTERN bool                    clearson  EQ(1);               // Do we clear every time?
EXTERN bool                    gameactive  EQ(false);         // Stay in game or quit to windows?
EXTERN bool                    moduleactive  EQ(false);       // Is the control loop still going?
EXTERN bool                    soundon  EQ(1);                // Is the sound alive?
EXTERN bool                    staton  EQ(1);                 // Draw the status bars?
EXTERN bool                    phongon  EQ(1);                // Do phong overlay?
EXTERN bool                    serviceon  EQ(false);          // Do I need to free the interface?
EXTERN bool                    twolayerwateron  EQ(1);        // Two layer water?
EXTERN bool                    menuaneeded  EQ(false);         // Give them MENUA?
EXTERN bool                    menuactive  EQ(false);         // Menu running?
EXTERN bool                    hostactive  EQ(false);         // Hosting?
EXTERN bool                    readytostart;               // Ready to hit the Start Game button?
EXTERN bool                    waitingforplayers;          // Has everyone talked to the host?
EXTERN bool                    alllocalpladead;            // Has everyone died?
EXTERN bool                    respawn_mode;               // Can players respawn with Spacebar?
EXTERN bool                    respawnanytime;             // True if it's a small level...
EXTERN bool                    importvalid;                // Can it import?
EXTERN bool                    exportvalid;                // Can it export?
EXTERN bool                    allselect;                  // Select entire team at start?
EXTERN bool                    backgroundvalid;            // Allow large background?
EXTERN bool                    overlayvalid;               // Allow large overlay?
EXTERN bool                    nolocalplayers;             // Are there any local players?
EXTERN bool                    use_faredge;                 //
EXTERN bool                    beatmodule;                 // Show Module Ended text?
EXTERN Uint8                   cam_autoturn;             // Type of camera control...
EXTERN Uint8                   cam_turn_time;                 // Time for smooth turn

#define TURNTIME 16
EXTERN int                     importamount;               // Number of imports for this module
EXTERN int                     playeramount;               //
EXTERN Uint32                  seed  EQ(0);                   // The module seed
EXTERN char                    pickedmodule[0x40];           // The module load name
EXTERN int                     pickedindex;                // The module index number
EXTERN int                     playersready;               // Number of players ready to start
EXTERN int                     playersloaded;              //



#define MAXSELECT 16
#define MAXORDER  0x20

struct RTS_Info
{
  bool            on;                 // Play as a real-time stragedy?

  vec2_t          pos;                       // RTS Selection point
  Sint16          stt_x;                    // Selection rectangle
  Sint16          stt_y;                    //
  Sint16          end_x;                    //
  Sint16          end_y;                    //
  float           scrollrate;              //
  float           trailrate;               //
  Uint8           set_camera;               //
  float           set_camera_x;              //
  float           set_camera_y;              //
  Uint8           team_local;               //
  Uint16          select_count;               //
  Uint16          select[MAXSELECT];       //

  RTS_Info()
  {
    on =   false;

    scrollrate  = .6;         //
    trailrate   = .5;         //
    team_local   = 0;          //
  };
};

EXTERN RTS_Info GRTS;

EXTERN int                     localmachine  EQ(0);           // 0 is host, 1 is 1st remote, 2 is 2nd...
EXTERN int                     numimport;                  // Number of imports from this machine
EXTERN Uint8                   localcontrol[16];           // For local imports
EXTERN short                   localslot[16];              // For local imports

struct Order
{
  static int      lag;             //

  bool            valid;           // Network orders
  Uint16          who[MAXSELECT];  //
  Uint32          what;            //
  Uint32          when;            //
};

EXTERN Order GOrder[MAXORDER];

#define RTSFOV    75.0                                  // Fix for RTS orders
#define RTSSTEP   20.0                                  // Accuracy of order x, y
#define RTSMOVE    0
#define RTSATTACK  1
#define RTSASSIST  2
#define RTSSTAND   3
#define RTSTERRAIN 4

//Setup values
EXTERN int                     maxmessage  EQ(MAXMESSAGE);    //

EXTERN int            scrd  EQ(8);                   // Screen bit depth
EXTERN int            scrz  EQ(16);                  // Screen z-buffer depth ( 8 unsupported )
EXTERN int            scrx  EQ(320);                 // Screen X size
EXTERN int            scry  EQ(200);                 // Screen Y size

EXTERN Uint8          reffadeor  EQ(0);              // 0xFF = Don't fade reflections
EXTERN GLenum         antialias      EQ(GL_FASTEST);         // Antialiasing
EXTERN GLenum         shading        EQ(GL_SMOOTH);           // Shade mode (_FLAT, _SMOOTH)
EXTERN GLenum         fillmode_front EQ(GL_FILL);    // (GL_POINT, GL_LINE, or GL_FILL)
EXTERN GLenum         fillmode_back  EQ(GL_FILL);     // (GL_POINT, GL_LINE, or GL_FILL)
EXTERN GLenum         perspective    EQ(GL_DONT_CARE);       // Perspective correct textures? (GL_FASTEST, GL_NICEST, GL_DONT_CARE)
EXTERN bool           dither         EQ(false);             // Dithering?
//EXTERN GLenum         filter_min;        // Texture blurring ( _LINEAR/_NEAREST )
//EXTERN GLenum         filter_mag;        // Texture blurring ( _LINEAR/_NEAREST )
//EXTERN bool                  perspective  EQ(false);        // Perspective correct textures?
//EXTERN bool                  shading  EQ(false);             //Gourad shading?
//EXTERN bool                  antialiasing  EQ(false);       //Antialiasing?


EXTERN bool                  overlay_on  EQ(false);          //Draw overlay?


EXTERN Uint8                 messageon  EQ(true);           // Messages?
EXTERN bool                  refon  EQ(false);              // Reflections?
EXTERN bool                  shaon  EQ(false);              // Shadows?
EXTERN bool                  wateron  EQ(true);             // Water overlays?
EXTERN bool                  shasprite  EQ(false);          // Shadow sprites?
EXTERN bool                  zreflect  EQ(false);           // Reflection z buffering?




// EWWWW. GLOBALS ARE EVIL.


/* PORT
LPDIRECTDRAW            lpDD  EQ(NULL);                // DirectDraw object
LPDIRECTDRAWSURFACE     lpDDSPrimary EQ(NULL);         // DirectDraw primary surface
LPDIRECTDRAWSURFACE     lpDDSBack  EQ(NULL);           // DirectDraw back surface
LPDIRECTDRAWSURFACE     lpDDSZbuffer  EQ(NULL);        // DirectDraw z-buffer surface */



EXTERN int                     numloadplayer  EQ(0);
#define MAXLOADPLAYER   100
EXTERN char                    loadplayername[MAXLOADPLAYER][MAXCAPNAMESIZE];
EXTERN char                    loadplayerdir[MAXLOADPLAYER][16];


//EXTERN int       nextmenu  EQ(MENUI);           // Start at the title screen
#define TRIMX 640
#define TRIMY 480
EXTERN rect_t                    trimrect;                   // The menu trim rectangle


EXTERN rect_t                    tabrect[NUMBAR];            // The tab rectangles
EXTERN rect_t                    barrect[NUMBAR];            // The bar rectangles
EXTERN rect_t                    maprect;                    // The map rectangle

#define SPARKLESIZE 28
#define SPARKLEADD 2
#define MAPSIZE 0x40
#define BLIPSIZE 3


EXTERN Uint8           lighttable[MAXLIGHTLEVEL][MAXLIGHTROTATION][MD2LIGHTINDICES];
EXTERN Uint8           cLoadBuffer[MD2MAXLOADSIZE];// Where to put an MD2
//EXTERN float           turntosin[0x4000];           // Convert chrturn>>2...  to sine




EXTERN Uint32            map_twist_lr[0x0100];            // For surface normal of mesh
EXTERN Uint32            map_twist_ud[0x0100];            //
EXTERN float             vel_twist_lr[0x0100];            // For sliding down steep hills
EXTERN float             vel_twist_ud[0x0100];            //
EXTERN Uint8             flat_twist[0x0100];             //

#define TRACKXAREALOW     100
#define TRACKXAREAHIGH    180
#define TRACKYAREAMINLOW  320
#define TRACKYAREAMAXLOW  460
#define TRACKYAREAMINHIGH 460
#define TRACKYAREAMAXHIGH 600

#define SEEKURSEAND         0x1F                      // Blacking flash
#define SEEINVISIBLE        0x80                     // Cutoff for invisible characters
#define INVISIBLE           20                      // The character can't be detected
EXTERN bool                    localseeinvisible;
EXTERN bool                    localseekurse;



struct Cap;
struct Mad;
struct Eve;
struct Script_Info;
struct Pip;



#define SPAWNNOCHARACTER        0xFF                 // For particles that spawn characters...

//EXTERN float           textureoffset[0x0100];         // For moving textures



#define RANKSIZE 8
#define SUMMARYLINES 8
#define SUMMARYSIZE  80

struct Module
{
  static int      globalnum;                            // Number of modules

  char            rank[RANKSIZE];               // Number of stars
  char            longname[MAXCAPNAMESIZE];     // Module names
  char            loadname[MAXCAPNAMESIZE];     // Module load names
  Uint8           importamount;                 // # of import characters
  bool            allowexport;                  // Export characters?
  Uint8           minplayers;                   // Number of players
  Uint8           maxplayers;                   //
  Uint8           monstersonly;                 // Only allow monsters
  Uint8           rts_mode;                   // Real Time Stragedy?
  Uint8           respawn_mode;                 // Allow respawn
};

EXTERN Module ModList[MAXMODULE];

EXTERN GLTexture TxTitleImage[MAXMODULE];     //OpenGL title image surfaces

EXTERN int                     numlines;                                   // Lines in summary
EXTERN char                    modsummary[SUMMARYLINES][SUMMARYSIZE];      // Quest description


EXTERN Uint16          madloadframe;                               // Where to load next



// Character profiles

EXTERN Uint32 randsave;







EXTERN const char *globalname  EQ(NULL);  // For debuggin' goto_colon
EXTERN const char *globalparsename  EQ(NULL); // The SCRIPT.TXT filename
EXTERN FILE *globalparseerr  EQ(NULL); // For debuggin' scripted AI
EXTERN FILE *globalnetworkerr  EQ(NULL); // For debuggin' network

EXTERN Uint32          lighttospek[MAXSPEKLEVEL][0x0100];                     //

EXTERN char            cFrameName[16];                                     // MD2 Frame Name

struct Search_Params
{
  Uint16  stt_target;                                      // For find_target
  Uint16  stt_angle;                                       //
  Uint16  use_angle;                                        //
  int     best_distance;
  Uint32  nearest;
  float   distance;
};

EXTERN Search_Params GParams;

#define TXTMSG_SIZE 80

// Display messages

struct Msg_Info : public TAllocClient<Msg_Info, MAXMESSAGE>
{
  Sint16    time;                              //
  char      textdisplay[TXTMSG_SIZE];          // The displayed text
};

struct Msg : public TAllocList<Msg_Info, MAXMESSAGE>
{
  Uint16    timechange;                                    //
  Uint16    start;                                         // The message queue
  Sint16    time[MAXMESSAGE];                              //
  char      textdisplay[MAXMESSAGE][TXTMSG_SIZE];          // The displayed text

  // Message files
  Uint16    total;                                         // The number of messages
  Uint32    totalindex;                                    // Where to put letter
  Uint32    index[MAXTOTALMESSAGE];                        // Where it is
  char      text[MESSAGEBUFFERSIZE];                       // The text buffer

  Msg()
  {
    timechange = 0;
    start      = 0;
    total      = 0;
    totalindex = 0;
  }
};

EXTERN Msg GMsg;

// My lil' random number table
#define MAXRAND 0x1000
EXTERN Uint16 randie[MAXRAND];
EXTERN Uint16 randindex;
#define RANDIE randie[randindex];  randindex=(randindex+1)%MAXRAND

#define MAXENDTEXT 0x0400
EXTERN char generictext[80];         // Use for whatever purpose
EXTERN char endtext[MAXENDTEXT];     // The end-module text
EXTERN int endtextwrite;

// This is id's normal table for computing light values
#ifdef DECLARE_GLOBALS
float md2normals[MD2LIGHTINDICES][3] =
{{-0.5257,0.0000,0.8507},{-0.4429,0.2389,0.8642},{-0.2952,0.0000,0.9554},
  {-0.3090,0.5000,0.8090},{-0.1625,0.2629,0.9511},{0.0000,0.0000,1.0000},
  {0.0000,0.8507,0.5257},{-0.1476,0.7166,0.6817},{0.1476,0.7166,0.6817},
  {0.0000,0.5257,0.8507},{0.3090,0.5000,0.8090},{0.5257,0.0000,0.8507},
  {0.2952,0.0000,0.9554},{0.4429,0.2389,0.8642},{0.1625,0.2629,0.9511},
  {-0.6817,0.1476,0.7166},{-0.8090,0.3090,0.5000},{-0.5878,0.4253,0.6882},
  {-0.8507,0.5257,0.0000},{-0.8642,0.4429,0.2389},{-0.7166,0.6817,0.1476},
  {-0.6882,0.5878,0.4253},{-0.5000,0.8090,0.3090},{-0.2389,0.8642,0.4429},
  {-0.4253,0.6882,0.5878},{-0.7166,0.6817,-0.1476},{-0.5000,0.8090,-0.3090},
  {-0.5257,0.8507,0.0000},{0.0000,0.8507,-0.5257},{-0.2389,0.8642,-0.4429},
  {0.0000,0.9554,-0.2952},{-0.2629,0.9511,-0.1625},{0.0000,1.0000,0.0000},
  {0.0000,0.9554,0.2952},{-0.2629,0.9511,0.1625},{0.2389,0.8642,0.4429},
  {0.2629,0.9511,0.1625},{0.5000,0.8090,0.3090},{0.2389,0.8642,-0.4429},
  {0.2629,0.9511,-0.1625},{0.5000,0.8090,-0.3090},{0.8507,0.5257,0.0000},
  {0.7166,0.6817,0.1476},{0.7166,0.6817,-0.1476},{0.5257,0.8507,0.0000},
  {0.4253,0.6882,0.5878},{0.8642,0.4429,0.2389},{0.6882,0.5878,0.4253},
  {0.8090,0.3090,0.5000},{0.6817,0.1476,0.7166},{0.5878,0.4253,0.6882},
  {0.9554,0.2952,0.0000},{1.0000,0.0000,0.0000},{0.9511,0.1625,0.2629},
  {0.8507,-0.5257,0.0000},{0.9554,-0.2952,0.0000},{0.8642,-0.4429,0.2389},
  {0.9511,-0.1625,0.2629},{0.8090,-0.3090,0.5000},{0.6817,-0.1476,0.7166},
  {0.8507,0.0000,0.5257},{0.8642,0.4429,-0.2389},{0.8090,0.3090,-0.5000},
  {0.9511,0.1625,-0.2629},{0.5257,0.0000,-0.8507},{0.6817,0.1476,-0.7166},
  {0.6817,-0.1476,-0.7166},{0.8507,0.0000,-0.5257},{0.8090,-0.3090,-0.5000},
  {0.8642,-0.4429,-0.2389},{0.9511,-0.1625,-0.2629},{0.1476,0.7166,-0.6817},
  {0.3090,0.5000,-0.8090},{0.4253,0.6882,-0.5878},{0.4429,0.2389,-0.8642},
  {0.5878,0.4253,-0.6882},{0.6882,0.5878,-0.4253},{-0.1476,0.7166,-0.6817},
  {-0.3090,0.5000,-0.8090},{0.0000,0.5257,-0.8507},{-0.5257,0.0000,-0.8507},
  {-0.4429,0.2389,-0.8642},{-0.2952,0.0000,-0.9554},{-0.1625,0.2629,-0.9511},
  {0.0000,0.0000,-1.0000},{0.2952,0.0000,-0.9554},{0.1625,0.2629,-0.9511},
  {-0.4429,-0.2389,-0.8642},{-0.3090,-0.5000,-0.8090},{-0.1625,-0.2629,-0.9511},
  {0.0000,-0.8507,-0.5257},{-0.1476,-0.7166,-0.6817},{0.1476,-0.7166,-0.6817},
  {0.0000,-0.5257,-0.8507},{0.3090,-0.5000,-0.8090},{0.4429,-0.2389,-0.8642},
  {0.1625,-0.2629,-0.9511},{0.2389,-0.8642,-0.4429},{0.5000,-0.8090,-0.3090},
  {0.4253,-0.6882,-0.5878},{0.7166,-0.6817,-0.1476},{0.6882,-0.5878,-0.4253},
  {0.5878,-0.4253,-0.6882},{0.0000,-0.9554,-0.2952},{0.0000,-1.0000,0.0000},
  {0.2629,-0.9511,-0.1625},{0.0000,-0.8507,0.5257},{0.0000,-0.9554,0.2952},
  {0.2389,-0.8642,0.4429},{0.2629,-0.9511,0.1625},{0.5000,-0.8090,0.3090},
  {0.7166,-0.6817,0.1476},{0.5257,-0.8507,0.0000},{-0.2389,-0.8642,-0.4429},
  {-0.5000,-0.8090,-0.3090},{-0.2629,-0.9511,-0.1625},{-0.8507,-0.5257,0.0000},
  {-0.7166,-0.6817,-0.1476},{-0.7166,-0.6817,0.1476},{-0.5257,-0.8507,0.0000},
  {-0.5000,-0.8090,0.3090},{-0.2389,-0.8642,0.4429},{-0.2629,-0.9511,0.1625},
  {-0.8642,-0.4429,0.2389},{-0.8090,-0.3090,0.5000},{-0.6882,-0.5878,0.4253},
  {-0.6817,-0.1476,0.7166},{-0.4429,-0.2389,0.8642},{-0.5878,-0.4253,0.6882},
  {-0.3090,-0.5000,0.8090},{-0.1476,-0.7166,0.6817},{-0.4253,-0.6882,0.5878},
  {-0.1625,-0.2629,0.9511},{0.4429,-0.2389,0.8642},{0.1625,-0.2629,0.9511},
  {0.3090,-0.5000,0.8090},{0.1476,-0.7166,0.6817},{0.0000,-0.5257,0.8507},
  {0.4253,-0.6882,0.5878},{0.5878,-0.4253,0.6882},{0.6882,-0.5878,0.4253},
  {-0.9554,0.2952,0.0000},{-0.9511,0.1625,0.2629},{-1.0000,0.0000,0.0000},
  {-0.8507,0.0000,0.5257},{-0.9554,-0.2952,0.0000},{-0.9511,-0.1625,0.2629},
  {-0.8642,0.4429,-0.2389},{-0.9511,0.1625,-0.2629},{-0.8090,0.3090,-0.5000},
  {-0.8642,-0.4429,-0.2389},{-0.9511,-0.1625,-0.2629},{-0.8090,-0.3090,-0.5000},
  {-0.6817,0.1476,-0.7166},{-0.6817,-0.1476,-0.7166},{-0.8507,0.0000,-0.5257},
  {-0.6882,0.5878,-0.4253},{-0.5878,0.4253,-0.6882},{-0.4253,0.6882,-0.5878},
  {-0.4253,-0.6882,-0.5878},{-0.5878,-0.4253,-0.6882},{-0.6882,-0.5878,-0.4253},
  {0,0,0}  // Spikey mace
};
#else
EXTERN float md2normals[MD2LIGHTINDICES][3];
#endif

// These are for the AI script loading/parsing routines
extern int                     iNumAis;

enum ALERT_BITS
{
  ALERT_IF_SPAWNED                      = 1 <<   0,
  ALERT_IF_HITVULNERABLE                = 1 <<   1,
  ALERT_IF_ATWAYPOINT                   = 1 <<   2,
  ALERT_IF_ATLASTWAYPOINT               = 1 <<   3,
  ALERT_IF_ATTACKED                     = 1 <<   4,
  ALERT_IF_BUMPED                       = 1 <<   5,
  ALERT_IF_ORDERED                      = 1 <<   6,
  ALERT_IF_CALLEDFORHELP                = 1 <<   7,
  ALERT_IF_KILLED                       = 1 <<   8,
  ALERT_IF_TARGETKILLED                 = 1 <<   9,
  ALERT_IF_DROPPED                      = 1 <<  10,
  ALERT_IF_GRABBED                      = 1 <<  11,
  ALERT_IF_REAFFIRMED                   = 1 <<  12,
  ALERT_IF_LEADERKILLED                 = 1 <<  13,
  ALERT_IF_USED                         = 1 <<  14,
  ALERT_IF_CLEANEDUP                    = 1 <<  15,
  ALERT_IF_SCOREDAHIT                   = 1 <<  16,
  ALERT_IF_HEALED                       = 1 <<  17,
  ALERT_IF_DISAFFIRMED                  = 1 <<  18,
  ALERT_IF_CHANGED                      = 1 <<  19,
  ALERT_IF_INWATER                      = 1 <<  20,
  ALERT_IF_BORED                        = 1 <<  21,
  ALERT_IF_TOOMUCHBAGGAGE               = 1 <<  22,
  ALERT_IF_GROGGED                      = 1 <<  23,
  ALERT_IF_DAZED                        = 1 <<  24,
  ALERT_IF_HITGROUND                    = 1 <<  25,
  ALERT_IF_NOTDROPPED                   = 1 <<  26,
  ALERT_IF_BLOCKED                      = 1 <<  27,
  ALERT_IF_THROWN                       = 1 <<  28,
  ALERT_IF_CRUSHED                      = 1 <<  29,
  ALERT_IF_NOTPUTAWAY                   = 1 <<  30,
  ALERT_IF_TAKENOUT                     = 1 <<  0x1F
};

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
#define FSETTURNMODE_TOVELOCITY              26  //
#define FSETTURNMODE_TOWATCH                 27  //
#define FSETTURNMODE_TOSPIN                  28  //
#define FSETBUMPHEIGHT                      29  //
#define FIFTARGETHASID                      30  //
#define FIFTARGETHASITEMID                  0x1F  //
#define FIFTARGETHOLDINGITEMID              0x20  //
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
#define FSETWEATHERTIME                     0x3F  // Scripted AI functions (v0.40)
#define FGETBUMPHEIGHT                      0x40  //
#define FIFREAFFIRMED                       65  //
#define FUNKEEPACTION                       66  //
#define FIFTARGETISONOTHERTEAM              67  //
#define FIFTARGETISONHATEDTEAM              68  // Scripted AI functions (v0.50)
#define FPRESSLATCHBIT_                   69  //
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
#define FPRESSTARGETLATCHBIT_             119 //
#define FIFINVISIBLE                        120 //
#define FIFARMORIS                          121 //
#define FGETTARGETGROGTIME                  122 //
#define FGETTARGETDAZETIME                  123 //
#define FSETDAMAGETYPE                      124 //
#define FSETWATERLEVEL                      125 //
#define FENCHANTTARGET                      126 //
#define FENCHANTCHILD                       0x7F //
#define FTELEPORTTARGET                     0x80 //
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
#define FSETTURNMODE_TOWATCHTARGET           168 //
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
#define FORDERTARGET                        0xFF //
#define FSETTARGETTOWHOEVERISINPASSAGE      0x0100 //
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
#define FSETSPEECH                          289 //
#define FSETMOVESPEECH                      290 //
#define FSETSECONDMOVESPEECH                291 //
#define FSETATTACKSPEECH                    292 //
#define FSETASSISTSPEECH                    293 //
#define FSETTERRAINSPEECH                   294 //
#define FSETSELECTSPEECH                    295 //
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

#define OPADD 0
#define OPSUB 1
#define OPAND 2
#define OPSHR 3
#define OPSHL 4
#define OPMUL 5
#define OPDIV 6
#define OPMOD 7

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
#define VARTARGETSTR        0x1F
#define VARTARGETWIS        0x20
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
#define VARSELFMONEY        0x3F
#define VARSELFACCEL        0x40
#define VARTARGETEXP        65
#define VARSELFAMMO         66
#define VARTARGETAMMO       67
#define VARTARGETMONEY      68
#define VARTARGETTURNAWAY   69
#define VARSELFLEVEL     70

enum TMP
{
  TMP_OLDTARGET = 0,
  TMP_X,
  TMP_Y,
  TMP_TURN,
  TMP_DISTANCE,
  TMP_ARGUMENT,
  TMP_LASTINDENT,
  TMP_OPERATIONSUM,
  TMP_GOPOOF,
  TMP_COUNT
};

extern Sint32 val[TMP_COUNT];


// For damage/stat pair reads/writes
EXTERN int pairbase, pairrand;
EXTERN float pairfrom, pairto;



// Status displays
EXTERN int numstat  EQ(0);
EXTERN Uint16 statlist[MAXSTAT];
EXTERN int statdelay  EQ(0);
EXTERN Uint32 particletrans  EQ(0x80);
EXTERN Uint32 antialiastrans  EQ(0xC0);

#define SHORTLATCH float(0x0400)
#define CHAR_VELOCITY 5.0
#define MAXSENDSIZE 0x2000
#define COPYSIZE    0x1000
#define TOTALSIZE   2097152
#define PLAYER_COUNT   8                               // 2 to a power...  2^3

enum INPUT_BITS
{
  INPUT_NONE   = 0,                               //
  INPUT_MOUSE  = 1 << 0,                          // Input devices
  INPUT_KEY    = 1 << 1,                          //
  INPUT_JOYA   = 1 << 2,                          //
  INPUT_JOYB   = 1 << 3                           //
};

#define MAXLAG      0x40                              //
#define LAGAND      0x3F                              //
#define STARTTALK   10                              //

struct Player : public TAllocClient<Player, MODEL_COUNT>
{
  typedef std::pair<Uint32,Latch>   timelatchpair;
  typedef std::deque<timelatchpair> latchqueue;
  typedef latchqueue::iterator      latchit;

  bool           _on;                       // Player used?
  Uint16         index;                    // Which character?

  Latch          latch, latch_old;
  Uint8          device;                   // Input device

  latchqueue     timelatch;

  void reset() { memset(this,0,sizeof(Player)); }
};

struct Player_List : public TAllocList<Player, MODEL_COUNT>
{
  int            count_total;                            // Number of players
  int            count_local;                            //
  Uint32         count_times;

  bool add_player(Uint16 character, Uint8 device);
  void reset_players();

  Player_List()
  {
    count_total = 0;
    count_local = 0;
    count_times = 0;
  };

  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;
    _list[i]._on     = false;
    my_alist_type::return_one(i);
  }

};

EXTERN Player_List PlaList;

EXTERN int                     numfile;                                // For network copy
EXTERN int                     numfilesent;                            // For network copy
EXTERN int                     numfileexpected;                        // For network copy
EXTERN int                     numplayerrespond;                       //

#define TO_ANY_TEXT         25935                               // Message headers
#define TO_HOST_MODULEOK    14951                               //
#define TO_HOST_LATCH       33911                               //
#define TO_HOST_RTS         30376                               //
#define TO_HOST_IM_LOADED   40192                               //
#define TO_HOST_FILE        20482                               //
#define TO_HOST_DIR         49230                               //
#define TO_HOST_FILESENT    13131                               //
#define TO_REMOTE_MODULE    56025                               //
#define TO_REMOTE_LATCH     12715                               //
#define TO_REMOTE_FILE      62198                               //
#define TO_REMOTE_DIR       11034                               //
#define TO_REMOTE_RTS        5143                               //
#define TO_REMOTE_START     51390                               //
#define TO_REMOTE_FILESENT  19903                               //

extern Uint32 nexttimestamp;        // Expected timestamp

//Sound using SDL_Mixer
#define PLAYLISTLENGHT 25       //Max number of different tracks loaded into memory
EXTERN int              maxsoundchannel;   //Max number of sounds playing at the same time
EXTERN Uint8    musicinmemory;    //Is the music loaded in memory?
EXTERN int              musicon;
EXTERN int              buffersize;     //Buffer size set in setup.txt
EXTERN Mix_Chunk  *globalwave[10];   //All sounds loaded into memory
EXTERN Mix_Chunk  *sound;      //Used for playing one selected sound file
EXTERN bool   soundvalid;     //Allow playing of sound?
EXTERN int              soundvolume;    //Volume of sounds played
EXTERN int              channel;     //Which channel the current sound is using
EXTERN Mix_Music        *music;      //Used for playing one selected music file
EXTERN Mix_Music        *instrumenttosound[PLAYLISTLENGHT]; //This is a specific music file loaded into memory
EXTERN bool   musicvalid;     // Allow music and loops?
EXTERN int    musicvolume;    //The sound volume of music
EXTERN int              songplaying;    //Current song that is playing

//Some various other stuff
EXTERN char valueidsz[5];
EXTERN Uint8 changed;

#define MAXTAG              0x80                     // Number of tags in scancode.txt
#define TAGSIZE             0x20                      // Size of each tag
EXTERN int numscantag;
EXTERN char tagname[MAXTAG][TAGSIZE];                      // Scancode names
EXTERN Uint32 tagvalue[MAXTAG];                     // Scancode values

#define CONTROL_COUNT          0x40
struct Control
{
  Uint32 value;
  Uint32 key;
};

struct Control_List : public TList<Control, CONTROL_COUNT>
{
  void read(char *szFilename);
  bool key_is_pressed(Uint32 control);
  bool joya_is_pressed(Uint32 control);
  bool joyb_is_pressed(Uint32 control);
  bool mouse_is_pressed(Uint32 control);
};

EXTERN Control_List CtrlList;

#define KEY_JUMP            0
#define KEY_USE_LEFT        1
#define KEY_GET_LEFT        2
#define KEY_PACK_LEFT       3
#define KEY_USE_RIGHT       4
#define KEY_GET_RIGHT       5
#define KEY_PACK_RIGHT      6
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
#define MOS_USE_LEFT        17
#define MOS_GET_LEFT        18
#define MOS_PACK_LEFT       19
#define MOS_USE_RIGHT       20
#define MOS_GET_RIGHT       21
#define MOS_PACK_RIGHT      22
#define MOS_CAMERA          23

#define JOA_JUMP            24
#define JOA_USE_LEFT        25
#define JOA_GET_LEFT        26
#define JOA_PACK_LEFT       27
#define JOA_USE_RIGHT       28
#define JOA_GET_RIGHT       29
#define JOA_PACK_RIGHT      30
#define JOA_CAMERA          0x1F

#define JOB_JUMP            0x20
#define JOB_USE_LEFT        33
#define JOB_GET_LEFT        34
#define JOB_PACK_LEFT       35
#define JOB_USE_RIGHT       36
#define JOB_GET_RIGHT       37
#define JOB_PACK_RIGHT      38
#define JOB_CAMERA          39

// OPENGL specific declarations
EXTERN int title_tex[0x0200][0x0200];

//#ifdef DECLARE_GLOBALS
////GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};  /* White diffuse light. */
////GLfloat light_position[] = {10.0, 10.0, 30.0, 1.0};  /* nonInfinite light location. */
//#else
////extern GLfloat light_diffuse[];
////extern GLfloat light_position[];
//#endif

EXTERN int win_id;
EXTERN GLuint texName;

#define _VALID_EVE_RANGE(XX)     ((XX)>=0 && (XX) < Eve_List::SIZE)
#define _VALID_MAD_RANGE(XX)     ((XX)>=0 && (XX) < Mad_List::SIZE)
#define _VALID_CAP_RANGE(XX)     ((XX)>=0 && (XX) < Cap_List::SIZE)
#define _VALID_PIP_RANGE(XX)     ((XX)>=0 && (XX) < Pip_List::SIZE)

#define VALID_EVE(XX)     (_VALID_EVE_RANGE(XX.index) && EveList[XX].allocated() && EveList[XX].loaded)
#define VALID_MAD(XX)     (_VALID_MAD_RANGE(XX.index) && MadList[XX].allocated() && MadList[XX].loaded)
#define VALID_CAP(XX)     (_VALID_CAP_RANGE(XX.index) && CapList[XX].allocated() && CapList[XX].loaded)
#define VALID_PIP(XX)     (_VALID_PIP_RANGE(XX.index) && PipList[XX].allocated() && PipList[XX].loaded)

#define INVALID_EVE(XX)     (!VALID_EVE(XX))    
#define INVALID_MAD(XX)     (!VALID_MAD(XX))    
#define INVALID_CAP(XX)     (!VALID_CAP(XX)) 
#define INVALID_PIP(XX)     (!VALID_PIP(XX)) 

#define _VALID_MODEL_RANGE(XX)   ((XX)>=0 && (XX) < Profile_List::SIZE)
#define _VALID_PLAYER_RANGE(XX)  ((XX)>=0 && (XX) < Player_List::SIZE)

#define VALID_MODEL(XX)   (_VALID_MODEL_RANGE(XX) && ProfileList[XX].allocated() && ProfileList[XX].loaded)
#define VALID_PLAYER(XX)  (_VALID_PLAYER_RANGE(XX) && PlaList[XX].allocated() && PlaList[XX]._on && INPUT_NONE!=PlaList[XX].device)

#define INVALID_MODEL(XX)   (!VALID_MODEL(XX))  
#define INVALID_PLAYER(XX)  (!VALID_PLAYER(XX)) 

#include "StateMachine.h"

struct Machine_Root : public StateMachine
{
  //virtual void run(float deltaTime);

  Machine_Root(const JF::Scheduler * s) :
    StateMachine(s, "Root Process", NULL) { finished = false;};

  Machine_Root(const StateMachine * parent) :
    StateMachine("Root Process", parent)  { finished = false; };

  double frameDuration;
  bool   finished;

protected:
  virtual void Begin(float deltaTime);
  //virtual void Enter(float deltaTime);
  virtual void Run(float deltaTime);
  //virtual void Leave(float deltaTime);
  virtual void Finish(float deltaTime);
};

struct Machine_Game : public StateMachine
{
  //virtual void run(float deltaTime);

  double frameDuration;

  Machine_Game(const JF::Scheduler * s) :
    StateMachine(s, "Game Process", NULL) { setRunFrequency(1000.0f / FRAMESKIP); };

  Machine_Game(const StateMachine * parent) :
    StateMachine("Game Process", parent)  { setRunFrequency(1000.0f / FRAMESKIP); };

protected:
  virtual void Begin(float deltaTime);
  //virtual void Enter(float deltaTime);
  virtual void Run(float deltaTime);
  //virtual void Leave(float deltaTime);
  virtual void Finish(float deltaTime);
};




void expand_escape_sequence(char * szBuffer, char format, Uint32 character);
void expand_message(char * write, char * read, Uint32 character);


#endif
