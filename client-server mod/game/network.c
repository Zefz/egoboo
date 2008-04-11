/* Egoboo - network.c
 * Shuttles bits across the network, using Enet.  Networked play doesn't
 * really work at the moment.
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

#include "network.h"
#include "Client.h"
#include "Server.h"
#include "Log.h"
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

#define MAX_NET_LOG_MESSAGE 1024

NET_STATE ANetState;

// File transfer variables & structures
typedef struct NetFileTransfer
{
  char sourceName[NET_MAX_FILE_NAME];
  char destName[NET_MAX_FILE_NAME];
  ENetPeer *target;
}NetFileTransfer;

// File transfer queue
NetFileTransfer net_transferStates[NET_MAX_FILE_TRANSFERS];
int net_numFileTransfers = 0;
int net_fileTransferHead = 0; // Queue indices
int net_fileTransferTail = 0;
int net_waitingForXferAck = 0;

// Receiving files
NetFileTransfer net_receiveState;

static SDL_Thread * net_thread = NULL;

void net_sendPacketToPeer(ENetPeer *peer, PACKET * egop);
void net_sendPacketToPeerGuaranteed(ENetPeer *peer, PACKET * egop);

static PACKET_REQUEST * net_prepareWaitForPacket(PACKET_REQUEST * request_buff, size_t request_buff_sz, ENetPeer * peer, Uint32 timeout, Uint8 * buffer, size_t buffer_sz, Uint16 packet_type);
static retval_t net_loopWaitForPacket(PACKET_REQUEST * prequest, Uint32 granularity, size_t * data_size);


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
retval_t net_calculateCRC(char * filename, Uint32 seed, Uint32 * pCRC)
{
  Uint32 tmpint, tmpCRC;
  FILE * pfile = NULL;

  if(NULL==pCRC && NULL==filename || 0x00 == filename[0]) return bfalse;

  pfile = fopen(filename, "b");
  if(NULL==pfile) 
    return rv_error;

  tmpCRC = seed;
  while(!feof(pfile))
  {
    if( 0!= fread(&tmpint, sizeof(Uint32), 1, pfile) )
    {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
      tmpint = SDL_Swap32(tmpint);
#endif
      tmpCRC = ((tmpCRC + tmpint) * 0x0019660DL) + 0x3C6EF35FL;
    };
  }
  fclose(pfile);

  *pCRC = tmpCRC;
  return rv_succeed;
}


//--------------------------------------------------------------------------------------------
void net_KickOnePlayer(ENetPeer * peer)
{
  PACKET egopkt;

  // tell the player that he's gone
  net_startNewPacket(&egopkt);
  packet_addUnsignedShort(&egopkt, TO_REMOTE_KICK);
  net_sendPacketToPeerGuaranteed(peer, &egopkt);

  // disconnect him rudely
  enet_peer_reset(peer);
};

//--------------------------------------------------------------------------------------------
retval_t net_checkCRC(NET_STATE * ns, ENetPeer * peer, const char * source, Uint32 seed, Uint32 CRC)
{
  Uint8  buffer[1024];
  PACKET egopkt;
  Uint32 cnt;
  Uint32* usp;
  retval_t wait_return;

  // request a CRC calculation from the peer for the file source
  net_startNewPacket(&egopkt);
  packet_addUnsignedShort(&egopkt, NET_CHECK_CRC);
  packet_addUnsignedInt(&egopkt, seed);
  packet_addString(&egopkt, source);
  net_sendPacketToPeerGuaranteed(peer, &egopkt);


  // Wait for the client to acknowledge the request and to process the message (handled in the network thread)
  // BUT, we don't want an infinite loop, so make sure we don't wait for more than 5 seconds for
  // crc_acknowledged and 30 seconds for wating_for_crc

  // wait up to 5 seconds for the client to respond to the request
  wait_return = net_waitForPacket(ns->request_buffer, 16, peer, 5000, buffer, sizeof(buffer), NET_ACKNOWLEDGE_CRC, NULL);
  if(rv_fail == wait_return || rv_error == wait_return) 
  {
    net_KickOnePlayer(peer);
    return rv_error;
  };

  // After the request is acknowledged, wait up to 30 seconds for the CRC to be returned
  wait_return = net_waitForPacket(ns->request_buffer, 16, peer, 30000, buffer, sizeof(buffer), NET_SEND_CRC, NULL);
  if(rv_fail == wait_return || rv_error == wait_return) 
  {
    net_KickOnePlayer(peer);
    return rv_error;
  };

  usp = (Uint32 *)&buffer[2];  

  return (ENET_NET_TO_HOST_32(*usp) == CRC) ? rv_succeed : rv_fail;
};

//--------------------------------------------------------------------------------------------
static void net_writeLogMessage(FILE * logFile, const char *format, va_list args)
{
  static char logBuffer[MAX_NET_LOG_MESSAGE];

  if (logFile != NULL)
  {
    vsnprintf(logBuffer, MAX_NET_LOG_MESSAGE - 1, format, args);
    fputs(logBuffer, logFile);
    fflush(logFile);
  }
}

//--------------------------------------------------------------------------------------------
void net_logf(NET_STATE * ns, const char *format, ...)
{
  va_list args;

  va_start(args, format);
  net_writeLogMessage(ns->logfile,format, args);
  va_end(args);
}

//--------------------------------------------------------------------------------------------
void Net_Quit(void)
{
  if(NULL!=net_thread)
  {
    SDL_KillThread(net_thread);
    net_thread = NULL;
  }
};

//--------------------------------------------------------------------------------------------
void packet_addUnsignedByte(PACKET * egop, Uint8 uc)
{
  // ZZ> This function appends an Uint8 to the packet
  Uint8* ucp;
  ucp = (Uint8*)(&egop->buffer[egop->head]);
  *ucp = uc;
  egop->head += 1;
  egop->size += 1;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedByte(PACKET * egop, Sint8 sc)
{
  // ZZ> This function appends a Sint8 to the packet
  Sint8* scp;
  scp = (Sint8*)(&egop->buffer[egop->head]);
  *scp = sc;
  egop->head += 1;
  egop->size += 1;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedShort(PACKET * egop, Uint16 us)
{
  // ZZ> This function appends an Uint16 to the packet
  Uint16* usp;
  usp = (Uint16*)(&egop->buffer[egop->head]);

  *usp = ENET_HOST_TO_NET_16(us);
  egop->head += 2;
  egop->size += 2;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedShort(PACKET * egop, Sint16 ss)
{
  // ZZ> This function appends a Sint16 to the packet
  Sint16* ssp;
  ssp = (Sint16*)(&egop->buffer[egop->head]);

  *ssp = ENET_HOST_TO_NET_16(ss);

  egop->head += 2;
  egop->size += 2;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedInt(PACKET * egop, Uint32 ui)
{
  // ZZ> This function appends an Uint32 to the packet
  Uint32* uip;
  uip = (Uint32*)(&egop->buffer[egop->head]);

  *uip = ENET_HOST_TO_NET_32(ui);

  egop->head += 4;
  egop->size += 4;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedInt(PACKET * egop, Sint32 si)
{
  // ZZ> This function appends a Sint32 to the packet
  Sint32* sip;
  sip = (Sint32*)(&egop->buffer[egop->head]);

  *sip = ENET_HOST_TO_NET_32(si);

  egop->head += 4;
  egop->size += 4;
}

//--------------------------------------------------------------------------------------------
void packet_addFString(PACKET * egop, char *format, ...)
{
  char stringbuffer[COPYSIZE];
  va_list args;

  va_start(args, format);
  vsnprintf(stringbuffer, sizeof(stringbuffer)-1, format, args);
  va_end(args);

  packet_addString(egop, stringbuffer);
}

//--------------------------------------------------------------------------------------------
void packet_addString(PACKET * egop, char *string)
{
  // ZZ> This function appends a null terminated string to the packet
  char* cp;
  char cTmp;
  int cnt;

  cnt = 0;
  cTmp = 1;
  cp = (char*)(&egop->buffer[egop->head]);
  while (cTmp != 0)
  {
    cTmp = string[cnt];
    *cp = cTmp;
    cp += 1;
    egop->head += 1;
    egop->size += 1;
    cnt++;
  }
}

static bool_t stream_reset(STREAM * pwrapper)
{
  if(NULL==pwrapper) return bfalse;

  memset(pwrapper, 0, sizeof(STREAM));
  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t stream_startFile(STREAM * pwrapper, FILE * pfile)
{
  fpos_t pos;

  if(NULL==pwrapper || NULL==pfile || feof(pfile)) return bfalse;

  stream_reset(pwrapper);

  pwrapper->pfile        = pfile;

  // set the read position in the file
  pos = ftell(pfile);
  pwrapper->readLocation = pos;

  // measure the file length
  fseek(pfile, 0, SEEK_END);
  pwrapper->data_size    = ftell(pfile);

  // reset the file stream
  fseek(pfile, pos, SEEK_SET);

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t stream_startRaw(STREAM * pwrapper, Uint8 * buffer, size_t buffer_size)
{
  if(NULL==pwrapper || NULL==buffer || 0==buffer_size) return bfalse;

  stream_reset(pwrapper);
  pwrapper->data         = buffer;
  pwrapper->data_size    = buffer_size;
  pwrapper->readLocation = 0;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t stream_startLocal(STREAM * pwrapper, PACKET * pegopkt)
{
  if(NULL==pwrapper || NULL==pegopkt) return bfalse;

  stream_reset(pwrapper);
  pwrapper->data         = pegopkt->buffer;
  pwrapper->data_size    = pegopkt->size;
  pwrapper->readLocation = 0;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t stream_startENet(STREAM * pwrapper, ENetPacket * packet)
{
  if(NULL==pwrapper || NULL==packet) return bfalse;

  stream_reset(pwrapper);
  pwrapper->data         = packet->data;
  pwrapper->data_size    = packet->dataLength;
  pwrapper->readLocation = 0;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t stream_startRemote(STREAM * pwrapper, NET_PACKET * pnetpkt)
{
  if(NULL==pwrapper || NULL==pnetpkt) return bfalse;

  stream_reset(pwrapper);
  pwrapper->data         = pnetpkt->pkt->data;
  pwrapper->data_size    = pnetpkt->pkt->dataLength;
  pwrapper->readLocation = 0;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t stream_done(STREAM * pwrapper)
{
  return stream_reset(pwrapper);
}

//--------------------------------------------------------------------------------------------
bool_t stream_readString(STREAM * p, char *buffer, int maxLen)
{
  // ZZ> This function reads a NULL terminated string from the packet
  Uint32 copy_length;

  if(NULL==p) return bfalse;
  if(p->data_size==0 && maxLen==0) return btrue;
  if(p->data_size==0 && maxLen>0)
  {
    buffer[0] = 0x00;
    return btrue;
  };

  copy_length = MIN(maxLen, p->data_size);
  strncpy(buffer, p->data + p->readLocation, copy_length);
  copy_length = MIN(copy_length, strlen(buffer) + 1);
  p->readLocation += copy_length;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Uint8 stream_readUnsignedByte(STREAM * p)
{
  // ZZ> This function reads an Uint8 from the packet
  Uint8 uc;
  uc = (Uint8)(p->data[p->readLocation]);
  p->readLocation += sizeof(Uint8);
  return uc;
}

//--------------------------------------------------------------------------------------------
Sint8 stream_readSignedByte(STREAM * p)
{
  // ZZ> This function reads a Sint8 from the packet
  Sint8 sc;
  sc = (Sint8)(p->data[p->readLocation]);
  p->readLocation += sizeof(Sint8);
  return sc;
}

//--------------------------------------------------------------------------------------------
Uint16 stream_readUnsignedShort(STREAM * p)
{
  // ZZ> This function reads an Uint16 from the packet
  Uint16 us;
  Uint16* usp;
  usp = (Uint16*)(&p->data[p->readLocation]);

  us = ENET_NET_TO_HOST_16(*usp);

  p->readLocation += sizeof(Uint16);
  return us;
}

//--------------------------------------------------------------------------------------------
Uint16 stream_peekUnsignedShort(STREAM * p)
{
  // ZZ> This function reads an Uint16 from the packet
  Uint16 us;
  Uint16* usp;
  usp = (Uint16*)(&p->data[p->readLocation]);

  us = ENET_NET_TO_HOST_16(*usp);

  return us;
}

//--------------------------------------------------------------------------------------------
Sint16 stream_readSignedShort(STREAM * p)
{
  // ZZ> This function reads a Sint16 from the packet
  Sint16 ss;
  Sint16* ssp;
  ssp = (Sint16*)(&p->data[p->readLocation]);

  ss = ENET_NET_TO_HOST_16(*ssp);

  p->readLocation += sizeof(Sint16);
  return ss;
}

//--------------------------------------------------------------------------------------------
Uint32 stream_readUnsignedInt(STREAM * p)
{
  // ZZ> This function reads an Uint32 from the packet
  Uint32 ui;
  Uint32* uip;
  uip = (Uint32*)(&p->data[p->readLocation]);

  ui = ENET_NET_TO_HOST_32(*uip);

  p->readLocation += sizeof(Uint32);
  return ui;
}

//--------------------------------------------------------------------------------------------
Sint32 stream_readSignedInt(STREAM * p)
{
  // ZZ> This function reads a Sint32 from the packet
  Sint32 si;
  Sint32* sip;
  sip = (Sint32*)(&p->data[p->readLocation]);

  si = ENET_NET_TO_HOST_32(*sip);

  p->readLocation += sizeof(Sint32);
  return si;
}

//--------------------------------------------------------------------------------------------
size_t stream_remainingSize(STREAM * p)
{
  // ZZ> This function tells if there's still data left in the packet
  return p->data_size - p->readLocation;
}






//--------------------------------------------------------------------------------------------
void packet_startReading(NET_PACKET * p, ENetPacket * enpkt)
{
  p->pkt = enpkt;
  stream_startENet(&p->wrapper, enpkt);
}

//--------------------------------------------------------------------------------------------
void packet_doneReading(NET_PACKET * p)
{
  p->pkt = NULL;
  stream_done(&p->wrapper);
}
//--------------------------------------------------------------------------------------------
bool_t packet_readString(NET_PACKET * p, char *buffer, int maxLen)
{
  // ZZ> This function reads a null terminated string from the packet

  if(NULL==p) return bfalse;

  return stream_readString(&(p->wrapper), buffer, maxLen);
}

//--------------------------------------------------------------------------------------------
Uint8 packet_readUnsignedByte(NET_PACKET * p)
{
  // ZZ> This function reads an Uint8 from the packet
  if(NULL==p) return 0;

  return stream_readUnsignedByte(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
Sint8 packet_readSignedByte(NET_PACKET * p)
{
  // ZZ> This function reads a Sint8 from the packet
  if(NULL==p) return 0;

  return stream_readSignedByte(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
Uint16 packet_readUnsignedShort(NET_PACKET * p)
{
  // ZZ> This function reads an Uint16 from the packet
  if(NULL==p) return 0;

  return stream_readUnsignedShort(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
Uint16 packet_peekUnsignedShort(NET_PACKET * p)
{
  // ZZ> This function reads an Uint16 from the packet
  if(NULL==p) return 0;

  return stream_peekUnsignedShort(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
Sint16 packet_readSignedShort(NET_PACKET * p)
{
  // ZZ> This function reads a Sint16 from the packet
  if(NULL==p) return 0;

  return stream_readSignedShort(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
Uint32 packet_readUnsignedInt(NET_PACKET * p)
{
  // ZZ> This function reads an Uint32 from the packet
  if(NULL==p) return 0;

  return stream_readUnsignedInt(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
Sint32 packet_readSignedInt(NET_PACKET * p)
{
  // ZZ> This function reads a Sint32 from the packet
  if(NULL==p) return 0;

  return stream_readSignedInt(&(p->wrapper));
}

//--------------------------------------------------------------------------------------------
size_t packet_remainingSize(NET_PACKET * p)
{
  // ZZ> This function tells if there's still data left in the packet
  if(NULL==p) return 0;

  return stream_remainingSize(&(p->wrapper));
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void net_startNewPacket(PACKET * egop)
{
  // ZZ> This function starts building a network packet
  egop->head = 0;
  egop->size = 0;
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHost(ClientState_t * cs, PACKET * egop)
{
  // ZZ> This function sends a packet to the host
  ENetPacket *packet = enet_packet_create(egop->buffer, egop->size, 0);
  enet_peer_send(cs->gamePeer, NET_UNRELIABLE_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayers(ServerState_t * ss, PACKET * egop)
{
  // ZZ> This function sends a packet to all the players
  ENetPacket *packet;

  if(NULL==ss->serverHost) return;

  packet = enet_packet_create(egop->buffer, egop->size, 0);
  enet_host_broadcast(ss->serverHost, NET_UNRELIABLE_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHostGuaranteed(ClientState_t * cs, PACKET * egop)
{
  // ZZ> This function sends a packet to the host
  ENetPacket *packet;

  if(NULL==cs->gamePeer) return;

  packet = enet_packet_create(egop->buffer, egop->size, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(cs->gamePeer, NET_UNRELIABLE_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayersGuaranteed(ServerState_t * ss, PACKET * egop)
{
  // ZZ> This function sends a packet to all the players
  ENetPacket *packet;

  if(NULL==ss->serverHost) return;

  packet = enet_packet_create(egop->buffer, egop->size, ENET_PACKET_FLAG_RELIABLE);
  enet_host_broadcast(ss->serverHost, NET_GUARANTEED_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToOnePlayerGuaranteed(ENetPeer * peer, PACKET * egop)
{
  // ZZ> This function sends a packet to one of the players
  ENetPacket *packet;

  if(NULL==peer) return;

  packet = enet_packet_create(egop->buffer, egop->size, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, NET_GUARANTEED_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeer(ENetPeer *peer, PACKET * egop)
{
  // JF> This function sends a packet to a given peer
  ENetPacket *packet;

  if(NULL==peer) return;

  packet = enet_packet_create(egop->buffer, egop->size, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, NET_UNRELIABLE_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeerGuaranteed(ENetPeer *peer, PACKET * egop)
{
  // JF> This funciton sends a packet to a given peer, with guaranteed delivery
    ENetPacket *packet;

  if(NULL==peer) return;

  packet = enet_packet_create(egop->buffer, egop->size, 0);
  enet_peer_send(peer, NET_GUARANTEED_CHANNEL, packet);
}

//------------------------------------------------------------------------------
retval_t net_copyFileToOnePlayer(ServerState_t * ss, ENetPeer * peer, char *source, char *dest)
{
  // JF> This function queues up files to send to all the hosts.
  //     TODO: Deal with having to send to up to MAXPLAYER players...
  NetFileTransfer *state;
  retval_t retval;
  Uint32 CRC;

  // check the CRC
  net_calculateCRC(source, ss->rand_idx, &CRC);
  retval = net_checkCRC(ss->ns, peer, source, ss->rand_idx, CRC);
  if(rv_succeed != retval) return retval;

  // the CRCs are different. Send the file.
  if (net_numFileTransfers < NET_MAX_FILE_TRANSFERS)
  {
    // net_fileTransferTail should already be pointed at an open
    // slot in the queue.
    state = &(net_transferStates[net_fileTransferTail]);
    assert(state->sourceName[0] == 0);

    // Just do the first player for now
    state->target = peer;
    strncpy(state->sourceName, source, NET_MAX_FILE_NAME);
    strncpy(state->destName, dest, NET_MAX_FILE_NAME);

    // advance the tail index
    net_numFileTransfers++;
    net_fileTransferTail++;
    if (net_fileTransferTail >= NET_MAX_FILE_TRANSFERS)
    {
      net_fileTransferTail = 0;
    }

    if (net_fileTransferTail == net_fileTransferHead)
    {
      net_logf(ss->ns, "WARNING: net_copyFileToAllPlayers: Warning!  Queue tail caught up with the head!\n");
    }
  }

  return rv_succeed;
}


//------------------------------------------------------------------------------
void net_copyFileToAllPlayers(ServerState_t * ss, char *source, char *dest)
{
  // JF> This function queues up files to send to all the hosts.
  //     TODO: Deal with having to send to up to MAXPLAYER players...
  NetFileTransfer *state;

  if (net_numFileTransfers < NET_MAX_FILE_TRANSFERS)
  {
    // net_fileTransferTail should already be pointed at an open
    // slot in the queue.
    state = &(net_transferStates[net_fileTransferTail]);
    assert(state->sourceName[0] == 0);

    // Just do the first player for now
    state->target = &ss->serverHost->peers[0];
    strncpy(state->sourceName, source, NET_MAX_FILE_NAME);
    strncpy(state->destName, dest, NET_MAX_FILE_NAME);

    // advance the tail index
    net_numFileTransfers++;
    net_fileTransferTail++;
    if (net_fileTransferTail >= NET_MAX_FILE_TRANSFERS)
    {
      net_fileTransferTail = 0;
    }

    if (net_fileTransferTail == net_fileTransferHead)
    {
      net_logf(ss->ns, "WARNING: net_copyFileToAllPlayers: Warning!  Queue tail caught up with the head!\n");
    }
  }
}

//------------------------------------------------------------------------------
void net_copyFileToAllPlayersOld(NET_STATE * ns, char *source, char *dest)
{
  // ZZ> This function copies a file on the host to every remote computer.
  //     Packets are sent in chunks of COPYSIZE bytes.  The MAX file size
  //     that can be sent is 2 Megs ( TOTALSIZE ).
  FILE* fileread;
  int packetend, packetstart;
  int filesize;
  int fileisdir;
  char cTmp;
  PACKET egopkt;

  if(!ns->networkon  || !ns->ss->amHost || NULL==ns->ss->serverHost) return;

  net_logf(ns, "INFO: net_copyFileToAllPlayers: %s, %s\n", source, dest);

  fileisdir = fs_fileIsDirectory(source);
  if (fileisdir)
  {
    net_startNewPacket(&egopkt);
    packet_addUnsignedShort(&egopkt, TO_REMOTE_DIR);
    packet_addString(&egopkt, dest);
    net_sendPacketToAllPlayersGuaranteed(ns->ss, &egopkt);
  }
  else
  {
    fileread = fopen(source, "rb");
    if (fileread)
    {
      fseek(fileread, 0, SEEK_END);
      filesize = ftell(fileread);
      fseek(fileread, 0, SEEK_SET);
      if (filesize > 0 && filesize < TOTALSIZE)
      {
        packetend = 0;
        packetstart = 0;
        numfilesent++;

        net_startNewPacket(&egopkt);
        packet_addUnsignedShort(&egopkt, TO_REMOTE_FILE);
        packet_addString(&egopkt, dest);
        packet_addUnsignedInt(&egopkt, filesize);
        packet_addUnsignedInt(&egopkt, packetstart);
        while (packetstart < filesize)
        {
          // This will probably work...
          //fread((egop->buffer + egop->head), COPYSIZE, 1, fileread);

          // But I'll leave it alone for now
          fscanf(fileread, "%c", &cTmp);

          packet_addUnsignedByte(&egopkt, cTmp);
          packetend++;
          packetstart++;
          if (packetend >= COPYSIZE)
          {
            // Send off the packet
            net_sendPacketToAllPlayersGuaranteed(ns->ss, &egopkt);
            enet_host_flush(ns->ss->serverHost);

            // Start on the next 4K
            packetend = 0;
            net_startNewPacket(&egopkt);
            packet_addUnsignedShort(&egopkt, TO_REMOTE_FILE);
            packet_addString(&egopkt, dest);
            packet_addUnsignedInt(&egopkt, filesize);
            packet_addUnsignedInt(&egopkt, packetstart);
          }
        }
        // Send off the packet
        net_sendPacketToAllPlayersGuaranteed(ns->ss, &egopkt);
      }
      fclose(fileread);
    }
  }
}

//------------------------------------------------------------------------------
void net_copyFileToHost(NET_STATE * ns, char *source, char *dest)
{
  NetFileTransfer *state;

  // JF> New function merely queues up a new file to be sent

  // If this is the host, just copy the file locally
  if (ns->ss->amHost)
  {
    // Simulate a network transfer
    if (fs_fileIsDirectory(source))
    {
      fs_createDirectory(dest);
    }
    else
    {
      fs_copyFile(source, dest);
    }
    return;
  }

  if (net_numFileTransfers < NET_MAX_FILE_TRANSFERS)
  {
    // net_fileTransferTail should already be pointed at an open
    // slot in the queue.
    state = &(net_transferStates[net_fileTransferTail]);
    assert(state->sourceName[0] == 0);

    state->target = ns->cs->gamePeer;
    strncpy(state->sourceName, source, NET_MAX_FILE_NAME);
    strncpy(state->destName, dest, NET_MAX_FILE_NAME);

    // advance the tail index
    net_numFileTransfers++;
    net_fileTransferTail++;
    if (net_fileTransferTail >= NET_MAX_FILE_TRANSFERS)
    {
      net_fileTransferTail = 0;
    }

    if (net_fileTransferTail == net_fileTransferHead)
    {
      net_logf(ns, "WARNING: net_copyFileToHost: Warning!  Queue tail caught up with the head!\n");
    }
  }
}

//------------------------------------------------------------------------------
void net_copyFileToHostOld(NET_STATE * ns, char *source, char *dest)
{
  // ZZ> This function copies a file on the remote to the host computer.
  //     Packets are sent in chunks of COPYSIZE bytes.  The MAX file size
  //     that can be sent is 2 Megs ( TOTALSIZE ).
  FILE* fileread;
  int packetend, packetstart;
  int filesize;
  int fileisdir;
  char cTmp;
  PACKET egopkt;

  net_logf(ns, "INFO: net_copyFileToHost: \n");
  fileisdir = fs_fileIsDirectory(source);
  if (ns->ss->amHost)
  {
    // Simulate a network transfer
    if (fileisdir)
    {
      net_logf(ns, "INFO: Creating local directory %s\n", dest);
      fs_createDirectory(dest);
    }
    else
    {
      net_logf(ns, "INFO: Copying local file %s --> %s\n", source, dest);
      fs_copyFile(source, dest);
    }
  }
  else
  {
    if (fileisdir)
    {
      net_logf(ns, "INFO: Creating directory on host: %s\n", dest);
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, TO_HOST_DIR);
      packet_addString(&egopkt, dest);
//   net_sendPacketToAllPlayersGuaranteed(ns->ss, &egopkt);
      net_sendPacketToHost(ns->cs, &egopkt);
    }
    else
    {
      net_logf(ns, "INFO: Copying local file to host file: %s --> %s\n", source, dest);
      fileread = fopen(source, "rb");
      if (fileread)
      {
        fseek(fileread, 0, SEEK_END);
        filesize = ftell(fileread);
        fseek(fileread, 0, SEEK_SET);
        if (filesize > 0 && filesize < TOTALSIZE)
        {
          numfilesent++;
          packetend = 0;
          packetstart = 0;
          net_startNewPacket(&egopkt);
          packet_addUnsignedShort(&egopkt, TO_HOST_FILE);
          packet_addString(&egopkt, dest);
          packet_addUnsignedInt(&egopkt, filesize);
          packet_addUnsignedInt(&egopkt, packetstart);
          while (packetstart < filesize)
          {
            fscanf(fileread, "%c", &cTmp);
            packet_addUnsignedByte(&egopkt, cTmp);
            packetend++;
            packetstart++;
            if (packetend >= COPYSIZE)
            {
              // Send off the packet
              net_sendPacketToHostGuaranteed(ns->cs, &egopkt);
              enet_host_flush(ns->cs->clientHost);

              // Start on the next 4K
              packetend = 0;
              net_startNewPacket(&egopkt);
              packet_addUnsignedShort(&egopkt, TO_HOST_FILE);
              packet_addString(&egopkt, dest);
              packet_addUnsignedInt(&egopkt, filesize);
              packet_addUnsignedInt(&egopkt, packetstart);
            }
          }
          // Send off the packet
          net_sendPacketToHostGuaranteed(ns->cs, &egopkt);
        }
        fclose(fileread);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToHost(NET_STATE * ns, char *dirname, char *todirname)
{
  // ZZ> This function copies all files in a directory
  STRING searchname, fromname, toname;
  const char *searchResult;

  net_logf(ns, "INFO: net_copyDirectoryToHost: %s, %s\n", dirname, todirname);
  // Search for all files
  snprintf(searchname, sizeof(searchname), "%s/*", dirname);
  searchResult = fs_findFirstFile(dirname, NULL);
  if (searchResult != NULL)
  {
    // Make the new directory
    net_copyFileToHost(ns, dirname, todirname);

    // Copy each file
    while (searchResult != NULL)
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      // Also avoid copying directories in general.
      snprintf(fromname, sizeof(fromname), "%s/%s", dirname, searchResult);
      if (searchResult[0] == '.' || fs_fileIsDirectory(fromname))
      {
        searchResult = fs_findNextFile();
        continue;
      }

      snprintf(fromname, sizeof(fromname), "%s/%s", dirname, searchResult);
      snprintf(toname, sizeof(toname), "%s/%s", todirname, searchResult);

      net_copyFileToHost(ns, fromname, toname);
      searchResult = fs_findNextFile();
    }
  }

  fs_findClose();
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToAllPlayers(NET_STATE * ns, char *dirname, char *todirname)
{
  // ZZ> This function copies all files in a directory
  STRING searchname, fromname, toname;
  const char *searchResult;

  net_logf(ns, "INFO: net_copyDirectoryToAllPlayers: %s, %s\n", dirname, todirname);
  // Search for all files
  snprintf(searchname, sizeof(searchname), "%s/*.*", dirname);
  searchResult = fs_findFirstFile(dirname, NULL);
  if (searchResult != NULL)
  {
    // Make the new directory
    net_copyFileToAllPlayers(ns->ss, dirname, todirname);

    // Copy each file
    while (searchResult != NULL)
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      if (searchResult[0] == '.')
      {
        searchResult = fs_findNextFile();
        continue;
      }

      snprintf(fromname, sizeof(fromname), "%s/%s", dirname, searchResult);
      snprintf(toname, sizeof(toname), "%s/%s", todirname, searchResult);
      net_copyFileToAllPlayers(ns->ss, fromname, toname);

      searchResult = fs_findNextFile();
    }
  }
  fs_findClose();
}

//--------------------------------------------------------------------------------------------
retval_t net_copyDirectoryToOnePlayer(NET_STATE * ns, ENetPeer * peer, char *dirname, char *todirname)
{
  // ZZ> This function copies all files in a directory
  STRING searchname, fromname, toname;
  const char *searchResult;
  retval_t retval;

  net_logf(ns, "INFO: net_copyDirectoryToOnePlayers: %s, %s\n", dirname, todirname);

  // Search for all files
  snprintf(searchname, sizeof(searchname), "%s/*.*", dirname);
  searchResult = fs_findFirstFile(dirname, NULL);
  if (searchResult != NULL)
  {
    // Make the new directory
    retval = net_copyFileToOnePlayer(ns->ss, peer, dirname, todirname);
    if(rv_error == retval) return retval;

    // Copy each file
    while (searchResult != NULL)
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      if (searchResult[0] == '.')
      {
        searchResult = fs_findNextFile();
        continue;
      }

      snprintf(fromname, sizeof(fromname), "%s/%s", dirname, searchResult);
      snprintf(toname, sizeof(toname), "%s/%s", todirname, searchResult);
      retval = net_copyFileToOnePlayer(ns->ss, peer, fromname, toname);
      if(rv_error == retval) return retval;

      searchResult = fs_findNextFile();
    }
  }
  fs_findClose();

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
void net_sayHello(NET_STATE * ns)
{
  // ZZ> This function lets everyone know we're here  

  if (!ns->networkon)
  {
    ns->cs->waiting = bfalse;
    ns->ss->ready   = btrue;
  }
  else
  {
    // a given machine can be BOTH server and client at the moment
    if (ns->ss->amHost)
    {
      net_logf(ns, "INFO: net_sayHello: Server saying hello.\n");
    }
    
    if (ns->cs->amClient)
    {
      PACKET egopkt;
      net_logf(ns, "INFO: net_sayHello: Client saying hello.\n");
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, TO_HOST_IM_LOADED);
      net_sendPacketToHostGuaranteed(ns->cs, &egopkt);
      ns->cs->waiting = btrue;
    }
  }
}

//--------------------------------------------------------------------------------------------
char * localize_string(char * s)
{
  if(NULL==s) return "";

  if('@' == s[0])
  {
    return get_config_string(&CData, &s[1], NULL);
  }

  return s;
};

//--------------------------------------------------------------------------------------------
bool_t localize_filename(char * szin, char * szout)
{
  char * ptmp;
  size_t len;
  if(NULL==szin || NULL==szout) return bfalse;


  // scan through the string and localize every part of the string beginning with '@'
  while(*szin != 0)
  {
    if('@' == *szin)
    {
      ptmp = get_config_string(&CData, szin+1, &szin);
      if(NULL!=ptmp)
      {
        len = strlen(ptmp);
        strncpy(szout, ptmp, len);
        szout += len;
      };
    }
    else
    {
      *szout++ = *szin++;
    }
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t net_handlePacket(NET_STATE * ns, ENetEvent *event)
{
  Uint16 header;
  STRING filename, local_filename;   // also used for reading various strings
  FILE *file;
  size_t fileSize;
  bool_t retval = bfalse;
  PACKET egopkt;
  NET_PACKET netpkt;

  net_logf(ns, "INFO: net_handlePacket: Received ");

  if(ns->cs->amClient && cl_handlePacket(ns->cs, event)) return btrue;
  if(ns->ss->amHost   && sv_handlePacket(ns->ss, event)) return btrue;

  net_logf(ns, "INFO: net_handlePacket: Processing ");

  packet_startReading(&netpkt, event->packet);
  header = packet_readUnsignedShort(&netpkt);
  switch (header)
  {
  case TO_ANY_TEXT:
    net_logf(ns, "INFO: TO_ANY_TEXT\n");

    // someone has sent us some text to display

    packet_readString(&netpkt, filename, 255);
    debug_message(filename);
    retval = btrue;
    break;

  case NET_CHECK_CRC:
    {
      Uint32 CRC, CRCseed;
      CRCseed = packet_readUnsignedInt(&netpkt);
      packet_readString(&netpkt, filename, 256);
      
      // Acknowledge that we got the request file
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, NET_ACKNOWLEDGE_CRC);
      net_sendPacketToPeerGuaranteed(event->peer, &egopkt);

      if( rv_succeed != net_calculateCRC(filename, CRCseed, &CRC) ) CRC = (Uint32)(-1);

      // Acknowledge that we got the request file
      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, NET_SEND_CRC);
      packet_addUnsignedInt(&egopkt, CRC);
      net_sendPacketToPeerGuaranteed(event->peer, &egopkt);
    }
    retval = btrue;
    break;

  case NET_ACKNOWLEDGE_CRC:
    //assert(wating_for_crc);
    //crc_acknowledged = btrue;
    retval = btrue;
    break;

  case NET_SEND_CRC:
    //assert(crc_acknowledged);
    //returned_crc = packet_readUnsignedInt(&netpkt);
    //wating_for_crc = bfalse;
    retval = btrue;
    break;

  case NET_REQUEST_FILE:
    {
      // crack the incoming packet so we can localize the location of the file to this machine
  
      STRING remote_filename = {0}, local_filename = {0}, temp = {0};

      // read the remote filename
      packet_readString(&netpkt, temp, sizeof(remote_filename));
      localize_filename(temp, remote_filename);

      packet_readString(&netpkt, temp, sizeof(remote_filename));
      if(0x00 != temp[0])
      {
        strncat(local_filename, localize_string(temp), sizeof(local_filename));
        packet_readString(&netpkt, temp, sizeof(remote_filename));
        while(0x00 != temp[0])
        {
          strncat(local_filename, "/", sizeof(local_filename));
          strncat(local_filename, localize_string(temp), sizeof(local_filename));
          packet_readString(&netpkt, temp, sizeof(remote_filename));
        };

        // We successfully cracked the filename.  Now send the file if it exists on
        // our machine and if the remote machine's CRC does not match ours
        net_copyFileToOnePlayer(ns->ss, event->peer, local_filename, remote_filename);
      }
    }
    break;

  case TO_HOST_REQUEST_MODULE:
    {
      // The other computer is requesting all the files for the module
  
      STRING remote_filename = {0}, module_filename = {0}, module_path = {0}, temp = {0};

      // read the module name
      packet_readString(&netpkt, temp, sizeof(module_filename));


      //// Acknowledge that we got the request file
      //net_startNewPacket(&egopkt);
      //packet_addUnsignedShort(&egopkt, TO_CLIENT_ACKNOWLEDGE_REQUEST_MODULE);
      //net_sendPacketToPeerGuaranteed(event->peer, &egopkt);

      snprintf(module_filename, "%s/%s", CData.modules_dir, temp);
      snprintf(remote_filename, "@modules_dir/%s", module_filename);

      net_copyDirectoryToOnePlayer(ns, event->peer, module_filename, remote_filename);

      net_startNewPacket(&egopkt);
      packet_addUnsignedShort(&egopkt, NET_DONE_SENDING_FILES);
      net_sendPacketToPeerGuaranteed(event->peer, &egopkt);
    }
    break;

  case NET_TRANSFER_FILE:
    // we are receiving a file to a remote computer

    packet_readString(&netpkt, filename, 256);
    localize_filename(filename, local_filename);

    fileSize = packet_readUnsignedInt(&netpkt);

    net_logf(ns, "INFO: NET_TRANSFER_FILE: %s with size %d.\n", local_filename, fileSize);

    // Try and save the file
    file = fopen(local_filename, "wb");
    if (file != NULL)
    {
      fwrite(netpkt.wrapper.data + netpkt.wrapper.readLocation, 1, fileSize, file);
      fclose(file);
    }
    else
    {
      net_logf(ns, "WARNING: net_handlePacket: Couldn't write new file!\n");
    }

    // Acknowledge that we got this file
    net_startNewPacket(&egopkt);
    packet_addUnsignedShort(&egopkt, NET_TRANSFER_OK);
    net_sendPacketToPeer(event->peer, &egopkt);

    // And note that we've gotten another one
    numfile++;
    retval = btrue;
    break;

  case NET_TRANSFER_OK:
    net_logf(ns, "INFO: NET_TRANSFER_OK. The last file sent was successful.\n");

    // the other remote computer has responded that our file transfer was successful

    net_waitingForXferAck = 0;
    net_numFileTransfers--;
    retval = btrue;
    break;

  case NET_CREATE_DIRECTORY:

    // a remote computer is requesting that we create a directory

    packet_readString(&netpkt, filename, 256);
    localize_filename(filename, local_filename);
    net_logf(ns, "INFO: NET_CREATE_DIRECTORY: %s\n", local_filename);

    fs_createDirectory(local_filename);

    // Acknowledge that we got this file
    net_startNewPacket(&egopkt);
    packet_addUnsignedShort(&egopkt, NET_TRANSFER_OK);
    net_sendPacketToPeer(event->peer, &egopkt);

    numfile++; // The client considers directories it sends to be files, so ya.
    retval = btrue;
    break;

  case NET_DONE_SENDING_FILES:
    net_logf(ns, "INFO: NET_DONE_SENDING_FILES\n");

    // a remote computer is telling us they are done sending files

    numplayerrespond++;
    retval = btrue;
    break;

  case NET_NUM_FILES_TO_SEND:
    net_logf(ns, "INFO: NET_NUM_FILES_TO_SEND\n");

    // a remote computer is telling us how many files to expect to receive

    numfileexpected = (int)packet_readUnsignedShort(&netpkt);
    retval = btrue;
    break;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
void net_initialize(NET_STATE * ns, struct ClientState_t * cs, struct ServerState_t * ss, struct GameState_t * gs)
{
  // ZZ> This starts up the network and logs whatever goes on

  // Link us with the client state
  ns->cs = cs;
  cl_init(cs, ns);

  // Link us with the server state
  ns->ss = ss;
  sv_init(ss, ns);

  // Link it to the game state
  ns->gs = gs;

  ns->kill_me    = bfalse;
  ns->serviceon  = bfalse;
  ns->numservice = 0;
  ns->logfile    = fopen("net.txt", "w");

  // Clear all the state variables to 0 to start.
  memset(net_transferStates, 0, sizeof(NetFileTransfer) * NET_MAX_FILE_TRANSFERS);
  memset(&net_receiveState, 0, sizeof(NetFileTransfer));

  if (CData.request_network)
  {
    // initialize enet
    net_logf(ns, "INFO: net_initialize: Initializing enet...");
    if (0 != enet_initialize())
    {
      net_logf(ns, "INFO: Failed!\n");
      ns->networkon = bfalse;
      ns->serviceon = bfalse;
    }
    else
    {
      net_logf(ns, "INFO: Succeeded!\n");
      ns->networkon = btrue;
      ns->serviceon = btrue;
      ns->numservice = 1;
    }
  }
  else
  {
    // We're not doing networking this time...
    net_logf(ns, "INFO: net_initialize: Networking not enabled.\n");
    ns->networkon = bfalse;
  }

  ns->cs->amClient = bfalse;
  ns->ss->amHost   = bfalse;
  if(ns->networkon)
  {
    net_thread = ns->my_thread = SDL_CreateThread(net_Callback, ns);
  };

  atexit(Net_Quit);

}

//--------------------------------------------------------------------------------------------
void net_shutDown(NET_STATE * ns)
{
  net_logf(ns, "INFO: net_shutDown: Turning off networking.\n");

  if(NULL!=ns->logfile)
  {
    fclose(ns->logfile);
    ns->logfile = NULL;
  }

  enet_deinitialize();
}

//--------------------------------------------------------------------------------------------
int  net_pendingFileTransfers(NET_STATE * ns)
{
  return net_numFileTransfers;
}

//--------------------------------------------------------------------------------------------
Uint8 *transferBuffer = NULL;
size_t   transferSize = 0;

void net_updateFileTransfers(NET_STATE * ns)
{
  NetFileTransfer *state;
  ENetPacket *packet;
  size_t nameLen, fileSize;
  Uint32 networkSize;
  FILE *file;
  char *p;
  PACKET egopkt;

  // Are there any pending file sends?
  if (net_numFileTransfers > 0)
  {
    if (!net_waitingForXferAck)
    {
      state = &net_transferStates[net_fileTransferHead];

      // Check and see if this is a directory, instead of a file
      if (fs_fileIsDirectory(state->sourceName))
      {
        // Tell the target to create a directory
        net_logf(ns, "INFO: net_updateFileTranfers: Creating directory %s on target\n", state->destName);
        net_startNewPacket(&egopkt);
        packet_addUnsignedShort(&egopkt, NET_CREATE_DIRECTORY);
        packet_addString(&egopkt, state->destName);
        net_sendPacketToPeerGuaranteed(state->target, &egopkt);

        net_waitingForXferAck = 1;
      }
      else
      {
        file = fopen(state->sourceName, "rb");
        if (file)
        {
          net_logf(ns, "INFO: net_updateFileTransfers: Attempting to send %s to %s\n", state->sourceName, state->destName);

          fseek(file, 0, SEEK_END);
          fileSize = ftell(file);
          fseek(file, 0, SEEK_SET);

          // Make room for the file's name
          nameLen = strlen(state->destName) + 1;
          transferSize = nameLen;

          // And for the file's size
          transferSize += 6; // Uint32 size, and Uint16 message type
          transferSize += fileSize;

          transferBuffer = malloc(transferSize);
          *(Uint16*)transferBuffer = ENET_HOST_TO_NET_16(NET_TRANSFER_FILE);

          // Add the string and file length to the buffer
          p = transferBuffer + 2;
          strcpy(p, state->destName);
          p += nameLen;

          networkSize = ENET_HOST_TO_NET_32((Uint32)fileSize);
          *(size_t*)p = networkSize;
          p += 4;

          fread(p, 1, fileSize, file);
          fclose(file);

          packet = enet_packet_create(transferBuffer, transferSize, ENET_PACKET_FLAG_RELIABLE);
          enet_peer_send(state->target, NET_GUARANTEED_CHANNEL, packet);

          free(transferBuffer);
          transferBuffer = NULL;
          transferSize = 0;

          net_waitingForXferAck = 1;
        }
        else
        {
          net_logf(ns, "WARNING: net_updateFileTransfers: Could not open file %s to send it!\n", state->sourceName);
        }
      }

      // update transfer queue state
      memset(state, 0, sizeof(NetFileTransfer));
      net_fileTransferHead++;
      if (net_fileTransferHead >= NET_MAX_FILE_TRANSFERS)
      {
        net_fileTransferHead = 0;
      }

    } // end if waiting for ack
  } // end if net_numFileTransfers > 0

  // Let the recieve loop run at least once
  SDL_Delay(20);
  //listen_for_packets(ns);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void close_session(NET_STATE * ns)
{
  // ZZ> This function gets the computer out of a network game
  if ( !ns->networkon ) return;

  sv_unhostGame(ns->ss);
  cl_quitGame(ns->cs);

  // shut down the network thread
  if(NULL != ns->my_thread)
  {
    net_logf(ns, "INFO: close_session: Requesting the the network thread shut off at its earliest convenience.\n");
    ns->kill_me = btrue;
  };
}

//--------------------------------------------------------------------------------------------
bool_t net_beginGame(struct GameState_t * gs)
{
  bool_t retval = btrue;

  if(NULL==gs) return bfalse;

  // try to host a game
  if(retval && gs->ss->amHost)
  {
    bool_t bool_tmp = sv_hostGame(gs->ss);
    retval = retval && bool_tmp;
  };

  // try to join a hosted game
  if(retval && gs->cs->amClient) 
  {
    bool_t bool_tmp = cl_joinGame(gs->cs, gs->cd->net_hosts[0]);
    retval = retval && bool_tmp;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
PACKET_REQUEST * net_getRequest(PACKET_REQUEST * prlist, size_t prcount)
{
  int index;
  if(NULL==prlist || 0 == prcount) return NULL;

  for(index=0; index<prcount; index++)
  {
    if( !prlist[index].waiting )
    {
      // do some simple initialization
      prlist[index].received = bfalse;
      prlist[index].data_size = 0;
      prlist[index].data = NULL;
      return &prlist[index];
    };
  };

  return NULL;
};

//--------------------------------------------------------------------------------------------
bool_t net_testRequest(PACKET_REQUEST * prequest)
{
  if(NULL==prequest) return bfalse;
  return prequest->received;
};

//--------------------------------------------------------------------------------------------
bool_t net_releaseRequest(PACKET_REQUEST * prequest)
{
  if(NULL==prequest) return bfalse;
  prequest->waiting  = bfalse;
  prequest->received = bfalse;
  return btrue;
};



//--------------------------------------------------------------------------------------------
PACKET_REQUEST * net_checkRequest(PACKET_REQUEST * prlist, size_t prcount, ENetEvent * pevent)
{
  int index;
  PACKET_REQUEST * retval = NULL;
  NET_PACKET       npkt;
  if(NULL==prlist || 0 == prcount || NULL==pevent) return NULL;

  for(index=0; index<prcount; index++)
  {
    if( !prlist[index].waiting ) continue;

    // get rid of all expired requests
    if(SDL_GetTicks() > prlist[index].expiretimestamp)
    {
      prlist[index].waiting = bfalse;
      continue;
    }

    // grab the first one that fits out requirements
    if(NULL != retval) continue;
    if(NULL==pevent->packet || pevent->packet->dataLength == 0) continue;

    packet_startReading(&npkt, pevent->packet);

    if( prlist[index].packettype ==  packet_peekUnsignedShort(&npkt) &&
        0 == memcmp(&pevent->peer->address, &prlist[index].address, sizeof(ENetAddress)) )
    {
      retval = &prlist[index];
    }
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
int net_Callback(void * data)
{
  NET_STATE * ns = (NET_STATE *)data;
  ServerState_t * ss = ns->ss;
  ClientState_t * cs = ns->cs;

  if(NULL==ns)
  {
    net_logf(ns, "WARNING: Net callback thread - unable to start.\n");
    return bfalse;
  }

  if(NULL==cs || NULL==ss) 
  {
    net_logf(ns, "WARNING: Net callback thread - invalid configuration.\n");
    return bfalse;
  }

  net_logf(ns, "INFO: Net callback thread - starting normally.\n");

  while(!ns->kill_me)
  {
    // do the server stuff
    sv_dispatchPackets(ss);

    // do the client stuff (if any)
    cl_dispatchPackets(cs);

    SDL_Delay(10);
  };

  net_logf(ns, "INFO: Net callback thread - exiting normally");
  net_thread = ns->my_thread = NULL;
  return btrue;
};

//--------------------------------------------------------------------------------------------
int add_player(GAME_STATE * gs, Uint16 character, Uint16 player, Uint8 device)
{
  // ZZ> This function adds a player, returning bfalse if it fails, btrue otherwise

  if(PlaList[player].valid) return bfalse;

  ChrList[character].isplayer = btrue;
  PlaList[player].index = character;
  PlaList[player].valid = btrue;
  PlaList[player].device = device;
  if (device != INPUTNONE)  gs->modstate.nolocalplayers = bfalse;
  PlaList[player].latchx = 0;
  PlaList[player].latchy = 0;
  PlaList[player].latchbutton = 0;

  cl_resetTimeLatches(gs->cs, character);
  sv_resetTimeLatches(gs->ss, character);

  if (device != INPUTNONE)
  {
    ChrList[character].islocalplayer = btrue;
    numlocalpla++;
  }
  numpla++;
  return btrue;
}

//--------------------------------------------------------------------------------------------
void clear_messages()
{
  // ZZ> This function empties the message buffer
  int cnt;

  cnt = 0;
  while (cnt < MAXMESSAGE)
  {
    GMsg.list[cnt].time = 0;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void check_add(Uint8 key, char bigletter, char littleletter)
{
  // ZZ> This function adds letters to the net message

  if(KEYDOWN(key))
  {
    if(GNetMsg.write < MESSAGESIZE-2)
    {
      if(KEYDOWN(SDLK_LSHIFT) || KEYDOWN(SDLK_RSHIFT))
      {
        GNetMsg.buffer[GNetMsg.write] = bigletter;
      }
      else
      {
        GNetMsg.buffer[GNetMsg.write] = littleletter;
      }
      GNetMsg.write++;
      GNetMsg.buffer[GNetMsg.write] = '?'; // The flashing input cursor
      GNetMsg.buffer[GNetMsg.write+1] = 0;
    }
  }

}

//--------------------------------------------------------------------------------------------
void input_net_message(GAME_STATE * gs)
{
  // ZZ> This function lets players communicate over network by hitting return, then
  //     typing text, then return again

  int cnt;
  char cTmp;
  PACKET egopkt;
  NET_STATE * ns = gs->ns;
  ServerState_t * ss = gs->ss;
  ClientState_t * cs = gs->cs;


  if(gs->modstate.net_messagemode)
  {
    // Add new letters
    check_add(SDLK_a, 'A', 'a');
    check_add(SDLK_b, 'B', 'b');
    check_add(SDLK_c, 'C', 'c');
    check_add(SDLK_d, 'D', 'd');
    check_add(SDLK_e, 'E', 'e');
    check_add(SDLK_f, 'F', 'f');
    check_add(SDLK_g, 'G', 'g');
    check_add(SDLK_h, 'H', 'h');
    check_add(SDLK_i, 'I', 'i');
    check_add(SDLK_j, 'J', 'j');
    check_add(SDLK_k, 'K', 'k');
    check_add(SDLK_l, 'L', 'l');
    check_add(SDLK_m, 'M', 'm');
    check_add(SDLK_n, 'N', 'n');
    check_add(SDLK_o, 'O', 'o');
    check_add(SDLK_p, 'P', 'p');
    check_add(SDLK_q, 'Q', 'q');
    check_add(SDLK_r, 'R', 'r');
    check_add(SDLK_s, 'S', 's');
    check_add(SDLK_t, 'T', 't');
    check_add(SDLK_u, 'U', 'u');
    check_add(SDLK_v, 'V', 'v');
    check_add(SDLK_w, 'W', 'w');
    check_add(SDLK_x, 'X', 'x');
    check_add(SDLK_y, 'Y', 'y');
    check_add(SDLK_z, 'Z', 'z');


    check_add(SDLK_1, '!', '1');
    check_add(SDLK_2, '@', '2');
    check_add(SDLK_3, '#', '3');
    check_add(SDLK_4, '$', '4');
    check_add(SDLK_5, '%', '5');
    check_add(SDLK_6, '^', '6');
    check_add(SDLK_7, '&', '7');
    check_add(SDLK_8, '*', '8');
    check_add(SDLK_9, '(', '9');
    check_add(SDLK_0, ')', '0');


    check_add(SDLK_QUOTE, 34, 39);
    check_add(SDLK_SPACE,      ' ', ' ');
    check_add(SDLK_SEMICOLON,  ':', ';');
    check_add(SDLK_PERIOD,     '>', '.');
    check_add(SDLK_COMMA,      '<', ',');
    check_add(SDLK_BACKQUOTE,  '`', '`');
    check_add(SDLK_MINUS,      '_', '-');
    check_add(SDLK_EQUALS,     '+', '=');
    check_add(SDLK_LEFTBRACKET, '{', '[');
    check_add(SDLK_RIGHTBRACKET,'}', ']');
    check_add(SDLK_BACKSLASH,  '|', '\\');
    check_add(SDLK_SLASH,      '?', '/');



    // Make cursor flash
    if(GNetMsg.write < MESSAGESIZE-1)
    {
      if((wldframe & 7) == 0)
      {
        GNetMsg.buffer[GNetMsg.write] = '#';
      }
      else
      {
        GNetMsg.buffer[GNetMsg.write] = '+';
      }
    }


    // Check backspace and return
    if(GNetMsg.delay == 0)
    {
      if(KEYDOWN(SDLK_BACKSPACE))
      {
        if(GNetMsg.write < MESSAGESIZE)  GNetMsg.buffer[GNetMsg.write] = 0;
        if(GNetMsg.write > GNetMsg.writemin) GNetMsg.write--;
        GNetMsg.delay = 3;
      }


      // Ship out the message
      if(KEYDOWN(SDLK_RETURN))
      {
        // Is it long enough to bother?
        if(GNetMsg.write > 0)
        {
          // Yes, so send it
          GNetMsg.buffer[GNetMsg.write] = 0;
          if(ss->amHost || cs->amClient)
          {
            net_startNewPacket(&egopkt);
            packet_addUnsignedShort(&egopkt, TO_ANY_TEXT);
            packet_addString(&egopkt, GNetMsg.buffer);
            if(ss->amHost && !cs->amClient)
            {
              net_sendPacketToAllPlayers(ss, &egopkt);
            }
            else
            {
              net_sendPacketToHost(cs, &egopkt);
            };
          }
        }
        gs->modstate.net_messagemode = bfalse;
        GNetMsg.delay = 20;
      }
    }
    else
    {
      GNetMsg.delay--;
    }
  }
  else
  {
    // Input a new message?
    if(GNetMsg.delay == 0)
    {
      if(KEYDOWN(SDLK_RETURN))
      {
        // Copy the name
        cnt = 0;
        cTmp = CData.net_messagename[cnt];
        while(cTmp != 0 && cnt < 64)
        {
          GNetMsg.buffer[cnt] = cTmp;
          cnt++;
          cTmp = CData.net_messagename[cnt];
        }
        GNetMsg.buffer[cnt] = '>';  cnt++;
        GNetMsg.buffer[cnt] = ' ';  cnt++;
        GNetMsg.buffer[cnt] = '?';
        GNetMsg.buffer[cnt+1] = 0;
        GNetMsg.write = cnt;
        GNetMsg.writemin = cnt;

        gs->modstate.net_messagemode = btrue;
        GNetMsg.delay = 20;
      }
    }
    else
    {
      GNetMsg.delay--;
    }
  }

}

//--------------------------------------------------------------------------------------------
PACKET_REQUEST * net_prepareWaitForPacket(PACKET_REQUEST * request_buff, size_t request_buff_sz, ENetPeer * peer, Uint32 timeout, Uint8 * buffer, size_t buffer_sz, Uint16 packet_type)
{
  PACKET_REQUEST * prequest;

  prequest = net_getRequest(request_buff, request_buff_sz);
  assert(NULL!=prequest);

  prequest->waiting   = btrue;
  prequest->received  = bfalse;
  prequest->expiretimestamp = SDL_GetTicks() + timeout;
  prequest->data      = buffer;
  prequest->data_size = buffer_sz;
  memcpy(&prequest->address, &peer->address, sizeof(ENetAddress));
  prequest->packettype = packet_type;

  return prequest;
};

//--------------------------------------------------------------------------------------------
retval_t net_loopWaitForPacket(PACKET_REQUEST * prequest, Uint32 granularity, size_t * data_size)
{

  if(prequest->waiting && !prequest->received) 
  {
    if(SDL_GetTicks() >= prequest->expiretimestamp)
    {
      net_releaseRequest(prequest);
      return rv_fail;
    }
    else
    {
      SDL_Delay(granularity); 
      return rv_waiting; 
    };
  }

  if(NULL!=data_size)
  {
    *data_size = prequest->data_size;
  }
  net_releaseRequest(prequest);

  return rv_succeed;
};

//--------------------------------------------------------------------------------------------
retval_t net_waitForPacket(PACKET_REQUEST * request_buff, size_t request_buff_sz, ENetPeer * peer, Uint32 timeout, Uint8 * buffer, size_t buffer_sz, Uint16 packet_type, size_t * data_size)
{
  PACKET_REQUEST * prequest;
  retval_t wait_return;

  prequest = net_prepareWaitForPacket(request_buff, request_buff_sz, peer, timeout, buffer, buffer_sz, packet_type);
  if(NULL==prequest) return rv_error;

  wait_return = net_loopWaitForPacket(prequest, 10, data_size);
  while(rv_waiting == wait_return)
  {
    wait_return = net_loopWaitForPacket(prequest, 10, data_size);
  };

  return wait_return;
};

//--------------------------------------------------------------------------------------------
//bool_t listen_for_packets(NET_STATE * ns)
//{
//  // ZZ> This function reads any new messages and sets the player latch and matrix needed
//  //     lists...
//  ENetEvent event;
//  bool_t retval = bfalse;
//
//  if (!ns->networkon || NULL==ns->ss->serverHost) return bfalse;
//
//  // Listen for new messages
//  while (0 != enet_host_service(ns->ss->serverHost, &event, 0))
//  {
//    switch (event.type)
//    {
//    case ENET_EVENT_TYPE_RECEIVE:
//      if(!net_handlePacket(ns, &event))
//      {
//        net_logf(ns, "WARNING: listen_for_packets() - Unhandled packet");
//      }
//      enet_packet_destroy(event.packet);
//      retval = btrue;
//      break;
//
//    case ENET_EVENT_TYPE_CONNECT:
//      // don't allow anyone to connect during the game session
//      net_logf(ns, "WARNING: listen_for_packets: Client tried to connect during the game: %x:%u\n",
//        event.peer->address.host, event.peer->address.port);
//#ifdef ENET11
//      enet_peer_disconnect(event.peer, 0);
//#else
//      enet_peer_disconnect(event.peer);
//#endif
//      retval = btrue;
//      break;
//
//    case ENET_EVENT_TYPE_DISCONNECT:
//      // Is this a player disconnecting, or just a rejected connection
//      // from above?
//      if (event.peer->data != 0)
//      {
//        NetPlayerInfo *info = event.peer->data;
//
//        // uh oh, how do we handle losing a player?
//        net_logf(ns, "WARNING: listen_for_packets: Player %d disconnected!\n",
//          info->playerSlot);
//      }
//      retval = btrue;
//      break;
//    }
//  }
//
//  return retval;
//}


//--------------------------------------------------------------------------------------------
//void find_open_sessions(NET_STATE * ns)
//{
//  // ZZ> This function finds some open games to join
//  DPSESSIONDESC2      sessionDesc;
//  HRESULT             hr;
//
//  if(ns->networkon)
//  {
//  numsession = 0;
//  if(globalnetworkerr)  fprintf(globalnetworkerr, "  Looking for open games...\n");
//  ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
//  sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
//  sessionDesc.guidApplication = NETWORKID;
//  hr = lpDirectPlay3A->EnumSessions(&sessionDesc, 0, SessionsCallback, hGlobalWindow, DPENUMSESSIONS_AVAILABLE);
//  if(globalnetworkerr)  fprintf(globalnetworkerr, "    %d sessions found\n", numsession);
//  }
//}


//--------------------------------------------------------------------------------------------
//void stop_players_from_joining(NET_STATE * ns)
//{
//  // ZZ> This function stops players from joining a game
//}


////--------------------------------------------------------------------------------------------
//void turn_on_service(int service)
//{
//  // This function turns on a network service ( IPX, TCP, serial, modem )
//}

//--------------------------------------------------------------------------------------------
//void send_rts_order(int x, int y, Uint8 order, Uint8 target)
//{
//  // ZZ> This function asks the host to order the selected characters
//  /* Uint32 what, when, whichorder, cnt;
//
//   if(numrtsselect > 0)
//   {
//    x = (x >> 6) & 1023;
//    y = (y >> 6) & 1023;
//    what = (target << 24) | (x << 14) | (y << 4) | (order&15);
//    if(ss->Active)
//    {
//     when = wldframe + CData.GOrder.lag;
//     whichorder = get_empty_order();
//     if(whichorder != MAXORDER)
//     {
//      // Add a new order on own machine
//      GOrder.when[whichorder] = when;
//      GOrder.what[whichorder] = what;
//      cnt = 0;
//      while(cnt < numrtsselect)
//      {
//       GOrder.who[whichorder][cnt] = GRTS.select[cnt];
//       cnt++;
//      }
//      while(cnt < MAXSELECT)
//      {
//       GOrder.who[whichorder][cnt] = MAXCHR;
//       cnt++;
//      }
//
//
//      // Send the order off to everyone else
//      if(ns->networkon)
//      {
//       net_startNewPacket(&gPacket);
//       packet_addUnsignedShort(&gPacket, TO_REMOTE_RTS);
//       cnt = 0;
//       while(cnt < MAXSELECT)
//       {
//        packet_addUnsignedByte(&gPacket, GOrder.who[whichorder][cnt]);
//        cnt++;
//       }
//       packet_addUnsignedInt(&gPacket, what);
//       packet_addUnsignedInt(&gPacket, when);
//       net_sendPacketToAllPlayersGuaranteed(ns->ss, &gPacket);
//      }
//     }
//    }
//    else
//    {
//     // Send the order off to the host
//     net_startNewPacket(&gPacket);
//     packet_addUnsignedShort(&gPacket, TO_HOST_RTS);
//     cnt = 0;
//     while(cnt < numrtsselect)
//     {
//      packet_addUnsignedByte(&gPacket, GRTS.select[cnt]);
//      cnt++;
//     }
//     while(cnt < MAXSELECT)
//     {
//      packet_addUnsignedByte(&gPacket, MAXCHR);
//      cnt++;
//     }
//     packet_addUnsignedInt(&gPacket, what);
//     net_sendPacketToHostGuaranteed(ns->ss, &gPacket);
//    }
//   }*/
//}
//