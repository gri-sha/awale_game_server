#include <stdio.h>
#include <string.h>
#include "core/awale.h"
#include "utils/constants.h"

// Displays total seeds at the bottom of the screen without moving the cursor
void display_total_seeds_at_bottom(int total_seeds, int max_seeds)
{
    printf("%s", CURSOR_SAVE);        // Save cursor position
    printf("%s", CURSOR_MOVE_BOTTOM); // Move to bottom of screen
    printf("%s", CLEAR_LINE);         // Clear line
    printf("\nTotal seeds so far: %d/%d", total_seeds, max_seeds);
    printf("%s", CURSOR_RESTORE); // Restore cursor position
    fflush(stdout);
}

void remove_total_seeds_display()
{
    printf("%s", CURSOR_SAVE);        // Save cursor position
    printf("%s", CURSOR_MOVE_BOTTOM); // Move to bottom of screen
    printf("%s", CLEAR_LINE);         // Clear line
    printf("%s", CURSOR_RESTORE);     // Restore cursor position
    fflush(stdout);
}

// Get validated integer input from user
int get_validated_integer_input(const char *prompt)
{
    int value;
    int result;

    while (1)
    {
        printf("%s", prompt);
        result = scanf("%d", &value);

        if (result == 1)
        {
            // Valid integer input
            return value;
        }
        else if (result == 0)
        {
            // Non-numeric input, clear the buffer
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            printf("Invalid input! Please enter a number.\n");
        }
        else if (result == EOF)
        {
            // End of file
            printf("Input error!\n");
            return 0;
        }
    }
}

// Checks wherther the sseds are distributed properly across the board
bool is_valid_board_seeds(const Board *board)
{
    int total_seeds = 0;
    for (int i = 0; i < TOTAL_PITS; i++)
    {
        if (board->pits[i] < 0)
            return false; // Negative seeds in a pit is invalid
        total_seeds += board->pits[i];
    }
    // Total seeds must equal INITIAL_SEEDS * TOTAL_PITS
    return total_seeds + board->score[0] + board->score[1] == INITIAL_SEEDS * TOTAL_PITS;
}

// Retrurns true if the board is valid, false otherwise
bool input_init_board(Board *board)
{
    int total_seeds = 0;

    printf("Enter the number if seeds in each pit for player 1:\n");
    for (int i = 0; i < PITS_PER_PLAYER; i++)
    {
        char prompt[20];
        sprintf(prompt, "Pit %d: ", i);
        board->pits[i] = get_validated_integer_input(prompt);
        total_seeds += board->pits[i];
        display_total_seeds_at_bottom(total_seeds, INITIAL_SEEDS * TOTAL_PITS);
    }
    display_total_seeds_at_bottom(total_seeds, INITIAL_SEEDS * TOTAL_PITS);

    printf("\n\nEnter the number if seeds in each pit for player 2:\n");
    for (int i = PITS_PER_PLAYER; i < TOTAL_PITS; i++)
    {
        char prompt[20];
        sprintf(prompt, "Pit %d: ", i);
        board->pits[i] = get_validated_integer_input(prompt);
        total_seeds += board->pits[i];
        display_total_seeds_at_bottom(total_seeds, INITIAL_SEEDS * TOTAL_PITS);
    }
    display_total_seeds_at_bottom(total_seeds, INITIAL_SEEDS * TOTAL_PITS);

    printf("\n\nEnter the scores for both players:\n");
    board->score[0] = get_validated_integer_input("Player 1 Score: ");
    total_seeds += board->score[0];
    display_total_seeds_at_bottom(total_seeds, INITIAL_SEEDS * TOTAL_PITS);

    board->score[1] = get_validated_integer_input("Player 2 Score: ");
    total_seeds += board->score[1];
    display_total_seeds_at_bottom(total_seeds, INITIAL_SEEDS * TOTAL_PITS);

    if (!is_valid_board_seeds(board))
    {
        printf("\nInvalid board configuration. Please try again.\n");
        return false;
    }

    board->current_player = get_validated_integer_input("\nEnter the current player (0 for Player 1, 1 for Player 2): ");
    if (board->current_player != 0 && board->current_player != 1)
    {
        printf("Invalid player selection. Please enter 0 or 1.\n");
        return false;
    }

    // Clear the bottom line
    printf("%s", CURSOR_SAVE);        // Save cursor position
    printf("%s", CURSOR_MOVE_BOTTOM); // Move to bottom of screen
    printf("%s", CLEAR_LINE);         // Clear line
    printf("%s", CURSOR_RESTORE);     // Restore cursor position

    return true;
}

// Wrapper fo init board
void init_test_board(Board *board)
{
    bool valid = false;
    while (!valid)
    {
        valid = input_init_board(board);
        if (!valid)
        {
            printf("Invalid board configuration. Please try again.\n");
        }
    }
}

int main()
{
    Board board;
    init_board(&board);

    printf("\n");
    printf("╔═══════════════════════════════════╗\n");
    printf("║         AWALE TEST MODE           ║\n");
    printf("╚═══════════════════════════════════╝\n");
    printf("\n");

    init_test_board(&board);

    // Main game loop
    while (!is_game_over(&board))
    {
        display_board(&board);
        int pit = get_player_input(&board);
        make_move(&board, pit);
        printf("\n");
    }

    // Game is over
    display_board(&board);
    display_winner(&board);

    return 0;
}