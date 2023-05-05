#pragma once

#include "chess/movegen.hh"
#include "chess/pgn.hh"
#include "core/error.hh"
#include "util/iterator.hh"

namespace cdb::db {

using chess::GameResult;

class [[nodiscard]] GameStep {
private:
  friend class GameDecoder;

  chess::Position _prev {}, _next {};
  chess::Move _move {};
  std::string_view _comment;
  std::span<const std::byte> _nags;
  unsigned _var_depth = 0, _var_idx = 0;

protected:
  void set_variation_index(unsigned var_idx) { _var_idx = var_idx; }
  void set_variation_depth(unsigned var_depth) { _var_depth = var_depth; }
  void set_nags(std::span<const std::byte> nags) { _nags = nags; }
  void set_comment(std::string_view comment) { _comment = comment; }

  void advance(const chess::Move &move) {
    _prev = std::exchange(_next, chess::make_move(_next, move));
  }

public:
  GameStep() = default;

  GameStep(const GameStep &) = delete;
  GameStep &operator=(const GameStep &) = delete;

  GameStep(GameStep &&) = default;

  const chess::Position &previous() const { return _prev; }
  const chess::Position &next() const { return _next; }

  unsigned variation_index() const { return _var_idx; }
  unsigned variation_depth() const { return _var_depth; }

  std::span<const std::byte> nags() const { return _nags; }
  std::string_view comment() const { return _comment; }
  chess::Move move() const { return _move; }
};

struct TagView {
  std::string_view name, value;
};

struct Tag {
  std::string name, value;
};

class GameDecoder {
private:
protected:
  GameResult _result = GameResult::Unknown;
  GameStep _step;

public:
  GameDecoder(std::span<const std::byte> input) {};

  virtual ~GameDecoder() = default;

  constexpr virtual std::string_view name() const = 0;
  constexpr virtual std::string_view desc() const = 0;
  virtual float compression_ratio() const = 0;

  virtual std::error_code decode_tag(Tag &tag) = 0;
  virtual std::error_code decode_step(GameStep &step) = 0;

  template <class F = void>
  Result<F> visit_moves(std::invocable<F, GameStep> auto visitor) {

  }

  template <class F = void>
  Result<F> visit_tags(std::invocable<F, TagView> auto visitor) {

  }


  GameResult result() const { return _result; };

  virtual void skip_game() = 0;
};

} // cdb::db
