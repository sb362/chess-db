#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace cdb::chess {

struct Position;
struct Move;

std::optional<Move> parse_san(std::string_view san, Position pos, bool black);
std::string to_san(Move move, Position pos, bool black);

} // cdb::chess
