#include "Mad.h"
#include "Character.h"
#include "MD2_file.h"
#include "Profile.h"
#include "egoboo.h"

char cActionName[ACTION_COUNT][2];                  // Two letter name code

Mad_List MadList;

//---------------------------------------------------------------------------------------------
bool Mad::load(const char * filename, Mad * pmad)
{
  if(NULL==JF::MD2_Manager::loadFromFile(filename, pmad))
    return false;

  scale = .6;

  if(m_numFrames>0)
  {
    m_frame_ex = new Frame_Extras[m_numFrames];
  };

  return true;
};

//---------------------------------------------------------------------------------------------
void Profile_List::release_local_pips()
{
  // Now clear out the local pips

  for (int prof = 0; prof<SIZE; prof++)
  {
    if(!_list[prof].allocated()) continue;

    for (int cnt = 0; cnt < PRTPIP_COUNT; cnt++)
    {
      _list[prof].prtpip[cnt] = Pip_List::INVALID;
    }
  }
};

//---------------------------------------------------------------------------------------------
void Profile_List::release_all_models()
{
  // ZZ> This function clears out all of the models

  bool found;

  for (int cnt = 0; cnt<SIZE; cnt++)
  {
    if(!_list[cnt].allocated()) continue;

    found = false;

    MAD_REF mad_ref = _list[cnt].mad_ref;
    if(VALID_MAD(mad_ref))
    {
      MadList.return_one(mad_ref.index);
      found = true;
    };

    CAP_REF cap_ref = _list[cnt].cap_ref;
    if(VALID_CAP(cap_ref))
    {
      CapList.return_one(cap_ref.index);
      found = true;
    };

    EVE_REF eve_ref = _list[cnt].eve_ref;
    if(VALID_EVE(eve_ref))
    {
      EveList.return_one(eve_ref.index);
      found = true;
    };

    // only remove the profile if it is for a mad/cap, not a pip or eve
    if(found)
      return_one(cnt);
  }

  madloadframe=0;

  reset();
}

