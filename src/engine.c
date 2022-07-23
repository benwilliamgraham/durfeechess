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
typedef char Piece;

const Piece NULL_PIECE = 0xFF;

typedef enum { WHITE, BLACK } PieceColor;

PieceColor get_piece_color(Piece piece) { return piece & 0b00001000; }

typedef enum { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;

PieceType get_piece_type(Piece piece) { return piece & 0b00000111; }

Piece create_piece(PieceColor color, PieceType type) {
  return (color << 3) | type;
}

typedef struct {
  char x, y;
} Coord;

const Coord NULL_COORD = {-1, -1};

bool is_valid_coord(Coord coord) {
  return coord.x >= 0 && coord.x < 8 && coord.y >= 0 && coord.y < 8;
}

bool choord_eq(Coord a, Coord b) { return a.x == b.x && a.y == b.y; }

Coord coord_add(Coord a, Coord b) { return (Coord){a.x + b.x, a.y + b.y}; }

struct {
  Piece squares[8][8];
  PieceColor turn;
  bool can_castle[2][2];
  Coord en_passant;
} BOARD;

Piece get_piece(Coord coord) { return BOARD.squares[coord.y][coord.x]; }

void set_piece(Coord coord, Piece piece) {
  BOARD.squares[coord.y][coord.x] = piece;
}

typedef enum { LEFT, RIGHT } Side;

bool can_castle(PieceColor color, Side side) {
  return BOARD.can_castle[color][side];
}

void set_can_castle(PieceColor color, Side side, bool can_castle) {
  BOARD.can_castle[color][side] = can_castle;
}

bool can_en_passant() { return BOARD.en_passant.x != -1; }

typedef struct {
  Coord to, from;
  Piece moved, target, promotion;
  bool prev_can_castle;
  Coord prev_en_passant;
} Move;

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
int get_legal_moves(Move *move_buffer) { return 0; }

void make_move(Move *move) {}

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
    set_piece((Coord){x, 0}, create_piece(WHITE, bottom_top_pieces[x]));
    set_piece((Coord){x, 1}, create_piece(WHITE, PAWN));
    set_piece((Coord){x, 6}, create_piece(BLACK, PAWN));
    set_piece((Coord){x, 7}, create_piece(BLACK, bottom_top_pieces[x]));
  }

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
