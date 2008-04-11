/* Egoboo - Server.c
 * This code is not currently in use.
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

#include "Server.h"
#include "Network.h"
#include "Log.h"
#include "egoboo.h"
#include <enet/enet.h>
#include <assert.h>

ServerState_t AServerState;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t sv_init(ServerState_t * ss, NET_STATE * ns)
{
  int cnt;

  ss->ns = ns;

  ss->serverHost = NULL;
  ss->amHost = bfalse;
  ss->ready  = bfalse;
  ss->selectedModule = -1;
  ss->num_logon = 0;
  for(cnt=0; cnt<MAXNETPLAYER; cnt++)
  {
    ss->playerName[cnt][0] = 0x00;
    ss->playerAddress[cnt][0] = 0x00;
  }

  for(cnt=0; cnt<MAXMODULE; cnt++)
  {
    ss->loc_mod[cnt].host[0]     = 0x00;
    ss->loc_mod[cnt].texture_idx = MAXMODULE;
    ss->loc_mod[cnt].is_hosted   = bfalse;
    ss->loc_mod[cnt].is_verified = bfalse;
  }

  memset(ss->playerInfo, 0, sizeof(NetPlayerInfo) * MAXNETPLAYER);

  sv_reset(ss);
  return btrue;
}

//--------------------------------------------------------------------------------------------
void sv_frameStep(ServerState_t * ss)
{

}

//--------------------------------------------------------------------------------------------
void sv_bufferLatches(ServerState_t * ss)
{
  // ZZ> This function buffers the character latches
  Uint32 uiTime, ichr;

  if (!ss->amHost) return;

  if (wldframe > STARTTALK)
  {
    uiTime = wldframe + CData.lag;

    // Now pretend the host got the packet...
    uiTime = uiTime & LAGAND;
    for (ichr = 0; ichr < MAXPLAYER; ichr++)
    {
      ss->timelatchbutton[ichr][uiTime] = ss->latchbutton[ichr];
      ss->timelatchx[ichr][uiTime]      = ((Sint32)(ss->latchx[ichr]*SHORTLATCH)) / SHORTLATCH;
      ss->timelatchy[ichr][uiTime]      = ((Sint32)(ss->latchy[ichr]*SHORTLATCH)) / SHORTLATCH;
    }
    ss->numplatimes++;
  };

};

//--------------------------------------------------------------------------------------------
void sv_talkToRemotes(ServerState_t * ss)
{
  // ZZ> This function sends the character data to all the remote machines
  Uint32 uiTime, ichr;
  PACKET egopkt;
  int i,cnt,stamp;

  if(!ss->amHost || !ss->ns->networkon) return;

  if (wldframe > STARTTALK)
  {
                                  // The stamp

    // check all possible lags
    for(i=0; i<MAXLAG; i++)
    {
      stamp  = wldframe - i;
      if(stamp<0) continue;

      uiTime = stamp & LAGAND;

      // Send a message to all players
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, TO_REMOTE_LATCH);                       // The message header
      packet_addUnsignedInt(&egopkt, stamp);  

      // test all player latches...
      cnt = 0;
      for (ichr=0; ichr<MAXCHR; ichr++)
      {
        // do not look at characters that aren't on
        if (!ChrList[ichr].on) continue;

        // do not look at invalid latches
        if(!ss->timelatchvalid[ichr][uiTime]) continue;

        // do not send "future" latches
        if(ss->timelatchstamp[ichr][uiTime] <= uiTime)
        {
          packet_addUnsignedShort(&egopkt, ichr);                                // The character index
          packet_addUnsignedByte(&egopkt, ss->timelatchbutton[ichr][uiTime]);         // Player button states
          packet_addSignedShort(&egopkt, ss->timelatchx[ichr][uiTime]*SHORTLATCH);    // Player motion
          packet_addSignedShort(&egopkt, ss->timelatchy[ichr][uiTime]*SHORTLATCH);    // Player motion

          ss->timelatchvalid[ichr][uiTime] = bfalse;

          cnt++;
        }
      };

      // Send the packet
      if(cnt>0)
      {
        net_sendPacketToAllPlayers(ss, &egopkt);
      };
    }


  }
}

//--------------------------------------------------------------------------------------------
//void sv_letPlayersJoin(ServerState_t * ss)
//{
//  // ZZ> This function finds all the players in the game
//  ENetEvent event;
//  char hostName[64];
//
//  // Check all pending events for players joining
//  while (enet_host_service(ss->serverHost, &event, 0) > 0)
//  {
//    switch (event.type)
//    {
//    case ENET_EVENT_TYPE_CONNECT:
//      // Look up the hostname the player is connecting from
//      enet_address_get_host(&event.peer->address, hostName, 64);
//
//      if(ss->num_connect >= MAXNETPLAYER)
//      {
//        net_logf(ss->ns, "INFO: sv_letPlayersJoin: Too many connections. Connection from %s:%u refused\n", hostName, event.peer->address.port);
//        enet_peer_disconnect_now(event.peer);
//      }
//      else
//      {
//        net_logf(ss->ns, "INFO: sv_letPlayersJoin: A new player connected from %s:%u\n", hostName, event.peer->address.port);
//
//        for(cnt=0; cnt<MAXNETPLAYER; cnt++)
//        {
//          if(0x00 == ss->playerAddress[cnt][0] )
//          {
//            // found an empty connection
//            ss->num_connect++;
//            strncpy(ss->playerAddress[cnt], hostName, sizeof(ss->playeraddress))
//            event.peer->data = &(ss->playerInfo[cnt]);
//            break;
//          }
//        }
//      }
//      break;
//
//    case ENET_EVENT_TYPE_RECEIVE:
//      net_logf(ss->ns, "INFO: sv_letPlayersJoin: Recieved a packet when we weren't expecting it...\n");
//      net_logf(ss->ns, "INFO: \tIt GCamera.e from %x:%u\n", event.peer->address.host, event.peer->address.port);
//
//      // clean up the packet
//      enet_packet_destroy(event.packet);
//      break;
//
//    case ENET_EVENT_TYPE_DISCONNECT:
//      enet_address_get_host(&event.peer->address, hostName, 64);
//
//      if(ss->num_connect > 0)
//      {
//        net_logf(ss->ns, "INFO: sv_letPlayersJoin: A new player is disconnecting from %s:%u\n", hostName, event.peer->address.port);
//
//        for(cnt=0; cnt<MAXNETPLAYER; cnt++)
//        {
//          if(0 == strncmp(ss->playerAddress[cnt][0], hostName, sizeof(ss->PlaList[cnt].yeraddress)) )
//          {
//            // found the connection
//            ss->num_connect--;
//            ss->playerAddress[cnt][0] = 0x00;
//            break;
//          }
//        }
//      }
//
//      // Reset that peer's data
//      event.peer->data = NULL;
//    }
//  }
//}

//--------------------------------------------------------------------------------------------
bool_t sv_hostGame(ServerState_t * ss)
{
  // ZZ> This function tries to host a new session
  ENetAddress address;

  // Run in solo mode
  if (!ss->ns->networkon) return btrue; 

  // Try to create a new session
  address.host = ENET_HOST_ANY;
  address.port = NET_EGOBOO_SERVER_PORT;

  net_logf(ss->ns, "INFO: sv_hostGame: Creating game on port 0x%08x\n", NET_EGOBOO_SERVER_PORT);
  ss->serverHost = enet_host_create(&address, MAXPLAYER, 0, 0);
  if (ss->serverHost == NULL)
  {
    net_logf(ss->ns, "INFO: sv_hostGame: Could not create network connection!\n");
    return bfalse;
  }

  // Try to create a host player
  // return create_player(btrue);
  ss->amHost = (NULL != ss->serverHost);

  // Moved from net_sayHello because there they cause a race issue
  ss->num_loaded = 0;

  return ss->amHost;
}

//--------------------------------------------------------------------------------------------
void net_sendModuleInfoToAllPlayers(ServerState_t * ss)
{
  PACKET egopkt;

  // tell the which module we are hosting
  net_startNewPacket(&egopkt);
  packet_addUnsignedShort(&egopkt, TO_REMOTE_MODULE);
  packet_addUnsignedInt(&egopkt, ss->modstate.seed);
  packet_addString(&egopkt, ss->mod.loadname);
  net_sendPacketToAllPlayersGuaranteed(ss, &egopkt);
};

//--------------------------------------------------------------------------------------------
void net_sendModuleInfoToOnePlayer(ServerState_t * ss, ENetPeer * peer)
{
  PACKET egopkt;

  // tell the which module we are hosting
  net_startNewPacket(&egopkt);
  packet_addUnsignedShort(&egopkt, TO_REMOTE_MODULE);
  packet_addUnsignedInt(&egopkt, ss->modstate.seed);
  packet_addString(&egopkt, ss->mod.loadname);
  net_sendPacketToOnePlayerGuaranteed(peer, &egopkt);
};


//--------------------------------------------------------------------------------------------
bool_t sv_handlePacket(ServerState_t * ss, ENetEvent *event)
{
  Uint16 header;
  STRING filename;   // also used for reading various strings
  int filesize, newfilesize, fileposition;
  char newfile;
  Uint16 character;
  FILE *file;
  PACKET egopkt;
  NET_PACKET netpkt;
  Uint32 stamp, time;

  bool_t retval = bfalse;

  // do some error trapping
  if(!ss->amHost) return bfalse;

  // send some log info
  net_logf(ss->ns, "INFO: sv_handlePacket: Processing ");

  // rewind the packet
  packet_startReading(&netpkt, event->packet);
  header = packet_readUnsignedShort(&netpkt);

  // process out messages
  switch (header)
  {

  case TO_HOST_LOGON:
    {
      bool_t found;
      int cnt, tnc;
      net_logf(ss->ns, "INFO: TO_HOST_LOGON\n");

      // read the logon name
      packet_readString(&netpkt, filename, 255);
      found = bfalse;
      for(cnt=0, tnc=0; cnt<MAXNETPLAYER && tnc<ss->num_logon; cnt++)
      {
        if(0x00 != ss->playerName[cnt][0]) tnc++;

        if(0==strncmp(ss->playerName[cnt], filename, sizeof(ss->playerName[cnt])))
        {
          found = btrue;
          break;
        } 
      }

      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, TO_REMOTE_LOGON);
      if(found)
      {
        packet_addUnsignedByte(&egopkt, bfalse);       // deny the logon
        packet_addUnsignedByte(&egopkt, (Uint8)-1 );
      }
      else
      {
        packet_addUnsignedByte(&egopkt, btrue);            // accept the logon
        packet_addUnsignedByte(&egopkt, ss->num_logon);   // tell the client their place in the queue
      }
      net_sendPacketToOnePlayerGuaranteed(event->peer, &egopkt);

      if(!found)
      {
        // add the login name to the list
        strncpy(ss->playerName[cnt], filename, sizeof(ss->playerName[cnt]));
        ss->num_logon++;
      }
    }
    retval = btrue;
    break;

  case TO_HOST_LOGOFF:
    {
      bool_t found;
      Uint8  index;

      net_logf(ss->ns, "INFO: TO_HOST_LOGOFF\n");

      // read the logon name
      packet_readString(&netpkt, filename, 255);
      index = packet_readUnsignedByte(&netpkt);

      found = bfalse;
      if( index<MAXNETPLAYER && ss->num_logon>0 &&
          0==strncmp(ss->playerName[index], filename, sizeof(ss->playerName[index])) )
      {
        ss->playerName[index][0] = 0x00;
        ss->num_logon--;
      };

      if(found)
      {
        net_startNewPacket(&egopkt);
        packet_addUnsignedShort(&egopkt, TO_REMOTE_LOGOFF);
        packet_addUnsignedByte(&egopkt, index);
        net_sendPacketToOnePlayerGuaranteed(event->peer, &egopkt);
      }
    }
    retval = btrue;
    break;

  case TO_HOST_MODULE:
    if(-1 == ss->selectedModule)
    {
      // tell the client that no module has been selected
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, TO_REMOTE_MODULEBAD);
      net_sendPacketToOnePlayerGuaranteed(event->peer, &egopkt);
    }
    else
    {
      // give the client the module info
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, TO_REMOTE_MODULEINFO);
      packet_addString(&egopkt, ss->mod.rank);
      packet_addString(&egopkt, ss->mod.longname);
      packet_addString(&egopkt, ss->mod.loadname);
      packet_addUnsignedByte(&egopkt, ss->mod.importamount);
      packet_addUnsignedByte(&egopkt, ss->mod.allowexport);
      packet_addUnsignedByte(&egopkt, ss->mod.minplayers);
      packet_addUnsignedByte(&egopkt, ss->mod.maxplayers);
      packet_addUnsignedByte(&egopkt, ss->mod.monstersonly);
      packet_addUnsignedByte(&egopkt, ss->mod.minplayers);
      packet_addUnsignedByte(&egopkt, ss->mod.rts_control);
      packet_addUnsignedByte(&egopkt, ss->mod.respawnvalid);
      net_sendPacketToOnePlayerGuaranteed(event->peer, &egopkt);
    };
    break;


  case TO_HOST_MODULEOK:
    net_logf(ss->ns, "INFO: TO_HOST_MODULEOK\n");

    // the client is acknowledging that it actually has the module the server is hosting

    retval = btrue;
    break;

  case TO_HOST_MODULEBAD:
    net_logf(ss->ns, "INFO: TO_HOST_MODULEBAD\n");

    // copy the selected module from the server to a client that doesn't have the module
    if(ss->selectedModule != -1)
    {
      // send the directory
      net_copyDirectoryToOnePlayer(ss->ns, event->peer, ss->mod.loadname, ss->mod.loadname);
  
      // tell it to try to find the module again
      net_sendModuleInfoToOnePlayer(ss, event->peer);

      retval = btrue;
    }
    break;

  case TO_HOST_LATCH:
    net_logf(ss->ns, "INFO: TO_HOST_LATCH\n");

    stamp = packet_readUnsignedInt(&netpkt);
    time  = stamp & LAGAND;

    // the client is sending a latch
    while (packet_remainingSize(&netpkt) > 0)
    {
      character = packet_readUnsignedShort(&netpkt);
      ss->timelatchstamp[character][time]  = stamp;
      ss->timelatchvalid[character][time]  = btrue;
      ss->timelatchbutton[character][time] = packet_readUnsignedByte(&netpkt);
      ss->timelatchx[character][time]      = (float)packet_readSignedShort(&netpkt) / (float)SHORTLATCH;
      ss->timelatchy[character][time]      = (float)packet_readSignedShort(&netpkt) / (float)SHORTLATCH;
    }
    retval = btrue;
    break;

  case TO_HOST_IM_LOADED:
    net_logf(ss->ns, "INFO: TO_HOST_IMLOADED\n");

    // the client has finished loading the module and is waiting for the other players and for the server
    // excessive connection requests have already been handled at this point

    ss->num_loaded++;
    if (ss->num_loaded >= ss->mod.minplayers && ss->num_loaded <= ss->mod.maxplayers)
    {
      // Let the games begin...
      ss->ready = btrue;
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, TO_REMOTE_START);
      net_sendPacketToAllPlayersGuaranteed(ss, &egopkt);
    }

    retval = btrue;
    break;

  case TO_HOST_RTS:
    net_logf(ss->ns, "INFO: TO_HOST_RTS\n");

    // RTS no longer really supported

    /*whichorder = get_empty_order();
    if(whichorder < MAXORDER)
    {
    // Add the order on the host machine
    cnt = 0;
    while(cnt < MAXSELECT)
    {
    who = packet_readUnsignedByte();
    GOrder.who[whichorder][cnt] = who;
    cnt++;
    }
    what = packet_readUnsignedInt();
    when = wldframe + CData.GOrder.lag;
    GOrder.what[whichorder] = what;
    GOrder.when[whichorder] = when;


    // Send the order off to everyone else
    net_startNewPacket(&egopkt);
    packet_addUnsignedShort(TO_REMOTE_RTS);
    cnt = 0;
    while(cnt < MAXSELECT)
    {
    packet_addUnsignedByte(GOrder.who[whichorder][cnt]);
    cnt++;
    }
    packet_addUnsignedInt(what);
    packet_addUnsignedInt(when);
    net_sendPacketToAllPlayersGuaranteed();
    }*/
    retval = btrue;
    break;

  case TO_HOST_FILE:
    net_logf(ss->ns, "INFO: TO_HOST_FILE\n");

    // the client is sending us a file

    packet_readString(&netpkt, filename, 255);
    newfilesize = packet_readUnsignedInt(&netpkt);

    // Change the size of the file if need be
    newfile = 0;
    file = fopen(filename, "rb");
    if (file)
    {
      fseek(file, 0, SEEK_END);
      filesize = ftell(file);
      fclose(file);

      if (filesize != newfilesize)
      {
        // Destroy the old file
        newfile = 1;
      }
    }
    else
    {
      newfile = 1;
    }

    if (newfile)
    {
      // file must be created.  Write zeroes to the file to do it
      numfile++;
      file = fopen(filename, "wb");
      if (file)
      {
        filesize = 0;
        while (filesize < newfilesize)
        {
          fputc(0, file);
          filesize++;
        }
        fclose(file);
      }
    }

    // Go to the position in the file and copy data
    fileposition = packet_readUnsignedInt(&netpkt);
    file = fopen(filename, "r+b");

    if (fseek(file, fileposition, SEEK_SET) == 0)
    {
      while (packet_remainingSize(&netpkt) > 0)
      {
        fputc(packet_readUnsignedByte(&netpkt), file);
      }
    }
    fclose(file);
    retval = btrue;
    break;

  case TO_HOST_DIR:
    net_logf(ss->ns, "INFO: TO_HOST_DIR\n");

    // the client is creating a dir on the server

    packet_readString(&netpkt, filename, 255);
    fs_createDirectory(filename);
    retval = btrue;
    break;

  case TO_HOST_FILESENT:
    net_logf(ss->ns, "INFO: TO_HOST_FILESENT\n");

    // the client is telling us to expect a certain number of files

    numfileexpected += packet_readUnsignedInt(&netpkt);
    numplayerrespond++;
    retval = btrue;
    break;
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
void sv_unbufferLatches(ServerState_t * ss)
{
  // ZZ> This function sets character latches based on player input to the host
  Uint32 cnt, uiTime;
  GAME_STATE * gs = ss->ns->gs;
  Uint32 sv_randie = gs->randie_index;

  if(!ss->amHost) return;

  // Copy the latches
  uiTime = wldframe & LAGAND;
  for (cnt = 0; cnt < MAXCHR; cnt++)
  {
    if(!ChrList[cnt].on) continue;
    if(!ss->timelatchvalid[cnt][uiTime]) continue;
    if(INVALID_TIMESTAMP == ss->timelatchstamp[cnt][uiTime]) continue;

    ss->latchx[cnt]      = ss->timelatchx[cnt][uiTime];
    ss->latchy[cnt]      = ss->timelatchy[cnt][uiTime];
    ss->latchbutton[cnt] = ss->timelatchbutton[cnt][uiTime];

    // Let players respawn
    if (gs->modstate.respawnvalid && (ChrList[cnt].latchbutton & LATCHBUTTONRESPAWN))
    {
      if (!ChrList[cnt].alive)
      {
        respawn_character(cnt, &sv_randie);
        TeamList[ChrList[cnt].team].leader = cnt;
        ChrList[cnt].alert |= ALERTIFCLEANEDUP;

        // Cost some experience for doing this...  Never lose a level
        ChrList[cnt].experience = ChrList[cnt].experience * EXPKEEP;
      }
      ss->latchbutton[cnt] &= ~LATCHBUTTONRESPAWN;
    }

  }
  ss->numplatimes--;
}

