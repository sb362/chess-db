
#include "chess/bitboard.hh"
#include "util/bits.hh"

#include <array>

using namespace cdb::chess;

struct Bitbase {
  bitboard king, knight;

  struct {
    bitboard mask;
    size_t index;
  } bishop, rook;
};

constexpr bitboard diagonal(uint8_t n) {
  return (n < 8) ? 0x0102040810204080 >> (8 * (7 - n))
                 : 0x0102040810204080 << (8 * (n - 7));
}

constexpr bitboard sliding_attacks(uint8_t sq, bitboard mask, bitboard occ) {
  occ &= mask;

  bitboard s = 1ull << sq;
  bitboard lo = occ & (s - 1);
  bitboard hi = occ & ~lo;

  lo = 0x8000000000000000 >> cdb::msb(lo | 1);
  return mask & (hi ^ (hi - lo)) & ~s;
}

static constexpr auto bitbase = [] () constexpr {
  std::array<Bitbase, 64> bitbase;
  std::array<bitboard, 107648> attacks;
  std::size_t index = 0;

  for (uint8_t sq = 0; sq < 64; ++sq) {
    const bitboard s = 1ull << sq;
    const uint8_t rank = sq >> 3, file = sq & 7;

    { // king attacks
      bitbase[sq].king  = shiftm<West,  East> (s) | s;
      bitbase[sq].king |= shiftm<North, South>(bitbase[sq].king);
    }

    { // knight attacks
      bitbase[sq].knight = walk<North, NorthEast>(s)
                         | walk<North, NorthWest>(s)
                         | walk<East,  NorthEast>(s)
                         | walk<East,  SouthEast>(s)
                         | walk<West,  NorthWest>(s)
                         | walk<West,  SouthWest>(s)
                         | walk<South, SouthEast>(s)
                         | walk<South, SouthWest>(s);
    }

    auto carry_rippler = [sq, &index, &attacks] (bitboard mask, bitboard mask1, bitboard mask2) {
      bitboard occ = 0;
      do {
        attacks[index++] = sliding_attacks(sq, mask1, occ) | sliding_attacks(sq, mask2, occ);
        occ = (occ - mask) & mask;
      } while (occ);
    };

    { // bishop attacks
      const bitboard edges = RANK_1 | RANK_8 | FILE_A | FILE_H;
      const bitboard mask1 =               diagonal(file + rank);
      const bitboard mask2 = cdb::byteswap(diagonal(file + 7 - rank));
      const bitboard mask  = (mask1 | mask2) & ~(edges | s);

      bitbase[sq].bishop.index = index;
      bitbase[sq].bishop.mask  = mask;

      carry_rippler(mask, mask1, mask2);
    }

    { // rook attacks
      const bitboard mask1 = RANK_1 << (8 * rank);
      const bitboard mask2 = FILE_A << file;
      const bitboard mask  = ((mask1 & ~(FILE_A | FILE_H))
                            | (mask2 & ~(RANK_1 | RANK_8))) & ~s;

      bitbase[sq].rook.index = index;
      bitbase[sq].rook.mask  = mask;

      carry_rippler(mask, mask1, mask2);
    }
  }

  return std::make_pair(bitbase, attacks);
} ();

bitboard cdb::chess::attacks_from(PieceType piece_type, Square sq, bitboard occ)
{
  using enum PieceType;
  switch (piece_type) {
    case Knight: return bitbase.first[sq].knight;
    case King:   return bitbase.first[sq].king;
    case Bishop:
      return bitbase.second[cdb::pext(occ, bitbase.first[sq].bishop.mask)
                          + bitbase.first[sq].bishop.index];
    case Rook:
      return bitbase.second[cdb::pext(occ, bitbase.first[sq].rook.mask)
                          + bitbase.first[sq].rook.index];
    case Queen:  return attacks_from(Bishop, sq, occ) | attacks_from(Rook, sq, occ);
    default:
      return 0;
  }
}
