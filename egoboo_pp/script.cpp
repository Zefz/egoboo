// script.c

// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Script.h"
#include "Character.h"
#include "Particle.h"
#include "Mad.h"
#include "Camera.h"
#include "input.h"
#include "Passage.h"
#include "Profile.h"
#include "Enchant.h"
#include "egoboo.h"

Script_List ScrList;


Uint8                   cLineBuffer[MAXLINESIZE];
int                     iLoadSize;
int                     iLineSize;
int                     iNumLine;
Uint8                   cCodeType[MAXCODE];
Uint32                  iCodeValue[MAXCODE];
Uint8                   cCodeName[MAXCODE][MAXCODENAMESIZE];
int                     iCodeIndex;
int                     iCodeValueTmp;
int                     iNumCode;
Uint32                  iCompiledAis[AISMAXCOMPILESIZE];
int                     iAisIndex;



//------------------------------------------------------------------------------
//AI Script Routines------------------------------------------------------------
//------------------------------------------------------------------------------
void insert_space(int position)
{
  // ZZ> This function adds a space into the load line if there isn't one
  //     there already
  Uint8 cTmp, cSwap;

  if (cLineBuffer[position]!=' ')
  {
    cTmp = cLineBuffer[position];
    cLineBuffer[position]=' ';
    position++;
    iLineSize++;
    while (position < iLineSize)
    {
      cSwap=cLineBuffer[position];
      cLineBuffer[position]=cTmp;
      cTmp=cSwap;
      position++;
    }
  }
}

//------------------------------------------------------------------------------
void copy_one_line(int write)
{
  // ZZ> This function copies the line back to the load buffer
  int read;
  Uint8 cTmp;

  read = 0;
  cTmp=cLineBuffer[read];
  while (cTmp != 0)
  {
    cTmp=cLineBuffer[read];  read++;
    cLoadBuffer[write]=cTmp;  write++;
  }

  iNumLine++;
}

//------------------------------------------------------------------------------
int load_one_line(int read)
{
  // ZZ> This function loads a line into the line buffer
  bool stillgoing, foundtext;
  Uint8 cTmp;

  // Parse to start to maintain indentation
  iLineSize = 0;
  stillgoing = true;
  while (stillgoing)
  {
    cTmp = cLoadBuffer[read];
    stillgoing = false;
    if (cTmp==' ')
    {
      read++;
      cLineBuffer[iLineSize]=cTmp; iLineSize++;
      stillgoing = true;
    }
  }

  // Parse to comment or end of line
  foundtext = false;
  stillgoing = true;
  while (stillgoing)
  {
    cTmp = cLoadBuffer[read];  read++;
    if (cTmp=='\t')
      cTmp = ' ';
    if (cTmp != ' ' && cTmp != 0x0d && cTmp != 0x0a && 
        (cTmp != '/' || cLoadBuffer[read]!='/'))
      foundtext=true;
    cLineBuffer[iLineSize]=cTmp;
    if (cTmp!=' ' || (cLoadBuffer[read]!=' ' && cLoadBuffer[read]!='\t'))
      iLineSize++;
    if (cTmp == 0x0d || cTmp == 0x0a)
      stillgoing=false;
    if (cTmp == '/' && cLoadBuffer[read] == '/')
      stillgoing=false;
  }
  iLineSize--;  cLineBuffer[iLineSize]=0;
  if (iLineSize>=1)
  {
    if (cLineBuffer[iLineSize-1]==' ')
    {
      iLineSize--;  cLineBuffer[iLineSize]=0;
    }
  }
  iLineSize++;

  // Parse to end of line
  stillgoing=true;
  read--;
  while (stillgoing)
  {
    cTmp = cLoadBuffer[read];  read++;
    if (cTmp == 0x0a || cTmp==0x0d)
      stillgoing=false;
  }

  if (!foundtext)
  {
    iLineSize=0;
  }

  return read;
}

//------------------------------------------------------------------------------
int load_parsed_line(int read)
{
  // ZZ> This function loads a line into the line buffer
  Uint8 cTmp;

  // Parse to start to maintain indentation
  iLineSize = 0;
  cTmp=cLoadBuffer[read];
  while (cTmp!=0)
  {
    cLineBuffer[iLineSize]=cTmp;  iLineSize++;
    read++;  cTmp = cLoadBuffer[read];
  }
  cLineBuffer[iLineSize]=0;  iLineSize++; read++;
  return read;
}

//------------------------------------------------------------------------------
void surround_space(int position)
{
  insert_space(position+1);
  if (position>0)
  {
    if (cLineBuffer[position-1]!=' ')
    {
      insert_space(position);
    }
  }
}

//------------------------------------------------------------------------------
void parse_null_terminate_comments()
{
  // ZZ> This function removes comments and endline codes, replacing
  //     them with a 0
  int read, write;

  read = 0;
  write = 0;
  while (read < iLoadSize)
  {
    read=load_one_line(read);
    if (iLineSize>2)
    {
      copy_one_line(write);
      write+=iLineSize;
    }
  }
}

//------------------------------------------------------------------------------
int get_indentation()
{
  // ZZ> This function returns the number of starting spaces in a line
  int cnt;
  Uint8 cTmp;

  cnt = 0;
  cTmp=cLineBuffer[cnt];
  while (cTmp==' ')
  {
    cnt++;
    cTmp=cLineBuffer[cnt];
  }
  cnt=cnt>>1;
  if (cnt > 15)
  {
    if (globalparseerr)
    {
      fprintf(globalparseerr, "  %s - %d levels of indentation\n", globalparsename, cnt + 1);
    }
    parseerror = true;
    cnt = 15;
  }
  return cnt;
}

//------------------------------------------------------------------------------
void fix_operators()
{
  // ZZ> This function puts spaces around operators to seperate words
  //     better
  int cnt;
  Uint8 cTmp;

  cnt = 0;
  while (cnt < iLineSize)
  {
    cTmp=cLineBuffer[cnt];
    if (cTmp=='+' || cTmp=='-' || cTmp=='/' || cTmp=='*' || 
        cTmp=='%' || cTmp=='>' || cTmp=='<' || cTmp=='&' || 
        cTmp=='=')
    {
      surround_space(cnt);
      cnt++;
    }
    cnt++;
  }
}

//------------------------------------------------------------------------------
int starts_with_capital_letter()
{
  // ZZ> This function returns true if the line starts with a capital
  int cnt;
  Uint8 cTmp;

  cnt = 0;
  cTmp=cLineBuffer[cnt];
  while (cTmp==' ')
  {
    cnt++;
    cTmp=cLineBuffer[cnt];
  }
  if (cTmp>='A' && cTmp<='Z')
    return true;
  return false;
}

//------------------------------------------------------------------------------
Uint32 get_high_bits()
{
  // ZZ> This function looks at the first word and generates the high
  //     bit codes for it
  Uint32 highbits;

  highbits = get_indentation();
  if (starts_with_capital_letter())
  {
    highbits=highbits|16;
  }
  else
  {
  }
  highbits=highbits<<27;
  return highbits;
}

//------------------------------------------------------------------------------
int tell_code(int read)
{
  // ZZ> This function tells what code is being indexed by read, it
  //     will return the next spot to read from and stick the code number
  //     in iCodeIndex
  int cnt, wordsize, codecorrect;
  Uint8 cTmp;
  IDSZ idsz, test;
  char cWordBuffer[MAXCODENAMESIZE];

  // Check bounds
  iCodeIndex = MAXCODE;
  if (read >= iLineSize)  return read;

  // Skip spaces
  cTmp=cLineBuffer[read];
  while (cTmp==' ' || cTmp==0)
  {
    read++;
    cTmp=cLineBuffer[read];
  }
  if (read >= iLineSize)  return read;

  // Load the word into the other buffer
  wordsize = 0;
  while (cTmp!=' ' && cTmp!=0)
  {
    cWordBuffer[wordsize] = cTmp;  wordsize++;
    read++;
    cTmp=cLineBuffer[read];
  }
  cWordBuffer[wordsize] = 0;

  // Check for numeric constant
  if (cWordBuffer[0]>='0' && cWordBuffer[0]<='9')
  {
    sscanf(&cWordBuffer[0], "%d", &iCodeValueTmp);
    iCodeIndex=-1;
    return read;
  }

  // Check for IDSZ constant
  if (cWordBuffer[0]=='[')
  {
    idsz = IDSZ(&cWordBuffer[1]);
    if (idsz == IDSZ("NONE"))
    {
      idsz = IDSZ::NONE;
    }
    iCodeValueTmp = idsz;
    iCodeIndex=-1;
    return read;
  }

  // Compare the word to all the codes
  codecorrect = false;
  iCodeIndex = 0;
  while (iCodeIndex < iNumCode && !codecorrect)
  {
    codecorrect = false;
    if (cWordBuffer[0] == cCodeName[iCodeIndex][0] && !codecorrect)
    {
      codecorrect = true;
      cnt = 1;
      while (cnt < wordsize)
      {
        if (cWordBuffer[cnt]!=cCodeName[iCodeIndex][cnt])
        {
          codecorrect=false;
          cnt = wordsize;
        }
        cnt++;
      }
      if (cnt < MAXCODENAMESIZE)
      {
        if (cCodeName[iCodeIndex][cnt]!=0)  codecorrect = false;
      }
    }
    iCodeIndex++;
  }
  if (codecorrect)
  {
    iCodeIndex--;
    iCodeValueTmp=iCodeValue[iCodeIndex];
    if (cCodeType[iCodeIndex]=='C')
    {
      // Check for constants
      iCodeIndex = -1;
    }
  }
  else
  {
    // Throw out an error code if we're loggin' 'em
    if (cWordBuffer[0] != '=' || cWordBuffer[1] != 0)
    {
      if (globalparseerr)
      {
        fprintf(globalparseerr, "  %s - %s undefined\n", globalparsename, cWordBuffer);
      }
      parseerror = true;
    }
  }
  return read;
}

//------------------------------------------------------------------------------
void add_code(Uint32 highbits)
{
  Uint32 value;

  if (iCodeIndex==-1)  highbits=highbits|0x80000000;
  if (iCodeIndex != MAXCODE)
  {
    value = highbits|iCodeValueTmp;
    iCompiledAis[iAisIndex]=value;
    iAisIndex++;
  }
}

//------------------------------------------------------------------------------
void parse_line_by_line()
{
  // ZZ> This function removes comments and endline codes, replacing
  //     them with a 0
  int read, line;
  Uint32 highbits;
  int parseposition;
  int operands;

  line = 0;
  read = 0;
  while (line < iNumLine)
  {
    read=load_parsed_line(read);
    fix_operators();
    highbits = get_high_bits();
    parseposition = 0;
    parseposition=tell_code(parseposition);  // VALUE
    add_code(highbits);
    iCodeValueTmp=0;  // SKIP FOR CONTROL CODES
    add_code(0);
    if ((highbits&0x80000000)==0)
    {
      parseposition=tell_code(parseposition);  // EQUALS
      parseposition=tell_code(parseposition);  // VALUE
      add_code(0);
      operands = 1;
      while (parseposition<iLineSize)
      {
        parseposition=tell_code(parseposition);  // OPERATOR
        if (iCodeIndex==-1) iCodeIndex=1;
        else iCodeIndex=0;
        highbits=((iCodeValueTmp&15)<<27)|(iCodeIndex<<0x1F);
        parseposition=tell_code(parseposition);  // VALUE
        add_code(highbits);
        if (iCodeIndex!=MAXCODE)
          operands++;
      }
      iCompiledAis[iAisIndex-operands-1]=operands;  // Number of operands
    }
    line++;
  }
}

//------------------------------------------------------------------------------
Uint32 jump_goto(int index)
{
  // ZZ> This function figures out where to jump to on a fail based on the
  //     starting location and the following code.  The starting location
  //     should always be a function code with indentation
  Uint32 value;
  int targetindent, indent;

  value = iCompiledAis[index];  index+=2;
  targetindent = (value>>27)&15;
  indent = 100;
  while (indent>targetindent)
  {
    value = iCompiledAis[index];
    indent = (value>>27)&15;
    if (indent>targetindent)
    {
      // Was it a function
      if ((value&0x80000000)!=0)
      {
        // Each function needs a jump
        index++;
        index++;
      }
      else
      {
        // Operations cover each operand
        index++;
        value = iCompiledAis[index];
        index++;
        index+=(value&0xFF);
      }
    }
  }
  return index;
}

//------------------------------------------------------------------------------
void parse_jumps(Script_Info & rscr)
{
  // ZZ> This function sets up the fail jumps for the down and dirty code
  int index;
  Uint32 value, iTmp;

  index = rscr.StartPosition;
  value = iCompiledAis[index];
  while (value != 0x80000035) // End Function
  {
    value = iCompiledAis[index];
    // Was it a function
    if ((value&0x80000000)!=0)
    {
      // Each function needs a jump
      iTmp = jump_goto(index);
      index++;
      iCompiledAis[index]=iTmp;
      index++;
    }
    else
    {
      // Operations cover each operand
      index++;
      iTmp = iCompiledAis[index];
      index++;
      index+=(iTmp&0xFF);
    }
  }
}

//------------------------------------------------------------------------------
void log_code(Script_Info & rscr, char* savename)
{
  // ZZ> This function shows the actual code, saving it in a file
  int index;
  Uint32 value;
  FILE* filewrite;

  filewrite = fopen(savename, "w");
  if (filewrite)
  {
    index = rscr.StartPosition;
    value = iCompiledAis[index];
    while (value != 0x80000035) // End Function
    {
      value = iCompiledAis[index];
      fprintf(filewrite, "0x%08x--0x%08x\n", index, value);
      index++;
    }
    fclose(filewrite);
  }
  SDL_Quit();
}

//------------------------------------------------------------------------------
int ai_goto_colon(int read)
{
  // ZZ> This function goes to spot after the next colon
  Uint8 cTmp;

  cTmp = cLoadBuffer[read];
  while (cTmp!=':' && read < iLoadSize)
  {
    read++;  cTmp=cLoadBuffer[read];
  }
  if (read < iLoadSize)  read++;
  return read;
}

//------------------------------------------------------------------------------
void get_code(int read)
{
  // ZZ> This function gets code names and other goodies
  Uint8 cTmp;
  int iTmp;

  sscanf((char*) &cLoadBuffer[read], "%c%d%s", &cTmp, &iTmp, &cCodeName[iNumCode][0]);
  cCodeType[iNumCode]=cTmp;
  iCodeValue[iNumCode]=iTmp;
}

//------------------------------------------------------------------------------
void load_ai_codes(char* loadname)
{
  // ZZ> This function loads all of the function and variable names
  FILE* fileread;
  int read;

  iNumCode = 0;
  fileread = fopen(loadname, "rb");
  if (fileread)
  {
    iLoadSize = (int)fread(&cLoadBuffer[0], 1, MD2MAXLOADSIZE, fileread);
    read = 0;
    read = ai_goto_colon(read);
    while (read!=iLoadSize)
    {
      get_code(read);
      iNumCode++;
      read = ai_goto_colon(read);
    }
    fclose(fileread);
  }
}

