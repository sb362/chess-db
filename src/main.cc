
#include <fmt/format.h>

#include <iostream>
#include <fstream>
#include <sstream>

#include "chess/uchess.h"
#include "pgn/parser.hh"

using namespace pgn;

int main(int, char *[])
{
	init_bitbase();

	std::ifstream input_file {"test.pgn"};
	std::stringstream ss;
	ss << input_file.rdbuf();

	std::string input_pgn = ss.str();

	fmt::print("{}\n", input_pgn);

	PositionState pos = {Startpos, WHITE, 0, 0};
	pos = do_move(pos, Move {12, 28, Pawn, 0});
	
	char fen[256] = {0};
	generate_fen(pos, fen);
	fmt::print("{:x}\n{:x}\n", pos.pos.white, occupied(pos.pos));
	fmt::print("{}\n", fen);

	const char *san = "Nf6";
	bool ok = false;
	const Move move = parse_san(san, strlen(san), pos, &ok);
	fmt::print("{}\n", ok);
	fmt::print("{:06b} {:06b} {:03b} {:01b}\n", move.start, move.end, move.piece, move.castling);

	pos = do_move(pos, move);
	
	char fen2[256] = {0};
	generate_fen(pos, fen2);
	fmt::print("{:x}\n{:x}\n", pos.pos.white, occupied(pos.pos));
	fmt::print("{}\n", fen2);


	/*auto game = std::make_unique<Game>();
	parse_game(input_pgn, game.get());

	GameExporter exporter {stdout};
	exporter.visit_game(game.get());*/

	return 0;
}
