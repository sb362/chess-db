#pragma once

#include "core/io.hh"
#include "db/page.hh"

namespace cdb::db {

constexpr std::string_view Magic = "\u00bfChessDB\x1a\r\n";
constexpr std::size_t HeaderSize = 128;
constexpr std::size_t NameLength = 64;

struct DbHeader {
  std::uint64_t checksum;      // 8 bytes
  std::uint32_t version;       // 4 bytes

  std::string name;            // 64 bytes

  std::uint64_t data_length;   // 8 bytes
  std::uint64_t data_offset;   // 8 bytes
  std::uint64_t data_checksum; // 8 bytes
	
  std::uint64_t no_games;      // 8 bytes
  std::uint32_t no_pages;      // 4 bytes

  std::uint64_t date_modified; // 8 bytes
};

class PageAllocator {
private:
  std::span<std::byte> _mem;
  stable_vector<Page, 8> _pages;

  std::size_t _data_length = 0;

public:
  PageAllocator() = default;

  PageAllocator(std::span<std::byte> mem, std::size_t data_length)
    : _mem(mem), _pages(), _data_length(data_length)
  {
    for (;;) {
      // loop over pages
      break;
    }
  }

  std::size_t space_used() const {
    return _data_length;
  }

  std::size_t max_space() const {
    return _mem.size();
  }

  std::size_t space_remaining() const {
    return max_space() - space_used();
  }

  Result<std::uint32_t> allocate(std::size_t size) {
    if (size > space_remaining()) {
      // todo: log
      return std::unexpected(DbError::OutOfMemory);
    }

    Page &page = _pages.emplace_back(_mem.subspan(_data_length, size), true);

    _data_length += size;
  }

  std::uint32_t no_pages() const {
    return _pages.size();
  }

  const Page &page(std::uint32_t page_no) const {
    assert(page_no < no_pages());
    return _pages[page_no];
  }

  Page &page(std::uint32_t page_no) {
    assert(page_no < no_pages());
    return _pages[page_no];
  }

  auto begin() { return _pages.begin(); }
  auto end()   { return _pages.end(); }
};

class Db {
private:
  DbHeader hdr;

  io::mm_file file;
  std::unique_ptr<PageAllocator> page_alloc;

public:
  void close();

  static Result<Db> open(const fs::path &path);
  static Result<Db> create(const fs::path &path, std::size_t size /*bytes*/);
  static Result<Db> from_pgn(const fs::path &db_path, const fs::path &pgn_path);

  void for_each(std::invocable<> auto fn) {
    for (auto &page : *page_alloc) {
      for (auto &game : page.games()) {
        
      }
    }
  }
};

} // cdb::db


