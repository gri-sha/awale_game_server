#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "../protocol/protocol.h"

#define PITS_PER_PLAYER 6
#define TOTAL_PITS 12
#define INITIAL_SEEDS 4

/* GAME CONSTANTS */
// seeds
#define PITS_PER_PLAYER 6
#define TOTAL_PITS 12
#define INITIAL_SEEDS 4
#define TOTAL_SEEDS (INITIAL_SEEDS * TOTAL_PITS)
#define MIN_SEEDS_TO_WIN (TOTAL_SEEDS / 2 + 1)
#define MIN_CAPTURE_SEEDS 2
#define MAX_CAPTURE_SEEDS 3
// players
#define PLAYER_1 0
#define PLAYER_2 1
// game states
#define GAME_RUNNING 0
#define GAME_OVER 1
#define GAME_PAUSED 2

/* UI CONSTANTS (for terminal display) */
// terminal colors (ANSI codes)
#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RED "\033[31m"
#define COLOR_BLUE "\033[34m"
#define COLOR_WHITE "\033[37m"
// text styles
#define STYLE_DIM "\033[2m"
// cursor control
#define CURSOR_SAVE "\033[s"
#define CURSOR_RESTORE "\033[u"
#define CURSOR_MOVE_BOTTOM "\033[999;0H"
#define CLEAR_LINE "\033[K"
// symbols
#define CRLF "\r\n"

/* NETWORKING CONSTANTS */
// error code
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
// serever info
#define SERVER_PORT 5050
#define SERVER_ADDR "127.0.0.1"
// limits
#define MAX_USERNAME_LEN 32
#define MAX_BIO_LEN 256
#define MAX_CLIENTS 256
#define MAX_CHALLENGES 512
#define MAX_NOTIFS 512
#define MAX_MATCHES 128
// buffer size (max message size)
#define BUF_SIZE 1024

// useful types
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

typedef struct
{
   MessageType type;
   char payload[BUF_SIZE - sizeof(int)];
} ServerMessage;

#endif