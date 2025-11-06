#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "awale.h"
#include "../utils/constants.h"

// Initialize the board with starting position
void init_board(Board *board)
{
    // Each pit starts with 4 seeds
    for (int i = 0; i < TOTAL_PITS; i++)
    {
        board->pits[i] = INITIAL_SEEDS;
    }
    // No seeds captured yet
    board->score[0] = 0;
    board->score[1] = 0;
    // Player 1 starts
    board->current_player = 0;
}

// Display the board in ASCII art
void display_board(const Board *board)
{
    printf("\n");
    printf("=====================================\n\n");

    // Player 2's side (top, reversed for visual clarity)
    printf("%sPlayer 2 [Score: %2d]           <-- Direction%s\n", COLOR_BOLD COLOR_YELLOW, board->score[1], COLOR_RESET);
    printf("     ");
    for (int i = TOTAL_PITS - 1; i >= PITS_PER_PLAYER; i--)
    {
        printf("%s[%2d]%s", COLOR_GREEN, board->pits[i], COLOR_RESET);
    }
    printf("\n");

    // Pit numbers for Player 2
    printf("%sPit:  %s", STYLE_DIM, COLOR_RESET);
    for (int i = TOTAL_PITS - 1; i >= PITS_PER_PLAYER; i--)
    {
        printf("%s %2d %s", STYLE_DIM, i, COLOR_RESET);
    }
    printf("\n");

    printf("%s     -------------------------\n%s", COLOR_BLUE, COLOR_RESET);

    // Pit numbers for Player 1
    printf("%sPit:  %s", STYLE_DIM, COLOR_RESET);
    for (int i = 0; i < PITS_PER_PLAYER; i++)
    {
        printf("%s %2d %s", STYLE_DIM, i, COLOR_RESET);
    }
    printf("\n");

    // Player 1's side (bottom)
    printf("     ");
    for (int i = 0; i < PITS_PER_PLAYER; i++)
    {
        printf("%s[%2d]%s", COLOR_RED, board->pits[i], COLOR_RESET);
    }
    printf("\n%sDirection -->           Player 1 [Score: %2d]%s\n\n", COLOR_BOLD COLOR_YELLOW, board->score[0], COLOR_RESET);
}

// Render the board to a provided buffer (similar to display_board but as text)
int render_board(const Board *board, char *out, size_t out_size)
{
    if (out == NULL || out_size == 0) return 0;
    size_t off = 0;
    #define APPENDF(fmt, ...) do { \
        if (off < out_size) { \
            int _n = snprintf(out + off, out_size - off, fmt, ##__VA_ARGS__); \
            if (_n > 0) off += (size_t)_n; \
        } \
    } while (0)

    APPENDF("\n");
    APPENDF("=====================================\n\n");

    APPENDF("%sPlayer 2 [Score: %2d]           <-- Direction%s\n", COLOR_BOLD COLOR_YELLOW, board->score[1], COLOR_RESET);
    APPENDF("     ");
    for (int i = TOTAL_PITS - 1; i >= PITS_PER_PLAYER; i--)
    {
        APPENDF("%s[%2d]%s", COLOR_GREEN, board->pits[i], COLOR_RESET);
    }
    APPENDF("\n");

    APPENDF("%sPit:  %s", STYLE_DIM, COLOR_RESET);
    for (int i = TOTAL_PITS - 1; i >= PITS_PER_PLAYER; i--)
    {
        APPENDF("%s %2d %s", STYLE_DIM, i, COLOR_RESET);
    }
    APPENDF("\n");

    APPENDF("%s     -------------------------%s\n", COLOR_BLUE, COLOR_RESET);

    APPENDF("%sPit:  %s", STYLE_DIM, COLOR_RESET);
    for (int i = 0; i < PITS_PER_PLAYER; i++)
    {
        APPENDF("%s %2d %s", STYLE_DIM, i, COLOR_RESET);
    }
    APPENDF("\n");

    APPENDF("     ");
    for (int i = 0; i < PITS_PER_PLAYER; i++)
    {
        APPENDF("%s[%2d]%s", COLOR_RED, board->pits[i], COLOR_RESET);
    }
    APPENDF("\n%sDirection -->           Player 1 [Score: %2d]%s\n\n", COLOR_BOLD COLOR_YELLOW, board->score[0], COLOR_RESET);

    #undef APPENDF
    if (off >= out_size) {
        // ensure null-termination in worst case
        out[out_size - 1] = '\0';
        return (int)(out_size - 1);
    }
    return (int)off;
}

