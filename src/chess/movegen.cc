
#include "chess/movegen.hh"

#include <chrono>
#include <doctest.h>
#include "bench/nanobench.h"
#include "chess/notation.hh"

using namespace ankerl;

using namespace cdb;
using namespace cdb::chess;

std::uint64_t chess::perft(const Position pos, unsigned depth) {
  MoveList moves = movegen(pos);
  if (depth == 1) return moves.size();

  std::uint64_t count = 0;
  for (const Move &move : moves)
    count += perft(make_move(pos, move), depth - 1);

  return count;
}

TEST_SUITE("perft") {
  TEST_CASE("startpos") {
    constexpr std::string_view fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    const auto pos = Position::from_fen(fen);
    REQUIRE(pos);

    REQUIRE(perft(*pos, 1) == 20);
    REQUIRE(perft(*pos, 2) == 400);
    REQUIRE(perft(*pos, 3) == 8902);
    REQUIRE(perft(*pos, 4) == 197281);
    REQUIRE(perft(*pos, 5) == 4865609);
    REQUIRE(perft(*pos, 6) == 119060324);
    //REQUIRE(perft(*pos, 7) == 3195901860);
  }

  TEST_CASE("Kiwipete") {
    constexpr std::string_view fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    const auto pos = Position::from_fen(fen);
    REQUIRE(pos);

    REQUIRE(perft(*pos, 1) == 48);
    REQUIRE(perft(*pos, 2) == 2039);
    REQUIRE(perft(*pos, 3) == 97862);
    REQUIRE(perft(*pos, 4) == 4085603);
    REQUIRE(perft(*pos, 5) == 193690690);
    //REQUIRE(perft(*pos, 6) == 8031647685);
  }

  TEST_CASE("CPW #3") {
    constexpr std::string_view fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    const auto pos = Position::from_fen(fen);
    REQUIRE(pos);

    REQUIRE(perft(*pos, 1) == 14);
    REQUIRE(perft(*pos, 2) == 191);
    REQUIRE(perft(*pos, 3) == 2812);
    REQUIRE(perft(*pos, 4) == 43238);
    REQUIRE(perft(*pos, 5) == 674624);
    REQUIRE(perft(*pos, 6) == 11030083);
    REQUIRE(perft(*pos, 7) == 178633661);
    //REQUIRE(perft(*pos, 8) == 3009794393);
  }

  TEST_CASE("CPW #4A") {
    constexpr std::string_view fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -";
    const auto pos = Position::from_fen(fen);
    REQUIRE(pos);

    REQUIRE(perft(*pos, 1) == 6);
    REQUIRE(perft(*pos, 2) == 264);
    REQUIRE(perft(*pos, 3) == 9467);
    REQUIRE(perft(*pos, 4) == 422333);
    REQUIRE(perft(*pos, 5) == 15833292);
    REQUIRE(perft(*pos, 6) == 706045033);
  }

  TEST_CASE("CPW #4B") {
    constexpr std::string_view fen = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -";
    const auto pos = Position::from_fen(fen);
    REQUIRE(pos);

    REQUIRE(perft(*pos, 1) == 6);
    REQUIRE(perft(*pos, 2) == 264);
    REQUIRE(perft(*pos, 3) == 9467);
    REQUIRE(perft(*pos, 4) == 422333);
    REQUIRE(perft(*pos, 5) == 15833292);
    REQUIRE(perft(*pos, 6) == 706045033);
  }

  TEST_CASE("CPW #5") {
    constexpr std::string_view fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -";
    const auto pos = Position::from_fen(fen);
    REQUIRE(pos);

    REQUIRE(perft(*pos, 1) == 44);
    REQUIRE(perft(*pos, 2) == 1486);
    REQUIRE(perft(*pos, 3) == 62379);
    REQUIRE(perft(*pos, 4) == 2103487);
    REQUIRE(perft(*pos, 5) == 89941194);
  }

  TEST_CASE("CPW #6") {
    constexpr std::string_view fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -";
    const auto pos = Position::from_fen(fen);
    REQUIRE(pos);

    REQUIRE(perft(*pos, 1) == 46);
    REQUIRE(perft(*pos, 2) == 2079);
    REQUIRE(perft(*pos, 3) == 89890);
    REQUIRE(perft(*pos, 4) == 3894594);
    REQUIRE(perft(*pos, 5) == 164075551);
    REQUIRE(perft(*pos, 6) == 6923051137);
    //REQUIRE(perft(*pos, 7) == 287188994746);
  }

  TEST_CASE("Promotions") {
    constexpr std::string_view fen = "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - -";
    const auto pos = Position::from_fen(fen);
    REQUIRE(pos);

    REQUIRE(perft(*pos, 1) == 24);
    REQUIRE(perft(*pos, 2) == 496);
    REQUIRE(perft(*pos, 3) == 9483);
    REQUIRE(perft(*pos, 4) == 182838);
    REQUIRE(perft(*pos, 5) == 3605103);
    REQUIRE(perft(*pos, 6) == 71179139);
  }
}

