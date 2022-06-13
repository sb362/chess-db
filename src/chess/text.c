#include "text.h"

#include "bits.h"
#include "movegen.h"
#include "position.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF(fmt, args) __attribute__((format (printf, fmt, args)))
#else
#define PRINTF(fmt, args)
#endif

#define RED	"\033[31;1m"
#define RESET	"\033[0m"

struct Parser {
	FILE *output;
	const char *fen;
	const char *offset;
};

static inline
char peek_next(struct Parser *parser) {
	return *parser->offset;
}

static inline
char chop_next(struct Parser *parser) {
	return *parser->offset++;
}

static inline
int chop_int(struct Parser *parser) {
	char *endptr;
	int val = strtol(parser->offset, &endptr, 10);

	if (endptr == parser->offset) {
		return 0;
	}

	parser->offset = endptr;
	return val;
}

static
enum PieceType lookup[0x80] = {
	['p'] = Pawn,
	['n'] = Knight,
	['b'] = Bishop,
	['r'] = Rook,
	['q'] = Queen,
	['k'] = King,
};


static inline PRINTF(2,3)
void log_error(struct Parser *parser, const char *fmt, ...) {
	// silence output on NULL
	if (parser->output == NULL)
		return;

	fprintf(parser->output, RED "error: " RESET);

	va_list args;
	va_start(args, fmt);
	vfprintf(parser->output, fmt, args);
	va_end(args);

	// print context
	fprintf(parser->output, "\n  fen | \"%s\"", parser->fen);
	fprintf(parser->output, "\n      |  ");

	const char *offset = parser->offset;

	while (offset --> parser->fen) {
		fputc(' ', parser->output);
	}

	fprintf(parser->output, "^\n\n");
}

static inline
void set_square(struct Position *pos, int sq, enum PieceType T) {
	ASSERT(0 <= sq && sq < 64 && "invalid square");

	pos->X |= (bitboard)((T >> 0) & 1) << sq;
	pos->Y |= (bitboard)((T >> 1) & 1) << sq;
	pos->Z |= (bitboard)((T >> 2) & 1) << sq;
}


static inline
enum PieceType get_square(struct Position pos, int sq) {
	ASSERT(0 <= sq && sq < 64 && "invalid square");
	enum PieceType T = None;

	T |= ((pos.X >> sq) & 1) << 0;
	T |= ((pos.Y >> sq) & 1) << 1;
	T |= ((pos.Z >> sq) & 1) << 2;

	return T;
}

struct PositionState parse_fen(const char *fen, bool *ok, FILE *stream) {
	struct Parser parser = {
		.fen = fen,
		.offset = fen,
		.output = stream,
	};

	struct Position pos = {0};
	struct PositionState state = {0};

	bitboard white = 0, black = 0, info = 0;
	int sq = 56, file = 0;

	// parse board
	while (sq != 8 || file != 8) {
		unsigned char c = chop_next(&parser);
		unsigned char lower = 0x20;

		if (file > 8) {
			log_error(&parser, "rank description is more than 8 squares");
			goto error;
		}

		// end of rank
		if (file == 8) {
			if (c != '/') {
				log_error(&parser, "expected / after end of rank description");
				goto error;
			}

			file = 0, sq += S+S;
			continue;
		}

		// empty squares
		if ('1' <= c && c <= '8') {
			int offset = c - '0';
			sq += offset, file += offset;
		}

		// piece
		else {
			enum PieceType piece = lookup[c | lower];

			if (piece == None) {
				log_error(&parser, "invalid pieces type; must be one of PNBRQK");
				goto error;
			}

			if (c & lower) black |= 1ULL << sq;
			else           white |= 1ULL << sq;

			set_square(&pos, sq, piece);
			sq++, file++;
		}
	}

	if (chop_next(&parser) != ' ') {
		log_error(&parser, "expected space before side-to-move");
		goto error;
	}

	// side-to-move
	switch (chop_next(&parser)) {
		case 'w': state.side_to_move = WHITE; pos.white = white; break;
		case 'b': state.side_to_move = BLACK; pos.white = black; break;

		default:
			log_error(&parser, "expected w or b for side-to-move");
			goto error;
	}

	if (chop_next(&parser) != ' ') {
		log_error(&parser, "expected space before castling rights");
		goto error;
	}

