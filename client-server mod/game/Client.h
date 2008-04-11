/* Egoboo - Client.h
 * Basic skeleton for the client portion of a client-server architecture,
 * this is totally not in use yet.
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

#ifndef egoboo_Client_h
#define egoboo_Client_h

#include "enet/enet.h"
#include "network.h"
#include "egobootypedef.h"
#include "egoboo.h"

typedef struct ClientState_t
{
  // my network
  NET_STATE  * ns;
  ENetHost   * clientHost;
  ENetPeer   * gamePeer;
  Uint32       gameID;
  char         server_names[MAXPLAYER];
  bool_t       amClient;

  int    numsession;                              // How many we found
  char   sessionname[MAXSESSION][NETNAMESIZE];    // Names of sessions

  // local states
  Uint16 msg_timechange;
  int    statdelay;
  Uint8  statclock;                        // For stat regeneration
  bool_t seeinvisible;
  bool_t seekurse;
  bool_t allpladead;                       // Has everyone died?
  bool_t waiting;                          // Has everyone talked to the host?
  int    selectedPlayer;
  int    selectedModule;
  bool_t logged_on;

  // latch info
  bool_t timelatchvalid[MAXCHR][MAXLAG];
  Uint32 timelatchstamp[MAXCHR][MAXLAG];
  float  timelatchx[MAXCHR][MAXLAG];       // Timed latches
  float  timelatchy[MAXCHR][MAXLAG];       //
  Uint8  timelatchbutton[MAXCHR][MAXLAG];  //
  Uint32 numplatimes;
  Uint32 nexttimestamp;                    // Expected timestamp

  // client information about remote modules
  MOD_INFO    rem_mod[MAXMODULE];
  MOD_SUMMARY rem_modtxt;

 // client information about desired local module
  char      req_host[MAXCAPNAMESIZE];
  MOD_INFO  req_mod;
  MOD_STATE req_modstate;

  PACKET_REQUEST request_buffer[16];
} ClientState_t;


// Globally accesible client state
extern ClientState_t AClientState;

void cl_reset(ClientState_t * cs);
void cl_resetTimeLatches(ClientState_t * cs, Sint32 ichr);
void cl_bufferLatches(ClientState_t * cs);

bool_t cl_init(ClientState_t * cs, NET_STATE * ns);
void   cl_shutDown();
void   cl_frameStep();

bool_t cl_connectHost(ClientState_t * cs, const char* hostname);
bool_t cl_disconnectHost(ClientState_t * cs);
 
void     cl_talkToHost(ClientState_t * cs);
retval_t cl_joinGame(ClientState_t *, const char *hostname);
bool_t   cl_quitGame(ClientState_t * cs);

void   cl_unbufferLatches(ClientState_t * cs);
void   cl_request_module_images(ClientState_t * cs);
bool_t cl_load_module_images(ClientState_t * cs);

bool_t cl_dispatchPackets(ClientState_t * cs);
bool_t cl_handlePacket(ClientState_t * cs, ENetEvent *event);

// Much more to come...

//int  cl_connectToServer(...);
//int  cl_loadModule(...);
#endif // include guard
