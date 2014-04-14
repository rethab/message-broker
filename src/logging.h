#ifndef LOGGING_HEADER
#define LOGGING_HEADER

#include <stdio.h>

typedef enum { ERR = 4, WARN = 3, INFO = 2, DBG = 1 } LOGLVL;

/* logs the string to stderr depending on the log level */
void logging(LOGLVL lvl, const char *msg);

#endif
