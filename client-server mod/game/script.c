/* Egoboo - script.c
 * Implements the game's scripting language.
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

#include "egoboo.h"
#include "mathstuff.h"
#include "Log.h"

Uint8           cLineBuffer[MAXLINESIZE];
int                     iLoadSize;
int                     iLineSize;
int                     iNumLine;
Uint8           cCodeType[MAXCODE];
Uint32            iCodeValue[MAXCODE];
Uint8           cCodeName[MAXCODE][MAXCODENAMESIZE];
int                     iCodeIndex;
int                     iCodeValueTmp;
int                     iNumCode;
Uint32            iCompiledAis[AISMAXCOMPILESIZE];
int                     iAisStartPosition[MAXAI];
int                     iNumAis;
int                     iAisIndex;

//------------------------------------------------------------------------------
//AI Script Routines------------------------------------------------------------
//------------------------------------------------------------------------------
void insert_space(int position)
{
  // ZZ> This function adds a space into the load line if there isn't one
  //     there already
  Uint8 cTmp, cSwap;

  if (cLineBuffer[position] != ' ')
  {
    cTmp = cLineBuffer[position];
    cLineBuffer[position] = ' ';
    position++;
    iLineSize++;
    while (position < iLineSize)
    {
      cSwap = cLineBuffer[position];
      cLineBuffer[position] = cTmp;
      cTmp = cSwap;
      position++;
    }
    cLineBuffer[position] = 0;
  }
}

//------------------------------------------------------------------------------
void copy_one_line(int write)
{
  // ZZ> This function copies the line back to the load buffer
  int read;
  Uint8 cTmp;


  read = 0;
  cTmp = cLineBuffer[read];
  while (cTmp != 0)
  {
    cTmp = cLineBuffer[read];  read++;
    cLoadBuffer[write] = cTmp;  write++;
  }



  iNumLine++;
}

//------------------------------------------------------------------------------
int load_one_line(int read)
{
  // ZZ> This function loads a line into the line buffer
  int stillgoing, foundtext;
  Uint8 cTmp;


  // Parse to start to maintain indentation
  iLineSize = 0;
  stillgoing = btrue;
  while (stillgoing && read < iLoadSize)
  {
    cTmp = cLoadBuffer[read];
    stillgoing = bfalse;
    if (cTmp == ' ')
    {
      read++;
      cLineBuffer[iLineSize] = cTmp; iLineSize++;
      stillgoing = btrue;
    }
    // cLineBuffer[iLineSize] = 0; // or cTmp as cTmp == 0
  }


  // Parse to comment or end of line
  foundtext = bfalse;
  stillgoing = btrue;
  while (stillgoing && read < iLoadSize)
  {
    cTmp = cLoadBuffer[read];  read++;
    if (cTmp == '\t')
      cTmp = ' ';
    if (cTmp != ' ' && cTmp != 0x0d && cTmp != 0x0a &&
        (cTmp != '/' || cLoadBuffer[read] != '/'))
      foundtext = btrue;
    cLineBuffer[iLineSize] = cTmp;
    if (cTmp != ' ' || (cLoadBuffer[read] != ' ' && cLoadBuffer[read] != '\t'))
      iLineSize++;
    if (cTmp == 0x0d || cTmp == 0x0a)
      stillgoing = bfalse;
    if (cTmp == '/' && cLoadBuffer[read] == '/')
      stillgoing = bfalse;
  }
  if (stillgoing == bfalse)iLineSize--;

  cLineBuffer[iLineSize] = 0;
  if (iLineSize >= 1)
  {
    if (cLineBuffer[iLineSize-1] == ' ')
    {
      iLineSize--;  cLineBuffer[iLineSize] = 0;
    }
  }
  iLineSize++;


  // Parse to end of line
  stillgoing = btrue;
  read--;
  while (stillgoing)
  {
    cTmp = cLoadBuffer[read];  read++;
    if (cTmp == 0x0a || cTmp == 0x0d)
      stillgoing = bfalse;
  }

  if (foundtext == bfalse)
  {
    iLineSize = 0;
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
  cTmp = cLoadBuffer[read];
  while (cTmp != 0)
  {
    cLineBuffer[iLineSize] = cTmp;  iLineSize++;
    read++;  cTmp = cLoadBuffer[read];
  }
  cLineBuffer[iLineSize] = 0;
  read++; // skip terminating zero for next call of load_parsed_line()
  return read;
}

//------------------------------------------------------------------------------
void surround_space(int position)
{
  insert_space(position + 1);
  if (position > 0)
  {
    if (cLineBuffer[position-1] != ' ')
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
    read = load_one_line(read);
    if (iLineSize > 2)
    {
      copy_one_line(write);
      write += iLineSize;
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
  cTmp = cLineBuffer[cnt];
  while (cTmp == ' ')
  {
    cnt++;
    cTmp = cLineBuffer[cnt];
  }
  cnt = cnt >> 1;
  if (cnt > 15)
  {
    if (globalparseerr)
    {
      log_message("SCRIPT ERROR FOUND: %s - %d levels of indentation\n", globalparsename, cnt + 1);
    }
    parseerror = btrue;
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
    cTmp = cLineBuffer[cnt];
    if (cTmp == '+' || cTmp == '-' || cTmp == '/' || cTmp == '*' ||
        cTmp == '%' || cTmp == '>' || cTmp == '<' || cTmp == '&' ||
        cTmp == '=')
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
  // ZZ> This function returns btrue if the line starts with a capital
  int cnt;
  Uint8 cTmp;

  cnt = 0;
  cTmp = cLineBuffer[cnt];
  while (cTmp == ' ')
  {
    cnt++;
    cTmp = cLineBuffer[cnt];
  }
  if (cTmp >= 'A' && cTmp <= 'Z')
    return btrue;
  return bfalse;
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
    highbits = highbits | 16;
  }
  else
  {
  }
  highbits = highbits << 27;
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
  IDSZ idsz;
  char cWordBuffer[MAXCODENAMESIZE];


  // Check bounds
  iCodeIndex = MAXCODE;
  if (read >= iLineSize)  return read;


  // Skip spaces
  cTmp = cLineBuffer[read];
  while (cTmp == ' ')
  {
    read++;
    cTmp = cLineBuffer[read];
  }
  if (read >= iLineSize)  return read;


  // Load the word into the other buffer
  wordsize = 0;
  while (cTmp != ' ' && cTmp != 0)
  {
    cWordBuffer[wordsize] = cTmp;  wordsize++;
    read++;
    cTmp = cLineBuffer[read];
  }
  cWordBuffer[wordsize] = 0;


  // Check for numeric constant
  if (cWordBuffer[0] >= '0' && cWordBuffer[0] <= '9')
  {
    sscanf(&cWordBuffer[0], "%d", &iCodeValueTmp);
    iCodeIndex = -1;
    return read;
  }


  // Check for IDSZ constant
  idsz = IDSZNONE;
  if (cWordBuffer[0] == '[')
  {
    idsz = MAKE_IDSZ( &cWordBuffer[1] );
    iCodeValueTmp = idsz;
    iCodeIndex = -1;
    return read;
  }



  // Compare the word to all the codes
  codecorrect = bfalse;
  iCodeIndex = 0;
  while (iCodeIndex < iNumCode && !codecorrect)
  {
    codecorrect = bfalse;
    if (cWordBuffer[0] == cCodeName[iCodeIndex][0] && !codecorrect)
    {
      codecorrect = btrue;
      cnt = 1;
      while (cnt < wordsize)
      {
        if (cWordBuffer[cnt] != cCodeName[iCodeIndex][cnt])
        {
          codecorrect = bfalse;
          cnt = wordsize;
        }
        cnt++;
      }
      if (cnt < MAXCODENAMESIZE)
      {
        if (cCodeName[iCodeIndex][cnt] != 0)  codecorrect = bfalse;
      }
    }
    iCodeIndex++;
  }
  if (codecorrect)
  {
    iCodeIndex--;
    iCodeValueTmp = iCodeValue[iCodeIndex];
    if (cCodeType[iCodeIndex] == 'C')
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

        log_message("SCRIPT ERROR FOUND: %s - %s undefined\n", globalparsename, cWordBuffer);
      }

      parseerror = btrue;
    }
  }
  return read;
}

//------------------------------------------------------------------------------
void add_code(Uint32 highbits)
{
  Uint32 value;

  if (iCodeIndex == -1)  highbits = highbits | 0x80000000;
  if (iCodeIndex != MAXCODE)
  {
    value = highbits | iCodeValueTmp;
    iCompiledAis[iAisIndex] = value;
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
    read = load_parsed_line(read);
    fix_operators();
    highbits = get_high_bits();
    parseposition = 0;
    parseposition = tell_code(parseposition);  // VALUE
    add_code(highbits);
    iCodeValueTmp = 0;  // SKIP FOR CONTROL CODES
    add_code(0);
    if ((highbits&0x80000000) == 0)
    {
      parseposition = tell_code(parseposition);  // EQUALS
      parseposition = tell_code(parseposition);  // VALUE
      add_code(0);
      operands = 1;
      while (parseposition < iLineSize)
      {
        parseposition = tell_code(parseposition);  // OPERATOR
        if (iCodeIndex == -1) iCodeIndex = 1;
        else iCodeIndex = 0;
        highbits = ((iCodeValueTmp & 15) << 27) | (iCodeIndex << 31);
        parseposition = tell_code(parseposition);  // VALUE
        add_code(highbits);
        if (iCodeIndex != MAXCODE)
          operands++;
      }
      iCompiledAis[iAisIndex-operands-1] = operands;  // Number of operands
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


  value = iCompiledAis[index];  index += 2;
  targetindent = (value >> 27) & 15;
  indent = 100;
  while (indent > targetindent)
  {
    value = iCompiledAis[index];
    indent = (value >> 27) & 15;
    if (indent > targetindent)
    {
      // Was it a function
      if ((value&0x80000000) != 0)
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
        index += (value & 255);
      }
    }
  }
  return index;
}

//------------------------------------------------------------------------------
void parse_jumps(int ainumber)
{
  // ZZ> This function sets up the fail jumps for the down and dirty code
  int index;
  Uint32 value, iTmp;


  index = iAisStartPosition[ainumber];
  value = iCompiledAis[index];
  while (value != 0x80000035) // End Function
  {
    value = iCompiledAis[index];
    // Was it a function
    if ((value&0x80000000) != 0)
    {
      // Each function needs a jump
      iTmp = jump_goto(index);
      index++;
      iCompiledAis[index] = iTmp;
      index++;
    }
    else
    {
      // Operations cover each operand
      index++;
      iTmp = iCompiledAis[index];
      index++;
      index += (iTmp & 255);
    }
  }
}

//------------------------------------------------------------------------------
void log_code(int ainumber, char* savename)
{
  // ZZ> This function shows the actual code, saving it in a file
  int index;
  Uint32 value;
  FILE* filewrite;

  filewrite = fopen(savename, "w");
  if (filewrite)
  {
    index = iAisStartPosition[ainumber];
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
  while (cTmp != ':' && read < iLoadSize)
  {
    read++;  cTmp = cLoadBuffer[read];
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
  cCodeType[iNumCode] = cTmp;
  iCodeValue[iNumCode] = iTmp;
}

//------------------------------------------------------------------------------
void load_ai_codes(char* loadname)
{
  // ZZ> This function loads all of the function and variable names
  FILE* fileread;
  int read;

  iNumCode = 0;
  fileread = fopen(loadname, "rb");
  if (NULL == fileread)
  {

  };

  {
    iLoadSize = (int)fread(&cLoadBuffer[0], 1, MD2MAXLOADSIZE, fileread);
    read = 0;
    read = ai_goto_colon(read);
    while (read != iLoadSize)
    {
      get_code(read);
      iNumCode++;
      read = ai_goto_colon(read);
    }
    fclose(fileread);
  }
}

//------------------------------------------------------------------------------
int load_ai_script(char *loadname)
{
  // ZZ> This function loads a script to memory and
  //     returns bfalse if it fails to do so
  FILE* fileread;

  iNumLine = 0;
  globalparsename = loadname;  // For error logging in log.TXT
  fileread = fopen(loadname, "rb");
  if (fileread && iNumAis < MAXAI)
  {
    // Make room for the code
    iAisStartPosition[iNumAis] = iAisIndex;

    // Load into md2 load buffer
    iLoadSize = (int)fread(&cLoadBuffer[0], 1, MD2MAXLOADSIZE, fileread);
    fclose(fileread);
    parse_null_terminate_comments();
    parse_line_by_line();
    parse_jumps(iNumAis);
    iNumAis++;
    return btrue;
  }

  return bfalse;
}

//------------------------------------------------------------------------------
void reset_ai_script()
{
  // ZZ> This function starts ai loading in the right spot
  int cnt;

  iAisIndex = 0;
  for (cnt = 0; cnt < MAXMODEL; cnt++)
    MadList[cnt].ai = 0;

  iNumAis = 0;
}

//--------------------------------------------------------------------------------------------
Uint8 run_function(GAME_STATE * gs, Uint32 value, int character)
{
  // ZZ> This function runs a script function for the AI.
  //     It returns bfalse if the script should jump over the
  //     indented code that follows

  // Mask out the indentation
  Uint16 valuecode = value;

  // Assume that the function will pass, as most do
  Uint8 returncode = btrue;

  Uint16 sTmp;
  float fTmp;
  int iTmp, tTmp;
  int volume;
  Uint32 test;
  STRING cTmp;
  Uint32 scr_randie = gs->randie_index;

  // Figure out which function to run
  switch (valuecode)
  {
  case FIFSPAWNED:
    // Proceed only if it's a new character
    returncode = ((ChrList[character].alert & ALERTIFSPAWNED) != 0);
    break;

  case FIFTIMEOUT:
    // Proceed only if time alert is set
    returncode = (ChrList[character].aitime == 0);
    break;

  case FIFATWAYPOINT:
    // Proceed only if the character reached a waypoint
    returncode = ((ChrList[character].alert & ALERTIFATWAYPOINT) != 0);
    break;

  case FIFATLASTWAYPOINT:
    // Proceed only if the character reached its last waypoint
    returncode = ((ChrList[character].alert & ALERTIFATLASTWAYPOINT) != 0);
    break;

  case FIFATTACKED:
    // Proceed only if the character was damaged
    returncode = ((ChrList[character].alert & ALERTIFATTACKED) != 0);
    break;

  case FIFBUMPED:
    // Proceed only if the character was bumped
    returncode = ((ChrList[character].alert & ALERTIFBUMPED) != 0);
    break;

  case FIFORDERED:
    // Proceed only if the character was GOrder.ed
    returncode = ((ChrList[character].alert & ALERTIFORDERED) != 0);
    break;

  case FIFCALLEDFORHELP:
    // Proceed only if the character was called for help
    returncode = ((ChrList[character].alert & ALERTIFCALLEDFORHELP) != 0);
    break;

  case FSETCONTENT:
    // Set the content
    ChrList[character].aicontent = valuetmpargument;
    break;

  case FIFKILLED:
    // Proceed only if the character's been killed
    returncode = ((ChrList[character].alert & ALERTIFKILLED) != 0);
    break;

  case FIFTARGETKILLED:
    // Proceed only if the character's target has just died
    returncode = ((ChrList[character].alert & ALERTIFTARGETKILLED) != 0);
    break;

  case FCLEARWAYPOINTS:
    // Clear out all waypoints
    ChrList[character].aigoto = 0;
    ChrList[character].aigotoadd = 0;
    ChrList[character].aigotox[0] = ChrList[character].xpos;
    ChrList[character].aigotoy[0] = ChrList[character].ypos;
    break;

  case FADDWAYPOINT:
    // Add a waypoint to the waypoint list
    ChrList[character].aigotox[ChrList[character].aigotoadd] = valuetmpx;
    ChrList[character].aigotoy[ChrList[character].aigotoadd] = valuetmpy;
    ChrList[character].aigotoadd++;
    if (ChrList[character].aigotoadd > MAXWAY)  ChrList[character].aigotoadd = MAXWAY - 1;
    break;

  case FFINDPATH:
    // This function adds enough waypoints to get from one point to another
    // !!!BAD!!!
    break;

  case FCOMPASS:
    // This function changes tmpx and tmpy in a circlular manner according
    // to tmpturn and tmpdistance
    sTmp = (valuetmpturn + 16384);
    valuetmpx = valuetmpx - turntosin[(sTmp>>2) & TRIGTABLE_MASK] * valuetmpdistance;
    valuetmpy = valuetmpy - turntosin[(valuetmpturn>>2) & TRIGTABLE_MASK] * valuetmpdistance;
    break;

  case FGETTARGETARMORPRICE:
    // This function gets the armor cost for the given skin
    sTmp = valuetmpargument  % MAXSKIN;
    valuetmpx = CapList[ChrList[ChrList[character].aitarget].model].skincost[sTmp];
    break;

  case FSETTIME:
    // This function resets the time
    ChrList[character].aitime = valuetmpargument;
    break;

  case FGETCONTENT:
    // Get the content
    valuetmpargument = ChrList[character].aicontent;
    break;

  case FJOINTARGETTEAM:
    // This function allows the character to leave its own team and join another
    returncode = bfalse;
    if (ChrList[ChrList[character].aitarget].on)
    {
      switch_team(character, ChrList[ChrList[character].aitarget].team);
      returncode = btrue;
    }
    break;

  case FSETTARGETTONEARBYENEMY:
    // This function finds a nearby enemy, and proceeds only if there is one
    sTmp = get_nearby_target(character, bfalse, bfalse, btrue, bfalse, IDSZNONE);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FSETTARGETTOTARGETLEFTHAND:
    // This function sets the target to the target's left item
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[0];
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FSETTARGETTOTARGETRIGHTHAND:
    // This function sets the target to the target's right item
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[1];
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FSETTARGETTOWHOEVERATTACKED:
    // This function sets the target to whoever attacked the character last,
    // failing for damage tiles
    if (ChrList[character].attacklast != MAXCHR)
    {
      ChrList[character].aitarget = ChrList[character].attacklast;
    }
    else
    {
      returncode = bfalse;
    }
    break;

  case FSETTARGETTOWHOEVERBUMPED:
    // This function sets the target to whoever bumped into the
    // character last.  It never fails
    ChrList[character].aitarget = ChrList[character].bumplast;
    break;

  case FSETTARGETTOWHOEVERCALLEDFORHELP:
    // This function sets the target to whoever needs help
    ChrList[character].aitarget = TeamList[ChrList[character].team].sissy;
    break;

  case FSETTARGETTOOLDTARGET:
    // This function reverts to the target with whom the script started
    ChrList[character].aitarget = valueoldtarget;
    break;

  case FSETTURNMODETOVELOCITY:
    // This function sets the turn mode
    ChrList[character].turnmode = TURNMODEVELOCITY;
    break;

  case FSETTURNMODETOWATCH:
    // This function sets the turn mode
    ChrList[character].turnmode = TURNMODEWATCH;
    break;

  case FSETTURNMODETOSPIN:
    // This function sets the turn mode
    ChrList[character].turnmode = TURNMODESPIN;
    break;

  case FSETBUMPHEIGHT:
    // This function changes a character's bump height
    ChrList[character].bumpheight = valuetmpargument * ChrList[character].fat;
    ChrList[character].bumpheightsave = valuetmpargument;
    break;

  case FIFTARGETHASID:
    // This function proceeds if ID matches tmpargument
    sTmp = ChrList[ChrList[character].aitarget].model;
    returncode = CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument;
    returncode = returncode | (CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument);
    break;

  case FIFTARGETHASITEMID:
    // This function proceeds if the target has a matching item in his/her pack
    returncode = bfalse;
    // Check the pack
    sTmp = ChrList[ChrList[character].aitarget].nextinpack;
    while (sTmp != MAXCHR)
    {
      if (CapList[ChrList[sTmp].model].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[ChrList[sTmp].model].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
      {
        returncode = btrue;
        sTmp = MAXCHR;
      }
      else
      {
        sTmp = ChrList[sTmp].nextinpack;
      }
    }
    // Check left hand
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[0];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
        returncode = btrue;
    }
    // Check right hand
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[1];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
        returncode = btrue;
    }
    break;

  case FIFTARGETHOLDINGITEMID:
    // This function proceeds if ID matches tmpargument and returns the latch for the
    // hand in tmpargument
    returncode = bfalse;
    // Check left hand
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[0];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
      {
        valuetmpargument = LATCHBUTTONLEFT;
        returncode = btrue;
      }
    }
    // Check right hand
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[1];
    if (sTmp != MAXCHR && returncode == bfalse)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
      {
        valuetmpargument = LATCHBUTTONRIGHT;
        returncode = btrue;
      }
    }
    break;

  case FIFTARGETHASSKILLID:
    // This function proceeds if ID matches tmpargument
	  returncode = check_skills(ChrList[character].aitarget, valuetmpargument);
    break;

  case FELSE:
    // This function fails if the last one was more indented
    if ((valuelastindent&0x78000000) > (value&0x78000000))
      returncode = bfalse;
    break;

  case FRUN:
    reset_character_accel(character);
    break;

  case FWALK:
    reset_character_accel(character);
    ChrList[character].maxaccel *= .66;
    break;

  case FSNEAK:
    reset_character_accel(character);
    ChrList[character].maxaccel *= .33;
    break;

  case FDOACTION:
    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid or if the character is doing
    // something else already
    returncode = bfalse;
    if (valuetmpargument < MAXACTION && ChrList[character].actionready)
    {
      if (MadList[ChrList[character].model].actionvalid[valuetmpargument])
      {
        ChrList[character].action = valuetmpargument;
        ChrList[character].lip = 0;
        ChrList[character].lastframe = ChrList[character].frame;
        ChrList[character].frame = MadList[ChrList[character].model].actionstart[valuetmpargument];
        ChrList[character].actionready = bfalse;
        returncode = btrue;
      }
    }
    break;

  case FKEEPACTION:
    // This function makes the current animation halt on the last frame
    ChrList[character].keepaction = btrue;
    break;

  case FISSUEORDER:
    // This function issues an order to all teammates
    issue_order(character, valuetmpargument);
    break;

  case FDROPWEAPONS:
    // This funtion drops the character's in hand items/riders
    sTmp = ChrList[character].holdingwhich[0];
    if (sTmp != MAXCHR)
    {
      detach_character_from_mount(gs, sTmp, btrue, btrue, &scr_randie);
      if (ChrList[character].ismount)
      {
        ChrList[sTmp].zvel = DISMOUNTZVEL;
        ChrList[sTmp].zpos += DISMOUNTZVEL;
        ChrList[sTmp].jumptime = JUMPDELAY;
      }
    }
    sTmp = ChrList[character].holdingwhich[1];
    if (sTmp != MAXCHR)
    {
      detach_character_from_mount(gs, sTmp, btrue, btrue, &scr_randie);
      if (ChrList[character].ismount)
      {
        ChrList[sTmp].zvel = DISMOUNTZVEL;
        ChrList[sTmp].zpos += DISMOUNTZVEL;
        ChrList[sTmp].jumptime = JUMPDELAY;
      }
    }
    break;

  case FTARGETDOACTION:
    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid or if the target is doing
    // something else already
    returncode = bfalse;
    if (ChrList[ChrList[character].aitarget].alive)
    {
      if (valuetmpargument < MAXACTION && ChrList[ChrList[character].aitarget].actionready)
      {
        if (MadList[ChrList[ChrList[character].aitarget].model].actionvalid[valuetmpargument])
        {
          ChrList[ChrList[character].aitarget].action = valuetmpargument;
          ChrList[ChrList[character].aitarget].lip = 0;
          ChrList[ChrList[character].aitarget].lastframe = ChrList[ChrList[character].aitarget].frame;
          ChrList[ChrList[character].aitarget].frame = MadList[ChrList[ChrList[character].aitarget].model].actionstart[valuetmpargument];
          ChrList[ChrList[character].aitarget].actionready = bfalse;
          returncode = btrue;
        }
      }
    }
    break;

  case FOPENPASSAGE:
    // This function opens the passage specified by tmpargument, failing if the
    // passage was already open
    returncode = open_passage(valuetmpargument);
    break;

  case FCLOSEPASSAGE:
    // This function closes the passage specified by tmpargument, and proceeds
    // only if the passage is clear of obstructions
    returncode = close_passage(valuetmpargument);
    break;

  case FIFPASSAGEOPEN:
    // This function proceeds only if the passage specified by tmpargument
    // is both valid and open
    returncode = bfalse;
    if (valuetmpargument < numpassage && valuetmpargument >= 0)
    {
      returncode = passopen[valuetmpargument];
    }
    break;

  case FGOPOOF:
    // This function flags the character to be removed from the game
    returncode = bfalse;
    if (ChrList[character].isplayer == bfalse)
    {
      returncode = btrue;
      valuegopoof = btrue;
    }
    break;

  case FCOSTTARGETITEMID:
    // This function checks if the target has a matching item, and poofs it
    returncode = bfalse;
    // Check the pack
    iTmp = MAXCHR;
    tTmp = ChrList[character].aitarget;
    sTmp = ChrList[tTmp].nextinpack;
    while (sTmp != MAXCHR)
    {
      if (CapList[ChrList[sTmp].model].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[ChrList[sTmp].model].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
      {
        returncode = btrue;
        iTmp = sTmp;
        sTmp = MAXCHR;
      }
      else
      {
        tTmp = sTmp;
        sTmp = ChrList[sTmp].nextinpack;
      }
    }
    // Check left hand
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[0];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
      {
        returncode = btrue;
        iTmp = ChrList[ChrList[character].aitarget].holdingwhich[0];
      }
    }
    // Check right hand
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[1];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
      {
        returncode = btrue;
        iTmp = ChrList[ChrList[character].aitarget].holdingwhich[1];
      }
    }
    if (returncode)
    {
      if (ChrList[iTmp].ammo <= 1)
      {
        // Poof the item
        if (ChrList[iTmp].inpack)
        {
          // Remove from the pack
          ChrList[tTmp].nextinpack = ChrList[iTmp].nextinpack;
          ChrList[ChrList[character].aitarget].numinpack--;
          free_one_character(iTmp);
        }
        else
        {
          // Drop from hand
          detach_character_from_mount(gs, iTmp, btrue, bfalse, &scr_randie);
          free_one_character(iTmp);
        }
      }
      else
      {
        // Cost one ammo
        ChrList[iTmp].ammo--;
      }
    }
    break;

  case FDOACTIONOVERRIDE:
    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid
    returncode = bfalse;
    if (valuetmpargument < MAXACTION)
    {
      if (MadList[ChrList[character].model].actionvalid[valuetmpargument])
      {
        ChrList[character].action = valuetmpargument;
        ChrList[character].lip = 0;
        ChrList[character].lastframe = ChrList[character].frame;
        ChrList[character].frame = MadList[ChrList[character].model].actionstart[valuetmpargument];
        ChrList[character].actionready = bfalse;
        returncode = btrue;
      }
    }
    break;

  case FIFHEALED:
    // Proceed only if the character was healed
    returncode = ((ChrList[character].alert & ALERTIFHEALED) != 0);
    break;

  case FSENDMESSAGE:
    // This function sends a message to the players
    display_message(MadList[ChrList[character].model].msg_start + valuetmpargument, character);
    break;

  case FCALLFORHELP:
    // This function issues a call for help
    call_for_help(character);
    break;

  case FADDIDSZ:
    // This function adds an idsz to the module's menu.txt file
    add_module_idsz(pickedmodule, valuetmpargument);
    break;

  case FSETSTATE:
    // This function sets the character's state variable
    ChrList[character].aistate = valuetmpargument;
    break;

  case FGETSTATE:
    // This function reads the character's state variable
    valuetmpargument = ChrList[character].aistate;
    break;

  case FIFSTATEIS:
    // This function fails if the character's state is inequal to tmpargument
    returncode = (valuetmpargument == ChrList[character].aistate);
    break;

  case FIFTARGETCANOPENSTUFF:
    // This function fails if the target can't open stuff
    returncode = ChrList[ChrList[character].aitarget].openstuff;
    break;

  case FIFGRABBED:
    // Proceed only if the character was picked up
    returncode = ((ChrList[character].alert & ALERTIFGRABBED) != 0);
    break;

  case FIFDROPPED:
    // Proceed only if the character was dropped
    returncode = ((ChrList[character].alert & ALERTIFDROPPED) != 0);
    break;

  case FSETTARGETTOWHOEVERISHOLDING:
    // This function sets the target to the character's mount or holder,
    // failing if the character has no mount or holder
    returncode = bfalse;
    if (ChrList[character].attachedto < MAXCHR)
    {
      ChrList[character].aitarget = ChrList[character].attachedto;
      returncode = btrue;
    }
    break;

  case FDAMAGETARGET:
    // This function applies little bit of love to the character's target.
    // The amount is set in tmpargument
    damage_character(gs, ChrList[character].aitarget, 0, valuetmpargument, 1, ChrList[character].damagetargettype, ChrList[character].team, character, DAMFXBLOC, &scr_randie);
    break;

  case FIFXISLESSTHANY:
    // Proceed only if tmpx is less than tmpy
    returncode = (valuetmpx < valuetmpy);
    break;

  case FSETWEATHERTIME:
    // Set the weather timer
    GWeather.timereset = valuetmpargument;
    GWeather.time = valuetmpargument;
    break;

  case FGETBUMPHEIGHT:
    // Get the characters bump height
    valuetmpargument = ChrList[character].bumpheight;
    break;

  case FIFREAFFIRMED:
    // Proceed only if the character was reaffirmed
    returncode = ((ChrList[character].alert & ALERTIFREAFFIRMED) != 0);
    break;

  case FUNKEEPACTION:
    // This function makes the current animation start again
    ChrList[character].keepaction = bfalse;
    break;

  case FIFTARGETISONOTHERTEAM:
    // This function proceeds only if the target is on another team
    returncode = (ChrList[ChrList[character].aitarget].alive && ChrList[ChrList[character].aitarget].team != ChrList[character].team);
    break;

  case FIFTARGETISONHATEDTEAM:
    // This function proceeds only if the target is on an enemy team
    returncode = (ChrList[ChrList[character].aitarget].alive && TeamList[ChrList[character].team].hatesteam[ChrList[ChrList[character].aitarget].team] && ChrList[ChrList[character].aitarget].invictus == bfalse);
    break;

  case FPRESSLATCHBUTTON:
    // This function sets the latch buttons
    ChrList[character].latchbutton = ChrList[character].latchbutton | valuetmpargument;
    break;

  case FSETTARGETTOTARGETOFLEADER:
    // This function sets the character's target to the target of its leader,
    // or it fails with no change if the leader is dead
    returncode = bfalse;
    if (TeamList[ChrList[character].team].leader != NOLEADER)
    {
      ChrList[character].aitarget = ChrList[TeamList[ChrList[character].team].leader].aitarget;
      returncode = btrue;
    }
    break;

  case FIFLEADERKILLED:
    // This function proceeds only if the character's leader has just died
    returncode = ((ChrList[character].alert & ALERTIFLEADERKILLED) != 0);
    break;

  case FBECOMELEADER:
    // This function makes the character the team leader
    TeamList[ChrList[character].team].leader = character;
    break;

  case FCHANGETARGETARMOR:
    // This function sets the target's armor type and returns the old type
    // as tmpargument and the new type as tmpx
    iTmp = (ChrList[ChrList[character].aitarget].texture - MadList[ChrList[ChrList[character].aitarget].model].skinstart) % MAXSKIN;
    valuetmpx = change_armor(gs, ChrList[character].aitarget, valuetmpargument, &scr_randie);
    valuetmpargument = iTmp;  // The character's old armor
    break;

  case FGIVEMONEYTOTARGET:
    // This function transfers money from the character to the target, and sets
    // tmpargument to the amount transferred
    iTmp = ChrList[character].money;
    tTmp = ChrList[ChrList[character].aitarget].money;
    iTmp -= valuetmpargument;
    tTmp += valuetmpargument;
    if (iTmp < 0) { tTmp += iTmp;  valuetmpargument += iTmp;  iTmp = 0; }
    if (tTmp < 0) { iTmp += tTmp;  valuetmpargument += tTmp;  tTmp = 0; }
    if (iTmp > MAXMONEY) { iTmp = MAXMONEY; }
    if (tTmp > MAXMONEY) { tTmp = MAXMONEY; }
    ChrList[character].money = iTmp;
    ChrList[ChrList[character].aitarget].money = tTmp;
    break;

  case FDROPKEYS:
    drop_keys(character);
    break;

  case FIFLEADERISALIVE:
    // This function fails if there is no team leader
    returncode = (TeamList[ChrList[character].team].leader != NOLEADER);
    break;

  case FIFTARGETISOLDTARGET:
    // This function returns bfalse if the target has changed
    returncode = (ChrList[character].aitarget == valueoldtarget);
    break;

  case FSETTARGETTOLEADER:
    // This function fails if there is no team leader
    if (TeamList[ChrList[character].team].leader == NOLEADER)
    {
      returncode = bfalse;
    }
    else
    {
      ChrList[character].aitarget = TeamList[ChrList[character].team].leader;
    }
    break;

  case FSPAWNCHARACTER:
    // This function spawns a character, failing if x,y is invalid
    sTmp = spawn_one_character(valuetmpx, valuetmpy, 0, ChrList[character].model, ChrList[character].team, 0, valuetmpturn, NULL, MAXCHR, &scr_randie);
    returncode = bfalse;
    if (sTmp < MAXCHR)
    {
      if (__chrhitawall(sTmp))
      {
        free_one_character(sTmp);
      }
      else
      {
        tTmp = ChrList[character].turnleftright >> 2;
        ChrList[sTmp].xvel += turntosin[(tTmp+12288) & TRIGTABLE_MASK] * valuetmpdistance;
        ChrList[sTmp].yvel += turntosin[(tTmp+8192) & TRIGTABLE_MASK] * valuetmpdistance;
        ChrList[sTmp].passage = ChrList[character].passage;
        ChrList[sTmp].iskursed = bfalse;
        ChrList[character].aichild = sTmp;
        ChrList[sTmp].aiowner = ChrList[character].aiowner;
        returncode = btrue;
      }
    }
    break;

  case FRESPAWNCHARACTER:
    // This function respawns the character at its starting location
    respawn_character(character, &scr_randie);
    break;

  case FCHANGETILE:
    // This function changes the floor image under the character
    Mesh.fanlist[ChrList[character].onwhichfan].tile = valuetmpargument & (255);
    break;

  case FIFUSED:
    // This function proceeds only if the character has been used
    returncode = ((ChrList[character].alert & ALERTIFUSED) != 0);
    break;

  case FDROPMONEY:
    // This function drops some of a character's money
    drop_money(character, valuetmpargument, &scr_randie);
    break;

  case FSETOLDTARGET:
    // This function sets the old target to the current target
    valueoldtarget = ChrList[character].aitarget;
    break;

  case FDETACHFROMHOLDER:
    // This function drops the character, failing only if it was not held
    if (ChrList[character].attachedto != MAXCHR)
    {
      detach_character_from_mount(gs, character, btrue, btrue, &scr_randie);
    }
    else
    {
      returncode = bfalse;
    }
    break;

  case FIFTARGETHASVULNERABILITYID:
    // This function proceeds if ID matches tmpargument
    returncode = (CapList[ChrList[ChrList[character].aitarget].model].idsz[IDSZVULNERABILITY] == (Uint32) valuetmpargument);
    break;

  case FCLEANUP:
    // This function issues the clean up order to all teammates
    issue_clean(character);
    break;

  case FIFCLEANEDUP:
    // This function proceeds only if the character was told to clean up
    returncode = ((ChrList[character].alert & ALERTIFCLEANEDUP) != 0);
    break;

  case FIFSITTING:
    // This function proceeds if the character is riding another
    returncode = (ChrList[character].attachedto != MAXCHR);
    break;

  case FIFTARGETISHURT:
    // This function passes only if the target is hurt and alive
    if (ChrList[ChrList[character].aitarget].alive == bfalse || ChrList[ChrList[character].aitarget].life > ChrList[ChrList[character].aitarget].lifemax - HURTDAMAGE)
      returncode = bfalse;
    break;

  case FIFTARGETISAPLAYER:
    // This function proceeds only if the target is a player ( may not be local )
    returncode = ChrList[ChrList[character].aitarget].isplayer;
    break;

  case FPLAYSOUND:
    // This function plays a sound
    if (ChrList[character].oldz > PITNOSOUND)
    {
      play_sound(ChrList[character].oldx, ChrList[character].oldy, CapList[ChrList[character].model].waveindex[valuetmpargument]);
    }
    break;

  case FSPAWNPARTICLE:
    // This function spawns a particle
    tTmp = character;
    if (ChrList[character].attachedto != MAXCHR)  tTmp = ChrList[character].attachedto;
    tTmp = spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, ChrList[character].turnleftright, ChrList[character].model, valuetmpargument, character, valuetmpdistance, ChrList[character].team, tTmp, 0, MAXCHR, &scr_randie);
    if (tTmp != MAXPRT)
    {
      // Detach the particle
      attach_particle_to_character(tTmp, character, valuetmpdistance);
      PrtList[tTmp].attachedtocharacter = MAXCHR;
      // Correct X, Y, Z spacing
      PrtList[tTmp].xpos += valuetmpx;
      PrtList[tTmp].ypos += valuetmpy;
      PrtList[tTmp].zpos += PipList[PrtList[tTmp].pip].zspacingbase;
      // Don't spawn in walls
      if (__prthitawall(tTmp))
      {
        PrtList[tTmp].xpos = ChrList[character].xpos;
        if (__prthitawall(tTmp))
        {
          PrtList[tTmp].ypos = ChrList[character].ypos;
        }
      }
    }
    break;

  case FIFTARGETISALIVE:
    // This function proceeds only if the target is alive
    returncode = (ChrList[ChrList[character].aitarget].alive == btrue);
    break;

  case FSTOP:
    ChrList[character].maxaccel = 0;
    break;

  case FDISAFFIRMCHARACTER:
    disaffirm_attached_particles(character, &scr_randie);
    break;

  case FREAFFIRMCHARACTER:
    reaffirm_attached_particles(character, &scr_randie);
    break;

  case FIFTARGETISSELF:
    // This function proceeds only if the target is the character too
    returncode = (ChrList[character].aitarget == character);
    break;

  case FIFTARGETISMALE:
    // This function proceeds only if the target is male
    returncode = (ChrList[character].gender == GENMALE);
    break;

  case FIFTARGETISFEMALE:
    // This function proceeds only if the target is female
    returncode = (ChrList[character].gender == GENFEMALE);
    break;

  case FSETTARGETTOSELF:
    // This function sets the target to the character
    ChrList[character].aitarget = character;
    break;

  case FSETTARGETTORIDER:
    // This function sets the target to the character's left/only grip weapon,
    // failing if there is none
    if (ChrList[character].holdingwhich[0] == MAXCHR)
    {
      returncode = bfalse;
    }
    else
    {
      ChrList[character].aitarget = ChrList[character].holdingwhich[0];
    }
    break;

  case FGETATTACKTURN:
    // This function sets tmpturn to the direction of the last attack
    valuetmpturn = ChrList[character].directionlast;
    break;

  case FGETDAMAGETYPE:
    // This function gets the last type of damage
    valuetmpargument = ChrList[character].damagetypelast;
    break;

  case FBECOMESPELL:
    // This function turns the spellbook character into a spell based on its
    // content
    ChrList[character].money = (ChrList[character].texture - MadList[ChrList[character].model].skinstart) % MAXSKIN;
    change_character(gs, character, ChrList[character].aicontent, 0, LEAVENONE, &scr_randie);
    ChrList[character].aicontent = 0;  // Reset so it doesn't mess up
    ChrList[character].aistate = 0;  // Reset so it doesn't mess up
    changed = btrue;
    break;

  case FBECOMESPELLBOOK:
    // This function turns the spell into a spellbook, and sets the content
    // accordingly
    ChrList[character].aicontent = ChrList[character].model;
    change_character(gs, character, SPELLBOOK, ChrList[character].money % MAXSKIN, LEAVENONE, &scr_randie);
    ChrList[character].aistate = 0;  // Reset so it doesn't burn up
    changed = btrue;
    break;

  case FIFSCOREDAHIT:
    // Proceed only if the character scored a hit
    returncode = ((ChrList[character].alert & ALERTIFSCOREDAHIT) != 0);
    break;

  case FIFDISAFFIRMED:
    // Proceed only if the character was disaffirmed
    returncode = ((ChrList[character].alert & ALERTIFDISAFFIRMED) != 0);
    break;

  case FTRANSLATEORDER:
    // This function gets the order and sets tmpx, tmpy, tmpargument and the
    // target ( if valid )
    sTmp = ChrList[character].order >> 24;
    if (sTmp < MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
    }
    valuetmpx = ((ChrList[character].order >> 14) & 1023) << 6;
    valuetmpy = ((ChrList[character].order >> 4) & 1023) << 6;
    valuetmpargument = ChrList[character].order & 15;
    break;

  case FSETTARGETTOWHOEVERWASHIT:
    // This function sets the target to whoever the character hit last,
    ChrList[character].aitarget = ChrList[character].hitlast;
    break;

  case FSETTARGETTOWIDEENEMY:
    // This function finds an enemy, and proceeds only if there is one
    sTmp = get_wide_target(character, bfalse, bfalse, btrue, bfalse, IDSZNONE, bfalse);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FIFCHANGED:
    // Proceed only if the character was polymorphed
    returncode = ((ChrList[character].alert & ALERTIFCHANGED) != 0);
    break;

  case FIFINWATER:
    // Proceed only if the character got wet
    returncode = ((ChrList[character].alert & ALERTIFINWATER) != 0);
    break;

  case FIFBORED:
    // Proceed only if the character is bored
    returncode = ((ChrList[character].alert & ALERTIFBORED) != 0);
    break;

  case FIFTOOMUCHBAGGAGE:
    // Proceed only if the character tried to grab too much
    returncode = ((ChrList[character].alert & ALERTIFTOOMUCHBAGGAGE) != 0);
    break;

  case FIFGROGGED:
    // Proceed only if the character was grogged
    returncode = ((ChrList[character].alert & ALERTIFGROGGED) != 0);
    break;

  case FIFDAZED:
    // Proceed only if the character was dazed
    returncode = ((ChrList[character].alert & ALERTIFDAZED) != 0);
    break;

  case FIFTARGETHASSPECIALID:
    // This function proceeds if ID matches tmpargument
    returncode = (CapList[ChrList[ChrList[character].aitarget].model].idsz[IDSZSPECIAL] == (Uint32) valuetmpargument);
    break;

  case FPRESSTARGETLATCHBUTTON:
    // This function sets the target's latch buttons
    ChrList[ChrList[character].aitarget].latchbutton = ChrList[ChrList[character].aitarget].latchbutton | valuetmpargument;
    break;

  case FIFINVISIBLE:
    // This function passes if the character is invisible
    returncode = (ChrList[character].alpha <= INVISIBLE) || (ChrList[character].light <= INVISIBLE);
    break;

  case FIFARMORIS:
    // This function passes if the character's skin is tmpargument
    tTmp = (ChrList[character].texture - MadList[ChrList[character].model].skinstart) % MAXSKIN;
    returncode = (tTmp == valuetmpargument);
    break;

  case FGETTARGETGROGTIME:
    // This function returns tmpargument as the grog time, and passes if it is not 0
    valuetmpargument = ChrList[character].grogtime;
    returncode = (valuetmpargument != 0);
    break;

  case FGETTARGETDAZETIME:
    // This function returns tmpargument as the daze time, and passes if it is not 0
    valuetmpargument = ChrList[character].dazetime;
    returncode = (valuetmpargument != 0);
    break;

  case FSETDAMAGETYPE:
    // This function sets the bump damage type
    ChrList[character].damagetargettype = valuetmpargument & (MAXDAMAGETYPE - 1);
    break;

  case FSETWATERLEVEL:
    // This function raises and lowers the module's water
    fTmp = (valuetmpargument / 10.0) - waterdouselevel;
    watersurfacelevel += fTmp;
    waterdouselevel += fTmp;
    for (iTmp = 0; iTmp < MAXWATERLAYER; iTmp++)
      waterlayerz[iTmp] += fTmp;
    break;

  case FENCHANTTARGET:
    // This function enchants the target
    sTmp = spawn_enchant(gs, ChrList[character].aiowner, ChrList[character].aitarget, character, MAXENCHANT, MAXMODEL, &scr_randie);
    returncode = (sTmp != MAXENCHANT);
    break;

  case FENCHANTCHILD:
    // This function can be used with SpawnCharacter to enchant the
    // newly spawned character
    sTmp = spawn_enchant(gs, ChrList[character].aiowner, ChrList[character].aichild, character, MAXENCHANT, MAXMODEL, &scr_randie);
    returncode = (sTmp != MAXENCHANT);
    break;

  case FTELEPORTTARGET:
    // This function teleports the target to the X, Y location, failing if the
    // location is off the map or blocked
    returncode = bfalse;
    if (valuetmpx > EDGE && valuetmpy > EDGE && valuetmpx < Mesh.edgex - EDGE && valuetmpy < Mesh.edgey - EDGE)
    {
      // Yeah!  It worked!
      sTmp = ChrList[character].aitarget;
      detach_character_from_mount(gs, sTmp, btrue, bfalse, &scr_randie);
      ChrList[sTmp].oldx = ChrList[sTmp].xpos;
      ChrList[sTmp].oldy = ChrList[sTmp].ypos;
      ChrList[sTmp].xpos = valuetmpx;
      ChrList[sTmp].ypos = valuetmpy;
      if (__chrhitawall(sTmp))
      {
        // No it didn't...
        ChrList[sTmp].xpos = ChrList[sTmp].oldx;
        ChrList[sTmp].ypos = ChrList[sTmp].oldy;
        returncode = bfalse;
      }
      else
      {
        ChrList[sTmp].oldx = ChrList[sTmp].xpos;
        ChrList[sTmp].oldy = ChrList[sTmp].ypos;
        returncode = btrue;
      }
    }
    break;

  case FGIVEEXPERIENCETOTARGET:
    // This function gives the target some experience, xptype from distance,
    // amount from argument...
    give_experience(ChrList[character].aitarget, valuetmpargument, valuetmpdistance);
    break;

  case FINCREASEAMMO:
    // This function increases the ammo by one
    if (ChrList[character].ammo < ChrList[character].ammomax)
    {
      ChrList[character].ammo++;
    }
    break;

  case FUNKURSETARGET:
    // This function unkurses the target
    ChrList[ChrList[character].aitarget].iskursed = bfalse;
    break;

  case FGIVEEXPERIENCETOTARGETTEAM:
    // This function gives experience to everyone on the target's team
    give_team_experience(ChrList[ChrList[character].aitarget].team, valuetmpargument, valuetmpdistance);
    break;

  case FIFUNARMED:
    // This function proceeds if the character has no item in hand
    returncode = (ChrList[character].holdingwhich[0] == MAXCHR && ChrList[character].holdingwhich[1] == MAXCHR);
    break;

  case FRESTOCKTARGETAMMOIDALL:
    // This function restocks the ammo of every item the character is holding,
    // if the item matches the ID given ( parent or child type )
    iTmp = 0;  // Amount of ammo given
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[0];
    iTmp += restock_ammo(sTmp, valuetmpargument);
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[1];
    iTmp += restock_ammo(sTmp, valuetmpargument);
    sTmp = ChrList[ChrList[character].aitarget].nextinpack;
    while (sTmp != MAXCHR)
    {
      iTmp += restock_ammo(sTmp, valuetmpargument);
      sTmp = ChrList[sTmp].nextinpack;
    }
    valuetmpargument = iTmp;
    returncode = (iTmp != 0);
    break;

  case FRESTOCKTARGETAMMOIDFIRST:
    // This function restocks the ammo of the first item the character is holding,
    // if the item matches the ID given ( parent or child type )
    iTmp = 0;  // Amount of ammo given
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[0];
    iTmp += restock_ammo(sTmp, valuetmpargument);
    if (iTmp == 0)
    {
      sTmp = ChrList[ChrList[character].aitarget].holdingwhich[1];
      iTmp += restock_ammo(sTmp, valuetmpargument);
      if (iTmp == 0)
      {
        sTmp = ChrList[ChrList[character].aitarget].nextinpack;
        while (sTmp != MAXCHR && iTmp == 0)
        {
          iTmp += restock_ammo(sTmp, valuetmpargument);
          sTmp = ChrList[sTmp].nextinpack;
        }
      }
    }
    valuetmpargument = iTmp;
    returncode = (iTmp != 0);
    break;

  case FFLASHTARGET:
    // This function flashes the character
    flash_character(ChrList[character].aitarget, 255);
    break;

  case FSETREDSHIFT:
    // This function alters a character's coloration
    ChrList[character].redshift = valuetmpargument;
    break;

  case FSETGREENSHIFT:
    // This function alters a character's coloration
    ChrList[character].grnshift = valuetmpargument;
    break;

  case FSETBLUESHIFT:
    // This function alters a character's coloration
    ChrList[character].blushift = valuetmpargument;
    break;

  case FSETLIGHT:
    // This function alters a character's transparency
    ChrList[character].light = valuetmpargument;
    break;

  case FSETALPHA:
    // This function alters a character's transparency
    ChrList[character].alpha = valuetmpargument;
    break;

  case FIFHITFROMBEHIND:
    // This function proceeds if the character was attacked from behind
    returncode = bfalse;
    if (ChrList[character].directionlast >= BEHIND - 8192 && ChrList[character].directionlast < BEHIND + 8192)
      returncode = btrue;
    break;

  case FIFHITFROMFRONT:
    // This function proceeds if the character was attacked from the front
    returncode = bfalse;
    if (ChrList[character].directionlast >= 49152 + 8192 || ChrList[character].directionlast < FRONT + 8192)
      returncode = btrue;
    break;

  case FIFHITFROMLEFT:
    // This function proceeds if the character was attacked from the left
    returncode = bfalse;
    if (ChrList[character].directionlast >= LEFT - 8192 && ChrList[character].directionlast < LEFT + 8192)
      returncode = btrue;
    break;

  case FIFHITFROMRIGHT:
    // This function proceeds if the character was attacked from the right
    returncode = bfalse;
    if (ChrList[character].directionlast >= RIGHT - 8192 && ChrList[character].directionlast < RIGHT + 8192)
      returncode = btrue;
    break;

  case FIFTARGETISONSAMETEAM:
    // This function proceeds only if the target is on another team
    returncode = bfalse;
    if (ChrList[ChrList[character].aitarget].team == ChrList[character].team)
      returncode = btrue;
    break;

  case FKILLTARGET:
    // This function kills the target
    kill_character(gs, ChrList[character].aitarget, character, &scr_randie);
    break;

  case FUNDOENCHANT:
    // This function undoes the last enchant
    returncode = (ChrList[character].undoenchant != MAXENCHANT);
    remove_enchant(gs, ChrList[character].undoenchant, &scr_randie);
    break;

  case FGETWATERLEVEL:
    // This function gets the douse level for the water, returning it in tmpargument
    valuetmpargument = waterdouselevel * 10;
    break;

  case FCOSTTARGETMANA:
    // This function costs the target some mana
    returncode = cost_mana(gs, ChrList[character].aitarget, valuetmpargument, character, &scr_randie);
    break;

  case FIFTARGETHASANYID:
    // This function proceeds only if one of the target's IDSZ's matches tmpargument
    returncode = 0;
    tTmp = 0;
    while (tTmp < MAXIDSZ)
    {
      returncode |= (CapList[ChrList[ChrList[character].aitarget].model].idsz[tTmp] == (Uint32) valuetmpargument);
      tTmp++;
    }
    break;

  case FSETBUMPSIZE:
    // This function sets the character's bump size
    fTmp = ChrList[character].bumpsizebig;
    fTmp = fTmp / ChrList[character].bumpsize;  // 1.5 or 2.0
    ChrList[character].bumpsize = valuetmpargument * ChrList[character].fat;
    ChrList[character].bumpsizebig = fTmp * ChrList[character].bumpsize;
    ChrList[character].bumpsizesave = valuetmpargument;
    ChrList[character].bumpsizebigsave = fTmp * ChrList[character].bumpsizesave;
    break;

  case FIFNOTDROPPED:
    // This function passes if a kursed item could not be dropped
    returncode = ((ChrList[character].alert & ALERTIFNOTDROPPED) != 0);
    break;

  case FIFYISLESSTHANX:
    // This function passes only if tmpy is less than tmpx
    returncode = (valuetmpy < valuetmpx);
    break;

  case FSETFLYHEIGHT:
    // This function sets a character's fly height
    ChrList[character].flyheight = valuetmpargument;
    break;

  case FIFBLOCKED:
    // This function passes if the character blocked an attack
    returncode = ((ChrList[character].alert & ALERTIFBLOCKED) != 0);
    break;

  case FIFTARGETISDEFENDING:
    returncode = (ChrList[ChrList[character].aitarget].action >= ACTIONPA && ChrList[ChrList[character].aitarget].action <= ACTIONPD);
    break;

  case FIFTARGETISATTACKING:
    returncode = (ChrList[ChrList[character].aitarget].action >= ACTIONUA && ChrList[ChrList[character].aitarget].action <= ACTIONFD);
    break;

  case FIFSTATEIS0:
    returncode = (0 == ChrList[character].aistate);
    break;

  case FIFSTATEIS1:
    returncode = (1 == ChrList[character].aistate);
    break;

  case FIFSTATEIS2:
    returncode = (2 == ChrList[character].aistate);
    break;

  case FIFSTATEIS3:
    returncode = (3 == ChrList[character].aistate);
    break;

  case FIFSTATEIS4:
    returncode = (4 == ChrList[character].aistate);
    break;

  case FIFSTATEIS5:
    returncode = (5 == ChrList[character].aistate);
    break;

  case FIFSTATEIS6:
    returncode = (6 == ChrList[character].aistate);
    break;

  case FIFSTATEIS7:
    returncode = (7 == ChrList[character].aistate);
    break;

  case FIFCONTENTIS:
    returncode = (valuetmpargument == ChrList[character].aicontent);
    break;

  case FSETTURNMODETOWATCHTARGET:
    // This function sets the turn mode
    ChrList[character].turnmode = TURNMODEWATCHTARGET;
    break;

  case FIFSTATEISNOT:
    returncode = (valuetmpargument != ChrList[character].aistate);
    break;

  case FIFXISEQUALTOY:
    returncode = (valuetmpx == valuetmpy);
    break;

  case FDEBUGMESSAGE:
    // This function spits out a debug message
    debug_message("aistate %d, aicontent %d, target %d", ChrList[character].aistate, ChrList[character].aicontent, ChrList[character].aitarget);
    debug_message("tmpx %d, tmpy %d", valuetmpx, valuetmpy);
    debug_message("tmpdistance %d, tmpturn %d", valuetmpdistance, valuetmpturn);
    debug_message("tmpargument %d, selfturn %d", valuetmpargument, ChrList[character].turnleftright);
    break;

  case FBLACKTARGET:
    // This function makes the target flash black
    flash_character(ChrList[character].aitarget, 0);
    break;

  case FSENDMESSAGENEAR:
    // This function sends a message if the camera is in the nearby area.
    iTmp = ABS(ChrList[character].oldx - GCamera.trackx) + ABS(ChrList[character].oldy - GCamera.tracky);
    if (iTmp < MSGDISTANCE)
      display_message(MadList[ChrList[character].model].msg_start + valuetmpargument, character);
    break;

  case FIFHITGROUND:
    // This function passes if the character just hit the ground
    returncode = ((ChrList[character].alert & ALERTIFHITGROUND) != 0);
    break;

  case FIFNAMEISKNOWN:
    // This function passes if the character's name is known
    returncode = ChrList[character].nameknown;
    break;

  case FIFUSAGEISKNOWN:
    // This function passes if the character's usage is known
    returncode = CapList[ChrList[character].model].usageknown;
    break;

  case FIFHOLDINGITEMID:
    // This function passes if the character is holding an item with the IDSZ given
    // in tmpargument, returning the latch to press to use it
    returncode = bfalse;
    // Check left hand
    sTmp = ChrList[character].holdingwhich[0];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
      {
        valuetmpargument = LATCHBUTTONLEFT;
        returncode = btrue;
      }
    }
    // Check right hand
    sTmp = ChrList[character].holdingwhich[1];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[sTmp].idsz[IDSZTYPE] == (Uint32) valuetmpargument)
      {
        valuetmpargument = LATCHBUTTONRIGHT;
        if (returncode == btrue)  valuetmpargument = LATCHBUTTONLEFT + (ego_rand(&ego_rand_seed) & 1);
        returncode = btrue;
      }
    }
    break;

  case FIFHOLDINGRANGEDWEAPON:
    // This function passes if the character is holding a ranged weapon, returning
    // the latch to press to use it.  This also checks ammo/ammoknown.
    returncode = bfalse;
    valuetmpargument = 0;
    // Check left hand
    tTmp = ChrList[character].holdingwhich[0];
    if (tTmp != MAXCHR)
    {
      sTmp = ChrList[tTmp].model;
      if (CapList[sTmp].isranged && (ChrList[tTmp].ammomax == 0 || (ChrList[tTmp].ammo != 0 && ChrList[tTmp].ammoknown)))
      {
        valuetmpargument = LATCHBUTTONLEFT;
        returncode = btrue;
      }
    }
    // Check right hand
    tTmp = ChrList[character].holdingwhich[1];
    if (tTmp != MAXCHR)
    {
      sTmp = ChrList[tTmp].model;
      if (CapList[sTmp].isranged && (ChrList[tTmp].ammomax == 0 || (ChrList[tTmp].ammo != 0 && ChrList[tTmp].ammoknown)))
      {
        if (valuetmpargument == 0 || (allframe&1))
        {
          valuetmpargument = LATCHBUTTONRIGHT;
          returncode = btrue;
        }
      }
    }
    break;

  case FIFHOLDINGMELEEWEAPON:
    // This function passes if the character is holding a melee weapon, returning
    // the latch to press to use it
    returncode = bfalse;
    valuetmpargument = 0;
    // Check left hand
    sTmp = ChrList[character].holdingwhich[0];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].isranged == bfalse && CapList[sTmp].weaponaction != ACTIONPA)
      {
        valuetmpargument = LATCHBUTTONLEFT;
        returncode = btrue;
      }
    }
    // Check right hand
    sTmp = ChrList[character].holdingwhich[1];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].isranged == bfalse && CapList[sTmp].weaponaction != ACTIONPA)
      {
        if (valuetmpargument == 0 || (allframe&1))
        {
          valuetmpargument = LATCHBUTTONRIGHT;
          returncode = btrue;
        }
      }
    }
    break;

  case FIFHOLDINGSHIELD:
    // This function passes if the character is holding a shield, returning the
    // latch to press to use it
    returncode = bfalse;
    valuetmpargument = 0;
    // Check left hand
    sTmp = ChrList[character].holdingwhich[0];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].weaponaction == ACTIONPA)
      {
        valuetmpargument = LATCHBUTTONLEFT;
        returncode = btrue;
      }
    }
    // Check right hand
    sTmp = ChrList[character].holdingwhich[1];
    if (sTmp != MAXCHR)
    {
      sTmp = ChrList[sTmp].model;
      if (CapList[sTmp].weaponaction == ACTIONPA)
      {
        valuetmpargument = LATCHBUTTONRIGHT;
        returncode = btrue;
      }
    }
    break;

  case FIFKURSED:
    // This function passes if the character is kursed
    returncode = ChrList[character].iskursed;
    break;

  case FIFTARGETISKURSED:
    // This function passes if the target is kursed
    returncode = ChrList[ChrList[character].aitarget].iskursed;
    break;

  case FIFTARGETISDRESSEDUP:
    // This function passes if the character's skin is dressy
    iTmp = (ChrList[character].texture - MadList[ChrList[character].model].skinstart) % MAXSKIN;
    iTmp = 1 << iTmp;
    returncode = ((CapList[ChrList[character].model].skindressy & iTmp) != 0);
    break;

  case FIFOVERWATER:
    // This function passes if the character is on a water tile
    returncode = ((Mesh.fanlist[ChrList[character].onwhichfan].fx & MESHFXWATER) != 0 && wateriswater);
    break;

  case FIFTHROWN:
    // This function passes if the character was thrown
    returncode = ((ChrList[character].alert & ALERTIFTHROWN) != 0);
    break;

  case FMAKENAMEKNOWN:
    // This function makes the name of an item/character known.
    ChrList[character].nameknown = btrue;
//            ChrList[character].icon = btrue;
    break;

  case FMAKEUSAGEKNOWN:
    // This function makes the usage of an item known...  For XP gains from
    // using an unknown potion or such
    CapList[ChrList[character].model].usageknown = btrue;
    break;

  case FSTOPTARGETMOVEMENT:
    // This function makes the target stop moving temporarily
    ChrList[ChrList[character].aitarget].xvel = 0;
    ChrList[ChrList[character].aitarget].yvel = 0;
    if (ChrList[ChrList[character].aitarget].zvel > 0) ChrList[ChrList[character].aitarget].zvel = gravity;
    break;

  case FSETXY:
    // This function stores tmpx and tmpy in the storage array
    ChrList[character].aix[valuetmpargument&STORAND] = valuetmpx;
    ChrList[character].aiy[valuetmpargument&STORAND] = valuetmpy;
    break;

  case FGETXY:
    // This function gets previously stored data, setting tmpx and tmpy
    valuetmpx = ChrList[character].aix[valuetmpargument&STORAND];
    valuetmpy = ChrList[character].aiy[valuetmpargument&STORAND];
    break;

  case FADDXY:
    // This function adds tmpx and tmpy to the storage array
    ChrList[character].aix[valuetmpargument&STORAND] += valuetmpx;
    ChrList[character].aiy[valuetmpargument&STORAND] += valuetmpy;
    break;

  case FMAKEAMMOKNOWN:
    // This function makes the ammo of an item/character known.
    ChrList[character].ammoknown = btrue;
    break;

  case FSPAWNATTACHEDPARTICLE:
    // This function spawns an attached particle
    tTmp = character;
    if (ChrList[character].attachedto != MAXCHR)  tTmp = ChrList[character].attachedto;
    tTmp = spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, ChrList[character].turnleftright, ChrList[character].model, valuetmpargument, character, valuetmpdistance, ChrList[character].team, tTmp, 0, MAXCHR, &scr_randie);
    break;

  case FSPAWNEXACTPARTICLE:
    // This function spawns an exactly placed particle
    tTmp = character;
    if (ChrList[character].attachedto != MAXCHR)  tTmp = ChrList[character].attachedto;
    spawn_one_particle(valuetmpx, valuetmpy, valuetmpdistance, ChrList[character].turnleftright, ChrList[character].model, valuetmpargument, MAXCHR, 0, ChrList[character].team, tTmp, 0, MAXCHR, &scr_randie);
    break;

  case FACCELERATETARGET:
    // This function changes the target's speeds
    ChrList[ChrList[character].aitarget].xvel += valuetmpx;
    ChrList[ChrList[character].aitarget].yvel += valuetmpy;
    break;

  case FIFDISTANCEISMORETHANTURN:
    // This function proceeds tmpdistance is greater than tmpturn
    returncode = (valuetmpdistance > (int) valuetmpturn);
    break;

  case FIFCRUSHED:
    // This function proceeds only if the character was crushed
    returncode = ((ChrList[character].alert & ALERTIFCRUSHED) != 0);
    break;

  case FMAKECRUSHVALID:
    // This function makes doors able to close on this object
    ChrList[character].canbecrushed = btrue;
    break;

  case FSETTARGETTOLOWESTTARGET:
    // This sets the target to whatever the target is being held by,
    // The lowest in the set.  This function never fails
    while (ChrList[ChrList[character].aitarget].attachedto != MAXCHR)
    {
      ChrList[character].aitarget = ChrList[ChrList[character].aitarget].attachedto;
    }
    break;

  case FIFNOTPUTAWAY:
    // This function proceeds only if the character couln't be put in the pack
    returncode = ((ChrList[character].alert & ALERTIFNOTPUTAWAY) != 0);
    break;

  case FIFTAKENOUT:
    // This function proceeds only if the character was taken out of the pack
    returncode = ((ChrList[character].alert & ALERTIFTAKENOUT) != 0);
    break;

  case FIFAMMOOUT:
    // This function proceeds only if the character has no ammo
    returncode = (ChrList[character].ammo == 0);
    break;

  case FPLAYSOUNDLOOPED:
    // This function plays a looped sound
    if (gs->moduleActive)
    {
      //You could use this, but right now there's no way to stop the sound later, so it's better not to start it
      //play_sound_pvf_looped(CapList[ChrList[character].model].waveindex[valuetmpargument], PANMID, volume, valuetmpdistance);
    }
    break;

  case FSTOPSOUND:
    //TODO: implement this (the scripter doesn't know which channel to stop)
    // This function stops playing a sound
    //stop_sound([valuetmpargument]);
    break;

  case FHEALSELF:
    // This function heals the character, without setting the alert or modifying
    // the amount
    if (ChrList[character].alive)
    {
      iTmp = ChrList[character].life + valuetmpargument;
      if (iTmp > ChrList[character].lifemax) iTmp = ChrList[character].lifemax;
      if (iTmp < 1) iTmp = 1;
      ChrList[character].life = iTmp;
    }
    break;

  case FEQUIP:
    // This function flags the character as being equipped
    ChrList[character].isequipped = btrue;
    break;

  case FIFTARGETHASITEMIDEQUIPPED:
    // This function proceeds if the target has a matching item equipped
    returncode = bfalse;
    sTmp = ChrList[ChrList[character].aitarget].nextinpack;
    while (sTmp != MAXCHR)
    {
      if (sTmp != character && ChrList[sTmp].isequipped && (CapList[ChrList[sTmp].model].idsz[IDSZPARENT] == (Uint32) valuetmpargument || CapList[ChrList[sTmp].model].idsz[IDSZTYPE] == (Uint32) valuetmpargument))
      {
        returncode = btrue;
        sTmp = MAXCHR;
      }
      else
      {
        sTmp = ChrList[sTmp].nextinpack;
      }
    }
    break;

  case FSETOWNERTOTARGET:
    // This function sets the owner
    ChrList[character].aiowner = ChrList[character].aitarget;
    break;

  case FSETTARGETTOOWNER:
    // This function sets the target to the owner
    ChrList[character].aitarget = ChrList[character].aiowner;
    break;

  case FSETFRAME:
    // This function sets the character's current frame
    sTmp = valuetmpargument & 3;
    iTmp = valuetmpargument >> 2;
    set_frame(character, iTmp, sTmp);
    break;

  case FBREAKPASSAGE:
    // This function makes the tiles fall away ( turns into damage terrain )
    returncode = break_passage(valuetmpargument, valuetmpturn, valuetmpdistance, valuetmpx, valuetmpy);
    break;

  case FSETRELOADTIME:
    // This function makes weapons fire slower
    ChrList[character].reloadtime = valuetmpargument;
    break;

  case FSETTARGETTOWIDEBLAHID:
    // This function sets the target based on the settings of
    // tmpargument and tmpdistance
    sTmp = get_wide_target(character, ((valuetmpdistance >> 3) & 1),
                           ((valuetmpdistance >> 2) & 1),
                           ((valuetmpdistance >> 1) & 1),
                           ((valuetmpdistance) & 1),
                           valuetmpargument, ((valuetmpdistance >> 4) & 1));
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FPOOFTARGET:
    // This function makes the target go away
    returncode = bfalse;
    if (ChrList[ChrList[character].aitarget].isplayer == bfalse)
    {
      returncode = btrue;
      if (ChrList[character].aitarget == character)
      {
        // Poof self later
        valuegopoof = btrue;
      }
      else
      {
        // Poof others now
        if (ChrList[ChrList[character].aitarget].attachedto != MAXCHR)
          detach_character_from_mount(gs, ChrList[character].aitarget, btrue, bfalse, &scr_randie);
        if (ChrList[ChrList[character].aitarget].holdingwhich[0] != MAXCHR)
          detach_character_from_mount(gs, ChrList[ChrList[character].aitarget].holdingwhich[0], btrue, bfalse, &scr_randie);
        if (ChrList[ChrList[character].aitarget].holdingwhich[1] != MAXCHR)
          detach_character_from_mount(gs, ChrList[ChrList[character].aitarget].holdingwhich[1], btrue, bfalse, &scr_randie);
        free_inventory(ChrList[character].aitarget);
        free_one_character(ChrList[character].aitarget);
        ChrList[character].aitarget = character;
      }
    }
    break;

  case FCHILDDOACTIONOVERRIDE:
    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid
    returncode = bfalse;
    if (valuetmpargument < MAXACTION)
    {
      if (MadList[ChrList[ChrList[character].aichild].model].actionvalid[valuetmpargument])
      {
        ChrList[ChrList[character].aichild].action = valuetmpargument;
        ChrList[ChrList[character].aichild].lip = 0;
        ChrList[ChrList[character].aichild].frame = MadList[ChrList[ChrList[character].aichild].model].actionstart[valuetmpargument];
        ChrList[ChrList[character].aichild].lastframe = ChrList[ChrList[character].aichild].frame;
        ChrList[ChrList[character].aichild].actionready = bfalse;
        returncode = btrue;
      }
    }
    break;

  case FSPAWNPOOF:
    // This function makes a lovely little poof at the character's location
    spawn_poof(character, ChrList[character].model, &scr_randie);
    break;

  case FSETSPEEDPERCENT:
    reset_character_accel(character);
    ChrList[character].maxaccel = ChrList[character].maxaccel * valuetmpargument / 100.0;
    break;

  case FSETCHILDSTATE:
    // This function sets the child's state
    ChrList[ChrList[character].aichild].aistate = valuetmpargument;
    break;

  case FSPAWNATTACHEDSIZEDPARTICLE:
    // This function spawns an attached particle, then sets its size
    tTmp = character;
    if (ChrList[character].attachedto != MAXCHR)  tTmp = ChrList[character].attachedto;
    tTmp = spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, ChrList[character].turnleftright, ChrList[character].model, valuetmpargument, character, valuetmpdistance, ChrList[character].team, tTmp, 0, MAXCHR, &scr_randie);
    if (tTmp < MAXPRT)
    {
      PrtList[tTmp].size = valuetmpturn;
    }
    break;

  case FCHANGEARMOR:
    // This function sets the character's armor type and returns the old type
    // as tmpargument and the new type as tmpx
    valuetmpx = valuetmpargument;
    iTmp = (ChrList[character].texture - MadList[ChrList[character].model].skinstart) % MAXSKIN;
    valuetmpx = change_armor(gs, character, valuetmpargument, &scr_randie);
    valuetmpargument = iTmp;  // The character's old armor
    break;

  case FSHOWTIMER:
    // This function turns the timer on, using the value for tmpargument
    timeron = btrue;
    timervalue = valuetmpargument;
    break;

  case FIFFACINGTARGET:
    // This function proceeds only if the character is facing the target
    sTmp = atan2(ChrList[ChrList[character].aitarget].ypos - ChrList[character].ypos, ChrList[ChrList[character].aitarget].xpos - ChrList[character].xpos) * RAD_TO_SHORT;
    sTmp += 32768 - ChrList[character].turnleftright;
    returncode = (sTmp > 55535 || sTmp < 10000);
    break;

  case FPLAYSOUNDVOLUME:
    // This function sets the volume of a sound and plays it
    if (gs->moduleActive && valuetmpdistance >= 0)
    {
      volume = valuetmpdistance;
      iTmp = play_sound(ChrList[character].oldx, ChrList[character].oldy, CapList[ChrList[character].model].waveindex[valuetmpargument]);
      if (iTmp != -1) Mix_Volume(iTmp, valuetmpdistance);
    }
    break;

  case FSPAWNATTACHEDFACEDPARTICLE:
    // This function spawns an attached particle with facing
    tTmp = character;
    if (ChrList[character].attachedto != MAXCHR)  tTmp = ChrList[character].attachedto;
    tTmp = spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, valuetmpturn, ChrList[character].model, valuetmpargument, character, valuetmpdistance, ChrList[character].team, tTmp, 0, MAXCHR, &scr_randie);
    break;

  case FIFSTATEISODD:
    returncode = (ChrList[character].aistate & 1);
    break;

  case FSETTARGETTODISTANTENEMY:
    // This function finds an enemy, within a certain distance to the character, and
    // proceeds only if there is one
    sTmp = find_distant_target(character, valuetmpdistance);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FTELEPORT:
    // This function teleports the character to the X, Y location, failing if the
    // location is off the map or blocked
    returncode = bfalse;
    if (valuetmpx > EDGE && valuetmpy > EDGE && valuetmpx < Mesh.edgex - EDGE && valuetmpy < Mesh.edgey - EDGE)
    {
      // Yeah!  It worked!
      detach_character_from_mount(gs, character, btrue, bfalse, &scr_randie);
      ChrList[character].oldx = ChrList[character].xpos;
      ChrList[character].oldy = ChrList[character].ypos;
      ChrList[character].xpos = valuetmpx;
      ChrList[character].ypos = valuetmpy;
      if (__chrhitawall(character))
      {
        // No it didn't...
        ChrList[character].xpos = ChrList[character].oldx;
        ChrList[character].ypos = ChrList[character].oldy;
        returncode = bfalse;
      }
      else
      {
        ChrList[character].oldx = ChrList[character].xpos;
        ChrList[character].oldy = ChrList[character].ypos;
        returncode = btrue;
      }
    }
    break;

  case FGIVESTRENGTHTOTARGET:
    // Permanently boost the target's strength
    if (ChrList[ChrList[character].aitarget].alive)
    {
      iTmp = valuetmpargument;
      getadd(0, ChrList[ChrList[character].aitarget].strength, PERFECTSTAT, &iTmp);
      ChrList[ChrList[character].aitarget].strength += iTmp;
    }
    break;

  case FGIVEWISDOMTOTARGET:
    // Permanently boost the target's wisdom
    if (ChrList[ChrList[character].aitarget].alive)
    {
      iTmp = valuetmpargument;
      getadd(0, ChrList[ChrList[character].aitarget].wisdom, PERFECTSTAT, &iTmp);
      ChrList[ChrList[character].aitarget].wisdom += iTmp;
    }
    break;

  case FGIVEINTELLIGENCETOTARGET:
    // Permanently boost the target's intelligence
    if (ChrList[ChrList[character].aitarget].alive)
    {
      iTmp = valuetmpargument;
      getadd(0, ChrList[ChrList[character].aitarget].intelligence, PERFECTSTAT, &iTmp);
      ChrList[ChrList[character].aitarget].intelligence += iTmp;
    }
    break;

  case FGIVEDEXTERITYTOTARGET:
    // Permanently boost the target's dexterity
    if (ChrList[ChrList[character].aitarget].alive)
    {
      iTmp = valuetmpargument;
      getadd(0, ChrList[ChrList[character].aitarget].dexterity, PERFECTSTAT, &iTmp);
      ChrList[ChrList[character].aitarget].dexterity += iTmp;
    }
    break;

  case FGIVELIFETOTARGET:
    // Permanently boost the target's life
    if (ChrList[ChrList[character].aitarget].alive)
    {
      iTmp = valuetmpargument;
      getadd(LOWSTAT, ChrList[ChrList[character].aitarget].lifemax, PERFECTBIG, &iTmp);
      ChrList[ChrList[character].aitarget].lifemax += iTmp;
      if (iTmp < 0)
      {
        getadd(1, ChrList[ChrList[character].aitarget].life, PERFECTBIG, &iTmp);
      }
      ChrList[ChrList[character].aitarget].life += iTmp;
    }
    break;

  case FGIVEMANATOTARGET:
    // Permanently boost the target's mana
    if (ChrList[ChrList[character].aitarget].alive)
    {
      iTmp = valuetmpargument;
      getadd(0, ChrList[ChrList[character].aitarget].manamax, PERFECTBIG, &iTmp);
      ChrList[ChrList[character].aitarget].manamax += iTmp;
      if (iTmp < 0)
      {
        getadd(0, ChrList[ChrList[character].aitarget].mana, PERFECTBIG, &iTmp);
      }
      ChrList[ChrList[character].aitarget].mana += iTmp;
    }
    break;

  case FSHOWMAP:
    // Show the map...  Fails if map already visible
    if (mapon)  returncode = bfalse;
    mapon = btrue;
    break;

  case FSHOWYOUAREHERE:
    // Show the camera target location
    youarehereon = btrue;
    break;

  case FSHOWBLIPXY:
    // Add a blip
    if (numblip < MAXBLIP)
    {
      if (valuetmpx > 0 && valuetmpx < Mesh.edgex && valuetmpy > 0 && valuetmpy < Mesh.edgey)
      {
        if (valuetmpargument < NUMBAR && valuetmpargument >= 0)
        {
          BlipList[numblip].x = valuetmpx * MAPSIZE / Mesh.edgex;
          BlipList[numblip].y = valuetmpy * MAPSIZE / Mesh.edgey;
          BlipList[numblip].c = valuetmpargument;
          numblip++;
        }
      }
    }
    break;

  case FHEALTARGET:
    // Give some life to the target
    if (ChrList[ChrList[character].aitarget].alive)
    {
      iTmp = valuetmpargument;
      getadd(1, ChrList[ChrList[character].aitarget].life, ChrList[ChrList[character].aitarget].lifemax, &iTmp);
      ChrList[ChrList[character].aitarget].life += iTmp;
      // Check all enchants to see if they are removed
      iTmp = ChrList[ChrList[character].aitarget].firstenchant;
      while (iTmp != MAXENCHANT)
      {
        sTmp = EncList[iTmp].nextenchant;
        if (MAKE_IDSZ("HEAL") == EveList[EncList[iTmp].eve].removedbyidsz)
        {
          remove_enchant(gs, iTmp, &scr_randie);
        }
        iTmp = sTmp;
      }
    }
    break;

  case FPUMPTARGET:
    // Give some mana to the target
    if (ChrList[ChrList[character].aitarget].alive)
    {
      iTmp = valuetmpargument;
      getadd(0, ChrList[ChrList[character].aitarget].mana, ChrList[ChrList[character].aitarget].manamax, &iTmp);
      ChrList[ChrList[character].aitarget].mana += iTmp;
    }
    break;

  case FCOSTAMMO:
    // Take away one ammo
    if (ChrList[character].ammo > 0)
    {
      ChrList[character].ammo--;
    }
    break;

  case FMAKESIMILARNAMESKNOWN:
    // Make names of matching objects known
    iTmp = 0;
    while (iTmp < MAXCHR)
    {
      sTmp = btrue;
      tTmp = 0;
      while (tTmp < MAXIDSZ)
      {
        if (CapList[ChrList[character].model].idsz[tTmp] != CapList[ChrList[iTmp].model].idsz[tTmp])
        {
          sTmp = bfalse;
        }
        tTmp++;
      }
      if (sTmp)
      {
        ChrList[iTmp].nameknown = btrue;
      }
      iTmp++;
    }
    break;

  case FSPAWNATTACHEDHOLDERPARTICLE:
    // This function spawns an attached particle, attached to the holder
    tTmp = character;
    if (ChrList[character].attachedto != MAXCHR)  tTmp = ChrList[character].attachedto;
    tTmp = spawn_one_particle(ChrList[character].xpos, ChrList[character].ypos, ChrList[character].zpos, ChrList[character].turnleftright, ChrList[character].model, valuetmpargument, tTmp, valuetmpdistance, ChrList[character].team, tTmp, 0, MAXCHR, &scr_randie);
    break;

  case FSETTARGETRELOADTIME:
    // This function sets the target's reload time
    ChrList[ChrList[character].aitarget].reloadtime = valuetmpargument;
    break;

  case FSETFOGLEVEL:
    // This function raises and lowers the module's fog
    fTmp = (valuetmpargument / 10.0) - GFog.top;
    GFog.top += fTmp;
    GFog.distance += fTmp;
    GFog.on = gs->cd->fogallowed;
    if (GFog.distance < 1.0)  GFog.on = bfalse;
    break;

  case FGETFOGLEVEL:
    // This function gets the fog level
    valuetmpargument = GFog.top * 10;
    break;

  case FSETFOGTAD:
    // This function changes the fog color
    GFog.red = valuetmpturn;
    GFog.grn = valuetmpargument;
    GFog.blu = valuetmpdistance;
    break;

  case FSETFOGBOTTOMLEVEL:
    // This function sets the module's bottom fog level...
    fTmp = (valuetmpargument / 10.0) - GFog.bottom;
    GFog.bottom += fTmp;
    GFog.distance -= fTmp;
    GFog.on = gs->cd->fogallowed;
    if (GFog.distance < 1.0)  GFog.on = bfalse;
    break;

  case FGETFOGBOTTOMLEVEL:
    // This function gets the fog level
    valuetmpargument = GFog.bottom * 10;
    break;

  case FCORRECTACTIONFORHAND:
    // This function turns ZA into ZA, ZB, ZC, or ZD...
    // tmpargument must be set to one of the A actions beforehand...
    if (ChrList[character].attachedto != MAXCHR)
    {
      if (ChrList[character].inwhichhand == GRIPLEFT)
      {
        // A or B
        valuetmpargument = valuetmpargument + (ego_rand(&ego_rand_seed) & 1);
      }
      else
      {
        // C or D
        valuetmpargument = valuetmpargument + 2 + (ego_rand(&ego_rand_seed) & 1);
      }
    }
    break;

  case FIFTARGETISMOUNTED:
    // This function proceeds if the target is riding a mount
    returncode = bfalse;
    if (ChrList[ChrList[character].aitarget].attachedto != MAXCHR)
    {
      returncode = ChrList[ChrList[ChrList[character].aitarget].attachedto].ismount;
    }
    break;

  case FSPARKLEICON:
    // This function makes a blippie thing go around the icon
    if (valuetmpargument < NUMBAR && valuetmpargument > -1)
    {
      ChrList[character].sparkle = valuetmpargument;
    }
    break;

  case FUNSPARKLEICON:
    // This function stops the blippie thing
    ChrList[character].sparkle = NOSPARKLE;
    break;

  case FGETTILEXY:
    // This function gets the tile at x,y
    if (valuetmpx >= 0 && valuetmpx < Mesh.edgex)
    {
      if (valuetmpy >= 0 && valuetmpy < Mesh.edgey)
      {
        iTmp = Mesh.fanstart[valuetmpy>>7] + (valuetmpx >> 7);
        valuetmpargument = Mesh.fanlist[iTmp].tile & 255;
      }
    }
    break;

  case FSETTILEXY:
    // This function changes the tile at x,y
    if (valuetmpx >= 0 && valuetmpx < Mesh.edgex)
    {
      if (valuetmpy >= 0 && valuetmpy < Mesh.edgey)
      {
        iTmp = Mesh.fanstart[valuetmpy>>7] + (valuetmpx >> 7);
        Mesh.fanlist[iTmp].tile = (valuetmpargument & 255);
      }
    }
    break;

  case FSETSHADOWSIZE:
    // This function changes a character's shadow size
    ChrList[character].shadowsize = valuetmpargument * ChrList[character].fat;
    ChrList[character].shadowsizesave = valuetmpargument;
    break;

  case FORDERTARGET:
    // This function GOrder.s one specific character...  The target
    // Be careful in using this, always checking IDSZ first
    ChrList[ChrList[character].aitarget].order = valuetmpargument;
    ChrList[ChrList[character].aitarget].counter = 0;
    ChrList[ChrList[character].aitarget].alert |= ALERTIFORDERED;
    break;

  case FSETTARGETTOWHOEVERISINPASSAGE:
    // This function lets passage rectangles be used as event triggers
    sTmp = who_is_blocking_passage(valuetmpargument);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FIFCHARACTERWASABOOK:
    // This function proceeds if the base model is the same as the current
    // model or if the base model is SPELLBOOK
    returncode = (ChrList[character].basemodel == SPELLBOOK ||
                  ChrList[character].basemodel == ChrList[character].model);
    break;

  case FSETENCHANTBOOSTVALUES:
    // This function sets the boost values for the last enchantment
    iTmp = ChrList[character].undoenchant;
    if (iTmp != MAXENCHANT)
    {
      EncList[iTmp].ownermana = valuetmpargument;
      EncList[iTmp].ownerlife = valuetmpdistance;
      EncList[iTmp].targetmana = valuetmpx;
      EncList[iTmp].targetlife = valuetmpy;
    }
    break;

  case FSPAWNCHARACTERXYZ:
    // This function spawns a character, failing if x,y,z is invalid
    sTmp = spawn_one_character(valuetmpx, valuetmpy, valuetmpdistance, ChrList[character].model, ChrList[character].team, 0, valuetmpturn, NULL, MAXCHR, &scr_randie);
    returncode = bfalse;
    if (sTmp < MAXCHR)
    {
      if (__chrhitawall(sTmp))
      {
        free_one_character(sTmp);
      }
      else
      {
        ChrList[sTmp].iskursed = bfalse;
        ChrList[character].aichild = sTmp;
        ChrList[sTmp].passage = ChrList[character].passage;
        ChrList[sTmp].aiowner = ChrList[character].aiowner;
        returncode = btrue;
      }
    }
    break;

  case FSPAWNEXACTCHARACTERXYZ:
    // This function spawns a character ( specific model slot ),
    // failing if x,y,z is invalid
    sTmp = spawn_one_character(valuetmpx, valuetmpy, valuetmpdistance, valuetmpargument, ChrList[character].team, 0, valuetmpturn, NULL, MAXCHR, &scr_randie);
    returncode = bfalse;
    if (sTmp < MAXCHR)
    {
      if (__chrhitawall(sTmp))
      {
        free_one_character(sTmp);
      }
      else
      {
        ChrList[sTmp].iskursed = bfalse;
        ChrList[character].aichild = sTmp;
        ChrList[sTmp].passage = ChrList[character].passage;
        ChrList[sTmp].aiowner = ChrList[character].aiowner;
        returncode = btrue;
      }
    }
    break;

  case FCHANGETARGETCLASS:
    // This function changes a character's model ( specific model slot )
    change_character(gs, ChrList[character].aitarget, valuetmpargument, 0, LEAVEALL, &scr_randie);
    break;

  case FPLAYFULLSOUND:
    // This function plays a sound loud for everyone...  Victory music
    if (gs->moduleActive)
    {
      play_sound(GCamera.trackx, GCamera.tracky, CapList[ChrList[character].model].waveindex[valuetmpargument]);
    }
    break;

  case FSPAWNEXACTCHASEPARTICLE:
    // This function spawns an exactly placed particle that chases the target
    tTmp = character;
    if (ChrList[character].attachedto != MAXCHR)  tTmp = ChrList[character].attachedto;
    tTmp = spawn_one_particle(valuetmpx, valuetmpy, valuetmpdistance, ChrList[character].turnleftright, ChrList[character].model, valuetmpargument, MAXCHR, 0, ChrList[character].team, tTmp, 0, MAXCHR, &scr_randie);
    if (tTmp < MAXPRT)
    {
      PrtList[tTmp].target = ChrList[character].aitarget;
    }
    break;

  case FCREATEORDER:
    // This function packs up an order, using tmpx, tmpy, tmpargument and the
    // target ( if valid ) to create a new tmpargument
    sTmp = ChrList[character].aitarget << 24;
    sTmp |= ((valuetmpx >> 6) & 1023) << 14;
    sTmp |= ((valuetmpy >> 6) & 1023) << 4;
    sTmp |= (valuetmpargument & 15);
    valuetmpargument = sTmp;
    break;

  case FORDERSPECIALID:
    // This function issues an order to all with the given special IDSZ
    issue_special_order(valuetmpargument, valuetmpdistance);
    break;

  case FUNKURSETARGETINVENTORY:
    // This function unkurses every item a character is holding
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[0];
    ChrList[sTmp].iskursed = bfalse;
    sTmp = ChrList[ChrList[character].aitarget].holdingwhich[1];
    ChrList[sTmp].iskursed = bfalse;
    sTmp = ChrList[ChrList[character].aitarget].nextinpack;
    while (sTmp != MAXCHR)
    {
      ChrList[sTmp].iskursed = bfalse;
      sTmp = ChrList[sTmp].nextinpack;
    }
    break;

  case FIFTARGETISSNEAKING:
    // This function proceeds if the target is doing ACTIONDA or ACTIONWA
    returncode = (ChrList[ChrList[character].aitarget].action == ACTIONDA || ChrList[ChrList[character].aitarget].action == ACTIONWA);
    break;

  case FDROPITEMS:
    // This function drops all of the character's items
    drop_all_items(gs, character, &scr_randie);
    break;

  case FRESPAWNTARGET:
    // This function respawns the target at its current location
    sTmp = ChrList[character].aitarget;
    ChrList[sTmp].oldx = ChrList[sTmp].xpos;
    ChrList[sTmp].oldy = ChrList[sTmp].ypos;
    ChrList[sTmp].oldz = ChrList[sTmp].zpos;
    respawn_character(sTmp, &scr_randie);
    ChrList[sTmp].xpos = ChrList[sTmp].oldx;
    ChrList[sTmp].ypos = ChrList[sTmp].oldy;
    ChrList[sTmp].zpos = ChrList[sTmp].oldz;
    break;

  case FTARGETDOACTIONSETFRAME:
    // This function starts a new action, if it is valid for the model and
    // sets the starting frame.  It will fail if the action is invalid
    returncode = bfalse;
    if (valuetmpargument < MAXACTION)
    {
      if (MadList[ChrList[ChrList[character].aitarget].model].actionvalid[valuetmpargument])
      {
        ChrList[ChrList[character].aitarget].action = valuetmpargument;
        ChrList[ChrList[character].aitarget].lip = 0;
        ChrList[ChrList[character].aitarget].frame = MadList[ChrList[ChrList[character].aitarget].model].actionstart[valuetmpargument];
        ChrList[ChrList[character].aitarget].lastframe = ChrList[ChrList[character].aitarget].frame;
        ChrList[ChrList[character].aitarget].actionready = bfalse;
        returncode = btrue;
      }
    }
    break;

  case FIFTARGETCANSEEINVISIBLE:
    // This function proceeds if the target can see invisible
    returncode = ChrList[ChrList[character].aitarget].canseeinvisible;
    break;

  case FSETTARGETTONEARESTBLAHID:
    // This function finds the nearest target that meets the
    // requirements
    sTmp = get_nearest_target(character, ((valuetmpdistance >> 3) & 1),
                              ((valuetmpdistance >> 2) & 1),
                              ((valuetmpdistance >> 1) & 1),
                              ((valuetmpdistance) & 1),
                              valuetmpargument);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FSETTARGETTONEARESTENEMY:
    // This function finds the nearest target that meets the
    // requirements
    sTmp = get_nearest_target(character, 0, 0, 1, 0, IDSZNONE);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FSETTARGETTONEARESTFRIEND:
    // This function finds the nearest target that meets the
    // requirements
    sTmp = get_nearest_target(character, 0, 1, 0, 0, IDSZNONE);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FSETTARGETTONEARESTLIFEFORM:
    // This function finds the nearest target that meets the
    // requirements
    sTmp = get_nearest_target(character, 0, 1, 1, 0, IDSZNONE);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FFLASHPASSAGE:
    // This function makes the passage light or dark...  For debug...
    flash_passage(valuetmpargument, valuetmpdistance);
    break;

  case FFINDTILEINPASSAGE:
    // This function finds the next tile in the passage, tmpx and tmpy are
    // required and set on return
    returncode = find_tile_in_passage(valuetmpargument, valuetmpdistance);
    break;

  case FIFHELDINLEFTHAND:
    // This function proceeds if the character is in the left hand of another
    // character
    returncode = bfalse;
    sTmp = ChrList[character].attachedto;
    if (sTmp != MAXCHR)
    {
      returncode = (ChrList[sTmp].holdingwhich[0] == character);
    }
    break;

  case FNOTANITEM:
    // This function makes the character a non-item character
    ChrList[character].isitem = bfalse;
    break;

  case FSETCHILDAMMO:
    // This function sets the child's ammo
    ChrList[ChrList[character].aichild].ammo = valuetmpargument;
    break;

  case FIFHITVULNERABLE:
    // This function proceeds if the character was hit by a weapon with the
    // correct vulnerability IDSZ...  [SILV] for Werewolves...
    returncode = ((ChrList[character].alert & ALERTIFHITVULNERABLE) != 0);
    break;

  case FIFTARGETISFLYING:
    // This function proceeds if the character target is flying
    returncode = (ChrList[ChrList[character].aitarget].flyheight > 0);
    break;

  case FIDENTIFYTARGET:
    // This function reveals the target's name, ammo, and usage
    // Proceeds if the target was unknown
    returncode = bfalse;
    sTmp = ChrList[character].aitarget;
    if (ChrList[sTmp].ammomax != 0)  ChrList[sTmp].ammoknown = btrue;
    if (ChrList[sTmp].name[0] != 'B' ||
        ChrList[sTmp].name[1] != 'l' ||
        ChrList[sTmp].name[2] != 'a' ||
        ChrList[sTmp].name[3] != 'h' ||
        ChrList[sTmp].name[4] != 0)
    {
      returncode = !ChrList[sTmp].nameknown;
      ChrList[sTmp].nameknown = btrue;
    }
    CapList[ChrList[sTmp].model].usageknown = btrue;
    break;

  case FBEATMODULE:
    // This function displays the Module Ended message
    gs->modstate.beat = btrue;
    break;

  case FENDMODULE:
    // This function presses the Escape key
    sdlkeybuffer[SDLK_ESCAPE] = 1;
    break;

  case FDISABLEEXPORT:
    // This function turns export off
    gs->modstate.exportvalid = bfalse;
    break;

  case FENABLEEXPORT:
    // This function turns export on
    gs->modstate.exportvalid = btrue;
    break;

  case FGETTARGETSTATE:
    // This function sets tmpargument to the state of the target
    valuetmpargument = ChrList[ChrList[character].aitarget].aistate;
    break;

  case FIFEQUIPPED:
    //This proceeds if the character is equipped
    returncode = bfalse;
    if (ChrList[character].isequipped == btrue) returncode = btrue;
    break;

  case FDROPTARGETMONEY:
    // This function drops some of the target's money
    drop_money(ChrList[character].aitarget, valuetmpargument, &scr_randie);
    break;

  case FGETTARGETCONTENT:
    //This sets tmpargument to the current target's content value
    valuetmpargument = ChrList[ChrList[character].aitarget].aicontent;
    break;

  case FDROPTARGETKEYS:
    //This function makes the target drops GKeyb.s in inventory (Not inhand)
    drop_keys(ChrList[character].aitarget);
    break;

  case FJOINTEAM:
    //This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)
    switch_team(character, valuetmpargument);
    break;

  case FTARGETJOINTEAM:
    //This makes the target join a team specified in tmpargument (A = 0, 23 = Z, etc.)
    switch_team(ChrList[character].aitarget, valuetmpargument);
    break;

  case FCLEARMUSICPASSAGE:
    //This clears the music for an specified passage
    passagemusic[valuetmpargument] = -1;
    break;

  case FCLEARENDMESSAGE:
    // This function empties the end-module text buffer
    endtext[0] = 0;
    endtextwrite = 0;
    break;

  case FADDENDMESSAGE:
    // This function appends a message to the end-module text buffer
    append_end_text(MadList[ChrList[character].model].msg_start + valuetmpargument, character);
    break;

  case FPLAYMUSIC:
    // This function begins playing a new track of music
    if (gs->cd->musicvalid && (songplaying != valuetmpargument))
    {
      play_music(valuetmpargument, valuetmpdistance, -1);
    }
    break;

  case FSETMUSICPASSAGE:
    // This function makes the given passage play music if a player enters it
    // tmpargument is the passage to set and tmpdistance is the music track to play...
    passagemusic[valuetmpargument] = valuetmpdistance;
    break;

  case FMAKECRUSHINVALID:
    // This function makes doors unable to close on this object
    ChrList[character].canbecrushed = bfalse;
    break;

  case FSTOPMUSIC:
    // This function stops the interactive music
    stop_music();
    break;

  case FFLASHVARIABLE:
    // This function makes the character flash according to tmpargument
    flash_character(character, valuetmpargument);
    break;

  case FACCELERATEUP:
    // This function changes the character's up down velocity
    ChrList[character].zvel += valuetmpargument / 100.0;
    break;

  case FFLASHVARIABLEHEIGHT:
    // This function makes the character flash, feet one color, head another...
    flash_character_height(character, valuetmpturn, valuetmpx,
                           valuetmpdistance, valuetmpy);
    break;

  case FSETDAMAGETIME:
    // This function makes the character invincible for a little while
    ChrList[character].damagetime = valuetmpargument;
    break;

  case FIFSTATEIS8:
    returncode = (8 == ChrList[character].aistate);
    break;

  case FIFSTATEIS9:
    returncode = (9 == ChrList[character].aistate);
    break;

  case FIFSTATEIS10:
    returncode = (10 == ChrList[character].aistate);
    break;

  case FIFSTATEIS11:
    returncode = (11 == ChrList[character].aistate);
    break;

  case FIFSTATEIS12:
    returncode = (12 == ChrList[character].aistate);
    break;

  case FIFSTATEIS13:
    returncode = (13 == ChrList[character].aistate);
    break;

  case FIFSTATEIS14:
    returncode = (14 == ChrList[character].aistate);
    break;

  case FIFSTATEIS15:
    returncode = (15 == ChrList[character].aistate);
    break;

  case FIFTARGETISAMOUNT:
    returncode = ChrList[ChrList[character].aitarget].ismount;
    break;

  case FIFTARGETISAPLATFORM:
    returncode = ChrList[ChrList[character].aitarget].platform;
    break;

  case FADDSTAT:
    if (ChrList[character].staton == bfalse) add_stat(character);
    break;

  case FDISENCHANTTARGET:
    returncode = (ChrList[ChrList[character].aitarget].firstenchant != MAXENCHANT);
    disenchant_character(gs, ChrList[character].aitarget, &scr_randie);
    break;

  case FDISENCHANTALL:
    iTmp = 0;
    while (iTmp < MAXENCHANT)
    {
      remove_enchant(gs, iTmp, &scr_randie);
      iTmp++;
    }
    break;

  case FSETVOLUMENEARESTTEAMMATE:
    /*PORT
                if(gs->moduleActive && valuetmpdistance >= 0)
                {
                    // Find the closest teammate
                    iTmp = 10000;
                    sTmp = 0;
                    while(sTmp < MAXCHR)
                    {
                        if(ChrList[sTmp].on && ChrList[sTmp].alive && ChrList[sTmp].team == ChrList[character].team)
                        {
                            distance = ABS(GCamera.trackx-ChrList[sTmp].oldx)+ABS(GCamera.tracky-ChrList[sTmp].oldy);
                            if(distance < iTmp)  iTmp = distance;
                        }
                        sTmp++;
                    }
                    distance=iTmp+valuetmpdistance;
                    volume = -distance;
                    volume = volume<<VOLSHIFT;
                    if(volume < VOLMIN) volume = VOLMIN;
                    iTmp = CapList[ChrList[character].model].waveindex[valuetmpargument];
                    if(iTmp < numsound && iTmp >= 0 && soundon)
                    {
                        lpDSBuffer[iTmp]->SetVolume(volume);
                    }
                }
    */
    break;

  case FADDSHOPPASSAGE:
    // This function defines a shop area
    add_shop_passage(character, valuetmpargument);
    break;

  case FTARGETPAYFORARMOR:
    // This function costs the target some money, or fails if 'e doesn't have
    // enough...
    // tmpx is amount needed
    // tmpy is cost of new skin
    sTmp = ChrList[character].aitarget;   // The target
    tTmp = ChrList[sTmp].model;           // The target's model
    iTmp =  CapList[tTmp].skincost[valuetmpargument % MAXSKIN];
    valuetmpy = iTmp;                // Cost of new skin
    iTmp -= CapList[tTmp].skincost[(ChrList[sTmp].texture - MadList[tTmp].skinstart) % MAXSKIN];  // Refund
    if (iTmp > ChrList[sTmp].money)
    {
      // Not enough...
      valuetmpx = iTmp - ChrList[sTmp].money;  // Amount needed
      returncode = bfalse;
    }
    else
    {
      // Pay for it...  Cost may be negative after refund...
      ChrList[sTmp].money -= iTmp;
      if (ChrList[sTmp].money > MAXMONEY)  ChrList[sTmp].money = MAXMONEY;
      valuetmpx = 0;
      returncode = btrue;
    }
    break;

  case FJOINEVILTEAM:
    // This function adds the character to the evil team...
    switch_team(character, EVILTEAM);
    break;

  case FJOINNULLTEAM:
    // This function adds the character to the null team...
    switch_team(character, NULLTEAM);
    break;

  case FJOINGOODTEAM:
    // This function adds the character to the good team...
    switch_team(character, GOODTEAM);
    break;

  case FPITSKILL:
    // This function activates pit deaths...
    pitskill = btrue;
    break;

  case FSETTARGETTOPASSAGEID:
    // This function finds a character who is both in the passage and who has
    // an item with the given IDSZ
    sTmp = who_is_blocking_passage_ID(valuetmpargument, valuetmpdistance);
    returncode = bfalse;
    if (sTmp != MAXCHR)
    {
      ChrList[character].aitarget = sTmp;
      returncode = btrue;
    }
    break;

  case FMAKENAMEUNKNOWN:
    // This function makes the name of an item/character unknown.
    ChrList[character].nameknown = bfalse;
    break;

  case FSPAWNEXACTPARTICLEENDSPAWN:
    // This function spawns a particle that spawns a character...
    tTmp = character;
    if (ChrList[character].attachedto != MAXCHR)  tTmp = ChrList[character].attachedto;
    tTmp = spawn_one_particle(valuetmpx, valuetmpy, valuetmpdistance, ChrList[character].turnleftright, ChrList[character].model, valuetmpargument, MAXCHR, 0, ChrList[character].team, tTmp, 0, MAXCHR, &scr_randie);
    if (tTmp != MAXPRT)
    {
      PrtList[tTmp].spawncharacterstate = valuetmpturn;
    }
    break;

  case FSPAWNPOOFSPEEDSPACINGDAMAGE:
    // This function makes a lovely little poof at the character's location,
    // adjusting the xy speed and spacing and the base damage first
    // Temporarily adjust the values for the particle type
    sTmp = ChrList[character].model;
    sTmp = MadList[sTmp].prtpip[CapList[sTmp].gopoofprttype];
    iTmp = PipList[sTmp].xyvelbase;
    tTmp = PipList[sTmp].xyspacingbase;
    test = PipList[sTmp].damagebase;
    PipList[sTmp].xyvelbase = valuetmpx;
    PipList[sTmp].xyspacingbase = valuetmpy;
    PipList[sTmp].damagebase = valuetmpargument;
    spawn_poof(character, ChrList[character].model, &scr_randie);
    // Restore the saved values
    PipList[sTmp].xyvelbase = iTmp;
    PipList[sTmp].xyspacingbase = tTmp;
    PipList[sTmp].damagebase = test;
    break;

  case FGIVEEXPERIENCETOGOODTEAM:
    // This function gives experience to everyone on the G Team
    give_team_experience(GOODTEAM, valuetmpargument, valuetmpdistance);
    break;

  case FDONOTHING:
    //This function does nothing (For use with combination with Else function or debugging)
    break;

  case FGROGTARGET:
    //This function grogs the target for a duration equal to tmpargument
    ChrList[ChrList[character].aitarget].grogtime += valuetmpargument;
    break;

  case FDAZETARGET:
    //This function dazes the target for a duration equal to tmpargument
    ChrList[ChrList[character].aitarget].dazetime += valuetmpargument;
    break;

  case FADDQUEST:
    //This function adds a quest idsz set in tmpargument into the targets quest.txt
    if(ChrList[ChrList[character].aitarget].isplayer)
    {
      snprintf(cTmp, sizeof(cTmp), "%s.obj", ChrList[ChrList[character].aitarget].name);
      returncode = add_quest_idsz(cTmp, valuetmpargument);									  
      //Todo: Should we add the quest idsz into all the players quest.txt?
    }
    break;

  case FBEATQUEST:
    //This function marks a IDSZ in the targets quest.txt as beaten
    if(ChrList[ChrList[character].aitarget].isplayer)
    {
      snprintf(cTmp, sizeof(cTmp), "%s.obj", ChrList[ChrList[character].aitarget].name);
      returncode = beat_quest_idsz(cTmp, valuetmpargument);									  
      //Todo: Should we add the quest idsz into all the players quest.txt?
    }
    break;

  case FIFTARGETHASQUEST:
    //This function proceeds if the target has the unfinished quest specified in tmpargument
    if(ChrList[ChrList[character].aitarget].isplayer)
	  {
      snprintf(cTmp, sizeof(cTmp), "%s.obj", ChrList[ChrList[character].aitarget].name);
		  iTmp = check_player_quest(cTmp, valuetmpargument);
		  if(iTmp > -1) returncode = btrue;
		  else returncode = bfalse;
	  }
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
    valuetmpx = valueoperationsum;
    break;

  case VARTMPY:
    valuetmpy = valueoperationsum;
    break;

  case VARTMPDISTANCE:
    valuetmpdistance = valueoperationsum;
    break;

  case VARTMPTURN:
    valuetmpturn = valueoperationsum;
    break;

  case VARTMPARGUMENT:
    valuetmpargument = valueoperationsum;
    break;

  }
}

