#include "game.h"
#include "interface.h"

/* Global constants */
const Coord NULL_COORD = {-1, -1};

const Piece NULL_PIECE = -1;

/* Internal constants */
const int8_t BACK_ROWS[] = {0, 7};

const int8_t PAWN_START_ROWS[] = {1, 6};

const int8_t FWD_DIRS[] = {1, -1};

const int8_t ROOK_START_COLS[] = {0, 7};

const int8_t ROOK_CASTLE_COLS[] = {3, 5};

const int8_t KING_START_COL = 4;

const int8_t KING_CASTLE_COLS[] = {2, 6};

const Coord PIECE_OFFSETS[] = {
    /* Knight */
    {-2, -1},
    {-2, 1},
    {-1, -2},
    {-1, 2},
    {1, -2},
    {1, 2},
    {2, -1},
    {2, 1},
    /* Diagonals */
    {-1, -1},
    {1, -1},
    {-1, 1},
    {1, 1},
    /* Horizontal */
    {-1, 0},
    {1, 0},
    /* Vertical */
    {0, -1},
    {0, 1},
};

const Coord *KNIGHT_OFFSETS = PIECE_OFFSETS,
            *KNIGHT_OFFSETS_END = PIECE_OFFSETS + 8;

const Coord *LINEAR_OFFSETS = PIECE_OFFSETS + 8,
            *LINEAR_OFFSETS_END = PIECE_OFFSETS + 16;

const Coord *DIAG_OFFSETS = PIECE_OFFSETS + 8,
            *DIAG_OFFSETS_END = PIECE_OFFSETS + 12;

const Coord *HV_OFFSETS = PIECE_OFFSETS + 12,
            *HV_OFFSETS_END = PIECE_OFFSETS + 16;

const PieceType PROMOTION_TYPES[] = {QUEEN, ROOK, BISHOP, KNIGHT};

const PieceType *PROMOTION_TYPES_END = PROMOTION_TYPES + 4;

/* Piece implementation
 *  Piece layout:
 *  ____CTTT
 *  C: color
 *  T: type
 */

PieceColor get_piece_color(Piece piece) { return (piece >> 3) & 0b1; }

PieceColor get_opposite_color(PieceColor color) {
  return color == BLACK ? WHITE : BLACK;
}

PieceType get_piece_type(Piece piece) { return piece & 0b111; }

Piece create_piece(PieceColor color, PieceType type) {
  return (color << 3) | type;
}

/* Coord implementation */

bool is_valid_coord(Coord coord) {
  return coord.x >= 0 && coord.x < BOARD_SIZE && coord.y >= 0 &&
         coord.y < BOARD_SIZE;
}

bool choord_eq(Coord a, Coord b) { return a.x == b.x && a.y == b.y; }

/* Board implementation */

Piece get_piece(Coord coord) { return BOARD.squares[coord.y][coord.x]; }

void set_piece(Coord coord, Piece piece) {
  BOARD.squares[coord.y][coord.x] = piece;
}

bool can_castle(PieceColor color, Side side) {
  return BOARD.can_castle.status[color][side];
}

void set_can_castle(PieceColor color, Side side, bool can_castle) {
  BOARD.can_castle.status[color][side] = can_castle;
}

bool can_en_passant() { return BOARD.en_passant.x != -1; }

/* Move implementation */

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

/* Game implementation */

void init_board() {
  /* Initialize board */
  for (int y = 2; y < BOARD_SIZE - 2; y++) {
    for (int x = 0; x < BOARD_SIZE; x++) {
      set_piece((Coord){x, y}, NULL_PIECE);
    }
  }

  PieceType bottom_top_pieces[] = {ROOK, KNIGHT, BISHOP, QUEEN,
                                   KING, BISHOP, KNIGHT, ROOK};

  for (int x = 0; x < BOARD_SIZE; x++) {
    set_piece((Coord){x, 0}, create_piece(BLACK, bottom_top_pieces[x]));
    set_piece((Coord){x, PAWN_START_ROWS[BLACK]}, create_piece(BLACK, PAWN));
    set_piece((Coord){x, PAWN_START_ROWS[WHITE]}, create_piece(WHITE, PAWN));
    set_piece((Coord){x, BOARD_SIZE - 1},
              create_piece(WHITE, bottom_top_pieces[x]));
  }

  set_can_castle(BLACK, QUEEN_SIDE, true);
  set_can_castle(BLACK, KING_SIDE, true);
  set_can_castle(WHITE, QUEEN_SIDE, true);
  set_can_castle(WHITE, KING_SIDE, true);

  BOARD.king_pos[BLACK] = (Coord){KING_START_COL, BACK_ROWS[BLACK]};
  BOARD.king_pos[WHITE] = (Coord){KING_START_COL, BACK_ROWS[WHITE]};

  BOARD.en_passant = NULL_COORD;

  BOARD.turn = WHITE;
};

