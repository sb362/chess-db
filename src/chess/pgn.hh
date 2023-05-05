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
    case INTEGER:
    case WHITESPACE:
    case NEWLINE:
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
};

template <typename T, std::size_t Size>
struct Stack {
  static_vector<T, Size> data;

  constexpr void push(T &&value) { data.push_back(std::forward<T>(value)); }

  template <typename ...Args>
  constexpr decltype(auto) emplace(Args &&...args) {
    return data.emplace_back(std::forward<Args>(args)...);
  }

  constexpr T &top() { return data.back(); }
  constexpr const T &top() const { return data.back(); }

  constexpr T pop() {
    auto &&value = std::move(data.back());
    data.pop_back();
    return value;
  }
};

enum class GameResult {
  Unknown = 0,
  Incomplete,
  White,
  Draw,
  Black
};

struct Error {
  std::string_view reason;
  std::size_t pos = 0;

  constexpr operator bool() const { return !reason.empty(); }
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

struct ParseResult {
  std::error_code ec;
  std::string_view msg;
  std::size_t bytes_read = 0;

  ParseResult(std::size_t bytes_read, std::error_code ec = {}, std::string_view msg = {})
    : ec(ec), msg(msg), bytes_read(bytes_read)
  {
  }
};

ParseResult parse_movetext(std::string_view pgn, MoveVisitor auto visitor) {
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
          //stream.pos += 6;
        } else {
          return {stream.pos - 1, ParseError::Invalid, "malformed result token"};
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
          return {stream.pos - 1, ParseError::Invalid, "malformed result token"};
        }
        break;
      } else if (stream.accept('.')) {
        stream.accept('.');
        stream.accept('.');
      } else {
        return {stream.pos, ParseError::Invalid, "invalid move number"};
      }

      token = stream.next_token();
    } else {
    }

    // eat any whitespace between move number and SAN
    for (; token.is(WHITESPACE, NEWLINE); token = stream.next_token()) {}

    if (token.is(SYMBOL)) {
      ++step.move_no;
      step.san = token.contents;
      if (step.san == "Nd4") {
          std::cout << "hello\n";
      }

      // todo: fixme for black having side to move in starting position
      auto move = chess::parse_san(step.san, step.next, step.move_no % 2 == 0);
      if (!move)
        return {stream.pos, move.error(), "invalid SAN"};

      step.move = *move; // todo: check error
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
        return {stream.pos, ParseError::Illegal, "unexpected closing bracket"};
      } else {
        return {stream.pos, ParseError::Reserved, "reserved token"};
      }

      token = stream.next_token();
    }
  }

  if (result != GameResult::Unknown) {
    // todo
    //visitor(step);
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
      return {stream.pos, ParseError::Invalid, "missing tag name"};
    
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
      return {stream.pos, ParseError::Invalid, "missing closing bracket"};

    std::string_view value = pgn.substr(pos, stream.pos - pos - 1);
    visitor(name, value);
  }
  
  return {token ? stream.pos : 0};
}

} // cdb::chess
