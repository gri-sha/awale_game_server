#include <stdio.h>
#include "core/awale.h"
#include "utils/constants.h"

int main(void)
{
    Board board;
    init_board(&board);

    printf("\n");
    printf("%s%s╔═══════════════════════════════════╗%s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);
    printf("%s%s║               AWALE               ║%s\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);
    printf("%s%s╚═══════════════════════════════════╝%s\n\n", COLOR_BOLD, COLOR_BLUE, COLOR_RESET);
    printf("Enter a pit number (0-5 for Player 1, 6-11 for Player 2). Type 'q' to quit.\n\n");

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
