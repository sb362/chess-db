#pragma once

#include "core/error.hh"
#include "db/codec.hh"
#include "util/bits.hh"
#include "util/komihash.hh"

#include <cassert>
#include <memory>
#include <span>

namespace cdb::db {
  class Db;

  namespace GameFormat { // todo: make bitmask class
    using Type = std::uint8_t;
    constexpr Type Empty       = 0x0;
    constexpr Type HasTagData  = 0x1;
    constexpr Type HasComments = 0x2;
    constexpr Type HasNAGs     = 0x4;
  }

  class Game {
  private:
    Db *_db; // weak/shared_ptr ??? should be page instead?

    // this is only non-null for non-reference games
    std::unique_ptr<std::byte []> _mem;

    // points to either _mem (non-reference) or an area in _db (reference)
    std::span<std::byte> _data;

  public: /* game format */
    GameFormat::Type format() const {
      // todo: cache this?
      return read_le<1>(_data);
    }

    bool has_tags() const {
      return format() & GameFormat::HasTagData;
    }

    bool has_comments() const {
      return format() & GameFormat::HasComments;
    }

    bool has_nags() const {
      return format() & GameFormat::HasNAGs;
    }

  public: /* data */
    unsigned tag_data_size() const {
      assert(has_tags());
      return read_le<2>(_data, 1);
    }

    std::span<std::byte> tag_data() const {
      return _data.subspan(1, tag_data_size());
    }

    std::span<std::byte> move_data() const {
      auto offset = 3 + tag_data_size();
      return _data.subspan(offset, read_le<2>(_data, offset));
    }

    std::uint64_t checksum() const {
      return komihash(_data, 0);
    }
  
  public: /* codec */
    struct Moves {
      const Game &game;

      GameDecoder begin() const {
        return DecoderImpl(game.move_data());
      }

      GameDecoder end() const {
        return {};
      }
    };
    Moves moves() const { return {*this}; }

    struct Tags {
      const Game &game;

      TagDecoder begin() const {
        return {game.tag_data()};
      }

      TagDecoder end() const {
        return {};
      }
    };
    Tags tags() const { return {*this}; }

  public: /* manipulating game data */

  // insert_comment etc...
  // iterator
  // use _mem as staging area for changes?

  public: /* database */
    // returns true if game data was moved in memory, false otherwise.
    // returns an error code if the resize failed.
    Result<bool> request_resize(std::size_t new_size, bool allow_move) {
      if (is_reference()) {
        // ask db to move the game
        return std::unexpected(CoreError::NotImplemented);
      } else if (allow_move) {
        assert(_mem);

        auto tmp = std::make_unique<std::byte []>(new_size);
        std::ranges::copy(_data, tmp.get());
        return _mem = std::exchange(tmp, nullptr), true;
      } else {
        return std::unexpected(IOError::NotEnoughSpace);
      }
    }

    // returns true if this game relies on a database
    bool is_reference() const {
      return _db != nullptr;
    }

    void dereference() {
      if (is_reference()) {
        _mem = std::make_unique<std::byte []>(_data.size());

        std::ranges::copy(_data, _mem.get());
        _data = {_mem.get(), _data.size()};
        _db = nullptr;
      }
    }
  };
} // cdb::db