//------------------------------------------------------------------------------
Uint32 Script_List::load_ai_script(const char *loadname, Uint32 force_idx)
{
  // ZZ> This function loads a script to memory and
  //     returns false if it fails to do so
  FILE* fileread;

  // get a free script info structure
  Uint32 scr_idx = get_free(force_idx);
  if(INVALID == scr_idx) return INVALID;

  // make life simple
  Script_Info & rscr = ScrList[ AI_REF(scr_idx) ];

  iNumLine = 0;
  globalparsename = loadname;  // For error logging in ParseErr.TXT
  fileread = fopen(loadname, "r");
  if (!fileread)
  {
    return_one(scr_idx);
    return INVALID;
  };

  // Make room for the code
  rscr.StartPosition = iAisIndex;

  // Load into md2 load buffer
  iLoadSize = (int)fread(&cLoadBuffer[0], 1, MD2MAXLOADSIZE, fileread);
  fclose(fileread);

  parse_null_terminate_comments();
  parse_line_by_line();
  parse_jumps(rscr);

  strcpy(rscr.filename, loadname);
  rscr.loaded = true;
  return scr_idx;
}

//------------------------------------------------------------------------------
void Script_List::reset_ai_script()
{
  // ZZ> This function starts ai loading in the right spot
  _setup();

  //load the default AI script
  load_ai_script("basicdat/script.txt", 0);
}

