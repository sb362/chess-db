
#include "bench/nanobench.h"
#include "chess/pgn.hh"

#include <doctest.h>

#include <fstream>

using namespace ankerl;

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

std::pair<std::string, std::size_t> get_pgn_data(const std::string &path) {
  std::ifstream ifs {path};
  std::stringstream ss;
  ss << ifs.rdbuf();
  std::string s = ss.str();

  return {s, count_occurence(s, "[Event ")};
}

#define USE_BYTES

#define BENCHMARK(name, path)                           \
  TEST_CASE(name) {                                     \
    std::size_t actual_games;                           \
    auto [data, exp_games] = get_pgn_data(path);        \
    b.batch(data.size()); b.unit("bytes");              \
    b.batch(exp_games); b.unit("games");                \
    b.run(name, [&] {                                   \
    nanobench::doNotOptimizeAway(                       \
      parse_games(data, [] (auto, auto) {},             \
                        [] (const auto &) {},           \
                        [&] (auto) { ++actual_games; }, \
                        [] (auto) {}                    \
      ));                                               \
    });                                                 \
  }

TEST_SUITE("bench-pgn") {
  nanobench::Bench b;

  TEST_CASE("") {
    b.title("pgn");
  }

  BENCHMARK("Lichess Elite 2013-09", "./pgn-elite/lichess_elite_2013-09.pgn");
  BENCHMARK("Lichess Elite 2016-01", "./pgn-elite/lichess_elite_2016-01.pgn");
  BENCHMARK("Lichess Elite 2018-08", "./pgn-elite/lichess_elite_2018-08.pgn");
  BENCHMARK("Lichess Elite 2020-05", "./pgn-elite/lichess_elite_2020-05.pgn");
  BENCHMARK("Lichess Elite 2021-11", "./pgn-elite/lichess_elite_2021-11.pgn");
  BENCHMARK("Lichess Fripouille",    "./pgn/lichess_frip.pgn");
  BENCHMARK("TWIC 1334",             "./pgn/twic1334.pgn");
  BENCHMARK("TWIC 1333",             "./pgn/twic1333.pgn");
  BENCHMARK("KingBase2019 E00-E19",  "./pgn-otb-big/KingBase2019-E00-E19.pgn");
  BENCHMARK("KingBase2019 D70-D99",  "./pgn-otb-big/KingBase2019-D70-D99.pgn");
  BENCHMARK("Lichess realPioneer",   "./pgn-lich/lichess_realPioneer_2022-10-09.pgn");
  BENCHMARK("Lichess allegro",       "./pgn-lich/lichess_swiss_2021.04.22_ODPOqRGJ_tafca-allegro-standrews-uni.pgn");
  BENCHMARK("OTB Borwell",           "./pgn-otb-small/borwell, alan p.pgn");

}
#undef BENCHMARK
