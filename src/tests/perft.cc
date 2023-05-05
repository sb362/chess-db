#include "chess/movegen.hh"

#include <array>
#include <chrono>
#include <format>
#include <iostream>
#include <string_view>

struct Test {
  std::string_view name, fen;
  std::array<std::uint64_t, 8> counts;
  unsigned depth;
};

static constexpr std::array<Test, 8> tests =
{{
	//
	// Source: https://www.chessprogramming.org/Perft_Results
	//
	{
		"Startpos",
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
		{20, 400, 8902, 197281, 4865609, 119060324, 3195901860, 84998978956}, 6
	},
	{
		"Kiwipete",
		"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
		{48, 2039, 97862, 4085603, 193690690, 8031647685}, 5
	},
	{
		"CPW #3",
		"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
		{14, 191, 2812, 43238, 674624, 11030083, 178633661, 3009794393}, 7
	},
	{
		"CPW #4A",
		"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -",
		{6, 264, 9467, 422333, 15833292, 706045033}, 6
	},
	{
		"CPW #4B",
		"r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -",
		{6, 264, 9467, 422333, 15833292, 706045033}, 6
	},
	{
		"CPW #5",
		"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",
		{44, 1486, 62379, 2103487, 89941194}, 5
	},
	{
		"CPW #6",
		"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -",
		{46, 2079, 89890, 3894594, 164075551, 6923051137, 287188994746}, 5
	},
	
	//
	// Source: http://www.rocechess.ch/perft.html
	//
	{
		"Promotions",
		"n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - -",
		{24, 496, 9483, 182838, 3605103, 71179139}, 6
	}
}};

int main(int, char *[]) {
  using clock = std::chrono::high_resolution_clock;
  using std::chrono::microseconds;
  using namespace std::chrono_literals;
  using namespace cdb::chess;

  std::uint64_t avg = 0, runs = 0;

  constexpr std::string_view row_fmt = "{:<8} {:<5} {:<10} {:<10} {:<10}\n";
  std::cout << fmt::format(row_fmt, "position", "depth", "count", "time (μs)", "speed (Mnps)");

  for (const auto &test : tests) {
    const auto pos = Position::from_fen(test.fen);
	if (!pos) {
		std::cerr << "failed to parse FEN: " << test.fen << std::endl;
		return -2;
	}

    for (unsigned depth = 1; depth <= test.depth; ++depth) {
      const auto t0 = clock::now();
      const auto count = perft(*pos, depth);
      const auto dt = (clock::now() - t0) / 1us;
      const auto nps = dt == 0 ? count : (count / dt);
      std::cout << fmt::format(row_fmt, test.name, depth, count, dt, nps);

      if (count != test.counts[depth - 1]) {
        std::cerr << " Expected " << test.counts[depth - 1] << "!";
        return -1;
      }

      if (depth == test.depth) {
        avg  += nps;
        runs += 1;
      }
    }

    std::cout << std::endl;
  }

  avg /= runs;

  std::cout << avg << " Mnps" << std::endl;
  return 0;
}
