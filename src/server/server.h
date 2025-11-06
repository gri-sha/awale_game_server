#ifndef SERVER_H
#define SERVER_H
#include "../utils/constants.h"

typedef enum {
   CLIENT_IDLE,
   CLIENT_WAITING_FOR_ACCEPT,
   CLIENT_IN_MATCH
} ClientStatus;

typedef struct
{
   int sock;
   char name[MAX_USERNAME_LEN];
   char bio[MAX_BIO_LEN];
   ClientStatus status;        // CLIENT_IDLE, CLIENT_WAITING_FOR_ACCEPT, CLIENT_IN_MATCH
   int current_match; // match id, -1 if not in a match
   // Challenge state
   char pending_challenge_to[MAX_USERNAME_LEN];      // username we challenged (if any)
   char pending_challenge_from[MAX_USERNAME_LEN];    // username who challenged us (if any)
   int is_turn; // for in-game: 1 if it's this client's turn, else 0
} Client;

void init(void);
void end(void);
void app(void);
int init_connection(void);
void end_connection(int sock);
int read_from_client(int sock, char *buffer);
void write_client(int sock, const char *buffer);
void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
void remove_client(Client *clients, int to_remove, int *actual);
void clear_clients(Client *clients, int actual);
char *get_server_ip(void);
void handle_list_command(int sock, Client *clients, int actual);
void handle_message_command(int sock, Client *clients, Client sender, int actual, const char *message);
void handle_bio_command(int sock, Client *clients, int client_index, const char *bio_text);
void handle_getbio_command(int sock, Client *clients, int actual, const char *username);
void handle_pm_command(int sock, Client *clients, Client sender, int actual, const char *args);
int is_username_unique(Client *clients, int actual, const char *username);
/* Challenge & Game handlers */
void handle_challenge_command(int sock, Client *clients, int client_index, int actual, const char *target_name);
void handle_accept_command(int sock, Client *clients, int client_index, int actual, const char *target_name);
void handle_refuse_command(int sock, Client *clients, int client_index, int actual, const char *target_name);
void handle_cancel_command(int sock, Client *clients, int client_index, int actual, const char *target_name);
void handle_move_command(int sock, Client *clients, int client_index, int actual, const char *pit_str);
void handle_quit_command(int sock, Client *clients, int client_index, int actual);
void handle_games_command(int sock, Client *clients, int actual);

#endif /* guard */
