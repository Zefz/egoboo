#include "Profile.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Chop_Data    GChops;
Profile_List ProfileList;

//--------------------------------------------------------------------------------------------
void Profile::load_all_messages(char *loadname)
{
  // ZZ> This function loads all of an objects messages
  FILE *fileread;

  msg_start = 0;
  fileread = fopen(loadname, "r");
  if (fileread)
  {
    msg_start = GMsg.total;
    while (goto_colon_yesno(fileread))
    {
      get_message(fileread);
    }
    fclose(fileread);
  }
}


//--------------------------------------------------------------------------------------------
void Profile::reset()
{
  // set to their invalid values
  strcpy(name, "*NONE*");
  mad_ref   = Mad_List::INVALID;
  cap_ref   = Cap_List::INVALID;
  eve_ref   = Eve_List::INVALID;
  scr_ref   = Script_List::INVALID;

  int cnt;
  for (cnt = 0; cnt < PRTPIP_COUNT; cnt++)
  {
    prtpip[cnt] = Pip_List::INVALID;
  }

  for (cnt = 0; VALID_WAVE_RANGE(cnt); cnt++)
  {
    waveindex[cnt] = NULL;
  };
};


//--------------------------------------------------------------------------------------------
void Profile::naming_names()
{
  // ZZ> This function generates a random name
  int read, write, section, mychop;
  char cTmp;

  if (this->sectionsize[0] == 0)
  {
    strcpy(GChops.name, "Blah");
  }
  else
  {
    write = 0;
    section = 0;
    while (section < MAXSECTION)
    {
      if (this->sectionsize[section] != 0)
      {
        mychop = sectionstart[section] + (rand()%sectionsize[section]);
        read = GChops.start[mychop];
        cTmp = GChops.data[read];
        while (cTmp != 0 && write < MAXCAPNAMESIZE-1)
        {
          GChops.name[write]=cTmp;
          write++;
          read++;
          cTmp = GChops.data[read];
        }
      }
      section++;
    }
    if (write >= MAXCAPNAMESIZE) write = MAXCAPNAMESIZE-1;
    GChops.name[write] = 0;
  }
}

//--------------------------------------------------------------------------------------------
void Profile::read_naming(char *szPathname)
{
  // ZZ> This function reads a naming file
  FILE *fileread;
  int section, chopinsection, cnt;
  char mychop[0x20], cTmp;
  char szLoadname[0x0100];

  make_newloadname(szPathname, "naming.txt", szLoadname);
  fileread = fopen(szLoadname, "r");
  if (fileread)
  {
    section = 0;
    chopinsection = 0;
    while (goto_colon_yesno(fileread) && section < MAXSECTION)
    {
      fscanf(fileread, "%s", mychop);
      if (mychop[0] != 'S' || mychop[1] != 'T' || mychop[2] != 'O' || mychop[3] != 'P')
      {
        if (GChops.write >= CHOPDATACHUNK)  GChops.write = CHOPDATACHUNK-1;
        GChops.start[GChops.count] = GChops.write;
        cnt = 0;
        cTmp = mychop[0];
        while (cTmp != 0 && cnt < 0x1F && GChops.write < CHOPDATACHUNK)
        {
          if (cTmp == '_') cTmp = ' ';
          GChops.data[GChops.write]=cTmp;
          cnt++;
          GChops.write++;
          cTmp = mychop[cnt];
        }
        if (GChops.write >= CHOPDATACHUNK)  GChops.write = CHOPDATACHUNK-1;
        GChops.data[GChops.write]=0;  GChops.write++;
        chopinsection++;
        GChops.count++;
      }
      else
      {
        sectionsize[section] = chopinsection;
        sectionstart[section] = GChops.count-chopinsection;
        section++;
        chopinsection = 0;
      }
    }
    fclose(fileread);
  }
}

//--------------------------------------------------------------------------------------------
void Profile_List::prime_names(void)
{
  // ZZ> This function prepares the name chopper for use
  int cnt, tnc;

  GChops.count = 0;
  GChops.write = 0;
  for(cnt = 0; cnt < SIZE; cnt++)
  {
    tnc = 0;
    while (tnc < MAXSECTION)
    {
      _list[cnt].sectionstart[tnc] = MAXCHOP;
      _list[cnt].sectionsize[tnc] = 0;
      tnc++;
    }
  }

}

