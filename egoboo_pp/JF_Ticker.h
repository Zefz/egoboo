// Egoboo - Ticker.h
// Time-keeping objects that track how many updates should occur within
// a given timespan

#pragma once

#define egoboo_Ticker_h

namespace JF
{
  struct Ticker
  {
    double lastTime;
    double tickInterval;
    int numTicks;

    static void initWithInterval(Ticker *ticker, double interval);
    static void initWithFrequency(Ticker *ticker, int freq);

    static void update(Ticker *ticker);
    static int  tick(Ticker *ticker);

  };

}
