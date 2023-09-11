#pragma once

#include "core/byteio.hh"
#include "chess/pgn.hh"

namespace cdb::db {

struct Date {
  uint32_t yyyymmdd;
};

class GameHeader {
private:
  /* seven tag roster */

  std::string event;
  std::string site;
  Date date;
  std::uint16_t round;
  std::string white, black;
  chess::GameResult result;

  std::uint16_t ply_count;

public:


  void serialise(io::mutable_buffer &buf) const;
  void deserialise(io::const_buffer &buf) const;
};

} // cdb::cdb
