#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "bits.h"
#include "movegen.h"
#include "position.h"
#include "text.h"

// Unit Tests:
// https://www.chessprogramming.org/Perft_Results

struct UnitTest {
	const char *name;
	const char *fen;
	size_t depth, result;
};

struct UnitTest tests[] = {
	{ .name = "startpos", .fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
	  .depth = 6, .result = 119060324 },

	{ .name  = "kiwipete", .fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
	  .depth = 5, .result = 193690690 },

	{ .name = "position 3", .fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
	  .depth = 7, .result = 178633661 },

	{ .name = "position 4", .fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
	  .depth = 6, .result = 706045033 },

	{ .name = "position 5", .fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
	  .depth = 5, .result =  89941194 },

	{ .name = "position 6", .fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
	  .depth = 5, .result = 164075551 },
};

static
size_t perft(struct Position pos, size_t depth) {
	if (depth == 0) return 1;

	struct MoveList list = generate_moves(pos);
	if (depth == 1) return list.length;

	size_t total = 0;

	for (size_t i = 0; i < list.length; i++) {
		struct Position child = make_move(pos, list.moves[i]);
		total += perft(child, depth - 1);
	}

	return total;
}

static
void run_test(struct UnitTest test) {
	// test reading fen
	bool ok;
	struct PositionState state = parse_fen(test.fen, &ok, stderr);
	assert(ok);

	// test writing fen
	char buffer[1024];
	size_t length = generate_fen(state, buffer);
	assert(strncmp(test.fen, buffer, strlen(test.fen)) == 0);
	(void)length;

	// test move generation
	clock_t start = clock();
	size_t result = perft(state.pos, test.depth);
	clock_t end = clock();

	assert(result == test.result);

	// print benchmark
	double seconds = (double)(end - start) / CLOCKS_PER_SEC;
	double mnps = (result / seconds) / 1e6;

	printf("%s\t| %zu\t| %.3f Mnps\n", test.name, result, mnps);
}

int main() {
	init_bitbase();

	int count = sizeof tests / sizeof tests[0];

	for (int i = 0; i < count; i++) {
		run_test(tests[i]);
	}
}