//--------------------------------------------------------------------------------------------
void Mad::rip_actions()
{
  // ZZ> This function creates the frame lists for each action based on the
  //     name of each md2 frame in the model

  int frame, framesinaction;
  ACTION_TYPE action, lastaction;

  // Clear out all actions and reset to invalid
  for(int i=0; i<ACTION_COUNT; i++)
  {
    actinfo[i].start = 0;
    actinfo[i].end   = 0;
    actinfo[i].valid = false;
  }

  Uint32 frame_cnt = numFrames();
  if(0==frame_cnt) return;

  // Set the primary dance action to be the first frame, just as a default
  lastaction = ACTION_DA;
  actinfo[ACTION_DA].valid = true;
  actinfo[ACTION_DA].start = 0;
  actinfo[ACTION_DA].end   = 1;

  // Now go huntin' to see what each frame is, look for runs of same action
  framesinaction = 0;
  for (frame = 0; frame < frame_cnt; frame++)
  {
    const JF::MD2_Frame * pframe = getFrame(frame);  assert(NULL!=pframe);

    action = action_number(pframe->name);
    if(0==frame) lastaction = action;

    if (lastaction==action)
    {
      framesinaction++;
    }
    else
    {
      // Write the old action
      if (lastaction < ACTION_COUNT)
      {
        actinfo[lastaction].valid = true;
        actinfo[lastaction].start = frame-framesinaction;
        actinfo[lastaction].end   = frame;
      }
      framesinaction = 1;
      lastaction = action;
    }

    m_frame_ex[frame].framefx = get_framefx(frame);
  }

  // Write the old action
  if (lastaction < ACTION_COUNT)
  {
    actinfo[lastaction].valid = true;
    actinfo[lastaction].start = frame-framesinaction;
    actinfo[lastaction].end   = frame;
  }

  // Make sure actions are made valid if a similar one exists
  action_copy_correct(ACTION_DA, ACTION_DB);  // All dances should be safe
  action_copy_correct(ACTION_DB, ACTION_DC);
  action_copy_correct(ACTION_DC, ACTION_DD);
  action_copy_correct(ACTION_DB, ACTION_DC);
  action_copy_correct(ACTION_DA, ACTION_DB);
  action_copy_correct(ACTION_UA, ACTION_UB);
  action_copy_correct(ACTION_UB, ACTION_UC);
  action_copy_correct(ACTION_UC, ACTION_UD);
  action_copy_correct(ACTION_TA, ACTION_TB);
  action_copy_correct(ACTION_TC, ACTION_TD);
  action_copy_correct(ACTION_CA, ACTION_CB);
  action_copy_correct(ACTION_CC, ACTION_CD);
  action_copy_correct(ACTION_SA, ACTION_SB);
  action_copy_correct(ACTION_SC, ACTION_SD);
  action_copy_correct(ACTION_BA, ACTION_BB);
  action_copy_correct(ACTION_BC, ACTION_BD);
  action_copy_correct(ACTION_LA, ACTION_LB);
  action_copy_correct(ACTION_LC, ACTION_LD);
  action_copy_correct(ACTION_XA, ACTION_XB);
  action_copy_correct(ACTION_XC, ACTION_XD);
  action_copy_correct(ACTION_FA, ACTION_FB);
  action_copy_correct(ACTION_FC, ACTION_FD);
  action_copy_correct(ACTION_PA, ACTION_PB);
  action_copy_correct(ACTION_PC, ACTION_PD);
  action_copy_correct(ACTION_ZA, ACTION_ZB);
  action_copy_correct(ACTION_ZC, ACTION_ZD);
  action_copy_correct(ACTION_WA, ACTION_WB);
  action_copy_correct(ACTION_WB, ACTION_WC);
  action_copy_correct(ACTION_WC, ACTION_WD);
  action_copy_correct(ACTION_DA, ACTION_WD);  // All walks should be safe
  action_copy_correct(ACTION_WC, ACTION_WD);
  action_copy_correct(ACTION_WB, ACTION_WC);
  action_copy_correct(ACTION_WA, ACTION_WB);
  action_copy_correct(ACTION_JA, ACTION_JB);
  action_copy_correct(ACTION_JB, ACTION_JC);
  action_copy_correct(ACTION_DA, ACTION_JC);    // All jumps should be safe
  action_copy_correct(ACTION_JB, ACTION_JC);
  action_copy_correct(ACTION_JA, ACTION_JB);
  action_copy_correct(ACTION_HA, ACTION_HB);
  action_copy_correct(ACTION_HB, ACTION_HC);
  action_copy_correct(ACTION_HC, ACTION_HD);
  action_copy_correct(ACTION_HB, ACTION_HC);
  action_copy_correct(ACTION_HA, ACTION_HB);
  action_copy_correct(ACTION_KA, ACTION_KB);
  action_copy_correct(ACTION_KB, ACTION_KC);
  action_copy_correct(ACTION_KC, ACTION_KD);
  action_copy_correct(ACTION_KB, ACTION_KC);
  action_copy_correct(ACTION_KA, ACTION_KB);
  action_copy_correct(ACTION_MH, ACTION_MI);
  action_copy_correct(ACTION_DA, ACTION_MM);
  action_copy_correct(ACTION_MM, ACTION_MN);


  // Create table for doing transition from one type of walk to another...
  // Clear 'em all to start
  for (frame = 0; frame < numFrames(); frame++)
    m_frame_ex[frame].framelip = 0;

  // Need to figure out how far into action each frame is
  make_framelip(ACTION_WA);
  make_framelip(ACTION_WB);
  make_framelip(ACTION_WC);

  // Now do the same, in reverse, for walking animations
  get_walk_frame(LIPDA, ACTION_DA);
  get_walk_frame(LIPWA, ACTION_WA);
  get_walk_frame(LIPWB, ACTION_WB);
  get_walk_frame(LIPWC, ACTION_WC);
  get_walk_frame(LIPWD, ACTION_WD);
}

//--------------------------------------------------------------------------------------------
void make_md2_equally_lit(MAD_REF & mad_ref)
{
  // ZZ> This function makes ultra low poly models look better

  if ( INVALID_MODEL(mad_ref.index) ) return;

  JF::MD2_Model * mdl = &MadList[mad_ref];
  if(NULL==mdl) return;

  Uint32 frame_cnt = mdl->numFrames();
  Uint32 vrt_cnt   = mdl->numVertices();

  for (Uint32 cnt = 0; cnt < frame_cnt; cnt++)
  {
    const JF::MD2_Frame  * frame = mdl->getFrame(cnt);

    for (Uint32 vert = 0; vert < vrt_cnt; vert++  )
    {
      frame->vertices[vert].normal = EQUALLIGHTINDEX;
    }
  }
}

