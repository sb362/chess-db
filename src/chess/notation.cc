#include "notation.hh"

#include "chess/bitboard.hh"
#include "movegen.hh"
#include "position.hh"

#include <iostream>
#include <format>

using namespace cdb;
using namespace chess;

constexpr std::string_view PieceChars = "/PNBR/QK";

std::optional<Move> chess::parse_san(std::string_view san, Position pos, bool black) {
  using enum PieceType;

  if (black) {
    pos.white = byteswap(pos.occupied() & ~pos.white);
    pos.x = byteswap(pos.x);
    pos.y = byteswap(pos.y);
    pos.z = byteswap(pos.z);
  }

  unsigned i = 0;
  char c = san[i++];

  // pawn moves
  if ('a' <= c && c <= 'h') {
    const auto f = (FILE_A << (c - 'a'));
    auto piece_type = Pawn;
    bitboard srcs = pos.white & pos.extract(Pawn) & f;
    bitboard targets = ~pos.white;

    c = san[i++];
  
    if ('1' <= c & c <= '8') {
      const auto r = RANK_1 << (8 * (c - '1'));
      targets &= r;
      targets &= f;

    // captures
    } else if (c == 'x') {
      c = san[i++];
      assert('a' <= c && c <= 'h');
      targets &= (FILE_A << (c - 'a'));
      
      c = san[i++];
      assert('1' <= c && c <= '8');
      targets &= (RANK_1 << (8 * (c - '1')));

      srcs &= shiftm<SouthWest, SouthEast>(targets);

    } else {
      assert(false);
    }

    // promotion
    if (i < san.size() - 1 && san[i++] == '=') {
      size_t idx = PieceChars.find(san[i]);
      assert(idx != std::string_view::npos);
      piece_type = static_cast<PieceType>(idx);
    }

    std::cout << std::format("{:x} {:x}\n", srcs, targets);

    assert(only_one(srcs)); // todo: remove asserts, return nullopt instead
    assert(only_one(targets));
    const auto src = static_cast<Square>(lsb(srcs));
    const auto dst = static_cast<Square>(lsb(targets));

    return Move {src, dst, piece_type, false};
  // non-pawn moves
  } else if (size_t idx = PieceChars.find(c); idx != std::string_view::npos) {
    const auto piece_type = static_cast<PieceType>(idx);
    bitboard srcs = pos.white & pos.extract(piece_type), tmp = ~0ull;
    bitboard targets = ~pos.white;

    if (c = san[i]; 'a' <= c && c <= 'h') {
      tmp &= FILE_A << (c - 'a'); // todo: move this into file_bb
      ++i;
    }

    if (c = san[i]; '1' <= c && c <= '8') {
      tmp &= RANK_1 << (8 * (c - '1')); // todo: move this into rank_bb
      ++i;
    }

    if (c = san[i]; c == 'x') {
      targets &= pos.occupied();
      ++i;
    }

    if (c = san[i]; 'a' <= c && c <= 'h') {
      srcs &= tmp;
      targets &= FILE_A << (c - 'a'); // todo: move this into file_bb
      ++i;

      c = san[i];
      targets &= RANK_1 << (8 * (c - '1')); // todo: move this into rank_bb
      ++i;
    } else {
      targets &= tmp;
    }
  
    std::cout << std::format("{:x} {:x}\n", pos.occupied(), pos.white);
    std::cout << std::format("{:x} {:x}\n", srcs, targets);

    assert(only_one(targets)); // todo: remove asserts, return nullopt instead
    const auto dst = static_cast<Square>(lsb(targets));
    if (more_than_one(srcs))
      srcs &= attacks_from(piece_type, dst, pos.occupied());

    std::cout << std::format("{:x} {:x}\n", srcs, targets);

    assert(only_one(srcs));
    const auto src = static_cast<Square>(lsb(srcs));
  
    return Move {src, dst, piece_type, false};
  // castling
  } else if (c == 'O') {
    if (san == "O-O")
      return Move {E1, G1, King, true};
    else if (san == "O-O-O")
      return Move {E1, C1, King, true};
  }

  return std::nullopt;
}

std::string chess::to_san(Move move, Position pos, bool black) {
  return {};
}
