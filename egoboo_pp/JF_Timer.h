// Egoboo - Timer.h
// Implements the timer portion of the timing module.

#pragma once

#define Egoboo_Timer_h

namespace JF
{
  class Timer
  {
    public:
	    Timer();

	    // Update's the timers internal counters.  This is the *only* way that the
	    // internal values will change; any call to currenTime returns the time when
	    // the timer was last updated.
	    void update();

	    // Return the 'current' time; when update was last called.
	    unsigned int currentTime() const { return m_currentTime; }
    	
	    // Return the previous 'current' time
	    unsigned int lastTime() const { return m_lastTime; }

	    // Return the difference between the last two calls to update
	    unsigned int deltaTime() const { return m_currentTime - m_lastTime; }

    private:
	    unsigned int m_created;
	    unsigned int m_lastTime;
	    unsigned int m_currentTime;
  };
};