bool is_square_attacked(Coord coord, PieceColor attack_color) {
  PieceColor under_attack_color = get_opposite_color(attack_color);
  // Check pawn attack
  Coord pawn_attack_coords[] = {
      {coord.x - 1, coord.y + FWD_DIRS[under_attack_color]},
      {coord.x + 1, coord.y + FWD_DIRS[under_attack_color]},
  };
  for (int i = 0; i < 2; i++) {
    if (is_valid_coord(pawn_attack_coords[i]) &&
        get_piece(pawn_attack_coords[i]) == create_piece(attack_color, PAWN)) {
      return true;
    }
  }

  // Check knight attack
  for (const Coord *offset = KNIGHT_OFFSETS; offset < KNIGHT_OFFSETS_END;
       offset++) {
    Coord knight_coord = (Coord){coord.x + offset->x, coord.y + offset->y};
    if (is_valid_coord(knight_coord) &&
        get_piece(knight_coord) == create_piece(attack_color, KNIGHT)) {
      return true;
    }
  }

  // Check linear attack
  for (const Coord *offset = LINEAR_OFFSETS; offset < LINEAR_OFFSETS_END;
       offset++) {
    for (int dist = 1;; dist++) {
      Coord square =
          (Coord){coord.x + offset->x * dist, coord.y + offset->y * dist};
      if (!is_valid_coord(square)) {
        break;
      }
      Piece piece = get_piece(square);
      if (piece != NULL_PIECE) {
        if (get_piece_color(piece) == attack_color &&
            (
                /* Bishop */
                (get_piece_type(piece) == BISHOP && offset >= DIAG_OFFSETS &&
                 offset < DIAG_OFFSETS_END) ||
                /* Rook */
                (get_piece_type(piece) == ROOK && offset >= HV_OFFSETS &&
                 offset < HV_OFFSETS_END) ||
                /* Queen */
                get_piece_type(piece) == QUEEN ||
                /* King */
                (get_piece_type(piece) == KING && dist == 1))) {
          return true;
        }
        break;
      }
    }
  }

  return false;
}

