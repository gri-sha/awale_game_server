#include "client/client.h"
#include "utils/constants.h"
#include "protocol/protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

static void display_help_menu(char *exec_name)
{
   printf("Usage: %s --name <pseudo> [--ip <address>] [--port <port>]\n", exec_name);
   printf("Options:\n");
   printf("  --name <pseudo>        Username for the client (required)\n");
   printf("  --ip <address>         Server IP address (default: %s)\n", SERVER_ADDR);
   printf("  --port <port>          Server port number (default: %d)\n", SERVER_PORT);
   printf("  --help                 Show this help message\n");
}

int main(int argc, char **argv)
{
   const char *address = SERVER_ADDR;
   int port = SERVER_PORT;
   const char *name = NULL;

   // Parse command-line arguments
   // --name <pseudo> [--ip <address>] [--port <port>] [--help]
   // if ip not provided, use default SERVER_ADDR
   // if port not provided, use default SERVER_PORT
   for (int i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--name") == 0)
      {
         if (i + 1 < argc)
         {
            name = argv[i + 1];
            i++;
         }
         else
         {
            fprintf(stderr, "%s[error]%s --name requires an argument\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            display_help_menu(argv[0]);
            return EXIT_FAILURE;
         }
      }
      else if (strcmp(argv[i], "--ip") == 0)
      {
         if (i + 1 < argc)
         {
            address = argv[i + 1];
            i++;
         }
         else
         {
            fprintf(stderr, "%s[error]%s --ip requires an argument\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            display_help_menu(argv[0]);
            return EXIT_FAILURE;
         }
      }
      else if (strcmp(argv[i], "--port") == 0)
      {
         if (i + 1 < argc)
         {
            port = atoi(argv[i + 1]);
            if (port <= 0 || port > 65535)
            {
               fprintf(stderr, "%s[error]%s Invalid port number: %s. Port must be between 1 and 65535.\n", COLOR_RED COLOR_BOLD, COLOR_RESET, argv[i + 1]);
               display_help_menu(argv[0]);
               return EXIT_FAILURE;
            }
            i++;
         }
         else
         {
            fprintf(stderr, "%s[error]%s --port requires a port number argument\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            display_help_menu(argv[0]);
            ;
            return EXIT_FAILURE;
         }
      }
      else if (strcmp(argv[i], "--help") == 0)
      {
         display_help_menu(argv[0]);
         return EXIT_SUCCESS;
      }
      else
      {
         fprintf(stderr, "%s[error]%s Unknown argument: %s\n", COLOR_RED COLOR_BOLD, COLOR_RESET, argv[i]);
         display_help_menu(argv[0]);
         return EXIT_FAILURE;
      }
   }

   if (name == NULL)
   {
      fprintf(stderr, "%s[error]%s Name is unknown.\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
      display_help_menu(argv[0]);
      return EXIT_FAILURE;
   }

   int sock = init_connection(address, port);
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
         return EXIT_FAILURE;
      }
      else
      {
         printf("%s[error]%s Unexpected response from server\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
         end_connection(sock);
         return EXIT_FAILURE;
      }
   }
   else
   {
      printf("%s[error]%s Failed to connect to server\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
      end_connection(sock);
      return EXIT_FAILURE;
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
               printf("%s[board]%s%s\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, payload);
               break;
            case MSG_MOVE:
               printf("%s[move]%s %s\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, payload);
               break;
            case MSG_MATCH_LIST:
               printf("%s[games]%s\n%s", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, payload);
               break;
            case MSG_FRIEND_REQUEST:
               printf("%s[friend]%s %s\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, payload);
               break;
            case MSG_FRIEND_RESPONSE:
               printf("%s[friend]%s %s\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, payload);
               break;
            case MSG_FRIEND_LIST:
               printf("%s[friends]%s %s\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, payload);
               break;
            case MSG_RANK_LIST:
               printf("%s[ranking]%s\n%s", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, payload);
               break;
            case MSG_REPLAY_DATA:
               printf("%s[replay]%s\n%s\n", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, payload);
               fflush(stdout);
               sleep(5); // pace replay steps by 5 seconds between frames
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

   return EXIT_SUCCESS;
}