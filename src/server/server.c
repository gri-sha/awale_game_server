#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "server.h"
#include "../utils/constants.h"
#include "../protocol/protocol.h"
#include "../core/awale.h"
#include <time.h>
#include <stdarg.h>

typedef struct {
   int id;
   int player1_index; // index in clients array
   int player2_index; // index in clients array
   Board board;
} Match;

static Match matches[MAX_MATCHES];
static int match_count = 0;

static int find_client_index_by_name(Client *clients, int actual, const char *name) {
   for (int i = 0; i < actual; i++) {
      if (strcmp(clients[i].name, name) == 0) return i;
   }
   return -1;
}

static void notify(int sock, MessageType type, const char *fmt, ...) {
   char payload[BUF_SIZE];
   va_list ap; va_start(ap, fmt);
   vsnprintf(payload, sizeof(payload), fmt, ap);
   va_end(ap);
   char msg[BUF_SIZE];
   protocol_create_message(msg, sizeof(msg), type, payload);
   write_client(sock, msg);
}

static void broadcast_board(Match *m, Client *clients) {
   char board_txt[BUF_SIZE];
   render_board(&m->board, board_txt, sizeof(board_txt));
   char msg[BUF_SIZE];
   protocol_create_message(msg, sizeof(msg), MSG_BOARD_UPDATE, board_txt);
   write_client(clients[m->player1_index].sock, msg);
   write_client(clients[m->player2_index].sock, msg);
}

static void end_match(Match *m, Client *clients) {
   if (!m) return;
   clients[m->player1_index].status = CLIENT_IDLE;
   clients[m->player2_index].status = CLIENT_IDLE;
   clients[m->player1_index].current_match = -1;
   clients[m->player2_index].current_match = -1;
   clients[m->player1_index].is_turn = 0;
   clients[m->player2_index].is_turn = 0;
}

static Match* start_match(Client *clients, int a, int b) {
   if (match_count >= MAX_MATCHES) return NULL;
   Match *m = &matches[match_count];
   m->id = match_count;
   m->player1_index = a;
   m->player2_index = b;
   init_board(&m->board);
   clients[a].status = CLIENT_IN_MATCH;
   clients[b].status = CLIENT_IN_MATCH;
   clients[a].current_match = m->id;
   clients[b].current_match = m->id;
   // Randomly choose who starts: 0 -> a, 1 -> b
   srand((unsigned int)time(NULL) ^ (unsigned int)(a<<8) ^ (unsigned int)(b<<16));
   int starter = rand() % 2;
   if (starter == 0) {
      m->board.current_player = 0; // we'll map: current_player 0 -> a, 1 -> b
      clients[a].is_turn = 1; clients[b].is_turn = 0;
   } else {
      m->board.current_player = 1;
      clients[a].is_turn = 0; clients[b].is_turn = 1;
   }
   // Notify players
   notify(clients[a].sock, MSG_CHALLENGE_RESPONSE, "Game started vs %s. %s starts.", clients[b].name, clients[a].is_turn?"You":"Opponent");
   notify(clients[b].sock, MSG_CHALLENGE_RESPONSE, "Game started vs %s. %s starts.", clients[a].name, clients[b].is_turn?"You":"Opponent");
   broadcast_board(m, clients);
   match_count++;
   return m;
}

