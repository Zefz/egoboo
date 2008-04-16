#pragma once

#include "egobootypedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXSERVICE   16
#define NETNAMESIZE  16
#define MAXSESSION   16
#define MAXNETPLAYER  8
#define MESSAGESIZE  80

//--------------------------------------------------------------------------------------------

struct Net_Info
{
  int                     lag_tolerance;                                // Lag tolerance

  int                     workservice;
  char                    hostname[0x40];                            // Name for hosting session

  int                     numservice;                              // How many we found
  char                    servicename[MAXSERVICE][NETNAMESIZE];    // Names of services

  int                     numsession;                              // How many we found
  char                    sessionname[MAXSESSION][NETNAMESIZE];    // Names of sessions

  int                     numplayer;                               // How many we found
  char                    playername[MAXNETPLAYER][NETNAMESIZE];   // Names of machines

  /* PORT
  LPVOID                  lpconnectionbuffer[MAXSERVICE];          // Location of service info
  LPGUID                  lpsessionguid[MAXSESSION];               // GUID for joining
  DPID                    playerid[MAXNETPLAYER];                  // Player ID
  DPID                    selfid;                                     // Player ID
  */

  bool                    on;                      // Try to connect?

  Net_Info()
  {
    lag_tolerance = 3;                                // Lag tolerance
    numservice = 0;
    numsession = 0;
    numplayer  = 0;
    on         = false;
  };

};

extern Net_Info GNet;

//--------------------------------------------------------------------------------------------
struct NetMsg
{
  char                    name[0x40];             // Name for messages
  bool                    mode;                 // Input text from keyboard?
  Uint8                   delay;                // For slowing down input
  int                     write;                // The cursor position
  int                     writemin;             // The starting cursor position
  char                    buffer[MESSAGESIZE];  // The input message
};

extern NetMsg GNetMsg;

