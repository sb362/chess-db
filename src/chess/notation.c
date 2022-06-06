#include "bits.h"
#include "position.h"
#include "movegen.h"

static enum PieceType lookup[0x80] = {
	['p'] = Pawn,
	['n'] = Knight,
	['b'] = Bishop,
	['r'] = Rook,
	['q'] = Queen,
	['k'] = King,
};

struct Move parse_move_san(const char *in, size_t len, struct Position *pos, bool *ok)
{
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
		}

		// standard pawn move
		else {
			square end_rank = in[1] - '1';

			if (end_rank >= 8)
				goto error;

			move.end = 8*end_rank + start_file;
			move.start = move.end + S;
			move.piece = (len == 3) ? lookup[in[2]] : Pawn;

			// double move
			bitboard mask = 1L << move.start;

			if (mask & ~occupied(*pos)) {
				move.start += S;
			}
		}
	}

	// other piece
	else {
		enum PieceType piece = lookup[c | lower];

		if (piece == None)
			goto error;

		bitboard pieces = extract(*pos, piece);
		bitboard occ = occupied(*pos);

		// only one piece can move to destination
		if (len == 3 || (len == 4 && in[1] == 'x')) {
			if (in[1] == 'x') in++;

			square end_file = in[1] - 'a';
			square end_rank = in[2] - '1';

			if (end_file >= 8 || end_rank >= 8)
				goto error;

			move.end = 8*end_rank + end_file;
			move.piece = piece;

			pieces &= generic_attacks(piece, move.end, occ);

			if (only_one(pieces))
				goto error;

			move.start = lsb(pieces);
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

			// TODO: maybe check that disambiguation was not necessary?
			pieces &= generic_attacks(piece, move.end, occ);
			pieces &= mask;

			if (only_one(pieces))
				goto error;

			move.start = lsb(pieces);
		}
	}

	*ok = true;

error:
	return move;
}

const char *position_to_fen(struct Position *pos)
{
	return NULL;
}

const char *move_to_san(struct Position *pos, struct Move move)
{
	return NULL;
}
