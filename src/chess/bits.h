#ifndef BITS_H_
#define BITS_H_

#include <stdint.h>
#include <string.h>

#include "util.h"

typedef uint8_t square;
typedef uint64_t bitboard;

enum { N = 8, E = 1, S = -N, W = -E };

static const bitboard AFILE = 0x0101010101010101;
static const bitboard HFILE = 0x8080808080808080;
static const bitboard RANK1 = 0x00000000000000ff;
static const bitboard RANK3 = 0x0000000000ff0000;
static const bitboard RANK8 = 0xff00000000000000;

static inline bitboard shiftN(bitboard bb) { return bb << 8; }
static inline bitboard shiftS(bitboard bb) { return bb >> 8; }
static inline bitboard shiftE(bitboard bb) { return (bb & ~HFILE) << 1; }
static inline bitboard shiftW(bitboard bb) { return (bb & ~AFILE) >> 1; }

#define shift1(x,bb) shift##x(bb)
#define shift2(x,y,bb) shift##x(shift##y(bb))
#define shift3(x,y,z,bb) shift##x(shift##y(shift##z(bb)))

#define _nargs(_1,_2,_3,_4,_N,...) _N
#define nargs(...) _nargs(__VA_ARGS__,3,2,1,0)

#define __shift(n,...) shift##n(__VA_ARGS__)
#define _shift(n,...) __shift(n, __VA_ARGS__)
#define shift(...) _shift(nargs(__VA_ARGS__), __VA_ARGS__)

#define reverse(bb) bswap(bb)

struct bitbase {
	bitboard knight, king, mask[2], *attacks[2];
};

extern struct bitbase bitbase[64];
void init_bitbase();


static inline bitboard knight_attacks(square sq) {
	ASSERT(sq < 64 && "invalid square");
	return bitbase[sq].knight;
}

static inline bitboard bishop_attacks(square sq, bitboard occ) {
	ASSERT(sq < 64 && "invalid square");
	return bitbase[sq].attacks[0][pext(occ, bitbase[sq].mask[0])];
}

static inline bitboard rook_attacks(square sq, bitboard occ) {
	ASSERT(sq < 64 && "invalid square");
	return bitbase[sq].attacks[1][pext(occ, bitbase[sq].mask[1])];
}

static inline bitboard queen_attacks(square sq, bitboard occ) {
	ASSERT(sq < 64 && "invalid square");
	return bishop_attacks(sq, occ) | rook_attacks(sq, occ);
}

static inline bitboard king_attacks(square sq) {
	ASSERT(sq < 64 && "invalid square");
	return bitbase[sq].king;
}

#endif /*BITS_H_*/
