#ifndef POSITION_H_
#define POSITION_H_

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

#endif /*POSITION_H_*/
