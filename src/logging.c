#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#define LOGLEVEL DBG

typedef enum { ERR = 4, WARN = 3, INFO = 2, DBG = 1 } LOGLVL;

static pthread_mutex_t stderr_mutex = PTHREAD_MUTEX_INITIALIZER;

static void lvl_to_string(LOGLVL lvl, char **strlevel) {
    switch (lvl) {
        case ERR:
            *strlevel = strdup("ERR");
            break;
        case WARN:
            *strlevel = strdup("WARN");
            break;
        case INFO:
            *strlevel = strdup("INFO");
            break;
        case DBG:
            *strlevel = strdup("DBG");
            break;
        default:
            fprintf(stderr, "Unhandled LogLevel: %d\n", lvl);
            exit(EXIT_FAILURE);
    }
}

void logging(LOGLVL lvl, const char *msg) {
    char *strlevel;
    if (lvl < LOGLEVEL) {
        return;
    }
    lvl_to_string(lvl, &strlevel);
    assert(pthread_mutex_lock(&stderr_mutex) == 0);
    fprintf(stderr, "%s: %s\n", strlevel, msg);
    assert(pthread_mutex_unlock(&stderr_mutex) == 0);
    free(strlevel);
}
