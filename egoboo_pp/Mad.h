#pragma once

#include "egobootypedef.h"
#include "mathstuff.h"
#include "MD2_file.h"

//#include "egoboo.h"

#define MAXFRAME                        (0x80*0x20)    // Max number of frames in all models
#define MAXTEXTURE                      0x0200 //0x80         // Max number of textures
#define MODEL_COUNT                        0x0100         // Max number of models

#define GRIP_POINTS                      4
#define GRIP_COUNT                       2
#define GRIP_VERTICES                    (GRIP_POINTS*GRIP_COUNT) // Each model has 8 grip vertices
#define GRIP_RIGHT                       (2*GRIP_POINTS)          // Right weapon grip starts 8 from last
#define GRIP_LEFT                        (1*GRIP_POINTS)          // Left weapon grip starts 4 from last
#define GRIP_SADDLE                      GRIP_LEFT               // Only weapon grip starts 4 from last
#define GRIP_INVENTORY                   0                      //
#define GRIP_LAST                        1                      // Position for spawn attachments

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

// This stuff is for actions
enum ACTION_TYPE
{
  ACTION_DA = 0,               // DA - Dance ( Typical standing )
  ACTION_DB,                   // DB - Dance ( Bored )
  ACTION_DC,                   // DC - Dance ( Bored )
  ACTION_DD,                   // DD - Dance ( Bored )
  ACTION_UA,                   // UA - Unarmed Attack ( Left )
  ACTION_UB,                   // UB - Unarmed Attack ( Left )
  ACTION_UC,                   // UC - Unarmed Attack ( Right )
  ACTION_UD,                   // UD - Unarmed Attack ( Right )
  ACTION_TA,                   // TA - Thrust Attack ( Left )
  ACTION_TB,                   // TB - Thrust Attack ( Left )
  ACTION_TC,                   // TC - Thrust Attack ( Right )
  ACTION_TD,                   // TD - Thrust Attack ( Right )
  ACTION_CA,                   // CA - Chop Attack ( Left )
  ACTION_CB,                   // CB - Chop Attack ( Left )
  ACTION_CC,                   // CC - Chop Attack ( Right )
  ACTION_CD,                   // CD - Chop Attack ( Right )
  ACTION_SA,                   // SA - Slice Attack ( Left )
  ACTION_SB,                   // SB - Slice Attack ( Left )
  ACTION_SC,                   // SC - Slice Attack ( Right )
  ACTION_SD,                   // SD - Slice Attack ( Right )
  ACTION_BA,                   // BA - Bash Attack ( Left )
  ACTION_BB,                   // BB - Bash Attack ( Left )
  ACTION_BC,                   // BC - Bash Attack ( Right )
  ACTION_BD,                   // BD - Bash Attack ( Right )
  ACTION_LA,                   // LA - Longbow Attack ( Left )
  ACTION_LB,                   // LB - Longbow Attack ( Left )
  ACTION_LC,                   // LC - Longbow Attack ( Right )
  ACTION_LD,                   // LD - Longbow Attack ( Right )
  ACTION_XA,                   // XA - Crossbow Attack ( Left )
  ACTION_XB,                   // XB - Crossbow Attack ( Left )
  ACTION_XC,                   // XC - Crossbow Attack ( Right )
  ACTION_XD,                   // XD - Crossbow Attack ( Right )
  ACTION_FA,                   // FA - Flinged Attack ( Left )
  ACTION_FB,                   // FB - Flinged Attack ( Left )
  ACTION_FC,                   // FC - Flinged Attack ( Right )
  ACTION_FD,                   // FD - Flinged Attack ( Right )
  ACTION_PA,                   // PA - Parry or Block ( Left )
  ACTION_PB,                   // PB - Parry or Block ( Left )
  ACTION_PC,                   // PC - Parry or Block ( Right )
  ACTION_PD,                   // PD - Parry or Block ( Right )
  ACTION_EA,                   // EA - Evade
  ACTION_EB,                   // EB - Evade
  ACTION_RA,                   // RA - Roll
  ACTION_ZA,                   // ZA - Zap Magic ( Left )
  ACTION_ZB,                   // ZB - Zap Magic ( Left )
  ACTION_ZC,                   // ZC - Zap Magic ( Right )
  ACTION_ZD,                   // ZD - Zap Magic ( Right )
  ACTION_WA,                   // WA - Sneak
  ACTION_WB,                   // WB - Walk
  ACTION_WC,                   // WC - Run
  ACTION_WD,                   // WD - Push
  ACTION_JA,                   // JA - Jump
  ACTION_JB,                   // JB - Falling ( End of Jump ) ( Dropped Item left )
  ACTION_JC,                   // JC - Falling [ Dropped item right ]
  ACTION_HA,                   // HA - Hit
  ACTION_HB,                   // HB - Hit
  ACTION_HC,                   // HC - Hit
  ACTION_HD,                   // HD - Hit
  ACTION_KA,                   // KA - Killed
  ACTION_KB,                   // KB - Killed
  ACTION_KC,                   // KC - Killed
  ACTION_KD,                   // KD - Killed
  ACTION_MA,                   // MA - Misc ( Drop Left Item )
  ACTION_MB,                   // MB - Misc ( Drop Right Item )
  ACTION_MC,                   // MC - Misc ( Cheer/Slam Left )
  ACTION_MD,                   // MD - Misc ( Show Off/Slam Right/Rise from ground )
  ACTION_ME,                   // ME - Misc ( Grab Item Left )
  ACTION_MF,                   // MF - Misc ( Grab Item Right )
  ACTION_MG,                   // MG - Misc ( Open Chest )
  ACTION_MH,                   // MH - Misc ( Sit )
  ACTION_MI,                   // MI - Misc ( Ride )
  ACTION_MJ,                   // MJ - Misc ( Object Activated )
  ACTION_MK,                   // MK - Misc ( Snoozing )
  ACTION_ML,                   // ML - Misc ( Unlock )
  ACTION_MM,                   // MM - Misc ( Held Left )
  ACTION_MN,                   // MN - Misc ( Held Right )
  ACTION_COUNT,                // Number of action types
  ACTION_INVALID   =  0xffff   // Action not valid for this character
};















































































