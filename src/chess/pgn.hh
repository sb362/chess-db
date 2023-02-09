#pragma once


#include "chess/movegen.hh"
#include "chess/pgn.hh"

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
    start = std::clamp(start, (int)pos, (int)(pgn.size()) - (int)pos);
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

enum class State {
  InMainline,
  EnterVariation,
  InVariation,
  ExitVariation,
};

enum class Result {
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

  State state = State::InMainline;
  Position prev {}, next {};

  Error error {};
};

template <class F> concept MoveVisitor = std::invocable<F, ParseStep>;
template <class F> concept TagVisitor = std::invocable<F, std::string_view, std::string_view>;

std::size_t parse_movetext(std::string_view pgn, MoveVisitor auto visitor) {
  TokenStream stream {pgn};
  Token token = stream.next_token();
  Result result = Result::Unknown;
  ParseStep step;

  struct ParserFrame { Position prev, next; };
  Stack<ParserFrame, 8> frames {};
  frames.emplace(startpos, startpos);

  for (; token; token = stream.next_token()) {
    auto &top = frames.top();

    step = {.bytes_read = stream.pos}; // reset

    // move number/result
    // if result: break
    // if move number: eat period & whitespace, parse SAN, parse NAGs, parse comment
    // if open bracket: start new variation, continue
    // if close bracket: exit previous variation, continue

    if (token.is(INTEGER, ASTERISK)) {
      if (token.is(ASTERISK)) {
        result = Result::Incomplete;
        break;
      } else if (stream.accept('/')) {
        if (stream.peek(-1, 7) == "1/2-1/2") {
          result = Result::Draw;
          //stream.pos += 6;
        } else {
          step.error = {"malformed result token", stream.pos - 1};
        }

        break;
      } else if (stream.accept('-')) {
        const auto ss = stream.peek(-1, 3);
        if (ss == "1-0") {
          result = Result::White;
          stream.pos += 2;
        } else if (ss == "0-1") {
          result = Result::Black;
          stream.pos += 2;
        } else {
          step.error = {"malformed result token", stream.pos - 1};
        }
        break;
      } else if (stream.accept('.')) {
        // move number
        std::cout << "move no: " << token.contents << std::endl;
      } else {
      }
    } else {
    }

    // eat any whitespace between move number and SAN
    token = stream.next_token();
    for (; token.is(WHITESPACE); token = stream.next_token()) {}

    if (token.is(SYMBOL)) {
      step.san = token.contents;

      const auto move = parse_san(top.next, step.san, false);
      assert(move);

      top.prev = std::exchange(top.next, make_move(top.next, move));
    }

    // eat any NAGs and whitespace
    token = stream.next_token();
    for (; token.is(WHITESPACE, NAG); token = stream.next_token()) {
      if (token.is(NAG)) {
        // add NAG
      }
    }

    // consome comment
    if (token.is(COMMENT)) {
      step.comment = token.contents;
    }

    step.bytes_read = stream.pos;
    visitor(step);

    if (token.is(BRACKET)) {
      if (token.contents == "(") {

      } else if (token.contents == ")") {

      } else {
        step.error = {"reserved token", stream.pos};
        break;
      }
    }
  }
  std::cout << "last token: " << token.contents << std::endl;

  visitor(step);
  return stream.pos;
}

std::size_t parse_headers(std::string_view pgn, TagVisitor auto visitor) {
  TokenStream stream {pgn};
  Token token;

  for (; stream.accept('['); token = stream.next_token()) {
    token = stream.next_token();
    assert(token.type == SYMBOL);
    
    std::string_view name = token.contents;

    stream.accept(' ');

    token = stream.next_token();
    assert(token.type == STRING);

    if (visitor(name, token.contents))
      break;

    assert(stream.accept(']'));
    stream.eat("\r\n \t");
  }
  
  return stream.pos;
}

} // cdb::chess
