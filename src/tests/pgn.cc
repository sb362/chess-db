#include "chess/notation.hh"
#include "chess/movegen.hh"
#include "chess/pgn.hh"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>

using namespace cdb::chess;

std::uint64_t count_games(const std::string &data) {
  return data.size();
}

struct PerftResult {
  std::uint64_t games, dt;

  constexpr PerftResult &operator+=(const PerftResult &r) {
    games += r.games, dt += r.dt;
    return *this;
  }
};

PerftResult pgn_perft(const std::string &path) {
  using clock = std::chrono::high_resolution_clock;
  using std::chrono::microseconds;
  using namespace std::chrono_literals;

  std::ifstream ifs {path};
  std::stringstream ss;
  ss << ifs.rdbuf();

  PerftResult r {0, 0};
  const auto t0 = clock::now();
  r.games = count_games(ss.str());
  r.dt = (clock::now() - t0) / 1us;
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
    for (const auto &pgn_file : fs::directory_iterator(path)) {
      if (!pgn_file.is_regular_file()) continue;

      std::cout << pgn_file.path() << " (" << (pgn_file.file_size() / 1024) << " KiB)\n";
      const auto r = pgn_perft(pgn_file.path().string());
      tr += r;

      std::cout << std::format("{} games in {} ms\n", r.games, r.dt);
    }
  } else if (fs::is_regular_file(path)) {
    std::cout << path << " (" << (fs::file_size(path) / 1024) << " KiB)\n";
    tr = pgn_perft(path.string());
  } else {
    std::cerr << "file/directory not found\n";
    return -1;
  }

  std::cout << std::format("\n{} games in {} ms\n", tr.games, tr.dt);
  return 0;
}
