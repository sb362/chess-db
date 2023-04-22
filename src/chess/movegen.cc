
#include "chess/movegen.hh"

using namespace cdb;

std::uint64_t chess::perft(const Position pos, unsigned depth) {
  MoveList moves = movegen(pos);
  if (depth == 1) return moves.size();

  std::uint64_t count = 0;
  for (const Move &move : moves)
    count += perft(make_move(pos, move), depth - 1);

  return count;
}
