#pragma once

#include "util/unordered_dense.h"
#include "chess/pgn.hh"

namespace cdb::format {

// todo: move this elsewhere
struct string_hash {
  using is_transparent = void;
  using is_avalanching = void;

  std::uint64_t operator()(std::string_view str) const noexcept {
    return ankerl::unordered_dense::hash<std::string_view>()(str);
  }
};

template <typename T>
using string_map = ankerl::unordered_dense::map<std::string_view, T,
                                                string_hash, std::equal_to<>>;

enum class TagId {
  Null = 0,
  Event,
  Site,
  Date,
  Round,
  White,
  Black,
  Result,
  WhiteElo,
  BlackElo
};

static string_map<TagId> TagIdMap = {
  {"(null)",   TagId::Null},
  {"Event",    TagId::Event},
  {"Site",     TagId::Site},
  {"Date",     TagId::Date},
  {"Round",    TagId::Round},
  {"White",    TagId::White},
  {"Black",    TagId::Black},
  {"Result",   TagId::Result},
  {"WhiteElo", TagId::WhiteElo},
  {"BlackElo", TagId::BlackElo},
};

// convert tag name string to TagID - case sensitive,
// will return TagId::Null if given unknown tag
TagId find_tag_id(std::string_view tag_name) {
  auto it = TagIdMap.find(tag_name);
  return it == TagIdMap.end() ? TagId::Null : it->second;
}

struct ParseStep {
  chess::Move move {};
  std::size_t bytes_read = 0;
  unsigned move_no = 0;
  chess::Position prev {}, next {};
};

template <class F> concept Predicate = std::invocable<F, const ParseStep &>;
template <class F> concept MoveVisitor = std::invocable<F, const ParseStep &>;
template <class F> concept TagVisitor = std::invocable<F, TagId, std::string_view>;
template <class F> concept ResultVisitor = std::invocable<F, chess::GameResult>;

} // cdb::format
