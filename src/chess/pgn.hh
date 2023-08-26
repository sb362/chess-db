#pragma once

#include "chess/bitboard.hh"
#include "chess/movegen.hh"
#include "chess/notation.hh"
#include "chess/pgn.hh"
#include "core/error.hh"
#include "util/vector.hh"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <utility>

namespace cdb::chess {

enum TokenType {
  NONE,

  NEWLINE,    // \r\n
  WHITESPACE, // \s\t

  INTEGER,    // 0-9
  STRING,     // "..."
  COMMENT,    // {...}
  SYMBOL,     // A-Za-z0-9_+#=:-

  PERIOD,     // .
  ASTERISK,   // *
  BRACKET,    // []()<>
    
  NAG,        // $123 or !!/??/?!/!?/!/?

  MISC,       // ;%
};

struct TokenTypeLookup {
  std::array<TokenType, 256> data;

  constexpr TokenTypeLookup()
    : data()
  {
    set(" \t", WHITESPACE);
    set("\r\n", NEWLINE);

    set("abcdefghijklmnopqrstuvwxyz", SYMBOL);
    set("ABCDEFGHIJKLMNOPQRSTUVWXYZ", SYMBOL);
    set("_+#=:-",                     SYMBOL);
    set("0123456789",                 INTEGER);

    set("\"\"", STRING);
    set("{}", COMMENT);
      
    set(".", PERIOD);
    set("*", ASTERISK);

    set("[]()<>", BRACKET);

    set("$?!", NAG);

    set(";%", MISC);
  }

  TokenType &operator[](const char c) {
    return data[static_cast<uint8_t>(c)];
  }

  TokenType const &operator[](const char c) const {
    return data[static_cast<uint8_t>(c)];
  }

  constexpr void set(std::string_view s, TokenType type) {
    for (char c : s)
      data[static_cast<uint8_t>(c)] = type;
  }
};
static constexpr TokenTypeLookup pgn_lookup;

struct Token {
  TokenType type = NONE;
  std::string_view contents;

  constexpr operator bool() const { return type != NONE; }

  constexpr bool is(auto ...types) const {
    return (... || (type == types));
  }
};

struct TokenStream {
  std::string_view pgn;
  std::size_t pos;

  constexpr TokenStream(std::string_view pgn, std::size_t pos = 0) : pgn(pgn), pos(pos) {}

  bool eof() const { return pos >= pgn.size(); }

  bool accept(char c) {
    if (pgn[pos] == c) {
      ++pos;
      return true;
    }
    else return false;
  }

  bool accept(std::string_view chars) {
    for (char c : chars)
      if (accept(c))
        return true;

    return false;
  }

  bool eat(std::string_view chars) {
    bool b = false;
    for (; accept(chars); ) b = true;
    return b;
  }

  void skip_line() {
    for (; pos < pgn.size(); ++pos) {
      if (pgn[pos] == '\n') {
        ++pos;
        break;
      }
    }
  }

  std::string_view peek(int start, std::size_t len = 0) const {
    return pgn.substr(pos + start, len);
  }

  Token next_token() {
    size_t start_pos = pos;
    char c = pgn[pos];
    TokenType type = pgn_lookup[c];

    switch (type)
    {
    case NEWLINE:
    case WHITESPACE:
    case INTEGER:
    case PERIOD:
      do { c = pgn[++pos]; } while (pgn_lookup[c] == type && pos < pgn.size());
      return {type, pgn.substr(start_pos, pos - start_pos)};
    case SYMBOL:
      do {
        c = pgn[++pos];
        if (pos >= pgn.size()) break;
      } while (pgn_lookup[c] == SYMBOL || pgn_lookup[c] == INTEGER);
      return {type, pgn.substr(start_pos, pos - start_pos)};
    case STRING:
    case COMMENT:
      do {
        c = pgn[++pos]; // todo: handle escaped quotes
        if (pgn_lookup[c] == type) {
          ++pos;
          break;
        }
      } while (pos < pgn.size());
      return {type, pgn.substr(start_pos, pos - start_pos)};
    case ASTERISK:
    case BRACKET:
    case MISC:
      return {type, pgn.substr(pos++, 1)};
    case NAG:
      if (c == '$')
        do { c = pgn[++pos]; } while (pgn_lookup[c] == INTEGER && pos < pgn.size());
      else if (c == '?' || c == '!')
        eat("?!");

      return {type, pgn.substr(start_pos, pos - start_pos)};
    default:
      break;
    }

    return {NONE, ""};
  }