	// castling rights
	if (peek_next(&parser) == '-') {
		chop_next(&parser);
	}

	else while (peek_next(&parser) != ' ') {
		switch (chop_next(&parser)) {
			case 'K': info |= WK_MASK; break;
			case 'Q': info |= WQ_MASK; break;
			case 'k': info |= BK_MASK; break;
			case 'q': info |= BQ_MASK; break;

			default:
				log_error(&parser, "expected one of KQkq for castling rights");
				goto error;
		}

		if (state.side_to_move == BLACK) {
			// swap castling sides
			info = ((info << 2) | (info >> 2)) & CA_MASK;
		}
	}

	if (chop_next(&parser) != ' ') {
		log_error(&parser, "expected space before en-passant square");
		goto error;
	}

	// en-passant square
	if (peek_next(&parser) == '-') {
		chop_next(&parser);
	}

	else {
		unsigned file = chop_next(&parser) - 'a';

		if (file >= 8) {
			log_error(&parser, "invalid file in en-passant square");
			goto error;
		}

		unsigned rank = chop_next(&parser) - '1';

		if (rank >= 8) {
			log_error(&parser, "invalid rank in en-passant square");
			goto error;
		}

		unsigned valid_rank = (state.side_to_move == WHITE) ? 5 : 2; // 6th or 3rd rank

		if (rank != valid_rank) {
			log_error(&parser, "invalid en-passant square; not on correct rank");
			goto error;
		}

		// set en-passant bit
		info |= 1 << file;
	}

	// half-move clock and full-move number are optional
	if (peek_next(&parser) != '\0') {
		if (chop_next(&parser) != ' ') {
			log_error(&parser, "expected space before half-move clock");
			goto error;
		}
	}

	state.fifty_move_clock = chop_int(&parser);

	if (state.fifty_move_clock > 100) {
		log_error(&parser, "fifty move clock must in range 0..100");
	}

	if (peek_next(&parser) != '\0') {
		if (chop_next(&parser) != ' ') {
			log_error(&parser, "expected space before full-move number");
			goto error;
		}
	}

	state.movenumber = chop_int(&parser);

	// write info bits
	bitboard occ = white | black;
	info = pdep(info, ~occ);

	pos.X |= info;
	pos.Y |= info;
	pos.Z |= info;

	state.pos = pos;

	*ok = true;
	return state;

error:
	*ok = false;
	return state;
}


static inline
void write_char(char **buffer, size_t *count, char c) {
	if (*buffer) {
		**buffer = c;
		*buffer += 1;
	}

	*count += 1;
}

static inline
void write_string(char **buffer, size_t *count, const char *str) {
	size_t length = strlen(str);

	if (*buffer) {
		memcpy(*buffer, str, length);
		*buffer += length;
	}

	*count += length;
}

static inline
void write_unsigned(char **buffer, size_t *count, unsigned x) {
	enum { MAX_DIGITS = 10 };

	char tmp[MAX_DIGITS];
	size_t length = 0;

	do { tmp[length++] = (x % 10) + '0'; } while(x /= 10);
	*count += length;

	if (*buffer) {
		while (length --> 0) {
			**buffer = tmp[length];
			*buffer += 1;
		}
	}
}

