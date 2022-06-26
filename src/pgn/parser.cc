
#include "parser.hh"

#include <charconv>

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

struct TokenStream
{
	std::string_view str;
	std::size_t pos;
};

bool accept(TokenStream &stream, char c)
{
	if (stream.str[stream.pos] == c)
	{
		++stream.pos;
		return true;
	}
	else return false;
}

bool accept(TokenStream &stream, std::string_view chars)
{
	for (char c : chars)
		if (accept(stream, c))
			return true;

	return false;
}

bool eat(TokenStream &stream, std::string_view chars)
{
	bool b = false;
	for (; accept(stream, chars); ) b = true;
	return b;
}

void skip_line(TokenStream &stream)
{
	for (; stream.pos < stream.str.size(); ++stream.pos)
		if (stream.str[stream.pos] == '\n')
		{
			++stream.pos;
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
static constexpr TokenTypeLookup pgn_lookup;

Token next_token(TokenStream &stream)
{
	size_t &pos = stream.pos;
	std::string_view str = stream.str;

	size_t start_pos = pos;
	char c = str[pos];
	TokenType type = pgn_lookup[c];

	switch (type)
	{
	case INTEGER:
	case WHITESPACE:
	case NEWLINE:
	case PERIOD:
		do { c = str[++pos]; } while (pgn_lookup[c] == type && pos < str.size());
		return {type, str.substr(start_pos, pos - start_pos)};
	case SYMBOL:
		do {
			c = str[++pos];
			if (pos >= str.size()) break;
		} while (pgn_lookup[c] == SYMBOL || pgn_lookup[c] == INTEGER);
		return {type, str.substr(start_pos, pos - start_pos)};
	case STRING:
	case COMMENT:
		do {
			c = str[++pos]; // todo: handle escaped quotes
			if (pgn_lookup[c] == type)
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
			do { c = str[++pos]; } while (pgn_lookup[c] == INTEGER && pos < str.size());
		else if (c == '?' || c == '!')
			eat(stream, "?!");

		return {type, str.substr(start_pos, pos - start_pos)};
	default:
		break;
	}

	return {NONE, ""};
}

#define EXPECT(cond, str, msg) if (!((cond))) {      \
	if (visitor->handle_error(stream.pos, str, msg)) \
		return -stream.pos; }


int pgn::parse_game(std::string_view str, IVisitor *visitor)
{
	int pos = parse_headers(str, visitor);

	pos += parse_movetext(str.substr(pos), visitor);

	return pos;
}

#include "fmt/format.h"
int pgn::parse_headers(std::string_view str, IVisitor *visitor)
{
	visitor->begin_headers();

	TokenStream stream {str, 0};
	Token token;

	eat(stream, "\r\n");

	while (accept(stream, '['))
	{
		token = next_token(stream);
		EXPECT(token.type == SYMBOL, token.value, "expected symbol");
		std::string tag_name {token.value};

		token = next_token(stream);
		EXPECT(token.type == WHITESPACE, token.value, "expected whitespace");

		token = next_token(stream);
		EXPECT(token.type == STRING, token.value, "expected string");
		std::string tag_value {token.value};

		EXPECT(accept(stream, ']'), str.substr(stream.pos, 1), "missing closing square bracket");
		EXPECT(eat(stream, "\r\n"), str.substr(stream.pos, 1), "expected newlines");

		visitor->accept(tag_name, tag_value);
	}

	visitor->end_headers();

	return stream.pos;
}

int pgn::parse_movetext(std::string_view movetext, IVisitor *visitor,
						const PositionState startpos)
{
	TokenStream stream {movetext, 0};
	Token token, previous_token;

	pgn::GameNode node {nullptr};

	struct Frame {
		PositionState prev, next;

		Frame(PositionState prev, PositionState next)
			: prev(prev), next(next)
		{
		}
	};

	std::stack<Frame> stack;
	stack.emplace(startpos, startpos);

	visitor->begin_game();

	bool done = false;
	auto result = Result::Unknown;
	
	while ((token = next_token(stream)).type != NONE)
	{
		ASSERT(!stack.empty());
		auto &top = stack.top();

		if (token.type == INTEGER)
		{
			unsigned move_no = 0;
			auto [useless, ec] = std::from_chars(
				token.value.data() + token.value.size(),
				token.value.end(),
				move_no);
			
			EXPECT(!(ec == std::errc()), token.value, "failed to parse move number");

			if (token.value == "1" || token.value == "0")
			{
				EXPECT(stack.size() == 1, token.value, "game terminated with unclosed variation");

				if (accept(stream, '-'))
				{
					if (token.value == "1")
					{
						EXPECT(accept(stream, '0'), token.value, "malformed result token");
						result = Result::WhiteWin;
					}
					else
					{
						EXPECT(accept(stream, '1'), token.value, "malformed result token");
						result = Result::BlackWin;
					}
				}
				else if (accept(stream, '/'))
				{
					EXPECT(
						   accept(stream, '2')
						&& accept(stream, '-')
						&& accept(stream, '1')
						&& accept(stream, '/')
						&& accept(stream, '2'),
						token.value,
						"malformed result token"
					);

					result = Result::Draw;
				}

				break;
			}
			else
			{
				token = next_token(stream);
				EXPECT(token.type == PERIOD, token.value, "expected period after movenumber")
			}
		}
		else if (token.type == SYMBOL)
		{
			if (done)
				visitor->accept(top.prev, &node);
			else
				done = true;

			node = {nullptr};

			bool ok = false;

			std::string s {token.value}; // c_str
			node.move = parse_san(s.c_str(), top.next, &ok, stderr);
			if (!ok)
			{
				//visitor->handle_error(stream.pos, token.value, "failed to parse SAN");

				// skip game
				char fen[256] = {0};
				generate_fen(top.next, fen);
				fmt::print("{}\n", fen);

				while (true)
				{
					token = next_token(stream);
					if (token.type == ASTERISK)
						break;
					
					if (token.value == "1" || token.value == "0")
					{
						if (accept(stream, '-'))
						{
							accept(stream, '0') || accept(stream, '1');
							break;
						}
						else if (accept(stream, '/'))
						{
							   accept(stream, '2')
							&& accept(stream, '-')
							&& accept(stream, '1')
							&& accept(stream, '/')
							&& accept(stream, '2');
							break;
						}
					}
				}

				// todo: try to fix SAN
				// todo: handle error returning bool... ???
				break;
			}

			top.prev = top.next;
			top.next = do_move(top.next, node.move);
		}
		else if (token.type == BRACKET)
		{
			if (token.value == "(")
			{
				visitor->begin_variation(top.prev, nullptr, nullptr, -1);
				stack.emplace(top.prev, top.prev);
			}
			else if (token.value == ")")
			{
				EXPECT(stack.size() >= 2, token.value, "no variation to close");
				stack.pop();
			}
			else
			{
				// todo
				EXPECT(false, token.value, "reserved token");
			}
		}
		else if (token.type == COMMENT)
		{
			node.comment = token.value.substr(1, token.value.size() - 2);
		}
		else if (token.type == NAG)
		{
			if (token.value == "!")
				{}// excellent
			else if (token.value == "?")
				{}// mistake
			else if (token.value == "??")
				{}// blunder
			else if (token.value == "!!")
				{}// brilliant
			else if (token.value == "!?")
				{}// interesting
			else if (token.value == "?!")
				{}// inaccuracy
			else if (token.value[0] == '$')
			{
				auto nag_str = token.value.substr(1);
				unsigned nag = 0;
				auto [useless, ec] = std::from_chars(nag_str.data() + nag_str.size(),
													 nag_str.end(), nag);

				ASSERT(!(ec == std::errc()));
			}

			// todo: add NAG to current node
		}
		else if (token.type == ASTERISK)
		{
			EXPECT(stack.size() == 1, token.value, "game terminated with unclosed variation");
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
					EXPECT(previous_token.type == NEWLINE, token.value,
						   "comment must start at beginning of line");

				skip_line(stream);
			}
		}

		previous_token = token;
	}

	visitor->end_game(result);

	return stream.pos;
}

#undef EXPECT

bool pgn::GameBuilder::handle_error(size_t stream_pos, std::string_view token,
								 	const std::string &message)
{
	if (try_fix_errors && message == "failed to parse SAN")
	{
		// todo
	}

	return false;
}