//--------------------------------------------------------------------------------------------
Uint8 run_function(Uint32 value, int character)
{
  // ZZ> This function runs a script function for the AI.
  //     It returns false if the script should jump over the
  //     indented code that follows

  // Mask out the indentation
  Uint16 valuecode = value;

  // Assume that the function will pass, as most do
  Uint8 returncode = true;

  Uint16 sTmp;
  float fTmp;
  int iTmp, tTmp, qTmp;
  int volume;
  IDSZ test;

  Character & chr_data    = ChrList[character];
  Profile   & rprof       = ProfileList[chr_data.model];
  Team      & team_data   = TeamList[chr_data.team];

  Cap       & cap_data    = chr_data.getCap();
  Mad       & mad_data    = chr_data.getMad();

  AI_State  & scr_state   = chr_data.ai;
  Character & target_data = ChrList[scr_state.target];


  // Figure out which function to run
  switch (valuecode)
  {
    case FIFSPAWNED:
      // Proceed only if it's a new character
      returncode = ((scr_state.alert&ALERT_IF_SPAWNED)!=0);
      break;

    case FIFTIMEOUT:
      // Proceed only if time ai.alert is set
      returncode = (scr_state.time==0);
      break;

    case FIFATWAYPOINT:
      // Proceed only if the character reached a waypoint
      returncode = ((scr_state.alert&ALERT_IF_ATWAYPOINT)!=0);
      break;

    case FIFATLASTWAYPOINT:
      // Proceed only if the character reached its last waypoint
      returncode = ((scr_state.alert&ALERT_IF_ATLASTWAYPOINT)!=0);
      break;

    case FIFATTACKED:
      // Proceed only if the character was damaged
      returncode = ((scr_state.alert&ALERT_IF_ATTACKED)!=0);
      break;

    case FIFBUMPED:
      // Proceed only if the character was bumped
      returncode = ((scr_state.alert&ALERT_IF_BUMPED)!=0);
      break;

    case FIFORDERED:
      // Proceed only if the character was GOrder.ed
      returncode = ((scr_state.alert&ALERT_IF_ORDERED)!=0);
      break;

    case FIFCALLEDFORHELP:
      // Proceed only if the character was called for help
      returncode = ((scr_state.alert&ALERT_IF_CALLEDFORHELP)!=0);
      break;

    case FSETCONTENT:
      // Set the content
      scr_state.content = val[TMP_ARGUMENT];
      break;

    case FIFKILLED:
      // Proceed only if the character's been killed
      returncode = ((scr_state.alert&ALERT_IF_KILLED)!=0);
      break;

    case FIFTARGETKILLED:
      // Proceed only if the character's target has just died
      returncode = ((scr_state.alert&ALERT_IF_TARGETKILLED)!=0);
      break;

    case FCLEARWAYPOINTS:
      // Clear out all waypoints
      scr_state.goto_cnt = 0;
      scr_state.goto_idx = 0;
      scr_state.goto_x[0] = chr_data.pos.x;
      scr_state.goto_y[0] = chr_data.pos.y;
      break;

    case FADDWAYPOINT:
      // Add a waypoint to the waypoint list
      scr_state.goto_x[scr_state.goto_idx]=val[TMP_X];
      scr_state.goto_y[scr_state.goto_idx]=val[TMP_Y];
      scr_state.goto_idx++;
      if (scr_state.goto_idx>MAXWAY)  scr_state.goto_idx=MAXWAY-1;
      break;

    case FFINDPATH:
      // This function adds enough waypoints to get from one point to another
      // !!!BAD!!!
      break;

    case FCOMPASS:
      // This function changes tmpx and tmpy in a circlular manner according
      // to tmpturn and tmpdistance
      val[TMP_X] += cos_tab[val[TMP_TURN]>>2] * val[TMP_DISTANCE];
      val[TMP_Y] += sin_tab[val[TMP_TURN]>>2] * val[TMP_DISTANCE];
      break;

    case FGETTARGETARMORPRICE:
      // This function gets the armor cost for the given skin
      sTmp = val[TMP_ARGUMENT] & 3;
      val[TMP_X] = target_data.getCap().skincost[sTmp];
      break;

    case FSETTIME:
      // This function resets the time
      scr_state.time=val[TMP_ARGUMENT];
      break;

    case FGETCONTENT:
      // Get the content
      val[TMP_ARGUMENT] = scr_state.content;
      break;

    case FJOINTARGETTEAM:
      // This function allows the character to leave its own team and join another
      returncode = false;
      if (target_data._on)
      {
        switch_team(character, target_data.team);
        returncode = true;
      }
      break;

    case FSETTARGETTONEARBYENEMY:
      // This function finds a nearby enemy, and proceeds only if there is one
      sTmp = get_nearby_target(character, false, false, true, false, IDSZ::NONE);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FSETTARGETTOTARGETLEFTHAND:
      // This function sets the target to the target's left item
      sTmp = target_data.holding_which[SLOT_LEFT];
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FSETTARGETTOTARGETRIGHTHAND:
      // This function sets the target to the target's right item
      sTmp = target_data.holding_which[SLOT_RIGHT];
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FSETTARGETTOWHOEVERATTACKED:
      // This function sets the target to whoever attacked the character last,
      // failing for damage tiles
      if (VALID_CHR(chr_data.ai.attacklast))
      {
        scr_state.target=chr_data.ai.attacklast;
      }
      else
      {
        returncode = false;
      }
      break;

    case FSETTARGETTOWHOEVERBUMPED:
      // This function sets the target to whoever bumped into the
      // character last.  It never fails
      scr_state.target=chr_data.ai.bumplast;
      break;

    case FSETTARGETTOWHOEVERCALLEDFORHELP:
      // This function sets the target to whoever needs help
      scr_state.target=team_data.sissy;
      break;

    case FSETTARGETTOOLDTARGET:
      // This function reverts to the target with whom the script started
      scr_state.target=val[TMP_OLDTARGET];
      break;

    case FSETTURNMODE_TOVELOCITY:
      // This function sets the turn mode
      chr_data.ai.turn_mode = TURNMODE_VELOCITY;
      break;

    case FSETTURNMODE_TOWATCH:
      // This function sets the turn mode
      chr_data.ai.turn_mode = TURNMODE_WATCH;
      break;

    case FSETTURNMODE_TOSPIN:
      // This function sets the turn mode
      chr_data.ai.turn_mode = TURNMODE_SPIN;
      break;

    case FSETBUMPHEIGHT:
      // This function changes a character's bump height
      chr_data.scale_vert = float(val[TMP_ARGUMENT]) / chr_data.calc_bump_height;
      make_one_character_matrix(chr_data);
      chr_data.calculate_bumpers();
      break;

    case FIFTARGETHASID:
      // This function proceeds if ID matches tmpargument
      sTmp = target_data.model;
      returncode = target_data.getCap().idsz[IDSZ_PARENT] == IDSZ(val[TMP_ARGUMENT]);
      returncode = returncode || (target_data.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]));
      break;

    case FIFTARGETHASITEMID:
      // This function proceeds if the target has a matching item in his/her pack
      returncode = false;

      // Check the pack
      SCAN_CHR_PACK_BEGIN(target_data, sTmp, rinv_item)
      {
        if (rinv_item.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rinv_item.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          returncode = true;
          break;
        }
      } SCAN_CHR_PACK_END;

      // Check left hand
      sTmp = target_data.holding_which[SLOT_LEFT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if ( rschr.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rschr.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          returncode = true;
        }
      }

      // Check right hand
      sTmp = target_data.holding_which[SLOT_RIGHT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rschr.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          returncode = true;
        }
      }
      break;

    case FIFTARGETHOLDINGITEMID:
      // This function proceeds if ID matches tmpargument and returns the latch for the
      // hand in tmpargument
      returncode = false;
      // Check left hand
      sTmp = target_data.holding_which[SLOT_LEFT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rschr.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          val[TMP_ARGUMENT] = LATCHBIT_USE_LEFT;
          returncode = true;
        }
      }

      // Check right hand
      sTmp = target_data.holding_which[SLOT_RIGHT];
      if (VALID_CHR(sTmp) && !returncode)
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rschr.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          val[TMP_ARGUMENT] = LATCHBIT_USE_RIGHT;
          returncode = true;
        }
      }
      break;

    case FIFTARGETHASSKILLID:
      // This function proceeds if ID matches tmpargument
      returncode = target_data.getCap().idsz[IDSZ_SKILL] == IDSZ(val[TMP_ARGUMENT]);
      break;

    case FELSE:
      // This function fails if the last one was more indented
      if ( (((Uint32)val[TMP_LASTINDENT]&0x78000000)) > (value&0x78000000))
        returncode = false;
      break;

    case FRUN:
      reset_character_accel(character);
      break;

    case FWALK:
      reset_character_accel(character);
      chr_data.maxaccel *= .66;
      break;

    case FSNEAK:
      reset_character_accel(character);
      chr_data.maxaccel *= .33;
      break;

    case FDOACTION:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid or if the character is doing
      // something else already
      if (chr_data.act.ready)
      {
        returncode = play_action(chr_data, (ACTION_TYPE)val[TMP_ARGUMENT], false);
      }
      break;

    case FKEEPACTION:
      // This function makes the current animation halt on the last frame
      chr_data.act.keep = true;
      break;

    case FISSUEORDER:
      // This function issues an order to all teammates
      issue_order(character, val[TMP_ARGUMENT]);
      break;

    case FDROPWEAPONS:
      // This funtion drops the character's in hand items/riders
      sTmp = chr_data.holding_which[SLOT_LEFT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        detach_character_from_mount(sTmp, true, true);
        if (chr_data.is_mount)
        {
          rschr.accumulate_vel_z(DISMOUNTZ_VELOCITY - DROPZ_VELOCITY);
          rschr.jumptime  = DELAY_JUMP;
          rschr.jumpready = false;
        }
      }

      sTmp = chr_data.holding_which[SLOT_RIGHT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        detach_character_from_mount(sTmp, true, true);
        if (chr_data.is_mount)
        {
          rschr.accumulate_vel_z(DISMOUNTZ_VELOCITY - DROPZ_VELOCITY);
          rschr.jumptime  = DELAY_JUMP;
          rschr.jumpready = false;
        }
      }
      break;

    case FTARGETDOACTION:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid or if the target is doing
      // something else already
      returncode = false;
      if (target_data.alive && target_data.act.ready)
      {
        returncode = play_action(target_data, (ACTION_TYPE)val[TMP_ARGUMENT], false);
      }
      break;

    case FOPENPASSAGE:
      // This function opens the passage specified by tmpargument, failing if the
      // passage was already open
      returncode = false;
      if( VALID_PASS(val[TMP_ARGUMENT]) )
      {
        returncode = open_passage(PassList[val[TMP_ARGUMENT]]);
      }
      break;

    case FCLOSEPASSAGE:
      // This function closes the passage specified by tmpargument, and proceeds
      // only if the passage is clear of obstructions
      returncode = PassList.close_passage(val[TMP_ARGUMENT]);
      break;

    case FIFPASSAGEOPEN:
      // This function proceeds only if the passage specified by tmpargument
      // is both valid and open
      returncode = false;
      if ( VALID_PASS(val[TMP_ARGUMENT]) )
      {
        returncode = PassList[val[TMP_ARGUMENT]].open;
      }
      break;

    case FGOPOOF:
      // This function flags the character to be removed from the game
      returncode = false;
      if (!chr_data.isplayer)
      {
        returncode = true;
        val[TMP_GOPOOF] = true;
      }
      break;

    case FCOSTTARGETITEMID:
      // This function checks if the target has a matching item, and poofs it
      returncode = false;
      // Check the pack
      iTmp = Character_List::INVALID;
      tTmp = scr_state.target;

      SCAN_CHR_PACK_BEGIN(ChrList[tTmp], sTmp, rinv_item)
      {
        if (rinv_item.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rinv_item.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          returncode = true;
          iTmp = sTmp;
          break;
        }
        else
        {
          tTmp = sTmp;
        }
      } SCAN_CHR_PACK_END;

      // Check left hand
      sTmp = target_data.holding_which[SLOT_LEFT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rschr.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          returncode = true;
          iTmp = target_data.holding_which[SLOT_LEFT];
        }
      }

      // Check right hand
      sTmp = target_data.holding_which[SLOT_RIGHT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rschr.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          returncode = true;
          iTmp = target_data.holding_which[SLOT_RIGHT];
        }
      }

      if (returncode && VALID_CHR(iTmp))
      {
        Character & richr = ChrList[iTmp];
        if (richr.ammo<=1)
        {
          // Poof the item
          if (richr.is_inpack)
          {
            // Remove from the pack
            ChrList[tTmp].nextinpack = richr.nextinpack;
            target_data.numinpack--;
            free_one_character(iTmp);
          }
          else
          {
            // Drop from hand
            detach_character_from_mount(iTmp, true, false);
            free_one_character(iTmp);
          }
        }
        else
        {
          // Cost one ammo
          richr.ammo--;
        }
      }
      break;

    case FDOACTIONOVERRIDE:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid
      returncode = play_action(chr_data, (ACTION_TYPE)val[TMP_ARGUMENT], false);
      break;

    case FIFHEALED:
      // Proceed only if the character was healed
      returncode = ((scr_state.alert&ALERT_IF_HEALED)!=0);
      break;

    case FSENDMESSAGE:
      // This function sends a message to the players
      display_message(rprof.msg_start + val[TMP_ARGUMENT], character);
      break;

    case FCALLFORHELP:
      // This function issues a call for help
      call_for_help(character);
      break;

    case FADDIDSZ:
      // This function adds an idsz to the module's menu.txt file
      add_module_idsz(pickedmodule, val[TMP_ARGUMENT]);
      break;

    case FSETSTATE:
      // This function sets the character's state variable
      scr_state.state = val[TMP_ARGUMENT];
      break;

    case FGETSTATE:
      // This function reads the character's state variable
      val[TMP_ARGUMENT] = scr_state.state;
      break;

    case FIFSTATEIS:
      // This function fails if the character's state is inequal to tmpargument
      returncode = (val[TMP_ARGUMENT] == scr_state.state);
      break;

    case FIFTARGETCANOPENSTUFF:
      // This function fails if the target can't open stuff
      returncode = target_data.canopenstuff;
      break;

    case FIFGRABBED:
      // Proceed only if the character was picked up
      returncode = ((scr_state.alert&ALERT_IF_GRABBED)!=0);
      break;

    case FIFDROPPED:
      // Proceed only if the character was dropped
      returncode = ((scr_state.alert&ALERT_IF_DROPPED)!=0);
      break;

    case FSETTARGETTOWHOEVERISHOLDING:
      // This function sets the target to the character's mount or holder,
      // failing if the character has no mount or holder
      returncode = false;
      if (VALID_CHR(chr_data.held_by))
      {
        scr_state.target = chr_data.held_by;
        returncode = true;
      }
      break;

    case FDAMAGETARGET:
      // This function applies little bit of love to the character's target.
      // The amount is set in tmpargument
      damage_character(scr_state.target, 0, val[TMP_ARGUMENT], 1, chr_data.damagetargettype, chr_data.team, character, DAMFX_BLOC);
      break;

    case FIFXISLESSTHANY:
      // Proceed only if tmpx is less than tmpy
      returncode = (val[TMP_X] < val[TMP_Y]);
      break;

    case FSETWEATHERTIME:
      // Set the weather timer
      GWeather.time_reset = val[TMP_ARGUMENT];
      GWeather.time = val[TMP_ARGUMENT];
      break;

    case FGETBUMPHEIGHT:
      // Get the characters bump height
      val[TMP_ARGUMENT] = chr_data.bump_height;
      break;

    case FIFREAFFIRMED:
      // Proceed only if the character was reaffirmed
      returncode = ((scr_state.alert&ALERT_IF_REAFFIRMED)!=0);
      break;

    case FUNKEEPACTION:
      // This function makes the current animation start again
      chr_data.act.keep = false;
      break;

    case FIFTARGETISONOTHERTEAM:
      // This function proceeds only if the target is on another team
      returncode = (target_data.alive && target_data.team!=chr_data.team);
      break;

    case FIFTARGETISONHATEDTEAM:
      // This function proceeds only if the target is on an enemy team
      returncode = (target_data.alive && team_data.hatesteam[target_data.team] && !target_data.invictus);
      break;

    case FPRESSLATCHBIT_:
      // This function sets the latch buttons
      chr_data.ai.latch.button |= val[TMP_ARGUMENT];
      break;

    case FSETTARGETTOTARGETOFLEADER:
      // This function sets the character's target to the target of its leader,
      // or it fails with no change if the leader is dead
      returncode = false;
      if (team_data.leader!=NOLEADER)
      {
        scr_state.target = ChrList[team_data.leader].ai.target;
        returncode = true;
      }
      break;

    case FIFLEADERKILLED:
      // This function proceeds only if the character's leader has just died
      returncode = ((scr_state.alert&ALERT_IF_LEADERKILLED)!=0);
      break;

    case FBECOMELEADER:
      // This function makes the character the team leader
      team_data.leader=character;
      break;

    case FCHANGETARGETARMOR:
      // This function sets the target's armor type and returns the old type
      // as tmpargument and the new type as tmpx
      iTmp = target_data.skin;
      val[TMP_X] = change_armor(scr_state.target, val[TMP_ARGUMENT]);
      val[TMP_ARGUMENT] = iTmp;  // The character's old armor
      break;

    case FGIVEMONEYTOTARGET:
      // This function transfers money from the character to the target, and sets
      // tmpargument to the amount transferred
      iTmp = chr_data.money;
      tTmp = target_data.money;
      iTmp-=val[TMP_ARGUMENT];
      tTmp+=val[TMP_ARGUMENT];
      if (iTmp < 0) { tTmp+=iTmp;  val[TMP_ARGUMENT]+=iTmp;  iTmp = 0; }
      if (tTmp < 0) { iTmp+=tTmp;  val[TMP_ARGUMENT]+=tTmp;  tTmp = 0; }
      if (iTmp > MAXMONEY) { iTmp = MAXMONEY; }
      if (tTmp > MAXMONEY) { tTmp = MAXMONEY; }
      chr_data.money = iTmp;
      target_data.money = tTmp;
      break;

    case FDROPKEYS:
      drop_keys(character);
      break;

    case FIFLEADERISALIVE:
      // This function fails if there is no team leader
      returncode = (team_data.leader!=NOLEADER);
      break;

    case FIFTARGETISOLDTARGET:
      // This function returns false if the target has changed
      returncode = (scr_state.target==val[TMP_OLDTARGET]);
      break;

    case FSETTARGETTOLEADER:
      // This function fails if there is no team leader
      if (team_data.leader==NOLEADER)
      {
        returncode = false;
      }
      else
      {
        scr_state.target = team_data.leader;
      }
      break;

    case FSPAWNCHARACTER:
      // This function spawns a character, failing if x,y is invalid
      returncode = false;
      // ifthe position is on the mesh?
      if( NULL != GMesh.getFanPos(val[TMP_X], val[TMP_Y]) )
      {
        vec3_t loc_vel;
        loc_vel.x += cos_tab[val[TMP_TURN]>>2]*val[TMP_DISTANCE];
        loc_vel.y += sin_tab[val[TMP_TURN]>>2]*val[TMP_DISTANCE];
        sTmp = spawn_one_character( vec3_t(val[TMP_X], val[TMP_Y], 0), val[TMP_TURN], loc_vel, 0, chr_data.model, chr_data.team, 0);
        if (VALID_CHR(sTmp))
        {
          Character & rschr = ChrList[sTmp];

          // is the character in a wall?
          if ( rschr.inawall(GMesh) )
          {
            free_one_character(sTmp);
          }
          else
          {
            rschr.passage   = chr_data.passage;
            rschr.iskursed  = false;
            scr_state.child = sTmp;
            rschr.ai.owner  = scr_state.owner;
            returncode      = true;
          }
        }
      }
      break;

    case FRESPAWNCHARACTER:
      // This function respawns the character at its starting location
      respawn_character(character);
      break;

    case FCHANGETILE:
      // This function changes the floor image under the character
      (Uint32)(GMesh.getFan(chr_data.onwhichfan)->textureTile) = val[TMP_ARGUMENT];
      break;

    case FIFUSED:
      // This function proceeds only if the character has been used
      returncode = ((scr_state.alert&ALERT_IF_USED)!=0);
      break;

    case FDROPMONEY:
      // This function drops some of a character's money
      drop_money(character, val[TMP_ARGUMENT]);
      break;

    case FSETOLDTARGET:
      // This function sets the old target to the current target
      val[TMP_OLDTARGET] = scr_state.target;
      break;

    case FDETACHFROMHOLDER:
      // This function drops the character, failing only if it was not held
      if (VALID_CHR(chr_data.held_by))
      {
        detach_character_from_mount(character, true, true);
      }
      else
      {
        returncode = false;
      }
      break;

    case FIFTARGETHASVULNERABILITYID:
      // This function proceeds if ID matches tmpargument
      returncode = target_data.getCap().idsz[IDSZ_VULNERABILITY] == IDSZ(val[TMP_ARGUMENT]);
      break;

    case FCLEANUP:
      // This function issues the clean up order to all teammates
      issue_clean(character);
      break;

    case FIFCLEANEDUP:
      // This function proceeds only if the character was told to clean up
      returncode = ((scr_state.alert&ALERT_IF_CLEANEDUP)!=0);
      break;

    case FIFSITTING:
      // This function proceeds if the character is riding another
      returncode = (VALID_CHR(chr_data.held_by));
      break;

    case FIFTARGETISHURT:
      // This function passes only if the target is hurt and alive
      if (!target_data.alive || target_data.life > target_data.lifemax-HURTDAMAGE)
        returncode = false;
      break;

    case FIFTARGETISAPLAYER:
      // This function proceeds only if the target is a player ( may not be local )
      returncode = target_data.isplayer;
      break;

    case FPLAYSOUND:
      // This function plays a sound
      if (chr_data.old.pos.z > PITNOSOUND)
      {
        if( VALID_WAVE_RANGE(val[TMP_ARGUMENT]) )
          play_sound(chr_data.old.pos, rprof.waveindex[val[TMP_ARGUMENT]]);
      }
      break;

    case FSPAWNPARTICLE:
      // This function spawns a particle
      tTmp = character;
      if (VALID_CHR(chr_data.held_by))  tTmp = chr_data.held_by;
      tTmp = spawn_one_particle(chr_data.pos, chr_data.turn_lr, chr_data.vel, 0, chr_data.model, val[TMP_ARGUMENT], character, val[TMP_DISTANCE], chr_data.team, tTmp, 0, Character_List::INVALID);
      if (VALID_PRT(tTmp))
      {
        // Detach the particle
        attach_particle_to_character(tTmp, character, val[TMP_DISTANCE]);
        PrtList[tTmp].attachedtocharacter = Character_List::INVALID;

        // Correct X, Y, Z spacing
        PrtList[tTmp].pos.x+=val[TMP_X];
        PrtList[tTmp].pos.y+=val[TMP_Y];
        PrtList[tTmp].pos.z+=PipList[PrtList[tTmp].pip].zspacingbase;

        // Don't spawn in walls
        if ( PrtList[tTmp].inawall(GMesh) )
        {
          PrtList[tTmp].pos.x = chr_data.pos.x;
          PrtList[tTmp].pos.y = chr_data.pos.y;
        }
      }
      break;

    case FIFTARGETISALIVE:
      // This function proceeds only if the target is alive
      returncode = target_data.alive;
      break;

    case FSTOP:
      chr_data.maxaccel = 0;
      break;

    case FDISAFFIRMCHARACTER:
      disaffirm_attached_particles(character, Pip_List::INVALID);
      break;

    case FREAFFIRMCHARACTER:
      reaffirm_attached_particles(character);
      break;

    case FIFTARGETISSELF:
      // This function proceeds only if the target is the character too
      returncode = (scr_state.target==character);
      break;

    case FIFTARGETISMALE:
      // This function proceeds only if the target is male
      returncode = (chr_data.gender==GENMALE);
      break;

    case FIFTARGETISFEMALE:
      // This function proceeds only if the target is female
      returncode = (chr_data.gender==GENFEMALE);
      break;

    case FSETTARGETTOSELF:
      // This function sets the target to the character
      scr_state.target = character;
      break;

    case FSETTARGETTORIDER:
      // This function sets the target to the character's left/only grip weapon,
      // failing if there is none
      if (INVALID_CHR(chr_data.holding_which[SLOT_LEFT]))
      {
        returncode = false;
      }
      else
      {
        scr_state.target = chr_data.holding_which[SLOT_LEFT];
      }
      break;

    case FGETATTACKTURN:
      // This function sets tmpturn to the direction of the last attack
      val[TMP_TURN] = chr_data.ai.directionlast;
      break;

    case FGETDAMAGETYPE:
      // This function gets the last type of damage
      val[TMP_ARGUMENT] = chr_data.ai.damagetypelast;
      break;

    case FBECOMESPELL:
      // This function turns the spellbook character into a spell based on its
      // content
      chr_data.money = (chr_data.skin)&3;
      change_character(character, scr_state.content, 0, LEAVENONE);
      scr_state.content = 0;  // Reset so it doesn't mess up
      scr_state.state = 0;  // Reset so it doesn't mess up
      changed = true;
      break;

    case FBECOMESPELLBOOK:
      // This function turns the spell into a spellbook, and sets the content
      // accordingly
      scr_state.content = chr_data.model;
      change_character(character, SPELLBOOK, chr_data.money&3, LEAVENONE);
      scr_state.state = 0;  // Reset so it doesn't burn up
      changed = true;
      break;

    case FIFSCOREDAHIT:
      // Proceed only if the character scored a hit
      returncode = ((scr_state.alert&ALERT_IF_SCOREDAHIT)!=0);
      break;

    case FIFDISAFFIRMED:
      // Proceed only if the character was disaffirmed
      returncode = ((scr_state.alert&ALERT_IF_DISAFFIRMED)!=0);
      break;

    case FTRANSLATEORDER:
      // This function gets the order and sets tmpx, tmpy, tmpargument and the
      // target ( if valid )
      sTmp = chr_data.order>>24;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
      }
      val[TMP_X] = ((chr_data.order>>14)&0x03FF)<<6;
      val[TMP_Y] = ((chr_data.order>>4)&0x03FF)<<6;
      val[TMP_ARGUMENT] = chr_data.order&15;
      break;

    case FSETTARGETTOWHOEVERWASHIT:
      // This function sets the target to whoever the character hit last,
      scr_state.target=chr_data.ai.hitlast;
      break;

    case FSETTARGETTOWIDEENEMY:
      // This function finds an enemy, and proceeds only if there is one
      sTmp = get_wide_target(character, false, false, true, false, IDSZ::NONE, false);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FIFCHANGED:
      // Proceed only if the character was polymorphed
      returncode = ((scr_state.alert&ALERT_IF_CHANGED)!=0);
      break;

    case FIFINWATER:
      // Proceed only if the character got wet
      returncode = ((scr_state.alert&ALERT_IF_INWATER)!=0);
      break;

    case FIFBORED:
      // Proceed only if the character is bored
      returncode = ((scr_state.alert&ALERT_IF_BORED)!=0);
      break;

    case FIFTOOMUCHBAGGAGE:
      // Proceed only if the character tried to grab too much
      returncode = ((scr_state.alert&ALERT_IF_TOOMUCHBAGGAGE)!=0);
      break;

    case FIFGROGGED:
      // Proceed only if the character was grogged
      returncode = ((scr_state.alert&ALERT_IF_GROGGED)!=0);
      break;

    case FIFDAZED:
      // Proceed only if the character was dazed
      returncode = ((scr_state.alert&ALERT_IF_DAZED)!=0);
      break;

    case FIFTARGETHASSPECIALID:
      // This function proceeds if ID matches tmpargument
      returncode = target_data.getCap().idsz[IDSZ_SPECIAL] == IDSZ(val[TMP_ARGUMENT]);
      break;

    case FPRESSTARGETLATCHBIT_:
      // This function sets the target's latch buttons
      target_data.ai.latch.button |= val[TMP_ARGUMENT];
      break;

    case FIFINVISIBLE:
      // This function passes if the character is invisible
      returncode = (chr_data.alpha<=INVISIBLE) || (chr_data.light<=INVISIBLE);
      break;

    case FIFARMORIS:
      // This function passes if the character's skin is tmpargument
      tTmp = chr_data.skin;
      returncode = (tTmp==val[TMP_ARGUMENT]);
      break;

    case FGETTARGETGROGTIME:
      // This function returns tmpargument as the grog time, and passes if it is not 0
      val[TMP_ARGUMENT] = chr_data.grogtime;
      returncode = (val[TMP_ARGUMENT]!=0);
      break;

    case FGETTARGETDAZETIME:
      // This function returns tmpargument as the daze time, and passes if it is not 0
      val[TMP_ARGUMENT] = chr_data.dazetime;
      returncode = (val[TMP_ARGUMENT]!=0);
      break;

    case FSETDAMAGETYPE:
      // This function sets the bump damage type
      chr_data.damagetargettype = val[TMP_ARGUMENT] % DAMAGE_COUNT;
      break;

    case FSETWATERLEVEL:
      // This function raises and lowers the module's water
      fTmp = (val[TMP_ARGUMENT]/10.0) - WaterList.level_douse;
      WaterList.level_surface+=fTmp;
      WaterList.level_douse+=fTmp;
      for (iTmp = 0; iTmp < MAXWATERLAYER; iTmp++)
        WaterList[iTmp].z+=fTmp;
      break;

    case FENCHANTTARGET:
      // This function enchants the target
      sTmp = spawn_one_enchant(scr_state.owner, scr_state.target, character, Profile_List::INVALID);
      returncode =  VALID_ENC(sTmp) ;
      break;

    case FENCHANTCHILD:
      // This function can be used with SpawnCharacter to enchant the
      // newly spawned character
      sTmp = spawn_one_enchant(scr_state.owner, scr_state.child, character, Profile_List::INVALID);
      returncode =  VALID_ENC(sTmp) ;
      break;

    case FTELEPORTTARGET:
      // This function teleports the target to the X, Y location, failing if the
      // location is off the map or blocked
      returncode = false;
      if (val[TMP_X] > EDGE && val[TMP_Y] > EDGE && val[TMP_X] < GMesh.width()-EDGE && val[TMP_Y] < GMesh.height()-EDGE)
      {
        // Yeah!  It worked!
        sTmp = scr_state.target;
        if(VALID_CHR(sTmp))
        {
          Character & rschr = ChrList[sTmp];

          detach_character_from_mount(sTmp, true, false);
          rschr.old.pos.x = rschr.pos.x;
          rschr.old.pos.y = rschr.pos.y;
          rschr.pos.x = val[TMP_X];
          rschr.pos.y = val[TMP_Y];
          if (rschr.inawall(GMesh))
          {
            // No it didn't...
            rschr.pos.x = rschr.old.pos.x;
            rschr.pos.y = rschr.old.pos.y;
            returncode = false;
          }
          else
          {
            rschr.old.pos.x = rschr.pos.x;
            rschr.old.pos.y = rschr.pos.y;
            returncode = true;
          }
        };
      }
      break;

    case FGIVEEXPERIENCETOTARGET:
      // This function gives the target some experience, xptype from distance,
      // amount from argument...
      give_experience(scr_state.target, val[TMP_ARGUMENT], val[TMP_DISTANCE]);
      break;

    case FINCREASEAMMO:
      // This function increases the ammo by one
      if (chr_data.ammo < chr_data.ammomax)
      {
        chr_data.ammo++;
      }
      break;

    case FUNKURSETARGET:
      // This function unkurses the target
      target_data.iskursed = false;
      break;

    case FGIVEEXPERIENCETOTARGETTEAM:
      // This function gives experience to everyone on the target's team
      give_team_experience(target_data.team, val[TMP_ARGUMENT], val[TMP_DISTANCE]);
      break;

    case FIFUNARMED:
      // This function proceeds if the character has no item in hand
      returncode = INVALID_CHR(chr_data.holding_which[SLOT_LEFT]) && INVALID_CHR(chr_data.holding_which[SLOT_RIGHT]);
      break;

    case FRESTOCKTARGETAMMOIDALL:
      // This function restocks the ammo of every item the character is holding,
      // if the item matches the ID given ( parent or child type )
      iTmp = 0;  // Amount of ammo given
      sTmp = target_data.holding_which[SLOT_LEFT];
      iTmp += restock_ammo(sTmp, val[TMP_ARGUMENT]);
      sTmp = target_data.holding_which[SLOT_RIGHT];
      iTmp += restock_ammo(sTmp, val[TMP_ARGUMENT]);

      SCAN_CHR_PACK_BEGIN(target_data, sTmp, rinv_item)
      {
        iTmp += restock_ammo(sTmp, val[TMP_ARGUMENT]);
      } SCAN_CHR_PACK_END;

      val[TMP_ARGUMENT] = iTmp;
      returncode = (iTmp != 0);
      break;

    case FRESTOCKTARGETAMMOIDFIRST:
      // This function restocks the ammo of the first item the character is holding,
      // if the item matches the ID given ( parent or child type )
      iTmp = 0;  // Amount of ammo given
      sTmp = target_data.holding_which[SLOT_LEFT];
      iTmp += restock_ammo(sTmp, val[TMP_ARGUMENT]);
      if (iTmp == 0)
      {
        sTmp  = target_data.holding_which[SLOT_RIGHT];
        iTmp += restock_ammo(sTmp, val[TMP_ARGUMENT]);
        if (iTmp == 0)
        {
          SCAN_CHR_PACK_BEGIN(target_data, sTmp, rinv_item)
          {
            if(0 != iTmp) break;
            iTmp += restock_ammo(sTmp, val[TMP_ARGUMENT]);
          } SCAN_CHR_PACK_END;
        }
      }
      val[TMP_ARGUMENT] = iTmp;
      returncode = (iTmp != 0);
      break;

    case FFLASHTARGET:
      // This function flashes the character
      flash_character(scr_state.target, 0xFF);
      break;

    case FSETREDSHIFT:
      // This function alters a character's coloration
      chr_data.redshift = val[TMP_ARGUMENT];
      break;

    case FSETGREENSHIFT:
      // This function alters a character's coloration
      chr_data.grnshift = val[TMP_ARGUMENT];
      break;

    case FSETBLUESHIFT:
      // This function alters a character's coloration
      chr_data.blushift = val[TMP_ARGUMENT];
      break;

    case FSETLIGHT:
      // This function alters a character's transparency
      chr_data.light = val[TMP_ARGUMENT];
      break;

    case FSETALPHA:
      // This function alters a character's transparency
      chr_data.alpha = val[TMP_ARGUMENT];
      break;

    case FIFHITFROMBEHIND:
      // This function proceeds if the character was attacked from behind
      returncode = false;
      if (chr_data.ai.directionlast >= BEHIND-0x2000 && chr_data.ai.directionlast < BEHIND+0x2000)
        returncode = true;
      break;

    case FIFHITFROMFRONT:
      // This function proceeds if the character was attacked from the front
      returncode = false;
      if (chr_data.ai.directionlast >= 49152+0x2000 || chr_data.ai.directionlast < FRONT+0x2000)
        returncode = true;
      break;

    case FIFHITFROMLEFT:
      // This function proceeds if the character was attacked from the left
      returncode = false;
      if (chr_data.ai.directionlast >= LEFT-0x2000 && chr_data.ai.directionlast < LEFT+0x2000)
        returncode = true;
      break;

    case FIFHITFROMRIGHT:
      // This function proceeds if the character was attacked from the right
      returncode = false;
      if (chr_data.ai.directionlast >= RIGHT-0x2000 && chr_data.ai.directionlast < RIGHT+0x2000)
        returncode = true;
      break;

    case FIFTARGETISONSAMETEAM:
      // This function proceeds only if the target is on another team
      returncode = false;
      if (target_data.team==chr_data.team)
        returncode = true;
      break;

    case FKILLTARGET:
      // This function kills the target
      kill_character(scr_state.target, character);
      break;

    case FUNDOENCHANT:
      // This function undoes the last enchant
      returncode = (chr_data.undoenchant!=Enchant_List::INVALID);
      remove_enchant(chr_data.undoenchant);
      break;

    case FGETWATERLEVEL:
      // This function gets the douse level for the water, returning it in tmpargument
      val[TMP_ARGUMENT] = WaterList.level_douse*10;
      break;

    case FCOSTTARGETMANA:
      // This function costs the target some mana
      returncode = cost_mana(scr_state.target, val[TMP_ARGUMENT], character);
      break;

    case FIFTARGETHASANYID:
      // This function proceeds only if one of the target's IDSZ's matches tmpargument
      returncode = 0;
      tTmp = 0;
      while (tTmp < IDSZ_COUNT)
      {
        returncode = returncode || (target_data.getCap().idsz[tTmp] == IDSZ(val[TMP_ARGUMENT]));
        tTmp++;
      }
      break;

    case FSETBUMPSIZE:
      // This function sets the character's bump size
      chr_data.scale_horiz = float(val[TMP_ARGUMENT]) / chr_data.calc_bump_size;
      make_one_character_matrix(chr_data);
      chr_data.calculate_bumpers();
      break;

    case FIFNOTDROPPED:
      // This function passes if a kursed item could not be dropped
      returncode = ((scr_state.alert&ALERT_IF_NOTDROPPED)!=0);
      break;

    case FIFYISLESSTHANX:
      // This function passes only if tmpy is less than tmpx
      returncode = (val[TMP_Y] < val[TMP_X]);
      break;

    case FSETFLYHEIGHT:
      // This function sets a character's fly height
      chr_data.flyheight = val[TMP_ARGUMENT];
      break;

    case FIFBLOCKED:
      // This function passes if the character blocked an attack
      returncode = ((scr_state.alert&ALERT_IF_BLOCKED)!=0);
      break;

    case FIFTARGETISDEFENDING:
      returncode = (target_data.act.which >= ACTION_PA && target_data.act.which <= ACTION_PD);
      break;

    case FIFTARGETISATTACKING:
      returncode = (target_data.act.which >= ACTION_UA && target_data.act.which <= ACTION_FD);
      break;

    case FIFSTATEIS0:
      returncode = (0 == scr_state.state);
      break;

    case FIFSTATEIS1:
      returncode = (1 == scr_state.state);
      break;

    case FIFSTATEIS2:
      returncode = (2 == scr_state.state);
      break;

    case FIFSTATEIS3:
      returncode = (3 == scr_state.state);
      break;

    case FIFSTATEIS4:
      returncode = (4 == scr_state.state);
      break;

    case FIFSTATEIS5:
      returncode = (5 == scr_state.state);
      break;

    case FIFSTATEIS6:
      returncode = (6 == scr_state.state);
      break;

    case FIFSTATEIS7:
      returncode = (7 == scr_state.state);
      break;

    case FIFCONTENTIS:
      returncode = (val[TMP_ARGUMENT] == scr_state.content);
      break;

    case FSETTURNMODE_TOWATCHTARGET:
      // This function sets the turn mode
      chr_data.ai.turn_mode = TURNMODE_WATCHTARGET;
      break;

    case FIFSTATEISNOT:
      returncode = (val[TMP_ARGUMENT] != scr_state.state);
      break;

    case FIFXISEQUALTOY:
      returncode = (val[TMP_X] == val[TMP_Y]);
      break;

    case FDEBUGMESSAGE:
      // This function spits out a debug message
      debug_message("ai.state %d, ai.content %d, target %d", scr_state.state, scr_state.content, scr_state.target);
      debug_message("tmpx %d, tmpy %d", val[TMP_X], val[TMP_Y]);
      debug_message("tmpdistance %d, tmpturn %d", val[TMP_DISTANCE], val[TMP_TURN]);
      debug_message("tmpargument %d, selfturn %d", val[TMP_ARGUMENT], chr_data.turn_lr);
      break;

    case FBLACKTARGET:
      // This function makes the target flash black
      flash_character(scr_state.target, 0);
      break;

    case FSENDMESSAGENEAR:
      // This function sends a message if the camera is in the nearby area.
      iTmp = diff_abs_horiz(chr_data.old.pos, GCamera.track);
      if (iTmp < MSGDISTANCE)
        display_message(rprof.msg_start + val[TMP_ARGUMENT], character);
      break;

    case FIFHITGROUND:
      // This function passes if the character just hit the ground
      returncode = ((scr_state.alert&ALERT_IF_HITGROUND)!=0);
      break;

    case FIFNAMEISKNOWN:
      // This function passes if the character's name is known
      returncode = chr_data.nameknown;
      break;

    case FIFUSAGEISKNOWN:
      // This function passes if the character's usage is known
      returncode = cap_data.usageknown;
      break;

    case FIFHOLDINGITEMID:
      // This function passes if the character is holding an item with the IDSZ given
      // in tmpargument, returning the latch to press to use it
      returncode = false;
      val[TMP_ARGUMENT] = 0;

      // Check left hand
      sTmp = chr_data.holding_which[SLOT_LEFT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rschr.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          val[TMP_ARGUMENT] |= LATCHBIT_USE_LEFT;
          returncode = true;
        }
      }

      // Check right hand
      sTmp = chr_data.holding_which[SLOT_RIGHT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rschr.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT]))
        {
          val[TMP_ARGUMENT] |= LATCHBIT_USE_RIGHT;
          returncode = true;
        }
      }
      break;

    case FIFHOLDINGRANGEDWEAPON:
      // This function passes if the character is holding a ranged weapon, returning
      // the latch to press to use it.  This also checks ammo/ammoknown.
      returncode = false;
      val[TMP_ARGUMENT] = 0;
      // Check left hand
      tTmp = chr_data.holding_which[SLOT_LEFT];
      if (VALID_CHR(tTmp))
      {
        Character & rtchr = ChrList[tTmp];
        if (rtchr.getCap().isranged && (rtchr.ammomax==0 || (rtchr.ammo!=0 && rtchr.ammoknown)))
        {
          val[TMP_ARGUMENT] = LATCHBIT_USE_LEFT;
          returncode = true;
        }
      }

      // Check right hand
      tTmp = chr_data.holding_which[SLOT_RIGHT];
      if (VALID_CHR(tTmp))
      {
        Character & rtchr = ChrList[tTmp];
        if (rtchr.getCap().isranged && (rtchr.ammomax==0 || (rtchr.ammo!=0 && rtchr.ammoknown)))
        {
          if (val[TMP_ARGUMENT] == 0 || (allframe&1))
          {
            val[TMP_ARGUMENT] = LATCHBIT_USE_RIGHT;
            returncode = true;
          }
        }
      }
      break;

    case FIFHOLDINGMELEEWEAPON:
      // This function passes if the character is holding a melee weapon, returning
      // the latch to press to use it
      returncode = false;
      val[TMP_ARGUMENT] = 0;
      // Check left hand
      sTmp = chr_data.holding_which[SLOT_LEFT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (!rschr.getCap().isranged && rschr.getCap().weaponaction!=ACTION_PA)
        {
          val[TMP_ARGUMENT] = LATCHBIT_USE_LEFT;
          returncode = true;
        }
      }

      // Check right hand
      sTmp = chr_data.holding_which[SLOT_RIGHT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (!rschr.getCap().isranged && rschr.getCap().weaponaction!=ACTION_PA)
        {
          if (val[TMP_ARGUMENT] == 0 || (allframe&1))
          {
            val[TMP_ARGUMENT] = LATCHBIT_USE_RIGHT;
            returncode = true;
          }
        }
      }
      break;

    case FIFHOLDINGSHIELD:
      // This function passes if the character is holding a shield, returning the
      // latch to press to use it
      returncode = false;
      val[TMP_ARGUMENT] = 0;
      // Check left hand
      sTmp = chr_data.holding_which[SLOT_LEFT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().weaponaction==ACTION_PA)
        {
          val[TMP_ARGUMENT] = LATCHBIT_USE_LEFT;
          returncode = true;
        }
      }
      // Check right hand
      sTmp = chr_data.holding_which[SLOT_RIGHT];
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.getCap().weaponaction==ACTION_PA)
        {
          val[TMP_ARGUMENT] = LATCHBIT_USE_RIGHT;
          returncode = true;
        }
      }
      break;

    case FIFKURSED:
      // This function passes if the character is kursed
      returncode = chr_data.iskursed;
      break;

    case FIFTARGETISKURSED:
      // This function passes if the target is kursed
      returncode = target_data.iskursed;
      break;

    case FIFTARGETISDRESSEDUP:
      // This function passes if the character's skin is dressy
      iTmp = chr_data.skin;
      iTmp = 1<<iTmp;
      returncode = ((cap_data.skindressy&iTmp)!=0);
      break;

    case FIFOVERWATER:
      // This function passes if the character is on a water tile
      returncode = (GMesh.has_flags(chr_data.onwhichfan, MESHFX_WATER) && WaterList.is_water);
      break;

    case FIFTHROWN:
      // This function passes if the character was thrown
      returncode = ((scr_state.alert&ALERT_IF_THROWN)!=0);
      break;

    case FMAKENAMEKNOWN:
      // This function makes the name of an item/character known.
      chr_data.nameknown = true;
      chr_data.show_icon = true;
      break;

    case FMAKEUSAGEKNOWN:
      // This function makes the usage of an item known...  For XP gains from
      // using an unknown potion or such
      cap_data.usageknown = true;
      break;

    case FSTOPTARGETMOVEMENT:
      // This function makes the target stop moving temporarily
      target_data.accumulate_vel( vec3_t(-target_data.vel.x, -target_data.vel.y, 0) );
      break;

    case FSETXY:
      // This function stores tmpx and tmpy in the storage array
      scr_state.x[val[TMP_ARGUMENT]&STORAND] = val[TMP_X];
      scr_state.y[val[TMP_ARGUMENT]&STORAND] = val[TMP_Y];
      break;

    case FGETXY:
      // This function gets previously stored data, setting tmpx and tmpy
      val[TMP_X] = scr_state.x[val[TMP_ARGUMENT]&STORAND];
      val[TMP_Y] = scr_state.y[val[TMP_ARGUMENT]&STORAND];
      break;

    case FADDXY:
      // This function adds tmpx and tmpy to the storage array
      scr_state.x[val[TMP_ARGUMENT]&STORAND] += val[TMP_X];
      scr_state.y[val[TMP_ARGUMENT]&STORAND] += val[TMP_Y];
      break;

    case FMAKEAMMOKNOWN:
      // This function makes the ammo of an item/character known.
      chr_data.ammoknown=true;
      break;

    case FSPAWNATTACHEDPARTICLE:
      // This function spawns an attached particle
      tTmp = character;
      if (VALID_CHR(chr_data.held_by))  tTmp = chr_data.held_by;
      tTmp = spawn_one_particle(chr_data.pos, chr_data.turn_lr, chr_data.vel, 0, chr_data.model, val[TMP_ARGUMENT], character, val[TMP_DISTANCE], chr_data.team, tTmp, 0, Character_List::INVALID);
      break;

    case FSPAWNEXACTPARTICLE:
      // This function spawns an exactly placed particle
      tTmp = character;
      if (VALID_CHR(chr_data.held_by))  tTmp = chr_data.held_by;
      spawn_one_particle( vec3_t(val[TMP_X], val[TMP_Y], val[TMP_DISTANCE]), chr_data.turn_lr, vec3_t(0,0,0), 0, chr_data.model, val[TMP_ARGUMENT], Character_List::INVALID, 0, chr_data.team, tTmp, 0, Character_List::INVALID);
      break;

    case FACCELERATETARGET:
      // This function changes the target's speeds
      target_data.vel.x+=val[TMP_X];
      target_data.vel.y+=val[TMP_Y];
      break;

    case FIFDISTANCEISMORETHANTURN:
      // This function proceeds tmpdistance is greater than tmpturn
      returncode = (val[TMP_DISTANCE] > (int) val[TMP_TURN]);
      break;

    case FIFCRUSHED:
      // This function proceeds only if the character was crushed
      returncode = ((scr_state.alert&ALERT_IF_CRUSHED)!=0);
      break;

    case FMAKECRUSHVALID:
      // This function makes doors able to close on this object
      chr_data.canbecrushed = true;
      break;

    case FSETTARGETTOLOWESTTARGET:
      // This sets the target to whatever the target is being held by,
      // The lowest in the set.  This function never fails
      while (_VALID_CHR_RANGE(target_data.held_by))
      {
        scr_state.target = target_data.held_by;
      }
      break;

    case FIFNOTPUTAWAY:
      // This function proceeds only if the character couln't be put in the pack
      returncode = ((scr_state.alert&ALERT_IF_NOTPUTAWAY)!=0);
      break;

    case FIFTAKENOUT:
      // This function proceeds only if the character was taken out of the pack
      returncode = ((scr_state.alert&ALERT_IF_TAKENOUT)!=0);
      break;

    case FIFAMMOOUT:
      // This function proceeds only if the character has no ammo
      returncode = (chr_data.ammo==0);
      break;

    case FPLAYSOUNDLOOPED:
      // This function plays a looped sound
      if (moduleactive)
      {
        volume = VOLMAX;
        //You could use this, but right now there's no way to stop the sound later, so it's better not to start it
        ////play_sound_pvf_looped(cap_data.waveindex[val[TMP_ARGUMENT]], PANMID, volume, val[TMP_DISTANCE]);
      }
      break;

    case FSTOPSOUND:
      //TODO: implement this (the scripter doesn't know which channel to stop)
      // This function stops playing a sound
      //stop_sound([val[TMP_ARGUMENT]]);
      break;

    case FHEALSELF:
      // This function heals the character, without setting the ai.alert or modifying
      // the amount
      if (chr_data.alive)
      {
        iTmp = chr_data.life + val[TMP_ARGUMENT];
        if (iTmp > chr_data.lifemax) iTmp = chr_data.lifemax;
        if (iTmp < 1) iTmp = 1;
        chr_data.life = iTmp;
      }
      break;

    case FEQUIP:
      // This function flags the character as being equipped
      chr_data.is_equipped = true;
      break;

    case FIFTARGETHASITEMIDEQUIPPED:
      // This function proceeds if the target has a matching item equipped
      returncode = false;

      SCAN_CHR_PACK_BEGIN(target_data, sTmp, rinv_item)
      {
        if (sTmp != character && rinv_item.is_equipped && (rinv_item.getCap().idsz[IDSZ_PARENT]==IDSZ(val[TMP_ARGUMENT]) || rinv_item.getCap().idsz[IDSZ_TYPE]==IDSZ(val[TMP_ARGUMENT])))
        {
          returncode = true;
          break;
        }
      } SCAN_CHR_PACK_END;

      break;

    case FSETOWNERTOTARGET:
      // This function sets the owner
      scr_state.owner = scr_state.target;
      break;

    case FSETTARGETTOOWNER:
      // This function sets the target to the owner
      scr_state.target = scr_state.owner;
      break;

    case FSETFRAME:
      // This function sets the character's current frame
      sTmp = val[TMP_ARGUMENT] & 3;
      iTmp = val[TMP_ARGUMENT] >> 2;
      returncode = set_frame(character, iTmp, sTmp);
      break;

    case FBREAKPASSAGE:
      // This function makes the tiles fall away ( turns into damage terrain )
      returncode = break_passage(PassList[val[TMP_ARGUMENT]], val[TMP_TURN], val[TMP_DISTANCE], val[TMP_X], val[TMP_Y]);
      break;

    case FSETRELOADTIME:
      // This function makes weapons fire slower
      chr_data.reloadtime = val[TMP_ARGUMENT];
      break;

    case FSETTARGETTOWIDEBLAHID:
      // This function sets the target based on the settings of
      // tmpargument and tmpdistance
      sTmp = get_wide_target(character, ((val[TMP_DISTANCE]>>3)&1),
                             ((val[TMP_DISTANCE]>>2)&1),
                             ((val[TMP_DISTANCE]>>1)&1),
                             ((val[TMP_DISTANCE])&1),
                             val[TMP_ARGUMENT], ((val[TMP_DISTANCE]>>4)&1));
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FPOOFTARGET:
      // This function makes the target go away
      returncode = false;
      if (!target_data.isplayer)
      {
        returncode = true;
        if (scr_state.target == character)
        {
          // Poof self later
          val[TMP_GOPOOF] = true;
        }
        else
        {
          // Poof others now
          if (VALID_CHR(target_data.held_by))
            detach_character_from_mount(scr_state.target, true, false);

          if (VALID_CHR(target_data.holding_which[SLOT_LEFT]))
            detach_character_from_mount(target_data.holding_which[SLOT_LEFT], true, false);

          if (VALID_CHR(target_data.holding_which[SLOT_RIGHT]))
            detach_character_from_mount(target_data.holding_which[SLOT_RIGHT], true, false);

          free_inventory(scr_state.target);
          free_one_character(scr_state.target);
          scr_state.target = character;
        }
      }
      break;

    case FCHILDDOACTIONOVERRIDE:
      // This function starts a new action, if it is valid for the model
      // It will fail if the action is invalid
      returncode = play_action(scr_state.child, (ACTION_TYPE)val[TMP_ARGUMENT], false);
      break;

    case FSPAWNPOOF:
      // This function makes a lovely little poof at the character's location
      spawn_poof(character, chr_data.model);
      break;

    case FSETSPEEDPERCENT:
      reset_character_accel(character);
      chr_data.maxaccel = chr_data.maxaccel * val[TMP_ARGUMENT] / 100.0;
      break;

    case FSETCHILDSTATE:
      // This function sets the child's state
      ChrList[scr_state.child].ai.state = val[TMP_ARGUMENT];
      break;

    case FSPAWNATTACHEDSIZEDPARTICLE:
      // This function spawns an attached particle, then sets its size
      tTmp = character;
      if (VALID_CHR(chr_data.held_by))  tTmp = chr_data.held_by;
      tTmp = spawn_one_particle( chr_data.pos, chr_data.turn_lr, chr_data.vel, 0, chr_data.model, val[TMP_ARGUMENT], character, val[TMP_DISTANCE], chr_data.team, tTmp, 0, Character_List::INVALID);
      if (VALID_PRT(tTmp))
      {
        PrtList[tTmp].size = val[TMP_TURN];
      }
      break;

    case FCHANGEARMOR:
      // This function sets the character's armor type and returns the old type
      // as tmpargument and the new type as tmpx
      val[TMP_X] = val[TMP_ARGUMENT];
      iTmp = chr_data.skin;
      val[TMP_X] = change_armor(character, val[TMP_ARGUMENT]);
      val[TMP_ARGUMENT] = iTmp;  // The character's old armor
      break;

    case FSHOWTIMER:
      // This function turns the timer on, using the value for tmpargument
      timeron = true;
      timervalue = val[TMP_ARGUMENT];
      break;

    case FIFFACINGTARGET:
      // This function proceeds only if the character is facing the target
      sTmp = diff_to_turn(target_data.pos, chr_data.pos) - chr_data.turn_lr;
      returncode = (sTmp > 55535 || sTmp < 10000);
      break;

    case FPLAYSOUNDVOLUME:
      // This function sets the volume of a sound and plays it
      if (moduleactive && val[TMP_DISTANCE] >= 0)
      {
        volume = val[TMP_DISTANCE];
        iTmp = -1;
        if( VALID_WAVE_RANGE(val[TMP_ARGUMENT]) )
          iTmp = play_sound(chr_data.old.pos, rprof.waveindex[val[TMP_ARGUMENT]]);

        if (iTmp != -1) Mix_Volume(iTmp, val[TMP_DISTANCE]);
      }
      break;

    case FSPAWNATTACHEDFACEDPARTICLE:
      // This function spawns an attached particle with facing
      tTmp = character;
      if (VALID_CHR(chr_data.held_by))  tTmp = chr_data.held_by;
      tTmp = spawn_one_particle( chr_data.pos, val[TMP_TURN], chr_data.vel, 0, chr_data.model, val[TMP_ARGUMENT], character, val[TMP_DISTANCE], chr_data.team, tTmp, 0, Character_List::INVALID);
      break;

    case FIFSTATEISODD:
      returncode = (scr_state.state&1);
      break;

    case FSETTARGETTODISTANTENEMY:
      // This function finds an enemy, within a certain distance to the character, and
      // proceeds only if there is one
      sTmp = find_distant_target(character, val[TMP_DISTANCE]);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FTELEPORT:
      // This function teleports the character to the X, Y location, failing if the
      // location is off the map or blocked
      returncode = false;
      if (val[TMP_X] > EDGE && val[TMP_Y] > EDGE && val[TMP_X] < GMesh.width()-EDGE && val[TMP_Y] < GMesh.height()-EDGE)
      {
        // Yeah!  It worked!
        detach_character_from_mount(character, true, false);
        chr_data.old.pos.x = chr_data.pos.x;
        chr_data.old.pos.y = chr_data.pos.y;
        chr_data.pos.x = val[TMP_X];
        chr_data.pos.y = val[TMP_Y];
        if (chr_data.inawall(GMesh))
        {
          // No it didn't...
          chr_data.pos.x = chr_data.old.pos.x;
          chr_data.pos.y = chr_data.old.pos.y;
          returncode = false;
        }
        else
        {
          chr_data.old.pos.x = chr_data.pos.x;
          chr_data.old.pos.y = chr_data.pos.y;
          returncode = true;
        }
      }
      break;

    case FGIVESTRENGTHTOTARGET:
      // Permanently boost the target's strength
      if (target_data.alive)
      {
        iTmp = val[TMP_ARGUMENT];
        getadd(0, target_data.strength, PERFECTSTAT, iTmp);
        target_data.strength+=iTmp;
      }
      break;

    case FGIVEWISDOMTOTARGET:
      // Permanently boost the target's wisdom
      if (target_data.alive)
      {
        iTmp = val[TMP_ARGUMENT];
        getadd(0, target_data.wisdom, PERFECTSTAT, iTmp);
        target_data.wisdom+=iTmp;
      }
      break;

    case FGIVEINTELLIGENCETOTARGET:
      // Permanently boost the target's intelligence
      if (target_data.alive)
      {
        iTmp = val[TMP_ARGUMENT];
        getadd(0, target_data.intelligence, PERFECTSTAT, iTmp);
        target_data.intelligence+=iTmp;
      }
      break;

    case FGIVEDEXTERITYTOTARGET:
      // Permanently boost the target's dexterity
      if (target_data.alive)
      {
        iTmp = val[TMP_ARGUMENT];
        getadd(0, target_data.dexterity, PERFECTSTAT, iTmp);
        target_data.dexterity+=iTmp;
      }
      break;

    case FGIVELIFETOTARGET:
      // Permanently boost the target's life
      if (target_data.alive)
      {
        iTmp = val[TMP_ARGUMENT];
        getadd(LOWSTAT, target_data.lifemax, PERFECTBIG, iTmp);
        target_data.lifemax+=iTmp;
        if (iTmp < 0)
        {
          getadd(1, target_data.life, PERFECTBIG, iTmp);
        }
        target_data.life+=iTmp;
      }
      break;

    case FGIVEMANATOTARGET:
      // Permanently boost the target's mana
      if (target_data.alive)
      {
        iTmp = val[TMP_ARGUMENT];
        getadd(0, target_data.manamax, PERFECTBIG, iTmp);
        target_data.manamax+=iTmp;
        if (iTmp < 0)
        {
          getadd(0, target_data.mana, PERFECTBIG, iTmp);
        }
        target_data.mana+=iTmp;
      }
      break;

    case FSHOWMAP:
      // Show the map...  Fails if map already visible
      if (Blip::mapon)  returncode = false;
      Blip::mapon = true;
      break;

    case FSHOWYOUAREHERE:
      // Show the camera target location
      Blip::youarehereon = true;
      break;

    case FSHOWBLIPXY:
      // Add a blip
      if (Blip::numblip < MAXBLIP)
      {
        if (val[TMP_X] > 0 && val[TMP_X] < GMesh.width() && val[TMP_Y] > 0 && val[TMP_Y] < GMesh.height())
        {
          if (val[TMP_ARGUMENT] < NUMBAR && val[TMP_ARGUMENT] >= 0)
          {
            BlipList[Blip::numblip].x = val[TMP_X]*MAPSIZE/GMesh.width();
            BlipList[Blip::numblip].y = val[TMP_Y]*MAPSIZE/GMesh.height();
            BlipList[Blip::numblip].c = val[TMP_ARGUMENT];
            Blip::numblip++;
          }
        }
      }
      break;

    case FHEALTARGET:
      // Give some life to the target
      if (target_data.alive)
      {
        iTmp = val[TMP_ARGUMENT];
        getadd(1, target_data.life, target_data.lifemax, iTmp);
        target_data.life+=iTmp;
        // Check all enchants to see if they are removed
        iTmp = target_data.firstenchant;
        while (iTmp != Enchant_List::INVALID)
        {
          sTmp = EncList[iTmp].nextenchant;
          if (IDSZ("HEAL") == EncList[iTmp].getEve().removedbyidsz)
          {
            remove_enchant(iTmp);
          }
          iTmp = sTmp;
        }
      }
      break;

    case FPUMPTARGET:
      // Give some mana to the target
      if (target_data.alive)
      {
        iTmp = val[TMP_ARGUMENT];
        getadd(0, target_data.mana, target_data.manamax, iTmp);
        target_data.mana+=iTmp;
      }
      break;

    case FCOSTAMMO:
      // Take away one ammo
      if (chr_data.ammo > 0)
      {
        chr_data.ammo--;
      }
      break;

    case FMAKESIMILARNAMESKNOWN:
      // Make names of matching objects known
      SCAN_CHR_BEGIN(iTmp, rchr_itmp)
      {
        sTmp = true;
        tTmp = 0;
        while (tTmp < IDSZ_COUNT)
        {
          if (cap_data.idsz[tTmp] != rchr_itmp.getCap().idsz[tTmp])
          {
            sTmp = false;
          }
          tTmp++;
        }

        if (sTmp)
        {
          rchr_itmp.nameknown = true;
        }

      } SCAN_CHR_END;
      break;

    case FSPAWNATTACHEDHOLDERPARTICLE:
      // This function spawns an attached particle, attached to the holder
      tTmp = character;
      if (VALID_CHR(chr_data.held_by))  tTmp = chr_data.held_by;
      tTmp = spawn_one_particle(chr_data.pos, chr_data.turn_lr, chr_data.vel, 0, chr_data.model, val[TMP_ARGUMENT], tTmp, val[TMP_DISTANCE], chr_data.team, tTmp, 0, Character_List::INVALID);
      break;

    case FSETTARGETRELOADTIME:
      // This function sets the target's reload time
      target_data.reloadtime = val[TMP_ARGUMENT];
      break;

    case FSETFOGLEVEL:
      // This function raises and lowers the module's fog
      fTmp = (val[TMP_ARGUMENT]/10.0) - GFog.top;
      GFog.top+=fTmp;
      GFog.distance+=fTmp;
      GFog.on = GFog.allowed;
      if (GFog.distance < 1.0)  GFog.on = false;
      break;

    case FGETFOGLEVEL:
      // This function gets the fog level
      val[TMP_ARGUMENT] = GFog.top*10;
      break;

    case FSETFOGTAD:
      // This function changes the fog color
      GFog.red = val[TMP_TURN];
      GFog.grn = val[TMP_ARGUMENT];
      GFog.blu = val[TMP_DISTANCE];
      break;

    case FSETFOGBOTTOMLEVEL:
      // This function sets the module's bottom fog level...
      fTmp = (val[TMP_ARGUMENT]/10.0) - GFog.bottom;
      GFog.bottom+=fTmp;
      GFog.distance-=fTmp;
      GFog.on = GFog.allowed;
      if (GFog.distance < 1.0)  GFog.on = false;
      break;

    case FGETFOGBOTTOMLEVEL:
      // This function gets the fog level
      val[TMP_ARGUMENT] = GFog.bottom*10;
      break;

    case FCORRECTACTIONFORHAND:
      // This function turns ZA into ZA, ZB, ZC, or ZD...
      // tmpargument must be set to one of the A actions beforehand...
      if (VALID_CHR(chr_data.held_by))
      {
        if (chr_data.gripoffset==GRIP_LEFT)
        {
          // A or B
          val[TMP_ARGUMENT] = val[TMP_ARGUMENT] + (rand()&1);
        }
        else
        {
          // C or D
          val[TMP_ARGUMENT] = val[TMP_ARGUMENT] + 2 + (rand()&1);
        }
      }
      break;

    case FIFTARGETISMOUNTED:
      // This function proceeds if the target is riding a mount
      returncode = false;
      if (VALID_CHR(target_data.held_by))
      {
        returncode = ChrList[target_data.held_by].is_mount;
      }
      break;

    case FSPARKLEICON:
      // This function makes a blippie thing go around the icon
      if (val[TMP_ARGUMENT] < NUMBAR && val[TMP_ARGUMENT] > -1)
      {
        chr_data.sparkle = val[TMP_ARGUMENT];
      }
      break;

    case FUNSPARKLEICON:
      // This function stops the blippie thing
      chr_data.sparkle = NOSPARKLE;
      break;

    case FGETTILEXY:
      // This function gets the tile at x,y
      if (val[TMP_X] >= 0 && val[TMP_X] < GMesh.width())
      {
        if (val[TMP_Y] >= 0 && val[TMP_Y] < GMesh.height())
        {
          val[TMP_ARGUMENT] = GMesh.getFanPos(val[TMP_X],val[TMP_Y])->textureTile;
        }
      }
      break;

    case FSETTILEXY:
      // This function changes the tile at x,y
      if (val[TMP_X] >= 0 && val[TMP_X] < GMesh.width())
      {
        if (val[TMP_Y] >= 0 && val[TMP_Y] < GMesh.height())
        {
          (Uint32)(GMesh.getFanPos(val[TMP_X],val[TMP_Y])->textureTile) = val[TMP_ARGUMENT];
        }
      }
      break;

    case FSETSHADOWSIZE:
      // This function changes a character's shadow size
      chr_data.scale_horiz = float(val[TMP_ARGUMENT]) / chr_data.calc_bump_size;
      make_one_character_matrix(chr_data);
      chr_data.calculate_bumpers();
      break;

    case FORDERTARGET:
      // This function orders one specific character...  The target
      // Be careful in using this, always checking IDSZ first
      target_data.order = val[TMP_ARGUMENT];
      target_data.counter = 0;
      target_data.ai.alert|=ALERT_IF_ORDERED;
      break;

    case FSETTARGETTOWHOEVERISINPASSAGE:
      // This function lets passage rectangles be used as event triggers
      sTmp = PassList[val[TMP_ARGUMENT]].who_is_blocking_passage();
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FIFCHARACTERWASABOOK:
      // This function proceeds if the base model is the same as the current
      // model or if the base model is SPELLBOOK
      returncode = (chr_data.basemodel == SPELLBOOK || 
                    chr_data.basemodel == chr_data.model);
      break;

    case FSETENCHANTBOOSTVALUES:
      // This function sets the boost values for the last enchantment
      iTmp = chr_data.undoenchant;
      if (iTmp != Enchant_List::INVALID)
      {
        EncList[iTmp].ownermana  = val[TMP_ARGUMENT];
        EncList[iTmp].ownerlife  = val[TMP_DISTANCE];
        EncList[iTmp].targetmana = val[TMP_X];
        EncList[iTmp].targetlife = val[TMP_Y];
      }
      break;

    case FSPAWNCHARACTERXYZ:
      // This function spawns a character, failing if x,y,z is invalid
      sTmp = spawn_one_character( vec3_t(val[TMP_X], val[TMP_Y], val[TMP_DISTANCE]), val[TMP_TURN], vec3_t(0,0,0), 0, chr_data.model, chr_data.team, 0);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.inawall(GMesh))
        {
          free_one_character(sTmp);
        }
        else
        {
          rschr.iskursed = false;
          scr_state.child = sTmp;
          rschr.passage = chr_data.passage;
          rschr.ai.owner = scr_state.owner;
          returncode = true;
        }
      }
      break;

    case FSPAWNEXACTCHARACTERXYZ:
      // This function spawns a character ( specific model slot ),
      // failing if x,y,z is invalid
      sTmp = spawn_one_character( vec3_t(val[TMP_X], val[TMP_Y], val[TMP_DISTANCE]), val[TMP_TURN], vec3_t(0,0,0), 0, val[TMP_ARGUMENT], chr_data.team, 0);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        if (rschr.inawall(GMesh))
        {
          free_one_character(sTmp);
        }
        else
        {
          rschr.iskursed = false;
          scr_state.child = sTmp;
          rschr.passage = chr_data.passage;
          rschr.ai.owner = scr_state.owner;
          returncode = true;
        }
      }
      break;

    case FCHANGETARGETCLASS:
      // This function changes a character's model ( specific model slot )
      change_character(scr_state.target, val[TMP_ARGUMENT], 0, LEAVEALL);
      break;

    case FPLAYFULLSOUND:
      // This function plays a sound loud for everyone...  Victory music
      if (moduleactive)
      {
        if( VALID_WAVE_RANGE(val[TMP_ARGUMENT]) )
          play_sound(GCamera.track, rprof.waveindex[val[TMP_ARGUMENT]]);
      }
      break;

    case FSPAWNEXACTCHASEPARTICLE:
      // This function spawns an exactly placed particle that chases the target
      tTmp = character;
      if (VALID_CHR(chr_data.held_by))  tTmp = chr_data.held_by;
      tTmp = spawn_one_particle( vec3_t(val[TMP_X], val[TMP_Y], val[TMP_DISTANCE]), chr_data.turn_lr, vec3_t(0,0,0), 0, chr_data.model, val[TMP_ARGUMENT], Character_List::INVALID, 0, chr_data.team, tTmp, 0, Character_List::INVALID);
      if (VALID_PRT(tTmp))
      {
        PrtList[tTmp].target = scr_state.target;
      }
      break;

    case FCREATEORDER:
      // This function packs up an order, using tmpx, tmpy, tmpargument and the
      // target ( if valid ) to create a new tmpargument
      sTmp = scr_state.target<<24;
      sTmp |= ((val[TMP_X]>>6)&0x03FF)<<14;
      sTmp |= ((val[TMP_Y]>>6)&0x03FF)<<4;
      sTmp |= (val[TMP_ARGUMENT]&15);
      val[TMP_ARGUMENT] = sTmp;
      break;

    case FORDERSPECIALID:
      // This function issues an order to all with the given special IDSZ
      issue_special_order(val[TMP_ARGUMENT], val[TMP_DISTANCE]);
      break;

    case FUNKURSETARGETINVENTORY:
      // This function unkurses every item a character is holding
      sTmp = target_data.holding_which[SLOT_LEFT];
      if(VALID_CHR(sTmp)) ChrList[sTmp].iskursed = false;

      sTmp = target_data.holding_which[SLOT_RIGHT];
      if(VALID_CHR(sTmp)) ChrList[sTmp].iskursed = false;

      SCAN_CHR_PACK_BEGIN(target_data, sTmp, rinv_item)
      {
        rinv_item.iskursed = false;
      } SCAN_CHR_PACK_END;

      break;

    case FIFTARGETISSNEAKING:
      // This function proceeds if the target is doing ACTION_DA or ACTION_WA
      returncode = (target_data.act.which == ACTION_DA || target_data.act.which == ACTION_WA);
      break;

    case FDROPITEMS:
      // This function drops all of the character's items
      drop_all_items(character);
      break;

    case FRESPAWNTARGET:
      // This function respawns the target at its current location
      sTmp = scr_state.target;
      if(VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];

        rschr.old.pos = rschr.pos;
        respawn_character(sTmp);
        rschr.pos = rschr.old.pos;
      };
      break;

    case FTARGETDOACTIONSETFRAME:
      // This function starts a new action, if it is valid for the model and
      // sets the starting frame.  It will fail if the action is invalid
      returncode = play_action(target_data, (ACTION_TYPE)val[TMP_ARGUMENT], false);
      break;

    case FIFTARGETCANSEEINVISIBLE:
      // This function proceeds if the target can see invisible
      returncode = target_data.canseeinvisible;
      break;

    case FSETTARGETTONEARESTBLAHID:
      // This function finds the nearest target that meets the
      // requirements
      sTmp = get_nearest_target(character, ((val[TMP_DISTANCE]>>3)&1),
                                           ((val[TMP_DISTANCE]>>2)&1),
                                           ((val[TMP_DISTANCE]>>1)&1),
                                           ((val[TMP_DISTANCE])&1),
                                           val[TMP_ARGUMENT]);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FSETTARGETTONEARESTENEMY:
      // This function finds the nearest target that meets the
      // requirements
      sTmp = get_nearest_target(character, false, false, true, false, IDSZ::NONE);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FSETTARGETTONEARESTFRIEND:
      // This function finds the nearest target that meets the
      // requirements
      sTmp = get_nearest_target(character, false, true, false, false, IDSZ::NONE);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FSETTARGETTONEARESTLIFEFORM:
      // This function finds the nearest target that meets the
      // requirements
      sTmp = get_nearest_target(character, false, true, true, false, IDSZ::NONE);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FFLASHPASSAGE:
      // This function makes the passage light or dark...  For debug...
      if( VALID_PASS(val[TMP_ARGUMENT]) )
      {
        flash_passage( PassList[val[TMP_ARGUMENT]], val[TMP_DISTANCE]);
      }
      break;

    case FFINDTILEINPASSAGE:
      // This function finds the next tile in the passage, tmpx and tmpy are
      // required and set on return
      returncode = PassList[val[TMP_ARGUMENT]].find_tile_in_passage(val[TMP_DISTANCE]);
      break;

    case FIFHELDINLEFTHAND:
      // This function proceeds if the character is in the left hand of another
      // character
      returncode = false;
      sTmp = chr_data.held_by;
      if (VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];
        returncode = (rschr.holding_which[SLOT_LEFT] == character);
      }
      break;

    case FNOTANITEM:
      // This function makes the character a non-item character
      chr_data.is_item = false;
      break;

    case FSETCHILDAMMO:
      // This function sets the child's ammo
      ChrList[scr_state.child].ammo = val[TMP_ARGUMENT];
      break;

    case FIFHITVULNERABLE:
      // This function proceeds if the character was hit by a weapon with the
      // correct vulnerability IDSZ...  [SILV] for Werewolves...
      returncode = ((scr_state.alert&ALERT_IF_HITVULNERABLE)!=0);
      break;

    case FIFTARGETISFLYING:
      // This function proceeds if the character target is flying
      returncode = (target_data.flyheight > 0);
      break;

    case FIDENTIFYTARGET:
      // This function reveals the target's name, ammo, and usage
      // Proceeds if the target was unknown
      returncode = false;
      sTmp = scr_state.target;
      if(VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];

        if (rschr.ammomax != 0)  rschr.ammoknown=true;
        if ( 0==strcmp(rschr.name, "Blah") )
        {
          returncode = !rschr.nameknown;
          rschr.nameknown=true;
        }
        rschr.getCap().usageknown = true;
      }
      break;

    case FBEATMODULE:
      // This function displays the Module Ended message
      beatmodule = true;
      break;

    case FENDMODULE:
      // This function presses the Escape key
      GKeyb.press(SDLK_ESCAPE);
      break;

    case FDISABLEEXPORT:
      // This function turns export off
      exportvalid = false;
      break;

    case FENABLEEXPORT:
      // This function turns export on
      exportvalid = true;
      break;

    case FGETTARGETSTATE:
      // This function sets tmpargument to the state of the target
      val[TMP_ARGUMENT] = target_data.ai.state;
      break;

    case FSETSPEECH:
      // This function sets all of the RTS speech registers to tmpargument
      sTmp = 0;
      while (sTmp < SPEECH_COUNT)
      {
        chr_data.wavespeech[sTmp] = val[TMP_ARGUMENT];
        sTmp++;
      }
      break;

    case FSETMOVESPEECH:
      // This function sets the RTS move speech register to tmpargument
      chr_data.wavespeech[SPEECH_MOVE] = val[TMP_ARGUMENT];
      break;

    case FSETSECONDMOVESPEECH:
      // This function sets the RTS movealt speech register to tmpargument
      chr_data.wavespeech[SPEECH_MOVEALT] = val[TMP_ARGUMENT];
      break;

    case FSETATTACKSPEECH:
      // This function sets the RTS attack speech register to tmpargument
      chr_data.wavespeech[SPEECH_ATTACK] = val[TMP_ARGUMENT];
      break;

    case FSETASSISTSPEECH:
      // This function sets the RTS assist speech register to tmpargument
      chr_data.wavespeech[SPEECH_ASSIST] = val[TMP_ARGUMENT];
      break;

    case FSETTERRAINSPEECH:
      // This function sets the RTS terrain speech register to tmpargument
      chr_data.wavespeech[SPEECH_TERRAIN] = val[TMP_ARGUMENT];
      break;

    case FSETSELECTSPEECH:
      // This function sets the RTS select speech register to tmpargument
      chr_data.wavespeech[SPEECH_SELECT] = val[TMP_ARGUMENT];
      break;

    case FCLEARENDMESSAGE:
      // This function empties the end-module text buffer
      endtext[0] = 0;
      endtextwrite = 0;
      break;

    case FADDENDMESSAGE:
      // This function appends a message to the end-module text buffer
      append_end_text(rprof.msg_start + val[TMP_ARGUMENT], character);
      break;

    case FPLAYMUSIC:
      // This function begins playing a new track of music
      if (musicvalid && (songplaying != val[TMP_ARGUMENT]))
      {
        play_music(val[TMP_ARGUMENT], val[TMP_DISTANCE], -1);
      }
      break;

    case FSETMUSICPASSAGE:
      // This function makes the given passage play music if a player enters it
      /* TODO: implement and port to SDL_Mixer
      if(val[TMP_ARGUMENT] < PassList.count && val[TMP_ARGUMENT] >= 0)
         {
             if(val[TMP_TURN] < IGTRACKS && val[TMP_TURN] >= 0)
             {
                 PassList[val[TMP_ARGUMENT]].track_type = val[TMP_TURN];
                 PassList[val[TMP_ARGUMENT]].track_count = val[TMP_DISTANCE];
             }
         }*/
      break;

    case FMAKECRUSHINVALID:
      // This function makes doors unable to close on this object
      chr_data.canbecrushed = false;
      break;

    case FSTOPMUSIC:
      // This function stops the interactive music
      stop_music();
      break;

    case FFLASHVARIABLE:
      // This function makes the character flash according to tmpargument
      flash_character(character, val[TMP_ARGUMENT]);
      break;

    case FACCELERATEUP:
      // This function changes the character's up down velocity
      chr_data.vel.z+=val[TMP_ARGUMENT]/100.0;
      break;

    case FFLASHVARIABLEHEIGHT:
      // This function makes the character flash, feet one color, head another...
      flash_character_height(character, val[TMP_TURN], val[TMP_X],
                             val[TMP_DISTANCE], val[TMP_Y]);
      break;

    case FSETDAMAGETIME:
      // This function makes the character invincible for a little while
      chr_data.damagetime = val[TMP_ARGUMENT];
      break;

    case FIFSTATEIS8:
      returncode = (8 == scr_state.state);
      break;

    case FIFSTATEIS9:
      returncode = (9 == scr_state.state);
      break;

    case FIFSTATEIS10:
      returncode = (10 == scr_state.state);
      break;

    case FIFSTATEIS11:
      returncode = (11 == scr_state.state);
      break;

    case FIFSTATEIS12:
      returncode = (12 == scr_state.state);
      break;

    case FIFSTATEIS13:
      returncode = (13 == scr_state.state);
      break;

    case FIFSTATEIS14:
      returncode = (14 == scr_state.state);
      break;

    case FIFSTATEIS15:
      returncode = (15 == scr_state.state);
      break;

    case FIFTARGETISAMOUNT:
      returncode = target_data.is_mount;
      break;

    case FIFTARGETISAPLATFORM:
      returncode = target_data.is_platform;
      break;

    case FADDSTAT:
      if (!chr_data.staton) add_stat(character);
      break;

    case FDISENCHANTTARGET:
      returncode = (target_data.firstenchant != Enchant_List::INVALID);
      disenchant_character(scr_state.target);
      break;

    case FDISENCHANTALL:
      iTmp = 0;
      while (iTmp < Enchant_List::SIZE)
      {
        remove_enchant(iTmp);
        iTmp++;
      }
      break;

    case FSETVOLUMENEARESTTEAMMATE:
      /*PORT
                  if(moduleactive && val[TMP_DISTANCE] >= 0)
                  {
                      // Find the closest teammate
                      iTmp = 10000;
                      SCAN_CHR_BEGIN(sTmp, rchr_stmp)
                      {
                          if(rchr_stmp.alive && rchr_stmp.team == chr_data.team)
                          {
                              distance = diff_abs_horiz(GCamera.track, rchr_stmp.old.pos);
                              if(distance < iTmp)  iTmp = distance;
                          }
                      } SCAN_CHR_END;

                      distance=iTmp+val[TMP_DISTANCE];
                      volume = -distance;
                      volume = volume<<VOLSHIFT;
                      if(volume < VOLMIN) volume = VOLMIN;
                      iTmp = cap_data.waveindex[val[TMP_ARGUMENT]];
                      if(iTmp < numsound && iTmp >= 0 && soundon)
                      {
                          lpDSBuffer[iTmp]->SetVolume(volume);
                      }
                  }
      */
      break;

    case FADDSHOPPASSAGE:
      // This function defines a shop area
      ShopList.add_shop_passage(character, val[TMP_ARGUMENT]);
      break;

    case FTARGETPAYFORARMOR:
      // This function costs the target some money, or fails if 'e doesn't have
      // enough...
      // tmpx is amount needed
      // tmpy is cost of new skin
      sTmp = scr_state.target;   // The target
      if(VALID_CHR(sTmp))
      {
        Character & rschr = ChrList[sTmp];

        iTmp = rschr.getCap().skincost[val[TMP_ARGUMENT]&3];
        val[TMP_Y] = iTmp;                // Cost of new skin
        iTmp -= rschr.getCap().skincost[(rschr.skin)&3];  // Refund
        if (iTmp > rschr.money)
        {
          // Not enough...
          val[TMP_X] = iTmp - rschr.money;  // Amount needed
          returncode = false;
        }
        else
        {
          // Pay for it...  Cost may be negative after refund...
          rschr.money-=iTmp;
          if (rschr.money > MAXMONEY)  rschr.money = MAXMONEY;
          val[TMP_X] = 0;
          returncode = true;
        }
      }
      break;

    case FJOINEVILTEAM:
      // This function adds the character to the evil team...
      switch_team(character, TEAM_EVIL);
      break;

    case FJOINNULLTEAM:
      // This function adds the character to the null team...
      switch_team(character, TEAM_NEUTRAL);
      break;

    case FJOINGOODTEAM:
      // This function adds the character to the good team...
      switch_team(character, TEAM_GOOD);
      break;

    case FPITSKILL:
      // This function activates pit deaths...
      pitskill=true;
      break;

    case FSETTARGETTOPASSAGEID:
      // This function finds a character who is both in the passage and who has
      // an item with the given IDSZ
      sTmp = PassList[val[TMP_ARGUMENT]].who_is_blocking_passage_ID(val[TMP_DISTANCE]);
      returncode = false;
      if (VALID_CHR(sTmp))
      {
        scr_state.target = sTmp;
        returncode = true;
      }
      break;

    case FMAKENAMEUNKNOWN:
      // This function makes the name of an item/character unknown.
      chr_data.nameknown=false;
      break;

    case FSPAWNEXACTPARTICLEENDSPAWN:
      // This function spawns a particle that spawns a character...
      tTmp = character;
      if (VALID_CHR(chr_data.held_by))  tTmp = chr_data.held_by;
      tTmp = spawn_one_particle( vec3_t(val[TMP_X], val[TMP_Y], val[TMP_DISTANCE]), chr_data.turn_lr, vec3_t(0,0,0), 0, chr_data.model, val[TMP_ARGUMENT], Character_List::INVALID, 0, chr_data.team, tTmp, 0, Character_List::INVALID);
      if (VALID_PRT(tTmp))
      {
        PrtList[tTmp].spawncharacterstate = val[TMP_TURN];
      }
      break;

    case FSPAWNPOOFSPEEDSPACINGDAMAGE:
      {
        // This function makes a lovely little poof at the character's location,
        // adjusting the xy speed and spacing and the base damage first
        // Temporarily adjust the values for the particle type
        sTmp = chr_data.model;
        CAP_REF cap_ref = ProfileList[sTmp].cap_ref;
        PIP_REF pip_ref = ProfileList[sTmp].prtpip[CapList[cap_ref].gopoofprttype];

        //save some values
        iTmp = PipList[pip_ref].xvel_ybase;
        tTmp = PipList[pip_ref].xyspacingbase;
        qTmp = PipList[pip_ref].damagebase;

        //load up new ones
        PipList[pip_ref].xvel_ybase = val[TMP_X];
        PipList[pip_ref].xyspacingbase = val[TMP_Y];
        PipList[pip_ref].damagebase = val[TMP_ARGUMENT];

        //do the poof
        spawn_poof(character, chr_data.model);

        // Restore the saved values
        PipList[pip_ref].xvel_ybase = iTmp;
        PipList[pip_ref].xyspacingbase = tTmp;
        PipList[pip_ref].damagebase = qTmp;
      }
      break;

    case FGIVEEXPERIENCETOGOODTEAM:
      // This function gives experience to everyone on the G Team
      give_team_experience(TEAM_GOOD, val[TMP_ARGUMENT], val[TMP_DISTANCE]);
      break;

    case FDONOTHING:
      //This function does nothing (For use with combination with Else function or debugging)
      break;

    case FGROGTARGET:
      //This function grogs the target for a duration equal to tmpargument
      target_data.grogtime+=val[TMP_ARGUMENT];
      break;

    case FDAZETARGET:
      //This function dazes the target for a duration equal to tmpargument
      target_data.dazetime+=val[TMP_ARGUMENT];
      break;
  }

  return returncode;
}

