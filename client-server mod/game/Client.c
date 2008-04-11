/* Egoboo - Client.c
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

#include "Client.h"
#include "Network.h"
#include "Log.h"
#include "egoboo.h"
#include <enet/enet.h>
#include <assert.h>

// Global ClientState instance
ClientState_t AClientState = {0,0,0};

//--------------------------------------------------------------------------------------------
bool_t cl_init(ClientState_t * cs, NET_STATE * ns)
{
  int cnt;

  cs->ns = ns;
  cs->clientHost  = NULL;
  cs->gamePeer  = NULL;
  cs->amClient  = bfalse;
  cs->logged_on = bfalse;
  cs->selectedPlayer = -1;
  cs->selectedModule = -1;
  cs->numsession     = 0;

  for(cnt=0; cnt<MAXNETPLAYER; cnt++)
  {
    cs->rem_mod[cnt].host[0]     = 0x00;
    cs->rem_mod[cnt].texture_idx = MAXMODULE;
    cs->rem_mod[cnt].is_hosted   = bfalse;
    cs->rem_mod[cnt].is_verified = bfalse;
  };

  cl_reset(cs);

  return btrue;
};

//--------------------------------------------------------------------------------------------
void cl_frameStep()
{

}

//--------------------------------------------------------------------------------------------
bool_t cl_startClient(ClientState_t * cs)
{
  ENetAddress address;

  if (!cs->ns->networkon) return bfalse;
  if(NULL != cs->clientHost) return btrue;

  net_logf(cs->ns, "INFO: cl_startClient: Starting the client ... ");

  // Create my host thingamabober
  // TODO: Should I limit client bandwidth here?
  address.host = ENET_HOST_ANY;
  address.port = NET_EGOBOO_CLIENT_PORT;
  cs->clientHost = enet_host_create(&address, 1, 0, 0);
  if (NULL == cs->clientHost)
  {
    net_logf(cs->ns, "INFO: Failed!\n");
  }
  else
  {
    net_logf(cs->ns, "INFO: Succeeded!\n");
  };

  return (NULL != cs->clientHost);
};

//--------------------------------------------------------------------------------------------
bool_t cl_disconnectHost(ClientState_t * cs)
{
  if(NULL==cs) return bfalse;

  if(NULL == cs->clientHost)
  {
    cs->gamePeer = NULL;
  }
  else if(NULL != cs->gamePeer)
  {
    int cnt;

    enet_peer_disconnect(cs->gamePeer);

    // Wait for up to 5 seconds for the disconnection attempt to succeed
    cnt = 0;
    while(cnt<500 && ENET_PEER_STATE_DISCONNECTED != cs->gamePeer->state)
    {
      SDL_Delay(10);
      cnt++;
    }

    if(ENET_PEER_STATE_DISCONNECTED == cs->gamePeer->state)
    {
      cs->gamePeer = NULL;
    }
    else
    {
      // force the thing to reset immediately
      enet_peer_disconnect_now(cs->gamePeer);
      cs->gamePeer = NULL;
    }
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t cl_connectHost(ClientState_t * cs, const char* hostname)
{
  // ZZ> This function tries to connect onto a server

  ENetAddress address;
  int cnt;

  if(NULL == cs->clientHost)
  {
    if(!cl_startClient(cs)) return bfalse;
  }
  else 
  {
    cl_disconnectHost(cs);
  }

  // Now connect to the remote host
  net_logf(cs->ns, "INFO: cl_connectHost: Attempting to connect to %s:0x%08x\n", hostname, NET_EGOBOO_SERVER_PORT);
  enet_address_set_host(&address, hostname);
  address.port = NET_EGOBOO_SERVER_PORT;
  cs->gamePeer = enet_host_connect(cs->clientHost, &address, NET_EGOBOO_NUM_CHANNELS);
  if (NULL==cs->gamePeer)
  {
    net_logf(cs->ns, "INFO: cl_connectHost: Cannot open channel to host.\n");
    return bfalse;
  }

  // Wait for up to 5 seconds for the connection attempt to succeed
  cnt = 0;
  while(cnt<500 && ENET_PEER_STATE_CONNECTED != cs->gamePeer->state)
  {
    SDL_Delay(10);
    cnt++;
  }

  return (ENET_PEER_STATE_CONNECTED == cs->gamePeer->state);
};


//--------------------------------------------------------------------------------------------
retval_t cl_joinGame(ClientState_t * cs, const char* hostname)
{
  // ZZ> This function tries to join one of the sessions we found
  int cnt;
  PACKET      egopkt;
  STREAM      stream;
  retval_t wait_return;
  Uint8 buffer[1024];

  if ( !cs->ns->networkon ) return bfalse;
  if ( !cl_startClient(cs) ) return bfalse;

  if(!cl_connectHost(cs, hostname)) return rv_error;

  net_startNewPacket(&egopkt);
  packet_addUnsignedShort(&egopkt, TO_HOST_LOGON);           // try to logon
  packet_addString(&egopkt, CData.net_messagename);             // logon name
  net_sendPacketToHost(cs, &egopkt);

  // wait up to 5 seconds for the client to respond to the request
  wait_return = net_waitForPacket(cs->request_buffer, 16, cs->gamePeer, 5000, buffer, sizeof(buffer), TO_REMOTE_LOGON, NULL);
  cl_disconnectHost(cs);
  if(rv_fail == wait_return || rv_error == wait_return) return rv_error;

  stream_startRaw(&stream, buffer, sizeof(buffer));
  assert(TO_REMOTE_LOGON == stream_readUnsignedShort(&stream));

  if(bfalse == stream_readUnsignedByte(&stream))
  {
    //login refused
    cl_disconnectHost(cs);
    cs->gameID = (Uint32)(-1);
    return rv_fail;
  };

  cs->gameID = stream_readUnsignedByte(&stream);

  return rv_succeed;

};


//--------------------------------------------------------------------------------------------
void cl_talkToHost(ClientState_t * cs)
{
  // ZZ> This function sends the latch packets to the host machine
  Uint8 player;
  PACKET egopkt;
  GAME_STATE * gs = cs->ns->gs;


  if(!cs->amClient || !cs->ns->networkon) return;


  // Start talkin'
  if (wldframe > STARTTALK && numlocalpla>0)
  {
    Uint32 ichr;
    Uint32 stamp = wldframe;
    Uint32 time = (stamp + 1) & LAGAND;

    net_startNewPacket(&egopkt);
    packet_addUnsignedShort(&egopkt, TO_HOST_LATCH);     // The message header
    packet_addUnsignedInt(&egopkt, stamp);

    for (player = 0; player < MAXPLAYER; player++)
    {
      // Find the local players
      if (!PlaList[player].valid || INPUTNONE==PlaList[player].device) continue;

      ichr = PlaList[player].index;
      packet_addUnsignedShort(&egopkt, ichr);                                   // The character index
      packet_addUnsignedByte(&egopkt, cs->timelatchbutton[ichr][time]);         // Player button states
      packet_addSignedShort(&egopkt, cs->timelatchx[ichr][time]*SHORTLATCH);    // Player motion
      packet_addSignedShort(&egopkt, cs->timelatchy[ichr][time]*SHORTLATCH);    // Player motion
    }

    // Send it to the host
    net_sendPacketToHost(cs, &egopkt);
  }
}

//--------------------------------------------------------------------------------------------
void cl_unbufferLatches(ClientState_t * cs)
{
  // ZZ> This function sets character latches based on player input to the host
  int    cnt;
  Uint32 uiTime, stamp;
  Sint32 dframes;

  // Copy the latches
  stamp = wldframe;
  uiTime  = stamp & LAGAND;
  for (cnt = 0; cnt < MAXCHR; cnt++)
  {
    if(!ChrList[cnt].on) continue;
    if(!cs->timelatchvalid[cnt][uiTime]) continue;
    if(INVALID_TIMESTAMP == cs->timelatchstamp[cnt][uiTime]) continue;

    dframes = (float)(cs->timelatchvalid[cnt][uiTime] - stamp);

    // copy the data over
    ChrList[cnt].latchx      = cs->timelatchx[cnt][uiTime];
    ChrList[cnt].latchy      = cs->timelatchy[cnt][uiTime];
    ChrList[cnt].latchbutton = cs->timelatchbutton[cnt][uiTime];

    // set the data to invalid
    cs->timelatchvalid[cnt][uiTime] = bfalse;
    cs->timelatchstamp[cnt][uiTime] = INVALID_TIMESTAMP;
  }
  cs->numplatimes--;
}



//--------------------------------------------------------------------------------------------
bool_t cl_handlePacket(ClientState_t * cs, ENetEvent *event)
{
  Uint16 header;
  STRING filename;   // also used for reading various strings
  int filesize, newfilesize, fileposition;
  char newfile;
  Uint32 stamp;
  int uiTime;
  FILE *file;
  PACKET egopkt;
  NET_PACKET netpkt;
  bool_t retval = bfalse;

  // do some error trapping
  if(cs->amClient) return bfalse;

  // send some log info
  net_logf(cs->ns, "INFO: cl_handlePacket: Processing ");

  // rewind the packet
  packet_startReading(&netpkt, event->packet);
  header = packet_readUnsignedShort(&netpkt);

  // process our messages
  switch (header)
  {

  case TO_REMOTE_KICK:
    // we were kicked from the server
    cs->logged_on = bfalse;
    cs->waiting   = btrue;

    // shut down the connection gracefully on this end
    cl_disconnectHost(cs);

    retval = btrue;
    break;

  case TO_REMOTE_LOGON:
    // someone has sent a message saying we are logged on

    if( (cs->gamePeer->address.host == event->peer->address.host) &&
        (cs->gamePeer->address.port == event->peer->address.port) )
    {
      cs->logged_on = btrue;
    }
    retval = btrue;
    break;

  case TO_REMOTE_FILESENT:
    net_logf(cs->ns, "INFO: TO_REMOTE_FILESENT\n");

    // the server is telling us to expect a certain number of files to download

    numfileexpected += packet_readUnsignedInt(&netpkt);
    numplayerrespond++;

    retval = btrue;
    break;



  case TO_REMOTE_MODULE:
    net_logf(cs->ns, "INFO: TO_REMOTE_MODULE\n");

    // the server is telling us which module it is hosting

    if (cs->waiting)
    {
      cs->req_modstate.seed = packet_readUnsignedInt(&netpkt);
      packet_readString(&netpkt, filename, 255);
      strcpy(pickedmodule, filename);

      // Check to see if the module exists
      cs->selectedModule = find_module(pickedmodule, cs->rem_mod, MAXMODULE);
      if (cs->selectedModule == -1)
      {
        net_startNewPacket(&egopkt);
        packet_addUnsignedShort(&egopkt, TO_HOST_MODULEBAD);
        net_sendPacketToHostGuaranteed(cs, &egopkt);
      }
      else
      {
        // Make ourselves ready
        cs->waiting = bfalse;

        // Tell the host we're ready
        net_startNewPacket(&egopkt);
        packet_addUnsignedShort(&egopkt, TO_HOST_MODULEOK);
        net_sendPacketToHostGuaranteed(cs, &egopkt);
      }
    }
    retval = btrue;
    break;

  case TO_REMOTE_START:
    net_logf(cs->ns, "INFO: TO_REMOTE_START\n");

    // the everyone has responded that they are ready. Stop waiting.

    cs->waiting = bfalse;

    retval = btrue;
    break;

  case TO_REMOTE_RTS:
    net_logf(cs->ns, "INFO: TO_REMOTE_RTS\n");

    // the host is sending an RTS order

    /*  whichorder = get_empty_order();
    if(whichorder < MAXORDER)
    {
    // Add the order on the remote machine
    cnt = 0;
    while(cnt < MAXSELECT)
    {
    who = packet_readUnsignedByte(event->packet);
    GOrder.who[whichorder][cnt] = who;
    cnt++;
    }
    what = packet_readUnsignedInt(event->packet);
    when = packet_readUnsignedInt(event->packet);
    GOrder.what[whichorder] = what;
    GOrder.when[whichorder] = when;
    }*/

    retval = btrue;
    break;

  case TO_REMOTE_FILE:
    net_logf(cs->ns, "INFO: TO_REMOTE_FILE\n");

    // the server is sending us a file

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
    if (file)
    {
      if (fseek(file, fileposition, SEEK_SET) == 0)
      {
        while (packet_remainingSize(&netpkt) > 0)
        {
          fputc(packet_readUnsignedByte(&netpkt), file);
        }
      }
      fclose(file);
    }

    retval = btrue;
    break;

  case TO_REMOTE_DIR:
    net_logf(cs->ns, "INFO: TO_REMOTE_DIR\n");

    // the server is telling us to create a directory on our machine

    packet_readString(&netpkt, filename, 255);
    fs_createDirectory(filename);

    retval = btrue;
    break;

  case TO_REMOTE_LATCH:
    net_logf(cs->ns, "INFO: TO_REMOTE_LATCH\n");

    // the server is sending us a latch

    stamp  = packet_readUnsignedInt(&netpkt);
    uiTime = stamp & LAGAND;

    if (INVALID_TIMESTAMP == cs->nexttimestamp)
    {
      cs->nexttimestamp = stamp;
    }
    if (stamp < cs->nexttimestamp)
    {
      net_logf(cs->ns, "WARNING: net_handlePacket: OUT OF ORDER PACKET\n");
      outofsync = btrue;
    }
    if (stamp <= wldframe)
    {
      net_logf(cs->ns, "WARNING: net_handlePacket: LATE PACKET\n");
      outofsync = btrue;
    }
    if (stamp > cs->nexttimestamp)
    {
      net_logf(cs->ns, "WARNING: net_handlePacket: MISSED PACKET\n");
      cs->nexttimestamp = stamp;  // Still use it
      outofsync = btrue;
    }
    if (stamp == cs->nexttimestamp)
    {
      Uint16 ichr;

      // Remember that we got it
      cs->numplatimes++;

      for(ichr=0; ichr<MAXCHR; ichr++)
      {
        cs->timelatchstamp[ichr][uiTime] = INVALID_TIMESTAMP;
        cs->timelatchvalid[ichr][uiTime] = bfalse;
      };

      // Read latches for each player sent
      while (packet_remainingSize(&netpkt) > 0)
      {
        ichr = packet_readUnsignedShort(&netpkt);
        cs->timelatchstamp[ichr][uiTime]  = stamp;
        cs->timelatchvalid[ichr][uiTime]  = btrue;
        cs->timelatchbutton[ichr][uiTime] = packet_readUnsignedByte(&netpkt);
        cs->timelatchx[ichr][uiTime]      = (float)packet_readSignedShort(&netpkt) / (float)SHORTLATCH;
        cs->timelatchy[ichr][uiTime]      = (float)packet_readSignedShort(&netpkt) / (float)SHORTLATCH;
      };

      cs->nexttimestamp = stamp + 1;
    }

    retval = btrue;
    break;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void cl_reset(ClientState_t * cs)
{
  int cnt;
  if(NULL==cs) return;

  for(cnt = 0; cnt<MAXCHR; cnt++)
  {
    cl_resetTimeLatches(cs, cnt);
  };

  cs->nexttimestamp = INVALID_TIMESTAMP;
  cs->numplatimes   = STARTTALK + 1;
};


