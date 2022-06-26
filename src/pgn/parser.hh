#pragma once

#include "pgn/game.hh"

#include <stack>

namespace pgn
{

class GameBuilder : public IVisitor
{
private:
	pgn::Game *root;
	std::stack<pgn::GameNode *> nodes;

	bool try_fix_errors;

public:
	GameBuilder(pgn::Game *root, bool try_fix_errors = false)
		: IVisitor(), root(root), nodes(), try_fix_errors(try_fix_errors)
	{
		nodes.emplace(root);
	}

	bool accept(const std::string &tag_name, std::string &tag_value) override
	{
		root->headers.insert_or_assign(tag_name, tag_value);
		return true;
	}

	bool accept(PositionState position, pgn::GameNode *node) override
	{
		nodes.top()->emplace_back(node->move, node->comment);
		nodes.top() = nodes.top()->next();
		return true;
	}

	bool begin_headers() override
	{
		root->headers.clear();
		return true;
	}

	void end_headers() override
	{
	}

	bool begin_game() override
	{
		root->result = Result::Unknown;
		root->clear();
		return true;
	}

	void end_game(Result result) override
	{
		root->result = result;
	}

	bool begin_variation(PositionState position,
						 GameNode *main_line,
						 GameNode *variation,
						 unsigned variation_idx) override
	{
		return true;
	}

	void end_variation() override
	{
		nodes.pop();
	}

	bool handle_error(size_t stream_pos, std::string_view token,
					  const std::string &message) override;
};

static const PositionState StartposState {Startpos, WHITE, 0, 0};

int parse_game(std::string_view str, IVisitor *visitor);
int parse_headers(std::string_view str, IVisitor *visitor);
int parse_movetext(std::string_view movetext, IVisitor *visitor,
				   const PositionState startpos = {Startpos, WHITE, 0, 0});

} // pgn
