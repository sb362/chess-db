#include "chess/notation.hh"
#include "chess/movegen.hh"
#include "chess/pgn.hh"
#include "core/error.hh"
#include "core/io.hh"
#include "util/bytesize.hh"

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace cdb;
using namespace cdb::chess;

std::size_t count_occurence(std::string_view haystack, std::string_view needle) {
  std::size_t i = 0;
  auto it = std::search(haystack.begin(), haystack.end(),
                        needle.begin(), needle.end());
  for (; it != haystack.end();
         it = std::search(it + needle.size(), haystack.end(),
                          needle.begin(), needle.end())) {
    ++i;
  }

  return i;
}

struct PerftResult {
  std::uint64_t dt, games, actual_games, errors, size;
  std::uint64_t nodes, unique_nodes;

  constexpr PerftResult &operator+=(PerftResult r) {
    dt += r.dt;
    games += r.games;
    actual_games += r.actual_games;
    errors += r.errors;
    size += r.size;

    nodes += r.nodes;
    return *this;
  }
};

struct string_hash {
  using is_transparent = void;
  using is_avalanching = void;

  [[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t {
    return ankerl::unordered_dense::hash<std::string_view>()(str);
  }
};

using namespace ankerl;
using TagValueMap = unordered_dense::map<std::string, std::uint32_t, string_hash, std::equal_to<>>;
using TagNameMap = unordered_dense::map<std::string, TagValueMap, string_hash, std::equal_to<>>;
static TagNameMap tag_name_map;

PerftResult count_games(std::string_view data) {
  using clock = std::chrono::high_resolution_clock;
  using std::chrono::microseconds;
  using namespace std::chrono_literals;
  
  PerftResult result {};

  const auto t0 = clock::now();
  auto r = parse_games(data, [] (auto name, auto value) {
    /*auto [it, inserted] = tag_name_map.try_emplace(name, TagValueMap());
    auto [it2, inserted2] = it->second.try_emplace(value, 1ul);
    if (!inserted2)
      it2->second += 1;*/
  },
                       [&] (const auto &step) { ++result.nodes; },
                       [&] (auto) { ++result.games; },
                       [&] (auto) { ++result.errors; }, true);
  result.dt = (clock::now() - t0) / 1us;
  result.size = data.size();

  if (r.ec) {
    std::cerr << "failed to parse game " << result.games << '\n';
    std::cerr << " err: " << r.ec.message() << '\n';
    std::cerr << " ctx: " << r.context << '\n';
    std::cerr << " pos: " << r.pos << '\n';
  }

  if (r.pos == 0) {
    std::cerr << "no bytes read?\n";
  }

  return result;
}

PerftResult pgn_perft(const std::string &path) {
#if 1
  std::string s;
  {
    std::ifstream ifs {path};
    std::stringstream ss;
    ss << ifs.rdbuf();
    s = ss.str();
    
  }
#else
  auto mm = io::mm_open(path);
  auto s = mm->str_view();
#endif

  auto r = count_games(s);
  r.actual_games = count_occurence(s, "[Event ");

  return r;
}

int main(int argc, char *argv[]) {
  namespace fs = std::filesystem;

  std::string path_str;
  if (argc == 1)
    path_str = "pgn";
  else if (argc == 2) {
    std::for_each(argv + 1, argv + argc, [&] (auto s) { path_str += s; path_str.push_back(' '); });
    path_str.pop_back();
  }

  fs::path path {path_str};

  PerftResult tr {};
  if (fs::is_directory(path)) {
    for (const auto &pgn_file : fs::recursive_directory_iterator(path)) {
      if (!pgn_file.is_regular_file()) continue;
      if (pgn_file.path().extension() != ".pgn") continue;

      const auto r = pgn_perft(pgn_file.path().string());
      tr += r;

      fmt::print("{: <40} {: <10} {:5d}/{:5d} ({:.2f}%, {} errors) in {} ms\n",
                 pgn_file.path().filename().string(),
                 best_size_unit {r.size}.string(),
                 r.games, r.actual_games,
                 (r.games / double(r.actual_games)) * 100,
                 r.errors, r.dt / 1'000);
    }
  } else if (fs::is_regular_file(path)) {
    fmt::print("{: <40}\n", path.filename().string());
    tr = pgn_perft(path.string());
  } else {
    std::cerr << "file/directory not found\n";
    return -1;
  }

  fmt::print("{: <40} {: <10} {:5d}/{:5d} ({:.2f}%, {} errors) in {} ms\n",
                 "total",
                 best_size_unit {tr.size}.string(),
                 tr.games, tr.actual_games,
                 (tr.games / double(tr.actual_games)) * 100,
                 tr.errors, tr.dt / 1'000);

  fmt::print("{} kgames/sec\n", tr.games / (tr.dt / (1'000'000)));
  fmt::print("{} knodes/sec\n", tr.nodes / (tr.dt / (1'000'000)));
  fmt::print("{} MB/sec\n", (tr.size / 1024 / 1024) / (tr.dt / (1'000'000)));

  for (const auto &[name, value] : tag_name_map) {
    fmt::print("{:<16} {}\n", name, value.size());
  }

  return 0;
}
