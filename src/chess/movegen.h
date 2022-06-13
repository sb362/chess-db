#ifndef MOVEGEN_H_
#define MOVEGEN_H_

#include <stddef.h>
#include <stdint.h>

#include "bits.h"
#include "position.h"

#define MAX_MOVELIST_LENGTH 256

struct Move {
	uint16_t start: 6, end: 6, piece: 3, castling: 1;
};

struct MoveList {
	struct Move moves[MAX_MOVELIST_LENGTH];
	size_t length;
};

struct MoveList generate_moves(struct Position pos);
struct Position make_move(struct Position pos, struct Move move);

bitboard enemy_checks(struct Position pos);

static inline
bitboard generic_attacks(enum PieceType T, square sq, bitboard occ) {
	switch (T) {
		case Knight: return knight_attacks(sq);
		case Bishop: return bishop_attacks(sq, occ);
		case Rook:   return rook_attacks(sq, occ);
		case Queen:  return queen_attacks(sq, occ);
		case King:   return king_attacks(sq);
		default:     ASSERT(0 && "unreachable");
		             return 0;
	}
}

#endif /*MOVEGEN_H_*/
