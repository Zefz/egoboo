// Egoboo - Log.c

#include "Log.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

#define MAX_LOG_MESSAGE 0x0400

static FILE *logFile = NULL;
static char logBuffer[MAX_LOG_MESSAGE];
static int logLevel = 1;

static void writeLogMessage(const char *prefix, const char *format, va_list args)
{
  if (logFile != NULL)
  {
    vsnprintf(logBuffer, MAX_LOG_MESSAGE-1, format, args);
    fputs(prefix, logFile);
    fputs(logBuffer, logFile);
  }
}

void log_init()
{
  if (logFile == NULL)
  {
    logFile = fopen("log.txt", "wt");
    atexit(log_shutdown);
  }
}

void log_shutdown()
{
  if (logFile != NULL)
  {
    fclose(logFile);
    logFile = NULL;
  }
}

void log_setLoggingLevel(int level)
{
  if (level > 0)
  {
    logLevel = level;
  }
}

void log_message(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  writeLogMessage("", format, args);
  va_end(args);
}

void log_info(const char *format, ...)
{
  va_list args;

  if (logLevel >= 2)
  {
    va_start(args, format);
    writeLogMessage("INFO: ", format, args);
    va_end(args);
  }
}

void log_warning(const char *format, ...)
{
  va_list args;

  if (logLevel >= 1)
  {
    va_start(args, format);
    writeLogMessage("WARN: ", format, args);
    va_end(args);
  }
}

void log_error(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  writeLogMessage("FATAL ERROR: ", format, args);
  va_end(args);

  fflush(logFile);
  exit(-1);
}
