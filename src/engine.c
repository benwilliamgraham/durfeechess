#include <stdbool.h>
#include <stdint.h>

void console_log(const char *msg);

void fatal(const char *msg);

void redraw();

void set_status_msg(const char *msg);

/* Piece layout:
 * ____CTTT
 * C: color
 * T: type
 */
typedef int8_t Piece;

const Piece NULL_PIECE = -1;

typedef enum { BLACK, WHITE } PieceColor;

PieceColor get_piece_color(Piece piece) { return (piece >> 3) & 0b1; }

typedef enum { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;

PieceType PROMOTION_TYPES[] = {QUEEN, ROOK, BISHOP, KNIGHT};

PieceType get_piece_type(Piece piece) { return piece & 0b111; }

Piece create_piece(PieceColor color, PieceType type) {
  return (color << 3) | type;
}

typedef struct {
  int8_t x, y;
} Coord;

const Coord NULL_COORD = {-1, -1};

bool is_valid_coord(Coord coord) {
  return coord.x >= 0 && coord.x < 8 && coord.y >= 0 && coord.y < 8;
}

bool choord_eq(Coord a, Coord b) { return a.x == b.x && a.y == b.y; }

Coord coord_add(Coord a, Coord b) { return (Coord){a.x + b.x, a.y + b.y}; }

typedef struct {
  bool status[2][2];
} CastleStatus;

struct {
  Piece squares[8][8];
  PieceColor turn;
  CastleStatus can_castle;
  Coord en_passant;
} BOARD;

Piece get_piece(Coord coord) { return BOARD.squares[coord.y][coord.x]; }

void set_piece(Coord coord, Piece piece) {
  BOARD.squares[coord.y][coord.x] = piece;
}

typedef enum { LEFT, RIGHT } Side;

bool can_castle(PieceColor color, Side side) {
  return BOARD.can_castle.status[color][side];
}

void set_can_castle(PieceColor color, Side side, bool can_castle) {
  BOARD.can_castle.status[color][side] = can_castle;
}

bool can_en_passant() { return BOARD.en_passant.x != -1; }

typedef struct {
  Coord from, to;
  Piece moved, target, promotion;
  CastleStatus prev_can_castle;
  Coord prev_en_passant;
} Move;

Move create_move(Coord from, Coord to, Piece moved, Piece target,
                 Piece promotion) {
  return (Move){
      .from = from,
      .to = to,
      .moved = moved,
      .target = target,
      .promotion = promotion,
      .prev_can_castle = BOARD.can_castle,
      .prev_en_passant = BOARD.en_passant,
  };
}

/* `move_buffer` is known to be large enough to hold all possible moves for a
 * given side (rounded up out of simplicity).
 * - Pawn (8x): 32
 *   - 2 move (1 space forward, 2 spaces forward if first move)
 *   - 2 capture (left and right)
 * - Knight (2x): 16
 *   - 8 moves
 * - Bishop (2x): 26
 *   - 13 moves
 * - Rook (2x): 28
 *   - 14 moves
 * - Queen (1x): 27
 *   - 27 moves
 * - King (1x): 8
 *   - 8 moves
 * - Total:
 *   32 + 16 + 26 + 28 + 27 + 8 = 137
 */
const int MAX_MOVES = 137;
int get_legal_moves(Move *move_buffer) {
  int num_legal_moves = 0;
  /* Iterate over squares */
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      Coord from = {x, y};
      Piece moved = get_piece(from);
      if (moved == NULL_PIECE || get_piece_color(moved) != BOARD.turn)
        continue;

      /* Pawn handling */
      if (get_piece_type(moved) == PAWN) {
        int forward = (BOARD.turn == BLACK) ? 1 : -1;

        /* Single square forward */
        Coord single_fwd = {x, y + forward};
        if (is_valid_coord(single_fwd) && get_piece(single_fwd) == NULL_PIECE) {
          /* Check for promotion */
          if (y == (BOARD.turn == BLACK ? 6 : 1)) {
            for (int i = 0; i < 4; i++) {
              move_buffer[num_legal_moves++] =
                  create_move(from, single_fwd, moved, NULL_PIECE,
                              create_piece(BOARD.turn, PROMOTION_TYPES[i]));
            }
          } else {
            move_buffer[num_legal_moves++] =
                create_move(from, single_fwd, moved, NULL_PIECE, NULL_PIECE);
          }
        }

        /* Double square forward */
        Coord double_fwd = {x, y + 2 * forward};
        if ((y == (BOARD.turn == BLACK ? 1 : 6)) &&
            get_piece(single_fwd) == NULL_PIECE &&
            get_piece(double_fwd) == NULL_PIECE)
          move_buffer[num_legal_moves++] =
              create_move(from, double_fwd, moved, NULL_PIECE, NULL_PIECE);

        /* Normal capture */
        Coord capture_coords[2] = {
            {x - 1, y + forward},
            {x + 1, y + forward},
        };
        for (int i = 0; i < 2; i++) {
          Coord capture_coord = capture_coords[i];
          if (is_valid_coord(capture_coord) &&
              get_piece(capture_coord) != NULL_PIECE &&
              get_piece_color(get_piece(capture_coord)) != BOARD.turn) {
            /* Check for promotion */
            if (y == (BOARD.turn == BLACK ? 6 : 1)) {
              for (int j = 0; j < 4; j++) {
                move_buffer[num_legal_moves++] = create_move(
                    from, capture_coord, moved, get_piece(capture_coord),
                    create_piece(BOARD.turn, PROMOTION_TYPES[j]));
              }
            } else {
              move_buffer[num_legal_moves++] =
                  create_move(from, capture_coord, moved,
                              get_piece(capture_coord), NULL_PIECE);
            }
          }
        }

        /* En passant capture */
        Coord en_passant_coords[2] = {
            {x - 1, y},
            {x + 1, y},
        };
        if (can_en_passant()) {
          Coord en_passant_coord = BOARD.en_passant;
          for (int i = 0; i < 2; i++) {
            if (choord_eq(en_passant_coord, en_passant_coords[i])) {
              move_buffer[num_legal_moves++] =
                  create_move(from, capture_coords[i], moved,
                              get_piece(en_passant_coord), NULL_PIECE);
            }
          }
        }
      }

      /* Knight handling */
      else if (get_piece_type(moved) == KNIGHT) {
        Coord knight_coords[8] = {
            {x - 1, y - 2}, {x - 2, y - 1}, {x + 1, y - 2}, {x + 2, y - 1},
            {x - 1, y + 2}, {x - 2, y + 1}, {x + 1, y + 2}, {x + 2, y + 1},
        };
        for (int i = 0; i < 8; i++) {
          Coord knight_coord = knight_coords[i];
          if (is_valid_coord(knight_coord)) {
            Piece target = get_piece(knight_coord);
            if (target == NULL_PIECE || get_piece_color(target) != BOARD.turn) {
              move_buffer[num_legal_moves++] =
                  create_move(from, knight_coord, moved, target, NULL_PIECE);
            }
          }
        }
      }

      /* Diagonal handling */
      if (get_piece_type(moved) == BISHOP || get_piece_type(moved) == QUEEN) {
        Coord diag_dirs[4] = {
            {-1, -1},
            {1, -1},
            {-1, 1},
            {1, 1},
        };
        for (int i = 0; i < 4; i++) {
          Coord diag_dir = diag_dirs[i];
          for (int dist = 1;; dist++) {
            Coord diag_coord = {x + dist * diag_dir.x, y + dist * diag_dir.y};
            if (!is_valid_coord(diag_coord))
              break;
            Piece target = get_piece(diag_coord);
            if (target == NULL_PIECE) {
              move_buffer[num_legal_moves++] =
                  create_move(from, diag_coord, moved, target, NULL_PIECE);
            } else {
              if (get_piece_color(target) != BOARD.turn) {
                move_buffer[num_legal_moves++] =
                    create_move(from, diag_coord, moved, target, NULL_PIECE);
              }
              break;
            }
          }
        }
      }

      /* horizontal/vertical handling */
      if (get_piece_type(moved) == ROOK || get_piece_type(moved) == QUEEN) {
        Coord hv_dirs[4] = {
            {-1, 0},
            {1, 0},
            {0, -1},
            {0, 1},
        };
        for (int i = 0; i < 4; i++) {
          Coord hv_dir = hv_dirs[i];
          for (int dist = 1;; dist++) {
            Coord hv_coord = {x + dist * hv_dir.x, y + dist * hv_dir.y};
            if (!is_valid_coord(hv_coord))
              break;
            Piece target = get_piece(hv_coord);
            if (target == NULL_PIECE) {
              move_buffer[num_legal_moves++] =
                  create_move(from, hv_coord, moved, target, NULL_PIECE);
            } else {
              if (get_piece_color(target) != BOARD.turn) {
                move_buffer[num_legal_moves++] =
                    create_move(from, hv_coord, moved, target, NULL_PIECE);
              }
              break;
            }
          }
        }
      }
    }
  }

  return num_legal_moves;
}

