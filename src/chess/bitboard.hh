#pragma once

#include <cstdint>

namespace cdb::chess {

enum class PieceType : uint8_t { None, Pawn, Knight, Bishop, Rook, Castle, Queen, King };
enum Square : uint8_t { A1 = 0, C1 = 2, E1 = 4, G1 = 6, H1 = 7, A8 = 56, H8 = 63 };
enum Direction : int8_t { North = 8, East = 1, South = -North, West = -East,
						  NorthEast = North + East, SouthEast = South + East,
						  SouthWest = South + West, NorthWest = North + West,
						  NorthNorth = North + North };

using bitboard = uint64_t;

constexpr bitboard FILE_A = 0x0101010101010101;
constexpr bitboard FILE_H = 0x8080808080808080;
constexpr bitboard RANK_1 = 0x00000000000000ff;
constexpr bitboard RANK_3 = 0x0000000000ff0000;
constexpr bitboard RANK_4 = 0x00000000ff000000;
constexpr bitboard RANK_8 = 0xff00000000000000;

constexpr bitboard file_bb(Square sq) {
	return FILE_A << (sq & 7);
}

constexpr bitboard square_bb(Square sq) {
	return 1ull << sq;
}

bitboard attacks_from(PieceType piece_type, Square sq, bitboard occ = 0);

template <Direction D> constexpr bitboard shift(bitboard bb);
template <> constexpr bitboard shift<North>(bitboard bb) { return bb << 8; }
template <> constexpr bitboard shift<South>(bitboard bb) { return bb >> 8; }
template <> constexpr bitboard shift<East>(bitboard bb) { return (bb & ~FILE_H) << 1; }
template <> constexpr bitboard shift<West>(bitboard bb) { return (bb & ~FILE_A) >> 1; }

template <> constexpr bitboard shift<NorthEast>(bitboard bb) { return (bb & ~FILE_H) << 9; }
template <> constexpr bitboard shift<NorthWest>(bitboard bb) { return (bb & ~FILE_A) << 7; }
template <> constexpr bitboard shift<SouthEast>(bitboard bb) { return (bb & ~FILE_H) >> 7; }
template <> constexpr bitboard shift<SouthWest>(bitboard bb) { return (bb & ~FILE_A) >> 9; }

template <class = void>
constexpr bitboard shiftm(bitboard) { return 0; }

template <Direction D, Direction... Ds>
constexpr bitboard shiftm(bitboard bb) { return shift<D>(bb) | shiftm<Ds...>(bb); }

template <class = void>
constexpr bitboard walk(bitboard bb) { return bb; }

template <Direction D, Direction... Ds>
constexpr bitboard walk(bitboard bb) { return walk<Ds...>(shift<D>(bb)); }

constexpr bool more_than_one(const bitboard bb) {
	return bb & (bb - 1);
}

constexpr bool only_one(const bitboard bb) {
	return bb && !more_than_one(bb);
}

inline bitboard line_between(Square a, Square b) {
	bitboard diag = attacks_from(PieceType::Bishop, a, square_bb(b));
	bitboard orth = attacks_from(PieceType::Rook,   a, square_bb(b));

	bitboard line = 0;
	if (diag & square_bb(b)) line |= attacks_from(PieceType::Bishop, b, square_bb(a)) & diag;
	if (orth & square_bb(b)) line |= attacks_from(PieceType::Rook,   b, square_bb(a)) & orth;

	return line;
}

inline bitboard line_connecting(Square a, Square b) {
	bitboard diag = attacks_from(PieceType::Bishop, a, 0);
	bitboard orth = attacks_from(PieceType::Rook,   a, 0);

	bitboard line = 0;
	if (diag & square_bb(b)) line |= square_bb(b) | (attacks_from(PieceType::Bishop, b, 0) & diag);
	if (orth & square_bb(b)) line |= square_bb(b) | (attacks_from(PieceType::Rook,   b, 0) & orth);

	return line;
}

} // cdb::chess
