#include "engine.h"
#include "interface.h"

void init_game() {
  /* Initialize board */
  init_board();

  /* Inform user of board setup */
  set_status_msg("Your turn...");

  redraw();
};

void attempt_move(int from_x, int from_y, int to_x, int to_y,
                  char promotion_type) {
  Move moves[MAX_MOVES];
  int num_legal_moves = get_legal_moves(moves);
  for (int i = 0; i < num_legal_moves; i++) {
    Move *move = &moves[i];
    if (move->to.x == to_x && move->to.y == to_y && move->from.x == from_x &&
        move->from.y == from_y) {
      PieceColor turn = BOARD.turn;
      make_move(move);
      /* Check if move resulted in a check */
      if (is_square_attacked(BOARD.king_pos[turn], BOARD.turn)) {
        set_status_msg("You're in check");
        unmake_move(move);
      } else {
        set_status_msg("Thinking...");
      }
      redraw();
      return;
    }
  }
  set_status_msg("Invalid move");
  redraw();
}
