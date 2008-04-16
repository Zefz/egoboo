// network.c
// Egoboo, Copyright (C) 2000 Aaron Bishop

#include "Network.h"
#include "input.h"
#include "Character.h"
#include "Camera.h"
#include "egoboo.h"

#include <enet/enet.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

int Order::lag;

Net_Info GNet;
NetMsg   GNetMsg;


// Networking constants
enum NetworkConstant
{
  NET_UNRELIABLE_CHANNEL  = 0,
  NET_GUARANTEED_CHANNEL  = 1,
  NET_EGOBOO_NUM_CHANNELS,
  NET_EGOBOO_PORT         = 34626,
  NET_MAX_FILE_NAME       = 0x80,
  NET_MAX_FILE_TRANSFERS  = 0x0400, // Maximum files queued up at once
};

// Network messages
enum NetworkMessage
{
  NET_TRANSFER_FILE   = 10001, // Packet contains a file.
  NET_TRANSFER_OK    = 10002, // Acknowledgement packet for a file send
  NET_CREATE_DIRECTORY  = 10003, // Tell the peer to create the named directory
  NET_DONE_SENDING_FILES  = 10009, // Sent when there are no more files to send.
  NET_NUM_FILES_TO_SEND  = 10010, // Let the other person know how many files you're sending
};

// Network players information
typedef struct NetPlayerInfo
{
  int playerSlot;
} NetPlayerInfo;

// Log file variables
FILE *net_logFile = NULL;
char net_logBuffer[0x0400];

// ENet host & client identifiers
ENetHost* net_myHost = NULL;
ENetPeer* net_gameHost = NULL;
ENetPeer* net_playerPeers[PLAYER_COUNT];
NetPlayerInfo net_playerInfo[MAXNETPLAYER];

bool net_amHost = false;

// Packet reading
ENetPacket*  net_readPacket = NULL;
size_t   net_readLocation = 0;

// Packet writing
Uint32 packethead;                             // The write head
Uint32 packetsize;                             // The size of the packet
Uint8 packetbuffer[MAXSENDSIZE];              // The data packet
Uint32 nexttimestamp;                          // Expected timestamp

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

void net_logf(const char *format, ...)
{
  va_list args;

  if (net_logFile)
  {
    va_start(args, format);
    vsnprintf(net_logBuffer, 0x0400, format, args);
    va_end(args);

    fputs(net_logBuffer, net_logFile);

    // TEMPORARY!
    fflush(net_logFile);
  }
}

//--------------------------------------------------------------------------------------------
void close_session()
{
  size_t i, numPeers;
  ENetEvent event;

  // ZZ> This function gets the computer out of a network game
  if (GNet.on)
  {
    if (net_amHost)
    {
      // Disconnect the peers
      numPeers = net_myHost->peerCount;
      for (i = 0;i < numPeers;i++)
      {
        enet_peer_disconnect(&net_myHost->peers[i]);
      }

      // Allow up to 5 seconds for peers to drop
      while (enet_host_service(net_myHost, &event, 5000))
      {
        switch (event.type)
        {
          case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;

          case ENET_EVENT_TYPE_DISCONNECT:
            net_logf("close_session: Peer id %d disconnected gracefully.\n", event.peer->address.host);
            numPeers--;
            break;
        }
      }

      // Forcefully disconnect any peers leftover
      for (i = 0;i < net_myHost->peerCount;i++)
      {
        enet_peer_reset(&net_myHost->peers[i]);
      }
    }

    net_logf("close_session: Disconnecting from network.\n");
    enet_host_destroy(net_myHost);
    net_myHost = NULL;
    net_gameHost = NULL;
  }
}

//--------------------------------------------------------------------------------------------
bool Player_List::add_player(Uint16 character, Uint8 device)
{
  // ZZ> This function adds a player, returning false if it fails, true otherwise
  Uint32 player = PlaList.get_free();
  if ( Player_List::INVALID == player ) return false;
  Player & rpla = PlaList[player];
  
  if (device != INPUT_NONE) 

  ChrList[character].isplayer = true;
  rpla.index  = character;
  rpla._on     = true;
  rpla.device = device;
  rpla.latch.clear();
  rpla.timelatch.clear();
  PlaList.count_total++;

  if (device != INPUT_NONE)
  {
    nolocalplayers = false;
    ChrList[character].islocalplayer = true;
    PlaList.count_local++;
  }

  return true;
}

