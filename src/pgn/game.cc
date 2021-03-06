#include "game.hh"

#include "chess/uchess.h"

#include "fmt/format.h"

#include <stack>

using namespace pgn;

struct Frame
{
	GameNode *node;
	Position position;

	unsigned next_var_idx, state;
	bool entered_variation;

	Frame(GameNode *node, Position position,
		  unsigned next_var_idx = 1,
		  unsigned state = 0,
		  bool entered_variation = false)
		: node(node), position(position),
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

void Visitor::visit_moves(Position start_pos, GameNode *root)
{
	std::stack<Frame> stack;
	stack.emplace(root, start_pos);

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
			if (!accept(top.position, top.node))
				break;

			top.state = 1;
			[[fallthrough]];
		case 1:
			// check for variations (sibling nodes)
			if (parent && top.next_var_idx < parent->size())
			{
				// do move
				GameNode *var_node = parent->next(top.next_var_idx);
				if (begin_variation(top.position, top.node,
									var_node, top.next_var_idx))
				{
					stack.emplace(var_node, top.position, MaxVariationDepth);
					top.entered_variation = true;
				}

				++top.next_var_idx;
			}
			else // no more variations left
			{
				// no more moves left - end of variation
				if (top.node->is_leaf())
				{
					top.state = 3;
				}
				else
				{
					top.position = make_move(top.position, top.node->move);
					stack.emplace(top.node->next(), top.position);
					top.state = 3;
				}
			}
			break;
		case 3:
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
		
		Position pos;
		if (it == game->headers.end())
			pos = startpos();
		else
		{
			bool ok = false;
			pos = parse_fen(it->second.c_str(), it->second.size(), &ok);

			ASSERT(ok);
		}

		visit_moves(pos, game->next());
	}
}

bool GameExporter::accept(const std::string &tag_name, std::string &tag_value)
{
	fmt::print(out, "[{} \"{}\"]\n", tag_name, tag_value);
	return true;
}

bool GameExporter::accept(Position position, GameNode *node)
{
	const char *san = move_to_san(position, node->move);

	if (need_space)
		fputc(' ', out);
	need_space = false;

	if (1/*position.side_to_move() == Colour::Black*/)
	{
		//if (exited_variation)
		//	fputc(' ', out);

		if (need_ellipsis)
			fmt::print(out, "{}... ", 1 + 0/*position.fullmoves()*/);

		fputs(san, out);
	}
	else
		fmt::print(out, "{}. {}", 1 + 0/*position.fullmoves()*/, san);

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

	return true;
}

bool GameExporter::begin_game()
{
	return true;
}

void GameExporter::end_game()
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

bool GameExporter::begin_variation(Position,
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
