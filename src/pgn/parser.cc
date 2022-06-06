#include "parser.hh"

#include "chess/uchess.h"
#include "nags.hh"

#include "fmt/format.h"

#include <charconv> // from_chars
#include <stack>

enum TokenType
{
	NONE,

	NEWLINE,    // \r\n
	WHITESPACE, // \s\t

	INTEGER,    // 0-9
	STRING,     // "..."
	COMMENT,    // {...}
	SYMBOL,     // A-Za-z0-9_+#=:-

	PERIOD,     // .
	ASTERISK,   // *
	BRACKET,    // []()<>
	
	NAG,        // $123 or !!/??/?!/!?/!/?

	MISC,       // ;%
};

struct Token
{
	TokenType type;
	std::string_view value;
};

struct Parser
{
	std::string_view str;
	std::size_t pos;
};

bool accept(Parser &parser, char c)
{
	if (parser.str[parser.pos] == c)
	{
		++parser.pos;
		return true;
	}
	else return false;
}

bool accept(Parser &parser, std::string_view chars)
{
	for (char c : chars)
		if (accept(parser, c))
			return true;

	return false;
}

void eat(Parser &parser, std::string_view chars)
{
	for (; accept(parser, chars); ) {};
}

void skip_line(Parser &parser)
{
	for (; parser.pos < parser.str.size(); ++parser.pos)
		if (parser.str[parser.pos] == '\n')
		{
			++parser.pos;
			break;
		}
}

struct TokenTypeLookup
{
	std::array<TokenType, 0x100> data;

	constexpr TokenTypeLookup()
		: data()
	{
		set(" \t", WHITESPACE);
		set("\r\n", NEWLINE);

		set("abcdefghijklmnopqrstuvwxyz", SYMBOL);
		set("ABCDEFGHIJKLMNOPQRSTUVWXYZ", SYMBOL);
		set("_+#=:-",                     SYMBOL);
		set("0123456789",                 INTEGER);

		set("\"\"", STRING);
		set("{}", COMMENT);
		
		set(".", PERIOD);
		set("*", ASTERISK);

		set("[]()<>", BRACKET);

		set("$?!", NAG);

		set(";%", MISC);
	}

	TokenType &operator[](const char c)
	{
		return data[static_cast<unsigned char>(c)];
	}

	TokenType const &operator[](const char c) const
	{
		return data[static_cast<unsigned char>(c)];
	}

	constexpr void set(std::string_view s, TokenType type)
	{
		for (size_t i = 0; i < s.size(); ++i)
			data[static_cast<unsigned char>(s[i])] = type;
	}
};
static constexpr TokenTypeLookup lookup;

void pgn::init()
{
}

Token next_token(Parser &parser)
{
	size_t &pos = parser.pos;
	std::string_view str = parser.str;

	size_t start_pos = pos;
	char c = str[pos];
	TokenType type = lookup[c];

	//fmt::print("TOKEN '{}' @ {:02d} -> {}\n", c, pos, type);

	switch (type)
	{
	case INTEGER:
	case WHITESPACE:
	case NEWLINE:
	case PERIOD:
		do { c = str[++pos]; } while (lookup[c] == type && pos < str.size());
		return {type, str.substr(start_pos, pos - start_pos)};
	case SYMBOL:
		do {
			c = str[++pos];
			if (pos >= str.size()) break;
		} while (lookup[c] == SYMBOL || lookup[c] == INTEGER);
		return {type, str.substr(start_pos, pos - start_pos)};
	case STRING:
	case COMMENT:
		do {
			c = str[++pos]; // todo: handle escaped quotes
			if (lookup[c] == type)
			{
				++pos;
				break;
			}
		} while (pos < str.size());
		return {type, str.substr(start_pos, pos - start_pos)};
	case ASTERISK:
	case BRACKET:
	case MISC:
		return {type, str.substr(pos++, 1)};
	case NAG:
		if (c == '$')
			do { c = str[++pos]; } while (lookup[c] == INTEGER && pos < str.size());
		else if (c == '?' || c == '!')
			eat(parser, "?!");

		return {type, str.substr(start_pos, pos - start_pos)};
	default:
		break;
	}

	return {NONE, ""};
}

struct Frame
{
	pgn::GameNode *node;
	Position position;
	unsigned move_no;

	Frame(pgn::GameNode *node,
		  const Position &position,
		  unsigned move_no)
		: node(node), position(position), move_no(move_no)
	{
	}
};

int pgn::parse_game(std::string_view str, GameNode *out)
{
	Parser parser {str, 0};
	Token token, previous_token;

	while ((token = next_token(parser)).value == "[")
	{
		token = next_token(parser);
		ASSERT(token.type == SYMBOL);
		std::string tag_name {token.value};

		token = next_token(parser);
		ASSERT(token.type == WHITESPACE);

		token = next_token(parser);
		ASSERT(token.type == STRING);
		std::string tag_value {token.value};

		token = next_token(parser);
		ASSERT(token.value == "]");

		token = next_token(parser);
		ASSERT(token.type == NEWLINE);


		fmt::print("Name: {}, value: {}\n", tag_name, tag_value);
	}

	eat(parser, " \t\n\r");

	std::stack<Frame> stack;
	stack.emplace(out, Position {}, 1);

	while ((token = next_token(parser)).type != NONE)
	{
		Frame &top = stack.top();

		if (token.type == INTEGER)
		{
			std::from_chars(token.value.data(),
							token.value.data() + token.value.size(),
							top.move_no);

			token = next_token(parser);
			ASSERT(token.type == PERIOD);
			/*ASSERT((position.side_to_move() == Colour::White
				 && token.value.size() == 1)  // todo: only after variation
				 || token.value.size() == 3);*/
		}
		else if (token.type == SYMBOL)
		{
			bool ok = false;
			Move move = parse_move_san(token.value.data(), token.value.size(), &top.position, &ok);
			top.node->emplace_back(move, "");
			top.node = top.node->next();
		}
		else if (token.type == BRACKET)
		{
			if (token.value == "(")
			{
				ASSERT(!top.node->is_dangling());

				stack.emplace(top.node->parent(),
							  top.position, top.move_no);
			}
			else if (token.value == ")")
			{
				ASSERT(stack.size() >= 2);
				stack.pop();
			}
			else
			{
				// todo: reserved
				ASSERT(false);
			}
		}
		else if (token.type == COMMENT)
		{
			top.node->comment = token.value;
		}
		else if (token.type == NAG)
		{
			// todo: add NAG to current node
		}
		else if (token.type == ASTERISK)
		{
			break;
		}
		else if (token.type == WHITESPACE)
		{

		}
		else if (token.type == NEWLINE)
		{
			
		}
		else if (token.type == MISC)
		{
			if (token.value == "%" || token.value == ";")
			{
				// should only appear at the start of a line
				if (token.value == "%")
					ASSERT(previous_token.type == NEWLINE);

				skip_line(parser);
			}
		}

		previous_token = token;
	}

	return 0;
}