void app(void)
{
   int sock = init_connection(); // listening socket
   char buffer[BUF_SIZE];

   int actual = 0; // the index for the array of client file descriptors (sockets)

   // Every Unix/Linux process automatically gets three open file descriptors:
   // 0 - STDIN_FILENO - Standard input (keyboard)
   // 1 - STDOUT_FILENO - Standard output (console/terminal)
   // 2 - STDERR_FILENO - Standard error output (console/terminal)
   // so the first socket created will be 3, the next 4, etc.
   int max = sock; // maximum file descriptor number (if open new file (sockets) it raises), used in select()

   Client clients[MAX_CLIENTS]; // array for all clients

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
      for (i = 0; i < actual; i++)
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
         if (!is_username_unique(clients, actual, buffer))
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
         clients[actual] = c;
         actual++;

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
         for (i = 0; i < actual; i++)
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
                  remove_client(clients, i, &actual);
                  printf("%s[disconnection]%s %s left the server\n", COLOR_YELLOW COLOR_BOLD, COLOR_RESET, client.name);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  printf("%s[message]%s %s: %s\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, client.name, buffer);
                  
                  /* Parse command and arguments */
                  char command[BUF_SIZE];
                  char args[BUF_SIZE];
                  protocol_parse_command(buffer, command, args, BUF_SIZE, BUF_SIZE);
                  
                  /* Handle different commands */
                  if (strcmp(command, CMD_LIST_USERS) == 0) {
                     handle_list_command(clients[i].sock, clients, actual);
                  }
                  else if (strcmp(command, CMD_MSG) == 0) {
                     handle_message_command(clients[i].sock, clients, client, actual, args);
                  }
                  else if (strcmp(command, CMD_SET_BIO) == 0) {
                     handle_bio_command(clients[i].sock, clients, i, args);
                  }
                  else if (strcmp(command, CMD_GET_BIO) == 0) {
                     handle_getbio_command(clients[i].sock, clients, actual, args);
                  }
                  else if (strcmp(command, CMD_PM) == 0) {
                     handle_pm_command(clients[i].sock, clients, client, actual, args);
                  }
                  else if (strcmp(command, CMD_GAMES) == 0) {
                     handle_games_command(clients[i].sock, clients, actual);
                  }
                  else if (strcmp(command, CMD_CHALLENGE) == 0) {
                     handle_challenge_command(clients[i].sock, clients, i, actual, args);
                  }
                  else if (strcmp(command, CMD_ACCEPT) == 0) {
                     handle_accept_command(clients[i].sock, clients, i, actual, args);
                  }
                  else if (strcmp(command, CMD_REFUSE) == 0) {
                     handle_refuse_command(clients[i].sock, clients, i, actual, args);
                  }
                  else if (strcmp(command, CMD_CANCEL) == 0) {
                     handle_cancel_command(clients[i].sock, clients, i, actual, args);
                  }
                  else if (strcmp(command, CMD_MOVE) == 0) {
                     handle_move_command(clients[i].sock, clients, i, actual, args);
                  }
                  else if (strcmp(command, CMD_QUIT) == 0) {
                     handle_quit_command(clients[i].sock, clients, i, actual);
                  }
                  else {
                     /* Unknown command or regular message */
                     handle_message_command(clients[i].sock, clients, client, actual, buffer);
                  }
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for (i = 0; i < actual; i++)
   {
      close(clients[i].sock);
   }
}

void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for (i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if (sender.sock != clients[i].sock)
      {
         if (from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

int init_connection(void)
{
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(SERVER_PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

void end_connection(int sock)
{
   close(sock);
}

int read_from_client(int sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

void write_client(int sock, const char *buffer)
{
   size_t len = strlen(buffer);
   
   /* Send message length first (4 bytes) */
   if (send(sock, (char*)&len, sizeof(len), 0) < 0)
   {
      perror("send() - length");
      exit(errno);
   }
   
   /* Send actual message */
   if (send(sock, buffer, len, 0) < 0)
   {
      perror("send() - message");
      exit(errno);
   }
}

char *get_server_ip(void)
{
   static char ip_str[INET_ADDRSTRLEN];
   struct sockaddr_in sin = {0};
   socklen_t len = sizeof(sin);
   
   /* Create a temporary socket to determine the local IP */
   int sock = socket(AF_INET, SOCK_DGRAM, 0);
   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      return NULL;
   }
   
   /* Connect to a public DNS server (doesn't actually send data) */
   sin.sin_family = AF_INET;
   sin.sin_port = htons(53);
   inet_pton(AF_INET, "8.8.8.8", &sin.sin_addr);
   
   if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
   {
      perror("connect()");
      close(sock);
      return NULL;
   }
   
   /* Get the local address bound to this socket */
   if (getsockname(sock, (struct sockaddr *)&sin, &len) < 0)
   {
      perror("getsockname()");
      close(sock);
      return NULL;
   }
   
   close(sock);
   
   /* Convert IP address to string */
   inet_ntop(AF_INET, &sin.sin_addr, ip_str, INET_ADDRSTRLEN);
   
   return ip_str;
}

void handle_list_command(int sock, Client *clients, int actual)
{
   /* Build list of all online users with names and bios on separate lines */
   char user_list[BUF_SIZE];
   user_list[0] = 0;
   
   for (int i = 0; i < actual; i++)
   {
      /* Add user name */
      strncat(user_list, clients[i].name, BUF_SIZE - strlen(user_list) - 1);
      
      /* Add bio or "no bio" (with dimmed style) */
      strncat(user_list, "\n", BUF_SIZE - strlen(user_list) - 1);
      strncat(user_list, STYLE_DIM, BUF_SIZE - strlen(user_list) - 1);
      
      if (strlen(clients[i].bio) > 0)
      {
         strncat(user_list, clients[i].bio, BUF_SIZE - strlen(user_list) - 1);
      }
      else
      {
         strncat(user_list, "no bio", BUF_SIZE - strlen(user_list) - 1);
      }
      
      strncat(user_list, COLOR_RESET, BUF_SIZE - strlen(user_list) - 1);
      
      /* Add separator except for last user */
      if (i < actual - 1)
      {
         strncat(user_list, "\n", BUF_SIZE - strlen(user_list) - 1);
      }
   }
   /* Send user list to sender */
   char response[BUF_SIZE];
   protocol_create_message(response, BUF_SIZE, MSG_LIST_USERS, user_list);
   write_client(sock, response);
   
   printf("%s[list]%s Sent user list to client\n", COLOR_YELLOW COLOR_BOLD, COLOR_RESET);
}

void handle_message_command(int sock, Client *clients, Client sender, int actual, const char *message)
{
   /* Send acknowledgment to sender */
   char ack[BUF_SIZE];
   protocol_create_message(ack, BUF_SIZE, MSG_INFO, "Message received");
   write_client(sock, ack);
   
   /* Broadcast message to all other clients */
   char formatted_msg[BUF_SIZE];
   snprintf(formatted_msg, BUF_SIZE, "%s: %s", sender.name, message);
   
   char broadcast[BUF_SIZE];
   protocol_create_message(broadcast, BUF_SIZE, MSG_CHAT, formatted_msg);
   
   for (int i = 0; i < actual; i++)
   {
      /* Send to all clients except sender */
      if (sender.sock != clients[i].sock)
      {
         write_client(clients[i].sock, broadcast);
      }
   }
   
   printf("%s[broadcast]%s Message from %s sent to %d clients\n", COLOR_BLUE COLOR_BOLD, COLOR_RESET, sender.name, actual - 1);
}

int is_username_unique(Client *clients, int actual, const char *username)
{
   /* Check if username already exists */
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, username) == 0)
      {
         return 0;  /* Username already exists - not unique */
      }
   }
   
   return 1;  /* Username is unique */
}

void handle_bio_command(int sock, Client *clients, int client_index, const char *bio_text)
{
   /* Validate bio text */
   if (bio_text == NULL || strlen(bio_text) == 0)
   {
      char error_msg[BUF_SIZE];
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, "Bio text cannot be empty");
      write_client(sock, error_msg);
      return;
   }
   
   /* Check bio length */
   if (strlen(bio_text) > MAX_BIO_LEN - 1)
   {
      char error_msg[BUF_SIZE];
      snprintf(error_msg, BUF_SIZE, "Bio text too long (max %d characters)", MAX_BIO_LEN - 1);
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, error_msg);
      write_client(sock, error_msg);
      return;
   }
   
   /* Update the bio for the current user */
   strncpy(clients[client_index].bio, bio_text, MAX_BIO_LEN - 1);
   clients[client_index].bio[MAX_BIO_LEN - 1] = 0;
   
   /* Send confirmation message to the user */
   char ack[BUF_SIZE];
   snprintf(ack, BUF_SIZE, "Bio updated: %s", clients[client_index].bio);
   protocol_create_message(ack, BUF_SIZE, MSG_BIO_SET, ack);
   write_client(sock, ack);
   
   printf("%s[bio]%s %s updated bio: %s\n", COLOR_GREEN COLOR_BOLD, COLOR_RESET, clients[client_index].name, clients[client_index].bio);
}

void handle_getbio_command(int sock, Client *clients, int actual, const char *username)
{
   /* Validate username */
   if (username == NULL || strlen(username) == 0)
   {
      char error_msg[BUF_SIZE];
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, "Usage: getbio <username>");
      write_client(sock, error_msg);
      return;
   }

   /* Find the user among connected clients */
   int found_index = -1;
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, username) == 0)
      {
         found_index = i;
         break;
      }
   }

   if (found_index == -1)
   {
      char error_msg[BUF_SIZE];
      char tmp[BUF_SIZE];
      snprintf(tmp, BUF_SIZE, "User '%s' not found", username);
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, tmp);
      write_client(sock, error_msg);
      return;
   }

   /* Build bio response */
   char payload[BUF_SIZE];
   if (strlen(clients[found_index].bio) > 0)
   {
      snprintf(payload, BUF_SIZE, "%s: %s", clients[found_index].name, clients[found_index].bio);
   }
   else
   {
      snprintf(payload, BUF_SIZE, "%s: no bio", clients[found_index].name);
   }

   char response[BUF_SIZE];
   protocol_create_message(response, BUF_SIZE, MSG_BIO_INFO, payload);
   write_client(sock, response);
}

