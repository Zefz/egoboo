// Egoboo - Client.h
// Interface to the client game code

#pragma once

#define egoboo_Client_h

namespace JF
{
  struct Client_State
  {
    int dummy;
  };

  struct Client_Manager
  {
    int  init(Client_State *);
    void shutDown(Client_State *);
    void frameStep(Client_State *);

    // Much more to come...

    //int  connectToServer(Client_State *, ...);
    //int  loadModule(Client_State *, ...);
  };

};

// Globally accesible client state
extern JF::Client_State AClientState;