size_t generate_fen(struct PositionState state, char *buffer) {
	size_t count = 0;

	bitboard occ = occupied(state.pos);
	bitboard white, black;

	if (state.side_to_move == WHITE) {
		white = state.pos.white;
		black = occ & ~white;
	} else {
		black = state.pos.white;
		white = occ & ~black;
	}

	bitboard info = extract(state.pos, Info);
	info = pext(info, ~occ);

	// write board
	for (int rank = 7; rank >= 0; rank--) {
		int empty = 0;

		for (int file = 0; file < 8; file++) {
			int square = 8*rank + file;

			// flip square if reversed
			if (state.side_to_move == BLACK) square ^= 56;

			enum PieceType piece = get_square(state.pos, square);
			bitboard mask = 1ULL << square;

			if (piece == None || piece == Info) {
				empty++;
			}

			else {
				if (empty) {
					write_char(&buffer, &count, empty + '0');
					empty = 0;
				}

				unsigned char c = " PNBRQK "[piece];
				if (mask & black) c |= 0x20; // lower case

				write_char(&buffer, &count, c);
			}
		}

		if (empty) {
			write_char(&buffer, &count, empty + '0');
		}

		if (rank != 0) {
			write_char(&buffer, &count, '/');
		}
	}

	// write side-to-move
	write_char(&buffer, &count, ' ');
	write_char(&buffer, &count, state.side_to_move == WHITE ? 'w' : 'b');

	// write castling rights
	write_char(&buffer, &count, ' ');

	if ((info & CA_MASK) == 0) {
		write_char(&buffer, &count, '-');
	}

	else {
		bitboard castling = info & CA_MASK;

		if (state.side_to_move == BLACK) {
			// flip castling rights
			castling = ((castling << 2) | (castling >> 2)) & CA_MASK;
		}

		if (castling & WK_MASK) write_char(&buffer, &count, 'K');
		if (castling & WQ_MASK) write_char(&buffer, &count, 'Q');
		if (castling & BK_MASK) write_char(&buffer, &count, 'k');
		if (castling & BQ_MASK) write_char(&buffer, &count, 'q');
	}

	// write en-passant square
	write_char(&buffer, &count, ' ');

	if ((info & EP_MASK) == 0) {
		write_char(&buffer, &count, '-');
	} else {
		write_char(&buffer, &count, lsb(info) + 'a');
		write_char(&buffer, &count, state.side_to_move == WHITE ? '6' : '3');
	}

	// write half-move clock
	write_char(&buffer, &count, ' ');
	write_unsigned(&buffer, &count, state.fifty_move_clock);

	// write full move number
	write_char(&buffer, &count, ' ');
	write_unsigned(&buffer, &count, state.movenumber);

	return count;
}

static inline
bool more_than_one(bitboard bb) {
	return (bb & (bb - 1)) != 0;
}


size_t generate_san(struct Move move, struct PositionState state,
					char *buffer, bool check_and_mate) {
	size_t count = 0;

	// handle castling separately
	if (move.castling) {
		if (move.end > move.start) {
			write_string(&buffer, &count, "O-O");
		} else {
			write_string(&buffer, &count, "O-O-O");
		}
	}

	else {
		enum PieceType piece = get_square(state.pos, move.start);
		bool capture = get_square(state.pos, move.end) != None;

		// flip squares
		if (state.side_to_move == BLACK) {
			move.start ^= 56;
			move.end ^= 56;
		}

		if (piece == Pawn) {
			// capture
			if ((move.start & 7) != (move.end & 7)) {
				char file = move.start & 7;

				write_char(&buffer, &count, file + 'a');
				write_char(&buffer, &count, 'x');
			}

			char file = move.end & 7;
			char rank = move.end >> 3;

			write_char(&buffer, &count, file + 'a');
			write_char(&buffer, &count, rank + '1');

			// promotion
			if (move.piece != Pawn) {
				write_char(&buffer, &count, '=');
				write_char(&buffer, &count, "--NBRQ--"[move.piece]);
			}
		}

		else {
			// write piece
			write_char(&buffer, &count, "--NBRQK-"[piece]);

			// differentiator for multiple possible moves
			bitboard pieces = extract(state.pos, piece);
			bitboard occ = occupied(state.pos);

			bitboard possible = generic_attacks(piece, move.end, occ);
			possible &= pieces;

			// more than two possible pieces
			if (more_than_one(possible)) {
				int file = move.start & 7;
				int rank = move.start >> 3;

				bitboard file_mask = AFILE << move.start;

				// rank differentiator needed
				if (more_than_one(possible & file_mask)) {
					write_char(&buffer, &count, rank + '1');
				} else {
					write_char(&buffer, &count, file + 'a');
				}
			}

			if (capture) {
				write_char(&buffer, &count, 'x');
			}

			int file = move.end & 7;
			int rank = move.end >> 3;

			write_char(&buffer, &count, file + 'a');
			write_char(&buffer, &count, rank + '1');
		}
	}

	if (check_and_mate) {
		struct Position child = make_move(state.pos, move);

		if (enemy_checks(child)) {
			struct MoveList moves = generate_moves(child);

			// checkmate (no moves)
			if (moves.length == 0) {
				write_char(&buffer, &count, '#');
			} else {
				write_char(&buffer, &count, '+');
			}
		}
	}

	return count;
}


