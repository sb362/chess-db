#include "bits.h"
#include "movegen.h"
#include "position.h"
#include <stdbool.h>

static inline
void append(struct MoveList *list, struct Move move) {
	list->moves[list->length++] = move;
}

static inline bitboard line_between(bitboard a, bitboard b) {
	square sqa = lsb(a);
	square sqb = lsb(b);

	bitboard diag = bishop_attacks(sqa, b);
	bitboard orth = rook_attacks(sqa, b);

	bitboard line = 0;
	if (diag & b) line |= bishop_attacks(sqb, a) & diag;
	if (orth & b) line |= rook_attacks(sqb, a) & orth;

	return line;
}

static inline
bitboard enemy_attacks(struct Position pos) {
	bitboard pawns   = extract(pos, Pawn)   & ~pos.white;
	bitboard knights = extract(pos, Knight) & ~pos.white;
	bitboard bishops = extract(pos, Bishop) & ~pos.white;
	bitboard rooks   = extract(pos, Rook)   & ~pos.white;
	bitboard queens  = extract(pos, Queen)  & ~pos.white;
	bitboard king    = extract(pos, King)   & ~pos.white;

	bishops |= queens;
	rooks |= queens;

	bitboard attacks = 0;
	bitboard occ = occupied(pos);

	bitboard our_king = extract(pos, King) & pos.white;
	occ &= ~our_king; // allow sliders to move through our king

	attacks |= shift(S,E, pawns);
	attacks |= shift(S,W, pawns);

	while (knights) {
		attacks |= knight_attacks(lsb(knights));
		knights &= knights - 1;
	}

	while (bishops) {
		attacks |= bishop_attacks(lsb(bishops), occ);
		bishops &= bishops - 1;
	}

	while (rooks) {
		attacks |= rook_attacks(lsb(rooks), occ);
		rooks   &= rooks - 1;
	}

	attacks |= king_attacks(lsb(king));
	return attacks;
}

static inline
bitboard enemy_checks(struct Position pos) {
	bitboard king = extract(pos, King) & pos.white;
	square sq = lsb(king);

	bitboard occ = occupied(pos);
	bitboard pawns   = extract(pos, Pawn)   & ~pos.white;
	bitboard knights = extract(pos, Knight) & ~pos.white;
	bitboard bishops = extract(pos, Bishop) & ~pos.white;
	bitboard rooks   = extract(pos, Rook)   & ~pos.white;
	bitboard queens  = extract(pos, Queen)  & ~pos.white;

	bishops |= queens;
	rooks |= queens;

	pawns   &= shift(N,E, king) | shift(N,W, king);
	knights &= knight_attacks(sq);
	bishops &= bishop_attacks(sq, occ);
	rooks   &= rook_attacks(sq, occ);

	return pawns | knights | bishops | rooks;
}

static inline
void generate_partial_moves(struct Position pos, enum PieceType T, bitboard targets, struct MoveList *list) {
	bitboard pieces = extract(pos, T) & pos.white;
	bitboard occ = occupied(pos);

	while (pieces) {
		square sq = lsb(pieces);
		bitboard attacks = generic_attacks(T, sq, occ) & targets;

		while (attacks) {
			square dst = lsb(attacks);
			append(list, (struct Move){ sq, dst, T });
			attacks &= attacks - 1;
		}
		pieces &= pieces - 1;
	}
}

static inline
void generate_king_moves(struct Position pos, struct MoveList *list) {
	square sq = lsb(extract(pos, King) & pos.white);
	bitboard attacked = enemy_attacks(pos);
	bitboard attacks = king_attacks(sq) & ~attacked & ~pos.white;

	while (attacks) {
		square dst = lsb(attacks);
		append(list, (struct Move){ sq, dst, King });
		attacks &= attacks - 1;
	}

	// generate castling moves
	bitboard occ = occupied(pos);

	bitboard info = extract(pos, Info);
	info = pext(info, ~occ);

	bitboard kingside_occ  = 0b01100000;
	bitboard queenside_occ = 0b00001110;
	bitboard kingside_attacked  = 0b01110000;
	bitboard queenside_attacked = 0b00011100;

	enum { C1 = 2, E1 = 4, G1 = 6 };

	if ((info & WK_MASK) && !(occ & kingside_occ) && !(attacked & kingside_attacked)) {
		append(list, (struct Move){ E1, G1, King, 1 });
	}

	if ((info & WQ_MASK) && !(occ & queenside_occ) && !(attacked & queenside_attacked)) {
		append(list, (struct Move){ E1, C1, King, 1 });
	}
}

