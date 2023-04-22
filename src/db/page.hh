#pragma once

#include "db/pageindex.hh"
#include "util/bits.hh"
#include "util/iterator.hh"
#include "util/komihash.hh"

namespace cdb::db {

namespace detail {
  template <bool Const>
  struct GameSpan {
    using Byte = std::conditional_t<Const, const std::byte, std::byte>;
    using Span = std::span<Byte>;
    Span tag_data, move_data;
  };
}

using GameView = detail::GameSpan<true>;
using GameSpan = detail::GameSpan<false>;

struct PageHeader {
  static constexpr std::size_t Size = 8;

  std::uint16_t size, cursor; // 2 bytes each
  std::uint32_t checksum;     // 4 bytes

  PageHeader(std::span<std::byte, PageHeader::Size> data)
    : size(read_le<2>(data)), cursor(read_le<2>(data, 2)), checksum(read_le<4>(data, 4))
  {
  }
};

class Page {
private:
  std::span<std::byte> _data;
  PageHeader _hdr;
  PageIndex _idx;

  bool _changed = false;


public:
  Page(std::span<std::byte> data, bool init)
    : _data(data), _hdr(data.first<PageHeader::Size>()), _idx(data.subspan(PageHeader::Size), init)
  {
    if (init) {
      _hdr.size     = data.size();
      _hdr.cursor   = 0;
      _hdr.checksum = 0;
    }
  }

  std::size_t size() const { return _hdr.size; }
  std::size_t cursor() const { return _hdr.cursor; }
  std::uint32_t checksum() const { return _hdr.checksum; }
  std::uint32_t actual_checksum() const {
    // todo: refactor this to use std::span::last
    return komihash(_data.subspan(PageHeader::Size,
                                  _data.size() - PageHeader::Size), 0) >> 32;
  }

  bool changed() const { return _changed; }
  void mark_changed(bool b = true) { _changed = b; }

  /**
   * @brief Compute new checksum & write page header data to memory.
   * 
   * @return new checksum value
   */
  std::uint32_t commit() {
    _hdr.checksum = actual_checksum();

    write_le<2>(_data, _hdr.size);
    write_le<2>(_data, _hdr.cursor);
    write_le<4>(_data, _hdr.checksum);

    mark_changed(false);
    return _hdr.checksum;
  }

public:
  class GameIterator : public iterator_facade<GameIterator, GameSpan> {
  private:
    Page &page;
    //GameSpan &span {};
    int game_idx = 0;

  public:
    GameIterator(Page &page, int game_idx)
      : page(page), game_idx(game_idx)
    {

    }

    GameSpan &value() const {

    }

    void increment() {
      ++game_idx;
    }

    bool equal(const GameIterator &other) const { return game_idx == other.game_idx; }
  };

private:
  struct Games {
    Page &page;

    Games(Page &page) : page(page) {};

    GameIterator begin() const {
      return {page,  0};
    }

    GameIterator end() const {
      return {page, -1};
    }
  };

public:
  Games games() {
    return *this;
  }
};

} // cdb::db