//--------------------------------------------------------------------------------------------
void cl_resetTimeLatches(ClientState_t * cs, Sint32 ichr)
{
  int cnt;

  if(NULL==cs) return;
  if(ichr<0 || ichr>=MAXCHR) return;

  for(cnt=0; cnt < MAXLAG; cnt++)
  {
    cs->timelatchvalid[ichr][cnt]  = bfalse;
    cs->timelatchstamp[ichr][cnt]  = INVALID_TIMESTAMP;
    cs->timelatchx[ichr][cnt]      = 0;
    cs->timelatchy[ichr][cnt]      = 0;
    cs->timelatchbutton[ichr][cnt] = 0;
  }
};

//--------------------------------------------------------------------------------------------
void cl_bufferLatches(ClientState_t * cs)
{
  // ZZ> This function buffers the player data
  Uint32 player, stamp, uiTime, ichr;

  stamp = wldframe + 1;
  uiTime = stamp & LAGAND;

  for (player = 0; player < MAXPLAYER; player++)
  {
    if (!PlaList[player].valid) continue;

    ichr = PlaList[player].index;

    cs->timelatchvalid[ichr][uiTime]  = btrue;
    cs->timelatchstamp[ichr][uiTime]  = stamp;
    cs->timelatchbutton[ichr][uiTime] = PlaList[player].latchbutton;
    cs->timelatchx[ichr][uiTime]      = ((Sint32)(PlaList[player].latchx * SHORTLATCH)) / SHORTLATCH;
    cs->timelatchy[ichr][uiTime]      = ((Sint32)(PlaList[player].latchy * SHORTLATCH)) / SHORTLATCH;
  }

  cs->numplatimes++;

};

