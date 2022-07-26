#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

typedef int8_t Piece;

extern const Piece NULL_PIECE;

typedef enum { BLACK, WHITE } PieceColor;

PieceColor get_piece_color(Piece piece);

PieceColor get_opposite_color(PieceColor color);

typedef enum { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;

PieceType get_piece_type(Piece piece);

Piece create_piece(PieceColor color, PieceType type);

typedef struct {
  int8_t x, y;
} Coord;

extern const Coord NULL_COORD;

bool is_valid_coord(Coord coord);

bool choord_eq(Coord a, Coord b);

typedef struct {
  bool status[2][2];
} CastleStatus;

#define BOARD_SIZE 8

extern struct {
  Piece squares[BOARD_SIZE][BOARD_SIZE];
  PieceColor turn;
  CastleStatus can_castle;
  Coord en_passant;
  Coord king_pos[2];
} BOARD;

void init_board();

Piece get_piece(Coord coord);

void set_piece(Coord coord, Piece piece);

typedef enum { QUEEN_SIDE, KING_SIDE } Side;

bool can_castle(PieceColor color, Side side);

void set_can_castle(PieceColor color, Side side, bool can_castle);

bool can_en_passant();

typedef struct {
  Coord from, to;
  Piece moved, target, promotion;
  CastleStatus prev_can_castle;
  Coord prev_en_passant;
} Move;

Move create_move(Coord from, Coord to, Piece moved, Piece target,
                 Piece promotion);

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
#define MAX_MOVES 137

/* Note: this function does not check for en passant. */
bool is_square_attacked(Coord coord, PieceColor attack_color);

int get_legal_moves(Move *move_buffer);

void make_move(Move *move);

void unmake_move(Move *move);

#endif
