#pragma once

#include <fmt/format.h>

#include "chess/uchess.h"
#include "pgn/parser.hh"
#include "db/io.hh"

namespace db
{

enum Protocol
{
	Binary001
};

template <Protocol P>
class Encoder : public pgn::IVisitor
{
private:
	io::buffer_t &buf;

public:
	Encoder(io::buffer_t &buf)
		: pgn::IVisitor(), buf(buf)
	{

	}

	bool accept(const std::string &tag_name, std::string &tag_value) override;
	bool accept(PositionState position, pgn::GameNode *node) override;

	bool begin_headers() override;
	void end_headers() override;

	bool begin_variation(PositionState position,
						 pgn::GameNode *main_line,
						 pgn::GameNode *variation,
						 unsigned variation_idx) override;
	void end_variation() override;

	bool begin_game() override;
	void end_game(pgn::Result) override;

	bool handle_error(size_t stream_pos, std::string_view token,
					  const std::string &message) override;
};

constexpr bool operator==(const Move &lhs, const Move &rhs)
{
	return lhs.start    == rhs.start
		&& lhs.end      == rhs.end
		&& lhs.piece    == rhs.piece
		&& lhs.castling == rhs.castling;
}

template <Protocol P>
bool Encoder<P>::accept(const std::string &tag_name, std::string &tag_value)
{
	io::write_string(buf, tag_name);
	io::write_string(buf, tag_value);

	return true;
}

template <Protocol P>
bool Encoder<P>::accept(PositionState position, pgn::GameNode *node)
{
	auto ml = generate_moves(position.pos);
	size_t i = 0;
	for (; i < ml.length; ++i)
		if (node->move == ml.moves[i])
			break;

	io::write_uint(buf, i, 1);
	io::write_string(buf, node->comment);

	return true;
}

template <Protocol P>
bool Encoder<P>::begin_headers()
{
	return true;
}

template <Protocol P>
void Encoder<P>::end_headers()
{
	io::write_string(buf, "");
}

template <Protocol P>
bool Encoder<P>::begin_variation(PositionState position,
					 pgn::GameNode *main_line,
					 pgn::GameNode *variation,
					 unsigned variation_idx)
{
	return true;
}

template <Protocol P>
void Encoder<P>::end_variation()
{
}

template <Protocol P>
bool Encoder<P>::begin_game()
{
	return true;
}

template <Protocol P>
void Encoder<P>::end_game(pgn::Result)
{

}

template <Protocol P>
bool Encoder<P>::handle_error(size_t stream_pos, std::string_view token,
							  const std::string &message)
{
	fmt::print(stderr, "{:03d} '{}' {}\n", stream_pos, token, message);
	std::abort();

	return false;
}

} // db
