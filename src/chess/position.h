#pragma once

#include "bits.h"

enum PieceType {
	None, Pawn, Knight, Bishop, Rook, Queen, King, Info
};

struct Position {
	bitboard white, X,Y,Z;
};

enum PositionInfo {
	EP_MASK = 0x0ff,
	WK_MASK = 0x100,
	WQ_MASK = 0x200,
	BK_MASK = 0x400,
	BQ_MASK = 0x800,
	CA_MASK = 0xf00,
};

static inline bitboard extract(struct Position pos, enum PieceType T) {
	bitboard bb;
	bb  = (T & 1) ? pos.X : ~pos.X;
	bb &= (T & 2) ? pos.Y : ~pos.Y;
	bb &= (T & 4) ? pos.Z : ~pos.Z;
	return bb;
}

static inline bitboard occupied(struct Position pos) {
	return (pos.X ^ pos.Y) | (pos.X ^ pos.Z);
}

static inline bitboard extract_info(struct Position pos) {
	return pext(pos.X & pos.Y & pos.Z, ~occupied(pos));
}

static inline enum PieceType get_piece(struct Position pos, int square) {
	ASSERT(0 <= square && square < 64);

	int T = 0;

	T |= ((pos.X >> square) & 1) << 0;
	T |= ((pos.Y >> square) & 1) << 1;
	T |= ((pos.Z >> square) & 1) << 2;

	return (enum PieceType) T;
}

static inline
bitboard generic_attacks(enum PieceType T, square sq, bitboard occ) {
	switch (T) {
		case Knight: return knight_attacks(sq);
		case Bishop: return bishop_attacks(sq, occ);
		case Rook:   return rook_attacks(sq, occ);
		case Queen:  return queen_attacks(sq, occ);
		case King:   return king_attacks(sq);
		default:     return 0;
	}
}

static inline struct Position startpos()
{
	struct Position pos = {
		.white = 0x000000000000ffff,
		.X     = 0x2cff00000000ff2c,
		.Y     = 0x7600000000000076,
		.Z     = 0x9900000000000099
	};

	bitboard mask = WK_MASK | WQ_MASK | BK_MASK | BQ_MASK;
	mask = pdep(mask, ~occupied(pos));

	pos.X |= mask;
	pos.Y |= mask;
	pos.Z |= mask;

	return pos;
}
