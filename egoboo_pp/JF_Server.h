// Egoboo - Server.h
// Interface to the game server code

#pragma once

#define egoboo_Server_h

namespace JF
{
  typedef struct GameState GameState;

  struct Server_State
  {
    int dummy;
    // GameState gameState;
  };

  struct Server_Manager
  {
    int  init(Server_State*);
    void shutDown(Server_State*);
    void frameStep(Server_State*);

    // More to come...
    // int  beginSinglePlayer(Server_State*, ...)
    // int  beginMultiPlayer(Server_State*, ...)
    // int  loadModule(Server_State*, ...)
  };
};

// Globally accessible server state
extern JF::Server_State AServerState;