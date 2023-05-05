#pragma once

#include "chess/bitboard.hh"
#include "core/error.hh"
#include "util/bits.hh"

#include <string>
#include <string_view>

namespace cdb::chess {

struct Position {
  bitboard x, y, z, white;

  constexpr bitboard occupied() const { return x | y | z; }
  constexpr bitboard extract(PieceType piece_type) const {
    if (piece_type == PieceType::Rook) return z & ~y;

    const int pt = static_cast<int>(piece_type);
    bitboard bb = 0;
    bb  = (pt & 1) ? x : ~x;
    bb &= (pt & 2) ? y : ~y;
    bb &= (pt & 4) ? z : ~z;

    return bb;
  }

  constexpr void set(Square square, PieceType piece_type) {
    const auto pt = static_cast<bitboard>(piece_type);
    const auto sq = static_cast<int>(square);

    x |= ((pt >> 0) & 1) << sq;
    y |= ((pt >> 1) & 1) << sq;
    z |= ((pt >> 2) & 1) << sq;
  }

  constexpr PieceType on(Square sq) const {
    unsigned pt = 0;

    pt |= ((x >> sq) & 1) << 0;
    pt |= ((y >> sq) & 1) << 1;
    pt |= ((z >> sq) & 1) << 2;

    const auto piece_type = static_cast<PieceType>(pt);
    return piece_type == PieceType::Castle ? PieceType::Rook : piece_type;
  }

  constexpr Position rotated() const {
    return {byteswap(x), byteswap(y), byteswap(z), byteswap(white)};
  }

  constexpr bool operator!=(const Position &pos) const { return *this == pos; }
  constexpr bool operator==(const Position &pos) const {
    return x == pos.x && y == pos.y && z == pos.z && white == pos.white;
  }

  static Result<Position> from_fen(std::string_view fen);
  std::string to_fen(bool black) const;

#ifdef CHESS_DEBUG_POS
  std::string fen = "<empty>";
#endif
};

constexpr Position startpos {
  .x = 0xb5ff00000000ffb5,
  .y = 0x7e0000000000007e,
  .z = 0x9900000000000099,
  .white = 0xffff
};

} // cdb::chess
