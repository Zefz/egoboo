// Egoboo - Ticker.c

#include "JF_Ticker.h"
#include "JF_Clock.h"
#include <assert.h>
#include <stddef.h>

using namespace JF;

void Ticker::initWithInterval(Ticker *ticker, double interval)
{
  assert(ticker != NULL && "Ticker::initWithInterval: NULL ticker passed!");
  if (ticker == NULL || interval <= 0) return;

  ticker->lastTime = GClock.getTime();
  ticker->numTicks = 0;
  ticker->tickInterval = interval;
}

void Ticker::initWithFrequency(Ticker *ticker, int freq)
{
  double interval = 1.0 / freq;
  initWithInterval(ticker, interval);
}

void Ticker::update(Ticker *ticker)
{
  double deltaTime, currentTime;
  assert(ticker != NULL && "Ticker::update: NULL ticker passed!");
  if (ticker == NULL) return;

  currentTime = GClock.getTime();
  deltaTime = currentTime - ticker->lastTime;

  while (deltaTime > ticker->tickInterval)
  {
    ticker->numTicks++;
    ticker->lastTime += ticker->tickInterval;

    deltaTime -= ticker->tickInterval;
  }
}

int Ticker::tick(Ticker *ticker)
{
  int numTicks;

  assert(ticker != NULL && "Ticker::tick: NULL ticker passed!");
  if (ticker == NULL) return 0;

  numTicks = ticker->numTicks;
  ticker->numTicks--;
  return numTicks;
}
