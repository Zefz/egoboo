// Egoboo - Clock.h
// Interface for the clock & timer module
// This implementation was adapted from Noel Lopis'
// article in Game Programming Gems 4.

#pragma once

#define Egoboo_Clock_h

#include "egobootypedef.h"
#include <stdlib.h>

namespace JF
{
  struct Clock
  {
    typedef double(*PSOURCE)();

    double sourceStartTime;  // The first value the clock receives from above function
    double sourceLastTime;  // The last value the clock received from above function
    double currentTime;   // The current time, not necessarily in sync w/ the source time
    double frameTime;   // The time this frame takes
    Uint32 frameNumber; // Which frame the clock is on

    // Circular buffer to hold frame histories
    double *frameHistory;
    int frameHistorySize;
    int frameHistoryWindow;
    int frameHistoryHead;

    const double maximumFrameTime; // The maximum time delta the clock accepts (default .2 seconds)

    Clock(const double max_frame_time = 0.2) :
        maximumFrameTime(max_frame_time),
        frameHistory(NULL) {};

    void init();      // Init the clock module
    void shutdown();     // Turn off the clock module

    void setTimeSource(double(*timeSource)());  // Specify where the clock gets its time values from
    // Defaults to sys_getTime()
    void setFrameHistoryWindow(int size);   // Set how many frames to keep a length history of
    // Defaults to 1

    void frameStep();     // Update the clock.
    double getTime();     // Returns the current time.  The clock's time only
    // changes when frameStep is called

    double getFrameDuration();  // Return the length of the current frame. (Sort of.)
    Uint32 getFrameNumber(); // Return which frame we're on
    float getFrameRate();    // Return the current instantaneous FPS

  protected:

    static double(*timeSource)();   // Function that the clock get it's time values from

    void addToFrameHistory(double frame);
    double getExactLastFrameDuration();
    double guessFrameDuration();
  };

}

extern JF::Clock GClock;