int get_legal_moves(Move *move_buffer) {
  int num_legal_moves = 0;
  /* Iterate over squares */
  for (int y = 0; y < BOARD_SIZE; y++) {
    for (int x = 0; x < BOARD_SIZE; x++) {
      Coord from = {x, y};
      Piece moved = get_piece(from);
      if (moved == NULL_PIECE || get_piece_color(moved) != BOARD.turn)
        continue;

      /* Pawn handling */
      if (get_piece_type(moved) == PAWN) {
        int forward = FWD_DIRS[get_piece_color(moved)];

        /* Single square forward */
        Coord single_fwd = {x, y + forward};
        if (is_valid_coord(single_fwd) && get_piece(single_fwd) == NULL_PIECE) {
          /* Check for promotion */
          if (y == PAWN_START_ROWS[get_opposite_color(BOARD.turn)]) {
            for (const PieceType *promotion_type = PROMOTION_TYPES;
                 promotion_type < PROMOTION_TYPES_END; promotion_type++) {
              move_buffer[num_legal_moves++] =
                  create_move(from, single_fwd, moved, NULL_PIECE,
                              create_piece(BOARD.turn, *promotion_type));
            }
          } else {
            move_buffer[num_legal_moves++] =
                create_move(from, single_fwd, moved, NULL_PIECE, NULL_PIECE);
          }
        }

        /* Double square forward */
        Coord double_fwd = {x, y + 2 * forward};
        if ((y == PAWN_START_ROWS[BOARD.turn]) &&
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
            if (y == PAWN_START_ROWS[get_opposite_color(BOARD.turn)]) {
              for (const PieceType *promotion_type = PROMOTION_TYPES;
                   promotion_type < PROMOTION_TYPES_END; promotion_type++) {
                move_buffer[num_legal_moves++] = create_move(
                    from, capture_coord, moved, get_piece(capture_coord),
                    create_piece(BOARD.turn, *promotion_type));
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
        for (const Coord *knight_move = KNIGHT_OFFSETS;
             knight_move < KNIGHT_OFFSETS_END; knight_move++) {
          Coord knight_coord = {x + knight_move->x, y + knight_move->y};
          if (is_valid_coord(knight_coord)) {
            Piece target = get_piece(knight_coord);
            if (target == NULL_PIECE || get_piece_color(target) != BOARD.turn) {
              move_buffer[num_legal_moves++] =
                  create_move(from, knight_coord, moved, target, NULL_PIECE);
            }
          }
        }
      }

      /* Linear movement handling */
      else {
        const Coord *start_coord =
            get_piece_type(moved) == ROOK ? HV_OFFSETS : LINEAR_OFFSETS;
        const Coord *end_coord = get_piece_type(moved) == BISHOP
                                     ? DIAG_OFFSETS_END
                                     : LINEAR_OFFSETS_END;
        for (const Coord *dir = start_coord; dir < end_coord; dir++) {
          for (int dist = 1;; dist++) {
            Coord to = {x + dist * dir->x, y + dist * dir->y};
            if (!is_valid_coord(to))
              break;
            Piece target = get_piece(to);
            if (target == NULL_PIECE) {
              move_buffer[num_legal_moves++] =
                  create_move(from, to, moved, target, NULL_PIECE);
            } else {
              if (get_piece_color(target) != BOARD.turn) {
                move_buffer[num_legal_moves++] =
                    create_move(from, to, moved, target, NULL_PIECE);
              }
              break;
            }
            if (get_piece_type(moved) == KING)
              break;
          }
        }
      }

      /* Castling */
      if (get_piece_type(moved) == KING) {
        if (can_castle(BOARD.turn, QUEEN_SIDE)) {
          /* Check that all of the squares in between are empty */
          for (int i = ROOK_START_COLS[QUEEN_SIDE] + 1; i < x; i++) {
            if (get_piece((Coord){i, y}) != NULL_PIECE)
              goto cant_castle_queen_side;
          }
          /* Check that the king isn't in check and won't go into check */
          for (int i = KING_CASTLE_COLS[QUEEN_SIDE]; i <= x; i++) {
            if (is_square_attacked((Coord){i, y},
                                   get_opposite_color(BOARD.turn)))
              goto cant_castle_queen_side;
          }
          move_buffer[num_legal_moves++] =
              create_move(from, (Coord){KING_CASTLE_COLS[QUEEN_SIDE], y}, moved,
                          NULL_PIECE, NULL_PIECE);
        cant_castle_queen_side:;
        }

        if (can_castle(BOARD.turn, KING_SIDE)) {
          /* Check that all of the squares in between are empty */
          for (int i = x + 1; i < ROOK_START_COLS[KING_SIDE]; i++) {
            if (get_piece((Coord){i, y}) != NULL_PIECE)
              goto cant_castle_king_side;
          }
          /* Check that the king isn't in check and won't go into check */
          for (int i = x; x <= KING_CASTLE_COLS[KING_SIDE]; i++) {
            if (is_square_attacked((Coord){i, y},
                                   get_opposite_color(BOARD.turn)))
              goto cant_castle_king_side;
          }
          move_buffer[num_legal_moves++] =
              create_move(from, (Coord){KING_CASTLE_COLS[KING_SIDE], y}, moved,
                          NULL_PIECE, NULL_PIECE);
        cant_castle_king_side:;
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

  /* Check for castling */
  if (get_piece_type(move->moved) == KING) {
    BOARD.king_pos[BOARD.turn] = move->to;
    set_can_castle(BOARD.turn, QUEEN_SIDE, false);
    set_can_castle(BOARD.turn, KING_SIDE, false);

    if (move->from.x == KING_START_COL) {
      /* Move rook */
      for (Side side = QUEEN_SIDE; side <= KING_SIDE; side++) {
        if (move->to.x == KING_CASTLE_COLS[side]) {
          set_piece((Coord){ROOK_START_COLS[side], move->from.y}, NULL_PIECE);
          set_piece((Coord){ROOK_CASTLE_COLS[side], move->from.y},
                    create_piece(BOARD.turn, ROOK));
        }
      }
    }
  }
  if (get_piece_type(move->moved) == ROOK) {
    if (move->from.x == ROOK_START_COLS[QUEEN_SIDE] &&
        move->from.y == BACK_ROWS[BOARD.turn]) {
      set_can_castle(BOARD.turn, QUEEN_SIDE, false);
    } else if (move->from.x == ROOK_START_COLS[KING_SIDE] &&
               move->from.y == BACK_ROWS[BOARD.turn]) {
      set_can_castle(BOARD.turn, KING_SIDE, false);
    }
  }

  /* Update turn */
  BOARD.turn = (BOARD.turn == BLACK) ? WHITE : BLACK;
}

void unmake_move(Move *move) {}
