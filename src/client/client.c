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

void app(const char *address, const char *name)
{
    int sock = init_connection(address);
    char buffer[BUF_SIZE];
    char payload[BUF_SIZE];
    MessageType msg_type;

    fd_set rdfs;

    /* send our name */
    write_to_server(sock, name);

    /* Wait for connection acknowledgment */
    int n = read_from_server(sock, buffer);
    if (n > 0 && protocol_parse_message(buffer, &msg_type, payload))
    {
        if (msg_type == MSG_CONNECT_ACK)
        {
            printf("%s[connected]%s Connected to server as '%s'\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, name);
            printf("%s[help]%s Type 'help' for available commands\n\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET);
        }
        else if (msg_type == MSG_ERROR)
        {
            printf("%s[error]%s Connection rejected: %s\n", COLOR_RED COLOR_BOLD, COLOR_RESET, payload);
            end_connection(sock);
            return;
        }
        else
        {
            printf("%s[error]%s Unexpected response from server\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            end_connection(sock);
            return;
        }
    }
    else
    {
        printf("%s[error]%s Failed to connect to server\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
        end_connection(sock);
        return;
    }

    while (1)
    {
        FD_ZERO(&rdfs);

        /* add STDIN_FILENO */
        FD_SET(STDIN_FILENO, &rdfs);

        /* add the socket */
        FD_SET(sock, &rdfs);

        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        // try to send a message (triggered by keyboard input)
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            fflush(stdout);
            fgets(buffer, BUF_SIZE - 1, stdin);
            {
                char *p = NULL;
                p = strstr(buffer, "\n");
                if (p != NULL)
                {
                    *p = 0;
                }
                else
                {
                    /* fclean */
                    buffer[BUF_SIZE - 1] = 0;
                }
            }
            process_command(sock, buffer);
            // received a message from the server
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            int n = read_from_server(sock, buffer);
            /* server down */
            if (n == 0)
            {
                printf("Server disconnected !\n");
                break;
            }

            /* Parse server message */
            char payload[BUF_SIZE];
            MessageType msg_type;
            if (protocol_parse_message(buffer, &msg_type, payload))
            {
                switch (msg_type)
                {
                case MSG_INFO:
                    printf("%s[ack]%s %s\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_CHAT:
                    printf("%s[chat]%s %s\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_LIST_USERS:
                    printf("%s[users]%s\n%s\n\n", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, payload);
                    fflush(stdout); // Force flush to terminal
                    break;
                case MSG_BIO_SET:
                    printf("%s[bio]%s %s\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_BIO_INFO:
                    printf("%s[bio]%s %s\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_PRIVATE_CHAT:
                    printf("%s[pm]%s %s\n", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_CHALLENGE:
                    printf("%s[challenge]%s %s\n", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_CHALLENGE_RESPONSE:
                    printf("%s[challenge]%s %s\n", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_BOARD_UPDATE:
                    printf("%s[board]%s\n%s\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_MOVE:
                    printf("%s[move]%s %s\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_MATCH_LIST:
                    printf("%s[games]%s\n%s", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_GAME_OVER:
                    printf("%s[game]%s %s\n", COLOR_RED COLOR_BOLD, COLOR_RESET, payload);
                    break;
                case MSG_ERROR:
                    printf("%s[error]%s %s\n", COLOR_RED COLOR_BOLD, COLOR_RESET, payload);
                    break;
                default:
                    printf("%s[message]%s %s\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, payload);
                }
            }
            else
            {
                /* Fallback for unparseable messages */
                printf("%s\n", buffer);
            }
        }
    }

    end_connection(sock);
}

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
        printf("    help               - Show this help message\n");
    }
    else
    {
        printf("%s[error]%s No command recognized: '%s'. Type 'help' for available commands.\n", COLOR_RED COLOR_BOLD, COLOR_RESET, command);
    }
}
