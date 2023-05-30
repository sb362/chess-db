
#include "chess/movegen.hh"
#include "chess/pgn.hh"
#include "core/byteio.hh"
#include "core/error.hh"
#include "util/bytehist.hh"
#include "util/bytesize.hh"
#include "util/unordered_dense.h"

#include <fmt/format.h>

#include <fstream>
#include <random>
#include <sstream>

using namespace cdb;

template <>
struct ankerl::unordered_dense::hash<chess::Position> {
  using is_avalanching = void;

  std::uint64_t operator()(chess::Position const &pos) const noexcept {
    return detail::wyhash::hash(&pos, sizeof pos);
  }
};

struct string_hash {
  using is_transparent = void;
  using is_avalanching = void;

  [[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t {
    return ankerl::unordered_dense::hash<std::string_view>()(str);
  }
};

struct TagInfo {
  std::byte id; // map tag name to a single byte
  
  void (*serialiser)(std::string_view, io::mutable_buffer &);

  constexpr TagInfo(std::byte id, decltype(serialiser) serialiser)
    : id(id), serialiser(serialiser)
  {
  }
};

class PRNG {
  uint64_t s;
  uint64_t rand64() {
    s ^= s >> 12, s ^= s << 25, s ^= s >> 27;
    return s * 2685821657736338717LL;
  }

public:
  PRNG(uint64_t seed) : s(seed) { assert(seed); }

  template <class T = std::uint64_t> T rand() { return T(rand64()); }
};

#include <x86intrin.h>

int main(int argc, char *argv[]) {

  io::mutable_buffer out(1024ull * 1024 * 512);
  
  std::string pgn_data;
  {
    std::ifstream ifs {"./pgn/lichess-elite/lichess_elite_2021-10.pgn"};
    std::stringstream ss;
    ss << ifs.rdbuf();
    pgn_data = ss.str();
  }

  using namespace ankerl;
  using TagValueMap = unordered_dense::map<std::string, std::uint32_t, string_hash, std::equal_to<>>;
  using TagNameMap = unordered_dense::map<std::string, TagValueMap, string_hash, std::equal_to<>>;
  TagNameMap tag_name_map;

  auto r = chess::parse_games(pgn_data,
    [&] (auto name, auto value) {
      auto [it, inserted] = tag_name_map.try_emplace(name, TagValueMap());
      auto [it2, inserted2] = it->second.try_emplace(value, 1ul);
      if (!inserted2)
        it2->second += 1;
    },
    [&] (const chess::ParseStep &step) {
      auto moves = chess::movegen(step.prev);
      const int move_idx = moves.index_of(step.move);

      out.write_byte(static_cast<std::byte>(move_idx));
    },
    [] (auto result) {
    },
    [] (auto result) {
      fmt::print("err: {}\n", result.ec.message());
      fmt::print("ctx: {}\n", result.context);
      fmt::print("pos: {}\n", result.pos);
    }, true
  );

  for (const auto &[name, value] : tag_name_map) {
    fmt::print("{:<16} {}\n", name, value.size());
  }

  return 0;
}