  std::string_view context() const {
    return get_context(pgn, pos, 8);
  }
};

enum class GameResult {
  Unknown = 0,
  Incomplete,
  White,
  Draw,
  Black
};

struct ParseStep {
  Move move {};
  std::string_view comment = "", san = "";
  std::size_t bytes_read = 0;
  unsigned move_no = 0;
  Position prev {}, next {};
};

template <class F> concept MoveVisitor = std::invocable<F, const ParseStep &>;
template <class F> concept TagVisitor = std::invocable<F, std::string_view, std::string_view>;
template <class F> concept ResultVisitor = std::invocable<F, GameResult>;
template <class F> concept ErrorVisitor = std::invocable<F, ParseResult>;


ParseResult parse_movetext(std::string_view pgn, MoveVisitor auto visitor,
                           ResultVisitor auto result_visitor, Position startpos = Startpos) {
  TokenStream stream {pgn};
  Token token = stream.next_token();
  GameResult result = GameResult::Unknown;
  ParseStep step {.next = startpos};
  unsigned variation_depth = 0;

  for (; token; ) {
    // move number/result
    // if result: break
    // if move number: eat period & whitespace, parse SAN, parse NAGs, parse comment
    // if open bracket: start new variation, continue
    // if close bracket: exit previous variation, continue

    // eat any whitespace before move number
    for (; token.is(WHITESPACE, NEWLINE); token = stream.next_token()) {}

    if (token.is(INTEGER, ASTERISK)) {
      if (token.is(ASTERISK)) {
        result = GameResult::Incomplete;
        break;
      } else if (stream.accept('/')) {
        auto ss = stream.peek(-2, 7);
        if (ss == "1/2-1/2") {
          result = GameResult::Draw;
          stream.pos += 6;
        } else {
          return {stream.pos - 1, PGNParseError::MalformedResultToken, stream.context()};
        }

        break;
      } else if (stream.accept('-')) {
        auto ss = stream.peek(-2, 3);
        if (ss == "1-0") {
          result = GameResult::White;
          stream.pos += 2;
        } else if (ss == "0-1") {
          result = GameResult::Black;
          stream.pos += 2;
        } else {
          return {stream.pos - 1, PGNParseError::MalformedResultToken, stream.context()};
        }
        break;
      } else if (stream.accept('.')) {
        stream.accept('.');
        stream.accept('.');
      } else {
        return {stream.pos, PGNParseError::InvalidMoveNumber, stream.context()};
      }

      token = stream.next_token();
    } else {
    }

    // eat any whitespace between move number and SAN
    for (; token.is(WHITESPACE, NEWLINE); token = stream.next_token()) {}

    if (token.is(SYMBOL)) {
      ++step.move_no;
      step.san = token.contents;

      // todo: fixme for black having side to move in starting position
      auto move = chess::parse_san(step.san, step.next, step.move_no % 2 == 0);
      if (!move)
        return {stream.pos, move.error(), stream.context()};

      step.move = *move;
      step.prev = std::exchange(step.next, chess::make_move(step.next, step.move));
      token = stream.next_token();
    }

    // eat any NAGs and whitespace
    for (; token.is(WHITESPACE, NEWLINE, NAG); token = stream.next_token()) {
      if (token.is(NAG)) {
        // add NAG
      }
    }

    // consume comment
    if (token.is(COMMENT)) {
      step.comment = token.contents;
      token = stream.next_token();
    } else {
      step.comment = "";
    }

    step.bytes_read = stream.pos;
    visitor(step);

    for (; token.is(WHITESPACE, NEWLINE); token = stream.next_token()) {}

    if (token.is(BRACKET)) {
      // todo: handle variations
      if (token.contents == "(") {
        ++variation_depth;

        for (++stream.pos; variation_depth && !stream.eof(); ++stream.pos) {
          char c = stream.pgn[stream.pos];
          if (c == ')') --variation_depth;
          if (c == '(') ++variation_depth;
        }
      } else if (token.contents == ")") {
        return {stream.pos, PGNParseError::NotInVariation, stream.context()};
      } else {
        return {stream.pos, PGNParseError::ReservedToken, stream.context()};
      }

      token = stream.next_token();
    }
  }

  if (stream.pos > 0)
    result_visitor(result);

  return {stream.pos};
}

inline ParseResult skip_movetext(std::string_view pgn) {
  unsigned variation_depth = 0;
  TokenStream stream {pgn};
  Token token = stream.next_token();

  for (; token; ) {
    // move number/result
    // if result: break
    // if move number: eat period & whitespace, parse SAN, parse NAGs, parse comment
    // if open bracket: start new variation, continue
    // if close bracket: exit previous variation, continue

    // eat any whitespace before move number
    for (; token.is(WHITESPACE, NEWLINE); token = stream.next_token()) {}

    if (token.is(INTEGER, ASTERISK)) {
      if (token.is(ASTERISK)) {
        break;
      } else if (stream.accept('/')) {
        auto ss = stream.peek(-2, 7);
        if (ss == "1/2-1/2") {
          stream.pos += 6;
        } else {
          return {stream.pos - 1, PGNParseError::MalformedResultToken, stream.context()};
        }

        break;
      } else if (stream.accept('-')) {
        auto ss = stream.peek(-2, 3);
        if (ss == "1-0" || ss == "0-1") {
          stream.pos += 2;
        } else {
          return {stream.pos - 1, PGNParseError::MalformedResultToken, stream.context()};
        }
        break;
      } else if (stream.accept('.')) {
        stream.accept('.');
        stream.accept('.');
      } else {
        return {stream.pos, PGNParseError::InvalidMoveNumber, stream.context()};
      }

      token = stream.next_token();
    } else {
    }

    // eat any whitespace between move number and SAN
    for (; token.is(WHITESPACE, NEWLINE); token = stream.next_token()) {}

    if (token.is(SYMBOL)) {
      token = stream.next_token();
    }

    // eat any NAGs and whitespace
    for (; token.is(WHITESPACE, NEWLINE, NAG); token = stream.next_token()) {
      if (token.is(NAG)) {
        // add NAG
      }
    }

    // consume comment
    if (token.is(COMMENT)) {
      token = stream.next_token();
    } else {
    }

    for (; token.is(WHITESPACE, NEWLINE); token = stream.next_token()) {}

    if (token.is(BRACKET)) {
      if (token.contents == "(") {
        ++variation_depth;

        for (++stream.pos; variation_depth && !stream.eof(); ++stream.pos) {
          char c = stream.pgn[stream.pos];
          if (c == ')') --variation_depth;
          if (c == '(') ++variation_depth;
        }
      } else if (token.contents == ")") {
        return {stream.pos, PGNParseError::NotInVariation, stream.context()};
      } else {
        return {stream.pos, PGNParseError::ReservedToken, stream.context()};
      }

      token = stream.next_token();
    }
  }

  return {stream.pos};
}

ParseResult parse_tags(std::string_view pgn, TagVisitor auto visitor) {
  TokenStream stream {pgn};
  Token token;

  stream.eat("\r\n \t");
  for (; stream.accept('['); stream.eat("\r\n \t")) {
    token = stream.next_token();
    if (token.type != SYMBOL)
      return {stream.pos, PGNParseError::MalformedTag, stream.context()};
    
    std::string_view name = token.contents;

    stream.accept(' ');

    std::size_t pos = stream.pos;
    bool closing_bracket = false;
    for (++stream.pos; ; ++stream.pos) {
      if (stream.accept(']')) {
        closing_bracket = true;
        break;
      }
    }

    if (!closing_bracket)
      return {pos, PGNParseError::UnterminatedTag, stream.pgn.substr(pos)};

    std::string_view value = pgn.substr(pos, stream.pos - pos - 1);
    visitor(name, value);
  }
  
  return {stream.pos};
}

ParseResult parse_game(std::string_view pgn, TagVisitor auto tag_visitor,
                       MoveVisitor auto move_visitor, ResultVisitor auto result_visitor,
                       ErrorVisitor auto on_error = [] (auto) {}, bool skip_on_error = false) {
  std::string_view fen;
  auto r = parse_tags(pgn, [&] (auto name, auto value) {
    if (name == "FEN") [[unlikely]]
      fen = value;

    return tag_visitor(name, value);
  });

  if (!r)
    return r;

  Result<Position> startpos = Startpos;
  if (!fen.empty()) // todo
    startpos = std::unexpected(PGNParseError::CustomFENNotImplemented);

  if (!startpos) {
    ParseResult s {r.pos, startpos.error(), fen};
    on_error(s);

    if (skip_on_error) {
      s = skip_movetext(pgn.substr(r.pos));
      return {r.pos + s.pos, s.ec, s.context};
    } else
      return s;
  }

  auto s = parse_movetext(pgn.substr(r.pos), move_visitor, result_visitor, *startpos);
  if (!s)
    on_error(s);

  return {r.pos + s.pos, s.ec, s.context};
}

ParseResult parse_games(std::string_view pgn, TagVisitor auto tag_visitor,
                        MoveVisitor auto move_visitor, ResultVisitor auto result_visitor,
                        ErrorVisitor auto on_error, bool skip_on_error = false) {
  std::size_t pos = 0;
  for (; pos < pgn.size(); ) {
    auto r = parse_game(pgn.substr(pos), tag_visitor, move_visitor, result_visitor, on_error, skip_on_error);
    if (!r)
      return {pos + r.pos, r.ec, r.context};

    if (r.pos == 0)
      break;

    pos += r.pos;
  }

  return {pos};
}

} // cdb::chess
