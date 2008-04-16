//---------------------------------------------------------
//
// egoboostrutil.c
//
//
//
//
//
//---------------------------------------------------------

#include "egoboostrutil.h"

// TrimStr remove all space and tabs in the beginning and at the end of the string
void TrimStr(char *pStr)
{
  Sint32 DebPos, EndPos, CurPos;

  if (pStr == NULL)
  {
    return;
  }
  // look for the first character in string
  DebPos = 0;
  while (isspace(pStr[DebPos]) && pStr[DebPos] != 0)
  {
    DebPos++;
  }

  // look for the last character in string
  CurPos = DebPos;
  while (pStr[CurPos] != 0)
  {
    if (!isspace(pStr[CurPos]))
    {
      EndPos = CurPos;
    }
    CurPos++;
  }

  if (DebPos != 0)
  {
    // shift string left
    for (CurPos = 0; CurPos <= (EndPos - DebPos); CurPos++)
    {
      pStr[CurPos] = pStr[CurPos + DebPos];
    }
    pStr[CurPos] = 0;
  }
  else
  {
    pStr[EndPos + 1] = 0;
  }
}
