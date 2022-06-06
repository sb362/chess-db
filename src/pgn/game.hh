#pragma once

#include "tree.hh"

#include "chess/uchess.h"

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace pgn
{

constexpr unsigned MaxVariationDepth = 255;

using Headers = std::unordered_map<std::string, std::string>;

struct GameNode : Node<GameNode>
{
	// The move leading to this node
	Move move;

	// Comment for this move
	std::string comment;

	GameNode(GameNode *parent, Move move, const std::string &comment)
		: Node(parent), move(move), comment(comment)
	{
	}
};

struct Game : GameNode
{
	Headers headers;

	Game()
		: GameNode(nullptr, {}, ""), headers()
	{
	}
};

struct IVisitor
{
	virtual ~IVisitor() = default;

	/**
	 * @brief Called on each tag name/value pair
	 * 
	 * @param tag_name 
	 * @param tag_value 
	 * @return true 
	 * @return false 
	 */
	virtual bool accept(const std::string &tag_name, std::string &tag_value) = 0;

	/**
	 * @brief Called on each node
	 * 
	 * @param position The position before reaching *node*
	 * @param node 
	 * @return true Continue traversing along this branch
	 * @return false Immediately stop visiting this branch
	 */
	virtual bool accept(Position position, GameNode *node) = 0;

	virtual bool begin_headers() = 0;
	virtual void end_headers() = 0;

	virtual bool begin_game() = 0;
	virtual void end_game() = 0;

	/**
	 * @brief Called before the start of each variation for the current node
	 * 
	 * @param position The position before *main_line* and *variation*
	 * @param main_line 
	 * @param variation The node that starts the variation, i.e. the sibling of *main_line*
	 * @param variation_idx Variation index, 1 = first variation
	 * @return true Accept this variation
	 * @return false Skip this variation
	 */
	virtual bool begin_variation(Position position,
								 GameNode *main_line,
								 GameNode *variation,
								 unsigned variation_idx) = 0;

	/**
	 * @brief Called after a variation has been fully visited
	 * 
	 */
	virtual void end_variation() = 0;
};

class Visitor : public IVisitor
{
public:
	Visitor() = default;
	~Visitor() = default;

	bool begin_headers() override { return true; };
	void end_headers() override {};

	bool begin_game() override { return true; };
	void end_game() override {};

	void visit_headers(Headers &headers);
	void visit_moves(Position start_pos, GameNode *node);

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
	bool accept(Position position, GameNode *node) override;

	bool begin_headers() override;
	void end_headers() override;

	bool begin_game() override;
	void end_game() override;

	bool begin_variation(Position position,
						 GameNode *main_line,
						 GameNode *variation,
						 unsigned variation_idx) override;
	void end_variation() override;
};

} // pgn
