#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include "client.h"
#include "../utils/constants.h"
#include "../protocol/protocol.h"

int init_connection(const char *address)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0); // create socket (to send and receive data)
    struct sockaddr_in sin = {0};               // socket address structure (to define server address)
    struct hostent *hostinfo;

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    hostinfo = gethostbyname(address);
    if (hostinfo == NULL)
    {
        fprintf(stderr, "Unknown host %s.\n", address);
        exit(EXIT_FAILURE);
    }

    sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
    sin.sin_port = htons(SERVER_PORT);
    sin.sin_family = AF_INET;

    if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        perror("connect()");
        exit(errno);
    }

    return sock;
}

void end_connection(int sock)
{
    close(sock);
}

int read_from_server(int sock, char *buffer)
{
    size_t len = 0;

    /* Receive message length */
    if (recv(sock, (char *)&len, sizeof(len), 0) < 0)
    {
        perror("recv() - length");
        return -1;
    }

    if (len > BUF_SIZE - 1)
    {
        fprintf(stderr, "Message too large: %zu\n", len);
        return -1;
    }

    /* Receive actual message */
    int n = recv(sock, buffer, len, 0);
    if (n < 0)
    {
        perror("recv() - message");
        return -1;
    }

#ifdef DEBUG
    printf("%s%s[read]%s%s %s%s\n", STYLE_DIM, COLOR_BOLD, COLOR_RESET, STYLE_DIM, buffer, COLOR_RESET);
#endif

    buffer[n] = 0;
    return n;
}

void write_to_server(int sock, const char *buffer)
{
#ifdef DEBUG
    printf("%s%s[send]%s%s %s%s\n", STYLE_DIM, COLOR_BOLD, COLOR_RESET, STYLE_DIM, buffer, COLOR_RESET);
#endif
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

void process_command(int sock, const char *input)
{
    char command[BUF_SIZE];
    char *args = NULL;

    /* Parse command and arguments */
    strncpy(command, input, BUF_SIZE - 1);

    /* Extract command (first word) */
    char *space = strchr(command, ' ');
    if (space)
    {
        *space = 0;
        args = space + 1;
    }

    /* Handle commands */
    if (strcmp(command, CMD_MSG) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: msg <message>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        /* Send message to server */
        write_to_server(sock, args);
    }
    else if (strcmp(command, CMD_CHALLENGE) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: challenge <user>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_CHALLENGE, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_ACCEPT) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: accept <user>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_ACCEPT, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_REFUSE) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: refuse <user>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_REFUSE, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_CANCEL) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: cancel <user>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_CANCEL, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_MOVE) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: move <pit_index>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_MOVE, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_QUIT) == 0)
    {
        write_to_server(sock, CMD_QUIT);
    }
    else if (strcmp(command, CMD_LIST_USERS) == 0)
    {
        /* Send list request to server */
        write_to_server(sock, CMD_LIST_USERS);
    }
    else if (strcmp(command, CMD_SET_BIO) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: bio <text>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        /* Format and send bio command to server */
        char bio_cmd[BUF_SIZE];
        snprintf(bio_cmd, BUF_SIZE, "%s %s", CMD_SET_BIO, args);
        write_to_server(sock, bio_cmd);
    }
    else if (strcmp(command, CMD_GET_BIO) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: getbio <user>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        /* Format and send getbio command to server */
        char getbio_cmd[BUF_SIZE];
        snprintf(getbio_cmd, BUF_SIZE, "%s %s", CMD_GET_BIO, args);
        write_to_server(sock, getbio_cmd);
    }
    else if (strcmp(command, CMD_GAMES) == 0)
    {
        write_to_server(sock, CMD_GAMES);
    }
    else if (strcmp(command, CMD_WATCH) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: watch <matchId>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char watch_cmd[BUF_SIZE];
        snprintf(watch_cmd, BUF_SIZE, "%s %s", CMD_WATCH, args);
        write_to_server(sock, watch_cmd);
    }
    else if (strcmp(command, CMD_UNWATCH) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: unwatch <matchId>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char unwatch_cmd[BUF_SIZE];
        snprintf(unwatch_cmd, BUF_SIZE, "%s %s", CMD_UNWATCH, args);
        write_to_server(sock, unwatch_cmd);
    }
    else if (strcmp(command, CMD_PM) == 0)
    {
        if (args == NULL || strlen(args) == 0 || strchr(args, ' ') == NULL)
        {
            printf("%s[error]%s Usage: pm <user> <message>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char pm_cmd[BUF_SIZE];
        snprintf(pm_cmd, BUF_SIZE, "%s %s", CMD_PM, args);
        write_to_server(sock, pm_cmd);
    }
    else if (strcmp(command, CMD_ADD_FRIEND) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: addfriend <user>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_ADD_FRIEND, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_ACCEPT_FRIEND) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: acceptfriend <user>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_ACCEPT_FRIEND, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_REFUSE_FRIEND) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: refusefriend <user>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_REFUSE_FRIEND, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_PRIVATE) == 0)
    {
        if (args == NULL || (strcmp(args, "on") != 0 && strcmp(args, "off") != 0))
        {
            printf("%s[error]%s Usage: private on|off\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_PRIVATE, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, CMD_FRIENDS) == 0)
    {
        write_to_server(sock, CMD_FRIENDS);
    }
    else if (strcmp(command, CMD_RANKING) == 0)
    {
        write_to_server(sock, CMD_RANKING);
    }
    else if (strcmp(command, CMD_WATCH_REPLAY) == 0)
    {
        if (args == NULL || strlen(args) == 0)
        {
            printf("%s[error]%s Usage: watchreplay <matchId>\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            return;
        }
        char cmd[BUF_SIZE];
        snprintf(cmd, BUF_SIZE, "%s %s", CMD_WATCH_REPLAY, args);
        write_to_server(sock, cmd);
    }
    else if (strcmp(command, "help") == 0)
    {
        printf("%s[help]%s Available commands:\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET);
        printf("    msg <message>      - Send a message to all users\n");
        printf("    list               - Show all online users and their bios\n");
        printf("    challenge <user>   - Challenge a user to a game\n");
        printf("    accept <user>      - Accept a challenge from a user\n");
        printf("    refuse <user>      - Refuse a challenge from a user\n");
        printf("    cancel <user>      - Cancel your pending challenge\n");
        printf("    bio <text>         - Set your bio (max 256 characters)\n");
        printf("    pm <user> <msg>    - Send a private message\n");
        printf("    getbio <user>      - Get a user's bio\n");
        printf("    move <pit>         - Make a move (in game)\n");
        printf("    quit               - Quit current game\n");
        printf("    games              - List running games\n");
        printf("    watch <id>         - Spectate a running game\n");
        printf("    unwatch <id>       - Stop spectating a game\n");
        printf("    watchreplay <id>   - Watch a finished game's replay (5s per move)\n");
        printf("    addfriend <user>   - Send friend request\n");
        printf("    acceptfriend <u>   - Accept friend request\n");
        printf("    refusefriend <u>   - Refuse friend request\n");
        printf("    private on|off     - Toggle match privacy (only friends watch)\n");
        printf("    friends            - Show your friend list\n");
        printf("    ranking            - Show ranking by wins\n");
        printf("    help               - Show this help message\n");
    }
    else
    {
        printf("%s[error]%s No command recognized: '%s'. Type 'help' for available commands.\n", COLOR_RED COLOR_BOLD, COLOR_RESET, command);
    }
}
