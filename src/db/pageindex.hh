#pragma once

#include "util/bits.hh"
#include "util/komihash.hh"
#include "util/vector.hh"

#include <algorithm>
#include <ranges>

namespace cdb::db {

namespace GameFormat {
  using Type = std::uint8_t;
  constexpr Type Empty       = 0x0;
  constexpr Type HasTagData  = 0x1;
  constexpr Type HasComments = 0x2;
  constexpr Type HasNAGs     = 0x4;
}

/**
 * Inspired by absl:
 * https://github.com/abseil/abseil-cpp/blob/master/absl/container/internal/raw_hash_set.h#L438 
 *
 * empty    = 0b10000000
 * deleted  = 0b11000000
 * sentinel = 0b11111111
 * hash     = 0b01111111
 */
namespace Metadata {
  using Type = std::uint8_t;
  constexpr Type Hash     = 0b01111111;
  constexpr Type Empty    = 0b10000000;
  constexpr Type Deleted  = 0b11000000;
  constexpr Type Sentinel = 0b11111111;
}

class PageIndex {
private:
  std::vector<Metadata::Type> _metadata;
  std::vector<std::span<std::byte>> _games;

public:
  PageIndex(std::span<std::byte> data, bool new_data = false) {
    if (new_data) {
      write_le<1>(data, GameFormat::Empty);
      write_le<2>(data, data.size() - 3, 1);
    }

    reindex(data);
  }

  // mark game for deletion
  void mark_deleted(std::uint16_t game_idx) {
    assert(game_idx < _metadata.size());
    _metadata[game_idx] = Metadata::Deleted;
  }

  // returns index of game with given hash
  int find(std::uint64_t hash) {
    for (auto &&[i, md, game] : std::views::zip(std::views::iota(0), _metadata, _games))
      if (md == (hash & Metadata::Hash))
        if (hash == komihash(game, 0))
          return i;

    return -1;
  }

  // returns index of first space that is at least min_size bytes long
  int find_space(std::size_t min_size) {
    for (auto &&[i, md, game] : std::views::zip(std::views::iota(0), _metadata, _games))
      if (md & Metadata::Empty /* || (md & Metadata::Deleted) */)
        if (game.size() >= min_size)
          return i;

    return -1;
  }

  // returns index of first space that is at least min_size bytes long
  int find_space_and_split(std::size_t new_size) {
    int i = find_space(new_size);
    if (i == -1)
      return -1;

    // split
    auto ss1 = _games[i].first(new_size);
    auto ss2 = _games[i].last(_games[i].size() - new_size);

    // resize and append new chunk
    _games[i] = ss1;
    _games.emplace_back(ss2);
    _metadata.emplace_back(Metadata::Empty);

    return i;
  }

  // merge any adjacent empty or deleted chunks
  void coalesce() {
    constexpr std::byte zero {};

    for (unsigned i = 0; i < _games.size() - 1; ) {
      if (_metadata[i] & _metadata[i + 1] & Metadata::Empty) {
        // remove i+1th game info
        std::ranges::fill(_games[i + 1].first(3), zero);

        // remove i+1th game data
        if (_metadata[i + 1] & Metadata::Deleted)
          std::ranges::fill(_games[i + 1], zero);

        // remove ith game data
        if (_metadata[i] & Metadata::Deleted)
          std::ranges::fill(_games[i], zero);

        // merge ith and i+1th span
        auto new_size = _games[i].size() + _games[i + 1].size();
        _games[i]     = {_games[i].data(), new_size};

        // overwrite ith game info
        write_le<1>(_games[i], GameFormat::Empty);
        write_le<2>(_games[i], new_size - 2, 1);

        // remove i+1th span and metadata
        // todo: optimise? removing from middle of vector is quite slow
        _games.erase(std::next(_games.begin(), i + 1));
        _metadata.erase(std::next(_metadata.begin(), i + 1));
      } else {
        ++i;
      }
    }
  }

  // recompute index
  void reindex(std::span<std::byte> data) {
    std::uint32_t pos = 0, next_pos = 0;

    _metadata.clear();
    _games.clear();

    while (pos < data.size()) {
      auto format = static_cast<GameFormat::Type>(read_le<1>(data, pos));

      if (format == GameFormat::Empty) {
        auto skip = read_le<2>(data, 1 + pos);
        next_pos = 1 + pos + skip;
      } else {
        const bool has_tags = bool(format & GameFormat::HasTagData);

        // get length of tag data
        auto tag_data_size  = has_tags ? read_le<2>(data, 1 + pos) : 0;

        // move data starts after tag data
        auto move_offset    = (has_tags ? 2 : 0) + tag_data_size; // todo: possible mistake here
        auto move_data_size = read_le<2>(data, 1 + pos + move_offset);

        next_pos = pos + move_offset + move_data_size;
      }

      auto ss = data.subspan(pos, next_pos - pos);
      auto md = format == GameFormat::Empty ? Metadata::Empty
                                            : (komihash(ss, 0) & Metadata::Hash);

      _metadata.emplace_back(md);
      _games.emplace_back(ss);

      pos = std::exchange(next_pos, 0);
    }
  }
};

} // cdb::db