void handle_pm_command(int sock, Client *clients, Client sender, int actual, const char *args)
{
   /* Validate args: expect "<username> <message>" */
   if (args == NULL || strlen(args) == 0)
   {
      char error_msg[BUF_SIZE];
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, "Usage: pm <username> <message>");
      write_client(sock, error_msg);
      return;
   }

   char target[MAX_USERNAME_LEN];
   char message[BUF_SIZE];
   target[0] = 0;
   message[0] = 0;

   /* Split first token (username) and the rest (message) */
   const char *space = strchr(args, ' ');
   if (space == NULL)
   {
      char error_msg[BUF_SIZE];
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, "Usage: pm <username> <message>");
      write_client(sock, error_msg);
      return;
   }
   size_t uname_len = (size_t)(space - args);
   if (uname_len == 0 || uname_len >= MAX_USERNAME_LEN)
   {
      char error_msg[BUF_SIZE];
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, "Invalid username");
      write_client(sock, error_msg);
      return;
   }
   strncpy(target, args, uname_len);
   target[uname_len] = '\0';
   strncpy(message, space + 1, BUF_SIZE - 1);
   message[BUF_SIZE - 1] = '\0';

   if (strlen(message) == 0)
   {
      char error_msg[BUF_SIZE];
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, "Message cannot be empty");
      write_client(sock, error_msg);
      return;
   }

   if (strcmp(target, sender.name) == 0)
   {
      char error_msg[BUF_SIZE];
      protocol_create_message(error_msg, BUF_SIZE, MSG_ERROR, "Cannot send PM to yourself");
      write_client(sock, error_msg);
      return;
   }

   /* Find target client */
   int target_index = -1;
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, target) == 0)
      {
         target_index = i;
         break;
      }
   }

   if (target_index == -1)
   {
      char error_buf[BUF_SIZE];
      char tmp[BUF_SIZE];
      snprintf(tmp, BUF_SIZE, "User '%s' not found", target);
      protocol_create_message(error_buf, BUF_SIZE, MSG_ERROR, tmp);
      write_client(sock, error_buf);
      return;
   }

   /* Build outgoing messages */
   char to_target_payload[BUF_SIZE];
   snprintf(to_target_payload, BUF_SIZE, "%s -> you: %s", sender.name, message);
   char to_sender_payload[BUF_SIZE];
   snprintf(to_sender_payload, BUF_SIZE, "you -> %s: %s", target, message);

   char to_target_msg[BUF_SIZE];
   protocol_create_message(to_target_msg, BUF_SIZE, MSG_PRIVATE_CHAT, to_target_payload);
   char to_sender_msg[BUF_SIZE];
   protocol_create_message(to_sender_msg, BUF_SIZE, MSG_PRIVATE_CHAT, to_sender_payload);

   /* Send to target and confirmation to sender */
   write_client(clients[target_index].sock, to_target_msg);
   write_client(sock, to_sender_msg);
}

