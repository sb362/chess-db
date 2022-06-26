#include "game.hh"

#include "chess/uchess.h"

#include "fmt/format.h"

#include <stack>

using namespace pgn;

bool Visitor::handle_error(size_t stream_pos,
						   std::string_view token,
						   const std::string &message)
{
	fmt::print(stderr, "{:03d} '{}' {}\n", stream_pos, token, message);
	std::abort();

	return false;
}

struct VisitorFrame
{
	GameNode *node;
	PositionState prev, next;

	unsigned next_var_idx, state;
	bool entered_variation;

	VisitorFrame(GameNode *node,
				 PositionState prev,
				 PositionState next,
				 unsigned next_var_idx = 1,
				 unsigned state = 0,
				 bool entered_variation = false)
		: node(node), prev(prev), next(next),
		  next_var_idx(next_var_idx),
		  state(state),
		  entered_variation(entered_variation)
	{
	}
};

void Visitor::visit_headers(Headers &headers)
{
	for (auto &tag : headers)
		if (!accept(tag.first, tag.second))
			break;
}

void Visitor::visit_moves(PositionState start_pos, GameNode *root)
{
	std::stack<VisitorFrame> stack;
	stack.emplace(root, start_pos, start_pos);

	while (!stack.empty())
	{
		auto &top = stack.top();
		GameNode *parent = top.node->parent();

		if (top.entered_variation)
		{
			top.entered_variation = false;
			end_variation();
		}

		switch (top.state)
		{
		case 0:
			if (!accept(top.next, top.node))
				return;

			top.state = 1;
			[[fallthrough]];
		case 1:
			// check for variations (sibling nodes)
			if (parent && top.next_var_idx < parent->size())
			{
				// do move
				GameNode *var_node = parent->next(top.next_var_idx);
				if (begin_variation(top.prev, top.node,
									var_node, top.next_var_idx))
				{
					stack.emplace(var_node, top.prev, top.prev, MaxVariationDepth);
					top.entered_variation = true;
				}

				++top.next_var_idx;
			}
			else // no more variations left
			{
				if (!top.node->is_leaf())
				{
					top.prev = top.next;
					top.next = do_move(top.next, top.node->move);

					stack.emplace(top.node->next(), top.prev, top.next);
				}

				top.state = 2;
			}

			break;
		case 2:
			stack.pop();
			break;
		}
	}
}

void Visitor::visit_game(Game *game)
{
	if (begin_headers())
	{
		visit_headers(game->headers);
		end_headers();
	}

	if (begin_game())
	{
		auto it = game->headers.find("FEN");
		
		PositionState pos;
		if (it == game->headers.end())
			pos = {Startpos, WHITE, 0, 0};
		else
		{
			bool ok = false;
			pos = parse_fen(it->second.c_str(), &ok, stderr);

			ASSERT(ok);
		}

		visit_moves(pos, game->next());
		end_game(game->result);
	}
}

bool GameExporter::accept(const std::string &tag_name, std::string &tag_value)
{
	fmt::print(out, "[{} \"{}\"]\n", tag_name, tag_value);
	return true;
}

bool GameExporter::accept(PositionState position, GameNode *node)
{
	uint16_t s = node->move.start, e = node->move.end, p = node->move.piece, c = node->move.castling;
	
	char fen[0xff] = {0};
	generate_fen(position, fen);
	fmt::print("{}\n", fen);
	fmt::print("{:06b} {:06b} {:03b} {:01b}\n", s, e, p, c);

	char san[10] = {0};
	size_t length = generate_san(node->move, position, san, false);
	(void)length;

	if (need_space)
		fputc(' ', out);
	need_space = false;

	if (position.side_to_move == BLACK)
	{
		if (need_ellipsis)
			fmt::print(out, "{}... ", 1 + fullmove_number(position));

		fputs(san, out);
	}
	else
		fmt::print(out, "{}. {}", 1 + fullmove_number(position), san);

	need_ellipsis = false;

	if (!node->comment.empty())
	{
		fputs(" {", out);
		fputs(node->comment.c_str(), out);
		fputc('}', out);

		need_ellipsis = true;
	}

	if (!node->is_leaf())
		fputc(' ', out);
	else
		need_space = true;

	fputc('\n', out);

	fflush(out);

	return true;
}

bool GameExporter::begin_game()
{
	return true;
}

void GameExporter::end_game(Result result)
{
}

bool GameExporter::begin_headers()
{
	return true;
}

void GameExporter::end_headers()
{
	fputc('\n', out);
}

bool GameExporter::begin_variation(PositionState,
								   GameNode *,
								   GameNode *,
								   unsigned variation_idx)
{
	if (variation_idx <= max_var_depth)
	{
		if (need_space)
			fputc(' ', out);

		fputc('(', out);
		need_ellipsis = true;
		need_space = false;
		return true;
	}

	return false;
}

void GameExporter::end_variation()
{
	fputc(')', out);
	need_ellipsis = true;
	need_space = true;
}
