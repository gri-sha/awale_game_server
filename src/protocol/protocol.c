#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "protocol.h"

void protocol_create_message(char *buffer, size_t buf_size, MessageType type, const char *payload)
{
    snprintf(buffer, buf_size, "%d|%s", type, payload);
}

int protocol_parse_message(const char *buffer, MessageType *type, char *payload)
{
    char type_str[16];
    
    /* Search for pipe symbol in first 4 characters */
    int pipe_pos = -1;
    for (int i = 0; i < 4 && buffer[i] != '\0'; i++)
    {
        if (buffer[i] == '|')
        {
            pipe_pos = i;
            break;
        }
    }
    
    /* Pipe not found in first 4 characters */
    if (pipe_pos == -1)
    {
        return 0;
    }
    
    /* Copy type part (before pipe) */
    strncpy(type_str, buffer, pipe_pos);
    type_str[pipe_pos] = '\0';
    
    /* Copy payload part (after pipe) */
    strcpy(payload, buffer + pipe_pos + 1);
    
    *type = (MessageType)atoi(type_str);
    return 1;
}

int protocol_is_command(const char *input)
{
    if (strncmp(input, CMD_MSG, strlen(CMD_MSG)) == 0 ||
        strcmp(input, CMD_LIST_USERS) == 0 ||
        strncmp(input, CMD_CHALLENGE, strlen(CMD_CHALLENGE)) == 0 ||
        strncmp(input, CMD_ACCEPT, strlen(CMD_ACCEPT)) == 0 ||
        strncmp(input, CMD_REFUSE, strlen(CMD_REFUSE)) == 0 ||
        strncmp(input, CMD_MOVE, strlen(CMD_MOVE)) == 0 ||
        strncmp(input, CMD_SET_BIO, strlen(CMD_SET_BIO)) == 0) {
        return 1;
    }
    return 0;
}

void protocol_parse_command(const char *input, char *command, char *args, size_t cmd_size, size_t args_size)
{
    strncpy(command, input, cmd_size - 1);
    command[cmd_size - 1] = 0;
    
    char *space = strchr(command, ' ');
    if (space) {
        *space = 0;
        strncpy(args, space + 1, args_size - 1);
        args[args_size - 1] = 0;
    } else {
        args[0] = 0;
    }
}