void make_move(Move *move) {
  /* Update previous square */
  set_piece(move->from, NULL_PIECE);

  /* Update target square */
  if (get_piece_type(move->moved) == PAWN) {
    /* Update en passant capture */
    if (move->target != NULL_PIECE && get_piece_type(move->target) == PAWN &&
        get_piece(move->to) == NULL_PIECE) {
      set_piece(BOARD.en_passant, NULL_PIECE);
    }

    /* Update en passant status */
    if (move->from.y - move->to.y == (BOARD.turn == BLACK ? -2 : 2)) {
      BOARD.en_passant = move->to;
    } else {
      BOARD.en_passant = NULL_COORD;
    }
  }

  if (move->promotion != NULL_PIECE) {
    set_piece(move->to, move->promotion);
  } else {
    set_piece(move->to, move->moved);
  }

  /* Update turn */
  BOARD.turn = (BOARD.turn == BLACK) ? WHITE : BLACK;
}

void unmake_move(Move *move) {}

/* Interface for JS: */

void init_game() {
  /* Initialize board */
  for (int y = 2; y < 6; y++) {
    for (int x = 0; x < 8; x++) {
      set_piece((Coord){x, y}, NULL_PIECE);
    }
  }

  PieceType bottom_top_pieces[8] = {ROOK, KNIGHT, BISHOP, QUEEN,
                                    KING, BISHOP, KNIGHT, ROOK};

  for (int x = 0; x < 8; x++) {
    set_piece((Coord){x, 0}, create_piece(BLACK, bottom_top_pieces[x]));
    set_piece((Coord){x, 1}, create_piece(BLACK, PAWN));
    set_piece((Coord){x, 6}, create_piece(WHITE, PAWN));
    set_piece((Coord){x, 7}, create_piece(WHITE, bottom_top_pieces[x]));
  }

  set_can_castle(BLACK, LEFT, true);
  set_can_castle(BLACK, RIGHT, true);
  set_can_castle(WHITE, LEFT, true);
  set_can_castle(WHITE, RIGHT, true);

  BOARD.en_passant = NULL_COORD;

  BOARD.turn = WHITE;

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
      make_move(move);
      set_status_msg("Thinking...");
      redraw();
      return;
    }
  }
  set_status_msg("Invalid move");
  redraw();
}
