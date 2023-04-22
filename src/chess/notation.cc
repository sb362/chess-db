
#include "chess/bitboard.hh"
#include "chess/movegen.hh"
#include "chess/notation.hh"
#include "chess/position.hh"

using namespace cdb;
using namespace chess;

#include <iostream>

constexpr std::string_view PieceChars = "/PNBR/QK";

Result<Move> chess::parse_san(std::string_view san, Position pos, bool black) {
  using enum PieceType;

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
      const auto r = RANK_1 << (8 * (c - '1') ^ (black ? 56 : 0));
      targets &= r;
      targets &= f;

      srcs &= shift<South>(targets) | walk<South, South>(targets & RANK_4);

    // captures
    } else if (c == 'x') {
      // en passant
      targets |= pos.white &~ pos.occupied();

      c = san[i++];
      if (!('a' <= c && c <= 'h'))
        return std::unexpected(ParseError::Invalid);

      targets &= (FILE_A << (c - 'a'));

      c = san[i++];
      if (!('1' <= c && c <= '8'))
        return std::unexpected(ParseError::Invalid);

      targets &= (RANK_1 << (8 * (c - '1') ^ (black ? 56 : 0)));
      srcs &= shiftm<SouthWest, SouthEast>(targets);

    } else {
      return std::unexpected(ParseError::Invalid);
    }

    // promotion
    if (i < san.size() - 1 && san[i++] == '=') {
      size_t idx = PieceChars.find(san[i]);
      
      if (idx == std::string_view::npos)
        return std::unexpected(ParseError::Invalid);

      piece_type = static_cast<PieceType>(idx);
    }

    if (!only_one(srcs) || !only_one(targets))
      return std::unexpected(ParseError::Ambiguous);

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
      tmp &= RANK_1 << (8 * (c - '1') ^ (black ? 56 : 0)); // todo: move this into rank_bb
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
      targets &= RANK_1 << (8 * (c - '1') ^ (black ? 56 : 0)); // todo: move this into rank_bb
      ++i;
    } else {
      targets &= tmp;
    }

    if (!only_one(targets))
      return std::unexpected(ParseError::Ambiguous);

    const auto dst = static_cast<Square>(lsb(targets));
    if (more_than_one(srcs))
      srcs &= attacks_from(piece_type, dst, pos.occupied());

    if (!only_one(srcs))
      return std::unexpected(ParseError::Ambiguous);

    const auto src = static_cast<Square>(lsb(srcs));
  
    return Move {src, dst, piece_type, false};
  // castling
  } else if (c == 'O') {
    if (san == "O-O")
      return Move {E1, G1, King, true};
    else if (san == "O-O-O")
      return Move {E1, C1, King, true};
  }

  return std::unexpected(ParseError::Invalid);
}

std::string chess::to_san(Move move, Position pos, bool black) {
  return {};
}
