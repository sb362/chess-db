#pragma once

#include "game.hh"

#include <string_view>

namespace pgn
{

void init();

int parse_game(std::string_view str, Game *game);
int parse_headers(std::string_view str, Headers &headers);
int parse_movetext(std::string_view str, GameNode *game);

} // pgn