// Check if a move is valid
bool is_valid_move(const Board *board, int pit)
{
    // Check if pit is in valid range
    if (pit < 0 || pit >= TOTAL_PITS)
    {
        printf("%s✗ Invalid pit number!%s\n", COLOR_RED, COLOR_RESET);
        return false;
    }
 
    // Check if pit belongs to current player
    int player = board->current_player;
    int start_pit = player * PITS_PER_PLAYER;
    int end_pit = start_pit + PITS_PER_PLAYER;

    if (pit < start_pit || pit >= end_pit)
    {
        printf("%s✗ You can only choose pits on your side!%s\n", COLOR_RED, COLOR_RESET);
        return false;
    }

    // Check if pit has seeds
    if (board->pits[pit] == 0)
    {
        printf("%s✗ This pit is empty!%s\n", COLOR_RED, COLOR_RESET);
        return false;
    }

    // Check if opponent has seeds
    if (!opponent_has_seeds(board, player))
    {
        // Must give seeds to opponent if possible
        if (!move_gives_seeds_to_opponent(board, pit))
        {
            printf("%s✗ You must give seeds to your opponent!%s\n", COLOR_RED, COLOR_RESET);
            return false;
        }
    }

    return true;
}

// Check if opponent has any seeds
bool opponent_has_seeds(const Board *board, int player)
{
    int opponent = 1 - player;
    int start_pit = opponent * PITS_PER_PLAYER;
    int end_pit = start_pit + PITS_PER_PLAYER;

    for (int i = start_pit; i < end_pit; i++)
    {
        if (board->pits[i] > 0)
        {
            return true;
        }
    }
    return false;
}

// Check if a move would give seeds to opponent
bool move_gives_seeds_to_opponent(const Board *board, int pit)
{
    int seeds = board->pits[pit];
    int opponent = 1 - board->current_player;
    int opp_start = opponent * PITS_PER_PLAYER;
    int opp_end = opp_start + PITS_PER_PLAYER;

    // Simulate sowing
    int current_pit = pit;
    for (int i = 0; i < seeds; i++)
    {
        current_pit = (current_pit + 1) % TOTAL_PITS;
        // Check if we land in opponent's territory
        if (current_pit >= opp_start && current_pit < opp_end)
        {
            return true;
        }
    }
    return false;
}