//--------------------------------------------------------------------------------------------
void sv_reset(ServerState_t * ss)
{
  int cnt;
  if(NULL==ss) return;

  for(cnt = 0; cnt<MAXCHR; cnt++)
  {
    sv_resetTimeLatches(ss,cnt);
  };

  ss->nexttimestamp = INVALID_TIMESTAMP;
  ss->numplatimes   = STARTTALK + 1;
};

//--------------------------------------------------------------------------------------------
void sv_resetTimeLatches(ServerState_t * ss, Sint32 ichr)
{
  int cnt;

  if(NULL==ss) return;
  if(ichr<0 || ichr>=MAXCHR) return;

  ss->latchx[ichr]      = 0;
  ss->latchy[ichr]      = 0;
  ss->latchbutton[ichr] = 0;

  for(cnt=0; cnt < MAXLAG; cnt++)
  {
    ss->timelatchvalid[ichr][cnt]  = bfalse;
    ss->timelatchstamp[ichr][cnt]  = INVALID_TIMESTAMP;
    ss->timelatchx[ichr][cnt]      = 0;
    ss->timelatchy[ichr][cnt]      = 0;
    ss->timelatchbutton[ichr][cnt] = 0;
  }
};

//--------------------------------------------------------------------------------------------
bool_t sv_unhostGame(ServerState_t * ss)
{
  PACKET egopkt;
  int cnt, numPeers;

  if(NULL!=ss->serverHost) return bfalse;

  // send logoff messages to all the clients
  net_startNewPacket(&egopkt);
  packet_addUnsignedShort(&egopkt, TO_REMOTE_LOGOFF);
  net_sendPacketToAllPlayersGuaranteed(ss, &egopkt);

  // Disconnect the peers
  numPeers = ss->serverHost->peerCount;
  for (cnt = 0;cnt < numPeers;cnt++)
  {
#ifdef ENET11
    enet_peer_disconnect(&ss->serverHost->peers[cnt], 0);
#else
    enet_peer_disconnect(&ss->serverHost->peers[cnt]);
#endif
  }

  // Allow up to 5 seconds for peers to drop
  cnt = 0;
  while(ss->serverHost->peerCount>0 && cnt<250)
  {
    SDL_Delay(20);
    cnt++;
  };

  // Forcefully disconnect any peers leftover
  for (cnt=0; cnt<ss->serverHost->peerCount; cnt++)
  {
    enet_peer_reset(&ss->serverHost->peers[cnt]);
  }

  net_logf(ss->ns, "INFO: sv_unhostGame: Disconnecting Server from network.\n");
  enet_host_destroy(ss->serverHost);
  ss->serverHost = NULL;

  return btrue;
};


