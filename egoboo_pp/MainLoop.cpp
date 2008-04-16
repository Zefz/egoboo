// Egoboo - MainLoop.c
// Implementation of the main game loop.  main() calls this once everything
// is set up; when the main loop returns the game exits.

#include "Log.h"
#include "JF_Clock.h"
#include "System.h"
#include "JF_Server.h"
#include "JF_Client.h"
#include "Menu.h"

int quitRequested = 0;

JF::Server_Manager SManager;
JF::Client_Manager CManager;

int MainLoop()
{
  int rVal = 0;

  log_info("Entering main loop.");

  while (!quitRequested)
  {
    // Absolute first thing to do is update the clock
    GClock.frameStep();

    // Next, let the system module gather any input from this frame, and
    // process any window system events.  This could result in having to
    // quit rather quickly
    rVal = sys_frameStep();
    if (rVal != 0) break;

    // Update the server component.  This component is responsible for updating
    // the game world as well. (Gameplay is hosted by the server)
    SManager.frameStep(&AServerState);

    // Update the client component.  This is where my stuff breaks down... shoot.
    // How do I fit menus & ui in with the client-server model?
    // Also, world rendering happens inside the client
    CManager.frameStep(&AClientState);

    // Update menus that exist outside of the client/server structure.  This is
    // stuff like the menus that you run through before the game starts, so most
    // of the time this will be inactive.
    // The menu might request that the game quit, but I don't need to worry about
    // checking it right now because it's the last thing that happens in the loop.
    //menu_frameStep();
  }

  log_info("Leaving main loop.");
  return (rVal != 0) ? rVal : quitRequested; // Return whichever one isn't zero
}
