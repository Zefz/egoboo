// Egoboo - sys_win32.c
// Microsoft Windows-specific platform code

#include "proto.h"
#include "Log.h"
#include "System.h"
#include <windows.h>

double win32_secondsPerTick = 0;

void sys_initialize()
{
  LARGE_INTEGER frequency;
  Uint32 f;

  log_info("Initializing high-performance counter...\n");

  QueryPerformanceFrequency(&frequency);
  win32_secondsPerTick = 1.0 / frequency.QuadPart;

  f = frequency.QuadPart;
  log_info("Frequency is %d hz\n", f);
}

void sys_shutdown()
{}

double sys_getTime()
{
  LARGE_INTEGER time;

  QueryPerformanceCounter(&time);
  return time.QuadPart * win32_secondsPerTick;
}

int sys_frameStep()
{
  return 0;
}
