// Egoboo - Timer.c
// Implementation of the timer code

#include "JF_Timer.h"

using namespace JF;

#ifdef WIN32
#include <windows.h>
#define timeFunc timeGetTime
#else
#include <SDL.h>
#define timeFunc SDL_GetTicks
#endif

Timer::Timer()
{
	m_created = timeFunc();
	m_lastTime = m_currentTime = m_created;
}

void Timer::update()
{
	m_lastTime = m_currentTime;
	m_currentTime = timeFunc();
}
