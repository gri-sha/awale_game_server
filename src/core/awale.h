/*
 * AWALE GAME (also known as Oware, Wari, or Mancala)
 *
 * RULES:
 * 1. The board has 12 pits (6 per player) and each pit starts with 4 seeds
 * 2. Players alternate turns, selecting a pit on their side
 * 3. Seeds from the selected pit are sown counter-clockwise, one per pit
 * 4. The starting pit is left empty (seeds are distributed to following pits) (the case when seeds >= 12)
 * 5. CAPTURING: After sowing, if the last seed lands in an opponent's pit
 *    and that pit now has 2 or 3 seeds, those seeds are captured
 * 6. Continue capturing backwards if previous pits also have 2 or 3 seeds (previous numerically, if we skipped the starting pit it will be stopped at starting pit)
 * 7. You cannot capture all opponent's seeds (must leave them with a move)
 * 8. If opponent has no seeds, you must give them seeds if possible (if it not possible player having the seeds takes all the seeds)
 * 9. Game ends when one player captures 25+ seeds or no more moves possible
 * 10. Player with most seeds wins
 */

#ifndef AWALE_H
#define AWALE_H

#include <stdbool.h>
#include "../utils/constants.h"

// Structure to represent the game board
typedef struct
{
    int pits[TOTAL_PITS]; // Pits 0-5: Player 1, Pits 6-11: Player 2
    int score[2];         // Captured seeds for each player
    int current_player;   // 0 for Player 1, 1 for Player 2
} Board;

void init_board(Board *board);
void display_board(const Board *board);
bool is_valid_move(const Board *board, int pit);
bool opponent_has_seeds(const Board *board, int player);
bool move_gives_seeds_to_opponent(const Board *board, int pit);
// Render the board into a buffer and return number of bytes written (excluding final null terminator).
// The rendering mirrors display_board, including colors, but as text for network/clients.
int render_board(const Board *board, char *out, size_t out_size);
void make_move(Board *board, int pit);
bool is_game_over(const Board *board);
void display_winner(const Board *board);
int get_player_input(const Board *board);

#endif
