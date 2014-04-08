/*
 * STOMP Protocol
 *
 * This rough* implementation of the stomp protocol supports a
 * number of commands, each of which is described in the following
 * section. All have uppercase have uppercase names followed by
 * a newline and a list of values. Each command is terminated
 * by a newline followed by the null-byte (^@).
 *
 * *rough: While this implementation is very similar to the
 *         original STOMP specification, it does not adhere
 *         to it entirely. Some commands and some headers
 *         are not supported.
 *
 * Terminology:
 * 1. Broker: The server that accepts connections from clients
 *            and dispatches messages
 * 2. Client: Publisher or Subscriber
 * 3. String: Null-terminated array of characters
 * 4. Publish: Sending a message to a topic
 * 5. Subscribe: Client wants to receive message from a certain
 *               topic
 *
 * Example:
 *    COMMAND
 *    key: value
 *    key: value
 *    ..
 *
 *    Content
 *
 *    ^@
 *
 * Commands:
 * 1. CONNECT
 *    a. Sent by client to initiate connection
 *    b. Headers
 *       i. login: a string identifying the client
 *    c. No Content
 *    d. Response from broker
 *       i.  CONNECTED on success
 *       ii. ERROR on failure
 * 2. CONNECTED
 *    a. Sent by broker to client upon successful connection
 *    b. No Headers
 *    c. No Content
 * 3. ERROR
 *    a. Sent by broker to client on any failure
 *    b. Headers
 *       i. message: a string describing what went wrong
 *    c. No Content
 * 4. SEND
 *    a. Sent by a connected publisher to publish to a topic
 *    b. Headers
 *       i. topic: a string identifying the topic
 *    c. Content: The message to be sent to the topic
 *    d. Response from broker
 *       i. ERROR on failure
 * 5. SUBSCRIBE
 *    a. Sent by a connected subscriber to subscribe to a topic
 *    b. Headers
 *       i. destination: a string identifying the topic to
 *                       subscribe to
 *    c. No Content
 *    d. Response from broker
 *       i. ERROR if subscription cannot be created
 * 6. MESSAGE
 *    a. Message sent to a subscriber of a topic
 *    b. Headers
 *       i.  message-id: an integer identifying the message
 *       ii. destination: a string identifying the topic
 *           this message was sent to
 *    c. Content: The contents of the message
 * 7. DISCONNECT
 *    a. Sent by a connected client to end a connection
 *    b. No Headers
 *    c. No Content
 *    d. Response from broker
 *       i. RECEIPT to confirm
 * 8. RECEIPT
 *    a. Sent by the broker to a client as confirmation that
 *       the connection has been successfully ended.
 *    b. No Headers
 *    c. No Content
 *
 */

#define STOMP_MISSING_HEADER     -2
#define STOMP_INVALID_HEADER     -3
#define STOMP_UNEXPECTED_HEADER  -4
#define STOMP_UNEXPECTED_CONTENT -5
#define STOMP_MISSING_CONTENT    -6
#define STOMP_INVALID_CONTENT    -7
#define STOMP_UNKNOWN_COMMAND    -8

#define STOMP_MAX_CONTENT 2048

struct stomp_header {
    char* key;
    char* val;
};

struct stomp_command {
    char*                name;
    struct stomp_header* headers;
    int                  nheaders;
    char*                content;
};

int parse_command(char* raw, struct stomp_command* cmd);

int create_command(struct stomp_command cmd, char** str);