#define BENCHMARK(name, fen)                                          \
  TEST_CASE(name) {                                                   \
    const auto pos = *Position::from_fen(fen);                        \
    b.run(name, [&] { nanobench::doNotOptimizeAway(movegen(pos)); }); \
  }

TEST_SUITE("bench-movegen") {
  nanobench::Bench b;

  TEST_CASE("") {
    b.title("movegen");
    b.performanceCounters(true);
  }

  BENCHMARK("Startpos",   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
  BENCHMARK("Kiwipete",   "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
  BENCHMARK("CPW #3",     "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
  BENCHMARK("CPW #4A",    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -");
  BENCHMARK("CPW #4B",    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -");
  BENCHMARK("CPW #5",     "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -");
  BENCHMARK("CPW #6",     "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -");
  BENCHMARK("Promotions", "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - -");
}
#undef BENCHMARK

#define BENCHMARK(name, depth, fen)                                        \
  TEST_CASE(name) {                                                        \
    const auto pos = *Position::from_fen(fen);                             \
    b.run(name, [&] { nanobench::doNotOptimizeAway(perft(pos, depth)); }); \
  }

TEST_SUITE("bench-perft") {
  nanobench::Bench b;

  TEST_CASE("") {
    b.title("perft");
    b.performanceCounters(true);
  }

  BENCHMARK("Startpos",   5, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
  BENCHMARK("Kiwipete",   4, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
  BENCHMARK("CPW #3",     5, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
  BENCHMARK("CPW #4A",    4, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -");
  BENCHMARK("CPW #4B",    4, "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -");
  BENCHMARK("CPW #5",     4, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -");
  BENCHMARK("CPW #6",     4, "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -");
  BENCHMARK("Promotions", 4, "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - -");
}
#undef BENCHMARK

#define BENCHMARK(name, san, fen, black)            \
  TEST_CASE(name) {                                 \
    const auto pos_ = Position::from_fen(fen);      \
    REQUIRE(pos_);                                  \
                                                    \
    const auto pos = *pos_;                         \
    const auto move_ = parse_san(san, pos, black);  \
    REQUIRE(move_);                                 \
                                                    \
    const auto move = *move_;                       \
                                                    \
    b.run(name, [&] { nanobench::doNotOptimizeAway(make_move(pos, move)); });     \
  }

TEST_SUITE("bench-make-move") {
  nanobench::Bench b;

  TEST_CASE("") {
    b.title("make_move");
    b.performanceCounters(true);
  }

  BENCHMARK("startpos",     "e4",     "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", false);
  BENCHMARK("kiwipete",     "O-O-O",  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", false);
  BENCHMARK("en passant",   "dxc6",   "r3k2r/p2pqpb1/bn2pnp1/2pPN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq c6", false);
  BENCHMARK("promotions Q", "bxc8=Q", "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", false);
  BENCHMARK("promotions N", "bxa8=N", "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", false);
  BENCHMARK("no castle",    "Kxf2",   "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", false);
  BENCHMARK("castle",       "O-O",    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", false);
}
#undef BENCHMARK

#define BENCHMARK(name, san, fen, black)                                             \
  TEST_CASE(name) {                                                                  \
    const auto pos = *Position::from_fen(fen);                                       \
    b.run(name, [&] { nanobench::doNotOptimizeAway(*parse_san(san, pos, black)); }); \
  }

TEST_SUITE("bench-parse_san") {
  nanobench::Bench b;

  TEST_CASE("") {
    b.title("parse_san");
    b.performanceCounters(true);
  }

  BENCHMARK("startpos",     "e4",     "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", false);
  BENCHMARK("kiwipete",     "O-O-O",  "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -", false);
  BENCHMARK("en passant",   "dxc6",   "r3k2r/p2pqpb1/bn2pnp1/2pPN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq c6", false);
  BENCHMARK("promotions Q", "bxc8=Q", "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", false);
  BENCHMARK("promotions N", "bxa8=N", "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", false);
  BENCHMARK("no castle",    "Kxf2",   "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", false);
  BENCHMARK("castle",       "O-O",    "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -", false);
}
#undef BENCHMARK
