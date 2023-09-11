#pragma once

#include "core/byteio.hh"
#include "core/io.hh"

#include <optional>

namespace cdb::db {

constexpr std::string_view Magic = "\u00bfChessDB\n";
constexpr std::size_t NameLength = 42;
constexpr std::size_t HeaderSize = 8 + 4 + NameLength + 8 + 8 + 8 + 8;

struct Header {
  std::uint64_t checksum = 0;      // 8 bytes
  std::uint32_t version = 0;       // 4 bytes

  std::string name {};

  std::uint64_t data_length = 0;   // 8 bytes
  std::uint64_t data_offset = 0;   // 8 bytes
  std::uint64_t data_checksum = 0; // 8 bytes
	
  std::uint64_t no_games = 0;      // 8 bytes

  // Serialise header to given buffer.
  // Must be at least HeaderSize in length.
  // Non-const because this also updates the header checksum.
  void serialise(io::mutable_buffer &buf);
  static Result<Header> deserialise(io::const_buffer &buf);
  static const decltype(version) VERSION_PGN = 0xffffffff;
};

struct OpenOptions {
  bool create = false;
  bool temporary = false;
  bool in_memory = false;
  std::size_t size = 0;
};

class Storage {
private:
  std::optional<fs::path> _path;
  io::mm_file _file;
  io::mutable_buffer _buf;

public:
  Storage(fs::path &&path, io::mm_file &&file)
    : _path(std::move(path)),
      _file(std::move(file)),
      _buf(_file.mutable_span())
  {
  }

  Storage(io::mutable_buffer &&buf)
    : _buf(std::move(buf))
  {
    assert(!_buf.is_view());
  }

  io::mutable_buffer mutable_buf() const noexcept {
    return {_buf.data(), _buf.size()};
  }

  io::const_buffer buf() const noexcept {
    return {_buf.data(), _buf.size()};
  }

  Result<std::size_t> request_resize(std::size_t new_size) {
    return 0;
  }
};

class Database {
private:
  Storage _storage;
  Header _header;

public:
  Database(Storage &&storage, Header &&header)
    : _storage(std::move(storage)), _header(std::move(header))
  {

  }

  bool is_pgn() const noexcept {
    return _header.version == Header::VERSION_PGN;
  }

  bool has_header() const noexcept {
    return !is_pgn();
  }

  const Header &header() const noexcept {
    return _header;
  }

  io::const_buffer game_buf() const noexcept {
    return _storage.buf().subbuf(header().data_offset, header().data_length);
  }

  io::mutable_buffer mutable_game_buf() const noexcept {
    return _storage.mutable_buf().subbuf(header().data_offset, header().data_length);
  }

  std::uint64_t checksum() const noexcept {
    return game_buf().hash();
  }

  void flush() {
    log().debug("db: flushing...");
    if (has_header()) {
      auto b = _storage.mutable_buf();
      _header.data_checksum = checksum();
      _header.serialise(b);
    }
  }

  /* opens an existing on-disk database, optionally resizing to given size */
  static Result<Database> open(fs::path path, const OpenOptions &open_options);
};

} // cdb::db
