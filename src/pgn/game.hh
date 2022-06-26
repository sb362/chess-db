#pragma once

#include "tree.hh"

#include "chess/uchess.h"

#include <string>
#include <string_view>
#include <unordered_map>

namespace pgn
{

enum class VisitorError
{
	
};

enum class Result
{
	Unknown,
	WhiteWin,
	Draw,
	BlackWin
};

constexpr unsigned MaxVariationDepth = 255;

using Headers = std::unordered_map<std::string, std::string>;

struct GameNode : Node<GameNode>
{
	// The move leading to this node
	Move move;

	// Comment for this move
	std::string comment;

	GameNode(GameNode *parent)
		: Node(parent), move(), comment()
	{
	}

	GameNode(GameNode *parent, Move move, const std::string &comment)
		: Node(parent), move(move), comment(comment)
	{
	}
};

struct Game : GameNode
{
	Headers headers;
	Result result;

	Game()
		: GameNode(nullptr), headers(), result(Result::Unknown)
	{
	}
};

struct IVisitor
{
	virtual ~IVisitor() = default;

	virtual bool accept(const std::string &tag_name, std::string &tag_value) = 0;
	virtual bool accept(PositionState position, GameNode *node) = 0;

	virtual bool begin_headers() = 0;
	virtual void end_headers() = 0;

	virtual bool begin_game() = 0;
	virtual void end_game(Result result) = 0;

	virtual bool begin_variation(PositionState position,
								 GameNode *main_line,
								 GameNode *variation,
								 unsigned variation_idx) = 0;
	virtual void end_variation() = 0;

	virtual bool handle_error(size_t stream_pos, std::string_view token,
							  const std::string &message) = 0;
};

class Visitor : public IVisitor
{
public:
	Visitor() = default;
	~Visitor() = default;

	bool begin_headers() override { return true; };
	void end_headers() override {};

	bool begin_game() override { return true; };
	void end_game(Result) override {};

	bool handle_error(size_t stream_pos, std::string_view token,
					  const std::string &message) override;

	void visit_headers(Headers &headers);
	void visit_moves(PositionState start_pos, GameNode *node);

	void visit_game(Game *game);
};

class GameExporter : public Visitor
{
private:
	FILE *out;
	unsigned line_size;

	bool include_comments;
	unsigned max_var_depth, max_line_size;

	bool need_ellipsis, need_space;

public:
	GameExporter(FILE *out,
				 bool include_comments  = true,
				 unsigned max_var_depth = MaxVariationDepth,
				 unsigned max_line_size = 80)
		: Visitor(), out(out), include_comments(include_comments),
		  max_var_depth(max_var_depth), max_line_size(max_line_size),
		  need_ellipsis(false), need_space(false)
	{
	}

private:
	bool accept(const std::string &tag_name, std::string &tag_value) override;
	bool accept(PositionState position, GameNode *node) override;

	bool begin_headers() override;
	void end_headers() override;

	bool begin_game() override;
	void end_game(Result) override;

	bool begin_variation(PositionState position,
						 GameNode *main_line,
						 GameNode *variation,
						 unsigned variation_idx) override;
	void end_variation() override;
};

} // pgn
