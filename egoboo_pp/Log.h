// Egoboo - Log.h
// Logging & stack trace functionality.

#ifndef egoboo_Log_h
#define egoboo_Log_h

void log_init();
void log_shutdown(void);

void log_setLoggingLevel(int level);

void log_message(const char *format, ...);
void log_info(const char *format, ...);
void log_warning(const char *format, ...);
void log_error(const char *format, ...);

#endif // include guard
