#ifndef SERVER_H
#define SERVER_H
#include "../utils/constants.h"
#include "../core/awale.h"
#include "../protocol/protocol.h"

typedef enum
{
   CLIENT_IDLE,
   CLIENT_WAITING_FOR_ACCEPT,
   CLIENT_IN_MATCH
} ClientStatus;

typedef struct
{
   int sock;
   char name[MAX_USERNAME_LEN];
   char bio[MAX_BIO_LEN];
   ClientStatus status; // CLIENT_IDLE, CLIENT_WAITING_FOR_ACCEPT, CLIENT_IN_MATCH
   int current_match;   // match id, -1 if not in a match
   // Challenge state - support multiple challenges
   char pending_challenge_to[MAX_CHALLENGES][MAX_USERNAME_LEN];   // usernames we challenged
   int pending_challenge_to_count;                                // number of pending challenges sent
   char pending_challenge_from[MAX_CHALLENGES][MAX_USERNAME_LEN]; // usernames who challenged us
   int pending_challenge_from_count;                              // number of pending challenges received
   int is_turn;                                   // for in-game: 1 if it's this client's turn, else 0
   // Friends
   char friends[MAX_FRIENDS][MAX_USERNAME_LEN];
   int friend_count;
   char pending_friend_to[MAX_USERNAME_LEN];
   char pending_friend_from[MAX_USERNAME_LEN];
   int wins; // number of games won (session)
} Client;

typedef struct
{
   int id;
   int player1_index; // index in clients array
   int player2_index; // index in clients array
   Board board;
   int watchers[MAX_CLIENTS]; // sockets of watchers
   int watcher_count;
   int private_mode; // if 1 only friends can watch
   // replay data
   int replay_move_count;
   char replay_boards[MAX_MOVES][BUF_SIZE]; // board snapshot after each move
} Match;


int init_connection(void);
void end_connection(int sock);
int read_from_client(int sock, char *buffer);
void write_client(int sock, const char *buffer);
void send_message_to_all_clients(Client *clients, Client client, int client_count, const char *buffer, char from_server);
void remove_client(Client *clients, int to_remove, int *client_count);
void clear_clients(Client *clients, int client_count);
char *get_server_ip(void);

/* Helper functions for client and match management */
int find_client_index_by_name(Client *clients, int client_count, const char *name);
int is_friend(const Client *c, const char *username);
int add_friend(Client *c, const char *username);
void notify(int sock, MessageType type, const char *fmt, ...);
void broadcast_board(Match *m, Client *clients);
void end_match(Match *m, Client *clients);
Match *start_match(Client *clients, int a, int b, Match *matches, int *match_count);
Match *get_match_by_id(int id, Match *matches, int match_count);
void handle_list_command(int sock, Client *clients, int client_count);
void handle_message_command(int sock, Client *clients, Client sender, int client_count, const char *message);
void handle_bio_command(int sock, Client *clients, int client_index, const char *bio_text);
void handle_getbio_command(int sock, Client *clients, int client_count, const char *username);
void handle_pm_command(int sock, Client *clients, Client sender, int client_count, const char *args);
int is_username_unique(Client *clients, int client_count, const char *username);
/* Challenge & Game handlers */
void handle_challenge_command(int sock, Client *clients, int client_index, int client_count, const char *target_name);
void handle_accept_command(int sock, Client *clients, int client_index, int client_count, const char *target_name, Match *matches, int *match_count);
void handle_refuse_command(int sock, Client *clients, int client_index, int client_count, const char *target_name);
void handle_cancel_command(int sock, Client *clients, int client_index, int client_count, const char *target_name);
void handle_move_command(int sock, Client *clients, int client_index, int client_count, const char *pit_str, Match *matches, int match_count);
void handle_quit_command(int sock, Client *clients, int client_index, int client_count, Match *matches, int match_count);
void handle_games_command(int sock, Client *clients, Match *matches, int match_count);
void handle_watch_command(int sock, Client *clients, int client_index, int client_count, const char *match_id_str, Match *matches, int match_count);
void handle_unwatch_command(int sock, Client *clients, int client_index, int client_count, const char *match_id_str, Match *matches, int match_count);
void handle_addfriend_command(int sock, Client *clients, int client_index, int client_count, const char *target_name);
void handle_acceptfriend_command(int sock, Client *clients, int client_index, int client_count, const char *target_name);
void handle_refusefriend_command(int sock, Client *clients, int client_index, int client_count, const char *target_name);
void handle_private_command(int sock, Client *clients, int client_index, int client_count, const char *arg, Match *matches, int match_count);
void handle_friends_command(int sock, Client *clients, int client_index, int client_count);
void handle_ranking_command(int sock, Client *clients, int client_count);
void handle_watchreplay_command(int sock, Client *clients, int client_index, int client_count, const char *match_id_str, Match *matches, int match_count);

#endif /* guard */
