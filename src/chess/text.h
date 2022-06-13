#ifndef TEXT_H_
#define TEXT_H_

#include "movegen.h"
#include "state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// forsyths edwards notation
struct PositionState parse_fen(const char *fen, bool *ok, FILE *stream);
size_t generate_fen(struct PositionState state, char *buffer);

// standard algebraic notation
size_t generate_san(struct Move move, struct PositionState state, char *buffer, bool check_and_mate);
struct Move parse_san(const char *in, size_t len, struct PositionState state, bool *ok);

// universal chess interface notation
size_t generate_uci(struct Move move, struct PositionState state, char *buffer);

#endif //TEXT_H_
