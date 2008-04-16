#pragma once

#include "egobootypedef.h"

#define AISMAXCOMPILESIZE   (0x80*0x1000/4)            // For parsing AI scripts
#define MAXLINESIZE         0x0400                    //
#define SCR_COUNT               129                     //
#define MAXCODE             0x0400                    // Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     0x40                      //


struct Script_Info : public TAllocClientStrict<Script_Info, SCR_COUNT>
{
  char              filename[0x0100];
  bool              loaded;

  Uint32 StartPosition;

  void reset() { memset(this,0, sizeof(Script_Info)); }
};

struct Script_List : public TAllocListStrict<Script_Info, SCR_COUNT>
{
  void return_one(Uint32 i)
  {
    if(i>=SIZE) return;
    _list[i].loaded = false;
    _list[i].filename[0] = 0;
    my_alist_type::return_one(i);
  }

  Uint32 load_ai_script(const char *loadname, Uint32 force_idx = INVALID);
  void   reset_ai_script();
};

extern Script_List ScrList;

typedef Script_List::index_t AI_REF;

Uint8 run_function(Uint32 value, int character);
void set_operand(Uint8 variable);
void run_operand(Uint32 value, int character);
void let_character_think(Uint32 character);
void let_ai_think();
