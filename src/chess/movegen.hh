#pragma once

#include "chess/position.hh"
#include "util/bits.hh"
#include "util/vector.hh"

#include <ostream>
#include <utility>
#include <vector>

namespace cdb::chess {

struct Move {
  constexpr Move() = default;
  constexpr Move(Square src, Square dst, PieceType piece, bool castling)
    : src(src), dst(dst), piece(piece), castling(castling) {}

  Square src, dst;
  PieceType piece;
  bool castling;

  friend std::ostream &operator<<(std::ostream &os, const Move &move) { 
    os << static_cast<char>((std::to_underlying(move.src) % 8) + 'a')
       << static_cast<char>((std::to_underlying(move.src) / 8) + '1')
       << static_cast<char>((std::to_underlying(move.dst) % 8) + 'a')
       << static_cast<char>((std::to_underlying(move.dst) / 8) + '1');
    return os;
  }

  constexpr bool operator==(const Move &b) const noexcept {
    return src == b.src && dst == b.dst && piece == b.piece && castling == b.castling;
  }
};

using MoveList = static_vector<Move, 160>;

namespace detail {

constexpr void append_partial_pawn_moves(MoveList &moves, bitboard mask, Direction shift, bool promotion) {
  using enum PieceType;

  while (mask) {
    auto dst = static_cast<Square>(lsb(mask));
    auto src = static_cast<Square>(dst - static_cast<int>(shift));

    if (promotion) {
      for (auto promotion : {Knight, Bishop, Rook, Queen})
        moves.emplace_back(src, dst, promotion, false);
    } else {
      moves.emplace_back(src, dst, Pawn, false);
    }

    mask &= mask - 1;
  }
}

inline void append_pawn_moves(MoveList &moves, const Position &pos,
                              bitboard targets, bitboard _pinned, Square ksq) {
  using enum PieceType;

  bitboard pawns = pos.extract(Pawn) & pos.white;
  bitboard occ   = pos.occupied();
  bitboard enemy = occ &~ pos.white;

  bitboard en_passant = pos.white &~ occ;
  bitboard candidates = shift<South>(shiftm<East, West>(en_passant)) & pawns;

  // check that en-passant doesn't allow horizontal check (not pinned as two blockers)
  // note: this only happens when the king is on the 5th rank
  if ((ksq >> 3) == 4 && popcount(candidates) == 1)
  {
    bitboard rooks  = pos.extract(Rook)  &~ pos.white;
    bitboard queens = pos.extract(Queen) &~ pos.white;

    candidates |= shift<South>(en_passant);
    rooks      |= queens;

    if (attacks_from(Rook, ksq, (occ | en_passant) &~ candidates) & rooks)
      en_passant = 0;
  }

  // allow en-passant if pawn is giving check
  targets |= en_passant & shift<North>(targets);
  enemy   |= en_passant;

  bitboard pinned = pawns & _pinned;
  pawns &= ~_pinned;

  bitboard single_move = shift<North>(pawns) &~ occ;
  bitboard double_move = shift<North>(single_move & RANK_3) &~ occ;

  bitboard pinned_single_move = shift<North>(pinned) & file_bb(ksq) &~ occ;
  bitboard pinned_double_move = shift<North>(pinned_single_move & RANK_3) &~ occ;

  single_move &= targets;
  double_move &= targets;

  pinned_single_move &= targets;
  pinned_double_move &= targets;

  // pinned orthogonal pawns cannot capture
  pinned &= ~attacks_from(Rook, ksq, 0);

  bitboard east_capture = shift<NorthEast>(pawns) & enemy & targets;
  bitboard west_capture = shift<NorthWest>(pawns) & enemy & targets;

  bitboard pinned_east_capture = shift<NorthEast>(pinned) & enemy & targets;
  bitboard pinned_west_capture = shift<NorthWest>(pinned) & enemy & targets;

  // make sure pinned captures are aligned to king
  pinned_east_capture &= attacks_from(Bishop, ksq, 0);
  pinned_west_capture &= attacks_from(Bishop, ksq, 0);

  single_move  |= pinned_single_move;
  double_move  |= pinned_double_move;
  east_capture |= pinned_east_capture;
  west_capture |= pinned_west_capture;

  append_partial_pawn_moves(moves, single_move  & RANK_8,  North,      true);
  append_partial_pawn_moves(moves, east_capture & RANK_8,  NorthEast,  true);
  append_partial_pawn_moves(moves, west_capture & RANK_8,  NorthWest,  true);

  append_partial_pawn_moves(moves, single_move  &~ RANK_8, North,      false);
  append_partial_pawn_moves(moves, double_move,            NorthNorth, false);
  append_partial_pawn_moves(moves, east_capture &~ RANK_8, NorthEast,  false);
  append_partial_pawn_moves(moves, west_capture &~ RANK_8, NorthWest,  false);
}

inline void append_piece_moves(MoveList &moves, PieceType piece_type, const Position &pos,
                               bitboard targets, bitboard filter, bool pinned, Square ksq) {
  bitboard pieces = pos.extract(piece_type) & pos.white & filter;
  bitboard occ = pos.occupied();

  while (pieces) {
    auto src = static_cast<Square>(lsb(pieces));
    bitboard attacks = attacks_from(piece_type, src, occ) & targets;

    if (pinned)
      attacks &= line_connecting(ksq, src);

    while (attacks) {
      auto dst = static_cast<Square>(lsb(attacks));
      moves.emplace_back(src, dst, piece_type, false);
      attacks &= (attacks - 1);
    }

    pieces &= (pieces - 1);
  }
}

inline void append_king_moves(MoveList &moves, const Position &pos, bitboard attacked, Square ksq) {
  using enum PieceType;

  bitboard occ = pos.occupied();
  bitboard attacks = attacks_from(King, ksq) &~ attacked &~ (pos.white & occ);

  while (attacks) {
    auto dst = static_cast<Square>(lsb(attacks));
    moves.emplace_back(ksq, dst, King, false);
    attacks &= (attacks - 1);
  }
  
  bitboard castle = pos.extract(PieceType::Castle) & RANK_1;
  constexpr bitboard qside_occ = 14, qside_attk = 28, kside_occ = 96, kside_attk = 112;
  
  if ((castle & square_bb(A1)) && !(occ & qside_occ) && !(attacked & qside_attk))
    moves.emplace_back(E1, C1, King, true);

  if ((castle & square_bb(H1)) && !(occ & kside_occ) && !(attacked & kside_attk))
    moves.emplace_back(E1, G1, King, true);
}

} // detail

inline bitboard enemy_attacks(const Position &pos, bitboard &checkers) {
  using enum PieceType;

  bitboard pawns   = pos.extract(Pawn)   &~ pos.white;
  bitboard knights = pos.extract(Knight) &~ pos.white;
  bitboard bishops = pos.extract(Bishop) &~ pos.white;
  bitboard rooks   = pos.extract(Rook)   &~ pos.white;
  bitboard queens  = pos.extract(Queen)  &~ pos.white;
  bitboard king    = pos.extract(King)   &~ pos.white;

  bishops |= queens;
  rooks   |= queens;

  bitboard our_king = pos.extract(King) & pos.white;
  bitboard occ = pos.occupied() &~ our_king;

  bitboard attacked = 0;
  checkers = 0;

  attacked |= shift<South>(shiftm<West, East>(pawns));
  attacked |= attacks_from(King, static_cast<Square>(lsb(king)));

  checkers |= pawns & shift<North>(shiftm<West, East>(our_king));
  checkers |= knights & attacks_from(Knight, static_cast<Square>(lsb(our_king)));

  while (knights) {
    attacked |= attacks_from(Knight, static_cast<Square>(lsb(knights)));
    knights  &= knights - 1;
  }

  while (bishops) {
    bitboard attacks = attacks_from(Bishop, static_cast<Square>(lsb(bishops)), occ);
    checkers |= attacks & our_king ? bishops &- bishops : 0;
    attacked |= attacks;
    bishops  &= bishops - 1;
  }

  while (rooks) {
    bitboard attacks = attacks_from(Rook, static_cast<Square>(lsb(rooks)), occ);
    checkers |= attacks & our_king ? rooks &- rooks : 0;
    attacked |= attacks;
    rooks    &= rooks - 1;
  }

  return attacked;
}

inline bitboard pinned_pieces(const Position &pos, Square ksq)
{
  using enum PieceType;

  bitboard occ     = pos.occupied();
  bitboard bishops = pos.extract(Bishop) &~ pos.white;
  bitboard rooks   = pos.extract(Rook)   &~ pos.white;
  bitboard queens  = pos.extract(Queen)  &~ pos.white;

  bishops |= queens;
  rooks   |= queens;

  bishops &= attacks_from(Bishop, ksq, bishops);
  rooks   &= attacks_from(Rook, ksq, rooks);

  bitboard pinned = 0;
  bitboard candidates = bishops | rooks;

  while (candidates) {
    bitboard line = line_between(ksq, static_cast<Square>(lsb(candidates))) & occ;

    if (popcount(line) == 1)
      pinned |= line;

    candidates &= candidates - 1;
  }

  return pinned;
}

inline MoveList movegen(const Position &pos, bitboard &checkers, bitboard &pinned) {
  using namespace detail;
  using enum PieceType;

  MoveList moves;

  auto ksq = static_cast<Square>(lsb(pos.extract(King) & pos.white));

  pinned = pinned_pieces(pos, ksq);
  bitboard attacked = enemy_attacks(pos, checkers);
  bitboard targets  = ~(pos.occupied() & pos.white);

  // if in check from more than one piece, can only move king,
  // otherwise we must block the check, or capture the checking piece
  if (checkers)
    targets &= (popcount(checkers) == 1)
      ? checkers | line_between(ksq, static_cast<Square>(lsb(checkers))) : 0;

  // pinned knights can never move
  append_piece_moves(moves, Bishop, pos, targets, pinned, true, ksq);
  append_piece_moves(moves, Rook,   pos, targets, pinned, true, ksq);
  append_piece_moves(moves, Queen,  pos, targets, pinned, true, ksq);

  append_pawn_moves(moves, pos, targets, pinned, ksq);
  append_piece_moves(moves, Knight, pos, targets, ~pinned, false, ksq);
  append_piece_moves(moves, Bishop, pos, targets, ~pinned, false, ksq);
  append_piece_moves(moves, Rook,   pos, targets, ~pinned, false, ksq);
  append_piece_moves(moves, Queen,  pos, targets, ~pinned, false, ksq);
  append_king_moves(moves, pos, attacked, ksq);

  return moves;
}

inline MoveList movegen(const Position &pos) {
  bitboard checkers = 0, pinned = 0;
  return movegen(pos, checkers, pinned);
}

#ifndef CHESS_DEBUG_POS
constexpr
#else
inline
#endif
Position make_move(Position pos, Move move) {
  using enum PieceType;

#ifdef CHESS_DEBUG_POS
  bool white_to_move = pos.fen.contains('w');
#endif

  bitboard clear  = square_bb(move.src) | square_bb(move.dst);

  bitboard occ = pos.occupied();
  bitboard en_passant = pos.white &~ occ;

  if (move.piece == Pawn)
    clear |= shift<South>(en_passant & clear);

  if (move.castling)
    clear |= (move.dst < move.src) ? square_bb(A1) : square_bb(H1);

  pos.x     &= ~clear;
  pos.y     &= ~clear;
  pos.z     &= ~clear;
  pos.white &= ~clear;

  pos.set(move.dst, move.piece);
  pos.white |= square_bb(move.dst);

  if (move.castling) {
    auto mid = static_cast<Square>((move.dst + move.src) >> 1);
    pos.set(mid, Rook);
    pos.white |= square_bb(mid);
  }

  if (move.piece == King)
    pos.x ^= pos.extract(Castle) & RANK_1; // remove castling rights

  bitboard black = pos.occupied() &~ pos.white;

  // update en-passant
  if (move.piece == Pawn && move.dst - move.src == NorthNorth)
    black |= 256ULL << move.src;

  pos.x     = byteswap(pos.x);
  pos.y     = byteswap(pos.y);
  pos.z     = byteswap(pos.z);
  pos.white = byteswap(black);

#ifdef CHESS_DEBUG_POS
  pos.fen = pos.to_fen(!white_to_move);
#endif

  return pos;
}

std::uint64_t perft(const Position pos, unsigned depth);

} // cdb::chess
