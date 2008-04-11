/* Egoboo - Server.h
 * Basic skeleton for the server portion of a client-server architecture,
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

#ifndef egoboo_Server_h
#define egoboo_Server_h

#include "egobootypedef.h"
#include "Network.h"
#include "egoboo.h"
#include "enet/enet.h"

typedef struct ServerState_t
{
  // my network
  NET_STATE   * ns;
  bool_t        amHost;
  ENetHost    * serverHost;
  NetPlayerInfo playerInfo[MAXNETPLAYER];

  // server states
  bool_t ready;                             // Ready to hit the Start Game button?
  Uint32 rand_idx;

  // a copy of all the character latches
  float  latchx[MAXCHR];
  float  latchy[MAXCHR];
  Uint8  latchbutton[MAXCHR];

  // the buffered latches that have been stored on the server
  bool_t timelatchvalid[MAXCHR][MAXLAG];
  Uint32 timelatchstamp[MAXCHR][MAXLAG];
  float  timelatchx[MAXCHR][MAXLAG];
  float  timelatchy[MAXCHR][MAXLAG];
  Uint8  timelatchbutton[MAXCHR][MAXLAG];
  Uint32 numplatimes;
  Uint32 nexttimestamp;                    // Expected timestamp

  // local module parameters
  int         selectedModule;
  MOD_INFO    loc_mod[MAXMODULE];
  MOD_SUMMARY loc_modtxt;

  // Selected module parameters. Store them here just in case.
  MOD_INFO  mod;
  MOD_STATE modstate;

  int    num_loaded;                       //
  int    num_logon, num_connect;
  char   playerName[MAXNETPLAYER][NETNAMESIZE];      // Names of machines
  char   playerAddress[MAXNETPLAYER][NETNAMESIZE];   // Net address of machines

  PACKET_REQUEST request_buffer[16];
} ServerState_t;

// Globally accessible server state
extern ServerState_t AServerState;

bool_t sv_init(ServerState_t * ss, NET_STATE * ns);
void   sv_shutDown(ServerState_t * ss);
void   sv_frameStep(ServerState_t * ss);

void sv_reset(ServerState_t * ss);
void sv_talkToRemotes(ServerState_t * ss);
void sv_bufferLatches(ServerState_t * ss);
void sv_unbufferLatches(ServerState_t * ss);
void sv_resetTimeLatches(ServerState_t * ss, Sint32 ichr);

bool_t sv_hostGame(ServerState_t * ss);
bool_t sv_unhostGame(ServerState_t * ss);
void   sv_letPlayersJoin(ServerState_t * ss);

bool_t sv_dispatchPackets(ServerState_t * ss);
bool_t sv_handlePacket(ServerState_t * ss, ENetEvent *event);

// More to come...
// int  sv_beginSinglePlayer(...)
// int  sv_beginMultiPlayer(...)
// int  sv_loadModule(...)

#endif // include guard