void handle_challenge_command(int sock, Client *clients, int client_index, int actual, const char *target_name)
{
   if (clients[client_index].status != CLIENT_IDLE) {
      notify(sock, MSG_ERROR, "You can't challenge while in a game or awaiting acceptance");
      return;
   }
   if (clients[client_index].pending_challenge_to[0] != '\0') {
      notify(sock, MSG_ERROR, "You already have a pending challenge to %s", clients[client_index].pending_challenge_to);
      return;
   }
   if (!target_name || strlen(target_name) == 0) {
      notify(sock, MSG_ERROR, "Usage: challenge <username>");
      return;
   }
   if (strcmp(target_name, clients[client_index].name) == 0) {
      notify(sock, MSG_ERROR, "You cannot challenge yourself");
      return;
   }
   int t = find_client_index_by_name(clients, actual, target_name);
   if (t == -1) {
      notify(sock, MSG_ERROR, "User '%s' not found", target_name);
      return;
   }
   if (clients[t].status != CLIENT_IDLE) {
      notify(sock, MSG_ERROR, "User '%s' is busy", clients[t].name);
      return;
   }
   if (clients[t].pending_challenge_from[0] != '\0') {
      notify(sock, MSG_ERROR, "User '%s' already has a pending challenge", clients[t].name);
      return;
   }
   strncpy(clients[client_index].pending_challenge_to, clients[t].name, MAX_USERNAME_LEN-1);
   clients[client_index].pending_challenge_to[MAX_USERNAME_LEN-1] = '\0';
   strncpy(clients[t].pending_challenge_from, clients[client_index].name, MAX_USERNAME_LEN-1);
   clients[t].pending_challenge_from[MAX_USERNAME_LEN-1] = '\0';
   clients[client_index].status = CLIENT_WAITING_FOR_ACCEPT;
   notify(sock, MSG_INFO, "Challenge sent to %s", clients[t].name);
   notify(clients[t].sock, MSG_CHALLENGE, "from %s", clients[client_index].name);
}