//---------------------------------------------------------------------------------------------
struct Frame_Extras
{
  Uint8           framelip;                      // 0-15, How far into action is each frame
  Uint16          framefx;                       // Invincibility, Spawning
};

//---------------------------------------------------------------------------------------------
struct Action_Info
{
  bool            valid;     // false if not valid
  Uint16          start;     // First frame of anim
  Uint16          end;       // One past last frame
};


//---------------------------------------------------------------------------------------------
struct BlendedVertexData
{
  Uint32  frame0;
  Uint32  frame1;
  Uint32  vrtmin;
  Uint32  vrtmax;
  float   lerp;

  // Storage for blended vertices
  vec3_t *Vertices;
  vec3_t *Normals;
  vec4_t *Colors;
  vec2_t *Texture;
  float  *Ambient;      // Lighting hack ( Ooze )


  void construct()
  {
    Vertices = NULL;
    Normals  = NULL;
    Colors   = NULL;
    Texture  = NULL;
    Ambient  = NULL;

    frame0 = 0;
    frame1 = 0;
    vrtmin = 0;
    vrtmax = 0;
    lerp   = 0.0f;
  }

  void deconstruct()
  {
    if(NULL!=Vertices)
    {
      delete [] Vertices;
      Vertices = NULL;
    } 

    if(NULL!=Normals)
    {
      delete [] Normals;
      Normals = NULL;
    } 

    if(NULL!=Colors)
    {
      delete [] Colors;
      Colors = NULL;
    } 

    if(NULL!=Texture)
    {
      delete [] Texture;
      Texture = NULL;
    } 

    if(NULL!=Ambient)
    {
      delete [] Ambient;
      Ambient = NULL;
    } 
  }

  void Allocate(size_t verts)
  {
    deconstruct();

    Vertices = new vec3_t[verts];
    Normals  = new vec3_t[verts];
    Colors   = new vec4_t[verts];
    Texture  = new vec2_t[verts];
    Ambient  = new float[verts];
  }
};


//---------------------------------------------------------------------------------------------
struct Mad  : public TAllocClientStrict<Mad, MODEL_COUNT>, public JF::MD2_Model
{
  friend struct Mad_List;

  char            filename[0x0100];
  bool            loaded;


  Uint16          framestart;                    // Starting frame of model
  Uint16          transvertices;                 // Number to transform
  float           scale;                         // Multiply by value

  Uint16          frameliptowalkframe[5][16];    // For walk animations
  Action_Info     actinfo[ACTION_COUNT];

  Frame_Extras & getExtras(int i) { return m_frame_ex[i]; }

  Mad()  { construct();   }
  ~Mad() { deconstruct(); }

  static ACTION_TYPE what_action(char cTmp);

  void construct()
  {
    m_frame_ex = NULL;
  }

  void deconstruct()
  {
    if(NULL!=m_frame_ex)
    {
      delete [] m_frame_ex;
      m_frame_ex = NULL;
    };
  }

  void initialize()
  {
    // BB> This function gives default values to all parameters
    deconstruct();
    construct();
  };

protected:

  void   rip_actions();
  void   action_copy_correct(Uint16 actiona, Uint16 actionb);
  void   make_framelip(int action);
  void   get_walk_frame(int lip, int action);

  void   check_copy(char* loadname);
  Uint32 get_framefx(Uint32 frame);
  Uint16 test_frame_name(const char * name, char letter);

  Frame_Extras *  m_frame_ex;

  void reset() { JF::MD2_Model::deallocate(); memset(this, 0, sizeof(Mad)); }

private:
  bool load(const char * filename, Mad * pmad = NULL);
};


struct Mad_List : public TAllocListStrict<Mad, MODEL_COUNT>
{

  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;
    _list[i].loaded = false;
    _list[i].filename[0] = 0;
    _list[i].deconstruct();
    my_alist_type::return_one(i);
  }

  Uint32 get_free(Uint32 force = INVALID)
  {
    Uint32 ref = my_alist_type::get_free(force);
    if(INVALID == ref) return INVALID;
    _list[ref].reset();
    return ref;
  };

  Uint32 load_one_mad(const char * filename);
};



extern Mad_List MadList;
typedef Mad_List::index_t    MAD_REF;


//---------------------------------------------------------------------------------------------

extern char cActionName[ACTION_COUNT][2];                  // Two letter name code

//---------------------------------------------------------------------------------------------

void make_md2_equally_lit(MAD_REF & mad_ref);


ACTION_TYPE action_number(const char * name, ACTION_TYPE last = ACTION_INVALID);
Uint32      action_frame(const char * name);

void log_madused(char *savename);

//int rip_md2_header(void);
//void fix_md2_normals(Uint16 modelindex);
//void rip_md2_commands(Uint16 modelindex);
//int rip_md2_frame_name(int frame);
//void rip_md2_frames(Uint16 modelindex);
//int process_one_md2(JF::MD2_Model * mdl);

int vertexconnected(MAD_REF & mad_ref, int vertex);
void get_madtransvertices(MAD_REF & mad_ref);
