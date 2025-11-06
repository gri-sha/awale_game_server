#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>

/* 
 * MESSAGE PROTOCOL
 * ================
 * Format: "TYPE|payload" of "<command> <args...>"
 * 
 * Examples:
 *  Server sends:  "0|Welcome alice!"                    (MSG_CONNECT_ACK)
 *  Server sends:  "2|alice,bob,charlie"                (MSG_LIST_USERS)
 *  Server sends:  "1|alice: Hello everyone"            (MSG_CHAT)
 *  Server sends:  "8|Error: invalid move"              (MSG_ERROR)
 * 
 * Client sends (keywords):
 *  "list"                     → requests user list
 *  "msg hello everyone"       → chat message
 *  "challenge alice"          → challenge a player
 *  "accept alice"             → accept a challenge
 *  "bio my bio text"          → set bio
 */

/* MESSAGE TYPES - Server to Client responses */
typedef enum {
    MSG_CONNECT_ACK = 0,
    MSG_CHAT = 1,
    MSG_LIST_USERS = 2,
    MSG_CHALLENGE = 3,
    MSG_CHALLENGE_RESPONSE = 4,
    MSG_MOVE = 5,
    MSG_BOARD_UPDATE = 6,
    MSG_GAME_OVER = 7,
    MSG_ERROR = 8,
    MSG_INFO = 9,
    MSG_BIO_SET = 10
} MessageType;

/* CLIENT COMMANDS - Client to Server requests */
#define CMD_MSG "msg"
#define CMD_LIST_USERS "list"
#define CMD_CHALLENGE "challenge"
#define CMD_ACCEPT "accept"
#define CMD_REFUSE "refuse"
#define CMD_MOVE "move"
#define CMD_WATCH "watch"
#define CMD_SET_BIO "bio"

/* Create a formatted message from type and payload */
void protocol_create_message(char *buffer, size_t buf_size, MessageType type, const char *payload);

/* Parse an incoming message - extracts type and payload */
int protocol_parse_message(const char *buffer, MessageType *type, char *payload);

/* Check if a client input is a recognized command */
int protocol_is_command(const char *input);

/* Extract command keyword and arguments */
void protocol_parse_command(const char *input, char *command, char *args, size_t cmd_size, size_t args_size);

#endif
