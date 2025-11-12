#include "server/server.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
   int sock = init_connection(); // listening socket
   char buffer[BUF_SIZE];

   int client_count = 0; // the index for the array of client file descriptors (sockets)

   // Every Unix/Linux process automatically gets three open file descriptors:
   // 0 - STDIN_FILENO - Standard input (keyboard)
   // 1 - STDOUT_FILENO - Standard output (console/terminal)
   // 2 - STDERR_FILENO - Standard error output (console/terminal)
   // so the first socket created will be 3, the next 4, etc.
   int max = sock; // maximum file descriptor number (if open new file (sockets) it raises), used in select()

   Client clients[MAX_CLIENTS]; // array for all clients

   // Initialize matches array
   Match *matches = (Match *)malloc(MAX_MATCHES * sizeof(Match));
   if (matches == NULL)
   {
      fprintf(stderr, "%s[error]%s Failed to allocate memory for matches\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
      exit(EXIT_FAILURE);
   }
   memset(matches, 0, MAX_MATCHES * sizeof(Match));
   int match_count = 0;

   // read file descriptors set (to store file descriptors to monitor)
   fd_set rdfs;
   // FD_ZERO(&rdfs) - Clears all bits - empties the set
   // FD_SET(fd, &rdfs) - Adds a file descriptor to monitor
   // FD_ISSET(fd, &rdfs) - Checks if a file descriptor has activity

   // log server startup information
   char *server_ip = get_server_ip();
   if (server_ip)
   {
      printf("%s[server]%s Started on %s:%d\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, server_ip, SERVER_PORT);
   }
   else
   {
      printf("%s[error]%s Failed to determine server IP address\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
   }

   while (1)
   {
      int i = 0;
      FD_ZERO(&rdfs);              // clear all the bits of the set (empty it)
      FD_SET(STDIN_FILENO, &rdfs); // add keyboard
      FD_SET(sock, &rdfs);         // add listening socket

      /* add socket of each client */
      for (i = 0; i < client_count; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      // if there is activity on keyboard stop the sevrer
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         break;
      }
      // if there is activity on the listening socket - new client
      else if (FD_ISSET(sock, &rdfs))
      {
#ifdef DEBUG
         printf("%sActivity on listening socket: new client connecting...%s\n", STYLE_DIM, COLOR_RESET);
#endif
         SOCKADDR_IN csin = {0};
         unsigned int sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize); // client socket
         if (csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         // try to read the name of the client (it is sended at connection)
         if (read_from_client(csock, buffer) == 0)
         {
            printf("%s[error]%s Client disconnected before sending name.\n", COLOR_RED COLOR_BOLD, COLOR_RESET);
            close(csock);
            continue;
         }

         /* Check if username is unique */
         if (!is_username_unique(clients, client_count, buffer))
         {
            printf("%s[error]%s Username '%s' already exists. Connection rejected.\n", COLOR_RED COLOR_BOLD, COLOR_RESET, buffer);
            char error_msg[BUF_SIZE];
            protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, "Username already taken. Connection rejected.");
            write_client(csock, error_msg);
            close(csock);
            continue;
         }

         max = csock > max ? csock : max; // update maximum fd number
#ifdef DEBUG
         printf("%sMax fd updated to %d%s\n", STYLE_DIM, max, COLOR_RESET);
#endif

         FD_SET(csock, &rdfs);

         Client c;
         c.sock = csock;
         strncpy(c.name, buffer, MAX_USERNAME_LEN - 1);
         c.status = CLIENT_IDLE;
         c.current_match = -1;
         memset(c.bio, 0, MAX_BIO_LEN);
         memset(c.pending_challenge_to, 0, MAX_CHALLENGES * MAX_USERNAME_LEN);
         c.pending_challenge_to_count = 0;
         memset(c.pending_challenge_from, 0, MAX_CHALLENGES * MAX_USERNAME_LEN);
         c.pending_challenge_from_count = 0;
         c.is_turn = 0;
         c.friend_count = 0;
         c.pending_friend_to[0] = '\0';
         c.pending_friend_from[0] = '\0';
         c.wins = 0;
         clients[client_count] = c;
         client_count++;

         printf("%s[connection]%s %s joined the server\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, c.name);

         /* Send connection acknowledgment to client */
         char ack_msg[BUF_SIZE];
         protocol_create_message(ack_msg, BUF_SIZE, MSG_CONNECT_ACK, c.name);
         write_client(csock, ack_msg);
      }
      // if there is activity not on listening socket nor on keyboard - maybe client is talking or an error
      // we need to check all clients whether they are talking
      else
      {
         int i = 0;
         for (i = 0; i < client_count; i++)
         {
            /* a client is talking */
            if (FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_from_client(clients[i].sock, buffer);
               /* client disconnected */
               if (c == 0)
               {
                  close(clients[i].sock);
                  
                  /* Handle match cleanup if client was in a match */
                  if (clients[i].status == CLIENT_IN_MATCH && clients[i].current_match >= 0)
                  {
                     Match *m = get_match_by_id(clients[i].current_match, matches, match_count);
                     if (m)
                     {
                        /* Determine opponent */
                        int opponent_idx = (i == m->player1_index) ? m->player2_index : m->player1_index;
                        
                        /* Notify opponent about disconnection */
                        notify(clients[opponent_idx].sock, MSG_GAME_OVER, "%s disconnected from the match", clients[i].name);
                        
                        /* Award win to opponent */
                        clients[opponent_idx].wins++;
                        
                        /* End the match */
                        end_match(m, clients);
                     }
                  }
                  
                  /* Clean up pending challenges sent by this client */
                  for (int j = 0; j < clients[i].pending_challenge_to_count; j++)
                  {
                     int target_idx = find_client_index_by_name(clients, client_count, clients[i].pending_challenge_to[j]);
                     if (target_idx != -1)
                     {
                        /* Remove from target's pending_challenge_from */
                        for (int k = 0; k < clients[target_idx].pending_challenge_from_count; k++)
                        {
                           if (strcmp(clients[target_idx].pending_challenge_from[k], clients[i].name) == 0)
                           {
                              for (int m = k; m < clients[target_idx].pending_challenge_from_count - 1; m++)
                              {
                                 strncpy(clients[target_idx].pending_challenge_from[m], 
                                        clients[target_idx].pending_challenge_from[m + 1], MAX_USERNAME_LEN - 1);
                              }
                              clients[target_idx].pending_challenge_from_count--;
                              break;
                           }
                        }
                     }
                  }
                  
                  /* Clean up pending challenges received by this client */
                  for (int j = 0; j < clients[i].pending_challenge_from_count; j++)
                  {
                     int challenger_idx = find_client_index_by_name(clients, client_count, clients[i].pending_challenge_from[j]);
                     if (challenger_idx != -1)
                     {
                        /* Remove from challenger's pending_challenge_to */
                        for (int k = 0; k < clients[challenger_idx].pending_challenge_to_count; k++)
                        {
                           if (strcmp(clients[challenger_idx].pending_challenge_to[k], clients[i].name) == 0)
                           {
                              for (int m = k; m < clients[challenger_idx].pending_challenge_to_count - 1; m++)
                              {
                                 strncpy(clients[challenger_idx].pending_challenge_to[m], 
                                        clients[challenger_idx].pending_challenge_to[m + 1], MAX_USERNAME_LEN - 1);
                              }
                              clients[challenger_idx].pending_challenge_to_count--;
                              break;
                           }
                        }
                        
                        /* Update challenger's status if needed */
                        if (clients[challenger_idx].pending_challenge_to_count == 0 && 
                            clients[challenger_idx].status == CLIENT_WAITING_FOR_ACCEPT)
                        {
                           clients[challenger_idx].status = CLIENT_IDLE;
                        }
                     }
                  }
                  
                  remove_client(clients, i, &client_count);
                  printf("%s[disconnection]%s %s left the server\n", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, client.name);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, client_count, buffer, 1);
               }
               else
               {
                  printf("%s[message]%s %s: %s\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, client.name, buffer);

                  /* Parse command and arguments */
                  char command[BUF_SIZE];
                  char args[BUF_SIZE];
                  protocol_parse_command(buffer, command, args, BUF_SIZE, BUF_SIZE);

                  /* Handle different commands */
                  if (strcmp(command, CMD_LIST_USERS) == 0)
                  {
                     handle_list_command(clients[i].sock, clients, client_count);
                  }
                  else if (strcmp(command, CMD_MSG) == 0)
                  {
                     handle_message_command(clients[i].sock, clients, client, client_count, args);
                  }
                  else if (strcmp(command, CMD_SET_BIO) == 0)
                  {
                     handle_bio_command(clients[i].sock, clients, i, args);
                  }
                  else if (strcmp(command, CMD_GET_BIO) == 0)
                  {
                     handle_getbio_command(clients[i].sock, clients, client_count, args);
                  }
                  else if (strcmp(command, CMD_PM) == 0)
                  {
                     handle_pm_command(clients[i].sock, clients, client, client_count, args);
                  }
                  else if (strcmp(command, CMD_GAMES) == 0)
                  {
                     handle_games_command(clients[i].sock, clients, matches, match_count);
                  }
                  else if (strcmp(command, CMD_WATCH) == 0)
                  {
                     handle_watch_command(clients[i].sock, clients, i, client_count, args, matches, match_count);
                  }
                  else if (strcmp(command, CMD_UNWATCH) == 0)
                  {
                     handle_unwatch_command(clients[i].sock, clients, i, client_count, args, matches, match_count);
                  }
                  else if (strcmp(command, CMD_ADD_FRIEND) == 0)
                  {
                     handle_addfriend_command(clients[i].sock, clients, i, client_count, args);
                  }
                  else if (strcmp(command, CMD_ACCEPT_FRIEND) == 0)
                  {
                     handle_acceptfriend_command(clients[i].sock, clients, i, client_count, args);
                  }
                  else if (strcmp(command, CMD_REFUSE_FRIEND) == 0)
                  {
                     handle_refusefriend_command(clients[i].sock, clients, i, client_count, args);
                  }
                  else if (strcmp(command, CMD_PRIVATE) == 0)
                  {
                     handle_private_command(clients[i].sock, clients, i, client_count, args, matches, match_count);
                  }
                  else if (strcmp(command, CMD_FRIENDS) == 0)
                  {
                     handle_friends_command(clients[i].sock, clients, i, client_count);
                  }
                  else if (strcmp(command, CMD_RANKING) == 0)
                  {
                     handle_ranking_command(clients[i].sock, clients, client_count);
                  }
                  else if (strcmp(command, CMD_WATCH_REPLAY) == 0)
                  {
                     handle_watchreplay_command(clients[i].sock, clients, i, client_count, args, matches, match_count);
                  }
                  else if (strcmp(command, CMD_CHALLENGE) == 0)
                  {
                     handle_challenge_command(clients[i].sock, clients, i, client_count, args);
                  }
                  else if (strcmp(command, CMD_ACCEPT) == 0)
                  {
                     handle_accept_command(clients[i].sock, clients, i, client_count, args, matches, &match_count);
                  }
                  else if (strcmp(command, CMD_REFUSE) == 0)
                  {
                     handle_refuse_command(clients[i].sock, clients, i, client_count, args);
                  }
                  else if (strcmp(command, CMD_CANCEL) == 0)
                  {
                     handle_cancel_command(clients[i].sock, clients, i, client_count, args);
                  }
                  else if (strcmp(command, CMD_MOVE) == 0)
                  {
                     handle_move_command(clients[i].sock, clients, i, client_count, args, matches, match_count);
                  }
                  else if (strcmp(command, CMD_QUIT) == 0)
                  {
                     handle_quit_command(clients[i].sock, clients, i, client_count, matches, match_count);
                  }
                  else
                  {
                     /* Unknown command or regular message */
                     handle_message_command(clients[i].sock, clients, client, client_count, buffer);
                  }
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, client_count);
   free(matches);
   matches = NULL;
   end_connection(sock);

   return EXIT_SUCCESS;
}