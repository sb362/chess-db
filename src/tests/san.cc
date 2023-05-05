#include "chess/notation.hh"
#include "chess/movegen.hh"

#include <fmt/format.h>

#include <iostream>
#include <string>

#include <catch2/catch_test_macros.hpp>

using namespace cdb::chess;

int test(std::string_view fen, std::string_view san, std::string_view exp) {
  std::cout << fmt::format("{:>32} {:>6} {:>32}\n", fen, san, exp);

  auto pos = Position::from_fen(fen);
  if (!pos) {
    std::cerr << "from_fen failed: " << pos.error().message() << "\n";
    return -1;
  }

  auto move = parse_san(san, *pos, !fen.contains('w'));
  if (!move) {
    std::cerr << "parse_san failed: " << move.error().message() << "\n";
    return -2;
  }

  pos = make_move(*pos, *move);
  auto got = pos->to_fen(fen.contains('w'));

  constexpr bool IgnoreEnpassant = true;
  if (IgnoreEnpassant) {
    got = got.substr(0, got.find_last_of(' '));
    exp = exp.substr(0, exp.find_last_of(' '));
  }

  if (got != exp) {
    std::cerr << "fen mismatch, got " << got << "\n";
    return -3;
  }

  return 0;
}

int test_exfl(std::string_view fen, std::string_view san) {
  std::cout << fmt::format("{:>32} {:>6} EXFL\n", fen, san);

  auto pos = Position::from_fen(fen);
  if (!pos) {
    std::cerr << "from_fen failed: " << pos.error().message() << "\n";
    return -1;
  }

  auto move = parse_san(san, *pos, fen.contains('b'));
  if (!move)
    return 0;

  std::cerr << "parse_san passed: " << move.error().message() << "\n";
  return -2;
}

TEST_CASE("basic SAN") {
  SECTION("best by test") {
    REQUIRE(test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", "e4",
                 "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq -") == 0);
  }

  SECTION("black pawn advancing two squares") {
    REQUIRE(test("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq -", "e5",
                 "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -") == 0);
  }

  SECTION("capture (bishop takes knight") {
    REQUIRE(test("r1bqkbnr/1ppp1ppp/p1n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -", "Bxc6",
                 "r1bqkbnr/1ppp1ppp/p1B5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq -") == 0);
  }

  SECTION("recapture (two pawns)") {
    REQUIRE(test("r1bqkbnr/1ppp1ppp/p1B5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq -", "dxc6",
                 "r1bqkbnr/1pp2ppp/p1p5/4p3/4P3/5N2/PPPP1PPP/RNBQK2R w KQkq -") == 0);
  }

  SECTION("two pawns on same file") {
    REQUIRE(test("8/R5p1/3k4/3n1bp1/1p6/1P3P1P/5KP1/8 b - -", "g6",
                 "8/R7/3k2p1/3n1bp1/1p6/1P3P1P/5KP1/8 w - -") == 0);
    
    REQUIRE(test("rn1rb1k1/2q2p1p/2P2p2/1P1p4/p2Np3/4P3/P3BPPP/2RQ1RK1 b - -", "f5",
                 "rn1rb1k1/2q2p1p/2P5/1P1p1p2/p2Np3/4P3/P3BPPP/2RQ1RK1 w - -") == 0);
  }

  SECTION("short castles (white)") {
    REQUIRE(test("rnbqkb1r/pp2pppp/5n2/2pp4/8/5NP1/PPPPPPBP/RNBQK2R w KQkq -", "O-O",
                 "rnbqkb1r/pp2pppp/5n2/2pp4/8/5NP1/PPPPPPBP/RNBQ1RK1 b kq -") == 0);
  }

  SECTION("en passant (black)") {
    REQUIRE(test("rnbqkb1r/pp2pppp/5n2/2p5/3pP3/5NP1/PPPP1PBP/RNBQ1RK1 b kq e3", "dxe3",
                 "rnbqkb1r/pp2pppp/5n2/2p5/8/4pNP1/PPPP1PBP/RNBQ1RK1 w kq -") == 0);
  }

  SECTION("ambiguous capture (files)") {
    REQUIRE(test("r1bqkb1r/pppppppp/5n2/2n5/3PP3/2N2N2/PPP2PPP/R1BQKB1R b KQkq -", "Ncxe4",
                 "r1bqkb1r/pppppppp/5n2/8/3Pn3/2N2N2/PPP2PPP/R1BQKB1R w KQkq -") == 0);
    
    REQUIRE(test_exfl("r1bqkb1r/pppppppp/5n2/2n5/3PP3/2N2N2/PPP2PPP/R1BQKB1R b KQkq -", "Nxe4") == 0);
  }
}