// Make a move and handle capturing
void make_move(Board *board, int pit)
{
    int seeds = board->pits[pit];
    board->pits[pit] = 0; // Empty the selected pit

    int current_pit = pit;

    // Sow seeds counter-clockwise
    for (int i = 0; i < seeds; i++)
    {
        current_pit = (current_pit + 1) % TOTAL_PITS;
        if (current_pit == pit)
        {
            current_pit = (current_pit + 1) % TOTAL_PITS; // Skip the starting pit
        }
        board->pits[current_pit]++;
    }

    // Check for capturing
    int player = board->current_player;
    int opponent = 1 - player;
    int opp_start = opponent * PITS_PER_PLAYER;
    int opp_end = opp_start + PITS_PER_PLAYER;

    // Only capture if last seed landed in opponent's territory
    if (current_pit >= opp_start && current_pit < opp_end)
    {
        // Capture backwards while conditions are met
        int capture_pit = current_pit;
        int captured_total = 0;

        // First, count how many seeds would be captured
        int temp_pit = capture_pit;
        while (temp_pit >= opp_start && temp_pit < opp_end &&
               (board->pits[temp_pit] == 2 || board->pits[temp_pit] == 3))
        {
            captured_total += board->pits[temp_pit];
            temp_pit--;
        }

        // Check if capturing would leave opponent with no seeds
        int opponent_remaining = 0;
        for (int i = opp_start; i < opp_end; i++)
        {
            opponent_remaining += board->pits[i];
        }

        // Only capture if opponent would still have seeds left
        if (opponent_remaining - captured_total > 0)
        {
            while (capture_pit >= opp_start && capture_pit < opp_end &&
                   (board->pits[capture_pit] == 2 || board->pits[capture_pit] == 3))
            {
                board->score[player] += board->pits[capture_pit];
                printf("[CAPTURE] Player %d captures %d seeds from pit %d!\n",
                       player + 1, board->pits[capture_pit], capture_pit);
                board->pits[capture_pit] = 0;
                capture_pit--;
            }
        }
    }

    // Switch player
    board->current_player = opponent;
}

// Check if game is over
bool is_game_over(const Board *board)
{
    // Someone has won by capturing 25+ seeds
    if (board->score[0] >= MIN_SEEDS_TO_WIN || board->score[1] >= MIN_SEEDS_TO_WIN)
    {
        return true;
    }

    // Check if current player has any valid moves
    int player = board->current_player;
    int start_pit = player * PITS_PER_PLAYER;
    int end_pit = start_pit + PITS_PER_PLAYER;

    bool has_seeds = false;
    for (int i = start_pit; i < end_pit; i++)
    {
        if (board->pits[i] > 0)
        {
            has_seeds = true;
            break;
        }
    }

    if (!has_seeds)
    {
        return true;
    }

    // Check if opponent has seeds
    if (!opponent_has_seeds(board, player))
    {
        // Check if current player can give seeds
        bool can_give_seeds = false;
        for (int i = start_pit; i < end_pit; i++)
        {
            if (board->pits[i] > 0 && move_gives_seeds_to_opponent(board, i))
            {
                can_give_seeds = true;
                break;
            }
        }
        if (!can_give_seeds)
        {
            return true;
        }
    }

    return false;
}

// Display the winner
void display_winner(const Board *board)
{
    printf("\n");
    printf("=====================================\n");
    printf("           GAME OVER!\n");
    printf("=====================================\n");
    printf("Player 1 Score: %d\n", board->score[0]);
    printf("Player 2 Score: %d\n", board->score[1]);

    if (board->score[0] > board->score[1])
    {
        printf("\n%s🎉 Player 1 WINS! 🎉%s\n", COLOR_BOLD COLOR_GREEN, COLOR_RESET);
    }
    else if (board->score[1] > board->score[0])
    {
        printf("\n%s🎉 Player 2 WINS! 🎉%s\n", COLOR_BOLD COLOR_GREEN, COLOR_RESET);
    }
    else
    {
        printf("\n%s🤝 It's a TIE! 🤝%s\n", COLOR_BOLD COLOR_YELLOW, COLOR_RESET);
    }
    printf("=====================================\n\n");
}

// Get valid input from player
int get_player_input(const Board *board)
{
    int pit;
    char input[10];

    while (true)
    {
        printf("%s→ Player %d: %s", 
               COLOR_BOLD COLOR_BLUE, board->current_player + 1, COLOR_RESET);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            continue;
        }

        // Check for quit
        if (input[0] == 'q' || input[0] == 'Q')
        {
            printf("Thanks for playing!\n");
            exit(0);
        }

        // Try to parse as integer
        if (sscanf(input, "%d", &pit) != 1)
        {
            printf("%s✗ Please enter a valid pit number!%s\n", COLOR_RED, COLOR_RESET);
            continue;
        }

        // Validate the move
        if (is_valid_move(board, pit))
        {
            printf("=====================================\n\n");
            return pit;
        }
    }
}