//--------------------------------------------------------------------------------------------
void run_operand(Uint32 value, int character)
{
  // ZZ> This function does the scripted arithmetic in operator,operand pairs
  Uint8 opcode;
  Uint8 variable;
  int iTmp;


  // Get the operation code
  opcode = (value >> 27);
  if (opcode&16)
  {
    // Get the working value from a constant, constants are all but high 5 bits
    iTmp = value & 0x07ffffff;
  }
  else
  {
    // Get the working value from a register
    variable = value;
    iTmp = 1;
    switch (variable)
    {
    case VARTMPX:
      iTmp = valuetmpx;
      break;

    case VARTMPY:
      iTmp = valuetmpy;
      break;

    case VARTMPDISTANCE:
      iTmp = valuetmpdistance;
      break;

    case VARTMPTURN:
      iTmp = valuetmpturn;
      break;

    case VARTMPARGUMENT:
      iTmp = valuetmpargument;
      break;

    case VARRAND:
      iTmp = RANDIE(randie_index);
      break;

    case VARSELFX:
      iTmp = ChrList[character].xpos;
      break;

    case VARSELFY:
      iTmp = ChrList[character].ypos;
      break;

    case VARSELFTURN:
      iTmp = ChrList[character].turnleftright;
      break;

    case VARSELFCOUNTER:
      iTmp = ChrList[character].counter;
      break;

    case VARSELFORDER:
      iTmp = ChrList[character].order;
      break;

    case VARSELFMORALE:
      iTmp = TeamList[ChrList[character].baseteam].morale;
      break;

    case VARSELFLIFE:
      iTmp = ChrList[character].life;
      break;

    case VARTARGETX:
      iTmp = ChrList[ChrList[character].aitarget].xpos;
      break;

    case VARTARGETY:
      iTmp = ChrList[ChrList[character].aitarget].ypos;
      break;

    case VARTARGETDISTANCE:
      iTmp = ABS((int)(ChrList[ChrList[character].aitarget].xpos - ChrList[character].xpos)) +
             ABS((int)(ChrList[ChrList[character].aitarget].ypos - ChrList[character].ypos));
      break;

    case VARTARGETTURN:
      iTmp = ChrList[ChrList[character].aitarget].turnleftright;
      break;

    case VARLEADERX:
      iTmp = ChrList[character].xpos;
      if (TeamList[ChrList[character].team].leader != NOLEADER)
        iTmp = ChrList[TeamList[ChrList[character].team].leader].xpos;
      break;

    case VARLEADERY:
      iTmp = ChrList[character].ypos;
      if (TeamList[ChrList[character].team].leader != NOLEADER)
        iTmp = ChrList[TeamList[ChrList[character].team].leader].ypos;
      break;

    case VARLEADERDISTANCE:
      iTmp = 10000;
      if (TeamList[ChrList[character].team].leader != NOLEADER)
        iTmp = ABS((int)(ChrList[TeamList[ChrList[character].team].leader].xpos - ChrList[character].xpos)) +
               ABS((int)(ChrList[TeamList[ChrList[character].team].leader].ypos - ChrList[character].ypos));
      break;

    case VARLEADERTURN:
      iTmp = ChrList[character].turnleftright;
      if (TeamList[ChrList[character].team].leader != NOLEADER)
        iTmp = ChrList[TeamList[ChrList[character].team].leader].turnleftright;
      break;

    case VARGOTOX:
      iTmp = ChrList[character].aigotox[ChrList[character].aigoto];
      break;

    case VARGOTOY:
      iTmp = ChrList[character].aigotoy[ChrList[character].aigoto];
      break;

    case VARGOTODISTANCE:
      iTmp = ABS((int)(ChrList[character].aigotox[ChrList[character].aigoto] - ChrList[character].xpos)) +
             ABS((int)(ChrList[character].aigotoy[ChrList[character].aigoto] - ChrList[character].ypos));
      break;

    case VARTARGETTURNTO:
      iTmp = atan2(ChrList[ChrList[character].aitarget].ypos - ChrList[character].ypos, ChrList[ChrList[character].aitarget].xpos - ChrList[character].xpos) * RAD_TO_SHORT;
      iTmp += 32768;
      iTmp = iTmp & 65535;
      break;

    case VARPASSAGE:
      iTmp = ChrList[character].passage;
      break;

    case VARWEIGHT:
      iTmp = ChrList[character].holdingweight;
      break;

    case VARSELFALTITUDE:
      iTmp = ChrList[character].zpos - ChrList[character].level;
      break;

    case VARSELFID:
      iTmp = CapList[ChrList[character].model].idsz[IDSZTYPE];
      break;

    case VARSELFHATEID:
      iTmp = CapList[ChrList[character].model].idsz[IDSZHATE];
      break;

    case VARSELFMANA:
      iTmp = ChrList[character].mana;
      if (ChrList[character].canchannel)  iTmp += ChrList[character].life;
      break;

    case VARTARGETSTR:
      iTmp = ChrList[ChrList[character].aitarget].strength;
      break;

    case VARTARGETWIS:
      iTmp = ChrList[ChrList[character].aitarget].wisdom;
      break;

    case VARTARGETINT:
      iTmp = ChrList[ChrList[character].aitarget].intelligence;
      break;

    case VARTARGETDEX:
      iTmp = ChrList[ChrList[character].aitarget].dexterity;
      break;

    case VARTARGETLIFE:
      iTmp = ChrList[ChrList[character].aitarget].life;
      break;

    case VARTARGETMANA:
      iTmp = ChrList[ChrList[character].aitarget].mana;
      if (ChrList[ChrList[character].aitarget].canchannel)  iTmp += ChrList[ChrList[character].aitarget].life;
      break;

    case VARTARGETLEVEL:
      iTmp = ChrList[ChrList[character].aitarget].experiencelevel;
      break;

    case VARTARGETSPEEDX:
      iTmp = ChrList[ChrList[character].aitarget].xvel;
      break;

    case VARTARGETSPEEDY:
      iTmp = ChrList[ChrList[character].aitarget].yvel;
      break;

    case VARTARGETSPEEDZ:
      iTmp = ChrList[ChrList[character].aitarget].zvel;
      break;

    case VARSELFSPAWNX:
      iTmp = ChrList[character].xstt;
      break;

    case VARSELFSPAWNY:
      iTmp = ChrList[character].ystt;
      break;

    case VARSELFSTATE:
      iTmp = ChrList[character].aistate;
      break;

    case VARSELFSTR:
      iTmp = ChrList[character].strength;
      break;

    case VARSELFWIS:
      iTmp = ChrList[character].wisdom;
      break;

    case VARSELFINT:
      iTmp = ChrList[character].intelligence;
      break;

    case VARSELFDEX:
      iTmp = ChrList[character].dexterity;
      break;

    case VARSELFMANAFLOW:
      iTmp = ChrList[character].manaflow;
      break;

    case VARTARGETMANAFLOW:
      iTmp = ChrList[ChrList[character].aitarget].manaflow;
      break;

    case VARSELFATTACHED:
      iTmp = number_of_attached_particles(character);
      break;

    case VARSWINGTURN:
      iTmp = GCamera.swing << 2;
      break;

    case VARXYDISTANCE:
      iTmp = sqrt(valuetmpx * valuetmpx + valuetmpy * valuetmpy);
      break;

    case VARSELFZ:
      iTmp = ChrList[character].zpos;
      break;

    case VARTARGETALTITUDE:
      iTmp = ChrList[ChrList[character].aitarget].zpos - ChrList[ChrList[character].aitarget].level;
      break;

    case VARTARGETZ:
      iTmp = ChrList[ChrList[character].aitarget].zpos;
      break;

    case VARSELFINDEX:
      iTmp = character;
      break;

    case VAROWNERX:
      iTmp = ChrList[ChrList[character].aiowner].xpos;
      break;

    case VAROWNERY:
      iTmp = ChrList[ChrList[character].aiowner].ypos;
      break;

    case VAROWNERTURN:
      iTmp = ChrList[ChrList[character].aiowner].turnleftright;
      break;

    case VAROWNERDISTANCE:
      iTmp = ABS((int)(ChrList[ChrList[character].aiowner].xpos - ChrList[character].xpos)) +
             ABS((int)(ChrList[ChrList[character].aiowner].ypos - ChrList[character].ypos));
      break;

    case VAROWNERTURNTO:
      iTmp = atan2(ChrList[ChrList[character].aiowner].ypos - ChrList[character].ypos, ChrList[ChrList[character].aiowner].xpos - ChrList[character].xpos) * RAD_TO_SHORT;
      iTmp += 32768;
      iTmp = iTmp & 65535;
      break;

    case VARXYTURNTO:
      iTmp = atan2(valuetmpy - ChrList[character].ypos, valuetmpx - ChrList[character].xpos) * RAD_TO_SHORT;
      iTmp += 32768;
      iTmp = iTmp & 65535;
      break;

    case VARSELFMONEY:
      iTmp = ChrList[character].money;
      break;

    case VARSELFACCEL:
      iTmp = (ChrList[character].maxaccel * 100.0);
      break;

    case VARTARGETEXP:
      iTmp = ChrList[ChrList[character].aitarget].experience;
      break;

    case VARSELFAMMO:
      iTmp = ChrList[character].ammo;
      break;

    case VARTARGETAMMO:
      iTmp = ChrList[ChrList[character].aitarget].ammo;
      break;

    case VARTARGETMONEY:
      iTmp = ChrList[ChrList[character].aitarget].money;
      break;

    case VARTARGETTURNAWAY:
      iTmp = atan2(ChrList[ChrList[character].aitarget].ypos - ChrList[character].ypos, ChrList[ChrList[character].aitarget].xpos - ChrList[character].xpos) * RAD_TO_SHORT;
      iTmp += 32768;
      iTmp = iTmp & 65535;
      iTmp = iTmp + 65535;
      break;

    case VARSELFLEVEL:
      iTmp = ChrList[character].experiencelevel;
      break;

    }
  }


  // Now do the math
  switch (opcode&15)
  {
  case OPADD:
    valueoperationsum += iTmp;
    break;

  case OPSUB:
    valueoperationsum -= iTmp;
    break;

  case OPAND:
    valueoperationsum = valueoperationsum & iTmp;
    break;

  case OPSHR:
    valueoperationsum = valueoperationsum >> iTmp;
    break;

  case OPSHL:
    valueoperationsum = valueoperationsum << iTmp;
    break;

  case OPMUL:
    valueoperationsum = valueoperationsum * iTmp;
    break;

  case OPDIV:
    if (iTmp != 0)
    {
      valueoperationsum = valueoperationsum / iTmp;
    }
    break;

  case OPMOD:
    if (iTmp != 0)
    {
      valueoperationsum = valueoperationsum % iTmp;
    }
    break;

  }
}