//--------------------------------------------------------------------------------------------
bool_t cl_quitGame(ClientState_t * cs)
{
  if(NULL!=cs->clientHost) return bfalse;

  net_logf(cs->ns, "INFO: close_session: Disconnecting Client from network.\n");
  enet_host_destroy(cs->clientHost);
  cs->clientHost = NULL;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t cl_dispatchPackets(ClientState_t * cs)
{
  ENetEvent event;
  PACKET_REQUEST * prequest;
  size_t copy_size;

  if(NULL==cs || NULL==cs->clientHost) return bfalse;

  while (cs->clientHost && 0 != enet_host_service(cs->clientHost, &event, 0))
  {
    switch (event.type)
    {
    case ENET_EVENT_TYPE_RECEIVE:
      prequest = net_checkRequest(cs->request_buffer, 16, &event);
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
      else if(!net_handlePacket(cs->ns, &event))
      {
        net_logf(cs->ns, "WARNING: cl_dispatchPackets - Unhandled packet");
      }

      enet_packet_destroy(event.packet);
      break;

    case ENET_EVENT_TYPE_CONNECT:
      // what is this????
      if (event.peer->data != 0)
      {
        NetPlayerInfo *info = event.peer->data;
        net_logf(cs->ns, "WARNING: cl_dispatchPackets - Player %d tried to connedt to a client...\n", info->playerSlot);
      }
      else
      {
        net_logf(cs->ns, "WARNING: cl_dispatchPackets - Someone tried to connect to a client...\n");
      }
      break;

    case ENET_EVENT_TYPE_DISCONNECT:
      // Is this a player disconnecting, or just a rejected connection
      // from above?
      if (event.peer->data != 0)
      {
        NetPlayerInfo *info = event.peer->data;

        net_logf(cs->ns, "WARNING: cl_dispatchPackets - Kicked from the server? %d \n", info->playerSlot);
      }
      else
      {
        net_logf(cs->ns, "WARNING: cl_dispatchPackets - Kicked from the server?\n");
      }
      break;
    }
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
void cl_request_module_images(ClientState_t * cs)
{
  // BB > send a request to the server to doenload the module images
  //      images will not be downloaded unless the local CRC does not match the
  //      remote CRC (or the file does not exist locally)


  PACKET egopkt;
  STRING tmpstring;
  int cnt;

  for(cnt=0; cnt<MAXMODULE; cnt++)
  {
    if(0x00 == cs->rem_mod[cnt].host[0]) continue;

    if(cl_connectHost(cs, cs->rem_mod[cnt].host))
    {
      snprintf( tmpstring, sizeof(tmpstring), "%s,%s/%s/%s", CData.modules_dir, cs->rem_mod[cnt].loadname, CData.basicdat_dir, CData.title_bitmap);
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, NET_REQUEST_FILE);

      // add the local file name
      packet_addString(&egopkt, tmpstring);

      // Send the directory and file info so that the names can be localized correctly
      // on the other machine
      packet_addFString(&egopkt, "@%s/%s/@%s/@%s", 
        get_config_string_name(&CData, &CData.modules_dir), 
        cs->rem_mod[cnt].loadname, 
        get_config_string_name(&CData, &CData.basicdat_dir), 
        get_config_string_name(&CData, &CData.title_bitmap));

      // terminate with an empty string
      packet_addString(&egopkt, "");
      net_sendPacketToHost(cs, &egopkt);

      cl_disconnectHost(cs);
    };
  };
  
};

//--------------------------------------------------------------------------------------------
bool_t cl_load_module_images(ClientState_t * cs)
{
  // BB> This function loads the title image(s) for the modules that the client
  //     is browsing

  int cnt;
  bool_t retval = btrue;

  for(cnt=0; cnt<MAXMODULE; cnt++)
  {
    // check to see if the module has been loaded
    if(MAXMODULE==cs->rem_mod[cnt].texture_idx)
    {
      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s/%s/%s/%s", CData.modules_dir, cs->rem_mod[cnt].loadname, CData.gamedat_dir, CData.title_bitmap);
      cs->rem_mod[cnt].texture_idx = load_one_module_image(cnt, CStringTmp1);
      if(MAXMODULE==cs->rem_mod[cnt].texture_idx) retval = bfalse;
    }
  }


  return retval;
}
