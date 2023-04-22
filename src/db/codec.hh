#pragma once

#include "chess/movegen.hh"
#include "core/error.hh"
#include "util/iterator.hh"

#include <array>
#include <optional>
#include <span>

namespace cdb::db {

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

class GameDecoder : public iterator_facade<GameDecoder, GameStep> {
private:
  std::uint32_t steps = 0, bytes_read = 0;
  std::error_code ec {};
  std::span<const std::byte> input;
  GameStep step {};

  static constexpr auto End = std::numeric_limits<decltype(steps)>::max();

  void advance() {
    if (steps == End)
      return;
    
    if (bytes_read >= input.size()) {
      steps = End;
      return;
    }

    auto read = decode_step(input.subspan(bytes_read), step);
    if (read) {
      bytes_read += *read;
      ++steps;
    } else {
      steps = End;
      ec = read.error();
    }
  }

public:
  GameDecoder()
    : steps(End)
  {
  }

  GameDecoder(std::span<const std::byte> input)
    : input(input)
  {
  }

  GameDecoder(const GameDecoder &) = delete;
  GameDecoder &operator=(const GameDecoder &) = delete;

  GameDecoder(GameDecoder &&) = default;

  virtual ~GameDecoder() = default;

  virtual Result<unsigned> decode_step(std::span<const std::byte> input, GameStep &step) {
    return std::unexpected(CoreError::NotImplemented);
  }

  bool equal(const GameDecoder &other) const { return steps == other.steps; }
};

class DecoderImpl final : public GameDecoder {
public:
  DecoderImpl(std::span<const std::byte> input) : GameDecoder(input) {}

  Result<unsigned> decode_step(std::span<const std::byte> input, GameStep &step) override {
    /* parsing logic */
    return std::unexpected(CoreError::NotImplemented);
  }
};

struct Tag {
  std::string name, value;
};

class TagDecoder : public iterator_facade<TagDecoder, Tag> {
private:
  std::uint32_t steps = 0, bytes_read = 0;
  std::error_code ec {};
  std::span<const std::byte> input;
  Tag tag {};

  static constexpr auto End = std::numeric_limits<decltype(steps)>::max();

  void advance() {
    if (steps == End)
      return;
    
    if (bytes_read >= input.size()) {
      steps = End;
      return;
    }

    auto read = decode_tag(input.subspan(bytes_read));
    if (read) {
      bytes_read += *read;
      ++steps;
    } else {
      steps = End;
      ec = read.error();
    }
  }

public:
  TagDecoder()
    : steps(End)
  {
  }

  TagDecoder(std::span<const std::byte> input)
    : input(input)
  {
  }

  TagDecoder(const TagDecoder &) = delete;
  TagDecoder &operator=(const TagDecoder &) = delete;

  TagDecoder(TagDecoder &&) = default;

  virtual ~TagDecoder() = default;

  virtual Result<unsigned> decode_tag(std::span<const std::byte> input) {
    return std::unexpected(CoreError::NotImplemented);
  }

  bool equal(const TagDecoder &other) const { return steps == other.steps; }
};

} // cdb::db
