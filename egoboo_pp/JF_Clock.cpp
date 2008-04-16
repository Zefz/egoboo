// Egoboo - timing.c
// Clock & timer functionality
// This implementation was adapted from Noel Lopis'
// article in Game Programming Gems 4.

#include "proto.h"
#include "JF_Clock.h"
#include "Log.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h> // memcpy & memset

using namespace JF;

Clock GClock;

Clock::PSOURCE Clock::timeSource = NULL;

void Clock::init()
{
  log_info("Initializing clock services...\n");

  shutdown(); // Use this to set everything to 0

  setTimeSource(sys_getTime);
  setFrameHistoryWindow(1);
}

void Clock::shutdown()
{
  if (frameHistory != NULL)
  {
    free(frameHistory);
    frameHistory = NULL;
  }

  timeSource = NULL;
  sourceStartTime = 0;
  sourceLastTime = 0;
  currentTime = 0;
  frameTime = 0;
  frameNumber = 0;

  frameHistorySize = 0;
  frameHistoryWindow = 0;
  frameHistoryHead = 0;
}

void Clock::setTimeSource(double(*ts)())
{
  timeSource = ts;

  if (timeSource)
  {
    sourceStartTime = timeSource();
    sourceLastTime = sourceStartTime;
  }
}

void Clock::setFrameHistoryWindow(int size)
{
  double *history;
  int oldSize = frameHistoryWindow;
  int less;

  // The frame history has to be at least 1
  frameHistoryWindow = (size > 1) ? size : 1;
  history = (double*)malloc(sizeof(double) * frameHistoryWindow);

  if (frameHistory != NULL)
  {
    // Copy over the older history.  Make sure that only the size of the
    // smaller buffer is copied
    less = (frameHistoryWindow < oldSize) ? frameHistoryWindow : oldSize;
    memcpy(history, frameHistory, less);
    free(frameHistory);
  }
  else
  {
    memset(history, 0, sizeof(double) * frameHistoryWindow);
  }

  frameHistoryHead = 0;
  frameHistory = history;
}

double Clock::guessFrameDuration()
{
  int c;
  double totalTime = 0;

  for (c = 0;c < frameHistorySize;c++)
  {
    totalTime += frameHistory[c];
  }

  return totalTime / frameHistorySize;
}

void Clock::addToFrameHistory(double frame)
{
  frameHistory[frameHistoryHead] = frame;

  frameHistoryHead++;
  if (frameHistoryHead >= frameHistoryWindow)
  {
    frameHistoryHead = 0;
  }

  frameHistorySize++;
  if (frameHistorySize > frameHistoryWindow)
  {
    frameHistorySize = frameHistoryWindow;
  }
}

double Clock::getExactLastFrameDuration()
{
  double sourceTime;
  double timeElapsed;

  if (timeSource)
  {
    sourceTime = timeSource();
  }
  else
  {
    sourceTime = 0;
  }

  timeElapsed = sourceTime - sourceLastTime;

  // If more time elapsed than the maximum we allow, say that only the maximum occurred
  if (timeElapsed > maximumFrameTime)
  {
    timeElapsed = maximumFrameTime;
  }

  sourceLastTime = sourceTime;
  return timeElapsed;
}

void Clock::frameStep()
{
  double lastFrame = getExactLastFrameDuration();
  addToFrameHistory(lastFrame);

  // This feels wrong to me; we're guessing at how long this
  // frame is going to be and accepting that as our time value.
  // I'll trust Mr. Lopis for now, but it may change.
  frameTime = guessFrameDuration();
  currentTime += frameTime;

  frameNumber++;
}

double Clock::getTime()
{
  return currentTime;
}

double Clock::getFrameDuration()
{
  return frameTime;
}

Uint32 Clock::getFrameNumber()
{
  return frameNumber;
}

float Clock::getFrameRate()
{
  return (float)(1.0 / frameTime);
}
