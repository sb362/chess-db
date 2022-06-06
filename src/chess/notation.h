#pragma once

#include "movegen.h"
#include "position.h"

struct Move parse_move_san(const char *in, size_t len, struct Position *pos, bool *ok);
struct Position parse_fen(const char *in, size_t len, bool *ok);

const char *position_to_fen(struct Position pos);
const char *move_to_san(struct Position pos, struct Move move);