static inline
void append_pawn_moves(bitboard mask, square shift, bool promotion, struct MoveList *list) {
	while (mask) {
		square dst = lsb(mask);
		square sq = dst - shift;

		if (promotion) {
			append(list, (struct Move){ sq, dst, Knight });
			append(list, (struct Move){ sq, dst, Bishop });
			append(list, (struct Move){ sq, dst, Rook   });
			append(list, (struct Move){ sq, dst, Queen  });
		} else {
			append(list, (struct Move){ sq, dst, Pawn });
		}

		mask &= mask - 1;
	}
}

static inline
void generate_pawn_moves(struct Position pos, bitboard targets, struct MoveList *list) {
	bitboard pawns = extract(pos, Pawn) & pos.white;

	bitboard occ = occupied(pos);
	bitboard them = occ & ~pos.white;

	bitboard info = extract(pos, Info);
	info = pext(info, ~occ);

	bitboard en_passant = (info & EP_MASK) << 40;

	targets |= shift(N, targets) & en_passant;
	them |= en_passant;

	bitboard single_up = shift(N, pawns) & ~occ;
	bitboard double_up = shift(N, single_up & RANK3) & ~occ;

	// mask with targets after to allow double move
	single_up &= targets;
	double_up &= targets;

	bitboard east_captures = shift(N,E, pawns) & them & targets;
	bitboard west_captures = shift(N,W, pawns) & them & targets;

	// promotions
	append_pawn_moves(single_up & RANK8, N, true, list);
	append_pawn_moves(east_captures & RANK8, N+E, true, list);
	append_pawn_moves(west_captures & RANK8, N+W, true, list);

	// non promotions
	append_pawn_moves(double_up, N+N, false, list);
	append_pawn_moves(single_up & ~RANK8, N, false, list);
	append_pawn_moves(east_captures & ~RANK8, N+E, false, list);
	append_pawn_moves(west_captures & ~RANK8, N+W, false, list);
}

static inline
void filter_pinned_moves(struct Position pos, struct MoveList *list) {
	square ksq = lsb(extract(pos, King) & pos.white);
	bitboard candidates = queen_attacks(ksq, pos.white) & pos.white;

	bitboard occ = occupied(pos);
	bitboard bishops = extract(pos, Bishop) & ~pos.white;
	bitboard rooks   = extract(pos, Rook)   & ~pos.white;
	bitboard queens  = extract(pos, Queen)  & ~pos.white;

	bishops |= queens;
	rooks |= queens;

	bitboard info = extract(pos, Info);
	info = pext(info, ~occ);

	// always check en_passant for possible pins
	bitboard en_passant = (info & EP_MASK) << 40;

	// reset length to zero
	size_t length = list->length;
	list->length = 0;

	for (size_t i = 0; i < length; i++) {
		struct Move move = list->moves[i];
		bitboard mask = 1L << move.start;
		bitboard dst = 1L << move.end;

		// remove captured en passant pawn from occupancy
		if (move.piece == Pawn) {
			mask |= shift(S, dst & en_passant);
		}

		if (mask & candidates) {
			bitboard nocc = (occ & ~mask) | dst;
			bitboard check = (bishops & ~dst & bishop_attacks(ksq, nocc))
			               | (rooks & ~dst & rook_attacks(ksq, nocc));

			if (check) continue;
		}

		append(list, move);
	}
}

struct MoveList generate_moves(struct Position pos) {
	struct MoveList list = {.length = 0};

	bitboard checkers = enemy_checks(pos);
	bitboard king = extract(pos, King) & pos.white;

	// if more than 1 check we can only move the king
	if (more_than_one(checkers))
		goto king_moves;

	bitboard targets = ~pos.white;

	if (checkers) {
		targets &= checkers | line_between(king, checkers);
	}

	generate_pawn_moves(pos, targets, &list);
	generate_partial_moves(pos, Knight, targets, &list);
	generate_partial_moves(pos, Bishop, targets, &list);
	generate_partial_moves(pos, Rook,   targets, &list);
	generate_partial_moves(pos, Queen,  targets, &list);
	filter_pinned_moves(pos, &list);

king_moves:
	generate_king_moves(pos, &list);
	return list;
}