//--------------------------------------------------------------------------------------------
void let_character_think(GAME_STATE * gs, int character, Uint32 * rand_idx)
{
  // ZZ> This function lets one character do AI stuff
  Uint16 aicode;
  Uint32 index;
  Uint32 value;
  Uint32 iTmp;
  Uint8 functionreturn;
  int operands;
  Uint32 scr_randie = *rand_idx;

  RANDIE(*rand_idx);


  // Make life easier
  valueoldtarget = ChrList[character].aitarget;
  aicode = ChrList[character].aitype;


  // Figure out alerts that weren't already set
  set_alerts(character);
  changed = bfalse;


  // Clear the button latches
  if (ChrList[character].isplayer == bfalse)
  {
    ChrList[character].latchbutton = 0;
  }


  // Reset the target if it can't be seen
  if (!ChrList[character].canseeinvisible && ChrList[character].alive)
  {
    if (ChrList[ChrList[character].aitarget].alpha <= INVISIBLE || ChrList[ChrList[character].aitarget].light <= INVISIBLE)
    {
      ChrList[character].aitarget = character;
    }
  }


  // Run the AI Script
  index = iAisStartPosition[aicode];
  valuegopoof = bfalse;


  value = iCompiledAis[index];
  while ((value&0x87ffffff) != 0x80000035) // End Function
  {
    value = iCompiledAis[index];
    // Was it a function
    if ((value&0x80000000) != 0)
    {
      // Run the function
      functionreturn = run_function(gs, value, character);
      // Get the jump code
      index++;
      iTmp = iCompiledAis[index];
      if (functionreturn == btrue)
      {
        // Proceed to the next function
        index++;
      }
      else
      {
        // Jump to where the jump code says to go
        index = iTmp;
      }
    }
    else
    {
      // Get the number of operands
      index++;
      operands = iCompiledAis[index];
      // Now run the operation
      valueoperationsum = 0;
      index++;
      while (operands > 0)
      {
        iTmp = iCompiledAis[index];
        run_operand(iTmp, character); // This sets valueoperationsum
        operands--;
        index++;
      }
      // Save the results in the register that called the arithmetic
      set_operand(value);
    }
    // This is used by the Else function
    valuelastindent = value;
  }


  // Set latches
  if (ChrList[character].isplayer == bfalse && aicode != 0)
  {
    if (ChrList[character].ismount && ChrList[character].holdingwhich[0] != MAXCHR)
    {
      // Mount
      ChrList[character].latchx = ChrList[ChrList[character].holdingwhich[0]].latchx;
      ChrList[character].latchy = ChrList[ChrList[character].holdingwhich[0]].latchy;
    }
    else
    {
      // Normal AI
      ChrList[character].latchx = (ChrList[character].aigotox[ChrList[character].aigoto] - ChrList[character].xpos) / 1000.0;
      ChrList[character].latchy = (ChrList[character].aigotoy[ChrList[character].aigoto] - ChrList[character].ypos) / 1000.0;
    }
  }


  // Clear alerts for next time around
  ChrList[character].alert = 0;
  if (changed)  ChrList[character].alert = ALERTIFCHANGED;


  // Do poofing
  if (valuegopoof)
  {
    if (ChrList[character].attachedto != MAXCHR)
      detach_character_from_mount(gs, character, btrue, bfalse, &scr_randie);
    if (ChrList[character].holdingwhich[0] != MAXCHR)
      detach_character_from_mount(gs, ChrList[character].holdingwhich[0], btrue, bfalse, &scr_randie);
    if (ChrList[character].holdingwhich[1] != MAXCHR)
      detach_character_from_mount(gs, ChrList[character].holdingwhich[1], btrue, bfalse, &scr_randie);
    free_inventory(character);
    free_one_character(character);
    // If this character was killed in another's script, we don't want the poof to
    // carry over...
    valuegopoof = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
void let_ai_think(GAME_STATE * gs, Uint32 * rand_idx)
{
  // ZZ> This function lets every computer controlled character do AI stuff
  int character;
  Uint32 ai_randie = *rand_idx;

  RANDIE(*rand_idx);


  numblip = 0;
  character = 0;
  while (character < MAXCHR)
  {
    if (ChrList[character].on && (!ChrList[character].inpack || CapList[ChrList[character].model].isequipment) && (ChrList[character].alive || (ChrList[character].alert&ALERTIFCLEANEDUP) || (ChrList[character].alert&ALERTIFCRUSHED)))
    {
      // Cleaned up characters shouldn't be alert to anything else
      if (ChrList[character].alert&ALERTIFCRUSHED)  ChrList[character].alert = ALERTIFCRUSHED;
      if (ChrList[character].alert&ALERTIFCLEANEDUP)  ChrList[character].alert = ALERTIFCLEANEDUP;
      let_character_think(gs, character, &ai_randie);
    }
    character++;
  }
}