void handle_cancel_command(int sock, Client *clients, int client_index, int actual, const char *target_name)
{
   (void)actual; // unused
   if (clients[client_index].pending_challenge_to[0] == '\0') {
      notify(sock, MSG_ERROR, "No pending challenge to cancel");
      return;
   }
   if (target_name && strlen(target_name) > 0 && strcmp(target_name, clients[client_index].pending_challenge_to) != 0) {
      notify(sock, MSG_ERROR, "Your pending challenge is to %s", clients[client_index].pending_challenge_to);
      return;
   }
   int t = find_client_index_by_name(clients, actual, clients[client_index].pending_challenge_to);
   if (t != -1) {
      notify(clients[t].sock, MSG_CHALLENGE_RESPONSE, "%s cancelled the challenge", clients[client_index].name);
      clients[t].pending_challenge_from[0] = '\0';
   }
   clients[client_index].pending_challenge_to[0] = '\0';
   clients[client_index].status = CLIENT_IDLE;
   notify(sock, MSG_INFO, "Challenge cancelled");
}

void handle_refuse_command(int sock, Client *clients, int client_index, int actual, const char *target_name)
{
   if (clients[client_index].pending_challenge_from[0] == '\0') {
      notify(sock, MSG_ERROR, "You have no incoming challenge");
      return;
   }
   if (target_name && strlen(target_name) > 0 && strcmp(target_name, clients[client_index].pending_challenge_from) != 0) {
      notify(sock, MSG_ERROR, "Incoming challenge is from %s", clients[client_index].pending_challenge_from);
      return;
   }
   int s = find_client_index_by_name(clients, actual, clients[client_index].pending_challenge_from);
   if (s != -1) {
      notify(clients[s].sock, MSG_CHALLENGE_RESPONSE, "%s refused your challenge", clients[client_index].name);
      clients[s].pending_challenge_to[0] = '\0';
      clients[s].status = CLIENT_IDLE;
   }
   clients[client_index].pending_challenge_from[0] = '\0';
   notify(sock, MSG_INFO, "Challenge refused");
}

void handle_accept_command(int sock, Client *clients, int client_index, int actual, const char *target_name)
{
   if (clients[client_index].pending_challenge_from[0] == '\0') {
      notify(sock, MSG_ERROR, "You have no incoming challenge");
      return;
   }
   if (target_name && strlen(target_name) > 0 && strcmp(target_name, clients[client_index].pending_challenge_from) != 0) {
      notify(sock, MSG_ERROR, "Incoming challenge is from %s", clients[client_index].pending_challenge_from);
      return;
   }
   int s = find_client_index_by_name(clients, actual, clients[client_index].pending_challenge_from);
   if (s == -1) {
      notify(sock, MSG_ERROR, "Challenger disconnected");
      clients[client_index].pending_challenge_from[0] = '\0';
      return;
   }
   // clear challenge states
   clients[s].pending_challenge_to[0] = '\0';
   clients[client_index].pending_challenge_from[0] = '\0';
   // start match
   start_match(clients, s, client_index);
}

