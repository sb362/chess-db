#pragma once

#include "chess/pgn.hh"
#include "core/byteio.hh"

#include "common.hh"

namespace cdb::format::u16 {

ParseResult parse_movedata(std::span<const std::byte> data,
                           MoveVisitor auto visitor,
                           ResultVisitor auto result_visitor,
                           chess::Position startpos = chess::Startpos) {

  io::const_buffer buf(data);
  ParseStep step {.next = startpos};

  for (; buf.pos() < buf.size(); ) {
    auto move_u16 = buf.read_le<2>();
    chess::Move move {move_u16};

    step.move = move;
    step.prev = std::exchange(step.next, chess::make_move(step.next, move));
    step.move_no++;
    step.bytes_read = buf.pos();
    visitor(step);

    // end of game
    if (move.src == move.dst) {
      auto result = static_cast<chess::GameResult>(move.piece);
      result_visitor(result);
      break;
    }
  }
}

inline std::size_t skip_movedata(std::span<const std::byte> data) {
  io::const_buffer buf(data);

  for (; buf.pos() < buf.size(); ) {
    chess::Move move {buf.read_le<2>()};

    // end of game
    if (move.src == move.dst)
      break;
  }

  return buf.size();
}

std::size_t parse_tags(std::span<const std::byte> data, TagVisitor auto visitor) {
  io::const_buffer buf(data);

  for (; buf.pos() < buf.size(); ) {
    auto tag_id = buf.read_le<1>();
    if (!tag_id)
      break;

    auto value = buf.read_string();
    visitor(static_cast<TagId>(tag_id), value);
  }

  return buf.pos();
}

void write_tag(io::mutable_buffer &buf, TagId id, std::string_view value) {
  buf.write_le(static_cast<std::uint8_t>(id));
  buf.write_string(value);
}

void write_tag(io::mutable_buffer &buf,
               std::string_view name, std::string_view value) {
  
  write_tag(buf, find_tag_id(name), value);
}

void write_move(io::mutable_buffer &buf, chess::Move move) {
  buf.write_le(static_cast<std::uint16_t>(move.to_u16()), 2);
}

void write_move(io::mutable_buffer &buf, const ParseStep &step) {
  write_move(buf, step.move);
}

}