//--------------------------------------------------------------------------------------------
void Mad::check_copy(char* loadname)
{
  // ZZ> This function copies a model's actions
  FILE *fileread;
  int actiona, actionb;
  char szOne[16], szTwo[16];

  fileread = fopen(loadname, "r");
  if (fileread)
  {
    while (goto_colon_yesno(fileread))
    {
      fscanf(fileread, "%s%s", szOne, szTwo);
      actiona = what_action(szOne[0]);
      actionb = what_action(szTwo[0]);
      action_copy_correct(actiona+0, actionb+0);
      action_copy_correct(actiona+1, actionb+1);
      action_copy_correct(actiona+2, actionb+2);
      action_copy_correct(actiona+3, actionb+3);
    }
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
void Mad::action_copy_correct(Uint16 actiona, Uint16 actionb)
{
  // ZZ> This function makes sure both actions are valid if either of them
  //     are valid.  It will copy start and ends to mirror the valid action.
  if (actinfo[actiona].valid==actinfo[actionb].valid)
  {
    // They are either both valid or both invalid, in either case we can't help
  }
  else
  {
    // Fix the invalid one
    if (!actinfo[actiona].valid)
    {
      // Fix actiona
      actinfo[actiona].valid = true;
      actinfo[actiona].start = actinfo[actionb].start;
      actinfo[actiona].end   = actinfo[actionb].end;
    }
    else
    {
      // Fix actionb
      actinfo[actionb].valid = true;
      actinfo[actionb].start = actinfo[actiona].start;
      actinfo[actionb].end   = actinfo[actiona].end;
    }
  }
}

//--------------------------------------------------------------------------------------------
void Mad::get_walk_frame(int lip, int action)
{
  // ZZ> This helps make walking look right
  int frame = 0;
  int framesinaction = actinfo[action].end-actinfo[action].start;

  while (frame < 16)
  {
    int framealong = 0;
    if (framesinaction > 0)
    {
      framealong = ((frame*framesinaction/16) + 2)%framesinaction;
    }
    frameliptowalkframe[lip][frame] = actinfo[action].start + framealong;
    frame++;
  }
}

//--------------------------------------------------------------------------------------------
Uint32 Mad::get_framefx(Uint32 frame)
{
  // ZZ> This function figures out the IFrame invulnerability, and Attack, Grab, and
  //     Drop timings
  Uint32 fx = 0;

  char * name = m_frames[frame].name;
  if(0x00 == name[0]) return MADFX_NONE;

  if (test_frame_name(name, 'I')) fx |= MADFX_INVICTUS;
  if (test_frame_name(name, 'L'))
  {
    if (test_frame_name(name, 'A')) fx |= MADFX_ACTLEFT;
    if (test_frame_name(name, 'G')) fx |= MADFX_GRABLEFT;
    if (test_frame_name(name, 'D')) fx |= MADFX_DROPLEFT;
    if (test_frame_name(name, 'C')) fx |= MADFX_CHARLEFT;
  }

  if (test_frame_name(name, 'R'))
  {
    if (test_frame_name(name, 'A')) fx |= MADFX_ACTRIGHT;
    if (test_frame_name(name, 'G')) fx |= MADFX_GRABRIGHT;
    if (test_frame_name(name, 'D')) fx |= MADFX_DROPRIGHT;
    if (test_frame_name(name, 'C')) fx |= MADFX_CHARRIGHT;
  }

  if (test_frame_name(name, 'S')) fx |= MADFX_STOP;
  if (test_frame_name(name, 'F')) fx |= MADFX_FOOTFALL;
  if (test_frame_name(name, 'P')) fx |= MADFX_POOF;

  return fx;
}

//--------------------------------------------------------------------------------------------
void Mad::make_framelip(int action)
{
  // ZZ> This helps make walking look right
  int frame, framesinaction;

  if (actinfo[action].valid)
  {
    framesinaction = actinfo[action].end-actinfo[action].start;
    frame = actinfo[action].start;
    while (frame < actinfo[action].end)
    {
      m_frame_ex[frame].framelip = (frame-actinfo[action].start)*15/framesinaction;
      m_frame_ex[frame].framelip &= 15;
      frame++;
    }
  }
}

//--------------------------------------------------------------------------------------------
ACTION_TYPE action_number(const char * name, ACTION_TYPE last)
{
  // ZZ> This function returns the number of the action in cFrameName, or
  //     it returns ACTION_INVALID if it could not find a match

  if(name == NULL) return ACTION_INVALID;

  char first  = name[0];
  char second = name[1];

  // try for an optimization
  if(ACTION_INVALID != last)
  {
    if(first == cActionName[last][0] && second == cActionName[last][1])
      return last;
  }

  for (int cnt = 0; cnt < ACTION_COUNT; cnt++)
  {
    if (first == cActionName[cnt][0] && second == cActionName[cnt][1])
      return (ACTION_TYPE)cnt;
  }

  return ACTION_INVALID;
}

//--------------------------------------------------------------------------------------------
Uint32 action_frame(const char * name)
{
  // ZZ> This function returns the frame number in the third and fourth characters
  //     of cFrameName

  if(NULL==name) return -1;

  Uint32 number;
  sscanf(name + 2, "%ld", &number);

  return number;
}

//--------------------------------------------------------------------------------------------
Uint16 Mad::test_frame_name(const char * name, char letter)
{
  // ZZ> This function returns true if the 4th, 5th, 6th, or 7th letters
  //     of the frame name matches the input argument

  if(NULL==name) return false;

  if (name[4]==letter) return true;
  if (name[4]==0) return false;
  if (name[5]==letter) return true;
  if (name[5]==0) return false;
  if (name[6]==letter) return true;
  if (name[6]==0) return false;
  if (name[7]==letter) return true;

  return false;
}

//--------------------------------------------------------------------------------------------
void log_madused(char *savename)
{
  // ZZ> This is a debug function for checking model loads
  FILE* hFileWrite;
  int cnt;

  hFileWrite = fopen(savename, "w");
  if (hFileWrite)
  {
    fprintf(hFileWrite, "Slot usage for objects in last module loaded...\n");
    fprintf(hFileWrite, "%d of %d frames used...\n", madloadframe, MAXFRAME);

    for(cnt = 0; cnt<Profile_List::SIZE; cnt++)
    {
      if( VALID_MODEL(cnt) )
      {
        CAP_REF cap_ref = ProfileList[cnt].cap_ref;

        fprintf(hFileWrite, "%3d %32s %s\n", cnt, CapList[cap_ref].classname, ProfileList[cnt].name);
      };
    }
    fclose(hFileWrite);
  }
}

//---------------------------------------------------------------------------------------------
//void rip_md2_frames(JF::MD2_Model * mdl)
//{
//  // ZZ> This function gets frames from the load buffer and adds them to
//  //     the indexed model
//  Uint8 cTmpx, cTmpy, cTmpz;
//  Uint8 cTmpNormalIndex;
//  float fRealx, fRealy, fRealz;
//  float fScalex, fScaley, fScalez;
//  float fTranslatex, fTranslatey, fTranslatez;
//  int iFrameOffset;
//  int iNumVertices;
//  int iNumFrames;
//  int cnt, tnc;
//  char* cpCharPointer;
//  int* ipIntPointer;
//  float* fpFloatPointer;
//
//  // Jump to the Frames section of the md2 data
//  cpCharPointer = (char*) cLoadBuffer;
//  ipIntPointer = (int*) cLoadBuffer;
//  fpFloatPointer = (float*) cLoadBuffer;
//
//  iNumVertices = SDL_SwapBE32(ipIntPointer[6]);
//  iNumFrames   = SDL_SwapBE32(ipIntPointer[10]);
//  iFrameOffset = SDL_SwapBE32(ipIntPointer[14])>>2;
//
//
//  // Read in each frame
//  MadList[modelindex].framestart = madloadframe;
//  MadList[modelindex].frames = iNumFrames;
//  MadList[modelindex].vertices = iNumVertices;
//  MadList[modelindex].scale = (float)(1.0/320.0);   // Scale each vertex float to fit it in a short
//  cnt = 0;
//  while (cnt < iNumFrames && madloadframe < MAXFRAME)
//  {
//    fScalex     = SDL_SwapLEFloat(fpFloatPointer[iFrameOffset]); iFrameOffset++;
//    fScaley     = SDL_SwapLEFloat(fpFloatPointer[iFrameOffset]); iFrameOffset++;
//    fScalez     = SDL_SwapLEFloat(fpFloatPointer[iFrameOffset]); iFrameOffset++;
//    fTranslatex = SDL_SwapLEFloat(fpFloatPointer[iFrameOffset]); iFrameOffset++;
//    fTranslatey = SDL_SwapLEFloat(fpFloatPointer[iFrameOffset]); iFrameOffset++;
//    fTranslatez = SDL_SwapLEFloat(fpFloatPointer[iFrameOffset]); iFrameOffset++;
//
//    iFrameOffset+=4;
//    tnc = 0;
//    while (tnc < iNumVertices)
//    {
//      // This should work because it's reading a single character
//      cTmpx = cpCharPointer[(iFrameOffset<<2)];
//      cTmpy = cpCharPointer[(iFrameOffset<<2)+1];
//      cTmpz = cpCharPointer[(iFrameOffset<<2)+2];
//      cTmpNormalIndex = cpCharPointer[(iFrameOffset<<2)+3];
//      fRealx = (cTmpx*fScalex)+fTranslatex;
//      fRealy = (cTmpy*fScaley)+fTranslatey;
//      fRealz = (cTmpz*fScalez)+fTranslatez;
//      //            fRealx = (cTmpx*fScalex);
//      //            fRealy = (cTmpy*fScaley);
//      //            fRealz = (cTmpz*fScalez);
//      //            FrameList[madloadframe].vrt_x[tnc] = (Sint16) (fRealx*0x0100); // HUK
//      FrameList[madloadframe].vrt_x[tnc] = (Sint16)(-fRealx*0x0100);
//      FrameList[madloadframe].vrt_y[tnc] = (Sint16)(fRealy*0x0100);
//      FrameList[madloadframe].vrt_z[tnc] = (Sint16)(fRealz*0x0100);
//      FrameList[madloadframe].vrt_a[tnc] = cTmpNormalIndex;
//      iFrameOffset++;
//      tnc++;
//    }
//    madloadframe++;
//    cnt++;
//  }
//}
//
//---------------------------------------------------------------------------------------------
//int process_one_md2(JF::MD2_Model * mdl)
//{
//  // ZZ> This function loads an id md2 file, storing the converted data in the indexed model
//  //    int iFileHandleRead;
//
//  if(NULL == mdl) return false;
//
//  // Get the frame vertices
//  rip_md2_frames(mdl);
//
//  // Get the commands
//  rip_md2_commands(mdl);
//
//  // Fix them normals
//  fix_md2_normals(mdl);
//
//  // Figure out how many vertices to transform
//  get_madtransvertices(mdl);
//
//  fclose(file);
//
//  return true;
//}
//
//
//---------------------------------------------------------------------------------------------
//int rip_md2_header(void)
//{
//  // ZZ> This function makes sure an md2 is really an md2
//  int iTmp;
//  int* ipIntPointer;
//
//  // Check the file type
//  ipIntPointer = (int*) cLoadBuffer;
//  iTmp = SDL_SwapBE32(ipIntPointer[0]);
//
//  if (iTmp != MD2START) return false;
//
//  return true;
//}
//
//---------------------------------------------------------------------------------------------
//void fix_md2_normals(Uint16 modelindex)
//{
//  // ZZ> This function helps light not flicker so much
//  int cnt, tnc;
//  Uint8 indexofcurrent, indexofnext, indexofnextnext, indexofnextnextnext;
//  Uint8 indexofnextnextnextnext;
//  Uint32 frame;
//
//  frame = MadList[modelindex].framestart;
//  cnt = 0;
//  while (cnt < MadList[modelindex].vertices)
//  {
//    tnc = 0;
//    while (tnc < MadList[modelindex].frames)
//    {
//      indexofcurrent = FrameList[frame].vrt_a[cnt];
//      indexofnext = FrameList[frame+1].vrt_a[cnt];
//      indexofnextnext = FrameList[frame+2].vrt_a[cnt];
//      indexofnextnextnext = FrameList[frame+3].vrt_a[cnt];
//      indexofnextnextnextnext = FrameList[frame+4].vrt_a[cnt];
//      if (indexofcurrent == indexofnextnext && indexofnext != indexofcurrent)
//      {
//        FrameList[frame+1].vrt_a[cnt] = indexofcurrent;
//      }
//      if (indexofcurrent == indexofnextnextnext)
//      {
//        if (indexofnext != indexofcurrent)
//        {
//          FrameList[frame+1].vrt_a[cnt] = indexofcurrent;
//        }
//        if (indexofnextnext != indexofcurrent)
//        {
//          FrameList[frame+2].vrt_a[cnt] = indexofcurrent;
//        }
//      }
//      if (indexofcurrent == indexofnextnextnextnext)
//      {
//        if (indexofnext != indexofcurrent)
//        {
//          FrameList[frame+1].vrt_a[cnt] = indexofcurrent;
//        }
//        if (indexofnextnext != indexofcurrent)
//        {
//          FrameList[frame+2].vrt_a[cnt] = indexofcurrent;
//        }
//        if (indexofnextnextnext != indexofcurrent)
//        {
//          FrameList[frame+3].vrt_a[cnt] = indexofcurrent;
//        }
//      }
//      tnc++;
//    }
//    cnt++;
//  }
//}
//
//---------------------------------------------------------------------------------------------
//void rip_md2_commands(Uint16 modelindex)
//{
//  // ZZ> This function converts an md2's GL commands into our little command list thing
//  int iTmp;
//  float fTmpu, fTmpv;
//  int iNumVertices;
//  int tnc;
//
//  char* cpCharPointer = (char*) cLoadBuffer;
//  int* ipIntPointer = (int*) cLoadBuffer;
//  float* fpFloatPointer = (float*) cLoadBuffer;
//
//  // Number of GL commands in the MD2
//  int iNumCommands = SDL_SwapBE32(ipIntPointer[9]);
//
//  // Offset (in DWORDS) from the start of the file to the gl command list.
//  int iCommandOffset = SDL_SwapBE32(ipIntPointer[15])>>2;
//
//  // Read in each command
//  // iNumCommands isn't the number of commands, rather the number of dwords in
//  // the command list...  Use iCommandCount to figure out how many we use
//  int iCommandCount = 0;
//  int entry = 0;
//
//  int cnt = 0;
//  while (cnt < iNumCommands)
//  {
//    iNumVertices = SDL_SwapBE32(ipIntPointer[iCommandOffset]);  iCommandOffset++;  cnt++;
//
//    if (iNumVertices != 0)
//    {
//      if (iNumVertices < 0)
//      {
//        // Fans start with a negative
//        iNumVertices = -iNumVertices;
//        // PORT: MadList[modelindex].commandtype[iCommandCount] = (Uint8) D3DPT_TRIANGLEFAN;
//        MadList[modelindex].commandtype[iCommandCount] = GL_TRIANGLE_FAN;
//        MadList[modelindex].commandsize[iCommandCount] = (Uint8) iNumVertices;
//      }
//      else
//      {
//        // Strips start with a positive
//        // PORT: MadList[modelindex].commandtype[iCommandCount] = (Uint8) D3DPT_TRIANGLESTRIP;
//        MadList[modelindex].commandtype[iCommandCount] = GL_TRIANGLE_STRIP;
//        MadList[modelindex].commandsize[iCommandCount] = (Uint8) iNumVertices;
//      }
//
//      // Read in vertices for each command
//      tnc = 0;
//      while (tnc < iNumVertices)
//      {
//        fTmpu = SDL_SwapLEFloat(fpFloatPointer[iCommandOffset]);  iCommandOffset++;  cnt++;
//        fTmpv = SDL_SwapLEFloat(fpFloatPointer[iCommandOffset]);  iCommandOffset++;  cnt++;
//        iTmp  = SDL_SwapBE32(ipIntPointer[iCommandOffset]);  iCommandOffset++;  cnt++;
//
//        MadList[modelindex].commandu[entry] = fTmpu-(.5/0x40); // GL doesn't align correctly
//        MadList[modelindex].commandv[entry] = fTmpv-(.5/0x40); // with D3D
//        MadList[modelindex].commandvrt[entry] = (Uint16) iTmp;
//        entry++;
//        tnc++;
//      }
//      iCommandCount++;
//    }
//  }
//  MadList[modelindex].commands = iCommandCount;
//}
//
//---------------------------------------------------------------------------------------------
//int rip_md2_frame_name(int frame)
//{
//  // ZZ> This function gets frame names from the load buffer, it returns
//  //     true if the name in cFrameName[] is valid
//  int iFrameOffset;
//  int iNumVertices;
//  int iNumFrames;
//  int cnt;
//  int* ipNamePointer;
//  int* ipIntPointer;
//  int foundname;
//
//  // Jump to the Frames section of the md2 data
//  ipNamePointer = (int*) cFrameName;
//  ipIntPointer  = (int*) cLoadBuffer;
//
//  iNumVertices = SDL_SwapLE32(ipIntPointer[6]);
//  iNumFrames   = SDL_SwapLE32(ipIntPointer[10]);
//  iFrameOffset = SDL_SwapLE32(ipIntPointer[14])>>2;
//
//
//  // Chug through each frame
//  foundname = false;
//  cnt = 0;
//  while (cnt < iNumFrames && !foundname)
//  {
//    iFrameOffset+=6;
//    if (cnt == frame)
//    {
//      ipNamePointer[0] = ipIntPointer[iFrameOffset]; iFrameOffset++;
//      ipNamePointer[1] = ipIntPointer[iFrameOffset]; iFrameOffset++;
//      ipNamePointer[2] = ipIntPointer[iFrameOffset]; iFrameOffset++;
//      ipNamePointer[3] = ipIntPointer[iFrameOffset]; iFrameOffset++;
//      foundname = true;
//    }
//    else
//    {
//      iFrameOffset+=4;
//    }
//    iFrameOffset+=iNumVertices;
//    cnt++;
//  }
//  cFrameName[15] = 0;  // Make sure it's null terminated
//  return foundname;
//}
//
//---------------------------------------------------------------------------------------------
int vertexconnected(JF::MD2_Model * mdl, Uint32 index)
{
  // ZZ> This function returns 1 if the model vertex is connected, 0 otherwise

  if (NULL == mdl) return 0;

  // scan through the commands to see if this vertex index is referenced
  JF::MD2_GLCommand * cmd = (JF::MD2_GLCommand *)mdl->getCommands();
  for( /* nothing */;  NULL != cmd; cmd = cmd->next)
  {
    Uint32 cmd_cnt = cmd->command_count;

    for(int cnt=0; cnt<cmd_cnt; cnt++)
    {
      if(cmd->data[cnt].index == index) return 1;
    }
  };

  // The vertex is not drawn, it is not connected.
  return 0;
}

//---------------------------------------------------------------------------------------------
void get_madtransvertices(MAD_REF & mad_ref)
{
  // ZZ> This function gets the number of vertices to transform for a model...
  //     That means every one except the grip ( unconnected ) vertices
  Uint32 cnt, trans = 0;

  JF::MD2_Model * mdl = &MadList[mad_ref];
  if(NULL==mdl) return;

  Uint32 vrt_cnt = mdl->numVertices();
  for (cnt = 0; cnt < vrt_cnt; cnt++)
    trans += vertexconnected(mdl, cnt);

  MadList[mad_ref].transvertices = trans;
}

//---------------------------------------------------------------------------------------------
ACTION_TYPE Mad::what_action(char cTmp)
{
  // ZZ> This function changes a letter into an action code
  ACTION_TYPE action = ACTION_DA;

  switch(toupper(cTmp))
  {
    case 'U': action = ACTION_UA; break;
    case 'T': action = ACTION_TA; break;
    case 'S': action = ACTION_SA; break;
    case 'C': action = ACTION_CA; break;
    case 'B': action = ACTION_BA; break;
    case 'L': action = ACTION_LA; break;
    case 'X': action = ACTION_XA; break;
    case 'F': action = ACTION_FA; break;
    case 'P': action = ACTION_PA; break;
    case 'Z': action = ACTION_ZA; break;
  }

  return action;
}

Uint32 Mad_List::load_one_mad(const char * pathname)
{

  Uint32 mad_idx = get_free();
  if(Mad_List::INVALID==mad_idx) return mad_idx;

  char newloadname[0x0100];

  make_newloadname(pathname, "tris.md2", newloadname);

  if( !_list[mad_idx].load(newloadname, &_list[mad_idx]) )
  {
    // error loading the MD2
    return_one(mad_idx);
    return Mad_List::INVALID;
  };

  // Create the actions table for this model
  _list[mad_idx].rip_actions();

  // Copy entire actions to save frame space COPY.TXT
  make_newloadname(pathname, "copy.txt", newloadname);
  _list[mad_idx].check_copy(newloadname);


  return mad_idx;
};