static Match* get_match_by_id(int id) {
   if (id < 0 || id >= match_count) return NULL;
   return &matches[id];
}

void handle_move_command(int sock, Client *clients, int client_index, int actual, const char *pit_str)
{
   (void)actual; // unused
   if (clients[client_index].status != CLIENT_IN_MATCH) {
      notify(sock, MSG_ERROR, "You are not in a game");
      return;
   }
   if (!clients[client_index].is_turn) {
      notify(sock, MSG_ERROR, "Not your turn");
      return;
   }
   if (!pit_str || strlen(pit_str)==0) {
      notify(sock, MSG_ERROR, "Usage: move <pit>");
      return;
   }
   int pit = atoi(pit_str);
   Match *m = get_match_by_id(clients[client_index].current_match);
   if (!m) { notify(sock, MSG_ERROR, "Internal error: match missing"); return; }
   int is_player_a = (client_index == m->player1_index);
   int logical_player = is_player_a ? 0 : 1; // map to board.current_player
   if (m->board.current_player != logical_player) {
      notify(sock, MSG_ERROR, "Not your turn");
      return;
   }
   if (!is_valid_move(&m->board, pit)) {
      notify(sock, MSG_ERROR, "Invalid move");
      return;
   }
   make_move(&m->board, pit);
   // swap turns
   clients[m->player1_index].is_turn = (m->board.current_player == 0);
   clients[m->player2_index].is_turn = (m->board.current_player == 1);
   // notify move
   notify(clients[m->player1_index].sock, MSG_MOVE, "%s played pit %d", clients[client_index].name, pit);
   notify(clients[m->player2_index].sock, MSG_MOVE, "%s played pit %d", clients[client_index].name, pit);
   broadcast_board(m, clients);
   if (is_game_over(&m->board)) {
      // announce winner
      const char *winner = NULL;
      if (m->board.score[0] > m->board.score[1]) winner = clients[m->player1_index].name;
      else if (m->board.score[1] > m->board.score[0]) winner = clients[m->player2_index].name;
      char payload[BUF_SIZE];
      if (winner) snprintf(payload, sizeof(payload), "Game over. Winner: %s (%d-%d)", winner, m->board.score[0], m->board.score[1]);
      else snprintf(payload, sizeof(payload), "Game over. Draw (%d-%d)", m->board.score[0], m->board.score[1]);
      notify(clients[m->player1_index].sock, MSG_GAME_OVER, "%s", payload);
      notify(clients[m->player2_index].sock, MSG_GAME_OVER, "%s", payload);
      end_match(m, clients);
   }
}

void handle_quit_command(int sock, Client *clients, int client_index, int actual)
{
   (void)actual; // unused
   if (clients[client_index].status != CLIENT_IN_MATCH) {
      notify(sock, MSG_ERROR, "You are not in a game");
      return;
   }
   Match *m = get_match_by_id(clients[client_index].current_match);
   if (!m) { notify(sock, MSG_ERROR, "Internal error: match missing"); return; }
   int other = (client_index == m->player1_index) ? m->player2_index : m->player1_index;
   notify(clients[other].sock, MSG_GAME_OVER, "%s quit the game", clients[client_index].name);
   notify(sock, MSG_GAME_OVER, "You quit the game");
   end_match(m, clients);
}

void handle_games_command(int sock, Client *clients, int actual)
{
   (void)clients; // not needed directly, we reference via matches
   char list[BUF_SIZE];
   list[0] = '\0';
   if (match_count == 0) {
      protocol_create_message(list, BUF_SIZE, MSG_MATCH_LIST, "No games running");
      write_client(sock, list);
      return;
   }
   // Build a multi-line list: one game per line
   char payload[BUF_SIZE];
   payload[0] = '\0';
   for (int i = 0; i < match_count; i++)
   {
      Match *m = &matches[i];
      // Determine whose turn
      const char *p1 = clients[m->player1_index].name;
      const char *p2 = clients[m->player2_index].name;
      const char *turn = (m->board.current_player == 0) ? p1 : p2;
      char line[256];
      snprintf(line, sizeof(line), "#%d %s vs %s | turn: %s\n", m->id, p1, p2, turn);
      strncat(payload, line, sizeof(payload) - strlen(payload) - 1);
   }
   protocol_create_message(list, BUF_SIZE, MSG_MATCH_LIST, payload);
   write_client(sock, list);
}
