#include "bits.h"
#include "movegen.h"
#include "position.h"
#include <string.h>

// note: square must be None
static inline
void set_square(struct Position *pos, square sq, enum PieceType T) {
	ASSERT(sq < 64 && "invalid square");

	pos->X |= (bitboard)((T >> 0) & 1) << sq;
	pos->Y |= (bitboard)((T >> 1) & 1) << sq;
	pos->Z |= (bitboard)((T >> 2) & 1) << sq;
}

struct Position make_move(struct Position pos, struct Move move) {
	bitboard occ = occupied(pos);
	bitboard info = pext(extract(pos, Info), ~occ);
	bitboard ep_mask = (info & EP_MASK) << 40;

	enum { A1 = 0, E1 = 4, H1 = 7, A8 = 56, H8 = 63 };

	// construct clear mask
	bitboard clear = ~occ; // clear all info to replace with new info
	clear |= 1ULL << move.start;
	clear |= 1ULL << move.end;

	// remove captured en-passant pawn
	if (move.piece == Pawn)
		clear |= shift(S, ep_mask & (1ULL << move.end));

	// remove castling rook
	if (move.castling)
		clear |= (move.end < move.start) ? (1 << A1) : (1 << H1);

	// clear bits
	pos.white &= ~clear;
	pos.X &= ~clear;
	pos.Y &= ~clear;
	pos.Z &= ~clear;

	// set moved pieces
	pos.white |= 1ULL << move.end;
	set_square(&pos, move.end, move.piece);

	// set castled rook
	if (move.castling) {
		square mid = (move.start + move.end) >> 1;
		set_square(&pos, mid, Rook);

		pos.white |= 1ULL << mid;
	}

	// update castling rights
	if (move.piece == King)
		info &= ~(WK_MASK | WQ_MASK);

	if (move.start == A1) info &= ~WQ_MASK;
	if (move.start == H1) info &= ~WK_MASK;
	if (move.end   == A8) info &= ~BQ_MASK;
	if (move.end   == H8) info &= ~BK_MASK;

	// clear en passant square and swap white and black castling rights
	info &= ~EP_MASK;
	info  = ((info << 2) | (info >> 2)) & CA_MASK;

	// rotate board
	pos = rotate(pos);

	// update new en-passant square
	if (move.piece == Pawn && move.end - move.start == N+N)
		info |= 1 << (move.start & 7);

	// write info bits
	occ = occupied(pos);
	info = pdep(info, ~occ);

	pos.X |= info;
	pos.Y |= info;
	pos.Z |= info;

	// swap white & black bitboards
	pos.white = occ & ~pos.white;

	return pos;
}