//--------------------------------------------------------------------------------------------
void set_operand(Uint8 variable)
{
  // ZZ> This function sets one of the tmp* values for scripted AI
  switch (variable)
  {
    case VARTMPX:
      val[TMP_X]=val[TMP_OPERATIONSUM];
      break;

    case VARTMPY:
      val[TMP_Y]=val[TMP_OPERATIONSUM];
      break;

    case VARTMPDISTANCE:
      val[TMP_DISTANCE]=val[TMP_OPERATIONSUM];
      break;

    case VARTMPTURN:
      val[TMP_TURN]=val[TMP_OPERATIONSUM];
      break;

    case VARTMPARGUMENT:
      val[TMP_ARGUMENT] = val[TMP_OPERATIONSUM];
      break;
  }
}

//--------------------------------------------------------------------------------------------
void run_operand(Uint32 value, int character)
{
  // ZZ> This function does the scripted arithmetic in operator,operand pairs
  Uint8 opcode;
  Uint8 variable;
  int iTmp = 1;

  if(INVALID_CHR(character)) return;
  Character & rchr = ChrList[character];

  // Get the operation code
  opcode = (value>>27);
  if (opcode&16)
  {
    // Get the working value from a constant, constants are all but high 5 bits
    iTmp = value&0x07ffffff;
  }
  else
  {
    // Get the working value from a register
    variable = value;
    iTmp = 1;
    switch (variable)
    {
      case VARTMPX:
        iTmp = val[TMP_X];
        break;

      case VARTMPY:
        iTmp = val[TMP_Y];
        break;

      case VARTMPDISTANCE:
        iTmp = val[TMP_DISTANCE];
        break;

      case VARTMPTURN:
        iTmp = val[TMP_TURN];
        break;

      case VARTMPARGUMENT:
        iTmp = val[TMP_ARGUMENT];
        break;

      case VARRAND:
        iTmp = RANDIE;
        break;

      case VARSELFX:
        iTmp = rchr.pos.x;
        break;

      case VARSELFY:
        iTmp = rchr.pos.y;
        break;

      case VARSELFTURN:
        iTmp = rchr.turn_lr;
        break;

      case VARSELFCOUNTER:
        iTmp = rchr.counter;
        break;

      case VARSELFORDER:
        iTmp = rchr.order;
        break;

      case VARSELFMORALE:
        iTmp = TeamList[rchr.baseteam].morale;
        break;

      case VARSELFLIFE:
        iTmp = rchr.life;
        break;

      case VARTARGETX:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].pos.x;
        break;

      case VARTARGETY:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].pos.y;
        break;

      case VARTARGETDISTANCE:
        if(VALID_CHR(rchr.ai.target)) 
          iTmp = diff_abs_horiz(ChrList[rchr.ai.target].pos, rchr.pos);
        break;

      case VARTARGETTURN:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].turn_lr;
        break;

      case VARLEADERX:
        iTmp = rchr.pos.x;
        if (TeamList[rchr.team].leader!=NOLEADER)
          iTmp = ChrList[TeamList[rchr.team].leader].pos.x;
        break;

      case VARLEADERY:
        iTmp = rchr.pos.y;
        if (TeamList[rchr.team].leader!=NOLEADER)
          iTmp = ChrList[TeamList[rchr.team].leader].pos.y;
        break;

      case VARLEADERDISTANCE:
        iTmp = 10000;
        if (TeamList[rchr.team].leader!=NOLEADER)
          iTmp = diff_abs_horiz(ChrList[TeamList[rchr.team].leader].pos, rchr.pos);
        break;

      case VARLEADERTURN:
        iTmp = rchr.turn_lr;
        if (TeamList[rchr.team].leader!=NOLEADER)
          iTmp = ChrList[TeamList[rchr.team].leader].turn_lr;
        break;

      case VARGOTOX:
        iTmp = rchr.ai.goto_x[rchr.ai.goto_cnt];
        break;

      case VARGOTOY:
        iTmp = rchr.ai.goto_y[rchr.ai.goto_cnt];
        break;

      case VARGOTODISTANCE:
        iTmp = ABS((int)(rchr.ai.goto_x[rchr.ai.goto_cnt]-rchr.pos.x))+
               ABS((int)(rchr.ai.goto_y[rchr.ai.goto_cnt]-rchr.pos.y));
        break;

      case VARTARGETTURNTO:
        if(VALID_CHR(rchr.ai.target)) 
        {
          iTmp  = diff_to_turn(ChrList[rchr.ai.target].pos, rchr.pos);
          iTmp  = iTmp&0xFFFF;
        }
        break;

      case VARPASSAGE:
        iTmp = rchr.passage;
        break;

      case VARWEIGHT:
        iTmp = rchr.holding_weight;
        break;

      case VARSELFALTITUDE:
        iTmp = rchr.pos.z-rchr.level;
        break;

      case VARSELFID:
        iTmp = rchr.getCap().idsz[IDSZ_TYPE];
        break;

      case VARSELFHATEID:
        iTmp = rchr.getCap().idsz[IDSZ_HATE];
        break;

      case VARSELFMANA:
        iTmp = rchr.mana;
        if (rchr.canchannel)  iTmp += rchr.life;
        break;

      case VARTARGETSTR:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].strength;
        break;

      case VARTARGETWIS:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].wisdom;
        break;

      case VARTARGETINT:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].intelligence;
        break;

      case VARTARGETDEX:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].dexterity;
        break;

      case VARTARGETLIFE:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].life;
        break;

      case VARTARGETMANA:
        if(VALID_CHR(rchr.ai.target))
        {
          iTmp = ChrList[rchr.ai.target].mana;
          if (ChrList[rchr.ai.target].canchannel)  iTmp += ChrList[rchr.ai.target].life;
        }
        break;

      case VARTARGETLEVEL:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].experiencelevel;
        break;

      case VARTARGETSPEEDX:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].vel.x;
        break;

      case VARTARGETSPEEDY:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].vel.y;
        break;

      case VARTARGETSPEEDZ:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].vel.z;
        break;

      case VARSELFSPAWNX:
        iTmp = rchr.stt.x;
        break;

      case VARSELFSPAWNY:
        iTmp = rchr.stt.y;
        break;

      case VARSELFSTATE:
        iTmp = rchr.ai.state;
        break;

      case VARSELFSTR:
        iTmp = rchr.strength;
        break;

      case VARSELFWIS:
        iTmp = rchr.wisdom;
        break;

      case VARSELFINT:
        iTmp = rchr.intelligence;
        break;

      case VARSELFDEX:
        iTmp = rchr.dexterity;
        break;

      case VARSELFMANAFLOW:
        iTmp = rchr.manaflow;
        break;

      case VARTARGETMANAFLOW:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].manaflow;
        break;

      case VARSELFATTACHED:
        iTmp = number_of_attached_particles(character);
        break;

      case VARSWINGTURN:
        iTmp = GCamera.swing<<2;
        break;

      case VARXYDISTANCE:
        iTmp = sqrtf(val[TMP_X]*val[TMP_X] + val[TMP_Y]*val[TMP_Y]);
        break;

      case VARSELFZ:
        iTmp = rchr.pos.z;
        break;

      case VARTARGETALTITUDE:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].pos.z-ChrList[rchr.ai.target].level;
        break;

      case VARTARGETZ:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].pos.z;
        break;

      case VARSELFINDEX:
        iTmp = character;
        break;

      case VAROWNERX:
        if(VALID_CHR(rchr.ai.owner)) iTmp = ChrList[rchr.ai.owner].pos.x;
        break;

      case VAROWNERY:
        if(VALID_CHR(rchr.ai.owner)) iTmp = ChrList[rchr.ai.owner].pos.y;
        break;

      case VAROWNERTURN:
        if(VALID_CHR(rchr.ai.owner)) iTmp = ChrList[rchr.ai.owner].turn_lr;
        break;

      case VAROWNERDISTANCE:
        if(VALID_CHR(rchr.ai.owner)) 
          iTmp = diff_abs_horiz(ChrList[rchr.ai.owner].pos, rchr.pos);
        break;

      case VAROWNERTURNTO:
        if(VALID_CHR(rchr.ai.target)) 
        {
          iTmp = diff_to_turn(ChrList[rchr.ai.owner].pos, rchr.pos);
          iTmp = iTmp&0xFFFF;
        }
        break;

      case VARXYTURNTO:
        iTmp = diff_to_turn(vec3_t(val[TMP_Y], val[TMP_X],0), rchr.pos);
        iTmp = iTmp&0xFFFF;
        break;

      case VARSELFMONEY:
        iTmp = rchr.money;
        break;

      case VARSELFACCEL:
        iTmp = (rchr.maxaccel*100.0);
        break;

      case VARTARGETEXP:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].experience;
        break;

      case VARSELFAMMO:
        iTmp = rchr.ammo;
        break;

      case VARTARGETAMMO:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].ammo;
        break;

      case VARTARGETMONEY:
        if(VALID_CHR(rchr.ai.target)) iTmp = ChrList[rchr.ai.target].money;
        break;

      case VARTARGETTURNAWAY:
        if(VALID_CHR(rchr.ai.target)) 
        {
          iTmp = 0x8000 + diff_to_turn(ChrList[rchr.ai.target].pos, rchr.pos);
          iTmp &= 0xFFFF;
        }
        break;

      case VARSELFLEVEL:
        iTmp = rchr.experiencelevel;
        break;

    }
  }

  // Now do the math
  switch (opcode&15)
  {
    case OPADD:
      val[TMP_OPERATIONSUM]+=iTmp;
      break;

    case OPSUB:
      val[TMP_OPERATIONSUM]-=iTmp;
      break;

    case OPAND:
      val[TMP_OPERATIONSUM] &= iTmp;
      break;

    case OPSHR:
      val[TMP_OPERATIONSUM]=val[TMP_OPERATIONSUM]>>iTmp;
      break;

    case OPSHL:
      val[TMP_OPERATIONSUM]=val[TMP_OPERATIONSUM]<<iTmp;
      break;

    case OPMUL:
      val[TMP_OPERATIONSUM]=val[TMP_OPERATIONSUM]*iTmp;
      break;

    case OPDIV:
      if (iTmp != 0)
      {
        val[TMP_OPERATIONSUM]=val[TMP_OPERATIONSUM]/iTmp;
      }
      break;

    case OPMOD:
      if (iTmp != 0)
      {
        val[TMP_OPERATIONSUM]=val[TMP_OPERATIONSUM]%iTmp;
      }
      break;

  }
}

