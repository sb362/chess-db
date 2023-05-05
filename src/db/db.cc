
#include "core/error.hh"
#include "core/logger.hh"
#include "db/db.hh"
#include "util/bits.hh"
#include "util/bytesize.hh"
#include "util/komihash.hh"

#include <filesystem>
#include <ranges>
#include <vector>

using namespace cdb;
using namespace cdb::db;
namespace fs = std::filesystem;

static const log::logger logger("db");

namespace {
  template <std::size_t S>
  std::string read_c_str(std::span<const std::byte, S> data,
                              std::size_t offset = 0,
                              std::size_t max_size = std::string::npos) {
    // todo: should really avoid the loop and use string view here
    std::string s;
    for (std::size_t i = offset; max_size--; ++i) {
      char c = std::to_integer<char>(data[i]);
      if (c == '\0')
        break;

      s.push_back(c);
    }

    return s;
  }

  Result<DbHeader> read_header(std::span<const std::byte, HeaderSize> header_span) {
    DbHeader hdr;

    const auto MagicBytes = std::as_bytes(std::span {Magic.data(), Magic.size()});
    const auto magic = header_span.subspan<0, Magic.size()>();
    if (!std::ranges::equal(magic, MagicBytes)) {
      logger.error("bad header - magic does not match");
      return std::unexpected(DbError::BadMagic);
    }

    std::uint32_t actual_checksum = komihash(header_span.subspan<16, HeaderSize - 16>(), 0) >> 32;
    hdr.checksum = read_le<4>(header_span, 12);
    if (hdr.checksum != actual_checksum) {
      logger.error("bad header - checksums do not match - expected {:x}, got {:x}",
                   hdr.checksum, actual_checksum);
      return std::unexpected(DbError::BadChecksum);
    }

    hdr.version       = read_le<4>(header_span, 16);
    hdr.name          = read_c_str(header_span, 20, NameLength);
    hdr.data_length   = read_le<8>(header_span, 84);
    hdr.data_offset   = read_le<8>(header_span, 92);
    hdr.data_checksum = read_le<8>(header_span, 100);
    hdr.no_games      = read_le<8>(header_span, 108);
    hdr.no_pages      = read_le<4>(header_span, 116);
    hdr.date_modified = read_le<8>(header_span, 120);

    return hdr;
  }
}

Result<Db> Db::open(const fs::path &path) {
  Db db;

  if (auto ec = db.file.open(path)) {
    logger.error("failed to open file '{}' ({})", path.lexically_normal().string(), ec.message());
    return std::unexpected(ec);
  }

  auto hdr = read_header(db.file.span().subspan<0, HeaderSize>());
  if (!hdr) {
    logger.error("file '{}' has corrupted header: {}", path.lexically_normal().string(), hdr.error().message());
    return std::unexpected(hdr.error());
  } else
    db.hdr = std::move(*hdr);

  db.page_alloc = std::make_unique<PageAllocator>(db.file.mutable_span(), db.hdr.data_length);

  return db;
}

Result<Db> Db::create(const fs::path &path, std::size_t size) {
  if (size <= HeaderSize)
    return std::unexpected(IOError::NotEnoughSpace); // todo: revise error
  
  Db db;

  if (auto ec = db.file.open(path, size))
    return std::unexpected(ec);

  db.hdr.checksum = 0;
  db.hdr.version  = 0;

  db.hdr.data_length   = 0;
  db.hdr.data_offset   = HeaderSize;
  db.hdr.data_checksum = 0;

  db.hdr.no_games = 0;
  db.hdr.no_pages = 0;

  return db;
}

Result<Db> Db::from_pgn(const fs::path &db_path, const fs::path &pgn_path) {
  using FileInfo = std::tuple<std::size_t, fs::path>;
  std::vector<FileInfo> pgn_files;
  std::size_t total_size = 0;
  auto status = fs::status(pgn_path);

  if (status.type() == fs::file_type::directory) {
    for (const auto &p : fs::recursive_directory_iterator(pgn_path)) {
      if (p.path().extension() == ".pgn") {
        pgn_files.emplace_back(p.file_size(), p.path().lexically_normal());
        total_size += p.file_size();
      }
    }

    std::ranges::sort(pgn_files);
  } else if (status.type() == fs::file_type::regular) {
    std::error_code ec;
    const auto size = fs::file_size(pgn_path, ec);

    if (ec) {
      logger.error("{} : {}", pgn_path.lexically_normal().string(), ec.message());
      return std::unexpected(ec);
    }

    pgn_files.emplace_back(size, pgn_path.lexically_normal());
    total_size += size;
  } else {
    logger.error("{} is neither a directory nor file", pgn_path.lexically_normal().string());
    return std::unexpected(IOError::FileNotFound);
  }

  for (const auto &p : pgn_files) {
    logger.info("{} {}", std::get<1>(p).string(), best_size_unit {std::get<0>(p)});
  }

  logger.info("total size: {}", best_size_unit {total_size});
  logger.info("allocating {} for database", best_size_unit {total_size});

  const auto max_encoded_size = total_size; /* todo */
  const auto si = fs::space(pgn_path);
  if (max_encoded_size >= si.available) {
    logger.error("not enough space on disk ({} remaining, need {})",
                 best_size_unit {si.available}, best_size_unit {max_encoded_size});
    return std::unexpected(IOError::NotEnoughSpace);
  }

  auto db = create(db_path, max_encoded_size);

  return db;
}

/*std::unique_ptr<GameDecoder> Db::make_decoder() const {
  return nullptr;
}*/
