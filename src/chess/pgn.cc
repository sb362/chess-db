
#include "bench/nanobench.h"
#include "chess/pgn.hh"

#include <doctest.h>

#include <fstream>

using namespace ankerl;

using namespace cdb;
using namespace cdb::chess;

namespace {
  std::size_t count_occurence(std::string_view haystack, std::string_view needle) {
    auto search = [&] (auto it) {
      return std::search(it, haystack.end(), needle.begin(), needle.end());
    };

    std::size_t i = 0;
    for (auto it = search(haystack.begin());
        it != haystack.end();
        it = search(it + needle.size())) {
      ++i;
    }

    return i;
  }

  std::pair<std::string, std::size_t> get_pgn_data(const std::string &path) {
    if (std::ifstream ifs {path}; ifs.good()) {
      std::stringstream ss;
      ss << ifs.rdbuf();

      std::string s = ss.str();

      // Assumes there is one "[Event " per game, which in general is not true,
      // since PGN comments can contain PGN data.
      return {s, count_occurence(s, "[Event ")};
    } else {
      return {"", 0};
    }
  }
}

#define USE_BYTES

#define BENCHMARK(name, path)                             \
  TEST_CASE(name) {                                       \
    std::size_t actual_games;                             \
    auto [data, exp_games] = get_pgn_data(path);          \
    b.batch(data.size()); b.unit("bytes");                \
    b.batch(exp_games); b.unit("games");                  \
    b.run(name, [&] {                                     \
      actual_games = 0;                                   \
      nanobench::doNotOptimizeAway(                       \
        parse_games(data, [] (auto, auto) {},             \
                          [] (const auto &) {},           \
                          [&] (auto) { ++actual_games; }, \
                          [] (auto) {}                    \
      ));                                                 \
    });                                                   \
    REQUIRE(actual_games == exp_games);                   \
  }

TEST_SUITE("bench-pgn") {
  nanobench::Bench b;

  TEST_CASE("") {
    b.title("pgn");
  }

  BENCHMARK("Lichess Elite 2013-09", "./pgn/lichess-elite/lichess_elite_2013-09.pgn");
  BENCHMARK("Lichess Elite 2016-01", "./pgn/lichess-elite/lichess_elite_2016-01.pgn");
  BENCHMARK("Lichess Elite 2018-08", "./pgn/lichess-elite/lichess_elite_2018-08.pgn");
  BENCHMARK("Lichess Elite 2019-05", "./pgn/lichess-elite/lichess_elite_2019-05.pgn");

}
#undef BENCHMARK
