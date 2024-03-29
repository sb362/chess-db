
#include "util/bits.hh"
#include "chess/position.hh"
#include "core/logger.hh"

#include <algorithm>
#include <iostream>

using namespace cdb;
using namespace chess;

static const log::logger logger("fen");

constexpr std::string_view PieceChars = "/pnbr/qk";

Result<Position> Position::from_fen(std::string_view fen) {
  Position pos {};
  bitboard white = 0, black = 0;

  unsigned i = 0;
  int sq = A8;

  // piece placement
  for (; ; ++i) {
    char c = fen.at(i);

    if ('1' <= c && c <= '8') {
      sq += (c - '0') * East; // skip given number of files
    } else if (c == '/') {
      sq += South * 2; // jump down two ranks since we are on the previous rank
    } else if (size_t idx = PieceChars.find(std::tolower(c)); idx != std::string_view::npos) {
      pos.set(static_cast<Square>(sq), static_cast<PieceType>(idx));

      (std::isupper(c) ? white : black) |= square_bb(static_cast<Square>(sq));
      ++sq; // advance to next file
    } else if (c == ' ') {
      break;
    } else {
      // unexpected character in piece placement
      logger.error(R"(unexpected character '{}': "{}")", c, get_context(fen, i, 8));
      return std::unexpected(ParseError::Invalid);
    }
  }

  if (sq + South != A1) {
    logger.error(R"(incomplete piece placement: "{}")", get_context(fen, i, 8));
    return std::unexpected(ParseError::Illegal);
  }

  // side to move
  bool white_to_move = fen.at(++i) == 'w';
  if (!(white_to_move || fen[i] == 'b')) {
    logger.error(R"(invalid side to move '{}': "{}")", fen[i], get_context(fen, i, 8));
    return std::unexpected(ParseError::Invalid);
  }

  // castling
  if (char c = fen.at(i += 2); c != '-') {
    auto f = [&] (char d, Square sq) {
      if (c == d) {
        pos.x ^= square_bb(sq);
        c = fen.at(++i);
      }
    };

    f('K', A1);
    f('Q', H1);
    f('k', A8);
    f('q', H8);

    if (c != ' ') {
      logger.error(R"(expected space after castling: "{}")", fen[i], get_context(fen, i, 8));
      return std::unexpected(ParseError::Invalid);
    }
  } else {
    ++i; // eat '-'
  }

  // en passant
  bitboard ep = 0;
  if (char c = fen.at(++i); c != '-') {
    const uint8_t file = c - 'a', rank = (c = fen.at(++i)) - 'a';
    if (file < 8 && rank < 8) {
      logger.error(R"(invalid square "{}{}": "{}")", fen[i - 1], fen[i], get_context(fen, i, 8));
      return std::unexpected(ParseError::Invalid);
    }

    ep = square_bb(static_cast<Square>(8 * rank + file));
  }

  // flip position if black to move
  if (white_to_move) {
    pos.white = white | ep;
  } else {
    pos.x = byteswap(pos.x);
    pos.y = byteswap(pos.y);
    pos.z = byteswap(pos.z);
    pos.white = byteswap(black | ep);
  }

#ifndef NDEBUG
  pos.fen = pos.to_fen(!white_to_move);
#endif

  return pos;
}

std::string Position::to_fen(bool black) const {
  const Position pos = black ? rotated() : *this;
  std::string fen;

  int empty = 0;
  auto flush_empty = [&] {
    if (empty) {
      fen.push_back('0' + empty);
      empty = 0;
    }
  };

  for (int s = A8; ; ++s) {
    const auto sq = static_cast<Square>(s);
    const auto piece_type = pos.on(sq);

    if (piece_type == PieceType::None)
      ++empty;
    else {
      flush_empty();

      const char c = PieceChars[static_cast<int>(piece_type)];
      fen.push_back(((black ? ~pos.white : pos.white) & square_bb(sq)) ? std::toupper(c) : c);
    }

    if (s % 8 == 7) {      
      flush_empty();
      if (s == H1)
        break;
      else
        s += 2 * South;

      fen.push_back('/');
    }
  }

  fen.push_back(' ');
  fen.push_back(black ? 'b' : 'w');
  fen.push_back(' ');

  const bitboard castling = pos.extract(PieceType::Castle);

  if (castling & square_bb(H1)) fen.push_back('K');
  if (castling & square_bb(A1)) fen.push_back('Q');
  if (castling & square_bb(H8)) fen.push_back('k');
  if (castling & square_bb(A8)) fen.push_back('q');
  if (!castling) fen.push_back('-');

  fen.push_back(' ');

  if (bitboard ep = pos.white &~ pos.occupied()) {
    const auto s = lsb(ep);

    fen.push_back('a' + (s % 8));
    fen.push_back('1' + (s / 8));
  } else {
    fen.push_back('-');
  }

  return fen;
}