//--------------------------------------------------------------------------------------------
bool_t sv_dispatchPackets(ServerState_t * ss)
{
  ENetEvent event;
  char hostName[64];
  int cnt;
  PACKET_REQUEST * prequest;
  size_t copy_size;

  if(NULL==ss || NULL==ss->serverHost) return bfalse;

  while (0 != enet_host_service(ss->serverHost, &event, 0))
  {
    switch (event.type)
    {
    case ENET_EVENT_TYPE_RECEIVE:
      prequest = net_checkRequest(ss->request_buffer, 16, &event);
      if( NULL != prequest && prequest->waiting )
      {
        // we have received a packet that someone is waiting for

        // copy the data from the packet to a waiting buffer
        copy_size = MIN(event.packet->dataLength, prequest->data_size);
        memcpy(prequest->data, event.packet->data, copy_size);
        prequest->data_size = copy_size;

        // tell the other thread that we're done
        prequest->received = btrue;
      }
      else if(!net_handlePacket(ss->ns, &event))
      {
        net_logf(ss->ns, "WARNING: sv_dispatchPackets - Unhandled packet");
      }

      enet_packet_destroy(event.packet);
      break;

    case ENET_EVENT_TYPE_CONNECT:

      // Look up the hostname the player is connecting from
      enet_address_get_host(&event.peer->address, hostName, 64);

      if(ss->num_connect >= MAXNETPLAYER)
      {
        net_logf(ss->ns, "INFO: sv_dispatchPackets - Too many connections. Connection from %s:%u refused\n", hostName, event.peer->address.port);

#ifdef ENET11
        enet_peer_disconnect(event.peer, 0);
#else
        enet_peer_disconnect(event.peer);
#endif
      }
      else
      {
        net_logf(ss->ns, "INFO: sv_dispatchPackets - A new connection from %s:%u\n", hostName, event.peer->address.port);

        for(cnt=0; cnt<MAXNETPLAYER; cnt++)
        {
          if(0x00 == ss->playerAddress[cnt][0] )
          {
            // found an empty connection
            ss->num_connect++;
            strncpy(ss->playerAddress[cnt], hostName, sizeof(ss->playerAddress[cnt]));
            event.peer->data = &(ss->playerInfo[cnt]);
            break;
          }
        }
      }
      break;

    case ENET_EVENT_TYPE_DISCONNECT:
      enet_address_get_host(&event.peer->address, hostName, 64);

      if(ss->num_connect > 0)
      {
        net_logf(ss->ns, "INFO: sv_dispatchPackets - A new player is disconnecting from %s:%u\n", hostName, event.peer->address.port);

        for(cnt=0; cnt<MAXNETPLAYER; cnt++)
        {
          if(0 == strncmp(ss->playerAddress[cnt], hostName, sizeof(ss->playerAddress[cnt])) )
          {
            // found the connection
            ss->num_connect--;
            ss->playerAddress[cnt][0] = 0x00;
            ss->playerName[cnt][0]    = 0x00;
            break;
          }
        }
      }

      // Reset that peer's data
      event.peer->data = NULL;
      break;
    }
  }

  return btrue;
};