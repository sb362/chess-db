#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#include "util.h"

#define MAX_MOVELIST_LENGTH 256

typedef uint64_t bitboard;

enum PieceType {
	None, Pawn, Knight, Bishop, Rook, Queen, King, Info,
};

enum PositionInfo {
	EP_MASK = 0x0ff,  // 8-bit en passant mask (file)
	WK_MASK = 0x100,  // castling masks
	WQ_MASK = 0x200,
	BK_MASK = 0x400,
	BQ_MASK = 0x800,
};

struct Position {
	bitboard white, X, Y, Z;
};

enum Color { WHITE, BLACK };

struct PositionState {
	struct Position pos;
	enum Color side_to_move;

	unsigned fifty_move_clock;
	unsigned movenumber;
};

struct Move {
	uint16_t start: 6, end: 6, piece: 3, castling: 1;
};

// forsyths edwards notation
struct PositionState parse_fen(const char *fen, bool *ok, FILE *stream);
size_t generate_fen(struct PositionState state, char *buffer);

// standard algebraic notation
size_t generate_san(struct Move move, struct PositionState state, char *buffer, bool check_and_mate);
struct Move parse_san(const char *in, size_t len, struct PositionState state, bool *ok);

// universal chess interface notation
size_t generate_uci(struct Move move, struct PositionState state, char *buffer);

// move generation
struct MoveList {
	struct Move moves[MAX_MOVELIST_LENGTH];
	size_t length;
};

// initialises tables used for move generation
void init_bitbase();

struct MoveList generate_moves(struct Position pos);
struct Position make_move(struct Position pos, struct Move move);
bitboard enemy_checks(struct Position pos);

// inline functions
static inline
enum PieceType get_piece(struct Position pos, int square) {
	ASSERT(0 <= square && square < 64);
	int T = 0;

	T |= ((pos.X >> square) & 1) << 0;
	T |= ((pos.Y >> square) & 1) << 1;
	T |= ((pos.Z >> square) & 1) << 2;

	return (enum PieceType) T;
}

static inline
bitboard extract_bitboard(struct Position pos, enum PieceType T) {
	ASSERT(T != Info && "use extract info instead");

	bitboard bb;
	bb  = (T & 1) ? pos.X : ~pos.X;
	bb &= (T & 2) ? pos.Y : ~pos.Y;
	bb &= (T & 4) ? pos.Z : ~pos.Z;
	return bb;
}

static inline
bitboard extract_info(struct Position pos) {
	bitboard occ = (pos.X ^ pos.Y) | (pos.X ^ pos.Z);
	bitboard info = pos.X & pos.Y & pos.Z;
	return pext(info, ~occ);
}

static inline
bitboard occupied(struct Position pos) {
	return (pos.X ^ pos.Y) | (pos.X ^ pos.Z);
}

static inline
struct PositionState do_move(struct PositionState state, struct Move move)
{
	state.pos = make_move(state.pos, move);
	state.fifty_move_clock += 0/* todo */;
	state.movenumber += 1;
	state.side_to_move = state.side_to_move == BLACK ? WHITE : BLACK;

	return state;
}

static inline
unsigned fullmove_number(struct PositionState state)
{
	return (state.movenumber - (state.side_to_move == BLACK)) / 2;
}

static const char *StartposFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

static const struct Position Startpos = {
	.white = 0x000000000000ffff,
	.X     = 0x2cff00000f00ff2c,
	.Y     = 0x760000000f000076,
	.Z     = 0x990000000f000099
};

#if defined(__cplusplus)
} // extern "C"
#endif
