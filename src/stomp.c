#include <string.h>
#include <stdlib.h>

#include "stomp.h"

/*
 * parses header key value pairs from the
 * null-terminated raw string. each header is
 * on one line and the end of the headers marks
 * an empty line.
 *
 * the secod parameter is the number of headers
 * expected. this function returns an error code
 * if more than the expected headers are found.
 *
 * the third parameter points to a pre allocated
 * header array of size 'expected'.
 *
 * the return code is 0 on success or any of the
 * STOMP_ error codes on failure.
 */
static int parse_header(char* raw, size_t expected,
                        struct stomp_header** headers) {
    int i = 0;
    char* line;
    char* saferaw;
    line = strtok_r(raw, "\n", &saferaw);
    if (line == NULL) return STOMP_MISSING_HEADER;
    // at most expected, unless end or empty
    while (i < expected && line != NULL && strnlen(line, 1) != 0) {
        char *key = strtok_r(line, ":", &line);
        if (key == NULL) return STOMP_INVALID_HEADER;
        char *val = strtok_r(NULL, ":", &line);
        if (val == NULL) return STOMP_INVALID_HEADER;
        if (strtok_r(NULL, ":", &line) != NULL)
            return STOMP_INVALID_HEADER;
        
        headers[i]->key = key;
        headers[i]->val = val;

        line = strtok_r(NULL, "\n", &raw);
        i++;
    }

    if (i < (expected-1)) {
        return STOMP_MISSING_HEADER;   
    } else if (strnlen(line, 1) != 0) {
        return STOMP_INVALID_HEADER;   
    } else {
        return 0;
    }
}

static int parse_content(char* raw, size_t expected, char** content) {
    return -1;
}

static int parse_command_connect(char* raw, struct stomp_command* cmd) {
    // parse header 'login'
    int nheaders = 1;
    struct stomp_header* headers =
            malloc(sizeof(struct stomp_header) * nheaders);
    int parsed = parse_header(raw, nheaders, &headers);
    if (parsed != 0) {
        return parsed;   
    }

    // expect no content
    parsed = parse_content(raw, 0, NULL);
    if (parsed != 0) {
        return parsed;
    }

    cmd->name = "CONNECT";
    cmd->headers = headers;
    cmd->content = NULL;
    return 0;
}

int parse_command(char* raw, struct stomp_command* cmd) {
    if (strncmp("CONNECT\n", raw, 8) == 0) {
        return parse_command_connect(raw+8, cmd);
    } else {
        return STOMP_UNKNOWN_COMMAND;
    }
}

int create_command(struct stomp_command cmd, char* str) {
    return 0;
}