//--------------------------------------------------------------------------------------------
void let_character_think(Uint32 character)
{
  // ZZ> This function lets one character do AI stuff
  Uint32 index;
  Uint32 value;
  Uint32 iTmp;
  Uint8 functionreturn;
  int operands;

  if( INVALID_CHR(character) ) return;
  Character & chr = ChrList[character];

  // Make life easier
  val[TMP_OLDTARGET] = chr.ai.target;

  if(Script_List::INVALID == chr.scr_ref.index)
    return;

  Script_Info & rscr = ScrList[ chr.scr_ref ];

  // Figure out alerts that weren't already set
  set_alerts(character);
  changed = false;

  // Clear the button latches
  if (!chr.isplayer)
  {
    chr.ai.latch.button = 0;
  }

  // Reset the target if it can't be seen
  if (!chr.canseeinvisible && chr.alive)
  {
    if (ChrList[chr.ai.target].alpha<=INVISIBLE || ChrList[chr.ai.target].light<=INVISIBLE)
    {
      chr.ai.target = character;
    }
  }

  // Run the AI Script
  index = rscr.StartPosition;
  val[TMP_GOPOOF] = false;

  value = iCompiledAis[index];
  while ((value&0x87ffffff) != 0x80000035) // End Function
  {
    value = iCompiledAis[index];
    // Was it a function
    if ((value&0x80000000)!=0)
    {
      // Run the function
      functionreturn = run_function(value, character);
      // Get the jump code
      index++;
      iTmp=iCompiledAis[index];
      if (functionreturn)
      {
        // Proceed to the next function
        index++;
      }
      else
      {
        // Jump to where the jump code says to go
        index=iTmp;
      }
    }
    else
    {
      // Get the number of operands
      index++;
      operands = iCompiledAis[index];
      // Now run the operation
      val[TMP_OPERATIONSUM]=0;
      index++;
      while (operands > 0)
      {
        iTmp = iCompiledAis[index];
        run_operand(iTmp,character);  // This sets val[TMP_OPERATIONSUM]
        operands--;
        index++;
      }
      // Save the results in the register that called the arithmetic
      set_operand(value);
    }

    // This is used by the Else function
    val[TMP_LASTINDENT] = value;
  }

  // Set latches
  if (!chr.isplayer && chr.scr_ref.index!=0 ) /* 0 is the default script */
  {
    if (chr.is_mount && VALID_CHR(chr.holding_which[SLOT_LEFT]))
    {
      // Mount
      chr.ai.latch.x = ChrList[chr.holding_which[SLOT_LEFT]].ai.latch.x;
      chr.ai.latch.y = ChrList[chr.holding_which[SLOT_LEFT]].ai.latch.y;
    }
    else if(!chr.is_equipped && INVALID_CHR(chr.held_by))
    {
      // Normal AI
      chr.ai.latch.x = (chr.ai.goto_x[chr.ai.goto_cnt]-chr.pos.x);
      chr.ai.latch.y = (chr.ai.goto_y[chr.ai.goto_cnt]-chr.pos.y);

      if( fabsf(chr.ai.latch.x) + fabsf(chr.ai.latch.y) > 0 )
      {
        // scale the latches to lie between 0 and 1
        chr.ai.latch.x = chr.ai.latch.x/(WAYTHRESH + ABS(chr.ai.latch.x) + ABS(chr.ai.latch.y));
        chr.ai.latch.y = chr.ai.latch.y/(WAYTHRESH + ABS(chr.ai.latch.x) + ABS(chr.ai.latch.y));

        assert(fabs(chr.ai.latch.x)<1.1);
        assert(fabs(chr.ai.latch.y)<1.1);
      };
    }
    else
    {
      chr.ai.latch.clear();
    }
  }

  // Clear alerts for next time around
  chr.ai.alert = 0;
  if (changed)  chr.ai.alert = ALERT_IF_CHANGED;

  // Do poofing
  if (val[TMP_GOPOOF])
  {
    chr.ai.gopoof = true;

    //if (VALID_CHR(chr.held_by))
    //  detach_character_from_mount(character, true, false);

    //if (VALID_CHR(chr.holding_which[SLOT_LEFT]))
    //  detach_character_from_mount(chr.holding_which[SLOT_LEFT], true, false);

    //if (VALID_CHR(chr.holding_which[SLOT_RIGHT]))
    //  detach_character_from_mount(chr.holding_which[SLOT_RIGHT], true, false);

    //free_inventory(character);
    //free_one_character(character);

    // If this character was killed in another's script, we don't want the poof to
    // carry over...
    val[TMP_GOPOOF] = false;
  }
}

//--------------------------------------------------------------------------------------------
void let_ai_think()
{
  // ZZ> This function lets every computer controlled character do AI stuff
  int character;

  Blip::numblip = 0;
  SCAN_CHR_BEGIN(character, rchr_chr)
  {
    //if ( (rchr_chr.ai.alert&ALERT_IF_CLEANEDUP) || 
    //     (rchr_chr.ai.alert&ALERT_IF_CRUSHED)   || 
    //     (!rchr_chr.is_inpack && !rchr_chr.getCap().is_equipment)
    //{

      // Cleaned up characters shouldn't be alert to anything else
      if (rchr_chr.ai.alert&ALERT_IF_CLEANEDUP)  rchr_chr.ai.alert = ALERT_IF_CLEANEDUP;

      //if crushed, remove all other alerts because you be dead (or at least severely stunned)
      if (rchr_chr.ai.alert&ALERT_IF_CRUSHED)    rchr_chr.ai.alert = ALERT_IF_CRUSHED;

      let_character_think(character);
//    }
  } SCAN_CHR_END;

}

