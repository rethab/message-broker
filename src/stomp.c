#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "stomp.h"

/* trims a string on boths sides */
static void trim(char **str) {
    int i;

    // drop from left
    while (**str == ' ') (*str)++;

    i = strlen(*str) - 1;
    // move 0 on the to the left
    while (i > 0 && (*str)[i] == ' ') (*str)[i--] = '\0';
}

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
static int parse_header(char *raw, size_t expected,
                        struct stomp_header **headers) {
    char *saveraw;
    size_t i = 0;
    char *rawline;
    char *saveline;
    rawline = strtok_r(raw, "\n", &saveraw);
    if (rawline == NULL && expected != 0)
        return STOMP_MISSING_HEADER;
    // at most expected, unless end or empty
    while (i < expected && rawline != NULL && strnlen(rawline, 1) != 0) {
        char *line = strdup(rawline); // MALLOC

        // key
        char *key = strtok_r(line, ":", &saveline);
        if (key == NULL) return STOMP_INVALID_HEADER;
        trim(&key);
        if (strlen(key) == 0) return STOMP_INVALID_HEADER;

        // val
        char *val = strtok_r(NULL, ":", &saveline);
        if (val == NULL) return STOMP_INVALID_HEADER;
        trim(&val);
        if (strlen(val) == 0) return STOMP_INVALID_HEADER;

        // more?
        if (strtok_r(NULL, ":", &saveline) != NULL)
            return STOMP_INVALID_HEADER;


        headers[i]->key = key;
        headers[i]->val = val;

        printf("Header: %s=%s\n", key, val);

        rawline = strtok_r(NULL, "\n", &saveraw);
        i++;
    }

    if (i < (expected-1)) {
        return STOMP_MISSING_HEADER;   
    } else if (rawline != NULL) {
        return STOMP_INVALID_HEADER;
    } else {
        return 0;
    }
}

static int parse_content(const char *raw, size_t expected,
                    char **content) {
    return -1;
}

static int split_cmd(char *raw, char **header, char **content) {
    char *headerend, *contentend;

    // search for header/content separator
    headerend = strstr(raw, "\n\n"); 
    if (headerend == NULL) {
        // no header found, later parsing
        // will handle this
        *header = NULL;
        *content = NULL;
    } else {

        // mark end of string for header at newline
        *headerend = '\0';
        *header = raw;

        contentend = strstr(headerend+2, "\n\n");
        // content is optional
        if (contentend != NULL) {
            // end content
            *contentend = '\0';
            *content = headerend+2;
        } else {
            *content = NULL;    
        }
    }

    return 0;
}

static int parse_command_connect(char *rawheader,
                const char *rawcontent, struct stomp_command* cmd) {
    // parse header 'login'
    int nheaders = 1;
    struct stomp_header* headers =
            malloc(sizeof(struct stomp_header) * nheaders);
    int parsed = parse_header(rawheader, nheaders, &headers);
    if (parsed != 0) {
        return parsed;
    }

    if (rawcontent != NULL) {
        return STOMP_UNEXPECTED_CONTENT;
    }

    cmd->name = "CONNECT";
    cmd->headers = headers;
    cmd->content = NULL;
    return 0;
}

static int parse_command_send(char *rawheader,
                const char *rawcontent, struct stomp_command* cmd) {
    // parse header 'topic'
    int nheaders = 1;
    struct stomp_header* headers =
            malloc(sizeof(struct stomp_header) * nheaders);
    int parsed = parse_header(rawheader, nheaders, &headers);
    if (parsed != 0) {
        return parsed;
    }

    if (rawcontent == NULL) {
        return STOMP_MISSING_CONTENT;
    }
    
    printf("Content: %s\n", rawcontent);

    cmd->name = "SEND";
    cmd->headers = headers;
    cmd->content = strdup(rawcontent); // malloc
    return 0;
}

static int parse_command_subscribe(char *rawheader,
                const char *rawcontent, struct stomp_command* cmd) {
    // parse header 'destination'
    int nheaders = 1;
    struct stomp_header* headers =
            malloc(sizeof(struct stomp_header) * nheaders);
    int parsed = parse_header(rawheader, nheaders, &headers);
    if (parsed != 0) {
        return parsed;
    }

    if (rawcontent != NULL) {
        return STOMP_UNEXPECTED_CONTENT;
    }
    
    cmd->name = "SUBSCRIBE";
    cmd->headers = headers;
    cmd->content = NULL;
    return 0;
}

static int parse_command_disconnect(char *rawheader,
                const char *rawcontent, struct stomp_command* cmd) {
    // expect no header
    if (rawheader != NULL) return STOMP_UNEXPECTED_HEADER;
    if (rawcontent != NULL) return STOMP_UNEXPECTED_CONTENT;
    
    cmd->name = "DISCONNECT";
    cmd->headers = NULL;
    cmd->content = NULL;
    return 0;
}

int parse_command(char* raw, struct stomp_command* cmd) {
    char *header, *content;
    int split;

    if (strncmp("CONNECT\n", raw, 8) == 0) {
        split = split_cmd(raw+8, &header, &content);
        if (split != 0) return split;
        return parse_command_connect(header, content, cmd);
    } else if (strncmp("SEND\n", raw, 5) == 0) {
        split = split_cmd(raw+5, &header, &content);
        if (split != 0) return split;
        return parse_command_send(header, content, cmd);
    } else if (strncmp("SUBSCRIBE\n", raw, 10) == 0) {
        split = split_cmd(raw+10, &header, &content);
        if (split != 0) return split;
        return parse_command_subscribe(header, content, cmd);
    } else if (strncmp("DISCONNECT\n", raw, 10) == 0) {
        split = split_cmd(raw+10, &header, &content);
        if (split != 0) return split;
        return parse_command_disconnect(header, content, cmd);
    } else {
        return STOMP_UNKNOWN_COMMAND;
    }
}

int create_command(struct stomp_command cmd, char* str) {
    return 0;
}
