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

// standard algebraic notation and uci notation

struct Move parse_san(const char *san, struct PositionState state, bool *ok, FILE *stream);
struct Move parse_uci(const char *uci, struct PositionState state, bool *ok, FILE *stream);

size_t generate_san(struct Move move, struct PositionState state, char *buffer, bool check_and_mate);
size_t generate_uci(struct Move move, struct PositionState state, char *buffer);

#endif //TEXT_H_