//--------------------------------------------------------------------------------------------
void clear_messages()
{
  // ZZ> This function empties the message buffer
  int cnt;

  cnt = 0;
  while (cnt < MAXMESSAGE)
  {
    GMsg.time[cnt] = 0;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void clear_select()
{
  // ZZ> This function clears the RTS select list
  GRTS.select_count = 0;
}

//--------------------------------------------------------------------------------------------
void add_select(Uint16 character)
{
  // ZZ> This function selects a character
  if (GRTS.select_count < MAXSELECT)
  {
    GRTS.select[GRTS.select_count] = character;
    GRTS.select_count++;
  }
}

//--------------------------------------------------------------------------------------------
void check_add(Uint8 key, char bigletter, char littleletter)
{
  // ZZ> This function adds letters to the net message
  /*PORT
  if(GKeyb.pressed(key))
  {
  if(0==GKeyb.press[key])
  {
  GKeyb.press[key] = true;
  if(GNetMsg.write < MESSAGESIZE-2)
  {
  if(GKeyb.pressed(DIK_LSHIFT) || GKeyb.pressed(DIK_RSHIFT))
  {
  GNetMsg.buffer[netmessagewrite] = bigletter;
  }
  else
  {
  GNetMsg.buffer[netmessagewrite] = littleletter;
  }
  GNetMsg.write++;
  GNetMsg.buffer[netmessagewrite] = '?'; // The flashing input cursor
  GNetMsg.buffer[netmessagewrite+1] = 0;
  }
  }
  }
  else
  {
  GKeyb.press[key] = false;
  }
  */
}

//--------------------------------------------------------------------------------------------
void net_startNewPacket()
{
  // ZZ> This function starts building a network packet
  packethead = 0;
  packetsize = 0;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedByte(Uint8 uc)
{
  // ZZ> This function appends an Uint8 to the packet
  Uint8* ucp;
  ucp = (Uint8*)(&packetbuffer[packethead]);
  *ucp = uc;
  packethead+=1;
  packetsize+=1;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedByte(Sint8 sc)
{
  // ZZ> This function appends a Sint8 to the packet
  Sint8* scp;
  scp = (Sint8*)(&packetbuffer[packethead]);
  *scp = sc;
  packethead+=1;
  packetsize+=1;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedShort(Uint16 us)
{
  // ZZ> This function appends an Uint16 to the packet
  Uint16* usp;
  usp = (Uint16*)(&packetbuffer[packethead]);

  *usp = ENET_HOST_TO_NET_16(us);
  packethead+=2;
  packetsize+=2;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedShort(Sint16 ss)
{
  // ZZ> This function appends a Sint16 to the packet
  Sint16* ssp;
  ssp = (Sint16*)(&packetbuffer[packethead]);

  *ssp = ENET_HOST_TO_NET_16(ss);

  packethead+=2;
  packetsize+=2;
}

//--------------------------------------------------------------------------------------------
void packet_addUnsignedInt(Uint32 ui)
{
  // ZZ> This function appends an Uint32 to the packet
  Uint32* uip;
  uip = (Uint32*)(&packetbuffer[packethead]);

  *uip = ENET_HOST_TO_NET_32(ui);

  packethead+=4;
  packetsize+=4;
}

//--------------------------------------------------------------------------------------------
void packet_addSignedInt(Sint32 si)
{
  // ZZ> This function appends a Sint32 to the packet
  Sint32* sip;
  sip = (Sint32*)(&packetbuffer[packethead]);

  *sip = ENET_HOST_TO_NET_32(si);

  packethead+=4;
  packetsize+=4;
}

//--------------------------------------------------------------------------------------------
void packet_addString(char *string)
{
  // ZZ> This function appends a null terminated string to the packet
  char* cp;
  char cTmp;
  int cnt;

  cnt = 0;
  cTmp = 1;
  cp = (char*)(&packetbuffer[packethead]);
  while (cTmp != 0)
  {
    cTmp = string[cnt];
    *cp = cTmp;
    cp+=1;
    packethead+=1;
    packetsize+=1;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void packet_startReading(ENetPacket *packet)
{
  net_readPacket = packet;
  net_readLocation = 0;
}

//--------------------------------------------------------------------------------------------
void packet_doneReading()
{
  net_readPacket = NULL;
  net_readLocation = 0;
}

//--------------------------------------------------------------------------------------------
void packet_readString(char *buffer, int maxLen)
{
  // ZZ> This function reads a null terminated string from the packet
  Uint8 uc;
  Uint16 outindex;

  outindex = 0;
  uc = net_readPacket->data[net_readLocation];
  net_readLocation++;
  while (uc != 0 && outindex < maxLen)
  {
    buffer[outindex] = uc;
    outindex++;
    uc = net_readPacket->data[net_readLocation];
    net_readLocation++;
  }
  buffer[outindex] = 0;
}

//--------------------------------------------------------------------------------------------
Uint8 packet_readUnsignedByte()
{
  // ZZ> This function reads an Uint8 from the packet
  Uint8 uc;
  uc = (Uint8)net_readPacket->data[net_readLocation];
  net_readLocation++;
  return uc;
}

//--------------------------------------------------------------------------------------------
Sint8 packet_readSignedByte()
{
  // ZZ> This function reads a Sint8 from the packet
  Sint8 sc;
  sc = (Sint8)net_readPacket->data[net_readLocation];
  net_readLocation++;
  return sc;
}

//--------------------------------------------------------------------------------------------
Uint16 packet_readUnsignedShort()
{
  // ZZ> This function reads an Uint16 from the packet
  Uint16 us;
  Uint16* usp;
  usp = (Uint16*)(&net_readPacket->data[net_readLocation]);

  us = ENET_NET_TO_HOST_16(*usp);

  net_readLocation+=2;
  return us;
}

//--------------------------------------------------------------------------------------------
Sint16 packet_readSignedShort()
{
  // ZZ> This function reads a Sint16 from the packet
  Sint16 ss;
  Sint16* ssp;
  ssp = (Sint16*)(&net_readPacket->data[net_readLocation]);

  ss = ENET_NET_TO_HOST_16(*ssp);

  net_readLocation+=2;
  return ss;
}

//--------------------------------------------------------------------------------------------
Uint32 packet_readUnsignedInt()
{
  // ZZ> This function reads an Uint32 from the packet
  Uint32 ui;
  Uint32* uip;
  uip = (Uint32*)(&net_readPacket->data[net_readLocation]);

  ui = ENET_NET_TO_HOST_32(*uip);

  net_readLocation+=4;
  return ui;
}

//--------------------------------------------------------------------------------------------
Sint32 packet_readSignedInt()
{
  // ZZ> This function reads a Sint32 from the packet
  Sint32 si;
  Sint32* sip;
  sip = (Sint32*)(&net_readPacket->data[net_readLocation]);

  si = ENET_NET_TO_HOST_32(*sip);

  net_readLocation+=4;
  return si;
}

//--------------------------------------------------------------------------------------------
size_t packet_remainingSize()
{
  // ZZ> This function tells if there's still data left in the packet
  return net_readPacket->dataLength - net_readLocation;
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHost()
{
  // ZZ> This function sends a packet to the host
  ENetPacket *packet = enet_packet_create(packetbuffer, packetsize, 0);
  enet_peer_send(net_gameHost, NET_UNRELIABLE_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayers()
{
  // ZZ> This function sends a packet to all the players
  ENetPacket *packet = enet_packet_create(packetbuffer, packetsize, 0);
  enet_host_broadcast(net_myHost, NET_UNRELIABLE_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHostGuaranteed()
{
  // ZZ> This function sends a packet to the host
  ENetPacket *packet = enet_packet_create(packetbuffer, packetsize, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(net_gameHost, NET_UNRELIABLE_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayersGuaranteed()
{
  // ZZ> This function sends a packet to all the players
  ENetPacket *packet = enet_packet_create(packetbuffer, packetsize, ENET_PACKET_FLAG_RELIABLE);
  enet_host_broadcast(net_myHost, NET_GUARANTEED_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToOnePlayerGuaranteed(int player)
{
  // ZZ> This function sends a packet to one of the players
  ENetPacket *packet = enet_packet_create(packetbuffer, packetsize, ENET_PACKET_FLAG_RELIABLE);

  if (player < GNet.numplayer)
  {
    enet_peer_send(&net_myHost->peers[player], NET_GUARANTEED_CHANNEL, packet);
  }
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeer(ENetPeer *peer)
{
  // JF> This function sends a packet to a given peer
  ENetPacket *packet = enet_packet_create(packetbuffer, packetsize, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, NET_UNRELIABLE_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeerGuaranteed(ENetPeer *peer)
{
  // JF> This funciton sends a packet to a given peer, with guaranteed delivery
  ENetPacket *packet = enet_packet_create(packetbuffer, packetsize, 0);
  enet_peer_send(peer, NET_GUARANTEED_CHANNEL, packet);
}

//--------------------------------------------------------------------------------------------
void input_net_message()
{
  // ZZ> This function lets players communicate over network by hitting return, then
  //     typing text, then return again
  /*PORT
  int cnt;
  char cTmp;

  if(GNetMsg.mode)
  {
  // Add new letters
  check_add(DIK_A, 'A', 'a');
  check_add(DIK_B, 'B', 'b');
  check_add(DIK_C, 'C', 'c');
  check_add(DIK_D, 'D', 'd');
  check_add(DIK_E, 'E', 'e');
  check_add(DIK_F, 'F', 'f');
  check_add(DIK_G, 'G', 'g');
  check_add(DIK_H, 'H', 'h');
  check_add(DIK_I, 'I', 'i');
  check_add(DIK_J, 'J', 'j');
  check_add(DIK_K, 'K', 'k');
  check_add(DIK_L, 'L', 'l');
  check_add(DIK_M, 'M', 'm');
  check_add(DIK_N, 'N', 'n');
  check_add(DIK_O, 'O', 'o');
  check_add(DIK_P, 'P', 'p');
  check_add(DIK_Q, 'Q', 'q');
  check_add(DIK_R, 'R', 'r');
  check_add(DIK_S, 'S', 's');
  check_add(DIK_T, 'T', 't');
  check_add(DIK_U, 'U', 'u');
  check_add(DIK_V, 'V', 'v');
  check_add(DIK_W, 'W', 'w');
  check_add(DIK_X, 'X', 'x');
  check_add(DIK_Y, 'Y', 'y');
  check_add(DIK_Z, 'Z', 'z');

  check_add(DIK_1, '!', '1');
  check_add(DIK_2, '@', '2');
  check_add(DIK_3, '#', '3');
  check_add(DIK_4, '$', '4');
  check_add(DIK_5, '%', '5');
  check_add(DIK_6, '^', '6');
  check_add(DIK_7, '&', '7');
  check_add(DIK_8, '*', '8');
  check_add(DIK_9, '(', '9');
  check_add(DIK_0, ')', '0');

  check_add(DIK_APOSTROPHE, 34, 39);
  check_add(DIK_SPACE,      ' ', ' ');
  check_add(DIK_SEMICOLON,  ':', ';');
  check_add(DIK_PERIOD,     '>', '.');
  check_add(DIK_COMMA,      '<', ',');
  check_add(DIK_GRAVE,      '`', '`');
  check_add(DIK_MINUS,      '_', '-');
  check_add(DIK_EQUALS,     '+', '=');
  check_add(DIK_LBRACKET,   '{', '[');
  check_add(DIK_RBRACKET,   '}', ']');
  check_add(DIK_BACKSLASH,  '|', '\\');
  check_add(DIK_SLASH,      '?', '/');

  // Make cursor flash
  if(GNetMsg.write < MESSAGESIZE-1)
  {
  if((wldframe & 8) == 0)
  {
  GNetMsg.buffer[netmessagewrite] = '#';
  }
  else
  {
  GNetMsg.buffer[netmessagewrite] = '+';
  }
  }

  // Check backspace and return
  if(GNetMsg.delay == 0)
  {
  if(GKeyb.pressed(DIK_BACK))
  {
  if(GNetMsg.write < MESSAGESIZE)  GNetMsg.buffer[netmessagewrite] = 0;
  if(GNetMsg.write > GNetMsg.writemin) GNetMsg.write--;
  GNetMsg.delay = 3;
  }

  // Ship out the message
  if(GKeyb.pressed(DIK_RETURN))
  {
  // Is it long enough to bother?
  if(GNetMsg.write > 0)
  {
  // Yes, so send it
  GNetMsg.buffer[netmessagewrite] = 0;
  if(GNet.on)
  {
  start_building_packet();
  add_packet_us(TO_ANY_TEXT);
  add_packet_sz(GNetMsg.);
  send_packet_to_all_players();
  }
  }
  GNetMsg.mode = false;
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
  if(GKeyb.pressed(DIK_RETURN))
  {
  // Copy the name
  cnt = 0;
  cTmp = GNetMsg.name[cnt];
  while(cTmp != 0 && cnt < 0x40)
  {
  GNetMsg.buffer[cnt] = cTmp;
  cnt++;
  cTmp = GNetMsg.name[cnt];
  }
  GNetMsg.buffer[cnt] = '>';  cnt++;
  GNetMsg.buffer[cnt] = ' ';  cnt++;
  GNetMsg.buffer[cnt] = '?';
  GNetMsg.buffer[cnt+1] = 0;
  GNetMsg.write = cnt;
  GNetMsg.writemin = cnt;

  GNetMsg.mode = true;
  GNetMsg.delay = 20;
  }
  }
  else
  {
  GNetMsg.delay--;
  }
  }
  */
}

//------------------------------------------------------------------------------
void net_copyFileToAllPlayers(char *source, char *dest)
{
  // JF> This function queues up files to send to all the hosts.
  //     TODO: Deal with having to send to up to PLAYER_COUNT players...
  NetFileTransfer *state;

  if (net_numFileTransfers < NET_MAX_FILE_TRANSFERS)
  {
    // net_fileTransferTail should already be pointed at an open
    // slot in the queue.
    state = &(net_transferStates[net_fileTransferTail]);
    assert(state->sourceName[0] == 0);

    // Just do the first player for now
    state->target = &net_myHost->peers[0];
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
      net_logf("net_copyFileToAllPlayers: Warning!  Queue tail caught up with the head!\n");
    }
  }
}

//------------------------------------------------------------------------------
void net_copyFileToAllPlayersOld(char *source, char *dest)
{
  // ZZ> This function copies a file on the host to every remote computer.
  //     Packets are sent in chunks of COPYSIZE bytes.  The max file size
  //     that can be sent is 2 Megs ( TOTALSIZE ).
  FILE* fileread;
  int packetsize, packetstart;
  int filesize;
  int fileisdir;
  char cTmp;

  net_logf("net_copyFileToAllPlayers: %s, %s\n", source, dest);
  if (GNet.on && hostactive)
  {
    fileisdir = fs_fileIsDirectory(source);
    if (fileisdir)
    {
      net_startNewPacket();
      packet_addUnsignedShort(TO_REMOTE_DIR);
      packet_addString(dest);
      net_sendPacketToAllPlayersGuaranteed();
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
          packetsize = 0;
          packetstart = 0;
          numfilesent++;

          net_startNewPacket();
          packet_addUnsignedShort(TO_REMOTE_FILE);
          packet_addString(dest);
          packet_addUnsignedInt(filesize);
          packet_addUnsignedInt(packetstart);
          while (packetstart < filesize)
          {
            // This will probably work...
            //fread((packetbuffer + packethead), COPYSIZE, 1, fileread);

            // But I'll leave it alone for now
            fscanf(fileread, "%c", &cTmp);

            packet_addUnsignedByte(cTmp);
            packetsize++;
            packetstart++;
            if (packetsize >= COPYSIZE)
            {
              // Send off the packet
              net_sendPacketToAllPlayersGuaranteed();
              enet_host_flush(net_myHost);

              // Start on the next 4K
              packetsize = 0;
              net_startNewPacket();
              packet_addUnsignedShort(TO_REMOTE_FILE);
              packet_addString(dest);
              packet_addUnsignedInt(filesize);
              packet_addUnsignedInt(packetstart);
            }
          }
          // Send off the packet
          net_sendPacketToAllPlayersGuaranteed();
        }
        fclose(fileread);
      }
    }
  }
}

//------------------------------------------------------------------------------
void net_copyFileToHost(char *source, char *dest)
{
  NetFileTransfer *state;

  // JF> New function merely queues up a new file to be sent

  // If this is the host, just copy the file locally
  if (hostactive)
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

    state->target = net_gameHost;
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
      net_logf("net_copyFileToHost: Warning!  Queue tail caught up with the head!\n");
    }
  }
}

//------------------------------------------------------------------------------
void net_copyFileToHostOld(char *source, char *dest)
{
  // ZZ> This function copies a file on the remote to the host computer.
  //     Packets are sent in chunks of COPYSIZE bytes.  The max file size
  //     that can be sent is 2 Megs ( TOTALSIZE ).
  FILE* fileread;
  int packetsize, packetstart;
  int filesize;
  int fileisdir;
  char cTmp;

  net_logf("net_copyFileToHost: ");
  fileisdir = fs_fileIsDirectory(source);
  if (hostactive)
  {
    // Simulate a network transfer
    if (fileisdir)
    {
      net_logf("Creating local directory %s\n", dest);
      fs_createDirectory(dest);
    }
    else
    {
      net_logf("Copying local file %s --> %s\n", source, dest);
      fs_copyFile(source, dest);
    }
  }
  else
  {
    if (fileisdir)
    {
      net_logf("Creating directory on host: %s\n", dest);
      net_startNewPacket();
      packet_addUnsignedShort(TO_HOST_DIR);
      packet_addString(dest);
//   net_sendPacketToAllPlayersGuaranteed();
      net_sendPacketToHost();
    }
    else
    {
      net_logf("Copying local file to host file: %s --> %s\n", source, dest);
      fileread = fopen(source, "rb");
      if (fileread)
      {
        fseek(fileread, 0, SEEK_END);
        filesize = ftell(fileread);
        fseek(fileread, 0, SEEK_SET);
        if (filesize > 0 && filesize < TOTALSIZE)
        {
          numfilesent++;
          packetsize = 0;
          packetstart = 0;
          net_startNewPacket();
          packet_addUnsignedShort(TO_HOST_FILE);
          packet_addString(dest);
          packet_addUnsignedInt(filesize);
          packet_addUnsignedInt(packetstart);
          while (packetstart < filesize)
          {
            fscanf(fileread, "%c", &cTmp);
            packet_addUnsignedByte(cTmp);
            packetsize++;
            packetstart++;
            if (packetsize >= COPYSIZE)
            {
              // Send off the packet
              net_sendPacketToHostGuaranteed();
              enet_host_flush(net_myHost);

              // Start on the next 4K
              packetsize = 0;
              net_startNewPacket();
              packet_addUnsignedShort(TO_HOST_FILE);
              packet_addString(dest);
              packet_addUnsignedInt(filesize);
              packet_addUnsignedInt(packetstart);
            }
          }
          // Send off the packet
          net_sendPacketToHostGuaranteed();
        }
        fclose(fileread);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToHost(char *dirname, char *todirname)
{
  // ZZ> This function copies all files in a directory
  char searchname[0x80];
  char fromname[0x80];
  char toname[0x80];
  const char *searchResult;

  net_logf("net_copyDirectoryToHost: %s, %s\n", dirname, todirname);
  // Search for all files
  sprintf(searchname, "%s/*", dirname);
  searchResult = fs_findFirstFile(dirname, NULL);
  if (searchResult != NULL)
  {
    // Make the new directory
    net_copyFileToHost(dirname, todirname);

    // Copy each file
    while (searchResult != NULL)
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      // Also avoid copying directories in general.
      sprintf(fromname, "%s/%s", dirname, searchResult);
      if (searchResult[0] == '.' || fs_fileIsDirectory(fromname))
      {
        searchResult = fs_findNextFile();
        continue;
      }

      sprintf(fromname, "%s/%s", dirname, searchResult);
      sprintf(toname, "%s/%s", todirname, searchResult);

      net_copyFileToHost(fromname, toname);
      searchResult = fs_findNextFile();
    }
  }

  fs_findClose();
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToAllPlayers(char *dirname, char *todirname)
{
  // ZZ> This function copies all files in a directory
  char searchname[0x80];
  char fromname[0x80];
  char toname[0x80];
  const char *searchResult;

  net_logf("net_copyDirectoryToAllPlayers: %s, %s\n", dirname, todirname);
  // Search for all files
  sprintf(searchname, "%s/*.*", dirname);
  searchResult = fs_findFirstFile(dirname, NULL);
  if (searchResult != NULL)
  {
    // Make the new directory
    net_copyFileToAllPlayers(dirname, todirname);

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

      sprintf(fromname, "%s/%s", dirname, searchResult);
      sprintf(toname, "%s/%s", todirname, searchResult);
      net_copyFileToAllPlayers(fromname, toname);

      searchResult = fs_findNextFile();
    }
  }
  fs_findClose();
}

//--------------------------------------------------------------------------------------------
void net_sayHello()
{
  // ZZ> This function lets everyone know we're here
  if (GNet.on)
  {
    if (hostactive)
    {
      net_logf("net_sayHello: Server saying hello.\n");
      playersloaded++;
      if (playersloaded >= GNet.numplayer)
      {
        waitingforplayers = false;
      }
    }
    else
    {
      net_logf("net_sayHello: Client saying hello.\n");
      net_startNewPacket();
      packet_addUnsignedShort(TO_HOST_IM_LOADED);
      net_sendPacketToHostGuaranteed();
    }
  }
  else
  {
    waitingforplayers = false;
  }
}

//--------------------------------------------------------------------------------------------
void cl_talkToHost()
{
  // ZZ> This function sends the latch packets to the host machine
  Uint8 player;

  // Let the players respawn
  if (GKeyb.pressed(SDLK_SPACE)
      && (alllocalpladead || respawnanytime)
      && respawn_mode
      && !GRTS.on
      && !GNetMsg.mode)
  {
    player = 0;
    while (player<Player_List::SIZE)
    {
      if ( VALID_PLAYER(player) )
      {
        PlaList[player].latch.button |= LATCHBIT_RESPAWN;  // Press the respawn button...
      }
      player++;
    }
  }

  // Start talkin'
  if (GNet.on && !hostactive && !GRTS.on)
  {
    net_startNewPacket();
    packet_addUnsignedShort(TO_HOST_LATCH);     // The message header
    player = 0;
    while (player<Player_List::SIZE)
    {
      // Find the local players
      if ( VALID_PLAYER(player) )
      {
        packet_addUnsignedByte(player);                          // The player index
        packet_addUnsignedByte(PlaList[player].latch.button);          // Player button states
        packet_addSignedShort(PlaList[player].latch.x*SHORTLATCH);    // Player motion
        packet_addSignedShort(PlaList[player].latch.y*SHORTLATCH);    // Player motion
      }
      player++;
    }

    // Send it to the host
    net_sendPacketToHost();
  }
}

//--------------------------------------------------------------------------------------------
void sv_talkToRemotes()
{
  // ZZ> This function sends the character data to all the remote machines
  int player, time;

  if (wldframe > STARTTALK)
  {
    if (hostactive && !GRTS.on)
    {
      time = wldframe+ GNet.lag_tolerance;

      if (GNet.on)
      {
        // Send a message to all players
        net_startNewPacket();
        packet_addUnsignedShort(TO_REMOTE_LATCH);                         // The message header
        packet_addUnsignedInt(time);                                    // The stamp

        // Send all player latches...
        player = 0;
        while (player<Player_List::SIZE)
        {
          if ( VALID_PLAYER(player) )
          {
            packet_addUnsignedByte(player);                          // The player index
            packet_addUnsignedByte(PlaList[player].latch.button);          // Player button states
            packet_addSignedShort(PlaList[player].latch.x*SHORTLATCH);    // Player motion
            packet_addSignedShort(PlaList[player].latch.y*SHORTLATCH);    // Player motion
          }
          player++;
        }

        // Send the packet
        net_sendPacketToAllPlayers();
      }
      else
      {
        time = wldframe+1;
      }

    }
  }
}

//--------------------------------------------------------------------------------------------
void net_handlePacket(ENetEvent *event)
{
  Uint16 header;
  char filename[0x0100];   // also used for reading various strings
  int filesize, newfilesize, fileposition;
  char newfile;
  Uint16 player;
  Uint8  who;
  Uint32 stamp;
  Uint16 whichorder;
  Uint32 what, when;
  int cnt, time;
  FILE *file;
  size_t fileSize;

  net_logf("net_handlePacket: Received ");

  packet_startReading(event->packet);
  header = packet_readUnsignedShort();
  switch (header)
  {
    case TO_ANY_TEXT:
      net_logf("TO_ANY_TEXT\n");
      packet_readString(filename, 0xFF);
      debug_message(filename);
      break;

    case TO_HOST_MODULEOK:
      net_logf("TO_HOSTMODULEOK\n");
      if (hostactive)
      {
        playersready++;
        if (playersready >= GNet.numplayer)
        {
          readytostart = true;
        }
      }
      break;

    case TO_HOST_LATCH:
      net_logf("TO_HOST_LATCH\n");
      if (hostactive)
      {
        while (packet_remainingSize() > 0)
        {
          player = packet_readUnsignedByte();
          PlaList[player].latch.button = packet_readUnsignedByte();
          PlaList[player].latch.x = packet_readSignedShort() / SHORTLATCH;
          PlaList[player].latch.y = packet_readSignedShort() / SHORTLATCH;
        }

      }
      break;

    case TO_HOST_IM_LOADED:
      net_logf("TO_HOST_IMLOADED\n");
      if (hostactive)
      {
        playersloaded++;
        if (playersloaded == GNet.numplayer)
        {
          // Let the games begin...
          waitingforplayers = false;
          net_startNewPacket();
          packet_addUnsignedShort(TO_REMOTE_START);
          net_sendPacketToAllPlayersGuaranteed();
        }
      }
      break;

    case TO_HOST_RTS:
      net_logf("TO_HOST_RTS\n");
      if (hostactive)
      {
        whichorder = get_empty_order();
        if (whichorder < MAXORDER)
        {
          // Add the order on the host machine
          cnt = 0;
          while (cnt < MAXSELECT)
          {
            who = packet_readUnsignedByte();
            GOrder[whichorder].who[cnt] = who;
            cnt++;
          }
          what = packet_readUnsignedInt();
          when = wldframe + Order::lag;
          GOrder[whichorder].what = what;
          GOrder[whichorder].when = when;

          // Send the order off to everyone else
          net_startNewPacket();
          packet_addUnsignedShort(TO_REMOTE_RTS);
          cnt = 0;
          while (cnt < MAXSELECT)
          {
            packet_addUnsignedByte(GOrder[whichorder].who[cnt]);
            cnt++;
          }
          packet_addUnsignedInt(what);
          packet_addUnsignedInt(when);
          net_sendPacketToAllPlayersGuaranteed();
        }
      }
      break;

    case NET_TRANSFER_FILE:
      packet_readString(filename, 0x0100);
      fileSize = packet_readUnsignedInt();

      net_logf("NET_TRANSFER_FILE: %s with size %d.\n", filename, fileSize);

      // Try and save the file
      file = fopen(filename, "wb");
      if (file != NULL)
      {
        fwrite(net_readPacket->data + net_readLocation, 1, fileSize, file);
        fclose(file);
      }
      else
      {
        net_logf("net_handlePacket: Couldn't write new file!\n");
      }

      // Acknowledge that we got this file
      net_startNewPacket();
      packet_addUnsignedShort(NET_TRANSFER_OK);
      net_sendPacketToPeer(event->peer);

      // And note that we've gotten another one
      numfile++;
      break;

    case NET_TRANSFER_OK:
      net_logf("NET_TRANSFER_OK. The last file sent was successful.\n");
      net_waitingForXferAck = 0;
      net_numFileTransfers--;

      break;

    case NET_CREATE_DIRECTORY:
      packet_readString(filename, 0x0100);
      net_logf("NET_CREATE_DIRECTORY: %s\n", filename);

      fs_createDirectory(filename);

      // Acknowledge that we got this file
      net_startNewPacket();
      packet_addUnsignedShort(NET_TRANSFER_OK);
      net_sendPacketToPeer(event->peer);

      numfile++; // The client considers directories it sends to be files, so ya.
      break;

    case NET_DONE_SENDING_FILES:
      net_logf("NET_DONE_SENDING_FILES\n");
      numplayerrespond++;
      break;

    case NET_NUM_FILES_TO_SEND:
      net_logf("NET_NUM_FILES_TO_SEND\n");
      numfileexpected = (int)packet_readUnsignedShort();
      break;

    case TO_HOST_FILE:
      net_logf("TO_HOST_FILE\n");
      packet_readString(filename, 0xFF);
      newfilesize = packet_readUnsignedInt();

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
      fileposition = packet_readUnsignedInt();
      file = fopen(filename, "r+b");
      if (file)
      {
        if (fseek(file, fileposition, SEEK_SET) == 0)
        {
          while (packet_remainingSize() > 0)
          {
            fputc(packet_readUnsignedByte(), file);
          }
        }
        fclose(file);
      }
      break;

    case TO_HOST_DIR:
      net_logf("TO_HOST_DIR\n");
      if (hostactive)
      {
        packet_readString(filename, 0xFF);
        fs_createDirectory(filename);
      }
      break;

    case TO_HOST_FILESENT:
      net_logf("TO_HOST_FILESENT\n");
      if (hostactive)
      {
        numfileexpected += packet_readUnsignedInt();
        numplayerrespond++;
      }
      break;

    case TO_REMOTE_FILESENT:
      net_logf("TO_REMOTE_FILESENT\n");
      if (!hostactive)
      {
        numfileexpected += packet_readUnsignedInt();
        numplayerrespond++;
      }
      break;

    case TO_REMOTE_MODULE:
      net_logf("TO_REMOTE_MODULE\n");
      if (!hostactive && !readytostart)
      {
        seed = packet_readUnsignedInt();
        GRTS.team_local = packet_readUnsignedByte();
        localmachine = GRTS.team_local;

        packet_readString(filename, 0xFF);
        strcpy(pickedmodule, filename);

        // Check to see if the module exists
        pickedindex = find_module(pickedmodule);
        if (pickedindex == -1)
        {
          // The module doesn't exist locally
          // !!!BAD!!!  Copy the data from the host
          pickedindex = 0;
        }

        // Make ourselves ready
        readytostart = true;

        // Tell the host we're ready
        net_startNewPacket();
        packet_addUnsignedShort(TO_HOST_MODULEOK);
        net_sendPacketToHostGuaranteed();
      }
      break;

    case TO_REMOTE_START:
      net_logf("TO_REMOTE_START\n");
      if (!hostactive)
      {
        waitingforplayers = false;
      }
      break;

    case TO_REMOTE_RTS:
      net_logf("TO_REMOTE_RTS\n");
      if (!hostactive)
      {
        whichorder = get_empty_order();
        if (whichorder < MAXORDER)
        {
          // Add the order on the remote machine
          cnt = 0;
          while (cnt < MAXSELECT)
          {
            who = packet_readUnsignedByte();
            GOrder[whichorder].who[cnt] = who;
            cnt++;
          }
          what = packet_readUnsignedInt();
          when = packet_readUnsignedInt();
          GOrder[whichorder].what = what;
          GOrder[whichorder].when = when;
        }
      }
      break;

    case TO_REMOTE_FILE:
      net_logf("TO_REMOTE_FILE\n");
      if (!hostactive)
      {
        packet_readString(filename, 0xFF);
        newfilesize = packet_readUnsignedInt();

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
        fileposition = packet_readUnsignedInt();
        file = fopen(filename, "r+b");
        if (file)
        {
          if (fseek(file, fileposition, SEEK_SET) == 0)
          {
            while (packet_remainingSize() > 0)
            {
              fputc(packet_readUnsignedByte(), file);
            }
          }
          fclose(file);
        }
      }
      break;

    case TO_REMOTE_DIR:
      net_logf("TO_REMOTE_DIR\n");
      if (!hostactive)
      {
        packet_readString(filename, 0xFF);
        fs_createDirectory(filename);
      }
      break;

    case TO_REMOTE_LATCH:
      net_logf("TO_REMOTE_LATCH\n");
      if (!hostactive)
      {
        stamp = packet_readUnsignedInt();
        time = stamp; //time = stamp&LAGAND;
        if (nexttimestamp == -1)
        {
          nexttimestamp = stamp;
        }
        if (stamp < nexttimestamp)
        {
          net_logf("net_handlePacket: OUT OF ORDER PACKET\n");
          outofsync = true;
        }
        if (stamp <= wldframe)
        {
          net_logf("net_handlePacket: LATE PACKET\n");
          outofsync = true;
        }
        if (stamp > nexttimestamp)
        {
          net_logf("net_handlePacket: MISSED PACKET\n");
          nexttimestamp = stamp;  // Still use it
          outofsync = true;
        }
        if (stamp == nexttimestamp)
        {
          // Remember that we got it
          //PlaList.count_times++;

          // Read latches for each player sent
          while (packet_remainingSize() > 0)
          {
            player = packet_readUnsignedByte();
            Latch tmplatch;

            tmplatch.button = packet_readUnsignedByte();
            tmplatch.x      = packet_readSignedShort() / SHORTLATCH;
            tmplatch.y      = packet_readSignedShort() / SHORTLATCH;
            PlaList[player].timelatch.push_back( Player::timelatchpair(stamp, tmplatch) );
          }
          nexttimestamp = stamp+1;
        }
      }
      break;
  }
}

//--------------------------------------------------------------------------------------------
void listen_for_packets()
{
  // ZZ> This function reads any new messages and sets the player latch and matrix needed
  //     lists...
  ENetEvent event;

  if (GNet.on)
  {
    // Listen for new messages
    while (enet_host_service(net_myHost, &event, 0) != 0)
    {
      switch (event.type)
      {
        case ENET_EVENT_TYPE_RECEIVE:
          net_handlePacket(&event);
          enet_packet_destroy(event.packet);
          break;

        case ENET_EVENT_TYPE_CONNECT:
          // don't allow anyone to connect during the game session
          net_logf("listen_for_packets: Client tried to connect during the game: %x:%u\n",
                   event.peer->address.host, event.peer->address.port);
          enet_peer_disconnect(event.peer);

          break;

        case ENET_EVENT_TYPE_DISCONNECT:
          // Is this a player disconnecting, or just a rejected connection
          // from above?
          if (event.peer->data != 0)
          {
            NetPlayerInfo *info = (NetPlayerInfo *)event.peer->data;

            // uh oh, how do we handle losing a player?
            net_logf("listen_for_packets: Player %d disconnected!\n",
                     info->playerSlot);
          }
          break;
      }
    }
  }
}

//--------------------------------------------------------------------------------------------
void buffer_player_latches()
{
  // Now pretend the host got the packet...
  //int sTmp;
  for(int player=0; player < Player_List::SIZE; player++)
  {
    if ( INVALID_PLAYER(player) ) continue;

    int chr = PlaList[player].index;
    if ( INVALID_CHR(chr) ) continue;

    Player & plyr  = PlaList[player];
    Latch  & latch = plyr.latch;
    Latch    diff  = plyr.latch.diff(plyr.latch_old,10);

    if(!diff.is_null())
      PlaList[player].timelatch.push_back( Player::timelatchpair(wldframe+GNet.lag_tolerance, diff) );

    //PlaList[player].timelatch[time].button = PlaList[player].latch.button;

    //sTmp = PlaList[player].latch.x*SHORTLATCH;
    //PlaList[player].timelatch[time].x = sTmp/SHORTLATCH;

    //sTmp = PlaList[player].latch.y*SHORTLATCH;
    //PlaList[player].timelatch[time].y = sTmp/SHORTLATCH;

  }

  //PlaList.count_times++;
};


//--------------------------------------------------------------------------------------------
void unbuffer_player_latches()
{
  // ZZ> This function sets character latches based on player input to the host
  int cnt, time;

  // Copy the latches
  Latch diff_latch;
  Player::latchit it;
  int maxlag = 0, plyrlag = 0;
  for (cnt=0; cnt<Player_List::SIZE; cnt++)
  {
    if (INVALID_PLAYER(cnt) || GRTS.on) continue;
    Player & plyr = PlaList[cnt];

    int character = PlaList[cnt].index;
    if ( INVALID_CHR(character) ) continue;

    //bool updated = false;
    //diff_latch.clear();
    //Uint32 db = 0;
    //while( !plyr.timelatch.empty() )
    //{
    //  it = plyr.timelatch.begin();
    //  int remoteframe = it->first;

    //  if(remoteframe > wldframe) break;

    //  plyrlag = (wldframe - remoteframe);
    //  if(plyrlag > maxlag) maxlag = plyrlag;

    //  float factor  = (plyrlag+1)*(plyrlag+1);
    //  assert(factor>0);

    //  updated = true;
    //  diff_latch.x += it->second.x * factor;
    //  diff_latch.y += it->second.y * factor;
    //  diff_latch.button |= it->second.button;
    //  plyr.timelatch.pop_front();
    //};

    //if(updated)
    //{
    //  //save the old latch
    //  plyr.latch_old  = plyr.latch;

    //  //apply the diffs
    //  plyr.latch.x      += diff_latch.x;
    //  plyr.latch.y      += diff_latch.y;
    //  plyr.latch.button ^= diff_latch.button;
    //}
    //else
    //  plyr.latch     = plyr.latch_old;

    //transfer this diff_latch to the player's character
    ChrList[character].ai.latch = plyr.latch;
  }

  PlaList.count_times = maxlag + 1;
}

//--------------------------------------------------------------------------------------------
void chug_orders()
{
  // ZZ> This function takes care of lag in orders, issuing at the proper wldframe
  int cnt, character, tnc;

  cnt = 0;
  while (cnt < MAXORDER)
  {
    if (GOrder[cnt].valid && GOrder[cnt].when <= wldframe)
    {
      if (GOrder[cnt].when < wldframe)
      {
        debug_message("MISSED AN ORDER");
      }
      tnc = 0;
      while (tnc < MAXSELECT)
      {
        character = GOrder[cnt].who[tnc];
        if (VALID_CHR(character))
        {
          ChrList[character].order = GOrder[cnt].what;
          ChrList[character].counter = tnc;
          ChrList[character].ai.alert|=ALERT_IF_ORDERED;
        }
        tnc++;
      }
      GOrder[cnt].valid = false;
    }
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void send_rts_order(int x, int y, Uint8 order, Uint8 target)
{
  // ZZ> This function asks the host to order the selected characters
  Uint32 what, when, whichorder, cnt;

  if (GRTS.select_count > 0)
  {
    x = (x >> 6) & 0x03FF;
    y = (y >> 6) & 0x03FF;
    what = (target << 24) | (x << 14) | (y << 4) | (order&15);
    if (hostactive)
    {
      when = wldframe + Order::lag;
      whichorder = get_empty_order();
      if (whichorder != MAXORDER)
      {
        // Add a new order on own machine
        GOrder[whichorder].when = when;
        GOrder[whichorder].what = what;
        cnt = 0;
        while (cnt < GRTS.select_count)
        {
          GOrder[whichorder].who[cnt] = GRTS.select[cnt];
          cnt++;
        }
        while (cnt < MAXSELECT)
        {
          GOrder[whichorder].who[cnt] = Character_List::INVALID;
          cnt++;
        }

        // Send the order off to everyone else
        if (GNet.on)
        {
          net_startNewPacket();
          packet_addUnsignedShort(TO_REMOTE_RTS);
          cnt = 0;
          while (cnt < MAXSELECT)
          {
            packet_addUnsignedByte(GOrder[whichorder].who[cnt]);
            cnt++;
          }
          packet_addUnsignedInt(what);
          packet_addUnsignedInt(when);
          net_sendPacketToAllPlayersGuaranteed();
        }
      }
    }
    else
    {
      // Send the order off to the host
      net_startNewPacket();
      packet_addUnsignedShort(TO_HOST_RTS);
      cnt = 0;
      while (cnt < GRTS.select_count)
      {
        packet_addUnsignedByte(GRTS.select[cnt]);
        cnt++;
      }
      while (cnt < MAXSELECT)
      {
        packet_addUnsignedByte(Character_List::INVALID);
        cnt++;
      }
      packet_addUnsignedInt(what);
      net_sendPacketToHostGuaranteed();
    }
  }
}

//--------------------------------------------------------------------------------------------
void net_initialize()
{
  // ZZ> This starts up the network and logs whatever goes on
  serviceon = false;
  GNet.numsession = 0;
  GNet.numservice = 0;

  // Clear all the state variables to 0 to start.
  memset(net_playerPeers, 0, sizeof(ENetPeer*) * Player_List::SIZE);
  memset(net_playerInfo, 0, sizeof(NetPlayerInfo) * Player_List::SIZE);
  memset(packetbuffer, 0, MAXSENDSIZE);
  memset(net_transferStates, 0, sizeof(NetFileTransfer) * NET_MAX_FILE_TRANSFERS);
  memset(&net_receiveState, 0, sizeof(NetFileTransfer));

  // open up a file to log network information
  net_logFile = fopen("network.log", "wt");
  if (!net_logFile)
  {
    fprintf(stderr, "net_initialize: Could not open log file 'network.log'!\n");
  }

  if (GNet.on)
  {
    // initialize enet
    net_logf("net_initialize: Initializing enet...");
    if (enet_initialize() != 0)
    {
      net_logf("Failed!\n");
      GNet.on = false;
      serviceon = 0;
    }
    else
    {
      net_logf("Done\n");
      serviceon = true;
      GNet.numservice = 1;
    }
  }
  else
  {
    // We're not doing GNet.working this time...
    net_logf("net_initialize: Networking not enabled.\n");
  }
}

//--------------------------------------------------------------------------------------------
void net_shutDown()
{
  net_logf("net_shutDown: Turning off GNet.working.\n");
  enet_deinitialize();

  if (net_logFile)
  {
    fclose(net_logFile);
    net_logFile = NULL;
  }
}

//--------------------------------------------------------------------------------------------
void find_open_sessions()
{
  /*PORT
  // ZZ> This function finds some open games to join
  DPSESSIONDESC2      sessionDesc;
  HRESULT             hr;

  if(GNet.on)
  {
  GNet.numsession = 0;
  if(globalnetworkerr)  fprintf(globalnetworkerr, "  Looking for open games...\n");
  ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
  sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
  sessionDesc.guidApplication = NETWORKID;
  hr = lpDirectPlay3A->EnumSessions(&sessionDesc, 0, SessionsCallback, hGlobalWindow, DPENUMSESSIONS_AVAILABLE);
  if(globalnetworkerr)  fprintf(globalnetworkerr, "    %d sessions found\n", GNet.numsession);
  }
  */
}

//--------------------------------------------------------------------------------------------
void sv_letPlayersJoin()
{
  // ZZ> This function finds all the players in the game
  ENetEvent event;
  char hostName[0x40];

  // Check all pending events for players joining
  while (enet_host_service(net_myHost, &event, 0) > 0)
  {
    switch (event.type)
    {
      case ENET_EVENT_TYPE_CONNECT:
        // Look up the hostname the player is connecting from
        enet_address_get_host(&event.peer->address, hostName, 0x40);

        net_logf("sv_letPlayersJoin: A new player connected from %s:%u\n",
                 hostName, event.peer->address.port);

        // save the player data here.
        enet_address_get_host(&event.peer->address, hostName, 0x40);
        strncpy(GNet.playername[GNet.numplayer], hostName, 16);

        event.peer->data = &(net_playerInfo[GNet.numplayer]);
        GNet.numplayer++;

        break;

      case ENET_EVENT_TYPE_RECEIVE:
        net_logf("sv_letPlayersJoin: Recieved a packet when we weren't expecting it...\n");
        net_logf("\tIt came from %x:%u\n", event.peer->address.host, event.peer->address.port);

        // clean up the packet
        enet_packet_destroy(event.packet);
        break;

      case ENET_EVENT_TYPE_DISCONNECT:
        net_logf("sv_letPlayersJoin: A client disconnected!  Address %x:%u\n",
                 event.peer->address.host, event.peer->address.port);

        // Reset that peer's data
        event.peer->data = NULL;
    }
  }
}

//--------------------------------------------------------------------------------------------
int cl_joinGame(const char* hostname)
{
  // ZZ> This function tries to join one of the sessions we found
  ENetAddress address;
  ENetEvent event;

  if (GNet.on)
  {
    net_logf("cl_joinGame: Creating client network connection...");
    // Create my host thingamabober
    // TODO: Should I limit client bandwidth here?
    net_myHost = enet_host_create(NULL, 1, 0, 0);
    if (net_myHost == NULL)
    {
      // can't create a network connection at all
      net_logf("Failed!\n");
      return false;
    }
    net_logf("Succeeded\n");

    // Now connect to the remote host
    net_logf("cl_joinGame: Attempting to connect to %s:%d\n", hostname, NET_EGOBOO_PORT);
    enet_address_set_host(&address, hostname);
    address.port = NET_EGOBOO_PORT;
    net_gameHost = enet_host_connect(net_myHost, &address, NET_EGOBOO_NUM_CHANNELS);
    if (net_gameHost == NULL)
    {
      net_logf("cl_joinGame: No available peers to create a connection!\n");
      return false;
    }

    // Wait for up to 5 seconds for the connection attempt to succeed
    if (enet_host_service(net_myHost, &event, 5000) > 0 && 
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
      net_logf("cl_joinGame: Connected to %s:%d\n", hostname, NET_EGOBOO_PORT);
      return true;
      // return create_player(false);
    }
    else
    {
      net_logf("cl_joinGame: Could not connect to %s:%d!\n", hostname, NET_EGOBOO_PORT);
    }
  }
  return false;
}

//--------------------------------------------------------------------------------------------
void stop_players_from_joining()
{
  // ZZ> This function stops players from joining a game
}

//--------------------------------------------------------------------------------------------
int sv_hostGame()
{
  // ZZ> This function tries to host a new session
  ENetAddress address;

  if (GNet.on)
  {
    // Try to create a new session
    address.host = ENET_HOST_ANY;
    address.port = NET_EGOBOO_PORT;

    net_logf("sv_hostGame: Creating game on port %d\n", NET_EGOBOO_PORT);
    net_myHost = enet_host_create(&address, Player_List::SIZE, 0, 0);
    if (net_myHost == NULL)
    {
      net_logf("sv_hostGame: Could not create network connection!\n");
      return false;
    }

    // Try to create a host player
//  return create_player(true);
    net_amHost = true;

    // Moved from net_sayHello because there they cause a race issue
    waitingforplayers = true;
    playersloaded = 0;
  }
  // Run in solo mode
  return true;
}

//--------------------------------------------------------------------------------------------
void turn_on_service(int service)
{
  // This function turns on a network service ( IPX, TCP, serial, modem )
}

int  net_pendingFileTransfers()
{
  return net_numFileTransfers;
}

Uint8 *transferBuffer = NULL;
size_t   transferSize = 0;

void net_updateFileTransfers()
{
  NetFileTransfer *state;
  ENetPacket *packet;
  size_t nameLen, fileSize;
  Uint32 networkSize;
  FILE *file;
  char *p;

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
        net_logf("net_updateFileTranfers: Creating directory %s on target\n", state->destName);
        net_startNewPacket();
        packet_addUnsignedShort(NET_CREATE_DIRECTORY);
        packet_addString(state->destName);
        net_sendPacketToPeerGuaranteed(state->target);

        net_waitingForXferAck = 1;
      }
      else
      {
        file = fopen(state->sourceName, "rb");
        if (file)
        {
          net_logf("net_updateFileTransfers: Attempting to send %s to %s\n", state->sourceName, state->destName);

          fseek(file, 0, SEEK_END);
          fileSize = ftell(file);
          fseek(file, 0, SEEK_SET);

          // Make room for the file's name
          nameLen = strlen(state->destName) + 1;
          transferSize = nameLen;

          // And for the file's size
          transferSize += 6; // Uint32 size, and Uint16 message type
          transferSize += fileSize;

          transferBuffer = (Uint8 *)malloc(transferSize);
          *(Uint16*)transferBuffer = ENET_HOST_TO_NET_16(NET_TRANSFER_FILE);

          // Add the string and file length to the buffer
          p = (char *)(transferBuffer + 2);
          strcpy(p, state->destName);
          p += nameLen;

          networkSize = ENET_HOST_TO_NET_32((unsigned long)fileSize);
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
          net_logf("net_updateFileTransfers: Could not open file %s to send it!\n", state->sourceName);
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
  listen_for_packets();
}
