
#include "core/byteio.hh"
#include "db/database.hh"

using namespace cdb;
using namespace cdb::db;
 
void Header::serialise(io::mutable_buffer &buf) {
  static constexpr std::span<const char, Magic.size()> magic(Magic.data(), Magic.size());

  auto begin = buf.pos();
  buf.write_bytes(std::as_bytes(magic));
  buf.seek(8); // skip header checksum for now
  buf.write_le(version);

  std::span<const char> name_data(name.data(), name.size());
  buf.write_bytes(std::as_bytes(name_data));

  // overwrite any old bytes after the name string.
  assert(name_data.size() <= NameLength);
  auto zero_bytes = std::min(NameLength - name_data.size(), NameLength);
  for (std::size_t i = 0; i < zero_bytes; ++i)
    buf.write_byte(std::byte(0));

  buf.write_le(data_length);
  buf.write_le(data_offset);
  buf.write_le(data_checksum);

  buf.write_le(no_games);

  auto end = buf.pos();

  // write header checksum
  checksum = buf.subbuf(begin + Magic.size() + 8, HeaderSize - 8).hash();
  log().info("begin = {}, end = {}, hash = {:08x}", begin + Magic.size() + 8, HeaderSize - 8, checksum);
  buf.seek_abs(begin + Magic.size());
  buf.write_le(checksum);
  buf.seek_abs(end);
}

/* static */ 
Result<Header>
Header::deserialise(io::const_buffer &buf) {
  if (buf.str_view().substr(0, Magic.size()) != Magic) {
    log().error("db: bad magic in header");
    return std::unexpected(DbError::BadMagic);
  }

  Header header;

  buf.seek(Magic.size());
  header.checksum = buf.read_le<8>();

  std::uint64_t checksum = buf.subbuf(Magic.size() + 8, HeaderSize - 8).hash();
  log().info("begin = {}, end = {}, hash = {:08x}", Magic.size() + 8, HeaderSize - 8, checksum);

  if (checksum != header.checksum) {
    log().error("db: bad checksum in header: got {:x}, actual {:x}", header.checksum, checksum);
    //return std::unexpected(DbError::BadChecksum);
  }

  header.version  = buf.read_le<4>();

  auto tmp = buf.subbuf(buf.pos(), NameLength).str_view();
  header.name = tmp.substr(0, std::min(tmp.find_first_of('\0'), NameLength));

  buf.seek(NameLength);
  header.data_length   = buf.read_le<8>();
  header.data_offset   = buf.read_le<8>();
  header.data_checksum = buf.read_le<8>();

  header.no_games = buf.read_le<8>();

  log().info("db: successfully read header:\n"
             "  checksum:      {:08x}\n"
             "  version:       {}\n"
             "  name:          {}\n"
             "  data length:   {}\n"
             "  data offset:   {}\n"
             "  data checksum: {:08x}\n"
             "  no. games:     {}\n",
             header.checksum, header.version, header.name,
             header.data_length, header.data_offset,
             header.data_checksum, header.no_games);

  return header;
}

/* static */
Result<Database> Database::open(fs::path path, const OpenOptions &open_options) {
  log().debug("db: opening db at {} (create = {}, in_memory = {}, size = {}, temporary = {})",
              path,
              open_options.create,
              open_options.in_memory,
              open_options.size,
              open_options.temporary);

  std::optional<Storage> storage;
  bool is_pgn = path.extension() == ".pgn";

  auto st = fs::status(path);
  bool exists = fs::exists(st);
  if (exists) {
    if (!fs::is_regular_file(st)) {
      log().error("db: {} exists, but is not a regular file", path);
      return std::unexpected(IOError::FileNotFound);
    }

    if (open_options.temporary) {
      log().error("db: attempt to make temp db at {}, but file already exists", path);
      return std::unexpected(IOError::FileExists);
    }

    // do not use mmap, copy file into memory
    if (open_options.in_memory) {
      std::ifstream ifs {path, std::ios::binary};
      if (!ifs.good()) {
        log().error("db: failed to open db {}", path);
        return std::unexpected(IOError::FileNotFound);
      }

      // get file size
      ifs.seekg(0, std::ios::end);
      std::size_t size = ifs.tellg();
      ifs.seekg(0, std::ios::beg);

      io::mutable_buffer buf(size);
      ifs.read(reinterpret_cast<char *>(buf.data()), size);
      ifs.close();

      storage.emplace(std::move(buf));
    } else {
      auto file = io::mm_open(path);
      if (!file)
        return std::unexpected(file.error());

      storage.emplace(std::move(path), std::move(*file));
    }
  } else {
    if (!open_options.create)
      return std::unexpected(IOError::FileNotFound);
  
    if (open_options.in_memory) {
      io::mutable_buffer buf(open_options.size);
      storage.emplace(std::move(buf));
    } else {
      auto file = io::mm_open(path, open_options.size);
      if (!file)
        return std::unexpected(file.error());

      storage.emplace(std::move(path), std::move(*file));
    }
  }

  Header header;
  if (exists) {
    if (!is_pgn) {
      auto buf = storage->buf();
      auto r = Header::deserialise(buf);
      if (!r)
        return std::unexpected(r.error());

      header = *r;
    } else {
      header.version = Header::VERSION_PGN;
    }
  } else {
    header.name        = "";
    header.version     = 1;
    header.data_offset = HeaderSize;
    header.data_length = storage->buf().size() - HeaderSize;
  }

  return Database(std::move(*storage), std::move(header));
}
