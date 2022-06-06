#pragma once

#include <array>
#include <string_view>

namespace pgn
{

struct NAGInfo
{
	std::string_view meaning;
	std::string_view symbol;
};

constexpr std::array<NAGInfo, 0xff> NAGs =
{{
	{"null",                      ""  },  // 0

	// This is a ...
	{"good move",                 "!" },  // 1
	{"mistake",                   "?" },
	{"brilliant move",            "!!"},
	{"blunder",                   "??"},
	{"interesting move",          "!?"},
	{"dubious move",              "?!"},
	{"forced move",               "□" },  // 7

	{"", ""},
	{"", ""},

	// This position is ...
	{"drawish",                   "=" },  // 10

	{"", ""},
	{"", ""},

	// This position is ...
	{"unclear",                   "∞" },  // 13
	{"slightly better for white", "⩲" },
	{"slightly better for black", "⩱" },
	{"better for white",          "±" },
	{"better for black",          "∓" },
	{"winning for white",         "+-"},
	{"winning for black",         "-+"},  // 19

	{"", ""},
	{"", ""},

	{"zugzwang",                  "⨀"},   // 22
	{"zugzwang",                  "⨀"},   // 23

	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},
	{"", ""},

	{"initiative",                "↑"},   // 36
	{"initiative",                "↑"},   // 37

	{"", ""},
	{"", ""},

	{"attack",                    "→"},   // 40
	{"attack",                    "→"},   // 41

	{"", ""},
	{"", ""},
	{"", ""},

	{"compensation",              "=/∞"}, // 45
	{"compensation",              "=/∞"}  // 46
}};

} // pgn