size_t generate_uci(struct Move move, struct PositionState state, char *buffer) {
	size_t count = 0;

	bool promotion = get_square(state.pos, move.start) == Pawn
	              && move.piece != Pawn;

	if (state.side_to_move == BLACK) {
		move.start ^= 56;
		move.end ^= 56;
	}

	write_char(&buffer, &count, (move.start & 7)  + 'a');
	write_char(&buffer, &count, (move.start >> 3) + '1');
	write_char(&buffer, &count, (move.end   & 7)  + 'a');
	write_char(&buffer, &count, (move.end   >> 3) + '1');

	if (promotion) {
		write_char(&buffer, &count, "--nbrq--"[move.piece]);
	}

	return count;
}


struct Move parse_san(const char *in, size_t len, struct PositionState state, bool *ok)
{
	for (size_t i = 0; i < len; ++i)
	{
		putchar(in[i]);
	}
	putchar('\n');

	struct Move move = {0};
	*ok = false;

	// filter out check and checkmate (irrelevant)
	if (in[len - 1] == '+') len--;
	if (in[len - 1] == '#') len--;

	unsigned char lower = 0x20;
	unsigned char c = in[0];

	// pawn move
	if (c & lower) {
		square start_file = c - 'a';

		if (start_file >= 8)
			goto error;

		// pawn captures
		if (in[1] == 'x') {
			square end_file = in[2] - 'a';
			square end_rank = in[3] - '1';

			if (end_file >= 8 || end_rank >= 8)
				goto error;

			move.start = 8*(end_rank - 1) + start_file;
			move.end = 8*end_rank + end_file;
			move.piece = (len == 5) ? lookup[in[4]] : Pawn;

			if (state.side_to_move == BLACK)
			{
				move.start ^= 56;
				move.end ^= 56;
			}
		}

		// standard pawn move
		else {
			square end_rank = in[1] - '1';

			if (end_rank >= 8)
				goto error;

			move.end = 8*end_rank + start_file;
			move.start = move.end + S;
			move.piece = (len == 3) ? lookup[in[2]] : Pawn;

			if (state.side_to_move == BLACK)
			{
				move.end ^= 56;
			}

			// double move
			bitboard mask = 1L << move.start;

			if (mask & ~occupied(state.pos)) {
				move.start += S;
			}

			if (state.side_to_move == BLACK)
			{
				move.start ^= 56;
			}
		}
	}

	// other piece
	else {
		enum PieceType piece = lookup[c | lower];

		if (piece == None)
			goto error;

		bitboard pieces = extract(state.pos, piece);
		bitboard occ = occupied(state.pos);

		// only one piece can move to destination
		if (len == 3 || (len == 4 && in[1] == 'x')) {
			if (in[1] == 'x') in++;

			square end_file = in[1] - 'a';
			square end_rank = in[2] - '1';

			if (end_file >= 8 || end_rank >= 8)
				goto error;

			move.end = 8*end_rank + end_file;
			move.piece = piece;

			if (state.side_to_move == BLACK)
			{
				move.end ^= 56;
			}

			pieces &= generic_attacks(piece, move.end, occ);

			if (pieces == 0 || more_than_one(pieces))
				goto error;

			move.start = lsb(pieces);

			if (state.side_to_move == BLACK)
			{
				move.start ^= 56;
			}
		}

		// multiple pieces can move to destination
		else {
			unsigned char c = in[1];
			bitboard mask = 0;

			if ('1' <= c && c <= '8') {
				square rank = c - '1';
				mask = RANK1 << (8*rank);
			}

			else if ('a' <= c && c <= 'h') {
				square file = c - 'a';
				mask = AFILE << file;
			}

			else
				goto error;

			if (len == 5) {
				if (in[2] == 'x') in++;
				else goto error;
			}

			square end_file = in[2] - 'a';
			square end_rank = in[3] - '1';

			move.end = 8*end_rank + end_file;
			move.piece = piece;

			if (state.side_to_move == BLACK)
			{
				move.end ^= 56;
			}

			// TODO: maybe check that disambiguation was not necessary?
			pieces &= generic_attacks(piece, move.end, occ);
			pieces &= mask;

			if (pieces == 0 || more_than_one(pieces))
				goto error;

			move.start = lsb(pieces);

			if (state.side_to_move == BLACK)
			{
				move.start ^= 56;
			}
		}
	}

	*ok = true;

error:
	return move;
}


