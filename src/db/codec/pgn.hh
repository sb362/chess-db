#pragma once

#include "codec.hh"

namespace cdb::db {

class PGNDecoder final : GameDecoder {
private:
  TagView tag;

public:
  constexpr virtual float compression_ratio() const override {
    return 1;
  }

  constexpr virtual std::string_view name() const override {
    return "pgn";
  }

  std::string_view desc() const override {
    return "portable game notation";
  }


};

} // cdb::db
