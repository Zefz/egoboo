/* Egoboo - Network.h
 * Definitions for Egoboo network functionality
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

#ifndef egoboo_Network_h
#define egoboo_Network_h

#include <enet/enet.h>
#include <SDL_thread.h>
#include "egoboo.h"

struct GameState_t;
struct ClientState_t;
struct ServerState_t;

#define NETREFRESH          1000                    // Every second
#define NONETWORK           numservice              //

#define SHORTLATCH 1024.0
#define MAXSENDSIZE 8192
#define COPYSIZE    4096
#define TOTALSIZE   2097152

#define MAXLAG      (1<<6)                          //
#define LAGAND      (MAXLAG-1)                      //
#define STARTTALK   (MAXLAG>>3)                     //

#define INVALID_TIMESTAMP ((Uint32)(-1))

// these values for the message headers were generated using a ~15 bit linear congruential
// generator.  They should be relatively good choices that are free from any bias...
enum network_packet_types_e
{
  TO_ANY_TEXT             = 0x1D66,

  TO_HOST_LOGON           = 0xF8E1,
  TO_HOST_LOGOFF          = 0x3E46,
  TO_HOST_REQUEST_MODULE  = 0x5DB6,
  TO_HOST_MODULE          = 0xEFD0,
  TO_HOST_MODULEOK        = 0xA31A,
  TO_HOST_MODULEBAD       = 0xB473,
  TO_HOST_LATCH           = 0xF296,
  TO_HOST_RTS             = 0x0A47,
  TO_HOST_IM_LOADED       = 0xDD2C,
  TO_HOST_FILE            = 0xBC6B,
  TO_HOST_DIR             = 0xDB29,
  TO_HOST_FILESENT        = 0x6437,

  TO_REMOTE_LOGON         = 0x763A,
  TO_REMOTE_LOGOFF        = 0x989B,
  TO_REMOTE_KICK          = 0xA64F,
  TO_REMOTE_MODULE        = 0xF068,
  TO_REMOTE_MODULEBAD     = 0x2FE9,
  TO_REMOTE_MODULEINFO    = 0x00E8,
  TO_REMOTE_LATCH         = 0x9526,
  TO_REMOTE_FILE          = 0xA04E,
  TO_REMOTE_DIR           = 0x13E2,
  TO_REMOTE_RTS           = 0x4817,
  TO_REMOTE_START         = 0x27AF,
  TO_REMOTE_FILESENT      = 0x81B9,


  NET_CHECK_CRC           = 0x88CF,  // Someone is asking us to check the CRC of a certain file
  NET_ACKNOWLEDGE_CRC     = 0x6240,  // Someone is acknowledging a CRC request from us
  NET_SEND_CRC            = 0xA0E1,  // Someone is sending us a CRC
  NET_TRANSFER_FILE       = 0x16B5,  // Packet contains a file.
  NET_REQUEST_FILE        = 0x9ABF,  // Someone has asked us to send them a file
  NET_TRANSFER_OK         = 0x2B22,  // Acknowledgement packet for a file send
  NET_CREATE_DIRECTORY    = 0xCFC9,  // Tell the peer to create the named directory
  NET_DONE_SENDING_FILES  = 0xCB08,  // Sent when there are no more files to send.
  NET_NUM_FILES_TO_SEND   = 0x9E40,  // Let the other person know how many files you're sending

  // Unused values
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = 0x9840,
  //XXXX                    = 0xB312,
  //XXXX                    = 0x9D13,
  //XXXX                    = 0xEB6C,
  //XXXX                    = 0x4151,
  //XXXX                    = 0xA140,
  //XXXX                    = 0xB5DD,
  //XXXX                    = 0x7A54,
  //XXXX                    = 0xB64D,
};


// Networking constants
enum NetworkConstant
{
  NET_UNRELIABLE_CHANNEL  = 0,
  NET_GUARANTEED_CHANNEL  = 1,
  NET_EGOBOO_NUM_CHANNELS,
  NET_MAX_FILE_NAME       = 128,
  NET_MAX_FILE_TRANSFERS  = 1024, // Maximum files queued up at once
  NET_EGOBOO_SERVER_PORT  = 0x8742,
  NET_EGOBOO_CLIENT_PORT  = 0x8743
};

typedef struct packet_request_t
{
  bool_t      waiting, received;
  Uint32      starttimestamp, expiretimestamp;
  ENetAddress address;
  Uint16      packettype;
  size_t      data_size;
  void       *data;
} PACKET_REQUEST;

// Network players information
typedef struct NetPlayerInfo
{
  int playerSlot;
} NetPlayerInfo;

struct GameState_t;
struct ClientState_t;
struct ServerState_t;

typedef struct NetState_t
{
  // thread management
  bool_t       kill_me;
  SDL_Thread * my_thread;

  bool_t networkon;
  bool_t serviceon;          // Do I need to free the interface?
  int    numservice;                             // How many we found

  int    service;
  char   servicename[MAXSERVICE][NETNAMESIZE];    // Names of services

  FILE                 * logfile;
  struct GameState_t   * gs;
  struct ClientState_t * cs;
  struct ServerState_t * ss;

  PACKET_REQUEST request_buffer[16];
} NET_STATE;

extern NET_STATE ANetState;

// Packet writing
typedef struct local_packet_t
{
  Uint32 head;                             // The write head
  Uint32 size;                             // The size of the packet
  Uint8  buffer[MAXSENDSIZE];              // The data packet
} PACKET;

typedef struct stream_t
{
  FILE  * pfile;
  Uint8 * data;
  size_t data_size, readLocation;
} STREAM;

typedef struct remote_packet_t
{
  ENetPacket * pkt;
  STREAM wrapper;
} NET_PACKET;



int net_Callback(void *);


//---------------------------------------------------------------------------------------------
// Networking functions
void net_initialize(NET_STATE * ns, struct ClientState_t * cs, struct ServerState_t * ss, struct GameState_t * gs);
void net_shutDown(NET_STATE * ns);
void net_logf(NET_STATE * ns, const char *format, ...);

void net_startNewPacket(PACKET * egop);
retval_t net_checkCRC(NET_STATE * ns, ENetPeer * peer, const char * source, Uint32 seed, Uint32 CRC);

void packet_addUnsignedByte(PACKET * egop, Uint8 uc);
void packet_addSignedByte(PACKET * egop, Sint8 sc);
void packet_addUnsignedShort(PACKET * egop, Uint16 us);
void packet_addSignedShort(PACKET * egop, Sint16 ss);
void packet_addUnsignedInt(PACKET * egop, Uint32 ui);
void packet_addSignedInt(PACKET * egop, Sint32 si);
void packet_addString(PACKET * egop, char *string);
void packet_addFString(PACKET * egop, char *format, ...);

bool_t stream_startRaw(STREAM * pwrapper, Uint8 * buffer, size_t buffer_size);
bool_t stream_startLocal(STREAM * pwrapper, PACKET * pegopkt);
bool_t stream_startENet(STREAM * pwrapper, ENetPacket * packet);
bool_t stream_startRemote(STREAM * pwrapper, NET_PACKET * pnetpkt);
bool_t stream_done(STREAM * pwrapper);

bool_t stream_readString(STREAM * p, char *buffer, int maxLen);
Uint8  stream_readUnsignedByte(STREAM * p);
Sint8  stream_readSignedByte(STREAM * p);
Uint16 stream_readUnsignedShort(STREAM * p);
Uint16 stream_peekUnsignedShort(STREAM * p);
Sint16 stream_readSignedShort(STREAM * p);
Uint32 stream_readUnsignedInt(STREAM * p);
Sint32 stream_readSignedInt(STREAM * p);

void packet_startReading(NET_PACKET * p, ENetPacket * enpkt);
void packet_doneReading(NET_PACKET * p);
size_t packet_remainingSize(NET_PACKET * p);

bool_t packet_readString(NET_PACKET * p, char *buffer, int maxLen);
Uint8  packet_readUnsignedByte(NET_PACKET * p);
Sint8  packet_readSignedByte(NET_PACKET * p);
Uint16 packet_readUnsignedShort(NET_PACKET * p);
Uint16 packet_peekUnsignedShort(NET_PACKET * p);
Sint16 packet_readSignedShort(NET_PACKET * p);
Uint32 packet_readUnsignedInt(NET_PACKET * p);
Sint32 packet_readSignedInt(NET_PACKET * p);

void net_sendPacketToHost(struct ClientState_t * cs, PACKET * egop);
void net_sendPacketToAllPlayers(struct ServerState_t * ss, PACKET * egop);
void net_sendPacketToHostGuaranteed(struct ClientState_t * cs, PACKET * egop);
void net_sendPacketToAllPlayersGuaranteed(struct ServerState_t * ss, PACKET * egop);
void net_sendPacketToOnePlayerGuaranteed(ENetPeer * peer, PACKET * egop);

void net_updateFileTransfers(NET_STATE * ns);
int  net_pendingFileTransfers(NET_STATE * ns);

void net_copyFileToAllPlayers(struct ServerState_t * ss, char *source, char *dest);
void net_copyFileToHost(NET_STATE * ns, char *source, char *dest);
void net_copyDirectoryToHost(NET_STATE * ns, char *dirname, char *todirname);
void net_copyDirectoryToAllPlayers(NET_STATE * ns, char *dirname, char *todirname);
retval_t net_copyDirectoryToOnePlayer(NET_STATE * ns, ENetPeer * peer, char *dirname, char *todirname);

void input_net_message(GAME_STATE * gs);
void net_sayHello(NET_STATE * ns);
bool_t  net_beginGame(struct GameState_t * ns);

void close_session(struct NetState_t * ns);

bool_t listen_for_packets(NET_STATE * ns);
//void find_open_sessions(NET_STATE * ns);
void stop_players_from_joining(NET_STATE * ns);
//int create_player(int host);
//void turn_on_service(int service);
bool_t net_handlePacket(NET_STATE * ns, ENetEvent *event);

PACKET_REQUEST * net_checkRequest(PACKET_REQUEST * prlist, size_t prcount, ENetEvent * pevent);
PACKET_REQUEST * net_getRequest(PACKET_REQUEST * prlist, size_t prcount);
bool_t net_testRequest(PACKET_REQUEST * prequest);
bool_t net_releaseRequest(PACKET_REQUEST * prequest);

retval_t net_waitForPacket(PACKET_REQUEST * request_buff, size_t request_buff_sz, ENetPeer * peer, Uint32 timeout, Uint8 * buffer, size_t buffer_sz, Uint16 packet_type, size_t * data_size);

#endif