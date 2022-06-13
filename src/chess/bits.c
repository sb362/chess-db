#include "bits.h"

#include <stddef.h>

bitboard attacks[107648];
struct bitbase bitbase[64];

bitboard diagonal(uint8_t n) {
	ASSERT(n < 15 && "only 15 diagonals");

	return (n < 8) ? 0x0102040810204080 >> (8*(7-n))
	               : 0x0102040810204080 << (8*(n-7));
}

bitboard sliding_attacks(square sq, bitboard mask, bitboard occ) {
	occ &= mask;

	bitboard low = occ & ((1ULL << sq) - 1);
	bitboard high = occ & ~low;

	low = 0x8000000000000000 >> (msb(low | 1) ^ 63u);
	return mask & (high ^ (high - low)) & ~(1ULL << sq);
}

void init_bitbase() {
	size_t index = 0;

	for (square sq = 0; sq < 64; sq++) {
		bitboard bit = 1ULL << sq;

		bitbase[sq].knight = shift(N,N,E, bit)
		                   | shift(E,N,E, bit)
				   | shift(E,S,E, bit)
				   | shift(S,S,E, bit)
				   | shift(S,S,W, bit)
				   | shift(W,S,W, bit)
				   | shift(W,N,W, bit)
				   | shift(N,N,W, bit);

		bitbase[sq].king = shift(N, bit)
		                 | shift(E, bit)
				 | shift(S, bit)
				 | shift(W, bit)
				 | shift(N,E, bit)
				 | shift(S,E, bit)
				 | shift(S,W, bit)
				 | shift(N,W, bit);

		square rank = sq >> 3;
		square file = sq & 7;

		// bishop sliders
		{
			bitboard mask1 = diagonal(rank + file);
			bitboard mask2 = reverse(diagonal(7-rank + file));

			// remove outer and square bits
			bitboard mask = (mask1 | mask2) & ~(RANK1 | RANK8 | AFILE | HFILE | bit);
			bitboard occ = 0;

			bitbase[sq].mask[0] = mask;
			bitbase[sq].attacks[0] = attacks + index;

			// carry-rippler iterator
			do {
				attacks[index++] = sliding_attacks(sq, mask1, occ)
				                 | sliding_attacks(sq, mask2, occ);
				occ = (occ - mask) & mask;
			} while (occ);
		}

		// rook sliders
		{
			bitboard mask1 = RANK1 << (8*rank);
			bitboard mask2 = AFILE << file;

			// remove outer and square bits
			bitboard mask = ((mask1 & ~AFILE & ~HFILE)
			              |  (mask2 & ~RANK1 & ~RANK8)) & ~bit;

			bitboard occ = 0;

			bitbase[sq].mask[1] = mask;
			bitbase[sq].attacks[1] = attacks + index;

			// carry-rippler iterator
			do {
				attacks[index++] = sliding_attacks(sq, mask1, occ)
				                 | sliding_attacks(sq, mask2, occ);
				occ = (occ - mask) & mask;
			} while (occ);
		}
	}
}
